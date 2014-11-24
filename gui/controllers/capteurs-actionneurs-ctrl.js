function capteursactionneurs_controller(table_id)
{
/*   
   $('#'+table_id).edatagrid({
      url:        'models/get_sensorsactuators-jqg_grid.php',
      saveUrl:    'save_user.php',
      updateUrl:  'update_user.php',
      destroyUrl: 'destroy_user.php',
      onError:    function(index,row) { alert(row.msg); } // uniquement pour edatagrid ?
   });
*/
}

function ajax_error(xhr, ajaxOptions, thrownError){
    alert("index.js : responseText="+xhr.responseText+" status="+xhr.status+" thrownError="+thrownError);
}


function check_field_exist(table,fieldName,fieldValue,id)
{
   if(id)
     wsdata={table:table, field:fieldName,value:fieldValue, id:id};
   else
     wsdata={table:table, field:fieldName, value:fieldValue};
    
   $.ajax({ url: 'models/check_field-jqg_grid.php',
            async: false,
            type: 'GET',
            dataType: 'json',
            data: wsdata,
            success: function(data){
               if(data.exist==0)
                  passes=true;
               else
                  passes=false;
            },
            error: ajax_error
   });
    
   return passes;
}


function newSensorActuator(grid_id, dlgbox_id, form_id, datasource_url_id)
{
    $('#'+dlgbox_id).dialog('open').dialog('setTitle','Création ...');
    $('#'+form_id).form('clear');
    $('#'+datasource_url_id).val("models/new_sensors_actuators.php");
}


function editSensorActuator(grid_id, dlgbox_id, form_id, datasource_url_id)
{
   console.log(datasource_url_id);
   $('#'+datasource_url_id).val("models/update_sensors_actuators.php");
   var row = $('#'+grid_id).datagrid('getSelected');
   if (row){
      $('#'+dlgbox_id).dialog('open').dialog('setTitle','Modification ...');
      $('#'+form_id).form('load',row);
   }
}

function destroySensorActuator(grid_id)
{
   var row = $('#'+grid_id).datagrid('getSelected');
   if (row)
   {
      $.messager.confirm('Confirm','Are you sure you want to destroy this user?',function(r)
      {
         if (r)
         {
            $.post('models/delete_sensors_actuators.php',{id:row.id}, function(result) {
               if (!result.isError)
               {
                  $('#'+grid_id).datagrid('reload');    // reload the user data
               }
               else
               {
                  $.messager.show({    // show error message
                     title: 'Error',
                     msg: result.errorMsg
                  });
               }
            },'json');
         }
      });
   }
}


function updateSensorsActuators(table_id, dlgbox_id, form_id, datasource_url_id)
{
    console.log($('#'+datasource_url_id).val());
    $('#'+form_id).form('submit',{
        url: $('#'+datasource_url_id).val(),
        onSubmit: function() {
            var name_exist=-1;
            var id=$("#id").val();
            if(id!="")
               name_exist=check_field_exist("sensors_actuators","name",$('#sensor_acutator_name').val(),id);            
            else
               name_exist=check_field_exist("sensors_actuators","name",$('#sensor_acutator_name').val());
            if(!name_exist)
               alert("Name allready exist");
            return !name_exist;
        },
        success: function(result) {
            var result = eval('('+result+')');
            if (result.isError>0) {
                $.messager.show({
                    title: 'Error',
                    msg: result.msgError
                });
            } else {
                $('#'+dlgbox_id).dialog('close');        // close the dialog
                $('#'+table_id).datagrid('reload');    // reload the user data
            }
        }
    });
}