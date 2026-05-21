//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "ow_player_system.h"
#include "ow_hero_registry.h"
#include "ow_shareddefs.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_weaponbase.h"
#include "tf_obj_sentrygun.h"
#include "func_respawnroom.h"
#include "tf_player_shared.h"
#include "tf_item_inventory.h"
#include "bot/tf_bot.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_ow_mode;
extern ConVar tf_ow_hero_lock;
extern ConVar tf_ow_bots_per_team;
extern ConVar tf_ow_enemy_bonus_on_join;

//-----------------------------------------------------------------------------
void OW_PlayerSystem_Init( void )
{
	COWHeroRegistry::Instance().Reload();
}

//-----------------------------------------------------------------------------
void OW_SetPlayerHero( CTFPlayer *pPlayer, int iHeroId )
{
	if ( !pPlayer || !TFGameRules() || !TFGameRules()->IsOverwatchMode() )
	{
		return;
	}

	const OWHeroDefinition_t *pHero = COWHeroRegistry::Instance().GetHeroById( iHeroId );
	if ( !pHero )
	{
		return;
	}

	pPlayer->m_iOWHeroId = iHeroId;
	if ( pPlayer->GetPlayerClass()->GetClassIndex() != pHero->m_iTFClass )
	{
		pPlayer->SetDesiredPlayerClassIndex( pHero->m_iTFClass );
	}
}

//-----------------------------------------------------------------------------
void OW_ApplyHeroFromClass( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !TFGameRules() || !TFGameRules()->IsOverwatchMode() )
	{
		return;
	}

	const int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	const OWHeroDefinition_t *pHero = COWHeroRegistry::Instance().GetHeroByTFClass( iClass );
	if ( !pHero )
	{
		return;
	}

	pPlayer->m_iOWHeroId = pHero->m_iHeroId;
}

