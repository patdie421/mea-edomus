var inputs_inEdit;

function grid_inputs() {

   init_input_form();
   
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
         $("#table_inputs_edit_nav").addClass('ui-state-disabled');
         $("#table_inputs_del_nav").addClass('ui-state-disabled');
      },
      onSelectRow: function(){
         $("#table_inputs_edit").button('enable');
         $("#table_inputs_del").button('enable');
         $("#table_inputs_edit_nav").removeClass('ui-state-disabled');
         $("#table_inputs_del_nav").removeClass('ui-state-disabled');
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
    
   $("#table_inputs").jqGrid('navGrid',
      '#pager_inputs',
      {search:false, edit:false, add:false, del:false }
/*
      ,{width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {inputs_inEdit = true; return check_auth(); },  closeAfterEdit: true, afterShowForm: function(){ $("#name").focus() } }, // edit option
      {width:700, recreateForm: true, viewPagerButtons: false, beforeInitData: function () {interfaces_inEdit = false;return check_auth(); }, closeAfterAdd: true}, // add option
      {width:600}  // delete option
*/
   )
   .navButtonAdd('#pager_inputs',
      {  caption:"",
         id:"table_inputs_del_nav",
         buttonicon:"ui-icon-trash",
         onClickButton: function(){
            alert("Deleting Row"); },
      position:"first"
   })
   .navButtonAdd('#pager_inputs',
      {  id:"table_inputs_edit_nav",
         caption:"",
         buttonicon:"ui-icon-pencil",
         onClickButton: function(){
         alert("Editing Row"); },
      position:"first"
   })
   .navButtonAdd('#pager_inputs',
      {  caption:"",
         buttonicon:"ui-icon-plus",
         onClickButton: function(){
         open_input_form(); },
      position:"first"
   })
   ;
    
   $('#t_table_inputs').height(40);
   add_button_to_toolbar('table_inputs',"add",str_add.capitalize(),function(){if(check_auth()){_addRow('table_inputs',function(){inputs_inEdit = false;})}},"ui-icon-plus");
   add_button_to_toolbar('table_inputs',"edit",str_edit.capitalize(),function(){if(check_auth()){_editRow('table_inputs',function(){inputs_inEdit = true;})}},"ui-icon-pencil");
   add_button_to_toolbar('table_inputs',"del",str_del.capitalize(),function(){delete_input();},"ui-icon-trash");
   /*
   $("#table_inputs_edit").button('disable');
   $("#table_inputs_del").button('disable');
   $("#table_inputs_edit_nav").addClass('ui-state-disabled');
   $("#table_inputs_del_nav").addClass('ui-state-disabled');
   */
}

function updateTips( t ) {
   tips
   .text( t )
   .addClass( "ui-state-highlight" );
   setTimeout(function() {
      tips.removeClass( "ui-state-highlight", 1500 );
   }, 500 );
}

function checkLength( o, n, min, max ) {
   if ( o.val().length > max || o.val().length < min ) {
      o.addClass( "ui-state-error" );
      updateTips( "Length of " + n + " must be between " +
      min + " and " + max + "." );
      return false;
   } else {
      return true;
   }
}
   
function checkRegexp( o, regexp, n ) {
   v = o.val();
   $.trim(v);
   if ( !( regexp.test( v ) ) ) {
      o.addClass( "ui-state-error" );
      updateTips( n );
      return false;
   } else {
      return true;
   }
}

function disable_keyvalue(event) {
   $("#key2").attr("readonly", "readonly");
   $("#key2").addClass( "mea_disabled" );
}

function enable_keyvalue(event) {
   $("#key2").removeAttr("readonly");
   $("#key2").removeClass( "mea_disabled" );
   $("#key2").focus();
}

