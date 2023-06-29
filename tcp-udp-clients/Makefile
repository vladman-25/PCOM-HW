CFLAGS = -Wall -g -lm

PORT = 12363
IP_SERVER = 127.0.0.1

ID = C1

all: server subscriber

server: server.c

subscriber: subscriber.c

.PHONY: clean run_server run_subscriber

run_server:
	./server ${PORT}

run_subscriber:
	./subscriber $(ID) ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber