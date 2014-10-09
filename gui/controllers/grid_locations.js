
function id_location_DataInit(element){
    _DataInit(element,locations_inEdit,'models/get_next_id_location-jqg_grid.php');
}


function delete_location(){
    if(!check_auth())
        return false;
    var grid_id='table_locations',
        myGrid = $("#"+grid_id),
        selRowId = myGrid.jqGrid ('getGridParam', 'selrow');
         
    if(selRowId){
        var id = myGrid.jqGrid ('getCell', selRowId, 'id');
            id_location = myGrid.jqGrid ('getCell', selRowId, 'id_location'),
            name = myGrid.jqGrid ('getCell', selRowId, 'name');
                
        $.ajax({ url: 'models/get_field-jqg_grid.php',
            async: false,
            type: 'GET',
            dataType: 'json',
            data: {table:'sensors_actuators', field:'name', where:'id_location='+id_location},
            success: function(data){
                if(data.values.length > 0){
                    msg="<DIV style='padding-left:30px'>Le "+str_location+" "+
                        name+
                        str_cant_be_del_because2+"</BR></BR>"+str_sensor_s_actuator_s+str_double_dot+
                        data.values+
                        "</BR></BR>"+str_update_before+"</DIV>"
                        mea_alert2(str_delete_failed, msg);
                } else {
                    mea_yesno2(str_title_del, str_del_location, db_delete_location, id);
                }
            },
            error:ajax_error
        });
    }
}

function db_delete_location(id){
    db_delete('table_locations','locations',id);
}


function check_name_location(value,_name){
    if(!check_auth())
        return false;
    ret=check_name_string(value,_name);
    if(ret[0]==false)
        return ret;
    value=value.toUpperCase();
    passes=check_field_exist_by_id('table_locations','locations','name',value);
    return [passes, _name+str_double_dot+str_name+" "+str_location+" ("+value+") "+str_allready_used];
}


function check_id_location(value,_name){
    if(!check_auth())
        return false;
    passes=check_field_exist_by_id('table_locations','locations','id_location',value);
    return [passes, _name+str_double_dot+str_num+" "+str_location+" ("+value+") "+str_allready_used];
}

function grid_locations(){
    jQuery("#table_locations").jqGrid(
    { url:'models/get_locations-jqg_grid.php',
        datatype: "xml",
        colNames:['id',str_num.capitalize(), str_name.capitalize(), str_description.capitalize()],
        colModel:[ {name:'id',index:'id', align:"center", width:10, hidden: true
                   },
                   {name:'id_location',index:'id_location', align:"center", width:35, fixed: true, editable: true,
                        editoptions:{size: 5, dataInit: id_location_DataInit},
                        editrules:{required: true, integer: true, minValue:1, custom: true, custom_func: check_id_location},
                        formoptions:{label: str_num.capitalize(), rowpos:1}
                   },
                   {name:'name',index:'name', width:200, fixed: true, editable: true,
                        editrules:{required: true, custom: true, custom_func: check_name_location},
                        formoptions:{label: str_name.capitalize(), rowpos:2}
                   },
                   {name:'description', index:'description', width:200, editable: true, edittype:"textarea",
                        editoptions:{rows:5, wrap:"on", style:'width:500px'},
                        formoptions:{label: str_description.capitalize(), rowpos:3}
                   }
        ],
        editurl: 'models/set_locations-jqg_grid.php',
        rowNum:20,
        rowList:[20,50,100],
        pager: '#pager_locations',
        sortname: 'id_location',
        viewrecords: true,
        height:360,
        scrollOffset: 0,
        autowidth: true,
        hidegrid: false,
        caption:"",
        toolbar: [true,"top"],
        gridComplete: function(){
            $("#table_locations_edit").button('disable');
            $("#table_locations_del").button('disable');
        },
        onSelectRow: function(){
            $("#table_locations_edit").button('enable');
            $("#table_locations_del").button('enable');
        },
        loadError: function(xhr,st,err) { 
            if(st=="parsererror") {
                data = JSON.parse(xhr.responseText);
                if(data.error==99) {
                    mea_alert2(str_Error.capitalize()+" : ", str_not_connected.capitalize(), function(){window.location = "login.php";} );
                    return false;
                }
                if(authdata.profil==98) {
                    mea_alert2(str_Error.capitalize()+" : ", str_not_allowed.capitalize(), function(){window.location = "index.php";} );
                    return false;
                }
                alert(str_unknow_error.capitalize()+" : "+data.error_msg);
                return false;
            }
            else
                alert("Type: "+st+"; Response: "+ xhr.status + " "+xhr.statusText+" "+xhr.responseText);
        },
    });
        
    jQuery("#table_locations").jqGrid('navGrid',
        '#pager_locations',
        {search:false, edit:true, add:true, del:true, delfunc:delete_location },
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {locations_inEdit = true; return check_auth();},  closeAfterEdit: true, afterShowForm: function(){ $("#name").focus()} }, // edit option
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {locations_inEdit = false; return check_auth();}, closeAfterAdd: true}, // add option
        {width:600}  // delete option
    );
    $('#t_table_locations').height(40);
    add_button_to_toolbar('table_locations',"add",str_add.capitalize(),function(){if(check_auth()){_addRow('table_locations',function(){locations_inEdit = false;})} },"ui-icon-plusthick");
    add_button_to_toolbar('table_locations',"edit",str_edit.capitalize(),function(){if(check_auth()){_editRow('table_locations',function(){locations_inEdit = true;})} },"ui-icon-pencil");
    add_button_to_toolbar('table_locations',"del",str_del.capitalize(),function(){delete_location();},"ui-icon-trash");
}

