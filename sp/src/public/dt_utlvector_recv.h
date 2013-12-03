//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================//

#ifndef DT_UTLVECTOR_RECV_H
#define DT_UTLVECTOR_RECV_H
#pragma once


#include "dt_recv.h"
#include "dt_utlvector_common.h"



#define RECVINFO_UTLVECTOR( varName )	#varName, \
										offsetof(currentRecvDTClass, varName), \
										sizeof(((currentRecvDTClass*)0)->varName[0]), \
										GetResizeUtlVectorTemplate( ((currentRecvDTClass*)0)->varName ), \
										GetEnsureCapacityTemplate( ((currentRecvDTClass*)0)->varName )

// Use this macro to specify a utlvector where you specify the function
// that gets called to make sure the size of the utlvector is correct.
// The size function looks like this: void ResizeUtlVector( void *pVoid, int len )
#define RECVINFO_UTLVECTOR_SIZEFN( varName, resizeFn )	#varName, \
										offsetof(currentRecvDTClass, varName), \
										sizeof(((currentRecvDTClass*)0)->varName[0]), \
										resizeFn, \
										GetEnsureCapacityTemplate( ((currentRecvDTClass*)0)->varName )


#define RecvPropUtlVectorDataTable( varName, nMaxElements, dataTableName ) \
	RecvPropUtlVector( RECVINFO_UTLVECTOR( varName ), nMaxElements, RecvPropDataTable(NULL,0,0, &REFERENCE_RECV_TABLE( dataTableName  ) ) )


//
// Receive a property sent with SendPropUtlVector.
//
// Example usage:
//
//	RecvPropUtlVectorDataTable( m_StructArray, 11, DT_StructArray )
//
//	RecvPropUtlVector( RECVINFO_UTLVECTOR( m_FloatArray ), 16, RecvPropFloat(NULL,0,0) )
//
RecvProp RecvPropUtlVector(
	const char *pVarName,		// Use RECVINFO_UTLVECTOR to generate these first 5 parameters.
	int offset,
	int sizeofVar,
	ResizeUtlVectorFn fn,
	EnsureCapacityFn ensureFn,

	int nMaxElements,	// Max # of elements in the array. Keep this as low as possible.
	RecvProp pArrayProp	// The definition of the property you're receiving into.
						// You can leave all of its parameters at 0 (name, offset, size, etc).
	);


#endif // DT_UTLVECTOR_RECV_H
