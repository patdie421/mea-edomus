## description de l'interface ##
<p>L'interface "INTERFACE_TYPE_001" est basée sur un microcontroleur ATMEGAx28 (Arduino UNO par exemple) équipé du logiciel "interface_type_001.ino". Elle se connecte à l'ordinateur par l'intermédiaire d'une liaison série (TTL, USB, RS232, ...)</p>
<p>Cette interface très spécialisée assure 3 fonctions :</p>
  * la collecte d'informations de puissance électrique consommée (kwh) et la puissance instantannée (en watt, à l'aide du compteur d'impulsions)
  * la gestion de 3 entées logiques et 1 entrée analogique
  * la gestion de sorties logiques ou analogiques
<p>Ces trois fonctions correspondent à 3 types de capteurs (cf. déclaration des types)</p>
<p>Les capteurs/actionneurs se connectent au microcontrôleur de la façon suivante :</p>
  * 2 compteurs ERDF (via les interfaces téléinfo) sont connectés sur les pin 6, 7, 8 et 9 par l'intermédiaire d'une interface compteur ERDF (voir capteur téléinfo - à venir)
  * 2 compteurs d'impulsions associés aux deux compteurs sur les pin 1 et 2.
  * 3 entrées logiques sur les pin 4, 10 et 12
  * 1 entrée analogique (Raw, TMP36 ou "voltage") sur A3
  * 5 sorties en combinant les pin 5, 11, A0, A1 et A2 :
    * 5 sorties logiques
    * 4 sorties logiques et 1 sortie PWM (pin 5 ou 11)
    * 3 sorties logiques et 2 sorties PWM (pin 5 et 11)
<p>Le micro-programme utilise les librairies suivantes :</p>
  * **Comio2** : pour les échanges avec le PC (à partir de la version 0.1apha3, **comio** avant)
  * **pulse** : pour la gestion des impulsions
  * **InterfaceCompteurERDF** : pour des compteurs
  * **AltSoftSerial** (version modifiée) : pour la communication avec le compteur
  * **BlinkLeds** : pour la gestion des clignotements des Leds d'état
<p>Les entrées/sorties logiques et analogiques et les fonctions spécifiques sont fixées par le micro-programme (nombres, positions, traitements, ...). Les échanges avec l'ordinateur se font par les méthodes suivantes :</p>
  * L'accès aux informations des compteurs se fait via la lecture d'adresse mémoire _**Comio2**_ :
    * **Compteur 0** : 32bits big indian, aux adresses  0 (H) à  3 (L)
    * **Compteur 1** : 32bits big indian, aux adresses 10 (H) à 13 (L)
  * Un trap est transmis à chaque impulsion :
    * **Compteur 0** : trap **1**
    * **Compteur 1** : trap **2**
  * L'accès aux entrées/sorties est réalisé par des fonctions _**Comio2**_
    * 0 : impulsion sur une sortie logique
    * 1 : on/high ou off/low d'une sortie logique
    * 2 : PWM sur une sortie analogique
    * 5 : lecture TMP36
    * 6 : lecture entrées logiques
    * 7 : lecture entrées analogiques
En paramètre des fonctions, le "pin" est sur le premier octet, la valeur à fixer éventuelle sur l'octet suivant (voir doc. Comio2).
  * En complément, à chaque changment d'une entrées logiques un trap portant le numéro _numero\_pin + 10_ est transmis, le complément de trap contient l'état de l'entrée à ce moment.

## Paramétrage ##
<p>Le paramétrage est fait du côté de l'ordinateur. Le comportement du micro-programme de l'interface n'est pas personnalisable directement.</p>
### Déclaration des types ###
<p>Les types nécessaires au fonctionnement de l'INTERFACE_TYPE_001 sont créés lors de l'initialisation de la base de paramètrage (option --init ou --autoinit de mea-edomus).</p>
<p>Les types nécessaire au fonctionnement sont déclarés dans la table "TYPES" de la base de paramétrage de MEA-EDOMUS.</p>
#### Type : INTYP01 ####
<p>c'est le type de l'interface :</p>
  * id\_type : 100
  * name : "INTYP01"
  * description : "INTERFACE\_TYPE\_001"
  * parameters : aucun (pas utilisé par la version courante de MEA-EDOMUS).
#### Type : PWRCTR (compteur pour INTERFACE\_TYPE\_001) ####
<p>Déclaration du type capteur "compteur ERDF"</p>
  * id\_type : 1000
  * name : "PWRCTR"
  * description : "Compteurs ERDF (ICE)"
  * parameters : aucun.
