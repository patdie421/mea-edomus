extendClass(MeaWidget_button, MeaWidget);

var meaWidget_button = new MeaWidget_button("MeaWidget_button", "basic", "button");

function MeaWidget_button(name, group, type)
{
   MeaWidget_button.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);

   this.labelName = "title";
   this.valueName1 = "value1";
   this.valueName2 = "value2";
   this.valueUnit = "unit";
   this.action1 = "push";

   this.params = {};

   this.params["values"] = {};
   this.params["values"][this.valueName1] = "";
   this.params["values"][this.valueName2] = "";

   this.params["labels"] = {};
   this.params["labels"][this.labelName] = this.labelName;
   this.params["labels"][this.valueUnit] = this.valueUnit;

   this.params["actions"] = {};
   this.params["actions"][this.action1] = '{"xplsend":{"current":"high"}}';

   this.params["links"] = {};

}


MeaWidget_button.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   widget.find('label[mea_widgetlabelname="'+_this.labelName+'"]').text(_this.getValue(data, _this.labelName, _this.labelName));
   widget.find('label[mea_widgetlabelname="'+_this.valueUnit+'"]').text(_this.getValue(data, _this.valueUnit, _this.valueUnit));

   widget.find('label[mea_widgetvaluename="'+_this.valueName1+'"]').attr("name",_this.getValue(data, _this.valueName1, ""));
   widget.find('div[mea_widgetvaluename="'+_this.valueName2+'"]').attr("name",_this.getValue(data, _this.valueName2, ""));

   widget.find('div[mea_widgetactionname="'+_this.action1+'"]').on('click', function() {
      _this.doAction(_this.action1, data);
   });
}


MeaWidget_button.prototype.getFormaters = function()
{
   var formaters = {
      custom1: function(x,o) { o.html("##"+x+"##"); },
      custom2: function(x,o) { o.html("@@"+x+"@@"); }
   };

   return formaters;
}


MeaWidget_button.prototype.getActions = function()
{
   var _this = this;
}


MeaWidget_button.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width: 64px; height: 64px; border:1px solid red; background-color: lightred;'></div>";

   return html;
}


MeaWidget_button.prototype.getHtml = function()
{
var _this = this; var html =
" \
\
<style> \
.mea_button { \
   width:50px; \
   height:25px; \
   border:4px solid green; \
} \
.mea_button:hover { \
    cursor: pointer; \
} \
.mea_button:active { \
   background-color: green; \
   } \
</style> \
<div id = '"+_this.type+"_model' \
   mea_widget = '"+_this.type+"\' \
   class = 'mea-widget' data-widgetparams='"+JSON.stringify(_this.params)+"' \
   style = 'position:absolute; width: 120px; height: 80px; border:1px solid gray;' \
> \
   <div class='ui-widget-content'  style='width:100%;height:100%;text-align:center;'> \
      <div><label class = 'mea-label' mea_widgetlabelname='"+_this.labelName+"'>title</label></div> \
      <div> \
         <label class = 'mea-value' mea_widgetvaluename='"+_this.valueName1+"' mea_valueformater='float'>value</label> \
         <label class = 'mea-label' mea_widgetlabelname='"+_this.valueUnit+"'>unit</label> \
      </div> \
      <div class = 'mea-value' mea_widgetvaluename='"+_this.valueName2+"' mea_valueformater='custom2'>value</div> \
      <div class='mea_button' mea_widgetactionname='push'>push</div> \
   </div> \
</div> \
\
";
return html;
}
