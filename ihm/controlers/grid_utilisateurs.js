var utilisateurs_inEdit;


function id_utilisateur_DataInit(element){
    _DataInit(element,utilisateurs_inEdit,'../models/get_next_id_user-jqg_grid.php');
}


function delete_utilisateur(){
    var grid_id='table_utilisateurs',
        myGrid = $("#"+grid_id),
        selRowId = myGrid.jqGrid ('getGridParam', 'selrow');
         
    if(selRowId){
        var id = myGrid.jqGrid ('getCell', selRowId, 'id');
            id_location = myGrid.jqGrid ('getCell', selRowId, 'id_location'),
            name = myGrid.jqGrid ('getCell', selRowId, 'name');
                
            mea_yesno("OK pour suppression", "Oui ou Non", db_delete_utilisateur, id);
    }
}


function db_delete_utilisateur(id){
    db_delete('table_utilisateurs','users',id);
}


function check_name_utilisateur(value,_name){
    ret=check_name_string(value,_name);
    if(ret[0]==false)
        return ret;
    value=value.toUpperCase();
    var passes=check_field_exist_by_id('table_utilisateurs','users','name',value);
    return [passes, _name+" : nom d'utilisateur ("+value+") déjà utilisé "];
}


function check_id_utilisateur(value,_name){
    passes=check_field_exist_by_id('table_utilisateurs','users','id_user',value);
    return [passes, _name+" : numéro d'utilisateur ("+value+") déjà utilisé "];
}


function grid_utilisateurs(){
    jQuery("#table_utilisateurs").jqGrid(
    { url:'../models/get_utilisateurs-jqg_grid.php',
        datatype: "xml",
        colNames:['id','n°', 'nom', 'mdp', 'description', 'profil', 'flag'],
        colModel:[ {name:'id',index:'id', align:"center", width:10, hidden: true
                   },
                   {name:'id_user',index:'id_user', align:"center", width:35, fixed: true, editable: true,
                        editoptions:{size: 5, dataInit: id_utilisateur_DataInit},
                        editrules:{required: true, integer: true, minValue:1, custom: true, custom_func: check_id_utilisateur},
                        formoptions:{label: "identifiation utilisateur", rowpos:1}
                   },
                   {name:'name',index:'name', width:200, fixed: true, editable: true,
                        editrules:{required: true, custom: true, custom_func: check_name_utilisateur},
                        formoptions:{label: "identifiant", rowpos:2}
                   },
                   {name:'password',index:'password', width:200, fixed: true, hidden: true, editable: true, edittype:"password",
                        editrules:{edithidden:true},
                        formoptions:{label: "mot de passe", rowpos:3}
                   },
                   {name:'description', index:'description', width:200, editable: true, edittype:"textarea",
                        editoptions:{rows:5, wrap:"on", style:'width:500px'},
                        formoptions:{label: "description", rowpos:4}
                   },
                   { name:'profil',index:'profil', align:"center", width:50, fixed: true, editable: true, edittype:"select", formatter:'select',
                        editoptions:{value:{0:"user",1:"admin"}},
                        formoptions:{label: "profil", rowpos:5}
                   },
                   {name:'flag',index:'flag', align:"center", width:10, fixed: true, hidden: true, editable: true, edittype:"select", formatter:'select',
                        editoptions:{value:{0:"password change not need",1:"force password change at login",2:"can't change password"}},
                        editrules:{edithidden:true},
                   },

        ],
        editurl: '../models/set_utilisateurs-jqg_grid.php',
        rowNum:20,
        rowList:[20,50,100],
        pager: '#pager_utilisateurs',
        sortname: 'id_user',
        viewrecords: true,
        height:360,
        scrollOffset: 0,
        autowidth: true,
        hidegrid: false,
        caption:"",
        toolbar: [true,"top"],
        gridComplete: function(){
            $("#table_utilisateurs_Editer").button('disable');
            $("#table_utilisateurs_Supprimer").button('disable');
        },
        onSelectRow: function(){
            $("#table_utilisateurs_Editer").button('enable');
            $("#table_utilisateurs_Supprimer").button('enable');
        },
    });
        
    jQuery("#table_utilisateurs").jqGrid('navGrid',
        "#pager_utilisateurs",
        {search:false, edit:true, add:true, del:true, delfunc:delete_utilisateur },
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {utilisateurs_inEdit = true;},  closeAfterEdit: true, afterShowForm: function(){ $("#name").focus()} }, // edit option
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {utilisateurs_inEdit = false;}, closeAfterAdd: true}, // add option
        {width:600}  // delete option
    );
    $("#t_table_utilisateurs").height(40);
    add_button_to_toolbar("table_utilisateurs",'Ajouter',function(){_addRow("table_utilisateurs",function(){utilisateurs_inEdit = false;})},"ui-icon-plusthick");
    add_button_to_toolbar("table_utilisateurs",'Editer',function(){_editRow("table_utilisateurs",function(){utilisateurs_inEdit = true;})},"ui-icon-pencil");
    add_button_to_toolbar("table_utilisateurs",'Supprimer',function(){delete_utilisateur();},"ui-icon-trash");
}
