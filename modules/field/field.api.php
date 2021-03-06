<?php
// $Id: field.api.php,v 1.59 2010-01-02 15:00:34 dries Exp $

/**
 * @ingroup field_fieldable_type
 * @{
 */

/**
 * Expose "pseudo-field" components on fieldable objects.
 *
 * Field UI's 'Manage fields' page lets users re-order fields, but also
 * non-field components. For nodes, that would be title, menu settings, or
 * other elements exposed by contributed modules through hook_form() or
 * hook_form_alter().
 *
 * Fieldable entities or contributed modules that want to have their components
 * supported should expose them using this hook, and use
 * field_attach_extra_weight() to retrieve the user-defined weight when
 * inserting the component.
 *
 * @return @todo
 *   An array of 'pseudo-field' components. The keys are the name of the element
 *   as it appears in the form structure. The values are arrays with the
 *   following key/value pairs:
 *   - label: The human readable name of the component.
 *   - description: A short description of the component contents.
 *   - weight: The default weight of the element.
 *   - view: (optional) The name of the element as it appears in the rendered
 *     structure, if different from the name in the form.
 */
function hook_field_extra_fields() {
  $extra = array();

  if ($type = node_type_get_type($bundle)) {
    if ($type->has_title) {
      $extra['title'] = array(
        'label' => $type->title_label,
        'description' => t('Node module element.'),
        'weight' => -5,
      );
    }
    if ($bundle == 'poll' && module_exists('poll')) {
      $extra['title'] = array(
        'label' => t('Poll title'),
        'description' => t('Poll module title.'),
        'weight' => -5,
      );
      $extra['choice_wrapper'] = array(
        'label' => t('Poll choices'),
        'description' => t('Poll module choices.'),
        'weight' => -4,
      );
      $extra['settings'] = array(
        'label' => t('Poll settings'),
        'description' => t('Poll module settings.'),
        'weight' => -3,
      );
    }
  }
  return $extra;
}

/**
 * @} End of "ingroup field_fieldable_type"
 */

/**
 * @defgroup field_types Field Types API
 * @{
 * Define field types, widget types, display formatter types, storage types.
 *
 * The bulk of the Field Types API are related to field types. A field type
 * represents a particular type of data (integer, string, date, etc.) that
 * can be attached to a fieldable object. hook_field_info() defines the basic
 * properties of a field type, and a variety of other field hooks are called by
 * the Field Attach API to perform field-type-specific actions.
 * @see hook_field_info().
 * @see hook_field_info_alter().
 * @see hook_field_schema().
 * @see hook_field_load().
 * @see hook_field_validate().
 * @see hook_field_presave().
 * @see hook_field_insert().
 * @see hook_field_update().
 * @see hook_field_delete().
 * @see hook_field_delete_revision().
 * @see hook_field_prepare_view().
 * @see hook_field_is_empty().
 *
 * The Field Types API also defines two kinds of pluggable handlers: widgets
 * and formatters, which specify how the field appears in edit forms and in
 * displayed objects. Widgets and formatters can be implemented by a field-type
 * module for it's own field types, or by a third-party module to extend the
 * behavior of existing field types.
 * @see hook_field_widget_info().
 * @see hook_field_formatter_info().
 *
 * A third kind of pluggable handlers, storage backends, is defined by the
 * @link field_storage Field Storage API @endlink.
 */

/**
 * Define Field API field types.
 *
 * @return
 *   An array whose keys are field type names and whose values are arrays
 *   describing the field type, with the following key/value pairs:
 *   - label: The human-readable name of the field type.
 *   - description: A short description for the field type.
 *   - settings: An array whose keys are the names of the settings available
 *     for the field type, and whose values are the default values for those
 *     settings.
 *   - instance_settings: An array whose keys are the names of the settings
 *     available for instances of the field type, and whose values are the
 *     default values for those settings.
 *     Instance-level settings can have different values on each field
 *     instance, and thus allow greater flexibility than field-level settings.
 *     It is recommended to put settings at the instance level whenever
 *     possible. Notable exceptions: settings acting on the schema definition,
 *     or settings that Views needs to use across field instances (e.g. list of
 *     allowed values).
 *   - default_widget: The machine name of the default widget to be used by
 *     instances of this field type, when no widget is specified in the
 *     instance definition. This widget must be available whenever the field
 *     type is available (i.e. provided by the field type module, or by a module
 *     the field type module depends on).
 *   - default_formatter: The machine name of the default formatter to be used
 *     by instances of this field type, when no formatter is specified in the
 *     instance definition. This formatter must be available whenever the field
 *     type is available (i.e. provided by the field type module, or by a module
 *     the field type module depends on).
 */
function hook_field_info() {
  return array(
    'text' => array(
      'label' => t('Text'),
      'description' => t('This field stores varchar text in the database.'),
      'settings' => array('max_length' => 255),
      'instance_settings' => array('text_processing' => 0),
      'default_widget' => 'text_textfield',
      'default_formatter' => 'text_default',
    ),
    'text_long' => array(
      'label' => t('Long text'),
      'description' => t('This field stores long text in the database.'),
      'settings' => array('max_length' => ''),
      'instance_settings' => array('text_processing' => 0),
      'default_widget' => 'text_textarea',
      'default_formatter' => 'text_default',
    ),
    'text_with_summary' => array(
      'label' => t('Long text and summary'),
      'description' => t('This field stores long text in the database along with optional summary text.'),
      'settings' => array('max_length' => ''),
      'instance_settings' => array('text_processing' => 1, 'display_summary' => 0),
      'default_widget' => 'text_textarea_with_summary',
      'default_formatter' => 'text_summary_or_trimmed',
    ),
  );
}

/**
 * Perform alterations on Field API field types.
 *
 * @param $info
 *   Array of informations on widget types exposed by hook_field_info()
 *   implementations.
 */
function hook_field_info_alter(&$info) {
  // Add a setting to all field types.
  foreach ($info as $field_type => $field_type_info) {
    $info[$field_type]['settings'] += array(
      'mymodule_additional_setting' => 'default value',
    );
  }

  // Change the default widget for fields of type 'foo'.
  if (isset($info['foo'])) {
    $info['foo']['default widget'] = 'mymodule_widget';
  }
}

