var meaWidgetsJar = {};

var meaFormaters = {};

meaFormaters["boolean"] = function(x) { if(x > 0) return "true"; else return "false"; };
meaFormaters["highlow"] = function(x) { if(x > 0) return "high"; else return "low"; };
meaFormaters["float"]   = function(x) {
   if(isNaN(x)===true)
      return x;

   try {
      var ret=x.toFixed(2);
      return ret;
   }
   catch(e) {
      return false;
   }
};

function MeaWidget(name, groupe, type)
{
   this.type="undefined";
   this.params = {};
   this.params["variables"] = {};
   this.params["values"] = {};
   this.params["labels"] = {};
   this.params["actions"] = {};
   this.params["parameters"] = {};
   this.params["positions"] = {};
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

   getStyle: function()
   {
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
      var v     = false;
      $.each(data, function(i, val) {
         if(val.name == n) {
            v = val.value;
            found = true;
            return false;
         }
      });
      if(found !== false)
         return v;
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
                     xplmsgdata[_i]="'"+_this.getValue(data, v, v)+"'"; 
                  }
                  else
                  {
                     xplmsgdata[_i]="'"+v+"'"; 
                  }
               });
               _this.xplsend(JSON.stringify(xplmsgdata));
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
   },

   xplsend: function(msg) {
   var _this = this;

      $.post("/CMD/xplsend.php", { msg: msg }, function(response) {
         if(response.iserror===false)
         {
         }
         else
         {
         }
      }).done(function() {
      }).fail(function(jqXHR, textStatus, errorThrown) {
         $.messager.show({
            title:'error: ',
            msg: "communication error"+' ('+textStatus+')'
         });
      });
   }
};
