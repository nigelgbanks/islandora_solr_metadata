<?php
/**
 * @file
 * Houses the form and utilities used in islandora_solr_metadata configurations.
 */

/**
 * The configuration form for the islandora_solr_metadata module.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 * @param int $configuration_id
 *   The ID of the configuration we are currently editing.
 *
 * @return array
 *   An array representing the configuration form.
 */
function islandora_solr_metadata_config_form($form, &$form_state, $configuration_id) {
  form_load_include($form_state, 'inc', 'islandora', 'includes/content_model.autocomplete');
  form_load_include($form_state, 'inc', 'islandora_solr_metadata', 'includes/db');
  $field_to_add = FALSE;
  $cmodel_to_add = FALSE;

  if (!isset($form_state['field_data'])) {
    $form_state['field_data'] = islandora_solr_metadata_get_fields($configuration_id, FALSE);
  }

  // AJAX callback handling.
  if (isset($form_state['triggering_element'])) {
    if ($form_state['triggering_element']['#name'] == 'islandora-solr-metadata-add-field') {
      $field_name = $form_state['values']['islandora_solr_metadata_fields']['table_wrapper']['add_fieldset']['available_solr_fields'];
      $form_state['field_data'][$field_name] = array(
        'configuration_id' => $configuration_id,
        'solr_field' => $field_name,
        // Arbitrary large sort weight so it always comes last.
        'weight' => 10000,
        'ajax-volatile' => TRUE,
      );
    }

    if ($form_state['triggering_element']['#name'] == 'islandora-solr-metadata-fields-remove-selected') {
      $to_remove = function ($row) {
        return $row['remove_field'];
      };
      $form_state['field_data'] = array_diff_key(
        $form_state['field_data'],
        array_filter($form_state['values']['islandora_solr_metadata_fields']['table_wrapper']['table']['table']['rows'], $to_remove)
      );
    }

    if ($form_state['triggering_element']['#name'] == 'islandora-solr-metadata-cmodels-add-cmodel') {
      $cmodel_to_add = array('cmodel' => $form_state['values']['islandora_solr_metadata_cmodels']['table_wrapper']['cmodel_options']['cmodel_select']);
    }

    if ($form_state['triggering_element']['#name'] == 'islandora-solr-metadata-cmodels-remove-selected') {
      foreach ($form_state['values']['islandora_solr_metadata_cmodels']['table_wrapper']['table'] as $key => $row) {
        if ($row !== 0) {
          unset($form_state['complete form']['islandora_solr_metadata_cmodels']['table_wrapper']['table']['#options'][$key]);
        }
      }
    }
  }
  $form = array(
    '#tree' => TRUE,
  );
  $form['islandora_solr_metadata_configuration_id'] = array(
    '#type' => 'value',
    '#value' => $configuration_id,
  );

  $form['islandora_solr_metadata_cmodels'] = array(
    '#type' => 'fieldset',
    '#title' => t('Content Models'),
    '#collapsed' => TRUE,
    '#collapsible' => FALSE,
  );
  $form['islandora_solr_metadata_cmodels']['table_wrapper'] = array(
    '#prefix' => '<div id="islandora-solr-metadata-cmodels-wrapper">',
    '#suffix' => '</div>',
  );
  // If there are values in the form_state use them for persistence in case of
  // AJAX callbacks, otherwise grab fresh values from the database.
  if (isset($form_state['values'])) {
    if (isset($form_state['values']['islandora_solr_metadata_cmodels']['table_wrapper']['table'])) {
      $cmodels_associated = $form_state['complete form']['islandora_solr_metadata_cmodels']['table_wrapper']['table']['#options'];
    }
  }
  else {
    $cmodels_associated = islandora_solr_metadata_get_cmodels($configuration_id);
  }

  if ($cmodel_to_add !== FALSE) {
    $cmodels_associated[$cmodel_to_add['cmodel']] = $cmodel_to_add;
  }

  $form['islandora_solr_metadata_cmodels']['table_wrapper']['table'] = array(
    '#type' => 'tableselect',
    '#header' => array(
      'cmodel' => array('data' => t('Content Model Name')),
    ),
    '#options' => $cmodels_associated,
    '#empty' => t('No content models associated.'),
  );

  if (count($cmodels_associated)) {
    $form['islandora_solr_metadata_cmodels']['table_wrapper']['remove_selected'] = array(
      '#type' => 'button',
      '#value' => t('Remove selected'),
      '#name' => 'islandora-solr-metadata-cmodels-remove-selected',
      '#ajax' => array(
        'callback' => 'islandora_solr_metadata_cmodels_ajax',
        'wrapper' => 'islandora-solr-metadata-cmodels-wrapper',
      ),
    );
  }

  // Retrieve all content models and unset those currently in use in this
  // configuration and any others from other configurations.
  $add_options = islandora_get_content_model_names();
  foreach ($cmodels_associated as $entry) {
    unset($add_options[$entry['cmodel']]);
  }

  if (!empty($add_options)) {
    $form['islandora_solr_metadata_cmodels']['table_wrapper']['cmodel_options'] = array(
      '#type' => 'fieldset',
      '#title' => t('Add content model'),
      '#collapsed' => TRUE,
      '#collapsible' => FALSE,
    );

    $form['islandora_solr_metadata_cmodels']['table_wrapper']['cmodel_options']['cmodel_select'] = array(
      '#type' => 'select',
      '#options' => $add_options,
    );
    $form['islandora_solr_metadata_cmodels']['table_wrapper']['cmodel_options']['cmodel_add'] = array(
      '#type' => 'button',
      '#value' => t('Add'),
      '#name' => 'islandora-solr-metadata-cmodels-add-cmodel',
      '#ajax' => array(
        'callback' => 'islandora_solr_metadata_cmodels_ajax',
        'wrapper' => 'islandora-solr-metadata-cmodels-wrapper',
      ),
    );
  }

  $form['islandora_solr_metadata_fields'] = array(
    '#type' => 'fieldset',
    '#title' => 'Display fields',
    '#collapsed' => TRUE,
    '#collapsible' => FALSE,
  );

  $form['islandora_solr_metadata_fields']['table_wrapper'] = array(
    '#prefix' => '<div id="islandora-solr-metadata-fields-wrapper">',
    '#suffix' => '</div>',
  );
  $form['islandora_solr_metadata_fields']['table_wrapper']['table'] = islandora_solr_metadata_management($form_state['field_data']);
  if (count($form['islandora_solr_metadata_fields']['table_wrapper']['table']['table']['rows'])) {
    $form['islandora_solr_metadata_fields']['table_wrapper']['remove_selected'] = array(
      '#type' => 'button',
      '#value' => t('Remove selected'),
      '#name' => 'islandora-solr-metadata-fields-remove-selected',
      '#ajax' => array(
        'callback' => 'islandora_solr_metadata_fields_ajax',
        'wrapper' => 'islandora-solr-metadata-fields-wrapper',
      ),
    );
  }
  $form['islandora_solr_metadata_fields']['table_wrapper']['add_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add field'),
    '#collapsed' => TRUE,
    '#collapsible' => FALSE,
  );
  $form['islandora_solr_metadata_fields']['table_wrapper']['add_fieldset']['available_solr_fields'] = array(
    '#type' => 'textfield',
    '#description' => t('A field from within Solr'),
    '#size' => 105,
    '#autocomplete_path' => 'islandora_solr/autocomplete_luke',
    '#default_value' => '',
  );
  $form['islandora_solr_metadata_fields']['table_wrapper']['add_fieldset']['add_field'] = array(
    '#type' => 'button',
    '#value' => t('Add'),
    '#name' => 'islandora-solr-metadata-add-field',
    '#ajax' => array(
      'callback' => 'islandora_solr_metadata_fields_ajax',
      'wrapper' => 'islandora-solr-metadata-fields-wrapper',
    ),
  );
  $form['islandora_solr_metadata_fields']['description_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Description'),
    '#description' => t("The field used for rendering the description beneath the object's content"),
    '#collapsed' => TRUE,
    '#collapsible' => FALSE,
  );

  $description = islandora_solr_metadata_retrieve_description($configuration_id, FALSE);
  $form['islandora_solr_metadata_fields']['description_fieldset']['available_solr_fields'] = array(
    '#type' => 'textfield',
    '#description' => t('A field from within Solr'),
    '#size' => 105,
    '#autocomplete_path' => 'islandora_solr/autocomplete_luke',
    '#default_value' => $description['description_field'],
  );
  $form['islandora_solr_metadata_fields']['description_fieldset']['display_label'] = array(
    '#type' => 'textfield',
    '#description' => t('A label for displaying'),
    '#size' => 45,
    '#default_value' => $description['description_label'],
    '#states' => array(
      'visible' => array(
        ':input[name="islandora_solr_metadata_fields[description_fieldset][available_solr_fields]"]' => array('empty' => FALSE),
      ),
    ),
  );
  $form['islandora_solr_metadata_save'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
    '#weight' => 10,
  );
  $form['islandora_solr_metadata_delete'] = array(
    '#type' => 'submit',
    '#value' => t('Delete configuration'),
    '#weight' => 10,
  );
  return $form;
}

