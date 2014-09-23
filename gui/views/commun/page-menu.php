<?php
include_once('../../lib/configs.php');
session_start();
?>
<style>
.ui-menu { width: 150px; }
</style>
<ul id="menu1">
    <li><a href="index.php" id="menu_home">Home</a></li>
    <li class="ui-state-disabled"></li>
<!--
    <li><a href="commandes.php">Télé-commandes</a></li>
    <li class="ui-state-disabled"></li>
-->
<?php
    if($_SESSION['profil']==1){
        echo "<li>";
        echo "    <a href=\"#\" id=\"menu_admin\">Admin</a>";
        echo "    <ul>";
        echo "        <li><a href=\"parametrages.php\" id=\"menu_devices\">Devices</a></li>";
//        echo "        <li><a href=\"#\">Automate</a></li>";
        echo "    </ul>";
        echo "</li>";
    }

    if($_SESSION['profil']==1 || $_SESSION['flag']!=2) {
        echo "<li>";
        echo "<a href=\"#\" id=\"menu_preferences\">Preferences</a>";
        echo "<ul>";

        if($_SESSION['profil']==1){
            echo "<li><a href=\"config.php\" id=\"menu_application\">Application</a></li>";
            echo "<li><a href=\"utilisateurs.php\" id=\"menu_users\">Users</a></li>";
            echo "<li class=\"ui-state-disabled\"></li>";
        }
        if($_SESSION['flag']!=2){
            echo "<li><a href=\"change_password.php\" id=\"menu_mypasswd\">My password</a></li>";
        }

        echo "</ul>";
        echo "</li>";
    }
?>
    
    <li class="ui-state-disabled"></li>
    <li><a href="#" id='menu_logout'>Logout</a></li>
</ul>
<div style="font-size:9px; padding:5px">
<p id="youare">
You are :
</p>
</div>


<script type="text/javascript">
jQuery(document).ready(function(){
<?php 
if(isset($_SESSION['userid']))
    echo "userid=\"".$_SESSION['userid']."\";";
else
    echo "userid=false;";
?>
    $("#menu_mypasswd").text(str_menu_mypasswd.capitalize());
    $("#menu_home").text(str_menu_home.capitalize());
    $("#menu_admin").text(str_menu_admin.capitalize());
    $("#menu_devices").text(str_menu_devices.capitalize());
    $("#menu_preferences").text(str_menu_preferences.capitalize());
    $("#menu_application").text(str_menu_application.capitalize());
    $("#menu_users").text(str_menu_users.capitalize());
    $("#menu_logout").text(str_menu_logout.capitalize());

    if(userid==false)
        userid=str_notconnected;
    $("#youare").text(str_you_are+userid);

    $( "#menu1" ).menu();
    $('#menu_logout').click(function(){
                            $.get('models/destroy_php_session.php',
                                {},
                                function(data){
                                },
                                "json"
                            )
                            .always(function(){ window.location = "login.php"; });
                        });
});
</script>
