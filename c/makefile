CC = gcc
CFLAGS = -Wno-attributes -std=c11 -g
BIN = cc
SRCS = analyze.c codegen.c debug.c lex.c parse.c main.c node.c token.c type.c var.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	@ $(CC) $(CFLAGS) -o $@ $^

%.o: %.c cc.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@ rm -f $(OBJS) a.S a.out cc test.err

test: $(BIN)
	@ ./test.sh
