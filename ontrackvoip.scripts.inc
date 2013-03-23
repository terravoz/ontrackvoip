<?php

/*
 * Tropo doesn't support special UTF chars in text messages.
According to int standards text messages with unicode chars should be limited to 70chars per message.
Messages that are broken into multiple segments, might not arrive in proper sequence.*/
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
        array('@date' => format_date(time(), 'custom', 'D, m/d/Y - H:i:s')));
      $script->addLog($log_msg, 'MDT');

      // below was the only way to avoid problems with special characters in the text message
      $options['text'] = '%inbound_text_contents';
      $options['caller_number'] = '%caller_number';
      $options['dest_number'] = '%dest_number';
      $options['call_id'] = '%cid';
      $options['source'] = '%source';
      $script->addGosub('ontrackvoip_sms_process_request_script', $options);
      break;
    case 'ontrackvoip_sms_process_request_script':
      //Helper script to process SMS request
      $script = new VoipScript('ontrackvoip_sms_process_request_script');

      $result = _ontrackvoip_sms_process_request($options['text'], $options['caller_number'], $options['dest_number'], $options['call_id']);
      $result = voipcall_special_chars($result);
      if(strlen($result) == mb_strlen($result)) {
        // send response in chunks of 160 characters
        $char_limit = 160;
      }
      else {
        //This is unicode SMS, can only contain 70 chars
        $char_limit = 70;
      }
      // send response in chunks of $char_limit characters
      if (strlen($result) <= $char_limit) {
        $text_array[] = $result;
      }
      else {
        /// lets use 154 characters and keep room for message number like (1/10),
        /// we can have upto 9 parts of the message (9/9)
        $limit = $char_limit - 6;
        $tmp = wordwrap($result, $limit, '\n');
        $text_array = explode('\n', $tmp);
      }
      $num_of_msg = count($text_array);

      foreach ($text_array as $index => $text) {
        if ($num_of_msg > 1) {
          $msg_number = ($index + 1);
          $text = "(".$msg_number."/".$num_of_msg.") ".$text;
        }
        $script->addSendText($text);
        if ($options['source'] == 'twilio') {
          //Only Twilio supports Wait() cmd in text channel
          $script->addWait(2);
        }

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
  watchdog('ontrackvoip', "In _ontrackvoip_sms_process_request with $msg, $caller_number, $dest_number");
  global $language;
  $ontrack_phone = OnTrackPhone::load($dest_number);
  if (!$ontrack_phone) {
    //@todo: error message when ontrack phone not recognized
    ontrackvoip_log("Destination number $dest_number not recognized.");
    return t("Destination number !dest_number not recognized.", array(
      '!dest_number' => $dest_number,
    ));
  }

  //Get either a default phone number language or default site language.
  $default_langcode = !empty($ontrack_phone) ? $ontrack_phone->getLanguage() : $language->language;
  $langcode = $default_langcode;
  //watchdog('ontrackvoip', "Default language $default_langcode");
  if ($ontrack_phone->getSyntax() == 'retrieve') {
    $help_msg = t("To create a new support request, please text the problem description. To get the status of an existing request, please text the number of the request id, ie. 123, 657, 1298, etc.",
      array(),
      array(
        'langcode' => $default_langcode
      )
    );
  }
  else {
    $help_msg = t("To create a new support request, please text the site id followed by the actual problem description. To get the status of an existing request, please text the number of the request id, ie. 123, 657, 1298, etc.",
      array(),
      array(
        'langcode' => $default_langcode
      )
    );
  }

  $msg = trim($msg);
  if ($msg == '') {
    $ret = t("Empty request.", array(), array('langcode' => $default_langcode)) . ' ' . $help_msg;
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

          $ontrack_site = new OnTrackSite($site_id);
          $site_langcode = $ontrack_site->getSiteLanguage();
          //Use site language, if not set use phone number languaage
          $langcode = !empty($site_langcode) ? $site_langcode : $default_langcode;
          //watchdog('ontrackvoip', "Using site language $langcode");
          $site_name = $ontrack_site->getSiteName();
        }

        if(!$ticket) {
          $ret = t("Invalid request id !ticket_id. Please try again with a different one.",
            array(
              '!ticket_id' => $ticket_id,
            ),
            array(
              'langcode' => $default_langcode,
            )
        );
          //log the error
          ontrackvoip_log($ticket_id . " is not a valid ticket. Call ID: ".$call_id);
        }
        else if($ticket->uid == $author_uid || ontrackvoip_is_ticket_manager($author)) {
          //the following users might have permission to check a given ticket:
          //a) the user calling from the phone number that created the ticket
          //b) users with the "Ticket Manager" role
          $ret = t("Request !ticket_id status: !ticket_state.",
            array(
              '!ticket_id' => $ticket_id,
              '!ticket_state' => ontrack_get_ticket_status_description_from_code($ticket->state),
            ),
            array(
              'langcode' => $langcode,
            ));
        }
        else {
          //This user is not authorized to access the ticket
          $ret = t("!site_name. You are not authorized to access request !ticket_id. For additional information, please contact the site administrator.",
            array(
              '!ticket_id' => $ticket_id,
              '!site_name' => $site_name,
            ),
            array(
              'langcode' => $langcode
            )
          );
          //log the error
          ontrackvoip_log("Unauthorized access attempted by ".$caller_number . " for request " . $ticket_id.". Call ID: ".$call_id);
        }
      }
      else{
        //Else this is CREATE request
        //Get array of valid site_ids for this SMS number
        $valid_site_ids = $ontrack_phone->getSiteIds();
        if (count($valid_site_ids) == 1) {
          //There is just one matching site for this SMS number, check
          //if its using retrieve SMS syntax
          $site_id = reset($valid_site_ids);
          $ontrack_site = new OnTrackSite($site_id);
          if ($ontrack_phone->getSyntax() == 'retrieve') {
            //This SMS number is set to retrieve id from dest_number so create ticket
            $ticket_text = $msg;
            $site_langcode = $ontrack_site->getSiteLanguage();
            //Use site language, if not set use phone number languaage
            $langcode = !empty($site_langcode) ? $site_langcode : $default_langcode;
            $ret = ontrackvoip_create_ticket($ontrack_site, $ticket_text, $caller_number, $langcode);
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
            $ontrack_site = new OnTrackSite($site_id);
            //Create new ticket
            unset($request[0]); //remove site_id from ticket_text
            $site_langcode = $ontrack_site->getSiteLanguage();
            //Use site language, if not set use phone number languaage
            $langcode = !empty($site_langcode) ? $site_langcode : $default_langcode;
            $ticket_text = implode(" ", $request);
            $ret = ontrackvoip_create_ticket($ontrack_site, $ticket_text, $caller_number, $langcode);
          }
          else {
            //Invalid site id
            //@todo: replace XXX with contact number
            $ret = t("!site_id is not a valid site identifier. Please contact the site administrator for additional information.",
              array(
                '!site_id' => $site_id,
              ),
              array(
                'langcode' => $default_langcode,
              )
            );

            //log the error
            ontrackvoip_log($site_id . " is not a valid site. Call ID: ".$call_id);
          }
        }
        else {
          //Missing site id
          if(!$ret) {
            $ret = t("Missing site id.", array(), array('langcode' => $default_langcode)) . ' ' . $help_msg;
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
function ontrackvoip_create_ticket($ontrack_site, $ticket_text, $caller_number, $langcode) {
  //watchdog('ontrackvoip', "Using site language $langcode");
  $site_name = $ontrack_site->getSiteName();
  $site_id = $ontrack_site->getSiteId();

  //Create new ticket
  $ticket_id = ontrack_create_ticket_from_sms($site_id, $ticket_text, $caller_number);
  $ret = t("!site_name. New request created with id !ticket_id. Please use that id in further inquiries about this ticket.",
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
