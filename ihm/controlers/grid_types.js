function check_id_types(value,_name){
    passes=check_field_exist_by_id('table_types','types','id_type',value);
    return [passes, _name+" : numéro de type ("+value+") déjà utilisé "];
}


function check_name_types(value,_name){
    ret=check_name_string(value,_name);
    if(ret[0]==false)
        return ret;
    value=value.toUpperCase();
    var passes=check_field_exist_by_id('table_types','types','name',value);
    return [passes, _name+" : nom de type ("+value+") déjà utilisé "];
}


function id_types_DataInit(element){
    if(types_inEdit == true){
        $(element).attr("readonly", "readonly");
            $(element).css({color: "#b9b9b9", background: "#E0E0E0"});
    } else {
        $.get('../models/get_next_id_type-jqg_grid.php',
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
    var grid_id='table_types',
        myGrid = $("#"+grid_id),
        selRowId = myGrid.jqGrid ('getGridParam', 'selrow');
         
    if(selRowId){
        var id = myGrid.jqGrid ('getCell', selRowId, 'id');
            id_type = myGrid.jqGrid ('getCell', selRowId, 'id_type'),
            name = myGrid.jqGrid ('getCell', selRowId, 'name');
        if(id_type<2000){
            mea_alert("Suppression impossible !", "Les types standards ne peuvent pas être supprimés");
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
                        msg="<DIV style='padding-left:30px'>Le type "+name+" ne peut pas être supprimée car il est référencé par les objets suivants : </BR></BR>";
                        if(data.values1.length)
                            msg=msg+"Capteur(s)/actionneur(s) : "+data.values1+"</BR>";
                        if(data.values2.length)
                            msg=msg+"Interface(s) : "+data.values2+"</BR>";
                        msg=msg+"</BR>Modifiez d'abord les références.</DIV>";
                        mea_alert("Suppression impossible", msg);
                    } else {
                        mea_yesno("OK pour suppression", "Oui ou Non", db_delete_type, id);
                    }
                },
                error: ajax_error
            });
        }
    }
}


function grid_types(){
    jQuery("#table_types").jqGrid(
    { url:'../models/get_types-jqg_grid.php',
        datatype: "xml",
        colNames:['id','n°', 'nom', 'description','paramètres','flags'],
        colModel:[ { name:'id',index:'id', align:"center", width:10, hidden: true,
                   },
                   { name:'id_type',index:'id_type', align:"center", width:35, fixed: true, editable: true,
                     editoptions:{size: 5, dataInit: id_types_DataInit},
                     editrules:{required: true, integer: true, minValue:1, custom: true, custom_func: check_id_types},
                     formoptions:{label: "numero type", rowpos:1}
                   },
                   { name:'name',index:'name', width:100, fixed: true, editable: true,
                     editoptions:{dataInit: others_DataInit},
                     editrules:{required: true, custom: true, custom_func: check_name_types},
                     formoptions:{label: "nom type", rowpos:2}
                   },
                   { name:'description', index:'description', width:300, fixed: true, editable: true, edittype:"textarea",
                     editoptions:{rows:5, wrap:"on", style:'width:500px', dataInit: others_DataInit}, formoptions:{label: "description", rowpos:3},
                   },
                   { name:'parameters', index:'parameters', width:200, editable: true, edittype:"textarea",
                     editoptions:{rows:5, wrap:"on", style:'width:500px'},
                     formoptions:{label: "paramètres", rowpos:4}
                   },
                   { name:'flags',index:'flags', align:"center", width:10, editable: false, hidden: true,		
                   },
        ],
        editurl: '../models/set_types-jqg_grid.php',
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
            $("#table_types_Editer").button('disable');
            $("#table_types_Supprimer").button('disable');
        },
        onSelectRow: function(){
            myGrid = $("#table_types");
            selRowId = myGrid.jqGrid('getGridParam', 'selrow');
            id_type = myGrid.jqGrid('getCell', selRowId, 'id_type');
            $("#table_types_Editer").button('enable');
            if(id_type<2000)
            {
                $("#table_types_Supprimer").button('disable');
            }
            else
            {
                $("#table_types_Supprimer").button('enable');
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
        }
    });
    
    jQuery("#table_types").jqGrid('navGrid',
        '#pager_types',
        {search:false, edit:true, add:true, del:true, delfunc:grid_delete_type },
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {types_inEdit = true;},  closeAfterEdit: true, afterShowForm: table_types_set_focus},
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {types_inEdit = false;},  closeAfterAdd: true}
    );
    $('#t_table_types').height(40);

    add_button_to_toolbar('table_types','Ajouter',function(){_addRow('table_types',function(){types_inEdit = false;})},"ui-icon-plusthick");
    add_button_to_toolbar('table_types','Editer',function(){_editRow('table_types',function(){types_inEdit = true;}, table_types_set_focus)},"ui-icon-pencil");
    add_button_to_toolbar('table_types','Supprimer',function(){grid_delete_type();},"ui-icon-trash");
}
