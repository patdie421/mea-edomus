<?php
/**
 * \file    parameters.php
 * \author  patdie421
 * \version 0.1A1
 * \date    24 juillet 2013
 * \brief   Gestion des parametres de mea-edomus.
 *
 * \details Cette classe affiche une page permettant le paramétrage de l'application et des composants de la solution domotique
 */
class page_parameters extends Page {

    public $vp; // page virtuel pour le formulaire de saisie

    function init(){
        parent::init();

		$this->vp=$this->add('VirtualPage'); // page virtuel pour le formulaire
    }

    
    function page_index()
    /**
     * \brief   Page principale (et unique) affichée par la classe
     * \details Affiche une page avec 4 onglets de gestion des paramètres de l'application :
     *          - onglet Application       : les paramètres généraux de l'application (info connexion MYSQL, XPL, ...)
     *          - onglet Sensors/actuators : gestions des capteurs et actionneurs.
     *          - onglet Interface         : gestions des interfaces
     *          - onglet Types             : gestions des types d'interface et de capteur/actionneur
     * \return  None.
     */
    {
        // tous les $_GET transmis lors des soumissions (click sur boutons)
        $this->api->stickyGET('edit_devices');
        $this->api->stickyGET('edit_application');
        $this->api->stickyGET('edit_interface');
        $this->api->stickyGET('edit_type_o');
        $this->api->stickyGET('edit_type_s');

        if(!$this->vp->isActive()){ // affichage des grilles dans les onglets
            $f=$this->add('Form');
            $tabs=$f->add('Tabs');
            
            // pour sensors/actuators
            $ts=$tabs->addTab('Sensors/Actuators');
            $this->add_grid($ts,'devices','Devices','name');
            
            // pour interface
            $ti=$tabs->addTab('Interfaces');
            $this->add_grid($ti,'interface','InterfacesForCRUDEdition','name');
            
            // pour le type
            $tt=$tabs->addTab('Type');
            $tt->add('H1')->set('Defaults types');
		    $tt->add('P')->set('You can only change "parameters" of defaults types');
            $this->add_grid($tt,'type_s','TypesForCRUDEdition','name',array('allow_add'=>false,'allow_del'=>false,'condition'=>array('id_type','<','2000')));
            $tt->add('H1')->set('Users types');
		    $tt->add('P')->set('Create, edit or delete your own types (id >= 1000)');
            $this->add_grid($tt,'type_o','TypesForCRUDEdition','name',array('condition'=>array('id_type','>=','2000')));
            
            // pour les paramètres de l'application
            $ta=$tabs->addTab('Application');
            $this->add_grid($ta,'application','AppParams','key',array('allow_add'=>false,'allow_del'=>false));
   
        }else{ // affichage des formulaires
        
            $this->add_form($this->vp,'devices','Devices');
            $this->add_form($this->vp,'application','AppParams');
            $this->add_form($this->vp,'type_o','TypesForCRUDEdition');
            $this->add_form($this->vp,'type_s','TypesForCRUDEdition');
            $this->add_form($this->vp,'interface','InterfacesForCRUDEdition');
        }
	}


    function add_grid($f,$name,$model,$display_field,$option=array('allow_add'=>true,'allow_del'=>true,'allow_edit'=>true))
    /**
     * \brief   affiche une grille à partir d'un modèle (db) avec différentes options possibles
     * \details 
     * \param   $f              objet "form" qui va accueillir la grille. Form peut être un objet "Tab"
     * \param   $name           identifiant (string) de la grille utilisé pour la gestion de la grille. Doit être unique
     *                          (deux grilles ne peuvent pas avoir le même nom).
     * \param   $display_field  champ dont la valeur sera affichée dans le titre du formulaire de saisie.
     * \param   $options        liste (array) des options à appliquer à la grille. Les options possibles sont :
     *                          - allow_add  (true(def) | false) : permet d'ajouter un enregistrement (boutton "Add" haut dessus de la grille).
     *                          - allow_del  (true(def) | false) : permet de supprimer un enregistrement (boutton "del" pour chaque ligne).
     *                          - allow_edit (true(def) | false) : permet l'édition d'un enregistrement (boutton "edit" pour chaque ligne).
     * \return  l'objet "grid" créé
     */
     {
        if(!array_key_exists('allow_add',$option) || $option['allow_add']!=false){
           $f->add('Button')->setLabel('Add')->js('click')->univ()->frameURL('Adding...', $this->vp->getURL('add_'.$name));
           $f->add('P');
        }
        $g=$f->add('grid');
        
        if(!array_key_exists('condition',$option)){
           $f->m=$g->setModel($model);
        }else{
           $f->m=$g->setModel($model)->addCondition($option['condition'][0],$option['condition'][1],$option['condition'][2]);
        }
        if(!array_key_exists('allow_edit',$option) || $option['allow_edit']!=false)
           $g->addColumn('button','edit_'.$name,'edit');
        if(!array_key_exists('allow_del',$option) || $option['allow_del']!=false)
           $g->addColumn('delete','del');
        
        $this->js('reload_'.$name, $g->js()->reload());
   
        if($_GET['edit_'.$name]){
            $this->js()->univ()->frameURL('Editing : '.$this->get_field_by_id($g->model,$display_field,$_GET['edit_'.$name]),$this->vp->getURL('edit_'.$name))->execute();
        }
        
        return $g;
    }
    

