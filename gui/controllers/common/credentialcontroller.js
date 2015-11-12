function CredentialController(dataSource)
{
   this.userid=null;
   this.profil=null;
   this.loggedIn=null;
   this.last=0;
   this.cacheTime=5; // 5 secondes entre deux appels
   this.dataSource=dataSource;
}


CredentialController.prototype.get = function()
{
   var _controller = this;
   var retour=false;
   var now = Date.now();
      
   if((now - 5*1000) > _controller.last) {
      $.ajax({
         url: _controller.dataSource,
         async: false,
         type: 'GET',
         dataType: 'json',
         data: {},
//         beforeSend: function(){
//         },
         success: function(data) {
            if(data.result=="OK") {
               _controller.userid=data.userid;
               _controller.profil=parseInt(data.profil);
               _controller.loggedIn=true;
               _controller.last=now;
               retour=true;
            }
         },
         error: function(jqXHR, textStatus, errorThrown){
            // afficher message d'alerte ici ?
            _controller.userid=-1;
            _controller.profil=-1;
            _controller.loggedIn=false;
            _controller.last=0;
            retour=false;
           console.log("Communication Error : "+textStatus+" ("+errorThrown+")");
         }
      });
   }
   else {
      if(this.loggedIn!=-1)
         retour=true;
      else
         retour=false;
   }
   return retour;
};

   
CredentialController.prototype.isAdmin = function()
{
  if(this.get()==true) {
      if(this.profil)
         return true;
      else
         return false;
  }
  else
    return false;
};


CredentialController.prototype.isLoggedIn = function() {
   return this.get();
};

   
CredentialController.prototype.__auth = function(_controller,methode)
{
   var jqxhr = $.get(this.dataSource,
                     {}, // pas de parametre
                     function(data) {
                        if(data.result=="OK"){
                           _controller[methode]();
                        }
                        else {
                           $.messager.alert(_controller._toLocalC('error')+_controller._localDoubleDot(),_controller.toLocalC('you are not connected')+' !', 'error', function(){ window.location = "login.php"; });
                        }
                     },
                     "json")
                     .done(function() {
                     })
                     .fail(function(jqXHR, textStatus, errorThrown) {
                         $.messager.show({
                            title:_controller._toLocalC('error')+_controller._localDoubleDot(),
                            msg: _controller._toLocalC('communication error')+'('+textStatus+')'
                         });
                     })
                     .always(function() {
                     });
};
