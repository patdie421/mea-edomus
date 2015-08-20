# Description de l'interface #
L’interface « INTERFACE\_TYPE\_001 » assure 3 fonctions :
  * la collecte d'informations de puissances électriques consommées/produites en kwh (via démodulateur téléinfo) et les puissances instantanées en watt (via capteur d’impulsion / clignotement de LED du compteur) pour 2 compteurs ERDF
  * la gestion de 3 entées logiques et 1 entrée analogique
  * la gestion de sorties logiques ou analogiques
Cette interface est basée sur un microcontrôleur ATMEGAx28 (Arduino UNO par exemple) équipé du logiciel « interface\_type\_001.ino ». Elle se connecte à l'ordinateur par l'intermédiaire d'une liaison série (niveau TTL, RS232, USB, ...).
Les trois fonctions assurées correspondent à 3 types de capteurs/actionneurs. Ces capteurs/actionneurs se branchent au microcontrôleur de la façon suivante :
  * 2 compteurs ERDF sont connectés par l’intermédiaire des points de connexion (pins) 6, 7, 8 et 9 par l'intermédiaire d'une « interface compteur ERDF » (démodulateur téléinfo).
  * 2 compteurs d'impulsions associés aux deux compteurs sur les points de connexion 2 et 3.
  * 3 entrées logiques sur les points de connexion 4, 10 et 12.
  * 1 entrée analogique (TMP36 ou "voltage") sur pin A3.
  * 5 sorties en combinant les points de connexion 5, 11, A0, A1 et A2 :
    * 5 sorties logiques
    * 4 sorties logiques et 1 sortie PWM (pin 5 ou 11)
    * 3 sorties logiques et 2 sorties PWM (pin 5 et 11)
Le micro programme utilise les librairies suivantes :
  * **comio** : pour les échanges avec le PC.
  * **pulse** : pour la gestion des impulsions en sortie.
  * **InterfaceCompteurERDF** : pour des compteurs.
  * **AltSoftSerial** (version modifiée) : pour la communication avec l’ « interface compteur ERDF ».
Les échanges avec l'ordinateur se font de la façon suivante :
  * L'accès aux informations des compteurs se fait via la lecture d'adresses mémoires _comio_ :
    * **Compteur 0** : 32bits big Indian, aux adresses 0 (H) à 3 (L)
    * **Compteur 1** : 32bits big Indian, aux adresses 10 (H) à 13 (L)
  * Un trap (court) _**comio**_ est transmis à chaque impulsion :
    * **Compteur 0** : trap n° **1**
    * **Compteur 1** : trap n° **2**
  * L'accès aux entrées/sorties est réalisé par des fonctions _comio_ dont voici les numéros :
    * 0 : impulsion sur une sortie logique.
    * 1 : on ou off d'une sortie logique.
    * 2 : PWM sur une sortie analogique.
    * 5 : lecture TMP36 (traitement spécifique pour tmp36 : moyenne de 10000 lectures).
    * 6 : lecture entrées logiques.
    * 7 : lecture entrées analogiques.
En paramètre des fonctions, le "pin" est sur l'octet haut, la valeur sur l'octet bas (voir doc. comio).
  * En complément, à chaque changement d'une entrée logique un trap (long) portant le numéro « _numero\_pin + 10_ » est transmis, le _complément de trap_ contient l'état de l'entrée à ce moment.
# Paramétrage #
Le paramétrage est fait du côté de l'ordinateur. Le comportement du micro-programme de l'interface n'est pas personnalisable directement.
## Déclaration des types (de périphérique) ##
Les types nécessaires au fonctionnement de l'INTERFACE\_TYPE\_001 sont créés lors de l'initialisation de la base de paramétrage (option `--init` ou `--autoinit` de mea-edomus). Ils sont déclarés dans la table "TYPES" de la base de paramétrage de mea-edomus.
### Type : INTYP01 ###
C'est le type de l'interface :
  * id\_type : 100
  * name : "INTYP01"
  * description : "INTERFACE\_TYPE\_001"
  * parameters : aucun (pas utilisé par la version courante de MEA-EDOMUS).
### Type : PWRCTR (compteur pour INTERFACE\_TYPE\_001) ###
Déclaration du type capteur "compteur ERDF"
  * id\_type : 1000
  * name : "ARD01PWR"
  * description : "Compteurs ERDF (ICE)"
  * parameters : aucun (pas utilisé par la version courante de MEA-EDOMUS).
### Type : ARD01IN (entrées pour INTERFACE\_TYPE\_001) ###
Déclaration du type capteur entrées (logiques ou analogiques).
  * id\_type : 1001
  * name : "ARD01IN"
  * description : "Entrées pour Interface\_type\_001"
  * parameters : aucun (pas utilisé par la version courante de MEA-EDOMUS).