#### Type : TYP01IN (entrées pour INTERFACE\_TYPE\_001)" ####
<p>Déclaration du type capteur entrées (logiques ou analogiques)</p>
  * id\_type : 1001
  * name : "TYP01IN"
  * description : "Entrees pour Interface\_type\_001"
  * parameters : aucun (pas utilisé par la version courante de MEA-EDOMUS).
#### Type : TYP01OUT (sortie pour INTERFACE\_TYPE\_001)" ####
<p>Déclaration du type actionneur sorties (logiques ou analogique)</p>
  * id\_type : 1002
  * name : "TYP01OUT"
  * description : "Sorties pour Interface\_type\_001"
  * parameters : aucun (pas utilisé par la version courante de MEA-EDOMUS).
### Déclaration de l'interface ###
<p>Une interface de type INTERFACE_TYPE_001 doit être déclarée dans la table INTERFACES</p>
<p>Plusieurs interface de ce type peuvent fonctionner en même temps.</p>
<p>Pour la création de l'interface, les éléments suivants sont nécessaires :</p>

  * id\_interface : numéro d'identification de l'interface (libre)
  * id\_type : 100 - type de l'interface.
  * name : libre - nom de l'interface (8 caractères max, [a-zA-Z0-9])
  * description : une brêve description.
  * dev : le type de connexion, le nom du device unix dans /dev et éventuellement la vitesse (débit) de transmission. Exemple SERIAL://ttyS1:9600 correspond à une connexion serie sur /dev/ttyS1 avec des échanges à 9600bits/s (9600 est la valeur par defaut pour une interface série). Pour le moment seul les connexion de type SERIAL:// sont supportées.
  * parameters : aucun (pas utilisé pour le moment)
  * state : etat de l'interface - 0 = desactivé, 1 = activé, 2 = délégué (activé par une autre interface. Equivaut à 0 pour ce type d'interface).
```
INSERT INTO "interfaces" VALUES(NULL,1,100,'ARDUINO01','Compteurs EDF-ERDF','SERIAL://tty.usbmodem1421','',1);
```

### Déclaration des capteurs et actionneurs ###
<p>Pour la création d'un capteur ou d'un actionneur, les éléments suivants sont nécessaires :</p>

  * id\_sensor\_actuator : numéro d'identification du capteur/actionneur(libre mais doit être unique)
  * id\_type : 1000 (Compteur), 1001 (entrée) ou 1002 (sortie) - type du capteur.
  * name : libre - nom de du capteur/actionneur (16 caractères max [a-zA-Z0-9])
  * description : une brêve description.
  * parameters : les parametres du comportement du capteur/actionneur (cf paramétrage).
  * id\_location : identifiant du lieu d'installation du capteur (voir talbe "location")
  * state : etat du capteur/actionneur - 0 = desactivé, 1 = activé.

### Paramétrage capteur/actionneur ###
<p>Le paramétrage du comportement du capteur/actionneur est réalisé par combinaison de deux valeurs :</p>
  * au niveau du type du capteur/actioneur (champ TYPE de la base de paramétrage)
  * au niveau des paramètres (champ PARAMETERS) d'un capteur ou d'un actionneur.

<p>Les 3 types de capteurs/actionneurs (<i>id_type</i>) possibles sont :</p>
  * 1000 : capteur de type compteur ERDF
  * 1001 : sortie logique ou analogique
  * 1002 : entrée logique ou analogique

<p>Les paramètres "de spécialisation" sont précisés dans le champ "PARAMETERS" et les différentes possibilités sont listés ci-dessous. Notez que pour les capteurs de ce type d'interface, tous les paramètres listés sont obligatoires (pas de valeurs par défaut)</p>
> #### Sorties Logiques : (5 sorties logiques disponibles=) ####
<p>
</li></ul><ul><li>PIN : "D5", "D11", "AI0", "AI1", "AI2"<br>
</li><li>TYPE : "digital"<br>
</li></ul><blockquote>_Exemples_ :
  * "PIN=AI0;TYPE=DIGITAL" : Low/high ou pulse sur AI0
  * "PIN=D11;TYPE=DIGITAL" : Low/high ou pulse sur D11
</p></blockquote>

<blockquote><h4>Sorties analogiques (PWM) : (2 sorties disponibles) ####
<p>
</blockquote>  * PIN : "D5", "D11"
  * TYPE : "analog"
> _Exemple :_
  * "PIN=D5;TYPE=ANALOG"
</p></li></ul>

<blockquote><h4>Entrées logiques :</h4>
<p>
</blockquote><ul><li>PIN : "D4", "D10", "D12"
  * TYPE : "digital"
> _Exemple_ :
  * "PIN=D4;TYPE=DIGITAL"
</p></li></ul>

