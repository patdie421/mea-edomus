var isadmin=-1;

function page2_controller(tabName)
{
   authdata=get_auth_data();
   if(authdata==false) {
      $.messager.alert(str_Error+str_double_dot,str_not_connected,'error', function(){window.location = "login.php?dest=index.html&page=page2.php&tab="+tabName;});
      return false;
   } else {
      if(authdata.profil!=1) {
         isadmin=0;
      } else {
         isadmin=1;
      }
      $('#tabConfiguration').tabs('select', tabName);
   }
}

