# Installation de Mea-edomus sur Rasperry Pi/Raspbian #
Cette procédure a été testée sur `2013-05-25-wheezy-raspbian`. Elle est sûrement adaptable pour d'autres versions de Raspbian et pour d'autres Linux.

## Installation des prerequis ##
<p>Pour mettre en oeuvre <i>mea-edomus</i> il vous faut au préalable avoir installé un système d'exploitation Linux sur un Raspberry Pi ou un autres ordinateurs. Le système doit être fonctionnel et l'ordinateur connecté à un réseau (Ethernet, Wifi ...), disposé d'une adresse IP et être atteignable par <code>ssh</code> (ou autre système de connexion à distance).</p>
<p>Pour faire l'installation, les outils et bibliothèques suivants doivent avoir été installés :</p>
  * La bibliothèque de developpement Python C
```

$ sudo apt-get install python-dev
```
  * les outils GIT (installé par défaut sur `2013-05-25-wheezy-raspbian`)
```

$ sudo apt-get install git-core
```
  * sqlite3 et la bibliothèque de développement associées
```

$ sudo apt-get install sqlite3
$ sudo apt-get install libsqlite3-dev
```
  * la bibliothèque de développement du client mysql
```

$ sudo apt-get install libmysqlclient-dev
```

## Recuperation des sources ##
<p><i>mea-edomus</i> n'est pour l'instant disponible qu'en version source. Les sources sont disponibles sur le <code>git https://code.google.com/p/mea-edomus</code>. Pour récupérer ces sources procédez de la façon suivante :</p>

  * Positionnez vous dans le répertoire qui contiendra les sources ou créez en un nouveau :
```

$ SOURCES=/sources; export SOURCES
$ mkdir $SOURCES
$ cd $SOURCES
```
  * Depuis le répertoire :
```

$ git clone -b 0.1alpha3 https://code.google.com/p/mea-edomus/
$ cd mea-edomus
```
## Installation de xPLLib ##
<p>Pour linux il faut installer xPLLib. xPLLib existe sous forme de package debian sur Linux. Neanmoins, quelques modifications ont été faites dans les sources de xPLLib pour faciliter la compilation de mea-edomus et corriger une fuite mémoire, il est donc préférable de partir des sources récupérer dans le GIT <i>mea-edomus</i> et appliquer le patch fourni. Si vous préférer utiliser les packages, il faut à minima récupérer <code>xPL.h</code> dans <code>xPLLib.tar.zip</code> et le copier en lieu et place de celui fourni par les packages.</p>
  * Allez dans le répertoire `linux` :
```
$ cd $SOURCES/lib
```
  * Décompresser l'archive `xPLLib.tar.zip` :
```
$ unzip xPLLib.tar.zip 
Archive:  xPLLib.tar.zip
  inflating: xPLLib.tar              
```
  * Extrayez l'arboressance :
```
$ tar xvf xPLLib.tar 
xPLLib/
xPLLib/xPL_priv.h
xPLLib/xPL-config.c
xPLLib/xPL-hub.c
xPLLib/xPL-listeners.c
xPLLib/examples/
xPLLib/examples/xPL_Hub.c
xPLLib/examples/xPLSend.c
xPLLib/examples/xPLHub_INSTALL.txt
xPLLib/examples/xPL_Logger.c
xPLLib/examples/xPL_Clock.c
xPLLib/examples/xPL_ConfigClock.c
xPLLib/examples/Makefile
xPLLib/CONFIG.txt
xPLLib/xPL-io.c
xPLLib/xPL-utils.c
xPLLib/xPL-service.c
xPLLib/CHANGELOG.txt
xPLLib/xPL-message.c
xPLLib/xPL-store.c
xPLLib/xPL.h
xPLLib/TODO.txt
xPLLib/INSTALL.txt
xPLLib/Makefile
```
  * Patch de xPLLib (à partir de la version 0.1alpha5 uniquement)
```
$ patch -p0 < xPLLib.patch
```
  * Allez dans le répertoire `xPLLib` et lancez la compilation de la bibliothèque :
```
$ cd xPLLib
$ make 
cc -O2 -DLINUX -pedantic -Wall -g -fPIC -c xPL-io.c
cc -O2 -DLINUX -pedantic -Wall -g -fPIC -c xPL-utils.c
cc -O2 -DLINUX -pedantic -Wall -g -fPIC -c xPL-service.c
cc -O2 -DLINUX -pedantic -Wall -g -fPIC -c xPL-message.c
cc -O2 -DLINUX -pedantic -Wall -g -fPIC -c xPL-listeners.c
cc -O2 -DLINUX -pedantic -Wall -g -fPIC -c xPL-store.c
cc -O2 -DLINUX -pedantic -Wall -g -fPIC -c xPL-config.c
cc -O2 -DLINUX -pedantic -Wall -g -fPIC -c xPL-hub.c
Creating libxPL.so...
Creating xPL.a...
ar: creating xPL.a
```
  * Installez la bibliothèque et `xPL.h`
