<div style="text-align:center; margin:auto;">
<div id="tabs1">
    <ul>
        <li><a href="#tabs1-1">Capteurs/Actionneurs</a></li>
        <li><a href="#tabs1-2">Interfaces</a></li>
        <li><a href="#tabs1-3">Lieux</a></li>
        <li><a href="#tabs1-4">Types</a></li>
    </ul> 
    <div id="tabs1-1">
        <table id="table_sensors_actuators"></table>
        <div id="pager_sensors_actuators"></div>
    </div>
    <div id="tabs1-2">
        <table id="table_interfaces"></table>
        <div id="pager_interfaces"></div>
    </div>
    <div id="tabs1-3">
        <table id="table_locations"></table>
        <div id="pager_locations"></div>
    </div>
    <div id="tabs1-4">
        <table id="table_types"></table>
        <div id="pager_types"></div>
    </div>
</div>
<div id="aff">
</div>
</div>
<div id="dialog-confirm" title="Default" style="display:none;">
    <p><span class="ui-icon ui-icon-alert" style="float: left; margin: 0 7px 20px 0;"></span><div id="dialog-confirm-text">Default<div></p>
</div>

<script>
jQuery(document).ready(function(){
    $("div.ui-tabs-panel").css('padding-left','1px');
    $("div.ui-tabs-panel").css('padding-right','2px');
    $("div.ui-tabs-panel").css('padding-top','5px');
    $("div.ui-tabs-panel").css('padding-bottom','1px');
});
</script>


<script type="text/javascript" src="../lib/js/mea-grid-utils.js"></script>

<script type="text/javascript" src="../controlers/grid_sensors_actuators.js"></script>
<script type="text/javascript" src="../controlers/grid_interfaces.js"></script>
<script type="text/javascript" src="../controlers/grid_locations.js"></script>
<script type="text/javascript" src="../controlers/grid_types.js"></script>

<script>
var interfaces_inEdit, sensors_actuators_inEdit, locations_inEdit, types_inEdit,
    isActivate=[false,false,false,false];

jQuery(document).ready(function(){
    function resizeGrid(){
        active = $("#tabs1").tabs("option", "active");
        grid=null;
        switch(active)
        {
            case 0: grid='sensors_actuators';
                    break;
            case 1: grid='interfaces';
                    break;
            case 2: grid='locations';
                    break;
            case 3: grid='types';
                    break;

        }

        if(grid){
            var numTab=active+1;
            gridParentWidth = $('#gbox_' + 'table_'+grid).parent().width();
            $('#table_'+grid).jqGrid('setGridWidth',gridParentWidth, false);
        }
    }

    
    function activeGrids(event,ui) {
        active = $("#tabs1").tabs("option", "active");
        switch(active)
        {
            case 0: if(!isActivate[active]){
                        grid_sensors_actuators();
                    }
                    else
                        $('#table_sensors_actuators').trigger( 'reloadGrid' );
                    break;
            case 1: if(!isActivate[active]){
                        grid_interfaces();
                    }
                    else
                        $('#table_interfaces').trigger( 'reloadGrid' );
                    break;
            case 2: if(!isActivate[active]){
                        grid_locations();
                    }
                    break;
            case 3: if(!isActivate[active]){
                        grid_types();
                    }
                    else
                        $('#table_types').trigger( 'reloadGrid' );
                    break;
        }
        isActivate[active]=true;
        resizeGrid();
    }    
    
    $("#tabs1").tabs({activate:activeGrids});
    activeGrids();
    
    $(window).on('resize',resizeGrid).trigger('resize');
});
</script>