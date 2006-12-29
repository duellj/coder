<?php
// $Id: coder_50.inc,v 1.2 2006-12-29 03:47:16 douggreen Exp $

/** @file
 * This include file implements coder functionality for 4.7 -> 5.0 upgrades
 * 
 * Todo: The rules for this review have not yet been defined.
 */

function coder_50_reviews() {
  $reviews = array();
  $reviews['upgrade50'] = array(
    '#title' => t('Converting 4.7.x modules to 5.x'),
    '#link' => 'http://drupal.org/node/64279',
  );
  return $reviews;
}
?>
