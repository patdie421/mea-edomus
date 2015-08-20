LiveLogController = function(console_id,nbLines)
{
   this.started=0;
   this.whoIsScrollingFlag=0;
   this.scrollOnOffFlag=1;
   this.console=console_id;
   this.nbLines=nbLines;
}

LiveLogController.prototype = {
   append_line: function(line)
   {
      // analyse de la ligne
      var color="black"; // couleur par défaut
      if(line.indexOf("ERROR")==22)
         color="red";
      else if(line.indexOf("WARNING")==22)
         color="orange";
      else if(line.indexOf("INFO")==22)
         color="green";
      else if(line.indexOf("DEBUG")==22)
         color="blue";
      if(line.lenght==0)
         return;
     
      // ajout de la ligne
      $("#"+this.console).append("<div class='log' style='white-space:pre-wrap; color:"+color+";'>"+line+"</div>");
     
      // on retire une ligne si la limite est atteinte
      var logLineDivHigh = 0;
      if($('#'+this.console+" > *").length >= this.nbLines)
      {
         var toRemove = $("#"+this.console+" div:first-child"); // ligne à retirer
         this.whoIsScrollingFlag=1;
         logLineDivHigh = toRemove.height(); // hauteur de la ligne à retirer
         toRemove.remove(); // on la retire
      }

      if(this.scrollOnOffFlag==1)
      { // le scroll est actif
         this.whoIsScrollingFlag=1; // on dis qu'on va générer un événement de scroll (il faudra ne pas en tenir compte)
         $("#"+this.console).scrollTop($("#"+this.console).prop("scrollHeight"));
      }
      else
      {
         // le scroll n'est pas actif
         if(logLineDivHigh) // si une ligne a été retirée
            $("#"+this.console).scrollTop($("#"+this.console).scrollTop()-logLineDivHigh); // je remonte le slider de la hauteur de la ligne retirée
      }
   },
   
   start: function(s) {
      var _logViewer = this;
      $("#"+this.console).scroll(function() {
         if(_logViewer.scrollOnOffFlag==0 && _logViewer.whoIsScrollingFlag==0) // si scroll inactif
         {
            // le slider a-t-il été poussé jusqu'en bas ?
            if($(this).scrollTop() + $(this).innerHeight() >= $(this)[0].scrollHeight)
            { // scroller[0].scrollHeight
               _logViewer.scrollOnOffFlag=1; // on réactive le scroll (live)
            }
         }
         else if(_logViewer.whoIsScrollingFlag==0) // le scroll est activé et c'est l'utilisateur qui à scroller
         {
            _logViewer.scrollOnOffFlag=0; // dans ce cas on désactive le scroll
         }
         else
            _logViewer.whoIsScrollingFlag=0; // remise à 0 du flag.
      });

      s.removeAllListeners('log'); // à revoir si plusieurs logviewer en meme temps
      s.on('log', function(message) {
         _logViewer.append_line(message);
      });
      this.started=1;
   },
   
   scrollBottom: function()
   {
      this.scrollOnOffFlag=1;
      this.whoIsScrollingFlag=1;

      // on se met en bas de la log
      $("#"+this.console).scrollTop($("#"+this.console)[0].scrollHeight); // on scroll à la fin de la div
   },
   
   isStarted: function()
   {
      return this.started;
   }
};