```
$ sudo make install
cp -f libxPL.so /usr/local/lib
cp -f xPL.a /usr/local/lib
cp -f xPL.h /usr/local/include
ldconfig
```
  * Compilez les outils (surtout xPL-Hub)
```
$ cd examples
$ make
cc -g -DLINUX -pedantic -Wall -c xPL_Hub.c
cc -O -o xPL_Hub xPL_Hub.c ../xPL.a -g -lm 
cc -g -DLINUX -pedantic -Wall -c xPL_Logger.c
cc -O -o xPL_Logger xPL_Logger.c ../xPL.a -g -lm 
cc -g -DLINUX -pedantic -Wall -c xPL_Clock.c
cc -O -o xPL_Clock xPL_Clock.c ../xPL.a -g -lm 
cc -g -DLINUX -pedantic -Wall -c xPL_ConfigClock.c
cc -O -o xPL_ConfigClock xPL_ConfigClock.c ../xPL.a -g -lm 
cc -g -DLINUX -pedantic -Wall -c xPLSend.c
cc -O -o xPLSend xPLSend.c ../xPL.a -g -lm 
cc -O -static -o xPL_Hub_static xPL_Hub.o ../xPL.a -g -lm 
../xPL.a(xPL-io.o): In function 'setupBroadcastAddr':
/data/mea-edomus-0.01/linux/xPLLib/xPL-io.c:500: warning: Using 'getprotobyname' in statically linked applications requires at runtime the shared libraries from the glibc version used for linking
```
  * Installez les outils :
```
$ sudo cp xPL_Hub xPLSend xPL_Logger /usr/local/bin
```
## Installation de l'interpreteur CGI PHP ##
<p>Il y a 2 possibilités : utiliser la distribution standard de PHP de l'OS ou compiler <code>un php-cgi</code> minimum.</p>
<p>La première solution est la plus simple (car <code>php-cgi</code> est souvant déjà installé) mais est moins efficace en terme de performance car le module contient trop de choses inutiles pour <i>mea-edomus</i> et pèse plus de 20 Mo. Avec l'autre solution, la taille est réduite à envion 3 Mo.</p>
<p>La procédure d'installation via le "package" choisira, en fonction de ce que vous avez compiler/installer, la configuration à mettre en place en privilégiant le <code>php-cgi</code> "light".</p>
  * installation du package standard
```
$ sudo apt-get install php5-cgi
```
  * recompilation
