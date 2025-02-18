//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "soundscape.h"
#include "datamap.h"
#include "soundscape_system.h"
#include "triggers.h"
#include "saverestore_utlvector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar soundscape_debug( "soundscape_debug", "0", FCVAR_CHEAT, "When on, draws lines to all env_soundscape entities. Green lines show the active soundscape, red lines show soundscapes that aren't in range, and white lines show soundscapes that are in range, but not the active soundscape." );

// ----------------------------------------------------------------------------- //
// CEnvSoundscapeProxy stuff.
// ----------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS( env_soundscape_proxy, CEnvSoundscapeProxy );

BEGIN_DATADESC( CEnvSoundscapeProxy )
	
	DEFINE_KEYFIELD( m_MainSoundscapeName, FIELD_STRING, "MainSoundscapeName" )

END_DATADESC()


CEnvSoundscapeProxy::CEnvSoundscapeProxy()
{
	m_MainSoundscapeName = NULL_STRING;
}


void CEnvSoundscapeProxy::Activate()
{
	if ( m_MainSoundscapeName != NULL_STRING )
	{
		CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, m_MainSoundscapeName );
		if ( pEntity )
		{
			m_hProxySoundscape = dynamic_cast< CEnvSoundscape* >( pEntity );
		}
	}

	if ( m_hProxySoundscape )
	{
		// Copy the relevant parameters from our main soundscape.
		m_soundscapeIndex = m_hProxySoundscape->m_soundscapeIndex;
		for ( int i=0; i < ARRAYSIZE( m_positionNames ); i++ )
			m_positionNames[i] = m_hProxySoundscape->m_positionNames[i];
	}
	else
	{
		Warning( "env_soundscape_proxy can't find target soundscape: '%s'\n", STRING( m_MainSoundscapeName ) );
	}

	BaseClass::Activate();
}


// ----------------------------------------------------------------------------- //
// CEnvSoundscape stuff.
// ----------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS( env_soundscape, CEnvSoundscape );

BEGIN_DATADESC( CEnvSoundscape )

	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),
	// don't save, recomputed on load
	//DEFINE_FIELD( m_soundscapeIndex, FIELD_INTEGER ),
	DEFINE_FIELD( m_soundscapeName, FIELD_STRING ),
	DEFINE_FIELD( m_hProxySoundscape, FIELD_EHANDLE ),

// Silence, Classcheck!
//	DEFINE_ARRAY( m_positionNames, FIELD_STRING, 4 ),

	DEFINE_KEYFIELD( m_positionNames[0], FIELD_STRING, "position0" ),
	DEFINE_KEYFIELD( m_positionNames[1], FIELD_STRING, "position1" ),
	DEFINE_KEYFIELD( m_positionNames[2], FIELD_STRING, "position2" ),
	DEFINE_KEYFIELD( m_positionNames[3], FIELD_STRING, "position3" ),
	DEFINE_KEYFIELD( m_positionNames[4], FIELD_STRING, "position4" ),
	DEFINE_KEYFIELD( m_positionNames[5], FIELD_STRING, "position5" ),
	DEFINE_KEYFIELD( m_positionNames[6], FIELD_STRING, "position6" ),
	DEFINE_KEYFIELD( m_positionNames[7], FIELD_STRING, "position7" ),

	DEFINE_KEYFIELD( m_bDisabled,	FIELD_BOOLEAN,	"StartDisabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ToggleEnabled", InputToggleEnabled ),

	DEFINE_OUTPUT( m_OnPlay, "OnPlay" ),


END_DATADESC()

CEnvSoundscape::CEnvSoundscape()
{
	m_soundscapeName = NULL_STRING;
	m_soundscapeIndex = -1;
	m_soundscapeEntityId = -1;
	m_bDisabled = false;
	g_SoundscapeSystem.AddSoundscapeEntity( this );
}

CEnvSoundscape::~CEnvSoundscape()
{
	g_SoundscapeSystem.RemoveSoundscapeEntity( this );
}

void CEnvSoundscape::InputEnable( inputdata_t &inputdata )
{
	if (!IsEnabled())
	{
		Enable();
	}
}

void CEnvSoundscape::InputDisable( inputdata_t &inputdata )
{
	if (IsEnabled())
	{
		Disable();
	}
}

