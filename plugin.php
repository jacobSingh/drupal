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

// Some unavoidable errors happen because the database is not yet up-to-date.
// Our custom error handler is not yet installed, so we just suppress them.
ini_set('display_errors', FALSE);

require_once DRUPAL_ROOT . '/includes/bootstrap.inc';

// Access check
drupal_bootstrap(DRUPAL_BOOTSTRAP_SESSION);
if (empty($update_free_access) && $user->uid != 1) {
  print theme('update_page', array('content' => $output, 'show_messages' => !$progress_page));
  exit;
}

// We prepare a minimal bootstrap.
drupal_bootstrap(DRUPAL_BOOTSTRAP_VARIABLES);

// This must go after drupal_bootstrap(), which unsets globals!
global $conf;

// Load barebones libraries.
require_once DRUPAL_ROOT . '/includes/common.inc';
require_once DRUPAL_ROOT . '/includes/batch.inc';
require_once DRUPAL_ROOT . '/includes/form.inc';
require_once DRUPAL_ROOT . '/includes/file.inc';
require_once DRUPAL_ROOT . '/includes/path.inc';

// Load module basics (needed for hook invokes).
require_once DRUPAL_ROOT . '/includes/module.inc';
require_once DRUPAL_ROOT . '/includes/session.inc';
require_once DRUPAL_ROOT . '/includes/entity.inc';

$module_list['system']['filename'] = 'modules/system/system.module';
$module_list['update']['filename'] = 'modules/update/update.module';
$module_list['user']['filename'] = 'modules/user/user.module';
module_list(TRUE, FALSE, FALSE, $module_list);
drupal_load('module', 'system');
drupal_load('module', 'update');
drupal_load('module', 'user');

// Includes needed specifically for the upgrade / install process.
require_once DRUPAL_ROOT . '/includes/plugin.inc';
require_once DRUPAL_ROOT . '/modules/update/update.admin.inc';

drupal_path_initialize();
drupal_language_initialize();

drupal_set_title(t('Updating your site'));
drupal_maintenance_theme();

/**
 * This case occurrs when the operation has been run, and a report needs to be shown.
 * Currently uses the update.php report, although should be changed.
 *
 * @todo: pretty up this report.
 */
if (isset($_SESSION['update_batch_results']) && $results = $_SESSION['update_batch_results']) {

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
  drupal_set_title(t('Update complete'));

  // These links are returned by the Updaters as "next tasks" to complete.
  $links = array();
  if (is_array($results['tasks'])) {
    $links += $results['tasks'];
  }

  $links = array_merge($links, array(
    l(t('Administration pages'), 'admin'),
    l(t('Front page'), '<front>'),
  ));

  $output .= theme('item_list', array('items' => $links));

  print theme('update_page', array('content' => $output));
  return;
}

/**
 * If a batch is running, let it run.
 */
if (isset($_GET['batch'])) {
  $output = _batch_page();
}
else {
  if (empty($_SESSION['plugin_op'])) {
    $output = t("It appears you have reached this page in error.");
    print theme('update_page', array('content' => $output));
    return;
  }
  elseif (!$batch = batch_get()) {
    // We have a batch to process, show the filetransfer form.
    $output = drupal_render(drupal_get_form('plugin_filetransfer_form'));
  }
}

if (!empty($output)) {
  // We defer the display of messages until all updates are done.
  $progress_page = ($batch = batch_get()) && isset($batch['running']);
  print theme('update_page', array('content' => $output, 'show_messages' => !$progress_page));
}
