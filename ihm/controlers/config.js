function ajax_error(xhr, ajaxOptions, thrownError){
    alert("responseText="+xhr.responseText+" status="+xhr.status+" thrownError="+thrownError);
}


function auth_error(data) {
    if(data.error==99) {
        mea_alert2(str_title_error.capitalize(), str_not_connected.capitalize(), function(){window.location = "index.php";} );
        return false;
    }
    if(data.error==98) {
        mea_alert2(str_title_error.capitalize(), str_not_allowed.capitalize(), function(){window.location = "index.php";} );
        return false;
    }
    return true;
}


function other_error(data) {
    mea_alert2(str_title_error.capitalize(), str_unknow_error.capitalize()+" : "+data.error+" "+data.error_msg, function(){} );
}

function ceate_querydb(querydb,loading,info){
    $.ajax({
        url: 'models/create_querydb.php',
        async: true,
        type: 'GET',
        data: { 'querydb': querydb },
        dataType: 'json',
        timeout: 5000,
        beforeSend: function(){
           $(loading).dialog('open').html("</BR><p>"+str_please_wait.capitalize()+"</p>");},
        success: function(data){
            $(loading).dialog('close');
            if(data.result=="OK") {
                $(info).dialog({width:400, height:200});
                $(info).dialog('open').html("</BR><p>"+str_db_init_succes.capitalize()+"</p>");
            } else {
                if(data.result=="KO") {
                    if(auth_error(data)) {
                        $(info).dialog('open').html("<p>"+str_db_init_failed.capitalize()+". "+str_server_return+":</p><p><i>"+data.error_msg+"</i></p>");
                    }
                }
                return;
            }
        },
        error: function() { $(loading).dialog('close');
            ajax_error();
        }
    });
}


function check_mysql(server, port, base, user, password, loading, info){
    $.ajax({
        url: 'models/check_mysql_connexion.php',
        async: true,
        type: 'GET',
        data: { 'server': server, 'port':port, 'base':base, 'user':user, 'password':password },
        dataType: 'json',
        timeout: 10000,
        beforeSend: function(){
           $(loading).dialog('open').html("</BR><p>"+str_please_wait.capitalize()+"</p>");},
        success: function(data){
            $(loading).dialog('close');
            if(data.result=="OK") {
                $(info).dialog({width:400, height:200});
                $(info).dialog('open').html("</BR><p>"+str_connection_succes.capitalize()+"</p>");
            } else {
                if(data.result=="KO") {
                    if(auth_error(data)) {
                        $(info).dialog({width:400, height:230});
                        $(info).dialog('open').html("<p>"+str_connection_failed.capitalize()+". "+str_server_return+":</p><p><i>"+data.error_msg+"</i></p>");
                    }
                }
                return;
            }
        },
        error: function(jqXHR, textStatus, errorThrown ){
            $(loading).dialog('close');
            ajax_error( jqXHR, textStatus, errorThrown );
        }
    });
}


function load_config(loading){
    $.ajax({
        url: 'models/get_config.php',
        async: true,
        type: 'GET',
        dataType: 'json',
        beforeSend: function(){
           $(loading).dialog('open').html("</BR><p>"+str_please_wait.capitalize()+"</p>");
        },
        success: function(data){
           if(data.result=="KO") {
                if(auth_error(data)) {
                    other_error(data);
                }
                return;
            }
            for (var i = 0; i < data.length; i++) {
                var object = data[i];
                $("#"+object['key']).val(object['value']);
            }
            $(loading).dialog('close');
        },
        error: function(jqXHR, textStatus, errorThrown ){
            ajax_error( jqXHR, textStatus, errorThrown );
        }
    });
}


function save_config(loading){
    var donnees = {}; 
    donnees["VENDORID"]=$("#VENDORID").val();
    donnees["DEVICEID"]=$("#DEVICEID").val();
    donnees["INSTANCEID"]=$("#INSTANCEID").val();
    donnees["PLUGINPATH"]=$("#PLUGINPATH").val();
    donnees["DBSERVER"]=$("#DBSERVER").val();
    donnees["DBPORT"]=$("#DBPORT").val();
    donnees["DATABASE"]=$("#DATABASE").val();
    donnees["USER"]=$("#USER").val();
    donnees["PASSWORD"]=$("#PASSWORD").val();
    donnees["BUFFERDB"]=$("#BUFFERDB").val();
    
    $.ajax({
        url: 'models/set_config.php',
        async: true,
        type: 'GET',
        dataType: 'json',
        data: donnees,
        beforeSend: function(){
           $(loading).dialog('open').html("</BR><p>"+str_please_wait.capitalize()+"</p>");
        },
        success: function(data){
            $(loading).dialog('close');
            if(data.result=="KO") {
                if(auth_error(data)) {
                    other_error(data);
                }
                return;
            }
        },
        error: function(){
            $(loading).dialog('close');
            ajax_error();
        }
    });
}
