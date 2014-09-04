# à lancer de préférence en sudo

if [ $# -ne 1 ]
then
   echo "usage : $0 BASEPATH"
   echo "suggestion : $0 /usr/local/mea-edomus"
   exit 1
fi

# recupération des parametres
BASEPATH="$1"

if [ "$BASEPATH" == "/usr" || "$BASEPATH" == "/" ]
then
   echo "ERROR : can't actualy install in $BASEPATH. Choose an other directory, or install manualy !"
   exit 0
fi

if [ ! -d "$BASEPATH" ]
then
   echo "ERROR : $BASEPATH must exist. Create it !"
   exit 1
fi

if [ ! -w "$BASEPATH" ]
then
   echo "ERROR : You must have write permissions on $BASEPATH ! (use sudo)"
   exit 1
fi

NBFILEINBASEPATH=`ls -a "$BASEPATH" | wc -l`
if [ $NBFILEINBASEPATH -ne 2 ]
then
   echo "WARNING : $BASEPATH is not empty. Installation could be instable ... or other(s) application(s) could be damadged"
   while true; do
      read -p "Continue (Y/N) ? " YESNO
      case $YESNO in
        [YyOo] ) break;;
        [Nn] ) exit 1;;
        * ) echo "Please answer y or n !";;
      esac
   done
fi

MEAUSER='mea-edomus'
MEAGROUP='mea-edomus'

sudo groupadd -f "$MEAUSER" > /dev/null 2>&1
sudo useradd -r -N -g "$MEAGROUP" -M -d '' -s /dev/false > /dev/null 2>&1


CURRENTPATH=`pwd`;
cd "$BASEPATH"

sudo tar xf "$CURRENTPATH"/mea-edomus.tar

sudo strip ./bin/*

sudo ./bin/mea-edomus --autoinit --basepath="$BASEPATH"

sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/lib/mea-gui
sudo chmod -R 775 "$MEAUSER":"$MEAGROUP" "$BASEPATH"/lib/mea-gui
sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/lib/mea-plugin
sudo chmod -R 775 "$MEAUSER":"$MEAGROUP" "$BASEPATH"/lib/mea-plugin
sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/var/log
sudo chmod -R 775 "$MEAUSER":"$MEAGROUP" "$BASEPATH"/var/log
sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/var/db
sudo chmod -R 775 "$MEAUSER":"$MEAGROUP" "$BASEPATH"/var/db
sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/etc
sudo chmod -R 775 "$MEAUSER":"$MEAGROUP" "$BASEPATH"/etc

# déclaration du service
# les slashs sont transformés en \/ pour sed
BASEPATH4SED=`echo "$BASEPATH" | sed -e 's/\\//\\\\\\//g'`
# Pour mémoire :
# echo $BASEPATH | sed -e 's/\//\\\//g' donne le résultat attendu
# mais pour utilisation avec les `` il faut remplacer en plus les \ par des \\

# déclaration du service
sudo sed -e 's/<BASEPATH>/'"$BASEPATH4SED"'/g' -e 's/<USER>/'"$MEAUSER"'/g' ./etc/init.d/mea-edomus > /etc/init.d/mea-edomus

# activation du service
sudo update-rc.d mea-edomus defaults

# lancement du service
sudo service mea-edomus start

cd "$CURRENTPATH"