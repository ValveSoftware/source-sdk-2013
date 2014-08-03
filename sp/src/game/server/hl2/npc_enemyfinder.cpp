//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Bullseyes act as targets for other NPC's to attack and to trigger
//			events 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "decals.h"
#include "IEffects.h"
#include "ai_squad.h"
#include "ai_utils.h"
#include "ai_senses.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_ENEMY_FINDER_CHECK_VIS (1 << 16)
#define SF_ENEMY_FINDER_APC_VIS (1 << 17)
#define SF_ENEMY_FINDER_SHORT_MEMORY (1 << 18)
#define SF_ENEMY_FINDER_ENEMY_ALLOWED (1 << 19)

ConVar  ai_debug_enemyfinders( "ai_debug_enemyfinders", "0" );


class CNPC_EnemyFinder : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CNPC_EnemyFinder, CAI_BaseNPC );

	CNPC_EnemyFinder()
	{
		m_PlayerFreePass.SetOuter( this );
	}


	void	Precache( void );
	void	Spawn( void );
	void	StartNPC ( void );
	void	PrescheduleThink();
	bool 	ShouldAlwaysThink();
	void	UpdateEfficiency( bool bInPVS )	{ SetEfficiency( ( GetSleepState() != AISS_AWAKE ) ? AIE_DORMANT : AIE_NORMAL ); SetMoveEfficiency( AIME_NORMAL ); }
	void	GatherConditions( void );
	bool	ShouldChooseNewEnemy();
	bool	IsValidEnemy( CBaseEntity *pTarget );
	bool	CanBeAnEnemyOf( CBaseEntity *pEnemy ) { return HasSpawnFlags( SF_ENEMY_FINDER_ENEMY_ALLOWED ); }
	bool	FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker );
	Class_T Classify( void );
	bool CanBeSeenBy( CAI_BaseNPC *pNPC ) { return CanBeAnEnemyOf( pNPC ); } // allows entities to be 'invisible' to NPC senses.

	virtual int	SelectSchedule( void );
	virtual void DrawDebugGeometryOverlays( void );

	// Input handlers.
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );

	virtual	void Wake( bool bFireOutput = true );

private:
	int		m_nStartOn;
	float	m_flMinSearchDist;
	float	m_flMaxSearchDist;
	CAI_FreePass m_PlayerFreePass;
	CSimpleSimTimer m_ChooseEnemyTimer;

	bool	m_bEnemyStatus;

	COutputEvent m_OnLostEnemies;
	COutputEvent m_OnAcquireEnemies;

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};


LINK_ENTITY_TO_CLASS( npc_enemyfinder, CNPC_EnemyFinder );


//-----------------------------------------------------------------------------
// Custom schedules.
//-----------------------------------------------------------------------------
enum
{
	SCHED_EFINDER_SEARCH = LAST_SHARED_SCHEDULE,
};

IMPLEMENT_CUSTOM_AI( npc_enemyfinder, CNPC_EnemyFinder );

