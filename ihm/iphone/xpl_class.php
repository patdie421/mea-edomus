<?php
   
   class xPLSender {
      
      private $port = 3865;
      private $listenOnAddress="ANY_LOCAL";
      private $broadcast = "255.255.255.255";

      public function send($msg) {
         
         if(!function_exists('socket_create')) {
            trigger_error( 'Sockets are not enabled in this version of PHP', E_USER_ERROR );
         }
         
         // create low level socket
         if(!$socket = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP)) {
            trigger_error('Error creating new socket',E_USER_ERROR);
         }
         
         // Set the socket to broadcast
         if(!socket_set_option( $socket, SOL_SOCKET, SO_BROADCAST, 1)) {
            trigger_error('Unable to set socket into broadcast mode', E_USER_ERROR);
         }
         
         // If the listenOnAddress is not set to ANY_LOCAL, we need to bind the socket (we can use any port).
         if($this->listenOnAddress != "ANY_LOCAL") {
            if(!socket_bind( $socket, $this->listenOnAddress, 0)) {
               trigger_error('Error binding socket to ListenOnAddress', E_USER_ERROR);
            }
         }
         
         //Send the message
         if(FALSE === socket_sendto($socket, $msg, strlen($msg), 0, $this->broadcast, $this->port)) {
            trigger_error('Failed to send message', E_USER_ERROR);
         }
         
         // We're done
         socket_close($socket);
      }
   }


   class xPLMsg {
      // xpl msg type identifier
      private $identifier = '';
      // header
      private $hop = 1;
      private $source = '';
      private $target = '';
      // xpl msg schema identifier (class.type)
      private $schema = '';
      // body
      private $body = array();
      
      
      public function __construct($i) {
         if(self::isIdentifier($i)==true)
            $this->identifier=$i;
      }
      
      public function send($xplSender) {
         if(is_int($this->hop) &&
            $this->hop > 0 &&
            $this->hop < 9 &&
            $this->source != "" &&
            $this->target != "" &&
            $this->schema != "" &&
            count($this->body) > 0) {
            $xplSender->send($this->toString());
         }
         else {
            return false;
         }
      }
      
      public static function isIdentifier($i) {
         switch($i)
         {
            case "xpl-cmnd":
            case "xpl-stat":
            case "xpl-trig":
               return true;
               break;
            default:
               return false;
               break;
         }
      }
      
      public static function isSchema($s) {
         $s=trim($s);
         if(preg_match("#^[a-z0-9\-]{1,8}.[a-z0-9\-]{1,8}$#",$s))
            return true;
         else
            return false;
      }
      
      public static function isAdress($a) {
         $a=trim($a);
         if(preg_match("#^[a-z0-9]{1,8}-[a-z0-9]{1,8}.[a-z0-9\-]{1,16}$#",$a))
            return true;
         else
            return false;
      }
      
      public function setIdentifier($i) {
         $i=trim($i);
         if(self::isIdentifier($i)) {
            $this->identifier=$i;
            return true;
         }
         else
            return false;
      }
      
      public function setSchema($s) {
         $s=trim($s);
         if(self::isSchema($s)) {
            $this->schema=$s;
            return true;
         }
         else {
            $this->schema='';
            return false;
         }
      }
      
      public function setSource($s) {
         $s=trim($s);
         if(self::isAdress($s)) {
            $this->source=$s;
            return true;
         }
         else {
            $this->source='';
            return false;
         }
      }
      
      public function setTarget($t) {
         $t=trim($t);
         if(self::isAdress($t) || $t=='*') {
            $this->target=$t;
            return true;
         }
         else {
            $this->target='';
            return false;
         }
      }
      
      public function addBodyItem($key, $value) {
         $k=trim($key);
         if(preg_match("#^[a-z0-9\-]{1,16}$#",$k)) {
            $this->body[$k] = trim($value);
            return true;
         }
         else {
            return false;
         }
      }
      
      public function bodyItem( $key ) {
         if (array_key_exists($key, $this->body)) {
            return $this->_body[$key];
         }
         return null;
      }
      
      private function __headerToString() {
         $msg ="{\n";
         $msg.="hop=".$this->hop."\n";
         $msg.="source=".$this->source."\n";
         $msg.="target=".$this->target."\n";
         $msg.="}\n";
         
         return $msg;
      }
      
      private function __bodyToString() {
         $msg="{\n";
         foreach( $this->body as $key => $value ) {
            $msg .= sprintf("%s=%s\n", $key, $value);
         }
         $msg.="}\n";
         
         return $msg;
      }
      
      public function toString() {
         $msg="";
         $msg.=$this->identifier."\n";
         $msg.=$this->__headerToString();
         $msg.=$this->schema."\n";
         $msg.=$this->__bodyToString();
         
         return $msg;
      }
   }
