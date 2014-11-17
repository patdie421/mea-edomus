<?php
/**
 * @file   delete_row_by_id-jqg_grid.php
 * @date   2014/11/17
 * @author Patrice Dietsch <patrice.dietsch@gmail.com>
 * @brief  supprime un enregistrement en utilisation sont "id"
 * @detail ceci est un cgi de type GET. Il va supprimer un enregistrement dans une
 *         table spécifiée en paramètre. Le resultat de l'opération se trouve dans
 *         un json.
 * @return { result=>["OK" ou "KO"] : OK = suppression réalisée, KO une erreur
 *           error=>[numéro d'erreur] : les numéros d'erreur possible sont :
 *              1 : erreur inconnue
 *              2 : erreur de paramètre (table ou id absent)
 *              3 : connexion impossible à la base (voir errmsg pour le retour du SGBDR)
 *              4 : erreur de la requête (voir errmsg pour le retour du SGBDR)
 *             98 : demandeur pas habilité
 *             99 : demandeur pas connecté
 *              0 : opération réussie
 *           errmsg=>[un message],
 *           debug=>[requete sql executée] }
 
 * @param  table   nom de la table contenant l'enregistrement à supprimer
 * @param  id      numéro de l'enregistrement à supprimer    
 */
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

switch(check_admin()){
    case 98:
        echo json_encode(array("result"=>"KO","error"=>98,"error_msg"=>"pas habilité" ));
        exit(1);
    case 99:
        echo json_encode(array("result"=>"KO","error"=>99,"error_msg"=>"non connecté" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array("result"=>"KO","error"=>1,"error_msg"=>"erreur inconnue" ));
        exit(1);
}


if(!isset($_GET['table']) || !isset($_GET['id'])){
    echo json_encode(array("error"=>2,"error_msg"=>"parameters error" ));
    exit(1);
}else{
    $table=$_GET['table'];
    $id=$_GET['id'];
}

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("error"=>3,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$SQL="DELETE FROM $table WHERE id=$id";
try{    
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("error"=>4,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

header('Content-type: application/json');

echo json_encode(array("result"=>"OK","error"=>0));

$file_db = null;
