SHELL = /bin/bash
CC = gcc

DEBUGFLAGS  = -D__DEBUG_ON__ -D_BSD_SOURCE -g
CFLAGS      = -std=c99 -I/usr/include/mysql $(DEBUGFLAGS)
LDFLAGS     = -L/usr/lib/arm-linux-gnueabihf \
              -lmysqlclient -lpthread -lm -lrt -ldl -lsqlite3 -lxPL

SOURCES=$(shell echo src/*.c)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=mea-edomus

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

clean:
	-rm -f $(OBJECTS)
