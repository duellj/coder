<?php
// $Id: coder_upgrade.db.inc,v 1.3 2010-03-11 18:42:06 solotandem Exp $

/**
 * @file
 * Database function call conversion routine file for the coder_upgrade module.
 * These routines upgrade functions (or hooks) using the grammar parser.
 *
 * The functions in this conversion routine file correspond to topics in the
 * category roadmap at http://drupal.org/node/394070 that are marked with a
 * green check mark in the Upgrade column.
 *
 * Copyright 2009-10 by Jim Berry ("solotandem", http://drupal.org/user/240748)
 */

/**
 * The upgrades to these functions are documented at the following urls.
 *
 * Database
 * http://drupal.org/node/224333#dbtng (NOT FINISHED)
 */

/**
 * Implements hook_upgrade_call_db_query_alter().
 *
 * @param PGPFunctionCall $item
 */
function coder_upgrade_upgrade_call_db_query_alter(&$item, &$reader) {
  // Disable this routine until it is improved.

  // Create helper objects.
  $editor = PGPEditor::getInstance();

  /*
   * Use cases
   * - query is a string: parse directly
   * - query is a variable; find the variable and see if it is a string (assigned only once or always from strings, no expressions)
   * - query is an expression (leave this alone)
   */

  // Process function call.
  $name = &$item->name; // DONE (NEEDS WORK)
  $p0 = $item->getParameter(0);
  if ($p0->count() == 1) {
    $operand0 = $p0->getElement();
    if (is_array($operand0) && $operand0['type'] == T_CONSTANT_ENCAPSED_STRING) {
      // Convert values to strings.
      // Parse the string.
      coder_upgrade_parse_query_string($item, $operand0, $reader);
    }
  }
}

/**
 * Implements hook_upgrade_call_db_query_alter().
 *
 * @param PGPFunctionCall $item
 */
