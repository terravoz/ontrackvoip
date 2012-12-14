<?php
// $Id$
/**
 * @file
 * Audio Blog VoIP Scripts and helper functions
 */

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
  $help_msg = t("To create a new Support Ticket, please text CREATE followed by space and the actual ticket text.
  To get Support Ticket, please text GET followed by space and ticket id.");
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
    elseif ($cmd == 'CREATE') {
      unset($request[0]);
      $ticket_text = implode(" ", $request);
      //Load OnTrackSite from destination phone number
      $site = OnTrackSite::getSiteFromPhoneNumber($dest_number);
      $site_id = $site->getSiteId();
      $ret = voipsupport_create_ticket_from_sms($site_id, $ticket_text, $caller_number);
    }
    elseif ($cmd == 'GET') {
      $ticket_id = isset($request[1]) ? $request[1] : NULL;
      if ($ticket_id) {
        $ret = voipsupport_get_ticket($ticket_id);
      }
      else {
        $ret = t("Missing ticket id.") . ' ' . $help_msg;
      }

    }
    else {
      $ret = t("Invalid request.") . ' ' . $help_msg;
    }
  }
  return $ret;
}

/*Create voipsupport ticket*/
function voipsupport_create_ticket_from_sms($site_id, $ticket_text, $caller_number) {
  watchdog('voipsupport', "New ticket for site id: $site_id from number: $caller_number text: $ticket_text");
  $ticket_id = rand(1,1000);
  return "ticket $ticket_id created";
}

function voipsupport_get_ticket($ticket_id) {
  return "ticket $ticket_id retrieved";
}
