<?php
class page_parameters extends Page {
    function init(){
        parent::init();

// voir http://agiletoolkit.org/doc/dsql
// voir aussi http://demo39.agiletoolkit.org/demo.html

// Pour faire des graphes : http://www.sterlingend.co.uk/?page=start

        $f=$this->add('Form');
		  $t=$f->add('Tabs');

		  $t3=$t->addTab('Application');
		  $c3=$t3->add('CRUD',array('allow_add'=>false,'allow_del'=>false));
		  $c3->entity_name='Application parameter';
		  $m3=$c3->setModel('AppParams');
        if($c3->form){
		      // $c3->form->getElement('key')->setAttr('disabled'); //->disable()->no_save=null;
		      if($m3['key']=='USER'){
				}
		      if($m3['key']=='PASSWORD'){
//				   $c3->form->getElement('value')->addButton('bouton de test',array('after'));
				}
		  }
		  $t1=$t->addTab('Interfaces');
		  $c1=$t1->add('CRUD');
		  $c1->entity_name='Interface';
		  $m1=$c1->setModel('InterfacesForCRUDEdition');
//		    if ($c1->add_button){
//		      $c1->add_button->setLabel('Add Interface');
//        }
        if($c1->form){
		     $c1->form->getElement('name')->setFieldHint('8 caracters in [a-z][A-Z][0-9]');
		  	  if(!$m1['id_interface']){
			     $c1->form->set('id_interface',$this->get_max($m1,'id_interface')+1);
			  }
//			  else{
//			     $c1->form->getElement('id_interface')->disable();
//			  }
		  }

		  $t4=$t->addTab('Sensors/Actuators');
		  $c4=$t4->add('CRUD');
		  $c4->entity_name='Sensor or Actuator';
		  $m4=$c4->setModel('Devices');
//		    if ($c4->add_button){
//           $c4->add_button->setLabel('Add Sensor or Actuator');
//        }

        if($c4->form){
		     if(!$m4['id_sensor_actuator'])
			      $c4->form->set('id_sensor_actuator',$this->get_max($m4,'id_sensor_actuator')+1);
//			  $b4=$c4->form->getElement('parameters')->addButton('edit',array('after'));
		  }

		  $t2=$t->addTab('Types');
        $t2->add('H1')->set('Manage default types');
		  $t2->add('P')->set('You can only change default types parameters');
		  $c5=$t2->add('CRUD',array('allow_add'=>false,'allow_del'=>false));
		  $c5->setModel('TypesForCRUDEdition',array('name','parameters'),array('id','name','description','parameters'))->addCondition('id_type','<','1000');
		  if($c5->form){
//            $c5->form->getElement('name')->disable();
		  }

        $t2->add('H1')->set('Manage user types');
		  $t2->add('P')->set('Create, edit or delete your own types (id >= 1000)');
		  $c2=$t2->add('CRUD');
        $c2->entity_name='Type';
		  $m2=$c2->setModel('TypesForCRUDEdition')->addCondition('id_type','>=','1000');
//		    if ($c2->add_button){
//            $c2->add_button->setLabel('Add Type');
//        }
        if($c2->form){
		     if(!$m2['id_type'])
			      $c2->form->set('id_type',$this->get_max($m2,'id_type')+1);
//			  else
//               $c2->form->getElement('id_type')->disable();
		  }
	 }
	 
	 function get_max($m,$i)
	 {
	    return (string)$m->dsql()->field('max('.$i.')')->select();
	 }
}
