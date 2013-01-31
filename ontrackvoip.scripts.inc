<?php

/**
 * Implementation of hook_voipscript_get_script_names()
 */
function ontrackvoip_voipscript_get_script_names() {
  $script_names[] = 'ontrackvoip_sms_handler_script';
  return $script_names;
}

/**
 * Implementation of hook_voipscript_load_script()
 */
function ontrackvoip_voipscript_load_script($script_name, $options = NULL) {

  $script = NULL;
  switch ($script_name) {
    default:
      break;

    case 'ontrackvoip_sms_handler_script':
      //OnTrack VoIP script for Text channel
      $script = new VoipScript('ontrackvoip_sms_handler_script');
      // log the sms
      $log_msg = t("SMS from %caller_number on @date (id: %call_id)",
        array('@date' => format_date(time())));
      $script->addLog($log_msg, 'MDT');

      // below was the only way to avoid problems with special characters in the text message
      $options['text'] = '%inbound_text_contents';
      $options['caller_number'] = '%caller_number';
      $options['dest_number'] = '%dest_number';
      $options['call_id'] = '%cid';
      $options['language'] = 'en';
      $script->addGosub('ontrackvoip_sms_process_request_script', $options);
      break;

    case 'ontrackvoip_sms_process_request_script':
      //Helper script to process SMS request
      $script = new VoipScript('ontrackvoip_sms_process_request_script');
      $result = _ontrackvoip_sms_process_request($options['text'], $options['caller_number'], $options['dest_number'], $options['call_id']);
      // send response in chunks of 160 characteres
      if (strlen($result) <= 160) {
        $text_array[] = $result;
      }
      else {
        /// lets use 154 characters and keep room for message number like (1/10),
        /// we can have upto 9 parts of the message (9/9)
        $tmp = wordwrap($result, 154, '\n');
        $text_array = explode('\n', $tmp);
      }
      $num_of_msg = count($text_array);
      foreach ($text_array as $index => $text) {
        if ($num_of_msg > 1) {
          $msg_number = ($index + 1);
          $text = "(".$msg_number."/".$num_of_msg.") ".$text;
        }
        $script->addSendText($text);
        $script->addLog("sending $text");
      }
      $script->addHangup();
      break;
  }

  return $script;
}

/**
 * Process incoming SMS messages
 */
