//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseanimating.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include <ctype.h>
#include "animation.h"
#include "eventlist.h"
#include "npcevent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

enum EffectType
{
	EFFECT_TYPE_TRAIL = 1,
	EFFECT_TYPE_SPRITE
};


static bool			    s_bUnget = false;
static unsigned char	*s_buffer;
static char			    s_name[ 256 ];
static const char		*s_currenttoken;
static int				s_tokencount;
static char			    s_token[ 1204 ];

class CEffectScriptElement
{
public:

	CEffectScriptElement();

	char m_szEffectName[128];
	CHandle<CSpriteTrail> m_pTrail;
	CHandle<CSprite> m_pSprite;
	int m_iType;
	int m_iRenderType;

	int m_iR;
	int m_iG;
	int m_iB;
	int m_iA;

	char m_szAttachment[128];
	char m_szMaterial[128];

	float m_flScale;
	float m_flFadeTime;
	float m_flTextureRes;

	bool  m_bStopFollowOnKill;

	bool IsActive( void ) { return m_bActive; }
	void Activate( void ) { m_bActive = true; }
	void Deactivate( void ) { m_bActive = false; }
private:

	bool m_bActive;
};

CEffectScriptElement::CEffectScriptElement()
{
	m_pTrail = NULL;
	m_pSprite = NULL;
	m_iType = 0;

	Deactivate();
	m_iRenderType = kRenderTransAdd;

	m_iR = 255;
	m_iG = 0;
	m_iB = 0;
	m_iA = 255;

	m_flScale = 1.0f;
	m_flFadeTime = 1.0f;
	m_flTextureRes = -1.0f;
	m_bStopFollowOnKill = false;
}


//-----------------------------------------------------------------------------
// An entity which emits other entities at points 
//-----------------------------------------------------------------------------
class CEnvEffectsScript : public CBaseAnimating
{
public:
	DECLARE_CLASS( CEnvEffectsScript, CBaseAnimating );
	DECLARE_DATADESC();

	virtual void Precache();
	virtual void Spawn();
	virtual int  UpdateTransmitState();

	void InputSetSequence( inputdata_t &inputdata );
	void ParseScriptFile( void );
	void LoadFromBuffer( const char *scriptfile, const char *buffer );

	virtual void Think( void );

	void ParseNewEffect( void );

	const char *GetScriptFile( void ) 
	{
		return STRING( m_iszScriptName );
	}

	void HandleAnimEvent ( animevent_t *pEvent );
	void TrailEffectEvent( CEffectScriptElement *pEffect );
	void SpriteEffectEvent( CEffectScriptElement *pEffect );

	CEffectScriptElement *GetScriptElementByName( const char *pName );

private:
	
	string_t m_iszScriptName;
		
	CUtlVector< CEffectScriptElement > m_ScriptElements;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
	bool IsRootCommand( void )
	{
		if ( !Q_stricmp( s_token, "effect" ) )
			return true;

		return false;
	}
};

inline bool ParseToken( void )
{
	if ( s_bUnget )
	{
		s_bUnget = false;
		return true;
	}

	s_currenttoken = engine->ParseFile( s_currenttoken, s_token, sizeof( s_token ) );
	s_tokencount++;
	return s_currenttoken != NULL ? true : false;
}

inline void Unget()
{
	s_bUnget = true;
}