/**
 * Define the Field API schema for a field structure.
 *
 * @param $field
 *   A field structure.
 * @return
 *   An associative array with the following keys:
 *   - columns: An array of Schema API column specifications, keyed by column name.
 *     This specifies what comprises a value for a given field.
 *     For example, a value for a number field is simply 'value', while a
 *     value for a formatted text field is the combination of 'value' and
 *     'format'.
 *     It is recommended to avoid having the columns definitions depend on
 *     field settings when possible.
 *     No assumptions should be made on how storage engines internally use the
 *     original column name to structure their storage.
 *   - indexes: An array of Schema API indexes definitions. Only columns that
 *     appear in the 'columns' array are allowed.
 *     Those indexes will be used as default indexes. Callers of
 *     field_create_field() can specify additional indexes, or, at their own
 *     risk, modify the default indexes specified by the field-type module.
 *     Some storage engines might not support indexes.
 */
function hook_field_schema($field) {
  if ($field['type'] == 'text_long') {
    $columns = array(
      'value' => array(
        'type' => 'text',
        'size' => 'big',
        'not null' => FALSE,
      ),
    );
  }
  else {
    $columns = array(
      'value' => array(
        'type' => 'varchar',
        'length' => $field['settings']['max_length'],
        'not null' => FALSE,
      ),
    );
  }
  $columns += array(
    'format' => array(
      'type' => 'int',
      'unsigned' => TRUE,
      'not null' => FALSE,
    ),
  );
  return array(
    'columns' => $columns,
    'indexes' => array(
      'format' => array('format'),
    ),
  );
}

/**
 * Defines custom load behavior for this module's field types.
 *
 * Unlike most other field hooks, this hook operates on multiple objects. The
 * $objects, $instances and $items parameters are arrays keyed by object id.
 * For performance reasons, information for all available objects should be
 * loaded in a single query where possible.
 *
 * Note that the changes made to the field values get cached by the field cache
 * for subsequent loads. You should never use this hook to load fieldable
 * entities, since this is likely to cause infinite recursions when
 * hook_field_load() is run on those as well. Use
 * hook_field_formatter_prepare_view() instead.
 *
 * @param $obj_type
 *   The type of $object.
 * @param $objects
 *   Array of objects being loaded, keyed by object id.
 * @param $field
 *   The field structure for the operation.
 * @param $instances
 *   Array of instance structures for $field for each object, keyed by object
 *   id.
 * @param $langcode
 *   The language associated to $items.
 * @param $items
 *   Array of field values already loaded for the objects, keyed by object id.
 * @param $age
 *   FIELD_LOAD_CURRENT to load the most recent revision for all fields, or
 *   FIELD_LOAD_REVISION to load the version indicated by each object.
 * @return
 *   Changes or additions to field values are done by altering the $items
 *   parameter by reference.
 */
function hook_field_load($obj_type, $objects, $field, $instances, $langcode, &$items, $age) {
  // Sample code from text.module: precompute sanitized strings so they are
  // stored in the field cache.
  foreach ($objects as $id => $object) {
    foreach ($items[$id] as $delta => $item) {
      // Only process items with a cacheable format, the rest will be handled
      // by formatters if needed.
      if (empty($instances[$id]['settings']['text_processing']) || filter_format_allowcache($item['format'])) {
        $items[$id][$delta]['safe_value'] = isset($item['value']) ? _text_sanitize($instances[$id], $langcode, $item, 'value') : '';
        if ($field['type'] == 'text_with_summary') {
          $items[$id][$delta]['safe_summary'] = isset($item['summary']) ? _text_sanitize($instances[$id], $langcode, $item, 'summary') : '';
        }
      }
    }
  }
}

/**
 * Prepares field values prior to display.
 *
 * This hook is invoked before the field values are handed to formatters
 * for display, and runs before the formatters' own
 * hook_field_formatter_prepare_view().
 * @see hook_field_formatter_prepare_view()
 *
 * Unlike most other field hooks, this hook operates on multiple objects. The
 * $objects, $instances and $items parameters are arrays keyed by object id.
 * For performance reasons, information for all available objects should be
 * loaded in a single query where possible.
 *
 * @param $obj_type
 *   The type of $object.
 * @param $objects
 *   Array of objects being displayed, keyed by object id.
 * @param $field
 *   The field structure for the operation.
 * @param $instances
 *   Array of instance structures for $field for each object, keyed by object
 *   id.
 * @param $langcode
 *   The language associated to $items.
 * @param $items
 *   $object->{$field['field_name']}, or an empty array if unset.
 */
function hook_field_prepare_view($obj_type, $objects, $field, $instances, $langcode, &$items) {
  // Sample code from image.module: if there are no images specified at all,
  // use the default.
  foreach ($objects as $id => $object) {
    if (empty($items[$id]) && $field['settings']['default_image']) {
      if ($file = file_load($field['settings']['default_image'])) {
        $items[$id][0] = (array) $file + array(
          'is_default' => TRUE,
          'alt' => '',
          'title' => '',
        );
      }
    }
  }
}

/**
 * Define custom validate behavior for this module's field types.
 *
 * @param $obj_type
 *   The type of $object.
 * @param $object
 *   The object for the operation.
 * @param $field
 *   The field structure for the operation.
 * @param $instance
 *   The instance structure for $field on $object's bundle.
 * @param $langcode
 *   The language associated to $items.
 * @param $items
 *   $object->{$field['field_name']}[$langcode], or an empty array if unset.
 * @param $errors
 *   The array of errors, keyed by field name and by value delta, that have
 *   already been reported for the object. The function should add its errors
 *   to this array. Each error is an associative array, with the following
 *   keys and values:
 *   - 'error': an error code (should be a string, prefixed with the module name)
 *   - 'message': the human readable message to be displayed.
 */