//-----------------------------------------------------------------------------
static bool OW_GiveWeaponByEntName( CTFPlayer *pPlayer, const char *pszWeaponEnt )
{
	if ( !pPlayer || !pszWeaponEnt || !pszWeaponEnt[0] )
	{
		return false;
	}

	const int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	const char *pszTranslated = TranslateWeaponEntForClass( pszWeaponEnt, iClass );
	const char *pszGive = pszTranslated ? pszTranslated : pszWeaponEnt;

	if ( dynamic_cast<CTFWeaponBase *>( pPlayer->GiveNamedItem( pszGive, 0, NULL, true ) ) )
	{
		return true;
	}

	CTFInventoryManager *pInvMgr = TFInventoryManager();
	if ( !pInvMgr )
	{
		return false;
	}

	for ( int iSlot = 0; iSlot < CLASS_LOADOUT_POSITION_COUNT; ++iSlot )
	{
		CEconItemView *pItem = pInvMgr->GetBaseItemForClass( iClass, iSlot );
		if ( !pItem || !pItem->GetStaticData() )
		{
			continue;
		}

		const char *pszClass = pItem->GetStaticData()->GetItemClass();
		if ( !pszClass )
		{
			continue;
		}

		if ( FStrEq( pszClass, pszWeaponEnt ) || ( pszTranslated && FStrEq( pszClass, pszTranslated ) ) )
		{
			if ( dynamic_cast<CTFWeaponBase *>( pPlayer->GiveNamedItem( pszClass, 0, pItem, true ) ) )
			{
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
static void OW_GiveStockClassWeapons( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
	{
		return;
	}

	CTFInventoryManager *pInvMgr = TFInventoryManager();
	if ( !pInvMgr )
	{
		return;
	}

	const int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	static const int s_WeaponSlots[] = { LOADOUT_POSITION_PRIMARY, LOADOUT_POSITION_SECONDARY, LOADOUT_POSITION_MELEE };

	for ( int i = 0; i < ARRAYSIZE( s_WeaponSlots ); ++i )
	{
		CEconItemView *pItem = pInvMgr->GetBaseItemForClass( iClass, s_WeaponSlots[i] );
		if ( pItem && pItem->GetStaticData() && pItem->GetStaticData()->GetItemClass() )
		{
			pPlayer->GiveNamedItem( pItem->GetStaticData()->GetItemClass(), 0, pItem, true );
		}
	}
}

//-----------------------------------------------------------------------------
static void OW_ApplyHeroWeapons( CTFPlayer *pPlayer, const OWHeroDefinition_t *pHero )
{
	if ( !pPlayer || !pHero )
	{
		return;
	}

	for ( int i = 0; i < MAX_WEAPONS; ++i )
	{
		CTFWeaponBase *pWpn = assert_cast<CTFWeaponBase *>( pPlayer->GetWeapon( i ) );
		if ( pWpn )
		{
			pPlayer->Weapon_Detach( pWpn );
			UTIL_Remove( pWpn );
		}
	}

	OW_GiveWeaponByEntName( pPlayer, pHero->m_szPrimaryWeapon );
	OW_GiveWeaponByEntName( pPlayer, pHero->m_szSecondaryWeapon );
	OW_GiveWeaponByEntName( pPlayer, pHero->m_szMeleeWeapon );

	if ( pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_ENGINEER )
	{
		TFPlayerClassData_t *pData = pPlayer->GetPlayerClass()->GetData();
		if ( pData )
		{
			pPlayer->ManageBuilderWeapons( pData );
		}
	}

	CTFWeaponBase *pActive = pPlayer->Weapon_GetWeaponByType( TF_WPN_TYPE_PRIMARY );
	if ( !pActive )
	{
		pActive = pPlayer->Weapon_GetWeaponByType( TF_WPN_TYPE_SECONDARY );
	}
	if ( !pActive )
	{
		pActive = pPlayer->Weapon_GetWeaponByType( TF_WPN_TYPE_MELEE );
	}
	if ( pActive )
	{
		pPlayer->Weapon_Switch( pActive );
	}
	else
	{
		OW_GiveStockClassWeapons( pPlayer );
		pActive = pPlayer->Weapon_GetWeaponByType( TF_WPN_TYPE_PRIMARY );
		if ( !pActive )
		{
			pActive = pPlayer->Weapon_GetWeaponByType( TF_WPN_TYPE_MELEE );
		}
		if ( pActive )
		{
			pPlayer->Weapon_Switch( pActive );
		}
	}
}

//-----------------------------------------------------------------------------
void OW_OnPlayerSpawn( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !TFGameRules() || !TFGameRules()->IsOverwatchMode() )
	{
		return;
	}

	OW_ApplyHeroFromClass( pPlayer );

	const OWHeroDefinition_t *pHero = COWHeroRegistry::Instance().GetHeroById( pPlayer->m_iOWHeroId );
	if ( !pHero )
	{
		return;
	}

	pPlayer->m_bOWHeroLocked = false;
	pPlayer->m_flOWUltCharge = 0.0f;
	for ( int i = 0; i < 3; ++i )
	{
		pPlayer->m_flOWCooldownEnd[i] = 0.0f;
	}

	const int iMaxHealth = Max( 1, pHero->m_iMaxHealth );
	pPlayer->SetMaxHealth( iMaxHealth );
	pPlayer->SetHealth( iMaxHealth );

	const float flBaseSpeed = pPlayer->MaxSpeed();
	if ( flBaseSpeed > 0.0f && pHero->m_flMoveSpeedScale > 0.0f )
	{
		pPlayer->SetMaxSpeed( flBaseSpeed * pHero->m_flMoveSpeedScale );
	}

	OW_ApplyHeroWeapons( pPlayer, pHero );

	// Per-hero spawn tweaks
	switch ( pHero->m_iHeroId )
	{
	case OW_HERO_TANK:
		pPlayer->m_Shared.AddCond( TF_COND_DEFENSEBUFF, 600.0f );
		break;
	case OW_HERO_SKIRMISHER:
		break;
	default:
		break;
	}

	if ( !pPlayer->IsBot() )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, CFmtStr( "OW: %s — Shift/E/Q abilities, ultimate on R", pHero->m_szName ) );
	}
}

//-----------------------------------------------------------------------------
void OW_OnPlayerPostThink( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsAlive() || !TFGameRules() || !TFGameRules()->IsOverwatchMode() )
	{
		return;
	}

	if ( tf_ow_hero_lock.GetBool() )
	{
		const bool bInSpawn = PointInRespawnRoom( pPlayer, pPlayer->WorldSpaceCenter() );
		if ( bInSpawn )
		{
			pPlayer->m_bOWHeroLocked = false;
		}
		else if ( !pPlayer->m_bOWHeroLocked )
		{
			pPlayer->m_bOWHeroLocked = true;
		}
	}

	// Passive ult charge over time while alive in combat zones
	OW_TickUltCharge( pPlayer, 0.0f, 0.0f, 0.0f );
}

