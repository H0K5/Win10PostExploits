

#include "driver.h"
#include "device.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, absCreateDevice)
#endif

NTSTATUS
absCreateDevice(
	_Inout_ PWDFDEVICE_INIT DeviceInit
)

{
	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	PDEVICE_CONTEXT deviceContext;
	WDFDEVICE device;
	NTSTATUS status;

	PAGED_CODE();

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

	status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

	if (NT_SUCCESS(status)) {

		deviceContext = DeviceGetContext(device);

		deviceContext->PrivateDeviceData = 0;


		status = WdfDeviceCreateDeviceInterface(
			device,
			&GUID_DEVINTERFACE_abs,
			NULL
		);

		if (NT_SUCCESS(status)) {
			status = absQueueInitialize(device);
		}
	}

	return status;
}