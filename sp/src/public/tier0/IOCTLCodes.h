//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef IOCTLCODES_H
#define IOCTLCODES_H
#pragma once

// Define the IOCTL codes we will use.  The IOCTL code contains a command
// identifier, plus other information about the device, the type of access
// with which the file must have been opened, and the type of buffering.

// Device type           -- in the "User Defined" range."
#define DEVICE_FILE_TYPE 40000


// The IOCTL function codes from 0x800 to 0xFFF are for customer use.

#define IOCTL_WRITE_MSR \
    CTL_CODE( DEVICE_FILE_TYPE, 0x900, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_READ_MSR \
    CTL_CODE( DEVICE_FILE_TYPE, 0x901, METHOD_BUFFERED, FILE_READ_ACCESS )


#endif IOCTLCODES_H
