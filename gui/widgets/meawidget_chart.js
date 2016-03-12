extendClass(MeaWidget_chart4, MeaWidget);

var meaWidget_chart4 = new MeaWidget_chart4("MeaWidget_chart4", "charts", "chart4");

function MeaWidget_chart4(name, group, type)
{
   MeaWidget_chart4.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);
   this.chartName = "chart";

   this.minwidth=540;
   this.minheight=400;

   this.params["parameters"]["title"] = '';
   this.params["parameters"]["subtitle"] = '';
   this.params["parameters"]["width"] = this.minwidth;
   this.params["parameters"]["height"] = this.minheight;

   this.params["parameters"]["sensor1_id"] = 0;
   this.params["parameters"]["collector1_id"] = 0;
   this.params["parameters"]["sensor2_id"] = 0;
   this.params["parameters"]["collector2_id"] = 0;
   this.params["parameters"]["sensor3_id"] = 0;
   this.params["parameters"]["collector3_id"] = 0;
   this.params["parameters"]["sensor4_id"] = 0;
   this.params["parameters"]["collector4_id"] = 0;
}


MeaWidget_chart4.prototype.disabled = function(id, d)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   if(d===true)
   {
      widget.removeClass("mea_chart4_enabled");
      widget.addClass("mea_chart4_disabled");
      try
      {
         var chart = widget.highcharts();
         chart.destroy();
         widget.append("<div class='mea_dragzone_for_resizable' style='width:calc( 100% - 12px );height:calc( 100% - 12px );'><div>");
      }
      catch(e) {};
   }
   else
   {
      widget.removeClass("mea_chart4_disabled");
      widget.addClass("mea_chart4_enabled");
      _this.mkchart(widget, data);
   }
}


MeaWidget_chart4.prototype.mkchart = function(widget, data)
{
   var _this = this;
   if(!data)
      return;

   var sensors=[];
   sensors[0] = _this.getValue(data,"sensor1_id", 0);
   sensors[1] = _this.getValue(data,"sensor2_id", 0);
   sensors[2] = _this.getValue(data,"sensor3_id", 0);
   sensors[3] = _this.getValue(data,"sensor4_id", 0);

   var title = _this.getValue(data,"title", -1);
   var subtitle = _this.getValue(data,"subtitle", -1);
   var afterSetExtremes_on = false;

   console.log(sensors);
   function afterSetExtremes(e)
   {
      if(afterSetExtremes_on === false)
         return;
      afterSetExtremes_on = false;

      var chart = widget.highcharts();
      chart.showLoading('Loading data from server...');
      var _i=0;
      var max=sensors.length-1;
      for(var i=0;i<sensors.length;i++)
      {
         if(sensors[i]===0)
         {
            max--;
            continue;
         }
         $.getJSON('models/get_msql_data.php?sensor_id='+sensors[i]+'&start=' + Math.round(e.min) + '&end=' + Math.round(e.max) + '&callback=?', function (data) {
            chart.series[_i].setData(data);
            if(_i===max)
            {
               chart.hideLoading();
               afterSetExtremes_on=true;
            }
            _i++;
         })
         .fail(function( jqxhr, textStatus, error ) {
            var err = textStatus + ", " + error;
            console.log( "Request Failed: " + err );
            _i++;
         });
      }
   }
   

   // create the chart
   widget.highcharts('StockChart', {
      chart : {
//         type: 'arearange',
         zoomType: 'x'
      },

      navigator : {
         adaptToUpdatedData: false,
         series : {
            id: "navigation"
         }
      },

      scrollbar: {
         liveRedraw: false
      },
      title: {
         text: title
      },

      subtitle: {
         text: subtitle
      },
      rangeSelector : {
         buttons: [{
            type: 'hour',
            count: 1,
            text: '1h'
         }, {
            type: 'day',
            count: 1,
            text: '1d'
         }, {
            type: 'month',
            count: 1,
            text: '1m'
         }, {
            type: 'year',
            count: 1,
            text: '1y'
         }, {
            type: 'all',
            text: 'All'
         }],
         inputEnabled: false, // it supports only days
         selected : 4 // all
      },

      xAxis : {
         events : {
            afterSetExtremes : afterSetExtremes
         },
         minRange: 1 * 3600 * 1000 // 1 hour
      },

      yAxis: {
         floor: 0
      },

      series : [{
         dataGrouping: {
            enabled: false
         }
      },{
         dataGrouping: {
            enabled: false
         }
      },{
         dataGrouping: {
            enabled: false
         }
      }]
   });

   var chart = widget.highcharts();
   var nav = chart.get('navigation');

   var _i=0; 
   var max = sensors.length-1;
   var navset=false;
   for(var i=0;i<sensors.length;i++)
   {
      if(sensors[i]===0)
      {
         max--;
         continue;
      }

      $.getJSON('models/get_msql_data.php?sensor_id='+sensors[i]+'&callback=?', function (data) {
         chart.series[_i].setData(data); 
         if(navset===false)
         {
            navset=true;
            nav.setData(data);
         }
         if(_i===max)
            afterSetExtremes_on=true;
         _i++;
      })
      .fail(function( jqxhr, textStatus, error ) {
         var err = textStatus + ", " + error;
         console.log( "Request Failed: " + err );
         _i++;
      });
   }
}


MeaWidget_chart4.prototype.init = function(id)
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
   }
}


MeaWidget_chart4.prototype.getTip = function()
{
   return "chart";
}


MeaWidget_chart4.prototype.getFormaters = function()
{
   var _this = this;
   var formaters = {
      mea_chart: function(v,o) {
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


MeaWidget_chart4.prototype.getActions = function()
{
   var _this = this;
}


MeaWidget_chart4.prototype.getStyle = function()
{
var style = 
".mea_chart4_disabled {" +
   "background:url('/widgets/meawidget_chart.svg') rgb(243, 243, 243);" +
   "background-repeat:no-repeat;" +
   "background-position:center;" +
   "background-size:contain;" +
   "border-style: dotted;" +
   "border-width: 1px;" +
   "border-color: gray;" +
"}" +
".mea_chart4_enabled {" + 
   "border-style: solid;" +
   "border-width: 1px;" +
   "border-color: transparent;" +
   "background-color: rgb(243, 243, 243);" +
   "opacity: 1;" +
"}";
return style;
}


MeaWidget_chart4.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width: 50px; height: 50px; background:rgb(180, 95, 60);'></div>";

   return html;
}


MeaWidget_chart4.prototype.getHtml = function()
{
var _this = this;
var html = 
"<div id = '"+_this.type+"_model' " +
     "class = 'mea-widget mea_chart4_disabled' " +
     "mea_widget = '"+_this.type+"' " +
     "style = 'position:absolute;width:"+_this.minwidth+"px;height:"+_this.minheight+"px;' " +
     "mea_widgetname='"+_this.chartName+"' " +
     "data-widgetparams='"+JSON.stringify(_this.params)+"' " +
">" +
   "<div class='mea_dragzone_for_resizable' style='margin:6px;width:calc( 100% - 12px );height:calc( 100% - 12px );'><div>" +
"</div>";
return html;
}
