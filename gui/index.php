<?php
include "lib/configs.php";
//
//  PAGE PRINCIPALE (VIEW) : home page
//
session_start();
?>
<!DOCTYPE html>
<?php
include "lib/configs.php";

// contrôle et redirections
if(!isset($_SESSION['logged_in']))
{
    $dest=$_SERVER['PHP_SELF'];
    echo "<script>window.location = \"login2.php?dest=$dest\";</script>";
    exit();
}
?>

<html>
<head>
   <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
   <meta name="description" content="domotique DIY !">
   <title>mea-eDomus</title>
   
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.1/themes/default/easyui.css">
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.1/themes/icon.css">
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.1/themes/color.css">
   <link rel="stylesheet" type="text/css" href="lib/mea-edomus.css">
   
   <script type="text/javascript" src="lib/jquery-easyui-1.4.1/jquery.min.js"></script>
   <script type="text/javascript" src="lib/jquery-easyui-1.4.1/jquery.easyui.min.js"></script>
   
   <script type="text/javascript" src="lib/jquery-easyui-datagridview/datagrid-groupview.js"></script>
   <script type="text/javascript" src="lib/noty-2.2.10/js/noty/packaged/jquery.noty.packaged.min.js"></script>

   <script type="text/javascript" src="lib/js/fr/mea-translation.js"></script>
   <script type="text/javascript" src="lib/js/mea-livecom-utils.js"></script>
   <script type="text/javascript" src="lib/js/mea-auth-utils.js"></script>
   
   <script>
   var currentPage = "";
   
   jQuery(document).ready(function(){
      $.ajaxSetup({ cache: false });
      
      page('page1.php', 'ff', 'Indicateurs'); // chargement de la première page.

<?php
      echo "var socketio_port=";
      echo $IOSOCKET_PORT;
      echo ";\n";
?>
//      var socketio_port=8000;
      liveCom.connect(socketio_port);
   });

   function page(pageUrl, tab, tabName)
   {
      authdata=get_auth_data();
      if(authdata==false)
      {
         $.messager.alert("Erreur :","Vous n'êtes pas/plus connectés ...",'error', function(){window.location = "login2.php?dest=index.php&page="+pageUrl+"&tab="+tabName;});
         $('#content').panel('refresh',"notlogged.html"); // on vide l'affichage
         return -1;
      }

      $('#tabtodisplay').val(tabName);
      if(currentPage != pageUrl)
      {
         $('#content').panel('refresh',pageUrl); // on charge la page
         currentPage = pageUrl;
      }
      else if(tab && tabName)
      {
         $('#'+tab).tabs('select', tabName);
      }
   }
   </script>
</head>

<style>

a.meamenu {
   color:black;
   text-decoration:none;
}

a.meamenu:visited {
}

a.meamenu:hover {
   font-weight:bold;
}

</style>

<body>

   <div style="min-width:750px;">
      <div id='logo'  style="float:left; width:250px; height:50px; text-align:left;">LOGO</div>
      <div id='pub' style="width:250px; float:right; height:50px; text-align:right;">INFO</div>
      <div id='titre-page' style="height:50px; text-align:center;">CENTRE</div>
   </div>
   <div style="min-width:950px;min-height:600px;margin:10px;">
      <div class="easyui-layout" fit=true>

         <div region="west" split="true" collapsible="false" title="" style="width:200px;">
            <div id="aa" class="easyui-accordion" data-options="animate:false,border:false" style="width:100%;">
            
               <div title="Supervision" data-options="selected:true" style="overflow:auto;padding:10px;">
                  <div><a href="#" class="meamenu" onclick="javascript:page('page1.php','tt','Indicateurs')">Indicateurs</a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:page('page1.php','tt','Services')">Services</a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:page('page1.php','tt','Journal')">Journal</a></div>
               </div>
               
               <div title="Configuration" style="overflow:auto;padding:10px;">
                  <div><a href="#" class="meamenu" onclick="javascript:page('page2.php','tabConfiguration','Capteurs/Actionneurs')">Capteurs/Actionneurs</a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:page('page2.php','tabConfiguration','Interfaces')">Interfaces</a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:page('page2.php','tabConfiguration','Types')">Types</a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:page('page2.php','tabConfiguration','Localisations')">Localisations</a></div>
               </div>

               <div title="Préférences" style="overflow:auto;padding:10px;">
                  content3
               </div>
            </div>
       </div>


       <div id="content" region="center" title=""></div>

       <input id="tabtodisplay" type="hidden">

       </div>
   </div>
   <div style="font-size: 10px; text-align:center;">
   (c) 2014 Mea soft
   </div>

</body>
</html>
