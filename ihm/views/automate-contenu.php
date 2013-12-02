<?php
//
//  SOUS-PAGE (SUB-VIEW) de automate.php :
//
?>
<div style="text-align:center; margin:auto;">
<div id="tabs1">
    <ul>
        <li><a href="#tabs1-1" id='inputs'>inputs</a></li>
        <li><a href="#tabs1-2" id='outputs'>outputs</a></li>
        <li><a href="#tabs1-3" id='program'>program</a></li>
    </ul>
    <div id="tabs1-1">
        <table id="table_inputs"></table>
        <div id="pager_inputs"></div>

    </div>
    <div id="tabs1-2">
    </div>
    <div id="tabs1-3">
    </div>
</div>
<div id="aff">
</div>
</div>

<script type="text/javascript" src="lib/js/mea-auth-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-gui-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-grid-utils.js"></script>

<script type="text/javascript" src="controlers/grid_inputs.js"></script>

<script>

jQuery(document).ready(function(){
   $( "#tabs1" ).tabs();
   grid_inputs();
});
</script>