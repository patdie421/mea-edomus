/*
if(typeof(CommonController)=="undefined")  {// ControlPanel héritera de CommonController ... qui doit donc être défini !
   window.location = "error5.html";
}
*/
function ControlPanel()
{
   ControlPanel.superConstructor.call(this);
   
   this.table_reload="";
   this.table_interfaces="";
   this.table_processes="";
}

extendClass(ControlPanel, CommonController); // héritage de CommonController


ControlPanel.prototype.init=function(table_reload, table_interfaces, table_processes)
{
   this.table_reload="#"+table_reload;
   this.table_interfaces="#"+table_interfaces;
   this.table_processes="#"+table_processes;   
};
   

ControlPanel.prototype.load=function() {
   var _controlPanel=this;
   var retour = 0;
   var isadmin = _controlPanel._isAdmin();

   $.ajax({
      url: 'CMD/ps.php',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         var nb=0;
         if(_controlPanel.checkError(data)!=false)
         {
            for(var key in data)
            {
               if(data[key]['type']==1)
                  continue;
               if(data[key]['group']==0)
               {
                  _controlPanel.add_row(_controlPanel.table_processes, key, data[key]['desc'], data[key]['pid'], "start", _controlPanel.startTask, "stop", _controlPanel.stopTask);
               }
               else if(data[key]['group']==2)
               {
                  _controlPanel.add_row(_controlPanel.table_reload, key, data[key]['desc'], data[key]['pid'], "reload", _controlPanel.reloadTask, null, null);
               }
               else if(data[key]['group']==1)
               {
                  nb++;
                  _controlPanel.add_row(_controlPanel.table_interfaces, key, data[key]['desc'], data[key]['pid'], "start", _controlPanel.startTask, "stop", _controlPanel.stopTask);
               }
            }
            if(nb==0)
               _controlPanel.messageListeInterfaceVide(_controlPanel.table_interfaces);
            _controlPanel.update_status(data);
         }
      },
      error: function(jqXHR, textStatus, errorThrown ){
         $.messager.show({
            title: _controlPanel._toLocalC('error')+_controlPanel._localDoubleDot(),
            msg: _controlPanel._toLocalC('communication error')+' ('+textStatus+')',
         });
      }
   });
};


ControlPanel.prototype.add_row=function(table, name, desc, id, start_str, start, stop_str, stop)
{
   var _controlPanel=this;
   var isadmin = _controlPanel._isAdmin();
   newRow =  "<tr>" +
                 "<td style=\"width:10%;height:40px;\"><div style='width:40px;text-align:center;'><div id=\"process_"+id+"\" class=\"pastille\" style=\"background:gray;\"></div></div></td>" +
                 "<td style=\"width:60%;\">" +
                     "<div class=\"process\">" +
                        "<div class=\"name\">"+name+"</div>";
   if(desc!="" && desc!==undefined)
   {
                newRow+="<div class=\"description\">"+desc+"</div>";
   }
             newRow+="</div></td>" +
                 "<td style=\"width:30%;\">";
   if(isadmin==1)
   {
             newRow+="<div class='bouton'>";
   if(start != null)
      {
             newRow+="<a id=\"bstart"+id+"\" href=\"#\">"+start_str+"</a>";
      }
      if(start != null && stop != null)
      {
             newRow+="&nbsp;";
      }
      if(stop != null)
      {
             newRow+="<a id=\"bstop"+id+"\" href=\"#\">"+stop_str+"</a>";
      }
           newRow+="</div>";
   }
         newRow+="</td>" +
             "</tr>";
   $(table).append(newRow);
   if(start != null)
   {
      var icon_str="icon-ok";
      if(name=="RELOAD")
         icon_str="icon-reload";
      $("#bstart"+id).linkbutton({
         plain: false,
         iconCls: icon_str,
      });
      $("#bstart"+id).click(function(event){start(_controlPanel,id);});
   }
   if(stop != null)
   {
      $("#bstop"+id).linkbutton({
         plain: false,
         iconCls: 'icon-cancel',
      });
      $("#bstop"+id).click(function(event){stop(_controlPanel,id);});
   }
},


ControlPanel.prototype.update_status=function(data)
{
   for(var key in data)
   {
      if(data[key]['status']==1) // process démarré
      {
         if(data[key]['heartbeat']=='KO' && data[key]['type']!=2) // type = 2 => tache donc pas de "black" possible
         {
            $("#process_"+data[key]['pid']).css("background","black");
         }
         else
         {
            $("#process_"+data[key]['pid']).css("background","green");
         }
         if(data[key]['type']!=2)
         {
            $("#bstart"+data[key]['pid']).linkbutton('disable');
            $("#bstop"+data[key]['pid']).linkbutton('enable');
         }
      }
      else if(data[key]['status']==0 && data[key]['type']!=2) // non demarré
      {
         $("#process_"+data[key]['pid']).css("background","red");
         $("#bstart"+data[key]['pid']).linkbutton('enable');
         $("#bstop"+data[key]['pid']).linkbutton('disable');
      }
      else
      {
         $("#process_"+data[key]['pid']).css("background","gray");
      }
   }
};