    function add_form($vp,$name,$model)
    /**
     * \brief   Affiche un formulaire de saisie pour un enregistrement
     * \details Le contenu (champs et remplissages) du formulaire doit être défini par une fonction portant le nom 'form_'.$name
     *          acceptant deux paramètres : $f, objet form ouùajouter les champs/info et $m, le modèle db contenant les données.
     *
     *          Exemple : si $name = 'data' definir une fonction de la façon suivante "function form_data($f,$m)"
     *
     *          Pour la sauvegarde, une fonction 'doSave_'.$name doit être définie et accepter un parametre : $f (objet Form).
     *          Par soucis de simplification, l'objet Form transmis porte une propriété $f->fname contenant le nom du formulaire ($name)
     *          et $f->m qui contient le modèle (objet).
     *
     * \param   $vp     page virtuel qui doit recevoir le formulaire
     * \param   $name   identifiant (string) de la forme utilisé pour la gestion du formulaire. Doit être identique au nom de la
     *                  grille associée.
     * \param   $model  nom du modèle (string et pas objet model). Identifiant du modèle (sans 'Model_').
     */
    {
        if($vp->isActive()=='edit_'.$name || $vp->isActive()=='add_'.$name){
            $p=$vp->getPage(); // affichage du formulaire
            
            $f=$p->add('Form');
            $m=$this->add('Model_'.$model);
            
            $f->fname=$name; // stockage du nom de la forme dans la forme.
            $f->m=$m; // stockage du model dans la forme
            $this->m=$m; // stockage du model dans la page (compatibilité)
            
            if($vp->isActive()=='edit_'.$name){
                $this->m->load($_GET['edit_'.$name]); // chargement des données
            }
            
            $fx='form_'.$name;
            $this->$fx($f,$m);

            $f->addSubmit();
            $f->onSubmit(array($this,'doSave_'.$name));
        }
    }
    

    function get_max($m,$dbfield)
    /**
     * \brief   retourne la valeur max d'un champ d'un table (défini par le model)
     * \param   $m          le modèle de la table à interroger
     * \param   $dbfield    nom du champ (string) à interroger.
     * \return  valeur max du champ
     */
	{
        return (string)$m->dsql()->field('max('.$dbfield.')')->select();
	}

    
    function get_field_by_id($m,$dbfield,$id)
    /**
     * \brief   retourne la valeur d'un champ en fonction de l'id de l'enregistrement
     * \param   $m          le modèle de la table à interroger
     * \param   $dbfield    nom du champ (string) à interroger
     * \param   $id         l'id de l'enregistrement
     * \return  valeur du champ
     */
	{
        return (string)$m->dsql()->field($dbfield)->where('id',$id)->do_getOne();
	}


