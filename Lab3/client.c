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
#include <sys/times.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <netdb.h>
#include <pthread.h> 
#include "LinkedList.h"

#define PORT 5555
#define hostNameLength 50
#define messageLength  256

#define INIT 0 
#define WAIT_SYNACK 1  
#define WAIT_ACK 2
#define WAIT_TIMEOUT 3
#define ERR 4
#define ESTABLISHED 5

#define send_syn 6
#define got_synack 7
#define send_ack 8

#define DATA 10
#define ACK 11

#define WAIT_FINACK 12
#define send_fin 13
#define got_finack 14

void Teardown(int fileDescriptor, socklen_t size);

int state;
fd_set set;
int wsize = 2;
struct sockaddr_in serverName;

//Variable for Sent Headers.
rtp Header;

/*rtp checkSum(rtp Header)
{
  //Digest length always 128bits
  char hash[MD5_DIGEST_LENGTH];
  int random;

  memset(Header->crc, 0, MD5_DIGEST_LENGTH);
  //MD5 hash function which produces a 128bit hash value
  MD5((unsigned char *) Header, sizeof(rtp), hash);

  random = (ERROR) ? rand() % 100 : 100;
  if(random < 10) 
  {//Error generation
      hash[15] += 6;
  }
  else if(random > 30)
  {
      memcpy(Header->crc, hash, MD5_DIGEST_LENGTH);
      return Header;
  }
  return 0;
}*/

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
void writeMessage(int fileDescriptor, rtp message, socklen_t size) 
{
  int nOfBytes;

  nOfBytes = sendto(fileDescriptor, &message, sizeof(message) + 1, 0, (struct sockaddr *)&serverName, size);
  if(nOfBytes < 0) 
  {
    perror("writeMessage - Could not write data\n");
    exit(EXIT_FAILURE);
  }
}
/*SendMessage
Handling input and sending messages to file(socket).*/
void *SendMessage(int socket, socklen_t size)
{
  //Integer that keeps track of package sequence number. Is added when ever a package is sent.
  int seqnr = 0;
  /* Send data to the server */
  while(1) 
  {

    //Send packages aslong as we are inside the windowsize
    if(PList == NULL || ((PTail->header->seq-PList->header->seq) + 1) < wsize)
    {
      rtp *Header = (rtp*)malloc(sizeof(rtp));
      if(!Header)
      {
        printf("Could not allocate memory in Readmessage \n");
        exit(EXIT_FAILURE);
      }
      printf("\n>");

      strcpy(Header->data ,"Hello\0");
      Header->flags = DATA;
      Header->id = 1;
      Header->seq = seqnr;
      Header->windowsize = wsize;
      Header->crc = 0;

      writeMessage(socket, *Header, size);
      createHeader(Header);
      printf("Package %d was sent to the server at timestamp %ld!\n", Header->seq, time(0));
      seqnr++;
      if(seqnr == 10)
      {
        sleep(20);
        printf("\n[Client shutting down...]\n");
        Teardown(socket, size);
        return(NULL);
      }

    }
  }
}
void *Slidingwindow(void *data)
{
  int fileDescriptor = (int)(*(int*)data);
  rtp *RHeader;

  for(;;)
  { 
    //Gives the set, zero bits for all file desctiptors
    FD_ZERO(&set);
    //Sets all bits of sock in set 
    FD_SET(fileDescriptor, &set);
    free(RHeader);
    socklen_t size = sizeof(struct sockaddr_in);
    int nOfBytes;

    RHeader = (rtp*)malloc(sizeof(rtp));
    if(!RHeader)
    {
       printf("Could not allocate memory in Slidingw \n");
       exit(EXIT_FAILURE);
    }

    if(select(sizeof(int), &set, NULL, NULL, NULL))
    {
      nOfBytes = recvfrom(fileDescriptor, RHeader, sizeof(rtp), 0, (struct sockaddr*) &serverName, &size);
      if(nOfBytes < 0) 
      {
        free(RHeader);
        perror("Could not read data from client\n");
        exit(EXIT_FAILURE);
      }

      if(nOfBytes == 0) 
        /* End of file */
        pthread_exit(NULL);

      if(RHeader != NULL)
      {
        if(RHeader->seq == 9)
        {
          printf("Last ACK on package %d received, package was surely transmitted.\n", PList->header->seq);
          removeHead();
          printf("\n");
          pthread_exit(NULL);
        }
        if((RHeader->seq == PList->header->seq && RHeader->flags == ACK ) || (RHeader->seq > PList->header->seq && RHeader->flags == ACK))
        {
          //Remove all packages that has gotten an ACK. Handles ACK bigger then the expected.
          while(PList != NULL && PList->header->seq <= RHeader->seq)
          {
            printf("ACK on package %d received, package was surely transmitted.\n", PList->header->seq);
            removeHead();
          }
        }
        else
        {
          //Wrong ACK/Drop
          printf("Package out of order! Dropping package.\n");
          free(RHeader);
        }
      }
      else
      {
        free(RHeader);
        printf("Received Empty Header.");
      }
    }
  } 
  
}
/* readMessage
 * Reads and prints data read from the file (socket
 * denoted by the file descriptor 'fileDescriptor'.
 */
