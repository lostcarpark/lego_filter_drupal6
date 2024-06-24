<?php

define("REBRICKABLE_API_URL", "https://rebrickable.com/api/v3/lego/");
define("BRICKSET_API_URL", "https://brickset.com/api/v3.asmx");

define("BRICKSET_PART_URL", "https://brickset.com/parts/design-");
define("BRICKLINK_PART_URL", "https://www.bricklink.com/v2/catalog/catalogitem.page?P=");
define("BRICKOWL_PART_URL", "https://www.brickowl.com/search/catalog?query=");

define('DAY', 24 * 60 * 60);
define('FORTNIGHT', 14 * DAY);
define('MONTH', 30 * DAY);

// The order to rank sources in. Lower is more desirable.
const SOURCES = ['BrickLink', 'Rebrickable', 'LEGO', 'Brickset', 'LDraw', 'BrickOwl'];


require_once("brickcat_api.inc");

function _lego_filter_process(&$body, $format = -1) {
  // Define Lego tags
  $preg = array(
    // Convert part no to picture
    '#\[part\:\s*([0-9a-zA-Z]+)?\s*\]#sie' => '_lego_part_tag(\'\\1\')',
    '#\[part\:\s*([0-9a-zA-Z]+)?[,; ]\s*colo[u]?r\:\s*([-._0-9a-zA-Z ]+)\]#sie' => '_lego_part_tag(\'\\1\', \'\\2\')',
    '#\[part\:\s*([0-9a-zA-Z]+)?\s*[,;]\s*([-._0-9a-zA-Z ]+)\]#sie' => '_lego_part_tag(\'\\1\', \'\\2\')',
    '#\[element\:\s*([0-9]+)?\s*\]#sie' => '_lego_element_tag(\'\\1\')',
    '#\[set\:\s*([0-9]+)?\s*\]#sie' => '_lego_set_tag(\'\\1-1\')',
    '#\[set\:\s*([0-9]+\-[0-9]+)?\s*\]#sie' => '_lego_set_tag(\'\\1\')',
  );
  $body = preg_replace(array_keys($preg), array_values($preg), $body);

  return $body;
}


/*
 * Functions for converting part tags.
 */

function _lego_part_tag($part = NULL, $color = NULL) {
  // If part already saved, insert snippit directly in page.
  $key_name = 'lego_filter_parts_'.$part;
  if (variable_get($key_name, FALSE)) {
    return _lego_part_snippit($part, $color, FALSE);
  }
  // Display placeholder for JavaScript to replace with part image to allow for
  // delay fetching...
  return '<div class="brick-filter-part" part="'.$part.'" color="'.$color.'">Fetching part '.$part.'...</div>';
}

