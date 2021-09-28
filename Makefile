CC=gcc
CFLAGS=-Wall

LIBS=$(shell echo "lib/*")
SRC=$(shell echo "src/*")

build_server:
	$(CC) $(CFLAGS) -o tiny $(SRC) $(LIBS)