function init_input_form() {

   var tips = $( "#tips" ),
       _key = $( "#key" ),
       _value = $( "#value" ),
       _xplsource = $("#xplsource"),
       _xplschema = $("#xplschema"),
       _inputindex = $("#inputindex"),
       _key2 = $("#key2");
       
       allFields = $( [] ).add( _key ).add( _value ).add( _xplsource ).add( _xplschema ).add( _inputindex ).add( _key2 );

//    $.ajaxSetup({ cache: false });
   
   $("#true").click(disable_keyvalue);
   $("#false").click(disable_keyvalue);
   $("#keyvalue").click(enable_keyvalue);
   
   $("#selectable").selectable();
   $("#del_condition").button().click(function( event ) { event.preventDefault(); $(".ui-selected","#selectable").remove(); });
   $("#add_condition").button().click(function( event ) {
      event.preventDefault();
      
      key=_key.val();
      value=_value.val();
      $.trim(key);
      $.trim(value);
      
      _key.removeClass( "ui-state-error" );
      _value.removeClass( "ui-state-error" );
      
      if(key=="") {
         _key.addClass( "ui-state-error" );
      }
      if(value=="") {
         _value.addClass( "ui-state-error" );
      }
      if(key!="" && value!="") {
         b = checkRegexp( _key, /^[0-9a-z_]{1,16}$/, "'Value for key' may consist of a-z, 0-9 or underscores." );
         if(b) {
            if(($("#op").val() == '=') || ($("#op").val() == '!=') ) {
               if(!$.isNumeric( _value.val() )) {
                  value="\""+value+"\"";
               }
            }
            else {
               if(!$.isNumeric( _value.val() )) {
                  _value.addClass( "ui-state-error" );
                  updateTips("With '>', '>=', '<' and '<=' operators, 'A value' must be numeric.");
                  b = false;
               }
            }
         }
         if(b) {
            $("#key").val("");
            $("#value").val("");
            keyvalue=key+" "+$("#op").val()+" "+value;
            x="<li>"+keyvalue+"</li>";
            $("#selectable").append(x);
         }
      }
      else {
         updateTips("'Value for key' and 'A value' can't be empty !");
      }
      
   });
 
   $( "#dialog-form" ).dialog({
      autoOpen: false,
      height: 600,
      width: 631,
      modal: true,
      resizable: false,
      buttons: {
         "Add": function() {
            allFields.removeClass( "ui-state-error" );
            b = checkRegexp( _xplsource, /^[0-9a-z]{1,8}-[0-9a-z]{1,8}\.[0-9a-z\-]{1,16}$/, "xlp address format error (VendorID-DeviceID.InstanceID)" );
            if(!b)
               return;
            b = checkRegexp( _xplschema, /^[0-9a-z]{1,8}\.[0-9a-z]{1,8}$/, "xlp schema format error ()" );
            if(!b)
               return;
            b = checkRegexp( _inputindex, /^[0-9]+$/, "must be numeric (0 <= input index <= 255)" );
            if(!b)
               return;
            v = parseInt($.trim(_inputindex.val()));
            if(v > 255) {
               updateTips("must be numeric (0 <= input index <= 255)");
               _inputindex.addClass( "ui-state-error" );
               return;
            }
            if($("#selectable").children().length == 0) {
               updateTips("At least one condition is needed");
               return;
            }
            if($("#keyvalue:checked").val()==3) {
               b = checkRegexp( _key2, /^[0-9a-z_]{1,16}$/, "'Value from body key' may consist of a-z, 0-9 or underscores." );
               if(!b)
                  return;
            }
            
            // recolter les données et le transmettre pour mise à jour
            
            $( this ).dialog( "close" );
         },
         Cancel: function() {
            $( this ).dialog( "close" );
         }
      },
      close: function() {
         // allFields.val( "" ).removeClass( "ui-state-error" );
      }
   });
}

function open_input_form() {
   allFields.removeClass( "ui-state-error" ).val( "" ).on( "focus", function() { $(this).removeClass( "ui-state-error" ); } );
   disable_keyvalue();
   $("#true").attr('checked','checked');
   $("#selectable li").remove();

   var myGrid = $("#table_inputs"),
   selRowId = myGrid.jqGrid ('getGridParam', 'selrow'),
   celValue = myGrid.jqGrid ('getCell', selRowId, 'name');

   $("#xplsource").val(celValue);
   $( "#dialog-form" ).dialog( "open" );
}
