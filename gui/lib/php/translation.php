<?php
$localStrings=array();

function mea_loadTranslationData($LANG,$base)
{
   global $localStrings;

   $data=file_get_contents($base."lib/translation_".$LANG.".json");
   error_log($base."lib/translation_".$LANG.".json");
   if($data === false)
   {
      error_log("Can't open translation file");
      exit(1);
   }
   else
   {
      $localStrings=json_decode($data);
   }
}


function mea_toLocalStr($string)
{
   global $localStrings;

   $str=strtolower($string);
   if(isset($localStrings->{$str}))
      return $localStrings->{$str};
   else
      return $str;
}


function mea_toLocal($string)
{
   global $localStrings;

   echo mea_toLocalStr($string);
}


function mea_toLocalC($string)
{
   global $localStrings;

   $str=mea_toLocalStr($string);
   echo ucfirst($str);
}


function mea_toLocalC_2d($string)
{
   global $localStrings;

   mea_toLocalC($string);
   echo ":&nbsp";
}
