extendClass(MeaWidget_slider, MeaWidget);

var meaWidget_button = new MeaWidget_slider("MeaWidget_slider", "multiple", "slider");

function MeaWidget_slider(name, group, type)
{
   MeaWidget_slider.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);

   this.sliderName1 = "slider1";
   this.sliderName2 = "slider2";
   this.params = {};

   this.params["variables"] = {}
   this.params["variables"][this.sliderName1+"_value"]=0.0;
   this.params["variables"][this.sliderName2+"_value"]=0.0;

   this.params["values"] = {};

   this.params["labels"] = {};

   this.params["actions"] = {};
   this.params["actions"][this.sliderName1] = '';
   this.params["actions"][this.sliderName2] = '';

   this.params["links"] = {};
}


MeaWidget_slider.prototype.disabled = function(id, d)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   widget.find('div[mea_widgetslidername="'+_this.sliderName1+'"]').slider({
      disabled: d
   });
   widget.find('div[mea_widgetslidername="'+_this.sliderName2+'"]').slider({
      disabled: d
   });
}


MeaWidget_slider.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   widget.find('div[mea_widgetslidername="'+_this.sliderName1+'"]').slider({
      disabled: true,
      mode: "h",
      width: 100,
      showTip: true,
      min:0,
      max:100,
      step:5,
      value:_this.getValue(data,_this.sliderName1+"_value", 0),
      onComplete: function(value) {
         _this.setValue(data, _this.sliderName1+"_value", value);
         _this.doAction(_this.sliderName1, data);
      },
      tipFormatter: function(value) {
         return value+"%";
      }
   });
   widget.find('div[mea_widgetslidername="'+_this.sliderName2+'"]').slider({
      disabled: true,
      mode: "h",
      width: 100,
      showTip: true,
      min:0,
      max:100,
      step:5,
      value:_this.getValue(data,_this.sliderName2+"_value", 0),
      onComplete: function(value) {
         _this.setValue(data,_this.sliderName2+"_value", value);
         _this.doAction(_this.sliderName2, data);
      },
      tipFormatter: function(value) {
         return value+"%";
      }
   });
}


MeaWidget_slider.prototype.getTip = function()
{
   return "double % slider"
}


MeaWidget_slider.prototype.getFormaters = function()
{
   var formaters = {
   };

   return formaters;
}


MeaWidget_slider.prototype.getActions = function()
{
   var _this = this;
}


MeaWidget_slider.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width: 50px; height: 50px; background-image: url(\"/widgets/slider1.png\");'></div>";
   return html;
}


MeaWidget_slider.prototype.getHtml = function()
{
var _this = this; var html =
" \
<div id = '"+_this.type+"_model' \
   mea_widget = '"+_this.type+"\' \
   class = 'mea-widget' data-widgetparams='"+JSON.stringify(_this.params)+"' \
   style = 'position:absolute; width: 200px; height: 100px; border:1px solid gray;background:pink;' \
> \
   <div style='position:relative;top:20px;left:50px;width:200px;height:25px;'> \
      <div mea_widgetslidername='"+_this.sliderName1+"'></div> \
   </div> \
   <div style='position:relative;top:40px;left:50px;width:200px;height:25px;'> \
      <div mea_widgetslidername='"+_this.sliderName2+"'></div> \
   </div> \
</div> \
";

return html;
}
