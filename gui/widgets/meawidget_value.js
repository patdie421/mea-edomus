extendClass(MeaWidget_value, MeaWidget);

var meaWidget_value = new MeaWidget_value("MeaWidget_value", "basic", "value");

function MeaWidget_value(name, group, type)
{
   MeaWidget_value.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);

   this.valueName = "value";
   this.valueUnit = "unit";

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

   this.params["values"][this.valueName] = this.automator_vars;;
   this.params["labels"][this.valueUnit] = "";
   this.params["parameters"]["low alarm"] = "";
   this.params["parameters"]["high alarm"] = "";
}


MeaWidget_value.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   if(data)
   {
      widget.find('label[mea_widgetlabelname="'+_this.valueUnit+'"]').text(_this.getValue(data, _this.valueUnit, _this.valueUnit));
      widget.find('label[mea_widgetvaluename="'+_this.valueName+'"]').attr("name",_this.getValue(data, _this.valueName, "")).text('?.??');
   
      widget.find('div[mea_widgetvaluename="'+_this.valueName+'"]')
         .attr("name",_this.getValue(data, _this.valueName, ""))
         .attr("lowalarm", _this.getValue(data, "low alarm", false))
         .attr("highalarm", _this.getValue(data, "high alarm", false));
   }
}


MeaWidget_value.prototype.disabled = function(id, d) {
   var _this = this;
   var widget = $("#"+id);

   if(d===true)
   {
      widget.find('div[mea_widgetvaluename="'+_this.valueName+'"]').removeClass('mea_value_alarmOn');
      widget.removeClass("mea_value_enabled");
      widget.addClass("mea_value_disabled");
   }
   else
   {
      widget.removeClass("mea_value_disabled");
      widget.addClass("mea_value_enabled");
   }
}


MeaWidget_value.prototype.getFormaters = function()
{
   var formaters = {
      mea_value_alarm: function(v,o) {

         if(isNaN(v)===true)
            return;

         try {
            var low = o.attr('lowalarm');
            var high = o.attr('highalarm');

            if(low && v<=low)
               o.addClass("mea_value_alarmOn");
            else if(high && v>=high)
               o.addClass("mea_value_alarmOn");
            else
               o.removeClass("mea_value_alarmOn");

         }
         catch(e) { console.log("mea_value_alarm error") };
      }
   };

   return formaters;
}


MeaWidget_value.prototype.getTip = function()
{
   return "simple value + unit with alarm";
}


MeaWidget_value.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width:80px;height:80px;line-height:80px;background-image:url(/widgets/meawidget_value.png);'></div>";

   return html;
}


MeaWidget_value.prototype.getStyle = function()
{
var style = "\
.mea_value_disabled { \
   border-style: dotted; \
   border-width: 1px; \
   border-color: gray; \
   opacity: 0.5; \
} \
.mea_value_enabled { \
   border-style: solid; \
   border-width: 1px; \
   border-color: transparent; \
} \
.mea_value_alarmOn { \
   background:red; \
}"
return style;
}


MeaWidget_value.prototype.getHtml = function()
{
var _this = this;

var html = " \
<div id = '"+_this.type+"_model' \
     mea_widget = '"+ _this.type +"' \
     class = 'mea-widget mea_value_disabled' \
     style = 'position:absolute;width:138px;height:23px;' \
     data-widgetparams='"+JSON.stringify(_this.params)+"' \
> \
   <div \
        mea_widgetvaluename='"+ _this.valueName +"' \
        mea_notooltip='true' \
        mea_valueformater='mea_value_alarm' \
        style='width:100%;height:100%;text-align:center;font-size:18px'> \
      <div style='float:left;width:98px;height:23px;overflow:hidden;text-align:right'> \
         <label class = 'mea-value'\
                mea_widgetvaluename='"+ _this.valueName +"' \
                mea_valueformater='float' style='font-size:18px'> \
            ?.?? \
         </label> \
      </div> \
      <div style='float:right;width:40px;height:23px;line-height:23px;padding:0;overflow:hidden'> \
         <label class = 'mea-label' \
                mea_widgetlabelname='"+ _this.valueUnit +"' \
                style='font-size:12px'> \
            unit \
         </label> \
      </div> \
   </div> \
</div>";
return html;
}

