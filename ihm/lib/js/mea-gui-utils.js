jQuery.fn.extend({ 
   disableSelection : function() { 
      return this.each(function() { 
         this.onselectstart = function() { return false; }; 
         this.unselectable = "on"; 
         jQuery(this).css('user-select', 'none'); 
         jQuery(this).css('-o-user-select', 'none'); 
         jQuery(this).css('-moz-user-select', 'none'); 
         jQuery(this).css('-khtml-user-select', 'none'); 
         jQuery(this).css('-webkit-user-select', 'none');
     }); 
   } 
});

/*
function _mea_alert(title, text, fx){
    $("#dialog-confirm-text").html(text);
    $( "#dialog-confirm" ).dialog({
        title: title,
        resizable: true,
        height:250,
        width:500,
        modal: true,
        buttons: {
            Ok: function() {
                $( this ).dialog( "close" );
                fx();
            }
        }
    });
}
*/

String.prototype.capitalize = function() {
    return this.charAt(0).toUpperCase() + this.slice(1);
}


function mea_alert2(title, text, fx){
    if($("#mea-utils-dialog-confirm").length==0) {
        $('body').append("<div id=\"mea-utils-dialog-confirm\" title=\"Default\" style=\"display:none;\"><p><div style=\"float: left; width:10%\"><span class=\"ui-icon ui-icon-alert\"></span></div><div id=\"mea-utils-dialog-confirm-text\" style=\"float: right; width:90%\">Default<div></p></div>");
    }
    
    $("#mea-utils-dialog-confirm-text").html(text);
    $( "#mea-utils-dialog-confirm" ).dialog({
        title: title,
        resizable: true,
        height:250,
        width:500,
        modal: true,
        buttons: {
            Ok: function() {
                $( this ).dialog( "close" );
                fx();
            }
        }
    });
}

function mea_yesno(title, text, yesFunc, yesFuncParams){
    $("#dialog-confirm-text").html(text);
    $( "#dialog-confirm" ).dialog({
        title:title,
        resizable: true,
        height:250,
        width:500,
        modal: true,
        buttons: {
            Oui: function() {
                $( this ).dialog( "close" );
                yesFunc(yesFuncParams);
            },
            Non: function() {
                $( this ).dialog( "close" );
            }
        }
    });
}

function mea_yesno2(title, text, yesFunc, yesFuncParams){
    if($("#mea-utils-dialog-confirm").length==0) {
        $('body').append("<div id=\"mea-utils-dialog-confirm\" title=\"Default\" style=\"display:none;\"><p><div style=\"float: left; width:10%\"><span class=\"ui-icon ui-icon-help\"></span></div><div id=\"mea-utils-dialog-confirm-text\" style=\"float: right; width:90%\">Default<div></p></div>");
    }
 
    $("#mea-utils-dialog-confirm-text").html(text);
    $( "#mea-utils-dialog-confirm" ).dialog({
        title:title,
        resizable: true,
        height:250,
        width:500,
        modal: true,
        buttons: {
            Oui: function() {
                $( this ).dialog( "close" );
                yesFunc(yesFuncParams);
            },
            Non: function() {
                $( this ).dialog( "close" );
            }
        }
    });
}
