<style>
.display {
 display:none;
}

.pastille {
 height:20px;
 width:20px;
 margin-right:10px;
 margin-left:10px;
 border-radius: 20px;
}

</style>

<script type="text/javascript" src="lib/js/mea-auth-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-gui-utils.js"></script>

<script type="text/javascript" src="controllers/page1-ctrl.js"></script>

<script>
jQuery(document).ready(function(){
   tabName=$('#tabtodisplay').val();

   page1_controller();

   $('#tt').tabs({
      onSelect:function(title){
//         if(title=="Journal")
//         {
//            if(typeof(logViewer)!="undefined")
//               logViewer.scrollBottom();
//         }
      }
   });
   $('#tt').tabs('select', tabName);

});
</script>

<div id="tt" class="easyui-tabs" border=false fit=true>
    <div title="Indicateurs" href="views/indicateurs-view.html" style="padding:20px;">
    </div>
    <div title="Services" href="views/services-view.html" style="padding:20px;">
    </div>
    <div title="Journal" href="views/journal-view.html" style="padding:20px;">
    </div>
</div>
