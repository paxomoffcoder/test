CC = gcc
CFLAGS = -Wall -Wextra -std=c11

main: main.c
	$(CC) $(CFLAGS) main.c -o main

run: main
	./main

clean:
	rm -f main
