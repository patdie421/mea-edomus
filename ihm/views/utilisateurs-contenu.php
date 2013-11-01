<?php
//
//  SOUS-PAGE (SUB-VIEW) de utilisateurs.php : grille des utilisateurs de l'application
//
session_start()
?>

<div class="ui-corner-all" style="border: solid 1px #a6c9e2; text-align:center; margin:auto; padding: 20px">
        <table id="table_utilisateurs"></table>
        <div id="pager_utilisateurs"></div>
</div>

<script type="text/javascript" src="lib/js/mea-auth-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-gui-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-grid-utils.js"></script>

<script type="text/javascript" src="controlers/grid_utilisateurs.js"></script>

<script type="text/javascript">
jQuery(document).ready(function(){
    function resizeGrid(){
        var grid='utilisateurs';
        
        gridParentWidth = $('#gbox_' + 'table_'+grid).parent().width();
        $('#table_'+grid).jqGrid('setGridWidth',gridParentWidth, false);
    }

    $(window).on('resize',resizeGrid).trigger('resize');

    grid_utilisateurs();
});
</script>