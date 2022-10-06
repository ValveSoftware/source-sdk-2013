//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "mathlib/mathlib.h"
#include "env_speaker.h"
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
#include "AI_ResponseSystem.h"
#ifdef MAPBASE
#include "sceneentity.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_SPEAKER_START_SILENT		1
#define SF_SPEAKER_EVERYWHERE		2

extern ISaveRestoreOps *responseSystemSaveRestoreOps;
#include "saverestore.h"

LINK_ENTITY_TO_CLASS( env_speaker, CSpeaker );

BEGIN_DATADESC( CSpeaker )

#ifdef MAPBASE
	DEFINE_FIELD( m_hTarget, FIELD_EHANDLE ),
#endif

	DEFINE_KEYFIELD( m_delayMin, FIELD_FLOAT, "delaymin" ),
	DEFINE_KEYFIELD( m_delayMax, FIELD_FLOAT, "delaymax" ),
	DEFINE_KEYFIELD( m_iszRuleScriptFile, FIELD_STRING, "rulescript" ),
	DEFINE_KEYFIELD( m_iszConcept, FIELD_STRING, "concept" ),

	// Needs to be set up in the Activate methods of derived classes
	//DEFINE_CUSTOM_FIELD( m_pInstancedResponseSystem, responseSystemSaveRestoreOps ),

	// Function Pointers
	DEFINE_FUNCTION( SpeakerThink ),

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

#ifdef MAPBASE
	DEFINE_OUTPUT( m_OnSpeak, "OnSpeak" ),
#endif

END_DATADESC()


