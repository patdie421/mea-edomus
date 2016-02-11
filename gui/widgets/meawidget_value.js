extendClass(MeaWidget_value, MeaWidget);


var meaWidget_value = new MeaWidget_value("MeaWidget_value", "basic", "value");

function MeaWidget_value(name, group, type)
{
   MeaWidget_value.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);

   this.valueName = "value";
   this.valueUnit = "unit";

   this.params["values"][this.valueName] = "";
   this.params["labels"][this.valueUnit] = this.valueUnit;
}


MeaWidget_value.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   widget.find('label[mea_widgetlabelname="'+_this.valueUnit+'"]').text(_this.getValue(data, _this.valueUnit, _this.valueUnit));
   widget.find('label[mea_widgetvaluename="'+_this.valueName+'"]').attr("name",_this.getValue(data, _this.valueName, "")).text('?.??');
}


MeaWidget_value.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width:50px;height:50px;line-height:50px;text-align:center;background-color:\"#FFFFFF\";'>value</div>";

   return html;
}


MeaWidget_value.prototype.getHtml = function()
{
   var _this = this;

   var html = " \
      <div id = '"+_this.type+"_model' \
         mea_widget = '"+_this.type+"\' \
              class = 'mea-widget' data-widgetparams='"+JSON.stringify(_this.params)+"' \
              style = 'position:absolute;width:140px;height:25px;' \
      > \
         <div class='ui-widget-content'  style='width:100%;height:100%;text-align:center;font-size:18px'> \
            <div style='float:left;width:100px;height:25px;overflow:hidden;text-align:right'> \
               <label class = 'mea-value' mea_widgetvaluename='"+_this.valueName+"' mea_valueformater='float' style='font-size:18px'>value</label> \
            </div> \
            <div style='float:right;width:40px;height:25px;overflow:hidden'> \
               <label class = 'mea-label' mea_widgetlabelname='"+_this.valueUnit+"' style='font-size:18px'>unit</label> \
            </div> \
         </div> \
      </div>";

   return html;
}

