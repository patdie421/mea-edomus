var utilisateurs_inEdit;


function id_utilisateur_DataInit(element){
    _DataInit(element,utilisateurs_inEdit,'models/get_next_id_user-jqg_grid.php');
}


function delete_utilisateur() {
    if(!check_auth())
        return false;
    var grid_id='table_utilisateurs',
        myGrid = $("#"+grid_id),
        selRowId = myGrid.jqGrid ('getGridParam', 'selrow');
         
    if(selRowId){
        var id = myGrid.jqGrid ('getCell', selRowId, 'id');
        var username = myGrid.jqGrid ('getCell', selRowId, 'name');
        if(username==userid){
            mea_alert2(str_warning.capitalize()+str_double_dot,
                str_warning_delete,
                function(){
                    mea_yesno2(str_title_del, str_del_user.capitalize(), db_delete_utilisateur, {id:id, userid:username});
                }
            );
        }
        else
            mea_yesno2(str_title_del, str_del_user.capitalize(), db_delete_utilisateur, {id:id, userid:username});
    }
}


function db_delete_utilisateur(params){
    db_delete('table_utilisateurs','users',params.id);
    deconnect_user(params.userid);
    if(!check_auth())
        return false;
}


function check_name_utilisateur(value,_name){
    ret=check_name_string(value,_name);
    if(ret[0]==false)
        return ret;
    var passes=check_field_exist_by_id('table_utilisateurs','users','name',value);

    return [passes, _name+str_double_dot+str_name+" "+str_user+" ("+value+") "+str_allready_used];
}


function check_id_utilisateur(value,_name){
    passes=check_field_exist_by_id('table_utilisateurs','users','id_user',value);

    return [passes, _name+str_double_dot+str_num+" "+str_user+" ("+value+") "+str_allready_used];
}


function check_profil(){
    alert("OK");
}


function grid_utilisateurs(){
    jQuery("#table_utilisateurs").jqGrid({
        url:'models/get_utilisateurs-jqg_grid.php',
        datatype: "xml",
        colNames:['id',str_num.capitalize(), str_name.capitalize(), 'mdp', str_description.capitalize(), str_profile.capitalize(), 'mot de passe'],
        colModel:[ {name:'id',index:'id', align:"center", width:10, hidden: true
                   },
                   {name:'id_user',index:'id_user', align:"center", width:35, fixed: true, editable: true,
                        editoptions:{size: 5, dataInit: id_utilisateur_DataInit},
                        editrules:{required: true, integer: true, minValue:1, custom: true, custom_func: check_id_utilisateur},
                        formoptions:{label: str_num.capitalize(), rowpos:1}
                   },
                   {name:'name',index:'name', width:200, fixed: true, editable: true,
                        editrules:{required: true, custom: true, custom_func: check_name_utilisateur},
                        formoptions:{label: str_ident.capitalize(), rowpos:2}
                   },
                   {name:'password',index:'password', width:200, fixed: true, hidden: true, editable: true, edittype:"password",
                        editrules:{edithidden:true},
                        formoptions:{label: str_passwd.capitalize(), rowpos:3}
                   },
                   {name:'description', index:'description', width:200, editable: true, edittype:"textarea",
                        editoptions:{rows:5, wrap:"on", style:'width:500px'},
                        formoptions:{label: str_description.capitalize(), rowpos:4}
                   },
                   { name:'profil',index:'profil', align:"center", width:50, fixed: true, editable: true, edittype:"select", formatter:'select',
                        editoptions:{value:{0:"user",1:"admin"}},
                        formoptions:{label: str_profile.capitalize(), rowpos:5}
                   },
                   {name:'flag',index:'flag', align:"center", width:10, fixed: true, hidden: true, editable: true, edittype:"select", formatter:'select',
                        editoptions:{value:{0:str_chg_non_needed,1:str_chg_forced,2:str_chg_disalowed}},
                        editrules:{edithidden:true},
                        formoptions:{label: str_connection_action.capitalize(), rowpos:6}
                   },
        ],
        editurl: 'models/set_utilisateurs-jqg_grid.php',
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
            $("#table_utilisateurs_edit").button('disable');
            $("#table_utilisateurs_del").button('disable');
        },
        onSelectRow: function(){
            $("#table_utilisateurs_edit").button('enable');
            $("#table_utilisateurs_del").button('enable');
        },
        loadError: function(xhr,st,err) { 
            if(st=="parsererror") {
                data = JSON.parse(xhr.responseText);
                if(data.error==99) {
                    mea_alert2(str_error.capitalize()+str_double_dot, str_not_connected, function(){window.location = "login.php";} );
                    return false;
                }
                if(authdata.profil==98) {
                    mea_alert2(str_error.capitalize()+str_double_dot, str_not_allowed, function(){window.location = "index.php";} );
                    return false;
                }
                alert(str_unknow_error.capitalize()+data.error_msg);
                return false;
            }
            else
                alert("Type: "+st+"; Response: "+ xhr.status + " "+xhr.statusText+" "+xhr.responseText);
        },
    });

    jQuery("#table_utilisateurs").jqGrid('navGrid',
        "#pager_utilisateurs",
        {search:false, edit:true, add:true, del:true, delfunc:delete_utilisateur, afterSubmit: check_profil },
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {utilisateurs_inEdit = true; return check_auth(); },
                    closeAfterEdit: true, afterShowForm: function(){ $("#name").focus()}, afterSubmit: function(){check_profil();} }, // edit option
        {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {utilisateurs_inEdit = false; return check_auth(); },
                    closeAfterAdd: true }, // add option
        {width:600}  // delete option
    );
    $("#t_table_utilisateurs").height(40);
    add_button_to_toolbar("table_utilisateurs","add",str_add.capitalize(),function(){ if(check_auth()){_addRow("table_utilisateurs", function(){utilisateurs_inEdit = false; })} },"ui-icon-plusthick");
    add_button_to_toolbar("table_utilisateurs","edit",str_edit.capitalize(),function(){ if(check_auth()){_editRow("table_utilisateurs", function(){utilisateurs_inEdit = true; })} },"ui-icon-pencil");
    add_button_to_toolbar("table_utilisateurs","del",str_del.capitalize(),function(){ delete_utilisateur(); },"ui-icon-trash");
}
