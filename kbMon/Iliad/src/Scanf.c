
#include <ntddk.h>
#include <wdf.h>
#include <Wdm.h>
#include "Public.h"

#define INVALID 0X00 
#define SPACE 0X01 
#define ENTER 0X02 
#define LSHIFT 0x03 
#define RSHIFT 0x04 
#define CTRL  0x05 
#define ALT	  0x06 

char KeyMap[84] = {
	INVALID, //0
	INVALID, //1
	'1', //2
	'2', //3
	'3', //4
	'4', //5
	'5', //6
	'6', //7
	'7', //8
	'8', //9
	'9', //A
	'0', //B
	'-', //C
	'=', //D
	INVALID, //E
	INVALID, //F
	'q', //10
	'w', //11
	'e', //12
	'r', //13
	't', //14
	'y', //15
	'u', //16
	'i', //17
	'o', //18
	'p', //19
	'[', //1A
	']', //1B
	ENTER, //1C
	CTRL, //1D
	'a', //1E
	's', //1F
	'd', //20
	'f', //21
	'g', //22
	'h', //23
	'j', //24
	'k', //25
	'l', //26
	';', //27
	'\'', //28
	'`', //29
	LSHIFT,	//2A
	'\\', //2B
	'z', //2C
	'x', //2D
	'c', //2E
	'v', //2F
	'b', //30
	'n', //31
	'm' , //32
	',', //33
	'.', //34
	'/', //35
	RSHIFT, //36
	INVALID, //37
	ALT, //38
	SPACE, //39
	INVALID, //3A
	INVALID, //3B
	INVALID, //3C
	INVALID, //3D
	INVALID, //3E
	INVALID, //3F
	INVALID, //40
	INVALID, //41
	INVALID, //42
	INVALID, //43
	INVALID, //44
	INVALID, //45
	INVALID, //46
	'7', //47
	'8', //48
	'9', //49
	INVALID, //4A
	'4', //4B
	'5', //4C
	'6', //4D
	INVALID, //4E
	'1', //4F
	'2', //50
	'3', //51
	'0', //52
};


char ExtendedKeyMap[84] = {
	INVALID, //0
	INVALID, //1
	'!', //2
	'@', //3
	'#', //4
	'$', //5
	'%', //6
	'^', //7
	'&', //8
	'*', //9
	'(', //A
	')', //B
	'_', //C
	'+', //D
	INVALID, //E
	INVALID, //F
	'Q', //10
	'W', //11
	'E', //12
	'R', //13
	'T', //14
	'Y', //15
	'U', //16
	'I', //17
	'O', //18
	'P', //19
	'{', //1A
	'}', //1B
	ENTER, //1C
	INVALID, //1D
	'A', //1E
	'S', //1F
	'D', //20
	'F', //21
	'G', //22
	'H', //23
	'J', //24
	'K', //25
	'L', //26
	':', //27
	'"', //28
	'~', //29
	LSHIFT,	//2A
	'|', //2B
	'Z', //2C
	'X', //2D
	'C', //2E
	'V', //2F
	'B', //30
	'N', //31
	'M' , //32
	'<', //33
	'>', //34
	'?', //35
	RSHIFT, //36
	INVALID, //37
	INVALID, //38
	SPACE, //39
	INVALID, //3A
	INVALID, //3B
	INVALID, //3C
	INVALID, //3D
	INVALID, //3E
	INVALID, //3F
	INVALID, //40
	INVALID, //41
	INVALID, //42
	INVALID, //43
	INVALID, //44
	INVALID, //45
	INVALID, //46
	'7', //47
	'8', //48
	'9', //49
	INVALID, //4A
	'4', //4B
	'5', //4C
	'6', //4D
	INVALID, //4E
	'1', //4F
	'2', //50
	'3', //51
	'0', //52
};


VOID 
ConvertScanCodeToKeyCode(
	_In_ PDEVICE_EXTENSION pDevExt,
	_In_ KEY_DATA* kData,
	_In_ char* keys
)
{

	char key = 0;
	key = KeyMap[kData->KeyData];
	KEVENT event = { 0 };
	KEYBOARD_INDICATOR_PARAMETERS indParams = { 0 };
	IO_STATUS_BLOCK ioStatus = { 0 };
	NTSTATUS status = { 0 };
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	PIRP irp = IoBuildDeviceIoControlRequest(IOCTL_KEYBOARD_QUERY_INDICATORS, pDevExt->pKeyboardDevice,
		NULL, 0, &indParams, sizeof(KEYBOARD_ATTRIBUTES), TRUE, &event, &ioStatus);
	status = IoCallDriver(pDevExt->pKeyboardDevice, irp);

	if (status == STATUS_PENDING)
	{
		(VOID)KeWaitForSingleObject(&event, Suspended, KernelMode,
			FALSE, NULL);
	}

	status = irp->IoStatus.Status;

	if (status == STATUS_SUCCESS)
	{
		indParams = *(PKEYBOARD_INDICATOR_PARAMETERS)irp->AssociatedIrp.SystemBuffer;
		if (irp)
		{
			int flag = (indParams.LedFlags & KEYBOARD_CAPS_LOCK_ON);
#ifdef DEBUG
			DbgPrint("Caps Lock Indicator Status: %x.\n", flag);
#endif 	
		}
		
#ifdef DEBUG
		else
			DbgPrint("Error allocating Irp");
#endif
	}

	switch (key)
	{
	case LSHIFT:
		if (kData->KeyFlags == KEY_MAKE)
			pDevExt->kState.kSHIFT = TRUE;
		else
			pDevExt->kState.kSHIFT = FALSE;
		break;

	case RSHIFT:
		if (kData->KeyFlags == KEY_MAKE)
			pDevExt->kState.kSHIFT = TRUE;
		else
			pDevExt->kState.kSHIFT = FALSE;
		break;
	case CTRL:
		if (kData->KeyFlags == KEY_MAKE)
			pDevExt->kState.kCTRL = TRUE;
		else
			pDevExt->kState.kCTRL = FALSE;
		break;
	case ALT:
		if (kData->KeyFlags == KEY_MAKE)
			pDevExt->kState.kALT = TRUE;
		else
			pDevExt->kState.kALT = FALSE;
		break;
	case SPACE:
		if ((pDevExt->kState.kALT != TRUE) && (kData->KeyFlags == KEY_BREAK)) 
			keys[0] = 0x20;				
		break;
	case ENTER:
		if ((pDevExt->kState.kALT != FALSE) && (kData->KeyFlags == KEY_BREAK)) 
		{								 
			keys[0] = 0x0D;				
			keys[1] = 0x0A;
		}
		break;

	default:
		if ((pDevExt->kState.kALT != TRUE) && (pDevExt->kState.kCTRL != TRUE) &&
			(kData->KeyFlags == KEY_BREAK)) 
		{
			if ((key >= 0x21) && (key <= 0x7E)) 
			{
				if (pDevExt->kState.kSHIFT == TRUE)
					keys[0] = ExtendedKeyMap[kData->KeyData];
				else
					keys[0] = key;
			}
		}
		break;
	}
}
