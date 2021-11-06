//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Color correction entity.
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "colorcorrection.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define COLOR_CORRECTION_ENT_THINK_RATE TICK_INTERVAL

static const char *s_pFadeInContextThink = "ColorCorrectionFadeInThink";
static const char *s_pFadeOutContextThink = "ColorCorrectionFadeOutThink";

LINK_ENTITY_TO_CLASS(color_correction, CColorCorrection);

BEGIN_DATADESC( CColorCorrection )

	DEFINE_THINKFUNC( FadeInThink ),
	DEFINE_THINKFUNC( FadeOutThink ),

	DEFINE_FIELD( m_flCurWeight,	      FIELD_FLOAT ),
	DEFINE_FIELD( m_flTimeStartFadeIn,	  FIELD_FLOAT ),
	DEFINE_FIELD( m_flTimeStartFadeOut,	  FIELD_FLOAT ),
	DEFINE_FIELD( m_flStartFadeInWeight,  FIELD_FLOAT ),
	DEFINE_FIELD( m_flStartFadeOutWeight, FIELD_FLOAT ),

	DEFINE_KEYFIELD( m_MinFalloff,		  FIELD_FLOAT,   "minfalloff" ),
	DEFINE_KEYFIELD( m_MaxFalloff,		  FIELD_FLOAT,   "maxfalloff" ),
	DEFINE_KEYFIELD( m_flMaxWeight,		  FIELD_FLOAT,	 "maxweight" ),
	DEFINE_KEYFIELD( m_flFadeInDuration,  FIELD_FLOAT,	 "fadeInDuration" ),
	DEFINE_KEYFIELD( m_flFadeOutDuration,  FIELD_FLOAT,	 "fadeOutDuration" ),
	DEFINE_KEYFIELD( m_lookupFilename,	  FIELD_STRING,  "filename" ),

	DEFINE_KEYFIELD( m_bEnabled,		  FIELD_BOOLEAN, "enabled" ),
	DEFINE_KEYFIELD( m_bStartDisabled,    FIELD_BOOLEAN, "StartDisabled" ),
#ifdef MAPBASE // From Alien Swarm SDK
	DEFINE_KEYFIELD( m_bExclusive,		  FIELD_BOOLEAN, "exclusive" ),
#endif
//	DEFINE_ARRAY( m_netlookupFilename, FIELD_CHARACTER, MAX_PATH ), 

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFadeInDuration", InputSetFadeInDuration ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFadeOutDuration", InputSetFadeOutDuration ),
#ifdef MAPBASE
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMinFalloff", InputSetMinFalloff ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMaxFalloff", InputSetMaxFalloff ),
#endif

END_DATADESC()

extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
IMPLEMENT_SERVERCLASS_ST_NOBASE(CColorCorrection, DT_ColorCorrection)
	SendPropVector( SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropFloat(  SENDINFO(m_MinFalloff) ),
	SendPropFloat(  SENDINFO(m_MaxFalloff) ),
	SendPropFloat(  SENDINFO(m_flCurWeight) ),
#ifdef MAPBASE // From Alien Swarm SDK
	SendPropFloat(  SENDINFO(m_flMaxWeight) ),
	SendPropFloat(  SENDINFO(m_flFadeInDuration) ),
	SendPropFloat(  SENDINFO(m_flFadeOutDuration) ),
#endif
	SendPropString( SENDINFO(m_netlookupFilename) ),
	SendPropBool( SENDINFO(m_bEnabled) ),
#ifdef MAPBASE // From Alien Swarm SDK
	SendPropBool( SENDINFO(m_bMaster) ),
	SendPropBool( SENDINFO(m_bClientSide) ),
	SendPropBool( SENDINFO(m_bExclusive) ),
#endif
END_SEND_TABLE()


CColorCorrection::CColorCorrection() : BaseClass()
{
	m_bEnabled = true;
	m_MinFalloff = 0.0f;
	m_MaxFalloff = 1000.0f;
	m_flMaxWeight = 1.0f;
	m_flCurWeight.Set( 0.0f );
	m_flFadeInDuration = 0.0f;
	m_flFadeOutDuration = 0.0f;
	m_flStartFadeInWeight = 0.0f;
	m_flStartFadeOutWeight = 0.0f;
	m_flTimeStartFadeIn = 0.0f;
	m_flTimeStartFadeOut = 0.0f;
	m_netlookupFilename.GetForModify()[0] = 0;
	m_lookupFilename = NULL_STRING;
#ifdef MAPBASE // From Alien Swarm SDK
	m_bMaster = false;
	m_bClientSide = false;
	m_bExclusive = false;
#endif
}


//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model
//------------------------------------------------------------------------------
int CColorCorrection::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CColorCorrection::Spawn( void )
{
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT | EFL_DIRTY_ABSTRANSFORM );
	Precache();
	SetSolid( SOLID_NONE );

	// To fade in/out the weight.
	SetContextThink( &CColorCorrection::FadeInThink, TICK_NEVER_THINK, s_pFadeInContextThink );
	SetContextThink( &CColorCorrection::FadeOutThink, TICK_NEVER_THINK, s_pFadeOutContextThink );
	
	if( m_bStartDisabled )
	{
		m_bEnabled = false;
		m_flCurWeight.Set ( 0.0f );
	}
	else
	{
		m_bEnabled = true;
		m_flCurWeight.Set ( 1.0f );
	}

	BaseClass::Spawn();
}

