//========= Copyright Valve Corporation, All rights reserved. ============//
//
//----------------------------------------------------------------------------------------

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replaymessagepanel.h"
#include "vgui_controls/CheckButton.h"
#include "ienginevgui.h"
#include "vgui_controls/PHandle.h"
#include "econ/econ_controls.h"
#if defined( CSTRIKE_DLL )
#  include "cstrike/clientmode_csnormal.h"
#elif defined( TF_CLIENT_DLL )
#  include "tf/clientmode_tf.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

#if _DEBUG
CON_COMMAND( testreplaymessagepanel, "" )
{
	CReplayMessagePanel *pPanel = new CReplayMessagePanel( "#Replay_StartRecord", replay_msgduration_misc.GetFloat(), rand()%2==0);
	pPanel->Show();
}

CON_COMMAND( testreplaymessagedlg, "" )
{
	CReplayMessageDlg *pPanel = SETUP_PANEL( new CReplayMessageDlg( "text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text." ) );
	pPanel->SetVisible( true );
	pPanel->MakePopup();
	pPanel->MoveToFront();
	pPanel->SetKeyBoardInputEnabled( true );
	pPanel->SetMouseInputEnabled( true );
	pPanel->RequestFocus();
	engine->ClientCmd_Unrestricted( "gameui_hide" );
}
#endif

//-----------------------------------------------------------------------------

using namespace vgui;

//-----------------------------------------------------------------------------

typedef vgui::DHANDLE< CReplayMessagePanel > ReplayMessagePanelHandle_t;
static CUtlVector< ReplayMessagePanelHandle_t > g_vecReplayMessagePanels;

//-----------------------------------------------------------------------------

ConVar replay_msgduration_startrecord( "replay_msgduration_startrecord", "6", FCVAR_DONTRECORD, "Duration for start record message.", true, 0.0f, true, 10.0f );
ConVar replay_msgduration_stoprecord( "replay_msgduration_stoprecord", "6", FCVAR_DONTRECORD, "Duration for stop record message.", true, 0.0f, true, 10.0f );
ConVar replay_msgduration_replaysavailable( "replay_msgduration_replaysavailable", "6", FCVAR_DONTRECORD, "Duration for replays available message.", true, 0.0f, true, 10.0f );
ConVar replay_msgduration_error( "replay_msgduration_error", "6", FCVAR_DONTRECORD, "Duration for replays available message.", true, 0.0f, true, 10.0f );
ConVar replay_msgduration_misc( "replay_msgduration_misc", "5", FCVAR_DONTRECORD, "Duration for misc replays messages (server errors and such).", true, 0.0f, true, 10.0f );
ConVar replay_msgduration_connectrecording( "replay_msgduration_connectrecording", "8", FCVAR_DONTRECORD, "Duration for the message that pops up when you connect to a server already recording replays.", true, 0.0f, true, 15.0f );

//-----------------------------------------------------------------------------

CReplayMessageDlg::CReplayMessageDlg( const char *pText )
:	BaseClass( NULL, "ReplayMessageDlg" ),
	m_pOKButton( NULL ),
	m_pDlg( NULL ),
	m_pMsgLabel( NULL )
{
	InvalidateLayout( true, true );

	m_pMsgLabel->SetText( pText );
}

CReplayMessageDlg::~CReplayMessageDlg()
{
}

void CReplayMessageDlg::ApplySchemeSettings( IScheme *pScheme )
{
	// Link in TF scheme
	extern IEngineVGui *enginevgui;
	vgui::HScheme pTFScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	SetScheme( pTFScheme );
	SetProportional( true );

	BaseClass::ApplySchemeSettings( vgui::scheme()->GetIScheme( pTFScheme ) );

	LoadControlSettings( "resource/ui/replaymessagedlg.res", "GAME" );

	m_pDlg = FindChildByName( "Dlg" );

	m_pOKButton = dynamic_cast< CExButton * >( m_pDlg->FindChildByName( "OKButton" ) );
	m_pMsgLabel = dynamic_cast< CExLabel * >( m_pDlg->FindChildByName( "TextLabel") );

	m_pOKButton->AddActionSignalTarget( this );
}

