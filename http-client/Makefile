# CC=gcc
# CFLAGS=-I.

# comm: comm.c requests.c helpers.c buffer.c parson.c
# 	$(CC) -o comm comm.c requests.c helpers.c buffer.c parson.c -Wall

# run: comm
# 	./comm

# clean:
# 	rm -f *.o comm
CC=gcc
CFLAGS=-I.

client: client.c requests.c helpers.c buffer.c parson.c
	$(CC) -o client client.c requests.c helpers.c buffer.c parson.c -Wall

run: client
	./client

clean:
	rm -f *.o client
