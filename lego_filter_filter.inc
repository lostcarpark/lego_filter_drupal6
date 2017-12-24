<?php

define("REBRICKABLE_URL", "https://rebrickable.com/api/v3/lego/");
define("BRICKLINK_PART_URL", "https://www.bricklink.com/v2/catalog/catalogitem.page?P=");

function _lego_filter_process(&$body, $format = -1) {
  // Define Lego tags
  $preg = array(
    // Convert part no to picture
    '#\[part\:\s*([0-9a-zA-Z]+)?\s*\]#sie' => '_lego_part_tag(\'\\1\')',
    '#\[part\:\s*([0-9a-zA-Z]+)?\s+colo[u]?r\:\s*([-_0-9a-zA-Z ]+)\]#sie' => '_lego_part_tag(\'\\1\', \'\\2\')',
    '#\[part\:\s*([0-9a-zA-Z]+)?\s*[,;]\s*([-_0-9a-zA-Z ]+)\]#sie' => '_lego_part_tag(\'\\1\', \'\\2\')',
  );
  $body = preg_replace(array_keys($preg), array_values($preg), $body);
  
  return $body;
}


function _lego_part_tag($part = NULL, $color = NULL) {
  $lego_part = _lego_fetch_part($part);
  //dpm($lego_part);
  if (isset($lego_part)) {
    $colorname = '';
    $element = '';
    $rebrickable_link = '';
    $bricklink_link = '';
    if (isset($color)) {
      $colors = _lego_fetch_colors();
      // If color name, look up
      if (!is_numeric($color)) {
        $lower = trim(strtolower($color));
        if (isset($colors->names[$lower]))
          $color = $colors->names[$lower]->id;
      }
      // Check color exists.
      if (is_numeric($color) && isset($colors->ids[$color])) {
        $color_info = _lego_fetch_part_color($part, $color);
        if (isset($lego_part)) {
          if (isset($color_info->part_img_url))
            $lego_part->part_img_url = $color_info->part_img_url;
          $colorname = t('%color (%legocolor)', ['%color' => $colors->ids[$color]->name, '%legocolor' => $colors->ids[$color]->legoname]).' - ';
          if (isset($color_info->elements[0]))
            $element = '<br />'.t('Element: %element', ['%element' => $color_info->elements[0]]);
        } else {
          $colorname = t('[Part not found in color %color]', ['%color' => $colors->ids[$color]]).' - ';
        }
      } else {
        $colorname = t('[Color %color not found]', ['%color' => $color]).' - ';
      }
    }
    if (isset($lego_part->part_url))
      $rebrickable_link = '<a href="'.$lego_part->part_url.'"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/rebrickable_icon.png" alt="Rebrickable" /></a>';
    $bricklink_link = '<a href="'.BRICKLINK_PART_URL.$lego_part->bricklink_id.'"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/bricklink_icon.png" alt="Bricklink" /></a>';
    // Get clickable Rebrickable icon.
    return '<div><img src="'.$lego_part->part_img_url.'" alt="'.$lego_part->name.'"><br /><b>'.$lego_part->part_num.': '.$colorname.$lego_part->name.' '.$rebrickable_link.$bricklink_link.$element.'</b></div>';
  } else {
    return '<div>'.t('Unknown part: %part', ['%part' => $part]).'</div>';
  }
}

function _lego_fetch_part($part_num, $reset = FALSE) {
  if (!$reset && ($cache = cache_get('lego_filter_parts_'.$part_num))) {
    // Use cached part data.
    return $cache->data;
  } else {
    // Fetch the part data.
    $part_info = _lego_rebrickable_part_fetch($part_num);
    if (isset($part_info->part_num)) {
      // Save the properties we need into a new object.
      $lego_part = (object) [
        'part_num' => $part_info->part_num,
        'name' => $part_info->name,
        'part_url' => $part_info->part_url,
        'part_img_url' => $part_info->part_img_url,
      ];
      if (isset($part_info->external_ids->BrickLink[0]))
        $lego_part->bricklink_id = $part_info->external_ids->BrickLink[0];
      else
        $lego_part->bricklink_id = $part_info->part_num;
      // Save part to cache.
      
      return $lego_part;
    } else {
      return NULL;
    }
  }
}

