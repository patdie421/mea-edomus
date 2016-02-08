var meaWidgetsJar = {};

var meaFormaters = {};
meaFormaters["boolean"] = function(x) { if(x > 0) return "true"; else return "false"; };
meaFormaters["highlow"] = function(x) { if(x > 0) return "high"; else return "low"; };
meaFormaters["float"]   = function(x) { return x.toFixed(2) ; };

function MeaWidget()
{
   this.type="undefined";
}


MeaWidget.prototype = {
   init: function(id) {
   },

   update: function(id, row) {
   },

   disabled: function(id) {
   },

   getHtmlIcon: function() {
      var _this = this;
      var html="<div id='"+_this.type+"' class='drag' style='width: 50px; height: 50px; border:1px solid red; background-color: #FFFFFF;'></div>";

      return html;
   },

   getTip: function() {
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

   getFormaters: function() {
      return {};
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
