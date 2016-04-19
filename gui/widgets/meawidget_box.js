extendClass(MeaWidget_box, MeaWidget);

var meaWidget_box = new MeaWidget_box("MeaWidget_box", "draw", "box");

function MeaWidget_box(name, group, type)
{
   MeaWidget_box.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);

   this.params["parameters"]["width"] = 100;
   this.params["parameters"]["height"] = 50;
   this.params["parameters"]["border"] = "black";
   this.params["parameters"]["background"] = "";
}


function isNumeric(obj) {
   return obj - parseFloat(obj) >= 0;
}

MeaWidget_box.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   if(data)
   {
      var width=_this.getValue(data, "width", false);
      if(width !== false && isNumeric(width) && width >0)
         widget.width(width);

      var height=_this.getValue(data, "height", false);
      if(height !== false && isNumeric(height) && height >0)
         widget.height(height);

      var border = _this.getValue(data, "border", false);
      if(border !== false)
         widget.css("border-color", border);

      var background = _this.getValue(data, "background", false);
      if(background !== false && background != "")
         widget.css("background-color", background);
      else
         widget.css('background', 'transparent');
   }
}


MeaWidget_box.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width:78px;height:78px;line-height:78px;text-align:center;background-color:\"#FFFFFF\";'> \
                <div style='margin:auto;vertical-align:middle;display:inline-block;width:60px;height:40px;line-height:40px;background-color:yellow;border:1px solid black'>box</div> \
             </div>";

   return html;
}


MeaWidget_box.prototype.getHtml = function()
{
var _this = this;
var html = "\
<div id = '"+_this.type+"_model' \
     mea_widget='"+_this.type+"' \
     class='mea-widget mea_box' \
     data-widgetparams='"+JSON.stringify(_this.params)+"' \
     style ='position:absolute;width:100px;height:50px;border: 1px solid red' \
> \
   <div style='width:100%;height:100%;padding:5px'> \
      <div class='mea_dragzone_for_resizable' style='width:calc( 100% - 12px );height:calc( 100% - 12px );'><div>\
   </div> \
</div> \
";
return html;
}
