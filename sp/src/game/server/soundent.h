//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// Soundent.h - the entity that spawns when the world 
// spawns, and handles the world's active and free sound
// lists.

#ifndef SOUNDENT_H
#define SOUNDENT_H

#ifdef _WIN32
#pragma once
#endif

enum
{
	MAX_WORLD_SOUNDS_SP	= 64,	// Maximum number of sounds handled by the world at one time in single player.
	// This is also the number of entries saved in a savegame file (for b/w compatibility).

	MAX_WORLD_SOUNDS_MP	= 128	// The sound array size is set this large but we'll only use gpGlobals->maxPlayers+32 entries in mp.
};

enum
{
	SOUND_NONE				= 0,
	SOUND_COMBAT			= 0x00000001,
	SOUND_WORLD				= 0x00000002,
	SOUND_PLAYER			= 0x00000004,
	SOUND_DANGER			= 0x00000008,
	SOUND_BULLET_IMPACT		= 0x00000010,
	SOUND_CARCASS			= 0x00000020,
	SOUND_MEAT				= 0x00000040,
	SOUND_GARBAGE			= 0x00000080,
	SOUND_THUMPER			= 0x00000100, // keeps certain creatures at bay
	SOUND_BUGBAIT			= 0x00000200, // gets the antlion's attention
	SOUND_PHYSICS_DANGER	= 0x00000400,
	SOUND_DANGER_SNIPERONLY	= 0x00000800, // only scares the sniper NPC.
	SOUND_MOVE_AWAY			= 0x00001000,
	SOUND_PLAYER_VEHICLE	= 0x00002000,
	SOUND_READINESS_LOW		= 0x00004000, // Changes listener's readiness (Player Companion only)
	SOUND_READINESS_MEDIUM	= 0x00008000,
	SOUND_READINESS_HIGH	= 0x00010000,

	// Contexts begin here.
	SOUND_CONTEXT_FROM_SNIPER		= 0x00100000, // additional context for SOUND_DANGER
	SOUND_CONTEXT_GUNFIRE			= 0x00200000, // Added to SOUND_COMBAT
	SOUND_CONTEXT_MORTAR			= 0x00400000, // Explosion going to happen here.
	SOUND_CONTEXT_COMBINE_ONLY		= 0x00800000, // Only combine can hear sounds marked this way
	SOUND_CONTEXT_REACT_TO_SOURCE	= 0x01000000, // React to sound source's origin, not sound's location
	SOUND_CONTEXT_EXPLOSION			= 0x02000000, // Context added to SOUND_COMBAT, usually.
	SOUND_CONTEXT_EXCLUDE_COMBINE	= 0x04000000, // Combine do NOT hear this
	SOUND_CONTEXT_DANGER_APPROACH   = 0x08000000, // Treat as a normal danger sound if you see the source, otherwise turn to face source.
	SOUND_CONTEXT_ALLIES_ONLY		= 0x10000000, // Only player allies can hear this sound
	SOUND_CONTEXT_PLAYER_VEHICLE	= 0x20000000, // HACK: need this because we're not treating the SOUND_xxx values as true bit values! See switch in OnListened.

	ALL_CONTEXTS			= 0xFFF00000,

	ALL_SCENTS				= SOUND_CARCASS | SOUND_MEAT | SOUND_GARBAGE,

	ALL_SOUNDS				= 0x000FFFFF & ~ALL_SCENTS,

};

// Make as many of these as you want. 
enum
{
	SOUNDENT_CHANNEL_UNSPECIFIED = 0,
	SOUNDENT_CHANNEL_REPEATING,
	SOUNDENT_CHANNEL_REPEATED_DANGER,	// for things that make danger sounds frequently.
	SOUNDENT_CHANNEL_REPEATED_PHYSICS_DANGER,
	SOUNDENT_CHANNEL_WEAPON,
	SOUNDENT_CHANNEL_INJURY,
	SOUNDENT_CHANNEL_BULLET_IMPACT,
	SOUNDENT_CHANNEL_NPC_FOOTSTEP,
	SOUNDENT_CHANNEL_SPOOKY_NOISE,		// made by zombies in darkness
	SOUNDENT_CHANNEL_ZOMBINE_GRENADE,
};

enum
{
	SOUNDLIST_EMPTY = -1
};

#define SOUNDENT_VOLUME_MACHINEGUN	1500.0
#define SOUNDENT_VOLUME_SHOTGUN		1500.0
#define SOUNDENT_VOLUME_PISTOL		1500.0
#define SOUNDENT_VOLUME_EMPTY		 500.0 // volume of the "CLICK" when you have no bullets

enum
{
	SOUND_PRIORITY_VERY_LOW = -2,
	SOUND_PRIORITY_LOW,
	SOUND_PRIORITY_NORMAL = 0,
	SOUND_PRIORITY_HIGH,
	SOUND_PRIORITY_VERY_HIGH,
	SOUND_PRIORITY_HIGHEST,
};

