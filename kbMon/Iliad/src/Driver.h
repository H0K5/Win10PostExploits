

#define INITGUID

#include <ntddk.h>
#include <wdf.h>

#include "device.h"
#include "queue.h"
#include "trace.h"

EXTERN_C_START

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD absEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP absEvtDriverContextCleanup;

EXTERN_C_END
