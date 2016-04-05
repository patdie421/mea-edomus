extendClass(MeaWidget_gauge, MeaWidget);

var meaWidget_gauge = new MeaWidget_gauge("MeaWidget_gauge", "basic", "gauge");

function MeaWidget_gauge(name, group, type)
{
   MeaWidget_gauge.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);
   this.gaugeName = "gauge";

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
   this.params["values"]["value"] = this.automator_vars;
//   this.params["values"]["value"]=0;
   this.params["parameters"]["data name"] = '';
   this.params["parameters"]["unit"] = '';
   this.params["parameters"]["min"] = 0;
   this.params["parameters"]["max"] = 100;
   this.params["parameters"]["stop1"] = 33;
   this.params["parameters"]["stop2"] = 66;
   this.params["parameters"]["color1"] = '#55BF3B'; // green
   this.params["parameters"]["color2"] = '#DDDF0D'; // yellow 
   this.params["parameters"]["color3"] = '#DF5353'; // red
}


MeaWidget_gauge.prototype.disabled = function(id, d)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   if(d===true)
   {
      widget.removeClass("mea_gauge_enabled");
      widget.addClass("mea_gauge_disabled");
   }
   else
   {
      widget.removeClass("mea_gauge_disabled");
      widget.addClass("mea_gauge_enabled");
   }
}


MeaWidget_gauge.prototype.mkgauge = function(widget, data)
{
   var _this = this;
   var value=0,
       reversed=false;
       min   = _this.params["parameters"]["min"],
       max   = _this.params["parameters"]["max"],
       stop1 = _this.params["parameters"]["stop1"],
       stop2 = _this.params["parameters"]["stop2"],
       unit  = _this.params["parameters"]["unit"],
       color1= _this.params["parameters"]["color1"],
       color2= _this.params["parameters"]["color2"],
       color3= _this.params["parameters"]["color3"];
   if(data)
   {
      value  = _this.getValue(data,_this.gaugeName+"_value", 0);
      min    = _this.getValue(data,"min", min)*1;
      max    = _this.getValue(data,"max", max)*1;
      stop1  = _this.getValue(data,"stop1", stop1)*1;
      stop2  = _this.getValue(data,"stop2", stop2)*1;
      color1 = _this.getValue(data,"color1", color1);
      color2 = _this.getValue(data,"color2", color2);
      color3 = _this.getValue(data,"color3", color3);
      unit   = _this.getValue(data,"unit", unit);
   }

   if(min>max)
   {
      reversed = true;
      var tmp = min;
      min = max;
      max = tmp;
   }

   if(stop1 > stop2)
   {
      var tmp = stop1;
      stop1 = stop2;
      stop2 = tmp;
   }

   if(stop1 < min)
   {
      stop1 = min;
      if(stop1 > stop2)
         stop2 = stop1;
   }

   if(stop2 > max)
   {
      stop2 = max;
      if(stop2 < stop1)
         stop1 = stop2;
   }

   widget.highcharts({
      credits: {
         enabled: false
      },

      tooltip: {
         enabled: false
      },

      chart: {
         type: 'gauge',
         plotBackgroundColor: null,
         plotBackgroundImage: null,
         plotBorderWidth: 0,
         plotShadow: false,

         spacingTop: -5,
         spacingLeft: -5,
         spacingRight: -5,
         spacingBottom: -5,

         backgroundColor:'rgba(255, 255, 255, 0)',

         animation: false
      },

      title: null,

      exporting: {
         enabled: false
      },

      pane: {
         startAngle: -150,
         endAngle: 150,
         background: [{
            backgroundColor: {
               linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
               stops: [
                  [0, '#FFF'],
                  [1, '#333']
               ]
            },
            borderWidth: 0,
            outerRadius: '109%'
         }, {
            backgroundColor: {
               linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
               stops: [
                  [0, '#333'],
                  [1, '#FFF']
               ]
            },
            borderWidth: 1,
            outerRadius: '107%'
         }, {
                // default background
         }, {
            backgroundColor: '#DDD',
            borderWidth: 0,
            outerRadius: '105%',
            innerRadius: '103%'
         }]
      },

        // the value axis
      yAxis: {
         min: min,
         max: max,

         reversed: reversed,

         minorTickInterval: 'auto',
         minorTickWidth: 1,
         minorTickLength: 10,
         minorTickPosition: 'inside',
         minorTickColor: '#666',

         tickPixelInterval: 30,
         tickWidth: 2,
         tickPosition: 'inside',
         tickLength: 10,
         tickColor: '#666',
         labels: {
            step: 2,
            rotation: 'auto'
         },
         title: {
            text: unit
         },
         plotBands: [{
            from: min,
            to: stop1,
            color: color1
            }, {
            from: stop1,
            to: stop2,
            color: color2
            }, {
            from: stop2,
            to: max,
            color: color3
         }]
      },

      series: [{
         name: name,
         data: [value],
         tooltip: {
            valueSuffix: ' '+unit
         }
      }]
   })
}


MeaWidget_gauge.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   _this.mkgauge(widget, data);

   if(data)
   {
      var vn = _this.getValue(data, "value", false);
      if(vn !== false)
         widget.attr("name", vn);
   }
}


MeaWidget_gauge.prototype.getTip = function()
{
   return "gauge";
}


MeaWidget_gauge.prototype.getFormaters = function()
{
   var _this = this;
   var formaters = {
      mea_gauge: function(v,o) {
         var point = o.highcharts().series[0].points[0];  
         if(!isNaN(v))
            point.update(v);
         else
            point.update(0);
      }
   };

   return formaters;
}


MeaWidget_gauge.prototype.getActions = function()
{
   var _this = this;
}


MeaWidget_gauge.prototype.getStyle = function()
{
var style = 
".mea_gauge_disabled {" +
   "border-style: dotted;" +
   "border-width: 1px;" +
   "border-color: gray;" +
   "opacity: 0.5;" +
"}" +
".mea_gauge_enabled {" + 
   "border-style: solid;" +
   "border-width: 1px;" +
   "border-color: transparent;" +
   "opacity: 1;" +
"}";
return style;
}


MeaWidget_gauge.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width: 80px; height: 80px; background-image:url(/widgets/meawidget_gauge.png);'></div>";

   return html;
}


MeaWidget_gauge.prototype.getHtml = function()
{
var _this = this;
var html = 
"<div id = '"+_this.type+"_model' " +
     "class = 'mea-widget mea_gauge_disabled' " +
     "mea_widget = '"+_this.type+"' " +
     "style = 'position:absolute;width:150px;height:150px;' " +
     "mea_widgetname='"+_this.gaugeName+"' " +
     "data-widgetparams='"+JSON.stringify(_this.params)+"' " +
     "mea_valueformater='mea_gauge' " +
">" +
"</div>";
return html;
}
