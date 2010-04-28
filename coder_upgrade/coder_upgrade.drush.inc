<?php
// $Id$

/**
 * @file
 * Drush support for Coder Upgrades
 */

/**
 * Implementation of hook_drush_help().
 */
function coder_upgrade_drush_help($section) {

}

/**
 * Implementation of hook_drush_command().
 */
function coder_upgrade_drush_command() {
  $items['coder-upgrade'] = array(
  );
  $items['coder-upgrade-list'] = array(
    'description' => 'List modules available for upgrade',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_NONE,
    'drupal dependecies' => array('coder_upgrade'),
  );
  return $items;
}

/**
 * List modules that are available for upgrade.
 */
function drush_coder_upgrade_list() {
  $modules = coder_upgrade_module_list();
  print_R($modules);
}
