//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base class for simple projectiles
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "cbasespriteprojectile.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( baseprojectile, CBaseSpriteProjectile );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CBaseSpriteProjectile )

	DEFINE_FIELD( m_iDmg,		FIELD_INTEGER ),
	DEFINE_FIELD( m_iDmgType,	FIELD_INTEGER ),
	DEFINE_FIELD( m_hIntendedTarget, FIELD_EHANDLE ),

END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------
void CBaseSpriteProjectile::Spawn(	char *pszModel,
								const Vector &vecOrigin,
								const Vector &vecVelocity,
								edict_t *pOwner,
								MoveType_t	iMovetype,
								MoveCollide_t nMoveCollide,
								int	iDamage,
								int iDamageType,
								CBaseEntity *pIntendedTarget )
{
	Precache();

	SetSolid( SOLID_BBOX );
	SetModel( pszModel );

	UTIL_SetSize( this, vec3_origin, vec3_origin );

	m_iDmg = iDamage;
	m_iDmgType = iDamageType;

	SetMoveType( iMovetype, nMoveCollide );

	UTIL_SetOrigin( this, vecOrigin );
	SetAbsVelocity( vecVelocity );

	SetOwnerEntity( Instance( pOwner ) );

	m_hIntendedTarget.Set( pIntendedTarget );

	// Call think for free the first time. It's up to derived classes to rethink.
	SetNextThink( gpGlobals->curtime );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBaseSpriteProjectile::Touch( CBaseEntity *pOther )
{
	HandleTouch( pOther );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBaseSpriteProjectile::HandleTouch( CBaseEntity *pOther )
{
	CBaseEntity *pOwner;

	pOwner = GetOwnerEntity();

	if( !pOwner )
	{
		pOwner = this;
	}

	trace_t	tr;
	tr = BaseClass::GetTouchTrace( );

	CTakeDamageInfo info( this, pOwner, m_iDmg, m_iDmgType );
	GuessDamageForce( &info, (tr.endpos - tr.startpos), tr.endpos );
	pOther->TakeDamage( info );
	
	UTIL_Remove( this );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBaseSpriteProjectile::Think()
{
	HandleThink();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBaseSpriteProjectile::HandleThink()
{
}

