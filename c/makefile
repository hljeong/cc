CC = gcc
CFLAGS = -Wno-attributes -std=c11 -g
SRCS = debug.c lex.c parse.c codegen.c main.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: cc

cc: $(OBJS)
	@ $(CC) $(CFLAGS) -o $@ $^

%.o: %.c cc.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@ rm -f $(OBJS) a.S a.out cc
