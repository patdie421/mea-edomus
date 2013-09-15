<?php
session_start();
?>
<ul id="menu1">
    <li><a href="index.php">Accueil</a></li>
    <li class="ui-state-disabled"></li>
    <li><a href="commandes.php">Télé-commandes</a></li>
    <li class="ui-state-disabled"></li>
<?php
    if($_SESSION['profil']==1){
        echo "<li>";
        echo "    <a href=\"#\">Administration</a>";
        echo "    <ul>";
        echo "        <li><a href=\"parametrages.php\">Périphériques</a></li>";
        echo "        <li><a href=\"#\">Automate</a></li>";
        echo "    </ul>";
        echo "</li>";
    }
?>
    <li>
        <a href="#">Préférences</a>
            <ul>
<?php
    if($_SESSION['profil']==1){
        echo "<li><a href=\"#\">Application</a></li>";
        echo "<li><a href=\"utilisateurs.php\">Utilisateurs</a></li>";
        echo "<li class=\"ui-state-disabled\"></li>";
    }
?>
                <li><a href="change_password.php">Mon mot de passe</a></li>
            </ul>
    </li>
    <li class="ui-state-disabled"></li>
    <li><a href="#" id='logout'>Déconnexion</a></li>
</ul>

<script>
 $(function() {
    $( "#menu1" ).menu();
    $('#logout').click(function(){
                            $.get('lib/php/destroy_php_session.php',
                                {},
                                function(data){
                                },
                                "json"
                            )
                            .always(function(){ window.location = "login.php"; });
                        });
});
</script>
<style>
.ui-menu { width: 150px; }
</style>
