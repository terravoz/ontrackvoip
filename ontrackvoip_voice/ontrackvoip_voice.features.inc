<?php
/**
 * @file
 * ontrackvoip_voice.features.inc
 */

/**
 * Implements hook_node_info().
 */
function ontrackvoip_voice_node_info() {
  $items = array(
    'ontrack_voice' => array(
      'name' => t('OnTrack Voice'),
      'base' => 'node_content',
      'description' => '',
      'has_title' => '1',
      'title_label' => t('Title'),
      'help' => '',
    ),
  );
  return $items;
}
