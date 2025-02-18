//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "filesystem.h"
#include "func_croc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( entity_croc, CEntityCroc );

LINK_ENTITY_TO_CLASS( func_croc, CFuncCroc );

BEGIN_DATADESC( CFuncCroc )
	DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "croc_model" ),
	// Outputs
	DEFINE_OUTPUT( m_OnEat, "OnEat" ),
	DEFINE_OUTPUT( m_OnEatRed, "OnEatRed" ),
	DEFINE_OUTPUT( m_OnEatBlue, "OnEatBlue" ),
END_DATADESC()

PRECACHE_REGISTER( func_croc );

#define CROC_MODEL "models/props_island/crocodile/crocodile.mdl"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEntityCroc::CEntityCroc()
{
	m_hTarget = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntityCroc::InitCroc( CBaseEntity *pOther, const char *pszModel )
{
	float flCrocDeath = 0.f;

	SetModel( pszModel );

	SetThink( &CEntityCroc::Think );
	SetNextThink( gpGlobals->curtime );

	int animSequence = LookupSequence( "attack" );
	if ( animSequence != ACT_INVALID )
	{
		SetSequence( animSequence );
		SetPlaybackRate( 1.0f );
		SetCycle( 0 );
		ResetSequenceInfo();

		flCrocDeath = SequenceDuration( animSequence );
	}

	if ( pOther && pOther->IsPlayer() && pOther->IsAlive() )
	{
		m_hTarget = dynamic_cast<CTFPlayer*>( pOther );
		SetContextThink( &CEntityCroc::CrocAttack, gpGlobals->curtime + 0.1f, "CrocAttack" );
	}

	SetContextThink( &CEntityCroc::SUB_Remove, gpGlobals->curtime + flCrocDeath, "RemoveThink" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntityCroc::CrocAttack( void )
{
	if ( m_hTarget && GetOwnerEntity() )
	{
		// make sure they're still touching the trigger
		CFuncCroc *pFuncCroc = dynamic_cast< CFuncCroc* >( GetOwnerEntity() );
		if ( pFuncCroc && pFuncCroc->IsTouching( m_hTarget ) )
		{
			m_hTarget->TakeDamage( CTakeDamageInfo( GetOwnerEntity(), GetOwnerEntity(), 1000, DMG_CRUSH, TF_DMG_CUSTOM_CROC ) );
			pFuncCroc->FireOutputs( m_hTarget.Get() );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CEntityCroc::Think( void )
{
	StudioFrameAdvance();
	DispatchAnimEvents( this );

	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncCroc::CFuncCroc()
{
	m_iszModel = NULL_STRING;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncCroc::Spawn( void )
{
	AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );

	BaseClass::Spawn();
	InitTrigger();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncCroc::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( GetCrocModel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CFuncCroc::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CFuncCroc::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	return FL_EDICT_ALWAYS;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncCroc::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( GetTeamNumber() == TEAM_UNASSIGNED )
		return false;

	if ( collisionGroup != COLLISION_GROUP_PLAYER_MOVEMENT )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncCroc::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	if ( PassesTriggerFilters( pOther ) )
	{
		CEntityCroc *pCroc = ( CEntityCroc* ) CBaseEntity::Create( "entity_croc", pOther->GetAbsOrigin(), vec3_angle, this );
		if ( pCroc )
		{
			pCroc->InitCroc( pOther, GetCrocModel() );
			pCroc->Activate();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncCroc::FireOutputs( CTFPlayer *pTFPlayer )
{
	if ( !pTFPlayer )
		return;

	m_OnEat.FireOutput( this, this );

	if ( pTFPlayer->GetTeamNumber() == TF_TEAM_RED )
	{
		m_OnEatRed.FireOutput( this, this );
	}
	else if ( pTFPlayer->GetTeamNumber() == TF_TEAM_BLUE )
	{
		m_OnEatBlue.FireOutput( this, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CFuncCroc::GetCrocModel( void )
{
	if ( m_iszModel != NULL_STRING )
	{
		if ( g_pFullFileSystem->FileExists( STRING( m_iszModel ), "GAME" ) )
		{
			return ( STRING( m_iszModel ) );
		}
	}

	return CROC_MODEL;
}