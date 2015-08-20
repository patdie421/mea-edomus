# Test de mea-edomus (Arduino) #
Pour ce test, il faut un microcontrôleur ATMEGA328 programmable avec l'IDE Arduino et la capacité de le connecter en USB ou en série (TTL) au Raspberry PI.
Un UNO est idéal pour les premiers tests.

## Installation du microprogramme Arduino ##
Le programme à installer est : "`interface_type_001.ino`". Pour pouvoir être compilé il faut les bibliothèques :
  * `AltSoftSerial.h` (version modifiée)
  * `InterfaceCompteurERDF.h`
  * `Comio.h`
  * `Pulses.h`
Elles se trouvent dans `$SOURCE/mea-edomus/arduino/Library` (pour `$SOURCE` voir [Récupération des sources](meaedomus_installation#Recuperation_des_sources.md)).
Copier les quatres bibliothèques dans le répertoire approprié de votre IDE Arduino.

Compilez ensuite "interface\_type\_001.ino" (`$SOURCE/mea-edomus/arduino/interface_type_001/`) de façon habituelle (avec l'IDE Arduino) et transférez le résultat sur votre microcontrôleur.
## Connexion de test ##
  * Connectez votre microcontrôleur en USB ou en série au Raspberry Pi.
Profitez en pour repérer le "device" Unix occupé par le microcontrôleur (/dev/ttyUSB0 en général pour une connexion via USB mais à vous de valider avec minicom par exemple).
Mettez à jour (si nécessaire) la table `interfaces` de la base de paramétrage avec le bon device (voir [Interface Type 001 : paramétrage](InterfaceType001_parametrage.md))
  * Connectez une LED (par l'intermédiaire d'une résistance c'est plus prudent) sur l'entrée `A0` (cf. montage ci-dessous).
<img src='http://wiki.mea-edomus.googlecode.com/git/arduinoled1.png' alt='page de login' height='50%' width='50%'>
<h2>Paramétrage de mea-edomus</h2>
<h3>Connexion à l'interface d'administration de mea-edomus</h3>
<p>Commencer par vous assurer que mea-edomus est bien lancé :</p>
<pre><code>$ sudo service mea-edomus status<br>
</code></pre>
S'il n'est pas démarré :<br>
<pre><code>$ sudo service mea-edomus start<br>
</code></pre>
<p>Vous pouvez ensuite vous connecter avec un navigateur à l'url <a href='http://[machine]:8083'>http://[machine]:8083</a>, avec <code>[machine]</code> à remplacer par l'adresse IP ou un nom resolvable par le système (via dns, zeroconf, ...). Une page de "login" doit apparaitre :</p></li></ul>

<img src='http://wiki.mea-edomus.googlecode.com/git/img000.png' alt='page de login' height='100%' width='100%'>

### Authentification à l'application ###
  * Pour la première utilisation la connexion se fait en "admin" avec le mot de passe "admin".

<img src='http://wiki.mea-edomus.googlecode.com/git/img005.png' alt='page de login' height='100%' width='100%'>

<ul><li>Lors de la première connexion à va falloir changer de mot de passe :</li></ul>

<img src='http://wiki.mea-edomus.googlecode.com/git/img010.png' alt='popup changement de mot de passe' height='100%' width='100%'>

<ul><li>Saisiez 2x votre nouveau mot de passe et validez.</li></ul>

<img src='http://wiki.mea-edomus.googlecode.com/git/img015.png' alt='page de changement de mod de passe' height='100%' width='100%'>

<ul><li>Si le changement de mot de passe a été correctement saisie ...</li></ul>

<img src='http://wiki.mea-edomus.googlecode.com/git/img020.png' alt='mot de passe a ete change' height='100%' width='100%'>

<ul><li>On obtient ensuite à la page d'acceuil :</li></ul>

<img src='http://wiki.mea-edomus.googlecode.com/git/img025.png' alt="page d'accueil" height='100%' width='100%'>

<h3>Déclaration d'une interface</h3>
<ul><li>Allez dans le menu "Administration" et le sous-menu "Périphériques".</li></ul>

<img src='http://wiki.mea-edomus.googlecode.com/git/img030.png' alt='menu administration/peripherique déroullé' height='100%' width='100%'>

<ul><li>La page suivante apparait :</li></ul>

<img src='http://wiki.mea-edomus.googlecode.com/git/img035.png' alt='Page administration des peripheriques' height='100%' width='100%'>

<ul><li>selectionner l'onglet "Interfaces"</li></ul>

<img src='http://wiki.mea-edomus.googlecode.com/git/img040.png' alt='Page administration des peripheriques' height='100%' width='100%'>

<ul><li>Cliquez sur "Ajouter" et remplissez le formulaire qui apparait de la façon suivante :</li></ul>

<img src='http://wiki.mea-edomus.googlecode.com/git/img050.png' alt='Page formulaire rempli' height='100%' width='100%'>

Pour "Interface" mettre le port Serie/USB de l'Arduino à la place de ttyAMA0 (ex : SERIAL://ttyUSBS0 dans la plupart des cas pour une connexion USB. ttyAMA0 est pour une connexion direct en serie TTL).<br>
<br>
<ul><li>Validez le formulaire</li></ul>

<h3>Déclaration d'un actionneur</h3>
<ul><li>selectionner l'onglet "Capteur(s)/Actionneur(s)"</li></ul>

<img src='http://wiki.mea-edomus.googlecode.com/git/img060.png' alt='Onglet Capteurs/actionneurs' height='100%' width='100%'>

<ul><li>Cliquez sur "Ajouter", remplisser le formulaire qui est apparu de la façon suivante :</li></ul>

<img src='http://wiki.mea-edomus.googlecode.com/git/img065.png' alt='formulaire Capteurs/actionneurs' height='100%' width='100%'>

<ul><li>Validez le formulaire, et c'est terminé ...</li></ul>

<img src='http://wiki.mea-edomus.googlecode.com/git/img070.png' alt='formulaire Capteurs/actionneurs rempli' height='100%' width='100%'>


<h3>Création directement dans la base</h3>
<ul><li>Création de l'interface : ajoutez l'entrée suivante dans la base de parametrage<br>
<pre><code>INSERT INTO "interfaces" VALUES(NULL,1,100,'ARDUINO','Premier test','SERIAL://ttyUSB0','',1);<br>
</code></pre>
</li><li>Création de l'acrtionneur LED : ajoutez l'entrée suivante dans la base de paramétrage.<br>
<pre><code>INSERT INTO "sensors_actuators" VALUES(NULL,1,1002,1,'R1','Impulsion sur une LED pour test',20,'PIN=AI0;TYPE=DIGITAL',1);<br>
</code></pre>
<h2>Premier lancement</h2>
<h3>Lancement d'un Hub xPL</h3>
Un hub xPL doit être lancé pour que mea-edomus puisse fonctionner. Si ce n'est pas déjà le cas, il suffit pour cela de lancer la commande suivante :<br>
<pre><code>$ /usr/local/bin/xPL_Hub<br>
</code></pre>
<h3>Prise ne compte du nouveau paramétrage</h3>
<pre><code>$ sudo service mea-edomus restart<br>
</code></pre>
<h2>Test du fonctionnement</h2>
Depuis la ligne de commande du Raspberry (ou d'une autre machine disposant de xPL), essayez les commandes suivantes :<br>
</li><li>Pour allumer la LED<br>
<pre><code>$ /usr/local/bin/xPLSend -c control.basic -m cmnd device=R1 type=output current=high<br>
</code></pre>
</li><li>Pour éteindre la LED<br>
<pre><code>$ /usr/local/bin/xPLSend -c control.basic -m cmnd device=R1 type=output current=low<br>
</code></pre>
</li><li>Pour générer une impulsion sur la LED<br>
<pre><code>$ /usr/local/bin/xPLSend -c control.basic -m cmnd device=R1 type=output current=pulse data1=1000<br>
</code></pre>
La LED doit s'allumer pendant 1 seconde puis s'éteindre.