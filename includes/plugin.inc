<?php

/**
 * Have to explicitly include because the registry is not available to us.
 */
require_once DRUPAL_ROOT . '/includes/filetransfer/filetransfer.inc';
require_once DRUPAL_ROOT . '/includes/filetransfer/ftp.inc';
require_once DRUPAL_ROOT . '/includes/filetransfer/ssh.inc';

function plugin_filetransfer_form($form_state) {
  global $base_url;
  $form = array();
  $form['#action'] = url($base_url . '/plugin.php', array('absolute' => TRUE));

  $form['#attached_css'][] = drupal_get_path('module', 'update') . '/update.css';
  $form['#attached_js'][] = drupal_get_path('module', 'update') . '/update.js';
  // Get all the available ways to transfer files.
  $available_backends = module_invoke_all('filetransfer_backends');
  if (!count($available_backends)) {
    // @TODO: Clean up this error handling
    drupal_set_message(t('Unable to continue, not available methods of file transfer'), 'error');
    return array();
  }

  uasort($available_backends, 'drupal_sort_weight');

  // Decide on a default backend.
  if (isset($form_state['values']['connection_settings']['update_filetransfer_default'])) {
    $update_filetransfer_default = $form_state['values']['connection_settings']['update_filetransfer_default'];
  }
  elseif ($update_filetransfer_default = variable_get('update_filetransfer_default', NULL));
  else {
    $update_filetransfer_default = key($available_backends);
  }

  $form['information']['main_header'] = array(
    '#prefix' => '<h3>',
    '#markup' => t('To upload your plugin please provide your server connection details'),
    '#suffix' => '</h3>',
  );

  $form['connection_settings']['#tree'] = TRUE;
  $form['connection_settings']['update_filetransfer_default'] = array(
    '#type' => 'select',
    '#title' => t('Connection method'),
    '#default_value' => $update_filetransfer_default,
    '#weight' => -10,
  );

  /**
     * Here we create two submit buttons.  For a JS enabled client,
     * they will only ever see submit_process.
     *
     * However, if a client doesn't have JS enabled, they will see submit_connection
     * on the first form (whden picking what filetranfer type to use, and
     * submit_process on the second one (which leads to the actual operation)
     *
     */
    $form['submit_connection'] = array(
      '#prefix' => "<br style='clear:both'/>",
      '#name' => 'enter_connection_settings', // This is later changed in JS.
      '#type' => 'submit',
      '#value' => t('Enter connetion settings'), // As is this.  @see update.js.
      '#weight' => 100,
    );

    $form['submit_process'] = array(
      '#name' => 'process_updates', // This is later changed in JS.
      '#type' => 'submit',
      '#value' => t('Process Updates'), // As is this.  @see update.js
      '#weight' => 100,
      '#attributes' => array('style' => 'display:none'),
    );

  // Build a hidden fieldset for each one.
  foreach ($available_backends as $name => $backend) {
    $form['connection_settings']['update_filetransfer_default']['#options'][$name] = $backend['title'];
    $form['connection_settings'][$name] = array (
      '#type' => 'fieldset',
      '#attributes' => array('class' => "filetransfer-$name filetransfer"),
      '#title' => t('@backend connection settings', array('@backend' => $backend['title'])),
    );

    $current_settings = variable_get("update_filetransfer_connection_settings_" . $name, array());
    $form['connection_settings'][$name] += system_get_filetransfer_settings_form($name, $current_settings);

    // Start non-JS code.
    if (isset($form_state['values']['connection_settings']['update_filetransfer_default']) && $form_state['values']['connection_settings']['update_filetransfer_default'] == $name) {
      /**
       * This means the form has been submitted by a non-JS user, and they have
       * choosen a transfer type.
       */

      // If the user switches from JS to non-JS, Drupal (and Batch API) will barf.
      // This is a known bug: http://drupal.org/node/229825.
      setcookie('has_js', '', time() - 3600, '/');
      unset($_COOKIE['has_js']);

      // Change the submit button to the submit_process one.
      $form['submit_process']['#attributes'] = array();
      unset($form['submit_connection']);
      //
      // Active the proper filetransfer settings form.
      $form['connection_settings'][$name]['#attributes']['style'] = 'display:block';
      // Disable the select box.
      $form['connection_settings']['update_filetransfer_default']['#disabled'] = TRUE;

      // Create a button for changing the type of connection.
      $form['connection_settings']['change_connection_type'] = array (
        '#name' => 'change_connection_type',
        '#type' => 'submit',
        '#value' => t('Change connection type'),
        '#weight' => -5,
        '#attributes' => array('class' => 'filetransfer-change-connection-type'),
      );
    }
    //End non-JS code
  }
  return $form;
}

/**
 * Validate function for the main update form.
 *
 * @see update_update_form
 */
function plugin_filetransfer_form_validate($form, &$form_state) {
  if ($form_state['clicked_button']['#name'] == 'process_updates') {
    if (isset($form_state['values']['connection_settings'])) {
      $backend = $form_state['values']['connection_settings']['update_filetransfer_default'];
      $filetransfer = update_get_filetransfer($backend, $form_state['values']['connection_settings'][$backend]);
      try {
        if (!$filetransfer) {
          throw new Exception($backend . " error, this type of connection protocol doesn't exist.");
        }
        $filetransfer->connect();
      }
      catch(Exception $e) {
        form_set_error('connection_settings', $e->getMessage());
      }
    }
  }
}



