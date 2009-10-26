// $Id: smackdown.js,v 1.1.2.2.2.11 2009/10/23 16:39:34 sirkitree Exp $

/**
 * Smackdown voting through AJAX
 *
 * Provide a means to vote on our nodereference fields and redirect to a new
 * smackdown node. Javascript techniques accredited to starbow's popups.js.
 */
(function ($) {
  /**
   * Create the smackdown object/namespace.
   */
  Drupal.smackdown = function() {};

  /**
   * Attach the smackdown behavior to nodereferences on the page.
   *
   * @param context
   *   The jQuery object to apply the behaviors to.
   */
  Drupal.behaviors.smackdown = function(context) {
    if (typeof Drupal.settings.smackdown !== 'undefined') {
      var field_1 = '.field-' + Drupal.settings.smackdown.field1.name.replace(/_/g, '-');
      var field_2 = '.field-' + Drupal.settings.smackdown.field2.name.replace(/_/g, '-');
      if (Drupal.settings.smackdownPerm == 1) {
        Drupal.smackdown.attachVote(context, '.' + field_1 + ' .field-item a');
        $(field_1 + ' .field-item a').attr('rel', Drupal.settings.smackdown.field1.nid).addClass('smackdown-processed');
        Drupal.smackdown.attachVote(context, '.' + field_2 + ' .field-item a');
        $(field_2 + ' .field-item a').attr('rel', Drupal.settings.smackdown.field2.nid).addClass('smackdown-processed');
      }
      else {
        Drupal.smackdown.attachNotice(context, '.' + field_1 + ' .field-item a');
        Drupal.smackdown.attachNotice(context, '.' + field_2 + ' .field-item a');
      }
    }
  };

  /**
   * Attach the smackdown behavior to a particular link.
   *
   * @param context
   * @param selector
   *   jQuery selector for links to attach behavior to.
   */
  Drupal.smackdown.attachVote = function(context, selector) {
    $(selector, context).each(function() {
      var $element = $(this);
      // Attach the on-click popup behavior to the element.
      $element.click(function(e){
        Drupal.theme('voting', $element);
        var nid = $element.attr('rel'); // not compatible with clean urls
        var sid = Drupal.settings.smackdown.sid;
        var params = {'cid':nid, 'sid':sid};
        // post nid and context to smackdown/vote
        ajaxOptions = {
          url: Drupal.settings.basePath + 'smackdown/vote/' + Drupal.settings.smackdown.token,
          dataType: 'json',
          data: params,
          success: function(json) {
            // we put the location into a variable so that it can be changed by other modules
            location.href = json.location
          }
        };
        $.ajax(ajaxOptions);
        return false;
      });
    });
  };

  Drupal.smackdown.attachNotice = function(context, selector) {
    $(selector, context).each(function() {
      var $element = $(this);
      $element.click(function(e) {
        Drupal.theme('notice', $element);
        return false;
      });
    });
  };

  Drupal.theme.prototype.voting = function(element) {
    var output = "<div id='voting-indicator'>" + Drupal.t('Voting...') + "</div>";
    element.parent().css({'background-color':'#ffc'});
    return element.parent().append(output);
  };

  Drupal.theme.prototype.notice = function(element) {
    var output = "<div id='voting-indicator'>" + Drupal.t('You do not have sufficient rights to vote.') + "</div>";
    element.parent().css({'background-color':'#ffc'});
    return element.parent().append(output);
  };
})(jQuery);
