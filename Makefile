CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g

test_minimal: src/datum.c tests/test_minimal.c
	$(CC) $(CFLAGS) src/datum.c tests/test_minimal.c -o test_minimal

run: test_minimal
	./test_minimal

clean:
	rm -f test_minimal