<p>Cette compilation prendra un temps certain ... environ 2 heures.</p>
```
$ cd $SOURCES
$ make -f Makefile.linux build-phpcgi
```
## Installation de node.js (à partir de la version 0.1alpha5) ##
<p>
<code>nodejs</code> est nécessaire pour le fonctionnement "en live" de l'ihm.<br>
Pour compiler <code>nodejs</code> il faut que le Rasberry dispose d'au moins 1 Go de RAM utilisable. Pour cela, soit vous utilisez un Raspberry pi B 2 (ou comme moi un BananaPi), soit vous mettez à disposition du "swap" (1Go recommandé) sur un Raspberry B rev2/B+ avec 512 Mo de Ram (sur un Raspberry B avec 256 Mo la compilation est probablement impossible). Ce swap doit être sur un support externe connecté au port USB (hdd ou cle USB rapide). Il ne sera pas possible d'utiliser du swap sur la carte SD du système (les accès sont beaucoup trop lents). Une fois l'environnement Raspberry prêt, il suffit de passer les commandes suivantes :<br>
<pre><code>$ cd $SOURCES<br>
$ make -f Makefile.linux build-nodejs<br>
</code></pre>
</p>
## Installation de xplhub ##
<p>
A décrire pour la version 0.1alpha5.<br>
</p>
## xplhub alternatif ##
<p>
Fiabilité de xplhub pas encore établi, deux autres hub xpl sont disponible pour mea-edomus :<br>
<ul><li>xPL-Hub (xPLLib) pour linux (fonctionne mal sur Mac OS X)<br>
</li><li>mea-xPLHub (hub xpl en python) pour Mac OS X<br>
</p>
<h2>Compilation des outils mea</h2>
<p>
A décrire pour la version 0.1alpha5.<br>
</p>
<h2>Compilation de mea-edomus</h2>
</li><li>Dans le répertoire des sources, lancez la compilation à partir du makefile <code>Makefile.linux</code>
<pre><code>&lt;&lt;&lt; refaire la copie de la sortie standard &gt;&gt;&gt;<br>
$ cd $SOURCES/mea-edomus<br>
$ make -f Makefile.linux <br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/arduino_pins.o src/arduino_pins.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/comio.o src/comio.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/debug.o src/debug.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/error.o src/error.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/httpServer.o src/httpServer.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/interface_type_001_actuators.o src/interface_type_001_actuators.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/interface_type_001.o src/interface_type_001.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/interface_type_001_counters.o src/interface_type_001_counters.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/interface_type_001_sensors.o src/interface_type_001_sensors.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/interface_type_002.o src/interface_type_002.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/main.o src/main.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/mea_api.o src/mea_api.c<br>
src/mea_api.c: In function ‘mea_sendAtCmdAndWaitResp’:<br>
src/mea_api.c:465:4: warning: passing argument 3 of ‘PyObject_AsWriteBuffer’ from incompatible pointer type [enabled by default]<br>
/usr/include/python2.7/abstract.h:517:22: note: expected ‘Py_ssize_t *’ but argument is of type ‘long int *’<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/memory.o src/memory.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/mongoose.o src/mongoose.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/parameters_mgr.o src/parameters_mgr.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/python_api_utils.o src/python_api_utils.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/pythonPluginServer.o src/pythonPluginServer.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/queue.o src/queue.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/token_strings.o src/token_strings.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/tomysqldb.o src/tomysqldb.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/xbee.o src/xbee.c<br>
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/xPLServer.o src/xPLServer.c<br>
gcc -lmysqlclient -lpthread -lm -ldl -lxPL -lsqlite3 -lpython2.7 src/arduino_pins.o src/comio.o src/debug.o src/error.o src/httpServer.o src/interface_type_001_actuators.o src/interface_type_001.o src/interface_type_001_counters.o src/interface_type_001_sensors.o src/interface_type_002.o src/main.o src/mea_api.o src/memory.o src/mongoose.o src/parameters_mgr.o src/python_api_utils.o src/pythonPluginServer.o src/queue.o src/token_strings.o src/tomysqldb.o src/xbee.o src/xPLServer.o -o mea-edomus<br>
</code></pre>
<h2>Fabrication du package d'installation</h2>
Pour pouvoir installer facilement l'ensemble, il est possible de faire un fichier tar contenant tous les éléments nécessaires et un script d'installation.<br>
<pre><code>$ cd $SOURCES<br>
$ make -f Makefile.linux build-package<br>
</code></pre>
<h2>Mise en place</h2>
<p>Dans cette version 0.1alpha3 pour une installation initiale, il suffit d'utiliser le script <code>install.sh</code> disponible dans le package d'installation.</p>
<p>Choisissez d'abord le répertoire de "base" de l'installation (<code>/usr/local/mea-edomus</code> par exemple, mais d'autres bases sont possibles comme <code>/opt/mea-edomus</code>, <code>/apps/domotique</code>, ...). Les seuls répertoires interdit par le script d'installation sont <code>/</code>, <code>/usr</code> et <code>/etc</code>. Essayer cependant de ne pas faire n'importe quoi (ex : répertoire de base sur <code>/var</code> ...) pour ne pas briquer votre OS. Le mot de passe "administrateur linux" va être demandé lors de l'installation (sur Raspberry, faites de préférence l'installation avec le user <code>pi</code> qui dispose des habilitations nécessaires).<br>
<pre><code>$ cp $SOURCE/package/mea-edomus.tar.pkg.bz2 $HOME<br>
$ cd $HOME<br>
$ mkdir extract<br>
$ cd extract<br>
$ tar xvjf ../mea-edomus.tar.pkg.bz2<br>
$ BASE=/usr/local/mea-edomus ; export BASE<br>
$ sudo mkdir $BASE<br>
$ sudo ./install.sh $BASE<br>
</code></pre>
<p>le service <i>mea-edomus</i> est automatiquement lancé à la fin de l'installation.</p>
<h2>Initialisation de l'environnement Mysql</h2>
<à venir : pour l'instant le logging dans la base mysql est desactivé><br>
<h2>Administration</h2>
<h3>Lancement du service</h3>
<pre><code>$ sudo service mea-edomus start<br>
</code></pre>
<h3>Arrêt du service</h3>
<pre><code>$ sudo service mea-edomus stop<br>
</code></pre>
<h3>Modification de configuration</h3>
<p>La commande <code>mea-edomus</code> permet de modifier certains paramètres en ligne de commandes avec l'option <code>--update</code>. Elle s'utilise de la façon suivante pour par exemple changer le chemin de l'interpréteur <code>php-cgi</code> après l'installation :</p>
<pre><code>$ $BASE/bin/mea-edomus --basepath=$BASE --update --phpcgipath=/new/path/to/php-cgi<br>
</code></pre>
Pour les options disponibles :<br>
<pre><code>$ $BASE/bin/mea-edomus --help<br>
</code></pre>