function hook_field_validate($obj_type, $object, $field, $instance, $langcode, &$items, &$errors) {
  foreach ($items as $delta => $item) {
    if (!empty($item['value'])) {
      if (!empty($field['settings']['max_length']) && drupal_strlen($item['value']) > $field['settings']['max_length']) {
        $errors[$field['field_name']][$delta][] = array(
          'error' => 'text_max_length',
          'message' => t('%name: the value may not be longer than %max characters.', array('%name' => $instance['label'], '%max' => $field['settings']['max_length'])),
        );
      }
    }
  }
}

/**
 * Define custom presave behavior for this module's field types.
 *
 * @param $obj_type
 *   The type of $object.
 * @param $object
 *   The object for the operation.
 * @param $field
 *   The field structure for the operation.
 * @param $instance
 *   The instance structure for $field on $object's bundle.
 * @param $langcode
 *   The language associated to $items.
 * @param $items
 *   $object->{$field['field_name']}[$langcode], or an empty array if unset.
 */
function hook_field_presave($obj_type, $object, $field, $instance, $langcode, &$items) {
  if ($field['type'] == 'number_decimal') {
    // Let PHP round the value to ensure consistent behavior across storage
    // backends.
    foreach ($items as $delta => $item) {
      if (isset($item['value'])) {
        $items[$delta]['value'] = round($item['value'], $field['settings']['scale']);
      }
    }
  }
}

/**
 * Define custom insert behavior for this module's field types.
 *
 * @param $obj_type
 *   The type of $object.
 * @param $object
 *   The object for the operation.
 * @param $field
 *   The field structure for the operation.
 * @param $instance
 *   The instance structure for $field on $object's bundle.
 * @param $langcode
 *   The language associated to $items.
 * @param $items
 *   $object->{$field['field_name']}[$langcode], or an empty array if unset.
 */
function hook_field_insert($obj_type, $object, $field, $instance, $langcode, &$items) {
}

/**
 * Define custom update behavior for this module's field types.
 *
 * @param $obj_type
 *   The type of $object.
 * @param $object
 *   The object for the operation.
 * @param $field
 *   The field structure for the operation.
 * @param $instance
 *   The instance structure for $field on $object's bundle.
 * @param $langcode
 *   The language associated to $items.
 * @param $items
 *   $object->{$field['field_name']}[$langcode], or an empty array if unset.
 */
function hook_field_update($obj_type, $object, $field, $instance, $langcode, &$items) {
}

/**
 * Define custom delete behavior for this module's field types.
 *
 * This hook is invoked just before the data is deleted from field storage.
 *
 * @param $obj_type
 *   The type of $object.
 * @param $object
 *   The object for the operation.
 * @param $field
 *   The field structure for the operation.
 * @param $instance
 *   The instance structure for $field on $object's bundle.
 * @param $langcode
 *   The language associated to $items.
 * @param $items
 *   $object->{$field['field_name']}[$langcode], or an empty array if unset.
 */
function hook_field_delete($obj_type, $object, $field, $instance, $langcode, &$items) {
}

/**
 * Define custom delete_revision behavior for this module's field types.
 *
 * This hook is invoked just before the data is deleted from field storage,
 * and will only be called for fieldable types that are versioned.
 *
 * @param $obj_type
 *   The type of $object.
 * @param $object
 *   The object for the operation.
 * @param $field
 *   The field structure for the operation.
 * @param $instance
 *   The instance structure for $field on $object's bundle.
 * @param $langcode
 *   The language associated to $items.
 * @param $items
 *   $object->{$field['field_name']}[$langcode], or an empty array if unset.
 */
function hook_field_delete_revision($obj_type, $object, $field, $instance, $langcode, &$items) {
}

/**
 * Define custom prepare_translation behavior for this module's field types.
 *
 * TODO: This hook may or may not survive in Field API.
 *
 * @param $obj_type
 *   The type of $object.
 * @param $object
 *   The object for the operation.
 * @param $field
 *   The field structure for the operation.
 * @param $instance
 *   The instance structure for $field on $object's bundle.
 * @param $langcode
 *   The language associated to $items.
 * @param $items
 *   $object->{$field['field_name']}[$langcode], or an empty array if unset.
 */
function hook_field_prepare_translation($obj_type, $object, $field, $instance, $langcode, &$items) {
}

/**
 * Define what constitutes an empty item for a field type.
 *
 * @param $item
 *   An item that may or may not be empty.
 * @param $field
 *   The field to which $item belongs.
 * @return
 *   TRUE if $field's type considers $item not to contain any data;
 *   FALSE otherwise.
 */
function hook_field_is_empty($item, $field) {
  if (empty($item['value']) && (string)$item['value'] !== '0') {
    return TRUE;
  }
  return FALSE;
}

/**
 * Expose Field API widget types.
 *
 * Widgets are Form API elements with additional processing capabilities.
 * Widget hooks are typically called by the Field Attach API during the
 * creation of the field form structure with field_attach_form().
 * @see hook_field_widget_info_alter().
 * @see hook_field_widget_form().
 * @see hook_field_widget_error().
 *
 * @return
 *   An array describing the widget types implemented by the module.
 *
 *   The keys are widget type names. To avoid name clashes, widget type
 *   names should be prefixed with the name of the module that exposes them.
 *
 *   The values are arrays describing the widget type, with the following
 *   key/value pairs:
 *   - label: The human-readable name of the widget type.
 *   - description: A short description for the widget type.
 *   - field types: An array of field types the widget supports.
 *   - settings: An array whose keys are the names of the settings available
 *     for the widget type, and whose values are the default values for those
 *     settings.
 *   - behaviors: (optional) An array describing behaviors of the widget.
 *     - multiple values:
 *       FIELD_BEHAVIOR_DEFAULT (default) if the widget allows the input of one
 *       single field value (most common case). The widget will be repeated for
 *       each value input.
 *       FIELD_BEHAVIOR_CUSTOM if one single copy of the widget can receive
 *       several field values. Examples: checkboxes, multiple select,
 *       comma-separated textfield...
 *     - default value:
 *       FIELD_BEHAVIOR_DEFAULT (default) if the widget accepts default values.
 *       FIELD_BEHAVIOR_NONE if the widget does not support default values.
 */
