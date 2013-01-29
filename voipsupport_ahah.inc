<?php

/**
 * Submit handler for the "add-one-more" button.
 *
 * Increments the max counter and causes a rebuild.
 */
function voipsupport_ahah_add_more_anon($form, &$form_state) {
  $form_state['voipsupport_ahah_rows_count']++;
  $form_state['rebuild'] = TRUE;
}

/**
 * Callback for both ajax-enabled buttons.
 *
 * Returns the anon page textfields
 */
function voipsupport_ahah_add_more_anon_callback($form, $form_state) {
  return $form['wrapper']['field'];
}


