.POSIX:

JX_VERSION := 0.1.0
JX_CFLAGS := $(CFLAGS) -std=gnu11 -Wall

SRC := jr.c jr_cursor.c jr_node.c jr_parser.c jw.c
OBJ := $(SRC:.c=.o)
HDR := jr_compiler.h jr_type.h jr_error.h jr_node.h jr_parser.h jr_cursor.h jr.h jw.h

all: libjx.a check

%.o: %.c $(HDR)
	$(CC) $(JX_CFLAGS) -c $<

libjx.a: $(OBJ)
	ar rcs $@ $^

test_read.o: test/read.c
	$(CC) $(JX_CFLAGS) -I. -c test/read.c -o test_read.o

test_read: test_read.o libjx.a
	$(CC) -o $@ $< -L. -ljx

test_write.o: test/write.c
	$(CC) $(JX_CFLAGS) -I. -c test/write.c -o test_write.o

test_write: test_write.o libjx.a
	$(CC) -o $@ $< -L. -ljx

check: test_read test_write
	./test_read
	./test_write

dist: $(HDR) $(SRC)
	./meld.sh hdr $(HDR) > jx.h
	./meld.sh src $(SRC) > jx.c

clean:
	rm -f $(OBJ) libjx.a test_read test_read.o test_write test_write.o jx.c jx.h

.PHONY: all check dist clean
