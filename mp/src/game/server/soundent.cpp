//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "soundent.h"
#include "game.h"
#include "world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Some enumerations needed by CSoundEnt
//-----------------------------------------------------------------------------

// identifiers passed to functions that can operate on either list, to indicate which list to operate on.
#define SOUNDLISTTYPE_FREE		1
#define SOUNDLISTTYPE_ACTIVE	2



LINK_ENTITY_TO_CLASS( soundent, CSoundEnt );

static CSoundEnt *g_pSoundEnt = NULL;

BEGIN_SIMPLE_DATADESC( CSound )

	DEFINE_FIELD( m_hOwner,				FIELD_EHANDLE ),
	DEFINE_FIELD( m_iVolume,			FIELD_INTEGER ),
	DEFINE_FIELD( m_flOcclusionScale,	FIELD_FLOAT ),
	DEFINE_FIELD( m_iType,				FIELD_INTEGER ),
//	DEFINE_FIELD( m_iNextAudible,		FIELD_INTEGER ),
	DEFINE_FIELD( m_bNoExpirationTime,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flExpireTime,		FIELD_TIME ),
	DEFINE_FIELD( m_iNext,				FIELD_SHORT ),
	DEFINE_FIELD( m_ownerChannelIndex,	FIELD_INTEGER ),
	DEFINE_FIELD( m_vecOrigin,			FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_bHasOwner,			FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_iMyIndex,			FIELD_INTEGER ),
	DEFINE_FIELD( m_hTarget,			FIELD_EHANDLE ),

END_DATADESC()


//=========================================================
// CSound - Clear - zeros all fields for a sound
//=========================================================
void CSound::Clear ( void )
{
	m_vecOrigin			= vec3_origin;
	m_iType				= 0;
	m_iVolume			= 0;
	m_flOcclusionScale	= 0;
	m_flExpireTime		= 0;
	m_bNoExpirationTime = false;
	m_iNext				= SOUNDLIST_EMPTY;
	m_iNextAudible		= 0;
}

//=========================================================
// Reset - clears the volume, origin, and type for a sound,
// but doesn't expire or unlink it. 
//=========================================================
void CSound::Reset ( void )
{
	m_vecOrigin		= vec3_origin;
	m_iType			= 0;
	m_iVolume		= 0;
	m_iNext			= SOUNDLIST_EMPTY;
}

//=========================================================
// FIsSound - returns true if the sound is an Audible sound
//=========================================================
bool CSound::FIsSound ( void )
{
	switch( SoundTypeNoContext() )
	{
	case SOUND_COMBAT:
	case SOUND_WORLD:
	case SOUND_PLAYER:
	case SOUND_DANGER:
	case SOUND_DANGER_SNIPERONLY:
	case SOUND_THUMPER:
	case SOUND_BULLET_IMPACT:
	case SOUND_BUGBAIT:
	case SOUND_PHYSICS_DANGER:
	case SOUND_MOVE_AWAY:
	case SOUND_PLAYER_VEHICLE:
		return true;

	default:
		return false;
	}
}

//=========================================================
// FIsScent - returns true if the sound is actually a scent
// do we really need this function? If a sound isn't a sound,
// it must be a scent. (sjb)
//=========================================================
bool CSound::FIsScent ( void )
{
	switch( m_iType )
	{
	case SOUND_CARCASS:
	case SOUND_MEAT:
	case SOUND_GARBAGE:
		return true;

	default:
		return false;
	}
}


//---------------------------------------------------------
// This function returns the spot the listener should be
// interested in if he hears the sound. MOST of the time,
// this spot is the same as the sound's origin. But sometimes
// (like with bullet impacts) the entity that owns the 
// sound is more interesting than the actual location of the
// sound effect.
//---------------------------------------------------------
const Vector &CSound::GetSoundReactOrigin( void )
{
	
	// Check pure types.
	switch( m_iType )
	{
	case SOUND_BULLET_IMPACT:
	case SOUND_PHYSICS_DANGER:
		if( m_hOwner.Get() != NULL )
		{
			// We really want the origin of this sound's 
			// owner.
			return m_hOwner->GetAbsOrigin();
		}
		else
		{
			// If the owner is somehow invalid, we'll settle
			// for the sound's origin rather than a crash.
			return GetSoundOrigin();
		}
		break;
	}

	if( m_iType & SOUND_CONTEXT_REACT_TO_SOURCE )
	{
		if( m_hOwner.Get() != NULL )
		{
			return m_hOwner->GetAbsOrigin();
		}
	}

	// Check for types with additional context.
	if( m_iType & SOUND_DANGER )
	{
		if( (m_iType & SOUND_CONTEXT_FROM_SNIPER) )
		{
			if( m_hOwner.Get() != NULL )
			{
				// Be afraid of the sniper's location, not where the bullet will hit.
				return m_hOwner->GetAbsOrigin();
			}
			else
			{
				return GetSoundOrigin();
			}
		}
	}


	return GetSoundOrigin();
}



