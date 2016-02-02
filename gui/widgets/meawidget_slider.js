extendClass(MeaWidget_slider, MeaWidget);

var meaWidget_button = new MeaWidget_slider("MeaWidget_slider", "basic", "slider");

function MeaWidget_slider(name, group, type)
{
   MeaWidget_slider.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);

   this.sliderName1 = "slider1";
   this.sliderName1 = "slider2";
   this.params = {};

   this.params["variables"] = {}
   this.params["variables"]["v1"]=0.0;
   this.params["variables"]["v2"]=0.0;

   this.params["values"] = {};

   this.params["labels"] = {};

   this.params["actions"] = {};
   this.params["actions"]["slider1"] = '{"xplsend":{"current":"[v1]"}}';
   this.params["actions"]["slider2"] = '{"xplsend":{"current":"[v2]"}}';

   this.params["links"] = {};

}

/*
function _setVal(data, n, v)
{
   $.each(data, function(i, val)
   {
      if(val.name == n)
      {
         val.value = v;
         return false;
      }
   });
}


function _getVal(data, n)
{
   var found = false;

   $.each(data, function(i, val)
   {
      if(val.name == n)
      {
         found = val.value;
         return false;
      }
   });
   if(found !== false)
      return found;
   else
      return "";
}
*/

MeaWidget_slider.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   widget.find('div[mea_widgetslidername="'+_this.sliderName1+'"]').slider({
      mode: "h",
      width: 100,
      showTip: true,
      min:0,
      max:100,
      step:5,
      value:_this.getValue(data,"v1", 0),
      onComplete: function(value) {
         _this.setValue(data, 'v1', value);
         _this.doAction("slider1", data);
      },
      tipFormatter: function(value) {
         return value+"%";
      }
   });
   widget.find('div[mea_widgetslidername="'+_this.sliderName2+'"]').slider({
      mode: "h",
      width: 100,
      showTip: true,
      min:0,
      max:100,
      step:5,
      value:_this.getValue(data,"v2", 0),
      onComplete: function(value) {
         _this.setValue(data,'v2', value);
         _this.doAction("slider2", data);
         //  console.log(id+" v2: "+ _this.getValue(data,'v2'));
      },
      tipFormatter: function(value) {
         return value+"%";
      }
   });
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

   var html="<div id='"+_this.type+"' class='drag' style='width: 64px; height: 64px; border:1px solid red; background-color: pink;'></div>";

   return html;
}


MeaWidget_slider.prototype.getHtml = function()
{
var _this = this; var html =
" \
\
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
\
";
return html;
}
