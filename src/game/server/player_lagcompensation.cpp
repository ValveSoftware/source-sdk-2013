//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "usercmd.h"
#include "igamesystem.h"
#include "ilagcompensationmanager.h"
#include "inetchannelinfo.h"
#include "utllinkedlist.h"
#include "BaseAnimatingOverlay.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define LC_NONE				0
#define LC_ALIVE			(1<<0)

#define LC_ORIGIN_CHANGED	(1<<8)
#define LC_ANGLES_CHANGED	(1<<9)
#define LC_SIZE_CHANGED		(1<<10)
#define LC_ANIMATION_CHANGED (1<<11)

static ConVar sv_lagcompensation_teleport_dist( "sv_lagcompensation_teleport_dist", "64", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "How far a player got moved by game code before we can't lag compensate their position back" );
#define LAG_COMPENSATION_EPS_SQR ( 0.1f * 0.1f )
// Allow 4 units of error ( about 1 / 8 bbox width )
#define LAG_COMPENSATION_ERROR_EPS_SQR ( 4.0f * 4.0f )

ConVar sv_unlag( "sv_unlag", "1", FCVAR_DEVELOPMENTONLY, "Enables player lag compensation" );
ConVar sv_maxunlag( "sv_maxunlag", "1.0", FCVAR_DEVELOPMENTONLY, "Maximum lag compensation in seconds", true, 0.0f, true, 1.0f );
ConVar sv_lagflushbonecache( "sv_lagflushbonecache", "1", FCVAR_DEVELOPMENTONLY, "Flushes entity bone cache on lag compensation" );
ConVar sv_showlagcompensation( "sv_showlagcompensation", "0", FCVAR_CHEAT, "Show lag compensated hitboxes whenever a player is lag compensated." );

