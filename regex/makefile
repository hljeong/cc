CC = gcc
CFLAGS = -Wno-attributes -std=gnu11 -g
BIN = rc
SRCS = lex.c main.c parse.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	@ $(CC) $(CFLAGS) -o $@ $^

%.o: %.c cc.h
	@ $(CC) $(CFLAGS) -c -o $@ $<

clean:
	@ rm -f $(OBJS) a.S a.out cc test.err

test: $(BIN)
	@ ./test.sh