//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CSoundEnt )

	DEFINE_FIELD( m_iFreeSound,			FIELD_INTEGER ),
	DEFINE_FIELD( m_iActiveSound,		FIELD_INTEGER ),
	DEFINE_FIELD( m_cLastActiveSounds,	FIELD_INTEGER ),
	DEFINE_EMBEDDED_ARRAY( m_SoundPool, MAX_WORLD_SOUNDS_SP ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Class factory methods
//-----------------------------------------------------------------------------
bool CSoundEnt::InitSoundEnt()
{
	///!!!LATER - do we want a sound ent in deathmatch? (sjb)
	g_pSoundEnt = (CSoundEnt*)CBaseEntity::Create( "soundent", vec3_origin, vec3_angle, GetWorldEntity() );
	if ( !g_pSoundEnt )
	{
		Warning( "**COULD NOT CREATE SOUNDENT**\n" );
		return false;
	}
	g_pSoundEnt->AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );
	return true;
}

void CSoundEnt::ShutdownSoundEnt()
{
	if ( g_pSoundEnt )
	{
		g_pSoundEnt->FreeList();
		g_pSoundEnt = NULL;
	}
}


//-----------------------------------------------------------------------------
// Construction, destruction
//-----------------------------------------------------------------------------
CSoundEnt::CSoundEnt()
{
}

CSoundEnt::~CSoundEnt()
{
}


//=========================================================
// Spawn 
//=========================================================
void CSoundEnt::Spawn( void )
{
	SetSolid( SOLID_NONE );
	Initialize();

	SetNextThink( gpGlobals->curtime + 1 );
}

void CSoundEnt::OnRestore()
{
	BaseClass::OnRestore();

	// Make sure the singleton points to the restored version of this.
	if ( g_pSoundEnt )
	{
		Assert( g_pSoundEnt != this );
		UTIL_Remove( g_pSoundEnt );
	}
	g_pSoundEnt = this;
}


