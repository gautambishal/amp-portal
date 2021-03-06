<?php

/**
 * AMP Reports custom views area handler.
 */
class ampreports_views_handler_area_result extends views_handler_area_result {

  /**
   * Define additional options.
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['chart'] = array(
      'default' => TRUE,
      'bool' => TRUE,
    );
    $options['chart_sectors'] = array(
      'default' => 8,
    );
    $options['chart_projects_length'] = array(
      'default' => 10,
    );
    unset($options['content']);
    return $options;
  }

  /**
   * Alter the form to include our newly defined options.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['chart'] = array(
      '#type' => 'checkbox',
      '#title' => t('Include the Google Chart'),
      '#description' => t('If you would like to render the Google Chart, check this box.'),
      '#default_value' => isset($this->options['chart']) ? $this->options['chart'] : TRUE,
    );
    $form['chart_sectors'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of Sectors to show'),
      '#description' => t('All of the other sectors will be Summed up together in the "Other" category.'),
      '#default_value' => isset($this->options['chart_sectors']) ? $this->options['chart_sectors'] : 8,
      '#dependency' => array('edit-options-chart' => array(TRUE)),
    );
    $form['chart_projects_length'] = array(
      '#type' => 'textfield',
      '#title' => t('Length of Project Names'),
      '#description' => t('Max length of the Projects Name.'),
      '#default_value' => isset($this->options['chart_projects_length']) ? $this->options['chart_projects_length'] : 10,
      '#dependency' => array('edit-options-chart' => array(TRUE)),
    );
    unset($form['content']);
    return $form;
  }

  /**
   * Take care of the rendering part.
   */
  function render($empty = FALSE) {
    global $user;
    $output = array();

    if (empty($this->view->exposed_input['location_dep']) && empty($this->view->exposed_input['location_mun'])) {
      return t('Please select a department or municipality.');
    }

    $department = $this->view->exposed_input['location_dep'];
    $municipality = !empty($this->view->exposed_input['location_mun']) ? $this->view->exposed_input['location_mun'] : FALSE;
    $status = !empty($this->view->exposed_input['status']) ? $this->view->exposed_input['status'] : 0;
    if (!empty($this->view->total_rows) && $this->view->total_rows !== 0) {
      $count_all = $this->view->total_rows;
    }
    else {
      $count_all = count($this->view->result);
    }

    $values['dep'] = $department;
    $values['mun'] = $municipality;
    $plugin = ampapi_get_mapper_by_property_name('location');
    $class_name = ctools_plugin_get_class($plugin, 'handler');
    $handler = new $class_name();
    $values_fetched = $handler->getAutocompleteValue($plugin, $values, 'tokeninputids');
    foreach ($values_fetched as $value_fetched) {
      if ($value_fetched['id'] == $values['dep']) {
        $department = $value_fetched;
      }
      if ($value_fetched['id'] == $values['mun']) {
        $municipality = $value_fetched;
      }
    }

    $output['stats'] = array(
      '#type' => 'container',
      '#attributes' => array('class' => array('stats-wrapper')),
    );
    if (!empty($department)) {
      if (!empty($municipality)) {
        $output['stats']['department'] = array(
          '#markup' => '<div class="stats-query">' . t('Department of @department, municipality @municipality', array(
            '@department' => $department['name'],
            '@municipality' => $municipality['name'],
          )) . '</div>',
        );
      }
      else {
        $output['stats']['department'] = array(
          '#markup' => '<div class="stats-query">' . t('Department of @department', array(
            '@department' => $department['name'],
          )) . '</div>',
          '#weight' => 0,
        );
      }
    }
    else {
      // The extra case, no department, just municipalities.
      if (!empty($municipality)) {
        $output['stats']['department'] = array(
          '#markup' => '<div class="stats-query">' . t('Municipality @municipality', array(
            '@municipality' => $municipality['name'],
          )) . '</div>',
        );
      }
    }

    // Let's find out, how many are 'in execution' (or, any value specified in the settings of 'status').
    $data = $this->get_data($this->view->build_info['efq_query']);
    if ($status == 0) {
      $view_clone = $this->view->clone_view();
      $new_exposed_input = $this->view->get_exposed_input();
      $new_exposed_input['status'] = 1;
      $view_clone->tag = 'charts';
      $view_clone->set_exposed_input($new_exposed_input);
      $view_clone->set_items_per_page('0');
      $view_clone->execute();
      $count_status = $view_clone->total_rows;

      // Load options.
      $status_value_ops = $this->view->filter['status']->get_value_options();

      if (is_array($this->view->filter['status']->value)) {
        $status_value_key = reset($this->view->filter['status']->value);
      }
      else {
        $status_value_key = $this->view->filter['status']->value;
      }
      $output['stats']['count_all'] = array(
        '#markup' => '<div class="count-all">' . t('Total number of projects:') .
            format_string('<div class="count-value">@count_all</div>', array(
              '@count_all' => $count_all,
            )) . '</div>',
        '#weight' => 5,
      );
      $output['stats']['count_status'] = array(
        '#markup' => '<div class="count-all">' . t('Total number of projects "@status":', array(
            '@status' => $status_value_ops[$status_value_key],
          )) . format_string('<div class="count-value">@count_status</div>', array(
            '@count_status' => $count_status,
          )) . '</div>',
        '#weight' => 10,
      );

      // OMG that's a pain, we NEED to revert.
      unset($new_exposed_input['status']);
      unset($this->view->exposed_raw_input['status']);
      $this->view->set_exposed_input($new_exposed_input);
    }
    else {
      // Load options.
      $status_value_ops = $this->view->filter['status']->get_value_options();
      $status_value_key = reset($this->view->filter['status']->value);

      $output['stats']['count_status'] = array(
        '#markup' => '<div class="count-all">' . t('Total number of projects "@status":', array(
            '@status' => $status_value_ops[$status_value_key],
          )) . format_string('<div class="count-value">@count_status</div>', array(
            '@count_status' => $count_all,
          )) . '</div>',
        '#weight' => 10,
      );
    }

    // Render the chart.
    if ($this->options['chart'] == TRUE) {
      $output['chart'] = array(
        '#markup' => '<div id="projects_chart"></div>',
      );
      $this->get_google_chart($data);
    }

    return render($output);
  }

