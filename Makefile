CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11
LDFLAGS = -pthread

SRC = src/mymalloc.c
HDR = src/mymalloc.h

all: test_basic test_stress test_patterns test_thread

test_basic: tests/test_basic.c $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ tests/test_basic.c $(SRC) $(LDFLAGS)

test_stress: tests/test_stress.c $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ tests/test_stress.c $(SRC) $(LDFLAGS)

test_patterns: tests/test_patterns.c $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ tests/test_patterns.c $(SRC) $(LDFLAGS)

test_thread: tests/test_thread.c $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ tests/test_thread.c $(SRC) $(LDFLAGS)

run: all
	./test_basic
	./test_stress
	./test_patterns
	./test_thread

clean:
	rm -f test_basic test_stress test_patterns test_thread results.csv

.PHONY: all run clean
