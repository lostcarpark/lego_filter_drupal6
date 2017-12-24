<?php

function _lego_filter_tip() {
  return t("<a id=\"filter-lego\"></a><h2>LEGO Filter Guide</h2>

<p>
 LEGO set and part codes.
</p>
<p>
 Set codes: use [set:XXXXX] or [set:XXXXX-N] where XXXXX is set number and N is variant. N is assumed to be 1 if not specified.
</p>
<p>
 Part codes: use [part:XXXXX] where XXXXX is part number.
</p>
");

}
