extendClass(MeaWidget_label, MeaWidget);

var meaWidget_label = new MeaWidget_label("MeaWidget_label", "draw", "label");

function MeaWidget_label(name, group, type)
{
   MeaWidget_label.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);

   this.label = "label";
   
   this.fontweight =  {
      "val" : "normal",
      "editor": {
         "type":"combobox",
         "options": {
             panelHeight: 22*2,
             valueField:'lv',
             textField:'lv',
             data: [
                { lv: 'normal' },
                { lv: 'bold' }
             ]
         }
      }
   };

   this.decoration =  {
      "val" : "none",
      "editor": {
         "type":"combobox",
         "options": {
             panelHeight: 21*4,
             valueField:'lv',
             textField:'lv',
             data: [
                { lv: 'none' },
                { lv: 'underline' },
                { lv: 'overline' },
                { lv: 'line-through' }
             ]
         }
      }
   };

   this.fontstyle =  {
      "val" : "normal",
      "editor": {
         "type":"combobox",
         "options": {
             panelHeight: 21*3,
             valueField:'lv',
             textField:'lv',
             data: [
                { lv: 'normal' },
                { lv: 'italic' },
                { lv: 'oblique' }
             ]
         }
      }
   };

   this.font =  {
      "val" : "Arial",
      "editor": {
         "type":"combobox",
         "options": {
             panelHeight: 21*8,
             valueField:'lv',
             textField:'lv',
             data: [
                { lv: 'Arial' },
                { lv: 'Arial Black' },
                { lv: 'Verdena' },
                { lv: 'Impact' },
                { lv: 'Tahoma' },
                { lv: 'Comic Sans MS' },
                { lv: 'Times' },
                { lv: 'Georgia' },
                { lv: 'Palatino' },
                { lv: 'Courier' },
                { lv: 'Lucida Console' }
             ]
         }
      }
   };

   this.fontsize =  {
      "val" : '16',
      "editor": {
         "type":"combobox",
         "options": {
             panelHeight: 21*8,
             valueField:'lv',
             textField:'lv',
             data: [
                { lv: '9' },
                { lv: '10' },
                { lv: '12' },
                { lv: '14' },
                { lv: '16' },
                { lv: '18' },
                { lv: '20' },
                { lv: '24' },
                { lv: '28' },
                { lv: '32' },
                { lv: '48' },
                { lv: '64' }
             ]
         }
      }
   };

   this.color =  {
      "val" : 'black',
      "editor": {
         "type":"combobox",
         "options": {
             panelHeight: 21*8,
             valueField:'lv',
             textField:'lv',
             data: [
                { lv: 'red' },
                { lv: 'green' },
                { lv: 'purple' },
                { lv: 'yellow' },
                { lv: 'gray' },
                { lv: 'lightgray' },
                { lv: 'blue' },
                { lv: 'orange' }
             ]
         }
      }
   };

   this.params["parameters"]["label"] = "Text";
   this.params["parameters"]["font"] = this.font;
   this.params["parameters"]["fontsize"] = this.fontsize;
   this.params["parameters"]["color"] = this.color;
   this.params["parameters"]["fontweight"] = this.fontweight;
   this.params["parameters"]["fontstyle"] = this.fontstyle;
   this.params["parameters"]["decoration"] = this.decoration;
}


MeaWidget_label.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   if(data)
   {
      var fontsize = parseFloat(_this.getValue(data, "fontsize", _this.fontsize));
      var label = _this.getValue(data, _this.label, "");
      var font  = _this.getValue(data, "font", _this.font);
      var color  = _this.getValue(data, "color", _this.color);
      var fontweight  = _this.getValue(data, "fontweight", _this.fontweight);
      var fontstyle  = _this.getValue(data, "fontstyle", _this.fontstyle);
      var decoration  = _this.getValue(data, "decoration", _this.decoration);

      widget.find('label[mea_widgetlabelname="'+_this.label+'"]')
         .text(label)
         .css({'font-size': fontsize,
               'font-family' : font,
               'font-weight' : fontweight,
               'font-style' : fontstyle,
               'text-decoration' : decoration,
               'color' : color});
   }
}


MeaWidget_label.prototype.disabled = function(id, d) {
   var _this = this;
   var widget = $("#"+id);

   if(d===true)
   {
      widget.removeClass("mea_label_enabled");
      widget.addClass("mea_label_disabled");
   }
   else
   {
      widget.removeClass("mea_label_disabled");
      widget.addClass("mea_label_enabled");
   }
}


MeaWidget_label.prototype.getTip = function()
{
   return "just a label";
}


MeaWidget_label.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width:80px;height:80px;line-height:80px;text-align:center;font-size:18px'>" + 
               "Label" +
            "</div>";

   return html;
}


MeaWidget_label.prototype.getStyle = function()
{
var style = "\
.mea_label_disabled { \
   border-style: dotted; \
   border-width: 1px; \
   border-color: gray; \
   opacity: 0.5; \
} \
.mea_label_enabled { \
   border-style: solid; \
   border-width: 1px; \
   border-color: transparent; \
   opacity: 1.0; \
}"
return style;
}


MeaWidget_label.prototype.getHtml = function()
{
var _this = this;

var html = " \
<div id = '"+_this.type+"_model' \
     mea_widget = '"+ _this.type +"' \
     class = 'mea-widget mea_value_disabled' \
     style = 'position:absolute;display:inline-block;' \
     data-widgetparams='"+JSON.stringify(_this.params)+"' \
> \
         <label \
                mea_widgetlabelname='"+ _this.label +"' \
                style='font-size:16px'> \
         Text \
         </label> \
</div>";
return html;
}