function hook_field_widget_info() {
    return array(
    'text_textfield' => array(
      'label' => t('Text field'),
      'field types' => array('text'),
      'settings' => array('size' => 60),
      'behaviors' => array(
        'multiple values' => FIELD_BEHAVIOR_DEFAULT,
        'default value' => FIELD_BEHAVIOR_DEFAULT,
      ),
    ),
    'text_textarea' => array(
      'label' => t('Text area (multiple rows)'),
      'field types' => array('text_long'),
      'settings' => array('rows' => 5),
      'behaviors' => array(
        'multiple values' => FIELD_BEHAVIOR_DEFAULT,
        'default value' => FIELD_BEHAVIOR_DEFAULT,
      ),
    ),
    'text_textarea_with_summary' => array(
      'label' => t('Text area with a summary'),
      'field types' => array('text_with_summary'),
      'settings' => array('rows' => 20, 'summary_rows' => 5),
      'behaviors' => array(
        'multiple values' => FIELD_BEHAVIOR_DEFAULT,
        'default value' => FIELD_BEHAVIOR_DEFAULT,
      ),
    ),
  );
}


/**
 * Perform alterations on Field API widget types.
 *
 * @param $info
 *   Array of informations on widget types exposed by hook_field_widget_info()
 *   implementations.
 */
function hook_field_widget_info_alter(&$info) {
  // Add a setting to a widget type.
  $info['text_textfield']['settings'] += array(
    'mymodule_additional_setting' => 'default value',
  );

  // Let a new field type re-use an existing widget.
  $info['options_select']['field types'][] = 'my_field_type';
}

/**
 * Return a single form element for a field widget.
 *
 * Field widget form elements should be based on the passed in $element, which
 * contains the base form element properties derived from the field
 * configuration.
 *
 * Field API will set the weight, field name and delta values for each
 * form element. If there are multiple values for this field, the
 * Field API will call this function as many times as needed.
 *
 * @param $form
 *   The entire form array.
 * @param $form_state
 *   The form_state, $form_state['values'][$field['field_name']]
 *   holds the field's form values.
 * @param $field
 *   The field structure.
 * @param $instance
 *   The field instance.
 * @param $langcode
 *   The language associated to $items.
 * @param $items
 *   Array of default values for this field.
 * @param $delta
 *   The order of this item in the array of subelements (0, 1, 2, etc).
 * @param $element
 *   A form element array containing basic properties for the widget:
 *   - #object_type: The name of the object the field is attached to.
 *   - #bundle: The name of the field bundle the field is contained in.
 *   - #field_name: The name of the field.
 *   - #columns: A list of field storage columns of the field.
 *   - #title: The sanitized element label for the field instance, ready for
 *     output.
 *   - #description: The sanitized element description for the field instance,
 *     ready for output.
 *   - #required: A Boolean indicating whether the element value is required;
 *     for required multiple value fields, only the first widget's values are
 *     required.
 *   - #delta: The order of this item in the array of subelements; see $delta
 *     above.
 *
 * @return
 *   The form elements for a single widget for this field.
 */
function hook_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  $element += array(
    '#type' => $instance['widget']['type'],
    '#default_value' => isset($items[$delta]) ? $items[$delta] : '',
  );
  return $element;
}

/**
 * Flag a field-level validation error.
 *
 * @param $element
 *   An array containing the form element for the widget. The error needs to be
 *   flagged on the right sub-element, according to the widget's internal
 *   structure.
 * @param $error
 *   An associative array with the following key-value pairs, as returned by
 *   hook_field_validate():
 *   - 'error': the error code. Complex widgets might need to report different
 *     errors to different form elements inside the widget.
 *   - 'message': the human readable message to be displayed.
 */
function hook_field_widget_error($element, $error) {
  form_error($element['value'], $error['message']);
}

/**
 * Expose Field API formatter types.
 *
 * Formatters handle the display of field values. Formatter hooks are typically
 * called by the Field Attach API field_attach_prepare_view() and
 * field_attach_view() functions.
 *
 * @see hook_field_formatter_info().
 * @see hook_field_formatter_info_alter().
 * @see hook_field_formatter_view().
 * @see hook_field_formatter_prepare_view().
 *
 * @return
 *   An array describing the formatter types implemented by the module.
 *
 *   The keys are formatter type names. To avoid name clashes, formatter type
 *   names should be prefixed with the name of the module that exposes them.
 *
 *   The values are arrays describing the formatter type, with the following
 *   key/value pairs:
 *   - label: The human-readable name of the formatter type.
 *   - description: A short description for the formatter type.
 *   - field types: An array of field types the formatter supports.
 *   - settings: An array whose keys are the names of the settings available
 *     for the formatter type, and whose values are the default values for
 *     those settings.
 */
function hook_field_formatter_info() {
  return array(
    'text_default' => array(
      'label' => t('Default'),
      'field types' => array('text', 'text_long', 'text_with_summary'),
    ),
    'text_plain' => array(
      'label' => t('Plain text'),
      'field types' => array('text', 'text_long', 'text_with_summary'),
    ),

    // The text_trimmed formatter displays the trimmed version of the
    // full element of the field. It is intended to be used with text
    // and text_long fields. It also works with text_with_summary
    // fields though the text_summary_or_trimmed formatter makes more
    // sense for that field type.
    'text_trimmed' => array(
      'label' => t('Trimmed'),
      'field types' => array('text', 'text_long', 'text_with_summary'),
    ),

    // The 'summary or trimmed' field formatter for text_with_summary
    // fields displays returns the summary element of the field or, if
    // the summary is empty, the trimmed version of the full element
    // of the field.
    'text_summary_or_trimmed' => array(
      'label' => t('Summary or trimmed'),
      'field types' => array('text_with_summary'),
    ),
  );
}


/**
 * Perform alterations on Field API formatter types.
 *
 * @param $info
 *   Array of informations on formatter types exposed by
 *   hook_field_field_formatter_info() implementations.
 */
function hook_field_formatter_info_alter(&$info) {
  // Add a setting to a formatter type.
  $info['text_default']['settings'] += array(
    'mymodule_additional_setting' => 'default value',
  );

  // Let a new field type re-use an existing formatter.
  $info['text_default']['field types'][] = 'my_field_type';
}

