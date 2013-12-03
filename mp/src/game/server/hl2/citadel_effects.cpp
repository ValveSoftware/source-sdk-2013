//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "citadel_effects_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( env_citadel_energy_core, CCitadelEnergyCore );

BEGIN_DATADESC( CCitadelEnergyCore )
	DEFINE_KEYFIELD( m_flScale, FIELD_FLOAT, "scale" ),
	DEFINE_FIELD( m_nState, FIELD_INTEGER ),
	DEFINE_FIELD( m_flDuration, FIELD_FLOAT ),
	DEFINE_FIELD( m_flStartTime, FIELD_TIME ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "StartCharge", InputStartCharge ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartDischarge", InputStartDischarge ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Stop", InputStop ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CCitadelEnergyCore, DT_CitadelEnergyCore )
	SendPropFloat( SENDINFO(m_flScale), 0, SPROP_NOSCALE),
	SendPropInt( SENDINFO(m_nState), 8, SPROP_UNSIGNED),
	SendPropFloat( SENDINFO(m_flDuration), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flStartTime), 0, SPROP_NOSCALE),
	SendPropInt( SENDINFO(m_spawnflags), 0, SPROP_UNSIGNED),
END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Precache: 
//-----------------------------------------------------------------------------
void CCitadelEnergyCore::Precache()
{
	BaseClass::Precache();
	PrecacheMaterial( "effects/combinemuzzle2_dark" ); 
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCitadelEnergyCore::Spawn( void )
{
	Precache();

	UTIL_SetSize( this, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );

	// See if we start active
	if ( HasSpawnFlags( SF_ENERGYCORE_START_ON ) )
	{
		m_nState = (int)ENERGYCORE_STATE_DISCHARGING;
		m_flStartTime = gpGlobals->curtime;
	}

	// No model but we still need to force this!
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flWarmUpTime - 
//-----------------------------------------------------------------------------
void CCitadelEnergyCore::StartCharge( float flWarmUpTime )
{
	m_nState = (int)ENERGYCORE_STATE_CHARGING;
	m_flDuration = flWarmUpTime;
	m_flStartTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCitadelEnergyCore::StartDischarge( void )
{
	m_nState = (int)ENERGYCORE_STATE_DISCHARGING;
	m_flStartTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flCoolDownTime - 
//-----------------------------------------------------------------------------
void CCitadelEnergyCore::StopDischarge( float flCoolDownTime )
{
	m_nState = (int)ENERGYCORE_STATE_OFF;
	m_flDuration = flCoolDownTime;
	m_flStartTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CCitadelEnergyCore::InputStartCharge( inputdata_t &inputdata )
{
	StartCharge( inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CCitadelEnergyCore::InputStartDischarge( inputdata_t &inputdata )
{
	StartDischarge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CCitadelEnergyCore::InputStop( inputdata_t &inputdata )
{
	StopDischarge( inputdata.value.Float() );
}

CBaseViewModel *IsViewModelMoveParent( CBaseEntity *pEffect )
{
	if ( pEffect->GetMoveParent() )
	{
		CBaseViewModel *pViewModel = dynamic_cast<CBaseViewModel *>( pEffect->GetMoveParent() );

		if ( pViewModel )
		{
			return pViewModel;
		}
	}

	return NULL;
}

int CCitadelEnergyCore::UpdateTransmitState( void )
{
	if ( IsViewModelMoveParent( this ) )
	{
		return SetTransmitState( FL_EDICT_FULLCHECK );
	}

	return BaseClass::UpdateTransmitState();
}

int CCitadelEnergyCore::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	CBaseViewModel *pViewModel = IsViewModelMoveParent( this );

	if ( pViewModel )
	{
		return pViewModel->ShouldTransmit( pInfo );
	}

	return BaseClass::ShouldTransmit( pInfo );
}