void CReplayMessageDlg::PerformLayout()
{
	BaseClass::PerformLayout();

	SetWide( ScreenWidth() );
	SetTall( ScreenHeight() );

	// Center dlg on screen
	m_pDlg->SetPos( ( ScreenWidth() - m_pDlg->GetWide() ) / 2, ( ScreenHeight() - m_pDlg->GetTall() ) / 2 );

	// Position OK below text label, centered horizontally
	int nButtonX = XRES(13);
	int nButtonY = m_pDlg->GetTall() - m_pOKButton->GetTall() - YRES( 10 );
	m_pOKButton->SetPos( nButtonX, nButtonY );
}

void CReplayMessageDlg::Close()
{
	// Hide / delete / hide game UI
	SetVisible( false );
	MarkForDeletion();
	engine->ClientCmd_Unrestricted( "gameui_hide" );
}

void CReplayMessageDlg::OnCommand( const char *pCommand )
{
	if ( FStrEq( pCommand, "close" ) )
	{
		Close();
	}

	BaseClass::OnCommand( pCommand );
}

void CReplayMessageDlg::OnKeyCodeTyped( KeyCode nCode )
{
	switch ( nCode )
	{
	case KEY_ESCAPE:
	case KEY_SPACE:
	case KEY_ENTER:
		Close();
		return;
	}

	BaseClass::OnKeyCodeTyped( nCode );
}

//-----------------------------------------------------------------------------

int CReplayMessagePanel::InstanceCount()
{
	return g_vecReplayMessagePanels.Count();
}

void CReplayMessagePanel::RemoveAll()
{
	FOR_EACH_VEC( g_vecReplayMessagePanels, i )
	{
		CReplayMessagePanel *pCurPanel = g_vecReplayMessagePanels[ i ];
		pCurPanel->MarkForDeletion();
	}

	g_vecReplayMessagePanels.RemoveAll();
}

//-----------------------------------------------------------------------------

ReplayMessagePanelHandle_t GetReplayMessagePanelHandle( CReplayMessagePanel *pPanel )
{
	ReplayMessagePanelHandle_t hThis;
	hThis = pPanel;
	return hThis;
}

CReplayMessagePanel::CReplayMessagePanel( const char *pLocalizeName, float flDuration, bool bUrgent )
:	EditablePanel( g_pClientMode->GetViewport(), "ReplayMessagePanel" ),
	m_bUrgent( bUrgent )
{
	m_flShowStartTime = 0;
	m_flShowDuration = flDuration;

	m_pMessageLabel = new CExLabel( this, "MessageLabel", pLocalizeName );
	m_pReplayLabel = new CExLabel( this, "ReplayLabel", "" );
	m_pIcon = new ImagePanel( this, "Icon" );

#if defined( TF_CLIENT_DLL )
	const char *pBorderName = bUrgent ? "ReplayFatLineBorderRedBGOpaque" : "ReplayFatLineBorderOpaque";
	V_strncpy( m_szBorderName, pBorderName, sizeof( m_szBorderName ) );
#endif

	g_vecReplayMessagePanels.AddToTail( GetReplayMessagePanelHandle( const_cast< CReplayMessagePanel * >( this ) ) );

	InvalidateLayout( true, true );

	ivgui()->AddTickSignal( GetVPanel(), 10 );
}

CReplayMessagePanel::~CReplayMessagePanel()
{
	// CUtlVector<>::Find() vomits.
	int iFind = g_vecReplayMessagePanels.InvalidIndex();
	FOR_EACH_VEC( g_vecReplayMessagePanels, i )
	{
		if ( g_vecReplayMessagePanels[ i ].Get() == this )
		{
			iFind = i;
		}
	}

	// Remove, if found.
	if ( iFind != g_vecReplayMessagePanels.InvalidIndex() )
	{
		g_vecReplayMessagePanels.FastRemove( iFind );	
	}

	ivgui()->RemoveTickSignal( GetVPanel() );
}

void CReplayMessagePanel::Show()
{
	m_pMessageLabel->SetVisible( true );

	// Setup start time
	m_flShowStartTime = gpGlobals->curtime;

	m_pMessageLabel->MoveToFront();

	SetAlpha( 0 );
}

inline float LerpScale( float flIn, float flInMin, float flInMax, float flOutMin, float flOutMax )
{
	float flDenom = flInMax - flInMin;
	if ( flDenom == 0.0f )
		return 0.0f;

	float t = clamp( ( flIn - flInMin ) / flDenom, 0.0f, 1.0f );
	return Lerp( t, flOutMin, flOutMax );
}

