//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "trains.h"
#include "entitylist.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern short g_sModelIndexFireball;
#define SPRITE_FIREBALL		"sprites/zerogxplode.vmt"
#define SPRITE_SMOKE		"sprites/steam1.vmt"

void UTIL_RemoveHierarchy( CBaseEntity *pDead )
{
	if ( !pDead )
		return;

	if ( pDead->edict() )
	{
		CBaseEntity *pChild = pDead->FirstMoveChild();
		while ( pChild )
		{
			CBaseEntity *pEntity = pChild;
			pChild = pChild->NextMovePeer();

			UTIL_RemoveHierarchy( pEntity );
		}
	}
	UTIL_Remove( pDead );
}

class CFuncTankTrain : public CFuncTrackTrain
{
public:
	DECLARE_CLASS( CFuncTankTrain, CFuncTrackTrain );

	void Spawn( void );

	// Filter out damage messages that don't contain blast damage (impervious to other forms of attack)
	int	OnTakeDamage( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );
	void Blocked( CBaseEntity *pOther )
	{
		// FIxme, set speed to zero?
	}
	DECLARE_DATADESC();

private:

	COutputEvent m_OnDeath;
};

LINK_ENTITY_TO_CLASS( func_tanktrain, CFuncTankTrain );

BEGIN_DATADESC( CFuncTankTrain )

	// Outputs
	DEFINE_OUTPUT(m_OnDeath, "OnDeath"),

END_DATADESC()


void CFuncTankTrain::Spawn( void )
{
	m_takedamage = true;
	BaseClass::Spawn();
}

// Filter out damage messages that don't contain blast damage (impervious to other forms of attack)
int	CFuncTankTrain::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( ! (info.GetDamageType() & DMG_BLAST) )
		return 0;

	return BaseClass::OnTakeDamage( info );
}


//-----------------------------------------------------------------------------
// Purpose: Called when the train is killed.
// Input  : pInflictor - What killed us.
//			pAttacker - Who killed us.
//			flDamage - The damage that the killing blow inflicted.
//			bitsDamageType - Bitfield of damage types that were inflicted.
//-----------------------------------------------------------------------------
void CFuncTankTrain::Event_Killed( const CTakeDamageInfo &info )
{
	m_takedamage = DAMAGE_NO;
	m_lifeState = LIFE_DEAD;

	m_OnDeath.FireOutput( info.GetInflictor(), this );
}


//-----------------------------------------------------------------------------
// Purpose: Changes the target entity for a func_tank or tanktrain_ai
//-----------------------------------------------------------------------------
class CTankTargetChange : public CPointEntity
{
public:
	DECLARE_CLASS( CTankTargetChange, CPointEntity );

	void Precache( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	DECLARE_DATADESC();

private:
	variant_t	m_newTarget;
	string_t	m_newTargetName;
};

LINK_ENTITY_TO_CLASS( tanktrain_aitarget, CTankTargetChange );

BEGIN_DATADESC( CTankTargetChange )

	// DEFINE_FIELD( m_newTarget, variant_t ),
	DEFINE_KEYFIELD( m_newTargetName, FIELD_STRING, "newtarget" ),

END_DATADESC()


void CTankTargetChange::Precache( void )
{
	BaseClass::Precache();

	// This needs to be in Precache so save/load works
	m_newTarget.SetString( m_newTargetName );
}

void CTankTargetChange::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_target, NULL, pActivator, pCaller );

	// UNDONE: This should use more of the event system
	while ( pTarget )
	{
		// Change the target over
		pTarget->AcceptInput( "TargetEntity", this, this, m_newTarget, 0 );
		pTarget = gEntList.FindEntityByName( pTarget, m_target, NULL, pActivator, pCaller );
	}
}


// UNDONE: Should be just a logical entity, but we act as another static sound channel for the train
class CTankTrainAI : public CPointEntity
{
public:
	DECLARE_CLASS( CTankTrainAI, CPointEntity );

	virtual ~CTankTrainAI( void );

	void Precache( void );
	void Spawn( void );
	void Activate( void );
	void Think( void );

	int		SoundEnginePitch( void );
	void	SoundEngineStart( void );
	void	SoundEngineStop( void );
	void	SoundShutdown( void );

	CBaseEntity *FindTarget( string_t target, CBaseEntity *pActivator );

	DECLARE_DATADESC();

	// INPUTS
	void InputTargetEntity( inputdata_t &inputdata );

private:
	CHandle<CFuncTrackTrain>	m_hTrain;
	EHANDLE			m_hTargetEntity;
	int				m_soundPlaying;

	CSoundPatch		*m_soundTreads;
	CSoundPatch		*m_soundEngine;

	string_t		m_startSoundName;
	string_t		m_engineSoundName;
	string_t		m_movementSoundName;
	string_t		m_targetEntityName;
};

LINK_ENTITY_TO_CLASS( tanktrain_ai, CTankTrainAI );

