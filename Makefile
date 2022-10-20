CC=g++

all: build

build: server subscriber


server:  server.o
		$(CC) $^ -o $@

server.o: server.cc
		$(CC) $^ -c

subscriber:  subscriber.o
		$(CC) $^ -o $@

subscriber.o: subscriber.cc
		$(CC) $^ -c

clean:
		rm *.o -f server subscriber

