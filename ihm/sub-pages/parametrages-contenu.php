<div style="text-align:center; margin:auto;">
<div id="tabs1">
    <ul>
        <li><a href="#tabs1-1">Capteurs/Actionneurs</a></li>
        <li><a href="#tabs1-2">Interfaces</a></li>
        <li><a href="#tabs1-3">Lieux</a></li>
        <li><a href="#tabs1-4">Automatismes</a></li>
        <li><a href="#tabs1-5">Types</a></li>
    </ul> 
    <div id="tabs1-1">
        <table id="table_sensors_actuators"></table>
        <div id="pager_sensors_actuators"></div>
        <p>
        <button id="sensors_actuators_add">Ajouter</button>
        <button id="sensors_actuators_edit">Editer</button>
        <button id="sensors_actuators_del">Supprimer</button>
        </p>
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
        <table id="table_locations"></table>
        <div id="pager_locations"></div>
    </div>
    <div id="tabs1-5">
        <table id="table_types_standards"></table>
        <div id="pager_types_standards"></div>
    <p>
    </p>
        <table id="table_types_personnalises"></table>
        <div id="pager_types_personnalises"></div>
    </div>
</div>
<div id="aff">
</div>
</div>
<div id="dialog-confirm" title="Default" style="display:none;">
    <p><span class="ui-icon ui-icon-alert" style="float: left; margin: 0 7px 20px 0;"></span><div id="dialog-confirm-text">Default<div></p>
</div>

<style1>
.ui-jqgrid .ui-jqgrid-view {font-size:12px;}
.ui-jqgrid tr.jqgrow td {height: 30px;}
</style1>


