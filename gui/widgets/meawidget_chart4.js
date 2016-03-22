extendClass(MeaWidget_chart4, MeaWidget);

var meaWidget_chart4 = new MeaWidget_chart4("MeaWidget_chart4", "charts", "chart4");

function MeaWidget_chart4(name, group, type)
{
   MeaWidget_chart4.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);
   this.chartName = "chart4";

   this.minwidth=540;
   this.minheight=400;

   this.params["positions"]["minWidth"] = this.minwidth;
   this.params["positions"]["minHeight"] = this.minheight;

   this.params["parameters"]["title"] = '';
   this.params["parameters"]["subtitle"] = '';
   this.params["parameters"]["width"] = this.minwidth;
   this.params["parameters"]["height"] = this.minheight;

   this.sensors_list = false;
   this.sensors =  {
      "val" : "",
      "editor": {
         "type":"combobox",
         "options": {
             panelMinWidth:300,
             valueField:'id',
             textField:'text',
             groupField:'group',
             method:'get',
             url:'models/get_mysql_sensors_list.php',
         }
      }
   };

   this.params["parameters"]["sensor1"] = this.sensors;
   this.params["parameters"]["sensor2"] = this.sensors;
   this.params["parameters"]["sensor3"] = this.sensors;
   this.params["parameters"]["sensor4"] = this.sensors;

   this.dragzone_str = "<div class='mea_dragzone_for_resizable' style='position:absolute;top:6px;left:6px;width:calc( 100% - 12px );height:calc( 100% - 12px );opacity:0.0'><div>";
}


MeaWidget_chart4.prototype.disabled = function(id, d)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');
   var chartdiv = widget.find('div[mea_widgetvaluename="'+_this.chartName+'"]');

   if(d===true)
   {
      try
      {
         var chart = chartdiv.highcharts();
         chart.destroy();
      }
      catch(e) {};

      widget.removeClass("mea_chart4_enabled");
      chartdiv.removeClass("mea_chart4_bg_enabled");
      widget.addClass("mea_chart4_disabled");
      chartdiv.addClass("mea_chart4_bg_disabled");
      widget.append(_this.dragzone_str);
   }
   else
   {
      widget.removeClass("mea_chart4_disabled");
      chartdiv.removeClass("mea_chart4_bg_disabled");
      widget.addClass("mea_chart4_enabled");
      chartdiv.addClass("mea_chart4_bg_enabled");
      _this.mkchart(chartdiv, data);
      widget.find('.mea_dragzone_for_resizable').remove();
   }
}


MeaWidget_chart4.prototype.mkchart = function(widget, data)
{
   var _this = this;
   if(!data)
      return;

   var sensors=[];
   sensors[0] = _this.getValue(data,"sensor1", 0);
   sensors[1] = _this.getValue(data,"sensor2", 0);
   sensors[2] = _this.getValue(data,"sensor3", 0);
   sensors[3] = _this.getValue(data,"sensor4", 0);

   var title = _this.getValue(data,"title", -1);
   var subtitle = _this.getValue(data,"subtitle", -1);
   var afterSetExtremes_on = false;

   var ids=new Array();
   var series_names = ["","","",""];
   var j=0;
   for(var i=0;i<sensors.length;i++)
   {
      if(sensors[i]===0)
         continue;

      id = false;
      try
      {
         id=JSON.parse(sensors[i]);
      }
      catch(e)
      {
         continue;
      }

      if(id===false || id.sensor_id < 0)
      {
         continue;
      }
      ids[j]=id;
      series_names[j]=id.text2;
      j++;
   }

   function afterSetExtremes(e)
   {
      if(afterSetExtremes_on === false)
         return;
      afterSetExtremes_on = false;

      var chart = widget.highcharts();

      var max = ids.length - 1; 
      if(max < 0)
         return;

      var _i=0;
      chart.hideNoData();
      chart.showLoading('Loading data from server...');

      for(var i=0;i<ids.length;i++)
      {
         $.getJSON('models/get_mysql_data.php?id='+i+'&sensor_id='+ids[i].sensor_id+'&collector_id='+ids[i].collector_id+'&start=' + Math.round(e.min) + '&end=' + Math.round(e.max) + '&callback=?', function (data) {
            chart.series[data.id].setData(data.data);
            if(_i===max)
            {
               afterSetExtremes_on=true;
               chart.hideLoading();
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
         type: 'datetime',
         ordinal: false,
         events : {
            afterSetExtremes : afterSetExtremes
         },
         minRange: 1 * 3600 * 1000 // 1 hour
      },

      yAxis: {
          plotLines: [{
             color: 'red',
             width: 1,
             dashStype: "Solid",
             value: 0.0,
             zIndex: 1
         }]
      },

      series : [{
         name: series_names[0],
         dataGrouping: {
            enabled: false
         }
      },{
         name: series_names[1],
         dataGrouping: {
            enabled: false
         }
      },{
         name: series_names[2],
         dataGrouping: {
            enabled: false
         }
      },{
         name: series_names[3],
         dataGrouping: {
            enabled: false
         }
      }]
   });

   var chart = widget.highcharts();
   var nav = chart.get('navigation');

   var _i=0; 
   var max = ids.length-1;
   var navset=false;

   if(max < 0)
      return;
   chart.hideNoData();
   chart.showLoading('Loading data from server...');
   for(var i=0;i<ids.length;i++)
   {
      $.getJSON('models/get_mysql_data.php?id='+i+'&sensor_id='+ids[i].sensor_id+'&collector_id='+ids[i].collector_id+'&callback=?', function (data) {
         chart.series[data.id].setData(data.data); 
         if(navset===false)
         {
            navset=true;
            nav.setData(data.data);
         }
         if(_i===max)
         {
            afterSetExtremes_on=true;
            chart.hideLoading();
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
   return "chart up to 4 values";
}


MeaWidget_chart4.prototype.getStyle = function()
{
var style = 
".mea_chart4_disabled {" +
   "background-color: rgb(243, 243, 243);" +
   "border-style: dotted;" +
   "border-width: 1px;" +
   "border-color: gray;" +
"}" +
".mea_chart4_enabled {" + 
   "background-color: rgb(243, 243, 243);" +
   "border-style: solid;" +
   "border-width: 1px;" +
   "border-color: transparent;" +
   "opacity: 1;" +
"}" +
".mea_chart4_bg_disabled {" +
   "background:url('/widgets/meawidget_chart.svg') rgb(243, 243, 243);" +
   "background-repeat:no-repeat;" +
   "background-position:center;" +
   "background-size:contain;" +
"}" +
".mea_chart4_bg_enabled {" +
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
   "<div style='postion:relative;height:100%;width:100%;display:table;'>" +
      "<div style='height:25px;width:100%;display:table-row;'></div>" + 
      "<div style='height:100%;width:100%;display:table-row;'>" +
         "<div class='mea_chart4_bg_disabled' mea_widgetvaluename='"+_this.chartName+"' style='height:100%;width:100%;display:block'>" +
         "</div>" +
      "</div>" + 
   "</div>" +
   _this.dragzone_str +
"</div>";
return html;
}
