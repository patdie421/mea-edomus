function FileChooserUploaderController(attachement)
{
   FileChooserUploaderController.superConstructor.call(this);

   this.attachement = attachement; 
   this.id = "dlg_"+Math.floor((Math.random() * 10000) + 1);
   this.files = false;
}

extendClass(FileChooserUploaderController, FileChooserController);

FileChooserUploaderController.prototype._getHtml = function()
{
   html="<div id='"+id+"' style=\"padding:10px 20px\"> \
            <div id='"+id+"_title' class='ftitle'></div> \
               <form id='"+id+"_fm' method='post' data-options=\"novalidate:false\"> \
                  <div class='fitem'> \
                     <select name='"+id+"_selectfiles' id='"+id+"_selectfiles' size='12' style=\"width:100%;font-family:verdana,helvetica,arial,sans-serif;font-size:12px;\"></select> \
                  </div> \
                  <div class='fitem' style='padding-top:10px;'> \
                     <input type='file' name='image' id='simpleUpload'> \
                  </div> \
                  <div class='fitem' style='padding-top:10px;'> \
                     <input id='"+id+"_filename' style='width:100%;'> \
                  </div> \
               </form> \
         </div>";

   return html;
};


var files;


FileChooserUploaderController.prototype._loadDialog = function(id, _type, response)
{
   var _this = this;

   FileChooserController.prototype._loadDialog.call(_this, id, _type, response);
};
