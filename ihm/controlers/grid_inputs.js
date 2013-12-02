var inputs_inEdit;

function grid_inputs() {
   jQuery("#table_inputs").jqGrid({
      url:'models/get_inputs-jqg_grid.php',
      datatype: "xml",
      colNames:['id','name', 'xplsource', 'xplschema', 'conditionslist','inputindex'],
      colModel:[ { name:'id',index:'id', hidden: true },
                 { name:'name',index:'name', editable: true },
                 { name:'xplsource',index:'xplsource', fixed: true, editable: true},
                 { name:'xplschema',index:'xplschema', fixed: true, editable: true},
                 { name:'conditionslist',index:'conditionslist', fixed: true, editable: true},
                 { name:'inputindex',index:'inputindex', fixed: true, editable: true}
      ],
      editurl: 'models/set_inputs-jqg_grid.php',
      rowNum:20,
      rowList:[20,50,100],
      pager: '#pager_inputs',
      sortname: 'name',
      viewrecords: true,
      height:360,
      scrollOffset: 0,
      autowidth: true,
      hidegrid: false,
      caption:"",
      toolbar: [true,"top"],
      gridComplete: function(){
         $("#table_inputs_edit").button('disable');
         $("#table_inputs_del").button('disable');
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
            if(authdata.profil==98) {
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
    
   $("#table_inputs").jqGrid('navGrid',
      '#pager_inputs',
      {search:false, edit:true, add:true, del:true },
      {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {inputs_inEdit = true; return check_auth(); },  closeAfterEdit: true, afterShowForm: function(){ $("#name").focus() } }, // edit option
      {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {interfaces_inEdit = false;return check_auth(); }, closeAfterAdd: true}, // add option
      {width:600}  // delete option
    );
    
   $('#t_table_inputs').height(40);
   add_button_to_toolbar('table_inputs',"add",str_add.capitalize(),function(){if(check_auth()){_addRow('table_inputs',function(){inputs_inEdit = false;})}},"ui-icon-plusthick");
   add_button_to_toolbar('table_inputs',"edit",str_edit.capitalize(),function(){if(check_auth()){_editRow('table_inputs',function(){inputs_inEdit = true;})}},"ui-icon-pencil");
   add_button_to_toolbar('table_inputs',"del",str_del.capitalize(),function(){delete_input();},"ui-icon-trash");
   $("#table_inputs_edit").button('disable');
   $("#table_inputs_del").button('disable');
}
