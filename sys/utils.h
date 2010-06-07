/*
 * Module: obutils.h
 * Author: Christopher Wood
 * Description: Outbound utility file header.
 * Environment: Kernel mode only
 */
 
 #ifndef OBUTILS_H_
 #define OBUTILS_H_
 
 /* Function prototypes */
 
 /*
  * Traverse a linked LIST_ENTRY list consisting of KEY_QUEUE structs
  *
  * @param queue - pointer to the head struct in the list
  */
 void traverseQueue(KEY_QUEUE *queue);
 
 #endif