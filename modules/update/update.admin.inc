<?php
// $Id$
require_once 'updaters.inc';

/**
 * @file
 * Administrative screens and processing functions for adding new projects
 * (modules and themes) or updating existing ones.
 */
 
/**
 * Menu callback; main updates form.
 */
function update_update_form($form, $form_state = array()) {
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
  $form['#attached']['js'][] = DRUPAL_ROOT . '/misc/plugin.js';

  $form['#theme'] = 'update_available_updates_form';
  $form['projects'] = array();
  // First step.
  $options = array();
  $available = update_get_available(TRUE);
  if ($available) {

    module_load_include('inc', 'update', 'update.compare');
    $project_data = update_calculate_project_data($available);
    foreach ($project_data as $name => $project) {
      // Filter out projects which are up2date already.
      if ($project['status'] == UPDATE_CURRENT) {
        continue;
      }

      $type = ""; // The type of update (security, recommended, manual, etc).
      $options[$name] = array(); // An array of projects to create the table with.

      
      $options[$name]['title'] = l($project['title'], $project['link']);
      if ($project['project_type'] == 'theme') {
        $options[$name]['title'] .= ' ' . t('(Theme)');
      }

      $options[$name]['installed_version'] = $project['existing_version'];
      $recommended_version = $project['releases'][$project['recommended']];

      // Mark projects which are dev versions with no stable release or core as manual installs
      if (($project['install_type'] == 'dev' && $recommended_version['version_extra'] == 'dev') || ($project['project_type'] == 'core')) {
        $options[$name]['#manual_update'] = TRUE;
      }

      $options[$name]['recommended_version'] = $recommended_version['version'] . ' ' . l(t('(Release notes)'), $recommended_version['release_link'], array('attributes' => array('title' => t('Release notes for @project_name', array('@project_name' => $project['title'])))));
      
      if ($recommended_version['version_major'] != $project['existing_major']) {
        $options[$name]['recommended_version'] .= '<div title="Major upgrade warning" class="update-major-version-warning">' . t('This update is a major version update which means that it may not be backwards compatible with your currently running version.  It is recommended that you read the release notes and proceed at your own risk.') . '</div>';
      }
      
      if (empty($type)) {
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

  $form['#attached']['css'][] = drupal_get_path('module', 'update') . '/update.css';

  $headers = array('title' => array('data' => t('Name'), 'class' => array('update-project-name')), 'installed_version' => t('Installed version'), 'recommended_version' => t('Recommended version'));
  // This is the kill switch for being able to perform updates
  if (update_plugin_manager_access()) {
    $manual_updates = array();
    foreach ($options as $key => $option) {
      if (!empty($option['#manual_update'])) {
        unset($option['#manual_update']);
        $manual_updates[] = $option;
        unset($options[$key]);
      }
    }
    if (count($options)) {
      $form['projects'] = array(
        '#type' => 'tableselect',
        '#options' => $options,
        '#header' => $headers,
      );

      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Install these updates'),
        '#weight' => 10,
      );
    }

    if ($manual_updates) {
      $form['manual_updates'] = array();
      $form['manual_updates']['#prefix'] = '<h2>' . t('Add-ons requiring manual updates') . '</h2>';
      $form['manual_updates']['#prefix'] .= '<p>' . t('Updates of development releases or Drupal core are not supported at this time');
      $form['manual_updates']['#type'] = "markup";
      $form['manual_updates']['#markup'] = theme('table', array('headers' => $headers, 'manual_updates' => $manual_updates));
      $form['manual_updates']['#weight'] = 20;
    }
    elseif (!count($options)) {
      $form['message'] = array(
        '#markup' => t('All of your projects are up to date.'),
      );
      return $form;
    }
  }
  else {
    $form['projects'] = array();
    $form['projects']['#type'] = "markup";
    $form['projects']['#markup'] = theme('table', array('headers' => $headers, 'options' => $options));
  }

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
 * @see update_update_form()
 */
function update_update_form_submit($form, &$form_state) {
  global $base_url;
  switch ($form_state['clicked_button']['#name']) {
    case 'process_updates':
      $operations = array();
      foreach ($form_state['storage']['projects'] as $project) {
        // @TODO: Present some type of warning when updating multi-site used modules.
        // Put this in the begining (download everything first)
        $operations[] = array('update_batch_get_project', array($project));
        $latest_version = _update_get_recommended_version($project);
        // This is the .tar.gz from d.o.
        $url = $latest_version['download_link'];
        
        $operations[] = array(
          'update_batch_copy_project',
          array(
            $project,
            $url,
          ),
        );
      }

      $batch = array(
        'title' => t('Installing updates'),
        'init_message' => t('Preparing update operation'),
        'operations' => $operations,
        'finished' => 'update_batch_finished',
        'file' => drupal_get_path('module', 'update') . '/update.admin.inc',
      );

      $_SESSION['plugin_op'] = $batch;

      if ($form_state['values']['site_offline'] == TRUE) {
        // Put site in offline mode.
        variable_set('site_offline', TRUE);
      }
      drupal_goto($base_url . '/plugin.php');
      return;
      
      break;
    
    default:
      $form_state['rebuild'] = TRUE;
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
    '#description' => t('Paste the url to a Drupal module or theme archive (.tar.gz) here to install it. (e.g http://ftp.drupal.org/files/projects/projectname.tar.gz)'),
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

  $archive_tar = new Archive_Tar(drupal_realpath($local_cache));

  $files = $archive_tar->listContent();
  if (!$files) {
    form_set_error($field, t('Provided URL is not a .tar.gz archive', array('%url' => $form_state['values']['url'])));
    return;
  }

  $project = drupal_substr($files[0]['filename'], 0, -1); // Unfortunately, we can only use the directory name for this. :(
  
  $project_location = DRUPAL_ROOT . '/' . file_directory_path('temporary') . '/update-extraction/' . $project;
  update_untar(drupal_realpath($local_cache));

  $updater = Updater::factory($project_location);
  $project_title = Updater::getProjectTitle($project_location);

  if (!$project) {
    form_set_error($field, t('Unable to determine %project name', array('%project' => $project_title)));
  }
  
  if ($updater->isInstalled()) {
    form_set_error($field, t('%project is already installed', array('%project' => $project_title)));
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

  $batch = array(
    'title' => t('Installing %project', array('%project' => $project_title)),
    'init_message' => t('Preparing update operation'),
    'operations' => $operations,
    'finished' => 'update_batch_finished',
    'file' => drupal_get_path('module', 'update') . '/update.admin.inc',
  );
  
  $_SESSION['plugin_op'] = $batch;
  drupal_goto($base_url . '/plugin.php');
  return;

}

/**
 * Batch operations related to installing / updating projects
 */

/**
 * Batch operation: download a project and put it in a temporary cache.
 *
 * @param string $project name of the project being installed
 * @param array &$context BatchAPI storage
 *
 * @return void;
 */
function update_batch_get_project($project, &$context) {
  if (!isset($context['results']['log'])) {
    $context['results']['log'] = array();
  }
  if (!isset($context['results']['log'][$project])) {
    $context['results']['log'][$project] = array();
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
 * @param string $filetransfer FileTransfer class
 * @param array &$context BatchAPI storage
 *
 * @return void
 */
function update_batch_copy_project($project, $url, $filetransfer, &$context) {

  // Initialize some variables
  if (!isset($context['results']['log'])) {
    $context['results']['log'] = array();
  }

  if (!isset($context['results']['tasks'])) {
    $context['results']['tasks'] = array();
  }

  /**
   * Unfortuantely, because the batch API uses a session and a connection
   * pointer will be lost between requests, when doing an update of multiple
   * modules, we need to unset the connection pointer to re-init a connect.
   */
  unset($filetransfer->connection);

  if (!isset($context['results']['log'][$project])) {
    $context['results']['log'][$project] = array();
  }
  
  if (!empty($context['results']['log'][$project]['#abort'])) {
    $context['#finished'] = 1;
    return;
  }

  $local_cache = update_get_file($url);

  // This extracts the file into the standard place.
  try {
    update_untar($local_cache);
  }
  catch (Exception $e) {
    _update_batch_create_message($context['results']['log'][$project], $e->getMessage(), FALSE);
    $context['results']['log'][$project]['#abort'] = TRUE;
    return;
  }
  
  $project_source_dir = DRUPAL_ROOT . '/' . file_directory_path('temporary') . '/update-extraction/' . $project;
  $updater = Updater::factory($project_source_dir);

  try {
    if ($updater->isInstalled()) {
      // This is an update.
      $tasks = $updater->update($filetransfer);
    }
    else {
      $tasks = $updater->install($filetransfer);
    }
  }
  catch (UpdaterError $e) {
    _update_batch_create_message($context['results']['log'][$project], t("Error installing / updating"), FALSE);
    _update_batch_create_message($context['results']['log'][$project], $e->getMessage(), FALSE);
    $context['results']['log'][$project]['#abort'] = TRUE;
    return;
  }

  _update_batch_create_message($context['results']['log'][$project], t('Installed %project_name successfully', array('%project_name' => $project)));
  $context['results']['tasks'] += $tasks;

  $context['finished'] = 1;
}

/**
 * Batch callback for when the batch is finished.
 */
function update_batch_finished($success, $results) {
  foreach ($results['log'] as $module => $messages) {
    if (!empty($messages['#abort'])) {
      $success = FALSE;
    }
  }
  $_SESSION['update_batch_results']['success'] = $success;
  $_SESSION['update_batch_results']['messages'] = $results['log'];
  $_SESSION['update_batch_results']['tasks'] = $results['tasks'];
}

/**
 * Helper function to create a structure of log messages similar to update.php
 *
 * @todo: Improve the update.php structure, or just fix this problem, or decide
 *        that plugin.php should never run updates.
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
 * @param $variables
 *   form: The form.
 * 
 * @ingroup themeable
 */
function theme_update_available_updates_form($variables) {
  extract($variables);
  $last = variable_get('update_last_check', 0);
  $output = '<div class="update checked">' . ($last ? t('Last checked: @time ago', array('@time' => format_interval(REQUEST_TIME - $last))) : t('Last checked: never'));
  $output .= ' <span class="check-manually">(' . l(t('Check manually'), 'admin/reports/updates/check') . ')</span>';
  $output .= "</div>\n";
  $output .= drupal_render_children($form);
  return $output;
}

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
  $archive_tar = new Archive_Tar(drupal_realpath($file));
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
  $filetransfer = call_user_func_array(array($available_backends[$method]['class'], 'factory'), array(DRUPAL_ROOT, $settings));
  return $filetransfer;
}

/**
 * Coppies a file from $url to the temporary directory for updates.
 *
 * If the file has already been downloaded, returns the the local path.
 *
 * @param $url
 *   The URL of the file on the server.
 *
 * @return string
 *   Path to local file.
 */
function update_get_file($url) {
  $parsed_url = parse_url($url);
  $remote_schemes = array('http', 'https', 'ftp', 'ftps', 'smb', 'nfs');
  if (!in_array($parsed_url['scheme'], $remote_schemes)) {
    // This is a local file, just return the path.
    return drupal_realpath($url);
  }

  // Check the cache and download the file if needed.
  $local = 'temporary://update-cache/' . basename($parsed_url['path']);

  if (!file_exists(DRUPAL_ROOT . '/' . file_directory_path('temporary') . '/update-cache/')) {
    mkdir(DRUPAL_ROOT . '/' . file_directory_path('temporary') . '/update-cache/');
  }
  
  if (!file_exists($local)) {
    return system_retrieve_file($url, $local);
  }
  else {
    return $local;
  }
}


/**
 * Gets the latest recommended release of a project.
 *
 * This function will prioritize updates on the same branch as the current version.
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
 * Returns a list of classes which implement the DrupalUpdater interface
 *
 * @return array
 */
function update_get_updaters() {
  $updaters = drupal_static(__FUNCTION__);
  if (!$updaters) {
    $updaters = array();
    foreach (get_declared_classes() as $class) {
      if (in_array('DrupalProjectUpdater', class_implements($class))) {
        $updaters[] = $class;
      }
    }
  }
  return $updaters;
}