//=========================================================
// CSound - an instance of a sound in the world.
//=========================================================
class CSound
{
	DECLARE_SIMPLE_DATADESC();

public:
	bool	DoesSoundExpire() const;
	float	SoundExpirationTime() const;
	void	SetSoundOrigin( const Vector &vecOrigin ) { m_vecOrigin = vecOrigin; }
	const	Vector& GetSoundOrigin( void ) { return m_vecOrigin; }
	const	Vector& GetSoundReactOrigin( void );
	bool	FIsSound( void );
	bool	FIsScent( void );
	bool	IsSoundType( int nSoundFlags ) const;
	int		SoundType( ) const;
	int		SoundContext() const;
	int		SoundTypeNoContext( ) const;
	int		Volume( ) const;
	float	OccludedVolume() { return m_iVolume * m_flOcclusionScale; }
	int		NextSound() const;
	void	Reset ( void );
	int		SoundChannel( void ) const;
	bool	ValidateOwner() const;

	EHANDLE	m_hOwner;				// sound's owner
	EHANDLE	m_hTarget;				// Sounds's target - an odd concept. For a gunfire sound, the target is the entity being fired at
	int		m_iVolume;				// how loud the sound is
	float	m_flOcclusionScale;		// How loud the sound is when occluded by the world. (volume * occlusionscale)
	int		m_iType;				// what type of sound this is
	int		m_iNextAudible;			// temporary link that NPCs use to build a list of audible sounds

private:
	void	Clear ( void );

	float	m_flExpireTime;	// when the sound should be purged from the list
	short	m_iNext;		// index of next sound in this list ( Active or Free )
	bool	m_bNoExpirationTime;
	int		m_ownerChannelIndex;

	Vector	m_vecOrigin;	// sound's location in space

	bool	m_bHasOwner;	// Lets us know if this sound was created with an owner. In case the owner goes null.

#ifdef DEBUG
	int		m_iMyIndex;		// debugging
#endif

	friend class CSoundEnt;
};

inline bool CSound::DoesSoundExpire() const
{
	return m_bNoExpirationTime == false;
}

inline float CSound::SoundExpirationTime() const
{
	return m_bNoExpirationTime ? FLT_MAX : m_flExpireTime;
}

inline bool CSound::IsSoundType( int nSoundFlags ) const
{
	return (m_iType & nSoundFlags) != 0;
}

inline int CSound::SoundType( ) const
{
	return m_iType;
}

inline int CSound::SoundContext( ) const
{
	return m_iType & ALL_CONTEXTS;
}

inline int CSound::SoundTypeNoContext( ) const
{
	return m_iType & ~ALL_CONTEXTS;
}

inline int CSound::Volume( ) const
{
	return m_iVolume;
}

inline int CSound::NextSound() const
{
	return m_iNext;
}

inline int CSound::SoundChannel( void ) const
{
	return m_ownerChannelIndex;
}

// The owner is considered valid if:
//		-The sound never had an assigned owner (quite common)
//		-The sound was assigned an owner and that owner still exists
inline bool CSound::ValidateOwner( void ) const
{
	return ( !m_bHasOwner || (m_hOwner.Get() != NULL) );
}

//=========================================================
// CSoundEnt - a single instance of this entity spawns when
// the world spawns. The SoundEnt's job is to update the 
// world's Free and Active sound lists.
//=========================================================
class CSoundEnt : public CPointEntity
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CSoundEnt, CPointEntity );

	// Construction, destruction
	static bool InitSoundEnt();
	static void ShutdownSoundEnt();

	CSoundEnt();
	virtual ~CSoundEnt();

	virtual void OnRestore();
	void Precache ( void );
	void Spawn( void );
	void Think( void );
	void Initialize ( void );
	int ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static void		InsertSound ( int iType, const Vector &vecOrigin, int iVolume, float flDuration, CBaseEntity *pOwner = NULL, int soundChannelIndex = SOUNDENT_CHANNEL_UNSPECIFIED, CBaseEntity *pSoundTarget = NULL );
	static void		FreeSound ( int iSound, int iPrevious );
	static int		ActiveList( void );// return the head of the active list
	static int		FreeList( void );// return the head of the free list
	static CSound*	SoundPointerForIndex( int iIndex );// return a pointer for this index in the sound list
	static CSound*	GetLoudestSoundOfType( int iType, const Vector &vecEarPosition );
	static int		ClientSoundIndex ( edict_t *pClient );

	bool	IsEmpty( void );
	int		ISoundsInList ( int iListType );
	int		IAllocSound ( void );
	int		FindOrAllocateSound( CBaseEntity *pOwner, int soundChannelIndex );
	
private:
	int		m_iFreeSound;	// index of the first sound in the free sound list
	int		m_iActiveSound; // indes of the first sound in the active sound list
	int		m_cLastActiveSounds; // keeps track of the number of active sounds at the last update. (for diagnostic work)
	CSound	m_SoundPool[ MAX_WORLD_SOUNDS_MP ];
};


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline bool CSoundEnt::IsEmpty( void ) 
{ 
	return m_iActiveSound == SOUNDLIST_EMPTY; 
}


#endif //SOUNDENT_H
