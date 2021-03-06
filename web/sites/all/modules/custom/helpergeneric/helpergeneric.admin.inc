<?php

/**
 * @file
 * This file contains form/UI cleanup functions.
 */

/**
 * Page callback for 'admin/settings/ui'.
 */
function helpergeneric_ui_settings_form($form, &$form_state) {
  $cleanup_ui_force = helpergeneric_cleanup_ui();

  $form['cleanup_ui'] = array(
    '#type' => 'fieldset',
    '#title' => t('UI settings'),
    '#description' => '<p>' . t('By default the UI is cleaned, you can disable this behaviour and have access to more options. This is only as a UI cleanup functionality and not a security feature.') . '</p><p>' . t('<strong>DO NOT CLICK</strong> on these buttons unless you were told to!') . '</p>',
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );

  $form['cleanup_ui']['enable'] = array(
    '#type' => 'button',
    '#value' => 'Enable',
    '#disabled' => $cleanup_ui_force,
    '#submit' => array('helpergeneric_toggle_cleanup_ui_force_submit'),
    '#executes_submit_callback' => TRUE,
  );

  $form['cleanup_ui']['disable'] = array(
    '#type' => 'button',
    '#value' => 'Disable',
    '#disabled' => !$cleanup_ui_force,
    '#submit' => array('helpergeneric_toggle_cleanup_ui_force_submit'),
    '#executes_submit_callback' => TRUE,
  );

  return $form;
}

/**
 * Submit handler for helpergeneric_ui_settings_form().
 */
function helpergeneric_toggle_cleanup_ui_force_submit($form, &$form_state) {
  $params = session_get_cookie_params();
  if ($form_state['triggering_element']['#value'] == t('Enable')) {
    // Enable UI cleanup.
    setcookie('helpergeneric_cleanup_ui_force', 1, REQUEST_TIME + (60 * 60 * 12), $params['path'], $params['domain'], $params['secure'], $params['httponly']);
  }
  else {
    // Disable UI cleanup.
    setcookie('helpergeneric_cleanup_ui_force', 0, REQUEST_TIME + (60 * 60 * 12), $params['path'], $params['domain'], $params['secure'], $params['httponly']);
  }
}

/**
 * Page callback for 'admin/settings/api'.
 */
