<?php
include "lib/configs.php";
//
//  PAGE PRINCIPALE (VIEW) : gestion des utilisateurs de l'application
//
session_start();
?>
<!DOCTYPE html>
<?php
    
    if(!isset($_SESSION['logged_in']))
    {
        $dest=$_SERVER['PHP_SELF'];
        echo "<script>window.location = \"login.php?dest=$dest\";</script>";
        exit();
    }
    
    if(!isset($_SESSION['profil']) || $_SESSION['profil']!=1){
        echo "<script>window.location = \"index.php\";</script>";
        exit();
    }
?>
<html>
<head>
    <meta charset="utf-8">
    <title>
    <?php echo $TITRE_APPLICATION; ?>
    </title>
    <?php include "lib/includes.php"; ?>
</head>
<body>

   <style>
      .ui-widget{font-size:14px;}
       
      #feedback { font-size: 1em; }
      #selectable .ui-selecting { background: #FECA40; }
      #selectable .ui-selected { background: #F39814; color: white; }
      #selectable { list-style-type: none; margin: 0; padding: 0; }
      #selectable li { padding: 0.4em; font-size: 1em; height: 1em; }
     
      
      label, input { display:block; }
      label { font-size:0.8em; }
      input.text { text-align:center; margin-bottom:12px; width:100%; padding-top:0.4em; padding-bottom:0.4em; }
      fieldset { padding:0; border:0; margin-top:25px; }


      .ui-dialog { padding: .3em; }
      .validateTips { border: 1px solid transparent; padding-bottom: 0.3em; }
      .mea_disabled { color: #b9b9b9; background: #E0E0E0; }
   </style>


   <div>
      <div id='entete'>
      ENTETE
      </div>
      <div id='titre'>
         <div id='users_admin' style="padding-left:170px; text-align:center; font-size:18px;">
         Users administration
         </div>
      </div>
      <div id='main'>
         <div id='menu'>
         MENU
         </div>
         <div id='contenu'>
         CONTENU
         </div>
      </div>
      <div id='piedpage'>
      PIEDPAGE
      </div>
   </div>


   <div id="dialog-form" title="Règle">

      <div class="validateTips">
         Associer un message xPL à une entrée de l'automate.
      </div>

      <label for="key" style="font-size:0.9em;">Conditions :</label>

      <div class="ui-widget-content" style="padding:15px;">
         <table>
            <tr>
            <td width="265px">
               <label>xPL message from (source address) :</label>
               <input id="xplsource" name="xplsource" style="width:265px" class="text ui-widget-content ui-corner-all">
            </td>
            <td width="24px">
            </td>
            <td width="265px">
               <label>With xPL Schema :</label>
               <input id="xplschema" name="xplschema" style="width:265px" class="text ui-widget-content ui-corner-all">
            </tr>
         </table>

         <table>
            <tr>
            <td valign="top" align="center" width="220px">
               <div>
                  <label style="text-align:left;">xPL body key/value condition :</label>
                  <div class="ui-widget-content" style="display:table;">
                     <div style="display:table-cell; vertical-align: middle; padding:5px">
                        <label for="key" style="text-align:left;">Value for key :</label>
                        <input id="key" name="key" style="width:205px" class="text ui-widget-content ui-corner-all">
                        <div style="text-align:center">
                        <select id="op" class="ui-widget-content ui-corner-all" style="margin-bottom:5px;">
                           <option>=</option>
                           <option>!=</option>
                           <option>></option>
                           <option>=></option>
                           <option><</option>
                           <option>=<</option>
                        </select>
                        </div>
                        <label for="value" style="text-align:left;">A value (string or numeric) :</label>
                        <input id="value" name="value" style="width:205px" class="text ui-widget-content ui-corner-all">
                     </div>
                  </div>
               </div>
            </td>

            <td width="105px">
               <div style="display:table; vertical-align: middle; align:center; width:100px;">
                  <div style="display:table-cell;">
                     <button id="add_condition" style="width:60px; display:block; margin:5px; margin-left:auto; margin-right:auto;">add</button>
                     <button id="del_condition" style="width:60px; display:block; margin:5px; margin-left:auto; margin-right:auto;">del</button>
                  </div>
               </div>
            </td>

            <td width="220px">
               <label>xPL body conditions list :</label>
               <div class="ui-widget-content" style="width:230px; height:150px; overflow:auto">
                  <ol id="selectable" style="text-align:center">
                  </ol>
               </div>
            </td>
 
            </tr>
         </table>

      </div>
      <p></p>
      <label for="key" style="font-size:0.9em;">Set :</label>

      <div class="ui-widget-content" style="padding:10px; padding-down:5px">
         <table>
            <tr>
     
            <td width="205px" valign="top">
               <label>Input type :</label>
               <div id="radio1" style="margin-right:5px; padding:5px; text-align:center;" class="ui-widget-content ui-corner-all ">
                     <input type="radio" class="picker" id="digital" name="radio1" checked='true' style="display:inline;"/>
                     <label for="digital" style="display:inline;">Digital</label>
                     <input type="radio" class="picker" id="analog" name="radio1" style="display:inline;"/>
                     <label for="analog" style="display:inline;">Analog</label>
               </div>
               <p></p>
               <label for="inputindex">input index number :</label>
               <input id="inputindex" name="inputindex" class="text ui-widget-content ui-corner-all" style="width:165px;">
            </td>

            <td width="24px">
            </td>

            <td width="340px" valign="top">
               <label for="setvalue" style="padding-left:5px;">Value to set :</label>
               <div id="radio3" style="text-align:left; margin-left:5px; margin-right:5px; padding:10px;" class="ui-widget-content ui-corner-all">
                  <div>
                     <input type="radio" value=1 class="picker" id="true" name="radio2" checked='true' style="display:inline;"/>
                     <label for="digital"  style="display:inline;">TRUE</label>
                  </div>
                  <div>
                     <input type="radio" value=2 class="picker" id="false" name="radio2" style="display:inline;"/>
                     <label for="analog"  style="display:inline;">FALSE</label>
                  </div>
                  <div>
                     <input type="radio" value=3 class="picker" id="keyvalue" name="radio2"  style="display:inline;"/>
                     <label for="keyvalue" style="display:inline;">value from body key :</label>
                     <input id="key2" name="key2" class="mea_disabled ui-widget-content ui-corner-all" style="text-align:center; display:inline; width:150px;">
                  </div>
               </div>
            </td>
 
            </tr>
         </table>
 
      </div>
   </div>
  
   <button id="create-user">Create new user</button>
   <div id="result"></div>

</body>

<script type="text/javascript" src="lib/js/strings.js"></script>
<script type="text/javascript">

jQuery(document).ready(function() {

   var tips = $( ".validateTips" ),
       _key = $( "#key" ),
       _value = $( "#value" ),
       _xplsource = $("#xplsource"),
       _xplschema = $("#xplschema"),
       _inputindex = $("#inputindex"),
       _key2 = $("#key2");
       
       allFields = $( [] ).add( _key ).add( _value ).add( _xplsource ).add( _xplschema ).add( _inputindex ).add( _key2 );

    $.ajaxSetup({ cache: false });

    // traduction
    $("#users_admin").text(str_users_admin.capitalize());
    
    // chargement des sous-pages
    $("#entete").load("views/commun/page-entete.php");
    $("#menu").load("views/commun/page-menu.php");
//    $("#contenu").load("views/utilisateurs-contenu.php");
    $("#piedpage").load("views/commun/page-pied.php");


   function updateTips( t ) {
      tips
      .text( t )
      .addClass( "ui-state-highlight" );
      setTimeout(function() {
         tips.removeClass( "ui-state-highlight", 1500 );
      }, 500 );
    }

   function checkLength( o, n, min, max ) {
      if ( o.val().length > max || o.val().length < min ) {
         o.addClass( "ui-state-error" );
         updateTips( "Length of " + n + " must be between " +
         min + " and " + max + "." );
         return false;
      } else {
         return true;
      }
   }
   
   function checkRegexp( o, regexp, n ) {
      v = o.val();
      $.trim(v);
      if ( !( regexp.test( v ) ) ) {
         o.addClass( "ui-state-error" );
         updateTips( n );
         return false;
      } else {
         return true;
      }
   }

   function disable_keyvalue(event) {
      $("#key2").attr("readonly", "readonly");
      $("#key2").addClass( "mea_disabled" );
   }

   function enable_keyvalue(event) {
      $("#key2").removeAttr("readonly");
      $("#key2").removeClass( "mea_disabled" );
      $("#key2").focus();
   }
   
   $("#true").click(disable_keyvalue);
   $("#false").click(disable_keyvalue);
   $("#keyvalue").click(enable_keyvalue);
   
   $("#selectable").selectable();
   $("#del_condition").button().click(function( event ) { event.preventDefault(); $(".ui-selected","#selectable").remove(); });
   $("#add_condition").button().click(function( event ) {
      event.preventDefault();
      
      key=_key.val();
      value=_value.val();
      $.trim(key);
      $.trim(value);
      
      _key.removeClass( "ui-state-error" );
      _value.removeClass( "ui-state-error" );
      
      if(key=="") {
         _key.addClass( "ui-state-error" );
      }
      if(value=="") {
         _value.addClass( "ui-state-error" );
      }
      if(key!="" && value!="") {
         b = checkRegexp( _key, /^[0-9a-z_]{1,16}$/, "'Value for key' may consist of a-z, 0-9 or underscores." );
         if(b) {
            if(($("#op").val() == '=') || ($("#op").val() == '!=') ) {
               if(!$.isNumeric( _value.val() )) {
                  value="\""+value+"\"";
               }
            }
            else {
               if(!$.isNumeric( _value.val() )) {
                  _value.addClass( "ui-state-error" );
                  updateTips("With '>', '>=', '<' and '<=' operators, 'A value' must be numeric.");
                  b = false;
               }
            }
         }
         if(b) {
            $("#key").val("");
            $("#value").val("");
            keyvalue=key+" "+$("#op").val()+" "+value;
            x="<li>"+keyvalue+"</li>";
            $("#selectable").append(x);
         }
      }
      else {
         updateTips("'Value for key' and 'A value' can't be empty !");
      }
      
   });
 
   $( "#dialog-form" ).dialog({
      autoOpen: false,
      height: 600,
      width: 631,
      modal: true,
      resizable: false,
      buttons: {
         "Add": function() {
            allFields.removeClass( "ui-state-error" );
            b = checkRegexp( _xplsource, /^[0-9a-z]{1,8}-[0-9a-z]{1,8}\.[0-9a-z\-]{1,16}$/, "xlp address format error (VendorID-DeviceID.InstanceID)" );
            if(!b)
               return;
            b = checkRegexp( _xplschema, /^[0-9a-z]{1,8}\.[0-9a-z]{1,8}$/, "xlp schema format error ()" );
            if(!b)
               return;
            b = checkRegexp( _inputindex, /^[0-9]+$/, "must be numeric (0 <= input index <= 255)" );
            if(!b)
               return;
            v = parseInt($.trim(_inputindex.val()));
            if(v > 255) {
               updateTips("must be numeric (0 <= input index <= 255)");
               _inputindex.addClass( "ui-state-error" );
               return;
            }
            if($("#selectable").children().length == 0) {
               updateTips("At least one condition is needed");
               return;
            }
            if($("#keyvalue:checked").val()==3) {
               b = checkRegexp( _key2, /^[0-9a-z_]{1,16}$/, "'Value from body key' may consist of a-z, 0-9 or underscores." );
               if(!b)
                  return;
            }
            
            // recolter les données et le transmettre pour mise à jour
            
            $( this ).dialog( "close" );
         },
         Cancel: function() {
            $( this ).dialog( "close" );
         }
      },
      close: function() {
         // allFields.val( "" ).removeClass( "ui-state-error" );
      }
   });

   $( "#create-user" ).button().click(function() {
      allFields.removeClass( "ui-state-error" ).val("").on( "focus", function() { $(this).removeClass( "ui-state-error" ); } );
      disable_keyvalue();
      $("#true").attr('checked','checked');
      $("#selectable li").remove();
      $( "#dialog-form" ).dialog( "open" );
   });
});
</script>
</html>