//-----------------------------------------------------------------------------
static bool OW_IsAbilityReady( CTFPlayer *pPlayer, int iSlot )
{
	return pPlayer && iSlot >= 0 && iSlot < 3 && gpGlobals->curtime >= pPlayer->m_flOWCooldownEnd[iSlot];
}

//-----------------------------------------------------------------------------
bool OW_UseAbility( CTFPlayer *pPlayer, int iSlot )
{
	if ( !pPlayer || !pPlayer->IsAlive() || !TFGameRules() || !TFGameRules()->IsOverwatchMode() )
	{
		return false;
	}

	if ( iSlot < 0 || iSlot >= 3 )
	{
		return false;
	}

	if ( !OW_IsAbilityReady( pPlayer, iSlot ) )
	{
		return false;
	}

	const OWHeroDefinition_t *pHero = COWHeroRegistry::Instance().GetHeroById( pPlayer->m_iOWHeroId );
	if ( !pHero )
	{
		return false;
	}

	const float flDuration = pHero->m_flAbilityDuration[iSlot];
	const int iType = pHero->m_iAbilityType[iSlot];

	switch ( iType )
	{
	case OW_ABILITY_HEAL_AURA:
		pPlayer->SetHealth( Min( pPlayer->GetMaxHealth(), pPlayer->GetHealth() + 40 ) );
		break;
	case OW_ABILITY_DAMAGE_BOOST:
		pPlayer->m_Shared.AddCond( TF_COND_OFFENSEBUFF, flDuration );
		break;
	case OW_ABILITY_SHIELD:
		pPlayer->m_Shared.AddCond( TF_COND_DEFENSEBUFF, flDuration );
		break;
	case OW_ABILITY_COND:
	default:
		pPlayer->m_Shared.AddCond( (ETFCond)pHero->m_iAbilityCond[iSlot], flDuration );
		break;
	}

	const float flEnd = gpGlobals->curtime + pHero->m_flAbilityCooldown[iSlot];
	pPlayer->m_flOWCooldownEnd[iSlot] = flEnd;
	pPlayer->m_flOWCooldown0 = pPlayer->m_flOWCooldownEnd[0];
	pPlayer->m_flOWCooldown1 = pPlayer->m_flOWCooldownEnd[1];
	pPlayer->m_flOWCooldown2 = pPlayer->m_flOWCooldownEnd[2];
	pPlayer->EmitSound( "Player.DenyWeaponSelection" );
	return true;
}

//-----------------------------------------------------------------------------
bool OW_UseUltimate( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsAlive() || !TFGameRules() || !TFGameRules()->IsOverwatchMode() )
	{
		return false;
	}

	if ( pPlayer->m_flOWUltCharge < OW_ULT_CHARGE_MAX )
	{
		return false;
	}

	const OWHeroDefinition_t *pHero = COWHeroRegistry::Instance().GetHeroById( pPlayer->m_iOWHeroId );
	if ( !pHero )
	{
		return false;
	}

	const float flDuration = pHero->m_flUltDuration;

	switch ( pHero->m_iUltType )
	{
	case OW_ABILITY_ULT_HEAL:
		pPlayer->m_Shared.AddCond( TF_COND_MEGAHEAL, flDuration );
		break;
	case OW_ABILITY_ULT_SNIPER:
		pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED, flDuration );
		break;
	case OW_ABILITY_ULT_RAGE:
	default:
		pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED, flDuration );
		pPlayer->m_Shared.AddCond( TF_COND_DEFENSEBUFF, flDuration );
		break;
	}

	pPlayer->m_flOWUltCharge = 0.0f;
	UTIL_ClientPrintAll( HUD_PRINTTALK, CFmtStr( "OW: %s activated ultimate!", pHero->m_szName ) );
	return true;
}

