//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: implements various common send proxies
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recvproxy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#include "cdll_client_int.h"
#include "proto_version.h"

void RecvProxy_IntToColor32( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	color32 *pOutColor = (color32*)pOut;
	unsigned int inColor = *((unsigned int*)&pData->m_Value.m_Int);

	pOutColor->r = (unsigned char)(inColor >> 24);
	pOutColor->g = (unsigned char)((inColor >> 16) & 0xFF);
	pOutColor->b = (unsigned char)((inColor >> 8) & 0xFF);
	pOutColor->a = (unsigned char)(inColor & 0xFF);
}

void RecvProxy_IntSubOne( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int *pInt = (int *)pOut;
	
	*pInt = pData->m_Value.m_Int - 1;
}

void RecvProxy_ShortSubOne( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	short *pInt = (short *)pOut;
	
	*pInt = pData->m_Value.m_Int - 1;
}

RecvProp RecvPropIntWithMinusOneFlag( const char *pVarName, int offset, int sizeofVar, RecvVarProxyFn proxyFn )
{
	return RecvPropInt( pVarName, offset, sizeofVar, 0, proxyFn );
}

void RecvProxy_IntToModelIndex16_BackCompatible( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int modelIndex = pData->m_Value.m_Int;
	if ( modelIndex < -1 && engine->GetProtocolVersion() <= PROTOCOL_VERSION_20 )
	{
		Assert( modelIndex > -20000 );
		modelIndex = -2 - ( ( -2 - modelIndex ) << 1 );
	}
	*(int16*)pOut = modelIndex;
}

void RecvProxy_IntToModelIndex32_BackCompatible( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int modelIndex = pData->m_Value.m_Int;
	if ( modelIndex < -1 && engine->GetProtocolVersion() <= PROTOCOL_VERSION_20 )
	{
		Assert( modelIndex > -20000 );
		modelIndex = -2 - ( ( -2 - modelIndex ) << 1 );
	}
	*(int32*)pOut = modelIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Okay, so we have to queue up the actual ehandle to entity lookup for the following reason:
//  If a player has an EHandle/CHandle to an object such as a weapon, since the player is in slot 1-31, then
//  if the weapon is created and given to the player in the same frame, then the weapon won't have been
//  created at the time we parse this EHandle index, since the player is ahead of every other entity in the
//  packet (except for the world).
// So instead, we remember which ehandles need to be set and we set them after all network data has
//  been received.  Sigh.
// Input  : *pData - 
//			*pStruct - 
//			*pOut - 
//-----------------------------------------------------------------------------

void RecvProxy_IntToEHandle( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CBaseHandle *pEHandle = (CBaseHandle*)pOut;
	
	if ( pData->m_Value.m_Int == INVALID_NETWORKED_EHANDLE_VALUE )
	{
		*pEHandle = INVALID_EHANDLE;
	}
	else
	{
		int iEntity = pData->m_Value.m_Int & ((1 << MAX_EDICT_BITS) - 1);
		int iSerialNum = pData->m_Value.m_Int >> MAX_EDICT_BITS;
		
		pEHandle->Init( iEntity, iSerialNum );
	}
}

RecvProp RecvPropEHandle(
	const char *pVarName, 
	int offset, 
	int sizeofVar,
	RecvVarProxyFn proxyFn )
{
	return RecvPropInt( pVarName, offset, sizeofVar, 0, proxyFn );
}


RecvProp RecvPropBool(
	const char *pVarName, 
	int offset, 
	int sizeofVar )
{
	Assert( sizeofVar == sizeof( bool ) );
	return RecvPropInt( pVarName, offset, sizeofVar );
}

RecvProp RecvPropBool(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int flags,
	RecvVarProxyFn varProxy
	)
{
	Assert( sizeofVar == sizeof( bool ) );
	return RecvPropInt( pVarName, offset, sizeofVar, flags, varProxy );
}


//-----------------------------------------------------------------------------
// Moveparent receive proxies
//-----------------------------------------------------------------------------
void RecvProxy_IntToMoveParent( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CHandle<C_BaseEntity> *pHandle = (CHandle<C_BaseEntity>*)pOut;
	RecvProxy_IntToEHandle( pData, pStruct, (CBaseHandle*)pHandle );
}


void RecvProxy_InterpolationAmountChanged( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// m_bSimulatedEveryTick & m_bAnimatedEveryTick are boolean
	if ( *((bool*)pOut) != (pData->m_Value.m_Int != 0) )
	{
		// Have the regular proxy store the data.
		RecvProxy_Int32ToInt8( pData, pStruct, pOut );

		C_BaseEntity *pEntity = (C_BaseEntity *) pStruct;
		pEntity->Interp_UpdateInterpolationAmounts( pEntity->GetVarMapping() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Decodes a time value
// Input  : *pStruct - ( C_BaseEntity * ) used to flag animtime is changine
//			*pVarData - 
//			*pIn - 
//			objectID - 
//-----------------------------------------------------------------------------
static void RecvProxy_Time( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	float	t;
	float	clock_base;
	float	offset;

	// Get msec offset
	offset	= ( float )pData->m_Value.m_Int / 1000.0f;

	// Get base
	clock_base = floor( engine->GetLastTimeStamp() );

	// Add together and clamp to msec precision
	t = ClampToMsec( clock_base + offset );

	// Store decoded value
	*( float * )pOut = t;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVarName - 
//			sizeofVar - 
// Output : RecvProp
//-----------------------------------------------------------------------------
RecvProp RecvPropTime(
	const char *pVarName, 
	int offset, 
	int sizeofVar/*=SIZEOF_IGNORE*/ )
{
//	return RecvPropInt( pVarName, offset, sizeofVar, 0, RecvProxy_Time );
	return RecvPropFloat( pVarName, offset, sizeofVar );
};

//-----------------------------------------------------------------------------
// Purpose: Okay, so we have to queue up the actual ehandle to entity lookup for the following reason:
//  If a player has an EHandle/CHandle to an object such as a weapon, since the player is in slot 1-31, then
//  if the weapon is created and given to the player in the same frame, then the weapon won't have been
//  created at the time we parse this EHandle index, since the player is ahead of every other entity in the
//  packet (except for the world).
// So instead, we remember which ehandles need to be set and we set them after all network data has
//  been received.  Sigh.
// Input  : *pData - 
//			*pStruct - 
//			*pOut - 
//-----------------------------------------------------------------------------
#if !defined( NO_ENTITY_PREDICTION )
static void RecvProxy_IntToPredictableId( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CPredictableId *pId = (CPredictableId*)pOut;
	Assert( pId );
	pId->SetRaw( pData->m_Value.m_Int );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVarName - 
//			sizeofVar - 
// Output : RecvProp
//-----------------------------------------------------------------------------
RecvProp RecvPropPredictableId(
	const char *pVarName, 
	int offset, 
	int sizeofVar/*=SIZEOF_IGNORE*/ )
{
	return RecvPropInt( pVarName, offset, sizeofVar, 0, RecvProxy_IntToPredictableId );
}
#endif
