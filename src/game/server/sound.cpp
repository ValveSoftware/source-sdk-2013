//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entities relating to in-level sound effects.
//
//			ambient_generic: a sound emitter used for one-shot and looping sounds.
//
//			env_speaker: used for public address announcements over loudspeakers.
//				This tries not to drown out talking NPCs.
//
//			env_soundscape: controls what sound script an area uses.
//
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "mathlib/mathlib.h"
#include "ai_speech.h"
#include "stringregistry.h"
#include "gamerules.h"
#include "game.h"
#include <ctype.h>
#include "entitylist.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "ndebugoverlay.h"
#include "soundscape.h"
#include "igamesystem.h"
#include "KeyValues.h"
#include "filesystem.h"

#ifdef PORTAL
#include "portal_gamerules.h"
#endif // PORTAL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Compute a suitable attenuation value given an audible radius
// Input  : radius - 
//			playEverywhere - (disable attenuation)
//-----------------------------------------------------------------------------
#define REFERENCE_dB			60.0

#define AMBIENT_GENERIC_UPDATE_RATE	5	// update at 5hz
#define AMBIENT_GENERIC_THINK_DELAY ( 1.0f / float( AMBIENT_GENERIC_UPDATE_RATE ) )

#ifdef HL1_DLL
ConVar hl1_ref_db_distance( "hl1_ref_db_distance", "18.0" );
#define	REFERENCE_dB_DISTANCE	hl1_ref_db_distance.GetFloat()
#else
#define REFERENCE_dB_DISTANCE	36.0
#endif//HL1_DLL

static soundlevel_t ComputeSoundlevel( float radius, bool playEverywhere )
{
	soundlevel_t soundlevel = SNDLVL_NONE;

	if ( radius > 0 && !playEverywhere )
	{
		// attenuation is set to a distance, compute falloff

		float dB_loss = 20 * log10( radius / REFERENCE_dB_DISTANCE );

		soundlevel = (soundlevel_t)(int)(40 + dB_loss); // sound at 40dB at reference distance
	}

	return soundlevel;
}

// ==================== GENERIC AMBIENT SOUND ======================================

// runtime pitch shift and volume fadein/out structure

// NOTE: IF YOU CHANGE THIS STRUCT YOU MUST CHANGE THE SAVE/RESTORE VERSION NUMBER
// SEE BELOW (in the typedescription for the class)
typedef struct dynpitchvol
{
	// NOTE: do not change the order of these parameters 
	// NOTE: unless you also change order of rgdpvpreset array elements!
	int preset;

	int pitchrun;		// pitch shift % when sound is running 0 - 255
	int pitchstart;		// pitch shift % when sound stops or starts 0 - 255
	int spinup;			// spinup time 0 - 100
	int spindown;		// spindown time 0 - 100

	int volrun;			// volume change % when sound is running 0 - 10
	int volstart;		// volume change % when sound stops or starts 0 - 10
	int fadein;			// volume fade in time 0 - 100
	int fadeout;		// volume fade out time 0 - 100

						// Low Frequency Oscillator
	int	lfotype;		// 0) off 1) square 2) triangle 3) random
	int lforate;		// 0 - 1000, how fast lfo osciallates
	
	int lfomodpitch;	// 0-100 mod of current pitch. 0 is off.
	int lfomodvol;		// 0-100 mod of current volume. 0 is off.

	int cspinup;		// each trigger hit increments counter and spinup pitch


	int	cspincount;

	int pitch;			
	int spinupsav;
	int spindownsav;
	int pitchfrac;

	int vol;
	int fadeinsav;
	int fadeoutsav;
	int volfrac;

	int	lfofrac;
	int	lfomult;


} dynpitchvol_t;

#define CDPVPRESETMAX 27

// presets for runtime pitch and vol modulation of ambient sounds