<script>
jQuery(document).ready(function(){
    var interfaces_inEdit, sensors_actuators_inEdit, locations_inEdit, types_standards_inEdit;

    function mea_yesno(title, text, yesFunc, yesFuncParams){
        $("#dialog-confirm-text").html(text);
        $( "#dialog-confirm" ).dialog({
            title:title,
            resizable: true,
            height:250,
            width:500,
            modal: true,
            buttons: {
                Oui: function() {
                    $( this ).dialog( "close" );
                    yesFunc(yesFuncParams);
                },
                Non: function() {
                    $( this ).dialog( "close" );
                }
            }
        });
    }

    
    function mea_alert(title, text){
        $("#dialog-confirm-text").html(text);
        $( "#dialog-confirm" ).dialog({
            title: title,
            resizable: true,
            height:250,
            width:500,
            modal: true,
            buttons: {
                Ok: function() {
                    $( this ).dialog( "close" );
                }
            }
        });
    }
    

    function ajax_error(xhr, ajaxOptions, thrownError){
                    alert(xhr.responseText);
                    alert(xhr.status);
                    alert(thrownError);
    }            
    
    function check_field_exist_by_id(grid,table,fieldName,fieldValue){
        var passes;
        var currentId=$('[name="'+grid+'_id"]').val();
        if(currentId!="_empty")
            wsdata={table:table, field:fieldName, value:fieldValue, id:currentId};
        else
            wsdata={table:table, field:fieldName, value:fieldValue};
        
        $.ajax({ url: 'lib/php/check_field-jqg_grid.php',
            async: false,
            type: 'GET',
            dataType: 'json',
            data: wsdata,
            success: function(data){
              if(data.exist==0)
                passes=true;
              else
                passes=false;
            },
            error:ajax_error
        });

        return passes;
    }

    function _DataInit(element,flag,ws){
        if(flag)
            $(element).attr("readonly", "readonly"); // à améliorer
        else{
            $.get(ws,
                {},
                function(data){
                    $(element).val(data.next_id);
                },
                "json"
            );
        }
    }
    
    function db_delete(grid,table,id){
        $.ajax({ url: 'lib/php/delete_row_by_id-jqg_grid.php',
            async: false,
            type: 'GET',
            dataType: 'json',
            data: {table:table,id:id},
            success: function(data){
                    $("#"+grid).trigger("reloadGrid", [{current: true}]);
            },
            error: ajax_error
        });
    }
    
        function check_name_string(value,_name){
        var regx = /^[A-Za-z0-9]+$/;
        
        if(!regx.test(value))
            return [false, _name+" - caractères autorisés : a-zA-Z0-9"];
                
        if(value.length>16)
            return [false, _name+" - 16 caractères maximum"];
            
        return [true];
    }
    

    function check_id_interface(value,_name){
        passes=check_field_exist_by_id('table_interfaces','interfaces','id_interface',value);
        return [passes, _name+" : valeur("+value+") déjà utilisée"];
    }

    function check_id_sensor_actuator(value,_name){
        passes=check_field_exist_by_id('table_sensors_actuators','sensors_actuators','id_sensor_actuator',value);
        return [passes, _name+" : id capteur ou actionneur ("+value+") déjà utilisé "];
    }
    
    function check_id_location(value,_name){
        passes=check_field_exist_by_id('table_locations','locations','id_location',value);
        return [passes, _name+" : numéro de lieu ("+value+") déjà utilisé "];
    }
    
    function check_id_types_standards(value,_name){
        passes=check_field_exist_by_id('table_types_standards','types','id_type',value);
        return [passes, _name+" : numéro de type ("+value+") déjà utilisé "];
    }

    function check_id_types_personnalises(value,_name){
        passes=check_field_exist_by_id('table_types_personnalises','types','id_type',value);
        return [passes, _name+" : numéro de type ("+value+") déjà utilisé "];
    }

    
    function check_name_interface(value,_name){
        ret=check_name_string(value,_name);
        if(ret[0]==false)
            return ret; 
        value=value.toUpperCase();
        var passes=check_field_exist_by_id('table_interfaces','interfaces','name',value);
        return [passes, _name+" : nom interface("+value+") déjà utilisé "];
    }

    function check_name_sensor_actuator(value,_name){
        ret=check_name_string(value,_name);
        if(ret[0]==false)
            return ret; 
        value=value.toUpperCase();
        var passes=check_field_exist_by_id('table_sensors_actuators','sensors_actuators','name',value);
        return [passes, _name+" : nom capteur ou actionneur ("+value+") déjà utilisé "];
    }

    function check_name_location(value,_name){
        ret=check_name_string(value,_name);
        if(ret[0]==false)
            return ret; 
        value=value.toUpperCase();
        var passes=check_field_exist_by_id('table_locations','locations','name',value);
        return [passes, _name+" : nom de lieu ("+value+") déjà utilisé "];
    }

    function check_name_types_standards(value,_name){
        ret=check_name_string(value,_name);
        if(ret[0]==false)
            return ret; 
        value=value.toUpperCase();
        var passes=check_field_exist_by_id('table_types_standards','types','name',value);
        return [passes, _name+" : nom de type ("+value+") déjà utilisé "];
    }

    function check_name_types_personnalises(value,_name){
        ret=check_name_string(value,_name);
        if(ret[0]==false)
            return ret; 
        value=value.toUpperCase();
        var passes=check_field_exist_by_id('table_types_personnalises','types','name',value);
        return [passes, _name+" : nom de type ("+value+") déjà utilisé "];
    }

    
    function id_interface_DataInit(element){
        _DataInit(element,interfaces_inEdit,'lib/php/get_next_id_interface-jqg_grid.php');
    }

    function id_sensor_actuator_DataInit(element){
        _DataInit(element,sensors_actuators_inEdit,'lib/php/get_next_id_sensor_actuator-jqg_grid.php');
    }

    function id_location_DataInit(element){
        _DataInit(element,locations_inEdit,'lib/php/get_next_id_location-jqg_grid.php');
    }

    function id_types_standards_DataInit(element){
        $(element).attr("readonly", "readonly"); // à améliorer
    }

    function id_types_personnalises_DataInit(element){
        _DataInit(element,types_standards_inEdit,'lib/php/get_next_id_type-jqg_grid.php');
    }

    
    function db_delete_sensor_actuator(id){
        db_delete('table_sensors_actuators','sensors_actuators',id);
    }

    function db_delete_interface(id){
        db_delete('table_interfaces','interfaces',id);
    }

    function db_delete_location(id){
        db_delete('table_locations','locations',id);
    }

    function db_delete_type(id){
        db_delete('table_types_personnalises','types',id);
    }


    function delete_sensor_actuator(){
        var grid_id='table_sensors_actuators',
            myGrid = $("#"+grid_id),
            selRowId = myGrid.jqGrid ('getGridParam', 'selrow');
         
        if(selRowId){
            var id = myGrid.jqGrid ('getCell', selRowId, 'id');
                
            mea_yesno("OK pour suppression", "Oui ou Non", db_delete_sensor_actuator, id);
        }
    }
    
    function delete_interface(){
        var grid_id='table_interfaces',
            myGrid = $("#"+grid_id),
            selRowId = myGrid.jqGrid ('getGridParam', 'selrow');
         
        if(selRowId){
            var id = myGrid.jqGrid ('getCell', selRowId, 'id');
                id_interface = myGrid.jqGrid ('getCell', selRowId, 'id_interface'),
                name = myGrid.jqGrid ('getCell', selRowId, 'name');
                
            $.ajax({ url: 'lib/php/get_field-jqg_grid.php',
                async: false,
                type: 'GET',
                dataType: 'json',
                data: {table:'sensors_actuators', field:'name', where:'id_interface='+id_interface},
                success: function(data){
                    if(data.values.length > 0){
                        msg="<DIV style='padding-left:30px'>L'interface "+
                             name+
                             " ne peut pas être supprimée car elle est référencée par les objets suivants : </BR></BR>Capteur(s)/actionneur(s) : "+
                             data.values+
                             "</BR></BR>Modifiez d'abord les références.</DIV>"
                    mea_alert("Suppression impossible", msg);
                    }else{
                        mea_yesno("OK pour suppression", "Oui ou Non", db_delete_interface, id);
                    }
                },
                error:ajax_error
            });
        }
    }

    function delete_location(){
        var grid_id='table_locations',
            myGrid = $("#"+grid_id),
            selRowId = myGrid.jqGrid ('getGridParam', 'selrow');
         
        if(selRowId){
            var id = myGrid.jqGrid ('getCell', selRowId, 'id');
                id_location = myGrid.jqGrid ('getCell', selRowId, 'id_location'),
                name = myGrid.jqGrid ('getCell', selRowId, 'name');
                
            $.ajax({ url: 'lib/php/get_field-jqg_grid.php',
                async: false,
                type: 'GET',
                dataType: 'json',
                data: {table:'sensors_actuators', field:'name', where:'id_location='+id_location},
                success: function(data){
                    if(data.values.length > 0){
                        msg="<DIV style='padding-left:30px'>Le lieu "+
                             name+
                             " ne peut pas être supprimée car il est référencé par les objets suivants : </BR></BR>Capteur(s)/actionneur(s) : "+
                             data.values+
                             "</BR></BR>Modifiez d'abord les références.</DIV>"
                        mea_alert("Suppression impossible", msg);
                    }else{
                        mea_yesno("OK pour suppression", "Oui ou Non", db_delete_location, id);
                    }
                },
                error:ajax_error
            });
        }
    }

    
    function delete_type(){
        var grid_id='table_types_personnalises',
            myGrid = $("#"+grid_id),
            selRowId = myGrid.jqGrid ('getGridParam', 'selrow');
         
        if(selRowId){
            var id = myGrid.jqGrid ('getCell', selRowId, 'id');
                id_type = myGrid.jqGrid ('getCell', selRowId, 'id_type'),
                name = myGrid.jqGrid ('getCell', selRowId, 'name');
                
            $.ajax({ url: 'lib/php/get_2fields-jqg_grid.php',
                async: true,
                type: 'GET',
                dataType: 'json',
                data: {table1:'sensors_actuators', field1:'name', where1:'id_type='+id_type,
                       table2:'interfaces', field2:'name', where2:'id_type='+id_type},
                success: function(data){
                    var msg="";
                    if(data.values1.length > 0 || data.values2.length > 0){
                        msg="<DIV style='padding-left:30px'>Le type "+name+" ne peut pas être supprimée car il est référencé par les objets suivants : </BR></BR>";
                        if(data.values1.length)
                            msg=msg+"Capteur(s)/actionneur(s) : "+data.values1+"</BR>";
                        if(data.values2.length)
                            msg=msg+"Interface(s) : "+data.values2+"</BR>";
                        msg=msg+"</BR>Modifiez d'abord les références.</DIV>";
                        mea_alert("Suppression impossible", msg);
                    }else{
                        mea_yesno("OK pour suppression", "Oui ou Non", db_delete_type, id);
                    }
                },
                error: ajax_error
            });
        }
    }

    isActivate=[false,false,false,false,false];
    
    function resizeGrid(){
        active = $("#tabs1").tabs("option", "active");
        grid=null;
        grid2=null;
        switch(active)
        {
            case 0: grid='sensors_actuators';
                    break;
            case 1: grid='interfaces';
                    break;
            case 2: grid='locations';
                    break;
            case 4: grid='types_standards';
                    grid2='types_personnalises';
                    break;

        }
        isActivate[active]=true;

        if(grid){
            var numTab=active+1;
            gridParentWidth = $('#gbox_' + 'table_'+grid).parent().width();
            $('#table_'+grid).jqGrid('setGridWidth',gridParentWidth, false);
        }
        if(grid2){
            var numTab=active+1;
            gridParentWidth = $('#gbox_' + 'table_'+grid2).parent().width();
            $('#table_'+grid2).jqGrid('setGridWidth',gridParentWidth, false);
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
            case 4: if(!isActivate[active]){
                        grid_types_personnalises();
                        grid_types_standards();
                    }
                    else
                        $('#table_types_personnalises').trigger( 'reloadGrid' );
                    break;
        }
        isActivate[active]=true;
        resizeGrid();
    }

    function grid_sensors_actuators(){
    jQuery("#table_sensors_actuators").jqGrid(
    { url:'lib/php/get_sensorsactuators-jqg_grid.php',
        datatype: "xml",
        colNames:['id','n°', 'nom', 'type', 'description', 'interface', 'paramètres','lieu','état','id_type','id_interface','id_location'],
        colModel:[ {name:'id',index:'id', align:"center", width:10, hidden: true
                   },
                   {name:'id_sensor_actuator',index:'id_sensor_actuator', align:"center", width:35, fixed: true, editable: true,
                        editoptions:{size: 5, dataInit: id_sensor_actuator_DataInit},
                        editrules:{required: true, integer: true, minValue:1, custom: true, custom_func: check_id_sensor_actuator},
                        formoptions:{label: "numero", rowpos:1}
                   },
                   {name:'name',index:'name', width:100, fixed: true, editable: true,
                        editrules:{required: true, custom: true, custom_func: check_name_sensor_actuator},
                        formoptions:{label: "nom", rowpos:2}
                   },
                   {name:'type',index:'tname', width:60, fixed: true, editable: false
                   },
                   {name:'description', index:'description', width:220, fixed: true, editable: true, edittype:"textarea",
                        editoptions:{rows:5, wrap:"on", style:'width:500px'},
                        formoptions:{label: "description", rowpos:4},
                   },
                   {name:'interface',index:'iname', width:100, fixed: true, editable: false
                   },
                   {name:'parameters',index:'parameters', width:100, editable: true, edittype:"textarea",
                        editoptions:{rows:5, wrap:"on", style:'width:500px'},
                        formoptions:{label: "parametres", rowpos:6}
                   },
                   {name:'location',index:'lname', width:100, fixed: true, editable: false
                   },
                   {name:'state',index:'state', align:"center", width:50, fixed: true, editable: true, edittype:"select", formatter:'select',
                        editoptions:{value:{0:"disable",1:"enable",2:"delegate"}},
                        formoptions:{label: "etat", rowpos:8}
                   },
                   {name:'id_type',index:'id_type', width:30, editable: true, edittype:"select",
                        editoptions:{dataUrl:'lib/php/get_type-jqg_select.php'},
                        editrules:{required:true, edithidden:true}, hidden: true,
                        formoptions:{ label: 'type', rowpos:3 }
                   },
                   {name:'id_interface',index:'id_interface', width:30, editable: true, edittype:"select",
                        editoptions:{dataUrl:'lib/php/get_interfaces-jqg_select.php'},
                        editrules:{required:true, edithidden:true}, hidden: true,
                        formoptions:{ label: 'Interface', rowpos:5 }
                   },
                   {name:'id_location',index:'id_location', width:30, editable: true, edittype:"select",
                        editoptions:{dataUrl:'lib/php/get_locations-jqg_select.php'},
                        editrules:{required:true, edithidden:true}, hidden: true,
                        formoptions:{ label: 'lieu', rowpos:7 }
                   }
        ],
        editurl: 'lib/php/set_sensorsactuators-jqg_grid.php',
        rowNum:20,
        rowList:[20,50,100],
        pager: '#pager_sensors_actuators',
        sortname: 'id_sensors_actuators',
        viewrecords: true,
        height:400,
        scrollOffset: 0,
        autowidth: true,
        hidegrid: false,
        caption:""
    });
        
    jQuery("#table_sensors_actuators").jqGrid('navGrid',
        '#pager_sensors_actuators',
        {search:false, edit:true, add:true, del:true, delfunc:delete_sensor_actuator},
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {sensors_actuators_inEdit = true;},  closeAfterEdit: true}, // edit option
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {sensors_actuators_inEdit = false;}, closeAfterAdd: true}, // add option
        {width:600}  // delete option
    );
    }
    
    function grid_interfaces(){
    jQuery("#table_interfaces").jqGrid(
    { url:'lib/php/get_interfaces-jqg_grid.php',
        datatype: "xml",
        colNames:['id','n°', 'nom', 'type', 'description','périférique','paramètres','état', 'id_type'],
        colModel:[ {name:'id',index:'id', align:"center", width:10, hidden: true
                   },
                   {name:'id_interface',index:'id_interface', align:"center", width:35, fixed: true, editable: true,
                        editoptions:{size: 5, dataInit: id_interface_DataInit},
                        editrules:{required: true, integer: true, minValue:1, custom: true, custom_func: check_id_interface},
                        formoptions:{label: "numero interface", rowpos:1}
                   },
                   {name:'name',index:'name', width:100, fixed: true, editable: true,
                        editrules:{required: true, custom: true, custom_func: check_name_interface},
                        formoptions:{label: "nom interface", rowpos:2}
                   },
                   {name:'type',index:'tname', width:60, fixed: true, editable: false,
                   },
                   {name:'description', index:'description', width:220, fixed: true, editable: true, edittype:"textarea",
                        editoptions:{rows:5, wrap:"on", style:'width:500px'},
                        formoptions:{label: "description", rowpos:4},
                   },
                   {name:'dev',index:'dev', width:170, fixed: true, editable: true,
                        editoptions:{style:'width:250px'},
                        formoptions:{label: "interface", rowpos:5}
                   },
                   {name:'parameters',index:'parameters', width:135, fixed:false, editable: true, edittype:"textarea",
                        editoptions:{rows:5, wrap:"on", style:'width:500px'},
                        formoptions:{label: "parametres", rowpos:6}
                   },
                   {name:'state',index:'state', align:"center", width:50, fixed: true, editable: true, edittype:"select", formatter:'select',
                        editoptions:{value:{0:"disable",1:"enable",2:"delegate"}},
                        formoptions:{label: "etat", rowpos:7}
                   },
                   {name:'id_type',index:'id_type', width:40, editable: true, edittype:"select",
                        editoptions:{dataUrl:'lib/php/get_type-jqg_select.php'},
                        editrules:{required:true, edithidden:true}, hidden: true,
                        formoptions:{ label: 'type', rowpos:3 }
                   }
        ],
        editurl: 'lib/php/set_interfaces-jqg_grid.php',
        rowNum:20,
        rowList:[20,50,100],
        pager: '#pager_interfaces',
        sortname: 'id_interface',
        viewrecords: true,
        height:400,
        scrollOffset: 0,
        autowidth: true,
        hidegrid: false,
        caption:""
    });
        
    jQuery("#table_interfaces").jqGrid('navGrid',
        '#pager_interfaces',
        {search:false, edit:true, add:true, del:true, delfunc:delete_interface },
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {interfaces_inEdit = true;},  closeAfterEdit: true}, // edit option
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {interfaces_inEdit = false;}, closeAfterAdd: true}, // add option
        {width:600}  // delete option
    );
    }

    function grid_locations(){
    jQuery("#table_locations").jqGrid(
    { url:'lib/php/get_locations-jqg_grid.php',
        datatype: "xml",
        colNames:['id','n°', 'nom', 'description'],
        colModel:[ {name:'id',index:'id', align:"center", width:10, hidden: true
                   },
                   {name:'id_location',index:'id_location', align:"center", width:35, fixed: true, editable: true,
                        editoptions:{size: 5, dataInit: id_location_DataInit},
                        editrules:{required: true, integer: true, minValue:1, custom: true, custom_func: check_id_location},
                        formoptions:{label: "identifiation lieu", rowpos:1}
                   },
                   {name:'name',index:'name', width:200, fixed: true, editable: true,
                        editrules:{required: true, custom: true, custom_func: check_name_location},
                        formoptions:{label: "nom du lieu", rowpos:2}
                   },
                   {name:'description', index:'description', width:200, editable: true, edittype:"textarea",
                        editoptions:{rows:5, wrap:"on", style:'width:500px'},
                        formoptions:{label: "description", rowpos:3}
                   }
        ],
        editurl: 'lib/php/set_locations-jqg_grid.php',
        rowNum:20,
        rowList:[20,50,100],
        pager: '#pager_locations',
        sortname: 'id_location',
        viewrecords: true,
        height:400,
        scrollOffset: 0,
        autowidth: true,
        hidegrid: false,
        caption:""
    });
        
    jQuery("#table_locations").jqGrid('navGrid',
        '#pager_locations',
        {search:false, edit:true, add:true, del:true, delfunc:delete_location },
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {locations_inEdit = true;},  closeAfterEdit: true}, // edit option
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {locations_inEdit = false;}, closeAfterAdd: true}, // add option
        {width:600}  // delete option
    );
    }
    
    function grid_types_standards(){
    jQuery("#table_types_standards").jqGrid(
    { url:'lib/php/get_types_standards-jqg_grid.php',
        datatype: "xml",
        colNames:['id','n°', 'nom', 'description','paramètres'],
        colModel:[ {name:'id',index:'id', align:"center", width:10, hidden: true
                   },
                   {name:'id_types',index:'id_types', align:"center", width:35, fixed: true, editable: false,
                   },
                   {name:'name',index:'name', width:100, fixed: true, editable: false,
                   },
                   {name:'description', index:'description', width:300, fixed: true, editable: false, edittype:"textarea",
                   },
                   {name:'parameters', index:'parameters', width:200, editable: true, edittype:"textarea",
                        editoptions:{rows:5, wrap:"on", style:'width:500px'},
                        formoptions:{label: "paramètres", rowpos:4}
                   }
        ],
        editurl: 'lib/php/set_types_standards-jqg_grid.php',
        rowNum:20,
        rowList:[20,50,100],
        pager: '#pager_types_standards',
        sortname: 'id_type',
        viewrecords: true,
        height:209,
        scrollOffset: 0,
        autowidth: true,
        hidegrid: false,
        caption:"types standards"
    });
        
    jQuery("#table_types_standards").jqGrid('navGrid',
        '#pager_types_standards',
        {search:false, edit:true, add:false, del:false },
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {types_standards_inEdit = true;},  closeAfterEdit: true} // edit option
    );
    }

