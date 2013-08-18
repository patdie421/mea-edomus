<?php
class page_types2 extends Page {

    public $vp; // page virtuel pour le formulaire de saisie

    function init(){
        parent::init();
    }

    
    function page_index()
    {
        $this->api->stickyGET('edit_1');
        
		$this->vp=$this->add('VirtualPage'); // page virtuel pour le formulaire

        $m=$this->add('Model_AppParams');
        $q=$m->dsql()->getAll();
        
        if(!$this->vp->isActive()){ // affichage des grilles
            $f=$this->add('Form');
            //$tabs=$f->add('Tabs');
            //$t=$tabs->addTab('Test');
            $t=$f;
            // modification du tableau pour masquer le PASSWORD
            for($x = 0;$x < count($q);$x++){
                if($q[$x]['key'] == 'PASSWORD'){
                    $q[$x]['value']='***';
                }
            }
            $this->add_grid($t,$q,'1');
        }else{ // affichage des formulaires
            if($this->vp->isActive()=='edit_1'){
                $this->add_form($this->vp,'1',$q,$m);
            }
        }
	}


    function add_grid($f,$q,$name)
    {
        $g=$f->add('grid');
        
        $fx='column_'.$name;
        $this->$fx($g,$q);

        $g->addColumn('button','edit_'.$name,'edit');
        $g->setSource($q);
        if($_GET['edit_'.$name]){
            $this->js()->univ()->frameURL('Editing : ',$this->vp->getURL('edit_'.$name))->execute();
        }
        $this->js('reload_'.$name, $g->js()->reload());
    }
    
    
    function add_form($vp,$name,$q,$m)
    {
        if($vp->isActive()=='edit_'.$name || $vp->isActive()=='add_'.$name){
            $p=$vp->getPage(); // affichage du formulaire

            $f=$p->add('Form');
            $f->fname=$name; // stockage du nom de la forme dans la forme.
            $f->m=$m;
            $fx='form_'.$name;
            $this->$fx($f,$q);

            $f->addSubmit();
            $f->onSubmit(array($this,'save_'.$name));
        }
    }

    
    function findId($q,$id)
    {
        for($x = 0;$x < count($q);$x++){
            if($q[$x]['id'] == $id){
                return $x;
            }
        }
        return -1;
    }

    
/*****************************************************************************/
    function column_1($g,$q)
    {
        $g->addColumn('key');
        $g->addColumn('value');
        $g->addColumn('complement');
    }
    
    
    function form_1($f,$q)
    {
       $i=$this->findId($q,$_GET['edit_1']);
        if($i<0)
           return false;
           
        if($q[$i]['key']=='PASSWORD'){
            $f->addField('password','p1','password');
            $f->addField('password','p2','confirmation');
            $f->set('p1',$q[$i]['value']);
            $f->set('p2',$q[$i]['value']);

            
        }elseif($q[$i]['key']=='BUFFERDB'){
            $f->addField('line','value')->addButton('check')->js('click')->univ()->frameURL('Check buffer file', $this->vp->getURL('check_buffer'));
            $f->set('value',$q[$i]['value']);
            
        }elseif($q[$i]['key']=='DBSERVER'){
            $f->addField('line','value')->addButton('check')->js('click')->univ()->frameURL('Check buffer file', $this->vp->getURL('check_dbserver'));
            $f->set('value',$q[$i]['value']);
            $f->addField('line','port')->addButton('check')->js('click')->univ()->frameURL('Check buffer file', $this->vp->getURL('check_dbserverport'));;
            $f->set('port',$q[$i]['complement']);
            
        }else{
            $f->addField('line','value');
            $f->set('value',$q[$i]['value']);
        }
        
        return true;
    }

    
    function save_1($f)
    {
        if (isset($_GET['edit_1'])){
            $id=$_GET['edit_1'];
            $f->m->load($id);
            if($f->m['key']=='PASSWORD')
            {
                if(!strlen($f->get('p1'))){
                    throw $f->exception('Password can\'t be empty')->setField('p1');
                    return false;
                }
                if($f->get('p1') != $f->get('p2')){
                    throw $f->exception('Confirmation does not match password','ValidityCheck')->setField('p2');
                }
                $f->m['value']=$f->get('p1');
            }else{
                $f->m['value']=$f->get('value');
            }
            $f->m['complement']=$f->get('complement');
          
            $f->m->save();
          
            return $f->js(null, $this->js(true)->trigger('reload_'.$f->fname))->univ()->closeDialog();
        }else{
            return false;
        }
    }
}
