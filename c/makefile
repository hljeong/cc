CC = gcc
CFLAGS = -Wno-attributes -std=gnu11 -g
BIN = cc
SRCS = analyze.c codegen.c io.c lex.c parse.c main.c node.c token.c str.c type.c var.c
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
