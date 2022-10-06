//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: An entity for creating instructor hints entirely with map logic
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "world.h"
#ifdef MAPBASE
#include "eventqueue.h"
#endif

#ifdef INFESTED_DLL
	#include "asw_marine.h"
	#include "asw_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEnvInstructorHint : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvInstructorHint, CPointEntity );
	DECLARE_DATADESC();

#ifdef MAPBASE
	CEnvInstructorHint( void );
#endif
	virtual ~CEnvInstructorHint( void ) {}

#ifdef MAPBASE
	virtual void OnRestore( void );
#endif

private:
	void InputShowHint( inputdata_t &inputdata );
	void InputEndHint( inputdata_t &inputdata );

#ifdef MAPBASE
	void InputSetCaption( inputdata_t &inputdata ) { m_iszCaption = inputdata.value.StringID(); }
#endif
	
	string_t	m_iszReplace_Key;
	string_t	m_iszHintTargetEntity;
	int			m_iTimeout;
	string_t	m_iszIcon_Onscreen;
	string_t	m_iszIcon_Offscreen;
	string_t	m_iszCaption;
	string_t	m_iszActivatorCaption;
	color32		m_Color;
	float		m_fIconOffset;
	float		m_fRange;
	uint8		m_iPulseOption;
	uint8		m_iAlphaOption;
	uint8		m_iShakeOption;
	bool		m_bStatic;
	bool		m_bNoOffscreen;
	bool		m_bForceCaption;
	string_t	m_iszBinding;
	bool		m_bAllowNoDrawTarget;
	bool		m_bLocalPlayerOnly;
#ifdef MAPBASE
	string_t	m_iszStartSound;
	int			m_iHintTargetPos;
	float		m_flActiveUntil;
	CHandle<CBasePlayer>	m_hActivator;
	EHANDLE		m_hTarget;
	bool		m_bFilterByActivator;
#endif
};

LINK_ENTITY_TO_CLASS( env_instructor_hint, CEnvInstructorHint );

BEGIN_DATADESC( CEnvInstructorHint )

	DEFINE_KEYFIELD( m_iszReplace_Key, FIELD_STRING, "hint_replace_key" ),
	DEFINE_KEYFIELD( m_iszHintTargetEntity, FIELD_STRING, "hint_target" ),
	DEFINE_KEYFIELD( m_iTimeout, FIELD_INTEGER, "hint_timeout" ),
	DEFINE_KEYFIELD( m_iszIcon_Onscreen, FIELD_STRING, "hint_icon_onscreen" ),
	DEFINE_KEYFIELD( m_iszIcon_Offscreen, FIELD_STRING, "hint_icon_offscreen" ),
	DEFINE_KEYFIELD( m_iszCaption, FIELD_STRING, "hint_caption" ),
	DEFINE_KEYFIELD( m_iszActivatorCaption, FIELD_STRING, "hint_activator_caption" ),
	DEFINE_KEYFIELD( m_Color, FIELD_COLOR32, "hint_color" ),
	DEFINE_KEYFIELD( m_fIconOffset, FIELD_FLOAT, "hint_icon_offset" ),
	DEFINE_KEYFIELD( m_fRange, FIELD_FLOAT, "hint_range" ),
	DEFINE_KEYFIELD( m_iPulseOption, FIELD_CHARACTER, "hint_pulseoption" ),
	DEFINE_KEYFIELD( m_iAlphaOption, FIELD_CHARACTER, "hint_alphaoption" ),
	DEFINE_KEYFIELD( m_iShakeOption, FIELD_CHARACTER, "hint_shakeoption" ),
	DEFINE_KEYFIELD( m_bStatic, FIELD_BOOLEAN, "hint_static" ),
	DEFINE_KEYFIELD( m_bNoOffscreen, FIELD_BOOLEAN, "hint_nooffscreen" ),
	DEFINE_KEYFIELD( m_bForceCaption, FIELD_BOOLEAN, "hint_forcecaption" ),
	DEFINE_KEYFIELD( m_iszBinding, FIELD_STRING, "hint_binding" ),
	DEFINE_KEYFIELD( m_bAllowNoDrawTarget, FIELD_BOOLEAN, "hint_allow_nodraw_target" ),	
	DEFINE_KEYFIELD( m_bLocalPlayerOnly, FIELD_BOOLEAN, "hint_local_player_only" ),
