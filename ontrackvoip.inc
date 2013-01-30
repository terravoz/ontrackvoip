<?php

/**
 * @file
 * Implementation of VoIP validation classes
 */

class OnTrackSite {

  private $id;
  private $phone_number;
  private $site_id;
  private $language;
  private $syntax;

  function __construct($phone_number, $site_id, $language, $syntax, $id = NULL ) {
    $this->id = $id;
    $this->phone_number = $phone_number;
    $this->site_id = $site_id;
    $this->language = $language;
    $this->syntax = $syntax;
  }


  /**
   * Returns the array of OnTrack site_ids associated with given phone_number
   *
   * @param $phone_number
   *   String representing phone number value.
   *
   * @return
   *   Array of site ids
   */
  public static function getSiteIds($phone_number) {
    $site_ids = array();
    $result = db_select('ontrack_sites', 'os')
      ->fields('os', array('site_id'))
      ->condition('phone_number' ,  $phone_number)
      ->execute();
    foreach($result as $row) {
      $site_ids[$row->site_id] = $row->site_id;
    }
    return $site_ids;
  }

  function getPhoneNumber() {
    return $this->phone_number;
  }

  function getSiteId() {
    return $this->site_id;
  }

  function getSiteName() {
    //@todo:
    $site_list = ontrack_get_site_list();
    $site_name = isset($site_list[$this->site_id]) ? $site_list[$this->site_id]->name : '';
    return $site_name;
  }

  function getLanguage() {
    return variable_get('ontrack_voip_language', 'es');
    //@todo: restore when we have working UI
    //return $this->language;
  }

  function getSyntax() {
    return $this->syntax;
  }

  function setPhoneNumber($phone_number) {
    $this->phone_number = $phone_number;
  }

  function setId($site_id) {
    $this->site_id = $site_id;
  }

  function setLanguage($language) {
    $this->language = $language;
  }

  function setSyntax($syntax) {
    $this->syntax = $syntax;
  }

  /**
   * Save the VoIP Number data to the database
   *
   * @return
   *   The updated VoipNumber instance, or NULL in case of an invalid vnid
   */
  public function save() {

    // if this is a new call data being created
    if (is_null($this->id)) {
      $data = new stdClass();
      $update = NULL;

      // initialize 'phone_number' field
      if (!isset($this->phone_number)) {
        $data->phone_number = '';
      }

      // initialize 'site_id' field
      if (!isset($this->site_id)) {
        $data->site_id = '';
      }

      // initialize 'language' field
      if (!isset($this->language)) {
        $data->language = '';
      }

      // initialize 'type' field
      if (!isset($this->syntax)) {
        $data->syntax = 'embedded';
      }
    }
    else {
      // load the data
      $data = db_query("SELECT * FROM {ontrack_sites} WHERE id = :id", array(':id' => $this->id))->fetchObject();
      $update = 'id';
    }

    // update voipnumber-specific fields
    $new_phone_number = $this->phone_number;
    if (isset($new_phone_number)) {
      $data->phone_number = $new_phone_number;
    }

    $new_site_id = $this->site_id;
    if (isset($new_site_id)) {
      $data->site_id = $new_site_id;
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
    if(is_null($update)) {
      drupal_write_record('ontrack_sites', $data);
    }
    else {
      drupal_write_record('ontrack_sites', $data, $update);
    }

    $this->id = $data->id;
    $this->phone_number = $data->phone_number;
    $this->site_id = $data->site_id;
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
   * Returns the array of OnTrackSite objects
   *
   * @param $site_id
   *   Optional integer representing site_id value.
   *
   * @return
   *   Array of OnTrackSite objects
   */
  public static function load($site_id = NULL) {
    $ontrack_sites = array();
    $query = db_select('ontrack_sites', 'os')
      ->fields('os');

    if ($site_id) {
      $query->condition('site_id' , $site_id);
    }
    $result = $query->execute();
    foreach($result as $row) {
      $ontrack_sites[] = new OnTrackSite($row->phone_number, $row->site_id, $row->language, $row->syntax, $row->id);
    }
    return $ontrack_sites;
  }

  /**
   * Returns the OnTrackSite object or NULL
   *
   * @param $site_id
   *   Integer representing site_id value.
   * @param $phone_number
   *   String representing phone_number value.
   *
   * @return
   *   OnTrackSite object matching give site_id and phone_number or NULL
   */
  public static function getSite($site_id, $phone_number) {
    $ontrack_sites = array();
    $result = db_select('ontrack_sites', 'os')
      ->fields('os')
      ->condition('site_id' , $site_id)
      ->condition('phone_number' , $phone_number)
      ->execute();

    $ontrack_site = NULL;
    foreach($result as $row) {
      $ontrack_site = new OnTrackSite($row->phone_number, $row->site_id, $row->language, $row->syntax, $row->id);
    }
    return $ontrack_site;
  }
}

