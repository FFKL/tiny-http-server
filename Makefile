CC=gcc
CFLAGS=-Wall

LIBS=$(shell echo "lib/*")
SRC=$(shell echo "src/*")

build_server:
	$(CC) $(CFLAGS) -o tiny $(SRC) $(LIBS) -lpthread

build_cgi_scripts:
	$(CC) $(CFLAGS) -o adder cgi-src/adder.c
