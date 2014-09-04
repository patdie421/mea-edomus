<script type="text/javascript">

function chgpasswd(passwd1, passwd2, old_passwd, info){
    <?php
    if(!isset($_SESSION['change_passwd_flag'])){
        echo "set_password_params={old_password:old_passwd, new_password:passwd1};";
    }else{
        echo "set_password_params={new_password:passwd1};";
    }
    ?>

    $.ajax({ url: 'models/set_passwd.php',
        async: false,
        type: 'GET',
        dataType: 'json',
        data: set_password_params,
        success: function(data) {
            if(data.retour==1) {
                mea_alert2(str_success,str_success_passwd_changed,function(){
                    <?php
                    if(isset($_GET['dest'])){
                        $dest=$_GET['dest'];
                        echo "window.location = \"$dest\";";
                    }else{
                        echo "window.location = \"index.php\";";
                    }
                    ?>
                });
            }
            else
                $(info).text(str_incorrect_passwd).delay(3000).fadeOut(1000);
        },
        error:function(){
            $(info).text(str_cant_change_passwd).delay(3000).fadeOut(1000);
        }
    });
}

</script>