void CEnvSoundscape::InputToggleEnabled( inputdata_t &inputdata )
{
	if ( IsEnabled() )
	{
		Disable();
	}
	else
	{
		Enable();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the laser is currently active.
//-----------------------------------------------------------------------------
bool CEnvSoundscape::IsEnabled( void ) const
{
	return !m_bDisabled;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvSoundscape::Disable( void )
{
	m_bDisabled = true;

	// Reset if we are the currently active soundscape
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvSoundscape::Enable( void )
{
	m_bDisabled = false;

	// Force the player to recheck soundscapes
}

bool CEnvSoundscape::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "soundscape"))
	{
		m_soundscapeName = AllocPooledString( szValue );
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

// returns true if the given sound entity is in range 
// and can see the given player entity (pTarget)

bool CEnvSoundscape::InRangeOfPlayer( CBasePlayer *pTarget ) 
{
	Vector vecSpot1 = EarPosition();
	Vector vecSpot2 = pTarget->EarPosition();

	// calc range from sound entity to player
	Vector vecRange = vecSpot2 - vecSpot1;
	float range = vecRange.Length();
	if ( m_flRadius > range || m_flRadius == -1 )
	{
		trace_t tr;

		UTIL_TraceLine( vecSpot1, vecSpot2, MASK_SOLID_BRUSHONLY|MASK_WATER, pTarget, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction == 1 && !tr.startsolid )
		{
			return true;
		}
	}

	return false;
}

void CEnvSoundscape::WriteAudioParamsTo( audioparams_t &audio )
{
	audio.entIndex = m_soundscapeEntityId;
	audio.soundscapeIndex = m_soundscapeIndex;
	audio.localBits = 0;
	for ( int i = 0; i < ARRAYSIZE(m_positionNames); i++ )
	{
		if ( m_positionNames[i] != NULL_STRING )
		{
			// We are a valid entity for a sound position
			CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, m_positionNames[i], this, this );
			if ( pEntity )
			{
				audio.localBits |= 1<<i;
				audio.localSound.Set( i, pEntity->GetAbsOrigin() );
			}
		}
	}

	m_OnPlay.FireOutput( this, this );
}


//
// A client that is visible and in range of a sound entity will
// have its soundscape set by that sound entity.  If two or more
// sound entities are contending for a client, then the nearest
// sound entity to the client will set the client's soundscape.
// A client's soundscape will remain set to its prior value until
// a new in-range, visible sound entity resets a new soundscape.
//

// CONSIDER: if player in water state, autoset and underwater soundscape? 
void CEnvSoundscape::UpdateForPlayer( ss_update_t &update )
{
	if ( !IsEnabled() )
	{
		if ( update.pCurrentSoundscape == this )
		{
			update.pCurrentSoundscape = NULL;
			update.currentDistance = 0;
			update.bInRange = false;
		}
		return;
	}

	// calc range from sound entity to player
	Vector target = EarPosition();
	float range = (update.playerPosition - target).Length();
	
	if ( update.pCurrentSoundscape == this )
	{
		update.currentDistance = range;
		update.bInRange = false;
		if ( m_flRadius > range || m_flRadius == -1 )
		{
			trace_t tr;

			update.traceCount++;
			UTIL_TraceLine( target, update.playerPosition, MASK_SOLID_BRUSHONLY|MASK_WATER, update.pPlayer, COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction == 1 && !tr.startsolid )
			{
				update.bInRange = true;
			}
		}
	}
	else
	{
		if ( (!update.bInRange || range < update.currentDistance ) && (m_flRadius > range || m_flRadius == -1) )
		{
			trace_t tr;

			update.traceCount++;
			UTIL_TraceLine( target, update.playerPosition, MASK_SOLID_BRUSHONLY|MASK_WATER, update.pPlayer, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction == 1 && !tr.startsolid )
			{
				audioparams_t &audio = update.pPlayer->GetAudioParams();
				WriteAudioParamsTo( audio );
				update.pCurrentSoundscape = this;
				update.bInRange = true;
				update.currentDistance = range;
			}
		}
	}


	if ( soundscape_debug.GetBool() )
	{
		// draw myself
		NDebugOverlay::Box(GetAbsOrigin(), Vector(-10,-10,-10), Vector(10,10,10),  255, 0, 255, 64, NDEBUG_PERSIST_TILL_NEXT_SERVER );

 		if ( update.pPlayer )
		{
			audioparams_t &audio = update.pPlayer->GetAudioParams();
			if ( audio.entIndex != m_soundscapeEntityId )
			{
				if ( InRangeOfPlayer( update.pPlayer ) )
				{
					NDebugOverlay::Line( GetAbsOrigin(), update.pPlayer->WorldSpaceCenter(), 255, 255, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
				}
				else
				{
					NDebugOverlay::Line( GetAbsOrigin(), update.pPlayer->WorldSpaceCenter(), 255, 0, 0, true, NDEBUG_PERSIST_TILL_NEXT_SERVER  );
				}
			}
			else
			{
				if ( InRangeOfPlayer( update.pPlayer ) )
				{
					NDebugOverlay::Line( GetAbsOrigin(), update.pPlayer->WorldSpaceCenter(), 0, 255, 0, true, NDEBUG_PERSIST_TILL_NEXT_SERVER  );
				}
  				else
				{
					NDebugOverlay::Line( GetAbsOrigin(), update.pPlayer->WorldSpaceCenter(), 255, 170, 0, true, NDEBUG_PERSIST_TILL_NEXT_SERVER  );
				}

				// also draw lines to each sound position.
				// we don't store the number of local sound positions, just a bitvector of which ones are on.
				unsigned int soundbits = audio.localBits.Get();
				float periodic = 2.0f * sin((fmod(gpGlobals->curtime,2.0f) - 1.0f) * M_PI); // = -4f .. 4f
				for (int ii = 0 ; ii < NUM_AUDIO_LOCAL_SOUNDS ; ++ii )
				{
					if ( soundbits & (1 << ii) )
					{
						const Vector &soundLoc = audio.localSound.Get(ii);
						NDebugOverlay::Line( GetAbsOrigin(), soundLoc, 0, 32 , 255 , false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						NDebugOverlay::Cross3D( soundLoc, 16.0f + periodic, 0, 0, 255, false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
					}
				}
			}
		}

		NDebugOverlay::EntityTextAtPosition( GetAbsOrigin(), 0, STRING(m_soundscapeName), NDEBUG_PERSIST_TILL_NEXT_SERVER );
	}
}

//
// env_soundscape - spawn a sound entity that will set player soundscape
// when player moves in range and sight.
//
//
void CEnvSoundscape::Spawn( )
{
	Precache();

}

void CEnvSoundscape::Precache()
{
	if ( m_soundscapeName == NULL_STRING )
	{
		DevMsg("Found soundscape entity with no soundscape name.\n" );
		return;
	}

	m_soundscapeIndex = g_SoundscapeSystem.GetSoundscapeIndex( STRING(m_soundscapeName) );
	if ( IsX360())
	{
		g_SoundscapeSystem.PrecacheSounds( m_soundscapeIndex );
	}
	if ( !g_SoundscapeSystem.IsValidIndex( m_soundscapeIndex ) )
	{
		DevWarning("Can't find soundscape: %s\n", STRING(m_soundscapeName) );
	}
}

void CEnvSoundscape::DrawDebugGeometryOverlays( void )
{
	if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
		if ( pPlayer )
		{
			audioparams_t &audio = pPlayer->GetAudioParams();
			if ( audio.entIndex != m_soundscapeEntityId )
			{
				CBaseEntity *pEnt = pPlayer; // ->GetSoundscapeListener();
				if ( pEnt )
				{
					NDebugOverlay::Line(GetAbsOrigin(), pEnt->WorldSpaceCenter(), 255, 0, 255, false, 0 );
				}
			}
		}
	}

	BaseClass::DrawDebugGeometryOverlays();
}


// ---------------------------------------------------------------------------------------------------- //
// CEnvSoundscapeTriggerable
// ---------------------------------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS( env_soundscape_triggerable, CEnvSoundscapeTriggerable );

BEGIN_DATADESC( CEnvSoundscapeTriggerable )
END_DATADESC()


CEnvSoundscapeTriggerable::CEnvSoundscapeTriggerable()
{
}


void CEnvSoundscapeTriggerable::DelegateStartTouch( CBaseEntity *pEnt )
{
	CBasePlayer *pPlayer = dynamic_cast< CBasePlayer* >( pEnt );
	if ( !pPlayer )
		return;

	// Just in case.. we shouldn't already be in the player's list because it should have 
	// called DelegateEndTouch, but this seems to happen when they're noclipping.
	pPlayer->m_hTriggerSoundscapeList.FindAndRemove( this );

	// Add us to the player's list of soundscapes and 
	pPlayer->m_hTriggerSoundscapeList.AddToHead( this );
	WriteAudioParamsTo( pPlayer->GetAudioParams() );
}


void CEnvSoundscapeTriggerable::DelegateEndTouch( CBaseEntity *pEnt )
{
	CBasePlayer *pPlayer = dynamic_cast< CBasePlayer* >( pEnt );
	if ( !pPlayer )
		return;

	// Remove us from the ent's list of soundscapes.
	pPlayer->m_hTriggerSoundscapeList.FindAndRemove( this );
	while ( pPlayer->m_hTriggerSoundscapeList.Count() > 0 )
	{
		CEnvSoundscapeTriggerable *pSS = dynamic_cast< CEnvSoundscapeTriggerable* >( pPlayer->m_hTriggerSoundscapeList[0].Get() );
		if ( pSS )
		{
			// Make this one current.
			pSS->WriteAudioParamsTo( pPlayer->GetAudioParams() );
			return;
		}
		else
		{
			pPlayer->m_hTriggerSoundscapeList.Remove( 0 );
		}
	}

	// No soundscapes left.
	pPlayer->GetAudioParams().entIndex = 0;
}


void CEnvSoundscapeTriggerable::Think()
{
	// Overrides the base class's think and prevents it from running at all.
}


// ---------------------------------------------------------------------------------------------------- //
// CTriggerSoundscape
// ---------------------------------------------------------------------------------------------------- //

class CTriggerSoundscape : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerSoundscape, CBaseTrigger );
	DECLARE_DATADESC();

	CTriggerSoundscape();

	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );

	virtual void Spawn();
	virtual void Activate();

	void PlayerUpdateThink();

private:
	CHandle<CEnvSoundscapeTriggerable> m_hSoundscape;
	string_t m_SoundscapeName;

	CUtlVector<CBasePlayerHandle> m_spectators; // spectators in our volume
};


LINK_ENTITY_TO_CLASS( trigger_soundscape, CTriggerSoundscape );

BEGIN_DATADESC( CTriggerSoundscape )
	DEFINE_THINKFUNC( PlayerUpdateThink ),
	DEFINE_KEYFIELD( m_SoundscapeName, FIELD_STRING, "soundscape" ),
	DEFINE_FIELD( m_hSoundscape, FIELD_EHANDLE ),
	DEFINE_UTLVECTOR( m_spectators, FIELD_EHANDLE ), 
END_DATADESC()


CTriggerSoundscape::CTriggerSoundscape()
{
}


void CTriggerSoundscape::StartTouch( CBaseEntity *pOther )
{
	if ( m_hSoundscape )
		m_hSoundscape->DelegateStartTouch( pOther );

	BaseClass::StartTouch( pOther );
}


void CTriggerSoundscape::EndTouch( CBaseEntity *pOther )
{
	if ( m_hSoundscape )
		m_hSoundscape->DelegateEndTouch( pOther );

	BaseClass::EndTouch( pOther );
}


void CTriggerSoundscape::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();

	SetThink( &CTriggerSoundscape::PlayerUpdateThink );
	SetNextThink( gpGlobals->curtime + 0.2 );
}


