//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A clientside, visual only model that's positioned relative to players
//
//=============================================================================

#include "cbase.h"
#include "c_playerrelativemodel.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PlayerRelativeModel *C_PlayerRelativeModel::Create( const char *pszModelName, C_BaseEntity *pParent, Vector vecOffset, QAngle angleOffset, float flAnimSpeed, float flLifetime, int iFlags )
{
	C_PlayerRelativeModel *pFlash = new C_PlayerRelativeModel;
	if ( !pFlash )
		return NULL;

	if ( !pFlash->Initialize( pszModelName, pParent, vecOffset, angleOffset, flAnimSpeed, flLifetime, iFlags ) )
		return NULL;

	return pFlash;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_PlayerRelativeModel::Initialize( const char *pszModelName, C_BaseEntity *pParent, Vector vecOffset, QAngle angleOffset, float flAnimSpeed, float flLifetime, int iFlags )
{
	AddEffects( EF_NORECEIVESHADOW | EF_NOSHADOW );
	if ( InitializeAsClientEntity( pszModelName, RENDER_GROUP_OPAQUE_ENTITY ) == false )
	{
		Release();
		return false;
	}

	m_vecOffsetPos = vecOffset;
	m_angleOffset = angleOffset;

	SetParent( pParent, 0 ); 
	SetLocalOrigin( vec3_origin );
	SetLocalAngles( vec3_angle );

	AddSolidFlags( FSOLID_NOT_SOLID );

	SetLifetime( flLifetime );
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	SetCycle( 0 );

	m_qOffsetRotation = vec3_angle;
	m_flAnimSpeed = flAnimSpeed;
	
	m_iFlags = iFlags;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PlayerRelativeModel::SetLifetime( float flLifetime )
{
	if ( flLifetime == PRM_PERMANENT )
	{
		m_flExpiresAt = PRM_PERMANENT;
	}
	else
	{
		// Expire when the lifetime is up
		m_flExpiresAt = gpGlobals->curtime + flLifetime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PlayerRelativeModel::ClientThink( void )
{
	if ( !GetMoveParent() || (m_flExpiresAt != PRM_PERMANENT && gpGlobals->curtime > m_flExpiresAt) )
	{
		Release();
		return;
	}

	// Animate 
	C_BaseEntity *pParent = GetMoveParent();

	Vector out(0, 0, 0);
	if ( m_iFlags & PRM_SPIN_Z )
	{
		m_qOffsetRotation += QAngle(0, gpGlobals->frametime * m_flAnimSpeed, 0);
		VectorRotate( m_vecOffsetPos, m_qOffsetRotation, out );
	}

	SetAbsOrigin( pParent->GetAbsOrigin() + out );
	SetAbsAngles( m_qOffsetRotation + m_angleOffset );
}

//-----------------------------------------------------------------------------
// C_MerasmusBombEffect
//-----------------------------------------------------------------------------
C_MerasmusBombEffect *C_MerasmusBombEffect::Create( const char *pszModelName, C_TFPlayer *pParent, Vector vecOffset, QAngle angleOffset, float flAnimSpeed, float flLifetime, int iFlags )
{
	C_MerasmusBombEffect *pFlash = new C_MerasmusBombEffect;
	if ( !pFlash )
		return NULL;

	if ( !pFlash->Initialize( pszModelName, pParent, vecOffset, angleOffset, flAnimSpeed, flLifetime, iFlags ) )
		return NULL;

	return pFlash;
}

//-----------------------------------------------------------------------------
bool C_MerasmusBombEffect::Initialize( const char *pszModelName, C_TFPlayer *pParent, Vector vecOffset, QAngle angleOffset, float flAnimSpeed, float flLifetime, int iFlags )
{
	if ( !BaseClass::Initialize(pszModelName, pParent, vecOffset, angleOffset, flAnimSpeed, flLifetime, iFlags ) )
		return false;

	// Create a particle effect 
	const char *pszEffectName = "bombonomicon_spell_trail";

	if ( m_pBombonomiconBeam )
	{
		m_pBombonomiconBeam->StopEmission();
		m_pBombonomiconBeam = NULL;
	}

	if ( m_pBombonomiconEffect )
	{
		m_pBombonomiconEffect->StopEmission();
		m_pBombonomiconEffect = NULL;
	}

	m_pBombonomiconBeam = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW, INVALID_PARTICLE_ATTACHMENT, Vector(0,0,-10) );
	if ( m_pBombonomiconBeam )
	{
		ParticleProp()->AddControlPoint( m_pBombonomiconBeam, 1, pParent, PATTACH_POINT_FOLLOW, "head", Vector(0,0,0) );
	}

	return true;
}


//-----------------------------------------------------------------------------
void C_MerasmusBombEffect::ClientThink( void )
{
	if ( !GetMoveParent() || (m_flExpiresAt != PRM_PERMANENT && gpGlobals->curtime > m_flExpiresAt) )
	{
		if ( m_pBombonomiconBeam )
		{
			m_pBombonomiconBeam->StopEmission();
			m_pBombonomiconBeam = NULL;
		}

		if ( m_pBombonomiconEffect )
		{
			m_pBombonomiconEffect->StopEmission();
			m_pBombonomiconEffect = NULL;
		}
	}

	BaseClass::ClientThink();
}