function _lego_part_snippit($part = NULL, $color = NULL, $sleep = TRUE) {
  $lego_part = _lego_fetch_part($part, FALSE, $sleep);
  if (isset($lego_part)) {
    // Use Rebrickable ID in further searches.
    $part = $lego_part->part_num;
    $part_img_url = $lego_part->part_img_url;
    $colorname = '';
    $element = '';
    $rebrickable_link = '';
    $brickset_link = '';
    $bricklink_link = '';
    $brickowl_link = '';
    if (!is_null($color)) {
      $colors = _lego_fetch_colors();
      // If color name, look up
      if (!is_numeric($color)) {
        $lower = trim(strtolower($color));
        if (isset($colors->names[$lower]))
          $color = $colors->names[$lower]->id;
      }
      // Check color exists.
      if (is_numeric($color) && isset($colors->ids[$color])) {
        $colorname = t('%color (%legocolor)', ['%color' => $colors->ids[$color]->name, '%legocolor' => $colors->ids[$color]->legoname]).' - ';
        if (isset($lego_part->colors[$color]))
          $part_img_url = $lego_part->colors[$color]->part_img_url;
        if (isset($lego_part->colors[$color]->elements[0])) {
          if (count($lego_part->colors[$color]->elements) > 1)
            $element = '<br />'.t('Elements: %element', ['%element' => implode(', ', $lego_part->colors[$color]->elements)]);
          else
            $element = '<br />'.t('Element: %element', ['%element' => $lego_part->colors[$color]->elements[0]]);
        } else {
          $colorname = t('[Part not found in color %color]', ['%color' => $colors->ids[$color]->name]).' - ';
        }
      } else {
        $colorname = t('[Color %color not found]', ['%color' => $color]).' - ';
      }
    }
    if (isset($lego_part->part_url))
      $rebrickable_link = '<span class="lego-filter-rebrickable-icon"><a href="'.$lego_part->part_url.'" title="Rebrickable"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/rebrickable_icon.png" alt="Rebrickable" />'.($lego_part->display_part_num != $lego_part->part_num ? $lego_part->part_num : '').'</a></span>';
    if (isset($lego_part->brickset_id))
      $brickset_link = '<span class="lego-filter-brickset-icon"><a href="'.BRICKSET_PART_URL.$lego_part->brickset_id.'" title="Brickset"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/brickset_icon.png" alt="Brickset" /></a></span>';
    $bricklink_link = '<span class="lego-filter-bricklink-icon"><a href="'.BRICKLINK_PART_URL.$lego_part->bricklink_id.'" title="BrickLink"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/bricklink_icon.png" alt="BrickLink" />'.($lego_part->display_part_num != $lego_part->bricklink_id ? $lego_part->bricklink_id : '').'</a></span>';
    $brickowl_link = '<span class="lego-filter-brickowl-icon"><a href="'.BRICKOWL_PART_URL.$lego_part->brickowl_id.'" title="Brick Owl"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/brickowl_icon.png" alt="Brick Owl" />'.($lego_part->display_part_num != $lego_part->brickowl_id ? $lego_part->brickowl_id : '').'</a></span>';
    // Get clickable Rebrickable icon.
    return '<div class="lego-filter-part"><a href="'.$part_img_url.'" rel="lightbox" title="'.$lego_part->name.'"><img src="'.$part_img_url.'" alt="'.$lego_part->name.'" class="lego-filter-part-image" /></a><br />'.$lego_part->display_part_num.': '.$colorname.$lego_part->name.' '.$rebrickable_link.$brickset_link.$bricklink_link.$brickowl_link.$element.'</div>';
  } else {
    return '<div>'.t('Unknown part: %part', ['%part' => $part]).'</div>';
  }
}

function _lego_fetch_part($part_num, $reset = FALSE, $sleep = TRUE) {
  $key_name = 'lego_filter_parts_'.$part_num;
  if (!$reset && ($lego_part = variable_get($key_name, FALSE))) {
    // Use cached part data.
    return $lego_part;
  } else {
    // Fetch the part data.
    $part_info = _lego_rebrickable_part_fetch($part_num, $sleep);
    if (isset($part_info->part_num)) {
      // Save the properties we need into a new object.
      $lego_part = (object) [
        'part_num' => $part_info->part_num,
        'name' => $part_info->name,
        'part_url' => $part_info->part_url,
        'part_img_url' => $part_info->part_img_url,
      ];
      // If LEGO ID available, use it in display.
      if (is_numeric($part_info->part_num))
        $lego_part->display_part_num = $part_info->part_num;
      elseif (isset($part_info->external_ids->LEGO[0]) && is_numeric($part_info->external_ids->LEGO[0]))
        $lego_part->display_part_num = $part_info->external_ids->LEGO[0];
      else
        $lego_part->display_part_num = $part_info->part_num;
      // Look up BrickLink ID, if different.
      if (isset($part_info->external_ids->BrickLink[0]))
        $lego_part->bricklink_id = $part_info->external_ids->BrickLink[0];
      else
        $lego_part->bricklink_id = $part_info->part_num;
      // Look up BrickOwl ID, if different.
      if (isset($part_info->external_ids->BrickOwl[0]))
        $lego_part->brickowl_id = $part_info->external_ids->BrickOwl[0];
      else
        $lego_part->brickowl_id = $part_info->part_num;
      // Check if part exists on BrickSet for part number.
      if (is_numeric($part_info->part_num))
        $lego_part->brickset_id = $part_info->part_num;
      elseif (isset($part_info->external_ids->LEGO[0]) && is_numeric($part_info->external_ids->LEGO[0]))
        $lego_part->brickset_id = $part_info->external_ids->LEGO[0];
      // Now fetch all colours for the part.
      $color_info = _lego_rebrickable_part_all_colors_fetch($part_num, $sleep);
      if ($color_info) {
        $colors = [];
        foreach ($color_info->results as $color) {
          $colors[$color->color_id] = (object) [
            'color_name' => $color->color_name,
            'part_img_url' => $color->part_img_url,
            'elements' => $color->elements,
          ];
          foreach ($color->elements as $element_id) {
            _lego_store_element($element_id, $part_info->part_num, $color->color_id);
          }
        }
        // Save part to cache - only if both part and color fetch successful.
        $lego_part->colors = $colors;
        variable_set($key_name, $lego_part, 'cache');
        _lego_save_refresh_time($key_name, FORTNIGHT);
      }
      return $lego_part;
    } else {
      return NULL;
    }
  }
}

