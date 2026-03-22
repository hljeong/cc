CC = gcc
CFLAGS = -Wno-attributes -std=c11 -g
BIN = lc

.PHONY: all clean test

all: $(BIN)

$(BIN): main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BIN)

test: $(BIN)
	./test.sh
