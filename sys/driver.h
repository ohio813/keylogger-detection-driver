/*
 * Module Name: obdriver.h
 * Author: Christopher Wood
 * Description: Outbound driver header.
 * Environment: Kernel mode only
 */

#ifndef OBDRIVER_H_
#define OBDRIVER_H_

#include "Ntifs.h"

#define DEBUG 1
#define VERSION "0.2"
#define NAME "OUTBOUND"

#define BUFFER_QUEUE_SIZE 128
#define WRITE_QUEUE_SIZE 128

/* Helper boolean */
typedef enum{false, true} bool;

/* LIST_ENTRY queue to maintain string buffers */
typedef struct _KEY_QUEUE
{
	KSEMAPHORE semQueue; // see docs. on how to use properly
	KSPIN_LOCK lockQueue; // see docs. on how to use properly
	LIST_ENTRY QueueListHead;
} KEY_QUEUE, *PKEY_QUEUE;

/* Key data struct to maintain data for a keystroke */
typedef struct _KEY_DATA
{
	LIST_ENTRY ListEntry;
	char KeyData;
	char KeyFlags;
	char Data;

	//TODO: add scancode here
}KEY_DATA, *PKEY_DATA;

/* Keyboard device driver extension struct */
typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT pKeyboardDevice;
	KEY_QUEUE bufferQueue;
	bool bExtractTerminate;
	bool bWorkerTerminate;
	bool bCompareTerminate;
	PETHREAD pExtractThread;
	PETHREAD pWorkerThread;
	PETHREAD pCompareThread;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/* External number of pending IRPs, maintained in obhook.c */
extern numPendingIrps;

/* Driver routine functions */
DRIVER_DISPATCH OutboundDispatchPassDown;
DRIVER_DISPATCH OutboundDispatchRead;
DRIVER_UNLOAD   Unload;

/* Prototypes */
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
OutboundDispatchPassDown(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    );

VOID
Unload(IN PDRIVER_OBJECT DriverObject);

#endif // OBDRIVER_H
