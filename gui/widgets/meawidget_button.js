extendClass(MeaWidget_button, MeaWidget);

var meaWidget_button = new MeaWidget_button("MeaWidget_button", "basic", "button");

function MeaWidget_button(name, group, type)
{
   MeaWidget_button.superConstructor.call(this);

   this.toWidgetsJarAs(name, group, type);

   this.action = "push";

   this.params["labels"]["label"] = "push me";
   this.params["actions"][this.action] = '{"xplsend":{"current":"high"}}';
}


MeaWidget_button.prototype.init = function(id)
{
   var _this = this;
   var widget = $("#"+id);

   var data = widget.prop('mea-widgetdata');
   var button = widget.find('div[mea_widgetactionname="'+_this.action+'"]');
   if(data)
   {
      button.html(_this.getValue(data, "label", false));
   }
}


MeaWidget_button.prototype.disabled = function(id,d)
{
   var _this = this;
   var widget = $("#"+id);
   var data = widget.prop('mea-widgetdata');

   var button = widget.find('div[mea_widgetactionname="'+_this.action+'"]');
   if(d===true)
   {
      button.off('click');
      button.removeClass('mea_button_anim');
      widget.removeClass("mea_button_enabled");
      widget.addClass("mea_button_disabled");
   }
   else
   {
      button.on('click', function() {
         _this.doAction(_this.action, data);
      });
      button.addClass('mea_button_anim');
      widget.removeClass("mea_button_disabled");
      widget.addClass("mea_button_enabled");
   }
}


MeaWidget_button.prototype.getTip = function()
{
   return "simple green button";
}


MeaWidget_button.prototype.getHtmlIcon = function()
{
   var _this = this;

   var html="<div id='"+_this.type+"' class='drag' style='width: 80px; height: 80px; background-image:url(/widgets/meawidget_button.png);'></div>";

   return html;
}


MeaWidget_button.prototype.getStyle = function()
{
var style="\
.mea_button { \
   width:90px; \
   height:25px; \
   line-height: 25px; \
   border:4px solid green; \
   border-radius: 5px; \
   text-align: center; \
   vertical-align: middle; \
   color:green; \
   font-weight:bold; \
} \
.mea_button_disabled { \
   border-style: dotted; \
   border-width: 1px; \
   border-color: gray; \
   opacity: 0.5; \
} \
.mea_button_enabled { \
   border-style: solid; \
   border-width: 1px; \
   border-color: transparent; \
} \
.mea_button_anim { \
} \
.mea_button_anim:hover { \
    background-color: lightgreen; \
    cursor: pointer; \
} \
.mea_button_anim:active { \
   background-color: green; \
   color:lightgreen; \
}";
return style;
}


MeaWidget_button.prototype.getHtml = function()
{
var _this = this;
var html ="\
<div id = '"+_this.type+"_model' \
     mea_widget = '"+_this.type+"' \
     style = 'position:absolute;width:98px;height:31px;' \
     class = 'mea-widget mea_button_disabled' \
     data-widgetparams='"+JSON.stringify(_this.params)+"' \
> \
   <div style='width:100%;height:100%;text-align:center;'> \
      <div class='mea_button' style='overflow:hidden;' \
           mea_widgetactionname='push'> \
         "+this.params['labels']['label']+" \
      </div> \
   </div> \
</div> \
";
return html;
}
