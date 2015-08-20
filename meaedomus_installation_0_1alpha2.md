# Installation de Mea-edomus 0.1alpha2 sur Rasperry Pi/Raspbian #
Cette procédure a été testée sur `2013-05-25-wheezy-raspbian`. Elle est sûrement adaptable pour d'autres versions de Raspbian et pour d'autres Linux.

## Installation des prérequis ##
<p>Pour mettre en oeuvre Mea-edomus il vous faut au préalable avoir installé un système d'exploitation Linux sur un Raspberry Pi ou un autres ordinateurs. Le système doit être fonctionnel et l'ordinateur connecté à un réseau (Ethernet, Wifi ...), disposer d'une adresse IP et être atteignable par ssh (ou autre système de connexion à distance).</p>
<p>Pour faire l'installation, les outils et bibliothèques suivants doivent avoir été installés :</p>
  * La bibliothèque de développement Python C
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

## Récupération des sources ##
<p>Mea-edomus n'est pour l'instant disponible qu'en mode source. Les sources sont disponibles sur le <code>git https://code.google.com/p/mea-edomus</code>. Pour récupérer les sources de la version 0.1Alpha2 procédez de la façon suivante :</p>

  * Positionnez vous dans le répertoire qui contiendra les sources ou créez en un nouveau :
```

$ SOURCES=/sources; export SOURCES
$ mkdir $SOURCES
$ cd $SOURCES
```
  * Depuis le répertoire :
```

$ git clone -b 0.1alpha2 https://code.google.com/p/mea-edomus/
$ cd mea-edomus
```
## Installation de xPLLib ##
<p>Pour linux il faut installer xPLLib. xPLLib existe sous forme de package debian sur Linux. Neanmoins, quelques modifications ont été faites dans les sources de xPLLib pour faciliter la compilation de mea-edomus, il est donc préférable de partir des sources récupérer dans le GIT Mea-edomus. Si vous préférer utiliser les packages, il faut à  minima récupérer <code>xPL.h</code> dans <code>xPLLib.tar.zip</code> et le copier en lieu et place de celui fourni par les packages.</p>
  * Allez dans le répertoire `linux` :
```
$ cd linux
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
  * Installez la bibliothèque et xPL.h
```
$ sudo make install
sudo make install
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

## Compilation de mea-edomus ##
  * Dans le répertoire des sources, lancez la compilation à partir du makefile `Makefile.linux`
