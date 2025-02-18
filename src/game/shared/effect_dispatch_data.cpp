//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "effect_dispatch_data.h"
#include "coordsize.h"

#ifdef CLIENT_DLL
#include "cliententitylist.h"
#endif

#include "qlimits.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define SUBINCH_PRECISION	3

#ifdef CLIENT_DLL

	#include "dt_recv.h"

	static void RecvProxy_EntIndex( const CRecvProxyData *pData, void *pStruct, void *pOut )
	{
		int nEntIndex = pData->m_Value.m_Int;
		((CEffectData*)pStruct)->m_hEntity = (nEntIndex < 0) ? INVALID_EHANDLE_INDEX : ClientEntityList().EntIndexToHandle( nEntIndex );
	}

	BEGIN_RECV_TABLE_NOBASE( CEffectData, DT_EffectData )

		RecvPropFloat( RECVINFO( m_vOrigin[0] ) ),
		RecvPropFloat( RECVINFO( m_vOrigin[1] ) ),
		RecvPropFloat( RECVINFO( m_vOrigin[2] ) ),

		RecvPropFloat( RECVINFO( m_vStart[0] ) ),
		RecvPropFloat( RECVINFO( m_vStart[1] ) ),
		RecvPropFloat( RECVINFO( m_vStart[2] ) ),

		RecvPropQAngles( RECVINFO( m_vAngles ) ),

		RecvPropVector(	RECVINFO( m_vNormal ) ),

		RecvPropInt( RECVINFO( m_fFlags ) ),
		RecvPropFloat( RECVINFO( m_flMagnitude ) ),
		RecvPropFloat( RECVINFO( m_flScale ) ),
		RecvPropInt( RECVINFO( m_nAttachmentIndex ) ),
		RecvPropIntWithMinusOneFlag( RECVINFO( m_nSurfaceProp ), RecvProxy_ShortSubOne ),
		RecvPropInt( RECVINFO( m_iEffectName ) ),

		RecvPropInt( RECVINFO( m_nMaterial ) ),
		RecvPropInt( RECVINFO( m_nDamageType ) ),
		RecvPropInt( RECVINFO( m_nHitBox ) ),

		RecvPropInt( "entindex", 0, SIZEOF_IGNORE, 0, RecvProxy_EntIndex ),

		RecvPropInt( RECVINFO( m_nColor ) ),

		RecvPropFloat( RECVINFO( m_flRadius ) ),

		RecvPropBool( RECVINFO( m_bCustomColors ) ),
		RecvPropVector( RECVINFO( m_CustomColors.m_vecColor1 ) ),
		RecvPropVector( RECVINFO( m_CustomColors.m_vecColor2 ) ),

		RecvPropBool( RECVINFO( m_bControlPoint1 ) ),
		RecvPropInt( RECVINFO( m_ControlPoint1.m_eParticleAttachment ) ),
		RecvPropFloat( RECVINFO( m_ControlPoint1.m_vecOffset[0] ) ),
		RecvPropFloat( RECVINFO( m_ControlPoint1.m_vecOffset[1] ) ),
		RecvPropFloat( RECVINFO( m_ControlPoint1.m_vecOffset[2] ) ),

	END_RECV_TABLE()

#else

	#include "dt_send.h"

	BEGIN_SEND_TABLE_NOBASE( CEffectData, DT_EffectData )

		// Everything uses _NOCHECK here since this is not an entity and we don't need
		// the functionality of CNetworkVars.

		// Get half-inch precision here.
#ifdef HL2_DLL
		SendPropFloat( SENDINFO_NOCHECK( m_vOrigin[0] ), COORD_INTEGER_BITS+SUBINCH_PRECISION, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),
		SendPropFloat( SENDINFO_NOCHECK( m_vOrigin[1] ), COORD_INTEGER_BITS+SUBINCH_PRECISION, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),
		SendPropFloat( SENDINFO_NOCHECK( m_vOrigin[2] ), COORD_INTEGER_BITS+SUBINCH_PRECISION, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),
		SendPropFloat( SENDINFO_NOCHECK( m_vStart[0] ), COORD_INTEGER_BITS+SUBINCH_PRECISION, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),
		SendPropFloat( SENDINFO_NOCHECK( m_vStart[1] ), COORD_INTEGER_BITS+SUBINCH_PRECISION, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),
		SendPropFloat( SENDINFO_NOCHECK( m_vStart[2] ), COORD_INTEGER_BITS+SUBINCH_PRECISION, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),
