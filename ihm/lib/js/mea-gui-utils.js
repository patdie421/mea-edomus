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

// classe bouton
function BoutonPush(id,label){
    this.id=id;
    if(label)
        this.label=label;
    else
        this.label="PUSH";
    this.isRendered=false;
    
    this.setOnClick = function(fx,data){
        if(fx)
        {
            this.onClick=fx;
            if(data)
                this.onClickData=data;
            return true;
        }
        return false;
    }
    
    this.render = function(selector){
        if(this.isRendered==true)
            return false;

        if(this.id && this.label && this.onClick){
            $(selector).append("<DIV id='"+this.id+"' class='bouton'>");
            $('#'+this.id).disableSelection()
                          .text(this.label)
                          .click({me: this}, this.onClick)
                          .hover(function(){ // annimation des boutons
                                    // souris sur le bouton
                                    $(this).css('background-color','#ff00ff'); // changement de couleur
                                    $(this).css('cursor','pointer'); // changement du curseur la main pour cliquer
            }                   ,function(){
                                    // souris hors du bouton
                                    $(this).css('background-color','#00ff00');
                                    $(this).css('cursor','auto');
                                });
            this.isRendered=true;
            return true;
        }
        return false;
    }
}

// class Section
function Section(id,label){
    this.id=id;
    this.label=label;
    this.isRendered=false;
    
    this.addBouton=function(bouton){
        this.bouton=bouton;
    }
    
    this.render=function(selector){
       if(this.isRendered==true)
            return false;

       $('#'+selector).append("<DIV id='" + this.id + "' class='section'>");
       $('#'+this.id).append("<DIV id='" + this.id + "_0' class='texte'>"+this.label);
       
       this.bouton.render('#'+this.id);
       
       $('#'+this.id).append("<DIV id='"+this.id+"_1' class='statut'>");
       $('#'+this.id).append("<DIV id='"+this.id+"_2' class='statut'>");
       
       $('#'+this.id+'_2').css('background-color','red').text("Cliquer Moins vite !!!");
       
       this.isRendered=true;
 
       return this.id;
    }
    
    this.info = function(string){
        $('#'+this.id+"_1").css('background-color','#00ff00');
        if(string)
            $('#'+this.id+"_1").text(string);
        $('#'+this.id+"_1").fadeIn(500).fadeOut(500);
    }

    this.alert = function(string){
        if(string)
            $('#'+this.id+"_2").text(string);
        $('#'+this.id+"_2").fadeIn(50).delay(75).fadeOut(50);;
    }
}

// class Sections
function Sections(selector,name){
    this.list=[];
    this.selector=selector;
    this.name=name;
    this.isRendered=false;
    
    this.addSection=function(section){
        this.list.push(section);
    }

    this.removeSection=function(sectionId){
        for (var i = 0, c = this.list.length; i < c; i++){
            if(this.list[i].id==sectionId)
                $('#'+this.list[i].id).remove();
        }
    }

    this.render=function(){
        if(this.isRendered==true)
            return false;
        
        if(this.selector){
            $(this.selector).append("<DIV id='"+this.name+"'>");
        
            for (var i = 0, c = this.list.length; i < c; i++){
                prev=this.list[i].render(this.name);
            }
            this.isRendered=true;
            return true;
        }
        return false;
    }
}

function mea_alert(title, text, fx){
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
