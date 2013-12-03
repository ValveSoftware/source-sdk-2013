//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: implements various common send proxies
//
// $NoKeywords: $
//=============================================================================//

#ifndef SENDPROXY_H
#define SENDPROXY_H


#include "dt_send.h"


class DVariant;

void SendProxy_Color32ToInt( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_EHandleToInt( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );
void SendProxy_IntAddOne( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );
void SendProxy_ShortAddOne( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );

SendProp SendPropBool(
	const char *pVarName,
	int offset,
	int sizeofVar );

SendProp SendPropEHandle(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int flags = 0,
	SendVarProxyFn proxyFn=SendProxy_EHandleToInt );

SendProp SendPropTime(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE );

#if !defined( NO_ENTITY_PREDICTION )
SendProp SendPropPredictableId(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE	);
#endif

SendProp SendPropIntWithMinusOneFlag(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int bits=-1,
	SendVarProxyFn proxyFn=SendProxy_IntAddOne );


// Send a string_t as a string property.
SendProp SendPropStringT( const char *pVarName, int offset, int sizeofVar );

//-----------------------------------------------------------------------------
// Purpose: Proxy that only sends data to team members
//-----------------------------------------------------------------------------
void* SendProxy_OnlyToTeam( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );


#endif // SENDPROXY_H
