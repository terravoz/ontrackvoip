<?php

/**
 * @file
 * Implementation of VoIP validation classes
 */

class OnTrackSite {

  private $site_id;
  private $nid;
  private $site_name;
  private $site_language;

  function __construct($site_id) {
    $this->site_id = $site_id;
    list($entity_type, $entity) = voipextension_get_entity('field_extension', $site_id);
    $this->nid = $entity->nid;
    $site_list = ontrack_get_site_list();
    $this->site_name = isset($site_list[$this->nid]) ? $site_list[$this->nid]['title'] : '';

    if(isset($site_list[$this->nid]) && $site_list[$this->nid]['language'] != 'und') {
      $this->site_language =  $site_list[$this->nid]['language'];
    }
    else {
      $this->site_language = '';
    }
  }

  function getSiteId() {
    return $this->site_id;
  }

  function getNid() {
    return $this->nid;
  }

  function getSiteName() {
    return $this->site_name;
  }

  function getSiteLanguage() {
    return $this->site_language;
  }
}

class OnTrackPhone {

  private $phone_number;
  private $site_ids;
  private $language;
  private $syntax;
  private $extension_ids;

  function __construct($phone_number, $site_ids, $language, $syntax) {
    $this->phone_number = $phone_number;
    $node_ids = is_array($site_ids) ? $site_ids : array($site_ids);
    /*if(is_array($site_ids)) {
      $site_ids = serialize($site_ids);
    }*/
//var_dump($node_ids);die();
    foreach($node_ids as $nid) {
      $node = node_load($nid);
      $extension = field_get_items('node', $node, 'field_extension');
      $extension_ids[$extension[0]['value']] = $extension[0]['value'];
    }
    $this->extension_ids = $extension_ids;
    $this->site_ids = serialize($site_ids);
    $this->language = $language;
    $this->syntax = $syntax;
  }

  function getPhoneNumber() {
    return $this->phone_number;
  }

  function getSiteIds() {
    return unserialize($this->site_ids);
  }

  function getExtensionIds() {
    return $this->extension_ids;
  }

  function getLanguage() {
    return $this->language;
  }

  function getSyntax() {
    return $this->syntax;
  }

  function setPhoneNumber($phone_number) {
    $this->phone_number = $phone_number;
  }

  function setLanguage($language) {
    $this->language = $language;
  }

  function setSyntax($syntax) {
    $this->syntax = $syntax;
  }

  /**
   * Save the OnTrackPhone data to the database
   *
   * @return
   *   The updated OnTrackPhone instance, or NULL in case of an failure
   */
  public function save() {

    // if this is a new call data being created
      $data = new stdClass();
      $update = NULL;

      // initialize 'phone_number' field
      if (!isset($this->phone_number)) {
        $data->phone_number = '';
      }

      // initialize 'site_id' field
      if (!isset($this->site_ids)) {
        $data->site_ids = array();
      }

      // initialize 'language' field
      if (!isset($this->language)) {
        $data->language = '';
      }

      // initialize 'type' field
      if (!isset($this->syntax)) {
        $data->syntax = 'embedded';
      }

    $new_phone_number = $this->phone_number;
    if (isset($new_phone_number)) {
      $data->phone_number = $new_phone_number;
    }

    $new_site_ids = $this->site_ids;
    if (isset($new_site_ids)) {
      $data->site_ids = $new_site_ids;
    }

    $new_language = $this->language;
    if (isset($new_language)) {
      $data->language = $new_language;
    }

    $new_syntax = $this->syntax;
    if (isset($new_syntax)) {
      $data->syntax = $new_syntax;
    }

    // save the data
    drupal_write_record('ontrack_sites', $data);

    $this->phone_number = $data->phone_number;
    $this->site_ids = $data->site_ids;
    $this->language = $data->language;
    $this->syntax = $data->syntax;
    return $this;
  }

  /**
   * Delete all OnTrack Sites data to the database
   *
   */
  public static function deleteAll() {
    db_query("DELETE FROM {ontrack_sites}");
  }

  /**
   * Load all OnTrack Sites data from the database
   *
   */
  public static function loadAll() {
    $result = db_select('ontrack_sites', 'os')
      ->fields('os')
      ->execute();

    $ontrack_phone = array();
    foreach($result as $row) {
      $ontrack_phone[] = new OnTrackPhone($row->phone_number, unserialize($row->site_ids), $row->language, $row->syntax);
    }
    return $ontrack_phone;
  }

  /**
   * Returns the OnTrackPhone instance or NULL
   *
   * @param $phone_number
   *   String representing phone_number value.
   *
   * @return
   *   OnTrackPhone instance
   */
  public static function load($phone_number) {
    $result = db_select('ontrack_sites', 'os')
      ->fields('os')
      ->condition('phone_number' , $phone_number)
      ->execute();

    foreach($result as $row) {
      $ontrack_phone = new OnTrackPhone($row->phone_number, unserialize($row->site_ids), $row->language, $row->syntax);
    }
    return $ontrack_phone;
  }
}
