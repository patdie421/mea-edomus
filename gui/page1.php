<style>
.display {
 display:none;
}
</style>

<script type="text/javascript" src="lib/js/mea-livecom-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-auth-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-gui-utils.js"></script>

<script type="text/javascript" src="controllers/page1-ctrl.js"></script>

<script>
jQuery(document).ready(function(){
  //   var socketio_port=8000;
  //   liveCom.connect(socketio_port);
   page1_controller(liveCom.getSocketio());
});

</script>

<div id="tt" class="easyui-tabs" fit=true>
    <div title="Indicateurs" href="views/indicateurs-view.html" style="padding:20px;">
      tab1
    </div>
    <div title="Services" href="views/services-view.html" style="padding:20px;">
      tab2
    </div>
    <div title="Journal" href="views/journal-view.html" style="padding:20px;">
      tab3
    </div>
</div>
