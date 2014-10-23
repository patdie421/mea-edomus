ifndef SOURCE
$(error "SOURCE not set, can't make xplhub binary")
endif 

XPLHUB=$(SOURCE)/complements/xplhub
XPLHUBSOURCE=$(XPLHUB)/src

all: $(XPLHUBSOURCE)/xplhub/xplhub

$(XPLHUBSOURCE)/xplhub/xplhub: $(XPLHUBSOURCE)/xplhub/Makefile
	cd $(XPLHUBSOURCE)/xplhub ; make

clean:
	cd $(XPLHUBSOURCE)/xplhub ; make clean
