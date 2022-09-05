.POSIX:

JX_VERSION := 0.1.0

SRC = jx.c
OBJ = $(SRC:.c=.o)
HDR = jx.h private.h
LIBS = libs/jsmn.h libs/zc_strto_static.h libs/cco.h libs/xxhash.h

all: libjx.a

%.o: %.c $(LIBS) $(HDR)
	$(CC) $(CFLAGS) -c $<

libjx.a: $(OBJ)
	ar rcs $@ $<

test_read.o: test_read.c
	$(CC) $(CFLAGS) -c test_read.c

test_read: test_read.o | libjx.a
	$(CC) -o $@ $< -L. -ljx

test: test_read
	./test_read

clean:
	rm -f $(OBJ) libjx.a test_read test_read.o

.PHONY: all test clean
