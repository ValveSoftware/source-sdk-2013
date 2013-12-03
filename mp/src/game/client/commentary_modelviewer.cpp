//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "vgui/IInput.h"
#include <vgui/IVGui.h>
#include "commentary_modelviewer.h"
#include "iclientmode.h"
#include "baseviewport.h"

DECLARE_BUILD_FACTORY( CCommentaryModelPanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCommentaryModelPanel::CCommentaryModelPanel( vgui::Panel *parent, const char *name ) : CModelPanel( parent, name )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCommentaryModelViewer::CCommentaryModelViewer(IViewPort *pViewPort) : Frame(NULL, PANEL_COMMENTARY_MODELVIEWER )
{
	m_pViewPort = pViewPort;
	m_pModelPanel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCommentaryModelViewer::~CCommentaryModelViewer()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCommentaryModelViewer::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/CommentaryModelViewer.res" );

	m_pModelPanel = dynamic_cast<CCommentaryModelPanel*>( FindChildByName( "modelpanel" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCommentaryModelViewer::PerformLayout( void ) 
{
	int w,h;
	GetParent()->GetSize( w, h );
	SetBounds(0,0,w,h);

	if ( m_pModelPanel )
	{
		m_pModelPanel->SetBounds(0,0,w,h);
	}

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCommentaryModelViewer::SetModel( const char *pszName, const char *pszAttached )
{
	if ( !m_pModelPanel )
		return;

	m_pModelPanel->SwapModel( pszName, pszAttached );

	m_flYawSpeed = 0;
	m_flZoomSpeed = 0;
	m_bTranslating = false;

	m_vecResetPos = m_pModelPanel->m_pModelInfo->m_vecOriginOffset;
	m_vecResetAngles = m_pModelPanel->m_pModelInfo->m_vecAbsAngles;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCommentaryModelViewer::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}

	m_pViewPort->ShowBackGround( bShow );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCommentaryModelViewer::OnCommand( const char *command )
{
	if ( Q_stricmp( command, "vguicancel" ) )
	{
		engine->ClientCmd( const_cast<char *>( command ) );
	}

	Close();
	m_pViewPort->ShowBackGround( false );

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCommentaryModelViewer::OnKeyCodePressed( vgui::KeyCode code )
{
	if ( code == KEY_ENTER )
	{
		Close();
		m_pViewPort->ShowBackGround( false );
	}
	else if ( code == KEY_SPACE )
	{
		m_bTranslating = !m_bTranslating;
	}
	else if ( code == KEY_R )
	{
		m_pModelPanel->m_pModelInfo->m_vecOriginOffset = m_vecResetPos;
		m_pModelPanel->m_pModelInfo->m_vecAbsAngles = m_vecResetAngles;
		m_flYawSpeed = 0;
		m_flZoomSpeed = 0;

		m_pModelPanel->ZoomToFrameDistance();
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCommentaryModelViewer::OnThink( void )
{
	HandleMovementInput();

	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCommentaryModelViewer::HandleMovementInput( void )
{
	bool bLeftDown = input()->IsKeyDown(KEY_LEFT);
	bool bRightDown = input()->IsKeyDown(KEY_RIGHT);
	bool bForwardDown = input()->IsKeyDown(KEY_UP);
	bool bBackDown = input()->IsKeyDown(KEY_DOWN);

	float flAccel = 0.05;

	// Rotation around Z
	if ( bLeftDown )
	{
		if ( m_flYawSpeed > 0 ) 
		{
			m_flYawSpeed = 0;
		}
		m_flYawSpeed = MAX(m_flYawSpeed-flAccel, -3.0);
	}
	else if ( bRightDown )
	{
		if ( m_flYawSpeed < 0 ) 
		{
			m_flYawSpeed = 0;
		}
		m_flYawSpeed = MIN(m_flYawSpeed+flAccel, 3.0);
	}
	if ( m_flYawSpeed != 0 )
	{
		if ( m_bTranslating ) 
		{
			m_pModelPanel->m_pModelInfo->m_vecOriginOffset.y = clamp( m_pModelPanel->m_pModelInfo->m_vecOriginOffset.y + m_flYawSpeed, -100.f, 100.f );
		}
		else
		{
			m_pModelPanel->m_pModelInfo->m_vecAbsAngles.y = anglemod( m_pModelPanel->m_pModelInfo->m_vecAbsAngles.y + m_flYawSpeed );
		}

		if ( !bLeftDown && !bRightDown )
		{
			m_flYawSpeed = ( m_flYawSpeed > 0 ) ? MAX(0,m_flYawSpeed-0.1) : MIN(0,m_flYawSpeed+0.1);
		}
	}

	// Zooming
	if ( bForwardDown )
	{
		if ( m_flZoomSpeed > 0 ) 
		{
			m_flZoomSpeed = 0;
		}
		m_flZoomSpeed = MAX(m_flZoomSpeed-flAccel, -3.0);
	}
	else if ( bBackDown )
	{
		if ( m_flZoomSpeed < 0 ) 
		{
			m_flZoomSpeed = 0;
		}
		m_flZoomSpeed = MIN(m_flZoomSpeed+flAccel, 3.0);
	}
	if ( m_flZoomSpeed != 0 )
	{
		if ( m_bTranslating ) 
		{
			m_pModelPanel->m_pModelInfo->m_vecOriginOffset.z = clamp( m_pModelPanel->m_pModelInfo->m_vecOriginOffset.z + m_flZoomSpeed, -100.f, 300.f );
		}
		else
		{
			float flZoomMin = m_pModelPanel->m_flFrameDistance * 0.75;
			float flZoomMax = m_pModelPanel->m_flFrameDistance * 1.5;
			m_pModelPanel->m_pModelInfo->m_vecOriginOffset.x = clamp( m_pModelPanel->m_pModelInfo->m_vecOriginOffset.x + m_flZoomSpeed, flZoomMin, flZoomMax );
		}

		if ( !bForwardDown && !bBackDown )
		{
			m_flZoomSpeed = ( m_flZoomSpeed > 0 ) ? MAX(0,m_flZoomSpeed-0.1) : MIN(0,m_flZoomSpeed+0.1);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CommentaryShowModelViewer( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		ConMsg( "Usage: commentary_showmodelviewer <model name> <optional attached model name>\n" );
		return;
	}

	CBaseViewport *pViewport = dynamic_cast<CBaseViewport *>( g_pClientMode->GetViewport() );
	if ( pViewport )
	{
		IViewPortPanel *pCommentaryPanel = pViewport->FindPanelByName( PANEL_COMMENTARY_MODELVIEWER );
		if ( !pCommentaryPanel )
		{
			pCommentaryPanel = pViewport->CreatePanelByName( PANEL_COMMENTARY_MODELVIEWER );
			pViewport->AddNewPanel( pCommentaryPanel, "PANEL_COMMENTARY_MODELVIEWER" );
		}

		if ( pCommentaryPanel )
		{
			//((CCommentaryModelViewer*)pCommentaryPanel)->InvalidateLayout( false, true );
			((CCommentaryModelViewer*)pCommentaryPanel)->SetModel( args[1], args[2] );
			pViewport->ShowPanel( pCommentaryPanel, true );
		}
	}
}

ConCommand commentary_showmodelviewer( "commentary_showmodelviewer", CommentaryShowModelViewer, "Display the commentary model viewer. Usage: commentary_showmodelviewer <model name> <optional attached model name>", FCVAR_NONE );
