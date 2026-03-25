CC = gcc
CFLAGS = -Wno-attributes -std=c11 -g
SRCS = debug.c lex.c parse.c reduce.c main.c
OBJS = $(SRCS:.c=.o)
BIN = lc

.PHONY: all clean test

all: $(BIN)

$(BIN): $(OBJS)
	@ $(CC) $(CFLAGS) -o $@ $^

%.o: %.c lc.h
	@ $(CC) $(CFLAGS) -c -o $@ $<

clean:
	@ rm -f $(OBJS) $(BIN)

test: $(BIN)
	@ ./test.sh
