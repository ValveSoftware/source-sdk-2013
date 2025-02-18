// NextBotCombatCharacter.cpp
// Next generation bot system
// Author: Michael Booth, April 2005
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "team.h"
#include "CRagdollMagnet.h"

#include "NextBot.h"
#include "NextBotLocomotionInterface.h"
#include "NextBotBodyInterface.h"

#ifdef TERROR
#include "TerrorGamerules.h"
#endif

#include "vprof.h"
#include "datacache/imdlcache.h"
#include "EntityFlame.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar NextBotStop( "nb_stop", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Stop all NextBots" );


//--------------------------------------------------------------------------------------------------------
class CSendBotCommand
{
public:
	CSendBotCommand( const char *command )
	{
		m_command = command;
	}

	bool operator() ( INextBot *bot )
	{
		bot->OnCommandString( m_command );
		return true;
	}

	const char *m_command;
};


CON_COMMAND_F( nb_command, "Sends a command string to all bots", FCVAR_CHEAT )
{
	if ( args.ArgC() <= 1 )
	{
		Msg( "Missing command string" );
		return;
	}

	CSendBotCommand sendCmd( args.ArgS() );
	TheNextBots().ForEachBot( sendCmd );
}



//-----------------------------------------------------------------------------------------------------
BEGIN_DATADESC( NextBotCombatCharacter )

	DEFINE_THINKFUNC( DoThink ),

END_DATADESC()


//-----------------------------------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST( NextBotCombatCharacter, DT_NextBot )
END_SEND_TABLE()

//-----------------------------------------------------------------------------------------------------

BEGIN_ENT_SCRIPTDESC( NextBotCombatCharacter, CBaseCombatCharacter, "Nextbot combat character" )
	DEFINE_SCRIPTFUNC( GetBotId, "Get this bot's id" )
	DEFINE_SCRIPTFUNC( FlagForUpdate, "Flag this bot for update" )
	DEFINE_SCRIPTFUNC( IsFlaggedForUpdate, "Is this bot flagged for update" )
	DEFINE_SCRIPTFUNC( GetTickLastUpdate, "Get last update tick" )
	DEFINE_SCRIPTFUNC_WRAPPED( GetLocomotionInterface, "Get this bot's locomotion interface" )
	DEFINE_SCRIPTFUNC_WRAPPED( GetBodyInterface, "Get this bot's body interface" )
	DEFINE_SCRIPTFUNC_WRAPPED( GetIntentionInterface, "Get this bot's intention interface" )
	DEFINE_SCRIPTFUNC_WRAPPED( GetVisionInterface, "Get this bot's vision interface" )
	DEFINE_SCRIPTFUNC_WRAPPED( IsEnemy, "Return true if given entity is our enemy" )
	DEFINE_SCRIPTFUNC_WRAPPED( IsFriend, "Return true if given entity is our friend" )
	DEFINE_SCRIPTFUNC( IsImmobile, "Return true if we haven't moved in awhile" )
	DEFINE_SCRIPTFUNC( GetImmobileDuration, "How long have we been immobile" )
	DEFINE_SCRIPTFUNC( ClearImmobileStatus, "Clear immobile status" )
	DEFINE_SCRIPTFUNC( GetImmobileSpeedThreshold, "Return units/second below which this actor is considered immobile" )
END_SCRIPTDESC();

//-----------------------------------------------------------------------------------------------------
NextBotDestroyer::NextBotDestroyer( int team )
{
	m_team = team;
}


//-----------------------------------------------------------------------------------------------------
bool NextBotDestroyer::operator() ( INextBot *bot  )
{
	if ( m_team == TEAM_ANY || bot->GetEntity()->GetTeamNumber() == m_team )
	{
		// players need to be kicked, not deleted
		if ( bot->GetEntity()->IsPlayer() )
		{
			CBasePlayer *player = dynamic_cast< CBasePlayer * >( bot->GetEntity() );
			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", player->GetUserID() ) );
		}
		else
		{
			UTIL_Remove( bot->GetEntity() );
		}
	}
	return true;
}


//-----------------------------------------------------------------------------------------------------
CON_COMMAND_F( nb_delete_all, "Delete all non-player NextBot entities.", FCVAR_CHEAT )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CTeam *team = NULL;

	if ( args.ArgC() == 2 )
	{
		const char *teamName = args[1];

		for( int i=0; i < g_Teams.Count(); ++i )
		{
			if ( FStrEq( teamName, g_Teams[i]->GetName() ) )
			{
				// delete all bots on this team
				team = g_Teams[i];
				break;
			}
		}

		if ( team == NULL )
		{
			Msg( "Invalid team '%s'\n", teamName );
			return;
		}
	}

	// delete all bots on all teams
	NextBotDestroyer destroyer( team ? team->GetTeamNumber() : TEAM_ANY );
	TheNextBots().ForEachBot( destroyer );
}


