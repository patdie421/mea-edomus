/*
if(typeof(CommonController)=="undefined") {// GridController héritera de CommonController ...
   window.location = "error.html";
}
*/

function IndicatorsTableController(table_id)
{
   IndicatorsTableController.superConstructor.call(this);

   this.compteur=0;
   this.started=0;
   this.table_id=table_id;
}
 
extendClass(IndicatorsTableController, CommonController);
 

IndicatorsTableController.prototype.indicatorsToExclude=["desc","pid","status","type","group","heartbeat"];
   
IndicatorsTableController.prototype.isStarted=function()
{
   return this.started;
};

 
IndicatorsTableController.prototype._add_indicator=function(table_id, service_name, indicator_name, indicator_description, indicator_value)
{
   $("#" + table_id).datagrid('appendRow', {
      Service: service_name,
      Indicateur: indicator_name,
      Description: indicator_description,
      Valeur: "<div id='value_"+service_name+"_"+indicator_name+"'>"+indicator_value+"</div>"
   });
};


IndicatorsTableController.prototype.build=function(services)
{
   var table_id = this.table_id;
   $("#"+table_id).datagrid('loadData', []);
   var nb=0;
   for(var service in services)
   {
      if(services[service].length>0)
       {
          for(var indicator in services[service])
          {
             this._add_indicator(table_id, service, services[service][indicator], "","N/A");
             nb++;
          }
       }
    }
    return nb;
};

   
IndicatorsTableController.prototype.load=function()
{
   var _indicatorsTable=this;
   $.ajax({
      url: 'CMD/indicators.php',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         var ret=_indicatorsTable.build(data);
         if(ret<=0)
         {
            console.log("error");
            // mettre un not available ...
         }
      },
      error: function(jqXHR, textStatus, errorThrown ){
         $.messager.show({
            title:_controller._toLocalC('error')+_controller._localDoubleDot(),
            msg: _controller._toLocalC("communication error")+' ('+textStatus+')'
         });
      }
   });
   this.started=1;
};


IndicatorsTableController.prototype.reload=function()
{
   this.load();
},


IndicatorsTableController.prototype.update_indicator=function (service, indicator, value)
{
   $("#value_"+service+"_"+indicator).text(value);
};
   

IndicatorsTableController.prototype.update_all_indicators=function(services)
{
   for(service in services)
   {
      if(services[service]['status']==1) // process démarré
         if(services[service]['heartbeat']=='KO' && services[service]['type']!=2) // type = 2 => tache donc pas de "black" possible
            $("#status_"+service).css("background","black");
         else
            $("#status_"+service).css("background","green");
      else if(services[service]['status']==0 && services[service]['type']!=2) // non demarré
         $("#status_"+service).css("background","red");
      else
         $("#status_"+service).css("background","gray");
         
      for(indicator in services[service])
      {
         if(this.indicatorsToExclude.indexOf(indicator) == -1)
            this.update_indicator(service, indicator, services[service][indicator]);
      }
   }
},


IndicatorsTableController.prototype.__mon_listener=function(message)
{
   var data = jQuery.parseJSON(message);

   try {
      this.update_all_indicators(data);
   }
   catch(ex) {
   }
};


IndicatorsTableController.prototype.__rel_listener=function(message)
{
   try {
      this.reload();
   }
   catch(ex) {
   }
};