int readMessage(int fileDescriptor, socklen_t size) 
{
  //Gives the set, zero bits for all file desctiptors
  FD_ZERO(&set);
  //Sets all bits of sock in set 
  FD_SET(fileDescriptor, &set);

  rtp *RHeader = (rtp*)malloc(sizeof(rtp));
  if(!RHeader)
  {
    printf("Could not allocate memory in Readmessage \n");
   exit(EXIT_FAILURE);
  }
   int nOfBytes;
  struct timeval timeout;

  size = sizeof(struct sockaddr_in);

  timeout.tv_sec = 5;
  timeout.tv_usec = 0;

  if(select(sizeof(int), &set, NULL, NULL, &timeout))
  {
    nOfBytes = recvfrom(fileDescriptor, RHeader, sizeof(rtp), 0, (struct sockaddr*) &serverName, &size);

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
            if(RHeader->flags == got_synack)
            {
              printf("Received SYN + ACK from server at timestamp %ld!\n", time(0));

              //If we get an ack or a synack
              return RHeader->flags;
            }
            if(RHeader->flags == got_finack)
            {
              printf("Received FIN + ACK from server at timestamp %ld!\n", time(0));
              return RHeader->flags;
            }
            //Checks the order of a package
            bool check = checkOrder(RHeader);

            if((check == true && RHeader->flags == ACK ) || (RHeader->seq > PList->header->seq && RHeader->flags == ACK))
            {
              printf("%s\n", RHeader->data);
              printf("%d\n", RHeader->flags);
              printf("%d\n", RHeader->id);
              printf("%d\n", RHeader->seq);
              printf("%d\n", RHeader->windowsize);
              printf("%d\n", RHeader->crc);
              //Remove all packages that has gotten an ACK. Handles ACK bigger then the expected.
              while(PList->header->seq <= RHeader->seq)
              {
                printf("ACK on package %d received, package was surely transmitted.\n", PList->header->seq);
                removeHead();
              }
            }
            else
            {
              //Wrong ACK/Drop
              printf("Package out of order! Dropping package.\n");
              free(RHeader);
            }
          }
          else
          {
            free(RHeader);
            printf("Received Empty Header.");
          }
      }
      else
      {
        //We have reached the timeout and we are connected.
        printf("Timeout reached on select!\n");
        state = ESTABLISHED;
        return ESTABLISHED;
      }
      
  return(0);
}
//Listen to incoming messages
void* ListenToMessages(int socket, socklen_t size)
{
  while(1)
  {
    readMessage(socket, size);
  }
}


