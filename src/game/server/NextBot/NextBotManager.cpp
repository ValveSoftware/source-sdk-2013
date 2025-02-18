// NextBotManager.cpp
// Author: Michael Booth, May 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "NextBotManager.h"
#include "NextBotInterface.h"

#ifdef TERROR
#include "ZombieBot/Infected/Infected.h"
#include "ZombieBot/Witch/Witch.h"
#include "ZombieManager.h"
#endif

#include "SharedFunctorUtils.h"
//#include "../../common/blackbox_helper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar ZombieMobMaxSize;

ConVar nb_update_frequency( "nb_update_frequency", ".1", FCVAR_CHEAT );
ConVar nb_update_framelimit( "nb_update_framelimit", ( IsDebug() ) ? "30" : "15", FCVAR_CHEAT );
ConVar nb_update_maxslide( "nb_update_maxslide", "2", FCVAR_CHEAT );
ConVar nb_update_debug( "nb_update_debug", "0", FCVAR_CHEAT );

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
/**
 * Singleton accessor.
 * By returning a reference, we guarantee construction of the 
 * instance before its first use.
 */
NextBotManager &TheNextBots( void )
{
	if ( NextBotManager::GetInstance() )
	{
		return *NextBotManager::GetInstance();
	}
	else
	{
		static NextBotManager manager;
		NextBotManager::SetInstance( &manager );
		return manager;
	}
}

NextBotManager* NextBotManager::sInstance = NULL;

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
static const char *debugTypeName[] =
{
	"BEHAVIOR",
	"LOOK_AT",
	"PATH",
	"ANIMATION",
	"LOCOMOTION",
	"VISION",
	"HEARING",
	"EVENTS",
	"ERRORS",
	NULL
};


static void CC_SetDebug( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Debugging stopped\n" );
		TheNextBots().SetDebugTypes( NEXTBOT_DEBUG_NONE );
		return;
	}

	int debugType = 0;

	for( int i=1; i<args.ArgC(); ++i )
	{
		int type;
		for( type = 0; debugTypeName[ type ]; ++type )
		{
			const char *token = args[i];

			// special token that means "all"
			if ( token[0] == '*' )
			{
				debugType = NEXTBOT_DEBUG_ALL;
				break;
			}

			if ( !Q_strnicmp( args[i], debugTypeName[ type ], Q_strlen( args[1] ) ) )
			{
				debugType |= ( 1 << type );
				break;
			}
		}

		if ( !debugTypeName[ type ] )
		{
			Msg( "Invalid debug type '%s'\n", args[i] );
		}
	}

	// enable debugging
	TheNextBots().SetDebugTypes( ( NextBotDebugType ) debugType );
}
static ConCommand SetDebug( "nb_debug", CC_SetDebug, "Debug NextBots.  Categories are: BEHAVIOR, LOOK_AT, PATH, ANIMATION, LOCOMOTION, VISION, HEARING, EVENTS, ERRORS.", FCVAR_CHEAT );

