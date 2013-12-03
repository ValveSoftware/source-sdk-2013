//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef OSVERSION_H
#define OSVERSION_H
#pragma once

#include "vstdlib/vstdlib.h"

// OS types we know about
// Must be in ascending capability order, we use this for min OS requirements
enum EOSType
{
	k_eOSUnknown = -1,
	k_eMacOSUnknown = -102,
	k_eMacOS104 = -101,
	k_eMacOS105 = -100,
	k_eMacOS1058 = -99,
	k_eMacOS106  = -95,
	k_eMacOS1063 = -94,
	k_eMacOS107  = -90,
	// k_eMacOSMax = -1
	k_eLinuxUnknown = -203,
	k_eLinux22 = -202,
	k_eLinux24 = -201,
	k_eLinux26 = -200,
	// k_eLinuxMax = -103
	k_eWinUnknown = 0,
	k_eWin311 = 1,
	k_eWin95,
	k_eWin98,
	k_eWinME,
	k_eWinNT,
	k_eWin2000,
	k_eWinXP,
	k_eWin2003,
	k_eWinVista,
	k_eWindows7,
	k_eWin2008,
	k_eWinMAX,
	k_eOSTypeMax = k_eWinMAX + 11 // win types + other ifdef'd types
};

VSTDLIB_INTERFACE const char *GetNameFromOSType( EOSType eOSType );
VSTDLIB_INTERFACE const char *GetOSDetailString( char *pchOutBuf, int cchOutBuf );
VSTDLIB_INTERFACE EOSType GetOSType();
VSTDLIB_INTERFACE bool OSTypesAreCompatible( EOSType eOSTypeDetected, EOSType eOSTypeRequired );
VSTDLIB_INTERFACE const char *GetPlatformName( bool *pbIs64Bit );

#endif // OSVERSION_H