/**
 * Allow formatters to load information for field values being displayed.
 *
 * This should be used when a formatter needs to load additional information
 * from the database in order to render a field, for example a reference field
 * which displays properties of the referenced objects such as name or type.
 *
 * This hook is called after the field type's own hook_field_prepare_view().
 * @see hook_field_prepare_view()
 *
 * Unlike most other field hooks, this hook operates on multiple objects. The
 * $objects, $instances and $items parameters are arrays keyed by object id.
 * For performance reasons, information for all available objects should be
 * loaded in a single query where possible.
 *
 * @param $obj_type
 *   The type of $object.
 * @param $objects
 *   Array of objects being displayed, keyed by object id.
 * @param $field
 *   The field structure for the operation.
 * @param $instances
 *   Array of instance structures for $field for each object, keyed by object
 *   id.
 * @param $langcode
 *   The language the field values are to be shown in. If no language is
 *   provided the current language is used.
 * @param $items
 *   Array of field values for the objects, keyed by object id.
 * @param $displays
 *   Array of display settings to use for each object, keyed by object id.
 * @return
 *   Changes or additions to field values are done by altering the $items
 *   parameter by reference.
 */
function hook_field_formatter_prepare_view($obj_type, $objects, $field, $instances, $langcode, &$items, $displays) {

}

/**
 * Builds a renderable array for a field value.
 *
 * @param $obj_type
 *   The type of $object.
 * @param $object
 *   The object being displayed.
 * @param $field
 *   The field structure.
 * @param $instance
 *   The field instance.
 * @param $langcode
 *   The language associated to $items.
 * @param $items
 *   Array of values for this field.
 * @param $display
 *   The display settings to use, as found in the 'display' entry of instance
 *   definitions. The array notably contains the following keys and values;
 *   - type: The name of the formatter to use.
 *   - settings: The array of formatter settings.
 *
 * @return
 *   A renderable array for the $items, as an array of child elements keyed
 *   by numeric indexes starting from 0.
 */
function hook_field_formatter_view($obj_type, $object, $field, $instance, $langcode, $items, $display) {
  $element = array();
  $settings = $display['settings'];

  switch ($display['type']) {
    case 'sample_field_formatter_simple':
      // Common case: each value is displayed individually in a sub-element
      // keyed by delta. The field.tpl.php template specifies the markup
      // wrapping each value.
      foreach ($items as $delta => $item) {
        $element[$delta] = array('#markup' => $settings['some_setting'] . $item['value']);
      }
      break;

    case 'sample_field_formatter_themeable':
      // More elaborate formatters can defer to a theme function for easier
      // customization.
      foreach ($items as $delta => $item) {
        $element[$delta] = array(
          '#theme' => 'mymodule_theme_sample_field_formatter_themeable',
          '#data' => $item['value'],
          '#some_setting' => $settings['some_setting'],
        );
      }
      break;

    case 'sample_field_formatter_combined':
      // Some formatters might need to display all values within a single piece
      // of markup.
      $rows = array();
      foreach ($items as $delta => $item) {
        $rows[] = array($delta, $item['value']);
      }
      $element[0] = array(
        '#theme' => 'table',
        '#header' => array(t('Delta'), t('Value')),
        '#rows' => $rows,
      );
      break;
  }

  return $element;
}

/**
 * @} End of "ingroup field_type"
 */

/**
 * @ingroup field_attach
 * @{
 */

/**
 * Act on field_attach_form.
 *
 * This hook is invoked after the field module has performed the operation.
 *
 * See field_attach_form() for details and arguments.
 */
function hook_field_attach_form($obj_type, $object, &$form, &$form_state, $langcode) {
  $tids = array();

  // Collect every possible term attached to any of the fieldable entities.
  foreach ($objects as $id => $object) {
    foreach ($items[$id] as $delta => $item) {
      // Force the array key to prevent duplicates.
      $tids[$item['value']] = $item['value'];
    }
  }
  if ($tids) {
    $terms = array();

    // Avoid calling taxonomy_term_load_multiple because it could lead to
    // circular references.
    $query = db_select('taxonomy_term_data', 't');
    $query->fields('t');
    $query->condition('t.tid', $tids, 'IN');
    $query->addTag('term_access');
    $terms = $query->execute()->fetchAllAssoc('tid');

    // Iterate through the fieldable entities again to attach the loaded term data.
    foreach ($objects as $id => $object) {
      foreach ($items[$id] as $delta => $item) {
        // Check whether the taxonomy term field instance value could be loaded.
        if (isset($terms[$item['value']])) {
          // Replace the instance value with the term data.
          $items[$id][$delta]['taxonomy_term'] = $terms[$item['value']];
        }
        // Otherwise, unset the instance value, since the term does not exist.
        else {
          unset($items[$id][$delta]);
        }
      }
    }
  }
}

/**
 * Act on field_attach_load.
 *
 * This hook is invoked after the field module has performed the operation.
 *
 * Unlike other field_attach hooks, this hook accounts for 'multiple loads'.
 * Instead of the usual $object parameter, it accepts an array of objects,
 * indexed by object id. For performance reasons, information for all available
 * objects should be loaded in a single query where possible.
 *
 * The changes made to the objects' field values get cached by the field cache
 * for subsequent loads.
 *
 * See field_attach_load() for details and arguments.
 */
function hook_field_attach_load($obj_type, $objects, $age) {
}

/**
 * Act on field_attach_validate.
 *
 * This hook is invoked after the field module has performed the operation.
 *
 * See field_attach_validate() for details and arguments.
 */
function hook_field_attach_validate($obj_type, $object, &$errors) {
}

/**
 * Act on field_attach_submit.
 *
 * This hook is invoked after the field module has performed the operation.
 *
 * See field_attach_submit() for details and arguments.
 */
function hook_field_attach_submit($obj_type, $object, $form, &$form_state) {
}

/**
 * Act on field_attach_presave.
 *
 * This hook is invoked after the field module has performed the operation.
 *
 * See field_attach_presave() for details and arguments.
 */
