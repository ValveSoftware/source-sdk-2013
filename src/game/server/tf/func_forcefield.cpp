//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "func_forcefield.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_AUTO_LIST( IFuncForceFieldAutoList );

//===========================================================================================================

LINK_ENTITY_TO_CLASS( func_forcefield, CFuncForceField );

BEGIN_DATADESC( CFuncForceField )
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CFuncForceField, DT_FuncForceField )
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncForceField::Spawn( void )
{
	BaseClass::Spawn();
	SetActive( IsOn() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CFuncForceField::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Only transmit this entity to clients that aren't in our team
//-----------------------------------------------------------------------------
int CFuncForceField::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	return FL_EDICT_ALWAYS;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncForceField::TurnOff( void )
{
	BaseClass::TurnOff();
	SetActive( false );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncForceField::TurnOn( void )
{
	BaseClass::TurnOn();
	SetActive( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncForceField::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	// Force fields are off during a team win
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
		return false;

	if ( GetTeamNumber() == TEAM_UNASSIGNED )
		return false;

	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		switch ( GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			if ( !( contentsMask & CONTENTS_BLUETEAM ) )
				return false;
			break;

		case TF_TEAM_RED:
			if ( !( contentsMask & CONTENTS_REDTEAM ) )
				return false;
			break;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncForceField::SetActive( bool bActive )
{
	if ( bActive )
	{
		// We're a trigger, but we want to be solid. Our ShouldCollide() will make
		// us non-solid to members of the team that spawns here.
		RemoveSolidFlags( FSOLID_TRIGGER );
		RemoveSolidFlags( FSOLID_NOT_SOLID );
	}
	else
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		AddSolidFlags( FSOLID_TRIGGER );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool PointsCrossForceField( const Vector& vecStart, const Vector &vecEnd, int nTeamToIgnore )
{
	// Setup the ray.
	Ray_t ray;
	ray.Init( vecStart, vecEnd );

	for ( int i = 0; i < IFuncForceFieldAutoList::AutoList().Count(); ++i )
	{
		CFuncForceField *pEntity = static_cast< CFuncForceField* >( IFuncForceFieldAutoList::AutoList()[i] );

		if ( pEntity->m_iDisabled )
			continue;

		if ( pEntity->GetTeamNumber() == nTeamToIgnore && nTeamToIgnore != TEAM_UNASSIGNED )
			continue;

		trace_t trace;
		enginetrace->ClipRayToEntity( ray, MASK_ALL, pEntity, &trace );
		if ( trace.fraction < 1.0f )
		{
			return true;
		}
	}

	return false;
}