extendClass(MeaWidget_switch, MeaWidget);

var meaWidget_button = new MeaWidget_switch("MeaWidget_switch", "basic", "switch");

function MeaWidget_switch(name, group, type)
{
   MeaWidget_switch.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);
   this.switchName = "switch";

   this.params["variables"][this.switchName+"_value"]=true;
   this.params["values"]["value"]=true;
   this.params["actions"][this.switchName] = '';
}


MeaWidget_switch.prototype.disabled = function(id, d)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   widget.find('input[mea_widgetswitchname="'+_this.switchName+'"]').switchbutton({
      disabled: d
   });
   if(d===true)
   {
      widget.removeClass("mea_switch_enabled");
      widget.addClass("mea_switch_disabled");
   }
   else
   {
      widget.removeClass("mea_switch_disabled");
      widget.addClass("mea_switch_enabled");
   }
}


MeaWidget_switch.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');
   var s = widget.find('input[mea_widgetswitchname="'+_this.switchName+'"]');

   s.prop('mea_action_flag', true);
   s.prop('mea_change_flag', true);
   s.switchbutton({
      checked: true,
      onText:  'I',
      offText: 'O',
      onChange: function(checked) {
         _this.setValue(data, _this.switchName+"_value", checked);
         if(s.prop('mea_action_flag')!==false)
         {
            _this.doAction(_this.switchName, data);
            s.prop('mea_change_flag', false);
            setTimeout(function() { s.prop('mea_change_flag', true); }, 125);
         }
      }
   });

   wg = widget.find('div[mea_widgetswitchname="'+_this.switchName+'"]').show();
   var vn = _this.getValue(data, "value", false);
   if(vn !== false)
      wg.attr("name", vn)
}


MeaWidget_switch.prototype.getTip = function()
{
   return "ON/OFF switch with return state";
}


MeaWidget_switch.prototype.getFormaters = function()
{
   var _this = this;
   var formaters = {
      mea_switch: function(v,o) {
         var val=false;
         if(v)
            val=true;
         var data = o.parent().prop('mea-widgetdata');
         var s=o.find('input[mea_widgetswitchname="'+_this.switchName+'"]');
         if(s.prop('mea_change_flag')!==false)
         {
            s.prop('mea_action_flag', false);
            if(val===false)
               s.switchbutton('uncheck');
            else
               s.switchbutton('check');
            setTimeout(function() { s.prop('mea_action_flag', true); }, 125);
         }
      }
   };

   return formaters;
}


MeaWidget_switch.prototype.getActions = function()
{
   var _this = this;
}


MeaWidget_switch.prototype.getStyle = function()
{
var style = "\
.mea_switch_disabled { \
   border-style: dotted; \
   border-width: 1px; \
   border-color: gray; \
} \
.mea_switch_enabled { \
   border-style: solid; \
   border-width: 1px; \
   border-color: transparent; \
}"
return style;
}


MeaWidget_switch.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width: 50px; height: 50px; background:blue;'></div>";
   return html;
}


MeaWidget_switch.prototype.getHtml = function()
{
var _this = this;
var html = 
" \
<div id = '"+_this.type+"_model' \
   mea_widget = '"+_this.type+"\' \
   class = 'mea-widget mea_switch_disabled' data-widgetparams='"+JSON.stringify(_this.params)+"' \
   style = 'position:absolute; width:60px;height:26px;' \
> \
   <div mea_widgetswitchname='"+_this.switchName+"' \
        mea_valueformater='mea_switch' \
        style='display:none' \
   > \
      <input mea_widgetswitchname='"+_this.switchName+"'> \
   </div> \
</div> \
";
return html;
}
