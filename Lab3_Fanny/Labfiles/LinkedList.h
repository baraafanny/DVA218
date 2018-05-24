
#ifndef LINKEDLIST_H
#define LINKEDLIST_H
#include <time.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <pthread.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#define PORT 5555
#define MAXMSG 50

#define INIT 0 
#define WAIT_ACK 2
#define ESTABLISHED 5
#define WAIT_TIMEOUT 3


#define DATA 10
#define ACK 11
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct
{ 
  int flags; 
  int id;
  int seq; 
  int windowsize;
  uint16_t crc;
  char data[MAXMSG];
}rtp;

//Linked list containing sent packages and their header. 
//Timestamp to handle timeouts. 
typedef struct pl
{
  rtp* header;
  time_t timestamp;
  struct pl *next;

}Packagelist;

//Pointer to our linkedlist of packages
extern Packagelist *PList;
extern Packagelist *PTail;
extern bool loop;

void addHeader(rtp *Header);
void createHeader(rtp *Header);
void removeHead();
bool checkOrder(rtp *Header);
void printPackagelist();
uint16_t checksum(void* indata,size_t datalength);
#endif 