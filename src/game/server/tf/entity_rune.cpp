//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Rune.
//
//=============================================================================//
#include "cbase.h"
#include "items.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "entity_rune.h"
#include "tf_gamestats.h"
#include "func_respawnroom.h"
#include "particle_parse.h"
#include "tf_fx.h"
#include "collisionutils.h"

//=============================================================================
//
// CTF Rune defines.
//

extern ConVar tf_powerup_mode;

#define RUNE_BLINK_CONTEXT "blink_think"

#define TF_RUNE_TEMP_RESPAWN_DELAY 	90.f
#define TF_RUNE_TEMP_UBER_RESPAWN_DELAY 	180.f

// Regular Runes (non temporary) will reposition themselves if they haven't been picked up in time
#define BLINK_TIME 10.f

LINK_ENTITY_TO_CLASS( item_powerup_rune, CTFRune );
LINK_ENTITY_TO_CLASS( item_powerup_rune_temp, CTFRuneTemp );
LINK_ENTITY_TO_CLASS( item_powerup_crit, CTFRuneTempCrit );
LINK_ENTITY_TO_CLASS( item_powerup_uber, CTFRuneTempUber );
LINK_ENTITY_TO_CLASS( info_powerup_spawn, CTFInfoPowerupSpawn );

IMPLEMENT_AUTO_LIST( IInfoPowerupSpawnAutoList );

