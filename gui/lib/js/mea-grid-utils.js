function ajax_error(xhr, ajaxOptions, thrownError){
    alert("mea-grid-utils.js : responseText="+xhr.responseText+" status="+xhr.status+" thrownError="+thrownError);
}


function check_name_string(value,_name){
    var regx = /^[A-Za-z0-9]+$/;
    
    if(!regx.test(value))
        return [false, _name+" - caractères autorisés : a-zA-Z0-9"];
    
    if(value.length>16)
        return [false, _name+" - 16 caractères maximum"];
    
    return [true];
}


function check_field_exist_by_id(grid,table,fieldName,fieldValue){
    var passes;
    var currentId=$('[name="'+grid+'_id"]').val();
    if(currentId!="_empty")
        wsdata={table:table, field:fieldName, value:fieldValue, id:currentId};
    else
        wsdata={table:table, field:fieldName, value:fieldValue};
    
    $.ajax({ url: 'models/check_field-jqg_grid.php',
           async: false,
           type: 'GET',
           dataType: 'json',
           data: wsdata,
           success: function(data){
                if(data.exist==0)
                    passes=true;
                else
                    passes=false;
           },
           error:ajax_error
           });
    
    return passes;
}


function _DataInit(element,flag,ws){
    if(flag){
        $(element).attr("readonly", "readonly"); // à améliorer
        $(element).css({color: "#b9b9b9", background: "#E0E0E0"});
    } else {
        $.get(ws,
              {},
              function(data){
              $(element).val(data.next_id);
              },
              "json"
              );
    }
}


function _editRow(grid, fx, fx2) {
    var grid = jQuery("#"+grid);
    var rowKey = grid.getGridParam("selrow");
    
    if (rowKey) {
        if(!fx2)
            fx2=function(){$("#name").focus();};
        grid.editGridRow( rowKey,
                        { width:700,
                          recreateForm: true,
                          viewPagerButtons:false,
                          beforeInitData: function () {fx();},
                          closeAfterEdit: true,
                          afterShowForm: function() { fx2();},
                        });
    } else {
        mea_alert2(str_title_warning, str_select_row);
    }
}


function _deleteRow(grid) {
    var grid = jQuery("#"+grid);
    var rowKey = grid.getGridParam("selrow");
    
    if (rowKey) {
        grid.delRowData(rowKey);
    } else {
        mea_alert2(str_title_warning.capitalize(), str_select_row.capitalize());
    }
}


function _addRow(ngrid,fx) {
    var grid = jQuery("#"+ngrid);
    
    grid.editGridRow("new",
                        { width:700,
                          recreateForm: true,
                          viewPagerButtons: false,
                          beforeInitData: function () {fx();},
                          closeAfterAdd: true,
                        });
}


function add_button_to_toolbar(grid,bouton,label,fx1,icon){
    $("<button id="+grid+"_"+bouton+">"+label+"</button>").button({icons: {primary: icon}})
        .css({
            float: "left",
            height: "30px",
            width: "auto",
            margin: "5px",
            'padding-left': "10px",
            'padding-right': "10px",
            'font-size': "14px"
        })
        .appendTo("#t_"+grid)
        .click(fx1);
}


function db_delete(grid,table,id){
    $.ajax({ url: 'models/delete_row_by_id-jqg_grid.php',
           async: false,
           type: 'GET',
           dataType: 'json',
           data: {table:table,id:id},
           success: function(data){
                $("#"+grid).trigger("reloadGrid", [{current: true}]);
           },
           error: ajax_error
    });
}


function deconnect_user(userid){
    $.ajax({ url: 'models/deconnect_user.php',
           async: false,
           type: 'GET',
           dataType: 'json',
           data: {user:userid},
           success: function(data) {
           },
           error: ajax_error
    });
}