function _lego_fetch_part_color($part_num, $color, $reset = FALSE, $sleep = TRUE) {
  $key_name = 'lego_filter_parts_'.$part_num.'_color_'.$color;
  if (!$reset && ($cache = cache_get($key_name))) {
    // Use cached part data.
    return $cache->data;
  } else {
    // Fetch the part data.
    $part_color = _lego_rebrickable_part_color_fetch($part_num, $color, $sleep);
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
      cache_set($key_name, $lego_part, 'cache', time() + _lego_cache_interval());
      return $lego_part;
    } else {
      return NULL;
    }
  }
}

function _lego_color_table() {
  $colors = _lego_fetch_colors();
  $table = '<table>';
  $table .= '<tr><th>'.t('Color ID').'</th><th>'.t('Color name').'</th><th>'.t('LEGO name').'</th><th>'.t('Alternative names').'</th><th>'.t('Sample').'</th></tr>';
  foreach($colors->ids as $color) {
    $table .= '<tr><td>'.$color->id.'</td><td>'.$color->name.'</td><td>'.$color->legoname.'</td><td>'.$color->alternatives.'</td><td style="background-color: #'.$color->rgb.';">&nbsp;</td></tr>';
  }
  $table .= '</table>';
  return $table;
}

function _lego_fetch_colors($reset = FALSE, $sleep = TRUE) {
  static $lego_colors;
  if (!isset($lego_colors) || $reset) {
    $key_name = 'lego_filter_colors';
    if (!$reset && ($lego_colors = variable_get($key_name, FALSE))) {
      // Use cached color data, so no action needed.
    } else {
      // No cached data so fetch from Rebrickable.
      $colors = _lego_rebrickable_colors_fetch($sleep);
      $ids = [];
      $names = [];
      foreach ($colors->results as $color) {
        // Store the AFOL name and LEGO name under the ID.
        $legoname = $color->external_ids->LEGO->ext_descrs[0][0];
        $ids[$color->id] = (object) ['id' => $color->id, 'name' => $color->name, 'legoname' => $legoname, 'rgb' => $color->rgb];
        $lower_name = strtolower($color->name);
        $lower_lego = strtolower($legoname);
        $names[$lower_name] = (object) ['id' => $color->id]; // Always store the Rebrickable name, so they will take priority.
        // Iterate through all known alternative names.
        $alt_names = '';
        $alt_sep = '';
        foreach($color->external_ids as $exts)
          foreach($exts->ext_descrs as $alts)
            foreach($alts as $altname) {
              $lower_alt = strtolower($altname); // Index under lower case names for case insensitive lookups.
              if (!empty($lower_alt) && !isset($names[$lower_alt])) // Only add name if not already present, so won't override a Rebrickable name with an alternative.
                $names[$lower_alt] = (object) ['id' => $color->id];
              if (!empty($altname) && $lower_alt != $lower_name && $lower_alt != $lower_lego) {
                $alt_names .= $alt_sep . $altname;
                $alt_sep = ', ';
              }
            }
        $ids[$color->id]->alternatives = $alt_names;
      }
      // Finally, add our own name variants.
      $ids[71]->alternatives .= ", LBG, Light Bley, LtBley, Bley";
      $names['lbg'] = (object) ['id' => 71];
      $names['light bley'] = (object) ['id' => 71];
      $names['ltbley'] = (object) ['id' => 71];
      $names['bley'] = (object) ['id' => 71];
      $ids[72]->alternatives .= ", DBG, Dark Bley, DkBley";
      $names['dbg'] = (object) ['id' => 72];
      $names['dark bley'] = (object) ['id' => 72];
      $names['dkbley'] = (object) ['id' => 72];
      ksort($names);
      // Combine IDs and names into object.
      $lego_colors = (object) ['ids' => $ids, 'names' => $names];
      // Store in the cache for 24 hours.
      _lego_save_refresh_time($key_name, DAY);
      variable_set($key_name, $lego_colors);
    }
  }
  return $lego_colors;

}

