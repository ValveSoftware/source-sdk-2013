//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements an entity that measures sound volume at a point in a map.
//
//			This entity listens as though it is an NPC, meaning it will only
//			hear sounds that were emitted using the CSound::InsertSound function.
//
//			It does not hear danger sounds since they are not technically sounds.
//
//=============================================================================

#include "cbase.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"
#include "mathlib/mathlib.h"
#include "soundent.h"
#include "envmicrophone.h"
#include "soundflags.h"
#include "engine/IEngineSound.h"
#include "filters.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define DEBUG_MICROPHONE

const float MICROPHONE_SETTLE_EPSILON = 0.005;

// List of env_microphones who want to be told whenever a sound is started
static CUtlVector< CHandle<CEnvMicrophone> > s_Microphones;


LINK_ENTITY_TO_CLASS(env_microphone, CEnvMicrophone);

BEGIN_DATADESC( CEnvMicrophone )

	DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),
	DEFINE_FIELD(m_hMeasureTarget, FIELD_EHANDLE),
	DEFINE_KEYFIELD(m_nSoundMask, FIELD_INTEGER, "SoundMask"),
	DEFINE_KEYFIELD(m_flSensitivity, FIELD_FLOAT, "Sensitivity"),
	DEFINE_KEYFIELD(m_flSmoothFactor, FIELD_FLOAT, "SmoothFactor"),
	DEFINE_KEYFIELD(m_iszSpeakerName, FIELD_STRING, "SpeakerName"),
	DEFINE_KEYFIELD(m_iszListenFilter, FIELD_STRING, "ListenFilter"),
	DEFINE_FIELD(m_hListenFilter, FIELD_EHANDLE),
	DEFINE_FIELD(m_hSpeaker, FIELD_EHANDLE),
	// DEFINE_FIELD(m_bAvoidFeedback, FIELD_BOOLEAN),	// DONT SAVE
	DEFINE_KEYFIELD(m_iSpeakerDSPPreset, FIELD_INTEGER, "speaker_dsp_preset" ),
	DEFINE_KEYFIELD(m_flMaxRange, FIELD_FLOAT, "MaxRange"),
	DEFINE_AUTO_ARRAY(m_szLastSound, FIELD_CHARACTER),

	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	DEFINE_INPUTFUNC(FIELD_STRING, "SetSpeakerName", InputSetSpeakerName),

	DEFINE_OUTPUT(m_SoundLevel, "SoundLevel"),
	DEFINE_OUTPUT(m_OnRoutedSound, "OnRoutedSound" ),
	DEFINE_OUTPUT(m_OnHeardSound, "OnHeardSound" ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEnvMicrophone::~CEnvMicrophone( void )
{
	s_Microphones.FindAndRemove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Called before spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CEnvMicrophone::Spawn(void)
{
	//
	// Build our sound type mask from our spawnflags.
	//
	static int nFlags[][2] =
	{
		{ SF_MICROPHONE_SOUND_COMBAT,			SOUND_COMBAT },
		{ SF_MICROPHONE_SOUND_WORLD,			SOUND_WORLD },
		{ SF_MICROPHONE_SOUND_PLAYER,			SOUND_PLAYER },
		{ SF_MICROPHONE_SOUND_BULLET_IMPACT,	SOUND_BULLET_IMPACT },
		{ SF_MICROPHONE_SOUND_EXPLOSION,		SOUND_CONTEXT_EXPLOSION },
	};

	for (int i = 0; i < sizeof(nFlags) / sizeof(nFlags[0]); i++)
	{
		if (m_spawnflags & nFlags[i][0])
		{
			m_nSoundMask |= nFlags[i][1];
		}
	}

	if (m_flSensitivity == 0)
	{
		//
		// Avoid a divide by zero in CanHearSound.
		//
		m_flSensitivity = 1;
	}
	else if (m_flSensitivity > 10)
	{
		m_flSensitivity = 10;
	}

	m_flSmoothFactor = clamp(m_flSmoothFactor, 0.f, 0.9f);

	if (!m_bDisabled)
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called after all entities have spawned and after a load game.
//			Finds the reference point at which to measure sound level.
//-----------------------------------------------------------------------------
void CEnvMicrophone::Activate(void)
{
	BaseClass::Activate();

	// Get a handle to my filter entity if there is one
	if (m_iszListenFilter != NULL_STRING)
	{
		m_hListenFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iszListenFilter ));
	}

	if (m_target != NULL_STRING)
	{
		m_hMeasureTarget = gEntList.FindEntityByName(NULL, STRING(m_target) );

		//
		// If we were given a bad measure target, just measure sound where we are.
		//
		if ((m_hMeasureTarget == NULL) || (m_hMeasureTarget->edict() == NULL))
		{
			// We've decided to disable this warning since this seems to be the 90% case.
			//Warning( "EnvMicrophone - Measure target not found or measure target with no origin. Using Self.!\n");
			m_hMeasureTarget = this;
		}
	}
	else
	{
		m_hMeasureTarget = this;
	}

	ActivateSpeaker();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvMicrophone::OnRestore( void )
{
	BaseClass::OnRestore();

	ActivateSpeaker();
}

//-----------------------------------------------------------------------------
// Purpose: If we've got a speaker, add ourselves to the list of microphones that want to listen
//-----------------------------------------------------------------------------
void CEnvMicrophone::ActivateSpeaker( void )
{
	// If we're enabled, set the dsp_speaker preset to my specified one
	if ( !m_bDisabled )
	{
		ConVarRef dsp_speaker( "dsp_speaker" );
		if ( dsp_speaker.IsValid() )
		{
			int iDSPPreset = m_iSpeakerDSPPreset;
			if ( !iDSPPreset )
			{
				// Reset it to the default
				iDSPPreset = atoi( dsp_speaker.GetDefault() );
			}
			DevMsg( 2, "Microphone %s set dsp_speaker to %d.\n", STRING(GetEntityName()), iDSPPreset);
			dsp_speaker.SetValue( m_iSpeakerDSPPreset );
		}
	}

	if ( m_iszSpeakerName != NULL_STRING )
	{
		// We've got a speaker to play heard sounds through. To do this, we need to add ourselves 
		// to the list of microphones who want to be told whenever a sound is played.
		if ( s_Microphones.Find(this) == -1 )
		{
			s_Microphones.AddToTail( this );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stops the microphone from sampling the sound level and firing the
//			SoundLevel output.
//-----------------------------------------------------------------------------
void CEnvMicrophone::InputEnable( inputdata_t &inputdata )
{
	if (m_bDisabled)
	{
		m_bDisabled = false;
		SetNextThink( gpGlobals->curtime + 0.1f );

		ActivateSpeaker();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Resumes sampling the sound level and firing the SoundLevel output.
//-----------------------------------------------------------------------------
void CEnvMicrophone::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
	if ( m_hSpeaker )
	{
		CBaseEntity::StopSound( m_hSpeaker->entindex(), CHAN_STATIC, m_szLastSound );
		m_szLastSound[0] = 0;

		// Remove ourselves from the list of active mics
		s_Microphones.FindAndRemove( this );
	}
	SetNextThink( TICK_NEVER_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CEnvMicrophone::InputSetSpeakerName( inputdata_t &inputdata )
{
	SetSpeakerName( inputdata.value.StringID() );
}

//-----------------------------------------------------------------------------
// Purpose: Checks whether this microphone can hear a given sound, and at what
//			relative volume level.
// Input  : pSound - Sound to test.
//			flVolume - Returns with the relative sound volume from 0 - 1.
// Output : Returns true if the sound could be heard at the sample point, false if not.
//-----------------------------------------------------------------------------
bool CEnvMicrophone::CanHearSound(CSound *pSound, float &flVolume)
{
	flVolume = 0;

	if ( m_bDisabled )
	{
		return false;
	}

	// Cull out sounds except from specific entities
	CBaseFilter *pFilter = m_hListenFilter.Get();
	if ( pFilter )
	{
		CBaseEntity *pSoundOwner = pSound->m_hOwner.Get();
		if ( !pSoundOwner || !pFilter->PassesFilter( this, pSoundOwner ) )
		{
			return false;
		}
	}

	float flDistance = (pSound->GetSoundOrigin() - m_hMeasureTarget->GetAbsOrigin()).Length();

	if (flDistance == 0)
	{
		flVolume = 1.0;
		return true;
	}

	// Over our max range?
	if ( m_flMaxRange && flDistance > m_flMaxRange )
	{
		return false;
	}

	if (flDistance <= pSound->Volume() * m_flSensitivity)
	{
		flVolume = 1 - (flDistance / (pSound->Volume() * m_flSensitivity));
		flVolume = clamp(flVolume, 0.f, 1.f);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the microphone can hear the specified sound
//-----------------------------------------------------------------------------
bool CEnvMicrophone::CanHearSound( int entindex, soundlevel_t soundlevel, float &flVolume, const Vector *pOrigin )
{
	if ( m_bDisabled )
	{
		flVolume = 0;
		return false;
	}

	if ( ( m_spawnflags & SF_MICROPHONE_IGNORE_NONATTENUATED ) && soundlevel == SNDLVL_NONE )
	{
		return false;
	}

	// Sound might be coming from an origin or from an entity.
	CBaseEntity *pEntity = NULL;
	if ( entindex )
	{
		pEntity = CBaseEntity::Instance( engine->PEntityOfEntIndex(entindex) );
	}
			    
	// Cull out sounds except from specific entities
	CBaseFilter *pFilter = m_hListenFilter.Get();
	if ( pFilter )
	{
		if ( !pEntity || !pFilter->PassesFilter( this, pEntity ) )
		{
			flVolume = 0;
			return false;
		}
	}

	float flDistance = 0;
	if ( pOrigin )
	{
		flDistance = pOrigin->DistTo( m_hMeasureTarget->GetAbsOrigin() );
	}
	else if ( pEntity )
	{
		flDistance = pEntity->WorldSpaceCenter().DistTo( m_hMeasureTarget->GetAbsOrigin() );
	}

	// Over our max range?
	if ( m_flMaxRange && flDistance > m_flMaxRange )
	{
#ifdef DEBUG_MICROPHONE
		Msg("OUT OF RANGE.\n" );
#endif
		return false;
	}

#ifdef DEBUG_MICROPHONE
	Msg(" flVolume %f ", flVolume );
#endif

	// Reduce the volume by the amount it fell to get to the microphone
	float gain = enginesound->GetDistGainFromSoundLevel( soundlevel, flDistance );
	flVolume *= gain;

#ifdef DEBUG_MICROPHONE
	Msg("dist %2f, soundlevel %d: gain %f", flDistance, (int)soundlevel, gain );
	if ( !flVolume )
	{
		Msg(" : REJECTED\n" );
	}
	else
	{
		Msg(" : SENT\n" );
	}
#endif

	return ( flVolume > 0 );
}

void CEnvMicrophone::SetSensitivity( float flSensitivity )
{
	m_flSensitivity = flSensitivity;
}

void CEnvMicrophone::SetSpeakerName( string_t iszSpeakerName )
{
	m_iszSpeakerName = iszSpeakerName;

	// Set the speaker to null. This will force it to find the speaker next time a sound is routed.
	m_hSpeaker = NULL;
	ActivateSpeaker();
}

//-----------------------------------------------------------------------------
// Purpose: Listens for sounds and updates the value of the SoundLevel output.
//-----------------------------------------------------------------------------
void CEnvMicrophone::Think(void)
{
	int nSound = CSoundEnt::ActiveList();
	bool fHearSound = false;

	float flMaxVolume = 0;
	
	//
	// Find the loudest sound that this microphone cares about.
	//
	while (nSound != SOUNDLIST_EMPTY)
	{
		CSound *pCurrentSound = CSoundEnt::SoundPointerForIndex(nSound);

		if (pCurrentSound)
		{
			if (m_nSoundMask & pCurrentSound->SoundType())
			{
				float flVolume = 0;
				if (CanHearSound(pCurrentSound, flVolume) && (flVolume > flMaxVolume))
				{
					flMaxVolume = flVolume;
					fHearSound = true;
				}
			}
		}

		nSound = pCurrentSound->NextSound();
	}

	if( fHearSound )
	{
		m_OnHeardSound.FireOutput( this, this );
	}

	if (flMaxVolume != m_SoundLevel.Get())
	{
		//
		// Don't smooth if we are within an epsilon. This allows the output to stop firing
		// much more quickly.
		//
		if (fabs(flMaxVolume - m_SoundLevel.Get()) < MICROPHONE_SETTLE_EPSILON)
		{
			m_SoundLevel.Set(flMaxVolume, this, this);
		}
		else
		{
			m_SoundLevel.Set(flMaxVolume * (1 - m_flSmoothFactor) + m_SoundLevel.Get() * m_flSmoothFactor, this, this);
		}
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: Hook for the sound system to tell us when a sound's been played
//-----------------------------------------------------------------------------
MicrophoneResult_t CEnvMicrophone::SoundPlayed( int entindex, const char *soundname, soundlevel_t soundlevel, float flVolume, int iFlags, int iPitch, const Vector *pOrigin, float soundtime, CUtlVector< Vector >& soundorigins )
{
	if ( m_bAvoidFeedback )
		return MicrophoneResult_Ok;

	// Don't hear sounds that have already been heard by a microphone to avoid feedback!
	if ( iFlags & SND_SPEAKER )
		return MicrophoneResult_Ok;

#ifdef DEBUG_MICROPHONE
	Msg("%s heard %s: ", STRING(GetEntityName()), soundname );
#endif

	if ( !CanHearSound( entindex, soundlevel, flVolume, pOrigin ) )
		return MicrophoneResult_Ok;

	// We've heard it. Play it out our speaker. If our speaker's gone away, we're done.
	if ( !m_hSpeaker )
	{
		// First time, find our speaker. Done here, because finding it in Activate() wouldn't
		// find players, and we need to be able to specify !player for a speaker.
		if ( m_iszSpeakerName != NULL_STRING )
		{
			m_hSpeaker = gEntList.FindEntityByName(NULL, STRING(m_iszSpeakerName) );

			if ( !m_hSpeaker )
			{
				Warning( "EnvMicrophone %s specifies a non-existent speaker name: %s\n", STRING(GetEntityName()), STRING(m_iszSpeakerName) );
				m_iszSpeakerName = NULL_STRING;
			}
		}

		if ( !m_hSpeaker )
		{
			return MicrophoneResult_Remove;
		}
	}

	m_bAvoidFeedback = true;

	// Add the speaker flag. Detected at playback and applies the speaker filter.
	iFlags |= SND_SPEAKER;
	CPASAttenuationFilter filter( m_hSpeaker );

	EmitSound_t ep;
	ep.m_nChannel = CHAN_STATIC;
	ep.m_pSoundName = soundname;
	ep.m_flVolume = flVolume;
	ep.m_SoundLevel = soundlevel;
	ep.m_nFlags = iFlags;
	ep.m_nPitch = iPitch;
	ep.m_pOrigin = &m_hSpeaker->GetAbsOrigin();
	ep.m_flSoundTime = soundtime;
	ep.m_nSpeakerEntity = entindex;

	CBaseEntity::EmitSound( filter, m_hSpeaker->entindex(), ep );

	Q_strncpy( m_szLastSound, soundname, sizeof(m_szLastSound) );
	m_OnRoutedSound.FireOutput( this, this, 0 );

	m_bAvoidFeedback = false;

	// Copy emitted origin to soundorigins array
	for ( int i = 0; i < ep.m_UtlVecSoundOrigin.Count(); ++i )
	{
		soundorigins.AddToTail( ep.m_UtlVecSoundOrigin[ i ] );
	}

	// Do we want to allow the original sound to play?
	if ( m_spawnflags & SF_MICROPHONE_SWALLOW_ROUTED_SOUNDS )
	{
		return MicrophoneResult_Swallow;
	}

	return MicrophoneResult_Ok;
}


//-----------------------------------------------------------------------------
// Purpose: Called by the sound system whenever a sound is played so that
//			active microphones can have a chance to pick up the sound.
// Output : Returns whether or not the sound was swallowed by the microphone.
//			Swallowed sounds should not be played by the sound system.
//-----------------------------------------------------------------------------
bool CEnvMicrophone::OnSoundPlayed( int entindex, const char *soundname, soundlevel_t soundlevel, float flVolume, int iFlags, int iPitch, const Vector *pOrigin, float soundtime, CUtlVector< Vector >& soundorigins )
{
	bool bSwallowed = false;

	// Loop through all registered microphones and tell them the sound was just played
	int iCount = s_Microphones.Count();
	if ( iCount > 0 )
	{
		// Iterate backwards because we might be deleting microphones.
		for ( int i = iCount - 1; i >= 0; i-- )
		{
			if ( s_Microphones[i] )
			{
				MicrophoneResult_t eResult = s_Microphones[i]->SoundPlayed(
					entindex, 
					soundname, 
					soundlevel, 
					flVolume, 
					iFlags, 
					iPitch, 
					pOrigin, 
					soundtime,
					soundorigins );

				if ( eResult == MicrophoneResult_Swallow )
				{
					// Microphone told us to swallow it
					bSwallowed = true;
				}
				else if ( eResult == MicrophoneResult_Remove )
				{
					s_Microphones.FastRemove( i );
				}
			}
		}
	}

	return bSwallowed;
}
