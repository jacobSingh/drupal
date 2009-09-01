<?php
// $Id$

/**
 * @file
 * Administrative screens and processing functions for adding new projects
 * (modules and themes) or updating existing ones.
 */
 
/**
 * Menu callback; main updates form.
 */
function update_update_form($form_state = array()) {
  $form = array();
  
  if (!isset($form_state['values'])) {
    // This is the first page of the update process.
    $form = _update_update_form_update_table();
  }
  elseif (is_array($form_state['values']) && (array_filter($form_state['storage']['projects']) || array_filter($form_state['values']['projects']))) {
    // This is the second page of the update process.
    $form =  _update_update_form_display_steps();
    $form['submit'] = array(
      '#type' => 'submit',
      '#name' => 'process_updates',
      '#value' => t('Install these updates'),
      '#weight' => 100,
    );
    //$form += _update_update_form_backend_chooser($form_state);
  }
  return $form;
}

/**
 * The first page of the update form.
 * This displays available updates in the form of a table.
 */
function _update_update_form_update_table() {
  drupal_add_css('misc/ui/ui.all.css');
  drupal_add_css('misc/ui/ui.dialog.css');
  drupal_add_js('misc/ui/ui.core.js', array('weight' => JS_LIBRARY + 5));
  drupal_add_js('misc/ui/ui.dialog.js', array('weight' => JS_LIBRARY + 6));
  $form['#attached_js'][] = DRUPAL_ROOT . '/misc/plugin.js';

  $form['#theme'] = 'update_available_updates_form';
  $form['projects'] = array();
  // First step.
  $options = array();
  $available = update_get_available(TRUE);
  if ($available) {
    module_load_include('inc', 'update', 'update.compare');
    $project_data = update_calculate_project_data($available);
    foreach ($project_data as $name => $project) {
      // Filter out projects which are dev versions, updated or core
      if (($project['install_type'] == 'dev') || ($project['project_type'] == 'core') || $project['status'] == UPDATE_CURRENT) {
        continue;
      }
      $options[$name]['title'] = l($project['title'], $project['link']);
      if ($project['project_type'] == 'theme') {
        $options[$name]['title'] .= ' ' . t('(Theme)');
      }
      $options[$name]['installed_version'] = $project['existing_version'];
      $recommended_version = $project['releases'][$project['recommended']];
      $options[$name]['recommended_version'] = $recommended_version['version'] . ' ' . l(t('(Release notes)'), $recommended_version['release_link'], array('attributes' => array('title' => t('Release notes for @project_name', array('@project_name' => $project['title'])))));
      if ($recommended_version['version_major'] != $project['existing_major']) {
        $options[$name]['recommended_version'] .= '<div title="Major upgrade warning" class="update-major-version-warning">' . t('This update is a major version update which means that it may not be backwards compatible with your currently running version.  It is recommended that you read the release notes and proceed at your own risk.') . '</div>';
      }

      switch ($project['status']) {
        case UPDATE_NOT_SECURE:
        case UPDATE_REVOKED:
          $options[$name]['title'] .= ' ' . t('(Security Update)');
          $options[$name]['#weight'] = -2;
          $type = 'security';
          break;
        case UPDATE_NOT_SUPPORTED:
          $type = 'unsupported';
          $options[$name]['title'] .= ' ' . t('(Unsupported)');
          $options[$name]['#weight'] = -1;
          break;
        case UPDATE_UNKNOWN:
        case UPDATE_NOT_FETCHED:
        case UPDATE_NOT_CHECKED:
        case UPDATE_NOT_CURRENT:
          $type = 'recommended';
          break;
        default:
          // Continues out of the switch and then out of the foreach.
          continue 2;
      }
    $options[$name]['#attributes'] = array('class' => array('update-' . $type));
    }
  }
  else {
    $form['message'] = array(
      '#markup' => t('There was a problem getting update information.  Please try again later.'),
    );
    return $form;
  }

  if (!count($options)) {
    $form['message'] = array(
      '#markup' => t('All of your projects are up to date.'),
    );
    return $form;
  }
  $form['projects'] = array(
    '#type' => 'tableselect',
    '#options' => $options,
    '#header' => array('title' => array('data' => t('Name'), 'class' => array('update-project-name')), 'installed_version' => t('Installed version'), 'recommended_version' => t('Recommended version')),
  );

  $form['#attached_css'][] = drupal_get_path('module', 'update') . '/update.css';
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Install these updates'),
    '#weight' => 100,
  );

  return $form;
}

