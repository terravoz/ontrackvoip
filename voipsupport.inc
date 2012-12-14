<?php

/**
 * @file
 * Implementation of VoIP validation classes
 */

class OnTrackSite {

  private $phone_number;
  private $site_id;
  private $type;
  private $site_name;
  private $language;

  function __construct($phone_number, $site_id, $type, $site_name, $language) {
    $this->phone_number = $phone_number;
    $this->site_id = $site_id;
    $this->type = $type;
    $this->site_name = $site_name;
    $this->language = $language;
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

}

