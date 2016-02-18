ifndef BASEDIR
$(error - BASEDIR is unset)
endif

ifndef TECHNO
$(error - TECHNO is unset)
endif

NAME=mealib

LIBNAME=$(NAME).a

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

LIBDIR=$(BASEDIR)/lib/$(NAME)/$(TECHNO)

SOURCES=arduino_pins.c \
cJSON.c \
comio2.c \
consts.c \
enocean.c  \
mongoose.c  \
notify.c  \
parameters_utils.c  \
philipshue.c  \
philipshue_color.c  \
processManager.c  \
python_utils.c  \
mea_queue.c  \
mea_sockets_utils.c \
sqlite3db_utils.c  \
mea_string_utils.c \
mea_timer.c  \
mea_verbose.c \
tokens.c  \
serial.c \
xbee.c 

OBJECTS=$(addprefix $(TECHNO).objects/, $(SOURCES:.c=.o))

$(TECHNO).objects/%.o: %.c
	@$(CC) $(INCLUDES) -c $(CFLAGS) -MM -MT $(TECHNO).objects/$*.o $*.c > .deps/$*.dep
	$(CC) $(INCLUDES) -c $(CFLAGS) $*.c -o $(TECHNO).objects/$*.o

all: .deps $(TECHNO).objects $(LIBDIR)/$(LIBNAME)

.deps:
	@mkdir -p .deps

$(TECHNO).objects:
	@mkdir -p $(TECHNO).objects

$(LIBDIR)/$(LIBNAME): $(OBJECTS)
	@mkdir -p $(LIBDIR)
	rm -f $(LIBDIR)/$(LIBNAME)
	ar q $(LIBDIR)/$(LIBNAME) $(OBJECTS)
	ranlib $(LIBDIR)/$(LIBNAME)

clean:
	rm -f $(TECHNO).objects/*.o $(LIBDIR)/$(LIBNAME) .deps/*.dep
 
-include .deps/*.dep
