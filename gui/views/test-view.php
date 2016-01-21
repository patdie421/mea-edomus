<?php
include_once('../lib/configs.php');
include_once('../lib/php/translation.php');
include_once('../lib/php/$LANG/translation.php');
include_once('../lib/php/auth_utils.php');
mea_loadTranslationData($LANG,'../');

session_start();

$isadmin = check_admin();
if($isadmin !=0 && $isadmin != 98) : ?>
<script>
   window.location = "login.php";
</script>
<?php
   exit(1);
endif;
?>

<script>
current_zindex=0;


function max_zIndex(div)
{
   var zi = 0;
   var max = 0;

   $('#'+div+' >').each(function(){
      zi=$(this).css('zIndex');
      if(zi > max)
         max = zi;
   });

   return max;
}


function onBeforeDrag(e)
{
   zindex = $(this).css("z-index");
   if(zindex != current_zindex)
      $(this).css("z-index", ++current_zindex);
}


function info(e)
{
   var id = $(this).attr('id')
   var t  = $(this).attr('mea_widget')
   var x  = $(this).css("top");
   var y  = $(this).css("left");
   alert(id+": x="+x+" y="+y+" ("+t+")"); 
}


function onStopDrag(e)
{
}


function onDrag(e) {
   var d = e.data;
   d.left = repair(d.left);
   d.top = repair(d.top);

   function repair(v){
      var r = parseInt(v/20)*20;
      if (Math.abs(v % 20) > 10) {
         r += v > 0 ? 20 : -20;
      }
      return r;
   }
}


function properties()
{
   var id = $("#mm").attr('mea_eid');
   var t  = $("#"+id).attr('mea_widget')
   var x  = $("#"+id).css("top");
   var y  = $("#"+id).css("left");

   alert(id+": x="+x+" y="+y+" ("+t+")");

   return false;
}


function open_context_menu(e)
{
   e.preventDefault();

   $('#context_mm').menu('show', {left: e.pageX, top: e.pageY });

   return false;
}


function open_widget_menu(e)
{
   e.preventDefault();

   $('#mm').attr('mea_eid', $(this).attr('id'));
   $('#mm').menu('show', {left: e.pageX, top: e.pageY });

   return false;
}


function context_menu(action)
{
   switch(action)
   {
      case 'disable':
         $('div[id^="draggable"]').each(function(){
/*
            var id = $(this).attr('id')
            var t  = $(this).attr('mea_widget')
            var x  = parseInt($(this).css("top"));
            var y  = parseInt($(this).css("left"));
            console.log(id+": x="+x+" y="+y+" ("+t+")");
*/
            $(this).draggable('disable'); 
         });
         break;
      case 'enable':
         $('div[id^="draggable"]').each(function(){
            $(this).draggable('enable'); 
         });
   }
}


jQuery(document).ready(function() {
   current_zindex = max_zIndex("testzone");
   options = { onDrag: onDrag, onBeforeDrag: onBeforeDrag, onStopDrag: onStopDrag };

   $('#mm').menu({onHide: function() { $('#mm').attr('mea_eid', false);} });

   $("#draggable1").css({top: 100, left: 100}).draggable(options);
   $("#draggable2").css({top: 250, left: 300}).draggable(options);

   $("#draggable1").bind('contextmenu', open_widget_menu);
   $("#draggable2").bind('contextmenu', open_widget_menu);

   $("#testzone").bind('contextmenu',   open_context_menu);
/*
   $("#draggable1").bind('contextmenu',function(e) {
      e.preventDefault();
      $('#mm').attr('mea_eid', $(this).attr('id'));
      $('#mm').menu('show', {left: e.pageX, top: e.pageY });
   });
   $("#draggable2").bind('contextmenu',function(e) {
      e.preventDefault();
      $('#mm').attr('mea_eid', $(this).attr('id'));
      $('#mm').menu('show', {left: e.pageX, top: e.pageY });
   });
*/
});
</script>

<div id="testzone" style="width:1920px;height:1080px;position:relative;overflow:auto">
<div id="draggable1" mea_widget="jauge" style="position:absolute; width: 150px; height: 150px; border:1px solid black; background-color: black; z-index:1" class="ui-widget-content">
   <div>
      <a id="btn" href="#" class="easyui-linkbutton">test</a>
   </div>
</div>
<div id="draggable2" mea_widget="toggle" style="position:absolute; width: 150px; height: 150px; border:1px solid green; background-color: green; z-index:2" class="ui-widget-content"><div></div></div>
</div>

<div id="context_mm" class="easyui-menu" style="width:120px;">
   <div onclick="javascript:context_menu('enable')">enable</div>
   <div onclick="javascript:context_menu('disable')">disable</div>
</div>

<div id="mm" class="easyui-menu" style="width:120px;">
   <div onclick="javascript:properties()">properties</div>
</div>