ControlPanel.prototype.messageListeInterfaceVide=function(table)
{
   var newRow="";
   newRow = "<tr>" +
               "<td style=\"width:100%; height:40px; text-align:center;\"><div>"+this._toLocalC('no interface available')+"</div></td>" +
            "</tr>";
   $("#"+table+" > tbody").before(newRow);
};


ControlPanel.prototype.load_interfaces_list_only=function()
{
   var _controlPanel=this;
   var isadmin = _controlPanel._isAdmin();
   $(_controlPanel.table_interfaces).empty();

   $.ajax({
      url: 'CMD/ps.php',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         var nb=0;
         if(_controlPanel.checkError(data)!=false)
         {
            for(var key in data)
            {
               if(data[key]['group']==1)
               {
                  _controlPanel.add_row(_controlPanel.table_interfaces, key, data[key]['desc'], data[key]['pid'], "start", _controlPanel.startTask, "stop", _controlPanel.stopTask);
                  nb++;
               }
            }
            if(nb>0)
               _controlPanel.update_status(data);
            else
               _controlPanel.messageListeInterfaceVide(_controlPanel.table_interfaces);
         }
      },
      error: function(jqXHR, textStatus, errorThrown) {
         $.messager.show({
            title: _controlPanel._toLocalC('error'),
            msg: _controlPanel._toLocalC('communication error')+' ('+textStatus+')',
         });
      }
   });
};


ControlPanel.prototype.reloadTask=function(_controlPanel,id)
{
   var isadmin = _controlPanel._isAdmin();
   
   if(isadmin != true)
      return -1;
      
   // on met la roue qui tourne ...
   $(_controlPanel.table_interfaces).empty();
   $(_controlPanel.table_interfaces).append("<tr><td style=\"width:100%; margin:auto;\" align=\"center\" valign=\"top\"><div class=\"wait_ball\"></div></td></tr>");
   $.ajax({
      url: 'CMD/startstop.php?process='+id+'&cmnd=task',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         _controlPanel.checkError(data);
         $(_controlPanel.table_interfaces).empty();
      },
      error: function(jqXHR, textStatus, errorThrown ) {
         $.messager.show({
            title: _controlPanel._toLocalC('error')+_controlPanel._localDoubleDot(),
            msg: _controlPanel._toLocalC('communication error')+' ('+textStatus+')',
         });
      }
   });
   
   return 0;
};

   
ControlPanel.prototype.startTask=function(_controlPanel,id)
{
   var isadmin = _controlPanel._isAdmin();
   
   if(isadmin != true)
      return -1;

   $.ajax({
      url: 'CMD/startstop.php?process='+id+'&cmnd=start',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         _controlPanel.checkError(data);
      },
      error: function(jqXHR, textStatus, errorThrown ){
         $.messager.show({
            title: _controlPanel._toLocalC('error')+_controlPanel._localDoubleDot(),
            msg: _controlPanel._toLocalC('communication error')+' ('+textStatus+')',
         });
      }
   });
   
   return 0;
};

   
ControlPanel.prototype.stopTask=function(_controlPanel,id)
{
   var isadmin = _controlPanel._isAdmin();
   
   if(isadmin != true)
      return -1;

   $.ajax({
      url: 'CMD/startstop.php?process='+id+'&cmnd=stop',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         _controlPanel.checkError(data);
      },
      error: function(jqXHR, textStatus, errorThrown ){
         $.messager.show({
            title: _controlPanel._toLocalC('error')+_controlPanel._localDoubleDot(),
            msg: _controlPanel._toLocalC('communication error')+' ('+textStatus+')',
         });
      }
   });
   
   return 0;
};


ControlPanel.prototype.checkError=function(data)
{
   var _controlPanel=this;
   var retour=false;
   switch(data.errno)
   {
      case 0:
         retour=true;
         break;
      case 2: // pas habilité
         window.location = "index.php?view=services"; // rechargement de la page index qui décidera ce qu'il faut faire (cf. start_index_controler).
         return false;
         break;
      case 4: // param process non trouvé
         break;
      case 5: // pas de parametre
         break;
      case 6: // commande inconnue (doit être start/stop/reload)
         break;
      case 7: // erreur de l'opération
         break;
      case -1: // opération incohérente (déjà lancé ou déjà arrêté).
         break;
      default:
         retour=true;
         break;
   }
   if(retour==false)
   {
      $.messager.show({
         title: _controlPanel._toLocalC('error')+_controlPanel._localDoubleDot(),
         msg: _controlPanel._toLocalC('server side error')+' ('+data.errMsg+' / errno='+data.errno+')',
      });
   }
   return retour;
};


ControlPanel.prototype.__mon_listener=function(message)
{
   var data = jQuery.parseJSON( message );

   try {
      this.update_status(data);
   }
   catch(ex) {
   }
};
   
ControlPanel.prototype.__rel_listener=function(message)
{
   try {
      this.load_interfaces_list_only();
  }
  catch(ex) {
  }
};