//=============================================================================
//
// CTF Rune functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFRune::CTFRune()
{
	m_bApplyForce = true;
	m_nRuneType = RUNE_STRENGTH;
	m_nTeam = TEAM_ANY;
	m_bShouldReposition = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFRune::~CTFRune()
{
}

//-----------------------------------------------------------------------------
// Purpose: Spawn powerup rune
//-----------------------------------------------------------------------------
void CTFRune::Spawn( void )
{
	BaseClass::Spawn();

	RemoveSolidFlags( FSOLID_TRIGGER );
	m_bActivateWhenAtRest = true;

	m_vecSpawnDirection.z = 0.7f;
	VectorNormalize( m_vecSpawnDirection );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetSolid( SOLID_BBOX );
	if ( m_bApplyForce )
	{
		SetAbsVelocity( m_vecSpawnDirection * 350.f );
		m_bThrownSingleInstance = true;
	}

	// Reposition if not picked up in time
	m_nBlinkCount = 0;
	m_flKillTime = gpGlobals->curtime + GetRuneRepositionTime() + BLINK_TIME;
	if ( m_bShouldReposition )
	{
		SetContextThink( &CTFRune::BlinkThink, gpGlobals->curtime + GetRuneRepositionTime(), RUNE_BLINK_CONTEXT );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Precache function for the powerup rune
//-----------------------------------------------------------------------------
void CTFRune::Precache( void )
{
	PrecacheScriptSound( "Powerup.PickUpResistance" );
	PrecacheScriptSound( "Powerup.PickUpRegeneration" );
	PrecacheScriptSound( "Powerup.PickUpStrength" );
	PrecacheScriptSound( "Powerup.PickUpHaste" );
	PrecacheScriptSound( "Powerup.PickUpVampire" );
	PrecacheScriptSound( "Powerup.PickUpReflect" );
	PrecacheScriptSound( "Powerup.PickUpPrecision" );
	PrecacheScriptSound( "Powerup.PickUpAgility" );
	PrecacheScriptSound( "Powerup.PickUpKnockout" );
	PrecacheScriptSound( "Powerup.Knockout_Melee_Hit" );
	PrecacheScriptSound( "Powerup.Reflect.Reflect" );
	PrecacheScriptSound( "Powerup.PickUpKing" );
	PrecacheScriptSound( "Powerup.PickUpPlague" ); 
	PrecacheScriptSound( "Powerup.PickUpSupernova" );
	PrecacheScriptSound( "Powerup.PickUpSupernovaActivate" );
	PrecacheScriptSound( "Powerup.PickUpPlagueInfected" );
	PrecacheScriptSound( "Powerup.PickUpPlagueInfectedLoop" );
	PrecacheScriptSound( "Mannpower.PlayerIsDominant" );
	PrecacheScriptSound( "Mannpower.DominantPlayerOtherTeam" );
	PrecacheScriptSound( "Mannpower.PlayerIsNoLongerDominant" );
	PrecacheModel( GetDefaultPowerupModel() ); 
	PrecacheModel( TF_RUNE_STRENGTH );
	PrecacheModel( TF_RUNE_RESIST );
	PrecacheModel( TF_RUNE_REGEN );
	PrecacheModel( TF_RUNE_HASTE );
	PrecacheModel( TF_RUNE_VAMPIRE );
	PrecacheModel( TF_RUNE_REFLECT );
	PrecacheModel( TF_RUNE_PRECISION );
	PrecacheModel( TF_RUNE_AGILITY );
	PrecacheModel( TF_RUNE_KNOCKOUT );
	PrecacheModel( TF_RUNE_KING );
	PrecacheModel( TF_RUNE_PLAGUE );
	PrecacheModel( TF_RUNE_SUPERNOVA );

	// precache all powerup icons
	for ( int i=0; i<RUNE_TYPES_MAX; ++i )
	{
		RuneTypes_t type = RuneTypes_t(i);
		PrecacheParticleSystem( GetPowerupIconName( type, TF_TEAM_RED ) );
		PrecacheParticleSystem( GetPowerupIconName( type, TF_TEAM_BLUE ) );
	}

	PrecacheParticleSystem( "plague_infect_player" ); 
	PrecacheParticleSystem( "plague_healthkit_pickup" );
	PrecacheParticleSystem( "powerup_king_red" );
	PrecacheParticleSystem( "powerup_king_blue" );
	PrecacheParticleSystem( "powerup_supernova_ready" );
	PrecacheParticleSystem( "powerup_supernova_strike_red" );
	PrecacheParticleSystem( "powerup_supernova_strike_blue" );
	PrecacheParticleSystem( "powerup_supernova_explode_red" );
	PrecacheParticleSystem( "powerup_supernova_explode_blue" );
	PrecacheParticleSystem( "powerup_plague_carrier" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Make sure Powerup Runes that are away from any players at spawn will accept particle attachments
//-----------------------------------------------------------------------------
int CTFRune::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Make sure Powerup Runes that are away from any players at spawn will accept particle attachments
//-----------------------------------------------------------------------------
int CTFRune::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	return FL_EDICT_ALWAYS;
}


//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the powerup rune
//-----------------------------------------------------------------------------
bool CTFRune::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = ValidTouch( pPlayer );
	if ( bSuccess )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( !pTFPlayer )
			return false;

		// don't allow different team player to pick up team based rune unless they are a spy disguised as the correct team
		if ( m_nTeam != TEAM_ANY && m_nTeam != pTFPlayer->GetTeamNumber() )
			if ( !pTFPlayer->GetPlayerClass()->IsClass( TF_CLASS_SPY ) || pTFPlayer->m_Shared.GetDisguiseTeam() != m_nTeam )
			{
				return false;
			}

		if ( pTFPlayer->m_Shared.IsStealthed() || 
			pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) || 
			pTFPlayer->m_Shared.GetPercentInvisible() > 0.25f )
		{
			return false;
		}

		if ( pTFPlayer->m_Shared.IsCarryingRune() )
		{
			ClientPrint( pPlayer, HUD_PRINTCENTER, "#TF_Powerup_Pickup_Deny" );
			return false;
		}

		if ( pTFPlayer->IsTaunting() )
		{
			return false;
		}

#ifdef GAME_DLL
		if ( PointInRespawnRoom( pTFPlayer, pTFPlayer->WorldSpaceCenter() ) )
			return false;
#endif
		Assert( m_nRuneType >= 0 && m_nRuneType < RUNE_TYPES_MAX );

		// Order is important because SetSpeed has to occur once SetCarryingRuneType has happened
		pTFPlayer->m_Shared.SetCarryingRuneType( m_nRuneType );
		CPASAttenuationFilter filter( pTFPlayer );

		switch ( m_nRuneType )
		{
		case RUNE_STRENGTH:
			ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Pickup_Strength" );
			pTFPlayer->EmitSound( "Powerup.PickUpStrength" );
			break;
		case RUNE_RESIST:
			ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Pickup_Resist" );
			pTFPlayer->EmitSound( "Powerup.PickUpResistance" );
			break;
		case RUNE_REGEN:
			ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Pickup_Regen" );
			pTFPlayer->EmitSound( "Powerup.PickUpRegeneration" );
			break;
		case RUNE_HASTE:
			ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Pickup_Haste" );
			pTFPlayer->EmitSound( "Powerup.PickUpHaste" );
			pTFPlayer->TeamFortress_SetSpeed();
			break;
		case RUNE_VAMPIRE:
			ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Pickup_Vampire" );
			pTFPlayer->EmitSound( "Powerup.PickUpVampire" );
			break;
		case RUNE_REFLECT:
			ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Pickup_Reflect" );
			pTFPlayer->EmitSound( "Powerup.PickUpReflect" );
			break;
		case RUNE_PRECISION:
			ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Pickup_Precision" );
			pTFPlayer->EmitSound( "Powerup.PickUpPrecision" );
			break;
		case RUNE_AGILITY:
			ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Pickup_Agility" );
			pTFPlayer->EmitSound( "Powerup.PickUpAgility" );
			pTFPlayer->TeamFortress_SetSpeed();
			break;
		case RUNE_KNOCKOUT:
		{
			ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Pickup_Knockout" );
			pTFPlayer->EmitSound( "Powerup.PickUpKnockout" );

			// Switch to melee to make sure Engies don't have build menus open
			CTFWeaponBase *pMeleeWeapon = dynamic_cast<CTFWeaponBase*>( pTFPlayer->GetEntityForLoadoutSlot( LOADOUT_POSITION_MELEE ) );
			Assert( pMeleeWeapon );
			if ( pMeleeWeapon )
			{
				if ( pTFPlayer->GetActiveTFWeapon() && pTFPlayer->GetActiveTFWeapon()->GetWeaponID() != TF_WEAPON_GRAPPLINGHOOK )
				{
					pTFPlayer->Weapon_Switch( pMeleeWeapon );
				}
				else
				{
					// when the player switch away from the hook, it'll go to the last weapon.
					// force it to be melee weapon
					pTFPlayer->Weapon_SetLast( pMeleeWeapon );
				}
			}

			pTFPlayer->m_Shared.AddCond( TF_COND_CANNOT_SWITCH_FROM_MELEE );
		}
			break;
		case RUNE_KING:
			ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Pickup_King" );
			pTFPlayer->EmitSound( "Powerup.PickUpKing" );
			break;
		case RUNE_PLAGUE:
			ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Pickup_Plague" );
			pTFPlayer->EmitSound( "Powerup.PickUpPlague" );
			break;
		case RUNE_SUPERNOVA:
			ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Pickup_Supernova" );
			pTFPlayer->EmitSound( "Powerup.PickUpSupernova" );
			break;
		}
		UTIL_Remove( this ); //power-up runes don't respawn, only one of them exists in the game
		
		if  ( pTFPlayer->m_bIsInMannpowerDominantCondition )
		{
			pTFPlayer->m_Shared.AddCond( TF_COND_MARKEDFORDEATH, 5.f );
		}
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: Create an instance of a powerup rune and drop it into the world with a random vector velocity
//-----------------------------------------------------------------------------
CTFRune* CTFRune::CreateRune( const Vector &vecOrigin, RuneTypes_t nType, int nTeam, bool bShouldReposition, bool bApplyForce, Vector vecSpawnDirection /*= vec3_angle*/ )
{
	// Make sure we're passing in a valid rune type
	Assert( nType >= 0 && nType < RUNE_TYPES_MAX );

	CTFRune *pRune = NULL;
	pRune = static_cast< CTFRune* >( CBaseEntity::CreateNoSpawn( "item_powerup_rune", vecOrigin + Vector( 0.f, 0.f, 48.f ), vec3_angle, NULL ) );
	pRune->m_bApplyForce = bApplyForce;
	pRune->m_nRuneType = nType;
	pRune->m_vecSpawnDirection = vecSpawnDirection;
	pRune->m_nTeam = nTeam;
	pRune->m_bShouldReposition = bShouldReposition;

	if ( nTeam == TEAM_ANY )
		pRune->m_nSkin = 0;
	else
		pRune->m_nSkin = nTeam == TF_TEAM_RED ? 1 : 2;

	DispatchSpawn( pRune );

	return pRune;
}

//-----------------------------------------------------------------------------
// Respawn the Powerup Rune in the event of it coming to rest inside a trigger_hurt or respawn room trigger
//-----------------------------------------------------------------------------
void CTFRune::ComeToRest( void )
{
	BaseClass::ComeToRest();

	// See if we've come to rest in a trigger_hurt or a respawn room trigger. If so, immediately reposition the Powerup

	for ( int i = 0; i < ITriggerHurtAutoList::AutoList().Count(); i++ )
	{
		CTriggerHurt *pTrigger = static_cast< CTriggerHurt* >( ITriggerHurtAutoList::AutoList()[i] );
		if ( pTrigger->m_bDisabled )
			continue;

		Vector vecMins, vecMaxs;
		pTrigger->GetCollideable()->WorldSpaceSurroundingBounds( &vecMins, &vecMaxs );
		if ( IsPointInBox( GetCollideable()->GetCollisionOrigin(), vecMins, vecMaxs ) )
		{
			if ( RepositionRune( m_nRuneType, m_nTeam ) )
			{
				UTIL_Remove( this );
			}
			return;
		}
	}
	
	for ( int j = 0; j < IFuncRespawnRoomAutoList::AutoList().Count(); j++ )
	{
		CFuncRespawnRoom *pRespawnRoom = static_cast<CFuncRespawnRoom*>( IFuncRespawnRoomAutoList::AutoList()[j] );
		Vector vecMins, vecMaxs;

		pRespawnRoom->GetCollideable()->WorldSpaceSurroundingBounds( &vecMins, &vecMaxs );
		if ( IsPointInBox( GetCollideable()->GetCollisionOrigin(), vecMins, vecMaxs ) )
		{
			if ( RepositionRune( m_nRuneType, m_nTeam ) )
			{
				UTIL_Remove( this );
			}
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Enter blinking state before removing and respawning this Powerup
//-----------------------------------------------------------------------------
void CTFRune::BlinkThink()
{
	float flTimeToKill = m_flKillTime - gpGlobals->curtime;
	float flNextBlink = RemapValClamped( flTimeToKill, BLINK_TIME, 0.f, 0.5f, 0.1f );

	SetCycle( RandomFloat() );

	SetContextThink( &CTFRune::BlinkThink, gpGlobals->curtime + flNextBlink, RUNE_BLINK_CONTEXT );

	SetRenderMode( kRenderTransAlpha );

	++m_nBlinkCount;
	if ( m_nBlinkCount % 2 == 0 )
	{
		SetRenderColorA( 25 );
	}
	else
	{
		SetRenderColorA( 255 );
	}
	if ( gpGlobals->curtime >= m_flKillTime )
	{
		if ( RepositionRune( m_nRuneType, m_nTeam ) )
		{
			UTIL_Remove( this );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Enter blinking state before removing and respawning this Powerup
//-----------------------------------------------------------------------------
bool CTFRune::RepositionRune( RuneTypes_t nType, int nTeamNumber )
{
	// Powerups will pick a spawn point based on what team color they were when they repositioned. First try your own team color, then neutral, then enemy

	CUtlVector< CTFInfoPowerupSpawn* > vecSpawnPoints;
	if ( nTeamNumber != TEAM_ANY )
	{
		for ( int n = 0; n < IInfoPowerupSpawnAutoList::AutoList().Count(); n++ )
		{
			CTFInfoPowerupSpawn *pSpawnPoint = static_cast<CTFInfoPowerupSpawn*>( IInfoPowerupSpawnAutoList::AutoList()[n] );
			// We want to try to spawn on a point that matches the team color
			if ( !pSpawnPoint->IsDisabled() && !pSpawnPoint->HasRune() && pSpawnPoint->GetTeamNumber() == nTeamNumber )
			{
				vecSpawnPoints.AddToTail( pSpawnPoint );
			}
		}
	}
	if ( nTeamNumber == TEAM_ANY || vecSpawnPoints.Count() < 1 )
	{
		for ( int n = 0; n < IInfoPowerupSpawnAutoList::AutoList().Count(); n++ )
		{
			CTFInfoPowerupSpawn *pSpawnPoint = static_cast<CTFInfoPowerupSpawn*>( IInfoPowerupSpawnAutoList::AutoList()[n] );
			// Don't include disabled info_powerup_spawn positions or ones that already have runes on them
			if ( !pSpawnPoint->IsDisabled() && !pSpawnPoint->HasRune() )
			{
				vecSpawnPoints.AddToTail( pSpawnPoint );
			}
		}
	}

	Assert( vecSpawnPoints.Count() > 0 ); // We need at least one valid info_powerup_spawn position

	if ( vecSpawnPoints.Count() )
	{
		int index = RandomInt( 0, vecSpawnPoints.Count() - 1 );
		CTFInfoPowerupSpawn *pSpawnPoint = vecSpawnPoints[index];
		CTFRune *pNewRune = CreateRune( vecSpawnPoints[index]->GetAbsOrigin(), nType, TEAM_ANY, false, false );
		pSpawnPoint->SetRune( pNewRune );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

float CTFRune::GetRuneRepositionTime()
{
	// In freeforall mode, killed players drop enemy team colored powerups. These powerups reposition quicker 
	if ( m_nTeam != TEAM_ANY )
	{
		return 30.f;
	}
	else
	{
		return 60.f;
	}
}



//=============================================================================
//
// CTF Temporary Rune functions.
//
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFRuneTemp::CTFRuneTemp()
{
	m_nRuneTempType = RUNETYPE_TEMP_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the temporary powerup rune
//-----------------------------------------------------------------------------
void CTFRuneTemp::Precache( void )
{
	PrecacheScriptSound( "Powerup.PickUpTemp.Crit" );
	PrecacheScriptSound( "Powerup.PickUpTemp.Uber" );
	PrecacheModel( TF_RUNE_TEMP_CRIT );
	PrecacheModel( TF_RUNE_TEMP_UBER );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn Temporary powerup rune
//-----------------------------------------------------------------------------
void CTFRuneTemp::Spawn( void )
{
	if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
	{
		BaseClass::Spawn();

		SetMoveType( MOVETYPE_NONE );
		SetSolid( SOLID_BBOX );
	}
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the temporary powerup rune - needed because these respawn, and don't set the condition TF_COND_RUNE
//-----------------------------------------------------------------------------
bool CTFRuneTemp::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = ValidTouch( pPlayer );
	if ( bSuccess )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( !pTFPlayer )
			return false;

		if ( pTFPlayer->m_Shared.IsStealthed() || pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) || pTFPlayer->m_Shared.GetPercentInvisible() > 0.25f )
		{
			return false;
		}
		
		if ( pTFPlayer->m_Shared.InCond( TF_COND_RUNE_IMBALANCE ) )
		{
			return false;
		}

		if ( pTFPlayer->IsTaunting() )
		{
			return false;
		}
		
		if ( pTFPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_USER_BUFF ) || pTFPlayer->m_Shared.InCond( TF_COND_CRITBOOSTED_RUNE_TEMP ) )
		{
			return false;
		}

		if ( m_nRuneTempType == RUNETYPE_TEMP_CRIT )
		{
			if ( pTFPlayer->m_Shared.IsCritBoosted() ) // Disallow players who are already crit boosted
			{
				return false;
			}
			else
			{
				pTFPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_RUNE_TEMP, 30.f );
				pTFPlayer->EmitSound( "Powerup.PickUpTemp.Crit" );
			}
		}

		if ( m_nRuneTempType == RUNETYPE_TEMP_UBER )
		{
			if ( pTFPlayer->HasTheFlag() || pTFPlayer->m_Shared.IsInvulnerable() ) // Disallow players who have the flag or are already ubercharged
			{
				return false;
			}
			else
			{
				pTFPlayer->m_Shared.AddCond( TF_COND_INVULNERABLE_USER_BUFF, 20.f );
				pTFPlayer->EmitSound( "Powerup.PickUpTemp.Uber" );
			}
		}
	}
	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFRuneTemp::GetRespawnDelay( void )
{
	if ( m_nRuneTempType == RUNETYPE_TEMP_UBER )
	{
		return TF_RUNE_TEMP_UBER_RESPAWN_DELAY;
	}
	else
		return TF_RUNE_TEMP_RESPAWN_DELAY;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFRuneTempCrit::CTFRuneTempCrit()
{
	m_nRuneTempType = RUNETYPE_TEMP_CRIT;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFRuneTempUber::CTFRuneTempUber()
{
	m_nRuneTempType = RUNETYPE_TEMP_UBER;
}

BEGIN_DATADESC( CTFInfoPowerupSpawn )

	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "disabled" ),
	DEFINE_KEYFIELD( m_nTeam, FIELD_INTEGER, "team" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFInfoPowerupSpawn::CTFInfoPowerupSpawn()
{
	m_bDisabled = false;
	m_nTeam = TEAM_ANY;
}

void CTFInfoPowerupSpawn::Spawn()
{
	BaseClass::Spawn();

	// set baseclass team number
	ChangeTeam( m_nTeam );
}
