CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g

# Bygg og kjør alle tester med Acutest
test: tests/test_minimal.c src/datum.c
	$(CC) $(CFLAGS) tests/test_minimal.c src/datum.c -o test_minimal
	./test_minimal

# Hvis du vil ha et eget mål for Acutest (valgfritt)
acutest: tests/test_acutest.c src/datum.c
	$(CC) $(CFLAGS) tests/test_acutest.c src/datum.c -o test_acutest
	./test_acutest

run: test
	@echo "Kjørte alle tester OK!"

clean:
	rm -f test_* *.o *.a