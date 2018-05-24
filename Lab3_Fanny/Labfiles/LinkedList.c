
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "LinkedList.h"
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h> 

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
/*printPackageList function
Prints Seqnr of everything in the packagelist*/
void printPackagelist()
{
	Packagelist* head = PList;

   	while (head != NULL) 
   	{
        printf("Seq: %d\n", head->header->seq);
        head = head->next;
    }
}
/*Checksum function
What it does is to divide the data of a package into a 16bit integer. 
If this integer is bigger then 0xffff(which means 16bit) it will subtract it with 0xffff. 
When the summation is done it will compute its bitwise complement and return it in network byte order. */
uint16_t checksum(void* indata,size_t datalength) 
{
    char* data = (char*)indata;

    // Initialise the sum.
    uint32_t sum = 0xffff;

    //Adds the 16bit blocks and sumates it to sum variable. 
    for (size_t i = 0; i+1 < datalength; i += 2) 
    {
        uint16_t word;
        memcpy(&word, data+i, 2);

        sum += ntohs(word);

        //If the sum variable is bigger then 16bits(max) it will subtract it by 16bits. 
        if (sum > 0xffff) 
            sum -= 0xffff;
    }

    // Handle any partial block at the end of the data.
    //If the data summation is not evenly divideable we need to handle it.
    if (datalength & 1) 
    {
        uint16_t word = 0;
        memcpy(&word, data+datalength-1, 1);

        sum += ntohs(word);

        if (sum > 0xffff) 
            sum -= 0xffff;
    }

    // Return the checksum in network byte order and ~ Means that we do a bitwise complement of the sum. Ex. 1011 will be 0100.
    return htons(~sum);
}