extendClass = function(subClass, baseClass) {
   function __inheritance() {}
   __inheritance.prototype = baseClass.prototype;
   subClass.prototype = new __inheritance();
   subClass.prototype.constructor = subClass;
   subClass.superConstructor = baseClass;
   subClass.superClass = baseClass.prototype;
}

// voir aussi pour les class : http://www.42hacks.com/notes/fr/20111213-comment-ecrire-du-code-oriente-objet-propre-et-maintenable-en-javascript/

function CommonController()
{
  this.loginUrl = "login.php"; // à mettre dans credential controller ?
}


CommonController.prototype = {

// from translation controller
   _toLocal: function(string)
   {
      return string.toLowerCase();
   },
   
   _toLocalC: function(string)
   {
      str=this._toLocal(string);
      return str.charAt(0).toUpperCase() + str.slice(1).toLowerCase();
   },
   
   _localDoubleDot: function()
   {
      return ": ";
   },
   
   linkToTranslationController: function(translationController)
   {
      var _toLocal = null;
      try
      {
         _toLocal = translationController.toLocal;
         this._toLocal = _toLocal.bind(translationController);
      }
      catch(e)
      {
         console.log("probably bad object");
      }
      
      var _localDoubleDot = null;
      try
      {
         _localDoubleDot = translationController.localDoubleDot;
         this._localDoubleDot = _localDoubleDot.bind(translationController);
      }
      catch(e)
      {
         console.log("probably bad object");
      }
   },


// from credentials controller
   _isLoggedIn: function()
   {
      return true;
   },
   
   _isAdmin: function()
   {
      return true;
   },
   
   __auth: function(_controller,methode)
   {
      _controller[methode]();
   },

   linkToCredentialController: function(_credentialController)
   {
      var _isLoggedIn = null;
      try
      {
         _isLoggedIn = _credentialController.isLoggedIn;
         this._isLoggedIn = _isLoggedIn.bind(_credentialController);
      }
      catch(e)
      {
         console.log("probably bad object");
      }
      
      var _isAdmin = null;
      try
      {
         _isAdmin = _credentialController.isAdmin;
         this._isAdmin = _isAdmin.bind(_credentialController);
      }
      catch(e)
      {
         console.log("probably bad object");
      }
      
      var __auth = null;
      try
      {
         __auth = _credentialController.__auth;
         this.__auth = __auth.bind(_credentialController);
      }
      catch(e)
      {
         console.log("probably bad object");
      }
   },

   errorAlert: function(title, msg)
   {
      $.messager.alert(title, msg, 'error');
   }
};

