//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//=========================================================
// Generic NPC - purely for scripted sequence work.
//=========================================================
#include "cbase.h"
#include "shareddefs.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "ai_baseactor.h"
#include "tier1/strtools.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar flex_looktime( "flex_looktime", "5" );

//---------------------------------------------------------
// Sounds
//---------------------------------------------------------


//=========================================================
// NPC's Anim Events Go Here
//=========================================================

class CGenericActor : public CAI_BaseActor
{
public:
	DECLARE_CLASS( CGenericActor, CAI_BaseActor );

	void	Spawn( void );
	void	Precache( void );
	float	MaxYawSpeed( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		GetSoundInterests ( void );

	
	void	TempGunEffect( void );

	string_t			m_strHullName;

	DECLARE_DATADESC();
};
LINK_ENTITY_TO_CLASS( generic_actor, CGenericActor );

BEGIN_DATADESC( CGenericActor )

	DEFINE_KEYFIELD(m_strHullName,			FIELD_STRING, "hull_name" ),

END_DATADESC()


//=========================================================
// Classify - indicates this NPC's place in the 
// relationship table.
//=========================================================
Class_T	CGenericActor::Classify ( void )
{
	return	CLASS_NONE;
}

//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CGenericActor::MaxYawSpeed ( void )
{
	return 90;
}

//=========================================================
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericActor::HandleAnimEvent( animevent_t *pEvent )
{
	BaseClass::HandleAnimEvent( pEvent );
}

//=========================================================
// GetSoundInterests - generic NPC can't hear.
//=========================================================
int CGenericActor::GetSoundInterests ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CGenericActor::Spawn()
{
	Precache();

	SetModel( STRING( GetModelName() ) );

/*
	if ( FStrEq( STRING( GetModelName() ), "models/player.mdl" ) )
		UTIL_SetSize(this, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	else
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
*/

	if ( FStrEq( STRING( GetModelName() ), "models/player.mdl" ) || 
		 FStrEq( STRING( GetModelName() ), "models/holo.mdl" ) ||
		 FStrEq( STRING( GetModelName() ), "models/blackout.mdl" ) )
	{
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
	}
	else
	{
		UTIL_SetSize(this, NAI_Hull::Mins(HULL_HUMAN), NAI_Hull::Maxs(HULL_HUMAN));
	}

	if ( !FStrEq( STRING( GetModelName() ), "models/blackout.mdl" ) )
	{
		SetSolid( SOLID_BBOX );
		AddSolidFlags( FSOLID_NOT_STANDABLE );
	}
	else
	{
		SetSolid( SOLID_NONE );
	}

	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= 8;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS );
	
	// remove head turn if no eyes or forward attachment
	if (LookupAttachment( "eyes" ) > 0 && LookupAttachment( "forward" ) > 0) 
	{
		CapabilitiesAdd(  bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE );
	}

	if (m_strHullName != NULL_STRING)
	{
		SetHullType( NAI_Hull::LookupId( STRING( m_strHullName ) ) );
	}
	else
	{
		SetHullType( HULL_HUMAN );
	}
	SetHullSizeNormal( );

	NPCInit();
}

//=========================================================
// Precache - precaches all resources this NPC needs
//=========================================================
void CGenericActor::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );
}	

//=========================================================
// AI Schedules Specific to this NPC
//=========================================================






// -----------------------------------------------------------------------


// FIXME: delete this code

class CFlextalkActor : public CGenericActor
{
private:
	DECLARE_CLASS( CFlextalkActor, CGenericActor );
public:
	DECLARE_DATADESC();

	CFlextalkActor() { m_iszSentence = NULL_STRING; m_sentence = 0; }
	//void GenericCyclerSpawn(char *szModel, Vector vecMin, Vector vecMax);
	//virtual int	ObjectCaps( void ) { return (BaseClass::ObjectCaps() | FCAP_IMPULSE_USE); }
	//int OnTakeDamage( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType );
	//void Spawn( void );
	//void Precache( void );
	//void Think( void );

	virtual void ProcessSceneEvents( void );

	// Don't treat as a live target
	//virtual bool IsAlive( void ) { return FALSE; }