function _lego_fetch_part_color($part_num, $color, $reset = FALSE) {
  $cache_key = 'lego_filter_parts_'.$part_num.'_color_'.$color;
  
  if (!$reset && ($cache = cache_get($cache_key))) {
    // Use cached part data.
    return $cache->data;
  } else {
    // Fetch the part data.
    $part_color = _lego_rebrickable_part_color_fetch($part_num, $color);
    if (isset($part_color->num_sets)) {
      // Save the properties we need into a new object.
      $lego_part = (object) [
        'part_num' => $part_num,
        'color' => $color,
        'elements' => $part_color->elements,
      ];
      if (isset($part_color->part_img_url))
        $lego_part->part_img_url = $part_color->part_img_url;
      // Store in the cache for 24 hours.
      cache_set($cache_key, $lego_part, 'cache', time() + 86400);
      return $lego_part;
    } else {
      return NULL;
    }
  }
}

function _lego_fetch_colors($reset = FALSE) {
  static $lego_colors;
  if (!isset($lego_colors) || $reset) {
    if (!$reset && ($cache = cache_get('lego_filter_colors'))) {
      // Use cached color data.
      $lego_colors = $cache->data;
    } else {
      // No cached data so fetch from Rebrickable.
      $colors = _lego_rebrickable_colors_fetch();
      $ids = [];
      $names = [];
      foreach ($colors->results as $color) {
        // Store the AFOL name and LEGO name under the ID.
        $ids[$color->id] = (object) ['id' => $color->id, 'name' => $color->name, 'legoname' => $color->external_ids->LEGO->ext_descrs[0][0]];
        $names[strtolower($color->name)] = (object) ['id' => $color->id]; // Always store the Rebrickable name, so they will take priority.
        // Iterate through all known alternative names.
        foreach($color->external_ids as $exts)
          foreach($exts->ext_descrs as $descs)
            foreach($descs as $desc) {
              $lower = strtolower($desc); // Index under lower case names for case insensitive lookups.
              if (!empty($lower) && !isset($names[$lower])) // Only add name if not already present, so won't override a Rebrickable name with an alternative.
                $names[$lower] = (object) ['id' => $color->id];
            }
      }
      // Finally, add our own name variants.
      $names['lbg'] = (object) ['id' => 71];
      $names['light bley'] = (object) ['id' => 71];
      $names['ltbley'] = (object) ['id' => 71];
      $names['bley'] = (object) ['id' => 71];
      $names['dbg'] = (object) ['id' => 72];
      $names['dark bley'] = (object) ['id' => 72];
      $names['dkbley'] = (object) ['id' => 72];
      ksort($names);
      // Combine IDs and names into object.
      $lego_colors = (object) ['ids' => $ids, 'names' => $names];
      // Store in the cache for 24 hours.
      cache_set('lego_filter_colors', $lego_colors, 'cache', time() + 86400);
    }
  }
  return $lego_colors;

}

function _lego_rebrickable_part_fetch($part_no) {

  return _lego_json_fetch(_lego_rebrickable_part_url($part_no));

}

function _lego_rebrickable_part_url($part) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_URL."parts/$part/?key=$key";

}

function _lego_rebrickable_part_color_fetch($part, $color) {

  return _lego_json_fetch(_lego_rebrickable_part_color_url($part, $color));

}

function _lego_rebrickable_part_color_url($part, $color) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_URL."parts/$part/colors/$color/?key=$key";

}

function _lego_rebrickable_colors_fetch() {

  return _lego_json_fetch(_lego_rebrickable_colors_url());

}

function _lego_rebrickable_colors_url() {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_URL."/colors/?key=$key";

}

function _lego_json_fetch($url) {

  $json = @file_get_contents($url);
  if (preg_match('/200/', $http_response_header[0])) {
    $part_info = json_decode($json);
    //dpm($part_info);
    return $part_info;
  } else {
    return null;
  }

}
