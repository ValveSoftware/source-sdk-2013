//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Color correction entity with simple radial falloff
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"

#include "filesystem.h"
#include "cdll_client_int.h"
#include "colorcorrectionmgr.h"
#include "materialsystem/MaterialSystemUtil.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static ConVar mat_colcorrection_disableentities( "mat_colcorrection_disableentities", "0", FCVAR_NONE, "Disable map color-correction entities" );


//------------------------------------------------------------------------------
// Purpose : Color correction entity with radial falloff
//------------------------------------------------------------------------------
class C_ColorCorrection : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_ColorCorrection, C_BaseEntity );

	DECLARE_CLIENTCLASS();

	C_ColorCorrection();
	virtual ~C_ColorCorrection();

	void OnDataChanged(DataUpdateType_t updateType);
	bool ShouldDraw();

	void ClientThink();

private:
	Vector	m_vecOrigin;

	float	m_minFalloff;
	float	m_maxFalloff;
	float	m_flCurWeight;
	char	m_netLookupFilename[MAX_PATH];

	bool	m_bEnabled;

	ClientCCHandle_t m_CCHandle;
};

IMPLEMENT_CLIENTCLASS_DT(C_ColorCorrection, DT_ColorCorrection, CColorCorrection)
	RecvPropVector( RECVINFO(m_vecOrigin) ),
	RecvPropFloat(  RECVINFO(m_minFalloff) ),
	RecvPropFloat(  RECVINFO(m_maxFalloff) ),
	RecvPropFloat(  RECVINFO(m_flCurWeight) ),
	RecvPropString( RECVINFO(m_netLookupFilename) ),
	RecvPropBool(   RECVINFO(m_bEnabled) ),

END_RECV_TABLE()


//------------------------------------------------------------------------------
// Constructor, destructor
//------------------------------------------------------------------------------
C_ColorCorrection::C_ColorCorrection()
{
	m_CCHandle = INVALID_CLIENT_CCHANDLE;
}

C_ColorCorrection::~C_ColorCorrection()
{
	g_pColorCorrectionMgr->RemoveColorCorrection( m_CCHandle );
}


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
			char filename[MAX_PATH];
			Q_strncpy( filename, m_netLookupFilename, MAX_PATH );

			m_CCHandle = g_pColorCorrectionMgr->AddColorCorrection( filename );
			SetNextClientThink( ( m_CCHandle != INVALID_CLIENT_CCHANDLE ) ? CLIENT_THINK_ALWAYS : CLIENT_THINK_NEVER );
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













