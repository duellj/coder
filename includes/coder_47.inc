<?php
// $Id: coder_47.inc,v 1.4 2006-12-30 12:24:37 douggreen Exp $

/** @file
 * This include file implements coder functionality for 4.6 -> 4.7 upgrades
 *
 * Todo: The rules for this review have not yet been defined.
 */ 

function coder_47_reviews() {
  $rules = array(
  );
  $review = array(
    '#title' => t('Converting 4.6.x modules to 4.7.x'),
    '#link' => 'http://drupal.org/node/22218',
    '#rules' => $rules,
  );
  return array('upgrade47' => $review);
}
