PHP=$SOURCE/complements/php
DOWNLOADS=$SOURCE/complements/downloads

all: $PHP/php-5.5.16/sapi/cgi/php-cgi

$PHP/php-5.5.16/sapi/cgi/php-cgi: $PHP/Makefile
   cd $PHP/php-5.5.16
   make

$PHP/Makefile: $PHP/php-5.5.16
   cd $PHP/php-5.5.16
   ./configure --prefix=/usr/local/php --disable-all --enable-session --enable-pdo --with-sqlite3 --with-pdo-sqlite --enable-json

$PHP/php-5.5.16: $DOWNLOAD/php5.tar.bz
   @mkdir -p $PHP
   cd $PHP
   bzip2 -dc $DOWNLOADS/php5.tar.bz | tar xvf -

$DOWNLOAD/php5.tar.bz2:
   @mkdir -p $DOWNLOADS
   curl -silent -L http://fr2.php.net/get/php-5.5.16.tar.bz2/from/this/mirror -o $DOWNLOADS/php5.tar.bz2

install: $PHP/php-5.5.16/sapi/cgi/php-cgi
   mkdir -p $BUILD/bin
   cp $PHP/php-5.5.16/sapi/cgi/php-cgi $BUILD/bin

clean:
   cd $PHP/php-5.5.16
   make clean
