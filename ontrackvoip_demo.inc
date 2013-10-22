<?php

function ontrack_create_ticket_from_sms($nid, $ticket_text, $caller_number) {
  $node = node_load($nid);
  $extension = field_get_items('node', $node, 'field_extension');
  $site_id = $extension[0]['value'];
  watchdog('ontrackvoip', "New ticket for site id: $site_id from number: $caller_number text: $ticket_text");
  //$ticket_id = rand(1,1000);

  $ticket->type = 'ontrackticket';
  $ticket->title = substr($ticket_text, 0, 255);
  $ticket->uid = 1;
  $ticket->body[LANGUAGE_NONE][] = array (
    'value' => $ticket_text,
  );

  $ticket->field_site_id[LANGUAGE_NONE][0]['nid'] = $nid;

  node_save($ticket);
  return $ticket->nid;
}

function ontrack_get_ticket($ticket_id) {
  $ticket_node = node_load($ticket_id);
  if($ticket_id<1 || $ticket_id > 1000) {
    return NULL;
  }

  $ticket_node->message_id = time();
  $ticket_node->client = rand(1,1000);
  $ticket_node->support_email = FALSE;
  $ticket_node->worldbank_ontrack_act_sup_phone[LANGUAGE_NONE][] = array (
    'value' => '123456789012',
  );

  $site->nid = $ticket_node->field_site_id[LANGUAGE_NONE][0]['nid'];
  $data = array(
    'ticket_node' => $ticket_node,
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
  $query = new EntityFieldQuery();
  $entities = $query->entityCondition('entity_type', 'node')
    ->propertyCondition('type', 'ontracksite')
    ->execute();

  if (!empty($entities['node'])) {
    foreach($entities['node'] as $node_data) {
      $node = node_load($node_data->nid);
      $extension = field_get_items('node', $node, 'field_extension');
      $sites[$node_data->nid] = array(
        'id' => $extension[0]['value'],
        'title' => $node->title,
        'language' => $node->language,
      );
    }
  }

  /*for($i=1;$i<=10;$i++) {

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
*/
  return $sites;
}

function ontrack_get_ticket_status_description_from_code($state, $langcode) {
  return 'New';
}