#else
		SendPropFloat( SENDINFO_NOCHECK( m_vOrigin[0] ), -1, SPROP_COORD_MP_INTEGRAL ),
		SendPropFloat( SENDINFO_NOCHECK( m_vOrigin[1] ), -1, SPROP_COORD_MP_INTEGRAL ),
		SendPropFloat( SENDINFO_NOCHECK( m_vOrigin[2] ), -1, SPROP_COORD_MP_INTEGRAL ),
		SendPropFloat( SENDINFO_NOCHECK( m_vStart[0] ), -1, SPROP_COORD_MP_INTEGRAL ),
		SendPropFloat( SENDINFO_NOCHECK( m_vStart[1] ), -1, SPROP_COORD_MP_INTEGRAL ),
		SendPropFloat( SENDINFO_NOCHECK( m_vStart[2] ), -1, SPROP_COORD_MP_INTEGRAL ),
#endif
		SendPropQAngles( SENDINFO_NOCHECK( m_vAngles ), 7 ),

#if defined( TF_DLL )
		SendPropVector( SENDINFO_NOCHECK( m_vNormal ), 6, 0, -1.0f, 1.0f ),
#else
		SendPropVector( SENDINFO_NOCHECK( m_vNormal ), 0, SPROP_NORMAL ),
#endif

		SendPropInt( SENDINFO_NOCHECK( m_fFlags ), MAX_EFFECT_FLAG_BITS, SPROP_UNSIGNED ),
		SendPropFloat( SENDINFO_NOCHECK( m_flMagnitude ), 12, SPROP_ROUNDDOWN, 0.0f, 1023.0f ),
		SendPropFloat( SENDINFO_NOCHECK( m_flScale ), 0, SPROP_NOSCALE ),
		SendPropInt( SENDINFO_NOCHECK( m_nAttachmentIndex ), 5, SPROP_UNSIGNED ),
		SendPropIntWithMinusOneFlag( SENDINFO_NOCHECK( m_nSurfaceProp ), 8, SendProxy_ShortAddOne ),
		SendPropInt( SENDINFO_NOCHECK( m_iEffectName ), MAX_EFFECT_DISPATCH_STRING_BITS, SPROP_UNSIGNED ),

		SendPropInt( SENDINFO_NOCHECK( m_nMaterial ), MAX_MODEL_INDEX_BITS, SPROP_UNSIGNED ),
		SendPropInt( SENDINFO_NOCHECK( m_nDamageType ), 32, SPROP_UNSIGNED ),
		SendPropInt( SENDINFO_NOCHECK( m_nHitBox ), 12, SPROP_UNSIGNED ),

		SendPropInt( SENDINFO_NAME( m_nEntIndex, entindex ), MAX_EDICT_BITS, SPROP_UNSIGNED ),

		SendPropInt( SENDINFO_NOCHECK( m_nColor ), 8, SPROP_UNSIGNED ),

		SendPropFloat( SENDINFO_NOCHECK( m_flRadius ), 10, SPROP_ROUNDDOWN, 0.0f, 1023.0f ),

		SendPropBool( SENDINFO_NOCHECK( m_bCustomColors) ),
		SendPropVector( SENDINFO_NOCHECK( m_CustomColors.m_vecColor1 ), 8, 0, 0, 1 ),
		SendPropVector( SENDINFO_NOCHECK( m_CustomColors.m_vecColor2 ), 8, 0, 0, 1 ),

		SendPropBool( SENDINFO_NOCHECK( m_bControlPoint1) ),
		SendPropInt( SENDINFO_NOCHECK( m_ControlPoint1.m_eParticleAttachment ), 5, SPROP_UNSIGNED ),
		SendPropFloat( SENDINFO_NOCHECK( m_ControlPoint1.m_vecOffset[0] ), -1, SPROP_COORD ),
		SendPropFloat( SENDINFO_NOCHECK( m_ControlPoint1.m_vecOffset[1] ), -1, SPROP_COORD ),
		SendPropFloat( SENDINFO_NOCHECK( m_ControlPoint1.m_vecOffset[2] ), -1, SPROP_COORD ),

	END_SEND_TABLE()

#endif

#ifdef CLIENT_DLL

IClientRenderable *CEffectData::GetRenderable() const
{
	return ClientEntityList().GetClientRenderableFromHandle( m_hEntity );
}

C_BaseEntity *CEffectData::GetEntity() const
{
	return ClientEntityList().GetBaseEntityFromHandle( m_hEntity );
}

int CEffectData::entindex() const
{
	C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( m_hEntity );
	return pEnt ? pEnt->entindex() : -1;
}

#endif

#ifdef CLIENT_DLL

bool g_bSuppressParticleEffects = false;

bool SuppressingParticleEffects()
{
	return g_bSuppressParticleEffects;
}

void SuppressParticleEffects( bool bSuppress )
{
	g_bSuppressParticleEffects = bSuppress;
}

#endif
