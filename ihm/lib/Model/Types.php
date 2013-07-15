<?php
class Model_Types extends Model_Table {
    public $entity_code='types';
	 
    function init(){
        parent::init();

		  /*
		  CREATE TABLE types
        (
        id INTEGER PRIMARY KEY,
        id_type INTEGER,
        name TEXT,
        description TEXT,
        parameters TEXT
        );
        */

		  $this->id_field='id_type';
		  
        $this->addField('id_type')->mandatory(true)->caption('id');
        $this->addField('name')->mandatory(true)->sortable(true);
        $this->addField('description')->type('text');
        $this->addField('parameters')->type('text');
		  
        $this->addHook('beforeSave',$this);
        $this->addHook('beforeDelete',$this);		  
    }
	 
	 function beforeDelete(){
	//	 throw $this->exception('Have child records!');
    }
	 
	 function beforeSave(){
	 
	     $identifier='id_type';
		  $name='name';
		  
		  if ($this[$identifier]<1000)
		     throw $this->exception('Type identifier must be greater than 1000 ...','ValidityCheck')->setField($identifier);
		  else if($this->dsql()->field('id')->where($identifier,$this[$identifier])->do_getOne())
		     throw $this->exception('Type identifier all-ready exist, choose an other ...','ValidityCheck')->setField($identifier);
		  else if($this->dsql()->field('id')->where($name,$this[$name])->do_getOne())
		     throw $this->exception('Type name all-ready exist, choose an other ...','ValidityCheck')->setField($name);

	 }
}