function _update_update_form_display_steps() {
  $form['information']['#weight'] = -100;
  $form['information']['backup_header'] = array(
    '#prefix' => '<h3>',
    '#markup' => t('Step 1: Backup your site'),
    '#suffix' => '</h3>',
  );

  $form['information']['backup_message'] = array(
    '#prefix' => '<p>',
    '#markup' => t('We do not currently have a web based backup tool. <a href="@backup_url">Learn more about how to take a backup</a>.', array('@backup_url' => url('http://drupal.org/node/22281'))),
    '#suffix' => '</p>',
  );

  $form['information']['maint_header'] = array(
    '#prefix' => '<h3>',
    '#markup' => t('Step 2: Enter maintenance mode'),
    '#suffix' => '</h3>',
  );

  $form['information']['maint_message'] = array(
    '#prefix' => '<p>',
    '#markup' => t('It is strongly recommended that you put your site into maintenance mode while performing an update.'),
    '#suffix' => '</p>',
  );

  $form['information']['site_offline'] = array(
    '#title' => t('Perform updates with site in maintenance mode'),
    '#type' => 'checkbox',
    '#default_value' => TRUE,
  );

  return $form;
}


/**
 * Submit function for the main update form.
 *
 * @see update_update_form
 */
function update_update_form_submit($form, &$form_state) {
  global $base_url;
  switch ($form_state['clicked_button']['#name']) {
    case 'process_updates':
      
      $operations = array();
      foreach ($form_state['storage']['projects'] as $project) {
        // Put this in the begining (download everything first)
        $operations[] = array('update_batch_get_project', array($project));
        $latest_version = _update_get_recommended_version($project);
        // This is the .tar.gz from d.o.
        $url = $latest_version['download_link'];
        
        // Put these on the end
        $operations[] = array(
          'update_batch_copy_project',
          array(
            $project,
            $url,
          ),
        );
        $operations[] = array('update_batch_update_project', array($project));
      }

      $batch = array(
        'title' => t('Installing updates'),
        'init_message' => t('Preparing update operation'),
        'operations' => $operations,
        'finished' => 'update_batch_finished',
        'file' => drupal_get_path('module', 'update') . '/update.admin.inc',
      );

      $_SESSION['plugin_op'] = $batch;
      drupal_goto(url($base_url . '/plugin.php', array('absolute' => TRUE)));
      return;
      
      break;
    
    default:
    // This is the first page, and store the list of selected projects
      $form_state['storage']['projects'] = array_keys(array_filter($form_state['values']['projects']));
    break;
  }
}


