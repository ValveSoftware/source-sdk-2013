//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: implements various common send proxies
//
// $NoKeywords: $
//=============================================================================//

#ifndef RECVPROXY_H
#define RECVPROXY_H


#include "dt_recv.h"

class CRecvProxyData;


// This converts the int stored in pData to an EHANDLE in pOut.
void RecvProxy_IntToEHandle( const CRecvProxyData *pData, void *pStruct, void *pOut );

void RecvProxy_IntToMoveParent( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_IntToColor32( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_IntSubOne( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_ShortSubOne( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_InterpolationAmountChanged( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_IntToModelIndex16_BackCompatible( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_IntToModelIndex32_BackCompatible( const CRecvProxyData *pData, void *pStruct, void *pOut );

RecvProp RecvPropTime(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE );

#if !defined( NO_ENTITY_PREDICTION )
RecvProp RecvPropPredictableId(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE );
#endif

RecvProp RecvPropEHandle(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE,
	RecvVarProxyFn proxyFn=RecvProxy_IntToEHandle );

RecvProp RecvPropBool(
	const char *pVarName, 
	int offset, 
	int sizeofVar );

RecvProp RecvPropBool(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int flags,
	RecvVarProxyFn varProxy );

RecvProp RecvPropIntWithMinusOneFlag(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE,
	RecvVarProxyFn proxyFn=RecvProxy_IntSubOne );

#endif // RECVPROXY_H

