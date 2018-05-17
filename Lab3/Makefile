CC = gcc
CFLAGS = -Wall -pthread -g
PROGRAMS = client server
EXTRA_FILES = LinkedList.c  

ALL: ${PROGRAMS}

client: client.c
	${CC} ${CFLAGS} -o client client.c ${EXTRA_FILES}

server: server.c
	${CC} ${CFLAGS} -o server server.c ${EXTRA_FILES}

clean:
	rm -f ${PROGRAMS}
