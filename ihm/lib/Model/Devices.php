<?php
class Model_Devices extends Model_Table {
    public $entity_code='sensors_actuators';
	 
    function init(){
        parent::init();

        /*
        CREATE TABLE sensors_actuators
        (
        id INTEGER PRIMARY KEY,
        id_sensor_actuator INTEGER,
        id_type INTEGER,
        id_interface INTERGER,
        name TEXT,
        description TEXT,
        id_location INTEGER,
        parameters TEXT,
		  state INTEGER
        );
        */
		  
        $this->addField('id_sensor_actuator')->mandatory(true)->caption('id');
        $this->addField('name')->mandatory(true);
		$this->addField('id_type')->refModel('Model_Types')->mandatory(true)->sortable(true);
		$this->addField('id_interface')->refModel('Model_Interfaces')->mandatory(true)->sortable(true);
        $this->addField('description')->type('text');
        $this->addField('parameters')->type('text');
		$this->addField('state')->mandatory(true)->setValueList(array(0=>'disable',1=>'enable',2=>'delegate'));
		  
//		  $this->addHook('beforeSave',$this);
    }

	function beforeSave(){
	    $identifier='id_sensor_actuator';
		$name='name';

        $anId=$this->dsql()->field('id')->where($identifier,$this[$identifier])->do_getOne();
        if($anId && $anId!=$this[$identifier]){
            throw $this->exception('Type identifier all-ready exist, choose an other ...','ValidityCheck')->setField($identifier);
        }
       
        $anId=$this->dsql()->field('id')->where($name,$this[$name])->do_getOne();
        if($anId && $anId!=$this[$identifier]){
            throw $this->exception('Sensor or actuator name all-ready exist, choose an other ...','ValidityCheck')->setField($name);
        }
    }
}
