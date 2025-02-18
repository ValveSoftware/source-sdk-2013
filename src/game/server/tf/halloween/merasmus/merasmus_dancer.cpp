//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tf_gamerules.h"
#include "merasmus_dancer.h"
#include "animation.h"

//-----------------------------------------------------------------------------

#include "tf_fx.h"

//-----------------------------------------------------------------------------

#define POOF_SOUND					"Halloween.Merasmus_Hiding_Explode"

//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( merasmus_dancer, CMerasmusDancer );

IMPLEMENT_SERVERCLASS_ST( CMerasmusDancer, DT_MerasmusDancer )
END_SEND_TABLE()

//-----------------------------------------------------------------------------

#define MERASMUS_MODEL_NAME						"models/bots/merasmus/merasmus.mdl"

//-----------------------------------------------------------------------------

CMerasmusDancer::CMerasmusDancer()
:	m_bEmitParticleEffect( false )
{
}

//-----------------------------------------------------------------------------

CMerasmusDancer::~CMerasmusDancer()
{
}

//-----------------------------------------------------------------------------

void CMerasmusDancer::Spawn()
{
	Precache();

	m_DieCountdownTimer.Invalidate();

	BaseClass::Spawn();

	SetModel( MERASMUS_MODEL_NAME );
	UseClientSideAnimation();

	SetThink( &CMerasmusDancer::DanceThink );
	SetNextThink( gpGlobals->curtime );

	m_bEmitParticleEffect = true;
}

//-----------------------------------------------------------------------------

void CMerasmusDancer::PlaySequence( const char *pSeqName )
{
	int iAnimSequence = LookupSequence( pSeqName );	// dance animation
	if ( iAnimSequence )
	{
		SetSequence( iAnimSequence );
		SetPlaybackRate( 1.0f );
		SetCycle( 0 );
		ResetSequenceInfo();

		HideStaff();
	}
}

//-----------------------------------------------------------------------------

void CMerasmusDancer::PlayActivity( int iActivity )
{
	int iAnimSequence = ::SelectWeightedSequence( GetModelPtr(), iActivity, GetSequence() );
	if ( iAnimSequence )
	{
		SetSequence( iAnimSequence );
		SetPlaybackRate( 1.0f );
		SetCycle( 0 );
		ResetSequenceInfo();

		HideStaff();
	}
}

//-----------------------------------------------------------------------------

void CMerasmusDancer::HideStaff()
{
	int nStaffBodyGroup = FindBodygroupByName( "staff" );
	SetBodygroup( nStaffBodyGroup, 2 );
}

//-----------------------------------------------------------------------------

void CMerasmusDancer::Dance()
{
	PlaySequence( "taunt06" );
}

//-----------------------------------------------------------------------------

void CMerasmusDancer::Vanish()
{
	m_bEmitParticleEffect = true;
	m_DieCountdownTimer.Start( 0.0f );
}

//-----------------------------------------------------------------------------

void CMerasmusDancer::BlastOff()
{
	m_bEmitParticleEffect = true;
	m_DieCountdownTimer.Start( 0.3f );

	PlayActivity( ACT_FLY );
}

//-----------------------------------------------------------------------------

void CMerasmusDancer::Precache()
{
	BaseClass::Precache();

	int model = PrecacheModel( MERASMUS_MODEL_NAME );
	PrecacheGibsForModel( model );
	PrecacheParticleSystem( "merasmus_tp" ); // puff effect

	PrecacheScriptSound( POOF_SOUND );

	// We deliberately allow late precaches here.
	bool bAllowPrecache = CBaseAnimating::IsPrecacheAllowed();
	CBaseAnimating::SetAllowPrecache( bAllowPrecache );
}

//-----------------------------------------------------------------------------

bool CMerasmusDancer::ShouldDelete() const
{
	return m_DieCountdownTimer.HasStarted() && m_DieCountdownTimer.IsElapsed();
}

//-----------------------------------------------------------------------------

void CMerasmusDancer::DanceThink()
{
	// Emit the initial effect here, rather than in Spawn(), since GetAbsOrigin() and GetAbsAngles() don't return useful values then.
	if ( m_bEmitParticleEffect )
	{
		DispatchParticleEffect( "merasmus_tp", GetAbsOrigin(), GetAbsAngles() );
		m_bEmitParticleEffect = false;
		EmitSound( POOF_SOUND );
	}

	if ( ShouldDelete() )
	{
		EmitSound( POOF_SOUND );
		UTIL_Remove( this );
		return;
	}
	
	SetNextThink( gpGlobals->curtime );
}
