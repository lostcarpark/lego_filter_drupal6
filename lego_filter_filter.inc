<?php

define("REBRICKABLE_API_URL", "https://rebrickable.com/api/v3/lego/");
define("BRICKSET_API_URL", "https://brickset.com/api/v2.asmx");

define("BRICKSET_PART_URL", "https://brickset.com/parts/design-");
define("BRICKLINK_PART_URL", "https://www.bricklink.com/v2/catalog/catalogitem.page?P=");
define("BRICKOWL_PART_URL", "https://www.brickowl.com/search/catalog?query=");


require_once("brickcat_api.inc");

function _lego_filter_process(&$body, $format = -1) {
  // Define Lego tags
  $preg = array(
    // Convert part no to picture
    '#\[part\:\s*([0-9a-zA-Z]+)?\s*\]#sie' => '_lego_part_tag(\'\\1\')',
    '#\[part\:\s*([0-9a-zA-Z]+)?[,; ]\s*colo[u]?r\:\s*([-._0-9a-zA-Z ]+)\]#sie' => '_lego_part_tag(\'\\1\', \'\\2\')',
    '#\[part\:\s*([0-9a-zA-Z]+)?\s*[,;]\s*([-._0-9a-zA-Z ]+)\]#sie' => '_lego_part_tag(\'\\1\', \'\\2\')',
    '#\[set\:\s*([0-9]+)?\s*\]#sie' => '_lego_set_tag(\'\\1-1\')',
    '#\[set\:\s*([0-9]+\-[0-9]+)?\s*\]#sie' => '_lego_set_tag(\'\\1\')',
  );
  $body = preg_replace(array_keys($preg), array_values($preg), $body);
  
  return $body;
}


/*
 * Functions for converting part tags.
 */

function _lego_part_tag($part = NULL, $color = NULL)
{
  // Display placeholder for JavaScript to replace with part image...
  return '<div class="brick-filter-part" part="'.$part.'" color="'.$color.'">Fetching part '.$part.'...</div>';
}