inline float SCurve( float t )
{
	t = clamp( t, 0.0f, 1.0f );
	return t * t * (3 - 2*t);
}

void CReplayMessagePanel::OnTick()
{
	// Hide if taking screenshot
	extern ConVar hud_freezecamhide;
	extern bool IsTakingAFreezecamScreenshot();
	if ( hud_freezecamhide.GetBool() && IsTakingAFreezecamScreenshot() )
	{
		SetVisible( false );
		return;
	}

	// Delete the panel if life exceeded
	const float flEndTime = m_flShowStartTime + m_flShowDuration;
	if ( gpGlobals->curtime >= flEndTime )
	{
		SetVisible( false );
		MarkForDeletion();
		return;
	}

	SetVisible( true );

	const float flFadeDuration = .4f;
	float flAlpha;

	// Fade out?
	if ( gpGlobals->curtime >= flEndTime - flFadeDuration )
	{
		flAlpha = LerpScale( gpGlobals->curtime, flEndTime - flFadeDuration, flEndTime, 1.0f, 0.0f );
	}

	// Fade in?
	else if ( gpGlobals->curtime <= m_flShowStartTime + flFadeDuration )
	{
		flAlpha = LerpScale( gpGlobals->curtime, m_flShowStartTime, m_flShowStartTime + flFadeDuration, 0.0f, 1.0f );
	}

	// Otherwise, we must be in between fade in/fade out
	else
	{
		flAlpha = 1.0f;
	}

	SetAlpha( 255 * SCurve( flAlpha ) );
}

void CReplayMessagePanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replaymessage.res", "GAME" );

#if defined( CSTRIKE_DLL )
	SetPaintBackgroundEnabled( true );
	SetPaintBorderEnabled( false );
	SetPaintBackgroundType( 0 );
	SetBgColor( pScheme->GetColor( m_bUrgent ? "DarkRed" : "DarkGray", Color( 255, 255, 255, 255 ) ) );
#endif
}

void CReplayMessagePanel::PerformLayout()
{
	BaseClass::PerformLayout();

#if defined( TF_CLIENT_DLL )
	// Set the border if one was specified
	if ( m_szBorderName[0] )
	{
		SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( m_szBorderName ) );
	}
#endif

	// Adjust overall panel size depending on min-mode
#if defined( TF_CLIENT_DLL )
	extern ConVar cl_hud_minmode;
	bool bMinMode = cl_hud_minmode.GetBool();
#else
	bool bMinMode = false;
#endif
	int nVerticalBuffer = bMinMode ? YRES(3) : YRES(5);
	int nMessageLabelY = nVerticalBuffer;
	int nVerticalOffsetBetweenPanels = YRES(6);

	// Only display replay icon and "replay" label if this is the top-most (vertically) panel
	// and we're not in min-mode
	Assert( InstanceCount() > 0 );
	if ( !InstanceCount() || bMinMode || g_vecReplayMessagePanels[ 0 ].Get() != this )
	{
		m_pIcon->SetTall( 0 );
		m_pReplayLabel->SetTall( 0 );
		nVerticalOffsetBetweenPanels = YRES(1);
	}
	else
	{
		m_pReplayLabel->SizeToContents();
		nMessageLabelY += m_pReplayLabel->GetTall();
		nVerticalOffsetBetweenPanels = YRES(6);
	}

	// Resize the message label to fit the text
	m_pMessageLabel->SizeToContents();

	// Adjust this panel's height to fit the label size
	SetTall( nMessageLabelY + m_pMessageLabel->GetTall() + nVerticalBuffer );

	// Set the message label's position
	m_pMessageLabel->SetPos( XRES(8), nMessageLabelY );

	// Get the bottom of the bottom-most message panel
	int nMaxY = 0;
	FOR_EACH_VEC( g_vecReplayMessagePanels, it )
	{
		CReplayMessagePanel *pPanel = g_vecReplayMessagePanels[ it ];
		if ( pPanel == this )
			continue;

		int nX, nY;
		pPanel->GetPos( nX, nY );
		nMaxY = MAX( nMaxY, pPanel->GetTall() + nY );
	}

	// Adjust this panel's position to be below bottom-most panel
	// NOTE: Intentionally using YRES() for xpos, since we want to match offsets in both x & y margins
	SetPos( YRES(6), nMaxY + nVerticalOffsetBetweenPanels );
}

//-----------------------------------------------------------------------------

#endif
