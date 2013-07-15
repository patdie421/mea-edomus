<?php
class page_index extends Page {
    function init(){
        parent::init();
        $page=$this;

		  if($this->api->auth->isLoggedIn()){
            $this->api->redirect('parameters');
		  }

		  $this->add('BasicAuth')->allow('demo','demo')->check();
		  
        // Adding view box with another view object inside with my custom HTML template
//        $this->add('View_Info')->add('View',null,null,array('view/myinfobox'));

        // Paste any Agile Toolkit examples BELOW THIS LINE. You can remove what I have here:

        // Adding a View and chaining example
//        $this->add('H1')->set('Hello World from your own copy of Agile Toolkit');

        // Assign reference to your object into variable $button
//        $button = $page->add('Button')->setLabel('Refresh following text with AJAX');

        // You can store multiple references, different views, will have different methods
//        $lorem_ipsum = $this->add('LoremIpsum')->setLength(1,200);

        // Bind button click with lorem_ipsum text reload
//        $button->js('click',$lorem_ipsum->js()->reload());


//		$this->add('Form')->addField('line','foo')->validateNotNull();

        // Oh and thanks for giving Agile Toolkit a try! You'll be excited how simple
        // it is to use.
		  
		  
//		  $f->add('grid')->setModel('Types'); // ->addCondition('id_type','<',1000); // affiche les types prédéfinis
//		  $f->add('CRUD')->setModel('Types'); // ->addCondition('id_type','>=',1000); // CRUD pour les types personnalisés

/*
		  $dsn = array('sqlite:/temp/mea-edomus.db');
		  $mydb=$this->add('DB')->connect($dsn);
		  $q = $mydb->dsql();
		  $q->table('interfaces')->join('types.id_type','id_type');
		  
		  $q->field(array('monid'=>'id'),'interfaces') // id est réservé, on renomme avec un alias
			 ->field('id_interface','interfaces')
			 ->field(array('type'=>'name'),'types')
			 ->field('name','interfaces')
			 ->field('description','interfaces');
		  
        $grid = $f->add('Grid');
        $grid->addColumn('monid');
        $grid->addColumn('name');
        $grid->addColumn('type');
        $grid->addColumn('description');
		  $grid->addColumn('button','modifier','Modifier');
        $grid->setSource( $q );
		  
		  $f->addSeparator();
		  
		  $f->addField('line','prenom')
		    ->setFieldHint('C\'pour dire bonjour ...')
			 ->validateNotNull('Un prénom est obligatoire')
			 ->setCaption('Ton prénom ?');
*/ 
		  
// ouvrir une autre page ATK4
/*
        $f->addSubmit('Hello');
		  
		  $f->addButton('Sign-up')->js('click')
          ->univ()->location($this->api->getDestinationURL('authtest'));
				
*/
    }
}