//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Color correction entity with simple radial falloff
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"

#include "c_colorcorrection.h"
#include "filesystem.h"
#include "cdll_client_int.h"
#include "colorcorrectionmgr.h"
#include "materialsystem/MaterialSystemUtil.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static ConVar mat_colcorrection_disableentities( "mat_colcorrection_disableentities", "0", FCVAR_NONE, "Disable map color-correction entities" );

#ifdef MAPBASE // From Alien Swarm SDK
static ConVar mat_colcorrection_forceentitiesclientside( "mat_colcorrection_forceentitiesclientside", "0", FCVAR_CHEAT, "Forces color correction entities to be updated on the client" );
#endif

IMPLEMENT_CLIENTCLASS_DT(C_ColorCorrection, DT_ColorCorrection, CColorCorrection)
	RecvPropVector( RECVINFO(m_vecOrigin) ),
	RecvPropFloat(  RECVINFO(m_minFalloff) ),
	RecvPropFloat(  RECVINFO(m_maxFalloff) ),
	RecvPropFloat(  RECVINFO(m_flCurWeight) ),
#ifdef MAPBASE // From Alien Swarm SDK
	RecvPropFloat(  RECVINFO(m_flMaxWeight) ),
	RecvPropFloat(  RECVINFO(m_flFadeInDuration) ),
	RecvPropFloat(  RECVINFO(m_flFadeOutDuration) ),
#endif
	RecvPropString( RECVINFO(m_netLookupFilename) ),
	RecvPropBool(   RECVINFO(m_bEnabled) ),
#ifdef MAPBASE // From Alien Swarm SDK
	RecvPropBool(   RECVINFO(m_bMaster) ),
	RecvPropBool(   RECVINFO(m_bClientSide) ),
	RecvPropBool(	RECVINFO(m_bExclusive) )
#endif

END_RECV_TABLE()


//------------------------------------------------------------------------------
// Constructor, destructor
//------------------------------------------------------------------------------
C_ColorCorrection::C_ColorCorrection()
{
#ifdef MAPBASE // From Alien Swarm SDK
	m_minFalloff = -1.0f;
	m_maxFalloff = -1.0f;
	m_flFadeInDuration = 0.0f;
	m_flFadeOutDuration = 0.0f;
	m_flCurWeight = 0.0f;
	m_flMaxWeight = 1.0f;
	m_netLookupFilename[0] = '\0';
	m_bEnabled = false;
	m_bMaster = false;
	m_bExclusive = false;
#endif
	m_CCHandle = INVALID_CLIENT_CCHANDLE;

#ifdef MAPBASE // From Alien Swarm SDK
	m_bFadingIn = false;
	m_flFadeStartWeight = 0.0f;
	m_flFadeStartTime = 0.0f;
	m_flFadeDuration = 0.0f;
#endif
}

C_ColorCorrection::~C_ColorCorrection()
{
#ifdef MAPBASE // From Alien Swarm SDK
	g_pColorCorrectionMgr->RemoveColorCorrectionEntity( this, m_CCHandle );
#else
	g_pColorCorrectionMgr->RemoveColorCorrection( m_CCHandle );
#endif
}

#ifdef MAPBASE // From Alien Swarm SDK
bool C_ColorCorrection::IsClientSide() const
{
	return m_bClientSide || mat_colcorrection_forceentitiesclientside.GetBool();
}
#endif

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_ColorCorrection::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( m_CCHandle == INVALID_CLIENT_CCHANDLE )
		{
#ifdef MAPBASE // From Alien Swarm SDK
			// forming a unique name without extension
			char cleanName[MAX_PATH];
			V_StripExtension( m_netLookupFilename, cleanName, sizeof( cleanName ) );
			char name[MAX_PATH];
			Q_snprintf( name, MAX_PATH, "%s_%d", cleanName, entindex() );

			m_CCHandle = g_pColorCorrectionMgr->AddColorCorrectionEntity( this, name, m_netLookupFilename );
#else
			char filename[MAX_PATH];
			Q_strncpy( filename, m_netLookupFilename, MAX_PATH );

			m_CCHandle = g_pColorCorrectionMgr->AddColorCorrection( filename );
			SetNextClientThink( ( m_CCHandle != INVALID_CLIENT_CCHANDLE ) ? CLIENT_THINK_ALWAYS : CLIENT_THINK_NEVER );
#endif
		}
	}
}

//------------------------------------------------------------------------------
// We don't draw...
//------------------------------------------------------------------------------
bool C_ColorCorrection::ShouldDraw()
{
	return false;
}

