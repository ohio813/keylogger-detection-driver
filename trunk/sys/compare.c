// compare.c

#include <stdio.h>
#include <string.h>

#include "driver.h"
#include "compare.h"
#include "thread.h"

#define CUSHION 5 // should be larger for more accurate detection

char *bufferArray;
char *writeArray;
int bufferIndex;
int writeIndex;
int bufferSize;
int writeSize;

// called from driverentry routine
void initializeCompariosonComponent(int bSize, int wSize)
{
    // allocate memory for buffer and write arrays
    bufferIndex = writeIndex = 0;
    bufferSize = bSize;
    writeSize = wSize;
    return;
}

void enqueueData(char data, char target)
{
    if (target == 'b')
    {
        // enqueue in buffer array
    }
    else if (target == 'w')
    {
        // enqueue in write array
    }

    return;
}

// brute force pattern matching algorithm, not efficient for this purpose
int bruteMatch(char *pI, char *pM, int targetLength, int sourceLength)
{
    int tPos = 0; //not used - needed?
    int sPos = 0; //not used - needed?
    int tTemp = 0;
    int sTemp = 0;
    int counter = 0;
    bool foundMatch = false;

    // continue searching...
    do{
        for (;tTemp < targetLength;) // turn into a while?
        {
            // comparing two chars
            if (*(pI+tTemp) != *(pM+sTemp))
            {
                // jumping up in target
                tTemp++;
            }
            else
            {
                // match found
                foundMatch = true;
                break;
                // or goto Compare;
            }
        }
        if (!foundMatch) // target in source didn't match any chars in the target, so take a step in source
        {
            sTemp++;
            tTemp = 0;
            // jump up in source
        }
        else // a match was found and we jumped out of loop and landed here, compare some sheet
        {
            // set the tPos and sPos
            tPos = tTemp;
            sPos = sTemp;

            // loop through until the chars don't match anymore
            counter = 0;
            //numMatches = 0;
            while (*(pI+tTemp+counter) == *(pM+sTemp+counter))
            {
                //numMatches++;
                counter++;
                // if (numMatches > cushion)
                // {
                    // // matches exceed cushion - likely pattern emerged
                // }
            }

            // reset search vars
            tTemp = 0;
            sTemp = sPos + 1;
        }
    }while (sTemp < sourceLength); // only go through this comparison until the source has been fully checked with the target...
}
