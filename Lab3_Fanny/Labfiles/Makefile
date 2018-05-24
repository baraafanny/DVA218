CC = gcc
CFLAGS = -Wall -pthread -g -o 
PROGRAMS = client server
EXTRA_FILES = LinkedList.c

ALL: ${PROGRAMS}

client: client.c
	${CC} ${CFLAGS} client client.c ${EXTRA_FILES}

server: server.c
	${CC} ${CFLAGS} server server.c ${EXTRA_FILES}

clean:
	rm -f ${PROGRAMS}
