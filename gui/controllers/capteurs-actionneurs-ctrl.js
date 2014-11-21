function capteursactionneurs_controller(table_id)
{
   console.log($("#"+table_id).text());
   
   $('#'+table_id).edatagrid({
      url:        'models/get_sensorsactuators-jqg_grid.php',
      saveUrl:    'save_user.php',
      updateUrl:  'update_user.php',
      destroyUrl: 'destroy_user.php',
      onError:    function(index,row) { alert(row.msg); }
   });


}