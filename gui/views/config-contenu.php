<?php
//
//  SOUS-PAGE (SUB-VIEW) de config.php : gestion de la configuration de l'application
//
include_once('../lib/configs.php');
include "../lib/php/auth_utils.php";
session_start();

switch(check_admin()){
    case 98:
        echo "<script>window.location = \"index.php\";</script>";
        return;
    case 99:
        echo "<script>window.location = \"login.php\";</script>";
        return;
}
?>

<style>
    .ui-widget{font-size:14px;}
    .ui-widget-content{font-size:14px;}
    
    label, input { display:block; font-size: 80%;}
    input.text {
        width:95%;
        padding:.3em;
    } 
        
    .filepicker, .filepicker_selection, .filepicker_cadre {
		padding: 5px;
        font-family: Verdana, sans-serif;
        font-size: 12px;
    }
        
    .filepicker {
        width: 400px;
        height: 300px;
        margin: 5px;
        overflow: scroll;
	}
        
    .filepicker_selection {
        width: 320px;
        max-width: 320px;
        margin-left:5px;
        margin-top: 10px;
        margin-bottom: 10px;
        margin-right: auto;
        overflow-wrap: break-word;
        word-wrap: break-word;
    }

    UL.jqueryFileTree A:hover {
<?php
        $jqui_theme="start";
        echo "background: #79c9ec url(lib/jquery-ui-1.10.3.custom/css/".$jqui_theme."/images/ui-bg_glass_75_79c9ec_1x400.png) 50% 50% repeat-x;";
?>
    }
    
    .titreboite {
        height:30px;
        line-height:30px;
        padding-left:5px;
        padding-right:10px;
    }
    
    .boite {
        padding-left:50px;
        padding-right:50px;
        padding-bottom:10px;
    }
    
    div.error_msg {
        display: none
        margin-top: -5px;
        color: #f00;
        font-size:11px;
        font-style: italic;
    }
    
    .no-close .ui-dialog-titlebar-close { display: none };
    
</style>

<div class="ui-corner-all ui-widget-content" style="padding-top: 20px">
<div style="margin:auto; width:600px; max-width:600px;">

    <div class="ui-widget-header ui-corner-top titreboite" id="title_xpl">xPL</div>
    <div class="ui-widget-content boite" style="padding-top:15px">
        <table width="100%" align="center" style="padding-bottom: 12px;">
            <col width="20%">
            <col width="3%">
            <col width="37%">
            <col width="3%">
            <col width="37%">
            <tr>
                <td width="20%">
                <label for="VENDORID" id="vendorid">Vendor ID:</label>
                <input type="text" name="ENDORID" id="VENDORID" style="text-align: center;" class="text ui-widget-content ui-corner-all" />
                </td>

                <td>
                <div width="100%" style="padding-top:0.5em; padding-left: 0.5em;">-</div>
                </td>
                <td>
                <label for="DEVICEID" id="deviceid">Device ID:</label>
                <input type="text" name="DEVICEID" id="DEVICEID" style="text-align: center;" class="text ui-widget-content ui-corner-all" />
                </td>
                <td>
                <div width="100%" style="padding-top:0.5em; padding-left: 0.5em;">.</div>
                </td>
                <td>
                <label for="INSTANCEID" id="instanceid">Instance ID:</label>
                <input type="text" name="INSTANCEID" id="INSTANCEID" style="text-align: center;" class="text ui-widget-content ui-corner-all" />
                </td>
            </tr>
            <tr>
                <td colspan="5"><div id="XPL_ERR" class="error_msg"></div></td>
            </tr>
        </table>
    </div>
    <div class="ui-widget-header titreboite" id="title_plugins">Plugins</div>
    <div class="ui-widget-content boite" style="padding-top:15px">
        <table width="100%" style="padding-bottom: 12px;">
            <col width="80%">
            <col width="20%">
            <tr>
                <td>
                <label for="PLUGINPATH" id="libplugins">Plugins library:</label>
                <input type="text" name="PLUGINPATH" id="PLUGINPATH" class="text ui-widget-content ui-corner-all" style="margin-bottom:0px;"/>
                </td>
                <td valign="bottom" align="center">
                    <button id='choix_2'>Choose</button>
                </td>
            </tr>
            <tr>
                <td  colspan="2"><div id="PLUGINPATH_ERR" class="error_msg"></div></td>
            </tr>
        </table>
    </div>
    <div class="ui-widget-header titreboite" id="mysql">Mysql</div>
    <div class="ui-widget-content boite">
        </br>
        <table width="100%" style="padding-bottom: 12px;">
            <col width="80%">
            <col width="20%">
            <tr>
                <td>
                <label for="DBSERVER" id="mysqlserver">Mysql server:</label>
                <input type="text" name="DBSERVER" id="DBSERVER" class="text ui-widget-content ui-corner-all" />
                </td>
                <td>
                <label for="DBPORT" id="port">Port:</label>
                <input type="text" name="DBPORT" id="DBPORT" class="text ui-widget-content ui-corner-all" />
                </td>
            </tr>
            <tr>
                <td colspan="2"><div id="DBSERVER_ERR" class="error_msg"></div></td>
            </tr>
            <tr>
                <td>
                <label for="DATABASE" id="base">Base:</label>
                <input type="text" name="DATABASE" id="DATABASE" class="text ui-widget-content ui-corner-all" />
                </td>
            </tr>
            <tr>
                <td  colspan="2"><div id="DATABASE_ERR" class="error_msg"></div></td>
            </tr>
            <tr>
                <td>
                <label for="USER" id="user">User:</label>
                <input type="text" name="USER" id="USER" class="text ui-widget-content ui-corner-all" />
                </td>
            </tr>
            <tr colspan="2">
                <td><div id="USER_ERR" class="error_msg"></div></td>
            </tr>
            <tr>
                <td>
                <label for="PASSWORD" id="passwdx">Password:</label>
                <input type="password" name="PASSWORD" id="PASSWORD" class="text ui-widget-content ui-corner-all" />
                </td>
            </tr>
            <tr colspan="2">
                <td><div id="PASSWORD_ERR" class="error_msg"></div></td>
            </tr>
        </table>
        <hr class="ui-widget-content">
        <div style="text-align: center; padding-bottom:12px">
            <button id='testMysql'>Test connection</button>
            <!-- <button id='createMysql'>Initialiser la base</button> -->
        </div>
    </div>
    <div class="ui-widget-header titreboite" id="buffer">Buffer</div>
    <div class="ui-widget-content ui-corner-bottom boite" style="padding-top:15px">
        <table width="100%" style="padding-bottom: 12px;">
            <col width="80%">
            <col width="20%">
            <tr>
                <td>
                <label for="BUFFERDB" id="sqlitebuffer">SQLite buffer db :</label>
                <input type="text" name="BUFFERDB" id="BUFFERDB" class="text ui-widget-content ui-corner-all" style="margin-bottom:0px;"/>
                </td>
                <td valign="bottom" align="center">
                    <button id='choix_1'>Choose</button>
                </td>
            </tr>
            <tr colspan="2">
                <td><div id="BUFFERDB_ERR" class="error_msg"></div></td>
            </tr>
        </table>
        <hr class="ui-widget-content">
        <div style="text-align: center; padding-bottom:12px">
            <button id='createBuffer'>Init database</button>
        </div>
    </div>