//-----------------------------------------------------------------------------
void OW_TickUltCharge( CTFPlayer *pPlayer, float flDamageDealt, float flDamageTaken, float flHealing )
{
	if ( !pPlayer || !pPlayer->IsAlive() )
	{
		return;
	}

	const OWHeroDefinition_t *pHero = COWHeroRegistry::Instance().GetHeroById( pPlayer->m_iOWHeroId );
	const float flRate = pHero ? pHero->m_flUltChargeRate : 1.0f;

	float flGain = gpGlobals->frametime * 2.0f * flRate;
	flGain += flDamageDealt * 0.08f;
	flGain += flDamageTaken * 0.05f;
	flGain += flHealing * 0.06f;

	pPlayer->m_flOWUltCharge = Min( OW_ULT_CHARGE_MAX, pPlayer->m_flOWUltCharge + flGain );
}

//-----------------------------------------------------------------------------
void OW_PrintPlayerStatus( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
	{
		return;
	}

	const OWHeroDefinition_t *pHero = COWHeroRegistry::Instance().GetHeroById( pPlayer->m_iOWHeroId );
	Msg( "OW player %s: hero=%s ult=%.0f%% locked=%d\n",
		pPlayer->GetPlayerName(),
		pHero ? pHero->m_szName : "?",
		pPlayer->m_flOWUltCharge,
		pPlayer->m_bOWHeroLocked ? 1 : 0 );
}

//-----------------------------------------------------------------------------
void OW_EnsureAllBotsHaveAI( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsOverwatchMode() )
	{
		return;
	}

	static bool s_bOWResetMissionsAfterSetup = false;

	if ( TFGameRules()->InSetup() )
	{
		s_bOWResetMissionsAfterSetup = false;

		// Idle during setup — prevents wall-flying from seek-and-destroy pathing.
		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CTFBot *pBot = dynamic_cast<CTFBot *>( UTIL_PlayerByIndex( i ) );
			if ( !pBot || !pBot->IsBot() )
			{
				continue;
			}

			if ( pBot->GetMission() != CTFBot::NO_MISSION )
			{
				pBot->SetMission( CTFBot::NO_MISSION, false );
			}
		}
		return;
	}

	// Once per round after setup: drop any forced mission so scenario monitor uses stock PL/KOTH/CTF AI.
	if ( !s_bOWResetMissionsAfterSetup && TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		s_bOWResetMissionsAfterSetup = true;

		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CTFBot *pBot = dynamic_cast<CTFBot *>( UTIL_PlayerByIndex( i ) );
			if ( !pBot || !pBot->IsBot() )
			{
				continue;
			}

			pBot->SetMission( CTFBot::NO_MISSION, false );
			pBot->GetIntentionInterface()->Reset();
		}
	}
}

//-----------------------------------------------------------------------------
void OW_OnHumanChangedTeam( CTFPlayer *pPlayer, int iNewTeam, int iOldTeam )
{
	if ( !pPlayer || pPlayer->IsBot() || !TFGameRules() || !TFGameRules()->IsOverwatchMode() )
	{
		return;
	}

	if ( iNewTeam != TF_TEAM_RED && iNewTeam != TF_TEAM_BLUE )
	{
		return;
	}

	if ( iOldTeam == iNewTeam )
	{
		return;
	}

	extern void OW_MaintainBotCounts( void );
	extern void OW_TickBotSpawnQueue( void );
	OW_MaintainBotCounts();
	OW_TickBotSpawnQueue();
}

