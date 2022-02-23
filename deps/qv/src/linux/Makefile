CC = gcc

CFLAGS = -Wall -Wextra -g -MMD -fPIC -fstack-protector-all -Wstack-protector
MODE = debug
ifeq ($(MODE),debug)
	CFLAGS += -O0
else ifeq ($(MODE),release)
	CFLAGS += -O2
else ifeq ($(MODE),prof)
	CFLAGS += -pg -O2
endif

CC_CHECK = clang
CFLAGS_CHECK = "-Weverything -Wno-padded -Wno-packed"

INCLUDES = -I./ -I../
RM = rm -rf

BUILDPATH = .build
SRCS = $(wildcard *.c)
OBJS = $(addprefix $(BUILDPATH)/, $(notdir $(SRCS:.c=.o)))
DEPS = $(OBJS:.o=.d)

default: $(OBJS)

$(BUILDPATH)/%.o: %.c
	@-mkdir -p $(BUILDPATH)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

all: clean $(OBJS)

clean:
	@$(RM) $(BUILDPATH)/*.o $(BUILDPATH)/*.d
	@$(RM) $(BUILDPATH)

-include $(DEPS)

check:
	@$(MAKE) CC=$(CC_CHECK) CFLAGS1=$(CFLAGS_CHECK)

