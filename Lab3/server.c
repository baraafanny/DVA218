/* File: server.c
 * Trying out socket communication between processes using the Internet protocol family.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include "LinkedList.h"
#include <stdbool.h>
#include <pthread.h> 

#define INIT 0 
#define WAIT_SYN 1  
#define WAIT_ACK 2
#define TIMEOUT 3
#define ERR 4
#define ESTABLISHED 5

#define got_syn 6
#define send_synack 7
#define got_ack 8

#define PORT 5555
#define MAXMSG 512

#define DATA 10
#define ACK 11

#define got_fin 13
#define send_finack 14

bool loopstate = true;
int state = WAIT_SYN;
int expectedseq = 0;
int event = INIT;
int wsize;
void Teardown(int fileDescriptor, socklen_t size);
struct sockaddr_in clientName;
/* makeSocket
 * Creates and names a socket in the Internet
 * name-space. The socket created exists
 * on the machine from which the function is 
 * called. Instead of finding and using the
 * machine's Internet address, the function
 * specifies INADDR_ANY as the host address;
 * the system replaces that with the machine's
 * actual address.
 */
int makeSocket(unsigned short int port) {
  int sock;
  struct sockaddr_in name;

  /* Create a socket. */
  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //MAke UDP socket.
  if(sock < 0) {
    perror("Could not create a socket\n");
    exit(EXIT_FAILURE);
  }
  memset((char*) &name, 0, sizeof(name));
  /* Give the socket a name. */
  /* Socket address format set to AF_INET for Internet use. */
  name.sin_family = AF_INET;
  /* Set port number. The function htons converts from host byte order to network byte order.*/
  name.sin_port = htons(port);

  /* Set the Internet address of the host the function is called from. */
  /* The function htonl converts INADDR_ANY from host byte order to network byte order. */
  /* (htonl does the same thing as htons but the former converts a long integer whereas
   * htons converts a short.) 
   */
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  /* Assign an address to the socket by calling bind. */
  if(bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) 
  {
    perror("Could not bind a name to the socket\n");
    exit(EXIT_FAILURE);
  }
  return(sock);
}
/* writeMessage
 * Writes the string message to the file (socket) 
 * denoted by fileDescriptor.
 */
void writeMessage(int fileDescriptor, rtp message, socklen_t size) 
{
  int nOfBytes;

  nOfBytes = sendto(fileDescriptor, &message, sizeof(message) + 1, 0, (struct sockaddr *)&clientName, size);
  if(nOfBytes < 0) 
  {
    perror("writeMessage - Could not write data\n");
    exit(EXIT_FAILURE);
  }
}
/* readMessage
 * Reads and prints data read from the file (socket
 * denoted by the file descriptor 'fileDescriptor'.
 */
int readMessage(int fileDescriptor, socklen_t size) 
{
  rtp *RHeader = (rtp*)malloc(sizeof(rtp));
  if(!RHeader)
  {
    printf("Could not allocate memory in Readmessage");
    exit(EXIT_FAILURE);
  }

  int nOfBytes;
  size = sizeof(struct sockaddr_in);
  nOfBytes = recvfrom(fileDescriptor, RHeader, sizeof(rtp), 0, (struct sockaddr *)&clientName, &size);
  if(nOfBytes < 0) 
  {
    free(RHeader);
    perror("Could not read data from client\n");
    exit(EXIT_FAILURE);
  }
  else
    if(nOfBytes == 0) 
      /* End of file */
      return(-1);
    else 
      /* Data read */

       if(RHeader != NULL)
        {
          if(RHeader->flags == got_syn)
          {
            printf("Received SYN from client at timestamp %ld!\n", time(0));
            int flags = RHeader->flags;
            free(RHeader);
            return flags;
          }
          if(RHeader->flags == got_ack)
          {
            removeHead();
            printf("Received ACK from client at timestamp %ld!\n", time(0));
            int flags = RHeader->flags;
            free(RHeader);
            return flags;
          }
          if(RHeader->flags == got_fin)
          {
            printf("\n[Got FIN, Server shutting down...]\n");
            Teardown(fileDescriptor, size);
            return(0);
          }

          if(expectedseq == RHeader->seq && RHeader->flags == DATA)
          {
            sleep(1);

            RHeader->flags = ACK;

            writeMessage(fileDescriptor, *RHeader, size);
            printf("ACK on package %d was sent to the client at timestamp %ld!\n", RHeader->seq, time(0));
            expectedseq++;
          }
          else
          {
            printf("Package out of order!\n");
            printf("[Dropping package...]\n");
            free(RHeader);
          }
        }
        else
        {
          free(RHeader);
          printf("Received Empty Header.");
        }
  return(0);
}
void Teardown(int fileDescriptor, socklen_t size)
{
  int event = INIT;
  rtp Header;
  Header.flags = send_finack; 
  Header.id = 0;
  Header.seq = -1;
  Header.windowsize = 1;
  Header.crc = 0;
 
  writeMessage(fileDescriptor, Header, size);
  createHeader(&Header);
  printf("FIN + ACK sent to the server at timestamp %ld!\n", time(0));
  state = WAIT_ACK;
  while(loop == true)
  {
    //Reads events from incoming packages. 
    event = readMessage(fileDescriptor, size); //Event received package information
    //Switches the current state of our connectionsetup. 
    switch(state)
    {
      case WAIT_ACK:
      {
        if(event == got_ack)
        {
          printf("Server is shutted down.\n");
          state = ESTABLISHED;
          event = INIT;
          loopstate = false;
          return;
        }
        break;
      }
      default: 
      {
        printf("Default reached \n");
        return;
        break;
      }
    }
  }

}
/*ConnectionSetup-function
Statemachine for connection setup on the receiving end.*/
void ConnectionSetup(int fileDescriptor, socklen_t size)
{ 
  rtp Header;
  Header.id = 0;
  Header.seq = -1;
  Header.crc = 0;

  while(loop == true)
  {
    
    //Reads events from incoming packages. 
    event = readMessage(fileDescriptor, size); //Event received package information
    //Switches the current state of our connectionsetup. 
    switch(state)
    {
      case WAIT_SYN:
      {
        if(event == got_syn)
        {
          state = WAIT_ACK;
          event = INIT;
          //Skicka syn + ack
          Header.windowsize = wsize;
          Header.flags = send_synack;
          writeMessage(fileDescriptor, Header,size); 
          printf("SYN + ACK was sent to the client at timestamp %ld!\n", time(0));
          createHeader(&Header);
          //Timer skicka om när de är slut.
        }
        break;
      }
      case WAIT_ACK:
      {
        if(event == got_ack)
        {
          printf("Server is connected!\n");
          return;
        }
        break;
      }
      default: 
      {
        printf("Default reached \n");
        return;
        break;
      }
    }
  }
}
//Listen to incoming messages
void* ListenToMessages(int socket, socklen_t size)
{
  while(1)
  {
    readMessage(socket, size);
  }
}

int main(int argc, char *argv[]) 
{
  socklen_t size;
  int sock;

  size = sizeof(struct sockaddr_in);
  /* Create a UDP socket */
  sock = makeSocket(PORT);

  printf("\n[Waiting for connections...]\n");

  ConnectionSetup(sock, size);

  printf("\n[Waiting for messages...]\n");

  while(loopstate == true) 
  {
      ListenToMessages(sock, size);
  }

  return 0;
}