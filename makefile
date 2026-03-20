CC = gcc
CFLAGS = -Wno-attributes -std=c11 -g

.PHONY: build clean

build:
	@ $(CC) $(CFLAGS) -o cc main.c

clean:
	@ rm -rf a.S a.out cc