//=========================================================
// Think - at interval, the entire active sound list is checked
// for sounds that have ExpireTimes less than or equal
// to the current world time, and these sounds are deallocated.
//=========================================================
void CSoundEnt::Think ( void )
{
	int iSound;
	int iPreviousSound;

	SetNextThink( gpGlobals->curtime + 0.1 );// how often to check the sound list.

	iPreviousSound = SOUNDLIST_EMPTY;
	iSound = m_iActiveSound; 

	while ( iSound != SOUNDLIST_EMPTY )
	{
		if ( (m_SoundPool[ iSound ].m_flExpireTime <= gpGlobals->curtime && (!m_SoundPool[ iSound ].m_bNoExpirationTime)) || !m_SoundPool[iSound].ValidateOwner() )
		{
			int iNext = m_SoundPool[ iSound ].m_iNext;

			if( displaysoundlist.GetInt() == 1 )
			{
				Msg("  Removed Sound: %d (Time:%f)\n", m_SoundPool[ iSound ].SoundType(), gpGlobals->curtime );
			}
			if( displaysoundlist.GetInt() == 2 && m_SoundPool[ iSound ].IsSoundType( SOUND_DANGER ) )
			{
				Msg("  Removed Danger Sound: %d (time:%f)\n", m_SoundPool[ iSound ].SoundType(), gpGlobals->curtime );
			}

			// move this sound back into the free list
			FreeSound( iSound, iPreviousSound );

			iSound = iNext;
		}
		else
		{
			if( displaysoundlist.GetBool() )
			{
				Vector forward, right, up;
				GetVectors( &forward, &right, &up );
				byte r, g, b;

				// Default to yellow.
				r = 255;
				g = 255;
				b = 0;

				CSound *pSound = &m_SoundPool[ iSound ];

				if( pSound->IsSoundType( SOUND_DANGER ) )
				{
					r = 255;
					g = 0;
					b = 0;
				}

				if( displaysoundlist.GetInt() == 1 || (displaysoundlist.GetInt() == 2 && pSound->IsSoundType( SOUND_DANGER ) ) )
				{
					NDebugOverlay::Line( pSound->GetSoundOrigin(), pSound->GetSoundOrigin() + forward * pSound->Volume(), r,g,b, false, 0.1 );
					NDebugOverlay::Line( pSound->GetSoundOrigin(), pSound->GetSoundOrigin() - forward * pSound->Volume(), r,g,b, false, 0.1 );

					NDebugOverlay::Line( pSound->GetSoundOrigin(), pSound->GetSoundOrigin() + right * pSound->Volume(), r,g,b, false, 0.1 );
					NDebugOverlay::Line( pSound->GetSoundOrigin(), pSound->GetSoundOrigin() - right * pSound->Volume(), r,g,b, false, 0.1 );

					NDebugOverlay::Line( pSound->GetSoundOrigin(), pSound->GetSoundOrigin() + up * pSound->Volume(), r,g,b, false, 0.1 );
					NDebugOverlay::Line( pSound->GetSoundOrigin(), pSound->GetSoundOrigin() - up * pSound->Volume(), r,g,b, false, 0.1 );

					if( pSound->m_flOcclusionScale != 1.0 )
					{
						// Draw the occluded radius, too.
						r = 0; g = 150; b = 255;
						NDebugOverlay::Line( pSound->GetSoundOrigin(), pSound->GetSoundOrigin() + forward * pSound->OccludedVolume(), r,g,b, false, 0.1 );
						NDebugOverlay::Line( pSound->GetSoundOrigin(), pSound->GetSoundOrigin() - forward * pSound->OccludedVolume(), r,g,b, false, 0.1 );

						NDebugOverlay::Line( pSound->GetSoundOrigin(), pSound->GetSoundOrigin() + right * pSound->OccludedVolume(), r,g,b, false, 0.1 );
						NDebugOverlay::Line( pSound->GetSoundOrigin(), pSound->GetSoundOrigin() - right * pSound->OccludedVolume(), r,g,b, false, 0.1 );

						NDebugOverlay::Line( pSound->GetSoundOrigin(), pSound->GetSoundOrigin() + up * pSound->OccludedVolume(), r,g,b, false, 0.1 );
						NDebugOverlay::Line( pSound->GetSoundOrigin(), pSound->GetSoundOrigin() - up * pSound->OccludedVolume(), r,g,b, false, 0.1 );
					}
				}

				DevMsg( 2, "Soundlist: %d / %d  (%d)\n", ISoundsInList( SOUNDLISTTYPE_ACTIVE ),ISoundsInList( SOUNDLISTTYPE_FREE ), ISoundsInList( SOUNDLISTTYPE_ACTIVE ) - m_cLastActiveSounds );
				m_cLastActiveSounds = ISoundsInList ( SOUNDLISTTYPE_ACTIVE );
			}

			iPreviousSound = iSound;
			iSound = m_SoundPool[ iSound ].m_iNext;
		}
	}

}

//=========================================================
// Precache - dummy function
//=========================================================
void CSoundEnt::Precache ( void )
{
}

//=========================================================
// FreeSound - clears the passed active sound and moves it 
// to the top of the free list. TAKE CARE to only call this
// function for sounds in the Active list!!
//=========================================================
void CSoundEnt::FreeSound ( int iSound, int iPrevious )
{
	if ( !g_pSoundEnt )
	{
		// no sound ent!
		return;
	}

	if ( iPrevious != SOUNDLIST_EMPTY )
	{
		// iSound is not the head of the active list, so
		// must fix the index for the Previous sound
		g_pSoundEnt->m_SoundPool[ iPrevious ].m_iNext = g_pSoundEnt->m_SoundPool[ iSound ].m_iNext;
	}
	else 
	{
		// the sound we're freeing IS the head of the active list.
		g_pSoundEnt->m_iActiveSound = g_pSoundEnt->m_SoundPool [ iSound ].m_iNext;
	}

	// make iSound the head of the Free list.
	g_pSoundEnt->m_SoundPool[ iSound ].m_iNext = g_pSoundEnt->m_iFreeSound;
	g_pSoundEnt->m_iFreeSound = iSound;
}

