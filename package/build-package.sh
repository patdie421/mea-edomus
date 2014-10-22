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

rm -r $SOURCE/package/tmp > /dev/null 2>&1

mkdir -p $SOURCE/package/tmp
mkdir -p $SOURCE/package/tmp/bin
mkdir -p $SOURCE/package/tmp/lib
mkdir -p $SOURCE/package/tmp/lib/mea-gui
mkdir -p $SOURCE/package/tmp/lib/mea-plugins
mkdir -p $SOURCE/package/tmp/etc
mkdir -p $SOURCE/package/tmp/etc/init.d
mkdir -p $SOURCE/package/tmp/var
mkdir -p $SOURCE/package/tmp/var/log
mkdir -p $SOURCE/package/tmp/var/db

chmod -R 775 $SOURCE/package/tmp/*
chmod -R g+x $SOURCE/package/tmp/*

cd $SOURCE/gui
tar cf - . | ( cd $SOURCE/package/tmp/lib/mea-gui ; tar xf - )

cd $SOURCE/plugins
tar cf - . | ( cd $SOURCE/package/tmp/lib/mea-plugins ; tar xf - )

cp $SOURCE/linux/init.d/* $SOURCE/package/tmp/etc/init.d

cp $SOURCE/mea-edomus $SOURCE/package/tmp/bin

if [ -f $SOURCE/complements/php-cgi/src/php-5.5.16/sapi/cgi/php-cgi ]
then
   cp $SOURCE/complements/php-cgi/src/php-5.5.16/sapi/cgi/php-cgi $SOURCE/package/tmp/bin
fi

if [ -f $SOURCE/complements/nodejs/src/node-v0.10.32/out/Release/node ]
then
   cp $SOURCE/complements/nodejs/src/node-v0.10.32/out/Release/node $SOURCE/package/tmp/bin
fi

strip $SOURCE/package/tmp/bin/*

cd $SOURCE/package/tmp
cd $SOURCE/package/tmp
tar cf $SOURCE/package/mea-edomus.tar *

cd $SOURCE/package
tar cjf $SOURCE/package/mea-edomus.tar.pkg.bz2 mea-edomus.tar install.sh

rm mea-edomus.tar > /dev/null 2>&1
rm -r $SOURCE/package/tmp > /dev/null 2>&1
