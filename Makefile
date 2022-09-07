.POSIX:

JX_VERSION := 0.1.0
JX_CFLAGS := $(CFLAGS) -std=gnu11 -Wall

SRC := jr.c jr_node.c jr_parser.c jr_cursor.c jw.c
OBJ := $(SRC:.c=.o)
HDR := jr.h jr_cursor.h jr_node.h jr_type.h zc_strto_static.h jr_compiler.h jr_error.h jr_parser.h jw.h

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

clean:
	rm -f $(OBJ) libjx.a test_read test_read.o test_write test_write.o

.PHONY: all check clean
