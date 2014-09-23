<?php
//
//  SOUS-PAGE (SUB-VIEW) de automate.php :
//
?>
<div style="text-align:center; margin:auto;">
<div id="tabs1">
    <ul>
        <li><a href="#tabs1-1" id='inputs'>inputs</a></li>
        <li><a href="#tabs1-2" id='outputs'>outputs</a></li>
        <li><a href="#tabs1-3" id='program'>program</a></li>
    </ul>
    <div id="tabs1-1">
        <table id="table_inputs"></table>
        <div id="pager_inputs"></div>

    </div>
    <div id="tabs1-2">
    </div>
    <div id="tabs1-3">
    </div>
</div>
<div id="aff">
</div>
</div>

<script type="text/javascript" src="lib/js/mea-auth-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-gui-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-grid-utils.js"></script>

<script type="text/javascript" src="controlers/grid_inputs.js"></script>

<script>

jQuery(document).ready(function(){
   $( "#tabs1" ).tabs();
   grid_inputs();
});
</script>


<style>
   .ui-widget{font-size:14px;}

   #feedback { font-size: 1em; }
   #selectable .ui-selecting { background: #FECA40; }
   #selectable .ui-selected { background: #F39814; color: white; }
   #selectable { list-style-type: none; margin: 0; padding: 0; }
   #selectable li { padding: 0.4em; font-size: 1em; height: 1em; }

//   label, input { display:block; }
   label { font-size:0.8em; }
   input.text { text-align:center; margin-bottom:12px; width:100%; padding-top:0.4em; padding-bottom:0.4em; }
   fieldset { padding:0; border:0; margin-top:25px; }

   .ui-dialog { padding: .3em; }
   .validateTips { border: 1px solid transparent; padding-bottom: 0.3em; }
   .mea_disabled { color: #b9b9b9; background: #E0E0E0; }
</style>


<div id="dialog-form" title="Règle">
   <div id="tips" class="validateTips">
      Associer un message xPL à une entrée de l'automate.
   </div>

   <label for="key" style="font-size:0.9em;">Conditions :</label>

   <div class="ui-widget-content" style="padding:15px;">
      <table>
         <tr>
         <td width="265px">
            <label>xPL message from (source address) :</label>
            <input id="xplsource" name="xplsource" style="width:265px" class="text ui-widget-content ui-corner-all">
         </td>
         <td width="24px">
         </td>
         <td width="265px">
            <label>With xPL Schema :</label>
            <input id="xplschema" name="xplschema" style="width:265px" class="text ui-widget-content ui-corner-all">
         </td>
         </tr>
      </table>

      <table>
         <tr>
         <td valign="top" align="center" width="220px">
            <div>
               <label style="text-align:left;">xPL body key/value condition :</label>
               <div class="ui-widget-content" style="display:table;">
                  <div style="display:table-cell; vertical-align: middle; padding:5px">
                     <label for="key" style="text-align:left;">Value for key :</label>
                     <input id="key" name="key" style="width:205px" class="text ui-widget-content ui-corner-all">
                     <div style="text-align:center">
                        <select id="op" class="ui-widget-content ui-corner-all" style="margin-bottom:5px;">
                           <option>=</option>
                           <option>!=</option>
                           <option>></option>
                           <option>=></option>
                           <option><</option>
                           <option>=<</option>
                        </select>
                     </div>
                     <label for="value" style="text-align:left;">A value (string or numeric) :</label>
                     <input id="value" name="value" style="width:205px" class="text ui-widget-content ui-corner-all">
                  </div>
               </div>
            </div>
         </td>

         <td width="105px">
            <div style="display:table; vertical-align: middle; align:center; width:100px;">
               <div style="display:table-cell;">
                  <button id="add_condition" style="width:60px; display:block; margin:5px; margin-left:auto; margin-right:auto;">add</button>
                  <button id="del_condition" style="width:60px; display:block; margin:5px; margin-left:auto; margin-right:auto;">del</button>
               </div>
            </div>
         </td>

         <td width="220px">
            <label>xPL body conditions list :</label>
            <div class="ui-widget-content" style="width:230px; height:150px; overflow:auto">
               <ol id="selectable" style="text-align:center">
               </ol>
            </div>
         </td>
         </tr>
      </table>

   </div>
   <p></p>
   <label for="key" style="font-size:0.9em;">Set :</label>

   <div class="ui-widget-content" style="padding:10px; padding-down:5px">
      <table>
         <tr>

         <td width="205px" valign="top">
            <label>Input type :</label>
            <div id="radio1" style="margin-right:5px; padding:5px; text-align:center;" class="ui-widget-content ui-corner-all ">
               <input type="radio" class="picker" id="digital" name="radio1" checked='true' style="display:inline;"/>
               <label for="digital" style="display:inline;">Digital</label>
               <input type="radio" class="picker" id="analog" name="radio1" style="display:inline;"/>
               <label for="analog" style="display:inline;">Analog</label>
            </div>
            <p></p>
            <label for="inputindex">input index number :</label>
            <input id="inputindex" name="inputindex" class="text ui-widget-content ui-corner-all" style="width:165px;">
         </td>

         <td width="24px">
         </td>

         <td width="340px" valign="top">
            <label for="setvalue" style="padding-left:5px;">Value to set :</label>
            <div id="radio3" style="text-align:left; margin-left:5px; margin-right:5px; padding:10px;" class="ui-widget-content ui-corner-all">
               <div>
                  <input type="radio" value=1 class="picker" id="true" name="radio2" checked='true' style="display:inline;"/>
                  <label for="digital"  style="display:inline;">TRUE</label>
               </div>
               <div>
                  <input type="radio" value=2 class="picker" id="false" name="radio2" style="display:inline;"/>
                  <label for="analog"  style="display:inline;">FALSE</label>
               </div>
               <div>
                  <input type="radio" value=3 class="picker" id="keyvalue" name="radio2"  style="display:inline;"/>
                  <label for="keyvalue" style="display:inline;">value from body key :</label>
                  <input id="key2" name="key2" class="mea_disabled ui-widget-content ui-corner-all" style="text-align:center; display:inline; width:150px;">
               </div>
            </div>
         </td>
 
         </tr>
      </table>
 
   </div>
</div>
