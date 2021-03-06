<?php

/**
 * @file
 * Shared classes and interfaces for the API mapper system.
 */

class activityMapperAutocompleteBase extends activityMapperBase implements activityMapperAutocompleteInterface {
  private $extra_filters = array();

  public function autocomplete($plugin, $data, $type = 'autocomplete') {
    $info = $this->getAutocompleteInfo($plugin, $data, $type);
    if (empty($info)) {
      return array();
    }

    $returned_data = ampAPIRestClient::get($info['endpoint']);
    if (empty($returned_data)) {
      return array();
    }

    switch ($plugin['report filter endpoint']) {
      case 'filters/locations':
        // Include top level (the country name) in the autocomplete suggestions.
        $returned_data = array($returned_data);
        break;
      case 'filters/programs':
      case 'filters/sectors':
        // Do not include top level option in the autocomplete suggestions.
        /** @noinspection PhpUndefinedFieldInspection */
        $returned_data = $returned_data->children;
        break;
    }
    // Offer subclasses a possibility to filter-out the returned data.
    $returned_data = $this->filterData($info, $returned_data);
    return $this->getMatchedValues($info, $returned_data);
  }

  public function getAutocompleteInfo($plugin, $data, $type) {
    $info = array();

    if (in_array($type, array('autocomplete', 'tokeninput'))) {
      $info['string'] = reset($data);
    }
    else {
      $info['values'] = $data;
    }

    $info['plugin'] = $plugin;
    $info['type'] = $type;

    $info['endpoint'] = $plugin['report filter endpoint'];

    return $info;
  }

  public function getMatchedValues(array $info, array $entries, $top_level = TRUE) {
    global $language;
    $matches = array();

    $search_string = '';
    if (in_array($info['type'], array('autocomplete', 'tokeninput'))) {
      $search_string = transliteration_get($info['string'], '', $language->language);
    }
    foreach ($entries as $entry) {
      if (count($matches) >= 10) {
        break;
      }

      // Match full codes and codes starting with the provided string.
      $prepend_entry = FALSE;
      if ($info['plugin']['report filter endpoint'] == 'filters/sectors') {
        if (isset($entry->code) && is_numeric($search_string) && $search_string == $entry->code) {
          $prepend_entry = TRUE;
        }
      }

      if (!$this->_valueMatches($info, $entry, $search_string)) {
        continue;
      }

      // Include parent Location Administrative Levels in name, if any.
      $name = '';
      if ($info['plugin']['report filter endpoint'] == 'filters/locations') {
        if (isset($info['parents'])) {
          foreach ($info['parents'] as $parent) {
            if (empty($parent->top_level)) {
              $name .= filter_xss_admin($parent->name) . ' > ';
            }
          }
        }
        $id = $entry->filterId . ':' . intval($entry->id);
      }
      else {
        $id = intval($entry->id);
      }

      // Include acronym in name, if any.
      $name .= filter_xss_admin($entry->name);
      if (isset($entry->acronym)) {
        $name .= ' (' . filter_xss_admin($entry->acronym) . ')';
      }

      // This filtered property is set in cases we want to filter-out some values in filterData().
      if (!isset($entry->filtered) || $entry->filtered === FALSE) {
        $new_value = array(
          'id' => $id,
          'name' => $name,
        );

        if ($prepend_entry) {
          array_unshift($matches, $new_value);
        }
        else {
          $matches[] = $new_value;
        }
      }
    }

    // Search sub-levels, if any.
    foreach ($entries as $entry) {
      if (count($matches) >= 10) {
        break;
      }

      if (!empty($entry->children)) {
        // We need to change the info only for the current children.
        $temp_info = $info;
        if ($top_level) {
          $entry->top_level = TRUE;
        }
        $temp_info['parents'][] = $entry;
        $matches = array_merge($matches, $this->getMatchedValues($temp_info, $entry->children, FALSE));
      }
    }

    return $matches;
  }

  /**
   * Public function that filters out the data according the certain values in $info.
   *
   * @param array $info
   *   Info array.
   * @param  array $data
   *   Array of data that needs to be filtered.
   * @return array
   *   Filtered data.
   */
  public function filterData($info, $data) {
    return $data;
  }

