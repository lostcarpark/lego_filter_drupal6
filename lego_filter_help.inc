<?php

function _lego_filter_tip() {

  include_once(drupal_get_path('module', 'lego_filter') .'/lego_filter_filter.inc');
  $table = _lego_color_table();

  return t("<h2>LEGO Filter Guide</h2>
<h3>LEGO part codes.</h3>
<p>
 Use [part:XXXXX] where XXXXX is part number. Part image will display with default color.
</p>
<p>
For part with color, use any of the following:
</p>
<ul>
  <li>[part:XXXXX color:YYYY]</li>
  <li>[part:XXXXX colour:YYYY]</li>
  <li>[part:XXXXX; YYYY]</li>
  <li>[part:XXXXX, YYYY]</li>
</ul>
<p>
where XXXXX is part number and YYYY is color name or number from the following list:
</p>
!table
", ['!table' => $table]);

/*
<p>
 Set codes: use [set:XXXXX] or [set:XXXXX-N] where XXXXX is set number and N is variant. N is assumed to be 1 if not specified.
</p>
*/

}
