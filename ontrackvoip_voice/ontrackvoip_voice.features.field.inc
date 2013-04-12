<?php
/**
 * @file
 * ontrackvoip_voice.features.field.inc
 */

/**
 * Implements hook_field_default_fields().
 */
function ontrackvoip_voice_field_default_fields() {
  $fields = array();

  // Exported field: 'node-ontrack_voice-field_ontrackvoice_audio'.
  $fields['node-ontrack_voice-field_ontrackvoice_audio'] = array(
    'field_config' => array(
      'active' => '1',
      'cardinality' => '1',
      'deleted' => '0',
      'entity_types' => array(),
      'field_name' => 'field_ontrackvoice_audio',
      'foreign keys' => array(
        'fid' => array(
          'columns' => array(
            'fid' => 'fid',
          ),
          'table' => 'file_managed',
        ),
      ),
      'indexes' => array(
        'fid' => array(
          0 => 'fid',
        ),
      ),
      'locked' => '0',
      'module' => 'file',
      'settings' => array(
        'display_default' => 0,
        'display_field' => 0,
        'uri_scheme' => 'public',
      ),
      'translatable' => '0',
      'type' => 'file',
    ),
    'field_instance' => array(
      'bundle' => 'ontrack_voice',
      'deleted' => '0',
      'description' => '',
      'display' => array(
        'default' => array(
          'label' => 'above',
          'module' => 'audiofield',
          'settings' => array(),
          'type' => 'audiofield_embedded',
          'weight' => 1,
        ),
        'teaser' => array(
          'label' => 'above',
          'settings' => array(),
          'type' => 'hidden',
          'weight' => 0,
        ),
      ),
      'entity_type' => 'node',
      'field_name' => 'field_ontrackvoice_audio',
      'label' => 'Audio',
      'required' => 0,
      'settings' => array(
        'description_field' => 0,
        'file_directory' => '',
        'file_extensions' => 'wav mp3',
        'max_filesize' => '',
        'user_register_form' => FALSE,
      ),
      'widget' => array(
        'active' => 0,
        'module' => 'audiorecorderfield',
        'settings' => array(
          'progress_indicator' => 'throbber',
        ),
        'type' => 'audiorecorderfield_widget',
        'weight' => '2',
      ),
    ),
  );

  // Exported field: 'node-ontrack_voice-field_ontrackvoice_type'.
  $fields['node-ontrack_voice-field_ontrackvoice_type'] = array(
    'field_config' => array(
      'active' => '1',
      'cardinality' => '1',
      'deleted' => '0',
      'entity_types' => array(),
      'field_name' => 'field_ontrackvoice_type',
      'foreign keys' => array(),
      'indexes' => array(
        'value' => array(
          0 => 'value',
        ),
      ),
      'locked' => '0',
      'module' => 'list',
      'settings' => array(
        'allowed_values' => array(
          'issue' => 'Issue',
          'story' => 'Story',
          'suggestion' => 'Suggestion',
        ),
        'allowed_values_function' => '',
      ),
      'translatable' => '0',
      'type' => 'list_text',
    ),
    'field_instance' => array(
      'bundle' => 'ontrack_voice',
      'default_value' => NULL,
      'deleted' => '0',
      'description' => '',
      'display' => array(
        'default' => array(
          'label' => 'above',
          'module' => 'list',
          'settings' => array(),
          'type' => 'list_default',
          'weight' => 2,
        ),
        'teaser' => array(
          'label' => 'above',
          'settings' => array(),
          'type' => 'hidden',
          'weight' => 0,
        ),
      ),
      'entity_type' => 'node',
      'field_name' => 'field_ontrackvoice_type',
      'label' => 'Type',
      'required' => 1,
      'settings' => array(
        'user_register_form' => FALSE,
      ),
      'widget' => array(
        'active' => 1,
        'module' => 'options',
        'settings' => array(),
        'type' => 'options_select',
        'weight' => '1',
      ),
    ),
  );

  // Translatables
  // Included for use with string extractors like potx.
  t('Audio');
  t('Type');

  return $fields;
}