function _lego_part_snippit($part = NULL, $color = NULL)
{
  $lego_part = _lego_fetch_part($part);
  //dpm($lego_part);
  if (isset($lego_part)) {
    // Use Rebrickable ID in further searches.
    $part = $lego_part->part_num;
    $colorname = '';
    $element = '';
    $rebrickable_link = '';
    $brickset_link = '';
    $bricklink_link = '';
    $brickowl_link = '';
    if (!empty($color)) {
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
          if (isset($color_info->elements[0])) {
            if (count($color_info->elements) > 1)
              $element = '<br />'.t('Elements: %element', ['%element' => implode(', ', $color_info->elements)]);
            else
              $element = '<br />'.t('Element: %element', ['%element' => $color_info->elements[0]]);
          }
        } else {
          $colorname = t('[Part not found in color %color]', ['%color' => $colors->ids[$color]]).' - ';
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
    return '<div class="lego-filter-part"><a href="'.$lego_part->part_img_url.'" rel="lightbox" title="'.$lego_part->name.'"><img src="'.$lego_part->part_img_url.'" alt="'.$lego_part->name.'" class="lego-filter-part-image" /></a><br />'.$lego_part->display_part_num.': '.$colorname.$lego_part->name.' '.$rebrickable_link.$brickset_link.$bricklink_link.$brickowl_link.$element.'</div>';
  } else {
    return '<div>'.t('Unknown part: %part', ['%part' => $part]).'</div>';
  }
}

function _lego_fetch_part($part_num, $reset = FALSE) {
  $cache_key = 'lego_filter_parts_'.$part_num;
  if (!$reset && ($cache = cache_get($cache_key))) {
    // Use cached part data.
    return $cache->data;
  } else {
    // Fetch the part data.
    $part_info = _lego_rebrickable_part_fetch($part_num);
    //dpm($part_info);
    if (isset($part_info->part_num)) {
      // Save the properties we need into a new object.
      $lego_part = (object) [
        'part_num' => $part_info->part_num,
        'name' => $part_info->name,
        'part_url' => $part_info->part_url,
        'part_img_url' => $part_info->part_img_url,
      ];
      // If LEGO ID available, us it in display.
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
      // Save part to cache.
      cache_set($cache_key, $lego_part, 'cache', time() + _lego_cache_interval());
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
      cache_set($cache_key, $lego_part, 'cache', time() + _lego_cache_interval());
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

function _lego_fetch_colors($reset = FALSE) {
  static $lego_colors;
  if (!isset($lego_colors) || $reset) {
    if (!$reset && ($cache = cache_get('lego_filter_colors'))) {
      // Use cached color data.
      $lego_colors = $cache->data;
    } else {
      // No cached data so fetch from Rebrickable.
      $colors = _lego_rebrickable_colors_fetch();
//dd($colors);
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
      cache_set('lego_filter_colors', $lego_colors, 'cache', time() + _lego_cache_interval());
    }
  }
  return $lego_colors;

}

function _lego_rebrickable_part_fetch($part_no) {

  $part_info = _lego_json_fetch(_lego_rebrickable_part_url($part_no));
  if (!isset($part_info->part_num)) {
    $results = _lego_json_fetch(_lego_rebrickable_lego_part_url($part_no));
    if ($results->count > 0)
      $part_info = $results->results[0];
  }
  if (!isset($part_info->part_num)) {
    $results = _lego_json_fetch(_lego_rebrickable_bricklink_part_url($part_no));
    if ($results->count > 0)
      $part_info = $results->results[0];
  }
  if (!isset($part_info->part_num)) {
    $results = _lego_json_fetch(_lego_rebrickable_brickowl_part_url($part_no));
    if ($results->count > 0)
      $part_info = $results->results[0];
  }
  return $part_info;

}


/*
 * Functions for converting part set.
 */


function _lego_set_tag($set = NULL) {
  // Display placeholder for JavaScript to replace with part image...
  return '<div class="brick-filter-set" set="'.$set.'">Fetching set '.$set.'...</div>';
}

function _lego_set_snippit($set = NULL)
{
  $lego_set = _lego_fetch_set($set);
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
    $bricklink_link = '<span class="lego-filter-bricklink-icon"><a href="'.'http://www.bricklink.com/v2/search.page?q='.$lego_set->number.'&utm_source=brick.ie'.'" title="BrickLink"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/bricklink_icon.png" alt="BrickLink" />'.($lego_part->display_part_num != $lego_part->bricklink_id ? $lego_part->bricklink_id : '').'</a></span>';
    $brickowl_link = '<span class="lego-filter-brickowl-icon"><a href="'.'http://www.brickowl.com/search/catalog?query='.$lego_set->displayNumber.'&utm_source=brick.ie'.'" title="Brick Owl"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/brickowl_icon.png" alt="Brick Owl" />'.($lego_part->display_part_num != $lego_part->brickowl_id ? $lego_part->brickowl_id : '').'</a></span>';
    if (isset($lego_set->legoURL))
      $lego_link = '<span class="lego-filter-lego-icon"><a href="'.$lego_set->legoURL.'" title="LEGO®"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/lego_icon.png" alt="LEGO" /></a></span>';
    if (isset($lego_set->brickcatURL))
      $brickcat_link = '<span class="lego-filter-brickcat-icon"><a href="'.$lego_set->brickcatURL.'" title="BrickCat"><img src="/'.drupal_get_path('module', 'lego_filter').'/images/brickcat_icon.png" alt="BrickCat" /></a>'.(!empty($lego_set->brickcatPrice) ? $lego_set->brickcatCurrency.number_format($lego_set->brickcatPrice, 2, '.', ',') : '').'</span>';
    return '<div class="lego-filter-set"><a href="'.$lego_set->imageURL.'" rel="lightbox" title="'.$lego_set->name.'"><img src="'.$lego_set->largeThumbnailURL.'" alt="'.$lego_set->name.'" class="lego-filter-set-image" /></a><br />'.$lego_set->displayNumber.': '.$lego_set->name.' ('.$lego_set->year.') '.$brickset_link.$rebrickable_link.$bricklink_link.$brickowl_link.$lego_link.$brickcat_link.'</div>';
  } else {
    return '<div>'.t('Unknown set: %set', ['%set' => $set]).'</div>';
  }
}

function _lego_fetch_set($set_num, $reset = FALSE) {
  $cache_key = 'lego_filter_sets_'.$set_num;
  if (!$reset && ($cache = cache_get($cache_key))) {
    // Use cached set data.
    return $cache->data;
  } else {
    // Fetch the set data.
    $result = _lego_brickset_set_fetch($set_num);
    if (isset($result->sets->number)) {
      $set_info = (object) [
        'number' => (string)$result->sets->number,
        'numberVariant' => (string)$result->sets->numberVariant,
        'name' => (string)$result->sets->name,
        'year' => (string)$result->sets->year,
        'theme' => (string)$result->sets->theme,
        'themeGroup' => (string)$resultset_info->sets->themeGroup,
        'subtheme' => (string)$result->sets->subtheme,
        'thumbnailURL' => (string)$result->sets->thumbnailURL,
        'largeThumbnailURL' => (string)$result->sets->largeThumbnailURL,
        'imageURL' => (string)$result->sets->imageURL,
        'bricksetURL' => (string)$result->sets->bricksetURL,
      ];
      // Combine number and variant into display number.
      $set_info->displayNumber = $set_info->number.'-'.$set_info->numberVariant;
      // Add some information from Rebrickable...
      $result = _lego_rebrickable_set_fetch($set_num);
      if (isset($result->set_num)) {
        $set_info->rebrickableURL = $result->set_url;
      }
      $brickCat = new BrickCatApi(variable_get("brickcat_api_key", ''));
      $result = $brickCat->getBestPrice($set_info->number, variable_get("brickcat_country", ''));
      if ($result->status) {
        $set_info->brickcatURL = $result->link;
        $set_info->brickcatPrice = $result->price;
        if (isset($result->currency))
          $set_info->brickcatCurrency = $result->currency;
        else
          $set_info->brickcatCurrency = $result->curreny;
        $result = $brickCat->getLegoUrl($set_info->number, variable_get("brickcat_country", ''));
        if ($result->status)
          $set_info->legoURL = $result->link;
      }
      cache_set($cache_key, $set_info, 'cache', time() + _lego_cache_interval());
      return $set_info;
    } else {
      return NULL;
    }
  }
}

/*
 * Functions for API calls.
 */


function _lego_rebrickable_part_url($part) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."parts/$part/?key=$key";

}

function _lego_rebrickable_lego_part_url($part) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."parts/?lego_id=$part;key=$key";

}

function _lego_rebrickable_bricklink_part_url($part) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."parts/?bricklink_id=$part;key=$key";

}