dynpitchvol_t rgdpvpreset[CDPVPRESETMAX] = 
{
// pitch	pstart	spinup	spindwn	volrun	volstrt	fadein	fadeout	lfotype	lforate	modptch modvol	cspnup		
{1,	255,	 75,	95,		95,		10,		1,		50,		95, 	0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0}, 
{2,	255,	 85,	70,		88,		10,		1,		20,		88,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0}, 
{3,	255,	100,	50,		75,		10,		1,		10,		75,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{4,	100,	100,	0,		0,		10,		1,		90,		90,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{5,	100,	100,	0,		0,		10,		1,		80,		80,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{6,	100,	100,	0,		0,		10,		1,		50,		70,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{7,	100,	100,	0,		0,		 5,		1,		40,		50,		1,		50,		0,		10,		0,		0,0,0,0,0,0,0,0,0,0},
{8,	100,	100,	0,		0,		 5,		1,		40,		50,		1,		150,	0,		10,		0,		0,0,0,0,0,0,0,0,0,0},
{9,	100,	100,	0,		0,		 5,		1,		40,		50,		1,		750,	0,		10,		0,		0,0,0,0,0,0,0,0,0,0},
{10,128,	100,	50,		75,		10,		1,		30,		40,		2,		 8,		20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{11,128,	100,	50,		75,		10,		1,		30,		40,		2,		25,		20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{12,128,	100,	50,		75,		10,		1,		30,		40,		2,		70,		20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{13,50,		 50,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{14,70,		 70,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{15,90,		 90,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{16,120,	120,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{17,180,	180,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{18,255,	255,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{19,200,	 75,	90,		90,		10,		1,		50,		90,		2,		100,	20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{20,255,	 75,	97,		90,		10,		1,		50,		90,		1,		40,		50,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{21,100,	100,	0,		0,		10,		1,		30,		50,		3,		15,		20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{22,160,	160,	0,		0,		10,		1,		50,		50,		3,		500,	25,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{23,255,	 75,	88,		0,		10,		1,		40,		0,		0,		0,		0,		0,		5,		0,0,0,0,0,0,0,0,0,0}, 
{24,200,	 20,	95,	    70,		10,		1,		70,		70,		3,		20,		50,		0,		0,		0,0,0,0,0,0,0,0,0,0}, 
{25,180,	100,	50,		60,		10,		1,		40,		60,		2,		90,		100,	100,	0,		0,0,0,0,0,0,0,0,0,0}, 
{26,60,		 60,	0,		0,		10,		1,		40,		70,		3,		80,		20,		50,		0,		0,0,0,0,0,0,0,0,0,0}, 
{27,128,	 90,	10,		10,		10,		1,		20,		40,		1,		5,		10,		20,		0,		0,0,0,0,0,0,0,0,0,0}
};

class CAmbientGeneric : public CPointEntity
{
public:
	DECLARE_CLASS( CAmbientGeneric, CPointEntity );

	CAmbientGeneric();

	bool KeyValue( const char *szKeyName, const char *szValue );
	void Spawn( void );
	void Precache( void );
	void Activate( void );
	void RampThink( void );
	void InitModulationParms(void);
	void ComputeMaxAudibleDistance( );

	// Rules about which entities need to transmit along with me
	virtual void SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );
	virtual void UpdateOnRemove( void );

	void ToggleSound();
	void SendSound( SoundFlags_t flags );

	// Input handlers
	void InputPlaySound( inputdata_t &inputdata );
	void InputStopSound( inputdata_t &inputdata );
	void InputToggleSound( inputdata_t &inputdata );
	void InputPitch( inputdata_t &inputdata );
	void InputVolume( inputdata_t &inputdata );
	void InputFadeIn( inputdata_t &inputdata );
	void InputFadeOut( inputdata_t &inputdata );

	DECLARE_DATADESC();

	float m_radius;
	float m_flMaxRadius;
	soundlevel_t m_iSoundLevel;		// dB value
	dynpitchvol_t m_dpv;	

	bool m_fActive;		// only true when the entity is playing a looping sound
	bool m_fLooping;		// true when the sound played will loop

	char m_szSoundFile[MAX_PATH];			// Path/filename of WAV or MP3 file to play.
	string_t m_sSourceEntName;
	EHANDLE m_hSoundSource;	// entity from which the sound comes
	int		m_nSoundSourceEntIndex; // In case the entity goes away before we finish stopping the sound...

private:
	void ValidateSoundFile( void );
	string_t m_iszSound;
	string_t m_iszPrevSound; // track if the sound has changed and we need to re-Validate
};

LINK_ENTITY_TO_CLASS( ambient_generic, CAmbientGeneric );

BEGIN_DATADESC( CAmbientGeneric )

	DEFINE_KEYFIELD( m_iszSound, FIELD_SOUNDNAME, "message" ),
	DEFINE_KEYFIELD( m_radius,			FIELD_FLOAT, "radius" ),
	DEFINE_KEYFIELD( m_sSourceEntName,	FIELD_STRING, "SourceEntityName" ),
	// recomputed in Activate()
	// DEFINE_FIELD( m_hSoundSource, EHANDLE ),
	// DEFINE_FIELD( m_nSoundSourceEntIndex, FIELD_INTERGER ),

	DEFINE_FIELD( m_flMaxRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_fActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fLooping, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iSoundLevel, FIELD_INTEGER ),

	// HACKHACK - This is not really in the spirit of the save/restore design, but save this
	// out as a binary data block.  If the dynpitchvol_t is changed, old saved games will NOT
	// load these correctly, so bump the save/restore version if you change the size of the struct
	// The right way to do this is to split the input parms (read in keyvalue) into members and re-init this
	// struct in Precache(), but it's unlikely that the struct will change, so it's not worth the time right now.
	DEFINE_ARRAY( m_dpv, FIELD_CHARACTER, sizeof(dynpitchvol_t) ),

	// Function Pointers
	DEFINE_FUNCTION( RampThink ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "PlaySound", InputPlaySound ),
	DEFINE_INPUTFUNC(FIELD_VOID, "StopSound", InputStopSound ),
	DEFINE_INPUTFUNC(FIELD_VOID, "ToggleSound", InputToggleSound ),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "Pitch", InputPitch ),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "Volume", InputVolume ),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "FadeIn", InputFadeIn ),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "FadeOut", InputFadeOut ),

END_DATADESC()


#define SF_AMBIENT_SOUND_EVERYWHERE			1
#define SF_AMBIENT_SOUND_START_SILENT		16
#define SF_AMBIENT_SOUND_NOT_LOOPING		32


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
CAmbientGeneric::CAmbientGeneric()
{
	m_szSoundFile[0] = 0;
	m_iszSound = NULL_STRING;
	m_iszPrevSound = NULL_STRING;
}

//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CAmbientGeneric::Spawn( void )
{
	m_iSoundLevel = ComputeSoundlevel( m_radius, FBitSet( m_spawnflags, SF_AMBIENT_SOUND_EVERYWHERE )?true:false );
	ComputeMaxAudibleDistance( );

	ValidateSoundFile();

	if ( V_strlen( m_szSoundFile ) < 1 )
	{
		Warning( "Empty %s (%s) at %.2f, %.2f, %.2f\n", GetClassname(), GetDebugName(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		UTIL_Remove(this);
		return;
	}

    SetSolid( SOLID_NONE );
    SetMoveType( MOVETYPE_NONE );

	// Set up think function for dynamic modification 
	// of ambient sound's pitch or volume. Don't
	// start thinking yet.

	SetThink(&CAmbientGeneric::RampThink);
	SetNextThink( TICK_NEVER_THINK );

	m_fActive = false;

	if ( FBitSet ( m_spawnflags, SF_AMBIENT_SOUND_NOT_LOOPING ) )
	{
		m_fLooping = false;
	}
	else
	{
		m_fLooping = true;
	}

	m_hSoundSource = NULL;
	m_nSoundSourceEntIndex = -1;

	Precache( );

	// init all dynamic modulation parms
	InitModulationParms();
}


//-----------------------------------------------------------------------------
// Computes the max audible radius for a given sound level
//-----------------------------------------------------------------------------
#define MIN_AUDIBLE_VOLUME 1.01e-3

void CAmbientGeneric::ComputeMaxAudibleDistance( )
{
	if (( m_iSoundLevel == SNDLVL_NONE )	|| ( m_radius == 0.0f ))
	{
		m_flMaxRadius = -1.0f;
		return;
	}

	// Sadly, there's no direct way of getting at this. 
	// We have to do an interative computation.
	float flGain = enginesound->GetDistGainFromSoundLevel( m_iSoundLevel, m_radius );
	if ( flGain <= MIN_AUDIBLE_VOLUME )
	{
		m_flMaxRadius = m_radius;
		return;
	}

	float flMinRadius = m_radius; 
	float flMaxRadius = m_radius * 2;
	while ( true )
	{
		// First, find a min + max range surrounding the desired distance gain
		float flGain = enginesound->GetDistGainFromSoundLevel( m_iSoundLevel, flMaxRadius );
		if ( flGain <= MIN_AUDIBLE_VOLUME )
			break;

		// Always audible.
		if ( flMaxRadius > 1e5 )
		{
			m_flMaxRadius = -1.0f;
			return;
		}

		flMinRadius = flMaxRadius;
		flMaxRadius *= 2.0f;
	}

	// Now home in a little bit
	int nInterations = 4;
	while ( --nInterations >= 0 )
	{
		float flTestRadius = (flMinRadius + flMaxRadius) * 0.5f;
		float flGain = enginesound->GetDistGainFromSoundLevel( m_iSoundLevel, flTestRadius );
		if ( flGain <= MIN_AUDIBLE_VOLUME )
		{
			flMaxRadius = flTestRadius;
		}
		else
		{
			flMinRadius = flTestRadius;
		}
	}

	m_flMaxRadius = flMaxRadius;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for changing pitch.
// Input  : Float new pitch from 0 - 255 (100 = as recorded).
//-----------------------------------------------------------------------------
void CAmbientGeneric::InputPitch( inputdata_t &inputdata )
{
	m_dpv.pitch = clamp( FastFloatToSmallInt( inputdata.value.Float() ), 0, 255 );

	SendSound( SND_CHANGE_PITCH );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for changing volume.
// Input  : Float new volume, from 0 - 10.
//-----------------------------------------------------------------------------
void CAmbientGeneric::InputVolume( inputdata_t &inputdata )
{
	//
	// Multiply the input value by ten since volumes are expected to be from 0 - 100.
	//
	m_dpv.vol = clamp( RoundFloatToInt( inputdata.value.Float() * 10.f ), 0, 100 );
	m_dpv.volfrac = m_dpv.vol << 8;

	SendSound( SND_CHANGE_VOL );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for fading in volume over time.
// Input  : Float volume fade in time 0 - 100 seconds
//-----------------------------------------------------------------------------
void CAmbientGeneric::InputFadeIn( inputdata_t &inputdata )
{
	// cancel any fade out that might be happening
	m_dpv.fadeout = 0;

	m_dpv.fadein = inputdata.value.Float();
	if (m_dpv.fadein > 100) m_dpv.fadein = 100;
	if (m_dpv.fadein < 0) m_dpv.fadein = 0;

	if (m_dpv.fadein > 0)
		m_dpv.fadein = ( 100 << 8 ) / ( m_dpv.fadein * AMBIENT_GENERIC_UPDATE_RATE );

	SetNextThink( gpGlobals->curtime + 0.1f );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for fading out volume over time.
// Input  : Float volume fade out time 0 - 100 seconds
//-----------------------------------------------------------------------------
void CAmbientGeneric::InputFadeOut( inputdata_t &inputdata )
{
	// cancel any fade in that might be happening
	m_dpv.fadein = 0;

	m_dpv.fadeout = inputdata.value.Float();

	if (m_dpv.fadeout > 100) m_dpv.fadeout = 100;
	if (m_dpv.fadeout < 0) m_dpv.fadeout = 0;

	if (m_dpv.fadeout > 0)
		m_dpv.fadeout = ( 100 << 8 ) / ( m_dpv.fadeout * AMBIENT_GENERIC_UPDATE_RATE );

	SetNextThink( gpGlobals->curtime + 0.1f );
}


void CAmbientGeneric::ValidateSoundFile( void )
{
	if ( m_iszSound == NULL_STRING )
	{
		m_iszPrevSound = NULL_STRING;
		m_szSoundFile[0] = 0;
		return;
	}

	if ( m_iszSound == m_iszPrevSound )
		return;

	m_iszPrevSound = m_iszSound;
	
	char *szSoundFile = (char *)STRING( m_iszSound );
	if ( V_strlen( szSoundFile ) < 1 )
		return;

	char szPathTest[MAX_PATH] = { 0 };
	char szNewSoundFile[MAX_PATH] = { 0 };

	// try to fix some legacy ambient generic entities that still use .wav file references for vo sounds instead of .mp3
	if ( V_strncmp( szSoundFile, "vo", 2 ) == 0 )
	{
		const char *pszExt = V_GetFileExtension( szSoundFile );
		if ( pszExt && pszExt[0] && FStrEq( pszExt, "wav" ) )
		{
			V_sprintf_safe( szPathTest, "sound/%s", szSoundFile );
			if ( !g_pFullFileSystem->FileExists( szPathTest ) )
			{
				V_strcpy_safe( szNewSoundFile, szSoundFile );
				V_SetExtension( szNewSoundFile, ".mp3", ARRAYSIZE( szNewSoundFile ) );
				V_sprintf_safe( szPathTest, "sound/%s", szNewSoundFile );
				if ( g_pFullFileSystem->FileExists( szPathTest ) )
				{
					szSoundFile = szNewSoundFile;
				}
			}
		}
	}

	V_strcpy_safe( m_szSoundFile, szSoundFile );
}


void CAmbientGeneric::Precache( void )
{
	if ( V_strlen( m_szSoundFile ) > 1 )
	{
		if ( m_szSoundFile[0] != '!' )
		{
			PrecacheScriptSound( m_szSoundFile );
		}
	}

	if ( !FBitSet (m_spawnflags, SF_AMBIENT_SOUND_START_SILENT ) )
	{
		// start the sound ASAP
		if (m_fLooping)
			m_fActive = true;
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CAmbientGeneric::Activate( void )
{
	BaseClass::Activate();

	// Initialize sound source.  If no source was given, or source can't be found
	// then this is the source
	if (m_hSoundSource == NULL)
	{
		if (m_sSourceEntName != NULL_STRING)
		{
			m_hSoundSource = gEntList.FindEntityByName( NULL, m_sSourceEntName );
			if ( m_hSoundSource != NULL )
			{
				m_nSoundSourceEntIndex = m_hSoundSource->entindex();
			}
		}

		if (m_hSoundSource == NULL)
		{
			m_hSoundSource = this;
			m_nSoundSourceEntIndex = entindex();
		}
		else
		{
			if ( !FBitSet( m_spawnflags, SF_AMBIENT_SOUND_EVERYWHERE ) )
			{
				AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
			}
		}
	}

#ifdef PORTAL
		// This is the only way we can silence the radio sound from the first room without touching them map -- jdw
		if ( PortalGameRules() && PortalGameRules()->ShouldRemoveRadio() )
		{		
			if ( V_strcmp( STRING( gpGlobals->mapname ), "testchmb_a_00" ) == 0 || 
			    V_strcmp( STRING( gpGlobals->mapname ), "testchmb_a_11" ) == 0 || 
			    V_strcmp( STRING( gpGlobals->mapname ), "testchmb_a_14" ) == 0 )
			{
				if ( V_strcmp( STRING( GetEntityName() ), "radio_sound" ) == 0 )
				{
					UTIL_Remove( this );
					return;
				}
			}
		}
#endif // PORTAL

	// If active start the sound
	if ( m_fActive )
	{
		int flags = SND_SPAWNING;
		// If we are loading a saved game, we can't write into the init/signon buffer here, so just issue
		//  as a regular sound message...
		if ( gpGlobals->eLoadType == MapLoad_Transition ||
			 gpGlobals->eLoadType == MapLoad_LoadGame || 
			 g_pGameRules->InRoundRestart() )
		{
			flags = SND_NOFLAGS;
		}
	
		// Tracker 76119:  8/12/07 ywb: 
		//  Make sure pitch and volume are set up to the correct value (especially after restoring a .sav file)
		flags |= ( SND_CHANGE_PITCH | SND_CHANGE_VOL );  

		// Don't bother sending over to client if volume is zero, though
		if ( m_dpv.vol > 0 )
		{
			SendSound( (SoundFlags_t)flags );
		}

		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}


//-----------------------------------------------------------------------------
// Rules about which entities need to transmit along with me
//-----------------------------------------------------------------------------
void CAmbientGeneric::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Ambient generics never transmit; this is just a way for us to ensure
	// the sound source gets transmitted; that's why we don't call pInfo->m_pTransmitEdict->Set
	if ( !m_hSoundSource || m_hSoundSource == this || !m_fActive )
		return;

	// Don't bother sending the position of the source if we have to play everywhere
	if ( FBitSet( m_spawnflags, SF_AMBIENT_SOUND_EVERYWHERE ) )
		return;

	Assert( pInfo->m_pClientEnt );
	CBaseEntity *pClient = (CBaseEntity*)(pInfo->m_pClientEnt->GetUnknown());
	if ( !pClient )
		return;

	// Send the sound source if he's close enough
	if ( ( m_flMaxRadius < 0 ) || ( pClient->GetAbsOrigin().DistToSqr( m_hSoundSource->GetAbsOrigin() ) <= m_flMaxRadius * m_flMaxRadius ) )
	{
		m_hSoundSource->SetTransmit( pInfo, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAmbientGeneric::UpdateOnRemove( void )
{
	if ( m_fActive )
	{
		// Stop the sound we're generating
		SendSound( SND_STOP );
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: Think at 5hz if we are dynamically modifying pitch or volume of the
//			playing sound.  This function will ramp pitch and/or volume up or
//			down, modify pitch/volume with lfo if active.
//-----------------------------------------------------------------------------
void CAmbientGeneric::RampThink( void )
{
	int pitch = m_dpv.pitch; 
	int vol = m_dpv.vol;
	int flags = 0;
	int fChanged = 0;		// false if pitch and vol remain unchanged this round
	int	prev;

	if (!m_dpv.spinup && !m_dpv.spindown && !m_dpv.fadein && !m_dpv.fadeout && !m_dpv.lfotype)
		return;						// no ramps or lfo, stop thinking

	// ==============
	// pitch envelope
	// ==============
	if (m_dpv.spinup || m_dpv.spindown)
	{
		prev = m_dpv.pitchfrac >> 8;

		if (m_dpv.spinup > 0)
			m_dpv.pitchfrac += m_dpv.spinup;
		else if (m_dpv.spindown > 0)
			m_dpv.pitchfrac -= m_dpv.spindown;

		pitch = m_dpv.pitchfrac >> 8;
		
		if (pitch > m_dpv.pitchrun)
		{
			pitch = m_dpv.pitchrun;
			m_dpv.spinup = 0;				// done with ramp up
		}

		if (pitch < m_dpv.pitchstart)
		{
			pitch = m_dpv.pitchstart;
			m_dpv.spindown = 0;				// done with ramp down

			// shut sound off
			SendSound( SND_STOP );
			
			// return without setting m_flNextThink
			return;
		}

		if (pitch > 255) pitch = 255;
		if (pitch < 1) pitch = 1;

		m_dpv.pitch = pitch;

		fChanged |= (prev != pitch);
		flags |= SND_CHANGE_PITCH;
	}

	// ==================
	// amplitude envelope
	// ==================
	if (m_dpv.fadein || m_dpv.fadeout)
	{
		prev = m_dpv.volfrac >> 8;

		if (m_dpv.fadein > 0)
			m_dpv.volfrac += m_dpv.fadein;
		else if (m_dpv.fadeout > 0)
			m_dpv.volfrac -= m_dpv.fadeout;

		vol = m_dpv.volfrac >> 8;

		if (vol > m_dpv.volrun)
		{
			vol = m_dpv.volrun;
			m_dpv.volfrac = vol << 8;
			m_dpv.fadein = 0;				// done with ramp up
		}

		if (vol < m_dpv.volstart)
		{
			vol = m_dpv.volstart;
			m_dpv.vol = vol;
			m_dpv.volfrac = vol << 8;
			m_dpv.fadeout = 0;				// done with ramp down
			
			// shut sound off
			SendSound( SND_STOP );
			
			// return without setting m_flNextThink
			return;
		}

		if (vol > 100) 
		{
			vol = 100;
			m_dpv.volfrac = vol << 8;
		}
		if (vol < 1) 
		{
			vol = 1;
			m_dpv.volfrac = vol << 8;
		}

		m_dpv.vol = vol;

		fChanged |= (prev != vol);
		flags |= SND_CHANGE_VOL;
	}

	// ===================
	// pitch/amplitude LFO
	// ===================
	if (m_dpv.lfotype)
	{
		int pos;

		if (m_dpv.lfofrac > 0x6fffffff)
			m_dpv.lfofrac = 0;

		// update lfo, lfofrac/255 makes a triangle wave 0-255
		m_dpv.lfofrac += m_dpv.lforate;
		pos = m_dpv.lfofrac >> 8;

		if (m_dpv.lfofrac < 0)
		{
			m_dpv.lfofrac = 0;
			m_dpv.lforate = abs(m_dpv.lforate);
			pos = 0;
		}
		else if (pos > 255)
		{
			pos = 255;
			m_dpv.lfofrac = (255 << 8);
			m_dpv.lforate = -abs(m_dpv.lforate);
		}

		switch(m_dpv.lfotype)
		{
		case LFO_SQUARE:
			if (pos < 128)
				m_dpv.lfomult = 255;
			else
				m_dpv.lfomult = 0;
			
			break;
		case LFO_RANDOM:
			if (pos == 255)
				m_dpv.lfomult = random->RandomInt(0, 255);
			break;
		case LFO_TRIANGLE:
		default: 
			m_dpv.lfomult = pos;
			break;
		}

		if (m_dpv.lfomodpitch)
		{
			prev = pitch;

			// pitch 0-255
			pitch += ((m_dpv.lfomult - 128) * m_dpv.lfomodpitch) / 100;

			if (pitch > 255) pitch = 255;
			if (pitch < 1) pitch = 1;

			
			fChanged |= (prev != pitch);
			flags |= SND_CHANGE_PITCH;
		}

		if (m_dpv.lfomodvol)
		{
			// vol 0-100
			prev = vol;

			vol += ((m_dpv.lfomult - 128) * m_dpv.lfomodvol) / 100;

			if (vol > 100) vol = 100;
			if (vol < 0) vol = 0;
			
			fChanged |= (prev != vol);
			flags |= SND_CHANGE_VOL;
		}

	}

	// Send update to playing sound only if we actually changed
	// pitch or volume in this routine.

	if (flags && fChanged) 
	{
		if (pitch == PITCH_NORM)
			pitch = PITCH_NORM + 1; // don't send 'no pitch' !

		CBaseEntity* pSoundSource = m_hSoundSource;
		if (pSoundSource)
		{
			ValidateSoundFile();

			UTIL_EmitAmbientSound(pSoundSource->GetSoundSourceIndex(), pSoundSource->GetAbsOrigin(), 
				m_szSoundFile, ( vol * 0.01 ), m_iSoundLevel, flags, pitch );
		}
	}

	// update ramps at 5hz
	SetNextThink( gpGlobals->curtime + AMBIENT_GENERIC_THINK_DELAY );
	return;
}


//-----------------------------------------------------------------------------
// Purpose: Init all ramp params in preparation to play a new sound.
//-----------------------------------------------------------------------------
void CAmbientGeneric::InitModulationParms(void)
{
	int pitchinc;

	m_dpv.volrun = m_iHealth * 10;	// 0 - 100
	if (m_dpv.volrun > 100) m_dpv.volrun = 100;
	if (m_dpv.volrun < 0) m_dpv.volrun = 0;

	// get presets
	if (m_dpv.preset != 0 && m_dpv.preset <= CDPVPRESETMAX)
	{
		// load preset values
		m_dpv = rgdpvpreset[m_dpv.preset - 1];

		// fixup preset values, just like
		// fixups in KeyValue routine.
		if (m_dpv.spindown > 0)
			m_dpv.spindown = (101 - m_dpv.spindown) * 64;
		if (m_dpv.spinup > 0)
			m_dpv.spinup = (101 - m_dpv.spinup) * 64;

		m_dpv.volstart *= 10;
		m_dpv.volrun *= 10;

		if (m_dpv.fadein > 0)
			m_dpv.fadein = (101 - m_dpv.fadein) * 64;
		if (m_dpv.fadeout > 0)
			m_dpv.fadeout = (101 - m_dpv.fadeout) * 64;

		m_dpv.lforate *= 256;

		m_dpv.fadeinsav = m_dpv.fadein;
		m_dpv.fadeoutsav = m_dpv.fadeout;
		m_dpv.spinupsav = m_dpv.spinup;
		m_dpv.spindownsav = m_dpv.spindown;
	}

	m_dpv.fadein = m_dpv.fadeinsav;
	m_dpv.fadeout = 0; 
	
	if (m_dpv.fadein)
		m_dpv.vol = m_dpv.volstart;
	else
		m_dpv.vol = m_dpv.volrun;

	m_dpv.spinup = m_dpv.spinupsav;
	m_dpv.spindown = 0; 

	if (m_dpv.spinup)
		m_dpv.pitch = m_dpv.pitchstart;
	else
		m_dpv.pitch = m_dpv.pitchrun;

	if (m_dpv.pitch == 0)
		m_dpv.pitch = PITCH_NORM;

	m_dpv.pitchfrac = m_dpv.pitch << 8;
	m_dpv.volfrac = m_dpv.vol << 8;

	m_dpv.lfofrac = 0;
	m_dpv.lforate = abs(m_dpv.lforate);

	m_dpv.cspincount = 1;
	
	if (m_dpv.cspinup) 
	{
		pitchinc = (255 - m_dpv.pitchstart) / m_dpv.cspinup;

		m_dpv.pitchrun = m_dpv.pitchstart + pitchinc;
		if (m_dpv.pitchrun > 255) m_dpv.pitchrun = 255;
	}

	if ((m_dpv.spinupsav || m_dpv.spindownsav || (m_dpv.lfotype && m_dpv.lfomodpitch))
		&& (m_dpv.pitch == PITCH_NORM))
		m_dpv.pitch = PITCH_NORM + 1; // must never send 'no pitch' as first pitch
									  // if we intend to pitch shift later!
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that begins playing the sound.
//-----------------------------------------------------------------------------
void CAmbientGeneric::InputPlaySound( inputdata_t &inputdata )
{
	if (!m_fActive)
	{
		//Adrian: Stop our current sound before starting a new one!
		SendSound( SND_STOP ); 
		
		ToggleSound();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that stops playing the sound.
//-----------------------------------------------------------------------------
void CAmbientGeneric::InputStopSound( inputdata_t &inputdata )
{
	if (m_fActive)
	{
		ToggleSound();
	}
}

void CAmbientGeneric::SendSound( SoundFlags_t flags)
{
	ValidateSoundFile();

	CBaseEntity* pSoundSource = m_hSoundSource;
	if ( pSoundSource )
	{
		if ( flags == SND_STOP )
		{
			UTIL_EmitAmbientSound( pSoundSource->GetSoundSourceIndex(), pSoundSource->GetAbsOrigin(), m_szSoundFile,
						0, SNDLVL_NONE, flags, 0);
		}
		else
		{
			UTIL_EmitAmbientSound( pSoundSource->GetSoundSourceIndex(), pSoundSource->GetAbsOrigin(), m_szSoundFile,
				(m_dpv.vol * 0.01), m_iSoundLevel, flags, m_dpv.pitch);
		}
	}	
	else
	{
		if ( ( flags == SND_STOP ) && 
			( m_nSoundSourceEntIndex != -1 ) )
		{
			UTIL_EmitAmbientSound( m_nSoundSourceEntIndex, GetAbsOrigin(), m_szSoundFile,
					0, SNDLVL_NONE, flags, 0);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that stops playing the sound.
//-----------------------------------------------------------------------------
void CAmbientGeneric::InputToggleSound( inputdata_t &inputdata )
{
	ToggleSound();
}


//-----------------------------------------------------------------------------
// Purpose: Turns an ambient sound on or off.  If the ambient is a looping sound,
//			mark sound as active (m_fActive) if it's playing, innactive if not.
//			If the sound is not a looping sound, never mark it as active.
// Input  : pActivator - 
//			pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CAmbientGeneric::ToggleSound()
{
	// m_fActive is true only if a looping sound is playing.
	
	if ( m_fActive )
	{// turn sound off

		if (m_dpv.cspinup)
		{
			// Don't actually shut off. Each toggle causes
			// incremental spinup to max pitch

			if (m_dpv.cspincount <= m_dpv.cspinup)
			{	
				int pitchinc;

				// start a new spinup
				m_dpv.cspincount++;
				
				pitchinc = (255 - m_dpv.pitchstart) / m_dpv.cspinup;

				m_dpv.spinup = m_dpv.spinupsav;
				m_dpv.spindown = 0;

				m_dpv.pitchrun = m_dpv.pitchstart + pitchinc * m_dpv.cspincount;
				if (m_dpv.pitchrun > 255) m_dpv.pitchrun = 255;

				SetNextThink( gpGlobals->curtime + 0.1f );
			}
			
		}
		else
		{
			m_fActive = false;
			
			// HACKHACK - this makes the code in Precache() work properly after a save/restore
			m_spawnflags |= SF_AMBIENT_SOUND_START_SILENT;

			if (m_dpv.spindownsav || m_dpv.fadeoutsav)
			{
				// spin it down (or fade it) before shutoff if spindown is set
				m_dpv.spindown = m_dpv.spindownsav;
				m_dpv.spinup = 0;

				m_dpv.fadeout = m_dpv.fadeoutsav;
				m_dpv.fadein = 0;
				SetNextThink( gpGlobals->curtime + 0.1f );
			}
			else
			{
				SendSound( SND_STOP ); // stop sound
			}
		}
	}
	else 
	{// turn sound on

		// only toggle if this is a looping sound.  If not looping, each
		// trigger will cause the sound to play.  If the sound is still
		// playing from a previous trigger press, it will be shut off
		// and then restarted.

		if (m_fLooping)
			m_fActive = true;
		else
		{
			// shut sound off now - may be interrupting a long non-looping sound
			SendSound( SND_STOP ); // stop sound
		}
			
		// init all ramp params for startup

		InitModulationParms();

		SendSound( SND_NOFLAGS ); // send sound

		SetNextThink( gpGlobals->curtime + 0.1f );

	} 
}


// KeyValue - load keyvalue pairs into member data of the
// ambient generic. NOTE: called BEFORE spawn!
bool CAmbientGeneric::KeyValue( const char *szKeyName, const char *szValue )
{
	// NOTE: changing any of the modifiers in this code
	// NOTE: also requires changing InitModulationParms code.

	// preset
	if (FStrEq(szKeyName, "preset"))
	{
		m_dpv.preset = atoi(szValue);
	}
	// pitchrun
	else if (FStrEq(szKeyName, "pitch"))
	{
		m_dpv.pitchrun = atoi(szValue);
		
		if (m_dpv.pitchrun > 255) m_dpv.pitchrun = 255;
		if (m_dpv.pitchrun < 0) m_dpv.pitchrun = 0;
	}		
	// pitchstart
	else if (FStrEq(szKeyName, "pitchstart"))
	{
		m_dpv.pitchstart = atoi(szValue);
		
		if (m_dpv.pitchstart > 255) m_dpv.pitchstart = 255;
		if (m_dpv.pitchstart < 0) m_dpv.pitchstart = 0;
	}
	// spinup
	else if (FStrEq(szKeyName, "spinup"))
	{
		m_dpv.spinup = atoi(szValue);
		
		if (m_dpv.spinup > 100) m_dpv.spinup = 100;
		if (m_dpv.spinup < 0) m_dpv.spinup = 0;

		if (m_dpv.spinup > 0)
			m_dpv.spinup = (101 - m_dpv.spinup) * 64;
		m_dpv.spinupsav = m_dpv.spinup;
	}		
	// spindown
	else if (FStrEq(szKeyName, "spindown"))
	{
		m_dpv.spindown = atoi(szValue);
		
		if (m_dpv.spindown > 100) m_dpv.spindown = 100;
		if (m_dpv.spindown < 0) m_dpv.spindown = 0;

		if (m_dpv.spindown > 0)
			m_dpv.spindown = (101 - m_dpv.spindown) * 64;
		m_dpv.spindownsav = m_dpv.spindown;
	}
	// volstart
	else if (FStrEq(szKeyName, "volstart"))
	{
		m_dpv.volstart = atoi(szValue);

		if (m_dpv.volstart > 10) m_dpv.volstart = 10;
		if (m_dpv.volstart < 0) m_dpv.volstart = 0;
		
		m_dpv.volstart *= 10;	// 0 - 100
	}
	// legacy fadein
	else if (FStrEq(szKeyName, "fadein"))
	{
		m_dpv.fadein = atoi(szValue);
		
		if (m_dpv.fadein > 100) m_dpv.fadein = 100;
		if (m_dpv.fadein < 0) m_dpv.fadein = 0;

		if (m_dpv.fadein > 0)
			m_dpv.fadein = (101 - m_dpv.fadein) * 64;
		m_dpv.fadeinsav = m_dpv.fadein;
	}
	// legacy fadeout
	else if (FStrEq(szKeyName, "fadeout"))
	{
		m_dpv.fadeout = atoi(szValue);
		
		if (m_dpv.fadeout > 100) m_dpv.fadeout = 100;
		if (m_dpv.fadeout < 0) m_dpv.fadeout = 0;

		if (m_dpv.fadeout > 0)
			m_dpv.fadeout = (101 - m_dpv.fadeout) * 64;
		m_dpv.fadeoutsav = m_dpv.fadeout;
	}
	// fadeinsecs
	else if (FStrEq(szKeyName, "fadeinsecs"))
	{
		m_dpv.fadein = atoi(szValue);

		if (m_dpv.fadein > 100) m_dpv.fadein = 100;
		if (m_dpv.fadein < 0) m_dpv.fadein = 0;

		if (m_dpv.fadein > 0)
			m_dpv.fadein = ( 100 << 8 ) / ( m_dpv.fadein * AMBIENT_GENERIC_UPDATE_RATE );
		m_dpv.fadeinsav = m_dpv.fadein;
	}
	// fadeoutsecs
	else if (FStrEq(szKeyName, "fadeoutsecs"))
	{
		m_dpv.fadeout = atoi(szValue);

		if (m_dpv.fadeout > 100) m_dpv.fadeout = 100;
		if (m_dpv.fadeout < 0) m_dpv.fadeout = 0;

		if (m_dpv.fadeout > 0)
			m_dpv.fadeout = ( 100 << 8 ) / ( m_dpv.fadeout * AMBIENT_GENERIC_UPDATE_RATE );
		m_dpv.fadeoutsav = m_dpv.fadeout;
	}
	// lfotype
	else if (FStrEq(szKeyName, "lfotype"))
	{
		m_dpv.lfotype = atoi(szValue);
		if (m_dpv.lfotype > 4) m_dpv.lfotype = LFO_TRIANGLE;
	}
	// lforate
	else if (FStrEq(szKeyName, "lforate"))
	{
		m_dpv.lforate = atoi(szValue);
		
		if (m_dpv.lforate > 1000) m_dpv.lforate = 1000;
		if (m_dpv.lforate < 0) m_dpv.lforate = 0;

		m_dpv.lforate *= 256;
	}
	// lfomodpitch
	else if (FStrEq(szKeyName, "lfomodpitch"))
	{
		m_dpv.lfomodpitch = atoi(szValue);
		if (m_dpv.lfomodpitch > 100) m_dpv.lfomodpitch = 100;
		if (m_dpv.lfomodpitch < 0) m_dpv.lfomodpitch = 0;
	}

	// lfomodvol
	else if (FStrEq(szKeyName, "lfomodvol"))
	{
		m_dpv.lfomodvol = atoi(szValue);
		if (m_dpv.lfomodvol > 100) m_dpv.lfomodvol = 100;
		if (m_dpv.lfomodvol < 0) m_dpv.lfomodvol = 0;
	}
	// cspinup
	else if (FStrEq(szKeyName, "cspinup"))
	{
		m_dpv.cspinup = atoi(szValue);
		if (m_dpv.cspinup > 100) m_dpv.cspinup = 100;
		if (m_dpv.cspinup < 0) m_dpv.cspinup = 0;
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}


// =================== ROOM SOUND FX ==========================================




// ==================== SENTENCE GROUPS, UTILITY FUNCTIONS  ======================================

int fSentencesInit = false;

// ===================== SENTENCE GROUPS, MAIN ROUTINES ========================

// given sentence group index, play random sentence for given entity.
// returns sentenceIndex - which sentence was picked 
// Ipick is only needed if you plan on stopping the sound before playback is done (see SENTENCEG_Stop). 
// sentenceIndex can be used to find the name/length of the sentence

int SENTENCEG_PlayRndI(edict_t *entity, int isentenceg, 
					  float volume, soundlevel_t soundlevel, int flags, int pitch)
{
	char name[64];
	int ipick;

	if (!fSentencesInit)
		return -1;

	name[0] = 0;

	ipick = engine->SentenceGroupPick( isentenceg, name, sizeof( name ) );
	if ( ( ipick > 0 ) && name[0] )
	{
		int sentenceIndex = SENTENCEG_Lookup( name );
		CPASAttenuationFilter filter( GetContainingEntity( entity ), soundlevel );
		CBaseEntity::EmitSentenceByIndex( filter, ENTINDEX(entity), CHAN_VOICE, sentenceIndex, volume, soundlevel, flags, pitch );
		return sentenceIndex;
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Picks a sentence, but doesn't play it
//-----------------------------------------------------------------------------
int SENTENCEG_PickRndSz(const char *szgroupname)
{
	char name[64];
	int ipick;
	int isentenceg;

	if (!fSentencesInit)
		return -1;

	name[0] = 0;

	isentenceg = engine->SentenceGroupIndexFromName(szgroupname);
	if (isentenceg < 0)
	{
		Warning( "No such sentence group %s\n", szgroupname );
		return -1;
	}

	ipick = engine->SentenceGroupPick(isentenceg, name, sizeof( name ));
	if (ipick >= 0 && name[0])
	{
		return SENTENCEG_Lookup( name );
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Plays a sentence by sentence index
//-----------------------------------------------------------------------------
void SENTENCEG_PlaySentenceIndex( edict_t *entity, int iSentenceIndex, float volume, soundlevel_t soundlevel, int flags, int pitch )
{
	if ( iSentenceIndex >= 0 )
	{
		CPASAttenuationFilter filter( GetContainingEntity( entity ), soundlevel );
		CBaseEntity::EmitSentenceByIndex( filter, ENTINDEX(entity), CHAN_VOICE, iSentenceIndex, volume, soundlevel, flags, pitch );
	}
}


int SENTENCEG_PlayRndSz(edict_t *entity, const char *szgroupname, 
					  float volume, soundlevel_t soundlevel, int flags, int pitch)
{
	char name[64];
	int ipick;
	int isentenceg;

	if (!fSentencesInit)
		return -1;

	name[0] = 0;

	isentenceg = engine->SentenceGroupIndexFromName(szgroupname);
	if (isentenceg < 0)
	{
		Warning( "No such sentence group %s\n", szgroupname );
		return -1;
	}

	ipick = engine->SentenceGroupPick(isentenceg, name, sizeof( name ));
	if (ipick >= 0 && name[0])
	{
		int sentenceIndex = SENTENCEG_Lookup( name );
		CPASAttenuationFilter filter( GetContainingEntity( entity ), soundlevel );
		CBaseEntity::EmitSentenceByIndex( filter, ENTINDEX(entity), CHAN_VOICE, sentenceIndex, volume, soundlevel, flags, pitch );
		return sentenceIndex;
	}

	return -1;
}

// play sentences in sequential order from sentence group.  Reset after last sentence.

int SENTENCEG_PlaySequentialSz(edict_t *entity, const char *szgroupname, 
					  float volume, soundlevel_t soundlevel, int flags, int pitch, int ipick, int freset)
{
	char name[64];
	int ipicknext;
	int isentenceg;

	if (!fSentencesInit)
		return -1;

	name[0] = 0;

	isentenceg = engine->SentenceGroupIndexFromName(szgroupname);
	if (isentenceg < 0)
		return -1;

	ipicknext = engine->SentenceGroupPickSequential(isentenceg, name, sizeof( name ), ipick, freset);
	if (ipicknext >= 0 && name[0])
	{
		int sentenceIndex = SENTENCEG_Lookup( name );
		CPASAttenuationFilter filter( GetContainingEntity( entity ), soundlevel );
		CBaseEntity::EmitSentenceByIndex( filter, ENTINDEX(entity), CHAN_VOICE, sentenceIndex, volume, soundlevel, flags, pitch );
		return sentenceIndex;
	}
	
	return -1;
}


#if 0
// for this entity, for the given sentence within the sentence group, stop
// the sentence.

void SENTENCEG_Stop(edict_t *entity, int isentenceg, int ipick)
{
	char buffer[64];
	char sznum[8];
	
	if (!fSentencesInit)
		return;

	if (isentenceg < 0 || ipick < 0)
		return;

	Q_snprintf(buffer,sizeof(buffer),"!%s%d", engine->SentenceGroupNameFromIndex( isentenceg ), ipick );

	UTIL_StopSound(entity, CHAN_VOICE, buffer);
}
#endif

// open sentences.txt, scan for groups, build rgsentenceg
// Should be called from world spawn, only works on the
// first call and is ignored subsequently.
void SENTENCEG_Init()
{
	if (fSentencesInit)
		return;

	engine->PrecacheSentenceFile( "scripts/sentences.txt" );
	fSentencesInit = true;
}

// convert sentence (sample) name to !sentencenum, return !sentencenum

int SENTENCEG_Lookup(const char *sample)
{
	return engine->SentenceIndexFromName( sample + 1 );
}


int SENTENCEG_GetIndex(const char *szrootname)
{
	return engine->SentenceGroupIndexFromName( szrootname );
}

void UTIL_RestartAmbientSounds( void )
{
	CAmbientGeneric *pAmbient = NULL;
	while ( ( pAmbient = (CAmbientGeneric*) gEntList.FindEntityByClassname( pAmbient, "ambient_generic" ) ) != NULL )
	{
		if ( pAmbient->m_fActive )
		{
			if ( V_strstr( pAmbient->m_szSoundFile, "mp3" ) )
			{
				pAmbient->SendSound( SND_CHANGE_VOL ); // fake a change, so we don't create 2 sounds
			}
			pAmbient->SendSound( SND_CHANGE_VOL ); // fake a change, so we don't create 2 sounds
		}
	}
}


// play a specific sentence over the HEV suit speaker - just pass player entity, and !sentencename

void UTIL_EmitSoundSuit(edict_t *entity, const char *sample)
{
	float fvol;
	int pitch = PITCH_NORM;

	fvol = suitvolume.GetFloat();
	if (random->RandomInt(0,1))
		pitch = random->RandomInt(0,6) + 98;

	// If friendlies are talking, reduce the volume of the suit
	if ( !g_AIFriendliesTalkSemaphore.IsAvailable( GetContainingEntity( entity ) ) )
	{
		fvol *= 0.3;
	}

	if (fvol > 0.05)
	{
		CPASAttenuationFilter filter( GetContainingEntity( entity ) );
		filter.MakeReliable();

		EmitSound_t ep;
		ep.m_nChannel = CHAN_STATIC;
		ep.m_pSoundName = sample;
		ep.m_flVolume = fvol;
		ep.m_SoundLevel = SNDLVL_NORM;
		ep.m_nPitch = pitch;

		CBaseEntity::EmitSound( filter, ENTINDEX(entity), ep );
	}
}

// play a sentence, randomly selected from the passed in group id, over the HEV suit speaker

int UTIL_EmitGroupIDSuit(edict_t *entity, int isentenceg)
{
	float fvol;
	int pitch = PITCH_NORM;
	int sentenceIndex = -1;

	fvol = suitvolume.GetFloat();
	if (random->RandomInt(0,1))
		pitch = random->RandomInt(0,6) + 98;

	// If friendlies are talking, reduce the volume of the suit
	if ( !g_AIFriendliesTalkSemaphore.IsAvailable( GetContainingEntity( entity ) ) )
	{
		fvol *= 0.3;
	}

	if (fvol > 0.05)
		sentenceIndex = SENTENCEG_PlayRndI(entity, isentenceg, fvol, SNDLVL_NORM, 0, pitch);

	return sentenceIndex;
}

// play a sentence, randomly selected from the passed in groupname

int UTIL_EmitGroupnameSuit(edict_t *entity, const char *groupname)
{
	float fvol;
	int pitch = PITCH_NORM;
	int sentenceIndex = -1;

	fvol = suitvolume.GetFloat();
	if (random->RandomInt(0,1))
		pitch = random->RandomInt(0,6) + 98;

	// If friendlies are talking, reduce the volume of the suit
	if ( !g_AIFriendliesTalkSemaphore.IsAvailable( GetContainingEntity( entity ) ) )
	{
		fvol *= 0.3;
	}

	if (fvol > 0.05)
		sentenceIndex = SENTENCEG_PlayRndSz(entity, groupname, fvol, SNDLVL_NORM, 0, pitch);

	return sentenceIndex;
}

// ===================== MATERIAL TYPE DETECTION, MAIN ROUTINES ========================
// 
// Used to detect the texture the player is standing on, map the
// texture name to a material type.  Play footstep sound based
// on material type.

char TEXTURETYPE_Find( trace_t *ptr )
{
	const surfacedata_t *psurfaceData = physprops->GetSurfaceData( ptr->surface.surfaceProps );

	return psurfaceData->game.material;
}