  /**
   * Test if the entry object show be included in the returned values or not.
   */
  private function _valueMatches(array $info, $entry, $search_string) {
    global $language;

    switch ($info['type']) {
      case 'tokeninput':
      case 'autocomplete':
        $matches = FALSE;

        $entry_name = transliteration_get($entry->name, '', $language->language);
        if (stripos($entry_name, $search_string) !== FALSE) {
          $matches = TRUE;
        }

        if ($info['plugin']['report filter endpoint'] == 'filters/orgs') {
          if (isset($entry->acronym) && stripos($entry->acronym, $search_string) !== FALSE) {
            $matches = TRUE;
          }
        }

        // Filter out donors without funding.
        // if ($info['plugin']['property name'] == 'donor_agency') {
        //   if (empty($entry->hasFundings)) {
        //     $matches = FALSE;
        //   }
        // }

        if ($matches) {
          return TRUE;
        }
        break;
      case 'autocompleteids':
        if ($info['plugin']['property name'] == 'location') {
          $name = '';
          if (isset($info['parents'])) {
            foreach ($info['parents'] as $parent) {
              if (empty($parent->top_level)) {
                $name .= filter_xss_admin($parent->name) . ' > ';
              }
            }
            $name .= filter_xss_admin($entry->name);
          }
          else {
            $name = filter_xss_admin($entry->name);
          }
          if (in_array($name, $info['values'])) {
            return TRUE;
          }
        }
        else if ($entry->name == $search_string) {
          return TRUE;
        }
        break;

      case 'tokeninputids':
        $id = $entry->id;

        // Slight adjustment for locations.
        if ($info['endpoint'] == 'filters/locations') {
          $id = $entry->filterId . ':' . intval($entry->id);
        }


        if (in_array($id, $info['values'])) {
          return TRUE;
        }
        break;
    }

    return FALSE;
  }

  public function getAutocompleteValue($plugin, $data, $type) {
    $matches = $this->autocomplete($plugin, $data, $type);
    if (empty($matches)) {
      return NULL;
    }

    // Special case for autocomplete powered view filters.
    // Not using the jQuery TokenInput library.
    if ($type == 'autocompleteids') {
      $entry = reset($matches);
      if (isset($entry['id'])) {
        return array(intval($entry['id']));
      }
    }

    return $matches;
  }

  /**
   * Sets a value to the specified key (for extra filtering).
   */
  public function setExtraFilter($key, $value) {
    $this->extra_filters[$key] = $value;
  }

  /**
   * Gets the value for the key. If none exists, uses default one.
   */
  public function getExtraFilter($key, $default_value) {
    if (!empty($this->extra_filters[$key])) {
      return $this->extra_filters[$key];
    }
    return $default_value;
  }
}

/**
 * Class activityMapperSubFilterAutocomplete
 *
 * Use this mapper for generic filters that are actually children of other
 * filters, like:
 *  - "National Plan Objective" a sub-filter of "Programs"
 *  - "Secondary Sectors" a sub-filter of "Sectors".
 *  - "Beneficiary Agency" a sub-filter of "Sectors".
 */
class activityMapperSubFilterAutocomplete extends activityMapperAutocompleteBase {
  public function getAutocompleteInfo($plugin, $data, $type) {
    $info = parent::getAutocompleteInfo($plugin, $data, $type);

    $sub_filter_type_id = $this->_getSubFilterTypeId($plugin);

    if (is_null($sub_filter_type_id)) {
      return NULL;
    }

    // This is the case where we use URL-based filtering (by changing the endpoint).
    // Alternatively, see activityMapperAutocompleteBase::filterData().
    $info['sub_filter_id'] = $sub_filter_type_id;
    if (isset($plugin['report filter endpoint']) && $plugin['report filter endpoint'] != 'filters/orgs') {
      $info['endpoint'] = $plugin['report filter endpoint'] . '/' . $sub_filter_type_id;
    }

    return $info;
  }

  /**
   * {@inheritdoc}
   */
  public function filterData($info, $data) {
    // Apply global / parent filter initially.
    $filtered_data = parent::filterData($info, $data);

    // Now, for filters/orgs, we want to apply sub_filter_id filtering.
    if ($info['endpoint'] == 'filters/orgs' && isset($info['sub_filter_id'])) {
      foreach ($filtered_data as $fd_key => $row) {
        if (!in_array($info['sub_filter_id'], $row->rolesIds)) {
          unset($filtered_data[$fd_key]);
        }
      }
    }
    return $filtered_data;
  }

  private function _getSubFilterTypeId($plugin) {
    // The organization roles sub-filters have a different structure.
    // #ConsistentAPI.
    if (isset($plugin['report filter endpoint extra'])) {
      $endpoint = $plugin['report filter endpoint extra'];
    }
    else {
      $endpoint = $plugin['report filter endpoint'];
    }

    $sub_filter_data = ampAPIRestClient::get($endpoint);

    if (empty($sub_filter_data)) {
      return FALSE;
    }

    // Setup the new and correct filterId value.
    $look_up_filter = (isset($plugin['report filter key'])) ? $plugin['report filter key'] : '';

    foreach ($sub_filter_data as $info) {
      if (isset($info->filterId) && $info->filterId == $look_up_filter) {
        return $info->id;
      }
    }

    return NULL;
  }
}
