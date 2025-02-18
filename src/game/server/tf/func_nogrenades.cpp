//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF NoGrenades Zone.
//
//=============================================================================//

#include "cbase.h"
#include "tf_player.h"
#include "tf_item.h"
#include "func_nogrenades.h"

LINK_ENTITY_TO_CLASS( func_nogrenades, CNoGrenadesZone );

//=============================================================================
//
// CTF NoGrenades Zone functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNoGrenadesZone::CNoGrenadesZone()
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the entity
//-----------------------------------------------------------------------------
void CNoGrenadesZone::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
	InitTrigger();

	AddSpawnFlags( SF_TRIGGER_ALLOW_ALL ); // so we can keep track of who is touching us
	AddEffects( EF_NODRAW );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNoGrenadesZone::Precache( void )
{
	PrecacheModel( NOGRENADE_SPRITE );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified entity is touching this zone
//-----------------------------------------------------------------------------
bool CNoGrenadesZone::IsTouching( const CBaseEntity *pEntity ) const
{
	return BaseClass::IsTouching( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNoGrenadesZone::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNoGrenadesZone::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CNoGrenadesZone::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNoGrenadesZone::InputToggle( inputdata_t &inputdata )
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
void CNoGrenadesZone::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified entity is in a NoGrenades zone
//-----------------------------------------------------------------------------
bool InNoGrenadeZone( CBaseEntity *pEntity )
{
	if ( pEntity )
	{
		CBaseEntity *pTempEnt = NULL;
		while ( ( pTempEnt = gEntList.FindEntityByClassname( pTempEnt, "func_nogrenades" ) ) != NULL )
		{
			CNoGrenadesZone *pZone = dynamic_cast<CNoGrenadesZone *>( pTempEnt );

			if ( !pZone->IsDisabled() && pZone->PointIsWithin( pEntity->GetAbsOrigin() ) )
			{
				int iTeam = pZone->GetTeamNumber();
				if ( !iTeam || ( iTeam && ( pEntity->GetTeamNumber() == iTeam ) ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}
