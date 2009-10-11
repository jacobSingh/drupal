<?php
// $Id$

/**
 * @file: Classes used for updating various things in Drupal via FileTransfers.
 *
 */
 
/**
 * Interface for a class which can update a Drupal project.
 *
 * An Updater currently serves the following purposes:
 *   - It can take a given directory, and determine if it can operate on it.
 *   - It can move the contents of that directoy into the appropriate place
 *    on the system using FileTransfer classes.
 *   - It can return a list of "next steps" after an update or install.
 *     - In the future, it will most likely perform some of those steps as well.
 */
interface DrupalProjectUpdater {

  /**
   * Checks if the project is installed.
   * @return bool
   */
  public function isInstalled();

  /**
   * Returns the system name of the project.
   * 
   * @param string $directory
   *  A directory containing a project.
   */
  public static function getProjectName($directory);

  /**
   * @return string an absolute path to the default install location.
   */
  public static function getInstallDirectory();

  /**
   * Determines if the Updater can handle the project provided in $directory
   * @todo: Provide something more rational here, like a project spec file.
   * @param string $directory
   *
   * @return bool
   */
  public static function canUpdateDirectory($directory);

  /**
   * Actions to run after an install has occurred.
   */
  function postInstall();

  /**
   * Actions to run after an update has occurred.
   */
  function postUpdate();
}

/**
 * Base class for Updaters used in Drupal.
 */
class Updater {

  /**
   * @var string $source Directory to install from.
   */
  public $source;
  
  function __construct($source) {
    $this->source = $source;
    $this->name = self::getProjectName($source);
    $this->title = self::getProjectTitle($source);
  }

  /**
   * Returns an Updater of the approiate type depending on the source provided.
   * If a directory is provided which contains a module, will return a ModuleUpdater.
   *
   * @param string $source
   *   Directory of a Drupal project
   * 
   * @return object Updater
   */
  static function factory($source) {
    if (is_dir($source)) {
      $updater = self::getUpdaterFromDirectory($source);
    }
    else {
      throw new UpdaterError('Unable to determine the type of the source directory');
    }
    return new $updater($source);
  }

  /**
   * Determines which Updater class can operate on the given directory.
   * 
   * @param string $directory
   *   Extracted Drupal project.
   * @return string
   *   The class name which can work with this project type.
   */
  static function getUpdaterFromDirectory($directory) {
    // Gets a list of possible implementing classes
    $updaters = update_get_updaters();
    foreach ($updaters as $updater) {
      if (call_user_func("{$updater}::canUpdateDirectory", $directory)) {
        return $updater;
      }
    }
    throw new UpdaterError("Cannot determine the type of project");
  }

  /**
   * Figure out what the most important (or only) info file is in a directory.
   * 
   * Since there is no enforcement of which info file is the project's "main" info
   * file, this will get one with the same name as the directory, or the first
   * one it finds.  Not ideal, but needs a larger solution.
   *
   * @param string $directory
   *  Directory to search in.
   * @return string
   *  Path to the info file.
   */
  public static function findInfoFile($directory) {
    $info_files = file_scan_directory($directory, '/.*\.info/');
    if (!$info_files) {
      return FALSE;
    }
    foreach ($info_files as $info_file) {
      if (drupal_substr($info_file->filename, 0, -5) == basename($directory)) {
        // Info file Has the same name as the directory, return it.
        return $info_file->uri;
      }
    }
    // Otherwise, return the first one.
    $info_file = array_shift($info_files);
    return $info_file->uri;
  }

  /**
   * Gets the name of the project directory (basename).
   *
   * @todo: It would be nice, if projects contained an info file which could
   *        provide their canonical name.
   * 
   * @param string $directory
   * @return string
   */
  public static function getProjectName($directory) {
    return basename($directory);
  }


  /**
   * Returns the project name from a Drupal info file.
   *
   * @param string $directory
   *   Directory to search for the info file.
   * 
   * @return string
   */
  public static function getProjectTitle($directory) {
    $info_file = self::findInfoFile($directory);
    $info = drupal_parse_info_file($info_file);
    if (!$info) {
      //@TODO: Add the variables, t(), etc
      throw new UpdaterError("Unable to parse info file");
    }
    return $info['name'];
  }

  /**
   * Stores default parameters for the Updater.
   *
   * @param array $overrides
   *   An array of overrides for the default parameters.
   *
   * @return array
   *   An array of configuration parameters for an update or install operation.
   */
  private function getInstallArgs($overrides = array()) {
    $args = array(
      'make_backup' => FALSE,
      'install_dir' => $this->getInstallDirectory(),
      'backup_dir'  => $this->getBackupDir(),
    );
    return array_merge($args, $overrides);
  }