/**
 * Validation for the islandora_solr_metadata configuration form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function islandora_solr_metadata_config_form_validate($form, $form_state) {
  if ($form_state['triggering_element']['#name'] == 'islandora-solr-metadata-add-field') {
    module_load_include('inc', 'islandora_solr_search', 'includes/luke');
    $solr_luke = islandora_solr_get_luke();
    $luke_fields = array_keys($solr_luke['fields']);

    if (!in_array($form_state['values']['islandora_solr_metadata_fields']['table_wrapper']['add_fieldset']['available_solr_fields'], $luke_fields)) {
      form_set_error('islandora_solr_metadata_fields][table_wrapper][add_fieldset][available_solr_fields', t('The field @field is not a valid field within Solr!', array('@field' => $form_state['values']['islandora_solr_metadata_fields']['table_wrapper']['add_fieldset']['available_solr_fields'])));
    }
    else {
      $added_values = isset($form_state['values']['islandora_solr_metadata_fields']['table_wrapper']['table']['table']) ?
        array_keys($form_state['values']['islandora_solr_metadata_fields']['table_wrapper']['table']['table']) :
        array();

      if (in_array($form_state['values']['islandora_solr_metadata_fields']['table_wrapper']['add_fieldset']['available_solr_fields'], $added_values)) {
        form_set_error('islandora_solr_metadata_fields][table_wrapper][add_fieldset][available_solr_fields', t('The field @field already exists in this configuration!', array('@field' => $form_state['values']['islandora_solr_metadata_fields']['table_wrapper']['add_fieldset']['available_solr_fields'])));
      }
    }
  }

  if ($form_state['triggering_element']['#name'] == 'islandora-solr-metadata-fields-remove-selected') {
    $rows_to_remove = array();
    foreach ($form_state['values']['islandora_solr_metadata_fields']['table_wrapper']['table']['table']['rows'] as $key => $row) {
      if ($row['remove_field'] == TRUE) {
        $rows_to_remove[] = $key;
      }
    }
    if (count($rows_to_remove) === 0) {
      form_set_error('islandora_solr_metadata', t('Must select at least one entry to remove!'));
    }
  }

  if ($form_state['triggering_element']['#name'] == 'islandora-solr-metadata-cmodels-remove-selected') {
    $rows_to_remove = array();
    foreach ($form_state['values']['islandora_solr_metadata_cmodels']['table_wrapper']['table'] as $key => $row) {
      if ($row !== 0) {
        $rows_to_remove[] = $key;
      }
    }
    if (count($rows_to_remove) === 0) {
      form_set_error('islandora_solr_metadata', t('Must select at least one entry to remove!'));
    }
  }

  if ($form_state['triggering_element']['#value'] == 'Save configuration') {
    $solr_field = $form_state['values']['islandora_solr_metadata_fields']['description_fieldset']['available_solr_fields'];
    if (!empty($solr_field)) {
      module_load_include('inc', 'islandora_solr_search', 'includes/luke');
      $solr_luke = islandora_solr_get_luke();
      $luke_fields = array_keys($solr_luke['fields']);
      if (!in_array($solr_field, $luke_fields)) {
        form_set_error('islandora_solr_metadata_fields][description_fieldset][available_solr_fields', t('The field @field is not a valid field within Solr!', array('@field' => $solr_field)));
      }
    }
  }
}

/**
 * Submit handler for the islandora_solr_metadata configuration form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function islandora_solr_metadata_config_form_submit($form, $form_state) {
  module_load_include('inc', 'islandora_solr_metadata', 'includes/db');
  $configuration_id = $form_state['values']['islandora_solr_metadata_configuration_id'];

  if ($form_state['clicked_button']['#value'] == 'Save configuration') {
    // Grab existing entries first for comparison.
    $remove_form_specifics = function ($field) {
      return array_diff_key($field, drupal_map_assoc(array(
        'ajax-volatile',
        'remove',
        'remove_field',
      )));
    };
    $fields_fs_mapped = array_map($remove_form_specifics, drupal_array_merge_deep($form_state['field_data'], $form_state['values']['islandora_solr_metadata_fields']['table_wrapper']['table']['table']['rows']));
    $fields_db = islandora_solr_metadata_get_fields($configuration_id);

    $cmodels_db = islandora_solr_metadata_get_cmodels($configuration_id);
    $cmodels_fs = $form_state['complete form']['islandora_solr_metadata_cmodels']['table_wrapper']['table']['#options'];

    $cmodels_deletion = array_diff_key($cmodels_db, $cmodels_fs);
    $cmodels_insertion = array_diff_key($cmodels_fs, $cmodels_db);

    // Handle insertion and deletion of content models first.
    if (count($cmodels_insertion)) {
      islandora_solr_metadata_add_content_models($configuration_id, $cmodels_insertion);
    }
    if (count($cmodels_deletion)) {
      islandora_solr_metadata_delete_content_models($configuration_id, $cmodels_deletion);
    }

    $fields_deletion = array_diff_key($fields_db, $fields_fs_mapped);
    $fields_insertion = array_diff_key($fields_fs_mapped, $fields_db);

    if (count($fields_insertion)) {
      islandora_solr_metadata_add_fields($configuration_id, $fields_insertion);
    }
    if (count($fields_deletion)) {
      islandora_solr_metadata_delete_fields($configuration_id, $fields_deletion);
    }

    $fields_update = array_intersect_key($fields_fs_mapped, $fields_db);
    if (count($fields_update)) {
      islandora_solr_metadata_update_fields($configuration_id, $fields_update);
    }

    $description_field = $form_state['values']['islandora_solr_metadata_fields']['description_fieldset']['available_solr_fields'];
    $description_label = $form_state['values']['islandora_solr_metadata_fields']['description_fieldset']['display_label'];
    islandora_solr_metadata_update_description($configuration_id, $description_field, $description_label);
    drupal_set_message(t('The Solr metadata display configuration options have been saved.'));
  }

  if ($form_state['clicked_button']['#value'] == 'Delete configuration') {
    drupal_goto("admin/islandora/search/islandora_solr_metadata/config/delete/$configuration_id");
  }
}

/**
 * AJAX callback for the fields portion of the configuration form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array representing the Drupal form state.
 *
 * @return array
 *   An array representing the portion of the form we are rendering.
 */