void CSpeaker::Spawn( void )
{
	const char *soundfile = (const char *)STRING( m_iszRuleScriptFile );

	if ( Q_strlen( soundfile ) < 1 )
	{
		Warning( "'speaker' entity with no Level/Sentence! at: %f, %f, %f\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		SetNextThink( gpGlobals->curtime + 0.1f );
		SetThink( &CSpeaker::SUB_Remove );
		return;
	}

//	const char *concept = (const char *)STRING( m_iszConcept );
//	if ( Q_strlen( concept ) < 1 )
//	{
//		Warning( "'speaker' entity using rule set %s with empty concept string\n", soundfile );
//	}

    SetSolid( SOLID_NONE );
    SetMoveType( MOVETYPE_NONE );
	
	SetThink(&CSpeaker::SpeakerThink);
	SetNextThink( TICK_NEVER_THINK );

	// allow on/off switching via 'use' function.

	Precache( );
}


void CSpeaker::Precache( void )
{
	if ( !FBitSet (m_spawnflags, SF_SPEAKER_START_SILENT ) )
	{
		// set first announcement time for random n second
		SetNextThink( gpGlobals->curtime + random->RandomFloat(5.0, 15.0) );
	}

	if ( !m_pInstancedResponseSystem && Q_strlen( STRING(m_iszRuleScriptFile) ) > 0 )
	{
		m_pInstancedResponseSystem = PrecacheCustomResponseSystem( STRING( m_iszRuleScriptFile ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Need a custom save restore so we can restore the instanced response system by name
//  after we've loaded the filename from disk...
// Input  : &save - 
//-----------------------------------------------------------------------------
int	CSpeaker::Save( ISave &save )
{
	int iret = BaseClass::Save( save );
	if ( iret )
	{
		bool doSave = ( m_pInstancedResponseSystem && ( m_iszRuleScriptFile != NULL_STRING ) ) ? true : false;
		save.WriteBool( &doSave );
		if ( doSave )
		{
			save.StartBlock( "InstancedResponseSystem" );
			{
				SaveRestoreFieldInfo_t fieldInfo = { &m_pInstancedResponseSystem, 0, NULL };
				responseSystemSaveRestoreOps->Save( fieldInfo, &save );
			}
			save.EndBlock();
		}
	}
	return iret;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &restore - 
//-----------------------------------------------------------------------------
int	CSpeaker::Restore( IRestore &restore )
{
	int iret = BaseClass::Restore( restore );
	if ( iret )
	{
		bool doRead = false;
		restore.ReadBool( &doRead );
		if ( doRead )
		{
			char szResponseSystemBlockName[SIZE_BLOCK_NAME_BUF];
			restore.StartBlock( szResponseSystemBlockName );
			if ( !Q_stricmp( szResponseSystemBlockName, "InstancedResponseSystem" ) )
			{
				if ( !m_pInstancedResponseSystem && Q_strlen( STRING(m_iszRuleScriptFile) ) > 0 )
				{
					m_pInstancedResponseSystem = PrecacheCustomResponseSystem( STRING( m_iszRuleScriptFile ) );
					if ( m_pInstancedResponseSystem )
					{
						SaveRestoreFieldInfo_t fieldInfo =
						{
							&m_pInstancedResponseSystem,
							0,
							NULL
						};
						responseSystemSaveRestoreOps->Restore( fieldInfo, &restore );
					}
				}
			}
			restore.EndBlock();
		}
	}
	return iret;
}

void CSpeaker::SpeakerThink( void )
{
	// Wait for the talking characters to finish first.
	if ( !g_AIFriendliesTalkSemaphore.IsAvailable( this ) || !g_AIFoesTalkSemaphore.IsAvailable( this ) )
	{
		float releaseTime = MAX( g_AIFriendliesTalkSemaphore.GetReleaseTime(), g_AIFoesTalkSemaphore.GetReleaseTime() );
		// Add some slop (only up to one second)
		releaseTime += random->RandomFloat( 0, 1 );
		SetNextThink( releaseTime );
		return;
	}
	
	DispatchResponse( m_iszConcept.ToCStr() );

	SetNextThink( gpGlobals->curtime + random->RandomFloat(m_delayMin, m_delayMax) );

	// time delay until it's ok to speak: used so that two NPCs don't talk at once
	g_AIFriendliesTalkSemaphore.Acquire( 5, this );		
	g_AIFoesTalkSemaphore.Acquire( 5, this );		
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CBaseEntity *CSpeaker::GetTarget()
{
	if (!m_hTarget && m_target != NULL_STRING)
		m_hTarget = gEntList.FindEntityByName(NULL, STRING(m_target), this, NULL, this);
	return m_hTarget;
}

//-----------------------------------------------------------------------------
// Purpose: Copied from CBaseEntity so we could use a !target for everything
// Input  : *conceptName - 
//-----------------------------------------------------------------------------
void CSpeaker::DispatchResponse( const char *conceptName )
{
	IResponseSystem *rs = GetResponseSystem();
	if ( !rs )
		return;

	CBaseEntity *pTarget = GetTarget();
	if (!pTarget)
		pTarget = this;

	// See CBaseEntity and stuff...
	AI_CriteriaSet set;
	set.AppendCriteria( "concept", conceptName, CONCEPT_WEIGHT );
	ModifyOrAppendCriteria( set );
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	if( pPlayer )
		pPlayer->ModifyOrAppendPlayerCriteria( set );
	ReAppendContextCriteria( set );
	AI_Response result;
	bool found = rs->FindBestResponse( set, result );
	if ( !found )
	{
		return;
	}

	// Handle the response here...
	char response[ 256 ];
	result.GetResponse( response, sizeof( response ) );
	if (response[0] == '$')
	{
		response[0] = '\0';
		DevMsg("Replacing %s with %s...\n", response, GetContextValue(response));
		Q_strncpy(response, GetContextValue(response), sizeof(response));
		PrecacheScriptSound( response );
	}

#ifdef NEW_RESPONSE_SYSTEM
	switch (result.GetType())
	{
	case ResponseRules::RESPONSE_SPEAK:
	{
		pTarget->EmitSound( response );
	}
	break;
	case ResponseRules::RESPONSE_SENTENCE:
	{
		int sentenceIndex = SENTENCEG_Lookup( response );
		if (sentenceIndex == -1)
		{
			// sentence not found
			break;
		}

		// FIXME:  Get pitch from npc?
		CPASAttenuationFilter filter( pTarget );
		CBaseEntity::EmitSentenceByIndex( filter, pTarget->entindex(), CHAN_VOICE, sentenceIndex, 1, result.GetSoundLevel(), 0, PITCH_NORM );
	}
	break;
	case ResponseRules::RESPONSE_SCENE:
	{
		CBaseFlex *pFlex = NULL;
		if (pTarget != this)
		{
			// Attempt to get flex on the target
			pFlex = dynamic_cast<CBaseFlex*>(pTarget);
		}
		InstancedScriptedScene(pFlex, response);
	}
	break;
	case ResponseRules::RESPONSE_PRINT:
	{

	}
	break;
	case ResponseRules::RESPONSE_ENTITYIO:
	{
		CAI_Expresser::FireEntIOFromResponse( response, pTarget );
		break;
	}
#ifdef MAPBASE_VSCRIPT
	case ResponseRules::RESPONSE_VSCRIPT:
	{
		CAI_Expresser::RunScriptResponse( pTarget, response, &set, false );
		break;
	}
	case ResponseRules::RESPONSE_VSCRIPT_FILE:
	{
		CAI_Expresser::RunScriptResponse( pTarget, response, &set, true );
		break;
	}
#endif
	default:
		break;
	}
#else
	switch ( result.GetType() )
	{
	case RESPONSE_SPEAK:
		{
			pTarget->EmitSound( response );
		}
		break;
	case RESPONSE_SENTENCE:
		{
			int sentenceIndex = SENTENCEG_Lookup( response );
			if( sentenceIndex == -1 )
			{
				// sentence not found
				break;
			}

			// FIXME:  Get pitch from npc?
			CPASAttenuationFilter filter( pTarget );
			CBaseEntity::EmitSentenceByIndex( filter, pTarget->entindex(), CHAN_VOICE, sentenceIndex, 1, result.GetSoundLevel(), 0, PITCH_NORM );
		}
		break;
	case RESPONSE_SCENE:
		{
			CBaseFlex *pFlex = NULL;
			if (pTarget != this)
			{
				// Attempt to get flex on the target
				pFlex = dynamic_cast<CBaseFlex*>(pTarget);
			}
			InstancedScriptedScene(pFlex, response);
		}
		break;
	case RESPONSE_PRINT:
		{

		}
		break;
	default:
		break;
	}
#endif

	// AllocPooledString?
	m_OnSpeak.Set(MAKE_STRING(response), pTarget, this);
}
#endif


void CSpeaker::InputTurnOn( inputdata_t &inputdata )
{
	// turn on announcements
	SetNextThink( gpGlobals->curtime + 0.1 );
}


void CSpeaker::InputTurnOff( inputdata_t &inputdata )
{
	// turn off announcements
	SetNextThink( TICK_NEVER_THINK );
}


//
// If an announcement is pending, cancel it.  If no announcement is pending, start one.
//
void CSpeaker::InputToggle( inputdata_t &inputdata )
{
	int fActive = (GetNextThink() > 0.0 );

	// fActive is true only if an announcement is pending
	if ( fActive )
	{
		// turn off announcements
		SetNextThink( TICK_NEVER_THINK );
	}
	else 
	{
		// turn on announcements
		SetNextThink( gpGlobals->curtime + 0.1f );
	} 
}
