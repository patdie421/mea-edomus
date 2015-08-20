## Description de l'interface ##
<p>L'INTERFACE_TYPE_002 est soit le point d'entrée sur un réseau MESH (réseau à base d'xbee v2), soit une interface capteurs/actionneurs de type xbee. En point d'entrée elle permet à MEA-EDOMUS de piloté des Xbee (autonomes ou accompagnés de microcontrôleurs) : les interfaces capteurs/actionneurs xbee ("sous-interfaces INTERFACE_TYPE_002"). Les sous-interfaces sont déclarées avec le même id_type=200 que l'interface "point d'entrée MESH" mais deux éléments permettent de les différencier :</p>
  * le statut d'une interface "point d'entrée" active est égal à 1 alors qu'il est égal à 2 pour la sous-interface.
  * Le chemin de connexion est "SERIAL://xxx" pour le point d'entrée et "MESH://@addr-xbee" pour une sous interface.

## Paramétrage de l'interface ##
### Déclaration du point d'entrée d'un réseau Mesh ###
Il est paramétré avec les éléments suivants (dans la base de paramétrage) :
  * identifiant : numerique (ex : 2)
  * type interface : 200
  * nom interface : une chaine de caractères de 20 caractères au plus (ex : MESHNETWORK01)
  * une description : chaine de caractères (ex "Réseau MESH principal")
  * le chemin de connexion : le chemin vers le "device" unix (ex : SERIAL://ttyS0 pour /dev/ttyS0)
  * des paramètres spécifiques à l'interface (ex : SPEED=9600, voir liste des parametres possibles)
  * un statut : 0 = desactivée, 1 = activée

(voir structure de la table de paramètre des interfaces)

La création dans la base sqlite3 peut se faire avec la requête SQL suivante :

```sql

INSERT INTO "interfaces" VALUES(NULL,2,200,'MESHNETWORK01','Reseau de capteurs/actionneurs XBEE','SERIAL://tty.usbserial-A4008TBe','',1);
```

### Déclaration d'une interface de capteurs/actionneurs ###

<p>Une interface capteurs/actionneurs se déclare de la même façon dans la table "interfaces". La différence est faite sur le chemin de connexion qui sera de la forme : MESH://xxxxxxxx-xxxxxxxx, avec xxxxxxxx-xxxxxxxx représentant l'adresse 64bits de l'xbee (ex : MESH://0013a200-4079a4c0).<br>
</p>
<p>Pour une interface de ce type, le statut doit être égal à 2 (ie sans activation) ou 0 si elle n'est pas active.</p>
<p>Par ailleurs les deux paramètres suivants peuvent être utilisés :<br>
<ul><li>PLUGIN : le plugin à appeler en cas de demande de commissionnement<br>
</li><li>PARAMETERS : les parametres du plugin.<br>
</p>
<h3>déclaration de capteurs/actionneurs</h3>
<p>Les capteurs et actionneurs sont déclarés dans la table capteurs/actionneurs et associés à une sous interface xbee (à l'aide du champs <i>id_interface</i>).</p>
<p>Pour fonctionner ce type de capteur doit être obligatoirement associé à un plugin accompagné des bons paramètres. L'écriture des plugins est libre (voir écrire un plugin INTERFACE_TYPE_002 en python). Néanmoins, un plugin generic (generic.py) permet de faire presque toutes les opérations de paramétrage d'un xbee (sans micro-contrôleur) par simple paramétrage.</p></li></ul>

<p>Si le plugin générique de contrôle des xbee est utilisé (generic_xbee.py) les paramètres à utiliser sont les suivants :</p>

<p>Format : <code>PARAMETERS=assoc1,assoc2, ...,assoc</code><i>n</i></p>
<p><code>assoc</code><i>n</i> prend des valeurs aux formats suivants :<br>
<ul><li><code>@at_cmd</code>
</li><li><code>@at_cmd:at_value</code>
</li><li><code>@at_cmd:$internal_variable</code>
</li><li><code>#internal_cmd</code>
</li><li><code>#internal_cmd:value</code>
</li><li><code>#internal_cmd:$internal_variable</code>
</p>
<p> Avec pour <code>@at_cmd</code> le <code>@</code> suivi des deux caractères d'une commande AT xbee (ex : <code>@SM</code>) et <code>at_value</code> une valeur numérique en base 10 ou hexa (format C commençant par "0x", <code>@ST:0xAF</code> par exemple) ou une chaine de caratères (pour <code>@NI</code> par exemple : <code>@NI:XBEE01</code>).</p>
<p> Avec pour <code>#internal_cmd</code> :<br>
</li><li><code>#d</code><i>n</i> : parametrage de l'entree <code>D</code><i>n</i> (<i>n</i> de 0 a 7). <code>value</code> prend alors l'une des valeurs suivantes :<br>
<ul><li><code>none</code> : aucune fonction<br>
</li><li><code>digital_out_l</code> : en sortie avec une valeur initiale a 0<br>
</li><li><code>digital_out_h</code> : en sortie avec une valeur initiale a 1<br>
</li><li><code>digital_in</code>    : en entree<br>
</li><li><code>analog_in</code>     : en entree analogique (pour x de 0 a 3)<br>
</li><li><code>default</code>       : fonctionnement par defaut de l'xbee<br>
</li><li><code>none</code>          : désactivation de la patte<br>
</li></ul></li><li><code>#set_dhdl</code> : possitionne l'adresse de destination des donnees sur l'xbee. L'adresse est au format <code>xxxxxxxx-xxxxxxxx</code> (avec x en hexadécimal) ou une variable interne ($local_xbee)<br>
</li><li><code>#set_sleep</code> : duree d'endormissement de l'xbee<br>
</li><li><code>#set_awake</code> : duree du reveil<br>
</li><li><code>#set_name</code> : possitionne le nom de peripherique sur l'xbee<br>
</li><li><code>#set_sample</code> : duree entre deux collectes d'état des entrées de l'xbee. A mettre en fin de liste de paramètres après qu'une entrée analogique est été déclarée<br>
</p>
<p>Et avec comme variables internes disponibles ($internal_variable) :<br>
</li><li><code>$my_name</code> : nom de l'interface<br>
</li><li><code>$local_xbee</code> : adresse de l'xbee hote de mea-edomus<br>
</p>
<p>
exemple : (voir dans base sqlite)<br>
<code>PLUGIN=generic_xbee;PARAMETERS=D1:digital_out_l,D0:default,D2:analog_in,#set_sample:2000,@wc</code>
</li><li><code>d1</code> en sortie avec une valeur initiale à 0<br>
</li><li><code>d0</code> à sa valeur par défaut (déclenchement hardware du commissionnement)<br>
</li><li><code>d2</code> en entrée analogique (sera <code>a2</code> pour la lecture des informations en xpl). Les valeurs des entrées seront transmises toutes le 2 secondes (2000 ms)<br>
</li><li><code>@wc</code> écriture des paramètres en mémoire non volatile<br>
</p>
<p>
<b>Attention</b> : les commandes AT (commandes et valeurs) ne sont pas contolees (existance, type et plage des valeurs). Elles sont transmisses directement de plus sans attendre de reponse. Plus globalement, pour des raisons de performances, les ordres envoyes aux xbee n'attendent pas de reponse.<br>
</p>
<p>La création dans la base sqlite3 peut se faire avec la requête SQL suivante (avec les données de l'exemple):</p>
<pre><code><br>
INSERT INTO "interfaces" VALUES(3,3,201,'XBEE01','Interface actionneur VMC','MESH://0013a200-4079a4c0','PLUGIN=generic;PARAMETERS=#d1:digital_out_h,#d2:analog_in,#set_name:$my_name,#set_sample:5000',2);<br>
</code></pre>