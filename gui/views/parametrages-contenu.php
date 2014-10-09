<?php
//
//  SOUS-PAGE (SUB-VIEW) de parametrages.php : 
//
?>
<div style="text-align:center; margin:auto;">
<div id="tabs1">
    <ul>
        <li><a href="#tabs1-1" id='sensors_actuators'>Sensors/Actuators</a></li>
        <li><a href="#tabs1-2" id='interfaces'>Interfaces</a></li>
        <li><a href="#tabs1-3" id='locations'>Locations</a></li>
        <li><a href="#tabs1-4" id='types'>Types</a></li>
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

<script type="text/javascript" src="lib/js/mea-auth-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-gui-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-grid-utils.js"></script>

<script type="text/javascript" src="controllers/grid_sensors_actuators.js"></script>
<script type="text/javascript" src="controllers/grid_interfaces.js"></script>
<script type="text/javascript" src="controllers/grid_locations.js"></script>
<script type="text/javascript" src="controllers/grid_types.js"></script>

<script>
var interfaces_inEdit, sensors_actuators_inEdit, locations_inEdit, types_inEdit,
    isActivate=[false,false,false,false];

jQuery(document).ready(function(){
    // redimentionnement des grilles
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

    // déclenchement des contrôleurs sur selection d'un nouvel onglet
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

    // traduction
    $("#sensors_actuators").text(str_sensors_actuators.capitalize());
    $("#interfaces").text(str_interfaces.capitalize());
    $("#locations").text(str_locations.capitalize());
    $("#types").text(str_types.capitalize());
    $("#tabs1").tabs({activate:activeGrids});

    // affichage grille
    activeGrids();
    
    // évenements
    $(window).on('resize',resizeGrid).trigger('resize');
});
</script>