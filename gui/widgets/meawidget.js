var meaWidgetsJar = {};

var meaFormaters = {};
meaFormaters["boolean"] = function(x) { if(x > 0) return "true"; else return "false"; };
meaFormaters["highlow"] = function(x) { if(x > 0) return "high"; else return "low"; };
meaFormaters["float"]   = function(x) { return x.toFixed(2) ; };

function MeaWidget()
{
}


MeaWidget.prototype = {
   init: function(id) {
   },

   disabled: function(id) {
   },

   getHtmlIcon: function() {
      return false;
   },

   getHtml: function() {
      return false;
   },

   getIdent: function() {
      return this.ident;
   },

   getType: function() {
      return this.type;
   },

   getGroup: function() {
      return this.group;
   },

   setValue: function(data, n, v) {
      $.each(data, function(i, val)
      {
         if(val.name == n) {
            val.value = v;
            return false;
         }
     });
   },

   getValue: function(data, n, notfound) {
      var found = false;

      $.each(data, function(i, val) {
         if(val.name == n) {
            found = val.value;
            return false;
         }
      });
      if(found !== false)
         return found;
      else
         return notfound;
   },

   doAction: function(action, data) {
      console.log(action+" "+JSON.stringify(data));
      var _this = this;
      var _actions = _this.getValue(data, action, false);

      if(_actions == false)
         return false;
      _actions = JSON.parse(_actions);

      $.each(_actions, function(i, val) {
         switch(i)
         {
            case "xplsend":
               var xplmsgdata = {};
               $.each(val, function(_i, _val) {
                  var v=_val.trim();
                  if(v.charAt(0)=='[' && v.charAt(v.length - 1)==']') {
                     v = v.substring(1, v.length - 1);
                     xplmsgdata[_i]=_this.getValue(data, v, v); 
                  }
                  else
                     xplmsgdata[_i]=v; 

               });
               console.log("XPLSEND: "+JSON.stringify(xplmsgdata));
               return true;
               break;
         }
      });
      return false;
   },

   toWidgetsJarAs: function(ident, group, type) {

      this.ident=ident; 
      this.group=group;
      this.type=type;

//      meaWidgetsJar[this.ident]=this;
      meaWidgetsJar[this.type]=this;
   }
};
