/*
 * Module: obcompare.h
 * Author: Christopher Wood
 * Description: String comparison header file
 * Environment: Kernel mode only
 */

#ifndef OBCOMPARE_H_
#define OBCOMPARE_H_

#include "driver.h"

/* Prototypes */
int bruteMatch(char *pI, char *pM, int targetLength, int sourceLength);
bool recursiveMatch(char *pTarget, char *pSource, int targetLength, int sourceLength, int targetIndex, int sourceIndex, int matches);
void enqueueData(char data, char target);
void initializeCompariosonComponent(int bSize, int wSize);

#endif // OBCOMPARE_H_