//-----------------------------------------------------------------------------
static void CC_OW_SelectHero( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "ow_hero <id|name> — ids: skirmisher tank bruiser sniper healer builder\n" );
		COWHeroRegistry::Instance().PrintRoster();
		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
	{
		return;
	}

	const char *pszArg = args[1];
	int iHeroId = atoi( pszArg );
	if ( iHeroId <= 0 && pszArg[0] )
	{
		for ( int i = 0; i < COWHeroRegistry::Instance().GetHeroCount(); ++i )
		{
			const OWHeroDefinition_t *pH = COWHeroRegistry::Instance().GetHeroByIndex( i );
			if ( pH && Q_stristr( pH->m_szInternalName, pszArg ) )
			{
				iHeroId = pH->m_iHeroId;
				break;
			}
		}
	}

	if ( pPlayer->m_bOWHeroLocked && tf_ow_hero_lock.GetBool() )
	{
		ClientPrint( pPlayer, HUD_PRINTTALK, "OW: return to spawn room to change hero." );
		return;
	}

	OW_SetPlayerHero( pPlayer, iHeroId );
	const OWHeroDefinition_t *pHero = COWHeroRegistry::Instance().GetHeroById( iHeroId );
	if ( pHero )
	{
		pPlayer->SetDesiredPlayerClassIndex( pHero->m_iTFClass );
		pPlayer->ForceRespawn();
	}
}

static ConCommand ow_hero( "ow_hero", CC_OW_SelectHero, "Select OW hero by id or name.", FCVAR_GAMEDLL );

static void CC_OW_Roster( const CCommand &args )
{
	COWHeroRegistry::Instance().PrintRoster();
}

static ConCommand ow_roster( "ow_roster", CC_OW_Roster, "Print OW hero roster.", FCVAR_GAMEDLL );

static void CC_OW_Ability1( const CCommand &args ) { OW_UseAbility( ToTFPlayer( UTIL_GetCommandClient() ), 0 ); }
static void CC_OW_Ability2( const CCommand &args ) { OW_UseAbility( ToTFPlayer( UTIL_GetCommandClient() ), 1 ); }
static void CC_OW_Ability3( const CCommand &args ) { OW_UseAbility( ToTFPlayer( UTIL_GetCommandClient() ), 2 ); }
static void CC_OW_Ultimate( const CCommand &args ) { OW_UseUltimate( ToTFPlayer( UTIL_GetCommandClient() ) ); }

static void CC_OW_Ability1Release( const CCommand &args ) {}
static void CC_OW_Ability2Release( const CCommand &args ) {}
static void CC_OW_Ability3Release( const CCommand &args ) {}
static void CC_OW_UltimateRelease( const CCommand &args ) {}

static ConCommand ow_ability1( "+ow_ability1", CC_OW_Ability1, "OW ability slot 1 (Shift).", FCVAR_GAMEDLL );
static ConCommand ow_ability1_release( "-ow_ability1", CC_OW_Ability1Release, "", FCVAR_GAMEDLL );
static ConCommand ow_ability2( "+ow_ability2", CC_OW_Ability2, "OW ability slot 2 (E).", FCVAR_GAMEDLL );
static ConCommand ow_ability2_release( "-ow_ability2", CC_OW_Ability2Release, "", FCVAR_GAMEDLL );
static ConCommand ow_ability3( "+ow_ability3", CC_OW_Ability3, "OW ability slot 3 (Q).", FCVAR_GAMEDLL );
static ConCommand ow_ability3_release( "-ow_ability3", CC_OW_Ability3Release, "", FCVAR_GAMEDLL );
static ConCommand ow_ultimate( "+ow_ultimate", CC_OW_Ultimate, "OW ultimate (R).", FCVAR_GAMEDLL );
static ConCommand ow_ultimate_release( "-ow_ultimate", CC_OW_UltimateRelease, "", FCVAR_GAMEDLL );

#endif // SOURCESDK
