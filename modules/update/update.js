// $Id$
(function ($) {

Drupal.behaviors.updateFileTransferForm = {
  attach: function(context) {
    $('#edit-connection-settings-update-filetransfer-default').change(function() {
      $('.filetransfer').hide().filter('.filetransfer-' + $(this).val()).show();
    });
    $('.filetransfer').hide().filter('.filetransfer-' + $('#edit-connection-settings-update-filetransfer-default').val()).show();

    // Hide any major version warnings, make a jquery UI dialog when checked.

    $('.update-major-version-warning').each(function (elem) {
      that = $(this);
      $(this).hide();
      $(":checkbox", $(this).parent().parent()).bind('change', function(e) {
        if (this.checked) {
          that.dialog("open");
          var buttons = {}
          buttons[Drupal.t("Ok")] = function() { $(this).dialog("close"); };
          buttons[Drupal.t("Cancel")] = function() { alert('cancel'); $(this).dialog("close"); };
          that.dialog({'buttons': buttons});
        }
      });
    });
    
    // Removes the float on the select box (used for non-JS interface)
    if($('.connection-settings-update-filetransfer-default-wrapper').length > 0) {
      console.log($('.connection-settings-update-filetransfer-default-wrapper'));
      $('.connection-settings-update-filetransfer-default-wrapper').css('float', 'none');
    }
    // Hides the submit button for non-js users
    $('#edit-submit-connection').hide();
    $('#edit-submit-process').show();
  }
}

})(jQuery);
