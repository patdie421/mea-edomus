var interfaces_inEdit;


function check_id_interface(value,_name){
    passes=check_field_exist_by_id('table_interfaces','interfaces','id_interface',value);
    return [passes, _name+" : valeur("+value+") déjà utilisée"];
}


function check_name_interface(value,_name){
    ret=check_name_string(value,_name);
    if(ret[0]==false)
        return ret;
    value=value.toUpperCase();
    var passes=check_field_exist_by_id('table_interfaces','interfaces','name',value);
    return [passes, _name+" : nom interface("+value+") déjà utilisé "];
}


function id_interface_DataInit(element){
    _DataInit(element,interfaces_inEdit,'../models/get_next_id_interface-jqg_grid.php');
}


function db_delete_interface(id){
    db_delete('table_interfaces','interfaces',id);
}


function delete_interface(){
    var grid_id='table_interfaces',
    myGrid = $("#"+grid_id),
    selRowId = myGrid.jqGrid ('getGridParam', 'selrow');
    
    if(selRowId){
        var id = myGrid.jqGrid ('getCell', selRowId, 'id');
        id_interface = myGrid.jqGrid ('getCell', selRowId, 'id_interface'),
        name = myGrid.jqGrid ('getCell', selRowId, 'name');
        
        $.ajax({
            url: '../models/get_field-jqg_grid.php',
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
                         } else {
                             mea_yesno("OK pour suppression", "Oui ou Non", db_delete_interface, id);
                         }
                     },
            error:ajax_error
        });
    }
}


function grid_interfaces(){
    jQuery("#table_interfaces").jqGrid({
        url:'../models/get_interfaces-jqg_grid.php',
        datatype: "xml",
        colNames:['id','n°', 'nom', 'type', 'description','périférique','paramètres','état', 'id_type'],
        colModel:[ { name:'id',index:'id', align:"center", width:10, hidden: true },
                   { name:'id_interface',index:'id_interface', align:"center", width:35, fixed: true, editable: true,
                     editoptions:{size: 5, dataInit: id_interface_DataInit},
                     editrules:{required: true, integer: true, minValue:1, custom: true, custom_func: check_id_interface},
                     formoptions:{label: "numero interface", rowpos:1}
                   },
                   { name:'name',index:'name', width:100, fixed: true, editable: true,
                     editrules:{required: true, custom: true, custom_func: check_name_interface},
                     formoptions:{label: "nom interface", rowpos:2}
                   },
                   { name:'type',index:'tname', width:60, fixed: true, editable: false,
                   },
                   { name:'description', index:'description', width:220, fixed: true, editable: true, edittype:"textarea",
                     editoptions:{rows:5, wrap:"on", style:'width:500px'},
                     formoptions:{label: "description", rowpos:4},
                   },
                   { name:'dev',index:'dev', width:170, fixed: true, editable: true,
                     editoptions:{style:'width:250px'},
                     formoptions:{label: "interface", rowpos:5}
                   },
                   { name:'parameters',index:'parameters', width:135, fixed:false, editable: true, edittype:"textarea",
                     editoptions:{rows:5, wrap:"on", style:'width:500px'},
                     formoptions:{label: "parametres", rowpos:6}
                   },
                   { name:'state',index:'state', align:"center", width:50, fixed: true, editable: true, edittype:"select", formatter:'select',
                     editoptions:{value:{0:"disable",1:"enable",2:"delegate"}},
                     formoptions:{label: "etat", rowpos:7}
                   },
                   { name:'id_type',index:'id_type', width:40, editable: true, edittype:"select",
                     editoptions:{dataUrl:'models/get_type-jqg_select.php'},
                     editrules:{required:true, edithidden:true}, hidden: true,
                     formoptions:{ label: 'type', rowpos:3 }
        } ],
        editurl: '../models/set_interfaces-jqg_grid.php',
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
                          $("#table_interfaces_Editer").button('disable');
                          $("#table_interfaces_Supprimer").button('disable');
                      },
        onSelectRow:  function(){
                          $("#table_interfaces_Editer").button('enable');
                          $("#table_interfaces_Supprimer").button('enable');
                      }
});

    
    jQuery("#table_interfaces").jqGrid('navGrid',
        '#pager_interfaces',
        {search:false, edit:true, add:true, del:true, delfunc:delete_interface },
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {interfaces_inEdit = true;},  closeAfterEdit: true, afterShowForm: function(){ $("#name").focus() } }, // edit option
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {interfaces_inEdit = false;}, closeAfterAdd: true}, // add option
        {width:600}  // delete option
    );
    
    $('#t_table_interfaces').height(40);
    add_button_to_toolbar('table_interfaces','Ajouter',function(){_addRow('table_interfaces',function(){interfaces_inEdit = false;})},"ui-icon-plusthick");
    add_button_to_toolbar('table_interfaces','Editer',function(){_editRow('table_interfaces',function(){interfaces_inEdit = true;})},"ui-icon-pencil");
    add_button_to_toolbar('table_interfaces','Supprimer',function(){delete_interface();},"ui-icon-trash");
}
