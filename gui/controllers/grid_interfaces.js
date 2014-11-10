var interfaces_inEdit;


function check_id_interface(value,_name){
    check_auth();

    passes=check_field_exist_by_id('table_interfaces','interfaces','id_interface',value);
    return [passes, _name+str_double_dot+str_num+" "+str_interface+" ("+value+") "+str_allready_used];
}


function check_name_interface(value,_name){
    check_auth();

        ret=check_name_string(value,_name);
    if(ret[0]==false)
        return ret;
    value=value.toUpperCase();
    var passes=check_field_exist_by_id('table_interfaces','interfaces','name',value);
    return [passes, _name+str_double_dot+str_name+" "+str_interface+" ("+value+") "+str_allready_used];
}


function id_interface_DataInit(element){
    _DataInit(element,interfaces_inEdit,'models/get_next_id_interface-jqg_grid.php');
}


function db_delete_interface(id){
    db_delete('table_interfaces','interfaces',id);
}


function delete_interface(){
    if(!check_auth())
        return false;

    var grid_id='table_interfaces',
    myGrid = $("#"+grid_id),
    selRowId = myGrid.jqGrid ('getGridParam', 'selrow');
    
    if(selRowId){
        var id = myGrid.jqGrid ('getCell', selRowId, 'id');
        id_interface = myGrid.jqGrid ('getCell', selRowId, 'id_interface'),
        name = myGrid.jqGrid ('getCell', selRowId, 'name');
        
        $.ajax({
            url: 'models/get_field-jqg_grid.php',
            async: false,
            type: 'GET',
            dataType: 'json',
            data: {table:'sensors_actuators', field:'name', where:'id_interface='+id_interface},
            success: function(data){
                         if(data.values.length > 0){
                             msg="<DIV style='padding-left:30px'>"+str_the1.capitalize()+" "+str_interface+
                                 name+
                                 str_cant_be_del_because+"</BR></BR>"+str_sensor_s_actuator_s+str_double_dot+
                                 data.values+
                                 "</BR></BR>"+str_update_before+"</DIV>";
                             mea_alert2(str_delete_failed.capitalize(), msg);

                         } else {
                             mea_yesno2(str_title_del.capitalize(), str_del_interface.capitalize(), db_delete_interface, id);
                         }
                     },
            error:ajax_error
        });
    }
}


function grid_interfaces(){
    jQuery("#table_interfaces").jqGrid({
        url:'models/get_interfaces-jqg_grid.php',
        datatype: "xml",
        colNames:['id',str_num.capitalize(), str_name.capitalize(), str_type.capitalize(), str_description.capitalize(),str_device.capitalize(),str_parameters.capitalize(),str_type.capitalize(), 'id_type'],
        colModel:[ { name:'id',index:'id', align:"center", width:10, hidden: true },
                   { name:'id_interface',index:'id_interface', align:"center", width:35, fixed: true, editable: true,
                     editoptions:{size: 5, dataInit: id_interface_DataInit},
                     editrules:{required: true, integer: true, minValue:1, custom: true, custom_func: check_id_interface},
                     formoptions:{label: str_num.capitalize(), rowpos:1}
                   },
                   { name:'name',index:'name', width:100, fixed: true, editable: true,
                     editrules:{required: true, custom: true, custom_func: check_name_interface},
                     formoptions:{label: str_name.capitalize(), rowpos:2}
                   },
                   { name:'type',index:'tname', width:60, fixed: true, editable: false,
                   },
                   { name:'description', index:'description', width:220, fixed: true, editable: true, edittype:"textarea",
                     editoptions:{rows:5, wrap:"on", style:'width:500px'},
                     formoptions:{label: str_description.capitalize(), rowpos:4},
                   },
                   { name:'dev',index:'dev', width:170, fixed: true, editable: true,
                     editoptions:{style:'width:250px'},
                     formoptions:{label: str_interface.capitalize(), rowpos:5}
                   },
                   { name:'parameters',index:'parameters', width:135, fixed:false, editable: true, edittype:"textarea",
                     editoptions:{rows:5, wrap:"on", style:'width:500px'},
                     formoptions:{label: str_parameters.capitalize(), rowpos:6}
                   },
                   { name:'state',index:'state', align:"center", width:50, fixed: true, editable: true, edittype:"select", formatter:'select',
                     editoptions:{value:{0:"disable",1:"enable",2:"delegate"}},
                     formoptions:{label: str_stat.capitalize(), rowpos:7}
                   },
                   { name:'id_type',index:'id_type', width:40, editable: true, edittype:"select",
                     editoptions:{dataUrl:'models/get_type-jqg_select.php'},
                     editrules:{required:true, edithidden:true}, hidden: true,
                     formoptions:{ label: str_type.capitalize(), rowpos:3 }
        } ],
        editurl: 'models/set_interfaces-jqg_grid.php',
        rowNum:20,
        rowList:[20,50,100],
        pager: '#pager_interfaces',
        sortname: 'id_interface',
        viewrecords: true,
        height:360,
        scrollOffset: 0,
        autowidth: true,
        hidegrid: false,
        caption:"",
        toolbar: [true,"top"],
        gridComplete: function(){
            $("#table_interfaces_edit").button('disable');
            $("#table_interfaces_del").button('disable');
        },
        onSelectRow: function(){
            $("#table_interfaces_edit").button('enable');
            $("#table_interfaces_del").button('enable');
        },
        loadError: function(xhr,st,err) { 
            if(st=="parsererror") {
                data = JSON.parse(xhr.responseText);
                if(data.error==99) {
                    mea_alert2(str_Error.capitalize()+" : ", str_not_connected, function(){window.location = "login.php";} );
                    return false;
                }
                if(data.error==98) {
                    mea_alert2(str_Error.capitalize()+" : ", str_not_allowed, function(){window.location = "index.php";} );
                    return false;
                }
                alert(str_unknow_error.capitalize()+" : "+data.error_msg);
                return false;
            }
            else
                alert("Type: "+st+"; Response: "+ xhr.status + " "+xhr.statusText+" "+xhr.responseText);
        }
});

    
    jQuery("#table_interfaces").jqGrid('navGrid',
        '#pager_interfaces',
        {search:false, edit:true, add:true, del:true, delfunc:delete_interface },
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {interfaces_inEdit = true;return check_auth(); },  closeAfterEdit: true, afterShowForm: function(){ $("#name").focus() } }, // edit option
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {interfaces_inEdit = false;return check_auth(); }, closeAfterAdd: true}, // add option
        {width:600}  // delete option
    );
    
    $('#t_table_interfaces').height(40);
    add_button_to_toolbar('table_interfaces',"add",str_add.capitalize(),function(){if(check_auth()){_addRow('table_interfaces',function(){interfaces_inEdit = false;})}},"ui-icon-plusthick");
    add_button_to_toolbar('table_interfaces',"edit",str_edit.capitalize(),function(){if(check_auth()){_editRow('table_interfaces',function(){interfaces_inEdit = true;})}},"ui-icon-pencil");
    add_button_to_toolbar('table_interfaces',"del",str_del.capitalize(),function(){delete_interface();},"ui-icon-trash");
}
