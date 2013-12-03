//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "dt_shared.h"

#if !defined (CLIENT_DLL)
#include "sendproxy.h"
#else
#include "recvproxy.h"
#endif


// ------------------------------------------------------------------------ //
// Just wrappers to make shared code look easier...
// ------------------------------------------------------------------------ //

// Use these functions to setup your data tables.
DataTableProp PropFloat(
	char *pVarName,					// Variable name.
	int offset,						// Offset into container structure.
	int sizeofVar,
	int nBits,					// Number of bits to use when encoding.
	int flags,
	float fLowValue,			// For floating point, low and high values.
	float fHighValue	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	)
{
#if !defined (CLIENT_DLL)
	return SendPropFloat( pVarName, offset, sizeofVar, nBits, flags, fLowValue, fHighValue );
#else
	return RecvPropFloat( pVarName, offset, sizeofVar, flags );
#endif
}

DataTableProp PropVector(
	char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,					// Number of bits (for each floating-point component) to use when encoding.
	int flags,
	float fLowValue,			// For floating point, low and high values.
	float fHighValue	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	)
{
#if !defined (CLIENT_DLL)
	return SendPropVector( pVarName, offset, sizeofVar, nBits, flags, fLowValue, fHighValue );
#else
	return RecvPropVector( pVarName, offset, sizeofVar, flags );
#endif
}

DataTableProp PropAngle(
	char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,
	int flags
	)
{
#if !defined (CLIENT_DLL)
	return SendPropAngle( pVarName, offset, sizeofVar, nBits, flags );
#else
	return RecvPropFloat( pVarName, offset, sizeofVar, flags );
#endif
}

DataTableProp PropInt(
	char *pVarName,
	int offset,
	int sizeofVar,	// Handled by SENDINFO macro.
	int nBits,					// Set to -1 to automatically pick (max) number of bits based on size of element.
	int flags,
	int rightShift
	)
{
#if !defined (CLIENT_DLL)
	return SendPropInt( pVarName, offset, sizeofVar, nBits, flags, rightShift );
#else
	return RecvPropInt( pVarName, offset, sizeofVar, flags );
#endif
}

DataTableProp PropString(
	char *pVarName,
	int offset,
	int bufferLen,
	int flags
	)
{
#if !defined (CLIENT_DLL)
	return SendPropString( pVarName, offset, bufferLen, flags );
#else
	return RecvPropString( pVarName, offset, bufferLen, flags );
#endif
}

DataTableProp PropEHandle(
	char *pVarName,
	int offset,
	int sizeofVar )
{
#if !defined (CLIENT_DLL)
	return SendPropEHandle( pVarName, offset, sizeofVar );
#else
	return RecvPropEHandle( pVarName, offset, sizeofVar );
#endif
}

