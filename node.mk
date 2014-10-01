ifndef SOURCE
$(error "SOURCE not set, can't make php-cgi binary")
endif 

NODEJS=$(SOURCE)/complements/nodejs
NODEJSSOURCE=$(NODEJS)/src
DOWNLOAD=$(SOURCE)/complements/downloads

all: $(NODEJSSOURCE)/node-v0.10.32/out/Release/node

$(NODEJSSOURCE)/node-v0.10.32/out/Release/node: $(NODEJSSOURCE)/node-v0.10.32/Makefile
	cd $(NODEJSSOURCE)/node-v0.10.32 ; make

$(NODEJSSOURCE)/node-v0.10.32/Makefile: $(NODEJSSOURCE)/node-v0.10.32/extract.ok
	cd $(NODEJSSOURCE)/node-v0.10.32 ; ./configure

http://nodejs.org/dist/v0.10.32/node-v0.10.32.tar.gz

$(NODEJSSOURCE)/node-v0.10.32/extract.ok: $(DOWNLOAD)/node-v0.10.32.tar.gz
	@mkdir -p $(NODEJSSOURCE)
	cd $(NODEJSSOURCE) ; tar xvjf $(DOWNLOAD)/node-v0.10.32.tar.gz ; touch $(NODEJSSOURCE)/node-v0.10.32/extract.ok

$(DOWNLOAD)/node-v0.10.32.tar.gz:
	@mkdir -p $(DOWNLOAD)
	curl -L http://nodejs.org/dist/v0.10.32/node-v0.10.32.tar.gz

clean:
	cd $(NODEJSSOURCE)/node-v0.10.32
	make clean

fullclean:
	rm -r $(NODEJSSOURCE)
	rm $(DOWNLOAD)/node-v0.10.32.tar.gz
