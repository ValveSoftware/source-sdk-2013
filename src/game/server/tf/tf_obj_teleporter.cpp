//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Teleporter Object
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "tf_obj_teleporter.h"
#include "engine/IEngineSound.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_gamerules.h"
#include "world.h"
#include "explode.h"
#include "particle_parse.h"
#include "tf_gamestats.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_fx.h"
#include "props.h"
#include "tf_objective_resource.h"
#include "rtime.h"
#include "tf_logic_player_destruction.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Ground placed version
#define TELEPORTER_MODEL_ENTRANCE_PLACEMENT	"models/buildables/teleporter_blueprint_enter.mdl"
#define TELEPORTER_MODEL_EXIT_PLACEMENT		"models/buildables/teleporter_blueprint_exit.mdl"
#define TELEPORTER_MODEL_BUILDING			"models/buildables/teleporter.mdl"
#define TELEPORTER_MODEL_LIGHT				"models/buildables/teleporter_light.mdl"

#define TELEPORTER_MINS			Vector( -24, -24, 0)
#define TELEPORTER_MAXS			Vector( 24, 24, 12)	

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
// Seconds it takes a teleporter to recharge
int g_iTeleporterRechargeTimes[4] =
{
	0,
	10,
	5,
	3
};

IMPLEMENT_SERVERCLASS_ST( CObjectTeleporter, DT_ObjectTeleporter )
	SendPropInt( SENDINFO(m_iState), 5 ),
	SendPropTime( SENDINFO(m_flRechargeTime) ),
	SendPropTime( SENDINFO(m_flCurrentRechargeDuration) ),
	SendPropInt( SENDINFO(m_iTimesUsed), 10, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_flYawToExit), 8, 0, 0.0, 360.0f ),
	SendPropBool( SENDINFO(m_bMatchBuilding) ),
END_SEND_TABLE()

BEGIN_DATADESC( CObjectTeleporter )
	// keys
	DEFINE_KEYFIELD( m_iTeleportType,					FIELD_INTEGER, "teleporterType" ),
	DEFINE_KEYFIELD( m_iszMatchingMapPlacedTeleporter,	FIELD_STRING, "matchingTeleporter" ),
	// other
	DEFINE_THINKFUNC( TeleporterThink ),
	DEFINE_ENTITYFUNC( TeleporterTouch ),
END_DATADESC()

PRECACHE_REGISTER( obj_teleporter );

#define TELEPORTER_THINK_CONTEXT				"TeleporterContext"

#define BUILD_TELEPORTER_DAMAGE					25		// how much damage an exploding teleporter can do

#define BUILD_TELEPORTER_FADEOUT_TIME			0.25	// time to teleport a player out (teleporter with full health)
#define BUILD_TELEPORTER_FADEIN_TIME			0.25	// time to teleport a player in (teleporter with full health)

#define BUILD_TELEPORTER_NEXT_THINK				0.05

#define BUILD_TELEPORTER_PLAYER_OFFSET			20		// how far above the origin of the teleporter to place a player

#define BUILD_TELEPORTER_EFFECT_TIME			12.0	// seconds that player glows after teleporting