#ifdef MAPBASE
	DEFINE_KEYFIELD( m_iszStartSound, FIELD_STRING, "hint_start_sound" ),
	DEFINE_KEYFIELD( m_iHintTargetPos, FIELD_INTEGER, "hint_target_pos" ),

	DEFINE_FIELD( m_flActiveUntil, FIELD_TIME ),
	DEFINE_FIELD( m_hActivator, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bFilterByActivator, FIELD_BOOLEAN ),
#endif

	DEFINE_INPUTFUNC( FIELD_STRING,	"ShowHint",	InputShowHint ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EndHint",	InputEndHint ),

#ifdef MAPBASE
	DEFINE_INPUTFUNC( FIELD_STRING, "SetCaption", InputSetCaption ),
#endif

END_DATADESC()


#define LOCATOR_ICON_FX_PULSE_SLOW		0x00000001
#define LOCATOR_ICON_FX_ALPHA_SLOW		0x00000008
#define LOCATOR_ICON_FX_SHAKE_NARROW	0x00000040
#define LOCATOR_ICON_FX_STATIC			0x00000100	// This icon draws at a fixed location on the HUD.

#ifdef MAPBASE
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CEnvInstructorHint::CEnvInstructorHint( void )
{
	m_hActivator = NULL;
	m_hTarget = NULL;
	m_bFilterByActivator = false;
	m_flActiveUntil = -1.0f;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvInstructorHint::OnRestore( void )
{
	BaseClass::OnRestore();

	int iTimeLeft = 0;
	if ( m_flActiveUntil < 0.0f )
	{
		return;
	}
	if ( m_iTimeout != 0 )
	{
		iTimeLeft = static_cast<int>( m_flActiveUntil - gpGlobals->curtime );
		if ( iTimeLeft <= 0 )
		{
			return;
		}
	}

	int iOriginalTimeout = m_iTimeout;
	m_iTimeout = iTimeLeft;
	g_EventQueue.AddEvent( this, "ShowHint", 0.01f, NULL, this );
	m_iTimeout = iOriginalTimeout;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Input handler for showing the message and/or playing the sound.
//-----------------------------------------------------------------------------
void CEnvInstructorHint::InputShowHint( inputdata_t &inputdata )
{
	IGameEvent * event = gameeventmanager->CreateEvent( "instructor_server_hint_create", false );
	if ( event )
	{
		CBaseEntity *pTargetEntity = NULL;

#ifdef MAPBASE
		pTargetEntity = m_hTarget;

		if ( pTargetEntity == NULL )
#endif
			pTargetEntity = gEntList.FindEntityByName( NULL, m_iszHintTargetEntity );

		if( pTargetEntity == NULL && !m_bStatic )
			pTargetEntity = inputdata.pActivator;

		if( pTargetEntity == NULL )
			pTargetEntity = GetWorldEntity();

		char szColorString[128];
		Q_snprintf( szColorString, sizeof( szColorString ), "%.3d,%.3d,%.3d", m_Color.r, m_Color.g, m_Color.b );

		int iFlags = 0;
		
		iFlags |= (m_iPulseOption == 0) ? 0 : (LOCATOR_ICON_FX_PULSE_SLOW << (m_iPulseOption - 1));
		iFlags |= (m_iAlphaOption == 0) ? 0 : (LOCATOR_ICON_FX_ALPHA_SLOW << (m_iAlphaOption - 1));
		iFlags |= (m_iShakeOption == 0) ? 0 : (LOCATOR_ICON_FX_SHAKE_NARROW << (m_iShakeOption - 1));
		iFlags |= m_bStatic ? LOCATOR_ICON_FX_STATIC : 0;

		CBasePlayer *pActivator = NULL;
		bool bFilterByActivator = m_bLocalPlayerOnly;

#ifdef INFESTED_DLL
		CASW_Marine *pMarine = dynamic_cast<CASW_Marine*>( inputdata.pActivator );
		if ( pMarine )
		{
			pActivator = pMarine->GetCommander();
		}
#else
#ifdef MAPBASE
		if ( m_hActivator )
		{
			pActivator = m_hActivator;
			bFilterByActivator = m_bFilterByActivator;
		}
		else
#endif

		if ( inputdata.value.StringID() != NULL_STRING )
		{
			CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, inputdata.value.String() );
			pActivator = dynamic_cast<CBasePlayer*>( pTarget );
			if ( pActivator )
			{
				bFilterByActivator = true;
			}
		}
		else
		{
			if ( GameRules()->IsMultiplayer() == false )
			{
				pActivator = UTIL_GetLocalPlayer();
			}
			else
			{
				Warning( "Failed to play server side instructor hint: no player specified for hint\n" );
				Assert( 0 );
			}
		}
#endif

		const char *pActivatorCaption = m_iszActivatorCaption.ToCStr();
		if ( !pActivatorCaption || pActivatorCaption[ 0 ] == '\0' )
		{
			pActivatorCaption = m_iszCaption.ToCStr();
		}

		event->SetString( "hint_name", GetEntityName().ToCStr() );
		event->SetString( "hint_replace_key", m_iszReplace_Key.ToCStr() );
		event->SetInt( "hint_target", pTargetEntity->entindex() );
		event->SetInt( "hint_activator_userid", ( pActivator ? pActivator->GetUserID() : 0 ) );
		event->SetInt( "hint_timeout", m_iTimeout );
		event->SetString( "hint_icon_onscreen", m_iszIcon_Onscreen.ToCStr() );
		event->SetString( "hint_icon_offscreen", m_iszIcon_Offscreen.ToCStr() );
		event->SetString( "hint_caption", m_iszCaption.ToCStr() );
		event->SetString( "hint_activator_caption", pActivatorCaption );
		event->SetString( "hint_color", szColorString );
		event->SetFloat( "hint_icon_offset", m_fIconOffset );
		event->SetFloat( "hint_range", m_fRange );
		event->SetInt( "hint_flags", iFlags );
		event->SetString( "hint_binding", m_iszBinding.ToCStr() );
		event->SetBool( "hint_allow_nodraw_target", m_bAllowNoDrawTarget );
		event->SetBool( "hint_nooffscreen", m_bNoOffscreen );
		event->SetBool( "hint_forcecaption", m_bForceCaption );
		event->SetBool( "hint_local_player_only", bFilterByActivator );
#ifdef MAPBASE
		event->SetString( "hint_start_sound", m_iszStartSound.ToCStr() );
		event->SetInt( "hint_target_pos", m_iHintTargetPos );
#endif

		gameeventmanager->FireEvent( event );

#ifdef MAPBASE
		m_flActiveUntil = gpGlobals->curtime + m_iTimeout;
		m_hTarget = pTargetEntity;
		m_hActivator = pActivator;
		m_bFilterByActivator = bFilterByActivator;
#endif
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvInstructorHint::InputEndHint( inputdata_t &inputdata )
{
	IGameEvent * event = gameeventmanager->CreateEvent( "instructor_server_hint_stop", false );
	if ( event )
	{
		event->SetString( "hint_name", GetEntityName().ToCStr() );

		gameeventmanager->FireEvent( event );

#ifdef MAPBASE
		m_flActiveUntil = -1.0f;
		m_hActivator = NULL;
		m_hTarget = NULL;
		m_bFilterByActivator = false;
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: A generic target entity that gets replicated to the client for instructor hint targetting
//-----------------------------------------------------------------------------
class CInfoInstructorHintTarget : public CPointEntity
{
public:
	DECLARE_CLASS( CInfoInstructorHintTarget, CPointEntity );

	virtual int UpdateTransmitState( void )	// set transmit filter to transmit always
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( info_target_instructor_hint, CInfoInstructorHintTarget );

BEGIN_DATADESC( CInfoInstructorHintTarget )

END_DATADESC()
