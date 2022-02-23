CC=gcc
RM=rm -rf
CFLAGS=-Wall -Wextra -MMD -fPIC
CFLAGS_DEBUG=-g -O0
CFLAGS_RELEASE=-O2
CFLAGS_PROF=-pg -O2 --no-pie
CFLAGS1=

CC_CHECK=clang
CFLAGS_CHECK="-Weverything -Wno-padded -Wno-packed -Wno-format-nonliteral"
MAKE=make

MODE=debug
ifeq ($(MODE),debug)
	CFLAGS+=$(CFLAGS_DEBUG)
else ifeq ($(MODE),release)
	CFLAGS+=$(CFLAGS_RELEASE)
else ifeq ($(MODE),prof)
	CFLAGS+=$(CFLAGS_RELEASE)
endif

PATH_DEPS=../deps

# QV
INCLUDES_QV=-I$(PATH_DEPS)/qv/src
LDFLAGS_QV=-L../deps/qv -lqv
ifeq ($(OS),Windows_NT)
	INCLUDES_QV+=-I$(PATH_DEPS)/qv/src/nt
	LDFLAGS_QV+=-lws2_32
else
	INCLUDES_QV+=-I$(PATH_DEPS)/qv/src/unix
	LDFLAGS_QV+=-lpthread
	UNAME_S:=$(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		INCLUDES_QV+=-I$(PATH_DEPS)/qv/src/linux
	endif
	ifeq ($(UNAME_S),FreeBSD)
		INCLUDES_QV+=-I$(PATH_DEPS)/qv/src/bsd
	endif
	ifeq ($(UNAME_S),Darwin)
		INCLUDES_QV+=-I$(PATH_DEPS)/qv/src/macos
	endif
endif

# EC
INCLUDES_EC=-I$(PATH_DEPS)/xenonjs/deps/ec/src
LDFLAGS_EC=-L../deps/xenonjs/deps/ec -lec

# HTTPParse
INCLUDES_HTTPPARSE=-I$(PATH_DEPS)/httpparse/src
LDFLAGS_HTTPPARSE=-L$(PATH_DEPS)/httpparse -lhttpparse

# uJSON
INCLUDES_UJSON=-I$(PATH_DEPS)/libujson/src
LDFLAGS_UJSON=-L$(PATH_DEPS)/libujson -lujson

# xjs
INCLUDES_XJS_COMPILER=-I$(PATH_DEPS)/xenonjs/compiler/include -I$(PATH_DEPS)/xenonjs/compiler/src $(INCLUDES_EC)
INCLUDES_XJS_RTS=-I$(PATH_DEPS)/xenonjs/rts/include -I$(PATH_DEPS)/xenonjs/rts/src
INCLUDES_LAUNCHER=-I. $(INCLUDES_EC) \
				  $(INCLUDES_XJS_COMPILER) \
				  $(INCLUDES_XJS_RTS) \
				  $(INCLUDES_QV) \
				  $(INCLUDES_UJSON) \
				  $(INCLUDES_HTTPPARSE)
LDFLAGS=-lm $(LDFLAGS_QV) $(LDFLAGS_EC) $(LDFLAGS_HTTPPARSE) $(LDFLAGS_UJSON)

BUILDPATH_LAUNCHER=.build_launcher
SRCS_LAUNCHER=$(wildcard *.c)
OBJS_LAUNCHER=$(addprefix $(BUILDPATH_LAUNCHER)/, $(notdir $(SRCS_LAUNCHER:.c=.o)))
DEPS_LAUNCHER=$(OBJS_LAUNCHER:.o=.d)

BUILDPATH_LIB_COMPILER=.build_lib_compiler
SRCS_LIB_COMPILER=$(wildcard $(PATH_DEPS)/xenonjs/compiler/src/*.c)
OBJS_LIB_COMPILER=$(addprefix $(BUILDPATH_LIB_COMPILER)/, $(notdir $(SRCS_LIB_COMPILER:.c=.o)))
DEPS_LIB_COMPILER=$(OBJS_LIB_COMPILER:.o=.d)

BUILDPATH_LIB_RTS=.build_lib_rts
SRCS_LIB_RTS=$(wildcard $(PATH_DEPS)/xenonjs/rts/src/*.c)
OBJS_LIB_RTS=$(addprefix $(BUILDPATH_LIB_RTS)/, $(notdir $(SRCS_LIB_RTS:.c=.o)))
DEPS_LIB_RTS=$(OBJS_LIB_RTS:.o=.d)

TARGET=../lws

default: launcher_part lib_part_compiler lib_part_rts
	@echo 'LD' $(abspath $(TARGET))
	@$(CC) $(CFLAGS) $(CFLAGS1) \
		$(INCLUDES_LAUNCHER) -o $(TARGET) \
		$(OBJS_LAUNCHER) $(OBJS_LIB_COMPILER) $(OBJS_LIB_RTS) \
		$(LDFLAGS)

release:
	$(MAKE) MODE=release

prof:
	$(MAKE) MODE=prof

launcher_part: $(OBJS_LAUNCHER)

lib_part_compiler: $(OBJS_LIB_COMPILER)

lib_part_rts: $(OBJS_LIB_RTS)

$(BUILDPATH_LAUNCHER)/%.o: %.c
	@-mkdir -p $(BUILDPATH_LAUNCHER)
	@echo 'CC' $(abspath $<)
	@$(CC) $(CFLAGS) $(CFLAGS1) $(INCLUDES_LAUNCHER) -o $@ -c $<

$(BUILDPATH_LIB_COMPILER)/%.o: $(PATH_DEPS)/xenonjs/compiler/src/%.c
	@-mkdir -p $(BUILDPATH_LIB_COMPILER)
	@echo 'CC' $(abspath $<)
	@$(CC) $(CFLAGS) $(CFLAGS1) $(INCLUDES_XJS_COMPILER) -o $@ -c $<

$(BUILDPATH_LIB_RTS)/%.o: $(PATH_DEPS)/xenonjs/rts/src/%.c
	@-mkdir -p $(BUILDPATH_LIB_RTS)
	@echo 'CC' $(abspath $<)
	@$(CC) $(CFLAGS) $(CFLAGS1) $(INCLUDES_XJS_RTS) -o $@ -c $<

all: clean $(OBJS_LAUNCHER)

clean:
	@$(RM) $(BUILDPATH_LAUNCHER)/*.o $(BUILDPATH_LAUNCHER)/*.d
	@$(RM) $(BUILDPATH_LIB_COMPILER)/*.o $(BUILDPATH_LIB_COMPILER)/*.d
	@$(RM) $(BUILDPATH_LIB_RTS)/*.o $(BUILDPATH_LIB_RTS)/*.d
	@$(RM) $(BUILDPATH_LAUNCHER)
	@$(RM) $(BUILDPATH_LIB_COMPILER)
	@$(RM) $(BUILDPATH_LIB_RTS)
	@$(RM) $(TARGET)

-include $(DEPS_LAUNCHER) $(DEPS_LIB_RTS) $(DEPS_LIB_RTS)

check:
	@$(MAKE) CC=$(CC_CHECK) CFLAGS1=$(CFLAGS_CHECK)