function _lego_rebrickable_part_fetch($part_no, $sleep = TRUE) {

  // Store the best match so far.
  $best_part = NULL;
  $best_key = NULL;

  // Use the Rebrickable part search call. This allows multiple external IDs to be matched with a single call.
  $results = _lego_json_fetch(_lego_rebrickable_search_part_url($part_no), $sleep);
  if (!empty($results) && $results->count > 0) {
    // There could be several parts returned (e.g. printed parts will generally also return the unprinted version.
    // Loop through found parts to check at least one part number matches.
    foreach ($results->results as $part) {
      // Match on Rebrickable part no.
      if ($part->part_num == $part_no) {
        // Find the key in the ranking, and see if it's lower than current ranking.
        $key = array_search('Rebrickable', SOURCES);
        if (!is_null($key) && (is_null($best_key) || $key < $best_key)) {
          $best_part = $part;
          $best_key = $key;
        }
      }
      // Check if any external ID matches.
      if (isset($part->external_ids)) {
        // Loop through all external systems.
        foreach ($part->external_ids as $source => $extids) {
          // Loop through all IDs for external system.
          foreach ($extids as $extid) {
            // Check if ID is exact match.
            if ($extid == $part_no) {
              // Find the key in the ranking, and see if it's lower than current ranking.
              $key = array_search($source, SOURCES);
              if (!is_null($key) && (is_null($best_key) || $key < $best_key)) {
                $best_part = $part;
                $best_key = $key;
              }
            }
          }
        }
      }
    }
    return $best_part;
  }

  // Nothing found, so return null.
  return NULL;

}


/*
 * Functions for converting element tag.
 */

function _lego_element_tag($element_id = NULL) {
  // If element already stored, show element directly
  $key_name = 'lego_filter_element_'.$element_id;
  if ($element_info = variable_get($key_name, FALSE)) {
    return _lego_element_snippit($element_id, FALSE);
  }
  // Display placeholder for JavaScript to replace with element image...
  return '<div class="brick-filter-element" element="'.$element_id.'">Fetching element '.$element_id.'...</div>';
}

function _lego_element_snippit($element_id = NULL, $sleep = TRUE) {
  $element_info = _lego_fetch_element($element_id, $sleep);
  if (isset($element_info)) {
    return _lego_part_snippit($element_info->part_num, $element_info->color_id, $sleep);
  } else {
    return '<div>'.t('Unknown element: %element', ['%element' => $element_id]).'</div>';
  }
}

