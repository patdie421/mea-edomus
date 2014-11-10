function check_id_types(value,_name){
    if(!check_auth())
        return false;

    passes=check_field_exist_by_id('table_types','types','id_type',value);
    return [passes, _name+str_double_dot+str_num+" "+str_type+" ("+value+") "+str_allready_used];
}


function check_name_types(value,_name){
    if(!check_auth())
        return false;

    ret=check_name_string(value,_name);
    if(ret[0]==false)
        return ret;
    value=value.toUpperCase();
    var passes=check_field_exist_by_id('table_types','types','name',value);
    return [passes, _name+str_double_dot+str_name+" "+str_type+" ("+value+") "+str_allready_used];
}


function id_types_DataInit(element){
    if(types_inEdit == true){
        $(element).attr("readonly", "readonly");
            $(element).css({color: "#b9b9b9", background: "#E0E0E0"});
    } else {
        $.get('models/get_next_id_type-jqg_grid.php',
              {},
              function(data){
              $(element).val(data.next_id);
              },
              "json"
              );
    }
}


function others_DataInit(element){
    if(types_inEdit == true){
        myGrid = $("#table_types");
        selRowId = myGrid.jqGrid('getGridParam', 'selrow');
        id_type = myGrid.jqGrid('getCell', selRowId, 'id_type');
        if(id_type<2000)
        {
            $(element).attr("readonly", "readonly");
            $(element).css({color: "#b9b9b9", background: "#E0E0E0"});
        }
    }
}


function table_types_set_focus()
{
    if(!check_auth())
        return false;

    if(types_inEdit == true){
        myGrid = $("#table_types");
        selRowId = myGrid.jqGrid('getGridParam', 'selrow');
        id_type = myGrid.jqGrid('getCell', selRowId, 'id_type');
        if(id_type<2000){
            $("#parameters").focus();
        } else {
            $("#name").focus();
        }
    }
}


function db_delete_type(id){
    db_delete('table_types','types',id);
}


function grid_delete_type(){
    if(!check_auth())
        return false;

    var grid_id='table_types',
        myGrid = $("#"+grid_id),
        selRowId = myGrid.jqGrid ('getGridParam', 'selrow');
         
    if(selRowId){
        var id = myGrid.jqGrid ('getCell', selRowId, 'id');
            id_type = myGrid.jqGrid ('getCell', selRowId, 'id_type'),
            name = myGrid.jqGrid ('getCell', selRowId, 'name');
        if(id_type<2000){
            mea_alert2(str_delete_failed.capitalize(), str_type_cant_be_del.capitalize());
        } else {
            $.ajax({ url: 'models/get_2fields-jqg_grid.php',
                async: true,
                type: 'GET',
                dataType: 'json',
                data: { table1:'sensors_actuators', field1:'name', where1:'id_type='+id_type,
                        table2:'interfaces', field2:'name', where2:'id_type='+id_type
                },
                success: function(data){
                    var msg="";
                    if(data.values1.length > 0 || data.values2.length > 0){
                        msg="<DIV style='padding-left:30px'>"+str_the2.capitalize()+" "+str_type+" "+name+str_cant_be_del_because2+"</BR></BR>";
                        if(data.values1.length)
                            msg=msg+str_sensor_s_actuator_s+str_double_dot+data.values1+"</BR>";
                        if(data.values2.length)
                            msg=msg+str_interface_s+str_double_dot+data.values2+"</BR>";
                        msg=msg+"</BR></BR>"+str_update_before+"</DIV>";
                        mea_alert2(str_delete_failed.capitalize(), msg);
                    } else {
                        mea_yesno2(str_title_del.capitalize(), str_del_type.capitalize(), db_delete_type, id);
                    }
                },
                error: ajax_error
            });
        }
    }
}


function grid_types(){
    jQuery("#table_types").jqGrid(
    { url:'models/get_types-jqg_grid.php',
        datatype: "xml",
        colNames:['id',str_num.capitalize(), str_name.capitalize(), str_description.capitalize(),str_parameters.capitalize(),str_flags.capitalize()],
        colModel:[ { name:'id',index:'id', align:"center", width:10, hidden: true,
                   },
                   { name:'id_type',index:'id_type', align:"center", width:35, fixed: true, editable: true,
                     editoptions:{size: 5, dataInit: id_types_DataInit},
                     editrules:{required: true, integer: true, minValue:1, custom: true, custom_func: check_id_types},
                     formoptions:{label: str_num.capitalize(), rowpos:1}
                   },
                   { name:'name',index:'name', width:100, fixed: true, editable: true,
                     editoptions:{dataInit: others_DataInit},
                     editrules:{required: true, custom: true, custom_func: check_name_types},
                     formoptions:{label: str_name.capitalize(), rowpos:2}
                   },
                   { name:'description', index:'description', width:300, fixed: true, editable: true, edittype:"textarea",
                     editoptions:{rows:5, wrap:"on", style:'width:500px', dataInit: others_DataInit}, formoptions:{label: str_description.capitalize(), rowpos:3},
                   },
                   { name:'parameters', index:'parameters', width:200, editable: true, edittype:"textarea",
                     editoptions:{rows:5, wrap:"on", style:'width:500px'},
                     formoptions:{label: str_parameters.capitalize(), rowpos:4}
                   },
                   { name:'flags',index:'flags', align:"center", width:10, editable: false, hidden: true,		
                   },
        ],
        editurl: 'models/set_types-jqg_grid.php',
        rowNum:20,
        rowList:[20,50,100],
        pager: '#pager_types',
        sortname: 'id_type',
        viewrecords: true,
        height:360,
        scrollOffset: 0,
        autowidth: true,
        hidegrid: false,
        caption:"",
        toolbar: [true,"top"],
        gridComplete: function(){
            $("#table_types_edit").button('disable');
            $("#table_types_del").button('disable');
        },
        onSelectRow: function(){
            myGrid = $("#table_types");
            selRowId = myGrid.jqGrid('getGridParam', 'selrow');
            id_type = myGrid.jqGrid('getCell', selRowId, 'id_type');
            $("#table_types_edit").button('enable');
            if(id_type<2000)
            {
                $("#table_types_del").button('disable');
            }
            else
            {
                $("#table_types_del").button('enable');
            }
        },
        grouping:true,
        groupingView : {
            groupField : ['flags'],
            groupColumnShow : [false],
            groupDataSorted : true,
            groupCollapse: true,
            groupText : ['<b>{0} - {1} Item(s)</b>']
        },
        loadComplete: function () {
            var $this = $(this), firstGroup = $this.find('tr.jqgroup:last');
            if (firstGroup.length > 0) {
                $this.jqGrid('groupingToggle', firstGroup[0].id);
            }
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
    
    jQuery("#table_types").jqGrid('navGrid',
        '#pager_types',
        {search:false, edit:true, add:true, del:true, delfunc:grid_delete_type },
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {types_inEdit = true; return check_auth();},  closeAfterEdit: true, afterShowForm: table_types_set_focus},
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {types_inEdit = false; return check_auth();},  closeAfterAdd: true}
    );
    $('#t_table_types').height(40);

    add_button_to_toolbar('table_types',"add",str_add.capitalize(),function(){if(check_auth()){_addRow('table_types',function(){types_inEdit = false;})} },"ui-icon-plusthick");
    add_button_to_toolbar('table_types',"edit",str_edit.capitalize(),function(){ if(check_auth()){_editRow('table_types',function(){types_inEdit = true;},table_types_set_focus);} },"ui-icon-pencil");
    add_button_to_toolbar('table_types',"del",str_del.capitalize(),function(){grid_delete_type();},"ui-icon-trash");
}
