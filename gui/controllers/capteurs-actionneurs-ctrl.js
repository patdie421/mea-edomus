function capteursactionneurs_controller(table_id)
{
   $('#'+table_id).datagrid({
      onDblClickRow: function(index,row){ // si double click on modifie
         alert(index+" "+field+" "+value);
         $('#'+table_id).selectRow(index);
         editSensorActuator(table_id, 'dlg', 'fm', 'datasource_url');
	    }
      onLoadSuccess: function(data){
         if(data.isError)
         {
            if(data.errno==99)
            {
               $.messager.alert("Erreur :","Vous n'êtes pas/plus connectés ...",'error', function(){window.location = "login.php?dest=index.html&page=page2.php&tab=Capteurs/Actionneurs";});
               return -1;
            }
            else
            {
               $.messager.alert("Erreur :","Message retourné par le serveur : "+data.errmsg,'error'});
               return -1;
            }
         }
      }
   });
}

function ajax_error(xhr, ajaxOptions, thrownError){
    alert("index.js : responseText="+xhr.responseText+" status="+xhr.status+" thrownError="+thrownError);
}


function check_field_exist(table,fieldName,fieldValue,id)
{
   if(id!=-1)
     wsdata={table:table, field:fieldName,value:fieldValue, id:id};
   else
     wsdata={table:table, field:fieldName, value:fieldValue};
    
   $.ajax({ url: 'models/check_field.php',
            async: false,
            type: 'GET',
            dataType: 'json',
            data: wsdata,
            success: function(data){
               if(data.exist==0)
                  passes=false;
               else
                  passes=true;
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
    $('#id').val(-1);

}


function editSensorActuator(grid_id, dlgbox_id, form_id, datasource_url_id)
{
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
      $.messager.confirm('Confirm','Etes vous sur de vouloir supprimer ce capteur/actionneur ?',function(r)
      {
         if (r)
         {
            $.post('models/delete_sensors_actuators.php',{id:row.id}, function(result) {
               if (!result.isError)
               {
                  $('#'+grid_id).datagrid('reload'); // reload the user data
               }
               else
               {
                  $.messager.show({ // show error message
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
            name_exist=check_field_exist("sensors_actuators","name",$('#sensor_acutator_name').val(),$('#id').val());
            if(name_exist)
            {
               // alert("Nom de capteur/actionneur déjà utilisé");
               $.messager.alert("Erreur :","Nom de capteur/actionneur déjà utilisé",'error'});
            }
            return name_exist;
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