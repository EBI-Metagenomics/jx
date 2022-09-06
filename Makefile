.POSIX:

JX_VERSION := 0.1.0
JX_CFLAGS := $(CFLAGS) -std=gnu11

SRC := jx.c
OBJ := $(SRC:.c=.o)
HDR := jx.h private.h

all: libjx.a

%.o: %.c $(HDR)
	$(CC) $(JX_CFLAGS) -c $<

libjx.a: $(OBJ)
	ar rcs $@ $<

test_read.o: test_read.c
	$(CC) $(JX_CFLAGS) -c test_read.c

test_read: test_read.o libjx.a
	$(CC) -o $@ $< -L. -ljx

test: test_read
	./test_read

clean:
	rm -f $(OBJ) libjx.a test_read test_read.o

.PHONY: all test clean