//---------------------------------------------------------------------------------------------
static void CC_SetDebugFilter( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Debug filter cleared.\n" );
		TheNextBots().DebugFilterClear();
		return;
	}

	for( int i=1; i<args.ArgC(); ++i )
	{
		int index = Q_atoi( args[i] );
		if ( index > 0 )
		{
			TheNextBots().DebugFilterAdd( index );
		}
		else
		{
			TheNextBots().DebugFilterAdd( args[i] );
		}
	}
}
static ConCommand SetDebugFilter( "nb_debug_filter", CC_SetDebugFilter, "Add items to the NextBot debug filter. Items can be entindexes or part of the indentifier of one or more bots.", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
class Selector
{
public:
	Selector( CBasePlayer *player, bool useLOS )
	{
		m_player = player;
		player->EyeVectors( &m_forward );

		m_pick = NULL;
		m_pickRange = 99999999999999.9f;
		m_useLOS = useLOS;
	}

	bool operator() ( INextBot *bot )
	{
		CBaseCombatCharacter *botEntity = bot->GetEntity();
		if ( botEntity->IsAlive() )
		{
			Vector to = botEntity->WorldSpaceCenter() - m_player->EyePosition();
			float range = to.NormalizeInPlace();

			if ( DotProduct( m_forward, to ) > 0.98f && range < m_pickRange )
			{
				if ( !m_useLOS || m_player->IsAbleToSee( botEntity, CBaseCombatCharacter::DISREGARD_FOV ) )
				{
					m_pick = bot;
					m_pickRange = range;
				}
			}
		}
		return true;
	}

	CBasePlayer *m_player;
	Vector m_forward;
	INextBot *m_pick;
	float m_pickRange;
	bool m_useLOS;
};

static void CC_SelectBot( const CCommand &args )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( player )
	{
		Selector select( player, false );
		TheNextBots().ForEachBot( select );

		TheNextBots().Select( select.m_pick );				

		if ( select.m_pick )
		{
			NDebugOverlay::Circle( select.m_pick->GetLocomotionInterface()->GetFeet() + Vector( 0, 0, 5 ), Vector( 1, 0, 0 ), Vector( 0, -1, 0 ), 25.0f, 0, 255, 0, 255, false, 1.0f );
		}
	}	
}
static ConCommand SelectBot( "nb_select", CC_SelectBot, "Select the bot you are aiming at for further debug operations.", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
static void CC_ForceLookAt( const CCommand &args )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	INextBot *pick = TheNextBots().GetSelected();

	if ( player && pick )
	{
		pick->GetBodyInterface()->AimHeadTowards( player, IBody::CRITICAL, 9999999.9f, NULL, "Aim forced" );
	}
}
static ConCommand ForceLookAt( "nb_force_look_at", CC_ForceLookAt, "Force selected bot to look at the local player's position", FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------
void CC_WarpSelectedHere( const CCommand &args )
{
	CBasePlayer *me = dynamic_cast< CBasePlayer * >( UTIL_GetCommandClient() ); 
	INextBot *pick = TheNextBots().GetSelected();

	if ( me == NULL || pick == NULL )
	{
		return;
	}

	Vector forward;
	me->EyeVectors( &forward );

	trace_t result;
	UTIL_TraceLine( me->EyePosition(), me->EyePosition() + 999999.9f * forward, MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, me, COLLISION_GROUP_NONE, &result );
	if ( result.DidHit() )
	{
		Vector spot = result.endpos + Vector( 0, 0, 10.0f );
		pick->GetEntity()->Teleport( &spot, &vec3_angle, &vec3_origin );
	}
}
static ConCommand WarpSelectedHere( "nb_warp_selected_here", CC_WarpSelectedHere, "Teleport the selected bot to your cursor position", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
NextBotManager::NextBotManager( void )
{
	m_debugType = 0;
	m_selectedBot = NULL;
	
	m_iUpdateTickrate = 0;
}

//---------------------------------------------------------------------------------------------
NextBotManager::~NextBotManager()
{
}


//---------------------------------------------------------------------------------------------
/**
 * Reset to initial state
 */
void NextBotManager::Reset( void )
{
	// remove the NextBots that should go away during a reset (they will unregister themselves as they go)
	int i = m_botList.Head();
	while ( i != m_botList.InvalidIndex() )
	{
		int iNext = m_botList.Next( i );
		if ( m_botList[i]->IsRemovedOnReset() )
		{
			UTIL_Remove( m_botList[i]->GetEntity() );
			//Assert( !m_botList.IsInList( i ) );	// UTIL_Remove() calls UpdateOnRemove, adds EFL_KILLME, but doesn't delete until the end of the frame
		}
		i = iNext;
	}

	m_selectedBot = NULL;
}


//---------------------------------------------------------------------------------------------

inline bool IsDead( INextBot *pBot )
{
	CBaseCombatCharacter *pEntity = pBot->GetEntity();
	if ( pEntity )
	{
		if ( pEntity->IsPlayer() && pEntity->m_lifeState == LIFE_DEAD )
		{
			return true;
		}
			
		if ( pEntity->IsMarkedForDeletion() )
		{
			return true;
		}

		if ( pEntity->m_pfnThink == &CBaseEntity::SUB_Remove )
		{
			return true;
		}
	}
	return false;
}

//---------------------------------------------------------------------------------------------

// Debug stats for update balancing
static int g_nRun;
static int g_nSlid;
static int g_nBlockedSlides;

void NextBotManager::Update( void )
{
	// do lightweight upkeep every tick
	for( int u=m_botList.Head(); u != m_botList.InvalidIndex(); u = m_botList.Next( u ) )
	{
		m_botList[ u ]->Upkeep();
	}

	// schedule full updates
	if ( m_botList.Count() )
	{
		static int iCurFrame = -1;
		if ( iCurFrame != gpGlobals->framecount )
		{
			iCurFrame = gpGlobals->framecount;
			m_SumFrameTime = 0;
		}
		else
		{
			// Don't run multiple ticks in a frame
			return;
		}

		int tickRate = TIME_TO_TICKS( nb_update_frequency.GetFloat() );
		if ( tickRate < 0 )
		{
			tickRate = 0;
		}

		if ( m_iUpdateTickrate != tickRate )
		{
			Msg( "NextBot tickrate changed from %d (%.3fms) to %d (%.3fms)\n", m_iUpdateTickrate, TICKS_TO_TIME( m_iUpdateTickrate ), tickRate, TICKS_TO_TIME( tickRate ) );
			m_iUpdateTickrate = tickRate;
		}

		int i = 0;
		int nScheduled = 0;
		int nNonResponsive = 0;
		int nDead = 0;
		if ( m_iUpdateTickrate > 0 )
		{
			INextBot *pBot;

			// Count dead bots, they won't update and balancing calculations should exclude them
			for( i = m_botList.Head(); i != m_botList.InvalidIndex(); i = m_botList.Next( i ) )
			{
				if ( IsDead( m_botList[i] ) )
				{
					nDead++;
				}
			}


			int nTargetToRun = ceilf( (float)( m_botList.Count() - nDead ) / (float)m_iUpdateTickrate );
			int curtickcount = gpGlobals->tickcount;

			for( i = m_botList.Head(); nTargetToRun && i != m_botList.InvalidIndex(); i = m_botList.Next( i ) )
			{
				pBot = m_botList[i];
				if ( pBot->IsFlaggedForUpdate() )
				{
					// Was offered a run last tick but didn't take it, push it back
					// Leave the flag set so that bot will run right away later, but be ignored
					// until then
					nNonResponsive++;
				}
				else
				{
					if ( curtickcount - pBot->GetTickLastUpdate() < m_iUpdateTickrate )
					{
						break;
					}
					if ( !IsDead( pBot ) )
					{
						pBot->FlagForUpdate();
						nTargetToRun--;
						nScheduled++;
					}
				}
			}
		}
		else
		{
			nScheduled = m_botList.Count();
		}

		if ( nb_update_debug.GetBool() )
		{
			int nIntentionalSliders = 0;
			if ( m_iUpdateTickrate > 0 )
			{
				for( ; i != m_botList.InvalidIndex(); i = m_botList.Next( i ) )
				{
					if ( gpGlobals->tickcount - m_botList[i]->GetTickLastUpdate() >= m_iUpdateTickrate )
					{
						nIntentionalSliders++;
					}
				}
			}

			Msg( "Frame %8d/tick %8d: %3d run of %3d, %3d sliders, %3d blocked slides, scheduled %3d for next tick, %3d intentional sliders, %d nonresponsive, %d dead\n", gpGlobals->framecount - 1, gpGlobals->tickcount - 1, g_nRun, m_botList.Count() - nDead, g_nSlid, g_nBlockedSlides, nScheduled, nIntentionalSliders, nNonResponsive, nDead );
			g_nRun = g_nSlid = g_nBlockedSlides = 0;
		}

	}
}

//---------------------------------------------------------------------------------------------
bool NextBotManager::ShouldUpdate( INextBot *bot )
{
	if ( m_iUpdateTickrate < 1 )
	{
		return true;
	}

	float frameLimit = nb_update_framelimit.GetFloat();
	float sumFrameTime = 0;
	if ( bot->IsFlaggedForUpdate() )
	{
		bot->FlagForUpdate( false );
		sumFrameTime = m_SumFrameTime * 1000.0;
		if ( frameLimit > 0.0f )
		{
			if ( sumFrameTime < frameLimit )
			{
				return true;
			}
			else if ( nb_update_debug.GetBool() )
			{
				Msg( "Frame %8d/tick %8d: frame out of budget (%.2fms > %.2fms)\n", gpGlobals->framecount, gpGlobals->tickcount, sumFrameTime, frameLimit );
			}
		}
	}

	int nTicksSlid = ( gpGlobals->tickcount - bot->GetTickLastUpdate() ) - m_iUpdateTickrate;

	if ( nTicksSlid >= nb_update_maxslide.GetInt() )
	{
		if ( frameLimit == 0.0 || sumFrameTime < nb_update_framelimit.GetFloat() * 2.0 )
		{
			g_nBlockedSlides++;
			return true;
		}
	}

	if ( nb_update_debug.GetBool() )
	{
		if ( nTicksSlid > 0 )
		{
			g_nSlid++;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------------------
void NextBotManager::NotifyBeginUpdate( INextBot *bot )
{
	if ( nb_update_debug.GetBool() )
	{
		g_nRun++;
	}

	m_botList.Unlink( bot->GetBotId() );
	m_botList.LinkToTail( bot->GetBotId() );
	bot->SetTickLastUpdate( gpGlobals->tickcount );

	m_CurUpdateStartTime = Plat_FloatTime();
}

//---------------------------------------------------------------------------------------------
void NextBotManager::NotifyEndUpdate( INextBot *bot )
{
	// This might be a good place to detect a particular bot had spiked [3/14/2008 tom]
	m_SumFrameTime += Plat_FloatTime() - m_CurUpdateStartTime;
}

//---------------------------------------------------------------------------------------------
/**
 * When the server has changed maps
 */
void NextBotManager::OnMapLoaded( void )
{
	Reset();
}


//---------------------------------------------------------------------------------------------
/**
 * When the scenario restarts
 */
void NextBotManager::OnRoundRestart( void )
{
	Reset();
}


//---------------------------------------------------------------------------------------------
int NextBotManager::Register( INextBot *bot )
{
	return m_botList.AddToHead( bot );
}


//---------------------------------------------------------------------------------------------
void NextBotManager::UnRegister( INextBot *bot )
{
	m_botList.Remove( bot->GetBotId() );

	if ( bot == m_selectedBot)
	{
		// we can't access virtual methods because this is called from a destructor, so just clear it
		m_selectedBot = NULL;
	}
}


//--------------------------------------------------------------------------------------------------------
void NextBotManager::OnBeginChangeLevel( void )
{
}


//----------------------------------------------------------------------------------------------------------
class NextBotKilledNotifyScan
{
public:
	NextBotKilledNotifyScan( CBaseCombatCharacter *victim, const CTakeDamageInfo &info )
	{
		m_victim = victim;
		m_info = info;
	}

	bool operator() ( INextBot *bot )
	{
		if ( bot->GetEntity()->IsAlive() && !bot->IsSelf( m_victim ) )
		{
			bot->OnOtherKilled( m_victim, m_info );
		}
		return true;
	}

	CBaseCombatCharacter *m_victim;
	CTakeDamageInfo m_info;
};


//---------------------------------------------------------------------------------------------
/**
 * When an actor is killed.  Propagate to all NextBots.
 */
void NextBotManager::OnKilled( CBaseCombatCharacter *victim, const CTakeDamageInfo &info )
{
	NextBotKilledNotifyScan notify( victim, info );
	TheNextBots().ForEachBot( notify );
}


//----------------------------------------------------------------------------------------------------------
class NextBotSoundNotifyScan
{
public:
	NextBotSoundNotifyScan( CBaseEntity *source, const Vector &pos, KeyValues *keys ) : m_source( source ), m_pos( pos ), m_keys( keys )
	{
	}

	bool operator() ( INextBot *bot )
	{
		if ( bot->GetEntity()->IsAlive() && !bot->IsSelf( m_source ) )
		{
			bot->OnSound( m_source, m_pos, m_keys );
		}
		return true;
	}

	CBaseEntity *m_source;
	const Vector &m_pos;
	KeyValues *m_keys;
};


//---------------------------------------------------------------------------------------------
/**
 * When an entity emits a sound
 */
void NextBotManager::OnSound( CBaseEntity *source, const Vector &pos, KeyValues *keys )
{
	NextBotSoundNotifyScan notify( source, pos, keys );
	TheNextBots().ForEachBot( notify );

	if ( source && IsDebugging( NEXTBOT_HEARING ) )
	{
		int r,g,b;
		switch( source->GetTeamNumber() )
		{
			case FIRST_GAME_TEAM:		r = 0;   g = 255; b = 0; break;
			case (FIRST_GAME_TEAM+1):	r = 255; g = 0;   b = 0; break;
			default:					r = 255; g = 255; b = 0; break;
		}
		NDebugOverlay::Circle( pos, Vector( 1, 0, 0 ), Vector( 0, -1, 0 ), 5.0f, r, g, b, 255, true, 3.0f );
	}
}


//----------------------------------------------------------------------------------------------------------
class NextBotResponseNotifyScan
{
public:
	NextBotResponseNotifyScan( CBaseCombatCharacter *who, AIConcept_t concept, AI_Response *response ) : m_who( who ), m_concept( concept ), m_response( response )
	{
	}

	bool operator() ( INextBot *bot )
	{
		if ( bot->GetEntity()->IsAlive() )
		{
			bot->OnSpokeConcept( m_who, m_concept, m_response );
		}
		return true;
	}

	CBaseCombatCharacter *m_who;
	AIConcept_t m_concept;
	AI_Response *m_response;
};


//---------------------------------------------------------------------------------------------
/**
 * When an Actor speaks a concept
 */
void NextBotManager::OnSpokeConcept( CBaseCombatCharacter *who, AIConcept_t concept, AI_Response *response )
{
	NextBotResponseNotifyScan notify( who, concept, response );
	TheNextBots().ForEachBot( notify );

	if ( IsDebugging( NEXTBOT_HEARING ) )
	{
		// const char *who = response->GetCriteria()->GetValue( response->GetCriteria()->FindCriterionIndex( "Who" ) );

		// TODO: Need concept.GetStringConcept()
		DevMsg( "%3.2f: OnSpokeConcept( %s, %s )\n", gpGlobals->curtime, who->GetDebugName(), "concept.GetStringConcept()" );
	}
}


//----------------------------------------------------------------------------------------------------------
class NextBotWeaponFiredNotifyScan
{
public:
	NextBotWeaponFiredNotifyScan( CBaseCombatCharacter *who, CBaseCombatWeapon *weapon ) : m_who( who ), m_weapon( weapon )
	{
	}

	bool operator() ( INextBot *bot )
	{
		if ( bot->GetEntity()->IsAlive() )
		{
			bot->OnWeaponFired( m_who, m_weapon );
		}
		return true;
	}

	CBaseCombatCharacter *m_who;
	CBaseCombatWeapon *m_weapon;
};


//---------------------------------------------------------------------------------------------
/**
 * When someone fires a weapon
 */
void NextBotManager::OnWeaponFired( CBaseCombatCharacter *whoFired, CBaseCombatWeapon *weapon )
{
	NextBotWeaponFiredNotifyScan notify( whoFired, weapon );
	TheNextBots().ForEachBot( notify );

	if ( IsDebugging( NEXTBOT_EVENTS ) )
	{
		DevMsg( "%3.2f: OnWeaponFired( %s, %s )\n", gpGlobals->curtime, whoFired->GetDebugName(), weapon->GetName() );
	}
}


//---------------------------------------------------------------------------------------------
/**
 * Add given entindex to the debug filter
 */
void NextBotManager::DebugFilterAdd( int index )
{
	DebugFilter filter;

	filter.index = index;
	filter.name[0] = '\000';

	m_debugFilterList.AddToTail( filter );
}


//---------------------------------------------------------------------------------------------
/**
 * Add given name to the debug filter
 */
void NextBotManager::DebugFilterAdd( const char *name )
{
	DebugFilter filter;

	filter.index = -1;
	Q_strncpy( filter.name, name, DebugFilter::MAX_DEBUG_NAME_SIZE );

	m_debugFilterList.AddToTail( filter );
}


//---------------------------------------------------------------------------------------------
/**
 * Remove given entindex from the debug filter
 */
void NextBotManager::DebugFilterRemove( int index )
{
	for( int i=0; i<m_debugFilterList.Count(); ++i )
	{
		if ( m_debugFilterList[i].index == index )
		{
			m_debugFilterList.Remove( i );
			break;
		}
	}
}


//---------------------------------------------------------------------------------------------
/**
 * Remove given name from the debug filter
 */
void NextBotManager::DebugFilterRemove( const char *name )
{
	for( int i=0; i<m_debugFilterList.Count(); ++i )
	{
		if ( m_debugFilterList[i].name[0] != '\000' &&
			 !Q_strnicmp( name, m_debugFilterList[i].name, MIN( Q_strlen( name ), sizeof( m_debugFilterList[i].name ) ) ) )
		{
			m_debugFilterList.Remove( i );
			break;
		}
	}
}


//---------------------------------------------------------------------------------------------
/**
 * Clear the debug filter (remove all entries)
 */
void NextBotManager::DebugFilterClear( void )
{
	m_debugFilterList.RemoveAll();
}


//---------------------------------------------------------------------------------------------
/**
 * Return true if the given bot matches the debug filter
 */
bool NextBotManager::IsDebugFilterMatch( const INextBot *bot ) const
{
	// if the filter is empty, all bots match
	if ( m_debugFilterList.Count() == 0 )
	{
		return true;
	}

	for( int i=0; i<m_debugFilterList.Count(); ++i )
	{
		// compare entity index
		if ( m_debugFilterList[i].index == const_cast< INextBot * >( bot )->GetEntity()->entindex() )
		{
			return true;
		}

		// compare debug filter
		if ( m_debugFilterList[i].name[0] != '\000' && bot->IsDebugFilterMatch( m_debugFilterList[i].name ) )
		{
			return true;
		}

		// compare special keyword meaning local player is looking at them
		if ( !Q_strnicmp( m_debugFilterList[i].name, "lookat", Q_strlen( m_debugFilterList[i].name ) ) )
		{
			CBasePlayer *watcher = UTIL_GetListenServerHost();
			if ( watcher )
			{
				CBaseEntity *subject = watcher->GetObserverTarget();

				if ( subject && bot->IsSelf( subject ) )
				{
					return true;
				}
			}
		}

		// compare special keyword meaning NextBot is selected
		if ( !Q_strnicmp( m_debugFilterList[i].name, "selected", Q_strlen( m_debugFilterList[i].name ) ) )
		{
			INextBot *selected = GetSelected();
			if ( selected && bot->IsSelf( selected->GetEntity() ) )
			{
				return true;
			}
		}
	}

	return false;
}

//---------------------------------------------------------------------------------------------
/**
 * Get the bot under the given player's crosshair
 */
INextBot *NextBotManager::GetBotUnderCrosshair( CBasePlayer *picker )
{
	if ( !picker )
		return NULL;

	const float MaxDot = 0.7f;
	const float MaxRange = 4000.0f;
	TargetScan< CBaseCombatCharacter > scan( picker, TEAM_ANY, 1.0f - MaxDot, MaxRange );
	ForEachCombatCharacter( scan );
	CBaseCombatCharacter *target = scan.GetTarget();
	if ( target && target->MyNextBotPointer() )
		return target->MyNextBotPointer();

	return NULL;
}

#ifdef NEED_BLACK_BOX
//---------------------------------------------------------------------------------------------
CON_COMMAND( nb_dump_debug_history, "Dumps debug history for the bot under the cursor to the blackbox" )
{
	if ( !NextBotDebugHistory.GetBool() )
	{
		BlackBox_Record( "bot", "nb_debug_history 0" );
		return;
	}

	CBasePlayer *player = UTIL_GetCommandClient();
	if ( !player )
	{
		player = UTIL_GetListenServerHost();
	}
	INextBot *bot = TheNextBots().GetBotUnderCrosshair( player );
	if ( !bot )
	{
		BlackBox_Record( "bot", "no bot under crosshairs" );
		return;
	}

	CUtlVector< const INextBot::NextBotDebugLineType * > lines;
	bot->GetDebugHistory( (NEXTBOT_DEBUG_ALL & (~NEXTBOT_EVENTS)), &lines );

	for ( int i=0; i<lines.Count(); ++i )
	{
		if ( IsPC() )
		{
			BlackBox_Record( "bot", "%s", lines[i]->data );
		}
	}
}
#endif // NEED_BLACK_BOX


//---------------------------------------------------------------------------------------------
void NextBotManager::CollectAllBots( CUtlVector< INextBot * > *botVector )
{
	if ( !botVector )
		return;

	botVector->RemoveAll();

	for( int i=m_botList.Head(); i != m_botList.InvalidIndex(); i = m_botList.Next( i ) )
	{
		botVector->AddToTail( m_botList[i] );
	}
}