ConVar sv_unlag_fixstuck( "sv_unlag_fixstuck", "0", FCVAR_DEVELOPMENTONLY, "Disallow backtracking a player for lag compensation if it will cause them to become stuck" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define MAX_LAYER_RECORDS (CBaseAnimatingOverlay::MAX_OVERLAYS)

struct LayerRecord
{
	int m_sequence;
	float m_cycle;
	float m_weight;
	int m_order;

	LayerRecord()
	{
		m_sequence = 0;
		m_cycle = 0;
		m_weight = 0;
		m_order = 0;
	}

	LayerRecord( const LayerRecord& src )
	{
		m_sequence = src.m_sequence;
		m_cycle = src.m_cycle;
		m_weight = src.m_weight;
		m_order = src.m_order;
	}
};

struct LagRecord
{
public:
	LagRecord()
	{
		m_fFlags = 0;
		m_vecOrigin.Init();
		m_vecAngles.Init();
		m_vecMinsPreScaled.Init();
		m_vecMaxsPreScaled.Init();
		m_flSimulationTime = -1;
		m_masterSequence = 0;
		m_masterCycle = 0;

		for( int i=0; i<MAXSTUDIOPOSEPARAM; i++ )
		{
			m_flPoseParameters[i] = 0;
		}
	}

	LagRecord( const LagRecord& src )
	{
		m_fFlags = src.m_fFlags;
		m_vecOrigin = src.m_vecOrigin;
		m_vecAngles = src.m_vecAngles;
		m_vecMinsPreScaled = src.m_vecMinsPreScaled;
		m_vecMaxsPreScaled = src.m_vecMaxsPreScaled;
		m_flSimulationTime = src.m_flSimulationTime;
		for( int layerIndex = 0; layerIndex < MAX_LAYER_RECORDS; ++layerIndex )
		{
			m_layerRecords[layerIndex] = src.m_layerRecords[layerIndex];
		}
		m_masterSequence = src.m_masterSequence;
		m_masterCycle = src.m_masterCycle;

		for( int i=0; i<MAXSTUDIOPOSEPARAM; i++ )
		{
			m_flPoseParameters[i] = src.m_flPoseParameters[i];
		}
	}

	// Did player die this frame
	int						m_fFlags;

	// Player position, orientation and bbox
	Vector					m_vecOrigin;
	QAngle					m_vecAngles;
	Vector					m_vecMinsPreScaled;
	Vector					m_vecMaxsPreScaled;

	float					m_flSimulationTime;	
	
	// Player animation details, so we can get the legs in the right spot.
	LayerRecord				m_layerRecords[MAX_LAYER_RECORDS];
	int						m_masterSequence;
	float					m_masterCycle;

	float					m_flPoseParameters[MAXSTUDIOPOSEPARAM];
};


//
// Try to take the player from his current origin to vWantedPos.
// If it can't get there, leave the player where he is.
// 

ConVar sv_unlag_debug( "sv_unlag_debug", "0", FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY );

float g_flFractionScale = 0.95;
static void RestorePlayerTo( CBasePlayer *pPlayer, const Vector &vWantedPos )
{
	// Try to move to the wanted position from our current position.
	trace_t tr;
	VPROF_BUDGET( "RestorePlayerTo", "CLagCompensationManager" );
	UTIL_TraceEntity( pPlayer, vWantedPos, vWantedPos, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );
	if ( tr.startsolid || tr.allsolid )
	{
		if ( sv_unlag_debug.GetBool() )
		{
			DevMsg( "RestorePlayerTo() could not restore player position for client \"%s\" ( %.1f %.1f %.1f )\n",
					pPlayer->GetPlayerName(), vWantedPos.x, vWantedPos.y, vWantedPos.z );
		}

		UTIL_TraceEntity( pPlayer, pPlayer->GetLocalOrigin(), vWantedPos, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );
		if ( tr.startsolid || tr.allsolid )
		{
			// In this case, the guy got stuck back wherever we lag compensated him to. Nasty.

			if ( sv_unlag_debug.GetBool() )
				DevMsg( " restore failed entirely\n" );
		}
		else
		{
			// We can get to a valid place, but not all the way back to where we were.
			Vector vPos;
			VectorLerp( pPlayer->GetLocalOrigin(), vWantedPos, tr.fraction * g_flFractionScale, vPos );
			UTIL_SetOrigin( pPlayer, vPos, true );

			if ( sv_unlag_debug.GetBool() )
				DevMsg( " restore got most of the way\n" );
		}
	}
	else
	{
		// Cool, the player can go back to whence he came.
		UTIL_SetOrigin( pPlayer, tr.endpos, true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLagCompensationManager : public CAutoGameSystemPerFrame, public ILagCompensationManager
{
public:
	CLagCompensationManager( char const *name ) : CAutoGameSystemPerFrame( name ), m_flTeleportDistanceSqr( 64 *64 )
	{
		m_isCurrentlyDoingCompensation = false;
	}

	// IServerSystem stuff
	virtual void Shutdown()
	{
		ClearHistory();
	}

	virtual void LevelShutdownPostEntity()
	{
		ClearHistory();
	}

	// called after entities think
	virtual void FrameUpdatePostEntityThink();

	// ILagCompensationManager stuff

	// Called during player movement to set up/restore after lag compensation
	void			StartLagCompensation( CBasePlayer *player, CUserCmd *cmd );
	void			FinishLagCompensation( CBasePlayer *player );

	bool			IsCurrentlyDoingLagCompensation() const OVERRIDE { return m_isCurrentlyDoingCompensation; }

private:
	void			BacktrackPlayer( CBasePlayer *player, float flTargetTime );

	void ClearHistory()
	{
		for ( int i=0; i<MAX_PLAYERS; i++ )
			m_PlayerTrack[i].Purge();
	}

	// keep a list of lag records for each player
	CUtlFixedLinkedList< LagRecord >	m_PlayerTrack[ MAX_PLAYERS ];

	// Scratchpad for determining what needs to be restored
	CBitVec<MAX_PLAYERS>	m_RestorePlayer;
	bool					m_bNeedToRestore;
	
	LagRecord				m_RestoreData[ MAX_PLAYERS ];	// player data before we moved him back
	LagRecord				m_ChangeData[ MAX_PLAYERS ];	// player data where we moved him back

	CBasePlayer				*m_pCurrentPlayer;	// The player we are doing lag compensation for

	float					m_flTeleportDistanceSqr;

	bool					m_isCurrentlyDoingCompensation;	// Sentinel to prevent calling StartLagCompensation a second time before a Finish.
};

static CLagCompensationManager g_LagCompensationManager( "CLagCompensationManager" );
ILagCompensationManager *lagcompensation = &g_LagCompensationManager;


//-----------------------------------------------------------------------------
// Purpose: Called once per frame after all entities have had a chance to think
//-----------------------------------------------------------------------------
void CLagCompensationManager::FrameUpdatePostEntityThink()
{
	if ( (gpGlobals->maxClients <= 1) || !sv_unlag.GetBool() )
	{
		ClearHistory();
		return;
	}
	
	m_flTeleportDistanceSqr = sv_lagcompensation_teleport_dist.GetFloat() * sv_lagcompensation_teleport_dist.GetFloat();

	VPROF_BUDGET( "FrameUpdatePostEntityThink", "CLagCompensationManager" );

	// remove all records before that time:
	int flDeadtime = gpGlobals->curtime - sv_maxunlag.GetFloat();

	// Iterate all active players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		CUtlFixedLinkedList< LagRecord > *track = &m_PlayerTrack[i-1];

		if ( !pPlayer )
		{
			if ( track->Count() > 0 )
			{
				track->RemoveAll();
			}

			continue;
		}

		Assert( track->Count() < 1000 ); // insanity check

		// remove tail records that are too old
		intp tailIndex = track->Tail();
		while ( track->IsValidIndex( tailIndex ) )
		{
			LagRecord &tail = track->Element( tailIndex );

			// if tail is within limits, stop
			if ( tail.m_flSimulationTime >= flDeadtime )
				break;
			
			// remove tail, get new tail
			track->Remove( tailIndex );
			tailIndex = track->Tail();
		}

		// check if head has same simulation time
		if ( track->Count() > 0 )
		{
			LagRecord &head = track->Element( track->Head() );

			// check if player changed simulation time since last time updated
			if ( head.m_flSimulationTime >= pPlayer->GetSimulationTime() )
				continue; // don't add new entry for same or older time
		}

		// add new record to player track
		LagRecord &record = track->Element( track->AddToHead() );

		record.m_fFlags = 0;
		if ( pPlayer->IsAlive() )
		{
			record.m_fFlags |= LC_ALIVE;
		}

		record.m_flSimulationTime	= pPlayer->GetSimulationTime();
		record.m_vecAngles			= pPlayer->GetLocalAngles();
		record.m_vecOrigin			= pPlayer->GetLocalOrigin();
		record.m_vecMinsPreScaled	= pPlayer->CollisionProp()->OBBMinsPreScaled();
		record.m_vecMaxsPreScaled	= pPlayer->CollisionProp()->OBBMaxsPreScaled();

		int layerCount = pPlayer->GetNumAnimOverlays();
		for( int layerIndex = 0; layerIndex < layerCount; ++layerIndex )
		{
			CAnimationLayer *currentLayer = pPlayer->GetAnimOverlay(layerIndex);
			if( currentLayer )
			{
				record.m_layerRecords[layerIndex].m_cycle = currentLayer->m_flCycle;
				record.m_layerRecords[layerIndex].m_order = currentLayer->m_nOrder;
				record.m_layerRecords[layerIndex].m_sequence = currentLayer->m_nSequence;
				record.m_layerRecords[layerIndex].m_weight = currentLayer->m_flWeight;
			}
		}
		record.m_masterSequence = pPlayer->GetSequence();
		record.m_masterCycle = pPlayer->GetCycle();

		for( int i=0; i<MAXSTUDIOPOSEPARAM; i++ )
		{
			record.m_flPoseParameters[i] = pPlayer->GetPoseParameter(i);
		}
	}

	//Clear the current player.
	m_pCurrentPlayer = NULL;
}

// Called during player movement to set up/restore after lag compensation
void CLagCompensationManager::StartLagCompensation( CBasePlayer *player, CUserCmd *cmd )
{
	Assert( !m_isCurrentlyDoingCompensation );

	//DONT LAG COMP AGAIN THIS FRAME IF THERES ALREADY ONE IN PROGRESS
	//IF YOU'RE HITTING THIS THEN IT MEANS THERES A CODE BUG
	if ( m_pCurrentPlayer )
	{
		Assert( m_pCurrentPlayer == NULL );
		Warning( "Trying to start a new lag compensation session while one is already active!\n" );
		return;
	}

	// Assume no players need to be restored
	m_RestorePlayer.ClearAll();
	m_bNeedToRestore = false;

	m_pCurrentPlayer = player;
	
	if ( !player->m_bLagCompensation		// Player not wanting lag compensation
		 || (gpGlobals->maxClients <= 1)	// no lag compensation in single player
		 || !sv_unlag.GetBool()				// disabled by server admin
		 || player->IsBot() 				// not for bots
		 || player->IsObserver()			// not for spectators
		)
		return;

	// NOTE: Put this here so that it won't show up in single player mode.
	VPROF_BUDGET( "StartLagCompensation", VPROF_BUDGETGROUP_OTHER_NETWORKING );
	Q_memset( m_RestoreData, 0, sizeof( m_RestoreData ) );
	Q_memset( m_ChangeData, 0, sizeof( m_ChangeData ) );

	m_isCurrentlyDoingCompensation = true;

	// Get true latency

	// correct is the amout of time we have to correct game time
	float correct = 0.0f;

	INetChannelInfo *nci = engine->GetPlayerNetInfo( player->entindex() ); 

	if ( nci )
	{
		// add network latency
		correct+= nci->GetLatency( FLOW_OUTGOING );
	}

	// calc number of view interpolation ticks - 1
	int lerpTicks = TIME_TO_TICKS( player->m_fLerpTime );

	// add view interpolation latency see C_BaseEntity::GetInterpolationAmount()
	correct += TICKS_TO_TIME( lerpTicks );
	
	// check bouns [0,sv_maxunlag]
	correct = clamp( correct, 0.0f, sv_maxunlag.GetFloat() );

	// correct tick send by player 
	int targettick = cmd->tick_count - lerpTicks;

	// calc difference between tick send by player and our latency based tick
	float deltaTime =  correct - TICKS_TO_TIME(gpGlobals->tickcount - targettick);

	if ( fabs( deltaTime ) > 0.2f )
	{
		// difference between cmd time and latency is too big > 200ms, use time correction based on latency
		// DevMsg("StartLagCompensation: delta too big (%.3f)\n", deltaTime );
		targettick = gpGlobals->tickcount - TIME_TO_TICKS( correct );
	}
	
	// Iterate all active players
	const CBitVec<MAX_EDICTS> *pEntityTransmitBits = engine->GetEntityTransmitBitsForClient( player->entindex() - 1 );
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
		{
			continue;
		}

		// Don't lag compensate yourself you loser...
		if ( player == pPlayer )
		{
			continue;
		}

		// Custom checks for if things should lag compensate (based on things like what team the player is on).
		if ( !player->WantsLagCompensationOnEntity( pPlayer, cmd, pEntityTransmitBits ) )
			continue;

		// Move other player back in time
		BacktrackPlayer( pPlayer, TICKS_TO_TIME( targettick ) );
	}
}

void CLagCompensationManager::BacktrackPlayer( CBasePlayer *pPlayer, float flTargetTime )
{
	Vector org;
	Vector minsPreScaled;
	Vector maxsPreScaled;
	QAngle ang;

	VPROF_BUDGET( "BacktrackPlayer", "CLagCompensationManager" );
	int pl_index = pPlayer->entindex() - 1;

	// get track history of this player
	CUtlFixedLinkedList< LagRecord > *track = &m_PlayerTrack[ pl_index ];

	// check if we have at leat one entry
	if ( track->Count() <= 0 )
		return;

	intp curr = track->Head();

	LagRecord *prevRecord = NULL;
	LagRecord *record = NULL;

	Vector prevOrg = pPlayer->GetLocalOrigin();
	
	// Walk context looking for any invalidating event
	while( track->IsValidIndex(curr) )
	{
		// remember last record
		prevRecord = record;

		// get next record
		record = &track->Element( curr );

		if ( !(record->m_fFlags & LC_ALIVE) )
		{
			// player most be alive, lost track
			return;
		}

		Vector delta = record->m_vecOrigin - prevOrg;
		if ( delta.Length2DSqr() > m_flTeleportDistanceSqr )
		{
			// lost track, too much difference
			return; 
		}

		// did we find a context smaller than target time ?
		if ( record->m_flSimulationTime <= flTargetTime )
			break; // hurra, stop

		prevOrg = record->m_vecOrigin;

		// go one step back
		curr = track->Next( curr );
	}

	Assert( record );

	if ( !record )
	{
		if ( sv_unlag_debug.GetBool() )
		{
			DevMsg( "No valid positions in history for BacktrackPlayer client ( %s )\n", pPlayer->GetPlayerName() );
		}

		return; // that should never happen
	}

	float frac = 0.0f;
	if ( prevRecord && 
		 (record->m_flSimulationTime < flTargetTime) &&
		 (record->m_flSimulationTime < prevRecord->m_flSimulationTime) )
	{
		// we didn't find the exact time but have a valid previous record
		// so interpolate between these two records;

		Assert( prevRecord->m_flSimulationTime > record->m_flSimulationTime );
		Assert( flTargetTime < prevRecord->m_flSimulationTime );

		// calc fraction between both records
		frac = ( flTargetTime - record->m_flSimulationTime ) / 
			( prevRecord->m_flSimulationTime - record->m_flSimulationTime );

		Assert( frac > 0 && frac < 1 ); // should never extrapolate

		ang				= Lerp( frac, record->m_vecAngles, prevRecord->m_vecAngles );
		org				= Lerp( frac, record->m_vecOrigin, prevRecord->m_vecOrigin );
		minsPreScaled	= Lerp( frac, record->m_vecMinsPreScaled, prevRecord->m_vecMinsPreScaled );
		maxsPreScaled	= Lerp( frac, record->m_vecMaxsPreScaled, prevRecord->m_vecMaxsPreScaled );
	}
	else
	{
		// we found the exact record or no other record to interpolate with
		// just copy these values since they are the best we have
		org				= record->m_vecOrigin;
		ang				= record->m_vecAngles;
		minsPreScaled	= record->m_vecMinsPreScaled;
		maxsPreScaled	= record->m_vecMaxsPreScaled;
	}

	// See if this is still a valid position for us to teleport to
	if ( sv_unlag_fixstuck.GetBool() )
	{
		// Try to move to the wanted position from our current position.
		trace_t tr;
		UTIL_TraceEntity( pPlayer, org, org, MASK_PLAYERSOLID, &tr );
		if ( tr.startsolid || tr.allsolid )
		{
			if ( sv_unlag_debug.GetBool() )
				DevMsg( "WARNING: BackupPlayer trying to back player into a bad position - client %s\n", pPlayer->GetPlayerName() );

			CBasePlayer *pHitPlayer = dynamic_cast<CBasePlayer *>( tr.m_pEnt );

			// don't lag compensate the current player
			if ( pHitPlayer && ( pHitPlayer != m_pCurrentPlayer ) )	
			{
				// If we haven't backtracked this player, do it now
				// this deliberately ignores WantsLagCompensationOnEntity.
				if ( !m_RestorePlayer.Get( pHitPlayer->entindex() - 1 ) )
				{
					// prevent recursion - save a copy of m_RestorePlayer,
					// pretend that this player is off-limits
					int pl_index = pPlayer->entindex() - 1;

					// Temp turn this flag on
					m_RestorePlayer.Set( pl_index );

					BacktrackPlayer( pHitPlayer, flTargetTime );

					// Remove the temp flag
					m_RestorePlayer.Clear( pl_index );
				}				
			}

			// now trace us back as far as we can go
			UTIL_TraceEntity( pPlayer, pPlayer->GetLocalOrigin(), org, MASK_PLAYERSOLID, &tr );

			if ( tr.startsolid || tr.allsolid )
			{
				// Our starting position is bogus

				if ( sv_unlag_debug.GetBool() )
					DevMsg( "Backtrack failed completely, bad starting position\n" );
			}
			else
			{
				// We can get to a valid place, but not all the way to the target
				Vector vPos;
				VectorLerp( pPlayer->GetLocalOrigin(), org, tr.fraction * g_flFractionScale, vPos );
				
				// This is as close as we're going to get
				org = vPos;

				if ( sv_unlag_debug.GetBool() )
					DevMsg( "Backtrack got most of the way\n" );
			}
		}
	}
	
	// See if this represents a change for the player
	int flags = 0;
	LagRecord *restore = &m_RestoreData[ pl_index ];
	LagRecord *change  = &m_ChangeData[ pl_index ];

	QAngle angdiff = pPlayer->GetLocalAngles() - ang;
	Vector orgdiff = pPlayer->GetLocalOrigin() - org;

	// Always remember the pristine simulation time in case we need to restore it.
	restore->m_flSimulationTime = pPlayer->GetSimulationTime();

	if ( angdiff.LengthSqr() > LAG_COMPENSATION_EPS_SQR )
	{
		flags |= LC_ANGLES_CHANGED;
		restore->m_vecAngles = pPlayer->GetLocalAngles();
		pPlayer->SetLocalAngles( ang );
		change->m_vecAngles = ang;
	}

	// Use absolute equality here
	if ( minsPreScaled != pPlayer->CollisionProp()->OBBMinsPreScaled() || maxsPreScaled != pPlayer->CollisionProp()->OBBMaxsPreScaled() )
	{
		flags |= LC_SIZE_CHANGED;

		restore->m_vecMinsPreScaled = pPlayer->CollisionProp()->OBBMinsPreScaled();
		restore->m_vecMaxsPreScaled = pPlayer->CollisionProp()->OBBMaxsPreScaled();
		
		pPlayer->SetSize( minsPreScaled, maxsPreScaled );
		
		change->m_vecMinsPreScaled = minsPreScaled;
		change->m_vecMaxsPreScaled = maxsPreScaled;
	}

	// Note, do origin at end since it causes a relink into the k/d tree
	if ( orgdiff.LengthSqr() > LAG_COMPENSATION_EPS_SQR )
	{
		flags |= LC_ORIGIN_CHANGED;
		restore->m_vecOrigin = pPlayer->GetLocalOrigin();
		pPlayer->SetLocalOrigin( org );
		change->m_vecOrigin = org;
	}

	// Sorry for the loss of the optimization for the case of people
	// standing still, but you breathe even on the server.
	// This is quicker than actually comparing all bazillion floats.
	flags |= LC_ANIMATION_CHANGED;
	restore->m_masterSequence = pPlayer->GetSequence();
	restore->m_masterCycle = pPlayer->GetCycle();

	bool interpolationAllowed = false;
	if( prevRecord && (record->m_masterSequence == prevRecord->m_masterSequence) )
	{
		// If the master state changes, all layers will be invalid too, so don't interp (ya know, interp barely ever happens anyway)
		interpolationAllowed = true;
	}
	
	////////////////////////
	// First do the master settings
	bool interpolatedMasters = false;
	if( frac > 0.0f && interpolationAllowed )
	{
		interpolatedMasters = true;
		pPlayer->SetSequence( Lerp( frac, record->m_masterSequence, prevRecord->m_masterSequence ) );
		pPlayer->SetCycle( Lerp( frac, record->m_masterCycle, prevRecord->m_masterCycle ) );

		if( record->m_masterCycle > prevRecord->m_masterCycle )
		{
			// the older record is higher in frame than the newer, it must have wrapped around from 1 back to 0
			// add one to the newer so it is lerping from .9 to 1.1 instead of .9 to .1, for example.
			float newCycle = Lerp( frac, record->m_masterCycle, prevRecord->m_masterCycle + 1 );
			pPlayer->SetCycle(newCycle < 1 ? newCycle : newCycle - 1 );// and make sure .9 to 1.2 does not end up 1.05
		}
		else
		{
			pPlayer->SetCycle( Lerp( frac, record->m_masterCycle, prevRecord->m_masterCycle ) );
		}

		for( int i=0; i<MAXSTUDIOPOSEPARAM; i++ )
		{
			//don't lerp pose params, just pick the closest
			pPlayer->SetPoseParameter( i, record->m_flPoseParameters[i] );
			//pAnimating->SetPoseParameter( i, Lerp( frac, record->m_flPoseParameters[i], prevRecord->m_flPoseParameters[i] ) );
		}
	}
	if( !interpolatedMasters )
	{
		pPlayer->SetSequence(record->m_masterSequence);
		pPlayer->SetCycle(record->m_masterCycle);

		for( int i=0; i<MAXSTUDIOPOSEPARAM; i++ )
		{
			pPlayer->SetPoseParameter( i, record->m_flPoseParameters[i] );
		}
	}

	////////////////////////
	// Now do all the layers
	int layerCount = pPlayer->GetNumAnimOverlays();
	for( int layerIndex = 0; layerIndex < layerCount; ++layerIndex )
	{
		CAnimationLayer *currentLayer = pPlayer->GetAnimOverlay(layerIndex);
		if( currentLayer )
		{
			restore->m_layerRecords[layerIndex].m_cycle = currentLayer->m_flCycle;
			restore->m_layerRecords[layerIndex].m_order = currentLayer->m_nOrder;
			restore->m_layerRecords[layerIndex].m_sequence = currentLayer->m_nSequence;
			restore->m_layerRecords[layerIndex].m_weight = currentLayer->m_flWeight;

			bool interpolated = false;
			if( (frac > 0.0f)  &&  interpolationAllowed )
			{
				LayerRecord &recordsLayerRecord = record->m_layerRecords[layerIndex];
				LayerRecord &prevRecordsLayerRecord = prevRecord->m_layerRecords[layerIndex];
				if( (recordsLayerRecord.m_order == prevRecordsLayerRecord.m_order)
					&& (recordsLayerRecord.m_sequence == prevRecordsLayerRecord.m_sequence)
					)
				{
					// We can't interpolate across a sequence or order change
					interpolated = true;
					if( recordsLayerRecord.m_cycle > prevRecordsLayerRecord.m_cycle )
					{
						// the older record is higher in frame than the newer, it must have wrapped around from 1 back to 0
						// add one to the newer so it is lerping from .9 to 1.1 instead of .9 to .1, for example.
						float newCycle = Lerp( frac, recordsLayerRecord.m_cycle, prevRecordsLayerRecord.m_cycle + 1 );
						currentLayer->m_flCycle = newCycle < 1 ? newCycle : newCycle - 1;// and make sure .9 to 1.2 does not end up 1.05
					}
					else
					{
						currentLayer->m_flCycle = Lerp( frac, recordsLayerRecord.m_cycle, prevRecordsLayerRecord.m_cycle  );
					}
					currentLayer->m_nOrder = recordsLayerRecord.m_order;
					currentLayer->m_nSequence = recordsLayerRecord.m_sequence;
					currentLayer->m_flWeight = Lerp( frac, recordsLayerRecord.m_weight, prevRecordsLayerRecord.m_weight  );
				}
			}
			if( !interpolated )
			{
				//Either no interp, or interp failed.  Just use record.
				currentLayer->m_flCycle = record->m_layerRecords[layerIndex].m_cycle;
				currentLayer->m_nOrder = record->m_layerRecords[layerIndex].m_order;
				currentLayer->m_nSequence = record->m_layerRecords[layerIndex].m_sequence;
				currentLayer->m_flWeight = record->m_layerRecords[layerIndex].m_weight;
			}
		}
	}
	
	if ( !flags )
		return; // we didn't change anything

	if ( sv_lagflushbonecache.GetBool() )
		pPlayer->InvalidateBoneCache();

	/*char text[256]; Q_snprintf( text, sizeof(text), "time %.2f", flTargetTime );
	pPlayer->DrawServerHitboxes( 10 );
	NDebugOverlay::Text( org, text, false, 10 );
	NDebugOverlay::EntityBounds( pPlayer, 255, 0, 0, 32, 10 ); */

	m_RestorePlayer.Set( pl_index ); //remember that we changed this player
	m_bNeedToRestore = true;  // we changed at least one player
	restore->m_fFlags = flags; // we need to restore these flags
	change->m_fFlags = flags; // we have changed these flags

	if( sv_showlagcompensation.GetInt() == 1 )
	{
		pPlayer->DrawServerHitboxes(4, true);
	}
}


void CLagCompensationManager::FinishLagCompensation( CBasePlayer *player )
{
	VPROF_BUDGET_FLAGS( "FinishLagCompensation", VPROF_BUDGETGROUP_OTHER_NETWORKING, BUDGETFLAG_CLIENT|BUDGETFLAG_SERVER );

	m_pCurrentPlayer = NULL;

	if ( !m_bNeedToRestore )
	{
		m_isCurrentlyDoingCompensation = false;
		return; // no player was changed at all
	}

	// Iterate all active players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		int pl_index = i - 1;
		
		if ( !m_RestorePlayer.Get( pl_index ) )
		{
			// player wasn't changed by lag compensation
			continue;
		}

		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
		{
			continue;
		}

		LagRecord *restore = &m_RestoreData[ pl_index ];
		LagRecord *change  = &m_ChangeData[ pl_index ];

		bool restoreSimulationTime = false;

		if ( restore->m_fFlags & LC_SIZE_CHANGED )
		{
			restoreSimulationTime = true;
	
			// see if simulation made any changes, if no, then do the restore, otherwise,
			//  leave new values in
			if ( pPlayer->CollisionProp()->OBBMinsPreScaled() == change->m_vecMinsPreScaled &&
				pPlayer->CollisionProp()->OBBMaxsPreScaled() == change->m_vecMaxsPreScaled )
			{
				// Restore it
				pPlayer->SetSize( restore->m_vecMinsPreScaled, restore->m_vecMaxsPreScaled );
			}
		}

		if ( restore->m_fFlags & LC_ANGLES_CHANGED )
		{		   
			restoreSimulationTime = true;

			if ( pPlayer->GetLocalAngles() == change->m_vecAngles )
			{
				pPlayer->SetLocalAngles( restore->m_vecAngles );
			}
		}

		if ( restore->m_fFlags & LC_ORIGIN_CHANGED )
		{
			restoreSimulationTime = true;

			// Okay, let's see if we can do something reasonable with the change
			Vector delta = pPlayer->GetLocalOrigin() - change->m_vecOrigin;
			
			// If it moved really far, just leave the player in the new spot!!!
			if ( delta.Length2DSqr() < m_flTeleportDistanceSqr )
			{
				RestorePlayerTo( pPlayer, restore->m_vecOrigin + delta );
			}
		}

		if( restore->m_fFlags & LC_ANIMATION_CHANGED )
		{
			restoreSimulationTime = true;

			pPlayer->SetSequence(restore->m_masterSequence);
			pPlayer->SetCycle(restore->m_masterCycle);

			int layerCount = pPlayer->GetNumAnimOverlays();
			for( int layerIndex = 0; layerIndex < layerCount; ++layerIndex )
			{
				CAnimationLayer *currentLayer = pPlayer->GetAnimOverlay(layerIndex);
				if( currentLayer )
				{
					currentLayer->m_flCycle = restore->m_layerRecords[layerIndex].m_cycle;
					currentLayer->m_nOrder = restore->m_layerRecords[layerIndex].m_order;
					currentLayer->m_nSequence = restore->m_layerRecords[layerIndex].m_sequence;
					currentLayer->m_flWeight = restore->m_layerRecords[layerIndex].m_weight;
				}
			}

			for( int i=0; i<MAXSTUDIOPOSEPARAM; i++ )
			{
				pPlayer->SetPoseParameter( i, restore->m_flPoseParameters[i] );
			}
		}

		if ( restoreSimulationTime )
		{
			pPlayer->SetSimulationTime( restore->m_flSimulationTime );
		}
	}

	m_isCurrentlyDoingCompensation = false;
}


