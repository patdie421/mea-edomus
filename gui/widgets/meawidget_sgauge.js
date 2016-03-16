extendClass(MeaWidget_sgauge, MeaWidget);

var meaWidget_sgauge = new MeaWidget_sgauge("MeaWidget_sgauge", "basic", "sgauge");

function MeaWidget_sgauge(name, group, type)
{
   MeaWidget_sgauge.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);
   this.sgaugeName = "sgauge";

   this.params["values"]["value"]=0;
   this.params["parameters"]["title"] = '';
   this.params["parameters"]["unit"] = '';
   this.params["parameters"]["min"] = 0;
   this.params["parameters"]["max"] = 100;
   this.params["parameters"]["stop1"] = 25; 
   this.params["parameters"]["stop2"] = 50;
   this.params["parameters"]["stop3"] = 75;
   this.params["parameters"]["color1"] = '#55BF3B'; // green
   this.params["parameters"]["color2"] = '#DDDF0D'; // yellow 
   this.params["parameters"]["color3"] = '#DF5353'; // red
}


MeaWidget_sgauge.prototype.disabled = function(id, d)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   if(d===true)
   {
      widget.removeClass("mea_sgauge_enabled");
      widget.addClass("mea_sgauge_disabled");
   }
   else
   {
      widget.removeClass("mea_sgauge_disabled");
      widget.addClass("mea_sgauge_enabled");
   }
}


MeaWidget_sgauge.prototype.mksgauge = function(widget, data)
{
   var _this = this;
   var value=0,
       title = _this.params["parameters"]["title"],
       min   = _this.params["parameters"]["min"],
       max   = _this.params["parameters"]["max"],
       stop1 = _this.params["parameters"]["stop1"],
       stop2 = _this.params["parameters"]["stop2"],
       stop3 = _this.params["parameters"]["stop3"],
       unit  = _this.params["parameters"]["unit"],
       color1= _this.params["parameters"]["color1"],
       color2= _this.params["parameters"]["color2"],
       color3= _this.params["parameters"]["color3"];
   if(data)
   {
      title  = _this.getValue(data,"title", title);
      min    = _this.getValue(data,"min", min)*1;
      max    = _this.getValue(data,"max", max)*1;
      stop1  = _this.getValue(data,"stop1", stop1)*1;
      stop2  = _this.getValue(data,"stop2", stop2)*1;
      stop3  = _this.getValue(data,"stop3", stop3)*1;
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

   var _stop1 = (stop1 - min) / (max - min);
   if(_stop1 < 0 || _stop1 > 1)
      _stop1 = 0;

   var _stop2 = (stop2 - min) / (max - min);
   if(_stop2 < 0 || _stop2 > 1)
      _stop2 = 0;

   var _stop3 = (stop3 - min) / (max - min);
   if(_stop3 < 0 || _stop3 > 1)
      _stop3 = 0;

   var stops = [_stop1, _stop2, _stop3].sort(function(a, b) { return a - b; });
   
   var gaugeOptions = {
        credits: {
           enabled: false
        },

        chart: {
            animation: false,
            type: 'solidgauge',
            spacingTop: 0,
            spacingLeft: 0,
            spacingRight: 1,
            spacingBottom: 1,

            backgroundColor:'rgba(255, 255, 255, 0)',
        },

        title: null,

        pane: {
            center: ['50%', '85%'],
            size: '100%',
            startAngle: -90,
            endAngle: 90,
            background: {
                backgroundColor: (Highcharts.theme && Highcharts.theme.background2) || '#EEE',
                innerRadius: '60%',
                outerRadius: '100%',
                shape: 'arc'
            }
        },

        tooltip: {
            enabled: false
        },

        // the value axis
        yAxis: {
            animation: false,
            min: 0,
            max: 100,
            stops: [
                [stops[0], color1],
                [stops[1], color2],
                [stops[2], color3]
            ],
            lineWidth: 0,
            minorTickInterval: null,
            tickPixelInterval: 400,
            tickWidth: 0,
            title: {
                y: -50,
                text: title
            },
            labels: {
                y: 16,
                formatter: function () {
                    return this.value ? max+unit : min+unit
                }
            }
        },

        plotOptions: {
            solidgauge: {
                dataLabels: {
                    y: 5,
                    borderWidth: 0,
                    useHTML: true
                }
            }
        },

        series: [{
            name: 'data',
            data: [1],
            dataLabels: {
                formatter: function() {
                   var v = Highcharts.numberFormat((this.y) /100 * (max - min) + min, 2);
                   return "<div style='text-align:center'>" +
                             "<span style='font-size:16px;color:" + ((Highcharts.theme && Highcharts.theme.contrastTextColor) || 'black') + "'>" +
                             v + unit +
                             "</span>" +
                          "</div>";
                }
            },
        }]

    };

    widget.highcharts(gaugeOptions);
}


MeaWidget_sgauge.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   _this.mksgauge(widget, data);

   if(data)
   {
      var vn = _this.getValue(data, "value", false);
      if(vn !== false)
         widget.attr("name", vn);
   }
}


MeaWidget_sgauge.prototype.getTip = function()
{
   return "sgauge";
}


MeaWidget_sgauge.prototype.getFormaters = function()
{
   var _this = this;
   var formaters = {
      mea_sgauge: function(v,o) {
         var data = o.prop('mea-widgetdata');
         var min = _this.getValue(data, "min", 0);
         var max = _this.getValue(data, "max", 100);
         var point = o.highcharts().series[0].points[0];  
         if(isNaN(v))
            v=0;
         v = (v-min)/(max - min)*100;
         point.update(v);
      }
   };

   return formaters;
}


MeaWidget_sgauge.prototype.getActions = function()
{
   var _this = this;
}


MeaWidget_sgauge.prototype.getStyle = function()
{
var style = 
".mea_sgauge_disabled {" +
   "border-style: dotted;" +
   "border-width: 1px;" +
   "border-color: gray;" +
   "opacity: 0.5;" +
"}" +
".mea_sgauge_enabled {" + 
   "border-style: solid;" +
   "border-width: 1px;" +
   "border-color: transparent;" +
   "opacity: 1;" +
"}";
return style;
}


MeaWidget_sgauge.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width: 50px; height: 50px; background:rgb(180, 95, 60);'></div>";

   return html;
}


MeaWidget_sgauge.prototype.getHtml = function()
{
var _this = this;
var html = 
"<div id = '"+_this.type+"_model' " +
     "class = 'mea-widget mea_sgauge_disabled' " +
     "mea_widget = '"+_this.type+"' " +
     "style = 'position:absolute;width:150px;height:150px;' " +
     "mea_widgetname='"+_this.sgaugeName+"' " +
     "data-widgetparams='"+JSON.stringify(_this.params)+"' " +
     "mea_valueformater='mea_sgauge' " +
">" +
"</div>";
return html;
}