  /**
   * Updates a Drupal project, returns a list of next actions.
   *
   * @param FileTransfer $ft
   *   Object which is a child of FileTransfer.  Used for moving files
   *   to the server.
   * @param array $overrides
   *   An array of settings to override defaults
   *   @see self::getInstallArgs
   *
   * @return array
   *   An array of links which the user may need to complete the update
   */
  public function update(&$ft, $overrides = array()) {
    try {
      // Establish arguments with possible overrides.
      $args = $this->getInstallArgs($overrides);

      // Take a Backup.
      if ($args['make_backup']) {
        $this->makeBackup($args['install_dir'], $args['backup_dir']);
      }
      
      if (!$this->name) {
        //This is bad, don't want delte the install dir.
        throw new UpdaterError("Fatal error in update, cowardly refusing to wipe out the install directory");
      }

      /**
       * Note: If the project is installed in sites/all, it will not be deleted.
       * It will be installed in sites/default as that will override the sites/all
       * reference and not break other sites which may be using it.
       */
      if (is_dir($args['install_dir'] . '/' . $this->name)) {
        // Remove the existing installed file.
        $ft->removeDirectory($args['install_dir'] . '/' . $this->name);
      }

      // Copy the directory in place.
      $ft->copyDirectory($this->source, $args['install_dir']);
      // Run the updates.
      // @TODO: decide if we want to implement this.
      $this->postUpdate();
      // For now, just return a list of links of things to do.
      return $this->postUpdateTasks();
    }
    catch (FileTransferException $e) {
      throw new UpdaterError(t("File Transfer failed, reason: !reason", array('!reason' => t($e->getMessage(), $e->arguments))));
    }
  }

  /**
   * Installs a Drupal project, returns a list of next actions.
   *
   * @param FileTransfer $ft
   *   Object which is a child of FileTransfer.
   * @param array $overrides
   *   An array of settings to override defaults.
   *   @see self::getInstallArgs
   * 
   * @return array
   *   An array of links which the user may need to complete the install.
   */
  public function install(&$ft, $overrides = array()) {
    try {
      // Establish arguments with possible overrides.
      $args = $this->getInstallArgs($overrides);
      // Copy the directory in place.
      $ft->copyDirectory($this->source, $args['install_dir']);
      // Potentially enable something?
      // @TODO: decide if we want to implement this.
      $this->postInstall();
      // For now, just return a list of links of things to do.
      return $this->postInstallTasks();
    }
    catch (FileTransferException $e) {
      throw new UpdateError(t("File Transfer failed, reason: !reason", array('!reason' => t($e->getMessage(), $this->arguments))));
    }
  }

  function makeBackup(&$ft, $from, $to) {
    //@TODO: Not implemented
  }

  function getBackupDir() {
    return file_directory_path('temporary');
  }

  /**
   * Needs to be overridden by children to work
   * Actions to take after the update is complete.
   */
  function postUpdate() {

  }

  /**
   * Needs to be overridden by children to work
   * Actions to take after the install is complete.
   */
  function postInstall() {

  }

  /**
   * Returns an array of links to pages that should be visited post operation.
   *
   * @return array Links which provide actions to take after the install is finished.
   */
  function postInstallTasks() {
    return array();
  }

  /**
   * Returns an array of links to pages that should be visited post operation.
   *
   * @return array Links which provide actions to take after the update is finished.
   */
  function postUpdateTasks() {
    return array();
  }
}

class ModuleUpdater extends Updater implements DrupalProjectUpdater{

  static function getInstallDirectory() {
    return DRUPAL_ROOT . '/' . conf_path() . '/modules';
  }

  function isInstalled() {
    return (bool) drupal_get_path('module', $this->name);
  }

  static function canUpdateDirectory($directory) {
    if (file_scan_directory($directory, '/.*\.module/')) {
      return TRUE;
    }
    return FALSE;
  }

  static function canUpdate($project_name) {
    return (bool) drupal_get_path('module', $project_name);
  }

  /**
   * @todo: Not implemented
   * @return <type>
   */
  function getSchemaUpdates() {
    require_once './includes/install.inc';
    require_once './includes/update.inc';

    if (_update_get_project_type($project) != 'module') {
      return array();
    }
    module_load_include('install', $project);

    if (!$updates = drupal_get_schema_versions($project)) {
      return array();
    }
    $updates_to_run = array();
    $modules_with_updates = update_get_update_list();
    if ($updates = $modules_with_updates[$project]) {
      if ($updates['start']) {
        return $updates['pending'];
      }
    }
    return array();
  }

  function postInstallTasks() {

    return array(
      l(t('Enable newly added modules in !project', array('!project' => $this->title)), 'admin/config/modules'),
    );
  }

  function postUpdateTasks() {
    // @todo: If there are schema updates.
    return array(
      l(t('Run database updates for !project', array('!project' => $this->title)), 'update.php'),
    );
  }

}

class ThemeUpdater extends Updater implements DrupalProjectUpdater {
  public $installDirectory;

  static function getInstallDirectory() {
    return DRUPAL_ROOT . '/' . conf_path() . '/themes';
  }

  function isInstalled() {
    return (bool) drupal_get_path('theme', $this->name);
  }

  static function canUpdateDirectory($directory) {
    // This is a lousy test, but don't know how else to confirm it is a theme.
    if (file_scan_directory($directory, '/.*\.module/')) {
      return FALSE;
    }
    return TRUE;
  }

  static function canUpdate($project_name) {
    return (bool) drupal_get_path('theme', $project_name);
  }

  function postInstall() {
    // Update the system table.
    system_get_theme_data();

    // Active the theme
    db_update('system')
      ->fields(array('status' => 1))
      ->condition('type', 'theme')
      ->condition('name', $this->name)
      ->execute();
  }
  
  function postInstallTasks() {
    return array(
      l(t('Set the !project theme as default', array('!project' => $this->title)), 'admin/appearance'),
    );
  }
}

/**
 * @TODO: implement this.
 */
class ProfileUpdater extends Updater {
  function installProject() {

  }
}

class UpdaterError extends Exception {
  
}