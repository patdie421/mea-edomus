extendClass(MeaWidget_rtchart, MeaWidget);

var meaWidget_rtchart = new MeaWidget_rtchart("MeaWidget_rtchart", "charts", "rtchart");

function MeaWidget_rtchart(name, group, type)
{
   MeaWidget_rtchart.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);
   this.chartName = "chart";

   this.minwidth=540;
   this.minheight=400;

   this.params["values"]["value"]=0;
   this.params["parameters"]["title"] = '';
   this.params["parameters"]["subtitle"] = '';
   this.params["parameters"]["width"] = this.minwidth;
   this.params["parameters"]["height"] = this.minheight;
}


MeaWidget_rtchart.prototype.disabled = function(id, d)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   _this.isdisabled=d;
   if(d===true)
   {
      widget.removeClass("mea_rtchart_enabled");
      widget.addClass("mea_rtchart_disabled");
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
      widget.removeClass("mea_rtchart_disabled");
      widget.addClass("mea_rtchart_enabled");
      _this.mkchart(widget, data);
   }
}


MeaWidget_rtchart.prototype.mkchart = function(widget, data)
{
   var _this = this;
   if(!data)
      return;
   var t = (new Date()).getTime();
   widget.highcharts({
        title: {
            text: 'Snow depth at Vikjafjellet, Norway'
        },
        subtitle: {
            text: 'Irregular time data in Highcharts JS'
        },
        xAxis: {
            type: 'datetime',
            dateTimeLabelFormats: { // don't display the dummy year
                month: '%e. %b',
                year: '%b'
            }
        },
        series: [{
            data: []
        }]
    });
}


MeaWidget_rtchart.prototype.init = function(id)
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

      var vn = _this.getValue(data, "value", false);
      if(vn !== false)
         widget.attr("name", vn);
   }
}


MeaWidget_rtchart.prototype.getTip = function()
{
   return "real time chart";
}


MeaWidget_rtchart.prototype.getFormaters = function()
{
   var _this = this;
   var formaters = {
      mea_rtchart: function(v,o) {

         var chart=o.highcharts();
         var serie=chart.series[0];

         if(serie.data.length > 100)
            serie.data[0].remove(false, false); 

         var t = (new Date()).getTime();

         chart.xAxis[0].update({max : t});
         chart.xAxis[0].update({min : t - 1000*60*60});

         serie.addPoint([t,v]);
      }
   };

   return formaters;
}


MeaWidget_rtchart.prototype.getActions = function()
{
   var _this = this;
}


MeaWidget_rtchart.prototype.getStyle = function()
{
var style = 
".mea_rtchart_disabled {" +
   "background:url('/widgets/meawidget_chart.svg') rgb(243, 243, 243);" +
   "background-repeat:no-repeat;" +
   "background-position:center;" +
   "background-size:contain;" +
   "border-style: dotted;" +
   "border-width: 1px;" +
   "border-color: gray;" +
"}" +
".mea_rtchart_enabled {" + 
   "border-style: solid;" +
   "border-width: 1px;" +
   "border-color: transparent;" +
   "background-color: rgb(243, 243, 243);" +
   "opacity: 1;" +
"}";
return style;
}


MeaWidget_rtchart.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width: 80px; height: 80px; background:rgb(180, 60, 95);'></div>";

   return html;
}


MeaWidget_rtchart.prototype.getHtml = function()
{
var _this = this;
var html = 
"<div id = '"+_this.type+"_model' " +
     "class = 'mea-widget mea_rtchart_disabled' " +
     "mea_widget = '"+_this.type+"' " +
     "style = 'position:absolute;width:"+_this.minwidth+"px;height:"+_this.minheight+"px;' " +
     "mea_widgetname='"+_this.chartName+"' " +
     "mea_valueformater='mea_rtchart' " +
     "data-widgetparams='"+JSON.stringify(_this.params)+"' " +
">" +
   "<div class='mea_dragzone_for_resizable' style='margin:6px;width:calc( 100% - 12px );height:calc( 100% - 12px );'><div>" +
"</div>";
return html;
}