	float m_flextime;
	LocalFlexController_t m_flexnum;
	float m_flextarget[64];
	float m_blinktime;
	float m_looktime;
	Vector m_lookTarget;
	float m_speaktime;
	int	m_istalking;
	int	m_phoneme;

	string_t m_iszSentence;
	int m_sentence;

	void SetFlexTarget( LocalFlexController_t flexnum, float value );
	LocalFlexController_t LookupFlex( const char *szTarget );
};

BEGIN_DATADESC( CFlextalkActor )

	DEFINE_FIELD( m_flextime, FIELD_TIME ),
	DEFINE_FIELD( m_flexnum, FIELD_INTEGER ),
	DEFINE_ARRAY( m_flextarget, FIELD_FLOAT, 64 ),
	DEFINE_FIELD( m_blinktime, FIELD_TIME ),
	DEFINE_FIELD( m_looktime, FIELD_TIME ),
	DEFINE_FIELD( m_lookTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_speaktime, FIELD_TIME ),
	DEFINE_FIELD( m_istalking, FIELD_INTEGER ),
	DEFINE_FIELD( m_phoneme, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_iszSentence, FIELD_STRING, "Sentence" ),
	DEFINE_FIELD( m_sentence, FIELD_INTEGER ),

END_DATADESC()



LINK_ENTITY_TO_CLASS( cycler_actor, CFlextalkActor );

extern ConVar	flex_expression;
extern ConVar	flex_talk;

// Cycler member functions


extern const char *predef_flexcontroller_names[];
extern int predef_flexcontroller_names_count;
extern float predef_flexcontroller_values[7][30];

