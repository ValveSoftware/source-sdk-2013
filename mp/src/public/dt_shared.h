//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef DATATABLE_SHARED_H
#define DATATABLE_SHARED_H

#ifdef _WIN32
#pragma once
#endif

#include "dt_common.h"

// ------------------------------------------------------------------------ //
// Client version
// ------------------------------------------------------------------------ //

#if defined (CLIENT_DLL)

#include "dt_recv.h"

#define PROPINFO(varName)							RECVINFO(varName)						
#define PROPINFO_DT(varName)						RECVINFO_DT(varName)					
#define PROPINFO_DT_NAME(varName, remoteVarName)	RECVINFO_DTNAME(varName,remoteVarName)	
#define PROPINFO_NAME(varName,remoteVarName)		RECVINFO_NAME(varName, remoteVarName)	

#define DataTableProp	RecvProp

#endif

// ------------------------------------------------------------------------ //
// Server version
// ------------------------------------------------------------------------ //

#if !defined (CLIENT_DLL)

#include "dt_send.h"

#define PROPINFO(varName)							SENDINFO(varName)			
#define PROPINFO_DT(varName)						SENDINFO_DT(varName)		
#define PROPINFO_DT_NAME(varName, remoteVarName)	SENDINFO_DT_NAME(varName, remoteVarName)
#define PROPINFO_NAME(varName,remoteVarName)		SENDINFO_NAME(varName,remoteVarName)

#define DataTableProp	SendProp

#endif

// Use these functions to setup your data tables.
DataTableProp PropFloat(
	char *pVarName,					// Variable name.
	int offset,						// Offset into container structure.
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,					// Number of bits to use when encoding.
	int flags=0,
	float fLowValue=0.0f,			// For floating point, low and high values.
	float fHighValue=HIGH_DEFAULT	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	);

DataTableProp PropVector(
	char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,					// Number of bits (for each floating-point component) to use when encoding.
	int flags=SPROP_NOSCALE,
	float fLowValue=0.0f,			// For floating point, low and high values.
	float fHighValue=HIGH_DEFAULT	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	);

DataTableProp PropAngle(
	char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,
	int flags=0
	);

DataTableProp PropInt(
	char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,	// Handled by SENDINFO macro.
	int nBits=-1,					// Set to -1 to automatically pick (max) number of bits based on size of element.
	int flags=0,
	int rightShift=0
	);

DataTableProp PropString(
	char *pVarName,
	int offset,
	int bufferLen,
	int flags=0
	);

DataTableProp PropEHandle(
	char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE );

#endif // DATATABLE_SHARED_H