void CTriggerSoundscape::Activate()
{
	m_hSoundscape = dynamic_cast< CEnvSoundscapeTriggerable* >( gEntList.FindEntityByName( NULL, m_SoundscapeName ) );
	BaseClass::Activate();
}


// look for dead/spectating players in our volume, to call touch on
void CTriggerSoundscape::PlayerUpdateThink()
{
	int i;
	SetNextThink( gpGlobals->curtime + 0.2 );

	CUtlVector<CBasePlayerHandle> oldSpectators;
	oldSpectators = m_spectators;
	m_spectators.RemoveAll();

	for ( i=1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = UTIL_PlayerByIndex( i );

		if ( !player )
			continue;

		if ( player->IsAlive() )
			continue;

		// if the spectator is intersecting the trigger, track it, and start a touch if it is just starting to touch
		if ( Intersects( player ) )
		{
			if ( !oldSpectators.HasElement( player ) )
			{
				StartTouch( player );
			}
			m_spectators.AddToTail( player );
		}
	}

	// check for spectators who are no longer intersecting
	for ( i=0; i<oldSpectators.Count(); ++i )
	{
		CBasePlayer *player = oldSpectators[i];

		if ( !player )
			continue;

		if ( !m_spectators.HasElement( player ) )
		{
			EndTouch( player );
		}
	}
}
