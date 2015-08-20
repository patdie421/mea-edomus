function ViewsController()
{
   this.currentPage="";
   this.views={};
}


ViewsController.prototype = {
   addView: function(viewname,url,tabname) {
      this.views[viewname]={url:url, tabname:tabname};
   },
   
   displayView: function(viewname) {
      if(this.currentPage!=this.views[viewname].url)
      {
         $('#content').panel('refresh',this.views[viewname].url+"?view="+viewname); // on charge la page
         this.currentPage = this.views[viewname].url;
      }
      else if(this.views[viewname].tabname!="")
      {
         $('#'+this.views[viewname].tabname).tabs('select', viewname);
      }
   }
};