### Type : ARD01OUT (sortie pour INTERFACE\_TYPE\_001) ###
Déclaration du type actionneur sorties (logiques ou analogique).
  * id\_type : 1002
  * name : "ARD01OUT"
  * description : "Sorties pour Interface\_type\_001"
  * parameters : aucun (pas utilisé par la version courante de MEA-EDOMUS).
## Déclaration de l'interface ##
Une interface de type INTERFACE\_TYPE\_001 doit être déclaré dans la table INTERFACES. Plusieurs interfaces de ce type peuvent fonctionner en même temps. Pour la création de l'interface, les éléments suivants sont nécessaires :
  * id\_interface : numéro d'identification de l'interface (libre).
  * id\_type : 100 - type de l'interface.
  * name : libre - nom de l'interface (16 caractères max).
  * description : une brève description.
  * dev : le type de connexion, le nom du « device Unix » dans `/dev` et éventuellement la vitesse (débit) de transmission. Exemple `SERIAL://ttyS1:9600` correspond à une connexion série sur `/dev/ttyS1` avec des échanges à 9600 bits/s (9600 est la valeur par défaut pour une interface série). Pour le moment seul les connexions de type `SERIAL://` sont supportées.
  * parameters : aucun (pas utilisé pour le moment).
  * state : etat de l'interface - 0 = desactivé, 1 = activé, 2 = délégué (activé par une autre interface. Equivaut à 0 pour ce type d'interface).
```
INSERT INTO "interfaces" VALUES(NULL,1,100,'ARDUINO01','Compteurs EDF-ERDF','SERIAL://tty.usbmodem1421','',1);
```
## Déclaration des capteurs et actionneurs ##
Pour la création d'un capteur ou d'un actionneur, les éléments suivants sont nécessaires :
  * id\_sensor\_actuator : numéro d'identification du capteur/actionneur (libre mais doit être unique).
  * id\_type : 1000, 1001 ou 1002 - type du capteur.
  * name : libre - nom de du capteur/actionneur (16 caractères max).
  * description : une brêve description.
  * parameters : les paramètres du comportement du capteur/actionneur (cf. paramétrage).
  * id\_location : identifiant du lieu d'installation du capteur (voir table "location").
  * state : etat du capteur/actionneur - 0 = desactivé, 1 = activé.
## Paramétrage capteurs et actionneurs ##
Le paramétrage du comportement du capteur/actionneur est réalisé par combinaison de deux valeurs :
  * au niveau du type du capteur/actionneur (champ TYPE de la base de paramétrage).
  * au niveau des paramètres (champ PARAMETERS) d'un capteur ou d'un actionneur.
Les 3 types de capteurs/actionneurs (_id\_type_) possibles sont :
  * 1000 : capteur de type compteur ERDF.
  * 1001 : sortie logique ou analogique.
  * 1002 : entrée logique ou analogique.
Les paramètres "de spécialisation" sont précisés dans le champ "PARAMETERS" et les différentes possibilités sont listés ci-dessous. Notez que pour les capteurs de ce type d'interface, tous les paramètres listés sont obligatoires (pas de valeurs par défaut). Le TYPE dans les paramètres correspond à la nature de l’entrée/sortie (analogique ou numérique).
### Sorties Logiques (5 sorties logiques disponibles) : ###
  * PIN : "D5", "D11", "D13", "AI0", "AI1", "AI2"
  * TYPE : "digital"
Exemples :
  * "PIN=AI0;TYPE=DIGITAL" : Low/high ou pulse sur D13 AI0
  * "PIN=D13;TYPE=DIGITAL" : Low/high ou pulse sur D13 (led)
### Sorties analogiques (PWM / 2 sorties disponibles) : ###
  * PIN : "D5", "D11"
  * TYPE : "analog"
Exemple :
  * "PIN=D5;TYPE=ANALOG"
### Entrées logiques : ###
  * PIN : "D4", "D10", "D12"
  * TYPE : "digital"
Exemple :
  * "PIN=D4;TYPE=DIGITAL"
### Entrée analogique (une seule entrée analogique disponible) : ###
  * PIN : "AI3" : entée analogique numéro 3
  * TYPE : "analog" : type analogique
  * COMPUTE :
    * "voltage" : affichage de la tension mesurée
    * "temp" : affichage d'une température
  * ALGO (chaine de caractères)
    * "tmp36" (si type = temp) : capteur TMP36
    * "aref5" (si type = voltage) : référence VCC interne
    * "aref11" (si type = voltage) : référence 1,1V interne
