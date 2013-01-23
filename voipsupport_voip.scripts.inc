<?php

/**
 * Implementation of hook_voipscript_get_script_names()
 */
function voipsupport_voipscript_get_script_names() {
  //$script_names[] = 'voipsupport_main_menu_script';
  $script_names[] = 'voipsupport_sms_handler_script';
  return $script_names;
}

/**
 * Implementation of hook_voipscript_load_script()
 */
function voipsupport_voipscript_load_script($script_name, $options = NULL) {

  $script = NULL;
  switch ($script_name) {
    default:
      break;

    case 'voipsupport_sms_handler_script':
      //VoIP Support script for Text channel
      $script = new VoipScript('voipsupport_sms_handler_script');
      // log the sms
      $log_msg = t("SMS from %caller_number on @date (id: %call_id)",
        array('@date' => format_date(time())));
      $script->addLog($log_msg, 'MDT');

      // below was the only way to avoid problems with special characters in the text message
      $options['text'] = '%inbound_text_contents';
      $options['caller_number'] = '%caller_number';
      $options['dest_number'] = '%dest_number';
      $options['language'] = 'en';
      $script->addGosub('voipsupport_sms_process_request_script', $options);
      break;

    case 'voipsupport_sms_process_request_script':
      //Helper script to process SMS request
      $script = new VoipScript('voipsupport_sms_process_request_script');
      $result = _voipsupport_sms_process_request($options['text'], $options['caller_number'], $options['dest_number']);
      // send response in chunks of 160 characteres
      if (strlen($result) <= 160) {
        $text_array[] = $result;
      }
      else {
        $tmp = wordwrap($result, 160, '\n');
        $text_array = explode('\n', $tmp);
      }
      foreach ($text_array as $text) {
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
function _voipsupport_sms_process_request($msg, $caller_number, $dest_number) {
  //watchdog('voipsupport', "In _voipsupport_sms_process_request with $msg, $caller_number, $dest_number");
  $help_msg = t("To create new Support Ticket, please text the actual ticket problem description. "
  ."To get a Support Ticket, please text number of the ticket id ex: 123 or 1 2 3.");
  $msg = trim($msg);
  if (!$msg) {
    $ret = t("Empty request.") . ' ' . $help_msg;
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
        $ticket = ontrack_get_ticket($ticket_id);

        if(!$ticket) {
          $ret = t("Ticket !ticket_id doesn't exists.",
            array('!ticket_id' => $ticket_id));
        }
        else if($ticket->uid == $author_uid || user_access("ontrack access all tickets", $author)) {
          //the following users might have permission to check a given ticket:
          //a) the user calling from the phone number that created the ticket
          //b) users with the "ontrack access all tickets" permission

          //@todo: state is numeric field, must be translated into string
          $ret = t("Ticket !ticket_id status is !ticket_state",
          array('!ticket_id' => $ticket_id, '!ticket_state' => _support_state($ticket->state)));

        }
        else {
          $ret = t("You don't have permission to view ticket !ticket_id",
          array('!ticket_id' => $ticket_id));
        }
      }
      else{
        //Else this is CREATE request
        $request = explode(" ", $msg);
        $site_id = $request[0]; //First word in SMS should be site id

        if (is_numeric($site_id)) {
          //Get array of site objects
          $sites = ontrack_get_site_list();
          //Check if site_id exists
          if (isset($sites[$site_id])) {
            //Create new ticket
            unset($request[0]); //remove site_id from ticket_text
            $ticket_text = implode(" ", $request);
            $ticket_id = ontrack_create_ticket_from_sms($site_id, $ticket_text, $caller_number);
            $ret = t("OnTrack La Paz. New ticket created with id !ticket_id. "
              ."Please use that id in further inquiries about this ticket.",
              array('!ticket_id' => $ticket_id));
          }
          else {
            //Invalid site id
            //@todo: replace XXX with contact number
            $ret = t("OnTrack La Paz. !site_id is not a valid OnTrack site"
            ." identifier. Please contact XXX for additional information",
            array('!site_id' => $site_id));
          }
        }
        else {
          //Missing site id
          //@todo: check if its site specific phone number, then we determine site_id from destination number
          $ret = t("Missing site id.") . ' ' . $help_msg;
        }
      }
    }
  }
  return $ret;
}

/*Create voipsupport ticket*/
function ontrack_create_ticket_from_sms($site_id, $ticket_text, $caller_number) {
  watchdog('voipsupport', "New ticket for site id: $site_id from number: $caller_number text: $ticket_text");
  $ticket_id = rand(1,1000);
  return $ticket_id;
}

function ontrack_get_ticket($ticket_id) {
  $ticket->title = 'title';
  $ticket->body[LANGUAGE_NONE][] = array (
    'value' => 'Hello world ticket.',
    'format' => filter_default_format($account),
  );

  $ticket->log = 'Support ticket created from SMS';

  $ticket->uid = 1; // Set uid again because node_object_prepare likes to mess with it.

  $ticket->message_id = time();

  $ticket->state = 1;

  $ticket->priority = 'default';

  $ticket->client = rand(1,1000);

  $ticket->assigned = 'test asignee';

  $ticket->notification = FALSE;

  $ticket->support_email = FALSE;

  $ticket->worldbank_ontrack_act_sup_phone[LANGUAGE_NONE][] = array (
    'value' => '123456789012',
  );
  return $ticket;
}

function ontrack_get_uid_from_phone($caller_number) {
  return 1;
}

function ontrack_get_site_id_from_phone($dest_number) {
  $site_id = rand(1,100);
  return $site_id;
}

function ontrack_get_site_list() {
  for($i=1;$i<=100;$i++) {
    $sites[$i]->name = 'Site '.$i;
    $sites[$i]->id = $i;
  }

  return $sites;
}