//-----------------------------------------------------------------------------------------------------
class NextBotApproacher
{
public:
	NextBotApproacher( void )
	{
		CBasePlayer *player = UTIL_GetListenServerHost();
		if ( player )
		{
			Vector forward;
			player->EyeVectors( &forward );

			trace_t result;
			unsigned int mask = MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE | CONTENTS_GRATE | CONTENTS_WINDOW;
			UTIL_TraceLine( player->EyePosition(), player->EyePosition() + 999999.9f * forward, mask, player, COLLISION_GROUP_NONE, &result );
			if ( result.DidHit() )
			{
				NDebugOverlay::Cross3D( result.endpos, 5, 0, 255, 0, true, 10.0f );
				m_isGoalValid = true;
				m_goal = result.endpos;
			}
			else
			{
				m_isGoalValid = false;
			}
		}
	}

	bool operator() ( INextBot *bot )
	{
		if ( TheNextBots().IsDebugFilterMatch( bot ) )
		{
			bot->OnCommandApproach( m_goal );
		}
		return true;
	}

	bool m_isGoalValid;
	Vector m_goal;
};

CON_COMMAND_F( nb_move_to_cursor, "Tell all NextBots to move to the cursor position", FCVAR_CHEAT )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	NextBotApproacher approach;
	TheNextBots().ForEachBot( approach );
}


//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
bool IgnoreActorsTraceFilterFunction( IHandleEntity *pServerEntity, int contentsMask )
{
	CBaseEntity *entity = EntityFromEntityHandle( pServerEntity );
	return ( entity->MyCombatCharacterPointer() == NULL );	// includes all bots, npcs, players, and TF2 buildings
}


//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
bool VisionTraceFilterFunction( IHandleEntity *pServerEntity, int contentsMask )
{
	// Honor BlockLOS also to allow seeing through partially-broken doors
	CBaseEntity *entity = EntityFromEntityHandle( pServerEntity );
	return ( entity->MyCombatCharacterPointer() == NULL && entity->BlocksLOS() );
}


//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

NextBotCombatCharacter::NextBotCombatCharacter( void )

{
	m_lastAttacker = NULL;
	m_didModelChange = false;
}


//----------------------------------------------------------------------------------------------------------
void NextBotCombatCharacter::Spawn( void )
{
	BaseClass::Spawn();
	
	// reset bot components
	Reset();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	
	SetMoveType( MOVETYPE_CUSTOM );	
	
	SetCollisionGroup( COLLISION_GROUP_PLAYER );

	m_iMaxHealth = m_iHealth;
	m_takedamage = DAMAGE_YES;

	MDLCACHE_CRITICAL_SECTION();
	InitBoneControllers( ); 

	// set up think callback
	SetThink( &NextBotCombatCharacter::DoThink );
	SetNextThink( gpGlobals->curtime );

	m_lastAttacker = NULL;
}


bool NextBotCombatCharacter::IsAreaTraversable( const CNavArea *area ) const
{
	if ( !area )
		return false;
	ILocomotion *mover = GetLocomotionInterface();
	if ( mover && !mover->IsAreaTraversable( area ) )
		return false;
	return BaseClass::IsAreaTraversable( area );
}


//----------------------------------------------------------------------------------------------------------
void NextBotCombatCharacter::DoThink( void )
{
	VPROF_BUDGET( "NextBotCombatCharacter::DoThink", "NextBot" );

	SetNextThink( gpGlobals->curtime );

	if ( BeginUpdate() )
	{
		// emit model change event	
		if ( m_didModelChange )
		{
			m_didModelChange = false;

			OnModelChanged();

			// propagate model change into NextBot event responders
			for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
			{
				sub->OnModelChanged();
			}	
		}

		UpdateLastKnownArea();	

		// update bot components
		if ( !NextBotStop.GetBool() && (GetFlags() & FL_FROZEN) == 0 )
		{
			Update();
		}

		EndUpdate();
	}
}


//----------------------------------------------------------------------------------------------------------
void NextBotCombatCharacter::Touch( CBaseEntity *other )
{
	if ( ShouldTouch( other ) )
	{
		// propagate touch into NextBot event responders
		trace_t result;
		result = GetTouchTrace();

		// OnContact refers to *physical* contact, not triggers or other non-physical entities
		if ( result.DidHit() || other->MyCombatCharacterPointer() != NULL )
		{
			OnContact( other, &result );
		}
	}
	
	BaseClass::Touch( other );
}


//----------------------------------------------------------------------------------------------------------
void NextBotCombatCharacter::SetModel( const char *szModelName )
{
	// actually change the model
	BaseClass::SetModel( szModelName );
	
	// need to do a lazy-check because precache system also invokes this
	m_didModelChange = true;
}


//----------------------------------------------------------------------------------------------------------
void NextBotCombatCharacter::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
	BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );
	
	// propagate event to components
	OnIgnite();
}


//----------------------------------------------------------------------------------------------------------
void NextBotCombatCharacter::Ignite( float flFlameLifetime, CBaseEntity *pAttacker )
{
	if ( IsOnFire() )
		return;

	// BaseClass::Ignite stuff, plus SetAttacker on the flame, so our attacker gets credit
	CEntityFlame *pFlame = CEntityFlame::Create( this );
	if ( pFlame )
	{
		pFlame->SetLifetime( flFlameLifetime );
		AddFlag( FL_ONFIRE );

		SetEffectEntity( pFlame );
	}
	m_OnIgnite.FireOutput( this, this );

	// propagate event to components
	OnIgnite();
}


