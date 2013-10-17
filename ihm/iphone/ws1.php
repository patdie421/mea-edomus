<?php

   require 'xpl_class.php';

   $sender=new xPLSender;
   
   $msg=new xPLMsg("xpl-cmnd");
   
   $msg->setSource("mea-edomus.phpsender");
   $msg->setTarget("*");
   
   $msg->setSchema("control.basic");
   
   $msg->addBodyItem("type","output");
   $msg->addBodyItem("current","pulse");
   $msg->addBodyItem("data1","125");
   
   $msg->addBodyItem("device",$_GET['device']);

   $msg->send($sender);
   
   echo json_encode(array("retour"=>"OK","device"=>$_GET['device']));
