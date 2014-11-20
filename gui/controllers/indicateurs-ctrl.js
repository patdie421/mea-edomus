var indicatorsTable = {
   compteur: 0,
   started: 0,
   indicatorsToExclude: ["desc","pid","status","type","group","heartbeat"],
   
   isStarted: function()
   {
      return this.started;
   },
   
  add_indicator: function(table_id, service_name, indicator_name, indicator_description, indicator_value)
   {
      $("#" + table_id).datagrid('appendRow', {
         Service: service_name,
         Indicateur: indicator_name,
         Description: indicator_description,
         Valeur: "<div id='value_"+service_name+"_"+indicator_name+"'>"+indicator_value+"</div>"
     });
   },


   build: function(table, services)
   {
      $("#"+table).datagrid('loadData', []);
      var nb=0;
      for(var service in services)
      {
         if(services[service].length>0)
          {
             for(var indicator in services[service])
             {
                this.add_indicator(table, service, services[service][indicator], "","N/A");
                nb++;
             }
         }
      }
      return nb;
   },
   
   load: function(table)
   {

      _indicatorsTable=this;
      $.ajax({
         url: 'CMD/indicators.php',
         async: true,
         type: 'GET',
         dataType: 'json',
         success: function(data){
//            $("#"+table).empty();
            var ret=_indicatorsTable.build(table, data);
            if(ret<=0)
            {
               console.log("erreur");
               // mettre un not available ...
            }
         },
         error: function(jqXHR, textStatus, errorThrown ){
            ajax_error( jqXHR, textStatus, errorThrown );
         }
      });
      this.started=1;
   },

   reload: function(table,processes)
   {
      this.load(table);
   },
   
   update_indicator: function (service, indicator, value)
   {
      $("#value_"+service+"_"+indicator).text(value);
   },
   
   update_all_indicators: function(services)
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
               indicatorsTable.update_indicator(service, indicator, services[service][indicator]);
         }
      }
   }
}


function indicateurs_controller(table_id)
{
   indicatorsTable.load(table_id);
}

