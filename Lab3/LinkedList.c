
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "LinkedList.h"
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h> 

#define INIT 0 
#define WAIT_SYN 1  
#define WAIT_SYNACK 2
#define TIMEOUT 3
#define ERR 4
#define ESTABLISHED 5

Packagelist *PList = NULL;
Packagelist *PTail = NULL;
bool loop = true;

void addHeader(rtp *Header)
{
	Packagelist *current = PList;

	while (current != NULL)
	{
		if (current->next == NULL)
		{
			current->next = malloc(sizeof(Packagelist));
			current->next->header = Header;
			current->next->timestamp = time(0);
			current->next->next = NULL;

			PTail = current->next;
			//printf("Header was added to List\n");
			return;
		}
		current = current->next;
	}
}
/*createHeader-function
If its an empty list of headers just allocate memory for one. 
If its not call addHeader which finds an empty spot for a new.*/
void createHeader(rtp *Header)
{

	if (PList == NULL)
	{
		PList = malloc(sizeof(Packagelist));
		PList->header = Header;
		PList->timestamp = time(0);
		PList->next = NULL;
		PTail = (PList);
		//printf("Header was added to List\n");
		return;
	}
	else
	{
		addHeader(Header);
	}
}
/*removeHead-function
Since we always remove the first package in the list(Last package sent) we only
need this function for removing.*/
void removeHead()
{
	if (PList == NULL)
	{
		printf("Nothing to remove in list.");
		return;
	}
	if(PList->next == NULL)
	{
		free(PList);
		PList = NULL;
		PTail = NULL;
		return;
	}

	Packagelist* tmp = PList;

	PList = PList->next; 

	free(tmp);
}
/*peekFrontTimestamp-functon.
Peeks the first package of the lists timestamp.*/
long double peekFrontTimestamp()
{
	return PList->timestamp;
}
/*peekFrontSeqnr-functon.
Peeks the first package of the lists sequence number.*/
int peekFrontSeqnr()
{
	return PList->header->seq;
}

/*checkOrder-function
Checks if the order of a incoming package(header) is correct. Is done by
comparing the incoming Sequence number with the first package sequence number + the windowsize. 
-1 since it starts with 0.*/
bool checkOrder(rtp *Header)
{
	if(Header->seq == -1)
	{
		return true;
	}
	else if(Header->seq == 0)
	{
		return true;
	}

	Packagelist *current = PList;

	
	while (current != NULL)
	{
		if (current->next == NULL)
		{
			if(Header->seq == (current->header->seq+1))
			{
				return true;
			}
		}
	}
	
	return false;
}
void printPackagelist()
{
	Packagelist* head = PList;

   	while (head != NULL) 
   	{
        printf("Seq: %d\n", head->header->seq);
        head = head->next;
    }
}
