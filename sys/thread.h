/*
 * Module: obthread.h
 * Author: Christopher Wood
 * Description: Worker thread file header.
 * Environment: Kernel mode only
 */
 
 #ifndef OBTHREAD_H_
 #define OBTHREAD_H_
 
 #include "Ntifs.h"
 
/* Static Driver Verifier */
KSTART_ROUTINE ExtractThread;
KSTART_ROUTINE WorkerThread;
 
/* Prototypes */
VOID ExtractThread(IN PVOID pContext); // context = deviceextension struct pointer
VOID WorkerThread(IN PVOID pContext);  // context = writequeue struct pointer
NTSTATUS InitThreads(IN PDRIVER_OBJECT pDriverObject, IN KEY_QUEUE *writeQueue);
 
 #endif
 