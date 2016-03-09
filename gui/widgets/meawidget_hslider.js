extendClass(MeaWidget_hslider, MeaWidget);

var meaWidget_button = new MeaWidget_hslider("MeaWidget_hslider", "basic", "hslider");

function MeaWidget_hslider(name, group, type)
{
   MeaWidget_hslider.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);
   this.hsliderName = "hslider";

   this.params["variables"][this.hsliderName+"_value"]=0;
   this.params["values"]["value"]=0;
   this.params["actions"][this.hsliderName] = '';
   this.params["parameters"]["unit"] = '';
   this.params["parameters"]["min"] = 0;
   this.params["parameters"]["max"] = 100;
   this.params["parameters"]["step"] = 1;
}


MeaWidget_hslider.prototype.disabled = function(id, d)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   widget.find('div[mea_widgethslidername="'+_this.hsliderName+'"]').slider({
      disabled: d
   });

   if(d===true)
   {
      widget.removeClass("mea_hslider_enabled");
      widget.addClass("mea_hslider_disabled");
   }
   else
   {
      widget.removeClass("mea_hslider_disabled");
      widget.addClass("mea_hslider_enabled");
   }
}


MeaWidget_hslider.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   var value=0;
   var min=0;
   var max=100;
   var step=1;
   var unit=' ';

   if(data)
   {
      value = _this.getValue(data,_this.hsliderName+"_value", 0);
      min = _this.getValue(data,"min", 0)*1;
      max = _this.getValue(data,"max", 100)*1;
      step= _this.getValue(data,"step", 1)*1;
      unit= _this.getValue(data,"unit", ' ');
   }

   if(step<=0)
      step=1;
   if(max<min)
   {
      var tmp = max;
      max = min;
      min = tmp;
   }
   var _max=(max-min)/step;

   sl = widget.find('div[mea_widgethslidername="'+_this.hsliderName+'"]');
   sl.slider({
      disabled: true,
      mode: "h",
      width: 150,
      showTip: true,
      min:0,
      max:_max,
      step:1,
      rule:[min,max],
      value: ((value-min)/step).toFixed(2),
      onComplete: function(value) {
         _this.setValue(data,_this.hsliderName+"_value", min+value*step);
         _this.doAction(_this.hsliderName, data);
      },
      tipFormatter: function(value) {
         return (min+value*step).toFixed(2)+unit;
      }
   });

   if(data)
   {
      var vn = _this.getValue(data, "value", false);
      if(vn !== false)
         sl.attr("name", vn)
   }
}


MeaWidget_hslider.prototype.getTip = function()
{
   return "slider with return state";
}


MeaWidget_hslider.prototype.getFormaters = function()
{
   var _this = this;
   var formaters = {
      mea_hslider: function(v,o) {
         if(!isNaN(v))
         {
            var data = o.parent().parent().prop('mea-widgetdata');
            var min = _this.getValue(data,"min", 0);
            var step= _this.getValue(data,"step", 1);

            o.slider("setValue",((v-min)/step).toFixed(2));
         }
      }
   };

   return formaters;
}


MeaWidget_hslider.prototype.getActions = function()
{
   var _this = this;
}


MeaWidget_hslider.prototype.getStyle = function()
{
var style = "\
.mea_hslider_disabled { \
   border-style: dotted; \
   border-width: 1px; \
   border-color: gray; \
} \
.mea_hslider_enabled { \
   border-style: solid; \
   border-width: 1px; \
   border-color: transparent; \
}"
return style;
}


MeaWidget_hslider.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width: 50px; height: 50px; background:purple;'></div>";

   return html;
}


MeaWidget_hslider.prototype.getHtml = function()
{
var _this = this;
var html = 
"<div id = '"+_this.type+"_model' " +
     "mea_widget = '"+_this.type+"\' " +
     "class = 'mea-widget mea_hslider_disabled' data-widgetparams='"+JSON.stringify(_this.params)+"' " +
     "style = 'position:absolute; width:195px;height:70px;' " +
">" +
  "<div style='position:relative;top:20px;left:20px;width:140px;height:25px;'>" + 
     "<div mea_widgethslidername='"+_this.hsliderName+"' " +
          "mea_valueformater='mea_hslider' " +
     ">" +
     "</div>" +
  "</div>" +
"</div>";
return html;
}