BEGIN_DATADESC( CNPC_EnemyFinder )

	DEFINE_EMBEDDED( m_PlayerFreePass ),
	DEFINE_EMBEDDED( m_ChooseEnemyTimer ),

	// Inputs
	DEFINE_INPUT( m_nStartOn,			FIELD_INTEGER,	"StartOn" ),
	DEFINE_INPUT( m_flFieldOfView,	FIELD_FLOAT,	"FieldOfView" ),
	DEFINE_INPUT( m_flMinSearchDist,	FIELD_FLOAT,	"MinSearchDist" ),
	DEFINE_INPUT( m_flMaxSearchDist,	FIELD_FLOAT,	"MaxSearchDist" ),

	DEFINE_FIELD( m_bEnemyStatus, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),

	DEFINE_OUTPUT( m_OnLostEnemies, "OnLostEnemies"),
	DEFINE_OUTPUT( m_OnAcquireEnemies, "OnAcquireEnemies"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::InitCustomSchedules( void )
{
	INIT_CUSTOM_AI( CNPC_EnemyFinder );

	ADD_CUSTOM_SCHEDULE( CNPC_EnemyFinder, SCHED_EFINDER_SEARCH );
	AI_LOAD_SCHEDULE( CNPC_EnemyFinder, SCHED_EFINDER_SEARCH );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning the enemy finder on.
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::InputTurnOn( inputdata_t &inputdata )
{
	SetThink( &CNPC_EnemyFinder::CallNPCThink );
	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning the enemy finder off.
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::InputTurnOff( inputdata_t &inputdata )
{
	SetThink(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::Precache( void )
{
	PrecacheModel( "models/player.mdl" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::Spawn( void )
{
	Precache();

	SetModel( "models/player.mdl" );
	// This is a dummy model that is never used!
	UTIL_SetSize(this, vec3_origin, vec3_origin);

	SetMoveType( MOVETYPE_NONE );
	SetBloodColor( DONT_BLEED );
	SetGravity( 0.0 );
	m_iHealth			= 1;
	
	AddFlag( FL_NPC );

	SetSolid( SOLID_NONE );

	m_bEnemyStatus = false;

	if (m_flFieldOfView < -1.0)
	{
		DevMsg("ERROR: EnemyFinder field of view must be between -1.0 and 1.0\n");
		m_flFieldOfView		= 0.5;
	}
	else if (m_flFieldOfView > 1.0)
	{
		DevMsg("ERROR: EnemyFinder field of view must be between -1.0 and 1.0\n");
		m_flFieldOfView		= 1.0;
	}
	CapabilitiesAdd	( bits_CAP_SQUAD );

	NPCInit();

	// Set this after NPCInit()
	m_takedamage	= DAMAGE_NO;
	AddEffects( EF_NODRAW );
	m_NPCState		= NPC_STATE_ALERT;	// always alert

	SetViewOffset( vec3_origin );
	if ( m_flMaxSearchDist )
	{
		SetDistLook( m_flMaxSearchDist );
	}

	if ( HasSpawnFlags( SF_ENEMY_FINDER_SHORT_MEMORY ) )
	{
		GetEnemies()->SetEnemyDiscardTime( 0.2 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
int CNPC_EnemyFinder::SelectSchedule( void )
{
	return SCHED_EFINDER_SEARCH;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CNPC_EnemyFinder::Wake( bool bFireOutput )
{
	BaseClass::Wake( bFireOutput );

	//Enemy finder is not allowed to become visible.
	AddEffects( EF_NODRAW );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool CNPC_EnemyFinder::FVisible( CBaseEntity *pTarget, int traceMask, CBaseEntity **ppBlocker )
{
	float flTargetDist = GetAbsOrigin().DistTo( pTarget->GetAbsOrigin() );
	if ( flTargetDist < m_flMinSearchDist)
		return false;

	if ( m_flMaxSearchDist && flTargetDist > m_flMaxSearchDist)
		return false;

	if ( !FBitSet( m_spawnflags, SF_ENEMY_FINDER_CHECK_VIS) )
		return true;

	if ( !HasSpawnFlags(SF_ENEMY_FINDER_APC_VIS) )
	{
		bool bIsVisible = BaseClass::FVisible( pTarget, traceMask, ppBlocker );
		
		if ( bIsVisible && pTarget == m_PlayerFreePass.GetPassTarget() )
			bIsVisible = m_PlayerFreePass.ShouldAllowFVisible( bIsVisible );

		return bIsVisible;
	}

	// Make sure I can see the target from my position
	trace_t tr;

	// Trace from launch position to target position.  
	// Use position above actual barral based on vertical launch speed
	Vector vStartPos = GetAbsOrigin();
	Vector vEndPos	 = pTarget->EyePosition();

	CBaseEntity *pVehicle = NULL;
	if ( pTarget->IsPlayer() )
	{
		CBasePlayer *pPlayer = assert_cast<CBasePlayer*>(pTarget);
		pVehicle = pPlayer->GetVehicleEntity();
	}

	CTraceFilterSkipTwoEntities traceFilter( pTarget, pVehicle, COLLISION_GROUP_NONE );
	AI_TraceLine( vStartPos, vEndPos, MASK_SHOT, &traceFilter, &tr );
	if ( ppBlocker )
	{
		*ppBlocker = tr.m_pEnt;
	}
	return (tr.fraction == 1.0);
}


//------------------------------------------------------------------------------
bool CNPC_EnemyFinder::ShouldChooseNewEnemy()
{
	if ( m_ChooseEnemyTimer.Expired() )
	{
		m_ChooseEnemyTimer.Set( 0.3 );
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
// Purpose : Override base class to check range and visibility
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_EnemyFinder::IsValidEnemy( CBaseEntity *pTarget )
{
	float flTargetDist = GetAbsOrigin().DistTo( pTarget->GetAbsOrigin() );
	if (flTargetDist < m_flMinSearchDist)
		return false;

	if ( m_flMaxSearchDist && flTargetDist > m_flMaxSearchDist)
		return false;

	if ( !FBitSet( m_spawnflags, SF_ENEMY_FINDER_CHECK_VIS) )
		return true;

	if ( GetSenses()->DidSeeEntity( pTarget ) )
		return true;

	// Trace from launch position to target position.  
	// Use position above actual barral based on vertical launch speed
	Vector vStartPos = GetAbsOrigin();
	Vector vEndPos	 = pTarget->EyePosition();

	// Test our line of sight to the target
	trace_t tr;
	AI_TraceLOS( vStartPos, vEndPos, this, &tr );

	// If the player is in a vehicle, see if we can see that instead
	if ( pTarget->IsPlayer() )
	{
		CBasePlayer *pPlayer = assert_cast<CBasePlayer*>(pTarget);
		if ( tr.m_pEnt == pPlayer->GetVehicleEntity() )
			return true;
	}

	// Line must be clear
	if ( tr.fraction == 1.0f || tr.m_pEnt == pTarget )
		return true;

	// Otherwise we can't see anything
	return false;
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_EnemyFinder::StartNPC ( void )
{
	AddSpawnFlags(SF_NPC_FALL_TO_GROUND);	// this prevents CAI_BaseNPC from slamming the finder to 
											// the ground just because it's not MOVETYPE_FLY
	BaseClass::StartNPC();

	if ( AI_IsSinglePlayer() && m_PlayerFreePass.GetParams().duration > 0.1 )
	{
		m_PlayerFreePass.SetPassTarget( UTIL_PlayerByIndex(1) );

		AI_FreePassParams_t freePassParams = m_PlayerFreePass.GetParams();

		freePassParams.coverDist = 120;
		freePassParams.peekEyeDist = 1.75;
		freePassParams.peekEyeDistZ = 4;

		m_PlayerFreePass.SetParams( freePassParams );
	}

	if (!m_nStartOn)
	{
		SetThink(NULL);
	}
}

//------------------------------------------------------------------------------
void CNPC_EnemyFinder::PrescheduleThink()
{
	BaseClass::PrescheduleThink();

	bool bHasEnemies = GetEnemies()->NumEnemies() > 0;
	
	if ( GetEnemies()->NumEnemies() > 0 )
	{
		//If I haven't seen my enemy in half a second then we'll assume he's gone.
		if ( gpGlobals->curtime - GetEnemyLastTimeSeen() >= 0.5f )
		{
			bHasEnemies = false;
		}
	}

	if ( m_bEnemyStatus != bHasEnemies )
	{
		if ( bHasEnemies )
		{
			m_OnAcquireEnemies.FireOutput( this, this );
		}
		else
		{
			m_OnLostEnemies.FireOutput( this, this );
		}
		
		m_bEnemyStatus = bHasEnemies;
	}

	if( ai_debug_enemyfinders.GetBool() )
	{
		m_debugOverlays |= OVERLAY_BBOX_BIT;

		if( IsInSquad() && GetSquad()->NumMembers() > 1 )
		{
			AISquadIter_t iter;
			CAI_BaseNPC *pSquadmate = m_pSquad ? m_pSquad->GetFirstMember( &iter ) : NULL;
			while ( pSquadmate )
			{
				NDebugOverlay::Line( WorldSpaceCenter(), pSquadmate->EyePosition(), 255, 255, 0, false, 0.1f );
				pSquadmate = m_pSquad->GetNextMember( &iter );
			}
		}
	}
}

//------------------------------------------------------------------------------
bool CNPC_EnemyFinder::ShouldAlwaysThink()
{
	if ( BaseClass::ShouldAlwaysThink() )
		return true;
		
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer && IRelationType( pPlayer ) == D_HT )
	{
		float playerDistSqr = GetAbsOrigin().DistToSqr( pPlayer->GetAbsOrigin() );

		if ( !m_flMaxSearchDist || playerDistSqr <= Square(m_flMaxSearchDist) )
		{
			if ( !FBitSet( m_spawnflags, SF_ENEMY_FINDER_CHECK_VIS) )
				return true;
				
			if ( playerDistSqr <= Square( 50 * 12 ) )
				return true;
		}
	}
	
	return false;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_EnemyFinder::GatherConditions()
{
	// This works with old data because need to do before base class so as to not choose as enemy
	m_PlayerFreePass.Update();
	BaseClass::GatherConditions();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_EnemyFinder::Classify( void )
{
	if ( GetSquad() )
	{
		AISquadIter_t iter;
		CAI_BaseNPC *pSquadmate = GetSquad()->GetFirstMember( &iter );
		while ( pSquadmate )
		{
			if ( pSquadmate != this && !pSquadmate->ClassMatches( GetClassname() ) )
			{
				return pSquadmate->Classify();
			}
			pSquadmate = GetSquad()->GetNextMember( &iter );
		}
	}

	return CLASS_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Add a visualizer to the text, if turned on
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::DrawDebugGeometryOverlays( void )
{
	// Turn on npc_relationships if we're displaying text
	int oldDebugOverlays = m_debugOverlays;
	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		m_debugOverlays |= OVERLAY_NPC_RELATION_BIT;
	}

	// Draw our base overlays
	BaseClass::DrawDebugGeometryOverlays();

	// Restore the old values
	m_debugOverlays = oldDebugOverlays;
}

ConVar  ai_ef_hate_npc_frequency( "ai_ef_hate_npc_frequency", "5" );
ConVar  ai_ef_hate_npc_duration( "ai_ef_hate_npc_duration", "1.5" );

//-----------------------------------------------------------------------------
// Derived class with a few changes that make the Combine Cannon behave the 
// way we want.
//-----------------------------------------------------------------------------
#define EF_COMBINE_CANNON_HATE_TIME_INVALID -1
static CUtlVector<CBaseEntity*> s_ListEnemyfinders;

class CNPC_EnemyFinderCombineCannon : public CNPC_EnemyFinder
{
public:
	DECLARE_CLASS( CNPC_EnemyFinderCombineCannon, CNPC_EnemyFinder );
	DECLARE_DATADESC();

	CNPC_EnemyFinderCombineCannon()
	{
		m_flTimeNextHateNPC = gpGlobals->curtime;
		m_flTimeStopHateNPC = EF_COMBINE_CANNON_HATE_TIME_INVALID;
	};

public:
	void	Spawn();
	void	Activate();
	void	UpdateOnRemove();
	bool	FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker );
	bool	IsValidEnemy( CBaseEntity *pTarget );
	void	GatherConditions();

	void	InputSetWideFOVForSeconds( inputdata_t &inputdata );

public:
	float		m_flTimeNextHateNPC;
	float		m_flTimeStopHateNPC;
	float		m_flOriginalFOV;
	float		m_flTimeWideFOV; // If this is > gpGlobals->curtime, we have 180 degree viewcone.
	string_t	m_iszSnapToEnt;
};
LINK_ENTITY_TO_CLASS( npc_enemyfinder_combinecannon, CNPC_EnemyFinderCombineCannon );

BEGIN_DATADESC( CNPC_EnemyFinderCombineCannon )
DEFINE_FIELD( m_flTimeNextHateNPC, FIELD_TIME ),
DEFINE_FIELD( m_flTimeStopHateNPC, FIELD_TIME ),
DEFINE_FIELD( m_flOriginalFOV, FIELD_FLOAT ),
DEFINE_FIELD( m_flTimeWideFOV, FIELD_TIME ),
DEFINE_KEYFIELD( m_iszSnapToEnt, FIELD_STRING, "snaptoent" ),
DEFINE_INPUTFUNC( FIELD_FLOAT, "SetWideFOVForSeconds", InputSetWideFOVForSeconds ),
END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_EnemyFinderCombineCannon::Spawn()
{
	BaseClass::Spawn();
	m_flOriginalFOV = m_flFieldOfView;
	m_flTimeWideFOV = -1.0f;

	if( m_iszSnapToEnt != NULL_STRING )
	{
		CBaseEntity *pSnapToEnt = gEntList.FindEntityByName( NULL, m_iszSnapToEnt );

		if( pSnapToEnt != NULL )
		{
			//!!!HACKHACK - this eight-inch offset puts this enemyfinder perfectly on-bore 
			// with the prefab for a func_tank_combinecannon
			UTIL_SetOrigin( this, pSnapToEnt->WorldSpaceCenter() + Vector( 0, 0, 8) );
		}
		else
		{
			DevMsg( this, "Enemyfinder %s can't snap to %s because it doesn't exist\n", GetDebugName(), STRING(m_iszSnapToEnt) );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_EnemyFinderCombineCannon::Activate() 
{
	BaseClass::Activate();

	// See if I'm in the list of Combine enemyfinders
	// If not, add me.
	if( s_ListEnemyfinders.Find(this) == -1 )
	{
		s_ListEnemyfinders.AddToTail(this);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_EnemyFinderCombineCannon::UpdateOnRemove() 
{
	BaseClass::UpdateOnRemove();

	// See if I'm in the list of Combine enemyfinders
	int index = s_ListEnemyfinders.Find(this);
	if( index != -1 )
	{
		s_ListEnemyfinders.Remove(index);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_EnemyFinderCombineCannon::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
#if 1
	CBaseEntity *pBlocker = NULL;
	bool result;

	if(ppBlocker == NULL)
	{
		// Whoever called this didn't care about the blocker, but we do. 
		// So substitute our local pBlocker pointer and don't disturb ppBlocker
		result = BaseClass::FVisible( pEntity, traceMask, &pBlocker );
	}
	else
	{
		// Copy the ppBlocker to our local pBlocker pointer, but do not
		// disturb the ppBlocker that was passed to us.
		result = BaseClass::FVisible( pEntity, traceMask, ppBlocker );
		pBlocker = (*ppBlocker);
	}

	if(pEntity->IsPlayer() && result == false)
	{
		// IF we are trying to see the player, but we don't, examine the blocker 
		// and see the player anyway if we can hurt the blocker.
		if(pBlocker != NULL)
		{
			if( pBlocker->m_takedamage >= DAMAGE_YES ) // also DAMAGE_AIM
			{
				// Anytime the line of sight is blocked by something I can hurt, I have line of sight.
				// This will make the func_tank_combinecannon shoot the blocking object. This will 
				// continue until the gun bores through to the player or clears all interposing breakables 
				// and finds its progress impeded by something truly solid. So lie, and say we CAN see the player.
				result = true;
			}
		}
	}

	return result;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Ignore NPC's most of the time when the player is a potential target.
//			Go through short periods of time where NPCs may distract me
//
//			ALSO- ignore NPC's (focus only on the player) when I'm in
//			wide viewcone mode. 
//-----------------------------------------------------------------------------
bool CNPC_EnemyFinderCombineCannon::IsValidEnemy( CBaseEntity *pTarget )
{
	if( m_flTimeWideFOV > gpGlobals->curtime && !pTarget->IsPlayer() )
	{
		// ONLY allowed to hate the player when I'm in hyper-vigilant wide FOV mode.
		// This forces all guns in outland_09 to shoot at the player once any
		// gun has fired at the player. This allows the other guns to specifically
		// kill zombies most of the time, but immediately turn their attention to the
		// player when necessary (by ignoring everything else)
		return pTarget->IsPlayer();
	}

	bool bResult = BaseClass::IsValidEnemy( pTarget );

	if( bResult && !pTarget->IsPlayer() )
	{
		// This is a valid enemy, but we have to make sure no other enemyfinders for
		// combine cannons are currently attacking it.
		int i;
		for( i = 0 ; i < s_ListEnemyfinders.Count() ; i++ )
		{
			if( s_ListEnemyfinders[i] == this )
				continue;

			if( s_ListEnemyfinders[i]->GetEnemy() == pTarget )
				return false;// someone else is already busy with this target.
		}
	}

	/*
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	int iPlayerRelationPriority = -1;

	if( pPlayer != NULL )
	{
		iPlayerRelationPriority = IRelationPriority(pPlayer);
	}

	if( bResult == true && pTarget->IsNPC() && pPlayer != NULL && FInViewCone( pPlayer ) )
	{
		if( HasCondition(COND_SEE_PLAYER) )
		{
			// The player is visible! Immediately ignore all NPCs as enemies.
			return false;
		}

		// The base class wants to call this a valid enemy. We may choose to interfere
		// If the player is in my viewcone. That means that my func_tank could potentially 
		// harass the player. This means I should meter the time I spend shooting at npcs 
		// NPCs so that I can focus on the player.
		if( m_flTimeStopHateNPC != EF_COMBINE_CANNON_HATE_TIME_INVALID )
		{
			// We currently hate NPC's. But is it time to stop?
			if( gpGlobals->curtime > m_flTimeStopHateNPC )
			{
				// Don't interfere with the result
				m_flTimeStopHateNPC = EF_COMBINE_CANNON_HATE_TIME_INVALID;
				m_flTimeNextHateNPC = gpGlobals->curtime + ai_ef_hate_npc_frequency.GetFloat();
				return bResult;
			}
		}
		else
		{
			// We do not hate NPCs at the moment. Is it time to turn it on?
			if( gpGlobals->curtime > m_flTimeNextHateNPC )
			{
				m_flTimeStopHateNPC = gpGlobals->curtime + ai_ef_hate_npc_duration.GetFloat();
			}
			else
			{
				// Stop harassing player to attack something else higher priority.
				if( IRelationPriority(pTarget) > iPlayerRelationPriority )
					return bResult;
			}

			// Make this enemy invalid.
			return false;
		}
	}
	*/

	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: Control the width of my viewcone
//-----------------------------------------------------------------------------
void CNPC_EnemyFinderCombineCannon::GatherConditions()
{
	if( m_flTimeWideFOV > gpGlobals->curtime )
	{
		// I'm in a hyper-vigilant period of time where I get a 270 degree viewcone
		m_flFieldOfView = -0.5f;
	}
	else
	{
		m_flFieldOfView = m_flOriginalFOV;
	}

	BaseClass::GatherConditions();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_EnemyFinderCombineCannon::InputSetWideFOVForSeconds( inputdata_t &inputdata )
{
	m_flTimeWideFOV	= gpGlobals->curtime + inputdata.value.Float();
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//=========================================================
// > SCHED_EFINDER_SEARCH
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_EFINDER_SEARCH,

	"	Tasks"
	"		TASK_WAIT_RANDOM			0.5		"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
);
