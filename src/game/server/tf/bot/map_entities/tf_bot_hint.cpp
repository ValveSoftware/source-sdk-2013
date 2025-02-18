//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_hint.cpp
// Designer-placed hint for TFBots

#include "cbase.h"
#include "bot/tf_bot.h"
#include "tf_bot_hint.h"

BEGIN_DATADESC( CTFBotHint )
	DEFINE_KEYFIELD( m_team, FIELD_INTEGER, "team" ),
	DEFINE_KEYFIELD( m_hint, FIELD_INTEGER, "hint" ),
	DEFINE_KEYFIELD( m_isDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_tfbot_hint, CTFBotHint );

//
// NOTE: For simplicity and runtime efficiency, this will not 
// play nice with nav area hints stored in the mesh,
// nor will overlapping hints of the same type work well.
//

//------------------------------------------------------------------------------
CTFBotHint::CTFBotHint( void )
{
	m_isDisabled = false;
}


//--------------------------------------------------------------------------------------------------------
// Return true if this hint applies to the given entity
bool CTFBotHint::IsFor( CTFBot *who ) const
{
	if ( m_isDisabled )
	{
		return false;
	}

	if ( m_team > 0 && who->GetTeamNumber() != m_team )
	{
		return false;		
	}

	return true;
}


//--------------------------------------------------------------------------------------------------------
void CTFBotHint::Spawn( void )
{
	BaseClass::Spawn();

	SetSolid( SOLID_BSP );	
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );
	AddEffects( EF_NODRAW );
	SetCollisionGroup( COLLISION_GROUP_NONE );

	VPhysicsInitShadow( false, false );

	UpdateNavDecoration();
}


//--------------------------------------------------------------------------------------------------------
void CTFBotHint::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

	UpdateNavDecoration();
}


//--------------------------------------------------------------------------------------------------------
void CTFBotHint::InputEnable( inputdata_t &inputdata )
{
	m_isDisabled = false;
	UpdateNavDecoration();
}


//--------------------------------------------------------------------------------------------------------
void CTFBotHint::InputDisable( inputdata_t &inputdata )
{
	m_isDisabled = true;
	UpdateNavDecoration();
}


//--------------------------------------------------------------------------------------------------------
void CTFBotHint::UpdateNavDecoration( void )
{
	Extent extent;
	extent.Init( this );

	CUtlVector< CTFNavArea * > overlapVector;
	TheNavMesh->CollectAreasOverlappingExtent( extent, &overlapVector );

	int attributeBits = 0;
	switch( m_hint )
	{
	case HINT_SNIPER_SPOT:
		attributeBits = TF_NAV_SNIPER_SPOT;
		break;

	case HINT_SENTRY_SPOT:
		attributeBits = TF_NAV_SENTRY_SPOT;
		break;
	}

	for( int j=0; j<overlapVector.Count(); ++j )
	{
		if ( m_isDisabled )
		{
			overlapVector[j]->ClearAttributeTF( attributeBits );
		}
		else
		{
			overlapVector[j]->SetAttributeTF( attributeBits );
		}
	}
}


