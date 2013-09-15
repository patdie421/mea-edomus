function check_id_sensor_actuator(value,_name){
    passes=check_field_exist_by_id('table_sensors_actuators','sensors_actuators','id_sensor_actuator',value);
    return [passes, _name+" : id capteur ou actionneur ("+value+") déjà utilisé "];
}


function check_name_sensor_actuator(value,_name){
    ret=check_name_string(value,_name);
    if(ret[0]==false)
        return ret;
    value=value.toUpperCase();
    var passes=check_field_exist_by_id('table_sensors_actuators','sensors_actuators','name',value);
    return [passes, _name+" : nom capteur ou actionneur ("+value+") déjà utilisé "];
}


function id_sensor_actuator_DataInit(element){
    _DataInit(element,sensors_actuators_inEdit,'../models/get_next_id_sensor_actuator-jqg_grid.php');
}


function db_delete_sensor_actuator(id){
    db_delete('table_sensors_actuators','sensors_actuators',id);
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


function grid_sensors_actuators(){
    jQuery("#table_sensors_actuators").jqGrid(
    { url:'../models/get_sensorsactuators-jqg_grid.php',
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
                        editoptions:{dataUrl:'../models/get_type-jqg_select.php'},
                        editrules:{required:true, edithidden:true}, hidden: true,
                        formoptions:{ label: 'type', rowpos:3 }
                   },
                   {name:'id_interface',index:'id_interface', width:30, editable: true, edittype:"select",
                        editoptions:{dataUrl:'../models/get_interfaces-jqg_select.php'},
                        editrules:{required:true, edithidden:true}, hidden: true,
                        formoptions:{ label: 'Interface', rowpos:5 }
                   },
                   {name:'id_location',index:'id_location', width:30, editable: true, edittype:"select",
                        editoptions:{dataUrl:'../models/get_locations-jqg_select.php'},
                        editrules:{required:true, edithidden:true}, hidden: true,
                        formoptions:{ label: 'lieu', rowpos:7 }
                   }
        ],
        editurl: '../models/set_sensorsactuators-jqg_grid.php',
        rowNum:20,
        rowList:[20,50,100],
        pager: '#pager_sensors_actuators',
        sortname: 'id_sensors_actuators',
        viewrecords: true,
        height:360,
        scrollOffset: 0,
        autowidth: true,
        hidegrid: false,
        caption:"",
        toolbar: [true,"top"],
        gridComplete: function(){
            $("#table_sensors_actuators_Editer").button('disable');
            $("#table_sensors_actuators_Supprimer").button('disable');
        },
        onSelectRow: function(){
            $("#table_sensors_actuators_Editer").button('enable');
            $("#table_sensors_actuators_Supprimer").button('enable');
        },
    });
        
    jQuery("#table_sensors_actuators").jqGrid('navGrid',
        '#pager_sensors_actuators',
        {search:false, edit:true, add:true, del:true, delfunc:delete_sensor_actuator},
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {sensors_actuators_inEdit = true;},  closeAfterEdit: true, afterShowForm: function(){ $("#name").focus()} }, // edit option
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {sensors_actuators_inEdit = false;}, closeAfterAdd: true}, // add option
        {width:600}  // delete option
    );

    $('#t_table_sensors_actuators').height(40);
    add_button_to_toolbar('table_sensors_actuators','Ajouter',function(){_addRow('table_sensors_actuators',function(){sensors_actuators_inEdit = false;})},"ui-icon-plusthick");
    add_button_to_toolbar('table_sensors_actuators','Editer',function(){_editRow('table_sensors_actuators',function(){sensors_actuators_inEdit = true;})},"ui-icon-pencil");
    add_button_to_toolbar('table_sensors_actuators','Supprimer',function(){delete_sensor_actuator(); },"ui-icon-trash");
}
