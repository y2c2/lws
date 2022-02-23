CC=gcc

CFLAGS=-Wall -Wextra -g -O2
MODE=debug
ifeq ($(MODE),debug)
	CFLAGS=-Wall -Wextra -g -O0
else ifeq ($(MODE),release)
	CFLAGS=-Wall -Wextra -g -O2
else ifeq ($(MODE),prof)
	CFLAGS=-Wall -Wextra -g -pg -O2
endif

INCLUDES=-I../../src/
LDFLAGS=-L../../ -lqv

ifeq ($(OS), Windows_NT)
	INCLUDES+=-I../../src/nt/
	LDFLAGS+=-lws2_32
else
	UNAME_S:=$(shell uname -s)
	ifeq ($(UNAME_S), Linux)
		INCLUDES+=-I../../src/unix/
		INCLUDES+=-I../../src/linux/
		LDFLAGS+=-lpthread
	endif
	ifeq ($(UNAME_S), FreeBSD)
		INCLUDES+=-I../../src/unix/
		INCLUDES+=-I../../src/bsd/
		LDFLAGS+=-lpthread
	endif
	ifeq ($(UNAME_S), Darwin)
		INCLUDES+=-I../../src/unix/
		INCLUDES+=-I../../src/macos/
		LDFLAGS+=-lpthread
	endif
endif

RM=rm -rf
SOURCES=$(wildcard *.c)
TARGET=test
CP=cp
RM=rm -rf

default:
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET) $(LDFLAGS) 

clean:
	$(RM) $(TARGET)

install:
	$(CP) $(TARGET) /usr/bin/

uninstall:
	$(RM) /usr/bin/$(TARGET)

