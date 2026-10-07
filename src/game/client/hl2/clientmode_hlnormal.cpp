//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws the normal TF2 or HL2 HUD.
//
//=============================================================================
#include "cbase.h"
#include "clientmode_hlnormal.h"
#include "eternalcity_hud.h"
#include "c_baseplayer.h"
#include "view_shared.h"
#include "usercmd.h"
#include "vgui_int.h"
#include "hud.h"
#include "hud_element_helper.h"
#include "hud_chat.h"
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include "iinput.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool g_bRollingCredits;

ConVar fov_desired( "fov_desired", "75", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets the base field-of-view.", true, 75.0, true, 90.0 );

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
vgui::HScheme g_hVGuiCombineScheme = 0;


// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
	static ClientModeHLNormal g_ClientModeNormal;
	return &g_ClientModeNormal;
}


//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public CBaseViewport
{
private:
	DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		gHUD.InitColors( pScheme );

		SetPaintBackgroundEnabled( false );
	}

	virtual void CreateDefaultPanels( void ) { /* don't create any panels yet*/ };
};


//-----------------------------------------------------------------------------
// ClientModeHLNormal implementation
//-----------------------------------------------------------------------------
ClientModeHLNormal::ClientModeHLNormal()
	: m_pEternalCityHud( NULL ),
	  m_bOwnsThirdPerson( false )
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeHLNormal::~ClientModeHLNormal()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeHLNormal::Init()
{
	BaseClass::Init();
	if ( !m_pEternalCityHud )
		m_pEternalCityHud = GET_HUDELEMENT( CEternalCityHud );

	// Load up the combine control panel scheme
	g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), IsXbox() ? "resource/ClientScheme.res" : "resource/CombinePanelScheme.res", "CombineScheme" );
	if ( g_hVGuiCombineScheme && m_pEternalCityHud )
		m_pEternalCityHud->SetScheme( g_hVGuiCombineScheme );
	if (!g_hVGuiCombineScheme)
	{
		Warning( "Couldn't load combine panel scheme!\n" );
	}
}

bool ClientModeHLNormal::ShouldDrawCrosshair( void )
{
	return ( g_bRollingCredits == false );
}

void ClientModeHLNormal::ReleaseEternalCityCamera()
{
	if ( m_bOwnsThirdPerson )
	{
		::input->CAM_ToFirstPerson();
		m_bOwnsThirdPerson = false;
	}
}

void ClientModeHLNormal::LevelShutdown( void )
{
	ReleaseEternalCityCamera();
	BaseClass::LevelShutdown();
}

bool ClientModeHLNormal::ShouldUseEternalCityCamera() const
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	return pPlayer && pPlayer->IsAlive() && !pPlayer->IsInAVehicle() && pPlayer->GetObserverMode() == OBS_MODE_NONE;
}

bool ClientModeHLNormal::ShouldDrawLocalPlayer( C_BasePlayer *pPlayer )
{
	if ( pPlayer == C_BasePlayer::GetLocalPlayer() && ShouldUseEternalCityCamera() )
		return true;

	return BaseClass::ShouldDrawLocalPlayer( pPlayer );
}

bool ClientModeHLNormal::ShouldDrawViewModel()
{
	return ShouldUseEternalCityCamera() ? false : BaseClass::ShouldDrawViewModel();
}

void ClientModeHLNormal::OverrideView( CViewSetup *pSetup )
{
	if ( !ShouldUseEternalCityCamera() )
	{
		ReleaseEternalCityCamera();
		BaseClass::OverrideView( pSetup );
		return;
	}

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !::input->CAM_IsThirdPerson() )
	{
		::input->CAM_ToThirdPerson();
		pPlayer->ThirdPersonSwitch( true );
		m_bOwnsThirdPerson = true;
	}
	BaseClass::OverrideView( pSetup );

	const float cameraDistance = 420.0f;
	const QAngle cameraAngles( 55.0f, 0.0f, 0.0f );
	Vector cameraForward;
	AngleVectors( cameraAngles, &cameraForward );

	const Vector cameraTarget = pPlayer->WorldSpaceCenter();
	const Vector desiredOrigin = cameraTarget - cameraForward * cameraDistance;
	trace_t trace;
	UTIL_TraceHull(
		cameraTarget,
		desiredOrigin,
		Vector( -4.0f, -4.0f, -4.0f ),
		Vector( 4.0f, 4.0f, 4.0f ),
		MASK_SOLID,
		pPlayer,
		COLLISION_GROUP_NONE,
		&trace );

	if ( trace.startsolid )
		pSetup->origin = cameraTarget - cameraForward * 16.0f;
	else if ( trace.DidHit() )
		pSetup->origin = trace.endpos + trace.plane.normal * 2.0f;
	else
		pSetup->origin = desiredOrigin;

	pSetup->angles = cameraAngles;
}

bool ClientModeHLNormal::CreateMove( float flInputSampleTime, CUserCmd *cmd )
{
	if ( m_pEternalCityHud && m_pEternalCityHud->IsModalOpen() )
	{
		cmd->forwardmove = 0.0f;
		cmd->sidemove = 0.0f;
		cmd->upmove = 0.0f;
		cmd->buttons = 0;
		cmd->impulse = 0;
		cmd->weaponselect = 0;
		cmd->weaponsubtype = 0;
		return true;
	}

	return BaseClass::CreateMove( flInputSampleTime, cmd );
}

int ClientModeHLNormal::KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	CHudChat *pChat = GET_HUDELEMENT( CHudChat );
	if ( engine->Con_IsVisible() || enginevgui->IsGameUIVisible() ||
		( pChat && pChat->GetMessageMode() != MM_NONE ) )
		return BaseClass::KeyInput( down, keynum, pszCurrentBinding );

	if ( down && m_pEternalCityHud && m_pEternalCityHud->HandleShortcut( keynum, pszCurrentBinding ) )
		return 0;

	if ( m_pEternalCityHud && m_pEternalCityHud->IsModalOpen() )
	{
		if ( down && keynum == KEY_ESCAPE )
		{
			m_pEternalCityHud->CloseModal();
			return 0;
		}
		if ( !down )
			return BaseClass::KeyInput( down, keynum, pszCurrentBinding );
		return 0;
	}

	return BaseClass::KeyInput( down, keynum, pszCurrentBinding );
}
