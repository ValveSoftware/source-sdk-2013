//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"
#include "game_controls/navigationpanel.h"
#include "econ/econ_controls.h"
#include "vgui_controls/Frame.h"
#include "vgui/IInput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

using namespace vgui;

//-----------------------------------------------------------------------------

DECLARE_BUILD_FACTORY( CNavigationPanel );

//-----------------------------------------------------------------------------

class CNavButton : public CExImageButton
{
	DECLARE_CLASS_SIMPLE( CNavButton, CExImageButton );
public:
	CNavButton( Panel *parent, const char *name, const char *text = "", Panel *pActionSignalTarget = NULL, const char *cmd = NULL )
	:	CExImageButton( parent, name, text, pActionSignalTarget, cmd ), m_iUserData( -1 ) {}

	CNavButton( Panel *parent, const char *name, const wchar_t *wszText = L"", Panel *pActionSignalTarget = NULL, const char *cmd = NULL )
	:	CExImageButton( parent, name, wszText, pActionSignalTarget, cmd ) , m_iUserData( -1 ) {}

	virtual void ApplySettings( KeyValues *pInResourceData )
	{
		BaseClass::ApplySettings( pInResourceData );

		if ( m_iUserData < 0 )
		{
			m_iUserData = pInResourceData->GetInt( "userdata", -1 );	AssertMsg( m_iUserData != -1, "Any messages sent for this button will have invalid user data!" );
		}
	}

	int m_iUserData;
};

//-----------------------------------------------------------------------------

CNavigationPanel::CNavigationPanel( Panel *pParent, const char *pName, bool bAddParentAsActionSignalTarget/*=true*/ )
:	BaseClass( pParent, pName ),
	m_bAutoLayout( false ),
	m_bAutoScale( true ),
	m_bDisplayVertical( false ),
	m_iSelectedButton( 0 ),
//	m_nAlignment( ALIGN_CENTER ),
	m_pKVButtonSettings( NULL )
{
	if ( bAddParentAsActionSignalTarget && pParent )
	{
		AddActionSignalTarget( pParent );
	}
}

CNavigationPanel::~CNavigationPanel()
{
}

void CNavigationPanel::AddButton( int iUserData, const char *pTextToken )
{
	const int i = m_vecButtons.Count();
	CNavButton *pNewButton = new CNavButton( this, CFmtStr( "button_%i", i ).Access(), pTextToken );
	pNewButton->m_iUserData = iUserData;
	pNewButton->InvalidateLayout( true, false );
	pNewButton->SetCommand( CFmtStr( "select_%i", i ).Access() );
	m_vecButtons.AddToTail( pNewButton );
}

CExImageButton *CNavigationPanel::GetButton( int index )
{
	return m_vecButtons[ index ];
}

void CNavigationPanel::ApplySettings( KeyValues *pInResourceData )
{
	BaseClass::ApplySettings( pInResourceData );

	/*
	const char *pAlignment = pInResourceData->GetString( "align", NULL );
	if ( pAlignment )
	{
		if ( !V_strnicmp( pAlignment, "west", 4 ) )
		{
			m_nAlignment = ALIGN_WEST;
		}
		else if ( !V_strnicmp( pAlignment, "center", 6 ) )
		{
			m_nAlignment = ALIGN_CENTER;
		}
		else if ( !V_strnicmp( pAlignment, "east", 4 ) )
		{
			AssertMsg( 0, "This type of alignment is not supported." );
		}
	}
	*/

	m_bAutoLayout = pInResourceData->GetBool( "auto_layout", false );
	m_bAutoScale = pInResourceData->GetBool( "auto_scale", true );
	m_bDisplayVertical = pInResourceData->GetBool( "display_vertically", false );

	KeyValues *pKVButtonSettings = pInResourceData->FindKey( "ButtonSettings" );		AssertMsg( pKVButtonSettings, "This is required" );
	if ( !pKVButtonSettings )
	{
		AssertMsg( 0, "No button settings specified.  CNavigationPanel is useless without this data." );
		return;
	}

	// Cache this off for later
	if ( m_pKVButtonSettings )
	{
		m_pKVButtonSettings->deleteThis();
		m_pKVButtonSettings = NULL;
	}
	m_pKVButtonSettings = pKVButtonSettings->MakeCopy();

	// Get individual button data and apply now
	KeyValues *pKVButtons = pInResourceData->FindKey( "Buttons" );
	if ( !pKVButtons )
		return;

	// Go through each image description and create a button
	if ( m_vecButtons.Count() )
		return;

	int i = 0;
	FOR_EACH_SUBKEY( pKVButtons, pKVCurButton )
	{
		CNavButton *pNewButton = new CNavButton( this, pKVCurButton->GetString( "FieldName", pKVCurButton->GetName() ), L"" );
		pNewButton->ApplySettings( pKVCurButton );
		pNewButton->InvalidateLayout( true, false );
		pNewButton->SetCommand( CFmtStr( "select_%i", i ).Access() );
		m_vecButtons.AddToTail( pNewButton );
		++i;
	}

	if ( m_vecButtons.IsValidIndex( m_iSelectedButtonDefault ) )
	{
		UpdateButtonSelectionStates( m_iSelectedButtonDefault );
		m_vecButtons[ m_iSelectedButtonDefault ]->InvalidateLayout( true );

		const int iUserData = m_vecButtons[ m_iSelectedButtonDefault ]->m_iUserData;
		PostActionSignal( new KeyValues( "NavButtonSelected", "userdata", iUserData ) );
	}
}

void CNavigationPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

void CNavigationPanel::PerformLayout()
{
	if ( !m_vecButtons.Count() )
		return;

	if ( !m_pKVButtonSettings )
		return;

	BaseClass::PerformLayout();

	// Get from settings
	int nSettingWidth = m_pKVButtonSettings->GetInt( "wide" );
	int nSettingHeight = m_pKVButtonSettings->GetInt( "tall" );

	// Button dimensions for setting positions
	int nButtonWidth = nSettingWidth;
	int nButtonHeight = nSettingHeight;

	if ( m_bAutoScale )
	{
		// Get image size
		int nImageW, nImageH;
		m_vecButtons[0]->GetImage()->GetSize( nImageW, nImageH );

		int nWidth, nHeight;
		if ( m_bDisplayVertical )
		{
			nWidth = GetWide();
			nHeight = nSettingHeight * nWidth / ( nSettingWidth > 0 ? nSettingWidth : 1 );
		}
		else
		{
			nHeight = GetTall();
			nWidth = nSettingWidth * nHeight / ( nSettingHeight > 0 ? nSettingHeight : 1 );
		}

		// Update button dimensions to scaled versions
		nButtonWidth = nWidth;
		nButtonHeight = nHeight;
	}

	FOR_EACH_VEC( m_vecButtons, i )
	{
		if ( m_vecButtons[i] && m_pKVButtonSettings )
		{
			// Apply generic settings
			m_vecButtons[i]->ApplySettings( m_pKVButtonSettings );
		}

		if ( m_bAutoLayout )
		{
			// Display buttons vertically or horizontally?
			if ( m_bDisplayVertical )
			{
				m_vecButtons[i]->SetPos( 0, i * ( nButtonHeight + m_nVerticalBuffer ) );
			}
			else
			{
				const int nStartX = 0;//0.5f * ( GetWide() - NumButtons() * ( nButtonWidth + m_nHorizontalBuffer ) );
				m_vecButtons[i]->SetPos( nStartX + i * ( nButtonWidth + m_nHorizontalBuffer ), 0 );
			}
		}

		if ( m_bAutoScale )
		{		
			m_vecButtons[i]->SetSize( nButtonWidth, nButtonHeight );
			m_vecButtons[i]->GetImage()->SetSize( nButtonWidth, nButtonHeight );
		}

		m_vecButtons[i]->SetVisible( true );
	}
}

void CNavigationPanel::OnCommand( const char *pCommand )
{
	if ( !V_strnicmp( pCommand, "select_", 7 ) )
	{
		const int iButton = atoi( pCommand + 7 );	AssertMsg( m_vecButtons.IsValidIndex( iButton ), "Button index out of range!" );
		if ( !m_vecButtons.IsValidIndex( iButton ) )
			return;

		UpdateButtonSelectionStates( iButton );

		const int iUserData = m_vecButtons[ iButton ]->m_iUserData;
		PostActionSignal( new KeyValues( "NavButtonSelected", "userdata", iUserData ) );
	}
	else
	{
		BaseClass::OnCommand( pCommand );
	}
}

void CNavigationPanel::OnThink()
{
	BaseClass::OnThink();

	// Make sure we only ever have one button in the selection state, since it's
	// possible to do this if you select a button, then click and drag on another
	// button, and release the mouse elsewhere.
	if ( !vgui::input()->IsMouseDown( MOUSE_LEFT ) )
	{
		UpdateButtonSelectionStates( m_iSelectedButton );
	}
}


void CNavigationPanel::UpdateButtonSelectionStates( int iButton )
{
	if ( m_iSelectedButton != iButton )
	{
		m_iSelectedButton = iButton;
	}

	// Set the correct button as selected, all other buttons as not selected
	for ( int i = 0; i < NumButtons(); ++i )
	{
		CNavButton *pCurButton = m_vecButtons[ i ];
		if ( !pCurButton )
			continue;

		bool bShouldSelect = iButton == i;
		pCurButton->SetSelected( bShouldSelect );
		pCurButton->InvalidateLayout();
	}
}

//-----------------------------------------------------------------------------

#ifdef _DEBUG

class CNavPanelTest : public Frame
{
	DECLARE_CLASS_SIMPLE( CNavPanelTest, Frame );

public:
	CNavPanelTest( vgui::Panel *pParent )
	:	Frame( pParent, "NavPanelTest" )
	{
		SetProportional( true );
		LoadControlSettings( "Resource/UI/NavigationPanelTest.res" );
	}
};

CON_COMMAND( open_navpanel_test, "" )
{
	CNavPanelTest *pPanel = SETUP_PANEL( new CNavPanelTest( NULL ) );
	pPanel->SetVisible( true );
	pPanel->InvalidateLayout( false, true );

	engine->ClientCmd_Unrestricted( "gameui_activate" );

	const int nWidth = XRES( 300 );
	const int nHeight = YRES( 300 );

	pPanel->SetBounds(
		( ScreenWidth() - nWidth ) / 2,
		( ScreenHeight() - nHeight ) / 2,
		nWidth,
		nHeight
	);

	pPanel->Activate();
}

#endif // _DEBUG
