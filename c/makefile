CC = gcc
CFLAGS = -Wno-attributes -std=gnu11 -g
BIN = cc
SRCS = analyze.c codegen.c io.c lex.c main.c node.c parse.c str.c symbol.c token.c type.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	@ $(CC) $(CFLAGS) -o $@ $^

%.o: %.c cc.h
	@ $(CC) $(CFLAGS) -c -o $@ $<

clean:
	@ rm -f *.o a.S a.out cc test.err

test: $(BIN)
	@ ./test.sh
