#!/bin/bash

if [ $# -ne 1 ]
then
   echo "usage : $0 <SOURCE_BASEPATH>"
   exit 1
fi

SOURCE=$1

if [ ! -d $SOURCE ]
then
   echo "Grrr !!!"
   exit 1
fi

cd $SOURCE

rm -r $SOURCE/build/tmp > /dev/null 2>&1

mkdir -p $SOURCE/build/tmp
mkdir -p $SOURCE/build/tmp/bin
mkdir -p $SOURCE/build/tmp/lib
mkdir -p $SOURCE/build/tmp/lib/mea-gui
mkdir -p $SOURCE/build/tmp/lib/mea-plugins
mkdir -p $SOURCE/build/tmp/etc
mkdir -p $SOURCE/build/tmp/etc/init.d
mkdir -p $SOURCE/build/tmp/var
mkdir -p $SOURCE/build/tmp/var/log
mkdir -p $SOURCE/build/tmp/var/db

chmod -R 775 $SOURCE/build/tmp/*
chmod -R g+x $SOURCE/build/tmp/*

cd $SOURCE/gui
tar cf - . | ( cd $SOURCE/build/tmp/lib/mea-gui ; tar xvf - )

cd $SOURCE/plugins
tar cf - . | ( cd $SOURCE/build/tmp/lib/mea-plugins ; tar xvf - )

cp $SOURCE/linux/init.d/* $SOURCE/build/tmp/etc/init.d

cp $SOURCE/mea-edomus $SOURCE/build/tmp/bin
if [ -f $SOURCE/complements/php-cgi/php-5.x.x/sapi/cgi/php-cgi ]
then
   cp $SOURCE/complements/php-cgi/php-5.x.x/sapi/cgi/php-cgi $SOURCE/build/tmp/bin
fi
strip $SOURCE/build/tmp/bin/*

cd $SOURCE/build/tmp
tar cvf $SOURCE/build/mea-edomus.tar *

cd $SOURCE/build
tar cvf $SOURCE/build/mea-edomus.tar.pkg mea-edomus.tar install.sh
