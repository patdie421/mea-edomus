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
                 -I$(BASEDIR)/src \
                 $(DEBUGFLAGS)
endif

LIBDIR=$(BASEDIR)/lib/$(NAME)/$(TECHNO)

SOURCES= cJSON.c \
consts.c \
notify.c  \
parameters_utils.c  \
processManager.c  \
python_utils.c  \
mea_queue.c  \
mea_sockets_utils.c \
sqlite3db_utils.c  \
mea_string_utils.c \
mea_timer.c  \
mea_verbose.c \
serial.c \
mea_xpl.c

SOURCES2=interfaces/drivers/enocean.c \
interfaces/drivers/comio2.c

OBJECTS=$(addprefix $(TECHNO).objects/, $(SOURCES:.c=.o))
OBJECTS2=$(addprefix $(TECHNO).objects/, $(SOURCES2:.c=.o))
OBJECTS2B=$(addprefix $(TECHNO).objects/, $(notdir $(SOURCES2:.c=.o)))

$(TECHNO).objects/%.o: %.c
	$(CC) $(INCLUDES) -c $(CFLAGS) $*.c -o $(TECHNO).objects/$(notdir $*.o)

all: $(TECHNO).objects $(LIBDIR)/$(LIBNAME)

$(TECHNO).objects:
	@mkdir -p $(TECHNO).objects

$(LIBDIR)/$(LIBNAME): $(OBJECTS) $(OBJECTS2)
	@mkdir -p $(LIBDIR)
	rm -f $(LIBDIR)/$(LIBNAME)
	ar q $(LIBDIR)/$(LIBNAME) $(OBJECTS) $(OBJECTS2B)
	ranlib $(LIBDIR)/$(LIBNAME)

clean:
	rm -f $(TECHNO).objects/*.o $(LIBDIR)/$(LIBNAME) .deps/*.dep
 
-include .deps/*.dep