inline bool TokenWaiting( void )
{
	
	const char *p = s_currenttoken;
	while ( *p && *p!='\n')
	{
		// Special handler for // comment blocks
		if ( *p == '/' && *(p+1) == '/' )
			return false;

		if ( !V_isspace( *p ) || V_isalnum( *p ) )
			return true;

		p++;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Save/load 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CEnvEffectsScript )
	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "SetSequence", InputSetSequence ),
	DEFINE_KEYFIELD( m_iszScriptName, FIELD_STRING, "scriptfile" ),
	// DEFINE_FIELD( m_ScriptElements, CUtlVector < CEffectScriptElement > ),

	DEFINE_FUNCTION( Think ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( env_effectscript, CEnvEffectsScript );

//-----------------------------------------------------------------------------
// Should we transmit it to the client?
//-----------------------------------------------------------------------------
int CEnvEffectsScript::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Precache
//-----------------------------------------------------------------------------
void CEnvEffectsScript::Precache()
{
	BaseClass::Precache();
	PrecacheModel( STRING( GetModelName() ) );

	if ( m_iszScriptName != NULL_STRING )
		 ParseScriptFile();
	else
		 Warning( "CEnvEffectsScript with no script!\n" );
}

//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CEnvEffectsScript::Spawn()
{
	Precache();
	BaseClass::Spawn();

	// We need a model for its animation sequences even though we don't render it
	SetModel( STRING( GetModelName() ) );

	AddEffects( EF_NODRAW );

	SetThink( &CEnvEffectsScript::Think );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CEnvEffectsScript::Think( void )
{
	StudioFrameAdvance();
	DispatchAnimEvents( this );

	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CEnvEffectsScript::TrailEffectEvent( CEffectScriptElement *pEffect )
{
	if ( pEffect->IsActive() == false )
	{
		//Only one type of this effect active at a time.
		if ( pEffect->m_pTrail == NULL )
		{
			pEffect->m_pTrail = CSpriteTrail::SpriteTrailCreate( pEffect->m_szMaterial, GetAbsOrigin(), true );
			pEffect->m_pTrail->FollowEntity( this );
			pEffect->m_pTrail->SetTransparency( pEffect->m_iRenderType, pEffect->m_iR, pEffect->m_iG, pEffect->m_iB, pEffect->m_iA, kRenderFxNone );
			pEffect->m_pTrail->SetStartWidth( pEffect->m_flScale );
			if ( pEffect->m_flTextureRes < 0.0f )
			{
				pEffect->m_pTrail->SetTextureResolution( 1.0f / ( 16.0f * pEffect->m_flScale ) );
			}
			else
			{
				pEffect->m_pTrail->SetTextureResolution( pEffect->m_flTextureRes );
			}
			pEffect->m_pTrail->SetLifeTime( pEffect->m_flFadeTime );
			pEffect->m_pTrail->TurnOn();
			pEffect->m_pTrail->SetAttachment( this, LookupAttachment( pEffect->m_szAttachment ) );

			pEffect->Activate();
		}
	}
}

void CEnvEffectsScript::SpriteEffectEvent( CEffectScriptElement *pEffect )
{
	if ( pEffect->IsActive() == false )
	{
		//Only one type of this effect active at a time.
		if ( pEffect->m_pSprite == NULL )
		{
			pEffect->m_pSprite = CSprite::SpriteCreate( pEffect->m_szMaterial, GetAbsOrigin(), true );
			pEffect->m_pSprite->FollowEntity( this );
			pEffect->m_pSprite->SetTransparency( pEffect->m_iRenderType, pEffect->m_iR, pEffect->m_iG, pEffect->m_iB, pEffect->m_iA, kRenderFxNone );
			pEffect->m_pSprite->SetScale( pEffect->m_flScale );
			pEffect->m_pSprite->TurnOn();
			pEffect->m_pSprite->SetAttachment( this, LookupAttachment( pEffect->m_szAttachment ) );

			pEffect->Activate();
		}
	}
}

void CEnvEffectsScript::HandleAnimEvent ( animevent_t *pEvent ) 
{
	if ( pEvent->event == AE_START_SCRIPTED_EFFECT )
	{
		CEffectScriptElement *pCurrent = GetScriptElementByName( pEvent->options );

		if ( pCurrent )
		{
			if ( pCurrent->m_iType == EFFECT_TYPE_TRAIL )
				 TrailEffectEvent( pCurrent );
			else if ( pCurrent->m_iType == EFFECT_TYPE_SPRITE )
				 SpriteEffectEvent( pCurrent );
		}

		return;
	}

	if ( pEvent->event == AE_STOP_SCRIPTED_EFFECT )
	{
		CEffectScriptElement *pCurrent = GetScriptElementByName( pEvent->options );

		if ( pCurrent && pCurrent->IsActive() )
		{
			pCurrent->Deactivate();

			if ( pCurrent->m_iType == EFFECT_TYPE_TRAIL )
			{
				if ( pCurrent->m_bStopFollowOnKill == true )
				{
					Vector vOrigin;
					GetAttachment( pCurrent->m_pTrail->m_nAttachment, vOrigin );

					pCurrent->m_pTrail->StopFollowingEntity();

					pCurrent->m_pTrail->m_hAttachedToEntity = NULL;
					pCurrent->m_pTrail->m_nAttachment = 0;

					pCurrent->m_pTrail->SetAbsOrigin( vOrigin);
				}

				pCurrent->m_pTrail->FadeAndDie( pCurrent->m_flFadeTime );
				pCurrent->m_pTrail = NULL;
			}

			else if ( pCurrent->m_iType == EFFECT_TYPE_SPRITE )
			{
				if ( pCurrent->m_bStopFollowOnKill == true )
				{
					Vector vOrigin;
					GetAttachment( pCurrent->m_pSprite->m_nAttachment, vOrigin );

					pCurrent->m_pSprite->StopFollowingEntity();

					pCurrent->m_pSprite->m_hAttachedToEntity = NULL;
					pCurrent->m_pSprite->m_nAttachment = 0;

					pCurrent->m_pSprite->SetAbsOrigin( vOrigin);
				}

				pCurrent->m_pSprite->FadeAndDie( pCurrent->m_flFadeTime );
				pCurrent->m_pSprite = NULL;
			}
		}
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}
//-----------------------------------------------------------------------------
// Purpose: Input that sets the sequence of the entity
//-----------------------------------------------------------------------------
void CEnvEffectsScript::InputSetSequence( inputdata_t &inputdata )
{
	if ( inputdata.value.StringID() != NULL_STRING )
	{
		int nSequence = LookupSequence( STRING( inputdata.value.StringID() ) );
		if ( nSequence != ACT_INVALID )
		{
			SetSequence( nSequence );
			ResetSequenceInfo();
			SetCycle( 0.0f );
			m_flPlaybackRate = 1.0f;
		}
	}
}

void CEnvEffectsScript::ParseScriptFile( void )
{
	int length = 0;
	m_ScriptElements.RemoveAll();
	const char *pScriptName = GetScriptFile();

	//Reset everything.
	s_bUnget = false;
	s_currenttoken = NULL;
	s_tokencount = 0;
	memset( s_token, 0, 1204 );
	memset( s_name, 0, 256 );


	unsigned char *buf = (unsigned char *)UTIL_LoadFileForMe( pScriptName, &length );
	if ( length <= 0 || !buf )
	{
		DevMsg( 1, "CEnvEffectsScript:  failed to load %s\n", pScriptName );
		return;
	}

	s_currenttoken = (const char *)buf;
	LoadFromBuffer( pScriptName, (const char *)buf );

	UTIL_FreeFile( buf );
}

// FIXME: VS2022 Port 
//        This parameter, buffer, is unused??
void CEnvEffectsScript::LoadFromBuffer( const char *scriptfile, const char *buffer )
{
	while ( 1 )
	{
		ParseToken();
		
		if ( !s_token[0] )
		{
			break;
		}

		if ( !Q_stricmp( s_token, "effect" ) )
		{
			ParseNewEffect();
		}
		else
		{
			Warning( "CEnvEffectsScript: Unknown entry type '%s'\n", s_token );
			break;
		}
	}
}

void CEnvEffectsScript::ParseNewEffect( void )
{
	//Add a new effect to the list.
	CEffectScriptElement NewElement;
	
	// Effect Group Name
	ParseToken();
	Q_strncpy( NewElement.m_szEffectName, s_token, sizeof( NewElement.m_szEffectName ) );

	while ( 1 )
	{
		ParseToken();

		// Oops, part of next definition
		if( IsRootCommand() )
		{
			Unget();
			break;
		}

		if ( !Q_stricmp( s_token, "{" ) )
		{
			while ( 1 )
			{
				ParseToken();
				if ( !Q_stricmp( s_token, "}" ) )
					break;

				if ( !Q_stricmp( s_token, "type" ) )
				{
					ParseToken();

					if ( !Q_stricmp( s_token, "trail" ) )
						NewElement.m_iType = EFFECT_TYPE_TRAIL;
					else if ( !Q_stricmp( s_token, "sprite" ) )
						NewElement.m_iType = EFFECT_TYPE_SPRITE;

					continue;
				}

				if ( !Q_stricmp( s_token, "material" ) )
				{
					ParseToken();
					Q_strncpy( NewElement.m_szMaterial, s_token, sizeof( NewElement.m_szMaterial ) );
					PrecacheModel( NewElement.m_szMaterial );

					continue;
				}

				if ( !Q_stricmp( s_token, "attachment" ) )
				{
					ParseToken();
					Q_strncpy( NewElement.m_szAttachment, s_token, sizeof( NewElement.m_szAttachment ) );

					continue;
				}

				if ( !Q_stricmp( s_token, "color" ) )
				{
					ParseToken();
					sscanf( s_token, "%i %i %i %i", &NewElement.m_iR, &NewElement.m_iG, &NewElement.m_iB, &NewElement.m_iA );

					continue;
				}

				if ( !Q_stricmp( s_token, "scale" ) )
				{
					ParseToken();

					NewElement.m_flScale = atof( s_token );
					continue;
				}

				if ( !Q_stricmp( s_token, "texturescale" ) )
				{
					ParseToken();

					float flTextureScale = atof( s_token );
					NewElement.m_flTextureRes = (flTextureScale > 0.0f) ? 1.0f / flTextureScale : 0.0f;
					continue;
				}

				if ( !Q_stricmp( s_token, "fadetime" ) )
				{
					ParseToken();

					NewElement.m_flFadeTime = atof( s_token );
					continue;
				}

				if ( !Q_stricmp( s_token, "stopfollowonkill" ) )
				{
					ParseToken();

					NewElement.m_bStopFollowOnKill = !!atoi( s_token );
					continue;
				}

			}
			break;
		}
	}

	m_ScriptElements.AddToTail( NewElement );
}

CEffectScriptElement *CEnvEffectsScript::GetScriptElementByName( const char *pName )
{
	for ( int i = 0; i < m_ScriptElements.Count(); i++ )
	{
		CEffectScriptElement *pCurrent = &m_ScriptElements.Element( i );

		if ( pCurrent && !Q_stricmp( pCurrent->m_szEffectName, pName ) ) 
		{
			return pCurrent;
		}
	}

	return NULL;
}
