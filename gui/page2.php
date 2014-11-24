<script type="text/javascript" src="lib/js/mea-auth-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-gui-utils.js"></script>

<script type="text/javascript" src="controllers/page2-ctrl.js"></script>

<script>
jQuery(document).ready(function(){
   tabName=$('#tabtodisplay').val();

   if(!page2_controller(tabName))
   {
      return -1;
   }
});
</script>

<div id="tabConfiguration" class="easyui-tabs" border=false fit=true>
    <div title="Capteurs/Actionneurs" href="views/capteurs-actionneurs-view.html">
    </div>
    <div title="Interfaces" href="views/interfaces-view.html" style="padding:20px;">
    </div>
    <div title="Types" href="views/types-view.html" style="padding:20px;">
    </div>
    <div title="Localisations" href="views/localisations-view.html" style="padding:20px;">
    </div>
</div>
