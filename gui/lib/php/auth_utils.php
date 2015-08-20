<?php
function check_admin()
{
    if( !isset($_SESSION['userid']) ||
        !isset($_SESSION['logged_in']) ||
        !isset($_SESSION['profil'])) {
        return 99;
    }

    if ($_SESSION['profil'] != 1) {
        return 98;
    }
    
    return 0;
}
