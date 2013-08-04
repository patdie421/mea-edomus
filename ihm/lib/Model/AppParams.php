<?php
class Model_AppParams extends Model_Table {
    public $entity_code='application_parameters';
    function init(){
        parent::init();

        $this->addField('key'); // ->editable(false)->visible(true);
        $this->addField('value')->mandatory(true);
        $this->addField('complement')->type('text');
    }
}
