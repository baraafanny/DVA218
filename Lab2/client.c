/* File: client.c
 * Trying out socket communication between processes using the Internet protocol family.
 * Usage: client [host name], that is, if a server is running on 'lab1-6.idt.mdh.se'
 * then type 'client lab1-6.idt.mdh.se' and follow the on-screen instructions.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h> 

#define PORT 5555
#define hostNameLength 50
#define messageLength  256
#define MAXMSG 512

/* initSocketAddress
 * Initialises a sockaddr_in struct given a host name and a port.
 */
void initSocketAddress(struct sockaddr_in *name, char *hostName, unsigned short int port) {
  struct hostent *hostInfo; /* Contains info about the host */
  /* Socket address format set to AF_INET for Internet use. */
  name->sin_family = AF_INET;     
  /* Set port number. The function htons converts from host byte order to network byte order.*/
  name->sin_port = htons(port);   
  /* Get info about host. */
  hostInfo = gethostbyname(hostName); 
  if(hostInfo == NULL) {
    fprintf(stderr, "initSocketAddress - Unknown host %s\n",hostName);
    exit(EXIT_FAILURE);
  }
  /* Fill in the host name into the sockaddr_in struct. */
  name->sin_addr = *(struct in_addr *)hostInfo->h_addr;
}
/* writeMessage
 * Writes the string message to the file (socket) 
 * denoted by fileDescriptor.
 */
void writeMessage(int fileDescriptor, char *message) {
  int nOfBytes;
  
  nOfBytes = write(fileDescriptor, message, strlen(message) + 1);
  if(nOfBytes < 0) {
    perror("writeMessage - Could not write data\n");
    exit(EXIT_FAILURE);
  }
}
/*SendMessage
Handling input and sending messages to file(socket).*/
void *SendMessage(void *sock)
{
  char messageString[messageLength];
  int *socket = (int*)sock;
  
  /* Send data to the server */
  printf("\nType something and press [RETURN] to send it to the server.\n");
  printf("Type 'quit' to nuke this program.\n");
  fflush(stdin);

  while(1) 
  {
    printf("\n>");
    fgets(messageString, messageLength, stdin);
    messageString[messageLength - 1] = '\0';
    if(strncmp(messageString,"quit\n",messageLength) != 0)
      writeMessage(*socket, messageString);
    else 
    {  
      close(*socket);
      exit(EXIT_SUCCESS);
    }
  }

  pthread_exit(NULL);
}
/* readMessage
 * Reads and prints data read from the file (socket
 * denoted by the file descriptor 'fileDescriptor'.
 */
int readMessage(int fileDescriptor) {
  char buffer[MAXMSG];
  int nOfBytes;

  nOfBytes = read(fileDescriptor, buffer, MAXMSG);
  if(nOfBytes < 0) {
    perror("Could not read data from client\n");
    exit(EXIT_FAILURE);
  }
  else
    if(nOfBytes == 0) 
      /* End of file */
      return(-1);
    else 
      /* Data read */
      printf(">Incoming message: %s\n",  buffer);
  return(0);
}

//Listen to incoming broadcasts
void* ListenToMessages(void* sock)
{
  int *socket = (int*) sock;

  while(1)
  {
    readMessage(*socket);
  }

  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in serverName;
  char hostName[hostNameLength];
  pthread_t thread, thread2;
  int eid1, eid2;

  /* Check arguments */
  if(argv[1] == NULL) {
    perror("Usage: client [host name]\n");
    exit(EXIT_FAILURE);
  }
  else {
    strncpy(hostName, argv[1], hostNameLength);
    hostName[hostNameLength - 1] = '\0';
  }
  /* Create the socket */
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if(sock < 0) {
    perror("Could not create a socket\n");
    exit(EXIT_FAILURE);
  }
  /* Initialize the socket address */
  initSocketAddress(&serverName, hostName, PORT);
  /* Connect to the server */
  if(connect(sock, (struct sockaddr *)&serverName, sizeof(serverName)) < 0) 
  {
    perror("Could not connect to server\n");
    exit(EXIT_FAILURE);
  } 
  else
  {
  /*Creating a thread running SendMessage-function and sending socket with it.*/
    eid1 = pthread_create(&thread, NULL, SendMessage, &sock);
    if(eid1)
    {
      printf("ERROR return code from threadcreate is %d\n", eid1);
      exit(-1);
    }
    /*Creating thread running ListenToMessages-function and sending socket with it.*/
    eid2 = pthread_create(&thread2, NULL, ListenToMessages, &sock);
    if(eid2)
    {
      printf("ERROR return code from threadcreate is %d\n", eid2);
      exit(-1);
    }

  }

  pthread_exit(NULL);
}