  /**
   * Get the data, make the call.
   *
   * @return mixed
   *   The data made after the call.
   */
  function get_data($efq) {
    $data = array();
    $api_status = ampAPIRestClient::status();
    if (!$api_status['online']) {
      return $data;
    }

    $langcode = (empty($langcode) ? LANGUAGE_NONE : $langcode);
    $cache_id = "api:reports:{$langcode}:chart:" . $this->get_input_hash();
    // Try to get the processed response from cache.
    if ($cached = cache_get($cache_id, 'cache')) {
      return $cached->data;
    }

    // Prepare a list of options for the request.
    $options = array();
    $options['headers'] = array(
      'Accept' => 'application/json',
      'Content-Type' => 'application/json',
    );
    $request_body = array();
    $request_body['name'] = $cache_id;
    $request_body['page'] = -1;
    $request_body['recordsPerPage'] = -1;
    $request_body['regenerate'] = TRUE;
    $request_body['summary']  = TRUE;
    $request_body['groupingOption'] = 'T';
    $request_body['add_columns'][] = 'Primary Sector';
    $request_body['add_columns'][] = 'Activity Count';
    $request_body['add_measures'][] = 'Actual Commitments';
    $request_body['add_hierarchies'][] = 'Primary Sector';

    // Populate filters from EFQ.
    if (!empty($efq) && !empty($efq->propertyConditions)) {
      foreach ($efq->propertyConditions as $condition) {
        if ($condition['column'] === 'location') {
          foreach ($condition['value'] as $value) {
            list($filter_id, $filter_value) = explode(':', $value);
            $request_body['filters'][$filter_id] = $filter_value;
          }
        }
        else {
          $request_body['filters'][$condition['column']] = $condition['value'];
        }
      }
    }

    // JSON Encode the request body.
    $options['body'] = empty($request_body) ? '{}' : json_encode($request_body);

    // Get the data.
    $use_cache = FALSE;
    $endpoint = 'data/report/custom/paginate';
    $response_data = ampAPIRestClient::post($endpoint, $options, $use_cache);

    if (empty($response_data)) {
      return array();
    }

    // Now let's process it.
    $processed_data = array();
    foreach ($response_data->page->pageArea->children as $result) {
      $processed_data[$result->contents->{'[Primary Sector]'}->displayedValue] = $result->contents->{'[Activity Count]'}->displayedValue;
    }

    $expire = 3600;
    cache_set($cache_id, $processed_data, 'cache', REQUEST_TIME + $expire);

    return $processed_data;
  }

