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

  function __construct($phone_number, $site_id, $language, $syntax ) {
    $this->phone_number = $phone_number;
    $this->site_id = $site_id;
    $this->language = $language;
    $this->syntax = $syntax;
  }


  /**
   * Returns the OnTrackSite associated with the given site id
   *
   * @param $site_id
   *   Integer representing site id value.
   *
   * @return
   *   Single OnTrackSite object.
   */
  public static function getSiteFromId($site_id) {
    //@todo: Implement logic to load site information given the site_id
    $phone_number = '123456789';
    $type = 'test';
    $site_name = 'test';
    $language = 'en';
    $site = new OnTrackSite($phone_number, $site_id, $type, $site_name, $language);
    return $site;
  }

  /**
   * Returns the OnTrackSite associated with the given phone number
   *
   * @param $site_id
   *   String representing phone number value.
   *
   * @return
   *   Single OnTrackSite object.
   */
  public static function getSiteFromPhoneNumber($phone_number) {
    //@todo: Implement logic to load site information given the phone_number
    $site_id = rand(1, 1000);
    $type = 'test';
    $site_name = 'test';
    $language = 'en';
    $site = new OnTrackSite($phone_number, $site_id, $type, $site_name, $language);
    return $site;
  }

  function getPhoneNumber() {
    return $this->phone_number;
  }

  function getType() {
    return $this->type;
  }

  function getSiteName() {
    return $this->site_name;
  }

  function getSiteId() {
    return $this->site_id;
  }

  function getLanguage() {
    return $this->language;
  }

  function setPhoneNumber($phone_number) {
    $this->phone_number = $phone_number;
  }

  function setType($type) {
    $this->type = $type;
  }

  function setSiteName($site_name) {
    $this->site_name = $site_name;
  }

  function setId($site_id) {
    $this->site_id = $site_id;
  }

  function setLanguage($language) {
    $this->language = $language;
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
      $data = db_fetch_object(db_query("SELECT * FROM {ontrack_sites} WHERE id=%d", $this->id));
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
    drupal_write_record('ontrack_sites', $data, $update);
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

}