function _lego_fetch_element($element_id, $reset = FALSE, $sleep = TRUE) {
  $key_name = 'lego_filter_element_'.$element_id;
  if (!$reset && ($element_info = variable_get($key_name, FALSE))) {
    // Use cached element data.
    return $element_info;
  } else {
    // Fetch the element data.
    $element_info = _lego_rebrickable_element_fetch($element_id, $sleep);

    if (isset($element_info->part) && isset($element_info->part->part_num)) {
      return _lego_store_element($element_id, $element_info->part->part_num, $element_info->color->id);
    }
  }
  return NULL;
}

function _lego_store_element($element_id, $part_num, $color_id) {
  $key_name = 'lego_filter_element_' . $element_id;
  $element_info = (object) [
    'part_num' => $part_num,
    'color_id' => $color_id,
  ];
  variable_set($key_name, $element_info);
  return $element_info;
}

function _lego_rebrickable_element_fetch($element, $sleep = TRUE) {

  // Use the Rebrickable part search call. This allows multiple external IDs to be matched with a single call.
  $results = _lego_json_fetch(_lego_rebrickable_element_url($element), $sleep);
  if (!empty($results) && !empty($results->part)) {
    return $results;
  }

  // Nothing found, so return null.
  return NULL;

}


/*
 * Functions for converting set tag.
 */


function _lego_set_tag($set_num = NULL) {
  $key_name = 'lego_filter_sets_'.$set_num;
  if (variable_get($key_name, FALSE)) {
    return _lego_set_snippit($set_num, FALSE);
  }
  // Display placeholder for JavaScript to replace with part image...
  return '<div class="brick-filter-set" set="'.$set_num.'">Fetching set '.$set_num.'...</div>';
}

function _lego_set_snippit($set = NULL, $sleep = TRUE) {
  $lego_set = _lego_fetch_set($set, $sleep);
  if (isset($lego_set->number)) {
    $rebrickable_link = '';
    $brickset_link = '';
    $bricklink_link = '';
    $brickowl_link = '';
    $lego_link = '';
    $brickcat_link = '';
    if (isset($lego_set->bricksetURL))
      $brickset_link = '<span class="lego-filter-brickset-icon"><a href="'.$lego_set->bricksetURL.'" title="Brickset"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/brickset_icon.png" alt="Brickset" /></a></span>';
    if (isset($lego_set->rebrickableURL))
      $rebrickable_link = '<span class="lego-filter-rebrickable-icon"><a href="'.$lego_set->rebrickableURL.'" title="Rebrickable"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/rebrickable_icon.png" alt="Rebrickable" /></a></span>';
    $bricklink_link = '<span class="lego-filter-bricklink-icon"><a href="'.'http://www.bricklink.com/v2/search.page?q='.$lego_set->number.'&utm_source=brick.ie'.'" title="BrickLink"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/bricklink_icon.png" alt="BrickLink" /></a></span>';
    $brickowl_link = '<span class="lego-filter-brickowl-icon"><a href="'.'http://www.brickowl.com/search/catalog?query='.$lego_set->displayNumber.'&utm_source=brick.ie'.'" title="Brick Owl"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/brickowl_icon.png" alt="Brick Owl" /></a></span>';
    if (isset($lego_set->legoURL))
      $lego_link = '<span class="lego-filter-lego-icon"><a href="'.$lego_set->legoURL.'" title="LEGO®"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/lego_icon.png" alt="LEGO" /></a></span>';
    // if (isset($lego_set->brickcatURL))
    //   $brickcat_link = '<span class="lego-filter-brickcat-icon"><a href="'.$lego_set->brickcatURL.'" title="BrickCat"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/brickcat_icon.png" alt="BrickCat" /></a>'.(!empty($lego_set->brickcatPrice) ? $lego_set->brickcatCurrency.number_format($lego_set->brickcatPrice, 2, '.', ',') : '').'</span>';
    return '<div class="lego-filter-set"><a href="'.$lego_set->imageURL.'" rel="lightbox" title="'.$lego_set->name.'"><img src="'.$lego_set->largeThumbnailURL.'" alt="'.$lego_set->name.'" class="lego-filter-set-image" /></a><br />'.$lego_set->displayNumber.': '.$lego_set->name.' ('.$lego_set->year.') '.$brickset_link.$rebrickable_link.$bricklink_link.$brickowl_link.$lego_link.'</div>';
  } else {
    return '<div>'.t('Unknown set: %set', ['%set' => $set]).'</div>';
  }
}