  /**
   * Returns a generated input hash.
   */
  private function get_input_hash() {
    return md5(implode(':', $this->view->exposed_input));
  }

  /**
   * Draw the chart.
   */
  function get_google_chart($results) {
    // Polish the results and structure them properly.
    $normalized_data = array();
    foreach ($results as $sector => $result) {
      if (strpos($sector, '.') !== FALSE) {
        list($id, $sector) = explode('.', $sector);
        $sector = trim($sector);
      }
      else if (strpos($sector, '-') !== FALSE) {
        list($id, $sector) = explode('-', $sector);
        $sector = trim($sector);
      }
      $sector = mb_convert_case($sector, MB_CASE_TITLE, "UTF-8");
      $normalized_data[$sector] = (int) $result;
    }
    arsort($normalized_data);
    $cutoff = $this->options['chart_sectors'];
    $showing = array_slice($normalized_data, 0, $cutoff);
    $rest = array_sum(array_slice($normalized_data, $cutoff));

    // Only if we have something left, we place all those under 'Other'.
    if ($rest != 0) {
      $all_sectors = $showing + array(t('Other') => $rest);
    }
    else {
      $all_sectors = $showing;
    }

    // Proper settings.
    $shorten_sectors = array();
    foreach ($all_sectors as $sector_name => $sector_data) {
      if (strlen($sector_name) > $this->options['chart_projects_length']) {
        $shorten_sectors[$sector_name] =  substr($sector_name, 0, $this->options['chart_projects_length']) . '...';
      }
      else {
        $shorten_sectors[$sector_name] = $sector_name;
      }
    }

    $header = array_keys($all_sectors);
    $rows = array_values($all_sectors);

    // Let's specify colors (generated) and add values to tooltips.
    $colors = array();
    $flat_colors = array(
      '#9b59b6',
      '#3498db',
      '#2ecc71',
      '#1abc9c',
      '#95a5a6',
      '#e74c3c',
      '#e67e22',
      '#d35400',
      '#f1c40f',
      '#7f8c8d',
      '#f39c12',
      '#bdc3c7'
    );

    shuffle($flat_colors);
    foreach ($header as $id => $sector) {
      $colors[$id] = $flat_colors[$id];
      $header[$id] = $sector . t(' - @num Projects', array(
        '@num' => $rows[$id]
      ));
    }

    $settings['chart']['projects_chart'] = array(
      'chartType' => 'ColumnChart',
      'containerId' => 'projects_chart',
      'columns' => array(t('Projects')),
      'header' => array_values($shorten_sectors),
      'rows' => array($rows, $header, $colors),
      'use_tooltips_column' => TRUE,
      'use_style_column' => TRUE,
      'input_hash' => $this->get_input_hash(),
      'options' => array(
        'forceIFrame' => FALSE,
        'title' => t('Projects by Sectors'),
        'width' => 1000,
        'height' => 600,
        'legend' => array('position' => 'none'),
        'vAxis' => array(
          'textStyle' => array(
            'fontSize' => '10',
          ),
        ),
        'hAxis' => array(
          'textStyle' => array(
            'fontSize' => '10',
          ),
        ),
      ),
    );

    // Now draw.
    _ampreports_custom_draw_chart($settings);
  }
}