    function standard_check_and_throw($f,$field,$c)
    /**
     * \brief   Effectue des tests standards sur un champ et lève une exception en cas d'anomalie
     * \param   $f          formulaire contenant la donnée à contrôler
     * \param   $field      nom du champ (string) à contrôler
     * \param   $c          caractères autorisés (ex "a-z" : tous les caractères alpa, "ab" : seulement 'a' et 'b' ...)
     * \return  false en cas d'erreur, true suivant
     */
    {
        if(!strlen($f->get($field))){
            throw $f->exception($field.' is mandaroy','ValidityCheck')->setField($field);
            return false;
        }
        $id=$f->m->dsql()->field('id')->where($field,$f->get($field))->do_getOne();
        if($id && $id!=$f->m['id']){
            throw $f->exception($field.' all-ready exist, choose an other ...','ValidityCheck')->setField($field);
            return false;
        }
        if(preg_match('/[^'.$c.']/i',$f->get($field))){
            throw $f->exception('Only ['.$c.'] allowed','ValidityCheck')->setField($field);
            return false;
        }
        return true;
    }

    
/*****************************************************************************/
    function form_application($f,$m)
    /**
     * \brief   remplissage du formulaire application
     * \param   $f          formulaire à remplir
     * \param   $m          modèle (objet) de la table qui contient les données manipulées par le formulaire
     */
    {
        $f->addField('line','value');
        $f->addField('text','complement');
        $f->set('value',$m['value']);
        $f->set('complement',$m['complement']);
 
    }
    
    
    function form_devices($f,$m)
    /**
     * \brief   remplissage du formulaire "capteurs / actionneur"
     * \param   $f          formulaire à remplir
     * \param   $m          modèle (objet) de la table qui contient les données manipulées par le formulaire
     */
    {
        $f->addField('line','id_sensor_actuator')->setCaption('id');
        $f->addField('line','name');
        $f->addField('Dropdown','type')->setModel('Model_Types');
        $f->addField('Dropdown','interface')->setModel('Model_Interfaces');
        $f->addField('text','description');
        $f->addField('text','parameters');
        $f->addField('Dropdown','state')->setValueList(array(0=>'disable',1=>'enable'));

        if($this->vp->isActive()=='edit_'.$f->fname){
            $f->set('id_sensor_actuator',$m['id_sensor_actuator']);
            $f->getElement('id_sensor_actuator')->disable();
            $f->set('name',$m['name']);
            $f->set('type',$m['id_type']);
            $f->set('interface',$m['id_interface']);
            $f->set('description',$m['description']);
            $f->set('parameters',$m['parameters']);
            $f->set('state',$m['state']);
        }else{
            $f->set('id_sensor_actuator',$this->get_max($m,'id_sensor_actuator')+1);
        }
    }
    
    
    function form_interface($f,$m)
    /**
     * \brief   remplissage du formulaire "interface"
     * \param   $f          formulaire à remplir
     * \param   $m          modèle (objet) de la table qui contient les données manipulées par le formulaire
     */
    {
        $f->addField('line','id_interface')->setCaption('id');
        $f->addField('line','name');
        $f->addField('Dropdown','type')->setModel('Model_Types');
        $f->addField('text','description');
        $f->addField('line','device');
        $f->addField('text','parameters');
        $f->addField('Dropdown','state')->setValueList(array(0=>'disable',1=>'enable',2=>'delegate'));
        
        if($this->vp->isActive()=='edit_'.$f->fname){
            $f->set('id_interface',$m['id_interface']);
            $f->getElement('id_interface')->disable();
            $f->set('name',$m['name']);
            $f->set('type',$m['id_type']);
            $f->set('description',$m['description']);
            $f->set('device',$m['dev']);
            $f->set('parameters',$m['parameters']);
            $f->set('state',$m['state']);
        }else{
            $f->set('id_interface',$this->get_max($m,'id_interface')+1);
        }
    }