BEGIN_DATADESC( CTankTrainAI )

	DEFINE_FIELD( m_hTrain, FIELD_EHANDLE),
	DEFINE_FIELD( m_hTargetEntity, FIELD_EHANDLE),
	DEFINE_FIELD( m_soundPlaying, FIELD_INTEGER),
	DEFINE_SOUNDPATCH( m_soundTreads ),
	DEFINE_SOUNDPATCH( m_soundEngine ),

	DEFINE_KEYFIELD( m_startSoundName, FIELD_STRING, "startsound" ),
	DEFINE_KEYFIELD( m_engineSoundName, FIELD_STRING, "enginesound" ),
	DEFINE_KEYFIELD( m_movementSoundName, FIELD_STRING, "movementsound" ),
	DEFINE_FIELD( m_targetEntityName, FIELD_STRING),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "TargetEntity", InputTargetEntity ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the target entity by name.
//-----------------------------------------------------------------------------
void CTankTrainAI::InputTargetEntity( inputdata_t &inputdata )
{
	m_targetEntityName = inputdata.value.StringID();
	m_hTargetEntity = FindTarget( m_targetEntityName, inputdata.pActivator );
	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: Finds the first entity in the entity list with the given name.
// Input  : target - String ID of the entity to find.
//			pActivator - The activating entity if this is called from an input
//				or Use handler, NULL otherwise.
//-----------------------------------------------------------------------------
CBaseEntity *CTankTrainAI::FindTarget( string_t target, CBaseEntity *pActivator )
{
	return gEntList.FindEntityGeneric( NULL, STRING( target ), this, pActivator );
}


CTankTrainAI::~CTankTrainAI( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( m_soundTreads )
	{
		controller.SoundDestroy( m_soundTreads );
	}
	
	if ( m_soundEngine )
	{
		controller.SoundDestroy( m_soundEngine );
	}
}

void CTankTrainAI::Precache( void )
{
	PrecacheScriptSound( STRING( m_startSoundName ) );
	PrecacheScriptSound( STRING( m_engineSoundName ) );
	PrecacheScriptSound( STRING( m_movementSoundName ) );
}

int CTankTrainAI::SoundEnginePitch( void )
{
	CFuncTrackTrain *pTrain = m_hTrain;
	
	// we know this isn't NULL here
	if ( pTrain->GetMaxSpeed() )
	{
		return 90 + (fabs(pTrain->GetCurrentSpeed()) * (20) / pTrain->GetMaxSpeed());
	}
	return 100;
}


void CTankTrainAI::SoundEngineStart( void )
{
	CFuncTrackTrain *pTrain = m_hTrain;

	SoundEngineStop();
	// play startup sound for train
	if ( m_startSoundName != NULL_STRING )
	{
		CPASAttenuationFilter filter( pTrain );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_ITEM;
		ep.m_pSoundName = STRING(m_startSoundName);
		ep.m_flVolume = 1.0f;
		ep.m_SoundLevel = SNDLVL_NORM;

		EmitSound( filter, pTrain->entindex(), ep );
	}

	// play the looping sounds using the envelope controller
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( m_soundTreads )
	{
		controller.Play( m_soundTreads, 1.0, 100 );
	}
	
	if ( m_soundEngine )
	{
		controller.Play( m_soundEngine, 0.5, 90 );
		controller.CommandClear( m_soundEngine );
		controller.CommandAdd( m_soundEngine, 0, SOUNDCTRL_CHANGE_PITCH, 1.5, random->RandomInt(130, 145) );
		controller.CommandAdd( m_soundEngine, 1.5, SOUNDCTRL_CHANGE_PITCH, 2, random->RandomInt(105, 115) );
	}
	
	m_soundPlaying = true;
}


void CTankTrainAI::SoundEngineStop( void )
{
	if ( !m_soundPlaying )
		return;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	
	if ( m_soundTreads )
	{
		controller.SoundFadeOut( m_soundTreads, 0.25 );
	}

	if ( m_soundEngine )
	{
		controller.CommandClear( m_soundEngine );
		controller.SoundChangePitch( m_soundEngine, 70, 3.0 );
	}
	m_soundPlaying = false;	
}


void CTankTrainAI::SoundShutdown( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	if ( m_soundTreads )
	{
		controller.Shutdown( m_soundTreads );
	}

	if ( m_soundEngine )
	{
		controller.Shutdown( m_soundEngine );
	}
	m_soundPlaying = false;	
}

//-----------------------------------------------------------------------------
// Purpose: Set up think and AI
//-----------------------------------------------------------------------------
void CTankTrainAI::Spawn( void )
{
	Precache();
	m_soundPlaying = false;
	m_hTargetEntity = NULL;
}

void CTankTrainAI::Activate( void )
{
	BaseClass::Activate();
	
	CBaseEntity *pTarget = NULL;

	CFuncTrackTrain *pTrain = NULL;

	if ( m_target != NULL_STRING )
	{
		do
		{
			pTarget = gEntList.FindEntityByName( pTarget, m_target );
			pTrain = dynamic_cast<CFuncTrackTrain *>(pTarget);
		} while (!pTrain && pTarget);
	}

	m_hTrain = pTrain;

	if ( pTrain )
	{
		SetNextThink( gpGlobals->curtime + 0.5f );
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		if ( m_movementSoundName != NULL_STRING )
		{
			CPASAttenuationFilter filter( this, ATTN_NORM * 0.5 );
			m_soundTreads = controller.SoundCreate( filter, pTrain->entindex(), CHAN_STATIC, STRING(m_movementSoundName), ATTN_NORM*0.5 );
		}
		if ( m_engineSoundName != NULL_STRING )
		{
			CPASAttenuationFilter filter( this );
			m_soundEngine = controller.SoundCreate( filter, pTrain->entindex(), CHAN_STATIC, STRING(m_engineSoundName), ATTN_NORM );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Dumb linear serach of the path
// Input  : *pStart - starting path node
//			&startPosition - starting position
//			&destination - position to move close to
// Output : int move direction 1 = forward, -1 = reverse, 0 = stop
//-----------------------------------------------------------------------------
int PathFindDirection( CPathTrack *pStart, const Vector &startPosition, const Vector &destination )
{
	if ( !pStart )
		return 0;		// no path, don't move

	CPathTrack *pPath = pStart->m_pnext;
	CPathTrack *pNearest = pStart;

	float nearestDist = (pNearest->GetLocalOrigin() - destination).LengthSqr();
	float length = 0;
	float nearestForward = 0, nearestReverse = 0;

	do
	{
		float dist = (pPath->GetLocalOrigin() - destination).LengthSqr();
		
		// This is closer than our current estimate
		if ( dist < nearestDist )
		{
			nearestDist = dist;
			pNearest = pPath;
			nearestForward = length;	// current path length forward
			nearestReverse = 0;			// count until we hit the start again
		}
		CPathTrack *pNext = pPath->m_pnext;
		if ( pNext )
		{
			// UNDONE: Cache delta in path?
			float delta = (pNext->GetLocalOrigin() - pPath->GetLocalOrigin()).LengthSqr();
			length += delta;
			// add to current reverse estimate
			nearestReverse += delta;
			pPath = pNext;
		}
		else
		{
			// not a looping path
			// traverse back to other end of the path
			int fail = 0;
			while ( pPath->m_pprevious )
			{
				fail++;
				// HACKHACK: Don't infinite loop
				if ( fail > 256 )
					break;
				pPath = pPath->m_pprevious;
			}
			// don't take the reverse path to old node
			nearestReverse = nearestForward + 1;
			// dont' take forward path to new node (if we find one)
			length = (float)COORD_EXTENT * (float)COORD_EXTENT; // HACKHACK: Max quad length
		}

	} while ( pPath != pStart );

	// UNDONE: Fix this fudge factor
	// if you are already at the path, or <100 units away, don't move
	if ( pNearest == pStart || (pNearest->GetLocalOrigin() - startPosition).LengthSqr() < 100 )
		return 0;

	if ( nearestForward <= nearestReverse )
		return 1;

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: Find a point on my path near to the target and move toward it
//-----------------------------------------------------------------------------
void CTankTrainAI::Think( void )
{
	CFuncTrackTrain *pTrain = m_hTrain;

	if ( !pTrain || pTrain->m_lifeState != LIFE_ALIVE )
	{
		SoundShutdown();
		if ( pTrain )
			UTIL_RemoveHierarchy( pTrain );
		UTIL_Remove( this );
		return;
	}

	int desired = 0;
	CBaseEntity *pTarget = m_hTargetEntity;
	if ( pTarget )
	{
		desired = PathFindDirection( pTrain->m_ppath, pTrain->GetLocalOrigin(), pTarget->GetLocalOrigin() );
	}

	// If the train wants to stop, figure out throttle
	// otherwise, just throttle in the indicated direction and let the train logic
	// clip the speed
	if ( !desired )
	{
		if ( pTrain->m_flSpeed > 0 )
		{
			desired = -1;
		}
		else if ( pTrain->m_flSpeed < 0 )
		{
			desired = 1;
		}
	}
	
	// UNDONE: Align the think time with arrival, and bump this up to a few seconds
	SetNextThink( gpGlobals->curtime + 0.5f );

	if ( desired != 0 )
	{
		int wasMoving = (pTrain->m_flSpeed == 0) ? false : true;
		// chaser wants train to move, send message
		pTrain->SetSpeed( desired );
		int isMoving = (pTrain->m_flSpeed == 0) ? false : true;

		if ( !isMoving && wasMoving )
		{
			SoundEngineStop();
		}
		else if ( isMoving )
		{
			if ( !wasMoving )
			{
				SoundEngineStart();
			}
		}
	}
	else
	{
		SoundEngineStop();
		// UNDONE: Align the think time with arrival, and bump this up to a few seconds
		SetNextThink( gpGlobals->curtime + 1.0f );
	}
}
