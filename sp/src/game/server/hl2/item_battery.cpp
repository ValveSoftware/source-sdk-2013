//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Handling for the suit batteries.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"
#include "basecombatweapon.h"
#include "gamerules.h"
#include "items.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CItemBattery : public CItem
{
public:
	DECLARE_CLASS( CItemBattery, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( DefaultOrCustomModel( "models/items/battery.mdl" ) );
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		PrecacheModel( DefaultOrCustomModel( "models/items/battery.mdl" ) );

		PrecacheScriptSound( "ItemBattery.Touch" );

	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player *>( pPlayer );
#ifdef MAPBASE
		return ( pHL2Player && pHL2Player->ApplyBattery( m_flPowerMultiplier ) );
#else
		return ( pHL2Player && pHL2Player->ApplyBattery() );
#endif
	}

#ifdef MAPBASE
	void	InputSetPowerMultiplier( inputdata_t &inputdata ) { m_flPowerMultiplier = inputdata.value.Float(); }
	float	m_flPowerMultiplier = 1.0f;

	DECLARE_DATADESC();
#endif
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);
PRECACHE_REGISTER(item_battery);

#ifdef MAPBASE
BEGIN_DATADESC( CItemBattery )

	DEFINE_KEYFIELD( m_flPowerMultiplier, FIELD_FLOAT, "PowerMultiplier" ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetPowerMultiplier", InputSetPowerMultiplier ),

END_DATADESC()
#endif