function _lego_fetch_set($set_num, $reset = FALSE, $sleep = TRUE) {
  $key_name = 'lego_filter_sets_'.$set_num;
  if (!$reset && ($set_info = variable_get($key_name, FALSE))) {
    // Use cached set data.
    return $set_info;
  } else {
    // Fetch the set data.
    $result = _lego_brickset_set_fetch($set_num);
    if (isset($result->sets[0]->number)) {
      $set_info = (object) [
        'number' => (string)$result->sets[0]->number,
        'numberVariant' => (string)$result->sets[0]->numberVariant,
        'name' => (string)$result->sets[0]->name,
        'year' => (string)$result->sets[0]->year,
        'theme' => (string)$result->sets[0]->theme,
        'themeGroup' => (string)$result->sets[0]->themeGroup,
        'subtheme' => (string)$result->sets[0]->subtheme,
        'thumbnailURL' => (string)$result->sets[0]->image->thumbnailURL,
        'largeThumbnailURL' => (string)$result->sets[0]->image->thumbnailURL,
        'imageURL' => (string)$result->sets[0]->image->imageURL,
        'bricksetURL' => (string)$result->sets[0]->bricksetURL,
      ];
      // Combine number and variant into display number.
      $set_info->displayNumber = $set_info->number.'-'.$set_info->numberVariant;
      // Add some information from Rebrickable...
      $result = _lego_rebrickable_set_fetch($set_num, $sleep);
      if (isset($result->set_num)) {
        $set_info->rebrickableURL = $result->set_url;
      }
    }
    else {
      // Set not found on Brickset. See if we can get from Rebrickable...
      $result = _lego_rebrickable_set_fetch($set_num, $sleep);
      if (isset($result->set_num)) {
        $set_num = explode('-', $result->set_num);
        $set_info = (object) [
          'number' => $set_num[0],
          'numberVariant' => $set_num[1],
          'displayNumber' => $result->set_num,
          'name' => $result->name,
          'year' => $result->year,
          'thumbnailURL' => $result->set_img_url,
          'largeThumbnailURL' => $result->set_img_url,
          'imageURL' => $result->set_img_url,
          'rebrickableURL' => $result->set_url,
        ];

      }
    }
    _lego_save_refresh_time($key_name, MONTH);
    variable_set($key_name, $set_info);
    return $set_info;
    // If a set was found from either Briskset or Rebrickable,
    // if (!empty($set_info) && !empty($set_info->number)) {
    //   $brickCat = new BrickCatApi(variable_get("brickcat_api_key", ''));
    //   $result = $brickCat->getBestPrice($set_info->number, variable_get("brickcat_country", ''));
    //   if ($result->status) {
    //     $set_info->brickcatURL = $result->link;
    //     $set_info->brickcatPrice = $result->price;
    //     if (isset($result->currency))
    //       $set_info->brickcatCurrency = $result->currency;
    //     else
    //       $set_info->brickcatCurrency = $result->curreny;
    //     $result = $brickCat->getLegoUrl($set_info->number, variable_get("brickcat_country", ''));
    //     if ($result->status)
    //       $set_info->legoURL = $result->link;
    //   }
    // } else {
    //   return NULL;
    // }
  }
}

/*
 * Functions for API calls.
 */


function _lego_rebrickable_part_url($part) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."parts/?part_num=$part&key=$key";

}

function _lego_rebrickable_lego_part_url($part) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."parts/?lego_id=$part&key=$key";

}

