<?php

/**
 * @file
 * Provides a custom Last Updated widget.
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => 'Last Updated',
  'description' => 'Will display a list of Last Updated activities.',
  'render callback' => 'contentwidget_lastupdated_widget_render',
  'settings form' => 'contentwidget_lastupdated_widget_settings',
  'settings storage' => 'contentwidget_lastupdated_widget_settings_storage',
  'resource api path' => 'gis/last-updated',
  'defaults' => array(
    'period' => '12',
    'count' => '10',
    'sort' => 'default',
    'visible_columns' => array(),
  ),
);

function contentwidget_lastupdated_widget_retrieve_data($plugin, $widget_settings, $langcode = NULL) {
  global $language;
  $data = array();

  $api_status = ampAPIRestClient::status();
  if (!$api_status['online']) {
    return $data;
  }

  // We actually don't want to have undefined language, so we fallback on the active language.
  if (empty($langcode) || $langcode == LANGUAGE_NONE) {
    $langcode = $language->language;
  }
  $cache_id = "contentblock:widget:{$langcode}:lastupdated:" . md5(serialize($widget_settings));

  // Try to get the processed response from cache.
  if ($cached = cache_get($cache_id, 'cache')) {
   return $cached->data;
  }

  // Prepare a list of parameters for the request.
  $options = array();
  $options['headers'] = array(
    'Accept' => 'application/json',
    'Content-Type' => 'application/json',
  );

  if (!empty($widget_settings['count'])) {
    $options['parameters']['count'] = $widget_settings['count'];
  }

  $request_body = array();

  // Get the API settings.
  $api_settings = helpergeneric_get_api_settings();

  // Handle the global "Humanitarian Aid" filter.
  // @see helpergeneric_api_settings_form()
  if (!empty($api_settings['humanitarian_aid_only'])) {
    $humanitarian_aid_plugin = ampapi_get_mapper_by_property_name('humanitarian_aid');
    $class_name = ctools_plugin_get_class($humanitarian_aid_plugin, 'handler');
    /** @var activityMapperOptionsInterface $handler */
    $handler = new $class_name();

    // Only display data related to humanitarian aid.
    $ha_key = $humanitarian_aid_plugin['report field key'];
    $ha_value = $handler->getMatchedValue($humanitarian_aid_plugin, TRUE);
    $request_body['filters'][$ha_key] = array($ha_value);
  }

  // Handle the global "Disaster Response Marker" filter.
  // @see helpergeneric_api_settings_form()
  if (!empty($api_settings['disaster_response_marker_only'])) {
    $disaster_response_marker_plugin = ampapi_get_mapper_by_property_name('disaster_response_marker');
    $class_name = ctools_plugin_get_class($disaster_response_marker_plugin, 'handler');
    /** @var activityMapperOptionsInterface $handler */
    $handler = new $class_name();

    // Only display data related to humanitarian aid.
    $drm_key = $disaster_response_marker_plugin['report field key'];
    $drm_value = $handler->getMatchedValue($disaster_response_marker_plugin, TRUE);
    $request_body['filters'][$drm_key] = array($drm_value);
  }

  // JSON Encode the request body. Can't use JSON_FORCE_OBJECT.
  $options['body'] = empty($request_body) ? '{}' : json_encode($request_body);

  // Get the data.
  $use_cache = FALSE;  // We cache the processed data, not the request.
  $response_data = ampAPIRestClient::post($plugin['resource api path'], $options, $use_cache);
  if (empty($response_data)) {
    return array();
  }

  // 1 hours cache life time for requests should be plenty for now.
  $expire = 3600;
  cache_set($cache_id, $response_data, 'cache', REQUEST_TIME + $expire);

  return $response_data;
}

/**
 * Render callback for the 'Last Updated' predefined widget.
 */
function contentwidget_lastupdated_widget_render($plugin, $widget_settings, $entity, $langcode) {
  // Provide some default values.
  $widget_settings += $plugin['defaults'];

  $data = contentwidget_lastupdated_widget_retrieve_data($plugin, $widget_settings, $langcode);

  $header = array();
  if (!empty($data)) {
    if (empty($data->headers) ||  empty($data->headers[0])) {
      drupal_set_message(t('Headers information missing from the API.'), 'error');
      return NULL;
    }

    // Create the table headers.
    foreach ($data->headers[0] as $key => $header_text) {
      $header[$key] = array(
        'data' => t(check_plain($key)),
        'class' => array(
          drupal_html_class('col-' . $key)
        ),
      );
    }
  }

  $rows = array();
  if (isset($data->activities)) {
    $rows = _contentwidget_lastupdated_get_data_rows($data->activities);
  }

  $build = array();
  $build['lastupdated'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#attributes' => array(
      'class' => array(
        'contentblock-widget',
        'contentblock-widget-lastupdated',
        'styled-lightblue-table',
      ),
    ),
    '#empty' => t('No data available.'),
    '#sticky' => FALSE,
    '#prefix' => '<div class="large-table-wrapper">',
    '#suffix' => '</div>',
  );

  return array('#markup' => drupal_render($build));
}

/**
 * Settings form for the 'Last Updated' predefined widget.
 */
function contentwidget_lastupdated_widget_settings(&$element, &$form_state, $plugin, $widget_settings) {
  $settings_form = array();

  // Provide some default values.
  $widget_settings += $plugin['defaults'];
  $form_state['lastupdated'] = array(
    'field_name' => $element['#field_name'],
    'language' => $element['#language'],
    'input_name_prefix' => $element['#field_name'] . '[' . $element['#language'] . '][settings][lastupdated]',
  );
  $form_state['lastupdated']['wrapper_id'] = drupal_html_class($form_state['lastupdated']['input_name_prefix'] . '-options');

  $settings_form['count'] = array(
    '#type' => 'select',
    '#title' => t('Projects Count'),
    '#description' => t('The number of projects to display in the widget.'),
    '#default_value' => $widget_settings['count'],
    '#options' => drupal_map_assoc(array(2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20)),
  );

  return $settings_form;
}

/**
 * Returns a list of rows for theme_table().
 */
function _contentwidget_lastupdated_get_data_rows($items) {
  $rows = array();

  foreach ($items as $delta => $values) {
    $row = array();
    $count = 0;
    foreach ((array) $values as $value) {
      $row_value = check_plain($value);
      $row[$count] = array(
        'data' => $row_value,
        'class' => array(
          drupal_html_class('col-' . $count)
        ),
      );
      $count++;
    }

    if (!empty($row)) {
      $rows[$delta] = $row;
    }
  }

  return $rows;
}

/**
 * Returns an serialized string for storing the widget settings in the database.
 */
function contentwidget_lastupdated_widget_settings_storage($widget_name, $form_values) {
  $settings = array();

  if (isset($form_values[$widget_name]) && is_array($form_values[$widget_name])) {
    $settings = $form_values[$widget_name];
  }

  return serialize($settings);
}