function islandora_solr_metadata_fields_ajax(&$form, &$form_state) {
  return $form['islandora_solr_metadata_fields']['table_wrapper'];
}

/**
 * AJAX callback for the content models portion of the configuration form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array representing the Drupal form state.
 *
 * @return array
 *   An array representing the portion of the form we are rendering.
 */
function islandora_solr_metadata_cmodels_ajax(&$form, &$form_state) {
  return $form['islandora_solr_metadata_cmodels']['table_wrapper'];
}

/**
 * Helper function to construct the fields tableselect.
 *
 * @param array $data
 *   An array of data representing rows to be rendered in the tableselect.
 *
 * @return array
 *   An array representing the tableselect to be rendered.
 */
function islandora_solr_metadata_management($data) {
  module_load_include('inc', 'islandora_solr_metadata', 'includes/db');
  module_load_include('inc', 'islandora_solr', 'includes/utilities');
  $form = array();
  $rows = array();

  if ($data) {
    uasort($data, 'drupal_sort_weight');
    $delta = count($data);
    $weight = 1;
    $map = function ($field) use ($delta, &$weight) {
      $value = $weight;
      $weight++;
      return array(
        'remove_field' => array(
          '#type' => 'checkbox',
          '#default_value' => isset($field['remove']) ? $field['remove'] : FALSE,
        ),
        'solr_field_markup' => array('#markup' => filter_xss($field['solr_field'])),
        'operations' => array(
          'edit' => array(
            '#access' => isset($field['ajax-volatile']) ? !$field['ajax-volatile'] : TRUE,
            '#theme' => 'link',
            '#path' => "admin/islandora/search/islandora_solr_metadata/config/{$field['configuration_id']}/" . islandora_solr_replace_slashes($field['solr_field']),
            '#text' => t('edit'),
            '#options' => array(
              'attributes' => array(),
              'html' => FALSE,
            ),
          ),
        ),
        'weight' => array(
          '#type' => 'hidden',
          '#default_value' => $weight,
        ),
      );
    };
    $rows = array_map($map, $data);
  }
  // Add the table to the form.
  $form['#tree'] = TRUE;
  $form['table'] = array(
    '#type' => 'markup',
    '#header' => array(
      t('Remove'),
      t('Solr Field'),
      t('Operations'),
      '',
    ),
    'rows' => $rows,
    '#theme' => 'islandora_solr_metadata_management_table',
    '#empty' => t('No fields associated'),
  );
  return $form;
}