```
$ cd $SOURCES/mea-edomus
$ make -f Makefile.linux 
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/arduino_pins.o src/arduino_pins.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/comio.o src/comio.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/debug.o src/debug.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/error.o src/error.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/httpServer.o src/httpServer.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/interface_type_001_actuators.o src/interface_type_001_actuators.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/interface_type_001.o src/interface_type_001.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/interface_type_001_counters.o src/interface_type_001_counters.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/interface_type_001_sensors.o src/interface_type_001_sensors.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/interface_type_002.o src/interface_type_002.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/main.o src/main.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/mea_api.o src/mea_api.c
src/mea_api.c: In function 'mea_sendAtCmdAndWaitResp':
src/mea_api.c:465:4: warning: passing argument 3 of 'PyObject_AsWriteBuffer' from incompatible pointer type [enabled by default]
/usr/include/python2.7/abstract.h:517:22: note: expected 'Py_ssize_t *' but argument is of type 'long int '
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/memory.o src/memory.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/mongoose.o src/mongoose.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/parameters_mgr.o src/parameters_mgr.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/python_api_utils.o src/python_api_utils.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/pythonPluginServer.o src/pythonPluginServer.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/queue.o src/queue.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/token_strings.o src/token_strings.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/tomysqldb.o src/tomysqldb.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/xbee.o src/xbee.c
gcc -std=c99 -O2 -D_BSD_SOURCE -D__NO_TOMYSQL__ -I/usr/include/mysql -I/usr/include/python2.7 -g -D__DEBUG_ON__   -c -o src/xPLServer.o src/xPLServer.c
gcc -lmysqlclient -lpthread -lm -ldl -lxPL -lsqlite3 -lpython2.7 src/arduino_pins.o src/comio.o src/debug.o src/error.o src/httpServer.o src/interface_type_001_actuators.o src/interface_type_001.o src/interface_type_001_counters.o src/interface_type_001_sensors.o src/interface_type_002.o src/main.o src/mea_api.o src/memory.o src/mongoose.o src/parameters_mgr.o src/python_api_utils.o src/pythonPluginServer.o src/queue.o src/token_strings.o src/tomysqldb.o src/xbee.o src/xPLServer.o -o mea-edomus
```
## Récupération d'un "`php-cgi`" ##
<p>L'interface graphique est mise en oeuvre par des "scripts" PHP et un serveur HTTP interne à mea-edomus. Ce serveur ne dispose pas de "module PHP" comme pour un serveur Apache, les scripts PHP sont donc appelés en mode CGI. Il faut pour cela disposer d'une commande "<code>php-cgi</code>". Sans "<code>php-cgi</code>" le moteur de <code>mea-edomus</code> pourra fonctionner mais l'interface graphique sera inaccessible (et des messages d'information ou d'alerte apparaitront dans les log).</p>
<p>Il existe un package linux qui fourni cet executable (à trouver) ou il est possible de le compiler spécifiquement pour <code>mea-edomus</code>. C'est cette dernière solution que je préconise car nous allons pouvoir disposer d'un "<code>php-cgi</code>" le plus léger possible en n'intégrant que les fonctionnalités dont nous avons besoin.</p>
### Compiltation de php-cgi ###
  * commencez par télécharger (voir http://php.net/downloads.php), decompresser et extraire les sources de php version 5.5 ou plus :
```
$  wget http://us1.php.net/get/php-5.5.5.tar.bz2/from/fr2.php.net/mirror -O php-5.5.5.tar.bz2
$ bzip2 -d php-5.5.5.tar.bz2
$ tar xvf php-5.5.5.tar
$ PHPSOURCE=/php-5.5.5; export PHPSOURCE
$ cd $PHPSOURCE
```
  * configurer la compilation avec les options minimum :
```
$ ./configure --prefix=/usr/local/php --disable-all --enable-session --enable-pdo --with-sqlite3 --with-pdo-sqlite --enable-json --disable-debug --with-pdo-mysql
```
  * compiler le source :
```
$ make
```
## Mise en place de l'ensemble ##
  * Choisissez le répertoire de "base" de l'installation (`/usr/local` par exemple, mais autres base possible comme `/opt`, `/apps`, ...). Assurez vous d'avoir les droits en écriture dans le répertoire choisi.
```
$ BASE=/usr/local/mea-edomus ; export BASE
$ mkdir $BASE
$ mkdir $BASE/bin
$ mkdir $BASE/var
$ mkdir $BASE/var/db
$ mkdir $BASE/var/log
$ mkdir $BASE/lib
$ mkdir $BASE/lib/mea-plugins
$ mkdir $BASE/lib/mea-gui
$ mkdir $BASE/etc
$ cp $SOURCES/mea-edomus/mea-edomus $BASE/bin
$ cp $PHPSOURCE/sapi/cgi/php-cgi $BASE/bin
$ cp $SOURCES/mea-edomus/plugins/* $BASE/lib/mea-plugins
$ cp -r $SOURCES/mea-edomus/ihm/* $BASE/lib/mea-gui
```
Remarque :
Si vous utilisez voulez remplacer `$BASE/mea-edomus` par `/usr`, l'organisation des répertoires à utiliser est légèrement différente :
    * utilisation de `/etc` à la place de `$BASE/etc`
    * utilisation de `/var` au lieu de `$BASE/var`. Vérifier que `/usr/var/db` existe bien ou le créer sinon.
```
$ mkdir /usr/lib/mea-plugins
$ mkdir /usr/lib/mea-gui
$ cp $SOURCES/mea-edomus/mea-edomus /usr/bin
$ cp $PHPSOURCE/sapi/cgi/php-cgi /usr/bin
$ cp $SOURCES/mea-edomus/plugins/* /usr/lib/mea-plugins
$ cp -r $SOURCES/mea-edomus/ihm/* /usr/lib/mea-gui
```
## Création et initialisation de la base de paramétrage ##
  * L'initialisation est automatisée (par rapport à la version 0.1Alpha1). Utilisez la commande suivante :
```
$ $BASE/bin/mea-edomus --basepath=$BASE --autoinit
```
## Installation et initialisation combinées ##
  * Vous pouvez utiliser l'éxecutable de `mea-edomus` pour vous aider à créer les répertoires et initialiser la base, il ne vous reste plus alors qu'à copier les fichiers :
```
$ BASE=/usr/local/mea-edomus ; export BASE
$ $SOURCES/mea-edomus/mea-edomus --autoinit --basepath=$BASE
$ cp $SOURCES/mea-edomus/mea-edomus $BASE/bin
$ cp $PHPSOURCE/sapi/cgi/php-cgi $BASE/bin
$ cp $SOURCES/mea-edomus/plugins/* $BASE/lib/mea-plugins
$ cp -r $SOURCES/mea-edomus/ihm/* $BASE/lib/mea-gui
```
<p>Remarque : cette solution fonctionne à l'identique pour <code>$BASE=/usr</code> (detection faite par le commande pour créer les bons répertoires).</p>
## Installation personnalisée ##
  * voir `mea-edomus --help`. Pour une configuration interactive passant en revue tous les paramètres, utilisez `mea-edomus --init --basepath=$BASE`.
## Initialisation de l'environnement Mysql ##
Pas encore disponible, fonctionnalité à valider pour la version 0.1-ALPHA3, mais la commande `mea-edomus --init` vous demandera des informations sur la base (bien qu'inutile pour le moment).