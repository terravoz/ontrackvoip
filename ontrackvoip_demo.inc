<?php

function ontrack_create_ticket_from_sms($site_id, $ticket_text, $caller_number) {
  watchdog('ontrackvoip', "New ticket for site id: $site_id from number: $caller_number text: $ticket_text");
  $ticket_id = rand(1,1000);
  return $ticket_id;
}

function ontrack_get_ticket($ticket_id) {
  if($ticket_id<1 || $ticket_id > 1000) {
    return NULL;
  }
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

  $site->nid = rand(1,10);
  $data = array(
    'ticket_node' => $ticket,
    'site_node' => $site,
  );
  return $data;
}

function ontrack_get_uid_from_phone($caller_number) {
  return 1;
}

function ontrack_get_site_id_from_phone($dest_number) {
  $site_id = rand(1,10);
  return $site_id;
}

function ontrack_get_site_list() {
  for($i=1;$i<=10;$i++) {

    $sites[$i]['site_id'] = $i;
    if($i>5) {
      $sites[$i]['title'] = 'aSite '.$i;
      $sites[$i]['language'] = 'und';
    }
    else {
      $sites[$i]['title'] = 'bSite '.$i;
      $sites[$i]['language'] = 'und';
    }
  }

  return $sites;
}

function ontrack_get_ticket_status_description_from_code($state, $langcode) {
  return 'New';
}
