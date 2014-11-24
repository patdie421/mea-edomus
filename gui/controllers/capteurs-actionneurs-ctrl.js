function onError(errno)
{
   if(errno==99)
   {
      $.messager.alert("Erreur :","Vous n'êtes pas/plus connectés ...",'error', function() { window.location = "login.php?dest=index.php&page=page2.php&tab=Capteurs/Actionneurs";} );
      return -1;
   }
   else
   {
      $.messager.alert("Erreur :","Message retourné par le serveur : "+data.errmsg,'error');
      return -1;
   }
}


function capteursactionneurs_controller(table_id)
{
   $('#'+table_id).datagrid({
      onDblClickRow: function(index,row) { // si double click on modifie
         alert(index+" "+field+" "+value);
         $('#'+table_id).selectRow(index);
         editSensorActuator(table_id, 'dlg', 'fm', 'datasource_url');
	    },

      onLoadSuccess: function(data){
         if(data.isError)
         {
            onErro(data.errno);
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
               if(data.result=="OK")
               {
                  if(data.exist==0)
                     passes=false;
                  else
                     passes=true;
               }
               else
               {
                  onErro(data.error);
               }
            },
            error: ajax_error
   });
    
   return passes;
}


function newSensorActuator(grid_id, dlgbox_id, form_id, datasource_url_id)
{
   authdata=get_auth_data();
   if(authdata==false) {
      $.messager.alert("Erreur : ","Vous n'est plus connecté !",'error', function(){window.location = "login2.php?dest=index.php&page=page2.php&tab=Capteurs/Actionneurs";});
      return false;
   } else {
      if(authdata.profil!=1) {
         isadmin=0;
      } else {
         isadmin=1;
      }
   }

   $('#'+dlgbox_id).dialog('open').dialog('setTitle','Création ...');
   $('#'+form_id).form('clear');
   $('#'+datasource_url_id).val("models/new_sensors_actuators.php");
   $('#database_id').val(-1);
}


function editSensorActuator(grid_id, dlgbox_id, form_id, datasource_url_id)
{
   authdata=get_auth_data();
   if(authdata==false) {
      $.messager.alert("Erreur : ","Vous n'est plus connecté !",'error', function(){window.location = "login2.php?dest=index.php&page=page2.php&tab=Capteurs/Actionneurs";});
      return false;
   } else {
      if(authdata.profil!=1) {
         isadmin=0;
      } else {
         isadmin=1;
      }
   }

   var row = $('#'+grid_id).datagrid('getSelected');
   if (row){
      $('#'+dlgbox_id).dialog('open').dialog('setTitle','Modification ...');
      $('#'+form_id).form('load',row);
      $('#'+datasource_url_id).val("models/update_sensors_actuators.php");
   }
}


function destroySensorActuator(grid_id)
{
   authdata=get_auth_data();
   if(authdata==false) {
      $.messager.alert("Erreur : ","Vous n'est plus connecté !",'error', function(){window.location = "login2.php?dest=index.php&page=page2.php&tab=Capteurs/Actionneurs";});
      return false;
   } else {
      if(authdata.profil!=1) {
         isadmin=0;
      } else {
         isadmin=1;
      }
   }

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
   
    $('#'+form_id).form('submit',{
        url: $('#'+datasource_url_id).val(),
        onSubmit: function() {
            var name_exist=-1;
            name_exist=check_field_exist("sensors_actuators","name",$('#sensor_acutator_name').val(),$('#database_id').val());
            
            if(name_exist)
            {
               // alert("Nom de capteur/actionneur déjà utilisé");
               $.messager.alert("Erreur :","Nom de capteur/actionneur déjà utilisé",'error');
               return false;
            }
            else
               return true;
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