function hook_field_attach_presave($obj_type, $object) {
}

/**
 * Act on field_attach_insert.
 *
 * This hook is invoked after the field module has performed the operation.
 *
 * See field_attach_insert() for details and arguments.
 */
function hook_field_attach_insert($obj_type, $object) {
}

/**
 * Act on field_attach_update.
 *
 * This hook is invoked after the field module has performed the operation.
 *
 * See field_attach_update() for details and arguments.
 */
function hook_field_attach_update($obj_type, $object) {
}

/**
 * Act on field_attach_preprocess.
 *
 * This hook is invoked while preprocessing the field.tpl.php template file.
 *
 * @param $variables
 *   The variables array is passed by reference and will be populated with field
 *   values.
 * @param $context
 *   An associative array containing:
 *   - obj_type: The type of $object; e.g. 'node' or 'user'.
 *   - object: The object with fields to render.
 *   - element: The structured array containing the values ready for rendering.
 */
function hook_field_attach_preprocess_alter(&$variables, $context) {
}

/**
 * Act on field_attach_delete.
 *
 * This hook is invoked after the field module has performed the operation.
 *
 * See field_attach_delete() for details and arguments.
 */
function hook_field_attach_delete($obj_type, $object) {
}

/**
 * Act on field_attach_delete_revision.
 *
 * This hook is invoked after the field module has performed the operation.
 *
 * See field_attach_delete_revision() for details and arguments.
 */
function hook_field_attach_delete_revision($obj_type, $object) {
}

/**
 * Act on field_attach_view.
 *
 * This hook is invoked after the field module has performed the operation.
 *
 * @param &$output
 *   The structured content array tree for all of $object's fields.
 * @param $context
 *   An associative array containing:
 *   - obj_type: The type of $object; e.g. 'node' or 'user'.
 *   - object: The object with fields to render.
 *   - view_mode: View mode, e.g. 'full', 'teaser'...
 *   - langcode: The language in which the field values will be displayed.
 */
function hook_field_attach_view_alter(&$output, $context) {
}

/**
 * Act on field_attach_create_bundle.
 *
 * This hook is invoked after the field module has performed the operation.
 *
 * See field_attach_create_bundle() for details and arguments.
 */
function hook_field_attach_create_bundle($obj_type, $bundle) {
}

/**
 * Act on field_attach_rename_bundle.
 *
 * This hook is invoked after the field module has performed the operation.
 *
 * See field_attach_rename_bundle() for details and arguments.
 */
function hook_field_attach_rename_bundle($obj_type, $bundle_old, $bundle_new) {
}

/**
 * Act on field_attach_delete_bundle.
 *
 * This hook is invoked after the field module has performed the operation.
 *
 * @param $obj_type
 *   The type of object; e.g. 'node' or 'user'.
 * @param $bundle
 *   The bundle that was just deleted.
 * @param $instances
 *   An array of all instances that existed for the bundle before it was
 *   deleted.
 */
function hook_field_attach_delete_bundle($obj_type, $bundle, $instances) {
}

/**
 * @} End of "ingroup field_attach"
 */

/**********************************************************************
 * Field Storage API
 **********************************************************************/

/**
 * @ingroup field_storage
 * @{
 */

/**
 * Expose Field API storage backends.
 *
 * @return
 *   An array describing the storage backends implemented by the module.
 *   The keys are storage backend names. To avoid name clashes, storage backend
 *   names should be prefixed with the name of the module that exposes them.
 *   The values are arrays describing the storage backend, with the following
 *   key/value pairs:
 *   - label: The human-readable name of the storage backend.
 *   - description: A short description for the storage backend.
 *   - settings: An array whose keys are the names of the settings available
 *     for the storage backend, and whose values are the default values for
 *     those settings.
 */
function hook_field_storage_info() {
  return array(
    'field_sql_storage' => array(
      'label' => t('Default SQL storage'),
      'description' => t('Stores fields in the local SQL database, using per-field tables.'),
      'settings' => array(),
    ),
  );
}

/**
 * Perform alterations on Field API storage types.
 *
 * @param $info
 *   Array of informations on storage types exposed by
 *   hook_field_field_storage_info() implementations.
 */
function hook_field_storage_info_alter(&$info) {
  // Add a setting to a storage type.
  $info['field_sql_storage']['settings'] += array(
    'mymodule_additional_setting' => 'default value',
  );
}

/**
 * Reveal the internal details about the storage for a field.
 *
 * For example, an SQL storage module might return the Schema API structure for
 * the table. A key/value storage module might return the server name,
 * authentication credentials, and bin name.
 *
 * Field storage modules are not obligated to implement this hook. Modules
 * that rely on these details must only use them for read operations.
 *
 * @param $field
 *   A field structure.
 * @param $instance
 *   A field instance structure.
 * @return
 *   An array of details.
 *    - The first dimension is a store type (sql, solr, etc).
 *    - The second dimension indicates the age of the values in the store
 *      FIELD_LOAD_CURRENT or FIELD_LOAD_REVISION.
 *    - Other dimensions are specific to the field storage module.
 */
function hook_field_storage_details($field, $instance) {
}

/**
 * Perform alterations on Field API storage details.
 *
 * The storage details are appended to the field instance structure after this
 * hook is invoked. Read and alter the $details only.
 *
 * @param $details
 *   An array of storage details for fields as exposed by
 *   hook_field_storage_details() implementations.
 * @param $field
 *   A field structure.
 * @param $instance
 *   A field instance structure.
 */
function hook_field_storage_details_alter(&$details, $field, $instance) {
}

/**
 * Load field data for a set of objects.
 *
 * @param $obj_type
 *   The entity type of object, such as 'node' or 'user'.
 * @param $objects
 *   The array of objects for which to load data, keyed by object id.
 * @param $age
 *   FIELD_LOAD_CURRENT to load the most recent revision for all
 *   fields, or FIELD_LOAD_REVISION to load the version indicated by
 *   each object.
 * @param $fields
 *   An array listing the fields to be loaded. The keys of the array are field
 *   ids, the values of the array are the object ids (or revision ids,
 *   depending on the $age parameter) to be loaded for each field.
 * @return
 *   Loaded field values are added to $objects. Fields with no values should be
 *   set as an empty array.
 */
