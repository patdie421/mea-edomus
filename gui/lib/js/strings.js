// ponctuation
var str_double_dot=" : ";

// mots
var str_Error="erreur";
var str_impossible="impossible";
var str_passwd="mot de passe";
var str_changment="changement";
var str_connection="connexion";
var str_name="nom";
var str_num="n°";
var str_type="type";
var str_description="description";
var str_interface="interface";
var str_parameters="paramètres";
var str_location="lieu";
var str_stat="état";
var str_device="périphérique";
var str_add="ajouter";
var str_edit="éditer";
var str_del="supprimer";
var str_id="numéro";
var str_sensor="capteur";
var str_actuator="actionneur";
var str_choose="choisir";
var str_ok="ok";
var str_cancel="annuler";
var str_success="succès";
var str_location="lieu";
var str_profile="profil";
var str_ident="identifiant";
var str_flag="drapeau";
var str_user="utilisateur";
var str_vendor="vendor";
var str_device2="device";
var str_instance="instance";
var str_ID="ID";
var str_plugin="plugin";
var str_port="port";
var str_base="base";
var str_buffer="mémoire tampon";
var str_save="enregistrer";
var str_logout="déconnexion";
var str_login="connexion";
var str_change="changer";
var str_admin="administration";
var str_warning="attention";
// articles
var str_the1="l'";
var str_the2="le";

// les pluriels
var str_types=str_type+"s";
var str_interfaces=str_interface+"s";
var str_locations=str_location+"x";
var str_flags=str_flag+"x";
var str_users=str_user+"s";
var str_plugins=str_plugin+"s";
var str_devices=str_device+"s";
var str_preferences="préférences";

// singulier ou pluriels
var str_sensor_s_actuator_s=str_sensor+"(s)/"+str_actuator+"(s)";
var str_sensors_actuators=str_sensor+"s/"+str_actuator+"s";
var str_interface_s=str_interface+"(s)";

// config
var str_application_prefs="préférence de l'application";
// config-contenu
var str_vendor_id=str_vendor+" "+str_ID+str_double_dot;
var str_device_id=str_device2+" "+str_ID+str_double_dot;
var str_instance_id=str_instance+" "+str_ID+str_double_dot;
var str_title_plugins=str_plugins;
var str_lib_plugins="librairie des "+str_plugins+str_double_dot;
var str_mysql_server="Serveur Mysql"+str_double_dot;
var str_server_port=str_port+str_double_dot;
var str_data_base=str_base+str_double_dot;
var str_db_user=str_user+str_double_dot;
var str_db_passwd=str_passwd+str_double_dot;
var str_test_mysql="Tester la "+str_connection;
var str_db_buffer=str_buffer+str_double_dot;
var str_sqlitebuffer=str_buffer+" SQLite"+str_double_dot;
var str_createbuffer="initialiser la "+str_base;
var str_choosedir="choisir un répertoire ...";
var str_selection="sélection";
var str_processdata="traitement en cours ...";
var str_forinformation="pour info ...";

// menu
var str_menu_home="accueil";
var str_menu_mypasswd="mon "+str_passwd;
var str_menu_admin=str_admin;
var str_menu_devices=str_devices;
var str_menu_preferences=str_preferences;
var str_menu_application="de l'application";
var str_menu_users=str_users;
var str_menu_logout=str_logout;
var str_you_are="Vous êtes"+str_double_dot;
var str_notconnected="non connecté";

// change password
var str_chg_passwd="changement de "+str_passwd;
str_label_old_passwd="ancien "+str_passwd;
str_label_new_passwd="nouveau "+str_passwd;
str_label_confirm_passwd="confirmation du "+str_passwd;

// login
var str_title_login="Accéder à Mea-eDomus";
var str_label_userid="identifiant"+str_double_dot;
var str_label_passwd=str_passwd+str_double_dot;

var str_connection_action="action connexion";

var str_redirect_login=" Vous allez être redirigé vers la page de connexion.";
var str_redirect_index=" Vous allez être redirigé vers la page d'index.";
var str_cant_operation="Vous ne pouvez pas/plus réaliser d'opération dans cette section car vous ";
    var str_not_connected=str_cant_operation+"n'êtes plus connecté !!!"+"</BR>"+str_redirect_login;
    var str_not_allowed=str_cant_operation+"n'avez pas/plus les habilitations nécessaires !!!"+"</BR>"+str_redirect_index;

var str_update_before="Modifiez d'abord les références.";

// questions
var str_wouldyou="Voulez-vous vraiment ";
    var str_wouldyou_del=str_wouldyou+str_del+" ";
        var str_del_sensoractuator=str_wouldyou_del+"ce "+str_sensor+"/"+str_actuator+" ?";
        var str_del_user=str_wouldyou_del+"cet "+str_user+" ?";
        var str_del_interface=str_wouldyou_del+"cette "+str_interface+" ?";
        var str_del_type=str_wouldyou_del+"ce "+str_type+" ?";
        var str_del_location=str_wouldyou_del+"ce "+str_location+" ?";

var str_title_del="OK pour suppression";
var str_title_error=str_Error;
var str_title_warning="Avertissement";
var str_select_row="Veuillez sélectionner une ligne !";
var str_please_wait="Merci de patienter ...";
var str_connection_succes="La "+str_connection+" a été établie avec succès";
var str_connection_failed="Connexion "+str_impossible;
var str_delete_failed="Suppression "+str_impossible;
var str_server_return="Retour du serveur ";
var str_db_init_succes="La base a été correctement initialisée";
var str_db_init_failed="Impossible de créer la base";
var str_unknow_error=str_Error+" inconnue";

var str_chg_non_needed=str_changment+" non nécessaire";
var str_chg_forced="forcer le "+str_changment+" à la première "+str_connection;
var str_chg_disalowed=str_changment+" interdit";

var str_title_save="Confirmation enregistrement";
var str_save_confirm=str_wouldyou+"enregistrer ces nouveaux paramètres ?";

var str_first_login="Première "+str_connection;
var str_passwd_must_be_changed=str_the2+" "+str_passwd+" doit être changé ...";
var str_not_com="Communication impossible avec le serveur";
var str_id_not_null=str_the1+str_ident+" ne peut pas être vide";

var str_success_passwd_changed=str_the2+" "+str_passwd+" a été changé avec succès.";
var str_incorrect_passwd=str_passwd+" incorrecte";
var str_cant_change_passwd="Changement de "+str_passwd+" "+str_impossible+" !";
var str_sensor_or_actuator=str_sensor+" ou "+str_actuator;
var str_allready_used="déjà utilisé";

var str_cant_be=" ne peut pas être ";
var str_following_objects="par les objets suivants";
    var str_cant_be_del_because=str_cant_be+"supprimée car elle est référencée "+str_following_objects+str_double_dot;
    var str_cant_be_del_because2=str_cant_be+"supprimé car il est référencé "+str_following_objects+str_double_dot;

var str_type_cant_be_del="Les types standards ne peuvent pas être supprimés";

var str_users_admin=str_admin+" des "+str_users;
var str_objects_admin=str_admin+" des objets";

var str_warning_delete="Vous allez supprimer l'utilisateur avec lequel vous êtes connecté. Une fois l'opération réalisée, vous n'aurez plus accès à l'applications. Faites le bon choix !";

var str_format_info1="Caractères acceptés : \"a\" à \"z\" en minuscule et chiffres";
var str_format_info2=" caractères au maximum";
