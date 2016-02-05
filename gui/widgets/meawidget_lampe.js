extendClass(MeaWidget_lampe, MeaWidget);

var meaWidget_lampe = new MeaWidget_lampe("MeaWidget_lampe", "basic", "numeric");

function MeaWidget_lampe(name, group, type)
{
   MeaWidget_lampe.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);

   this.labelName = "title";
   this.valueName1 = "value1";
   this.valueName2 = "value2";
   this.valueUnit = "unit";
   this.params = {};

   this.params["values"] = {};
   this.params["values"][this.valueName1] = "";
   this.params["values"][this.valueName2] = "";

   this.params["labels"] = {};
   this.params["labels"][this.labelName] = this.labelName;
   this.params["labels"][this.valueUnit] = this.valueUnit;

   this.params["actions"] = {};

   this.params["links"] = {};

}


MeaWidget_lampe.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   widget.find('label[mea_widgetlabelname="'+_this.labelName+'"]').text(_this.getValue(data, _this.labelName, _this.labelName));
   widget.find('label[mea_widgetlabelname="'+_this.valueUnit+'"]').text(_this.getValue(data, _this.valueUnit, _this.valueUnit));

   widget.find('label[mea_widgetvaluename="'+_this.valueName1+'"]').attr("name",_this.getValue(data, _this.valueName1, ""));
   widget.find('input[mea_widgetvaluename="'+_this.valueName1+'"]').attr("name",_this.getValue(data, _this.valueName1, ""));

   widget.find('div[mea_widgetvaluename="'+_this.valueName2+'"]').attr("name",_this.getValue(data, _this.valueName2, ""));
}


MeaWidget_lampe.prototype.getFormaters = function()
{
   var formaters = {
      custom1: function(x,o) { o.html("##"+x+"##"); },
      custom2: function(x,o) { o.html("@@"+x+"@@"); }
   };

   return formaters;
}


MeaWidget_lampe.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width: 50px; height: 50px; background-color: lightgray;'></div>";

   return html;
}


MeaWidget_lampe.prototype.getHtml = function()
{
   var _this = this;

   var html = " \
      <div id = '"+_this.type+"_model' \
         mea_widget = '"+_this.type+"\' \
              class = 'mea-widget' data-widgetparams='"+JSON.stringify(_this.params)+"' \
              style = 'position:absolute; width: 120px; height: 80px; border:1px solid lightgray; background-color: lightgray;' \
      > \
         <div class='ui-widget-content'  style='width:100%;height:100%;text-align:center;'> \
            <div><label class = 'mea-label' mea_widgetlabelname='"+_this.labelName+"'>title</label></div> \
            <div> \
               <label class = 'mea-value' mea_widgetvaluename='"+_this.valueName1+"' mea_valueformater='float'>value</label> \
               <label class = 'mea-label' mea_widgetlabelname='"+_this.valueUnit+"'>unit</label> \
            </div> \
            <div class = 'mea-value' mea_widgetvaluename='"+_this.valueName2+"' mea_valueformater='custom2'>value</div> \
            <input readonly class='mea-value' value='value' style='width:60px;margin-top: 10px;text-align:center;' mea_widgetvaluename='"+_this.valueName1+"' mea_valueformater='boolean'> \
         </div> \
      </div>";

   return html;
}
