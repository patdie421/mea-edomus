SHELL = /bin/bash
CC = gcc

DEBUGFLAGS  = -g \
              -D__DEBUG_ON__ \
              -D__NO_TOMYSQL__ \
              -D_BSD_SOURCE
CFLAGS      = -std=c99 \
              -O2 \
              -I./src/xPLLib-mac -I/usr/include/mysql \
              -I/usr/local/mysql/include \
              -I/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 \
              $(DEBUGFLAGS)
LDFLAGS     = -L/usr/local/mysql/lib \
              -L/System/Library/Frameworks/Python.framework/Versions/2.7/lib \
              -lmysqlclient \
              -lpthread \
              -lm \
              -ldl \
              -lsqlite3 \
              -lpython2.7

SOURCES=$(shell echo src/*.c src/xPLLib-mac/*.c)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=mea-edomus

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

clean:
	-rm -f $(OBJECTS)
