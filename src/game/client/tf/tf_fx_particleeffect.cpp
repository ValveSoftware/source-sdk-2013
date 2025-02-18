//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific explosion effects
//
//=============================================================================//
#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "tf_shareddefs.h"
#include "c_basetempentity.h"
#include "tier0/vprof.h"
#include "c_tf_fx.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_TETFParticleEffect : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TETFParticleEffect, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	C_TETFParticleEffect( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector m_vecOrigin;
	Vector m_vecStart;
	QAngle m_vecAngles;

	int m_iParticleSystemIndex;

	ClientEntityHandle_t m_hEntity;

	int m_iAttachType;
	int m_iAttachmentPointIndex;

	bool m_bResetParticles;

	bool							m_bCustomColors;
	te_tf_particle_effects_colors_t	m_CustomColors;

	bool									m_bControlPoint1;
	te_tf_particle_effects_control_point_t	m_ControlPoint1;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TETFParticleEffect::C_TETFParticleEffect( void )
{
	m_vecOrigin.Init();
	m_vecStart.Init();
	m_vecAngles.Init();

	m_iParticleSystemIndex = -1;

	m_hEntity = INVALID_EHANDLE;

	m_iAttachType = PATTACH_ABSORIGIN;
	m_iAttachmentPointIndex = 0;

	m_bResetParticles = false;

	m_bCustomColors = false;
	m_CustomColors.m_vecColor1.Init();
	m_CustomColors.m_vecColor2.Init();

	m_bControlPoint1 = false;
	m_ControlPoint1.m_eParticleAttachment = PATTACH_ABSORIGIN;
	m_ControlPoint1.m_vecOffset.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TETFParticleEffect::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TETFParticleEffect::PostDataUpdate" );

	CEffectData	data;

	data.m_nHitBox = m_iParticleSystemIndex;

	data.m_vOrigin = m_vecOrigin;
	data.m_vStart = m_vecStart;
	data.m_vAngles = m_vecAngles;

	if ( m_hEntity != INVALID_EHANDLE )
	{
		data.m_hEntity = m_hEntity;
		data.m_fFlags |= PARTICLE_DISPATCH_FROM_ENTITY;
	}
	else
	{
		data.m_hEntity = NULL;
	}

	data.m_nDamageType = m_iAttachType;
	data.m_nAttachmentIndex = m_iAttachmentPointIndex;

	if ( m_bResetParticles )
	{
		data.m_fFlags |= PARTICLE_DISPATCH_RESET_PARTICLES;
	}

	data.m_bCustomColors = m_bCustomColors;
	data.m_CustomColors = m_CustomColors;

	data.m_bControlPoint1 = m_bControlPoint1;
	data.m_ControlPoint1 = m_ControlPoint1;

	DispatchEffect( "ParticleEffect", data );
}

static void RecvProxy_ParticleSystemEntIndex( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int nEntIndex = pData->m_Value.m_Int;
	// The 'new' encoding for INVALID_EHANDLE_INDEX is 2047, but the old encoding
	// was -1. Old demos and replays will use the old encoding so we have to check
	// for it. The field is now unsigned so -1 will not be created in new replays.
	((C_TETFParticleEffect*)pStruct)->m_hEntity = (nEntIndex == kInvalidEHandleParticleEffect || nEntIndex == -1) ? INVALID_EHANDLE : ClientEntityList().EntIndexToHandle( nEntIndex );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT( C_TETFParticleEffect, DT_TETFParticleEffect, CTETFParticleEffect )
	RecvPropFloat( RECVINFO( m_vecOrigin[0] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[1] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[2] ) ),
	RecvPropFloat( RECVINFO( m_vecStart[0] ) ),
	RecvPropFloat( RECVINFO( m_vecStart[1] ) ),
	RecvPropFloat( RECVINFO( m_vecStart[2] ) ),
	RecvPropQAngles( RECVINFO( m_vecAngles ) ),
	RecvPropInt( RECVINFO( m_iParticleSystemIndex ) ),
	RecvPropInt( "entindex", 0, SIZEOF_IGNORE, 0, RecvProxy_ParticleSystemEntIndex ),
	RecvPropInt( RECVINFO( m_iAttachType ) ),
	RecvPropInt( RECVINFO( m_iAttachmentPointIndex ) ),
	RecvPropInt( RECVINFO( m_bResetParticles ) ),
	RecvPropBool( RECVINFO( m_bCustomColors ) ),
	RecvPropVector( RECVINFO( m_CustomColors.m_vecColor1 ) ),
	RecvPropVector( RECVINFO( m_CustomColors.m_vecColor2 ) ),
	RecvPropBool( RECVINFO( m_bControlPoint1 ) ),
	RecvPropInt( RECVINFO( m_ControlPoint1.m_eParticleAttachment ) ),
	RecvPropFloat( RECVINFO( m_ControlPoint1.m_vecOffset[0] ) ),
	RecvPropFloat( RECVINFO( m_ControlPoint1.m_vecOffset[1] ) ),
	RecvPropFloat( RECVINFO( m_ControlPoint1.m_vecOffset[2] ) ),
END_RECV_TABLE()