function hook_field_storage_load($obj_type, $objects, $age, $fields) {
}

/**
 * Write field data for an object.
 *
 * @param $obj_type
 *   The entity type of object, such as 'node' or 'user'.
 * @param $object
 *   The object on which to operate.
 * @param $op
 *   FIELD_STORAGE_UPDATE when updating an existing object,
 *   FIELD_STORAGE_INSERT when inserting a new object.
 * @param $fields
 *   An array listing the fields to be written. The keys and values of the
 *   array are field ids.
 */
function hook_field_storage_write($obj_type, $object, $op, $fields) {
}

/**
 * Delete all field data for an object.
 *
 * @param $obj_type
 *   The entity type of object, such as 'node' or 'user'.
 * @param $object
 *   The object on which to operate.
 * @param $fields
 *   An array listing the fields to delete. The keys and values of the
 *   array are field ids.
 */
function hook_field_storage_delete($obj_type, $object, $fields) {
}

/**
 * Delete a single revision of field data for an object.
 *
 * Deleting the current (most recently written) revision is not
 * allowed as has undefined results.
 *
 * @param $obj_type
 *   The entity type of object, such as 'node' or 'user'.
 * @param $object
 *   The object on which to operate. The revision to delete is
 *   indicated by the object's revision id property, as identified by
 *   hook_fieldable_info() for $obj_type.
 * @param $fields
 *   An array listing the fields to delete. The keys and values of the
 *   array are field ids.
 */
function hook_field_storage_delete_revision($obj_type, $object, $fields) {
}

/**
 * Handle a field query.
 *
 * @param $field_name
 *   The name of the field to query.
 * @param $conditions
 *   See field_attach_query().
 *   A storage module that doesn't support querying a given column should raise
 *   a FieldQueryException. Incompatibilities should be mentioned on the module
 *   project page.
 * @param $options
 *   See field_attach_query(). All option keys are guaranteed to be specified.
 * @return
 *   See field_attach_query().
 */
function hook_field_storage_query($field_name, $conditions, $options) {
}

/**
 * Act on creation of a new field.
 *
 * @param $field
 *   The field structure being created.
 */
function hook_field_storage_create_field($field) {
}

/**
 * Act on deletion of a field.
 *
 * @param $field
 *   The field being deleted.
 */
function hook_field_storage_delete_field($field) {
}

/**
 * Act on deletion of a field instance.
 *
 * @param $instance
 *   The instance being deleted.
 */
function hook_field_storage_delete_instance($instance) {
}

/**
 * Act before the storage backends load field data.
 *
 * This hook allows modules to load data before the Field Storage API,
 * optionally preventing the field storage module from doing so.
 *
 * This lets 3rd party modules override, mirror, shard, or otherwise store a
 * subset of fields in a different way than the current storage engine.
 * Possible use cases include: per-bundle storage, per-combo-field storage...
 *
 * @param $obj_type
 *   The type of objects for which to load fields; e.g. 'node' or 'user'.
 * @param $objects
 *   An array of objects for which to load fields, keyed by object id.
 * @param $age
 *   FIELD_LOAD_CURRENT to load the most recent revision for all fields, or
 *   FIELD_LOAD_REVISION to load the version indicated by each object.
 * @param $skip_fields
 *   An array keyed by field ids whose data has already been loaded and
 *   therefore should not be loaded again. The values associated to these keys
 *   are not specified.
 * @return
 *   - Loaded field values are added to $objects. Fields with no values should
 *     be set as an empty array.
 *   - Loaded field ids are set as keys in $skip_fields.
 */
function hook_field_storage_pre_load($obj_type, $objects, $age, &$skip_fields) {
}

/**
 * Act before the storage backends insert field data.
 *
 * This hook allows modules to store data before the Field Storage API,
 * optionally preventing the field storage module from doing so.
 *
 * @param $obj_type
 *   The type of $object; e.g. 'node' or 'user'.
 * @param $object
 *   The object with fields to save.
 * @param $skip_fields
 *   An array keyed by field ids whose data has already been written and
 *   therefore should not be written again. The values associated to these keys
 *   are not specified.
 * @return
 *   Saved field ids are set set as keys in $skip_fields.
 */
function hook_field_storage_pre_insert($obj_type, $object, &$skip_fields) {
  if ($obj_type == 'node' && $object->status && _forum_node_check_node_type($object)) {
    $query = db_insert('forum_index')->fields(array('nid', 'title', 'tid', 'sticky', 'created', 'comment_count', 'last_comment_timestamp'));
    foreach ($object->taxonomy_forums as $language) {
      foreach ($language as $delta) {
        $query->values(array(
          'nid' => $object->nid,
          'title' => $object->title[LANGUAGE_NONE][0]['value'],
          'tid' => $delta['value'],
          'sticky' => $object->sticky,
          'created' => $object->created,
          'comment_count' => 0,
          'last_comment_timestamp' => $object->created,
        ));
      }
    }
    $query->execute();
  }
}

/**
 * Act before the storage backends update field data.
 *
 * This hook allows modules to store data before the Field Storage API,
 * optionally preventing the field storage module from doing so.
 *
 * @param $obj_type
 *   The type of $object; e.g. 'node' or 'user'.
 * @param $object
 *   The object with fields to save.
 * @param $skip_fields
 *   An array keyed by field ids whose data has already been written and
 *   therefore should not be written again. The values associated to these keys
 *   are not specified.
 * @return
 *   Saved field ids are set set as keys in $skip_fields.
 */
