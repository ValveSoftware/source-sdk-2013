//========= Copyright Valve Corporation, All rights reserved. ============//
//
//  
//
//=============================================================================

#include "cbase.h"
#include "basetempentity.h"
#include "tf_fx.h"
#include "tf_shareddefs.h"
#include "coordsize.h"

#define NUM_BULLET_SEED_BITS	8

//-----------------------------------------------------------------------------
// Purpose: Display a blood sprite
//-----------------------------------------------------------------------------
class CTEFireBullets : public CBaseTempEntity
{
public:

	DECLARE_CLASS( CTEFireBullets, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEFireBullets( const char *name );
	virtual	~CTEFireBullets( void );

public:

	CNetworkVar( int, m_iPlayer );
	CNetworkVector( m_vecOrigin );
	CNetworkQAngle( m_vecAngles );
	CNetworkVar( int, m_iWeaponID );
	CNetworkVar( int, m_iMode );
	CNetworkVar( int, m_iSeed );
	CNetworkVar( float, m_flSpread );
	CNetworkVar( bool, m_bCritical );

};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEFireBullets::CTEFireBullets( const char *name ) : CBaseTempEntity( name )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEFireBullets::~CTEFireBullets( void )
{
}

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEFireBullets, DT_TEFireBullets )
SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD_MP_INTEGRAL ),
SendPropAngle( SENDINFO_VECTORELEM( m_vecAngles, 0 ), 7, 0 ),
SendPropAngle( SENDINFO_VECTORELEM( m_vecAngles, 1 ), 7, 0 ),
SendPropInt( SENDINFO( m_iWeaponID ), Q_log2(TF_WEAPON_COUNT)+1, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_iMode ), 1, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_iSeed ), NUM_BULLET_SEED_BITS, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_iPlayer ), 6, SPROP_UNSIGNED ), 	// max 64 players, see MAX_PLAYERS
SendPropFloat( SENDINFO( m_flSpread ), 8, 0, 0.0f, 1.0f ),	
SendPropBool( SENDINFO( m_bCritical ) ),
END_SEND_TABLE()

// Singleton
static CTEFireBullets g_TEFireBullets( "Fire Bullets" );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void TE_FireBullets( int iPlayerIndex, const Vector &vOrigin, const QAngle &vAngles, 
					 int iWeaponID, int	iMode, int iSeed, float flSpread, bool bCritical )
{
	CPASFilter filter( vOrigin );
	filter.UsePredictionRules();

	g_TEFireBullets.m_iPlayer = iPlayerIndex-1;
	g_TEFireBullets.m_vecOrigin = vOrigin;
	g_TEFireBullets.m_vecAngles = vAngles;
	g_TEFireBullets.m_iSeed = iSeed;
	g_TEFireBullets.m_flSpread = flSpread;
	g_TEFireBullets.m_iMode = iMode;
	g_TEFireBullets.m_iWeaponID = iWeaponID;
	g_TEFireBullets.m_bCritical = bCritical;

	Assert( iSeed < (1 << NUM_BULLET_SEED_BITS) );

	g_TEFireBullets.Create( filter, 0 );
}

//=============================================================================
//
// Explosions.
//
class CTETFExplosion : public CBaseTempEntity
{
public:

