
#ifndef LINKEDLIST_H
#define LINKEDLIST_H
#include <time.h>
#include <stdbool.h>
#define MAXMSG 512
//#include <openssl/md5.h>

typedef struct
{ 
  int flags; 
  int id;
  int seq; 
  int windowsize;
  int crc;
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
long double peekFrontTimestamp();
int peekFrontSeqnr();
bool checkOrder(rtp *Header);
void printPackagelist();
#endif 