void CFlextalkActor::SetFlexTarget( LocalFlexController_t flexnum, float value )
{
	m_flextarget[flexnum] = value;

	const char *pszType = GetFlexControllerType( flexnum );

	for (LocalFlexController_t i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
	{
		if (i != flexnum)
		{
			const char *pszOtherType = GetFlexControllerType( i );
			if (stricmp( pszType, pszOtherType ) == 0)
			{
				m_flextarget[i] = 0;
			}
		}
	}

	float value2 = value;
	if (1 || random->RandomFloat( 0.0, 1.0 ) < 0.2)
	{
		value2 = random->RandomFloat( value - 0.2, value + 0.2 );
		value2 = clamp( value2, 0.0f, 1.0f );
	}


	// HACK, for now, consider then linked is named "right_" or "left_"
	if (strncmp( "right_", GetFlexControllerName( flexnum ), 6 ) == 0)
	{
		m_flextarget[flexnum+1] = value2;
	}
	else if (strncmp( "left_", GetFlexControllerName( flexnum ), 5 ) == 0)
	{
		m_flextarget[flexnum-1] = value2;
	}
}


LocalFlexController_t CFlextalkActor::LookupFlex( const char *szTarget  )
{
	for (LocalFlexController_t i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
	{
		const char *pszFlex = GetFlexControllerName( i );
		if (stricmp( szTarget, pszFlex ) == 0)
		{
			return i;
		}
	}
	return LocalFlexController_t(-1);
}


void CFlextalkActor::ProcessSceneEvents( void )
{
	if ( HasSceneEvents() )
	{
		BaseClass::ProcessSceneEvents( );
		return;
	}
	
	// only do this if they have more than eyelid movement
	if (GetNumFlexControllers() > 2)
	{
		const char *pszExpression = flex_expression.GetString();

		if (pszExpression && pszExpression[0] == '+' && pszExpression[1] != '\0')
		{
			int i;
			int j = atoi( &pszExpression[1] );
			for (i = 0; i < GetNumFlexControllers(); i++)
			{
				m_flextarget[m_flexnum] = 0;
			}

			for (i = 0; i < predef_flexcontroller_names_count && predef_flexcontroller_names[i]; i++)
			{
				m_flexnum = LookupFlex( predef_flexcontroller_names[i] );
				m_flextarget[m_flexnum] = predef_flexcontroller_values[j][i];
				// Msg( "%s %.3f\n", predef_flexcontroller_names[i], predef_flexcontroller_values[j][i] );
			}
		}
		else if (pszExpression && pszExpression[0] != '\0' && strcmp(pszExpression, "+") != 0)
		{
			char szExpression[128];
			char szTemp[32];

			Q_strncpy( szExpression, pszExpression ,sizeof(szExpression));
			char *pszExpression = szExpression;

			while (*pszExpression != '\0')
			{
				if (*pszExpression == '+')
					*pszExpression = ' ';
				
				pszExpression++;
			}

			pszExpression = szExpression;

			while (*pszExpression)
			{
				if (*pszExpression != ' ')
				{
					if (*pszExpression == '-')
					{
						for (LocalFlexController_t i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
						{
							m_flextarget[i] = 0;
						}
					}
					else if (*pszExpression == '?')
					{
						for (LocalFlexController_t i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
						{
							Msg( "\"%s\" ", GetFlexControllerName( i ) );
						}
						Msg( "\n" );
						flex_expression.SetValue( "" );
					}
					else
					{
						if (sscanf( pszExpression, "%31s", szTemp ) == 1)
						{
							m_flexnum = LookupFlex( szTemp );

							if (m_flexnum != LocalFlexController_t(-1) && m_flextarget[m_flexnum] != 1)
							{
								m_flextarget[m_flexnum] = 1.0;
								// SetFlexTarget( m_flexnum );
							}
							pszExpression += strlen( szTemp ) - 1;
						}
					}
				}
				pszExpression++;
			}
		} 
		else if (m_flextime < gpGlobals->curtime)
		{
			m_flextime = gpGlobals->curtime + random->RandomFloat( 0.3, 0.5 ) * (30.0 / GetNumFlexControllers());
			m_flexnum = (LocalFlexController_t)random->RandomInt( 0, GetNumFlexControllers() - 1 );

			if (m_flextarget[m_flexnum] == 1)
			{
				m_flextarget[m_flexnum] = 0;
			}
			else if (stricmp( GetFlexControllerType( m_flexnum ), "phoneme" ) != 0)
			{
				if (strstr( GetFlexControllerName( m_flexnum ), "upper_raiser" ) == NULL)
				{
					Msg( "%s:%s\n", GetFlexControllerType( m_flexnum ), GetFlexControllerName( m_flexnum ) );
					SetFlexTarget( m_flexnum, random->RandomFloat( 0.5, 1.0 ) );
				}
			}
		}

		// slide it up.
		for (LocalFlexController_t i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
		{
			float weight = GetFlexWeight( i );

			if (weight != m_flextarget[i])
			{
				weight = weight + (m_flextarget[i] - weight) / random->RandomFloat( 2.0, 4.0 );
			}
			weight = clamp( weight, 0.0f, 1.0f );
			SetFlexWeight( i, weight );
		}

		if (flex_talk.GetInt() == -1)
		{
			m_istalking = 1;

			char pszSentence[256];
			Q_snprintf( pszSentence,sizeof(pszSentence), "%s%d", STRING(m_iszSentence), m_sentence++ );
			int sentenceIndex = engine->SentenceIndexFromName( pszSentence );
			if (sentenceIndex >= 0)
			{
				Msg( "%d : %s\n", sentenceIndex, pszSentence );
				CPASAttenuationFilter filter( this );
				CBaseEntity::EmitSentenceByIndex( filter, entindex(), CHAN_VOICE, sentenceIndex, 1, SNDLVL_TALKING, 0, PITCH_NORM );
			}
			else
			{
				m_sentence = 0;
			}

			flex_talk.SetValue( "0" );
		}
		else if (flex_talk.GetInt() == -2)
		{
			m_flNextEyeLookTime = gpGlobals->curtime + 1000.0;
		}
		else if (flex_talk.GetInt() == -3)
		{
			m_flNextEyeLookTime = gpGlobals->curtime;
			flex_talk.SetValue( "0" );
		}
		else if (flex_talk.GetInt() == -4)
		{
			AddLookTarget( UTIL_PlayerByIndex( 1 ), 0.5, flex_looktime.GetFloat()  );
			flex_talk.SetValue( "0" );
		}
		else if (flex_talk.GetInt() == -5)
		{
			PickLookTarget( true );
			flex_talk.SetValue( "0" );
		}
	}
}
