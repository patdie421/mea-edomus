<?php
//
//  SOUS-PAGE (SUB-VIEW) de index.php : statut de fonctionnement de mea-edomus 
//
include_once('../lib/configs.php');
session_start() ?>

<style>
.console
{
 font-family:"Lucida Console", Monaco, monospace;
 font-size:12px;
 border:1px dotted black;
 overflow-y:scroll;
}
</style>

<div class="ui-corner-all ui-widget-content" style="padding-top: 40px; padding-bottom: 40px;">
  <div id="console" class="console" style="margin:auto; width:800px; min-height:400px; height:400px;">
  </div>
</div>

<script type="text/javascript" src="lib/js/mea-auth-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-gui-utils.js"></script>

<script type="text/javascript" src="controlers/index.js"></script>
jQuery(document).ready(function(){
   start_controler();
});
