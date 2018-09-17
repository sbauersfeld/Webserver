CC=gcc
CFLAGS=-Wall -Wextra

all:
	$(CC) $(CFLAGS) webserver.c -o webserver
clean:
	rm -f webserver webserver.tar.gz
dist:
	tar -czvf webserver.tar.gz webserver.c report.pdf Makefile README