/**
 * Form building function; allow configuration of a particular field.
 */
function islandora_solr_metadata_config_field_form($form, &$form_state, $config_id, $escaped_field_name) {
  form_load_include($form_state, 'inc', 'islandora_solr', 'includes/utilities');
  $field_name = islandora_solr_restore_slashes($escaped_field_name);
  $get_default = function ($value, $default = '') use ($config_id, $field_name) {
    static $field_info = NULL;
    if ($field_info === NULL) {
      $fields = islandora_solr_metadata_get_fields($config_id);
      $field_info = $fields[$field_name];
    }
    $exists = FALSE;
    $looked_up = drupal_array_get_nested_value($field_info, (array) $value, $exists);
    return $exists ? $looked_up : $default;
  };

  $form['#tree'] = TRUE;
  $form['wrapper'] = array(
    '#type' => 'fieldset',
    '#title' => t('Field config'),
  );

  $set =& $form['wrapper'];
  $set['display_label'] = array(
    '#type' => 'textfield',
    '#title' => t('Display Label'),
    '#description' => t('A human-readable label to display alongside values found for this field.'),
    '#default_value' => $get_default('display_label', $field_name),
  );
  $set['hyperlink'] = array(
    '#type' => 'checkbox',
    '#title' => t('Hyperlink?'),
    '#description' => t('Should each value for this field be linked to a search to find objects with the value in this field?'),
    '#default_value' => $get_default('hyperlink', FALSE),
  );
  $set['truncation'] = array(
    '#type' => 'fieldset',
    '#title' => t('Truncation'),
    'truncation_type' => array(
      '#type' => 'radios',
      '#title' => t('Truncation Type'),
      '#options' => array('separate_value_option' => t('Limit length of each separate value'), 'whole_field_option' => t('Limit Length of the whole field')),
      '#default_value' => $get_default(array('truncation', 'truncation_type'), 'separate_value_option'),
    ),
    'max_length' => array(
      '#type' => 'textfield',
      '#title' => t('Max Length'),
      '#description' => t('The field contents will be truncated to be at most this length (in characters) for display. 0 or less to disable truncation.<br /> When truncating based on the whole field the max length may be exceeded by the length of ellispse string.'),
      '#default_value' => $get_default(array('truncation', 'max_length'), 0),
      '#element_validate' => array('element_validate_integer'),
    ),
    'word_safe' => array(
      '#type' => 'checkbox',
      '#title' => t('Word-safe'),
      '#description' => t('Attempt to truncate on a word boundary. See truncate_utf8() for more info.'),
      '#default_value' => $get_default(array('truncation', 'word_safe'), FALSE),
    ),
    'ellipsis' => array(
      '#type' => 'checkbox',
      '#title' => t('Ellipsis'),
      '#description' => t('Append ellipses when truncating occurs.'),
      '#default_value' => $get_default(array('truncation', 'ellipsis'), FALSE),
    ),
    'min_wordsafe_length' => array(
      '#type' => 'textfield',
      '#title' => t('Minimum word-safe length'),
      '#default_value' => $get_default(array('truncation', 'min_wordsafe_length'), 1),
      '#element_validate' => array('element_validate_integer_positive'),
      '#states' => array(
        'visible' => array(
          ':input[name="wrapper[truncation][word_safe]"]' => array(
            'checked' => TRUE,
          ),
        ),
      ),
    ),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save field configuration'),
  );
  return $form;
}

/**
 * Form validation handler; validate some values.
 */
function islandora_solr_metadata_config_field_form_validate(&$form, &$form_state) {
  if ($form_state['values']['wrapper']['hyperlink'] && $form_state['values']['wrapper']['truncation']['max_length'] > 0) {
    form_error($form['wrapper']['hyperlink'], t('Either hyperlinking or truncation can be used, but not both together on the same field. Disable one.'));
    form_error($form['wrapper']['truncation']['max_length']);
  }
}

/**
 * Form submission handler; save the configuration of the field.
 */
function islandora_solr_metadata_config_field_form_submit(&$form, &$form_state) {
  list($config_id, $escaped_field_name) = $form_state['build_info']['args'];
  $field_name = islandora_solr_restore_slashes($escaped_field_name);

  $fields = islandora_solr_metadata_get_fields($config_id);
  $field_info = $fields[$field_name];

  $field_info = $form_state['values']['wrapper'] + $field_info;
  islandora_solr_metadata_update_fields($config_id, array($field_info));

  $form_state['redirect'] = array(
    "admin/islandora/search/islandora_solr_metadata/config/$config_id",
  );
}
