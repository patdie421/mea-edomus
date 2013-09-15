function id_location_DataInit(element){
    _DataInit(element,locations_inEdit,'../models/get_next_id_location-jqg_grid.php');
}


function delete_location(){
    var grid_id='table_locations',
        myGrid = $("#"+grid_id),
        selRowId = myGrid.jqGrid ('getGridParam', 'selrow');
         
    if(selRowId){
        var id = myGrid.jqGrid ('getCell', selRowId, 'id');
            id_location = myGrid.jqGrid ('getCell', selRowId, 'id_location'),
            name = myGrid.jqGrid ('getCell', selRowId, 'name');
                
        $.ajax({ url: '../models/get_field-jqg_grid.php',
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
                } else {
                    mea_yesno("OK pour suppression", "Oui ou Non", db_delete_location, id);
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
    ret=check_name_string(value,_name);
    if(ret[0]==false)
        return ret;
    value=value.toUpperCase();
    var passes=check_field_exist_by_id('table_locations','locations','name',value);
    return [passes, _name+" : nom de lieu ("+value+") déjà utilisé "];
}


function check_id_location(value,_name){
    passes=check_field_exist_by_id('table_locations','locations','id_location',value);
    return [passes, _name+" : numéro de lieu ("+value+") déjà utilisé "];
}


function grid_locations(){
    jQuery("#table_locations").jqGrid(
    { url:'../models/get_locations-jqg_grid.php',
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
        editurl: '../models/set_locations-jqg_grid.php',
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
            $("#table_locations_Editer").button('disable');
            $("#table_locations_Supprimer").button('disable');
        },
        onSelectRow: function(){
            $("#table_locations_Editer").button('enable');
            $("#table_locations_Supprimer").button('enable');
        },
    });
        
    jQuery("#table_locations").jqGrid('navGrid',
        '#pager_locations',
        {search:false, edit:true, add:true, del:true, delfunc:delete_location },
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {locations_inEdit = true;},  closeAfterEdit: true, afterShowForm: function(){ $("#name").focus()} }, // edit option
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {locations_inEdit = false;}, closeAfterAdd: true}, // add option
        {width:600}  // delete option
    );
    $('#t_table_locations').height(40);
    add_button_to_toolbar('table_locations','Ajouter',function(){_addRow('table_locations',function(){locations_inEdit = false;})},"ui-icon-plusthick");
    add_button_to_toolbar('table_locations','Editer',function(){_editRow('table_locations',function(){locations_inEdit = true;})},"ui-icon-pencil");
    add_button_to_toolbar('table_locations','Supprimer',function(){delete_location();},"ui-icon-trash");
}
