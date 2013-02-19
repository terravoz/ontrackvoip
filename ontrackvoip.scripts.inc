<?php
//@todo: twilio unicode characters http://forum.twilio.com/twilio/topics/spanish_sms_inbound_outbound
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
        //@todo:temp fix
        $text = remove_accents($text);
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
  global $language;
  $ontrack_phone = OnTrackPhone::load($dest_number);
  if (!$ontrack_phone) {
    //@todo: error message when ontrack phone not recognized
    ontrackvoip_log("Destination number $dest_number not recognized.");
    return '';
  }

  //Get either a default phone number language or default site language.
  $default_langcode = isset($ontrack_phone) ? $ontrack_phone->getLanguage() : $language->language;
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
          $langcode = isset($site_langcode) ? $site_langcode : $default_langcode;
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
            $ontrack_site = new OnTrackSite($site_id);
            //Create new ticket
            unset($request[0]); //remove site_id from ticket_text
            $ticket_text = implode(" ", $request);
            $ret = ontrackvoip_create_ticket($ontrack_site, $ticket_text, $caller_number);
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
function ontrackvoip_create_ticket($ontrack_site, $ticket_text, $caller_number) {
  $langcode = $ontrack_site->getSiteLanguage();
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


function remove_accents($string) {
  if (!preg_match('/[\x80-\xff]/', $string))
    return $string;
  if (seems_utf8($string)) {
    $chars = array(
      // Decompositions for Latin-1 Supplement
      chr(195).chr(128) => 'A', chr(195).chr(129) => 'A',
      chr(195).chr(130) => 'A', chr(195).chr(131) => 'A',
      chr(195).chr(132) => 'A', chr(195).chr(133) => 'A',
      chr(195).chr(135) => 'C', chr(195).chr(136) => 'E',
      chr(195).chr(137) => 'E', chr(195).chr(138) => 'E',
      chr(195).chr(139) => 'E', chr(195).chr(140) => 'I',
      chr(195).chr(141) => 'I', chr(195).chr(142) => 'I',
      chr(195).chr(143) => 'I', chr(195).chr(145) => 'N',
      chr(195).chr(146) => 'O', chr(195).chr(147) => 'O',
      chr(195).chr(148) => 'O', chr(195).chr(149) => 'O',
      chr(195).chr(150) => 'O', chr(195).chr(153) => 'U',
      chr(195).chr(154) => 'U', chr(195).chr(155) => 'U',
      chr(195).chr(156) => 'U', chr(195).chr(157) => 'Y',
      chr(195).chr(159) => 's', chr(195).chr(160) => 'a',
      chr(195).chr(161) => 'a', chr(195).chr(162) => 'a',
      chr(195).chr(163) => 'a', chr(195).chr(164) => 'a',
      chr(195).chr(165) => 'a', chr(195).chr(167) => 'c',
      chr(195).chr(168) => 'e', chr(195).chr(169) => 'e',
      chr(195).chr(170) => 'e', chr(195).chr(171) => 'e',
      chr(195).chr(172) => 'i', chr(195).chr(173) => 'i',
      chr(195).chr(174) => 'i', chr(195).chr(175) => 'i',
      chr(195).chr(177) => 'n', chr(195).chr(178) => 'o',
      chr(195).chr(179) => 'o', chr(195).chr(180) => 'o',
      chr(195).chr(181) => 'o', chr(195).chr(182) => 'o',
      chr(195).chr(182) => 'o', chr(195).chr(185) => 'u',
      chr(195).chr(186) => 'u', chr(195).chr(187) => 'u',
      chr(195).chr(188) => 'u', chr(195).chr(189) => 'y',
      chr(195).chr(191) => 'y',
      // Decompositions for Latin Extended-A
      chr(196).chr(128) => 'A', chr(196).chr(129) => 'a',
      chr(196).chr(130) => 'A', chr(196).chr(131) => 'a',
      chr(196).chr(132) => 'A', chr(196).chr(133) => 'a',
      chr(196).chr(134) => 'C', chr(196).chr(135) => 'c',
      chr(196).chr(136) => 'C', chr(196).chr(137) => 'c',
      chr(196).chr(138) => 'C', chr(196).chr(139) => 'c',
      chr(196).chr(140) => 'C', chr(196).chr(141) => 'c',
      chr(196).chr(142) => 'D', chr(196).chr(143) => 'd',
      chr(196).chr(144) => 'D', chr(196).chr(145) => 'd',
      chr(196).chr(146) => 'E', chr(196).chr(147) => 'e',
      chr(196).chr(148) => 'E', chr(196).chr(149) => 'e',
      chr(196).chr(150) => 'E', chr(196).chr(151) => 'e',
      chr(196).chr(152) => 'E', chr(196).chr(153) => 'e',
      chr(196).chr(154) => 'E', chr(196).chr(155) => 'e',
      chr(196).chr(156) => 'G', chr(196).chr(157) => 'g',
      chr(196).chr(158) => 'G', chr(196).chr(159) => 'g',
      chr(196).chr(160) => 'G', chr(196).chr(161) => 'g',
      chr(196).chr(162) => 'G', chr(196).chr(163) => 'g',
      chr(196).chr(164) => 'H', chr(196).chr(165) => 'h',
      chr(196).chr(166) => 'H', chr(196).chr(167) => 'h',
      chr(196).chr(168) => 'I', chr(196).chr(169) => 'i',
      chr(196).chr(170) => 'I', chr(196).chr(171) => 'i',
      chr(196).chr(172) => 'I', chr(196).chr(173) => 'i',
      chr(196).chr(174) => 'I', chr(196).chr(175) => 'i',
      chr(196).chr(176) => 'I', chr(196).chr(177) => 'i',
      chr(196).chr(178) => 'IJ',chr(196).chr(179) => 'ij',
      chr(196).chr(180) => 'J', chr(196).chr(181) => 'j',
      chr(196).chr(182) => 'K', chr(196).chr(183) => 'k',
      chr(196).chr(184) => 'k', chr(196).chr(185) => 'L',
      chr(196).chr(186) => 'l', chr(196).chr(187) => 'L',
      chr(196).chr(188) => 'l', chr(196).chr(189) => 'L',
      chr(196).chr(190) => 'l', chr(196).chr(191) => 'L',
      chr(197).chr(128) => 'l', chr(197).chr(129) => 'L',
      chr(197).chr(130) => 'l', chr(197).chr(131) => 'N',
      chr(197).chr(132) => 'n', chr(197).chr(133) => 'N',
      chr(197).chr(134) => 'n', chr(197).chr(135) => 'N',
      chr(197).chr(136) => 'n', chr(197).chr(137) => 'N',
      chr(197).chr(138) => 'n', chr(197).chr(139) => 'N',
      chr(197).chr(140) => 'O', chr(197).chr(141) => 'o',
      chr(197).chr(142) => 'O', chr(197).chr(143) => 'o',
      chr(197).chr(144) => 'O', chr(197).chr(145) => 'o',
      chr(197).chr(146) => 'OE',chr(197).chr(147) => 'oe',
      chr(197).chr(148) => 'R',chr(197).chr(149) => 'r',
      chr(197).chr(150) => 'R',chr(197).chr(151) => 'r',
      chr(197).chr(152) => 'R',chr(197).chr(153) => 'r',
      chr(197).chr(154) => 'S',chr(197).chr(155) => 's',
      chr(197).chr(156) => 'S',chr(197).chr(157) => 's',
      chr(197).chr(158) => 'S',chr(197).chr(159) => 's',
      chr(197).chr(160) => 'S', chr(197).chr(161) => 's',
      chr(197).chr(162) => 'T', chr(197).chr(163) => 't',
      chr(197).chr(164) => 'T', chr(197).chr(165) => 't',
      chr(197).chr(166) => 'T', chr(197).chr(167) => 't',
      chr(197).chr(168) => 'U', chr(197).chr(169) => 'u',
      chr(197).chr(170) => 'U', chr(197).chr(171) => 'u',
      chr(197).chr(172) => 'U', chr(197).chr(173) => 'u',
      chr(197).chr(174) => 'U', chr(197).chr(175) => 'u',
      chr(197).chr(176) => 'U', chr(197).chr(177) => 'u',
      chr(197).chr(178) => 'U', chr(197).chr(179) => 'u',
      chr(197).chr(180) => 'W', chr(197).chr(181) => 'w',
      chr(197).chr(182) => 'Y', chr(197).chr(183) => 'y',
      chr(197).chr(184) => 'Y', chr(197).chr(185) => 'Z',
      chr(197).chr(186) => 'z', chr(197).chr(187) => 'Z',
      chr(197).chr(188) => 'z', chr(197).chr(189) => 'Z',
      chr(197).chr(190) => 'z', chr(197).chr(191) => 's',
      // Euro Sign
      chr(226).chr(130).chr(172) => 'E',
      // GBP (Pound) Sign
      chr(194).chr(163) => '');
    $string = strtr($string, $chars);
  } else {
    // Assume ISO-8859-1 if not UTF-8
    $chars['in'] = chr(128).chr(131).chr(138).chr(142).chr(154).chr(158)
      .chr(159).chr(162).chr(165).chr(181).chr(192).chr(193).chr(194)
      .chr(195).chr(196).chr(197).chr(199).chr(200).chr(201).chr(202)
      .chr(203).chr(204).chr(205).chr(206).chr(207).chr(209).chr(210)
      .chr(211).chr(212).chr(213).chr(214).chr(216).chr(217).chr(218)
      .chr(219).chr(220).chr(221).chr(224).chr(225).chr(226).chr(227)
      .chr(228).chr(229).chr(231).chr(232).chr(233).chr(234).chr(235)
      .chr(236).chr(237).chr(238).chr(239).chr(241).chr(242).chr(243)
      .chr(244).chr(245).chr(246).chr(248).chr(249).chr(250).chr(251)
      .chr(252).chr(253).chr(255);
    $chars['out'] = "EfSZszYcYuAAAAAACEEEEIIIINOOOOOOUUUUYaaaaaaceeeeiiiinoooooouuuuyy";
    $string = strtr($string, $chars['in'], $chars['out']);
    $double_chars['in'] = array(chr(140), chr(156), chr(198), chr(208), chr(222), chr(223), chr(230), chr(240), chr(254));
    $double_chars['out'] = array('OE', 'oe', 'AE', 'DH', 'TH', 'ss', 'ae', 'dh', 'th');
    $string = str_replace($double_chars['in'], $double_chars['out'], $string);
  }
  return $string;
}

/**
 * Checks to see if a string is utf8 encoded.
 *
 * @author bmorel at ssi dot fr
 *
 * @param string $Str The string to be checked
 * @return bool True if $Str fits a UTF-8 model, false otherwise.
 */
function seems_utf8($Str) { # by bmorel at ssi dot fr
  $length = strlen($Str);
  for ($i = 0; $i < $length; $i++) {
    if (ord($Str[$i]) < 0x80) continue; # 0bbbbbbb
    elseif ((ord($Str[$i]) & 0xE0) == 0xC0) $n = 1; # 110bbbbb
    elseif ((ord($Str[$i]) & 0xF0) == 0xE0) $n = 2; # 1110bbbb
    elseif ((ord($Str[$i]) & 0xF8) == 0xF0) $n = 3; # 11110bbb
    elseif ((ord($Str[$i]) & 0xFC) == 0xF8) $n = 4; # 111110bb
    elseif ((ord($Str[$i]) & 0xFE) == 0xFC) $n = 5; # 1111110b
    else return false; # Does not match any model
    for ($j = 0; $j < $n; $j++) { # n bytes matching 10bbbbbb follow ?
      if ((++$i == $length) || ((ord($Str[$i]) & 0xC0) != 0x80))
        return false;
    }
  }
  return true;
}

function utf8_uri_encode($utf8_string, $length = 0) {
  $unicode = '';
  $values = array();
  $num_octets = 1;
  $unicode_length = 0;
  $string_length = strlen($utf8_string);
  for ($i = 0; $i < $string_length; $i++) {
    $value = ord($utf8_string[$i]);
    if ($value < 128) {
      if ($length && ($unicode_length >= $length))
        break;
      $unicode .= chr($value);
      $unicode_length++;
    } else {
      if (count($values) == 0) $num_octets = ($value < 224) ? 2 : 3;
      $values[] = $value;
      if ($length && ($unicode_length + ($num_octets * 3)) > $length)
        break;
      if (count( $values ) == $num_octets) {
        if ($num_octets == 3) {
          $unicode .= '%' . dechex($values[0]) . '%' . dechex($values[1]) . '%' . dechex($values[2]);
          $unicode_length += 9;
        } else {
          $unicode .= '%' . dechex($values[0]) . '%' . dechex($values[1]);
          $unicode_length += 6;
        }
        $values = array();
        $num_octets = 1;
      }
    }
  }
  return $unicode;
}

/**
 * Sanitizes title, replacing whitespace with dashes.
 *
 * Limits the output to alphanumeric characters, underscore (_) and dash (-).
 * Whitespace becomes a dash.
 *
 * @param string $title The title to be sanitized.
 * @return string The sanitized title.
 */
function slugify($title) {
  $title = strip_tags($title);
  // Preserve escaped octets.
  $title = preg_replace('|%([a-fA-F0-9][a-fA-F0-9])|', '---$1---', $title);
  // Remove percent signs that are not part of an octet.
  $title = str_replace('%', '', $title);
  // Restore octets.
  $title = preg_replace('|---([a-fA-F0-9][a-fA-F0-9])---|', '%$1', $title);
  $title = remove_accents($title);
  if (seems_utf8($title)) {
    if (function_exists('mb_strtolower')) {
      $title = mb_strtolower($title, 'UTF-8');
    }
    $title = utf8_uri_encode($title, 200);
  }
  $title = strtolower($title);
  $title = preg_replace('/&.+?;/', '', $title); // kill entities
  $title = preg_replace('/[^%a-z0-9 _-]/', '', $title);
  $title = preg_replace('/\s+/', '-', $title);
  $title = preg_replace('|-+|', '-', $title);
  $title = trim($title, '-');
  return $title;
}
