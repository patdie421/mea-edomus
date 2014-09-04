ifndef SOURCE
$(error "SOURCE not set, can't make php-cgi binary")
endif 

PHPCGI=$SOURCE/complements/php-cgi
PHPSOURCE=$PHPCGI/src
DOWNLOADS=$SOURCE/complements/downloads

all: $PHPSOURCE/php-5.5.16/sapi/cgi/php-cgi

$PHPSOURCE/php-5.5.16/sapi/cgi/php-cgi: $PHP/Makefile
   cd $PHP/php-5.5.16
   make

$PHPSOURCE/Makefile: $PHPSOURCE/php-5.5.16
   cd $PHPSOURCE/php-5.5.16
   ./configure --prefix=/usr/local/php --disable-all --enable-session --enable-pdo --with-sqlite3 --with-pdo-sqlite --enable-json

$PHPSOURCE/php-5.5.16: $DOWNLOAD/php5.tar.bz
   @mkdir -p $PHPSOURCE
   cd $PHPSOURCE
   bzip2 -dc $DOWNLOADS/php5.tar.bz | tar xvf -

$DOWNLOAD/php5.tar.bz2:
   @mkdir -p $DOWNLOADS
   curl -silent -L http://fr2.php.net/get/php-5.5.16.tar.bz2/from/this/mirror -o $DOWNLOADS/php5.tar.bz2

install: $PHPSOURCE/php-5.5.16/sapi/cgi/php-cgi
   @mkdir -p $BUILD/bin
   cp $PHPSOURCE/php-5.5.16/sapi/cgi/php-cgi $BUILD/bin

clean:
   cd $PHPSOURCE/php-5.5.16
   make clean

fullclean:
   rm -r $PHPSOURCE
   rm $DOWNLOADS/php5.tar.bz2