ConVar tf_teleporter_fov_start( "tf_teleporter_fov_start", "120", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Starting FOV for teleporter zoom.", true, 1, false, 0 );
ConVar tf_teleporter_fov_time( "tf_teleporter_fov_time", "0.5", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "How quickly to restore FOV after teleport.", true, 0.0, false, 0 );

LINK_ENTITY_TO_CLASS( obj_teleporter, CObjectTeleporter );

//-----------------------------------------------------------------------------
// Purpose: Teleport the passed player to our destination
//-----------------------------------------------------------------------------
void CObjectTeleporter::TeleporterSend( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	SetTeleportingPlayer( pPlayer );
	pPlayer->m_Shared.AddCond( TF_COND_SELECTED_TO_TELEPORT );

	Vector origin = GetAbsOrigin();
	CPVSFilter filter( origin );

	int iTeam = pPlayer->GetTeamNumber();
	if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		if ( GetBuilder() && iTeam != GetBuilder()->GetTeamNumber() )
		{
			iTeam = GetBuilder()->GetTeamNumber();
		}
	}

	switch( iTeam )
	{
	case TF_TEAM_RED:
		TE_TFParticleEffect( filter, 0.0, "teleported_red", origin, vec3_angle );
		TE_TFParticleEffect( filter, 0.0, "player_sparkles_red", origin, vec3_angle, pPlayer, PATTACH_ABSORIGIN );
		break;
	case TF_TEAM_BLUE:
		TE_TFParticleEffect( filter, 0.0, "teleported_blue", origin, vec3_angle );
		TE_TFParticleEffect( filter, 0.0, "player_sparkles_blue", origin, vec3_angle, pPlayer, PATTACH_ABSORIGIN );
		break;
	default:
		break;
	}

	EmitSound( "Building_Teleporter.Send" );

	SetState( TELEPORTER_STATE_SENDING );
	m_flMyNextThink = gpGlobals->curtime + 0.1;

	m_iTimesUsed++;

	m_hReservedForPlayer = NULL;

	// Strange - Teleports Provided to Allies
	if ( GetBuilder() && GetBuilder()->GetTeam() == pPlayer->GetTeam() )
	{
		// Strange Health Provided to Allies
		EconEntity_OnOwnerKillEaterEvent( 
			dynamic_cast<CEconEntity *>( GetBuilder()->GetEntityForLoadoutSlot( LOADOUT_POSITION_PDA ) ),
			GetBuilder(),
			pPlayer,
			kKillEaterEvent_TeleportsProvided
		);

		if ( GetBuilder() != pPlayer &&
			 TFGameRules() && 
			 TFGameRules()->GameModeUsesUpgrades() &&
			 TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
		{
			CTF_GameStats.Event_PlayerAwardBonusPoints( GetBuilder(), pPlayer, 10 );
		}
	}

	int iSpeedBoost = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( GetBuilder(), iSpeedBoost, mod_teleporter_speed_boost );
	if ( iSpeedBoost )
	{
		pPlayer->m_Shared.AddCond( TF_COND_SPEED_BOOST, 4.f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Receive a teleporting player 
//-----------------------------------------------------------------------------
void CObjectTeleporter::TeleporterReceive( CTFPlayer *pPlayer, float flDelay )
{
	if ( !pPlayer )
		return;

	SetTeleportingPlayer( pPlayer );

	Vector origin = GetAbsOrigin();
	CPVSFilter filter( origin );

	int iTeam = pPlayer->GetTeamNumber();
	if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		if ( GetBuilder() && iTeam != GetBuilder()->GetTeamNumber() )
		{
			iTeam = GetBuilder()->GetTeamNumber();
		}
	}

	if ( GetBuilder() )
	{
		pPlayer->m_Shared.SetTeamTeleporterUsed( GetBuilder()->GetTeamNumber() );
	}

	switch( iTeam )
	{
	case TF_TEAM_RED:
		TE_TFParticleEffect( filter, 0.0, "teleportedin_red", origin, vec3_angle );
		break;
	case TF_TEAM_BLUE:
		TE_TFParticleEffect( filter, 0.0, "teleportedin_blue", origin, vec3_angle );
		break;
	default:
		break;
	}

	EmitSound( "Building_Teleporter.Receive" );

	SetState( TELEPORTER_STATE_RECEIVING );
	m_flMyNextThink = gpGlobals->curtime + BUILD_TELEPORTER_FADEOUT_TIME;

	m_iTimesUsed++;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CObjectTeleporter::CObjectTeleporter()
{
	int iHealth = GetMaxHealthForCurrentLevel();

	SetMaxHealth( iHealth );
	SetHealth( iHealth );
	UseClientSideAnimation();

	SetType( OBJ_TELEPORTER );

	m_bMatchBuilding.Set( false );

	m_iTeleportType = TTYPE_NONE;

	m_flCurrentRechargeDuration = 0.0f;
	m_flRechargeTime = 0.0f;

	ListenForGameEvent( "player_spawn" );
	ListenForGameEvent( "player_team" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectTeleporter::Spawn()
{
	SetSolid( SOLID_BBOX );
	
	m_takedamage = DAMAGE_NO;

	SetState( TELEPORTER_STATE_BUILDING );

	m_flNextEnemyTouchHint = gpGlobals->curtime;

	m_flYawToExit = 0;

	if ( IsEntrance() )
	{
		SetModel( TELEPORTER_MODEL_ENTRANCE_PLACEMENT );
	}
	else
	{
		SetModel( TELEPORTER_MODEL_EXIT_PLACEMENT );
	}

	m_iUpgradeLevel = 1;

	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectTeleporter::UpdateOnRemove()
{
	if ( GetTeamNumber() == TF_TEAM_PVE_INVADERS )
	{
		TFObjectiveResource()->DecrementTeleporterCount();
	}

	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectTeleporter::FirstSpawn()
{
	int iHealth = GetMaxHealthForCurrentLevel();

	SetMaxHealth( iHealth );
	SetHealth( iHealth );

	BaseClass::FirstSpawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectTeleporter::SetObjectMode( int iVal )
{
	if ( iVal == MODE_TELEPORTER_ENTRANCE )
	{
		SetTeleporterType( TTYPE_ENTRANCE );
	}
	else
	{
		SetTeleporterType( TTYPE_EXIT );
	}

	BaseClass::SetObjectMode( iVal );
}

//-----------------------------------------------------------------------------
int CObjectTeleporter::GetUpgradeMetalRequired()
{

	int nCost = GetObjectInfo( GetType() )->m_UpgradeCost;

	float flCostMod = 1.f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBuilder(), flCostMod, mod_teleporter_cost );
	if ( flCostMod != 1.f )
	{
		nCost *= flCostMod;
	}

	return nCost;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectTeleporter::SetModel( const char *pModel )
{
	BaseClass::SetModel( pModel );

	// Reset this after model change
	UTIL_SetSize( this, TELEPORTER_MINS, TELEPORTER_MAXS );

	CreateBuildPoints();

	ReattachChildren();

	m_iDirectionBodygroup = FindBodygroupByName( "teleporter_direction" );
	m_iBlurBodygroup = FindBodygroupByName( "teleporter_blur" );

	if ( m_iBlurBodygroup >= 0 )
	{
		SetBodygroup( m_iBlurBodygroup, 0 );
	}
}

void CObjectTeleporter::InitializeMapPlacedObject( void )
{
	BaseClass::InitializeMapPlacedObject();
	
	SetObjectMode( IsEntrance() ? MODE_TELEPORTER_ENTRANCE : MODE_TELEPORTER_EXIT );


	m_hMatchingTeleporter = dynamic_cast<CObjectTeleporter*>( gEntList.FindEntityByName( NULL, m_iszMatchingMapPlacedTeleporter.ToCStr() ) );

	// Select the teleporter with the most upgrade
	if ( m_hMatchingTeleporter.Get() )
	{
		bool bFrom = (m_hMatchingTeleporter->GetUpgradeLevel() > GetUpgradeLevel() || m_hMatchingTeleporter->GetUpgradeMetal() > GetUpgradeMetal() );
		CopyUpgradeStateToMatch( m_hMatchingTeleporter, bFrom );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Start building the object
//-----------------------------------------------------------------------------
bool CObjectTeleporter::StartBuilding( CBaseEntity *pBuilder )
{
	SetStartBuildingModel();

	if ( GetTeleporterType() == TTYPE_NONE )
	{
		if ( GetObjectMode() == MODE_TELEPORTER_ENTRANCE )
		{
			SetTeleporterType( TTYPE_ENTRANCE );
		}
		else
		{
			SetTeleporterType( TTYPE_EXIT );
		}
	}

	return BaseClass::StartBuilding( pBuilder );
}

void CObjectTeleporter::SetStartBuildingModel( void )
{
	SetState( TELEPORTER_STATE_BUILDING );

	SetModel( TELEPORTER_MODEL_BUILDING );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectTeleporter::IsPlacementPosValid( void )
{
	bool bResult = BaseClass::IsPlacementPosValid();

	if ( !bResult )
	{
		return false;
	}

	// m_vecBuildOrigin is the proposed build origin

	// start above the teleporter position
	Vector vecTestPos = m_vecBuildOrigin;
	vecTestPos.z += TELEPORTER_MAXS.z;

	// make sure we can fit a player on top in this pos
	trace_t tr;
	UTIL_TraceHull( vecTestPos, vecTestPos, VEC_HULL_MIN, VEC_HULL_MAX, MASK_SOLID | CONTENTS_PLAYERCLIP, this, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

	return ( tr.fraction >= 1.0 );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CObjectTeleporter::OnGoActive( void )
{
	Assert( GetBuilder() || m_bWasMapPlaced );

	SetModel( TELEPORTER_MODEL_LIGHT );
	SetActivity( ACT_OBJ_IDLE );

	SetContextThink( &CObjectTeleporter::TeleporterThink, gpGlobals->curtime + 0.1, TELEPORTER_THINK_CONTEXT );
	SetTouch( &CObjectTeleporter::TeleporterTouch );

	SetState( TELEPORTER_STATE_IDLE );

	BaseClass::OnGoActive();

	SetPlaybackRate( 0.0f );
	m_flLastStateChangeTime = 0.0f;	// used as a flag to initialize the playback rate to 0 in the first DeterminePlaybackRate

	// match our partner's maxhealth
	if ( IsMatchingTeleporterReady() )
	{
		CObjectTeleporter *pMatch = GetMatchingTeleporter();
		if ( pMatch )
		{
			UpdateMaxHealth( pMatch->GetMaxHealth() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectTeleporter::Precache()
{
	BaseClass::Precache();

	// Precache Object Models
	int iModelIndex;

	PrecacheModel( TELEPORTER_MODEL_ENTRANCE_PLACEMENT );
	PrecacheModel( TELEPORTER_MODEL_EXIT_PLACEMENT );	

	iModelIndex = PrecacheModel( TELEPORTER_MODEL_BUILDING );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( TELEPORTER_MODEL_LIGHT );
	PrecacheGibsForModel( iModelIndex );

	// Bread models
	int nRange = TF_LAST_NORMAL_CLASS - TF_FIRST_NORMAL_CLASS;
	for( int i = 0; i < nRange; ++i )
	{
		if ( g_pszBreadModels[i] && *g_pszBreadModels[i] )
		{
			PrecacheModel( g_pszBreadModels[i] );
		}
	}

	// Precache Sounds
	PrecacheScriptSound( "Building_Teleporter.Ready" );
	PrecacheScriptSound( "Building_Teleporter.Send" );
	PrecacheScriptSound( "Building_Teleporter.Receive" );
	PrecacheScriptSound( "Building_Teleporter.SpinLevel1" );
	PrecacheScriptSound( "Building_Teleporter.SpinLevel2" );
	PrecacheScriptSound( "Building_Teleporter.SpinLevel3" );

	PrecacheParticleSystem( "teleporter_red_charged" );
	PrecacheParticleSystem( "teleporter_blue_charged" );
	PrecacheParticleSystem( "teleporter_red_entrance" );
	PrecacheParticleSystem( "teleporter_blue_entrance" );
	PrecacheParticleSystem( "teleporter_red_exit" );
	PrecacheParticleSystem( "teleporter_blue_exit" );
	PrecacheParticleSystem( "teleporter_arms_circle_red" );
	PrecacheParticleSystem( "teleporter_arms_circle_blue" );
	PrecacheParticleSystem( "tpdamage_1" );
	PrecacheParticleSystem( "tpdamage_2" );
	PrecacheParticleSystem( "tpdamage_3" );
	PrecacheParticleSystem( "tpdamage_4" );
	PrecacheParticleSystem( "teleported_red" );
	PrecacheParticleSystem( "player_sparkles_red" );
	PrecacheParticleSystem( "teleported_blue" );
	PrecacheParticleSystem( "player_sparkles_blue" );
	PrecacheParticleSystem( "teleportedin_red" );
	PrecacheParticleSystem( "teleportedin_blue" );

	PrecacheParticleSystem( "teleporter_arms_circle_red_blink" );
	PrecacheParticleSystem( "teleporter_arms_circle_blue_blink" );

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CObjectTeleporter::PlayerCanBeTeleported( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return false;

	if ( pPlayer->HasTheFlag() )
	{
		if ( !CTFPlayerDestructionLogic::GetRobotDestructionLogic() || ( CTFPlayerDestructionLogic::GetRobotDestructionLogic()->GetType() != CTFPlayerDestructionLogic::TYPE_PLAYER_DESTRUCTION ) )
			return false;
	}

	CTFPlayer *pBuilder = GetBuilder();
	if ( !pBuilder && m_bWasMapPlaced == false )
		return false;

	if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) )
		return true;

	if ( pBuilder && pBuilder->GetTeamNumber() != pPlayer->GetTeamNumber() )
		return false;

	if ( m_bWasMapPlaced && GetTeamNumber() != pPlayer->GetTeamNumber() )
		return false;

	if ( TFGameRules() && TFGameRules()->IsPasstimeMode() && pPlayer->m_Shared.HasPasstimeBall() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CObjectTeleporter::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch(pOther);

	if ( m_hReservedForPlayer == pOther )
	{
		m_flReserveAfterTouchUntil = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CObjectTeleporter::EndTouch( CBaseEntity *pOther )
{
	BaseClass::EndTouch(pOther);

	if ( m_hReservedForPlayer == pOther )
	{
		// Players can push the reserved player off the teleporter. So after the player falls off the teleporter
		// we allow him to continue reserving it for a short time.
		m_flReserveAfterTouchUntil = gpGlobals->curtime + 2.0;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CObjectTeleporter::TeleporterTouch( CBaseEntity *pOther )
{
	if ( IsDisabled() )
	{
		return;
	}

	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	if ( !PlayerCanBeTeleported( pPlayer ) )
	{
		// are we able to teleport?
		if ( pPlayer->HasTheFlag() )
		{
			// If they have the flag, print a warning that you can't tele with the flag
			CSingleUserRecipientFilter filter( pPlayer );
			TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_NO_TELE_WITH_FLAG );
		}
		else if ( pPlayer->m_Shared.HasPasstimeBall() )
		{
			CSingleUserRecipientFilter filter( pPlayer );
			TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_PASSTIME_NO_TELE );
		}

		if ( m_hReservedForPlayer == pPlayer )
		{
			m_hReservedForPlayer = NULL;
		}

		return;
	}


	int iBiDirectional = 0;
	if ( GetOwner() )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( GetOwner(), iBiDirectional, bidirectional_teleport );
	}

	if ( IsEntrance() || iBiDirectional == 1 )
	{
		// Reserve ourselves for the first player who touches us.
		// Players can push the reserved player off the teleporter. So after the player falls off the teleporter
		// we allow him to continue reserving it for a short time.
		bool bSetReserved = !m_hReservedForPlayer;
		if ( !bSetReserved )
		{
			 bSetReserved = ( !PlayerCanBeTeleported(m_hReservedForPlayer) || !m_hReservedForPlayer->IsAlive() || 
							  (m_flReserveAfterTouchUntil != 0 && m_flReserveAfterTouchUntil < gpGlobals->curtime) );
		}

		if ( bSetReserved )
		{
			m_hReservedForPlayer = pPlayer;
			m_flReserveAfterTouchUntil = 0;
		}

		// If we're reserved for another player, ignore me
		if ( m_hReservedForPlayer != pPlayer )
			return;

		if ( ( m_iState == TELEPORTER_STATE_READY ) )
		{
			// get the velocity of the player touching the teleporter
			if ( pPlayer->GetAbsVelocity().LengthSqr() < (5.0*5.0) )
			{
				CObjectTeleporter *pDest = GetMatchingTeleporter();

				if ( pDest )
				{
					TeleporterSend( pPlayer );
				}
			}
			else
			{
				// If it's been some time since we went active, and the reserved player still 
				// hasn't teleported, we clear his reservation to prevent griefing.
				if ( gpGlobals->curtime - m_flLastStateChangeTime > 3.0 )
				{
					m_hReservedForPlayer = NULL;
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CObjectTeleporter::Command_Repair( CTFPlayer *pActivator, float flAmount, float flRepairMod, float flRepairToMetalRatio /*= 3.f*/, bool bSendEvent /*= false*/ )
{
	// Teleporter-specific: 5 health costs 1 metal
	flRepairToMetalRatio = 5.f;

	int iRepairAmount = BaseClass::Command_Repair( pActivator, flAmount, flRepairMod, flRepairToMetalRatio, bSendEvent );
	if ( iRepairAmount > 0 )
	{
		// add the same amount of health to our match
		CObjectTeleporter *pMatch = GetMatchingTeleporter();
		if ( pMatch )
		{
			pMatch->AddHealth( iRepairAmount );
		}

		return iRepairAmount;
	}
	// Nothing repaired - see if our matching teleporter needs repair
	else
	{
		CObjectTeleporter *pMatch = GetMatchingTeleporter();
		if ( pMatch && !pMatch->IsBuilding() )
		{
			float flRepairAmountMax = flAmount * flRepairMod;
			int iRepairAmount = Min( flRepairAmountMax, pMatch->GetMaxHealth() - pMatch->GetHealth() );
			int iRepairCost = ceil( (float)iRepairAmount / flRepairToMetalRatio );
			if ( iRepairCost > pActivator->GetBuildResources() )
			{
				// What can we afford?
				iRepairCost = pActivator->GetBuildResources();
			}

			TRACE_OBJECT( UTIL_VarArgs( "%0.2f CObjectTeleporter::Command_Repair ( %f / %d ) - cost = %d\n", gpGlobals->curtime, 
				pMatch->GetHealth(),
				pMatch->GetMaxHealth(),
				iRepairCost ) );

			if ( iRepairCost > 0 )
			{
				iRepairAmount = iRepairCost * flRepairToMetalRatio;
				pActivator->RemoveBuildResources( iRepairCost );
				pMatch->SetHealth( pMatch->GetHealth() + iRepairAmount );

				return iRepairAmount;
			}
		}
	}
				
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Is this teleporter connected and functional? (ie: not sapped, disabled, upgrading, unconnected, etc)
//-----------------------------------------------------------------------------
bool CObjectTeleporter::IsReady( void )
{
	if ( !IsMatchingTeleporterReady() )
		return false;

	return GetState() != TELEPORTER_STATE_BUILDING && !IsUpgrading() && !IsDisabled();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CObjectTeleporter::IsMatchingTeleporterReady( void )
{
	if ( m_hMatchingTeleporter.Get() == NULL )
	{
		m_hMatchingTeleporter = FindMatch();
	}

	if ( m_hMatchingTeleporter &&
		m_hMatchingTeleporter->GetState() != TELEPORTER_STATE_BUILDING && 
		!m_hMatchingTeleporter->IsUpgrading() &&
		!m_hMatchingTeleporter->IsDisabled() )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if we are in the process of teleporting the given player
//-----------------------------------------------------------------------------
bool CObjectTeleporter::IsSendingPlayer( CTFPlayer *pPlayer )
{
	return ( GetState() == TELEPORTER_STATE_SENDING && m_hTeleportingPlayer == pPlayer );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CObjectTeleporter::CheckUpgradeOnHit( CTFPlayer *pPlayer )
{
	if ( BaseClass::CheckUpgradeOnHit( pPlayer ) )
	{
		CopyUpgradeStateToMatch( GetMatchingTeleporter(), false );
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectTeleporter::CopyUpgradeStateToMatch( CObjectTeleporter *pMatch, bool bFrom )
{
	// Copy our upgrade state to the matching teleporter
	if ( pMatch )
	{
		if ( bFrom )
		{
			pMatch->CopyUpgradeStateToMatch( pMatch, false ); 
		}
		else
		{
			pMatch->m_iHighestUpgradeLevel = m_iHighestUpgradeLevel;
			pMatch->m_iUpgradeLevel = m_iUpgradeLevel;
			pMatch->m_iUpgradeMetal = m_iUpgradeMetal;
			pMatch->m_iUpgradeMetalRequired = m_iUpgradeMetalRequired;
			pMatch->m_nDefaultUpgradeLevel = m_nDefaultUpgradeLevel;
			pMatch->m_flUpgradeCompleteTime = m_flUpgradeCompleteTime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CObjectTeleporter *CObjectTeleporter::GetMatchingTeleporter( void )
{
	return m_hMatchingTeleporter.Get();
}

void CObjectTeleporter::DeterminePlaybackRate( void )
{
	float flPlaybackRate = GetPlaybackRate();

	bool bWasBelowFullSpeed = ( flPlaybackRate < 1.0f );

	if ( IsBuilding() )
	{
		// Fall back to standard object building to handle reverse sappers without duplicating code
		BaseClass::DeterminePlaybackRate();
		return;
	}
	else if ( IsPlacing() )
	{
		SetPlaybackRate( 1.0f );
	}
	else
	{
		float flFrameTime = 0.1;	// BaseObjectThink delay

		switch( m_iState )
		{
		case TELEPORTER_STATE_READY:	
			{
				// spin up to 1.0 from whatever we're at, at some high rate
				flPlaybackRate = Approach( 1.0f, flPlaybackRate, 0.5f * flFrameTime );
			}
			break;

		case TELEPORTER_STATE_RECHARGING:
			{
				// Recharge - spin down to low and back up to full speed over the recharge time

				float flTotalTime = m_flCurrentRechargeDuration;
				float flFirstStage = flTotalTime * 0.4;
				float flSecondStage = flTotalTime * 0.6;

				// 0 -> 4, spin to low
				// 4 -> 6, stay at low
				// 6 -> 10, spin to 1.0

				float flTimeSinceChange = gpGlobals->curtime - m_flLastStateChangeTime;

				float flLowSpinSpeed = 0.15f;

				if ( flTimeSinceChange <= flFirstStage )
				{
					flPlaybackRate = RemapVal( gpGlobals->curtime,
						m_flLastStateChangeTime,
						m_flLastStateChangeTime + flFirstStage,
						1.0f,
						flLowSpinSpeed );
				}
				else if ( flTimeSinceChange > flFirstStage && flTimeSinceChange <= flSecondStage )
				{
					flPlaybackRate = flLowSpinSpeed;
				}
				else
				{
					flPlaybackRate = RemapVal( gpGlobals->curtime,
						m_flLastStateChangeTime + flSecondStage,
						m_flLastStateChangeTime + flTotalTime,
						flLowSpinSpeed,
						1.0f );
				}
			}		
			break;

		default:
			{
				if ( m_flLastStateChangeTime <= 0.0f )
				{
					flPlaybackRate = 0.0f;
				}
				else
				{
					// lost connect - spin down to 0.0 from whatever we're at, slowish rate
					flPlaybackRate = Approach( 0.0f, flPlaybackRate, 0.25f * flFrameTime );
				}
			}
			break;
		}

		// Always spin when the teleporter is done building
		if ( TFGameRules()->IsMannVsMachineMode() && GetTeamNumber() == TF_TEAM_PVE_INVADERS )
		{
			flPlaybackRate = 1.f;
		}

		SetPlaybackRate( flPlaybackRate );
	}

	bool bBelowFullSpeed = ( GetPlaybackRate() < 1.0f );

	if ( m_iBlurBodygroup >= 0 && bBelowFullSpeed != bWasBelowFullSpeed )
	{
		if ( bBelowFullSpeed )
		{
			SetBodygroup( m_iBlurBodygroup, 0 );	// turn off blur bodygroup
		}
		else
		{
			SetBodygroup( m_iBlurBodygroup, 1 );	// turn on blur bodygroup
		}
	}

	StudioFrameAdvance();
}

//-----------------------------------------------------------------------------
// Purpose: Teleport a player to us
//-----------------------------------------------------------------------------
void CObjectTeleporter::RecieveTeleportingPlayer( CTFPlayer* pTeleportingPlayer )
{
	if ( !pTeleportingPlayer || IsMarkedForDeletion() )
		return;

	// get the position we'll move the player to
	Vector newPosition = GetAbsOrigin();
	newPosition.z += TELEPORTER_MAXS.z + 1;

	// Telefrag anyone in the way
	CBaseEntity *pEnts[256];
	Vector mins, maxs;
	Vector expand( 4, 4, 4 );

	mins = newPosition + VEC_HULL_MIN - expand;
	maxs = newPosition + VEC_HULL_MAX + expand;

	// move the player
	if ( pTeleportingPlayer )
	{
		CUtlVector<CBaseEntity*> hPlayersToKill;
		bool bClear = true;

		// Telefrag any players in the way
		int numEnts = UTIL_EntitiesInBox( pEnts, 256, mins,	maxs, 0 );
		if ( numEnts )
		{
			//Iterate through the list and check the results
			for ( int i = 0; i < numEnts && bClear; i++ )
			{
				if ( pEnts[i] == NULL )
					continue;

				if ( pEnts[i] == this )
					continue;

				// kill players
				if ( pEnts[i]->IsPlayer() && ( pEnts[i]->GetTeamNumber() >= FIRST_GAME_TEAM ) )
				{
					if ( !pTeleportingPlayer->InSameTeam( pEnts[i] ) && ( pTeleportingPlayer->GetTeamNumber() >= FIRST_GAME_TEAM ) )
					{
						hPlayersToKill.AddToTail( pEnts[i] );
					}
					continue;
				}

				if ( pEnts[i]->IsBaseObject() )
					continue;

				// Solid entities will prevent a teleport
				if ( pEnts[i]->IsSolid() && pEnts[i]->ShouldCollide( pTeleportingPlayer->GetCollisionGroup(), MASK_SOLID ) &&
						g_pGameRules->ShouldCollide( pTeleportingPlayer->GetCollisionGroup(), pEnts[i]->GetCollisionGroup() ) )
				{
					// HACK to solve the problem of building teleporter exits in CDynamicProp entities at
					// the end of maps like Badwater that have the VPhysics explosions when the point is capped
					CDynamicProp *pProp = dynamic_cast<CDynamicProp *>( pEnts[i] );
					if ( !pProp )
					{
 						CBaseProjectile *pProjectile = dynamic_cast<CBaseProjectile *>( pEnts[i] );
 						if ( !pProjectile )
 						{
							bClear = false;
						}
					}
					else
					{
						if ( !pProp->IsEffectActive( EF_NODRAW ) )
						{
							// We're going to teleport into something solid. Abort & destroy this exit.
							bClear = false;
						}
					}

					// need to make sure we're really overlapping geometry and not just overlapping the bounding boxes
					if ( !bClear )
					{
						Ray_t ray;
						ray.Init( newPosition, newPosition, VEC_HULL_MIN - expand, VEC_HULL_MAX + expand );

						trace_t trace;
						enginetrace->ClipRayToEntity( ray, MASK_PLAYERSOLID, pEnts[i], &trace );
						if ( trace.fraction >= 1.0f )
						{
							// not overlapping geometry so reset our check
							bClear = true;
						}
					}
				}
			}
		}

		if ( bClear )
		{
			// Telefrag all enemy players we've found
			for ( int player = 0; player < hPlayersToKill.Count(); player++ )
			{
				hPlayersToKill[player]->TakeDamage( CTakeDamageInfo( pTeleportingPlayer, pTeleportingPlayer, 1000, DMG_CRUSH, TF_DMG_CUSTOM_TELEFRAG ) );
			}

			pTeleportingPlayer->Teleport( &newPosition, &(GetAbsAngles()), &vec3_origin );

			// Unzoom if we are a sniper zoomed!
			pTeleportingPlayer->m_Shared.InstantlySniperUnzoom();

			pTeleportingPlayer->SetFOV( pTeleportingPlayer, 0, tf_teleporter_fov_time.GetFloat(), tf_teleporter_fov_start.GetInt() );

			color32 fadeColor = {255,255,255,100};
			UTIL_ScreenFade( pTeleportingPlayer, fadeColor, 0.25, 0.4, FFADE_IN );

			// 1/20 of te time teleport bread -- except for Soldier who does it 1/3 of the time.
			int nMax = pTeleportingPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SOLDIER  ? 2 : 19;
			if ( RandomInt( 0, nMax ) == 0 )
			{
				SpawnBread( pTeleportingPlayer );
			}
		}
		else
		{
			DetonateObject();
		}
	}			
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CObjectTeleporter::TeleporterThink( void )
{
	if ( IsCarried() )
		return;

	SetContextThink( &CObjectTeleporter::TeleporterThink, gpGlobals->curtime + BUILD_TELEPORTER_NEXT_THINK, TELEPORTER_THINK_CONTEXT );

	// At any point, if our match is not ready, revert to IDLE
	if ( IsDisabled() || IsMatchingTeleporterReady() == false )
	{
		if ( GetState() != TELEPORTER_STATE_IDLE && GetState() != TELEPORTER_STATE_UPGRADING )
		{
			SetState( TELEPORTER_STATE_IDLE );
			ShowDirectionArrow( false );
		}
		return;
	}

	if ( m_flMyNextThink && m_flMyNextThink > gpGlobals->curtime )
		return;

	// pMatch is not NULL and is not building
	CObjectTeleporter *pMatch = GetMatchingTeleporter();

	int iBiDirectional = 0;

	if ( GetOwner() )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( GetOwner(), iBiDirectional, bidirectional_teleport );
	}

	switch ( m_iState )
	{
	// Teleporter is not yet active, do nothing
	case TELEPORTER_STATE_BUILDING:
	case TELEPORTER_STATE_UPGRADING:
		ShowDirectionArrow( false );
		break;

	default:
	case TELEPORTER_STATE_IDLE:
		// Do we have a match that is active?
		if ( IsMatchingTeleporterReady() )
		{
			SetState( TELEPORTER_STATE_READY );
			EmitSound( "Building_Teleporter.Ready" );

			if ( IsEntrance() || iBiDirectional == 1 )
			{
				ShowDirectionArrow( true );
			}
		}
		break;

	case TELEPORTER_STATE_READY:
		if ( IsEntrance() || iBiDirectional == 1 )
		{
			ShowDirectionArrow( true );
		}
		break;

	case TELEPORTER_STATE_SENDING:
		{
			pMatch->TeleporterReceive( m_hTeleportingPlayer, 1.0 );

			m_flCurrentRechargeDuration = (float)g_iTeleporterRechargeTimes[GetUpgradeLevel()];

			if ( !m_bWasMapPlaced )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBuilder(), m_flCurrentRechargeDuration, mult_teleporter_recharge_rate );
			}

			m_flRechargeTime = gpGlobals->curtime + ( BUILD_TELEPORTER_FADEOUT_TIME + BUILD_TELEPORTER_FADEIN_TIME + m_flCurrentRechargeDuration );
		
			// change state to recharging...
			SetState( TELEPORTER_STATE_RECHARGING );
		}
		break;

	case TELEPORTER_STATE_RECEIVING:
		{
			RecieveTeleportingPlayer( m_hTeleportingPlayer.Get() );

			SetState( TELEPORTER_STATE_RECEIVING_RELEASE );

			m_flMyNextThink = gpGlobals->curtime + ( BUILD_TELEPORTER_FADEIN_TIME );
		}
		break;

	case TELEPORTER_STATE_RECEIVING_RELEASE:
		{
			CTFPlayer *pTeleportingPlayer = m_hTeleportingPlayer.Get();

			if ( pTeleportingPlayer )
			{
				pTeleportingPlayer->TeleportEffect();
				pTeleportingPlayer->m_Shared.RemoveCond( TF_COND_SELECTED_TO_TELEPORT );
				CTF_GameStats.Event_PlayerUsedTeleport( GetBuilder(), pTeleportingPlayer );

				pTeleportingPlayer->SpeakConceptIfAllowed( MP_CONCEPT_TELEPORTED );

				IGameEvent * event = gameeventmanager->CreateEvent( "player_teleported" );
				if ( event )
				{
					event->SetInt( "userid", pTeleportingPlayer->GetUserID() );
					event->SetInt( "builderid", GetBuilder() ? GetBuilder()->GetUserID() : 0 );
					if ( GetMatchingTeleporter() )
					{
						event->SetFloat( "dist", GetMatchingTeleporter()->GetAbsOrigin().DistTo( GetAbsOrigin() ) );					
					}
					else
					{
						event->SetFloat( "dist", 0 );
					}
					gameeventmanager->FireEvent( event );
				}
			}

			// reset the pointers to the player now that we're done teleporting
			SetTeleportingPlayer( NULL );
			pMatch->SetTeleportingPlayer( NULL );

			SetState( TELEPORTER_STATE_RECHARGING );

			m_flCurrentRechargeDuration = (float)g_iTeleporterRechargeTimes[GetUpgradeLevel()];
			m_flMyNextThink = gpGlobals->curtime + m_flCurrentRechargeDuration;
		}
		break;

	case TELEPORTER_STATE_RECHARGING:
		// If we are finished recharging, go active
		if ( gpGlobals->curtime > m_flRechargeTime )
		{
			SetState( TELEPORTER_STATE_READY );
			EmitSound( "Building_Teleporter.Ready" );
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectTeleporter::FinishedBuilding( void )
{
	BaseClass::FinishedBuilding();

	if ( GetTeamNumber() == TF_TEAM_PVE_INVADERS )
	{
		TFObjectiveResource()->IncrementTeleporterCount();
	}

	SetActivity( ACT_OBJ_RUNNING );
	SetPlaybackRate( 0.0f );
}

void CObjectTeleporter::SetState( int state )
{
	if ( state != m_iState )
	{
		m_iState = state;
		m_flLastStateChangeTime = gpGlobals->curtime;
	}
}

void CObjectTeleporter::ShowDirectionArrow( bool bShow )
{
	if ( bShow != m_bShowDirectionArrow )
	{
		if ( m_iDirectionBodygroup >= 0 )
		{
			SetBodygroup( m_iDirectionBodygroup, bShow ? 1 : 0 );
		}
			
		m_bShowDirectionArrow = bShow;

		if ( bShow )
		{
			CObjectTeleporter *pMatch = GetMatchingTeleporter();

			Assert( pMatch );

			Vector vecToOwner = pMatch->GetAbsOrigin() - GetAbsOrigin();
			QAngle angleToExit;
			VectorAngles( vecToOwner, Vector(0,0,1), angleToExit );
			angleToExit -= GetAbsAngles();

			// pose param is flipped and backwards, adjust.
			m_flYawToExit = anglemod( -angleToExit.y + 180 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CObjectTeleporter::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		CObjectTeleporter *pMatch = GetMatchingTeleporter();

		char tempstr[512];

		// match
		Q_snprintf( tempstr, sizeof( tempstr ), "Match Found: %s", ( pMatch != NULL ) ? "Yes" : "No" );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		// state
		Q_snprintf( tempstr, sizeof( tempstr ), "State: %d", m_iState.Get() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		// recharge time
		if ( gpGlobals->curtime < m_flRechargeTime )
		{
			float flPercent = ( m_flRechargeTime - gpGlobals->curtime ) / m_flCurrentRechargeDuration;

			Q_snprintf( tempstr, sizeof( tempstr ), "Recharging: %.1f", flPercent );
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CObjectTeleporter* CObjectTeleporter::FindMatch( void )
{
	int iObjType = GetType();
	CObjectTeleporter *pMatch = NULL;

	CTFPlayer *pBuilder = GetBuilder();
	Assert( pBuilder || m_bWasMapPlaced );
	if ( !pBuilder )
	{
		return NULL;
	}

	int i;
	int iNumObjects = pBuilder->GetObjectCount();
	for ( i=0; i<iNumObjects; i++ )
	{
		CBaseObject *pObj = pBuilder->GetObject(i);

		if ( pObj && (pObj != this) && (iObjType == pObj->GetType()) )
		{
			CObjectTeleporter *pTele = dynamic_cast<CObjectTeleporter*>(pObj);
			if ( pTele && (( IsEntrance() && pTele->IsExit() ) ||
				           ( IsExit() && pTele->IsEntrance() )) )
			{
				pMatch = pTele;
				CObjectTeleporter* pOtherMatch = pMatch->GetMatchingTeleporter();
				if ( pOtherMatch && pOtherMatch != this )
				{
					pMatch = NULL;
					continue;
				}
				break;
			}
		}
	}

	if ( pMatch )
	{
		// Select the teleporter with the most upgrade
		bool bFrom = (pMatch->GetUpgradeLevel() > GetUpgradeLevel() || pMatch->GetUpgradeMetal() > GetUpgradeMetal() );
		CopyUpgradeStateToMatch( pMatch, bFrom );
	}

	return pMatch;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CObjectTeleporter::Explode( void )
{
	CObjectTeleporter *pMatch = GetMatchingTeleporter();
	if ( pMatch )
	{
		pMatch->m_iHighestUpgradeLevel = 1;
		pMatch->m_iUpgradeLevel = 1;
		pMatch->m_iUpgradeMetal = 0;

		int iHealth = pMatch->GetMaxHealthForCurrentLevel();
		pMatch->UpdateMaxHealth( iHealth, true );

		if ( pMatch->GetTeleportingPlayer() )
		{
			pMatch->GetTeleportingPlayer()->m_Shared.RemoveCond( TF_COND_SELECTED_TO_TELEPORT );
		}
		pMatch->SetTeleportingPlayer( NULL );
	}

	if ( m_hTeleportingPlayer.Get() )
	{
		m_hTeleportingPlayer.Get()->m_Shared.RemoveCond( TF_COND_SELECTED_TO_TELEPORT );
	}
	SetTeleportingPlayer( NULL );

	BaseClass::Explode();
}

//-----------------------------------------------------------------------------
// Purpose: Update the max health value and scale the health value to match
//-----------------------------------------------------------------------------
void CObjectTeleporter::UpdateMaxHealth( int nHealth, bool bForce /* = false */ )
{
	if ( m_bCarryDeploy && !bForce )
		return;

	float flPercentageHealth = (float)GetHealth()/(float)GetMaxHealth();
	
	SetMaxHealth( nHealth );
	SetHealth( nHealth * flPercentageHealth );
}

//-----------------------------------------------------------------------------
// Purpose: Raises the Teleporter one level
//-----------------------------------------------------------------------------
void CObjectTeleporter::StartUpgrading( void )
{
	// Call our base class upgrading first to update our health and maxhealth
	BaseClass::StartUpgrading();

	// Tell our partner to match his maxhealth to ours
	CObjectTeleporter *pMatch = GetMatchingTeleporter();
	if ( pMatch && !m_bCarryDeploy && !pMatch->m_bCarryDeploy )
	{
		pMatch->UpdateMaxHealth( GetMaxHealth() );
	}

	SetState( TELEPORTER_STATE_UPGRADING );
}

void CObjectTeleporter::FinishUpgrading( void )
{
	SetState( TELEPORTER_STATE_IDLE );

	if ( ShouldQuickBuild() )
	{
		// See if we have a lower level match and upgrade them
		if ( m_hMatchingTeleporter.Get() && m_hMatchingTeleporter->GetUpgradeLevel() < GetUpgradeLevel() )
		{
			CopyUpgradeStateToMatch( m_hMatchingTeleporter, false );
		}
	}

	BaseClass::FinishUpgrading();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CObjectTeleporter::InputWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector hitLoc )
{
	return BaseClass::InputWrenchHit( pPlayer, pWrench, hitLoc );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CObjectTeleporter::MakeCarriedObject( CTFPlayer *pCarrier )
{
	ShowDirectionArrow( false );

	BaseClass::MakeCarriedObject( pCarrier );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CObjectTeleporter::InputEnable( inputdata_t &inputdata )
{
	BaseClass::InputEnable( inputdata );

	if ( !IsDisabled() )
	{
		if ( m_hMatchingTeleporter && m_hMatchingTeleporter->IsDisabled() )
		{
			m_hMatchingTeleporter->UpdateDisabledState();
			if ( !m_hMatchingTeleporter->IsDisabled() )
			{
				m_hMatchingTeleporter->OnGoActive();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CObjectTeleporter::InputDisable( inputdata_t &inputdata )
{
	BaseClass::InputDisable( inputdata );

 	if ( m_hMatchingTeleporter && !m_hMatchingTeleporter->IsDisabled() )
 	{
 		m_hMatchingTeleporter->SetDisabled( true );
 		m_hMatchingTeleporter->OnGoInactive();
 	}
}

void CObjectTeleporter::SpawnBread( const CTFPlayer* pTeleportingPlayer )
{
	if( !pTeleportingPlayer )
		return;

	const char* pszModelName = g_pszBreadModels[ RandomInt( 0, TF_LAST_NORMAL_CLASS - TF_FIRST_NORMAL_CLASS - 1 ) ];
	CPhysicsProp *pProp = NULL;

	MDLHandle_t h = mdlcache->FindMDL( pszModelName );
	if ( h != MDLHANDLE_INVALID )
	{
		// Must have vphysics to place as a physics prop
		studiohdr_t *pStudioHdr = mdlcache->GetStudioHdr( h );
		if ( pStudioHdr && mdlcache->GetVCollide( h ) )
		{	
			// Try to create entity
			pProp = dynamic_cast< CPhysicsProp * >( CreateEntityByName( "prop_physics_override" ) );
			if ( pProp )
			{
				Vector vecSpawn = GetAbsOrigin();
				vecSpawn.z += TELEPORTER_MAXS.z + 50;
				QAngle qSpawnAngles = GetAbsAngles();
				pProp->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
				// so it can be pushed by airblast
				pProp->AddFlag( FL_GRENADE );
				// so that it will always be interactable with the player
				char buf[512];
				// Pass in standard key values
				Q_snprintf( buf, sizeof(buf), "%.10f %.10f %.10f", vecSpawn.x, vecSpawn.y, vecSpawn.z );
				pProp->KeyValue( "origin", buf );
				Q_snprintf( buf, sizeof(buf), "%.10f %.10f %.10f", qSpawnAngles.x, qSpawnAngles.y, qSpawnAngles.z );
				pProp->KeyValue( "angles", buf );
				pProp->KeyValue( "model", pszModelName );
				pProp->KeyValue( "fademindist", "-1" );
				pProp->KeyValue( "fademaxdist", "0" );
				pProp->KeyValue( "fadescale", "1" );
				pProp->KeyValue( "inertiaScale", "1.0" );
				pProp->KeyValue( "physdamagescale", "0.1" );
				pProp->Precache();
				DispatchSpawn( pProp );
				pProp->m_takedamage = DAMAGE_YES;	// Take damage, otherwise this can block trains
				pProp->SetHealth( 5000 );
				pProp->Activate();
				IPhysicsObject *pPhysicsObj = pProp->VPhysicsGetObject();
				if ( pPhysicsObj )
				{
					AngularImpulse angImpulse( RandomFloat( -100, 100 ), RandomFloat( -100, 100 ), RandomFloat( -100, 100 ) );
					Vector vForward;
					AngleVectors( qSpawnAngles, &vForward );
					Vector vecVel = ( vForward * 100 ) + Vector( 0, 0, 200 ) + RandomVector( -50, 50 );
					pPhysicsObj->SetVelocityInstantaneous( &vecVel, &angImpulse );
				}

				// Die in 10 seconds
				pProp->ThinkSet( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 10, "DieContext" );
			}
		}

		mdlcache->Release( h ); // counterbalance addref from within FindMDL
	}
}

void CObjectTeleporter::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "player_spawn" ) ||
		 FStrEq( event->GetName(), "player_team" ) )
	{
		// On instant-spawn servers, players can change teams just as the teleporter
		// queues them for a teleport and will still teleport them even if they respawn / change team.
		//
		// If we hear a spawn or team-change event for our queued player, clear them from the queue
		if ( !m_hTeleportingPlayer.Get() )
			return;

		const int iUserID = event->GetInt( "userid" );
		if ( iUserID == m_hTeleportingPlayer->GetUserID() )
		{
			SetTeleportingPlayer( NULL );
		}
	}
}