function helpergeneric_api_settings_form($form, &$form_state) {
  // We use the same default value for restclient_hostname as the restclient.module
  $restclient_hostname = variable_get('restclient_hostname', 'http://localhost:80/rest');
  if (empty($restclient_hostname)) {
    drupal_set_message(t('API Server Hostname is not configured!'), 'warning');
  }

  // Do not display API status page when the page is submitted.
  if (empty($form_state['input'])) {
    $api_status = ampAPIRestClient::status(FALSE);
    if ($api_status['online']) {
      drupal_set_message(t('The API Server %url is online!', array('%url' => $restclient_hostname)), 'status');
    }
  }

  $form['hostname'] = array(
    '#type' => 'fieldset',
    '#title' => t('API Server Hostname'),
    '#description' => t('Enter the hostname of the server.'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );

  $form['hostname']['restclient_hostname'] = array(
    '#type' => 'textfield',
    '#title' => t('Hostname'),
    '#description' => t('The hostname for the server with no trailing slash. Ex: http://example.org/rest'),
    '#size' => 40,
    '#default_value' => $restclient_hostname,
  );

  // Provide an alternative link for links to AMP.
  $amp_hostname = variable_get('helpergeneric_amp_hostname', NULL);
  $form['hostname']['alternative_hostname'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use alternative hostname for Activity view links'),
    '#default_value' => !empty($amp_hostname),
    '#description' => t('If you are using localhost to access the API check here to use an alternative hostname for the Activity view links.'),
  );

  $form['hostname']['wrapper'] = array(
    '#type' => 'container',
    '#states' => array(
      'visible' => array(
        ":input[name=\"alternative_hostname\"]" => array('checked' => TRUE),
      ),
    ),
  );

  $form['hostname']['wrapper']['helpergeneric_amp_hostname'] = array(
    '#type' => 'textfield',
    '#title' => t('AMP Hostname'),
    '#description' => t('The hostname for the AMP environment, without a trailing slash. Ex: http://amp212.local'),
    '#size' => 40,
    '#default_value' => $amp_hostname,
  );

  // Get the API settings.
  $api_settings = helpergeneric_get_api_settings();

  $form['helpergeneric_api_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('API Settings'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
    '#tree' => TRUE,
  );

  // Allow administrator to limit all API queries to only show
  // "Humanitarian Aid" related data.
  $form['helpergeneric_api_settings']['humanitarian_aid_only'] = array(
    '#type' => 'checkbox',
    '#title' => t('Only show "Humanitarian Aid" related data'),
    '#default_value' => $api_settings['humanitarian_aid_only'],
  );

  // Allow administrator to limit all API queries to only show
  // "Disaster Response Marker" related data.
  $form['helpergeneric_api_settings']['disaster_response_marker_only'] = array(
    '#type' => 'checkbox',
    '#title' => t('Only show "Disaster Response Marker" related data'),
    '#default_value' => $api_settings['disaster_response_marker_only'],
  );

  $form['#validate'][] = 'helpergeneric_api_settings_form_validate';
  $form['#submit'][] = 'helpergeneric_api_settings_form_submit';

  return system_settings_form($form);
}

/**
 * Validation callback for helpergeneric_api_settings_form().
 */
function helpergeneric_api_settings_form_validate($form, &$form_state) {
  if (empty($form_state['values']['alternative_hostname'])) {
    $form_state['values']['helpergeneric_amp_hostname'] = NULL;
  }
  unset($form_state['values']['alternative_hostname']);
}

/**
 * Submit callback for helpergeneric_api_settings_form().
 */
function helpergeneric_api_settings_form_submit($form, &$form_state) {
  // Clear widget cache.
  $cache_id = 'contentblock:widget:';
  cache_clear_all($cache_id, 'cache', TRUE);
}

/**
 * Page callback for 'admin/settings/generic'.
 */
function helpergeneric_settings_form($form, &$form_state) {
  global $language;
  $language_aware = drupal_multilingual();

  $form['clear_cache'] = array(
    '#type' => 'fieldset',
    '#title' => t('Clear cache'),
  );

  $form['clear_cache']['clear'] = array(
    '#type' => 'submit',
    '#value' => t('Clear all caches'),
    '#submit' => array('helpergeneric_settings_form_clear_cache_submit'),
    '#prefix' => '<p>',
    '#suffix' => '</p>',
  );

  $form['site_information'] = array(
    '#type' => 'fieldset',
    '#title' => t('Site details'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  if ($language_aware) {
    $site_name = i18n_variable_get('site_name', $language->language, 'Drupal');
  }
  else {
    $site_name = variable_get('site_name', 'Drupal');
  }
  $form['site_information']['site_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Site name'),
    '#description' => t('The site name will be transformed to uppercase before displaying in the header.'),
    '#default_value' => $site_name,
    '#required' => TRUE
  );
  if ($language_aware) {
    $form['site_information']['site_name']['#description'] .= t('<br /><strong>This value is language-aware. Current language: @lang</strong>', array(
      '@lang' => $language->name,
    ));
  }
  // $form['site_information']['site_slogan'] = array(
  //   '#type' => 'textfield',
  //   '#title' => t('Slogan'),
  //   '#default_value' => variable_get('site_slogan', ''),
  //   '#description' => t("How this is used depends on your site's theme."),
  // );
  $form['site_information']['site_mail'] = array(
    '#type' => 'textfield',
    '#title' => t('E-mail address'),
    '#default_value' => variable_get('site_mail', ini_get('sendmail_from')),
    '#description' => t("The <em>From</em> address in automated e-mails sent during registration and new password requests, and other notifications. (Use an address ending in your site's domain to help prevent this e-mail being flagged as spam.)"),
    '#required' => TRUE,
  );

  // Logo settings
  $form['theme_settings'] = array(
    '#tree' => TRUE,
  );
  $form['theme_settings']['logo'] = array(
    '#type' => 'fieldset',
    '#title' => t('Logo image settings'),
    '#description' => t("Your logo is displayed in the top left of the website header."),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['theme_settings']['logo']['default_logo'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use the default logo'),
    '#default_value' => theme_get_setting('default_logo'),
    '#description' => t('Check here if you want the theme to use the default logo.'),
  );
  $form['theme_settings']['logo']['settings'] = array(
    '#type' => 'container',
    '#states' => array(
      // Hide the logo settings when using the default logo.
      'invisible' => array(
        'input[name="theme_settings[logo][default_logo]"]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['theme_settings']['logo']['settings']['logo_upload'] = array(
    '#type' => 'file',
    '#title' => t('Upload new logo image'),
    '#maxlength' => 40,
    '#description' => t("Use this field to upload your logo. It should be uploaded at the exact size you want it displayed."),
    '#tree' => FALSE,
  );

  $form['theme_settings']['favicon'] = array(
    '#type' => 'fieldset',
    '#title' => t('Shortcut icon settings'),
    '#description' => t("Your shortcut icon, or 'favicon', is displayed in the address bar and bookmarks of most browsers."),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['theme_settings']['favicon']['default_favicon'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use the default shortcut icon.'),
    '#default_value' => theme_get_setting('default_favicon'),
    '#description' => t('Check here if you want the theme to use the default shortcut icon.'),
  );
  $form['theme_settings']['favicon']['settings'] = array(
    '#type' => 'container',
    '#states' => array(
      // Hide the favicon settings when using the default favicon.
      'invisible' => array(
        'input[name="theme_settings[favicon][default_favicon]"]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['theme_settings']['favicon']['settings']['favicon_upload'] = array(
    '#type' => 'file',
    '#title' => t('Upload new icon image'),
    '#description' => t("Use this field to upload your shortcut icon."),
    '#tree' => FALSE,
  );

  // Miscellaneous settings go here.
  // Just to have the right vars, prefix all with misc_.
  $form['misc'] = array(
    '#type' => 'fieldset',
    '#title' => t('Miscellaneous'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['misc']['misc_use_export_buttons'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show Export Buttons'),
    '#description' => t('If you want to hide the export functionality, uncheck this option.'),
    '#default_value' => variable_get('misc_use_export_buttons', 1),
  );
  $form['misc']['misc_use_reports'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Reports'),
    '#description' => t('If you want to enable the Reports page, check this box.'),
    '#default_value' => module_exists('ampreports'),
  );

  $form['#validate'][] = 'helpergeneric_settings_form_validate';
  $form['#submit'][] = 'helpergeneric_settings_form_submit';

  $form['actions']['#type'] = 'actions';
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save configuration'));

  return $form;
}

/**
 * Validates the submitted site-information form.
 */
function helpergeneric_settings_form_validate($form, &$form_state) {
  // Validate the e-mail address.
  if ($error = user_validate_mail($form_state['values']['site_mail'])) {
    form_set_error('site_mail', $error);
  }

  // Check for a new uploaded logo.
  $validators = array(
    'file_validate_extensions' => array('png svg'),
    'file_validate_image_resolution' => array('240x60'),
  );
  $file = file_save_upload('logo_upload', $validators);
  if (isset($file)) {
    // File upload was attempted.
    if ($file) {
      // Put the temporary file in form_values so we can save it on submit.
      $form_state['values']['theme_settings']['logo_upload'] = $file;
    }
    else {
      // File upload failed.
      form_set_error('logo_upload', t('The logo could not be uploaded.'));
    }
  }

  // Check for a new uploaded favicon.
  $validators = array(
    'file_validate_extensions' => array('ico png svg'),
  );
  $file = file_save_upload('favicon_upload', $validators);
  if (isset($file)) {
    // File upload was attempted.
    if ($file) {
      // Put the temporary file in form_values so we can save it on submit.
      $form_state['values']['theme_settings']['favicon_upload'] = $file;
    }
    else {
      // File upload failed.
      form_set_error('favicon_upload', t('The favicon could not be uploaded.'));
    }
  }
}

/**
 * Clear Cache Submit callback for helpergeneric_settings_form().
 */
function helpergeneric_settings_form_clear_cache_submit($form, &$form_state) {
  drupal_flush_all_caches();
  drupal_flush_all_caches();  // Just to make sure.

  drupal_set_message(t('Caches cleared.'));
}

/**
 * Saves the submitted site-information.
 */
function helpergeneric_settings_form_submit($form, &$form_state) {
  global $theme;
  global $language;

  foreach (array('site_name', 'site_mail', 'misc_use_export_buttons') as $var) {
    if (isset($form_state['values'][$var]))
      variable_set($var, $form_state['values'][$var]);
  }

  // Enable the feature as requested.
  if (isset($form_state['values']['misc_use_reports']) && $form_state['values']['misc_use_reports'] == TRUE) {
    module_enable(array('ampreports'));
    features_revert_module('ampreports');
    features_revert_module('ampcontact');
    cache_clear_all();
  }
  elseif (isset($form_state['values']['misc_use_reports']) && $form_state['values']['misc_use_reports'] == FALSE) {
    module_disable(array('ampreports'), FALSE);

    $view_name = 'reports';
    if ($view = views_get_view($view_name)) {
      views_delete_view($view);
    }

    drupal_flush_all_caches();
    menu_rebuild();
  }

  // i18n variable.
  if (drupal_multilingual()) {
    i18n_variable_set('site_name', $form_state['values']['site_name'], $language->language);
  }

  // @TODO: Cleanup!
  $values = $form_state['values']['theme_settings'];
  foreach (array('logo', 'favicon') as $setting) {
    if (isset($values[$setting]['settings'])) {
      $setting_values = $values[$setting]['settings'];
      unset($values[$setting]['settings']);
      $values = array_merge($values, $setting_values);
    }
    if (!empty($values[$setting])) {
      $setting_values = $values[$setting];
      unset($values[$setting]);
      $values = array_merge($values, $setting_values);
    }
  }

  // Always display logo and favicon.
  $values['toggle_logo'] = 1;
  $values['toggle_favicon'] = 1;

  // If the user uploaded a new logo or favicon, save it to a permanent location
  // and use it in place of the default theme-provided file.
  if (!empty($values['default_logo'])) {
    $values['default_logo'] = 1;
    $values['logo_path'] = '';
  }
  else if (isset($values['logo_upload']) && $file = $values['logo_upload']) {
    unset($values['logo_upload']);
    $filename = file_unmanaged_copy($file->uri);
    $values['default_logo'] = 0;
    $values['logo_path'] = $filename;
  }
  else {
    $values['default_logo'] = 0;
    $values['logo_path'] = theme_get_setting('logo_path', $theme);
  }
  if (!empty($values['default_favicon'])) {
    $values['default_favicon'] = 1;
    $values['favicon_path'] = '';
  }
  else if (isset($values['favicon_upload']) && $file = $values['favicon_upload']) {
    unset($values['favicon_upload']);
    $filename = file_unmanaged_copy($file->uri);
    $values['default_favicon'] = 0;
    $values['favicon_path'] = $filename;
  }
  else {
    $values['default_favicon'] = 0;
    $values['favicon_path'] = theme_get_setting('favicon_path', $theme);
  }

  $var = 'theme_' . $theme . '_settings';
  variable_set($var, $values);
}

/**
 * Page callback for 'admin/settings/language'.
 *
 * @see locale_languages_overview_form()
 */
function helpergeneric_language_settings_form($form, &$form_state) {
  drupal_static_reset('language');
  $languages = language_list();

  $options = array();
  $form['weight'] = array('#tree' => TRUE);
  $enabled_languages = array();
  foreach ($languages as $langcode => $language) {
    $options[$langcode] = '';
    if ($language->enabled) {
      $enabled_languages[] = $langcode;
    }
    $form['weight'][$langcode] = array(
      '#type' => 'weight',
      '#title' => t('Weight for @title', array('@title' => $language->name)),
      '#title_display' => 'invisible',
      '#default_value' => $language->weight,
      '#attributes' => array('class' => array('language-order-weight')),
    );
    $form['name'][$langcode] = array('#markup' => check_plain($language->name));
    $form['native'][$langcode] = array('#markup' => check_plain($language->native));
    $form['direction'][$langcode] = array('#markup' => ($language->direction == LANGUAGE_RTL ? t('Right to left') : t('Left to right')));
  }
  $form['enabled'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Enabled languages'),
    '#title_display' => 'invisible',
    '#options' => $options,
    '#default_value' => $enabled_languages,
  );
  $form['site_default'] = array(
    '#type' => 'radios',
    '#title' => t('Default language'),
    '#title_display' => 'invisible',
    '#options' => $options,
    '#default_value' => language_default('language'),
  );

  $form['form_description'] = array(
    '#markup' => t('Language specific character sets are only loaded when the language is enable, that is why certain characters might have a different font style.'),
    '#prefix' => '<div class="description">',
    '#suffix' => '</div>',
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save configuration'));
  $form['#theme'] = 'helpergeneric_language_settings_form';

  drupal_set_message('Changing default language might cause existing inbound links to stop working.', 'warning');
  return $form;
}

/**
 * Returns HTML for the language settings form.
 *
 * @see theme_locale_languages_overview_form()
 */
function theme_helpergeneric_language_settings_form($variables) {
  $form = $variables['form'];
  $default = language_default();

  $rows = array();
  foreach ($form['name'] as $key => $element) {
    // Do not take form control structures.
    if (is_array($element) && element_child($key)) {
      // Disable checkbox for the default language, because it cannot be disabled.
      if ($key == $default->language) {
        $form['enabled'][$key]['#attributes']['disabled'] = 'disabled';
      }

      // Add invisible labels for the checkboxes and radio buttons in the table
      // for accessibility. These changes are only required and valid when the
      // form is themed as a table, so it would be wrong to perform them in the
      // form constructor.
      $title = drupal_render($form['name'][$key]);
      $form['enabled'][$key]['#title'] = t('Enable !title', array('!title' => $title));
      $form['enabled'][$key]['#title_display'] = 'invisible';
      $form['site_default'][$key]['#title'] = t('Set !title as default', array('!title' => $title));
      $form['site_default'][$key]['#title_display'] = 'invisible';
      $rows[] = array(
        'data' => array(
          '<strong>' . $title . '</strong>',
          drupal_render($form['native'][$key]),
          check_plain($key),
          drupal_render($form['direction'][$key]),
          array('data' => drupal_render($form['enabled'][$key]), 'align' => 'center'),
          drupal_render($form['site_default'][$key]),
          drupal_render($form['weight'][$key])
        ),
        'class' => array('draggable'),
      );
    }
  }
  $header = array(
    array('data' => t('English name')),
    array('data' => t('Native name')),
    array('data' => t('Code')),
    array('data' => t('Direction')),
    array('data' => t('Enabled')),
    array('data' => t('Default')),
    array('data' => t('Weight')),
  );
  $output = theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'language-order')));
  $output .= drupal_render_children($form);

  drupal_add_tabledrag('language-order', 'order', 'sibling', 'language-order-weight');

  return $output;
}


/**
 * Process language overview form submissions, updating existing languages.
 *
 * @see locale_languages_overview_form_submit()
 */
function helpergeneric_language_settings_form_submit($form, &$form_state) {
  $languages = language_list();
  $default = language_default();
  $url_prefixes = variable_get('locale_language_negotiation_url_part', LOCALE_LANGUAGE_NEGOTIATION_URL_PREFIX) == LOCALE_LANGUAGE_NEGOTIATION_URL_PREFIX;

  $enabled_count = 0;
  $disabled_languages = array();
  foreach ($languages as $langcode => $language) {
    if ($form_state['values']['site_default'] == $langcode || $default->language == $langcode) {
      // Automatically enable the default language and the language
      // which was default previously (because we will not get the
      // value from that disabled checkbox).
      $form_state['values']['enabled'][$langcode] = 1;
    }

    // If language URL prefixes are enabled we must clear language domains and
    // assign a valid prefix to each non-default language.
    if ($url_prefixes) {
      $language->domain = '';
      if (empty($language->prefix) && $form_state['values']['site_default'] != $langcode) {
        $language->prefix = $langcode;
      }
    }

    if ($form_state['values']['enabled'][$langcode]) {
      $enabled_count++;
      $language->enabled = 1;
    }
    else {
      $language->enabled = 0;
      $disabled_languages[] = $langcode;
    }

    $language->weight = $form_state['values']['weight'][$langcode];

    db_update('languages')
      ->fields(array(
        'enabled' => $language->enabled,
        'weight' => $language->weight,
        'prefix' => $language->prefix,
        'domain' => $language->domain,
      ))
      ->condition('language', $langcode)
      ->execute();

    $languages[$langcode] = $language;
  }

  // Set disabled languages. Overrides language settings from ampi18n feature.
  // @see helpergeneric_locale_default_languages_alter()
  variable_set('helpergeneric_disabled_languages', $disabled_languages);

  variable_set('language_default', $languages[$form_state['values']['site_default']]);
  variable_set('language_count', $enabled_count);
  drupal_set_message(t('Configuration saved.'));

  // Changing the language settings impacts the interface.
  cache_clear_all('*', 'cache_page', TRUE);
  module_invoke_all('multilingual_settings_changed');

  return;
}
