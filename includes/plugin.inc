<?php
// $Id$

/**
 * @file
 * Plugin manager helper functions and form handlers.
 */

/**
 * Have to explicitly include because the registry is not available to us.
 */
require_once DRUPAL_ROOT . '/includes/filetransfer/filetransfer.inc';
require_once DRUPAL_ROOT . '/includes/filetransfer/ftp.inc';
require_once DRUPAL_ROOT . '/includes/filetransfer/ssh.inc';
require_once DRUPAL_ROOT . '/modules/update/update.module';
require_once DRUPAL_ROOT . '/modules/update/update.admin.inc';

/**
 * Shows the form used for choosing a FileTransfer type and supplying credentials.
 */
function plugin_filetransfer_form($form_state) {
  global $base_url;
  $form = array();

  $form['#action'] = $base_url . '/plugin.php';
  $form['#attached']['css'][] =  $base_url . '/modules/update/update.css';
  $form['#attached']['js'][]  =  $base_url . '/modules/update/update.js';
  
  // Get all the available ways to transfer files.
  $available_backends = module_invoke_all('filetransfer_backends');
  if (!count($available_backends)) {
    drupal_set_message(t('Unable to continue, no available methods of file transfer'), 'error');
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
    '#markup' => t('To continue please provide your server connection details'),
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
    $form['connection_settings'][$name] = array(
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
      $form['connection_settings']['change_connection_type'] = array(
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
  if (isset($form_state['values']['connection_settings'])) {
    $backend = $form_state['values']['connection_settings']['update_filetransfer_default'];
    $filetransfer = update_get_filetransfer($backend, $form_state['values']['connection_settings'][$backend]);
    try {
      if (!$filetransfer) {
        throw new Exception(t("Error, this type of connection protocol (%backend) doesn't exist.", array('%backend' => $backend)));
      }
      $filetransfer->connect();
    }
    catch (Exception $e) {
      form_set_error('connection_settings', $e->getMessage());
    }
  }
}

/**
 * Submit function to handle server creds being entered.
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
      // the database is set up.
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

        try {
          // Create the modules and themes directories if needed.
          _plugin_manager_create_and_setup_directories($ft);
        } catch (Exception $e) {
          drupal_set_message($e->getMessage());
          return;
        }
        
        // Now run the batch.
        plugin_manager_run_batch($ft);
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

/**
 * Creates the installation directories for all of the types of
 * projects that can be operated on.
 *
 * @param FileTransfer $filetransfer
 */
function _plugin_manager_create_and_setup_directories($filetransfer) {
  $updater_classes = update_get_updaters();
  
  foreach ($updater_classes as $class) {
    $parent_dir = '';
    
    $install_location = call_user_func("{$class}::getInstallDirectory");
    // Make the parent dir writable if need be and create the dir.
    if (!is_dir($install_location)) {
      $parent_dir = dirname($install_location);
      if (!is_writable($parent_dir)) {
        @chmod($parent_dir, 0755);
        /**
         * It is expected that this will fail if the conf_dir()
         * is owned by the FTP user. If the FTP user == web server,
         * it will succeed.
         */
      }
      try {
        $filetransfer->createDirectory($install_location);
        plugin_make_world_executable($filetransfer, $install_location);
      } catch (FileTransferException $e) {
        $message = $e->getMessage();
        $thrown_message = t('Unable to create %directory due to the following: %reason', array('%directory' => $install_location, '%reason' => $message));
        throw new Exception($throw_message);
      }
      // Put the parent directory back.
      @chmod($parent_dir, 0555);
    }
  }
}

/**
 * Ensures that the provided directory is executable by the server
 *
 * @param FileTransfer $filetransfer
 *   A FileTransfer classes used to chmod
 * @param string $directory
 *   The directory to set
 * @param bool $recursive
 *   If the chmod should be applied recursively.
 *
 * @throws FileTransferException
 * 
 * @return void
 */
function plugin_make_world_executable(&$filetransfer, $directory, $recursive = TRUE) {
  if (!is_executable($directory) || 1) {
    // Check the mode.  If it isn't world execute, try to chmod it.
    $perms = fileperms($directory);
    $new_perms = substr(decoct($perms), 2, -1) . "5";
    error_log('Setting perms on ' . $directory . ' to ' . $new_perms);
    $filetransfer->chmod($directory, $new_perms, $recursive);
  }
}

/**
 * Blantantly (and shamefully) (and regrettably) ripped from update.php.
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


/**
 * Runs the batch operation specified in $_SESSION['plugin_op']
 *
 * @global string $base_url
 * @param FileTransfer $filetransfer
 */
function plugin_manager_run_batch($filetransfer) {
  global $base_url;
  drupal_set_title('Installing updates');
  $batch = $_SESSION['plugin_op'];
  unset($_SESSION['plugin_op']);
  foreach ($batch['operations'] as $key => $args) {
    // Add the filetransfer class to the existing batch which was defined elsewhere.
    // This is obviously not pretty, but don't know a better method.
    $function = $args[0];
    if ($function == 'update_batch_copy_project') {
      $batch['operations'][$key][1][] = $filetransfer;
    }
  }
  
  batch_set($batch);
  batch_process($base_url . '/plugin.php', $base_url . '/plugin.php?batch=1');
}
