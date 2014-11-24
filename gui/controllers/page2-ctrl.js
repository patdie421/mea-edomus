var isadmin=-1;
var _isauth=-1;

function page2_controller(tabName)
{
   authdata=get_auth_data();
   if(authdata==false) {
      $.messager.alert("Erreur : ","Vous n'est plus connecté !",'error', function(){window.location = "login2.php?dest=index.html&page=page2.php&tab="+tabName;});
      return false;
   } else {
      if(authdata.profil!=1) {
         isadmin=0;
      } else {
         isadmin=1;
      }
   }
 
   $('#tabConfiguration').tabs({
      onSelect:function(tabName) {
         if(_isauth!=-1) // pour pas avoir de auth de suite après chargement de la page
         {
            authdata=get_auth_data();
            if(authdata==false) {
               $.messager.alert("Erreur : ","Vous n'est plus connecté !",'error', function(){window.location = "login2.php?dest=index.html&page=page2.php&tab="+tabName;});
               return false;
            }
            _isauth=0;
         }
      }
   });

   $('#tabConfiguration').tabs('select', tabName);
}