/*ConnectionSetup
Function which handles the connection setup*/
void ConnectionSetup(int fileDescriptor, socklen_t size)
{ 
  state = WAIT_SYNACK;

  int event = INIT;
  //Send a SYN to the server.
  Header.flags = send_syn; 
  Header.id = 0;
  Header.seq = -1;
  Header.windowsize = wsize;
  Header.crc = 0;
 
  writeMessage(fileDescriptor, Header, size);
  createHeader(&Header);
  printf("SYN sent to the server at timestamp %ld!\n", time(0));
  state = WAIT_SYNACK;

  while(loop == true)
  {
    //Reads events from incoming packages. 
    event = readMessage(fileDescriptor, size); //Event received package information
    //Switches the current state of our connectionsetup. 
    switch(state)
    {
      case WAIT_SYNACK:
      {
        if(event == got_synack)
        {
          //
          event = WAIT_TIMEOUT;
          removeHead();
          //Skicka ack
          event = INIT;
          Header.flags = send_ack; 
          writeMessage(fileDescriptor, Header, size);
          printf("ACK sent to the server at timestamp %ld!\n", time(0));
          createHeader(&Header);
          //Timer skicka om när de är slut.
        }
        break;
      }
      case WAIT_TIMEOUT:
      {
        if(event == got_synack)
        {
          //If we got a syn + ack before timeout. Means that ACK got lost.
          printf("Wait timeout but got SYN + ACK\n");
          removeHead();
          Header.flags = send_ack; 
          writeMessage(fileDescriptor, Header, size);
          printf("ACK re-sent to the server at timestamp %ld!\n", time(0));
          createHeader(&Header);
        }
        break;
      }
      case ESTABLISHED:
      {
        removeHead();
        return;
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
/*checkTimeout-function.
Checks if the first package in the list has exceeded the timer. That will say if it took more then
11000 seconds for the ACK.*/
void *checkTimeout(void *data)
{
  int size = sizeof(struct sockaddr_in);
  int fileDescriptor = (int)(*(int*)data);
  while(1)
  {
    if(PList != NULL)
    {
      if(time(0) > (PList->timestamp+30))
      {
        if(PList->header->flags == send_ack)
        {
          loop = false;
          printf("Timer reached\n");
          printf("Connection established\n");
          removeHead();
          PList = NULL;
           // pthread_exit(NULL);
        }
        else
        {
          printf("Timeout breached on package %d.\n", PList->header->seq);
          printf("[Resending packages...]\n");

          Packagelist *current = PList;
          while(current != NULL)
          {
            writeMessage(fileDescriptor, (*current->header), size);
            current->timestamp = time(0);
            printf("Package %d was sent to the server at timestamp %ld!\n", current->header->seq, time(0));

            if(current->next != NULL)
              current = current->next;
            else
              break;
          }
          
        }
      }
    }
  }
}

void Teardown(int fileDescriptor, socklen_t size)
{
  int event = INIT;
  //Send a SYN to the server.
  Header.flags = send_fin; 
  Header.id = 0;
  Header.seq = -1;
  Header.windowsize = 1;
  Header.crc = 0;
 
  writeMessage(fileDescriptor, Header, size);
  createHeader(&Header);
  printf("FIN sent to the server at timestamp %ld!\n", time(0));
  state = WAIT_FINACK;

  while(loop == true)
  {
    //Reads events from incoming packages. 
    event = readMessage(fileDescriptor, size); //Event received package information
    //Switches the current state of our connectionsetup. 
    switch(state)
    {
      case WAIT_FINACK:
      {
        if(event == got_finack)
        {
          event = WAIT_TIMEOUT;
          removeHead();
          //Skicka ack
          event = INIT;
          Header.flags = send_ack; 
          writeMessage(fileDescriptor, Header, size);
          printf("ACK sent to the server at timestamp %ld!\n", time(0));
          createHeader(&Header);
          //Timer skicka om när de är slut.
        }
        break;
      }
      case WAIT_TIMEOUT:
      {
        if(event == got_finack)
        {
          //If we got a syn + ack before timeout. Means that ACK got lost.
          printf("Wait timeout but got FIN + ACK\n");
          removeHead();
          Header.flags = send_ack; 
          writeMessage(fileDescriptor, Header, size);
          printf("ACK re-sent to the server at timestamp %ld!\n", time(0));
          createHeader(&Header);
        }
        break;
      }
      case ESTABLISHED:
      {
        printf("Client is shutted down\n");
        removeHead();
        return;
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
int main(int argc, char *argv[]) {
  int sock;
  char hostName[hostNameLength];
  socklen_t size;
  char input[10];
  pthread_t thread, thread2;
  int eid1, eid2;

  //Gives the set, zero bits for all file desctiptors
  FD_ZERO(&set);
  //Sets all bits of sock in set 
  FD_SET(sock, &set);

  size = sizeof(struct sockaddr_in);
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
  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(sock < 0) {
    perror("Could not create a socket\n");
    exit(EXIT_FAILURE);
  }
  /* Initialize the socket address */
  initSocketAddress(&serverName, hostName, PORT);
  
  /*Starting connection setup*/
  printf("Hello Client! Do you want to connect to the server? y/n \n");
  fflush(stdin);
  fgets(input, 10, stdin);

  if(strcmp(input,"y\n") == 0)
  {
    printf("\n[Connecting...]\n");
    /*Connection setup here*/
    eid1 = pthread_create(&thread, NULL, checkTimeout, &sock);
    if(eid1)
    {
      printf("ERROR return code from threadcreate is %d\n", eid1);
      exit(-1);
    }
    ConnectionSetup(sock, size);
    printf("Connection up!\n");
    /*Creating a thread running SendMessage-function and sending socket with it.*/

    eid2 = pthread_create(&thread2, NULL, Slidingwindow, &sock);
    if(eid2)
    {
      printf("ERROR return code from threadcreate is %d\n", eid1);
      exit(-1);
    }
    SendMessage(sock, size);
  }
  else
  {
    printf("Byebye!\n");
    return(0);
  }

}
