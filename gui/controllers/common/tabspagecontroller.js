/*
if(typeof(CommonController)=="undefined") { // TabsPageController héritera de CommonController ...
   window.location = "error.html";
}
*/
function TabsPageController(tabs_id)
{
   TabsPageController.superConstructor.call(this);
   this.tabs_id=tabs_id;
}

extendClass(TabsPageController, CommonController);

TabsPageController.prototype.start=function(tabName)
{
   var _tabsPageController = this;
   $('#'+_tabsPageController.tabs_id).tabs ({
      onSelect:function(tabName) {
         if(_tabsPageController._isLoggedIn()==false) {
            $.messager.alert(_tabsPageController._toLocalC('error')+_tabsPageController._localDoubleDot(),_tabsPageController._toLocalC('you are not logged in'),function(){ window.location = "login.php?view="+tabName;});
            return false;
         }
         return true;
      },
      selected: tabName
   });   
};
   
TabsPageController.prototype.selectTab=function(tabName) {
   $('#'+this.tabs_id).tabs('select', tabName);
};