//----------------------------------------------------------------------------------------------------------
int NextBotCombatCharacter::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// track our last attacker
	if ( info.GetAttacker() && info.GetAttacker()->MyCombatCharacterPointer() )
	{
		m_lastAttacker = info.GetAttacker()->MyCombatCharacterPointer();
	}

	// propagate event to components
	OnInjured( info );
	
	return CBaseCombatCharacter::OnTakeDamage_Alive( info );
}


//----------------------------------------------------------------------------------------------------------
int NextBotCombatCharacter::OnTakeDamage_Dying( const CTakeDamageInfo &info )
{
	// track our last attacker	
	if ( info.GetAttacker()->MyCombatCharacterPointer() )
	{
		m_lastAttacker = info.GetAttacker()->MyCombatCharacterPointer();
	}

	// propagate event to components
	OnInjured( info );

	return CBaseCombatCharacter::OnTakeDamage_Dying( info );
}

//----------------------------------------------------------------------------------------------------------
/**
 * Can't use CBaseCombatCharacter's Event_Killed because it will immediately ragdoll us
 */
static int g_DeathStartEvent = 0;
void NextBotCombatCharacter::Event_Killed( const CTakeDamageInfo &info )
{
	// track our last attacker
	if ( info.GetAttacker() && info.GetAttacker()->MyCombatCharacterPointer() )
	{
		m_lastAttacker = info.GetAttacker()->MyCombatCharacterPointer();
	}

	// propagate event to my components
	OnKilled( info );
	
	// Advance life state to dying
	m_lifeState = LIFE_DYING;

#ifdef TERROR
	/*
	 * TODO: Make this game-generic
	 */
	// Create the death event just like players do.
	TerrorGameRules()->DeathNoticeForEntity( this, info );

	// Infected specific event
	TerrorGameRules()->DeathNoticeForInfected( this, info );
#endif

	if ( GetOwnerEntity() != NULL )
	{
		GetOwnerEntity()->DeathNotice( this );
	}

	// inform the other bots
	TheNextBots().OnKilled( this, info );
}


//----------------------------------------------------------------------------------------------------------
void NextBotCombatCharacter::PerformCustomPhysics( Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity )
{
	ILocomotion *mover = GetLocomotionInterface();
	if ( mover )
	{
		// hack to keep ground entity from being NULL'd when Z velocity is positive
		SetGroundEntity( mover->GetGround() );
	}
}


//----------------------------------------------------------------------------------------------------------
bool NextBotCombatCharacter::BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector )
{
	// See if there's a ragdoll magnet that should influence our force.
	Vector adjustedForceVector = forceVector;
	CRagdollMagnet *magnet = CRagdollMagnet::FindBestMagnet( this );
	if ( magnet )
	{
		adjustedForceVector += magnet->GetForceVector( this );
	}

	// clear the deceased's sound channels.(may have been firing or reloading when killed)
	EmitSound( "BaseCombatCharacter.StopWeaponSounds" );

	return BaseClass::BecomeRagdoll( info, adjustedForceVector );
}


//----------------------------------------------------------------------------------------------------------
void NextBotCombatCharacter::HandleAnimEvent( animevent_t *event )
{
	// propagate event to components
	OnAnimationEvent( event );
}


//----------------------------------------------------------------------------------------------------------
/**
 * Propagate event into NextBot event responders
 */
void NextBotCombatCharacter::OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea )
{
	INextBotEventResponder::OnNavAreaChanged( enteredArea, leftArea );

	BaseClass::OnNavAreaChanged( enteredArea, leftArea );
}


//----------------------------------------------------------------------------------------------------------
Vector NextBotCombatCharacter::EyePosition( void )
{
	if ( GetBodyInterface() )
	{
		return GetBodyInterface()->GetEyePosition();
	}

	return BaseClass::EyePosition();
}


//----------------------------------------------------------------------------------------------------------
/**
 * Return true if this object can be +used by the bot
 */
bool NextBotCombatCharacter::IsUseableEntity( CBaseEntity *entity, unsigned int requiredCaps )
{
	if ( entity )
	{
		int caps = entity->ObjectCaps();
		if ( caps & (FCAP_IMPULSE_USE|FCAP_CONTINUOUS_USE|FCAP_ONOFF_USE|FCAP_DIRECTIONAL_USE) )
		{
			if ( (caps & requiredCaps) == requiredCaps )
			{
				return true;
			}
		}
	}

	return false;
}


//----------------------------------------------------------------------------------------------------------
void NextBotCombatCharacter::UseEntity( CBaseEntity *entity, USE_TYPE useType )
{
	if ( IsUseableEntity( entity ) )
	{
		variant_t emptyVariant;
		entity->AcceptInput( "Use", this, this, emptyVariant, useType );
	}
}