function _ontrackvoip_sms_process_request($msg, $caller_number, $dest_number, $call_id) {
  //watchdog('ontrackvoip', "In _ontrackvoip_sms_process_request with $msg, $caller_number, $dest_number");
  //@todo: remove when we have working languages UI
  $langcode = variable_get('ontrack_voip_language', 'es');

  $help_msg = t("To create a new support ticket, please text us the actual "
      . "request description. To check the status of an existing ticket, "
      . "please text us the ticket id number (ex: 123, 657, 1298, etc).",
    array(),
    array(
      'langcode' => $langcode
    )
  );
  $msg = trim($msg);
  if ($msg == '') {
    $ret = t("Empty request.", array(), array('langcode' => $langcode)) . ' ' . $help_msg;

    //log the error
    ontrackvoip_log("Empty message received. Call ID: ".$call_id);
  }
  else {
    $request = explode(" ", $msg);
    $cmd = strtoupper($request[0]);

    if ($cmd == '?') {
      $ret = $help_msg;
    }
    else {
      $msg_temp = trim($msg);
      $msg_temp = str_replace(" ", "", $msg_temp);
      if (is_numeric($msg_temp)) {
        //If message is numeric then this is GET request
        $ticket_id = $msg_temp;
        $author_uid = ontrack_get_uid_from_phone($caller_number);
        $author = user_load($author_uid);
        $ticket_data = ontrack_get_ticket($ticket_id);
        if($ticket_data) {
          $site_id = $ticket_data['site_node']->nid;
          $ticket = $ticket_data['ticket_node'];
          $ontrack_site = OnTrackSite::getSite($site_id, $dest_number);
          if ($ontrack_site) {
            $langcode = $ontrack_site->getLanguage();
            $site_name = $ontrack_site->getSiteName();
          }
        }

        if(!$ticket_data) {
          $ret = t("Invalid ticket id !ticket_id. Please try again with a different one.",
            array(
              '!ticket_id' => $ticket_id
            ),
            array(
              'langcode' => $langcode
            )
        );
          //log the error
          ontrackvoip_log($ticket_id . " is not a valid ticket. Call ID: ".$call_id);
        }
        else if($ticket->uid == $author_uid || ontrackvoip_is_ticket_manager($author)) {
          //the following users might have permission to check a given ticket:
          //a) the user calling from the phone number that created the ticket
          //b) users with the "Ticket Manager" role
          $ret = t("Ticket !ticket_id status: !ticket_state.",
            array(
              '!ticket_id' => $ticket_id,
              '!ticket_state' => ontrack_get_ticket_status_description_from_code($ticket->state)
            ),
            array(
              'langcode' => $langcode
            ));
        }
        else {
          //This user is not authorized to access the ticket
          $ret = t("!site_name. You are not authorized to access ticket !ticket_id. For additional information, please contact XXX.",
            array(
              '!ticket_id' => $ticket_id,
              '!site_name' => $site_name,
            ),
            array(
              'langcode' => $langcode
            )
          );
          //log the error
          ontrackvoip_log("Unauthorized access attempted by ".$caller_number . " for ticket " . $ticket_id.". Call ID: ".$call_id);
        }
      }
      else{
        //Else this is CREATE request
        //Get array of valid site_ids for this SMS number
        $valid_site_ids = OnTrackSite::getSiteIds($dest_number);
        if (count($valid_site_ids) == 1) {
          //There is just one matching site for this SMS number, check
          //if its using retrieve SMS syntax
          $site_id = reset($valid_site_ids);
          $ontrack_site = OnTrackSite::getSite($site_id, $dest_number);
          if ($ontrack_site->getSyntax() == 'retrieve') {
            //This SMS number is set to retrieve id from dest_number so create ticket
            $ticket_text = $msg;
            $ret = ontrackvoip_create_ticket($ontrack_site, $ticket_text, $caller_number);
            return $ret;
          }
        }
        #Else
        //Check if this SMS has embedded site id in its body
        $request = explode(" ", $msg);
        $site_id = $request[0];
        if (is_numeric($site_id)) {
          //SMS syntax: embedded site id
          //Check if site_id is valid
          if (isset($valid_site_ids[$site_id])) {
            //Load site object for further info like language, site name,...
            $ontrack_site = OnTrackSite::getSite($site_id, $dest_number);
            //Create new ticket
            unset($request[0]); //remove site_id from ticket_text
            $ticket_text = implode(" ", $request);
            $ret = ontrackvoip_create_ticket($ontrack_site, $ticket_text, $caller_number);
          }
          else {
            //Invalid site id
            //@todo: replace XXX with contact number
            $ret = t("!site_id is not a valid site"
                ." identifier. Please contact XXX for additional information.",
              array(
                '!site_id' => $site_id
              ),
              array(
                'langcode' => $langcode
              )
            );

            //log the error
            ontrackvoip_log($site_id . " is not a valid site. Call ID: ".$call_id);
          }
        }
        else {
          //Missing site id
          if(!$ret) {
            $ret = t("Missing site id.", array(), array('langcode' => $langcode)) . ' ' . $help_msg;
            //log the error
            ontrackvoip_log("Message with invalid request received. Message content: ".print_r($msg, true)." Call ID: ".$call_id);
          }
        }
      }
    }
  }
  return $ret;
}

/*Helper function to create ticket and return appropriate response*/
function ontrackvoip_create_ticket($ontrack_site, $ticket_text, $caller_number) {
  $langcode = $ontrack_site->getLanguage();
  $site_name = $ontrack_site->getSiteName();
  $site_id = $ontrack_site->getSiteId();
  //watchdog('ontrackvoip', "Using language $langcode");

  //Create new ticket
  $ticket_id = ontrack_create_ticket_from_sms($site_id, $ticket_text, $caller_number);
  $ret = t("!site_name. New ticket created with id !ticket_id. "
      ."Please use that id in further inquiries about this ticket.",
    array(
      '!ticket_id' => $ticket_id,
      '!site_name' => $site_name
    ),
    array(
      'langcode' => $langcode
    )
  );

  return $ret;
}
