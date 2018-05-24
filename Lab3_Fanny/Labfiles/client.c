/* File: client.c */

#include "LinkedList.h"

#define hostNameLength 50

#define WAIT_SYNACK 1  
#define WAIT_FINACK 12

#define send_syn 6
#define got_synack 7
#define send_ack 8

#define send_fin 13
#define got_finack 14

/*packagelimit variable which can be changed if you want to send more packages.*/
#define packagelimit 10

void Teardown(int fileDescriptor, socklen_t size);

int state;
fd_set set;
int wsize = 2;
struct sockaddr_in serverName;
pthread_t thread2;

//Variable for Sent Headers.
rtp Header;

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
/*RandomError function
Randomizes either corrupt, lost or wrong order on packages.*/
void RandomError(rtp **Header, int sock, socklen_t size)
{

  int random=rand()%4;
  switch(random)
  {
      case 0:
          //Sends a normal package
          writeMessage(sock, **Header, size);
          break;
      case 1:
          //Makes a corrupted package
          printf("////Sends corrupt package////\n");
          rtp *WrongHeader = (rtp*)malloc(sizeof(rtp));
          strcpy(WrongHeader->data ,"Hello\0");
          WrongHeader->flags = DATA;
          WrongHeader->id = 1;
          WrongHeader->seq = (*Header)->seq;
          WrongHeader->windowsize = wsize;
          WrongHeader->crc = 1555;
          writeMessage(sock, *WrongHeader, size);
          break;
      case 2:
          //Makes and send a package with has seqnr 1. Probably wrong order/Lost package.
          printf("////Sends package with wrong seqnr//////\n");
          rtp *WrongHeader2 = (rtp*)malloc(sizeof(rtp));
          strcpy(WrongHeader2->data ,"Hello\0");
          WrongHeader2->flags = DATA;
          WrongHeader2->id = 1;
          WrongHeader2->seq = 1;
          WrongHeader2->windowsize = wsize;
          WrongHeader2->crc = checksum((void*)WrongHeader2, sizeof(*WrongHeader2));;
          writeMessage(sock, *WrongHeader2, size);
          break;
      case 3:
          //Sends a normal package
          writeMessage(sock, **Header, size);
          break;
      case 4:
          //Sends a normal package
          writeMessage(sock, **Header, size);
          break;
  }

}
/*SendMessage
Sending packages depending on the packagelimit. Also uses RandomError function to generate errors.*/
void SendMessage(int socket, socklen_t size)
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
      Header->crc = checksum((void*)Header, sizeof(*Header));
      createHeader(Header);

      RandomError(&Header, socket, size);

      printf("Package %d was sent to the server at timestamp %ld!\n", Header->seq, time(0));
      seqnr++;
      //Checks if we have sent all the packages(packagelimit).  
      if(seqnr == packagelimit)
      {
        //Waits for the thread which handles the sliding window is done. So that we can start teardown. 
        //which means last ACK was received then sliding window thread will terminate.
        pthread_join(thread2, NULL);
        if(PList == NULL)
        {
          printf("\n[Client shutting down...]\n");
          Teardown(socket, size);
          return;
        }  
      }

    }
  }
}
/*Slidingwindow function
Receives ACK's and checks if they are in the right order and not corrupted */
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
        //Here we will check if the checksum is correct, that will say package hasnt been corrupted.
        int Oldchecksum = RHeader->crc; 
        RHeader->crc = 0;
        int Newchecksum = checksum((void*) RHeader, sizeof(*RHeader));

        if(Oldchecksum == Newchecksum)
        {
          //If its the last package in the limit, -1 because it starts on 0.
          if(RHeader->seq == packagelimit-1)
          {
            printf("Last ACK on package %d received, package was surely transmitted.\n", PList->header->seq);
            removeHead();
            return (NULL);
          }
          else
          {
            //Checks if ACK is in the right order and flag is ACK. 
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
              printf("Package out of order! Dropping package.\n");
              free(RHeader);
            }
          }
        }
        else
        {
          printf("Corrupted package!\n");
          printf("[Dropping package...]\n");
          free(RHeader);
          RHeader = NULL;
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
 * Reads and prints data read from the socket and returns values to connection and teardown functions. 
 */
int readMessage(int fileDescriptor, socklen_t size) 
{
  //Gives the set, zero bits for all file desctiptors
  FD_ZERO(&set);
  //Sets all bits of sock in set 
  FD_SET(fileDescriptor, &set);

  int nOfBytes;
  struct timeval timeout;

  size = sizeof(struct sockaddr_in);

  timeout.tv_sec = 5;
  timeout.tv_usec = 0;

  rtp *RHeader = (rtp*)malloc(sizeof(rtp));
  if(!RHeader)
  {
    printf("Could not allocate memory in Readmessage \n");
    exit(EXIT_FAILURE);
  }

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
            int Oldchecksum = RHeader->crc; 
            RHeader->crc = 0;
            int Newchecksum = checksum((void*) RHeader, sizeof(*RHeader));

            if(Newchecksum == Oldchecksum)
            {
              //If we got syn + ack 
              if(RHeader->flags == got_synack)
              {
                printf("Received SYN + ACK from server at timestamp %ld!\n", time(0));

                return RHeader->flags;
              }
              //If we got fin + ack
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
              printf("Corrupted package!\n");
              printf("[Dropping package...]\n");
              free(RHeader);
              RHeader = NULL;
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
        
        printf("Timeout reached on select!\n");
        
        if(state == WAIT_TIMEOUT)
        {
          //We have reached the timeout and we are connected.
          state = ESTABLISHED;
          return ESTABLISHED;
        }

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
  strcpy(Header.data ,"Hello\0");
  Header.flags = send_syn; 
  Header.id = 0;
  Header.seq = -1;
  Header.windowsize = wsize;
  Header.crc = 0;
  Header.crc = checksum((void*) &Header, sizeof(Header));

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
          //If we got syn+ack and want to send ack
          state = WAIT_TIMEOUT;
          removeHead();
          event = INIT;
          Header.flags = send_ack; 
          Header.crc = 0;
          Header.crc = checksum((void*) &Header, sizeof(Header));
        
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
          Header.crc = 0;
          Header.crc = checksum((void*) &Header, sizeof(Header));
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
10 seconds for the ACK.*/
void *checkTimeout(void *data)
{
  int size = sizeof(struct sockaddr_in);
  int fileDescriptor = (int)(*(int*)data);

  while(1)
  {
    if(PList != NULL)
    {
      //If the time right now is greater then the timestamp on the package +10 seconds.
      if(time(0) > (PList->timestamp+10))
      {
        if(PList->header->flags == send_ack)
        {
          loop = false;
          printf("Timer reached\n");
          printf("Connection established\n");
          removeHead();
          PList = NULL;
        }
        else
        {
          printf("Timeout breached on package %d.\n", PList->header->seq);
          printf("[Resending packages...]\n");

          Packagelist *current = PList;
          //Resends all packages after last ACK
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
/*Teardown
Function which handles the teardown*/
void Teardown(int fileDescriptor, socklen_t size)
{
  int event = INIT;
  //Send a SYN to the server.
  Header.flags = send_fin; 
  Header.id = 0;
  Header.seq = -1;
  Header.windowsize = 1;
  Header.crc = 0;
  Header.crc = checksum((void*) &Header, sizeof(Header));
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
          //if we got fin + ack
          state = WAIT_TIMEOUT;
          removeHead();
          event = INIT;
          Header.flags = send_ack; 
          Header.crc = 0;
          Header.crc = checksum((void*) &Header, sizeof(Header));
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
          Header.crc = 0;
          Header.crc = checksum((void*) &Header, sizeof(Header));
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
  pthread_t thread;
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
    close(sock);
  }
  else
  {
    printf("Byebye!\n");
    return(0);
  }

}
