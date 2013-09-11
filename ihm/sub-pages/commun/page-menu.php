<ul id="menu1">
    <li><a href="index.php">Accueil</a></li>
    <li class="ui-state-disabled"></li>
    <li><a href="commandes.php">Télé-commandes</a></li>
    <li><a href="parametrages.php">Administration</a></li>
    <li class="ui-state-disabled"></li>
    <li><a href="#">Options</a></li>
    <li class="ui-state-disabled"></li>
    <li><a href="#">A propos</a></li>
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
