//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: implements various common send proxies
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "sendproxy.h"
#include "basetypes.h"
#include "baseentity.h"
#include "team.h"
#include "player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void SendProxy_Color32ToInt( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	color32 *pIn = (color32*)pData;
	*((unsigned int*)&pOut->m_Int) = ((unsigned int)pIn->r << 24) | ((unsigned int)pIn->g << 16) | ((unsigned int)pIn->b << 8) | ((unsigned int)pIn->a);
}

void SendProxy_EHandleToInt( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID)
{
	CBaseHandle *pHandle = (CBaseHandle*)pVarData;

	if ( pHandle && pHandle->Get() )
	{
		int iSerialNum = pHandle->GetSerialNumber() & (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1;
		pOut->m_Int = pHandle->GetEntryIndex() | (iSerialNum << MAX_EDICT_BITS);
	}
	else
	{
		pOut->m_Int = INVALID_NETWORKED_EHANDLE_VALUE;
	}
}

void SendProxy_IntAddOne( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID)
{
	int *pInt = (int *)pVarData;

	pOut->m_Int = (*pInt) + 1;
}

void SendProxy_ShortAddOne( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID)
{
	short *pInt = (short *)pVarData;

	pOut->m_Int = (*pInt) + 1;
}

SendProp SendPropBool(
	const char *pVarName,
	int offset,
	int sizeofVar )
{
	Assert( sizeofVar == sizeof( bool ) );
	return SendPropInt( pVarName, offset, sizeofVar, 1, SPROP_UNSIGNED );
}


SendProp SendPropEHandle(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int flags,
	SendVarProxyFn proxyFn )
{
	return SendPropInt( pVarName, offset, sizeofVar, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED|flags, proxyFn );
}

SendProp SendPropIntWithMinusOneFlag( const char *pVarName, int offset, int sizeofVar, int nBits, SendVarProxyFn proxyFn )
{
	return SendPropInt( pVarName, offset, sizeofVar, nBits, SPROP_UNSIGNED, proxyFn );
}

//-----------------------------------------------------------------------------
// Purpose: Proxy that only sends data to team members
// Input  : *pStruct - 
//			*pData - 
//			*pOut - 
//			objectID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void* SendProxy_OnlyToTeam( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	CBaseEntity *pEntity = (CBaseEntity*)pStruct;
	if ( pEntity )
	{
		CTeam *pTeam = pEntity->GetTeam();
		if ( pTeam )
		{
			pRecipients->ClearAllRecipients();
			for ( int i=0; i < pTeam->GetNumPlayers(); i++ )
				pRecipients->SetRecipient( pTeam->GetPlayer( i )->GetClientIndex() );
		
			return (void*)pVarData;
		}
	}

	return NULL;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_OnlyToTeam );

#define TIME_BITS 24

// This table encodes edict data.
static void SendProxy_Time( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID )
{
	float clock_base = floor( gpGlobals->curtime );
	float t = *( float * )pVarData;
	float dt = t - clock_base;
	int addt = Floor2Int( 1000.0f * dt + 0.5f );
	// TIME_BITS bits gives us TIME_BITS-1 bits plus sign bit
	int maxoffset = 1 << ( TIME_BITS - 1);

	addt = clamp( addt, -maxoffset, maxoffset );

	pOut->m_Int = addt;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVarName - 
//			sizeofVar - 
//			flags - 
//			pId - 
// Output : SendProp
//-----------------------------------------------------------------------------
SendProp SendPropTime(
	const char *pVarName,
	int offset,
	int sizeofVar )
{
//	return SendPropInt( pVarName, offset, sizeofVar, TIME_BITS, 0, SendProxy_Time );
	// FIXME:  Re-enable above when it doesn't cause lots of deltas
	return SendPropFloat( pVarName, offset, sizeofVar, -1, SPROP_NOSCALE );
}

#if !defined( NO_ENTITY_PREDICTION )

#define PREDICTABLE_ID_BITS 31

//-----------------------------------------------------------------------------
// Purpose: Converts a predictable Id to an integer
// Input  : *pStruct - 
//			*pVarData - 
//			*pOut - 
//			iElement - 
//			objectID - 
//-----------------------------------------------------------------------------
static void SendProxy_PredictableIdToInt( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID )
{
	CPredictableId* pId = ( CPredictableId * )pVarData;
	if ( pId )
	{
		pOut->m_Int = pId->GetRaw();
	}
	else
	{
		pOut->m_Int = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVarName - 
//			sizeofVar - 
//			flags - 
//			pId - 
// Output : SendProp
//-----------------------------------------------------------------------------
SendProp SendPropPredictableId(
	const char *pVarName,
	int offset,
	int sizeofVar )
{
	return SendPropInt( pVarName, offset, sizeofVar, PREDICTABLE_ID_BITS, SPROP_UNSIGNED, SendProxy_PredictableIdToInt );
}

#endif

void SendProxy_StringT_To_String( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID )
{
	string_t &str = *((string_t*)pVarData);
	pOut->m_pString = (char*)STRING( str );
}


SendProp SendPropStringT( const char *pVarName, int offset, int sizeofVar )
{
	// Make sure it's the right type.
	Assert( sizeofVar == sizeof( string_t ) );

	return SendPropString( pVarName, offset, DT_MAX_STRING_BUFFERSIZE, 0, SendProxy_StringT_To_String );
}
