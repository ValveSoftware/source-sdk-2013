//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF ChangeClass Zone.
//
//=============================================================================//

#include "cbase.h"
#include "viewport_panel_names.h"
#include "tf_player.h"
#include "tf_item.h"
#include "tf_team.h"
#include "func_changeclass.h"

LINK_ENTITY_TO_CLASS( func_changeclass, CChangeClassZone );

#define TF_CHANGECLASS_SOUND				"ChangeClass.Touch"
#define TF_CHANGECLASS_NEXT_USE_TIME		10.0f

//=============================================================================
//
// CTF ChangeClass Zone functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChangeClassZone::CChangeClassZone()
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the entity
//-----------------------------------------------------------------------------
void CChangeClassZone::Spawn( void )
{
	Precache();
	InitTrigger();
	SetTouch( &CChangeClassZone::Touch );
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the entity
//-----------------------------------------------------------------------------
void CChangeClassZone::Precache( void )
{
	PrecacheScriptSound( TF_CHANGECLASS_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChangeClassZone::Touch( CBaseEntity *pOther )
{
	if ( !IsDisabled() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pOther );
		if ( pPlayer )
		{
			if ( pPlayer->GetNextChangeClassTime() > gpGlobals->curtime )
				return;

			int iTeam = GetTeamNumber();
			if ( iTeam && ( pPlayer->GetTeamNumber() != iTeam ) )
				return;

			// bring up the player's changeclass menu
			CCommand args;
			args.Tokenize( "changeclass" );
			pPlayer->ClientCommand( args );
			pPlayer->SetNextChangeClassTime( gpGlobals->curtime + TF_CHANGECLASS_NEXT_USE_TIME );

			CPASAttenuationFilter filter( pOther, TF_CHANGECLASS_SOUND );
			EmitSound( filter, pOther->entindex(), TF_CHANGECLASS_SOUND );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChangeClassZone::EndTouch( CBaseEntity *pOther )
{

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CChangeClassZone::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CChangeClassZone::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CChangeClassZone::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CChangeClassZone::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		SetDisabled( false );
	}
	else
	{
		SetDisabled( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CChangeClassZone::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;
}
