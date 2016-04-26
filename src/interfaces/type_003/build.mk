ifndef BASEDIR
$(error - BASEDIR is unset)
endif

ifndef TECHNO
$(error - TECHNO is unset)
endif

SHELL = /bin/bash

DEBUGFLAGS  = -D__DEBUG_ON__
ifeq ($(TECHNO), linux)
   CFLAGS      = -std=c99 \
                 -std=gnu99 \
                 -D_BSD_SOURCE \
                 -O2 \
                 -DTECHNO_$(TECHNO) \
                 -I/usr/include/mysql \
                 -I/usr/include/python2.7 \
                 -I$(BASEDIR)/src \
                 $(DEBUGFLAGS)
endif
ifeq ($(TECHNO), macosx)
   CFLAGS      = -std=c99 \
                 -O2 \
                 -DTECHNO_$(TECHNO) \
                 -IxPLLib-mac \
                 -I/usr/local/mysql/include \
                 -I/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 \
                 $(DEBUGFLAGS)
endif

SOURCES=enocean.c \
interface_type_003.c

OBJECTS=$(addprefix $(TECHNO).objects/, $(SOURCES:.c=.o))

$(TECHNO).objects/%.o: %.c
	@$(CC) $(INCLUDES) -c $(CFLAGS) -MM -MT $(TECHNO).objects/$*.o $*.c > .deps/$*.dep
	$(CC) $(INCLUDES) -c $(CFLAGS) $*.c -o $(TECHNO).objects/$*.o

all: .deps $(TECHNO).objects $(OBJECTS)

.deps:
	@mkdir -p .deps

$(TECHNO).objects:
	@mkdir -p $(TECHNO).objects

clean:
	rm -f $(TECHNO).objects/*.o .deps/*.dep
 
-include .deps/*.dep
