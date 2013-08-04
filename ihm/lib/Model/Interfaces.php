<?php
class Model_Interfaces extends Model_Table {
    public $entity_code='interfaces'; // nom de la table dans la base
    function init(){
        parent::init();
		  
        /* 		  
		CREATE TABLE interfaces
        (
        id INTEGER PRIMARY KEY,
        id_interface INTEGER,
        id_type INTEGER,
        name TEXT,
        description TEXT,
        dev TEXT,
        parameters TEXT,
        state INTEGER
        );
        */
		  
		$this->id_field='id_interface';
		  
        $this->addField('id_interface')
             ->mandatory(true)
             ->caption('id');
        $this->addField('name')
		     ->mandatory(true)
             ->sortable(true);
        $this->addField('id_type')->caption('type')
             ->refModel('Model_Types')
             ->mandatory(true);
        $this->addField('description')
		     ->type('text');
        $this->addField('dev')
		     ->mandatory(true)
             ->caption('device');
        $this->addField('parameters')
		     ->type('text');
        $this->addField('state')
		     ->mandatory(true)
             ->setValueList(array(0=>'disable',1=>'enable',2=>'delegate'));
		  
//		  $this->addHook('beforeSave',$this);
    }
	 
    function beforeSave(){
	 
	    $identifier='id_interface';
		$name='name';

        $anId=$this->dsql()->field('id')->where($identifier,$this[$identifier])->do_getOne();
        if($anId && $anId!=$this['id']){
            throw $this->exception('Interface identifier all-ready exist, choose an other ...','ValidityCheck')->setField($identifier);
        }

        $anId=$this->dsql()->field('id')->where($name,$this[$name])->do_getOne();
        if($anId && $anId!=$this['id']){
            throw $this->exception('Interface name all-ready exist, choose an other ...','ValidityCheck')->setField($name);
        }
    }
}