function hook_field_storage_pre_update($obj_type, $object, &$skip_fields) {
  $first_call = &drupal_static(__FUNCTION__, array());

  if ($obj_type == 'node' && $object->status && _forum_node_check_node_type($object)) {
    // We don't maintain data for old revisions, so clear all previous values
    // from the table. Since this hook runs once per field, per object, make
    // sure we only wipe values once.
    if (!isset($first_call[$object->nid])) {
      $first_call[$object->nid] = FALSE;
      db_delete('forum_index')->condition('nid', $object->nid)->execute();
    }
    // Only save data to the table if the node is published.
    if ($object->status) {
      $query = db_insert('forum_index')->fields(array('nid', 'title', 'tid', 'sticky', 'created', 'comment_count', 'last_comment_timestamp'));
      foreach ($object->taxonomy_forums as $language) {
        foreach ($language as $delta) {
          $query->values(array(
            'nid' => $object->nid,
            'title' => $object->title,
            'tid' => $delta['value'],
            'sticky' => $object->sticky,
            'created' => $object->created,
            'comment_count' => 0,
            'last_comment_timestamp' => $object->created,
          ));
        }
      }
      $query->execute();
      // The logic for determining last_comment_count is fairly complex, so
      // call _forum_update_forum_index() too.
      _forum_update_forum_index($object->nid);
    }
  }
}

/**
 * Act before the storage backend runs the query.
 *
 * This hook should be implemented by modules that use
 * hook_field_storage_pre_load(), hook_field_storage_pre_insert() and
 * hook_field_storage_pre_update() to bypass the regular storage engine, to
 * handle field queries.
 *
 * @param $field_name
 *   The name of the field to query.
 * @param $conditions
 *   See field_attach_query().
 *   A storage module that doesn't support querying a given column should raise
 *   a FieldQueryException. Incompatibilities should be mentioned on the module
 *   project page.
 * @param $options
 *   See field_attach_query(). All option keys are guaranteed to be specified.
 * @param $skip_field
 *   Boolean, always coming as FALSE.
 * @return
 *   See field_attach_query().
 *   The $skip_field parameter should be set to TRUE if the query has been
 *   handled.
 */
function hook_field_storage_pre_query($field_name, $conditions, $options, &$skip_field) {
}

/**
 * @} End of "ingroup field_storage"
 */

/**********************************************************************
 * Field CRUD API
 **********************************************************************/

/**
 * @ingroup field_crud
 * @{
 */

/**
 * Act on a field being created.
 *
 * This hook is invoked after the field is created and so it cannot modify the
 * field itself.
 *
 * TODO: Not implemented.
 *
 * @param $field
 *   The field just created.
 */
function hook_field_create_field($field) {
}

/**
 * Act on a field instance being created.
 *
 * This hook is invoked after the instance record is saved and so it cannot
 * modify the instance itself.
 *
 * @param $instance
 *   The instance just created.
 */
function hook_field_create_instance($instance) {
}

/**
 * Forbid a field update from occurring.
 *
 * Any module may forbid any update for any reason. For example, the
 * field's storage module might forbid an update if it would change
 * the storage schema while data for the field exists. A field type
 * module might forbid an update if it would change existing data's
 * semantics, or if there are external dependencies on field settings
 * that cannot be updated.
 *
 * @param $field
 *   The field as it will be post-update.
 * @param $prior_field
 *   The field as it is pre-update.
 * @param $has_data
 *   Whether any data already exists for this field.
 * @return
 *   Throws a FieldUpdateForbiddenException to prevent the update from occuring.
 */
function hook_field_update_field_forbid($field, $prior_field, $has_data) {
  // A 'list' field stores integer keys mapped to display values. If
  // the new field will have fewer values, and any data exists for the
  // abandonded keys, the field will have no way to display them. So,
  // forbid such an update.
  if ($has_data && count($field['settings']['allowed_values']) < count($prior_field['settings']['allowed_values'])) {
    // Identify the keys that will be lost.
    $lost_keys = array_diff(array_keys($field['settings']['allowed_values']), array_keys($prior_field['settings']['allowed_values']));
    // If any data exist for those keys, forbid the update.
    $count = field_attach_query($prior_field['id'], array('value', $lost_keys, 'IN'), 1);
    if ($count > 0) {
      throw new FieldUpdateForbiddenException("Cannot update a list field not to include keys with existing data");
    }
  }
}

/**
 * Act on a field being updated.
 *
 * This hook is invoked just after field is updated.
 *
 * @param $field
 *   The field as it is post-update.
 * @param $prior_field
 *   The field as it was pre-update.
 * @param $has_data
 *   Whether any data already exists for this field.
 */
function hook_field_update_field($field, $prior_field, $has_data) {
}

/**
 * Act on a field being deleted.
 *
 * This hook is invoked just after field is deleted.
 *
 * @param $field
 *   The field just deleted.
 */
function hook_field_delete_field($field) {
}

/**
 * Act on a field instance being updated.
 *
 * This hook is invoked after the instance record is saved and so it cannot
 * modify the instance itself.
 *
 * TODO: Not implemented.
 *
 * @param $instance
 *   The instance just updated.
 */
function hook_field_update_instance($instance) {
}

/**
 * Act on a field instance being deleted.
 *
 * This hook is invoked just after the instance is deleted.
 *
 * @param $instance
 *   The instance just deleted.
 */
function hook_field_delete_instance($instance) {
}

/**
 * Act on field records being read from the database.
 *
 * @param $field
 *   The field record just read from the database.
 */
function hook_field_read_field($field) {
}

/**
 * Act on a field record being read from the database.
 *
 * @param $instance
 *   The instance record just read from the database.
 */
function hook_field_read_instance($instance) {
}

/**
 * @} End of "ingroup field_crud"
 */

/**********************************************************************
 * TODO: I'm not sure where these belong yet.
 **********************************************************************/

/**
 * Determine whether the user has access to a given field.
 *
 * @param $op
 *   The operation to be performed. Possible values:
 *   - "edit"
 *   - "view"
 * @param $field
 *   The field on which the operation is to be performed.
 * @param $obj_type
 *   The type of $object; e.g. 'node' or 'user'.
 * @param $object
 *   (optional) The object for the operation.
 * @param $account
 *   (optional) The account to check, if not given use currently logged in user.
 * @return
 *   TRUE if the operation is allowed;
 *   FALSE if the operation is denied.
 */
function hook_field_access($op, $field, $obj_type, $object, $account) {
}