function grid_types_personnalises(){
    jQuery("#table_types_personnalises").jqGrid(
    { url:'lib/php/get_types_personnalises-jqg_grid.php',
        datatype: "xml",
        colNames:['id','n°', 'nom', 'description','paramètres'],
        colModel:[ {name:'id',index:'id', align:"center", width:10, hidden: true
                   },
                   {name:'id_type',index:'id_type', align:"center", width:35, fixed: true, editable: true,
                        editoptions:{size: 5, dataInit: id_types_personnalises_DataInit},
                        editrules:{required: true, integer: true, minValue:1, custom: true, custom_func: check_id_types_personnalises},
                        formoptions:{label: "indentfiant type", rowpos:1}
                   },
                   {name:'name',index:'name', width:100, fixed: true, editable: true,
                        editrules:{required: true, custom: true, custom_func: check_name_types_personnalises},
                        formoptions:{label: "nom du type", rowpos:2}
                   },
                   {name:'description', index:'description', width:300, fixed: true, editable: true, edittype:"textarea",
                        editoptions:{rows:5, wrap:"on", style:'width:500px'},
                        formoptions:{label: "description", rowpos:3}
                   },
                   {name:'parameters', index:'parameters', width:200, editable: true, edittype:"textarea",
                        editoptions:{rows:5, wrap:"on", style:'width:500px'},
                        formoptions:{label: "paramètres", rowpos:4}
                   }
        ],
        editurl: 'lib/php/set_types_personnalises-jqg_grid.php',
        rowNum:20,
        rowList:[20,50,100],
        pager: '#pager_types_personnalises',
        sortname: 'id_type',
        viewrecords: true,
        height:209,
        scrollOffset: 0,
        autowidth: true,
        hidegrid: false,
        caption:"types personnalisés"
    });
        
    jQuery("#table_types_personnalises").jqGrid('navGrid',
        '#pager_types_personnalises',
        {search:false, edit:true, add:true, del:true, delfunc:delete_type },
         {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {types_personnalises_inEdit = true;},  closeAfterEdit: true}, // edit option
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {personnalises_inEdit = false;}, closeAfterAdd: true}, // add option
        {width:600}  // delete option
    );
    }


    $("#tabs1").tabs({activate:activeGrids});

    $("div.ui-tabs-panel").css('padding-left','1px');
    $("div.ui-tabs-panel").css('padding-right','2px');
    $("div.ui-tabs-panel").css('padding-top','5px');
    $("div.ui-tabs-panel").css('padding-bottom','1px');


    activeGrids();
    $("#sensors_actuators_add").button();
    $("#sensors_actuators_edit").button();
    $("#sensors_actuators_del").button();
    $(window).on('resize',resizeGrid).trigger('resize');
});
</script>
