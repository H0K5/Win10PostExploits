

#include "driver.h"
#include "queue.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, absQueueInitialize)
#endif

NTSTATUS
absQueueInitialize(
	_In_ WDFDEVICE Device
)

{
	WDFQUEUE queue;
	NTSTATUS status;
	WDF_IO_QUEUE_CONFIG queueConfig;

	PAGED_CODE();

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
		&queueConfig,
		WdfIoQueueDispatchParallel
	);

	queueConfig.EvtIoDeviceControl = absEvtIoDeviceControl;
	queueConfig.EvtIoStop = absEvtIoStop;

	status = WdfIoQueueCreate(
		Device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&queue
	);

	return status;
}

VOID
absEvtIoDeviceControl(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
)

{

	WdfRequestComplete(Request, STATUS_SUCCESS);

	return;
}

VOID
absEvtIoStop(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG ActionFlags
)

{


	return;
}