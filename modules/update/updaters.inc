<?php

interface DrupalUpdater {
  public function installProject();

  public function isAlreadyInstalled();

  /**
   * @return string an absolute path to the default install location
   */
  public static function getInstallDirectory();
}

class Updater {

  static function factory($source) {
    if (is_dir($source)) {
      $type = getProjectTypeFromDirectory($source);
    } else {
      throw new Exception('Unable to determine the type of the source directory');
    }
    $classname = ucfirst($type) . 'Updater';
    if (class_exists($classname)) {
      return new $classname($source);
    }
  }

  static function getProjectTypeFromDirectory($directory) {

  }

  static function getProjectTypeFromArchive($archive) {

  }
  
  public function downloadProject() {
    
  }

  public function copyProject() {

  }

}

class ModuleUpdater extends Updater implements DrupalUpdater{

  public $installDirectory;

  static function getInstallDirectory() {
    return DRUPAL_ROOT . '/' . conf_path() . '/modules';
  }

  function isAlreadyInstalled() {

  }
  
  function installProject() {

  }
}

class ThemeUpdater extends Updater implements DrupalUpdater{
  public $installDirectory;

  static function getInstallDirectory() {
    return DRUPAL_ROOT . '/' . conf_path() . '/themes';
  }

  function isAlreadyInstalled() {

  }

  function installProject() {

  }
}

class ProfileUpdater extends Updater {
  function installProject() {

  }
}
?>