<blockquote><h4>Entrée analogique : (une seule entrée analogique disponible)</h4>
<p>
</blockquote><ul><li>PIN : "AI3" : entée analogique numéro 3
  * TYPE : "analog" : type analogique
  * COMPUTE :
    * "voltage" : affichage de la tension mesurée
    * "temp" : affichage d'une temperature
  * ALGO (chaine de caractères)
    * "raw"    valeur "brute" du convertisseur A/N
    * "tmp36"  capteur TMP36
    * "aref5"  référence VCC
    * "aref11" référence 1,1V interne
> _Exemple_ :
  * "PIN=AI3;TYPE=ANALOG;COMPUTE=TEMP;ALGO=TMP36" : lecture de la température d'un TMP36 connecté sur A3
  * "PIN=AI3;TYPE=ANALOG;COMPUTE=VOLTAGE;ALGO=AREF5" : lecture d'une tension sur A3
</p></li></ul>

<blockquote><h4>Entrées compteurs</h4>
<p>
</blockquote><ul><li>COUNTER :
    * 0 : compteur 0
    * 1 : compteur 1
> _Exemple_ :
  * "COUNTER=0"
</p></li></ul>

<blockquote><h4>A noter :</h4>
<p>Le champ localisation prévu dans la déclaration d'un capteur ou d'un actionneur n'est pas encore utilisée (prévu dans une prochaine version).</p></blockquote>

<blockquote><h4>Exemple de déclaration :</h4></blockquote>

<pre><code>INSERT INTO "sensors_actuators" VALUES(1,1,1002,1,'R1','Commande telerupteur SALLE1',20,'PIN=AI0;TYPE=DIGITAL,1);<br>
INSERT INTO "sensors_actuators" VALUES(2,2,1001,1,'TEMP01','Temperature du capteur TMP36',10,'PIN=AI3;TYPE=ANALOG;COMPUTE=TEMP;ALGO=TMP36',1);<br>
</code></pre>
<h2>Commandes xPL disponibles</h2>
<p>Les capteurs/actionneurs de l'interface INTERFACE_TYPE_001 répondent au commandes (message xPL <i>xpl-cmnd</i>) xpl suivantes :</p>
<h3>schema <b>sensor.request</b></h3>
<p>Demande d'émission de l'état courrant d'un(de) capteur(s)/actionneur(s). Les éléments de la demande (body) doivent être :</p>
<ul><li>request=current<br>
</li><li>[device=<i>device</i>]<br>
</li><li>[type=<i>type</i>]<br>
Avec pour ce type d'interface :<br>
<p>_device_ = nom du capteur/actionneur
<p><i>type</i> = "power", "energy", "voltage", "temp", "raw", "output" ou "input"<br>
<p><i>device</i> et <i>type</i> sont optionnels et utilisables indipendement. Ainsi vous pouvez simplement faire une demande pour obtenir une émission de la valeur de tous les capteurs/actionneurs de l'interface ou préciser un type pour avoir l'état de capteurs/actionneurs disposant des valeurs de ce type.</p>
<p>Si la demande peut être traité, mea-edomus transmet un message <i>xpl-stat</i> avec un schéma <i>sensor.basic</i> qui contient <i>device</i>, <i>type</i> et <i>current</i>. <i>current</i> contient alors la dernière valeur "courante" mesurée.</p>
<h3>schema <b>control.basic</b></h3>
<p>Les messages <i>control.basic</i> permettent de commander les sorties de l'INTERFACE_TYPE_001. Les éléments de la demande (body) doivent être :</p>
</li><li>device=<i>device</i>
</li><li>type=(<i>output</i> | <i>variable</i>)<br>
</li><li>current=(<i>high</i> | <i>low</i> | <i>pulse</i>) pour type=<i>output</i> ou ( <i>inc</i> | <i>dec</i> | 0-255) pout type=<i>variable</i>
</li><li>[data1=1-250]<br>
<p>Lorsque <i>current=pulse</i>, <i>data1</i> doit contenir la durée en centième de seconde de l'impulsion. La valeur doit être entière et comprise entre 1 et 250. Lorsque <i>data1</i> n'est pas précisé ou pour toutes les valeurs hors plage la durée d'impulsion sera de 15 centièmes de seconde.</p>
<p>Toutes autres les associations clé/valeur se trouvant dans body sont ignorées.</p>
<h2>messages xpl émis</h2>
<h3>schema <b>sensor.basic</b></h3>
<p>Message de type <i>xpl-trig</i> envoyé à chaque changement d'état d'une entrée ou d'une sortie.<br>
Un message de type <i>xpl-stat</i> est envoyé en réponse à une commande "<b>sensor.request</b>" valide.</p>
<p>Le message est toujours adressé à target='<code>*</code>'</p>