</div>
</br>
<hr class="ui-widget-content">
<div style="text-align: center">
    <button id='enregistrer'>Save</button>
</div>

</br>
<div>

<script type="text/javascript" src="lib/js/mea-auth-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-gui-utils.js"></script>

<script type="text/javascript" src="controllers/config.js"></script>

<script type="text/javascript">
jQuery(document).ready(function(){

<?php
   echo "var socketio_port=";
   echo $IOSOCKET_PORT;
   echo ";\n";
   ?>
   liveCom.connect(socketio_port);
php>

    msg_txt1=str_format_info1;
    msg_txt2=str_format_info2;
    
    function setLiveCheck(id,msg_id,nb_cars,regex,msg,msg1,msg2)
    {
        $(id).bind('paste', function(event) {
            me=$(this);
            err=$(msg_id);
            $("#buffer").text(me.val());
            setTimeout(function(){
                t=me.val();
                if($("#buffer").text() != t) {
                    if(regex.test(t)) {
                    } else {
                        me.val($("#buffer").text());
                        err.stop(true,true).hide();
                        err.text(msg+" : "+msg1+". "+msg2);
                    err.fadeIn(1).delay(1500).fadeOut(500);
                    }
                }
            }
            ,0);
        });

        $(id).keypress(function(event){
            me=$(this);
            err=$(msg_id);
            if(event.which >= 32) {
                if( !regex.test(String.fromCharCode(event.which)) ) {
                    event.preventDefault();
                    err.stop(true,true).hide();
                    err.text(msg1);
                    err.fadeIn(1).delay(1500).fadeOut(500);
                    return;
                }
                if(me.val().length > (nb_cars - 1)){
                    event.preventDefault();
                    err.stop(true,true).hide();
                    err.text(msg2);
                    err.fadeIn(1).delay(1500).fadeOut(500);
                    return;
                }
            }
        });
    }

    //
    // Traduction
    //
    $("#vendorid").text(str_vendor_id.capitalize());
    $("#deviceid").text(str_device_id.capitalize());
    $("#instanceid").text(str_instance_id.capitalize());
    $("#title_plugins").text(str_title_plugins.capitalize());
    $("#libplugins").text(str_lib_plugins.capitalize());
    $("choix_2").text(str_choose.capitalize());
    $("#mysqlserver").text(str_mysql_server.capitalize());
    $("#port").text(str_server_port.capitalize());
    $("#base").text(str_data_base.capitalize());
    $("#user").text(str_db_user.capitalize());
    $("#passwdx").text(str_db_passwd.capitalize());
    $("#testMysql").text(str_test_mysql.capitalize());
    $("#buffer").text(str_db_buffer.capitalize());
    $("#sqlitebuffer").text(str_sqlitebuffer.capitalize());
    $("choix_1").text(str_choose.capitalize());
    $("#createBuffer").text(str_createbuffer.capitalize());
    $("#enregistrer").text(str_save.capitalize());
    $("#selection").text(str_selection.capitalize());
    //
    // Contrôles de surface
    //
    setLiveCheck("#VENDORID",
        "#XPL_ERR",
        8,
        /^[a-z0-9]{1,8}$/,
        "Vendor ID",
        msg_txt1,
        "8"+msg_txt2
    );


    setLiveCheck("#DEVICEID",
        "#XPL_ERR",
        16,
        /^[a-z0-9]{1,16}$/,
        "Device ID",
        msg_txt1,
        "16"+msg_txt2
    );

    
    setLiveCheck("#INSTANCEID",
        "#XPL_ERR",
        16,
        /^[a-z0-9\\-]{1,16}$/,
        "Device ID",
        msg_txt1,
        "16"+msg_txt2
    );

    
    //
    //  boites de dialogues
    //
    $('#filepicker').fileTree({
        root: '/',
        script: 'lib/jqueryFileTree/connectors/jqueryDirTree.php',
        expandSpeed: 100,
        collapseSpeed: 100,
        multiFolder: false,
    }, function(file) { // click
        $('#choix').text(file);
    });

    
    buttons_selecteurs={}; // pour la localisation des textes boutons
    buttons_selecteurs[str_choose.capitalize()]=
        function() {
            $('#'+$(this).data('field')).val($('#choix').text()+$(this).data('filename'));
            $( this ).dialog( "close" );
        };
    buttons_selecteurs[str_cancel.capitalize()]=
        function() {
            $( this ).dialog( "close" );
        };                           
    $( "#dialog-selecteur" ).dialog({
        autoOpen: false,
        width: 447,
        modal: true,
        buttons: buttons_selecteurs,
        close: function() {
        }    
    });
    $('#dialog-selecteur').dialog('option', 'title', str_choosedir.capitalize());

    
    $("#dialog-loading").dialog({
        autoOpen: false,
        modal: true,
        dialogClass: 'no-close',
        closeOnEscape: false
    });
    $('#dialog-loading').dialog('option', 'title', str_processdata.capitalize());


    buttons_info={}; // pour la localisation des textes boutons
    buttons_info[str_ok.capitalize()]=
        function() {
            $( this ).dialog( "close" );
        };
    $("#dialog-info").dialog({
        autoOpen: false,
        width: 400,
        height: 300,
        modal: true,
        closeOnEscape: true,
        buttons: buttons_info,
    });
    $('#dialog-info').dialog('option', 'title', str_forinformation.capitalize());

    
    //
    // association des actions
    //
    $( "#choix_1" ).button().click(function() {
        $( "#dialog-selecteur" ).data('field','BUFFERDB').data('filename','queries.db').dialog( "open" );
    });

    
    $( "#choix_2" ).button().click(function() {
        $( "#dialog-selecteur" ).data('field','PLUGINPATH').data('filename','').dialog( "open" );
    });

    
    $( "#testMysql" ).button().click(function() {
        check_mysql($("#DBSERVER").val(),
                    $("#DBPORT").val(),
                    $("#DATABASE").val(),
                    $("#USER").val(),
                    $("#PASSWORD").val(),
                    "#dialog-loading",
                    "#dialog-info");
    });

/*    
    $( "#createMysql" ).button().click(function() {
    });
*/

    $( "#createBuffer" ).button().click(function() {
        ceate_querydb($("#BUFFERDB").val(),"#dialog-loading","#dialog-info");
    });

    
    function wrap_save_config(){
        save_config("#dialog-loading", "#dialog-loading", "#dialog-info");
    }
    
    $( "#enregistrer" ).button().click(function() {
        mea_yesno2(str_title_save, str_save_confirm, wrap_save_config, null);
    });

    //
    // Chargement initial des données
    //
    load_config("#dialog-loading");
});
</script>

<div id="dialog-selecteur" title="Choisir un fichier ...">
    <form>
        <div  class="ui-widget-content filepicker">
            <div id='filepicker' style="border: 0px;">
            Choose a directory
            </div>
        </div>
        <table>
            <tr>
                <td width="100%" id="selection">selection&nbsp;:</td>
                <td>
                    <div id='choix' class="ui-widget-content ui-corner-all filepicker_selection">
                    &nbsp;
                    </div>
                </td>
            <tr>
        </table>
    </form>
</div>

<div id="dialog-loading" title="process data"></div>

<div id="dialog-info" title="Information"></div>

<div id="buffer" style="display: none"></div>
