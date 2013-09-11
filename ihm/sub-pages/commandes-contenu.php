<div id='command' style="  border-color:black; border-width:1px; border-style:solid;">
</div>
<script>
$(function(){
    function fx1(event){
        if(event.data.me.last==null)
            event.data.me.last=0;
        now=new Date().getTime();
        device=event.data.me.onClickData.device;
    
        if(now - event.data.me.last > 1000){
            $.get('lib/php/ws1.php',
                  { device: device }, // appel de Webservice
                  function(data){
                      event.data.me.onClickData.section.info(data.device+":"+data.retour);
                  },
                  "json"
            );
        }else{
            event.data.me.onClickData.section.alert();
        }
        event.data.me.last=now;
    };

    var sections = new Sections('#command','S');

    section = new Section('S1','sdb');
    bouton = new BoutonPush('B1');
    bouton.setOnClick(fx1,{device: "RELAY1", section: section});
    section.addBouton(bouton);
    sections.addSection(section);

    section = new Section('S2','couloir');
    bouton = new BoutonPush('B2');
    bouton.setOnClick(fx1,{device: "RELAY2", section: section});
    section.addBouton(bouton);
    sections.addSection(section);

        //sections.removeSection('S1');
    sections.render();
});
</script>
