extendClass(MeaWidget_pastille, MeaWidget);

var meaWidget_pastille = new MeaWidget_pastille("MeaWidget_pastille", "basic", "pastille");


function MeaWidget_pastille(name, group, type)
{
   MeaWidget_pastille.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);

   this.valueName = "value";

   this.params["values"][this.valueName] = "";
}


MeaWidget_pastille.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   widget.find('div[mea_widgetvaluename="'+_this.valueName+'"]').attr("name",_this.getValue(data, _this.valueName, ""));
}


MeaWidget_pastille.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width:48px;height:48px;line-height:48px;text-align:center;background-color:\"#FFFFFF\";'> \
                <div style='margin:auto;vertical-align:middle;display:inline-block;width:30px;height:30px;border-radius:15px;background-color:red;'></div> \
             </div>";

   return html;
}


MeaWidget_pastille.prototype.disabled = function(id,d) {
   var _this = this;
   var widget = $("#"+id);

   if(d===true)
      widget.find('div[mea_widgetvaluename="'+_this.valueName+'"]').removeClass('mea_pastille_red').removeClass('mea_pastille_green').addClass('mea_pastille_disable');
}


MeaWidget_pastille.prototype.getFormaters = function()
{
   var formaters = {
      greenred: function(v,o) {
         o.removeClass('mea_pastille_disable');
         if(v === true || v != 0 || v === "true")
         {
            o.addClass('mea_pastille_green');
            o.removeClass('mea_pastille_red');
         }
         else
         {
            o.addClass('mea_pastille_red');
            o.removeClass('mea_pastille_green');
         }
      } 
   };

   return formaters;
}


MeaWidget_pastille.prototype.getHtml = function()
{
var _this = this;
var html =
" \
\
<style> \
.mea_pastille { \
   width:30px; \
   height:30px; \
   border-radius: 15px; \
} \
.mea_pastille_red { \
   background:red; \
} \
.mea_pastille_disable { \
   background:black; \
} \
.mea_pastille_green { \
   background:green; \
} \
</style> \
<div id = '"+_this.type+"_model' \
   mea_widget = '"+_this.type+"\' \
   class = 'mea-widget' data-widgetparams='"+JSON.stringify(_this.params)+"' \
   style = 'position:absolute; width: 30px; height: 30px; \
> \
   <div class='ui-widget-content'  style='width:100%;height:100%;text-align:center;'> \
      <div class='mea_pastille mea_pastille_disable' mea_valueformater='greenred' mea_widgetvaluename='"+this.valueName+"'></div> \
   </div> \
</div> \
\
";
return html;
}