	DECLARE_CLASS( CTETFExplosion, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTETFExplosion( const char *name );

public:

	Vector m_vecOrigin;
	Vector m_vecNormal;
	int m_iWeaponID;
	int m_nEntIndex;
	int m_nDefID;
	int m_nSound;
	int m_iCustomParticleIndex;
};

// Singleton to fire explosion objects
static CTETFExplosion g_TETFExplosion( "TFExplosion" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTETFExplosion::CTETFExplosion( const char *name ) : CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_vecNormal.Init();
	m_iWeaponID = TF_WEAPON_NONE;
	m_nEntIndex = kInvalidEHandleExplosion;
	m_nDefID = -1;
	m_nSound = SPECIAL1;
	m_iCustomParticleIndex = INVALID_STRING_INDEX;
}

IMPLEMENT_SERVERCLASS_ST( CTETFExplosion, DT_TETFExplosion )
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[0] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[1] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[2] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropVector( SENDINFO_NOCHECK( m_vecNormal ), 6, 0, -1.0f, 1.0f ),
	SendPropInt( SENDINFO_NOCHECK( m_iWeaponID ), Q_log2( TF_WEAPON_COUNT )+1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_NAME( m_nEntIndex, entindex ), MAX_EDICT_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_NOCHECK( m_nDefID ), -1 ),
	SendPropInt( SENDINFO_NOCHECK( m_nSound ), -1 ),
	SendPropInt( SENDINFO_NOCHECK( m_iCustomParticleIndex ), -1 ),
END_SEND_TABLE()

void TE_TFExplosion( IRecipientFilter &filter, float flDelay, const Vector &vecOrigin, const Vector &vecNormal, int iWeaponID, int nEntIndex, int nDefID, int nSound, int iCustomParticleIndex )
{
	VectorCopy( vecOrigin, g_TETFExplosion.m_vecOrigin );
	VectorCopy( vecNormal, g_TETFExplosion.m_vecNormal );
	g_TETFExplosion.m_iWeaponID	= iWeaponID;	
	g_TETFExplosion.m_nEntIndex	= nEntIndex;
	g_TETFExplosion.m_nDefID = nDefID;
	g_TETFExplosion.m_nSound = nSound;
	g_TETFExplosion.m_iCustomParticleIndex = iCustomParticleIndex;

	// Send it over the wire
	g_TETFExplosion.Create( filter, flDelay );
}

void TE_TFExplosion( IRecipientFilter &filter, float flDelay, const Vector &vecOrigin, const Vector &vecNormal, int iWeaponID, int nEntIndex, int nDefID, int nSound )
{
	TE_TFExplosion( filter, flDelay, vecOrigin, vecNormal, iWeaponID, nEntIndex, nDefID, nSound, INVALID_STRING_INDEX );
}

//=============================================================================
//
// TF ParticleEffect
//
class CTETFParticleEffect : public CBaseTempEntity
{
public:

