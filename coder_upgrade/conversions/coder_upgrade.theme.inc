<?php
// $Id$

/**
 * @file
 * Theme specific conversion routine file for the coder_upgrade module.
 */

/**
 * Implements hook_upgrade_theme_alter().
 */
function coder_upgrade_upgrade_theme_alter(&$node, &$reader) {
  global $upgrade_theme_registry;

  // Create helper objects.
  $editor = PGPEditor::getInstance();

  // Function parameters are now a single array.
  $item = &$node->data;
  $body = &$item->body;
  for ($i = 0; $i < $item->parameterCount(); $i++) {
    $parameters[] = $item->printParameter($i);
  }
  $item->parameters->clear();
  $editor->setParameter($item, 0, '$variables');

  // Reassign function parameters to corresponding $variables array key.
  $theme_name = substr($item->name, strpos($item->name, "_") + 1);
  if ($upgrade_theme_registry[$theme_name]) {
    $hook = $upgrade_theme_registry[$theme_name];
    if (isset($hook['render element'])) {
      $string = $parameters[0] . ' = $variables[\'' . $hook['render element'] .'\']';
    }
    else {
      foreach ($hook['variables'] as $key => $variable) {
        $string = $parameters[$key] . ' = $variables[\'' . $variable ."'];\n";
        $body->insertFirst($editor->textToStatements($string)->getElement(0));
      }
    }
  }

  // If theme function calls drupal_render, make sure last call is to drupal_render_children
  // to avoid endless loops: http://drupal.org/node/224333#drupal_render_children
  if ($statement = $body->searchBackward('PGPFunctionCall', 'name', 'value', 'drupal_render')) {
    $statement->name['value'] = 'drupal_render_children';
  }
}