Exemple :
  * "PIN=AI3;TYPE=ANALOG;COMPUTE=TEMP;ALGO=TMP36" : lecture de la température d'un TMP36 connecté sur A3
  * "PIN=AI3;TYPE=ANALOG;COMPUTE=VOLTAGE;ALGO=AREF5" : lecture d'une tension sur A3
### Entrées compteurs ###
  * COUNTER :
    * 0 : compteur 0
    * 1 : compteur 1
Exemple :
  * "COUNTER=0"
### A noter : ###
Le champ localisation prévu dans la déclaration d'un capteur ou d'un actionneur n'est pas encore utilisé (prévu pour la prochaine version ALPHA) et peut donc rester vide pour le moment.
### Exemple de déclaration : ###
```
INSERT INTO "sensors_actuators" VALUES(1,1,1002,1,'R1','Commande telerupteur SALLE1',1,'PIN=AI0;TYPE=DIGITAL;ACTION=PULSE',1);
INSERT INTO "sensors_actuators" VALUES(2,2,1000,1,'CONSO','Consommation',1,'COUNTER=0;POLLING_PERIODE=300',1);
INSERT INTO "sensors_actuators" VALUES(3,3,1001,1,'TEMP01','Temperature du capteur TMP36',1,'PIN=AI3;TYPE=ANALOG;COMPUTE=TEMP;ALGO=TMP36 POLLING_PERIODE=30',1);
INSERT INTO "sensors_actuators" VALUES(4,4,1001,1,'PUSH01', 'Bouton poussoir',1,'PIN=D10;TYPE=DIGITAL',1);
```
# Commandes xPL disponibles (cmnd) #
Les capteurs/actionneurs de l'interface INTERFACE\_TYPE\_001 répondent aux commandes xPL suivantes :
## sensor.request ##
Demande d'émission de l'état courant de capteur(s)/actionneur(s). Les éléments de la demande (body) doivent être :
  * request=current
  * [device=_device_]
  * [type=_type_]

Avec pour ce type d'interface :
_device_ = nom du capteur/actionneur (name)
_type_ = `power`, `energy`, `voltage`, `temp`, `output` ou `input`. Il s’agit ici du type de valeur (à ne pas confondre avec le type de périphérique edomus et le type d’entrée sortie) répartie de la façon suivante :

Type périphérique = 1000 (**PWRCTR**)
  * type=`energy` | `power`
  * unit=`kwh` | `w`
Type périphérique = 1001 (**ARD01OUT**)
  * Type de sortie = analog ou digital
    * type = `output`
Type périphérique = 1002 (**ARD01IN**)
  * Type d’entrée = analog
    * type = `temp` | `voltage`
    * unit = `c`|`v`
  * Type d’entrée = digital
    * type = `input`
_device_ et _type_ sont optionnels et utilisables indépendamment. Ainsi vous pouvez simplement faire une demande pour obtenir une émission de la valeur de tous les capteurs/actionneurs de l'interface ou préciser un type pour avoir l'état de capteurs/actionneurs disposant des valeurs de ce type.
Exemples :
```
$ xPLSend –m cmnd –c sensor.basic device=temp01
```
Réponse :
```
xpl-stat
{
hop=1
source=mea-edomus.home
target=*
}
sensor.basic
{
device=temp01
type=temp
current=23.1
last=23.2
unit=c
}
```

```
$ xPLSend –m cmnd –c sensor.basic
```
Réponse :
```
xpl-stat
{
hop=1
source=mea-edomus.home
target=*
}
sensor.basic
{
device=temp01
type=temp
current=23.1
last=23.2
unit=c
}

xpl-stat
{
hop=1
source=mea-edomus.home
target=*
}
sensor.basic
{
device=push01
type=input
current=high
last=low
}

xpl-stat
{
hop=1
source=mea-edomus.home
target=*
}
sensor.basic
{
device=prod
type=energy
current=2030
last=2028
unit=kwh
}
```

## control.basic ##
Contrôle l’état des sorties de l’INTERFACE\_TYPE\_001. Les éléments de body peuvent être les suivants :
### Sorties logiques ###
  * device=_device_
  * type=`output`
  * current=`high` | `low` | `pulse`
  * [data1=_durée en ms_] (uniquement si `current=pulse`, durée par défaut 250 ms)
### Sorties analogiques ###
  * device=_device_
  * type=`variable`
  * current=`v` | `+x` | `-x` | `inc` | `dec`
Avec `v` et `x` une valeur numérique de 0 à 255
Exemples :
```
$ xPLSend –m cmnd –c control.basic device=r1 current=high
```

```
$ xPLSend –m cmnd –c control.basic device=r1 current=pulse data1=125
```