function update_install_form(&$form_state) {
  $form = array();

  $form['project_url'] = array(
    '#type' => 'textfield',
    '#title' => t('URL'),
    '#description' => t('Paste the url to a Drupal module or theme archive (.tar.gz) here to install it.'),
  );

  $form['information'] = array(
    '#prefix' => '<strong>',
    '#markup' => 'Or',
    '#suffix' => '</strong>',
  );

  $form['project_upload'] = array(
    '#type' => 'file',
    '#title' => t('Upload a module or theme'),
    '#description' => t('Upload a Drupal module or theme (in .tar.gz format) to install it.'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Install'),
  );

  return $form;
}

function update_install_form_validate($form, &$form_state) {
  if (!($form_state['values']['project_url'] XOR !empty($_FILES['files']['name']['project_upload']))) {
    form_set_error('project_url', t('Unable to continue, please provide a url or upload a module / theme'));
    return;
    //$data = drupal_http_request($form_state['values']['url']);
  }
}

function update_install_form_submit($form, &$form_state) {
  global $base_url;
  
  if ($form_state['values']['project_url']) {
    $field = 'project_url';
    $local_cache = update_get_file($form_state['values']['project_url']);
    if (!$local_cache) {
      form_set_error($field, t('Unable to retreive Drupal project from %url', array('%url' => $form_state['values']['project_url'])));
      return;
    }
  }
  elseif ($_FILES['files']['name']['project_upload']) {
    $field = 'project_upload';
    // @todo: add some validators here.
    $finfo = file_save_upload($field, array(), NULL, FILE_EXISTS_REPLACE);
    // @todo: find out if the module is already instealled, if so, throw an error.
    $local_cache = $finfo->uri;
  }

  watchdog('update', 'Un-tarring ' . drupal_realpath($local_cache));

  $a = new Archive_Tar(drupal_realpath($local_cache));
  $files = $a->listContent();
  if (!$files) {
    form_set_error($field, t('Provided URL is not a .tar.gz archive', array('%url' => $form_state['values']['url'])));
    return;
  }
  
  $project = substr($files[0]['filename'], 0, -1); // Remove the trailing slash.

  // We want to make sure that the project is not already installed.
  // Currently, there is no way to do this :(  Bummer.
  if (drupal_get_path('module', $project)) {
    form_set_error($field, t('%project is already installed', array('%project' => $project)));
    return;
  }

  $operations = array();
  
  $operations[] = array(
    'update_batch_copy_project',
    array(
      $project,
      $local_cache,
    ),
  );
  // @todo: Finish this.
  //$operations[] = array('update_batch_enable_project', array($project));

  $batch = array(
    'title' => t('Installing %project', array('%project' => $project)),
    'init_message' => t('Preparing update operation'),
    'operations' => $operations,
    'finished' => 'update_batch_finished',
    'file' => drupal_get_path('module', 'update') . '/update.admin.inc',
  );

  $_SESSION['plugin_op'] = $batch;
  drupal_goto(url($base_url . '/plugin.php', array('absolute' => TRUE)));
  return;
}

/**
 * Batch operations related to installing / updating projects
 *
 **/


/**
 * Batch operation: download a project and put it in a temporary cache.
 *
 * @param string $project name of the project being installed
 * @param array &$context BatchAPI storage
 *
 * @return void;
 */
function update_batch_get_project($project, &$context) {
  if (!isset($context['results'][$project])) {
    $context['results'][$project] = array();
  }
  if (!isset($context['sandbox']['started'])) {
    $context['sandbox']['started'] = TRUE;
    $context['message'] = t('Downloading %project', array('%project' => $project));
    $context['finished'] = 0;
    return;
  }
  $latest_version = _update_get_recommended_version($project);
  if ($local_cache = update_get_file($latest_version['download_link'])) {
    watchdog('update', 'Downloaded %project to %local_cache', array('%project' => $project, '%local_cache' => $local_cache));
  }
  else {
    $context['success'] = FALSE;
    $content['results'][$project][] = t('Failed to download %project', array('%project' => $project));
  }
}

/**
 * Batch operation: copy a project to it's proper place.
 * For updates, will locate the current project and replace it.
 * For new installs, will download and try to determine the type from the info file
 * and then place it variable_get(update_default_{$type}_location) i.e. update_default_module_location().
 *
 * @todo Fix the $project param (refactor)
 * @param string $project Either name of the project being installed or a
 * @param string $url Location of a tarball to install if recommended version of $project not required
 * @param array &$context BatchAPI storage
 *
 * @return void
 */
function update_batch_copy_project($project, $url, &$context) {
  if (!isset($context['results'][$project])) {
    $context['results'][$project] = array();
  }
  if (!empty($context['results'][$project]['#abort'])) {
    $context['#finished'] = 1;
    return;
  }

  watchdog('update', $url);
  $local_cache = update_get_file($url);
  watchdog('update', $local_cache);
  //this extracts the file into the standard place.
  try {
    update_untar($local_cache);
  } catch (Exception $e) {
    _update_batch_create_message($context['results'][$project], $e->getMessage(), FALSE);
    $context['results'][$project]['#abort'] = TRUE;
    return;
  }

  $project_source_dir = DRUPAL_ROOT . '/' . file_directory_path('temporary') . '/update-extraction/' . $project;
  // Now check the dependencies.
  _update_check_dependencies($project_source_dir . "/$project.info");


  if ($project_info = _update_get_project_type_and_location($project)) {
    // This is an update
    $is_install = FALSE;
    $location = $project_info['location'];
  }
  else {
    //This is a fresh install, not an update
    $type = _update_get_project_type($project_location . '/' . $project . '.info');
    if (!$type) {
      _update_batch_create_message($context['results'][$project], t("Unable to determine project type for %name", array("%name" => $project)), FALSE);
      $context['results'][$project]['#abort'] = TRUE;
      return;
    }

    $default_locations = array(
      'module' => conf_path() . '/modules',
      'theme' => conf_path() . '/themes',
      'theme_engine' => conf_path() . '/themes/engines'
    );

    $location = variable_get("update_default_{$type}_location", $default_locations[$type]) . '/' . $project;
    $is_install = TRUE;
  }

  $project_destination_dir = DRUPAL_ROOT . '/' . $location;
  
  try {

    if (!$is_install) {
      _update_remove_directory($project_destination_dir);
    }
    _update_copy_directory($project_source_dir, $project_destination_dir);
  }
  catch (Exception $e) {
    _update_batch_create_message($context['results'][$project], t("Error installing / updating)"), FALSE);
    $context['results'][$project]['#abort'] = TRUE;
    return;
  }
  _update_batch_create_message($context['results'][$project], t('Installed %project_name successfully', array('%project_name' => $project)));
  $context['finished'] = 1;
}

/**
 * Set the batch to run the update functions.
 */
function update_batch_update_project($project, &$context) {
  if (!empty($context['results'][$project]['#abort'])) {
    $context['#finished'] = 1;
    return;
  }
  $project_info = _update_get_project_type_and_location($project);
  if ($project_info['type'] != 'module') {
    $context['finished'] = 1;
    return;
  }
  $operations = array();
  require_once './includes/update.inc';

  foreach (_update_get_schema_updates($project) as $update) {
    update_do_one($project, $update, $context);
  }
}

/**
 * Batch callback for when the batch is finished.
 */
function update_batch_finished($success, $results) {
  foreach ($results as $module => $messages) {
    if (!empty($messages['#abort'])) {
      $success = FALSE;
    }
  }
  $_SESSION['update_batch_results']['success'] = $success;
  $_SESSION['update_batch_results']['messages'] = $results;
}

/**
 *
 * Very stupid function to provide compatibility with update.php's
 * silly functions
 * Some better error handling is needed, but batch API doesn't seem to support any.
 *
 * @param array $project_results
 * @param string $message
 * @param bool $success
 */
function _update_batch_create_message(&$project_results, $message, $success = TRUE) {
  $next_number = count($project_results) + 1;
  $project_results[$next_number] = array(array('query' => $message, 'success' => $success));
}

/**
 * Theme main updates page.
 *
 * @ingroup themeable
 */
function theme_update_available_updates_form($form) {
  $last = variable_get('update_last_check', 0);
  $output = '<div class="update checked">' . ($last ? t('Last checked: @time ago', array('@time' => format_interval(REQUEST_TIME - $last))) : t('Last checked: never'));
  $output .= ' <span class="check-manually">(' . l(t('Check manually'), 'admin/reports/updates/check') . ')</span>';
  $output .= "</div>\n";
  $output .= drupal_render_children($form);
  return $output;
}

/**

 * Previously update.module
 *
 */


/**
 * Untar a file, using the Archive_Tar class.
 *
 * @param string $file the filename you wish to extract
 *
 * @return void
 * @throws Exception on failure.
 */
function update_untar($file) {
  $extraction_dir = DRUPAL_ROOT . '/' . file_directory_path('temporary') . '/update-extraction';
  if (!file_exists($extraction_dir)) {
    mkdir($extraction_dir);
  }
  $archive_tar = new Archive_Tar($file);
  if (!$archive_tar->extract($extraction_dir)) {
    throw new Exception(t('Unable to extact %file', array('%file' => $file)));
  }
}

/**
 * Get a filetransfer class.
 *
 * @param $method
 *   The filetransfer method to get the class for.
 * @param $overrides
 *   A set of overrides over the defaults.
 */
function update_get_filetransfer($method, $overrides = array()) {
  // Fire up the connection class
  $settings = variable_get("update_filetransfer_connection_settings_" . $method, array());
  $settings = array_merge($settings, $overrides);
  $available_backends = module_invoke_all('filetransfer_backends');
  $filetransfer = call_user_func_array($available_backends[$method]['class'] . '::factory', array(DRUPAL_ROOT, $settings));
  return $filetransfer;
}


/**
 * Helper function, returns a an associative array of a project's type and
 * location returns false on failure.
 */
function _update_get_project_type_and_location($name) {
  foreach (array('module', 'theme') as $type) {
    if ($dir = drupal_get_path($type, $name)) {
      return array('type' => $type, 'location' => $dir);
    }
  }
  return FALSE;
}

/**
 * Get's the latest release of a project
 *
 * @param string $name Name of the project
 * @return array An array of information about the latest recommended
 * release of the project
 */
function _update_get_latest_version($name) {
  if ($available = update_get_available(FALSE)) {
    module_load_include('inc', 'update', 'update.compare');
    $project_data = update_calculate_project_data($available);
    $project = $project_data[$name];
    return $project['releases'][$project['latest_version']];
  }
}

/**
 * Returns the available updates for a given module in an array
 *
 * @param $project
 *  The name of the module.
 */
function _update_get_schema_updates($project) {
  require_once './includes/install.inc';
  require_once './includes/update.inc';
  $module_info = _update_get_project_type_and_location($project);
  if ($module_info['type'] != 'module') {
    return array();
  }
  module_load_include('install', $project);

  if (!$updates = drupal_get_schema_versions($project)) {
    return array();
  }
  $updates_to_run = array();
  $modules_with_updates = update_get_update_list();
  //@JS: This is broken
  if ($updates = $modules_with_updates[$project]) {
    if ($updates['start']) {
      return $updates['pending'];
    }
  }
  return array();
}

/**
 * Helper function, given an info file, will determine what type of project it is
 *
 * @param string $info_file path to info file
 *
 * @return string project type, could be theme or module.
 */
function _update_get_project_type($info_file) {
  $info = drupal_parse_info_file($info_file);
  if ($info['engine']) {
    return 'theme';
    //we can assume this is a theme
  }
  return 'module';
}


/**
 * Get a file from the server, or if it was already downloaded, get the local
 * path to the file.
 *
 * @param $url
 *   The URL of the file on the server.
 *
 * @return string Path to local file
 */
function update_get_file($url) {
  $parsed_url = parse_url($url);
  $remote_schemes = array('http', 'https', 'ftp', 'ftps', 'smb', 'nfs');
  if (!in_array($parsed_url['scheme'], $remote_schemes)) {
    // This is a local file, just return the path.
    return drupal_realpath($url);
  }

  // Check the cache and download the file if needed.
  $local = DRUPAL_ROOT . '/' . file_directory_path('temporary') . '/update-cache/' . basename($parsed_url['path']);
  if (!file_exists(DRUPAL_ROOT . '/' . file_directory_path('temporary') . '/update-cache/')) {
    mkdir(DRUPAL_ROOT . '/' . file_directory_path('temporary') . '/update-cache/');
  }
  
  system_retrieve_file($url);
  
  if (!file_exists($local)) {
    return system_retrieve_file($url, $local);
  }
  else {
    return $local;
  }
}


/**
 * Get's the latest recommended release of a project.  This function will
 * prioritize updates on the same branch as the current version.
 *
 * @param string $name Name of the project
 * @return array An array of information about the latest recommended
 * release of the project
 */
function _update_get_recommended_version($name) {
  if ($available = update_get_available(FALSE)) {
    module_load_include('inc', 'update', 'update.compare');
    $project_data = update_calculate_project_data($available);
    $project = $project_data[$name];
    return $project['releases'][$project['recommended']];
  }
}

/**
 * Can read an info file and check if it has any unmet dependencies.
 *
 * @param string $info_file Pathname of info file to check.
 */
function _update_check_dependencies($info_file) {
  $files = system_get_module_data();

  // Remove hidden modules from display list.
  foreach ($files as $filename => $file) {
    if (!empty($file->info['hidden']) || !empty($file->info['required'])) {
      unset($files[$filename]);
    }
  }

  $info = drupal_parse_info_file(($info_file));

  $errors = array();
  if (!is_array($info['dependencies'])) {
      return array();
  }
  foreach ($info['dependencies'] as $module) {
    if (!isset($files[$module])) {
      $errors[$module] = t('@module (<span class="admin-missing">missing</span>)', array('@module' => drupal_ucfirst($module)));
    }
  }
  return $errors;
}


/**
 * Recursively coppies a directory.
 *
 * @todo: Should use RecursiveDirectoryIterator.
 *
 * @param string $src
 * @param string $dest
 * @param array $exclude_patterns
 */
function _update_copy_directory($src, $dest) {
  if (!file_exists($dest)) {
    mkdir($dest);
  }
  foreach (new RecursiveIteratorIterator(new RecursiveDirectoryIterator($src), RecursiveIteratorIterator::SELF_FIRST) as $filename => $file) {
    $relative_path = substr($filename, strlen($src));
    if ($file->isDir()) {
      mkdir($dest . '/' . $relative_path);
    }
    else {
      copy($filename, $dest . '/' . $relative_path);
    }
  }
}

/**
 * Duh. rm -Rf
 *
 * @param string $src The directory you want to zap.
 * @param string $jail An optional jail to make sure you don't wipe /.
 */
function _update_remove_directory($src, $jail = DRUPAL_ROOT) {
  foreach (new RecursiveIteratorIterator(new RecursiveDirectoryIterator($src), RecursiveIteratorIterator::CHILD_FIRST) as $filename => $file) {
    if (strpos($filename, $jail) != 0) {
      throw new Exception("Tried to delete $filename, but it is outside of the jail: $jail");
    }
    if ($file->isDir()) {
      rmdir($file->getPathName());
    }
    else {
      unlink($file->getPathName());
    }
  }
  rmdir($src);
}

/**
 *
 * Should enable a theme or a module with the same name as the project.
 * In the future, project-wise info files are recommended which could specify
 * the modules to install by default inside the project
 *
 * @TODO: This is not implemented yet.  Just a stub
 * @param <type> $project
 * @return <type>
 */
function update_batch_enable_project($project) {
  return;

  if (_update_get_project_type() == 'theme') {
    // @todo: Enable the theme.  Would be ideal to prompt post install
    // if it should be default as well.
  }
  else {
    module_enable($project);
  }
}