#ifdef MAPBASE // From Alien Swarm SDK
void C_ColorCorrection::Update( C_BasePlayer *pPlayer, float ccScale )
{
	Assert( m_CCHandle != INVALID_CLIENT_CCHANDLE );

	if ( mat_colcorrection_disableentities.GetInt() )
	{
		// Allow the colorcorrectionui panel (or user) to turn off color-correction entities
		g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, 0.0f, m_bExclusive );
		return;
	}

	// fade weight on client
	if ( IsClientSide() )
	{
		m_flCurWeight = Lerp( GetFadeRatio(), m_flFadeStartWeight, m_bFadingIn ? m_flMaxWeight : 0.0f );
	}

	if( !m_bEnabled && m_flCurWeight == 0.0f )
	{
		g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, 0.0f, m_bExclusive );
		return;
	}

	Vector playerOrigin = pPlayer->GetAbsOrigin();

	float weight = 0;
	if ( ( m_minFalloff != -1 ) && ( m_maxFalloff != -1 ) && m_minFalloff != m_maxFalloff )
	{
		float dist = (playerOrigin - m_vecOrigin).Length();
		weight = (dist-m_minFalloff) / (m_maxFalloff-m_minFalloff);
		if ( weight<0.0f ) weight = 0.0f;	
		if ( weight>1.0f ) weight = 1.0f;	
	}

	g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, m_flCurWeight * ( 1.0 - weight ) * ccScale, m_bExclusive );
}

void C_ColorCorrection::EnableOnClient( bool bEnable, bool bSkipFade )
{
	if ( !IsClientSide() )
	{
		return;
	}

	m_bFadingIn = bEnable;

	// initialize countdown timer
	m_flFadeStartWeight = m_flCurWeight;
	float flFadeTimeScale = 1.0f;
	if ( m_flMaxWeight != 0.0f )
	{
		flFadeTimeScale = m_flCurWeight / m_flMaxWeight;
	}

	if ( m_bFadingIn )
	{
		flFadeTimeScale = 1.0f - flFadeTimeScale;
	}

	if ( bSkipFade )
	{
		flFadeTimeScale = 0.0f;
	}

	StartFade( flFadeTimeScale * ( m_bFadingIn ? m_flFadeInDuration : m_flFadeOutDuration ) );

	// update the clientside weight once here, in case the fade duration is 0
	m_flCurWeight = Lerp( GetFadeRatio(), m_flFadeStartWeight, m_bFadingIn ? m_flMaxWeight : 0.0f );
}

Vector C_ColorCorrection::GetOrigin()
{
	return m_vecOrigin;
}

float C_ColorCorrection::GetMinFalloff()
{
	return m_minFalloff;
}

float C_ColorCorrection::GetMaxFalloff()
{
	return m_maxFalloff;
}

void C_ColorCorrection::SetWeight( float fWeight )
{
	g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, fWeight, false );
}

void C_ColorCorrection::StartFade( float flDuration )
{
	m_flFadeStartTime = gpGlobals->curtime;
	m_flFadeDuration = MAX( flDuration, 0.0f );
}

float C_ColorCorrection::GetFadeRatio() const
{
	float flRatio = 1.0f;
	
	if ( m_flFadeDuration != 0.0f )
	{
		flRatio = ( gpGlobals->curtime - m_flFadeStartTime ) / m_flFadeDuration;
		flRatio = clamp( flRatio, 0.0f, 1.0f );
	}
	return flRatio;
}

bool C_ColorCorrection::IsFadeTimeElapsed() const
{
	return	( ( gpGlobals->curtime - m_flFadeStartTime ) > m_flFadeDuration ) ||
			( ( gpGlobals->curtime - m_flFadeStartTime ) < 0.0f );
}

void UpdateColorCorrectionEntities( C_BasePlayer *pPlayer, float ccScale, C_ColorCorrection **pList, int listCount )
{
	for ( int i = 0; i < listCount; i++ )
	{
		pList[i]->Update(pPlayer, ccScale);
	}
}
#else
void C_ColorCorrection::ClientThink()
{
	if ( m_CCHandle == INVALID_CLIENT_CCHANDLE )
		return;

	if ( mat_colcorrection_disableentities.GetInt() )
	{
		// Allow the colorcorrectionui panel (or user) to turn off color-correction entities
		g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, 0.0f );
		return;
	}

	if( !m_bEnabled && m_flCurWeight == 0.0f )
	{
		g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, 0.0f );
		return;
	}

	C_BaseEntity *pPlayer = C_BasePlayer::GetLocalPlayer();
	if( !pPlayer )
		return;

	Vector playerOrigin = pPlayer->GetAbsOrigin();

	float weight = 0;
	if ( ( m_minFalloff != -1 ) && ( m_maxFalloff != -1 ) && m_minFalloff != m_maxFalloff )
	{
		float dist = (playerOrigin - m_vecOrigin).Length();
		weight = (dist-m_minFalloff) / (m_maxFalloff-m_minFalloff);
		if ( weight<0.0f ) weight = 0.0f;	
		if ( weight>1.0f ) weight = 1.0f;	
	}
	
	g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, m_flCurWeight * ( 1.0 - weight ) );

	BaseClass::ClientThink();
}
#endif













