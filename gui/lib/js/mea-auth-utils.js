function get_auth_data()
{
    var retour;
    $.ajax({
        url: 'models/get_auth_data.php',
        async: false,
        type: 'GET',
        dataType: 'json',
        data: {},
        beforeSend: function(){
        },
        success: function(data){
            if(data.result=="OK"){
                retour=data;
            } else {
                retour=false;
            }
        },
        error: function(){
            retour=false;
        }
    });
    return retour;
}


function check_auth() {
    authdata=get_auth_data();
    if(authdata==false) {
        mea_alert2(str_Error+str_double_dot, str_not_connected, function(){window.location = "login.php";} );
        return false;
    } else {
        if(authdata.profil!=1) {
            mea_alert2(str_Error+str_double_dot, str_not_allowed, function(){window.location = "index.php";} );
            return false;
        } else
            return true;
    }
}
