var indicatorsTable = {
   compteur: 0,
   started: 0,
   indicatorsToExclude: ["desc","pid","status","type","group","heartbeat"],
   
   isStarted: function()
   {
      return this.started;
   },
   
   add_process: function (table, name)
   {
      newRow = "<tr>" +
                   "<td class=\"processes_column cell_table_indicateurs\">" +
                      "<div class=\"process_name\">"+name+"</div>" +
                      "<div id=\"desc_"+name+"\" class=\"process_description\"></div>" +
                   "</td>" +
                   "<td class=\"status_column cell_table_indicateurs\">" +
                      "<div class=\"process_status\">" +
                         "<div id=\"status_"+name+"\" class=\"pastille ui-widget ui-widget-content ui-corner-all\" style=\"background:gray; margin:auto;\"></div>" +
                      "</div>" +
                   "</td>" +
                   "<td class=\"indicators_column cell_table_indicateurs\"><div>" +
                      "<table border=\"0\" class=\"indicators_list\" id=\""+name+"\">" +
                      "</table>" +
                   "</div></td>" +
               "</tr>";
      $("#"+table).append(newRow);
   },

   add_indicator: function (process, name, indicator_name)
   {
      var couleur;
      if(this.compteur % 2 == 0)
          couleur=1;
       else
          couleur=2;

      newRow = "<tr class=\"couleur_"+couleur+"\">" +
                  "<td class=\"indicator_name\"><div>"+name+"</div></td>"+
                  "<td class=\"indicator_value\"><div id=\""+process+"_"+indicator_name+"\"\">N/A</div></td>" +
               "</tr>";
      $("#"+process).append(newRow);
      this.compteur++;
   },

   build: function(table, processes)
   {
      var nb=0;
      for(var process in processes)
      {
         if(processes[process].length>0)
          {
             this.add_process(table, process);
             for(var indicator in processes[process])
             {
                this.add_indicator(process, processes[process][indicator], processes[process][indicator]);
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
            $("#"+table).empty();
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
      $("#"+table).empty();
      this.load(table);
   },
   
   update_indicator: function (process, indicator, value)
   {
      $("#"+process+"_"+indicator).text(value);
   },
   
   update_all_indicators: function(data)
   {
      for(process in data)
      {
         if(data[process]['status']==1) // process démarré
            if(data[process]['heartbeat']=='KO' && data[process]['type']!=2) // type = 2 => tache donc pas de "black" possible
               $("#status_"+process).css("background","black");
            else
               $("#status_"+process).css("background","green");
         else if(data[process]['status']==0 && data[process]['type']!=2) // non demarré
            $("#status_"+process).css("background","red");
         else
            $("#status_"+process).css("background","gray");
         
         for(indicator in data[process])
         {
            if(this.indicatorsToExclude.indexOf(indicator) == -1)
               indicatorsTable.update_indicator(process, indicator, data[process][indicator]);
         }
      }
   }
}


function indicateurs_controller(table_id)
{
   indicatorsTable.load(table_id);
}