	DECLARE_CLASS( CTETFParticleEffect, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTETFParticleEffect( const char *name );

	void Init( void );

public:

	Vector m_vecOrigin;
	Vector m_vecStart;
	QAngle m_vecAngles;

	int m_iParticleSystemIndex;

	int m_nEntIndex;

	int m_iAttachType;
	int m_iAttachmentPointIndex;

	bool m_bResetParticles;

	bool							m_bCustomColors;
	te_tf_particle_effects_colors_t	m_CustomColors;

	bool									m_bControlPoint1;
	te_tf_particle_effects_control_point_t	m_ControlPoint1;
};

// Singleton to fire explosion objects
static CTETFParticleEffect g_TETFParticleEffect( "TFParticleEffect" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTETFParticleEffect::CTETFParticleEffect( const char *name ) : CBaseTempEntity( name )
{
	Init();
}

void CTETFParticleEffect::Init( void )
{
	m_vecOrigin.Init();
	m_vecStart.Init();
	m_vecAngles.Init();

	m_iParticleSystemIndex = 0;

	m_nEntIndex = kInvalidEHandleParticleEffect;

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


IMPLEMENT_SERVERCLASS_ST( CTETFParticleEffect, DT_TETFParticleEffect )
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[0] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[1] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[2] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecStart[0] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecStart[1] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecStart[2] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropQAngles( SENDINFO_NOCHECK( m_vecAngles ), 7 ),
	SendPropInt( SENDINFO_NOCHECK( m_iParticleSystemIndex ), 16, SPROP_UNSIGNED ),	// probably way too high
	SendPropInt( SENDINFO_NAME( m_nEntIndex, entindex ), MAX_EDICT_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_NOCHECK( m_iAttachType ), 5, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_NOCHECK( m_iAttachmentPointIndex ), -1 ),
	SendPropBool( SENDINFO_NOCHECK( m_bResetParticles ) ),
	SendPropBool( SENDINFO_NOCHECK( m_bCustomColors) ),
	SendPropVector( SENDINFO_NOCHECK( m_CustomColors.m_vecColor1 ), 8, 0, 0, 1 ),
	SendPropVector( SENDINFO_NOCHECK( m_CustomColors.m_vecColor2 ), 8, 0, 0, 1 ),
	SendPropBool( SENDINFO_NOCHECK( m_bControlPoint1) ),
	SendPropInt( SENDINFO_NOCHECK( m_ControlPoint1.m_eParticleAttachment ), 5, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO_NOCHECK( m_ControlPoint1.m_vecOffset[0] ), -1, SPROP_COORD ),
	SendPropFloat( SENDINFO_NOCHECK( m_ControlPoint1.m_vecOffset[1] ), -1, SPROP_COORD ),
	SendPropFloat( SENDINFO_NOCHECK( m_ControlPoint1.m_vecOffset[2] ), -1, SPROP_COORD ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, const char *pszAttachmentName, bool bResetAllParticlesOnEntity )
{
	int iAttachment = -1;
	if ( pEntity && pEntity->GetBaseAnimating() )
	{
		// Find the attachment point index
		iAttachment = pEntity->GetBaseAnimating()->LookupAttachment( pszAttachmentName );
		if ( iAttachment <= 0 )
		{
			return;
		}
	}

	TE_TFParticleEffect( filter, flDelay, pszParticleName, iAttachType, pEntity, iAttachment, bResetAllParticlesOnEntity );
}

//-----------------------------------------------------------------------------
// Purpose: Yet another overload, lets us supply vecStart
//-----------------------------------------------------------------------------
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CBaseEntity *pEntity )
{
	int iIndex = GetParticleSystemIndex( pszParticleName );
	TE_TFParticleEffect( filter, flDelay, iIndex, vecOrigin, vecStart, vecAngles, pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, int iAttachmentPoint, bool bResetAllParticlesOnEntity  )
{
	g_TETFParticleEffect.Init();

	g_TETFParticleEffect.m_iParticleSystemIndex = GetParticleSystemIndex( pszParticleName );
	if ( pEntity )
	{
		g_TETFParticleEffect.m_nEntIndex = pEntity->entindex();
	}

	g_TETFParticleEffect.m_iAttachType = iAttachType;
	g_TETFParticleEffect.m_iAttachmentPointIndex = iAttachmentPoint;

	if ( bResetAllParticlesOnEntity )
	{
		g_TETFParticleEffect.m_bResetParticles = true;
	}

	// Send it over the wire
	g_TETFParticleEffect.Create( filter, flDelay );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity /*= NULL*/, ParticleAttachment_t eAttachType /*= PATTACH_CUSTOMORIGIN*/ )
{
	TE_TFParticleEffectComplex( filter, flDelay, pszParticleName, vecOrigin, vecAngles, NULL, NULL, pEntity, eAttachType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TE_TFParticleEffectComplex
(
	IRecipientFilter &filter,
	float flDelay,
	const char *pszParticleName,
	Vector vecOrigin,
	QAngle vecAngles,
	te_tf_particle_effects_colors_t *pOptionalColors /*= NULL*/,
	te_tf_particle_effects_control_point_t *pOptionalControlPoint1 /*= NULL*/,
	CBaseEntity *pEntity /*= NULL*/,
	ParticleAttachment_t eAttachType /*= PATTACH_CUSTOMORIGIN*/,
	Vector vecStart /* = vec3_origin */
)
{
	g_TETFParticleEffect.Init();

	g_TETFParticleEffect.m_iParticleSystemIndex = GetParticleSystemIndex( pszParticleName );

	VectorCopy( vecOrigin, g_TETFParticleEffect.m_vecOrigin );
	VectorCopy( vecAngles, g_TETFParticleEffect.m_vecAngles );
	VectorCopy( vecStart,  g_TETFParticleEffect.m_vecStart );

	if ( pEntity )
	{
		g_TETFParticleEffect.m_nEntIndex = pEntity->entindex();
		g_TETFParticleEffect.m_iAttachType = eAttachType;
	}

	if ( pOptionalColors )
	{
		g_TETFParticleEffect.m_bCustomColors = true;
		g_TETFParticleEffect.m_CustomColors.m_vecColor1 = pOptionalColors->m_vecColor1;
		g_TETFParticleEffect.m_CustomColors.m_vecColor2 = pOptionalColors->m_vecColor2;
	}

	if ( pOptionalControlPoint1 )
	{
		g_TETFParticleEffect.m_bControlPoint1 = true;
		g_TETFParticleEffect.m_ControlPoint1.m_eParticleAttachment = pOptionalControlPoint1->m_eParticleAttachment;
		g_TETFParticleEffect.m_ControlPoint1.m_vecOffset = pOptionalControlPoint1->m_vecOffset;
	}

	// Send it over the wire
	g_TETFParticleEffect.Create( filter, flDelay );

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, int iEffectIndex, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CBaseEntity *pEntity )
{
	g_TETFParticleEffect.Init();

	g_TETFParticleEffect.m_iParticleSystemIndex = iEffectIndex;

	VectorCopy( vecOrigin, g_TETFParticleEffect.m_vecOrigin );
	VectorCopy( vecStart, g_TETFParticleEffect.m_vecStart );
	VectorCopy( vecAngles, g_TETFParticleEffect.m_vecAngles );

	if ( pEntity )
	{
		g_TETFParticleEffect.m_nEntIndex = pEntity->entindex();
		g_TETFParticleEffect.m_iAttachType = PATTACH_CUSTOMORIGIN;
	}

	// Send it over the wire
	g_TETFParticleEffect.Create( filter, flDelay );
}