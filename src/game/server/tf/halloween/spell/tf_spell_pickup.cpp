//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Spell
//
//=============================================================================//
#include "cbase.h"

#include "tf_spell_pickup.h"
#include "tf_player.h"
#include "halloween/tf_weapon_spellbook.h"
#include "tf_gamerules.h"

LINK_ENTITY_TO_CLASS( tf_spell_pickup, CSpellPickup );

BEGIN_DATADESC( CSpellPickup )

	// Keyfields.
	DEFINE_KEYFIELD( m_nTier, FIELD_INTEGER, "tier" ),

END_DATADESC();


//-----------------------------------------------------------------------------
CSpellPickup::CSpellPickup()
{
	m_nTier = 0;
}

//-----------------------------------------------------------------------------
void CSpellPickup::Spawn( void )
{
	BaseClass::Spawn();
	m_nSkin = m_nTier;
}

//-----------------------------------------------------------------------------
void CSpellPickup::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "Halloween.spell_pickup" );
	PrecacheScriptSound( "Halloween.spell_pickup_rare" );
}

//-----------------------------------------------------------------------------
bool CSpellPickup::MyTouch( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer )
	{
		CTFSpellBook *pSpellBook = dynamic_cast< CTFSpellBook* >( pTFPlayer->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
		if ( pSpellBook )
		{
			pSpellBook->RollNewSpell( m_nTier );

			CSingleUserRecipientFilter filter( pPlayer );
			const char *pszSoundName = ( m_nTier > 0 ) ? "Halloween.spell_pickup_rare" : "Halloween.spell_pickup";
			EmitSound( filter, entindex(), pszSoundName );
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
bool CSpellPickup::ItemCanBeTouchedByPlayer( CBasePlayer *pPlayer )
{
	if ( IsDisabled() )
		return false;

	// Dont let them pick up new spells if they already have a spell unless its a tier 1 spell
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer && m_nTier == 0 )
	{
		CTFSpellBook *pSpellBook = dynamic_cast< CTFSpellBook* >( pTFPlayer->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
		if ( !pSpellBook )
		{
			// TEMP
			ClientPrint( pPlayer, HUD_PRINTCENTER, "#TF_SpellBook_Equip", pPlayer->GetPlayerName() );
			return false;
		}
		
		if ( pSpellBook->HasASpellWithCharges() )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
const char *CSpellPickup::GetPowerupModel( void )
{
	if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
	{
		if ( m_nTier == 1 )
		{
			return "models/items/crystal_ball_pickup_major.mdl";
		}
		return "models/items/crystal_ball_pickup.mdl";
	}

	if ( m_nTier == 1 )
	{
		return "models/props_halloween/hwn_spellbook_upright_major.mdl";
	}

	return BaseClass::GetPowerupModel();
}