/**
 * Submit function for the main update form.
 *
 * @see update_update_form
 */
function plugin_filetransfer_form_submit($form, &$form_state) {
  global $base_url;
  switch ($form_state['clicked_button']['#name']) {
    case 'process_updates':

      // Save the connection settings to the DB.
      $filetransfer_backend = $form_state['values']['connection_settings']['update_filetransfer_default'];

      // If the database is available then try to save our settings.  We have to make sure it is available
      // since this code could potentially (will likely) be called during the installation process, before
      // the database is setup.
      if (db_is_active()) {
        $connection_settings = array();
        foreach ($form_state['values']['connection_settings'][$filetransfer_backend] as $key => $value) {
          if (!isset($form['connection_settings'][$filetransfer_backend][$key]['#filetransfer_save']) ||
            $form['connection_settings'][$filetransfer_backend][$key]['#filetransfer_save']) {
            $connection_settings[$key] = $value;
          }
        }
        // Set this one as the default update method.
        variable_set('update_filetransfer_default', $filetransfer_backend);
        // Save the connection settings minus the password.
        variable_set("update_filetransfer_connection_settings_" . $filetransfer_backend, $connection_settings);

        // chmod the modules directory to 755
        $ft = update_get_filetransfer($filetransfer_backend, $form_state['values']['connection_settings'][$filetransfer_backend]);

        // I don't like doing this, but I don't know how else to store this information but in the session.
        // See plugin.php for the consumption and deletion of this dangerous variable.
        // Should also have some sort of forced clearing I suppose.

        $_SESSION['filetransfer_settings'] = $form_state['values']['connection_settings'][$filetransfer_backend];

        _plugin_manager_create_and_setup_directories($ft, 0777);
        drupal_goto(url($base_url . '/plugin.php', array('absolute' => TRUE)));
      }
      break;

    case 'enter_connection_settings':
      $form_state['rebuild'] = TRUE;
      break;

    case 'change_connection_type':
      $form_state['rebuild'] = TRUE;
      unset($form_state['values']['connection_settings']['update_filetransfer_default']);
      break;
  }
}

function _plugin_manager_create_and_setup_directories($filetransfer, $mode = 0755) {

  $theme_dir = _plugin_get_theme_dir();
  $module_dir = _plugin_get_module_dir();
  // If there are no module or theme directories create them.
  if (!is_dir($theme_dir) || !is_dir($module_dir)) {
    if (!is_writable(DRUPAL_ROOT . '/' . conf_path())) {
      $filetransfer->chmod(DRUPAL_ROOT . '/' . conf_path(), $mode, FALSE);
    }
    _plugin_manager_create_dirs();
  }

  if (!is_writable($theme_dir)) {
    $filetransfer->chmod($theme_dir, $mode, TRUE);
  }
  if (!is_writable($module_dir)) {
    $filetransfer->chmod($module_dir, $mode, TRUE);
  }
}

function _plugin_manager_create_dirs() {
  $theme_dir = _plugin_get_theme_dir();
  $module_dir = _plugin_get_module_dir();
  
  if (!is_dir($theme_dir)) {
    mkdir($theme_dir);
  }
  if (!is_dir($module_dir)) {
    mkdir($module_dir);
  }
}

/**
 * Determines if the modules or themes directories are writable.
 *
 *
 * @param string $type Which type of plugin to check for. (Currently not implemented)
 * @return boolean
 */
function _plugin_manager_writable_dir($type = 'both') {
  switch ($type) {
    case 'both':
      $test[] = conf_path() . '/modules';
      $test[] = conf_path() . '/themes';
      break;
    case 'module':
      $test[] = conf_path() . '/modules';
      break;
    case 'theme':
      $test[] = conf_path() . '/themes';
      break;
  }

  foreach($test as $path) {
    if (!is_writable($path)) {
      return FALSE;
    }
  }
  return TRUE;
}

function _plugin_get_module_dir() {
  return DRUPAL_ROOT . '/' . conf_path() . '/modules';
}

function _plugin_get_theme_dir() {
  return DRUPAL_ROOT . '/' . conf_path() . '/themes';
}

/**
 * Blantantly (and shamefully) ripped from update.php.
 *
 * Interim measure, this code is not very clear.
 * @param array $messages
 * @return string Report similar to update.php
 */
function get_plugin_report($messages) {
  $output = "";
  // Output a list of queries executed
  if (!empty($messages)) {
    $output .= '<div id="update-results">';
    $output .= '<h2>The following actions were executed</h2>';
    foreach ($messages as $module => $updates) {
      $output .= '<h3>' . $module . '</h3>';
      foreach ($updates as $number => $queries) {
        if ($number != '#abort') {
          $output .= '<h4>Update #' . $number . '</h4>';
          $output .= '<ul>';
          foreach ($queries as $query) {
            if ($query['success']) {
              $output .= '<li class="success">' . $query['query'] . '</li>';
            }
            else {
              $output .= '<li class="failure"><strong>Failed:</strong> ' . $query['query'] . '</li>';
            }
          }
          if (!count($queries)) {
            $output .= '<li class="none">No queries</li>';
          }
        }
        $output .= '</ul>';
      }
    }
    $output .= '</div>';
  }
  return $output;
}