void CColorCorrection::Activate( void )
{
	BaseClass::Activate();

#ifdef MAPBASE // From Alien Swarm SDK (moved to Activate() for save/restore support)
	m_bMaster = IsMaster();
	m_bClientSide = IsClientSide();
#endif

	Q_strncpy( m_netlookupFilename.GetForModify(), STRING( m_lookupFilename ), MAX_PATH );
}

//-----------------------------------------------------------------------------
// Purpose: Sets up internal vars needed for fade in lerping
//-----------------------------------------------------------------------------
void CColorCorrection::FadeIn ( void )
{
#ifdef MAPBASE // From Alien Swarm SDK
	if ( m_bClientSide || ( m_bEnabled && m_flCurWeight >= m_flMaxWeight ) )
		return;
#endif

	m_bEnabled = true;
	m_flTimeStartFadeIn = gpGlobals->curtime;
	m_flStartFadeInWeight = m_flCurWeight;
	SetNextThink ( gpGlobals->curtime + COLOR_CORRECTION_ENT_THINK_RATE, s_pFadeInContextThink );
}

//-----------------------------------------------------------------------------
// Purpose: Sets up internal vars needed for fade out lerping
//-----------------------------------------------------------------------------
void CColorCorrection::FadeOut ( void )
{
#ifdef MAPBASE // From Alien Swarm SDK
	if ( m_bClientSide || ( !m_bEnabled && m_flCurWeight <= 0.0f ) )
		return;
#endif

	m_bEnabled = false;
	m_flTimeStartFadeOut = gpGlobals->curtime;
	m_flStartFadeOutWeight = m_flCurWeight;
	SetNextThink ( gpGlobals->curtime + COLOR_CORRECTION_ENT_THINK_RATE, s_pFadeOutContextThink );
}

//-----------------------------------------------------------------------------
// Purpose: Fades lookup weight from CurWeight->MaxWeight
//-----------------------------------------------------------------------------
void CColorCorrection::FadeInThink( void )
{
	// Check for conditions where we shouldnt fade in
	if (		m_flFadeInDuration <= 0 ||  // not set to fade in
		 m_flCurWeight >= m_flMaxWeight ||  // already past max weight
							!m_bEnabled ||  // fade in/out mutex
				  m_flMaxWeight == 0.0f ||  // min==max
  m_flStartFadeInWeight >= m_flMaxWeight )  // already at max weight
	{
		SetNextThink ( TICK_NEVER_THINK, s_pFadeInContextThink );
		return;
	}
	
	// If we started fading in without fully fading out, use a truncated duration
    float flTimeToFade = m_flFadeInDuration;
	if ( m_flStartFadeInWeight > 0.0f )
	{	
		float flWeightRatio		= m_flStartFadeInWeight / m_flMaxWeight;
		flWeightRatio = clamp ( flWeightRatio, 0.0f, 0.99f );
		flTimeToFade			= m_flFadeInDuration * (1.0 - flWeightRatio);
	}	
	
	Assert ( flTimeToFade > 0.0f );
	float flFadeRatio = (gpGlobals->curtime - m_flTimeStartFadeIn) / flTimeToFade;
	flFadeRatio = clamp ( flFadeRatio, 0.0f, 1.0f );
	m_flStartFadeInWeight = clamp ( m_flStartFadeInWeight, 0.0f, 1.0f );

#ifdef MAPBASE
	m_flCurWeight = Lerp( flFadeRatio, m_flStartFadeInWeight, m_flMaxWeight.Get() );
#else
	m_flCurWeight = Lerp( flFadeRatio, m_flStartFadeInWeight, m_flMaxWeight );
#endif

	SetNextThink( gpGlobals->curtime + COLOR_CORRECTION_ENT_THINK_RATE, s_pFadeInContextThink );
}

