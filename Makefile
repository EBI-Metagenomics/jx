.POSIX:

JX_VERSION := 0.1.0
JX_CFLAGS := $(CFLAGS) -std=gnu11

SRC := jx.c jx_node.c jx_parser.c jx_cursor.c
OBJ := $(SRC:.c=.o)
HDR := jx.h jx_cursor.h jx_node.h jx_type.h zc_strto_static.h jx_compiler.h jx_error.h jx_parser.h jx_cursor.h

all: libjx.a

%.o: %.c $(HDR)
	$(CC) $(JX_CFLAGS) -c $<

libjx.a: $(OBJ)
	ar rcs $@ $^

test_read.o: test_read.c
	$(CC) $(JX_CFLAGS) -c test_read.c

test_read: test_read.o libjx.a
	$(CC) -o $@ $< -L. -ljx

test: test_read
	./test_read

clean:
	rm -f $(OBJ) libjx.a test_read test_read.o

.PHONY: all test clean
