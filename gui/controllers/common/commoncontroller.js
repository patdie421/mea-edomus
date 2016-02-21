String.prototype.mea_hexDecode = function(){
    var j;
    var hexes = this.match(/.{1,4}/g) || [];
    var back = "";
    for(j = 0; j<hexes.length; j++) {
        back += String.fromCharCode(parseInt(hexes[j], 16));
    }

    return back;
}


String.prototype.mea_hexEncode = function(){
    var hex, i;

    var result = "";
    for (i=0; i<this.length; i++) {
        hex = this.charCodeAt(i).toString(16);
        result += ("000"+hex).slice(-4);
    }

    return result
}


// voir aussi pour les class : http://www.42hacks.com/notes/fr/20111213-comment-ecrire-du-code-oriente-objet-propre-et-maintenable-en-javascript/
function CommonController()
{
   CommonController.superConstructor.call(this);
}

extendClass(CommonController, MeaObject);