//-----------------------------------------------------------------------------
// Purpose: Fades lookup weight from CurWeight->0.0 
//-----------------------------------------------------------------------------
void CColorCorrection::FadeOutThink( void )
{
	// Check for conditions where we shouldn't fade out
	if ( m_flFadeOutDuration <= 0 || // not set to fade out
			m_flCurWeight <= 0.0f || // already faded out
					   m_bEnabled || // fade in/out mutex
		   m_flMaxWeight == 0.0f  || // min==max
	 m_flStartFadeOutWeight <= 0.0f )// already at min weight
	{
		SetNextThink ( TICK_NEVER_THINK, s_pFadeOutContextThink );
		return;
	}

	// If we started fading out without fully fading in, use a truncated duration
    float flTimeToFade = m_flFadeOutDuration;
	if ( m_flStartFadeOutWeight < m_flMaxWeight )
	{	
		float flWeightRatio		= m_flStartFadeOutWeight / m_flMaxWeight;
		flWeightRatio = clamp ( flWeightRatio, 0.01f, 1.0f );
		flTimeToFade			= m_flFadeOutDuration * flWeightRatio;
	}	
	
	Assert ( flTimeToFade > 0.0f );
	float flFadeRatio = (gpGlobals->curtime - m_flTimeStartFadeOut) / flTimeToFade;
	flFadeRatio = clamp ( flFadeRatio, 0.0f, 1.0f );
	m_flStartFadeOutWeight = clamp ( m_flStartFadeOutWeight, 0.0f, 1.0f );

	m_flCurWeight = Lerp( 1.0f - flFadeRatio, 0.0f, m_flStartFadeOutWeight );

	SetNextThink( gpGlobals->curtime + COLOR_CORRECTION_ENT_THINK_RATE, s_pFadeOutContextThink );
}

//------------------------------------------------------------------------------
// Purpose : Input handlers
//------------------------------------------------------------------------------
void CColorCorrection::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;

	if ( m_flFadeInDuration > 0.0f )
	{
		FadeIn();
	}
	else
	{
		m_flCurWeight = m_flMaxWeight;
	}
	
}

void CColorCorrection::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;

	if ( m_flFadeOutDuration > 0.0f )
	{
		FadeOut();
	}
	else
	{
		m_flCurWeight = 0.0f;
	}
	
}

void CColorCorrection::InputSetFadeInDuration( inputdata_t& inputdata )
{
	m_flFadeInDuration = inputdata.value.Float();
}

void CColorCorrection::InputSetFadeOutDuration( inputdata_t& inputdata )
{
	m_flFadeOutDuration = inputdata.value.Float();
}

#ifdef MAPBASE
void CColorCorrection::InputSetMinFalloff( inputdata_t& inputdata )
{
	m_MinFalloff = inputdata.value.Float();
}

void CColorCorrection::InputSetMaxFalloff( inputdata_t& inputdata )
{
	m_MaxFalloff = inputdata.value.Float();
}
#endif

#ifdef MAPBASE // From Alien Swarm SDK
CColorCorrectionSystem s_ColorCorrectionSystem( "ColorCorrectionSystem" );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CColorCorrectionSystem *ColorCorrectionSystem( void )
{
	return &s_ColorCorrectionSystem;
}


//-----------------------------------------------------------------------------
// Purpose: Clear out the fog controller.
//-----------------------------------------------------------------------------
void CColorCorrectionSystem::LevelInitPreEntity( void )
{
	m_hMasterController = NULL;
	ListenForGameEvent( "round_start" );
}

//-----------------------------------------------------------------------------
// Purpose: Find the master controller.  If no controller is 
//			set as Master, use the first controller found.
//-----------------------------------------------------------------------------
void CColorCorrectionSystem::InitMasterController( void )
{
	CColorCorrection *pColorCorrection = NULL;
	do
	{
		pColorCorrection = static_cast<CColorCorrection*>( gEntList.FindEntityByClassname( pColorCorrection, "color_correction" ) );
		if ( pColorCorrection )
		{
			if ( m_hMasterController.Get() == NULL )
			{
				m_hMasterController = pColorCorrection;
			}
			else
			{
				if ( pColorCorrection->IsMaster() )
				{
					m_hMasterController = pColorCorrection;
				}
			}
		}
	} while ( pColorCorrection );
}

//-----------------------------------------------------------------------------
// Purpose: On a multiplayer map restart, re-find the master controller.
//-----------------------------------------------------------------------------
void CColorCorrectionSystem::FireGameEvent( IGameEvent *pEvent )
{
	InitMasterController();
}

//-----------------------------------------------------------------------------
// Purpose: On level load find the master fog controller.  If no controller is 
//			set as Master, use the first fog controller found.
//-----------------------------------------------------------------------------
void CColorCorrectionSystem::LevelInitPostEntity( void )
{
	InitMasterController();

	// HACK: Singleplayer games don't get a call to CBasePlayer::Spawn on level transitions.
	// CBasePlayer::Activate is called before this is called so that's too soon to set up the fog controller.
	// We don't have a hook similar to Activate that happens after LevelInitPostEntity
	// is called, or we could just do this in the player itself.
	if ( gpGlobals->maxClients == 1 )
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
		if ( pPlayer && ( pPlayer->m_hColorCorrectionCtrl.Get() == NULL ) )
		{
			pPlayer->InitColorCorrectionController();
		}
	}
}
#endif