function _lego_rebrickable_brickowl_part_url($part) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."parts/?brickowl_id=$part;key=$key";

}

function _lego_rebrickable_part_color_fetch($part, $color) {

  return _lego_json_fetch(_lego_rebrickable_part_color_url($part, $color));

}

function _lego_rebrickable_part_color_url($part, $color) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."parts/$part/colors/$color/?key=$key";

}

function _lego_rebrickable_colors_fetch() {

  return _lego_json_fetch(_lego_rebrickable_colors_url());

}

function _lego_rebrickable_colors_url() {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."/colors/?key=$key&page_size=500";

}

function _lego_rebrickable_set_url($set) {

  $key = variable_get("rebrickable_api_key", '');
  return REBRICKABLE_API_URL."sets/$set/?key=$key";

}

function _lego_rebrickable_set_fetch($set) {

  return _lego_json_fetch(_lego_rebrickable_set_url($set));

}

function _lego_brickset_set_url($set) {

  $key = variable_get("brickset_api_key", '');
  return BRICKSET_API_URL."/getSets?apiKey=$key&userHash=&query=&theme=&subtheme=&setNumber=$set&year=&owned=&wanted=&orderBy=&pageSize=&pageNumber=&userName=";

}

function _lego_brickset_set_fetch($set_no) {

  $set_info = _lego_xml_fetch(_lego_brickset_set_url($set_no));
  return $set_info;

}


function _lego_json_fetch($url) {

  // Sleep for between 1 and 10 seconds to spread out requests.
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

