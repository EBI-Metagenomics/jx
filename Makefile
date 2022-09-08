.POSIX:

JX_VERSION := 0.4.0

CC ?= gcc
CFLAGS := $(CFLAGS) -std=c99 -Wall -Wextra

SRC := jr.c jr_cursor.c jr_node.c jr_parser.c jw.c
OBJ := $(SRC:.c=.o)
HDR := jr_compiler.h jr_type.h jr_error.h jr_node.h jr_parser.h jr_cursor.h jr.h jw.h

all: meld

jx.h: $(HDR)
	./meld.sh hdr $^ > jx.h

jx.c: $(SRC) | jx.h
	./meld.sh src $^ > jx.c

jx.o: jx.c
	$(CC) $(CFLAGS) -c $<

meld: jx.h jx.c

test_read.o: test/read.c | meld
	$(CC) $(CFLAGS) -I. -c $< -o $@

test_read: test_read.o jx.o
	$(CC) $(CFLAGS) $^ -o $@

test_write.o: test/write.c | meld
	$(CC) $(CFLAGS) -I. -c $< -o $@

test_write: test_write.o jx.o
	$(CC) $(CFLAGS) $^ -o $@

check: test_read test_write
	./test_read
	./test_write

test: check

dist: clean meld
	mkdir -p jx-$(JX_VERSION)
	cp -R README.md LICENSE jx.h jx.c jx-$(JX_VERSION)
	tar -cf - jx-$(JX_VERSION) | gzip > jx-$(JX_VERSION).tar.gz
	rm -rf jx-$(JX_VERSION)

distclean:
	rm -f jx-$(JX_VERSION).tar.gz

clean: distclean
	rm -f $(OBJ) test_read test_write *.o jx.c jx.h

.PHONY: all check test meld dist distclean clean
