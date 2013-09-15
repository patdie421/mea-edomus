<div style="text-align:center; margin:auto;">
        <table id="table_utilisateurs"></table>
        <div id="pager_utilisateurs"></div>
</div>

<div id="dialog-confirm" title="Default" style="display:none;">
    <p><span class="ui-icon ui-icon-alert" style="float: left; margin: 0 7px 20px 0;"></span><div id="dialog-confirm-text">Default<div></p>
</div>

<script type="text/javascript" src="../lib/js/mea-grid-utils.js"></script>

<script type="text/javascript" src="../controlers/grid_utilisateurs.js"></script>

<script>
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
