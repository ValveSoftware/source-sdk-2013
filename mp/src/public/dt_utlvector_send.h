//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================//

#ifndef DT_UTLVECTOR_SEND_H
#define DT_UTLVECTOR_SEND_H
#pragma once


#include "dt_send.h"
#include "dt_utlvector_common.h"


#define SENDINFO_UTLVECTOR( varName )	#varName, \
										offsetof(currentSendDTClass, varName), \
										sizeof(((currentSendDTClass*)0)->varName[0]), \
										GetEnsureCapacityTemplate( ((currentSendDTClass*)0)->varName )



#define SendPropUtlVectorDataTable( varName, nMaxElements, dataTableName ) \
	SendPropUtlVector( \
		SENDINFO_UTLVECTOR( varName ), \
		nMaxElements, \
		SendPropDataTable( NULL, 0, &REFERENCE_SEND_TABLE( dataTableName ) ) \
		)

//
// Set it up to transmit a CUtlVector of basic types or of structures.
//
// pArrayProp doesn't need a name, offset, or size. You can pass 0 for all those.
// Example usage:
//
//	SendPropUtlVectorDataTable( m_StructArray, 11, DT_TestStruct )
//
//	SendPropUtlVector( 
//		SENDINFO_UTLVECTOR( m_FloatArray ),
//		16,	// max elements
//		SendPropFloat( NULL, 0, 0, 0, SPROP_NOSCALE )
//		)
//
SendProp SendPropUtlVector(
	char *pVarName,		// Use SENDINFO_UTLVECTOR to generate these first 4 parameters.
	int offset,
	int sizeofVar,
	EnsureCapacityFn ensureFn,

	int nMaxElements,			// Max # of elements in the array. Keep this as low as possible.
	SendProp pArrayProp,		// Describe the data inside of each element in the array.
	SendTableProxyFn varProxy=SendProxy_DataTableToDataTable	// This can be overridden to control who the array is sent to.
	);


#endif	// DT_UTLVECTOR_SEND_H
