<?php
/**
 * @file
 * Forms and helper functions for administrative pages.
 */

include_once 'contentblock.admin.inc';

/**
 * Implements hook_form_FORM_ID_alter() for contentblock_form().
 */
function _contentblock_form_contentblock_form_alter(&$form, &$form_state, $form_id) {
  // Get the type info.
  $cbreference_info = contentblock_get_fixed_types($form['#entity']->machine_name);

  // Only show available fields for predefined entities.
  foreach (element_children($form) as $element_child) {
    if (substr($element_child, 0, 6) !== 'field_') {
      continue;
    }

    if (!in_array($element_child, $cbreference_info['available_fields'])) {
      $form[$element_child]['#access'] = FALSE;
    }
  }

  // Add a description on add form.
  if ($form_state['op'] == 'add' || !in_array('field_title', $cbreference_info['available_fields'])) {
    $title_suffix = '';
    if (!empty($form['#entity']->language) && $form['#entity']->language != LANGUAGE_NONE) {
      $languages = language_list();
      $title_suffix = ' [' . t($languages[$form['#entity']->language]->name) . ']';
    }
    drupal_set_title($cbreference_info['title'] . $title_suffix);
  }

  // Add a default title value for sections.
  if (!empty($cbreference_info['is section']) && FALSE) {
    $langcode = isset($form['title_field']['#language']) ? $form['title_field']['#language'] : LANGUAGE_NONE;
    $title_element = &$form['title_field'][$langcode][0];
    if (empty($title_element['value']['#value'])) {
      $title_element['value']['#value'] = $cbreference_info['title'];
    }
  }

  // Hide the default title element.
  if (!in_array('title_field', $cbreference_info['available_fields'])) {
    // $form['title_field']['#attributes']['class'][] = 'element-hidden';
  }

  // Attach validate callbacks.
  switch ($form['#entity']->machine_name) {
    case 'linkedmap':
      $form['#validate'][] = '_contentblock_cbreference_entity_form_linkedmap_validate';
      break;
    case 'footer':
      $form['#validate'][] = '_contentblock_cbreference_entity_form_footer_validate';
      break;
    case 'welcome':
      $form['#validate'][] = '_contentblock_cbreference_entity_form_welcome_validate';
      break;
  }

  // Use the widget form if specified so.
  if (isset($cbreference_info['use widget form']) && $cbreference_info['use widget form'] == TRUE) {
    $plugin = contentwidget_get_predefined_widget($form['#entity']->machine_name);
    if ($function = ctools_plugin_get_function($plugin, 'settings form')) {
      $widget_settings = _contentwidget_fixed_get_settings($form['#entity']->machine_name);
      $form = $function($form, $form_state, $plugin, $widget_settings);
    }
  }
}

/**
 * Form validation handler for contentblock_cbreference_entity_form() for linkedmap.
 */
function _contentblock_cbreference_entity_form_linkedmap_validate($form, &$form_state) {
  $entity_type = $form_state['entity_type'];
  $entity = $form_state[$entity_type];
  $langcode = isset($entity->language) ? $entity->language : LANGUAGE_NONE;

  if (!empty($form_state['values']['field_cblinks'][$langcode][0]['url']) && empty($form_state['values']['field_cbimage'][$langcode][0]['fid'])) {
    form_set_error('field_cbimage', t('Field %name is required.', array('%name' => $form['field_cbimage'][$langcode][0]['#title'])));
  }

  if (!empty($form_state['values']['field_cbimage'][$langcode][0]['fid']) && empty($form_state['values']['field_cblinks'][$langcode][0]['url'])) {
    form_set_error('field_cblinks', t('Field %name is required.', array('%name' => $form['field_cblinks'][$langcode][0]['#title'])));
  }
}

/**
 * Form validation handler for contentblock_cbreference_entity_form() for welcome.
 */
function _contentblock_cbreference_entity_form_welcome_validate($form, &$form_state) {
  $entity_type = $form_state['entity_type'];
  $entity = $form_state[$entity_type];
  $langcode = isset($entity->language) ? $entity->language : LANGUAGE_NONE;

  if (!empty($form_state['values']['field_cbbody'][$langcode][0]['value'])) {
    $max_char_count = 320;
    $body_char_count = _contentblock_cbreference_count_markup($form_state['values']['field_cbbody'][$langcode][0]['value']);
    if ($body_char_count > $max_char_count) {
      form_set_error('field_cbbody', t('Field %name should not have more than %max characters (HTML markup does not count), %count given.', array('%name' => $form['field_cbbody'][$langcode][0]['#title'], '%max' => $max_char_count, '%count' => $body_char_count)));
    }
  }
}

/**
 * Form validation handler for contentblock_cbreference_entity_form() for footer.
 */
function _contentblock_cbreference_entity_form_footer_validate($form, &$form_state) {
  $entity_type = $form_state['entity_type'];
  $entity = $form_state[$entity_type];
  $langcode = isset($entity->language) ? $entity->language : LANGUAGE_NONE;

  if (!empty($form_state['values']['field_cbbody'][$langcode][0]['value'])) {
    $max_char_count = 320;
    $body_char_count = _contentblock_cbreference_count_markup($form_state['values']['field_cbbody'][$langcode][0]['value']);
    if ($body_char_count > $max_char_count) {
      form_set_error('field_cbbody', t('Field %name should not have more than %max characters (HTML markup does not count), %count given.', array('%name' => $form['field_cbbody'][$langcode][0]['#title'], '%max' => $max_char_count, '%count' => $body_char_count)));
    }
  }
}

/**
 * Count the number of text character in a string of HTML markup.
 */
function _contentblock_cbreference_count_markup($markup) {
  // Strip HTML markup.
  $markup = strip_tags($markup);

  // Strip media.module tokens and whitespace around.
  $markup = preg_replace('/\s*\[\[.*?\]\]\s*/', '', $markup);

  return drupal_strlen(trim($markup));
}
