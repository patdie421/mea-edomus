extendClass(MeaWidget_bulb, MeaWidget);

var meaWidget_bulb = new MeaWidget_bulb("MeaWidget_bulb", "basic", "bulb");

function MeaWidget_bulb(name, group, type)
{
   MeaWidget_bulb.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);

   this.valueName = "value";

   this.automator_vars =  {
      "val" : "",
      "editor": {
         "type":"combobox",
         "options": {
             panelMinWidth:300,
             valueField:'name',
             textField:'name',
             groupField:'group',
             method:'get',
             url:'models/get_automator_variables.php'
         }
      }
   };

   this.params["values"][this.valueName] = this.automator_vars;
}


MeaWidget_bulb.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   if(data)
      widget.find('div[mea_widgetvaluename="'+_this.valueName+'"]').attr("name",_this.getValue(data, _this.valueName, ""));
}


MeaWidget_bulb.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width:80px;height:80px;background-size:70% 70%;background-repeat:no-repeat;background-position:center;background-image:url(/widgets/meawidget_bulbon.svg);'></div>";

   return html;
}


MeaWidget_bulb.prototype.disabled = function(id,d) {
   var _this = this;
   var widget = $("#"+id);

   if(d===true)
   {
      widget.find('div[mea_widgetvaluename="'+_this.valueName+'"]').removeClass('mea_bulb_off').removeClass('mea_bulb_on').addClass('mea_bulb_na');
      widget.removeClass("mea_bulb_enabled");
      widget.addClass("mea_bulb_disabled");
   }
   else
   {
      widget.removeClass("mea_bulb_disabled");
      widget.addClass("mea_bulb_enabled");
   }
}


MeaWidget_bulb.prototype.getFormaters = function()
{
   var formaters = {
      mea_bulb_onoff: function(v,o) {
         if(isNaN(v))
         {
            o.addClass('mea_bulb_na');
            o.removeClass('mea_bulb_off');
            o.removeClass('mea_bulb_on');
         }
         else
         {
            o.removeClass('mea_bulb_na');
            if(v === true || v != 0 || v === "true")
            {
               o.addClass('mea_bulb_on');
               o.removeClass('mea_bulb_off');
            }
            else
            {
               o.addClass('mea_bulb_off');
               o.removeClass('mea_bulb_on');
            }
         }
      } 
   };

   return formaters;
}


MeaWidget_bulb.prototype.getStyle = function()
{
var style = "\
.mea_bulb { \
   width:100px; \
   height:100px; \
   background-size:100% 100%; \
} \
.mea_bulb_off { \
background-image:url(/widgets/meawidget_bulboff.svg); \
} \
.mea_bulb_na { \
background-image:url(/widgets/meawidget_bulbna.svg); \
} \
.mea_bulb_on { \
background-image:url(/widgets/meawidget_bulbon.svg); \
} \
.mea_bulb_disabled { \
   border-style: dotted; \
   border-width: 1px; \
   border-color: gray; \
   opacity: 0.5; \
} \
.mea_bulb_enabled { \
   border-style: solid; \
   border-width: 1px; \
   border-color: transparent; \
}";

return style;
}


MeaWidget_bulb.prototype.getTip = function()
{
   return "on/off bulb";
}


MeaWidget_bulb.prototype.getHtml = function()
{
var _this = this;
var html = "\
<div id = '"+_this.type+"_model' \
     mea_widget='"+_this.type+"' \
     class='mea-widget mea_bulb_disabled' \
     data-widgetparams='"+JSON.stringify(_this.params)+"' \
     style ='width:100px;height:100px;position:absolute;' \
> \
   <div class='ui-widget-content' \
        style='width:100%;height:100%;text-align:center;'> \
      <div class='mea_bulb mea_bulb_na' \
           mea_valueformater='mea_bulb_onoff' \
           mea_widgetvaluename='"+this.valueName+"'></div> \
   </div> \
</div> \
";
return html;
}