function _lego_rebrickable_bricklink_part_url($part) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."parts/?bricklink_id=$part&key=$key";

}

function _lego_rebrickable_brickowl_part_url($part) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."parts/?brickowl_id=$part&key=$key";

}

function _lego_rebrickable_search_part_url($part) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."parts/?page_size=500&search=$part&key=$key";

}

function _lego_rebrickable_element_url($element) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."elements/$element/?key=$key";

}

function _lego_rebrickable_part_color_fetch($part, $color, $sleep = TRUE) {

  return _lego_json_fetch(_lego_rebrickable_part_color_url($part, $color), $sleep);

}

function _lego_rebrickable_part_color_url($part, $color) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."parts/$part/colors/$color/?key=$key";

}

function _lego_rebrickable_part_all_colors_fetch($part, $sleep = TRUE) {

  return _lego_json_fetch(_lego_rebrickable_part_all_colors_url($part), $sleep);

}

function _lego_rebrickable_part_all_colors_url($part) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."parts/$part/colors/?key=$key&page=1&page_size=1000";

}

function _lego_rebrickable_colors_fetch($sleep = TRUE) {

  return _lego_json_fetch(_lego_rebrickable_colors_url(), $sleep);

}

function _lego_rebrickable_colors_url() {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."/colors/?key=$key&page_size=500";

}

function _lego_rebrickable_set_url($set) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."sets/$set/?key=$key";

}

function _lego_rebrickable_set_fetch($set, $sleep = TRUE) {

  return _lego_json_fetch(_lego_rebrickable_set_url($set), $sleep);

}

function _lego_brickset_check_url() {

  $key = variable_get("brickset_api_key", '');
  return BRICKSET_API_URL."/checkKey?apiKey=$key";

}

function _lego_brickset_set_url($set) {

  $key = variable_get("brickset_api_key", '');
  return BRICKSET_API_URL."/getSets?apiKey=$key&userHash=&params={\"setNumber\":\"$set\"}";

}

function _lego_brickset_set_fetch($set_no) {

  $set_info = _lego_json_fetch(_lego_brickset_set_url($set_no), FALSE);
  return $set_info;

}


function _lego_json_fetch($url, $sleep=TRUE) {

  // Sleep for between 1 and 10 seconds to spread out requests.
  if ($sleep)
    sleep(rand(1, 10));
  while ($json = @file_get_contents($url)) {
    if (preg_match('/200/', $http_response_header[0])) {
      $part_info = json_decode($json);
      return $part_info;
    } else if (preg_match('/429/', $http_response_header[0])) {
      // 429 return code means too many calls. Sleep for a random number of seconds and try again.
      sleep(rand(5, 30));
    } else {
      return null;
    }
  }

}

function _lego_xml_fetch($url) {

  $xml = @file_get_contents($url);
  if (preg_match('/200/', $http_response_header[0])) {
    $object = simplexml_load_string($xml);
    return $object;
  } else {
    return null;
  }

}

function _lego_brickset_verify_url($url) {

  $data = @file_get_contents($url);
  if (preg_match('/200/', $http_response_header[0])) {
    if (preg_match('/<div class="results">0 to 0/', $data))
      return FALSE;
    else
      return TRUE;
  } else {
    return FALSE;
  }

}

function _lego_cache_interval() {
  $min = 3600 * 12; // Minimum 12 hours.
  $max = 3600 * 24; // Maximum 24 hours.
  return rand($min, $max);
}

function _lego_save_refresh_time($item_key, $interval) {
  $fetch_time = time();
  $refresh_time = $fetch_time + $interval;
  db_query(
    "insert into {lego_filter_refresh_time} (item_key, fetch_time, refresh_time) values ('%s', '%d', %d) on duplicate key update fetch_time = %d, refresh_time = %d",
    $item_key,
    $fetch_time,
    $refresh_time,
    $fetch_time,
    $refresh_time
  );
}

