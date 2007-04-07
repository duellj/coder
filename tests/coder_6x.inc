<?php
// $Id: coder_6x.inc,v 1.2 2007-04-07 21:14:47 douggreen Exp $

function this_is_not_a_menu_hook() {
  $items = array();
  $items[] = array(); // this should not generate an error
  return $items;
}
