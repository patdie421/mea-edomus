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


<script type="text/javascript" src="controllers/services-ctrl.js"></script>

<script>

jQuery(document).ready(function(){
  controlPanel = new ControlPanel();
  controlPanel.linkToTranslationController(translationController);
  controlPanel.linkToCredentialController(credentialController); // pour la gestion des habilitations

  s=liveComController.getSocketio();
  if(s!=null) {
     var _mon_listener=controlPanel.__mon_listener;
     var mon_listener=_mon_listener.bind(controlPanel);
     s.on('mon', mon_listener);

     var _rel_listener=controlPanel.__rel_listener;
     var rel_listener=_rel_listener.bind(controlPanel);
     s.on('rel', rel_listener);
  }
  else {
     window.location="index.php";
  }
  
  controlPanel.init("table_reload", "table_interfaces", "table_processes");
  controlPanel.load();
});
</script>


<style>
.process {
// border: dotted 1px red;
   max-height: 40px;
   margin: auto;
}
 
.name {
// border: dotted 1px blue;
   max-height:20px;
   line-height:100%;
   overflow: hidden;
}
 
.description {
// border: dotted 1px gray;
   line-height:100%;
   max-height:17px;
   font-size:10px;
   overflow: hidden;
}
 
.bouton {
// border: dotted 1px yellow;
// height:40px;
// line-height:40px;
   text-align:center;
}

.wait_ball {
   background-color: rgba(0,0,0,0);
   border: 3px solid rgba(75, 165, 202,0.9);
   opacity: .9;
   border-top: 5px solid rgba(0,0,0,0);
   border-left: 5px solid rgba(0,0,0,0);
   border-radius: 12px;
   box-shadow: 0 0 3px #2187e7;
   width: 12px;
   height: 12px;
   margin: 0 auto;
   -moz-animation: spin .75s infinite linear;
   -webkit-animation: spin .75s infinite linear;
}
 
@-moz-keyframes spin {
 0% {
  -moz-transform: rotate(0deg);
 }
 100% {
  -moz-transform: rotate(360deg);
 };
}
 
@-moz-keyframes spinoff {
 0% {
  -moz-transform: rotate(0deg);
 }
 100% {
  -moz-transform: rotate(-360deg);
 };
}
 
@-webkit-keyframes spin {
 0% {
  -webkit-transform: rotate(0deg);
 }
 100% {
  -webkit-transform: rotate(360deg);
 };
}
 
@-webkit-keyframes spinoff {
 0% {
  -webkit-transform: rotate(0deg);
 }
 100% {
  -webkit-transform: rotate(-360deg);
 };
}

@-moz-keyframes spin {
 0% {
  -moz-transform: rotate(0deg);
 }
 100% {
  -moz-transform: rotate(360deg);
 };
}

@-moz-keyframes spinoff {
 0% {
  -moz-transform: rotate(0deg);
 }
 100% {
  -moz-transform: rotate(-360deg);
 };
}

@-webkit-keyframes spin {
 0% {
  -webkit-transform: rotate(0deg);
 }
 100% {
  -webkit-transform: rotate(360deg);
 };
}

@-webkit-keyframes spinoff {
 0% {
  -webkit-transform: rotate(0deg);
 }
 100% {
  -webkit-transform: rotate(-360deg);
 };
}
</style>

<div style="width:100%;text-align:center;">
<?php
if($isadmin==0) : ?>
   <div id="p1" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC('reload interface(s) configuration(s)')?>" style="width:700px;padding:10px;">
      <table id="table_reload" style="width:100%;">
      </table>
   </div>

   <p></p>
<?php
endif; ?>
   <div id="p2" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC('interface(s) status') ?>" style="width:700px;padding:10px;">
      <table id="table_interfaces" style="width:100%;">
      </table>
   </div>
   
   <p></p>

   <div id="p3" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC('mea-edomus processes status') ?>" style="width:700px;padding:10px;">
      <table id="table_processes" style="width:100%;">
      </table>
   </div>
</div>
