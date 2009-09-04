<?php
// $Id$

/**
 * Root directory of Drupal installation.
 */
define('DRUPAL_ROOT', getcwd());

/**
 * @file
 * Sub file to deploy and upgrade modules.
 */

/**
 * Global flag to identify update.php and plugin.php runs, and so
 * avoid various unwanted operations, such as hook_init() and
 * hook_exit() invokes, css/js preprocessing and translation, and
 * solve some theming issues. This flag is checked on several places
 * in Drupal code (not just plugin.php).
 */
define('MAINTENANCE_MODE', 'update');

require_once DRUPAL_ROOT . '/includes/plugin.inc';

// Some unavoidable errors happen because the database is not yet up-to-date.
// Our custom error handler is not yet installed, so we just suppress them.
// ini_set('display_errors', FALSE);
// JS: This is crazy.

if (realpath($_SERVER['SCRIPT_FILENAME']) == __FILE__) {

  // We prepare a minimal bootstrap for the update requirements check to avoid
  // reaching the PHP memory limit.
  require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
  require_once DRUPAL_ROOT . '/includes/update.inc';
  include_once DRUPAL_ROOT . '/includes/install.inc';
  include_once DRUPAL_ROOT . '/includes/batch.inc';
  include_once DRUPAL_ROOT . '/modules/update/update.admin.inc';

  // Determine if the current user has access to run update.php.
  drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
  drupal_set_title('Updating your site');
  drupal_maintenance_theme();
  
  if (empty($update_free_access) && $user->uid != 1) {
    print theme('update_page', $output, !$progress_page);
    exit;
  }
  global $base_url;
  
  //module_list(TRUE, FALSE, $module_list);

  // Turn error reporting back on. From now on, only fatal errors (which are
  // not passed through the error handler) will cause a message to be printed.
  ini_set('display_errors', TRUE);

  
  drupal_load_updates();

  update_fix_d7_requirements();
  update_fix_compatibility();
  
  if (isset($_SESSION['update_batch_results']) && $results = $_SESSION['update_batch_results']) {
    // If this variable is set it means that the user entered FTP details because
    // they needed to chmod the sites/default/* dirs.  We want to set them back.
    if (isset($_SESSION['filetransfer_settings'])) {
      $filetransfer_settings = $_SESSION['filetransfer_settings'];

      // This contains the user's sensative credentials, we have to make sure
      // that it gets unset.
      unset($_SESSION['filetransfer_settings']);
      
      $filetransfer_backend = variable_get('update_filetransfer_default', NULL);
      $ft = update_get_filetransfer($filetransfer_backend, $filetransfer_settings);
      // This sets the permissions to non-writable for everyone.
      _plugin_manager_create_and_setup_directories($ft, 0755);
    }

    //Clear the session out;
    unset($_SESSION['update_batch_results']);

    //Add the update.php functions and css
    include_once './includes/update.inc';
    drupal_add_css('modules/system/maintenance.css');

    if ($results['success']) {
      variable_set('site_offline', FALSE);
      drupal_set_message(t("Update was completed successfully!  Your site has been taken out of maintenance mode."));
    }
    else {
      drupal_set_message(t("Update failed! See the log below for more information. Your site is still in maintenance mode"), 'error');
    }

    $output = "";
    $output .= get_plugin_report($results['messages']);
    drupal_set_title('Update complete');


    $links = array(
      l('Administration pages', 'admin'),
      l('Front page', '<front>'),
    );
    $output .= theme('item_list', $links);

    print theme('update_page', $output);
    return;
  }
  
  if (empty($_SESSION['plugin_op'])) {
    $output = t("It appears you have reached this page in error.");
    print theme('update_page', $output);
    return;
  }
  else {
    if (_plugin_manager_writable_dir()) {
      drupal_set_title('Installing updates');
      // Go ahead, process the batch.
      // I just process the batch, because everything is set-up.
      // I chown'd god.
      $batch = $_SESSION['plugin_op'];
      unset($_SESSION['plugin_op']);
      batch_set($batch);
      batch_process(url($base_url . '/plugin.php', array('absolute' => TRUE)));
    }
    else {
      // Still needs to be refactored, big time.
      $output = drupal_render(drupal_get_form('plugin_filetransfer_form'));
      // Then when it comes back, we need to:
      // Set the modules directory to be writable - is this good enough?
    }
  }

  if (!empty($output)) {
    // We defer the display of messages until all updates are done.
    $progress_page = ($batch = batch_get()) && isset($batch['running']);
    print theme('update_page', $output, !$progress_page);
  }
}
