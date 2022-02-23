MAKE = make
AR = ar
AR_FLAGS = rcs
CP = cp
RM = rm -rf
MKDIR = mkdir -p
TARGET = libqv.a
SRCDIR = ./src
SRCS = $(wildcard ./src/*.c)
HDRS = $(wildcard ./src/*.h)
OBJS = $(patsubst ./src/%.c,./src/.build/%.o,$(wildcard ./src/*.c))

ifeq ($(OS),Windows_NT)
	SRCDIR+=./src/nt
	SRCS+=$(wildcard ./src/nt/*.c)
	HDRS+=$(wildcard ./src/nt/*.h)
	OBJS+=$(patsubst ./src/nt/%.c,./src/nt/.build/%.o,$(wildcard ./src/nt/*.c))
else
	SRCDIR+=./src/unix
	SRCS+=$(wildcard ./src/unix/*.c)
	HDRS+=$(wildcard ./src/unix/*.h)
	OBJS+=$(patsubst ./src/unix/%.c,./src/unix/.build/%.o,$(wildcard ./src/unix/*.c))

	UNAME_S:=$(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		SRCDIR+=./src/linux
		SRCS+=$(wildcard ./src/linux/*.c)
		HDRS+=$(wildcard ./src/linux/*.h)
		OBJS+=$(patsubst ./src/linux/%.c,./src/linux/.build/%.o,$(wildcard ./src/linux/*.c))
	endif
	ifeq ($(UNAME_S),FreeBSD)
		SRCDIR+=./src/bsd
		SRCS+=$(wildcard ./src/bsd/*.c)
		HDRS+=$(wildcard ./src/bsd/*.h)
		OBJS+=$(patsubst ./src/bsd/%.c,./src/bsd/.build/%.o,$(wildcard ./src/bsd/*.c))
	endif
	ifeq ($(UNAME_S),Darwin)
		SRCDIR+=./src/macos
		SRCS+=$(wildcard ./src/macos/*.c)
		HDRS+=$(wildcard ./src/macos/*.h)
		OBJS+=$(patsubst ./src/macos/%.c,./src/macos/.build/%.o,$(wildcard ./src/macos/*.c))
		AR=libtool
		AR_FLAGS=-static -o
	endif
endif

debug: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	@for entry in $(SRCDIR); do \
		$(MAKE) -C $$entry; \
	done
	$(AR) $(AR_FLAGS) $(TARGET) $(OBJS)

clean:
	@for entry in $(SRCDIR); do \
		$(MAKE) -C $$entry clean; \
	done
	$(RM) $(TARGET)

check:
	@for entry in $(SRCDIR); do \
		$(MAKE) -C $$entry clean; \
		$(MAKE) -C $$entry check; \
	done
	$(RM) $(TARGET)

install:
	$(CP) $(TARGET) /usr/lib/
	$(MKDIR) /usr/include/qv/
	for entry in $(HDRS); do \
		$(CP) $$entry /usr/include/qv; \
	done

uninstall:
	$(RM) /usr/lib/$(TARGET)
	$(RM) /usr/include/qv/