//=========================================================
// IAllocSound - moves a sound from the Free list to the 
// Active list returns the index of the alloc'd sound
//=========================================================
int CSoundEnt::IAllocSound( void )
{
	int iNewSound;

	if ( m_iFreeSound == SOUNDLIST_EMPTY )
	{
		// no free sound!
		if ( developer.GetInt() >= 2 )
			Msg( "Free Sound List is full!\n" );

		return SOUNDLIST_EMPTY;
	}

	// there is at least one sound available, so move it to the
	// Active sound list, and return its SoundPool index.
	
	iNewSound = m_iFreeSound;// copy the index of the next free sound

	m_iFreeSound = m_SoundPool[ m_iFreeSound ].m_iNext;// move the index down into the free list. 

	m_SoundPool[ iNewSound ].m_iNext = m_iActiveSound;// point the new sound at the top of the active list.

	m_iActiveSound = iNewSound;// now make the new sound the top of the active list. You're done.

#ifdef DEBUG
	m_SoundPool[ iNewSound ].m_iMyIndex = iNewSound;
#endif // DEBUG

	return iNewSound;
}

//=========================================================
// InsertSound - Allocates a free sound and fills it with 
// sound info.
//=========================================================
void CSoundEnt::InsertSound ( int iType, const Vector &vecOrigin, int iVolume, float flDuration, CBaseEntity *pOwner, int soundChannelIndex, CBaseEntity *pSoundTarget )
{
	int	iThisSound;

	if ( !g_pSoundEnt )
		return;

	if( soundChannelIndex == SOUNDENT_CHANNEL_UNSPECIFIED )
	{
		// No sound channel specified. So just make a new sound.
		iThisSound = g_pSoundEnt->IAllocSound();
	}
	else
	{
		// If this entity has already got a sound in the soundlist that's on this
		// channel, update that sound. Otherwise add a new one.
		iThisSound = g_pSoundEnt->FindOrAllocateSound( pOwner, soundChannelIndex );
	}

	if ( iThisSound == SOUNDLIST_EMPTY )
	{
		DevMsg( "Could not AllocSound() for InsertSound() (Game DLL)\n" );
		return;
	}

	CSound *pSound;

	pSound = &g_pSoundEnt->m_SoundPool[ iThisSound ];

	pSound->SetSoundOrigin( vecOrigin );
	pSound->m_iType = iType;
	pSound->m_iVolume = iVolume;
	pSound->m_flOcclusionScale = 0.5;
	pSound->m_flExpireTime = gpGlobals->curtime + flDuration;
	pSound->m_bNoExpirationTime = false;
	pSound->m_hOwner.Set( pOwner );
	pSound->m_hTarget.Set( pSoundTarget );
	pSound->m_ownerChannelIndex = soundChannelIndex;

	// Keep track of whether this sound had an owner when it was made. If the sound has a long duration,
	// the owner could disappear by the time someone hears this sound, so we have to look at this boolean
	// and throw out sounds who have a NULL owner but this field set to true. (sjb) 12/2/2005
	if( pOwner )
	{
		pSound->m_bHasOwner = true;
	}
	else
	{
		pSound->m_bHasOwner = false;
	}

	if( displaysoundlist.GetInt() == 1 )
	{
		Msg("  Added Sound! Type:%d  Duration:%f (Time:%f)\n", pSound->SoundType(), flDuration, gpGlobals->curtime );
	}
	if( displaysoundlist.GetInt() == 2 && (iType & SOUND_DANGER) )
	{
		Msg("  Added Danger Sound! Duration:%f (Time:%f)\n", flDuration, gpGlobals->curtime );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
int CSoundEnt::FindOrAllocateSound( CBaseEntity *pOwner, int soundChannelIndex )
{
	int iSound = m_iActiveSound; 

	while ( iSound != SOUNDLIST_EMPTY )
	{
		CSound &sound = m_SoundPool[iSound];
		
		if ( sound.m_ownerChannelIndex == soundChannelIndex && sound.m_hOwner == pOwner )
		{
			return iSound;
		}

		iSound = sound.m_iNext;
	}

	return IAllocSound();
}

//=========================================================
// Initialize - clears all sounds and moves them into the 
// free sound list.
//=========================================================
void CSoundEnt::Initialize ( void )
{
  	int i;
	int iSound;

	m_cLastActiveSounds;
	m_iFreeSound = 0;
	m_iActiveSound = SOUNDLIST_EMPTY;

	// In SP, we should only use the first 64 slots so save/load works right.
	// In MP, have one for each player and 32 extras.
	int nTotalSoundsInPool = MAX_WORLD_SOUNDS_SP;
	if ( gpGlobals->maxClients > 1 )
		nTotalSoundsInPool = MIN( MAX_WORLD_SOUNDS_MP, gpGlobals->maxClients + 32 );

	if ( gpGlobals->maxClients+16 > nTotalSoundsInPool )
	{
		Warning( "CSoundEnt pool is low on sounds due to high number of clients.\n" );
	}

	for ( i = 0 ; i < nTotalSoundsInPool ; i++ )
	{
		// clear all sounds, and link them into the free sound list.
		m_SoundPool[ i ].Clear();
		m_SoundPool[ i ].m_iNext = i + 1;
	}

	m_SoundPool[ i - 1 ].m_iNext = SOUNDLIST_EMPTY;// terminate the list here.

	
	// now reserve enough sounds for each client
	for ( i = 0 ; i < gpGlobals->maxClients ; i++ )
	{
		iSound = IAllocSound();

		if ( iSound == SOUNDLIST_EMPTY )
		{
			DevMsg( "Could not AllocSound() for Client Reserve! (DLL)\n" );
			return;
		}

		m_SoundPool[ iSound ].m_bNoExpirationTime = true;
	}
}

//=========================================================
// ISoundsInList - returns the number of sounds in the desired
// sound list.
//=========================================================
int CSoundEnt::ISoundsInList ( int iListType )
{
	int i;
	int iThisSound = SOUNDLIST_EMPTY;

	if ( iListType == SOUNDLISTTYPE_FREE )
	{
		iThisSound = m_iFreeSound;
	}
	else if ( iListType == SOUNDLISTTYPE_ACTIVE )
	{
		iThisSound = m_iActiveSound;
	}
	else
	{
		Msg( "Unknown Sound List Type!\n" );
	}

	if ( iThisSound == SOUNDLIST_EMPTY )
	{
		return 0;
	}

	i = 0;

	while ( iThisSound != SOUNDLIST_EMPTY )
	{
		i++;

		iThisSound = m_SoundPool[ iThisSound ].m_iNext;
	}

	return i;
}

//=========================================================
// ActiveList - returns the head of the active sound list
//=========================================================
int CSoundEnt::ActiveList ( void )
{
	if ( !g_pSoundEnt )
	{
		return SOUNDLIST_EMPTY;
	}

	return g_pSoundEnt->m_iActiveSound;
}

//=========================================================
// FreeList - returns the head of the free sound list
//=========================================================
int CSoundEnt::FreeList ( void )
{
	if ( !g_pSoundEnt )
	{
		return SOUNDLIST_EMPTY;
	}

	return g_pSoundEnt->m_iFreeSound;
}

//=========================================================
// SoundPointerForIndex - returns a pointer to the instance
// of CSound at index's position in the sound pool.
//=========================================================
CSound*	CSoundEnt::SoundPointerForIndex( int iIndex )
{
	if ( !g_pSoundEnt )
	{
		return NULL;
	}

	if ( iIndex > ( MAX_WORLD_SOUNDS_MP - 1 ) )
	{
		Msg( "SoundPointerForIndex() - Index too large!\n" );
		return NULL;
	}

	if ( iIndex < 0 )
	{
		Msg( "SoundPointerForIndex() - Index < 0!\n" );
		return NULL;
	}

	return &g_pSoundEnt->m_SoundPool[ iIndex ];
}

//=========================================================
// Clients are numbered from 1 to MAXCLIENTS, but the client
// reserved sounds in the soundlist are from 0 to MAXCLIENTS - 1,
// so this function ensures that a client gets the proper index
// to his reserved sound in the soundlist.
//=========================================================
int CSoundEnt::ClientSoundIndex ( edict_t *pClient )
{
	int iReturn = ENTINDEX( pClient ) - 1;

#ifdef _DEBUG
	if ( iReturn < 0 || iReturn >= gpGlobals->maxClients )
	{
		Msg( "** ClientSoundIndex returning a bogus value! **\n" );
	}
#endif // _DEBUG

	return iReturn;
}

//-----------------------------------------------------------------------------
// Purpose: Return the loudest sound of the specified type at "earposition"
//-----------------------------------------------------------------------------
CSound*	CSoundEnt::GetLoudestSoundOfType( int iType, const Vector &vecEarPosition )
{
	CSound *pLoudestSound = NULL;

	int iThisSound; 
	int	iBestSound = SOUNDLIST_EMPTY;
	float flBestDist = MAX_COORD_RANGE*MAX_COORD_RANGE;// so first nearby sound will become best so far.
	float flDist;
	CSound *pSound;

	iThisSound = ActiveList();

	while ( iThisSound != SOUNDLIST_EMPTY )
	{
		pSound = SoundPointerForIndex( iThisSound );

		if ( pSound && pSound->m_iType == iType && pSound->ValidateOwner() )
		{
			flDist = ( pSound->GetSoundOrigin() - vecEarPosition ).Length();

			//FIXME: This doesn't match what's in Listen()
			//flDist = UTIL_DistApprox( pSound->GetSoundOrigin(), vecEarPosition );

			if ( flDist <= pSound->m_iVolume && flDist < flBestDist )
			{
				pLoudestSound = pSound;

				iBestSound = iThisSound;
				flBestDist = flDist;
			}
		}

		iThisSound = pSound->m_iNext;
	}

	return pLoudestSound;
}


//-----------------------------------------------------------------------------
// Purpose: Inserts an AI sound into the world sound list.
//-----------------------------------------------------------------------------
class CAISound : public CPointEntity
{
public:
	CAISound()
	{
		// Initialize these new keyvalues appropriately
		// in order to support legacy instances of ai_sound.
		m_iSoundContext = 0x00000000;
		m_iVolume = 0;
		m_flDuration = 0.3;
	}

	DECLARE_CLASS( CAISound, CPointEntity );

	DECLARE_DATADESC();

	// data
	int			m_iSoundType;
	int			m_iSoundContext;
	int			m_iVolume;
	float		m_flDuration;
	string_t	m_iszProxyEntityName;

	// Input handlers
	void InputInsertSound( inputdata_t &inputdata );
	void InputEmitAISound( inputdata_t &inputdata );
};

LINK_ENTITY_TO_CLASS( ai_sound, CAISound );

BEGIN_DATADESC( CAISound )

	DEFINE_KEYFIELD( m_iSoundType, FIELD_INTEGER, "soundtype" ),
	DEFINE_KEYFIELD( m_iSoundContext, FIELD_INTEGER, "soundcontext" ),
	DEFINE_KEYFIELD( m_iVolume, FIELD_INTEGER, "volume" ),
	DEFINE_KEYFIELD( m_flDuration, FIELD_FLOAT, "duration" ),
	DEFINE_KEYFIELD( m_iszProxyEntityName, FIELD_STRING, "locationproxy" ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "InsertSound", InputInsertSound ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EmitAISound", InputEmitAISound ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: *** OBSOLETE **** Here for legacy support only!
//-----------------------------------------------------------------------------
void CAISound::InputInsertSound( inputdata_t &inputdata )
{
	int iVolume;

	iVolume = inputdata.value.Int();

	Vector vecLocation = GetAbsOrigin();

	if( m_iszProxyEntityName != NULL_STRING )
	{
		CBaseEntity *pProxy = gEntList.FindEntityByName( NULL, m_iszProxyEntityName );

		if( pProxy )
		{
			vecLocation = pProxy->GetAbsOrigin();
		}
		else
		{
			DevWarning("Warning- ai_sound cannot find proxy entity named '%s'. Using self.\n", STRING(m_iszProxyEntityName) );
		}
	}

	g_pSoundEnt->InsertSound( m_iSoundType, vecLocation, iVolume, m_flDuration, this );
}

void CAISound::InputEmitAISound( inputdata_t &inputdata )
{
	Vector vecLocation = GetAbsOrigin();

	if( m_iszProxyEntityName != NULL_STRING )
	{
		CBaseEntity *pProxy = gEntList.FindEntityByName( NULL, m_iszProxyEntityName );

		if( pProxy )
		{
			vecLocation = pProxy->GetAbsOrigin();
		}
		else
		{
			DevWarning("Warning- ai_sound cannot find proxy entity named '%s'. Using self.\n", STRING(m_iszProxyEntityName) );
		}
	}

	g_pSoundEnt->InsertSound( m_iSoundType | m_iSoundContext, vecLocation, m_iVolume, m_flDuration, this );
}


