<?php

function coder_upgrade_drush_command() {
  return array(
    'coder-upgrade' => array(),
  );
}

function drush_coder_upgrade($name) {
  global $debug;
  $debug = TRUE;
  $form_state = array();
  $form_state['values']['extensions']['list']['module'] =  'module';
  $form_state['values']['extensions']['list']['inc'] =  'inc';
  $form_state['values']['extensions']['list']['info'] =  'info';
  $form_state['values']['extensions']['list']['install'] =  'install';

  $form_state['values']['upgrades']['list']['coder_upgrade'] =  'coder_upgrade';
  $form_state['values']['modules']['list'][$name] =  $name;

  drupal_form_submit('coder_upgrade_conversions_form', $form_state);
}
