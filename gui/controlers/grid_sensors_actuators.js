function check_id_sensor_actuator(value,_name){
    if(!check_auth())
        return false;
    passes=check_field_exist_by_id('table_sensors_actuators','sensors_actuators','id_sensor_actuator',value);
    return [passes, _name+str_double_dot+str_num+" "+str_sensor_or_actuator+" ("+value+") "+str_allready_used];
}


function check_name_sensor_actuator(value,_name){
    if(!check_auth())
        return false;
        
    ret=check_name_string(value,_name);
    if(ret[0]==false)
        return ret;
    value=value.toUpperCase();
    var passes=check_field_exist_by_id('table_sensors_actuators','sensors_actuators','name',value);
    return [passes, _name+str_double_dot+str_name+" "+str_sensor_or_actuator+" ("+value+") "+str_allready_used];
}


function id_sensor_actuator_DataInit(element){
    _DataInit(element,sensors_actuators_inEdit,'models/get_next_id_sensor_actuator-jqg_grid.php');
}


function db_delete_sensor_actuator(id){
    db_delete('table_sensors_actuators','sensors_actuators',id);
}


function delete_sensor_actuator(){
    if(!check_auth())
        return false;
    var grid_id='table_sensors_actuators',
    myGrid = $("#"+grid_id),
    selRowId = myGrid.jqGrid ('getGridParam', 'selrow');
    
    if(selRowId){
        var id = myGrid.jqGrid ('getCell', selRowId, 'id');
        
        mea_yesno2(str_title_del, str_del_sensoractuator, db_delete_sensor_actuator, id);
    }
}


function grid_sensors_actuators(){
    jQuery("#table_sensors_actuators").jqGrid(
    { url:'models/get_sensorsactuators-jqg_grid.php',
        datatype: "xml",
        colNames:['id',str_num.capitalize(), str_name.capitalize(), str_type.capitalize(), str_description.capitalize(), str_interface.capitalize(), str_parameters.capitalize(),str_location.capitalize(),str_stat.capitalize(),'id_type','id_interface','id_location'],
        colModel:[ {name:'id',index:'id', align:"center", width:10, hidden: true
                   },
                   {name:'id_sensor_actuator',index:'id_sensor_actuator', align:"center", width:35, fixed: true, editable: true,
                        editoptions:{size: 5, dataInit: id_sensor_actuator_DataInit},
                        editrules:{required: true, integer: true, minValue:1, custom: true, custom_func: check_id_sensor_actuator},
                        formoptions:{label: str_num.capitalize(), rowpos:1}
                   },
                   {name:'name',index:'name', width:100, fixed: true, editable: true,
                        editrules:{required: true, custom: true, custom_func: check_name_sensor_actuator},
                        formoptions:{label: str_name.capitalize(), rowpos:2}
                   },
                   {name:'type',index:'tname', width:60, fixed: true, editable: false
                   },
                   {name:'description', index:'description', width:220, fixed: true, editable: true, edittype:"textarea",
                        editoptions:{rows:5, wrap:"on", style:'width:500px'},
                        formoptions:{label: str_description.capitalize(), rowpos:4},
                   },
                   {name:'interface',index:'iname', width:100, fixed: true, editable: false
                   },
                   {name:'parameters',index:'parameters', width:100, editable: true, edittype:"textarea",
                        editoptions:{rows:5, wrap:"on", style:'width:500px'},
                        formoptions:{label: str_parameters.capitalize(), rowpos:6}
                   },
                   {name:'location',index:'lname', width:100, fixed: true, editable: false
                   },
                   {name:'state',index:'state', align:"center", width:50, fixed: true, editable: true, edittype:"select", formatter:'select',
                        editoptions:{value:{0:"disable",1:"enable",2:"delegate"}},
                        formoptions:{label: str_stat.capitalize(), rowpos:8}
                   },
                   {name:'id_type',index:'id_type', width:30, editable: true, edittype:"select",
                        editoptions:{dataUrl:'models/get_type-jqg_select.php'},
                        editrules:{required:true, edithidden:true}, hidden: true,
                        formoptions:{ label: str_type.capitalize(), rowpos:3 }
                   },
                   {name:'id_interface',index:'id_interface', width:30, editable: true, edittype:"select",
                        editoptions:{dataUrl:'models/get_interfaces-jqg_select.php'},
                        editrules:{required:true, edithidden:true}, hidden: true,
                        formoptions:{ label: str_interface.capitalize(), rowpos:5 }
                   },
                   {name:'id_location',index:'id_location', width:30, editable: true, edittype:"select",
                        editoptions:{dataUrl:'models/get_locations-jqg_select.php'},
                        editrules:{required:true, edithidden:true}, hidden: true,
                        formoptions:{ label: str_location.capitalize(), rowpos:7 }
                   }
        ],
        editurl: 'models/set_sensorsactuators-jqg_grid.php',
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
            $("#table_sensors_actuators_edit").button('disable');
            $("#table_sensors_actuators_del").button('disable');
        },
        onSelectRow: function(){
            $("#table_sensors_actuators_edit").button('enable');
            $("#table_sensors_actuators_del").button('enable');
        },
        loadError: function(xhr,st,err) { 
            if(st=="parsererror") {
                data = JSON.parse(xhr.responseText);
                if(data.error==99) {
                    mea_alert2(str_Error+" : ", str_not_connected, function(){window.location = "login.php";} );
                    return false;
                }
                if(authdata.profil==98) {
                    mea_alert2(str_Error+" : ", str_not_allowed, function(){window.location = "index.php";} );
                    return false;
                }
                alert(str_unknow_error+" : "+data.error_msg);
                return false;
            }
            else
                alert("Type: "+st+"; Response: "+ xhr.status + " "+xhr.statusText+" "+xhr.responseText);
        },
    });
        
    jQuery("#table_sensors_actuators").jqGrid('navGrid',
        '#pager_sensors_actuators',
        {search:false, edit:true, add:true, del:true, delfunc:delete_sensor_actuator},
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {sensors_actuators_inEdit = true; return check_auth(); },  closeAfterEdit: true, afterShowForm: function(){ $("#name").focus()} }, // edit option
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {sensors_actuators_inEdit = false; return check_auth(); }, closeAfterAdd: true}, // add option
        {width:600}  // delete option
    );

    $('#t_table_sensors_actuators').height(40);
    add_button_to_toolbar('table_sensors_actuators',"add",str_add.capitalize(),function(){if(check_auth()){_addRow('table_sensors_actuators',function(){sensors_actuators_inEdit = false;})} },"ui-icon-plusthick");
    add_button_to_toolbar('table_sensors_actuators',"edit",str_edit.capitalize(),function(){if(check_auth()){_editRow('table_sensors_actuators',function(){sensors_actuators_inEdit = true;})} },"ui-icon-pencil");
    add_button_to_toolbar('table_sensors_actuators',"del",str_del.capitalize(),function(){delete_sensor_actuator(); },"ui-icon-trash");
}