    function form_type_o($f,$m)
    /**
     * \brief   remplissage du formulaire "types utilisateurs"
     * \param   $f          formulaire à remplir
     * \param   $m          modèle (objet) de la table qui contient les données manipulées par le formulaire
     */
    {
        // création de la form
        $f->addField('line','id_type')->setCaption('id');
        $f->addField('line','name');
        $f->addField('text','description');
        $f->addField('text','parameters');
            
        if($this->vp->isActive()=='edit_type_o'){ // personalisation de la form si en mode édition
            $f->set('id_type',$m['id_type']);
            $f->getElement('id_type')->disable();
            $f->set('name',$m['name']);
            $f->set('description',$m['description']);
            $f->set('parameters',$m['parameters']);
        }else{
            $f->set('id_type',$this->get_max($m,'id_type')+1);
        }
    }

    
    function form_type_s($f,$m)
    /**
     * \brief   remplissage du formulaire "types standards"
     * \param   $f          formulaire à remplir
     * \param   $m          modèle (objet) de la table qui contient les données manipulées par le formulaire
     */
    {
        // création de la form
        $f->addField('text','parameters');
        $f->set('parameters',$m['parameters']);
    }
    
    
    function doSave_type_o($f)
    /**
     * \brief   sauvegarde des données du formulaire "types utilisateurs"
     * \param   $f          formulaire à sauvegarder
     * \return  false en cas d'erreur
     */
    {
        try{
            // validation de l'id_type (pour la création)
            if($this->vp->isActive('add_'.$f->fname)){
                if($this->standard_check_and_throw($f,'id_type','0-9')){
                    if($f->get('id_type')<2000){
                        throw $f->exception('id_type must be >= 1000','ValidityCheck')->setField('id_type');
                        return false;
                    }
                    $f->m['id_type']=$f->get('id_type');
                }else{
                    return false;
                }
            }
            // validation du nom
            if($this->standard_check_and_throw($f,'name','a-z0-9')){
                $f->m['name']=strtoupper($f->get('name'));
            }else{
                return false;
            }
            
            $f->m['description']=$f->get('description');
            $f->m['parameters']=$f->get('parameters');
               
            $f->m->save();

        }
        catch (Exception_ValidityCheck $e){
            $f->displayError($e->getField(), $e->getMessage());
        }
        return $f->js(null, $this->js(true)->trigger('reload_'.$f->fname))->univ()->closeDialog();
    }

    
    function doSave_type_s($f)
    /**
     * \brief   sauvegarde des données du formulaire "types standards"
     * \param   $f          formulaire à sauvegarder
     * \return  false en cas d'erreur
     */
    {
        try{
              $this->m['parameters']=$f->get('parameters');
              $this->m->save();
        }
        catch (Exception_ValidityCheck $e){
        
            $f->displayError($e->getField(), $e->getMessage());
        }
        return $f->js(null, $this->js(true)->trigger('reload_'.$f->fname))->univ()->closeDialog();
    }

    
    function doSave_interface($f)
    /**
     * \brief   sauvegarde des données du formulaire "interface"
     * \param   $f          formulaire à sauvegarder
     * \return  false en cas d'erreur
     */
    {
        try{
            if($this->vp->isActive('add_'.$f->fname)){
                if($this->standard_check_and_throw($f,'id_interface','0-9')){
                    $f->m['id_interface']=$f->get('id_interface');
                }else{
                    return false;
                }
            }
            if($this->standard_check_and_throw($f,'name','a-z0-9')){
                $f->m['name']=strtoupper($f->get('name'));
            }else{
                return false;
            }
            $f->m['id_type']=$f->get('type');
            $f->m['dev']=$f->get('device');
            $f->m['description']=$f->get('description');
            $f->m['parameters']=$f->get('parameters');
            $f->m['state']=$f->get('state');

            $f->m->save();
        }
        catch (Exception_ValidityCheck $e){
            $f->displayError($e->getField(), $e->getMessage());
        }
        return $f->js(null, $this->js(true)->trigger('reload_'.$f->fname))->univ()->closeDialog();
    }

    
    function doSave_devices($f)
    /**
     * \brief   sauvegarde des données du formulaire "Capteurs / Actionneurs"
     * \param   $f          formulaire à sauvegarder
     * \return  false en cas d'erreur
     */
    {
        try{
            if($this->vp->isActive('add_'.$f->fname)){
                if($this->standard_check_and_throw($f,'id_sensor_actuator','0-9')){
                    $f->m['id_sensor_actuator']=$f->get('id_sensor_actuator');
                }else{
                    return false;
                }
            }
            if($this->standard_check_and_throw($f,'name','a-z0-9')){
                $f->m['name']=strtoupper($f->get('name'));
            }else{
                return false;
            }
            
            $f->m['id_interface']=$f->get('interface');
            $f->m['id_type']=$f->get('type');
            $f->m['description']=$f->get('description');
            $f->m['parameters']=$f->get('parameters');
            $f->m['state']=$f->get('state');

            $f->m->save();
        }
        catch (Exception_ValidityCheck $e){
            $f->displayError($e->getField(), $e->getMessage());
        }
        
        return $f->js(null, $this->js(true)->trigger('reload_'.$f->fname))->univ()->closeDialog();
    }

    
    function doSave_application($f)
    /**
     * \brief   sauvegarde des données du formulaire "Paramétrage application"
     * \param   $f          formulaire à sauvegarder
     * \return  false en cas d'erreur
     */
    {
        try{
              $this->m['value']=$f->get('value');
              $this->m['complement']=$f->get('complement');
              $this->m->save();
        }
        catch (Exception_ValidityCheck $e){
            $f->displayError($e->getField(), $e->getMessage());
        }
        return $f->js(null, $this->js(true)->trigger('reload_'.$f->fname))->univ()->closeDialog();
    }
}
