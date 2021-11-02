Drupal.behaviors.brickFilterBehavior = function (context)  {
  $('.brick-filter-part').each(function () {
    $(this).removeClass('brick-filter-part').addClass('brick-filter-part-processed');
    var part = $(this).attr("part");
    var color = $(this).attr("color");
    var element = this;
    $.get('/brick_filter/part/'+part+'/color/'+color, function(data) {
      element.innerHTML = data;
      Drupal.attachBehaviors($('.brick-filter-part-processed'));
    });
  });
  $('.brick-filter-element').each(function () {
    $(this).removeClass('brick-filter-element').addClass('brick-filter-element-processed');
    var elmid = $(this).attr("element");
    var element = this;
    $.get('/brick_filter/element/'+elmid, function(data) {
      element.innerHTML = data;
      Drupal.attachBehaviors($('.brick-filter-element-processed'));
    });
  });
  $('.brick-filter-set').each(function () {
    $(this).removeClass('brick-filter-set').addClass('brick-filter-set-processed');
    var set = $(this).attr("set");
    var element = this;
    $.get('/brick_filter/set/'+set, function(data) {
      element.innerHTML = data;
      Drupal.attachBehaviors($('.brick-filter-set-processed'));
    });
  });
};