function coder_upgrade_parse_query_string(&$item, &$operand, &$reader) {
  // Create helper objects.
  $editor = PGPEditor::getInstance();

  module_load_include('inc', 'coder_upgrade', 'conversions/sql_parser');
  $sql_parser = new SqlParser();

  // TODO Parse the values - the other parameters
  // TODO The parameter could be an array variable or expression
  // See http://drupalcode.org/viewvc/drupal/drupal/modules/simpletest/tests/database_test.test?revision=1.80&view=markup
  $values = array();
  for ($i = 1; $i < $item->parameters->count(); $i++) {
    $values[] = $item->printParameter($i);
  }
  cdp($values);

  /*
   * TODO Run the table names through a conversion routine.
   * Ex: taxonomy, block, etc.
   */
  $new = array();
  // TODO trim won't work here, since it'll trim off the last ' if the string ends in '%s'
  $sql = substr($operand['value'], 1, -1);
  dpm($sql);
  dpm('$values = ' . print_r($values, 1));
  $parsed_sql = $sql_parser->parse(str_replace(array("{", "}"), array(), $sql));
  dpm($parsed_sql);

  $new = array();
  switch ($parsed_sql['command']) {
    case 'select':
      $new[] = "db_select('" . $parsed_sql['table_names'][0] . "')";
      $new[] = "\t->fields(array(" . join(", ", $parsed_sql['column_names']) . "))";
      $new = array_merge($new, coder_upgrade_convert_conditional($parsed_sql['where_clause'], $values));
      if ($parsed_sql['sort_order']) {
        foreach ($parsed_sql['sort_order'] as $field => $order) {
          $new[] = "\t->orderBy($field, $order)";
        }
      } 
      if ($parsed_sql['limit_clause']) {
        $new[] = "\t->range(" . $parsed_sql['limit_clause']['start'] . ', ' . $parsed_sql['limit_clause']['length'] . ")";
      }
      break;

    case 'insert':
      $new[] = '$id = db_insert(\'' . $parsed_sql['table_names'][0] . '\')';
      $new = array_merge($new, coder_upgrade_convert_fields($parsed_sql['column_names'], $parsed_sql['values'], $values));
      $new[] = "\t->execute();";
      break;

    case 'update':
      $new[] = "db_update('" . $parsed_sql['table_names'][0] . "')";
      $new = array_merge($new, coder_upgrade_convert_fields($parsed_sql['column_names'], $parsed_sql['values'], $values));
      $new = array_merge($new, coder_upgrade_convert_conditional($parsed_sql['where_clause'], $values));
      $new[] = "\t->execute();";
      break;

    case 'delete':
      break;
  }
  dpm(join($new, "\n"));
  dpm("\n");

  // TODO: remove
  unset($new);

  if (strpos($sql, 'SELECT') === 0) {
    $find = '@^(SELECT.*?WHERE\s+)(.*?)$@m';
    preg_match($find, $sql, $matches);
    cdp($matches);
    // Convert conditions from field = '%s' to field = :field
    // TODO handle more complex conditions
    if ($matches[2]) {
      preg_match_all('/
        (\S*)\s+([!=<>]+)\s+[\'"]?%[sdfb][\'"]? # Match field = %d
        |
        (\S*)\s+IN\s?\([\'"]%s[\'"]?\) # Match field IN(%s)
        |
        (\S*)\s+([!=<>]+)\s+(\S*) # Match field literals
      /x', $matches[2], $conditions); 
      foreach ($conditions[0] as $key => $condition) {
        // field = %d condition
        if ($conditions[1][$key]) {
          $condition_keys[] = ':'. $conditions[1][$key];
          $operand['value'] = str_replace($condition, $conditions[1][$key] . ' ' . $conditions[2][$key] . ' :' . $conditions[1][$key], $operand['value']);
        }
        // filed IN (%s) condition
        else if ($conditions[3][$key]) {
          $condition_keys[] = ':'. $conditions[3][$key];
          $operand['value'] = str_replace($condition, $conditions[3][$key] . ' IN (:' . $conditions[3][$key] . ')', $operand['value']);
          // D6 convention was to use implode(",", $array).  Now we can just pass in the array.
          $param = $item->getParameter($key + 1)->getElement()->getParameter(1);  
          $item->setParameter($key + 1, $param);
        }
        else if ($conditions[4][$key]) {
          $condition_keys[] = ':'. $conditions[4][$key];
          $operand['value'] = str_replace($condition, $conditions[4][$key] . ' ' . $conditions[5][$key] . ' :' . $conditions[4][$key], $operand['value']);
          $item->insertParameter($key + 1, $editor->expressionToStatement($conditions[6][$key]));
        }
      }
      $editor->setParameter($item, 0, $operand['value']);
      $string = $editor->arrayitize($item, 1, $condition_keys, array_fill(0, count($condition_keys), "'XXX_YYY'"));
      $temp = $editor->expressionToStatement($string);
      $temp->getElement(0)->multiline = 0;
      $item->setParameter(1, $temp);
    }
    return;
  }
  elseif (strpos($sql, 'INSERT') === 0) {
    // INSERT INTO {mytable_longer_name} (intvar, stringvar, floatvar) VALUES (%d, '%s', %f)
    $find = '@INSERT INTO\s+{(\w+)}\s+\((.*?)\)\s+VALUES\s+\((.*?)\)@';
    preg_match($find, $sql, $matches);
    cdp($matches);
    $fields = array_combine(explode(', ', $matches[2]) , explode(', ', $matches[3]));

    cdp($fields);
    // TODO Values could be literals not placeholders.
    $new[] = "\$id = db_insert('{$matches[1]}')";
    $new[] = "\t->fields(array(";
    foreach ($fields as $field => $value) {
      
      // Value is a placeholder
      if (preg_match('/%[sdbf]/', $value)) {
        $value = array_shift($values);
      }
      $new[] = "\t\t'$field' => $value,";
    }
    $new[] = "\t))";
    $new[] = "\t->execute();";
  }
  elseif (strpos($sql, 'UPDATE') === 0) {
    // UPDATE {node} SET title='%s', status=%d WHERE uid=%d
    $find = '@^UPDATE\s+{(\w+)}\s+SET\s+(.*?)\s+WHERE\s+(.*?)$@m';
    // TODO Check for WHERE and use a different regex.
    preg_match($find, $sql, $matches);
    cdp($matches);
    $fields = explode(', ', $matches[2]);
    cdp($fields);
    $new[] = "db_update('{$matches[1]}')";
    $new[] = "\t->fields(array(";
    foreach ($fields as $field) {
      list($field,$value) = explode("=", $field);
      // Value is a placeholder
      if (preg_match('/%[sdbf]/', $value)) {
        $value = array_shift($values);
      }
      // Trim the field for whitespace.
      $field = trim($field);
      $new[] = "\t\t'$field' => $value,";
    }
    $new[] = "\t))";
    if ($matches[3]) {
      $new[] = coder_upgrade_parse_sql_conditions($matches[3], $values);
    }
    $new[] = "\t->execute();";
  }
  elseif (strpos($sql, 'DELETE') === 0) {
    // DELETE FROM {node} WHERE uid=%d AND created < %d
    $find = '@^DELETE\s+FROM\s+{(\w+)}\s+WHERE\s+(.*?)$@m';
    // TODO Check for WHERE and use a different regex.
    preg_match($find, $sql, $matches);
    cdp($matches);
    $new[] = "db_delete('{$matches[1]}')";
    $new[] = coder_upgrade_parse_sql_conditions($matches[2], $values);
    $new[] = "\t->execute();";
  }

  cdp($new);
  $new = implode("\n", $new);
  $new = str_replace("\t", '  ', $new);
  cdp($new);

  // Get the parent = statement (i.e. node) this function call is part of.
  $parent = &$item->parent;
  // Get the statement list the parent is part of.
  $container = &$parent->container;
  // Insert a statement.
  $statement = $editor->textToStatements($new)->getElement(0);
  $container->insertAfter($parent, $statement, 'function_call');
}

/**
 * Implements hook_upgrade_call_db_rewrite_sql_alter().
 *
 * @param PGPFunctionCall $item
 */
function coder_upgrade_upgrade_call_db_rewrite_sql_alter(&$item, &$reader) {
  // Create helper objects.
  $editor = PGPEditor::getInstance();

  /*
   * Use cases
   * - query is a string: parse directly
   * - query is a variable; find the variable and see if it is a string
   * - query is an expression
   */

  // Process function call.
  $name = &$item->name; // NOT DONE - Copied from db_query above
  $p0 = $item->getParameter(0);
  if ($p0->count() == 1) {
    $operand0 = $p0->getElement();
    if (is_array($operand0) && $operand0['type'] == T_CONSTANT_ENCAPSED_STRING) {
      // Convert values to strings.
      // Parse the string.
      coder_upgrade_parse_query_string($item, $operand0, $reader);
      // Add $query->addTag('node_access' /* TODO Please indicate the appropriate tag */);
    }
  }
}

/**
 * Parse sql conditions into conditional object strings.
 *
 * @param string $conditions
 * @param array $values
 */
function coder_upgrade_parse_sql_conditions($conditions, $values) {
  // Check for inner conditionals or function calls.  For now, we'll just use
  // where() instead of conditionals, given the complexity of inner conditionals.
  if (preg_match("/\((.*)\)/", $conditions, $matches)) {
    $output = "\t\t// TODO: Complex conditional clauses should be rewritten using the conditon() method, if possible\n";
    $output .= "\t\t// see http://drupal.org/node/310086\n";
    $output .= "\t\t->where('$conditions', " . ($values) . ")";
    return $output;
  }
  // TODO handle other conjunctions besides AND.
  $split_conditions = preg_split('/AND|OR|XOR/', $conditions, -1, PREG_SPLIT_DELIM_CAPTURE);
  if (!is_array($split_conditions)) {
    $split_conditions = array($conditions);
  }

  foreach ($split_conditions as $condition) {
    list($condition, $operator, $value) = preg_split('/([!=><]+|LIKE)/', $condition, -1, PREG_SPLIT_DELIM_CAPTURE);
    if (preg_match('/%[sbdf]/', trim($value))) {
      $value = array_shift($values);
    }
    // Trim the condition for whitespace.
    $condition = trim($condition);
    $value = trim($value);
    // TODO handle IN and BETWEEN conditions
    switch ($operator) {
      case '=':
        $parsed_conditions[] = "\t\t->condition('$condition', $value)";
        break;
      default:
        $parsed_conditions[] = "\t\t->condition('$condition', $value, '$operator')";
        break;
    }
  }
  return implode("\n", $parsed_conditions);
}

function coder_upgrade_convert_fields($fields, $field_values = array(), &$passed_values = array()) {
  $new = array();
  $new[] = "\t->fields(array(";
  foreach ($fields as $key => $column) {
    if ($field_values[$key]['type'] == 'placeholder') {
      $value = array_shift($passed_values);
    }
    else {
      $value = $parsed_sql['values'][$key]['value'];
    }
    $new[] = "\t\t'$column' => $value,";
  }
  $new[] = "\t))";
  return $new;
}

/**
 * Transform parsed sql into conditional object strings.
 *
 * @param $conditionals
 *   Parsed conditional array.
 * @param $values
 *   Placeholder values
 * @param $level
 *   Level of conditional statements.
 *
 * @return
 *   Array of conditional object strings.
 */
function coder_upgrade_convert_conditional($conditionals, &$values, $level = 1) {
  $return = array();
  $closing_parenthesis = FALSE;

  // Convert operator.
  switch ($conditionals['op']) {
    case 'and':
      if ($level > 1) {
        $return[] = str_repeat("\t", $level - 1) . "->condition(db_and()";
        $closing_parenthesis = TRUE;
      }
      break;
    case 'or':
      $return[] = str_repeat("\t", $level - 1) . "->condition(db_or()";
      $closing_parenthesis = TRUE;
      break;
    case 'xor':
      $return[] = str_repeat("\t", $level - 1) . "->condition(db_xort()";
      $closing_parenthesis = TRUE;
      break;
  }

  // Convert conjunction.
  if (in_array($conditionals['op'], array('and', 'or', 'xor'))) {
    $return = array_merge($return, coder_upgrade_convert_conditional_fragment($conditionals['arg_1'], $values, $level));
    $return = array_merge($return, coder_upgrade_convert_conditional_fragment($conditionals['arg_2'], $values, $level));
  }
  // Convert single conditional fragment.
  else {
    $return = array_merge($return, coder_upgrade_convert_conditional_fragment($conditionals, $values, $level));
  }

  if ($closing_parenthesis) {
    $return[] = str_repeat("\t", $level - 1) . ")";
  }

  return $return;
}

/**
 * Transform conditional fragment into conditional object string.
 *
 * If conditional fragment is a subclause, pass it back to 
 * coder_upgrade_convert_conditional.
 *
 * @param $conditionals
 *   Parsed conditional fragment array.
 * @param $values
 *   Placeholder values
 * @param $level
 *   Level of conditional statements.
 *
 * @return
 *   Array of conditional object string.
 */
function coder_upgrade_convert_conditional_fragment($fragment, &$values = array(), $level = 1) {
  if ($fragment['arg_1']['type'] == 'subclause') {
    return coder_upgrade_convert_conditional($fragment['arg_1']['value'], $values, $level + 1);
  }
  else {
    $output = str_repeat("\t", $level) . "->condition('" . $fragment['arg_1']['value'];
    if ($fragment['arg_2']['type'] == 'placeholder') {
      $value = array_shift($values);

      // IN operator now just takes an array, rather than imploded array.
      if ($fragment['op'] == 'in' && preg_match("/db_placeholders\(([^,]*).*\)/", $value, $matches)) {
        $value = $matches[1];
      }
    }
    else {
      $value = $fragment['arg_2']['value'];
    }
    $output .=  "', $value";

    // Add operator if necessary.
    if ($fragment['op'] != '=') {
      $output .= ", '" . $fragment['op'] . "'";
    }

    $output .= ")";

    return array($output);
  }
}
