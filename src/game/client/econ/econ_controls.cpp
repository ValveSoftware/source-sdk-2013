//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"

#include "ienginevgui.h"
#include <vgui_controls/ScrollBarSlider.h>
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "econ_controls.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/PropertyPage.h"
#include "econ_item_system.h"
#include "econ_item_tools.h"
#include "iachievementmgr.h"
#include "econ_item_description.h"

#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
#include "tf_shareddefs.h"
#endif

using namespace vgui;

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CExButton, CExButton );
DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CExImageButton, CExImageButton );
DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CExLabel, CExLabel );
DECLARE_BUILD_FACTORY( CExRichText );
DECLARE_BUILD_FACTORY( CRichTextWithScrollbarBorders );
DECLARE_BUILD_FACTORY( CEconItemDetailsRichText );
DECLARE_BUILD_FACTORY( CExplanationPopup );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool SetChildPanelVisible( vgui::Panel *pParent, const char *pChildName, bool bVisible, bool bSearchForChildRecursively )
{
	vgui::Panel *pPanel = pParent->FindChildByName( pChildName, bSearchForChildRecursively );
	if ( pPanel )
	{
		if ( pPanel->IsVisible() != bVisible )
		{
			pPanel->SetVisible( bVisible );
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool SetChildPanelEnabled( vgui::Panel *pParent, const char *pChildName, bool bEnabled, bool bSearchForChildRecursively )
{
	vgui::Panel *pPanel = pParent->FindChildByName( pChildName, bSearchForChildRecursively );
	if ( pPanel )
	{
		if ( pPanel->IsEnabled() != bEnabled )
		{
			pPanel->SetEnabled( bEnabled );
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool SetChildButtonSelected( vgui::Panel *pParent, const char *pChildName, bool bSelected, bool bSearchForChildRecursively )
{
	vgui::Button *pPanel = dynamic_cast< vgui::Button* >( pParent->FindChildByName( pChildName, bSearchForChildRecursively ) );
	if ( pPanel )
	{
		if ( pPanel->IsSelected() != bSelected )
		{
			pPanel->SetSelected( bSelected );
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool IsChildButtonSelected( vgui::Panel *pParent, const char *pChildName, bool bSearchForChildRecursively )
{
	vgui::Button *pPanel = dynamic_cast< vgui::Button* >( pParent->FindChildByName( pChildName, bSearchForChildRecursively ) );
	if ( pPanel )
	{
		return pPanel->IsSelected();
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool AddChildActionSignalTarget( vgui::Panel *pParent, const char *pChildName, Panel *messageTarget, bool bSearchForChildRecursively )
{
	vgui::Panel *pPanel = pParent->FindChildByName( pChildName, bSearchForChildRecursively );
	if ( pPanel )
	{
		pPanel->AddActionSignalTarget( messageTarget );
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool SetXToRed( vgui::Label *pPanel )
{
	if ( !pPanel )
		return false;

	wchar_t wszConfirmText[256];
	pPanel->GetText( wszConfirmText, sizeof( wszConfirmText ) );

	if ( ( wszConfirmText[0] == L'x' || wszConfirmText[0] == L'X' ) && wszConfirmText[1] == L' ' )
	{
		pPanel->GetTextImage()->ClearColorChangeStream();
		pPanel->GetTextImage()->AddColorChange( Color(200,80,60,255), 0 );
		pPanel->GetTextImage()->AddColorChange( pPanel->GetFgColor(), 1 );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExButton::CExButton( Panel *parent, const char *name, const char *text, vgui::Panel *pActionSignalTarget, const char *cmd ) : Button( parent, name, text, pActionSignalTarget, cmd )
{
	m_szFont[0] = '\0';
	m_szColor[0] = '\0';
	m_pArmedBorder = NULL;
	m_pDefaultBorderOverride = NULL;
	m_pSelectedBorder = NULL;
	m_pDisabledBorder = NULL;
	m_bbCursorEnterExitEvent = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExButton::CExButton( Panel *parent, const char *name, const wchar_t *wszText, vgui::Panel *pActionSignalTarget, const char *cmd ) : Button( parent, name, wszText, pActionSignalTarget, cmd )
{
	m_szFont[0] = '\0';
	m_szColor[0] = '\0';
	m_pArmedBorder = NULL;
	m_pDefaultBorderOverride = NULL;
	m_pSelectedBorder = NULL;
	m_pDisabledBorder = NULL;
	m_bbCursorEnterExitEvent = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExButton::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	SetFontStr( inResourceData->GetString( "font", "Default" ) );
	SetColorStr( inResourceData->GetString( "fgcolor", "Button.TextColor" ) );

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

	const char *pszBorder = inResourceData->GetString( "border_default", "" );
	if ( *pszBorder )
	{
		m_pDefaultBorderOverride = pScheme->GetBorder( pszBorder );
	}

	pszBorder = inResourceData->GetString( "border_armed", "" );
	if ( *pszBorder )
	{
		m_pArmedBorder = pScheme->GetBorder( pszBorder );
	}

	pszBorder = inResourceData->GetString( "border_disabled", "" );
	if ( *pszBorder )
	{
		m_pDisabledBorder = pScheme->GetBorder( pszBorder );
	}

	const char *pszSelectedBorder = inResourceData->GetString( "border_selected", "" );
	if ( *pszSelectedBorder )
	{
		m_pSelectedBorder = pScheme->GetBorder( pszSelectedBorder );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
vgui::IBorder *CExButton::GetBorder(bool depressed, bool armed, bool selected, bool keyfocus)
{
	if ( !IsEnabled() && m_pDisabledBorder )
		return m_pDisabledBorder;

	if ( selected && m_pSelectedBorder )
		return m_pSelectedBorder;

	if ( armed && m_pArmedBorder )
		return m_pArmedBorder;

	if ( m_pDefaultBorderOverride )
		return m_pDefaultBorderOverride;
	
	return BaseClass::GetBorder( depressed, armed, selected, keyfocus );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExButton::SetFontStr( const char *pFont )
{
	V_strcpy_safe( m_szFont, pFont );

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	SetFont( pScheme->GetFont( m_szFont, true ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExButton::SetColorStr( const char *pColor )
{
	V_strcpy_safe( m_szColor, pColor );

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	SetFgColor( pScheme->GetColor( m_szColor, Color( 255, 255, 255, 255 ) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExButton::OnMouseFocusTicked()
{
	BaseClass::OnMouseFocusTicked();

	if ( m_hMouseTickTarget )
	{
		KeyValues *pMessage = new KeyValues("MouseFocusTicked");
		vgui::ipanel()->SendMessage( m_hMouseTickTarget, pMessage, GetVPanel());
		pMessage->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExButton::OnCursorEntered()
{
	BaseClass::OnCursorEntered();

	if ( m_hMouseTickTarget && m_bbCursorEnterExitEvent )
	{
		KeyValues *pMessage = new KeyValues("CursorEntered");
		vgui::ipanel()->SendMessage( m_hMouseTickTarget, pMessage, GetVPanel());
		pMessage->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExButton::OnCursorExited()
{
	BaseClass::OnCursorExited();

	if ( m_hMouseTickTarget && m_bbCursorEnterExitEvent )
	{
		KeyValues *pMessage = new KeyValues("CursorExited");
		vgui::ipanel()->SendMessage( m_hMouseTickTarget, pMessage, GetVPanel());
		pMessage->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExImageButton::CExImageButton( Panel *parent, const char *name, const char *text, vgui::Panel *pActionSignalTarget, const char *cmd ) : CExButton( parent, name, text, pActionSignalTarget, cmd )
{
	m_ImageDrawColor = Color(255,255,255,255);
	m_ImageArmedColor = Color(255,255,255,255);
	m_ImageDepressedColor = Color(255,255,255,255);
	m_ImageDisabledColor = Color(255,255,255,255);
	m_ImageSelectedColor = Color(255,255,255,255);
	m_pEmbeddedImagePanel = new vgui::ImagePanel( this, "SubImage" );
	m_szImageDefault[0] = '\0';
	m_szImageArmed[0] = '\0';
	m_szImageSelected[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExImageButton::CExImageButton( Panel *parent, const char *name, const wchar_t *wszText, vgui::Panel *pActionSignalTarget, const char *cmd ) : CExButton( parent, name, wszText, pActionSignalTarget, cmd )
{
	m_ImageDrawColor = Color(255,255,255,255);
	m_ImageArmedColor = Color(255,255,255,255);
	m_ImageDepressedColor = Color(255,255,255,255);
	m_ImageDisabledColor = Color(255,255,255,255);
	m_ImageSelectedColor = Color(255,255,255,255);
	m_pEmbeddedImagePanel = new vgui::ImagePanel( this, "SubImage" );
	m_szImageDefault[0] = '\0';
	m_szImageArmed[0] = '\0';
	m_szImageSelected[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExImageButton::~CExImageButton( void )
{
	m_pEmbeddedImagePanel->MarkForDeletion();
	m_pEmbeddedImagePanel = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	int r,g,b,a;

	const char *pszDrawColor = inResourceData->GetString("image_drawcolor", "");
	if (*pszDrawColor)
	{
		if (sscanf(pszDrawColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			m_ImageDrawColor = Color(r, g, b, a);
		}
	}

	pszDrawColor = inResourceData->GetString("image_armedcolor", "");
	if (*pszDrawColor)
	{
		if (sscanf(pszDrawColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			m_ImageArmedColor = Color(r, g, b, a);
		}
	}

	pszDrawColor = inResourceData->GetString("image_depressedcolor", "");
	if (*pszDrawColor)
	{
		if (sscanf(pszDrawColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			m_ImageDepressedColor = Color(r, g, b, a);
		}
	}

	pszDrawColor = inResourceData->GetString("image_disabledcolor", "");
	if (*pszDrawColor)
	{
		if (sscanf(pszDrawColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			m_ImageDisabledColor = Color(r, g, b, a);
		}
	}

	pszDrawColor = inResourceData->GetString( "image_selectedcolor", "" );
	if (*pszDrawColor)
	{
		if (sscanf(pszDrawColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			m_ImageSelectedColor = Color(r, g, b, a);
		}
	}

	KeyValues *pButtonKV = inResourceData->FindKey( "SubImage" );
	if ( pButtonKV )
	{
		m_pEmbeddedImagePanel->ApplySettings( pButtonKV );
	}

	const char *pszImageDefault = inResourceData->GetString("image_default", "");
	if (*pszImageDefault)
	{
		SetImageDefault( pszImageDefault );
	}

	const char *pszImageArmed = inResourceData->GetString("image_armed", "");
	if (*pszImageArmed)
	{
		SetImageArmed( pszImageArmed );
	}

	const char *pszImageSelected = inResourceData->GetString("image_selected", "");
	if (*pszImageSelected)
	{
		SetImageSelected( pszImageSelected );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CExImageButton::GetImageColor( void )
{
	if ( !IsEnabled() )
		return m_ImageDisabledColor;
	if ( IsSelected() )
		return m_ImageSelectedColor;
	if ( IsDepressed() )
		return m_ImageDepressedColor;
	if ( IsArmed() )
		return m_ImageArmedColor;
	return m_ImageDrawColor;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	m_pEmbeddedImagePanel->SetMouseInputEnabled( false );
	m_pEmbeddedImagePanel->SetDrawColor( GetImageColor() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetArmed(bool state)
{
	BaseClass::SetArmed( state );

	if ( m_pEmbeddedImagePanel )
	{
		m_pEmbeddedImagePanel->SetDrawColor( GetImageColor() );

		const char *pszImage = state ? m_szImageArmed : m_szImageDefault;
		if ( *pszImage )
		{
			SetSubImage( pszImage );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetEnabled(bool state)
{
	BaseClass::SetEnabled( state );

	if ( m_pEmbeddedImagePanel )
	{
		m_pEmbeddedImagePanel->SetDrawColor( GetImageColor() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetSelected(bool state)
{
	BaseClass::SetSelected( state );

	if ( m_pEmbeddedImagePanel )
	{
		m_pEmbeddedImagePanel->SetDrawColor( GetImageColor() );

		const char *pszImage = state ? m_szImageSelected : m_szImageDefault;
		if ( *pszImage )
		{
			SetSubImage( pszImage );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetSubImage( const char *pszImage )
{
	m_pEmbeddedImagePanel->SetImage( pszImage );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetImageDefault( const char *pszImageDefault )
{
	V_strcpy_safe( m_szImageDefault, pszImageDefault );	
	if ( !IsArmed() )
	{
		SetSubImage( pszImageDefault );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetImageArmed( const char *pszImageArmed )
{
	V_strcpy_safe( m_szImageArmed, pszImageArmed );
	if ( IsArmed() )
	{
		SetSubImage( m_szImageArmed );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetImageSelected( const char *pszImageSelected )
{
	V_strcpy_safe( m_szImageSelected, pszImageSelected );
	if ( IsSelected() )
	{
		SetSubImage( m_szImageSelected );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExLabel::CExLabel( Panel *parent, const char *name, const char *text ) : Label( parent, name, text )
{
	m_szColor[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExLabel::CExLabel( Panel *parent, const char *name, const wchar_t *wszText ) : Label( parent, name, wszText )
{
	m_szColor[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExLabel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	SetColorStr( inResourceData->GetString( "fgcolor", "Label.TextColor" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExLabel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// Reapply our custom color, so we stomp the base scheme's
	SetColorStr( m_szColor );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExLabel::SetColorStr( const char *pColor )
{
	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	SetColorStr( pScheme->GetColor( pColor, Color( 0, 255, 0, 255 ) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExLabel::SetColorStr( Color cColor )
{
	Q_snprintf( m_szColor, ARRAYSIZE(m_szColor), "%d %d %d %d", cColor.r(), cColor.g(), cColor.b(), cColor.a() );
	SetFgColor( cColor );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExRichText::CExRichText( Panel *parent, const char *name ) : RichText( parent, name )
{
	m_szFont[0] = '\0';
	m_szColor[0] = '\0';
	m_szImageUpArrow[0] = '\0';
	m_szImageDownArrow[0] = '\0';
	m_szImageLine[0] = '\0';
	m_szImageBox[0] = '\0';
	m_bUseImageBorders = false;
	m_pBox = NULL;
	m_pLine = NULL;

	SetCursor(dc_arrow);

	m_pUpArrow = new CExImageButton( this, "UpArrow", "" );
	if ( m_pUpArrow )
	{
		m_pUpArrow->AddActionSignalTarget( _vertScrollBar );
		m_pUpArrow->SetCommand(new KeyValues("ScrollButtonPressed", "index", 0));
		m_pUpArrow->GetImage()->SetShouldScaleImage( true );
		m_pUpArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
		m_pUpArrow->SetAlpha( 255 );
		m_pUpArrow->SetPaintBackgroundEnabled( false );
		m_pUpArrow->SetVisible( false );
	}

	m_pDownArrow = new CExImageButton( this, "DownArrow", "" );
	if ( m_pDownArrow )
	{
		m_pDownArrow->AddActionSignalTarget( _vertScrollBar );
		m_pDownArrow->SetCommand(new KeyValues("ScrollButtonPressed", "index", 1));
		m_pDownArrow->GetImage()->SetShouldScaleImage( true );
		m_pDownArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
		m_pDownArrow->SetAlpha( 255 );
		m_pDownArrow->SetPaintBackgroundEnabled( false );
		m_pDownArrow->SetVisible( false );
	}

	_vertScrollBar->SetOverriddenButtons( m_pUpArrow, m_pDownArrow );
	m_pUpArrow->PassMouseTicksTo( _vertScrollBar );
	m_pDownArrow->PassMouseTicksTo( _vertScrollBar );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExRichText::CreateImagePanels( void )
{
	if ( m_pBox || m_pLine )
		return;

	if ( m_bUseImageBorders )
	{
		m_pLine = new vgui::Panel( this, "Line" );
		m_pBox = new vgui::Panel( this, "Box" );
	}
	else
	{
		m_pLine = new vgui::ImagePanel( this, "Line" );
		m_pBox = new vgui::ImagePanel( this, "Box" );

		dynamic_cast<vgui::ImagePanel *>(m_pBox)->SetShouldScaleImage( true );
		dynamic_cast<vgui::ImagePanel *>(m_pLine)->SetShouldScaleImage( true );
	}
	m_pBox->SetVisible( false );
	m_pLine->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExRichText::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	SetFontStr( inResourceData->GetString( "font", "Default" ) );
	SetColorStr( inResourceData->GetString( "fgcolor", "RichText.TextColor" ) );

	SetCustomImage( m_pUpArrow->GetImage(), inResourceData->GetString( "image_up_arrow", "chalkboard_scroll_up" ), m_szImageUpArrow );
	SetCustomImage( m_pDownArrow->GetImage(), inResourceData->GetString( "image_down_arrow", "chalkboard_scroll_down" ), m_szImageDownArrow );
	SetCustomImage( m_pLine, inResourceData->GetString( "image_line", "chalkboard_scroll_line" ), m_szImageLine );
	SetCustomImage( m_pBox, inResourceData->GetString( "image_box", "chalkboard_scroll_box" ), m_szImageBox );

	const char *pszMouseover = inResourceData->GetString( "image_up_arrow_mouseover", NULL );
	if ( pszMouseover )
	{
		m_pUpArrow->SetImageArmed( pszMouseover );
		m_pUpArrow->SetImageDefault( m_szImageUpArrow );
	}
	pszMouseover = inResourceData->GetString( "image_down_arrow_mouseover", NULL );
	if ( pszMouseover )
	{
		m_pDownArrow->SetImageArmed( pszMouseover );
		m_pDownArrow->SetImageDefault( m_szImageDownArrow );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExRichText::SetFontStr( const char *pFont )
{
	if ( pFont != m_szFont )
	{
		V_strcpy_safe( m_szFont, pFont );
	}

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	SetFont( pScheme->GetFont( m_szFont, true ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExRichText::SetColorStr( const char *pColor )
{
	if ( pColor != m_szColor )
	{
		V_strcpy_safe( m_szColor, pColor );
	}

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	SetFgColor( pScheme->GetColor( m_szColor, Color( 255, 255, 255, 255 ) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExRichText::SetCustomImage( vgui::Panel *pImage, const char *pszImage, char *pszStorage )
{
	if ( pszStorage )
	{
		V_strcpy( pszStorage, pszImage );
	}
	if ( !pImage )
		return;

	if ( m_bUseImageBorders )
	{
		vgui::IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		IBorder *pBorder = pScheme->GetBorder( pszImage );

		if ( pBorder )
		{
			pImage->SetBorder( pBorder );
			return;
		}
	}

	vgui::ImagePanel *pImagePanel = dynamic_cast<vgui::ImagePanel *>(pImage);
	if ( pImagePanel )
	{
		pImagePanel->SetImage( pszImage );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExRichText::ApplySchemeSettings( IScheme *pScheme )
{
	CreateImagePanels();

	BaseClass::ApplySchemeSettings( pScheme );

	// Reapply any custom font/color, so we stomp the base scheme's
	SetFontStr( m_szFont );
	SetColorStr( m_szColor );
	SetCustomImage( m_pUpArrow->GetImage(), m_szImageUpArrow, NULL );
	SetCustomImage( m_pDownArrow->GetImage(), m_szImageDownArrow, NULL );
	SetCustomImage( m_pLine, m_szImageLine, NULL );
	SetCustomImage( m_pBox, m_szImageBox, NULL );

	SetBorder( pScheme->GetBorder( "NoBorder" ) );
	SetBgColor( pScheme->GetColor( "Blank", Color( 0,0,0,0 ) ) );
	SetPanelInteractive( false );
	SetUnusedScrollbarInvisible( true );

	if ( m_pDownArrow  )
	{
		m_pDownArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
	}

	if ( m_pUpArrow  )
	{
		m_pUpArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
	}

	SetScrollBarImagesVisible( false );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExRichText::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( _vertScrollBar )
	{
		_vertScrollBar->SetZPos( 500 );
		m_pUpArrow->SetZPos( 501 );
		m_pDownArrow->SetZPos( 501 );
		
		// turn off painting the vertical scrollbar
		_vertScrollBar->SetPaintBackgroundEnabled( false );
		_vertScrollBar->SetPaintBorderEnabled( false );
		_vertScrollBar->SetPaintEnabled( false );
		_vertScrollBar->SetScrollbarButtonsVisible( false );
		_vertScrollBar->GetButton(0)->SetMouseInputEnabled( false );
		_vertScrollBar->GetButton(1)->SetMouseInputEnabled( false );
		
		if (  _vertScrollBar->IsVisible() )
		{
			int nMin, nMax;
			_vertScrollBar->GetRange( nMin, nMax );
			_vertScrollBar->SetValue( nMin );

			int nScrollbarWide = _vertScrollBar->GetWide();

			int wide, tall;
			GetSize( wide, tall );

			if ( m_pUpArrow )
			{
				m_pUpArrow->SetBounds( wide - nScrollbarWide, 0, nScrollbarWide, nScrollbarWide );
				m_pUpArrow->GetImage()->SetSize( nScrollbarWide, nScrollbarWide );
			}

			if ( m_pLine )
			{
				m_pLine->SetBounds( wide - nScrollbarWide, nScrollbarWide, nScrollbarWide, tall - ( 2 * nScrollbarWide ) );
			}

			if ( m_pBox )
			{
				m_pBox->SetBounds( wide - nScrollbarWide, nScrollbarWide, nScrollbarWide, nScrollbarWide );
			}

			if ( m_pDownArrow )
			{
				m_pDownArrow->SetBounds( wide - nScrollbarWide, tall - nScrollbarWide, nScrollbarWide, nScrollbarWide );
				m_pDownArrow->GetImage()->SetSize( nScrollbarWide, nScrollbarWide );
			}

			SetScrollBarImagesVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExRichText::SetText( const wchar_t *text )
{
	wchar_t buffer[2048];
	Q_wcsncpy( buffer, text, sizeof( buffer ) );

	// transform '\r' to ' ' to eliminate double-spacing on line returns
	for ( wchar_t *ch = buffer; *ch != 0; ch++ )
	{
		if ( *ch == '\r' )
		{
			*ch = ' ';
		}
	}

	BaseClass::SetText( buffer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExRichText::SetText( const char *text )
{
	char buffer[2048];
	Q_strncpy( buffer, text, sizeof( buffer ) );

	// transform '\r' to ' ' to eliminate double-spacing on line returns
	for ( char *ch = buffer; *ch != 0; ch++ )
	{
		if ( *ch == '\r' )
		{
			*ch = ' ';
		}
	}

	BaseClass::SetText( buffer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExRichText::SetScrollBarImagesVisible( bool visible )
{
	if ( m_pDownArrow && m_pDownArrow->IsVisible() != visible )
	{
		m_pDownArrow->SetVisible( visible );
		m_pDownArrow->SetEnabled( visible );
	}

	if ( m_pUpArrow && m_pUpArrow->IsVisible() != visible )
	{
		m_pUpArrow->SetVisible( visible );
		m_pUpArrow->SetEnabled( visible );
	}

	if ( m_pLine && m_pLine->IsVisible() != visible )
	{
		m_pLine->SetVisible( visible );
	}

	if ( m_pBox && m_pBox->IsVisible() != visible )
	{
		m_pBox->SetVisible( visible );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExRichText::OnTick()
{
	if ( !IsVisible() )
		return;

	if ( m_pDownArrow && m_pUpArrow && m_pLine && m_pBox )
	{
		if ( _vertScrollBar && _vertScrollBar->IsVisible() )
		{
			// turn on our own images
			SetScrollBarImagesVisible ( true );

			// set the alpha on the up arrow
			int nMin, nMax;
			_vertScrollBar->GetRange( nMin, nMax );
			int nScrollPos = _vertScrollBar->GetValue();
			int nRangeWindow = _vertScrollBar->GetRangeWindow();
			int nBottom = nMax - nRangeWindow;
			if ( nBottom < 0 )
			{
				nBottom = 0;
			}

			// set the alpha on the up arrow
			int nAlpha = ( nScrollPos - nMin <= 0 ) ? 90 : 255;
			m_pUpArrow->SetAlpha( nAlpha );

			// set the alpha on the down arrow
			nAlpha = ( nScrollPos >= nBottom ) ? 90 : 255;
			m_pDownArrow->SetAlpha( nAlpha );

			ScrollBarSlider *pSlider = _vertScrollBar->GetSlider();
			if ( pSlider && pSlider->GetRangeWindow() > 0 )
			{
				int x, y, w, t, min, max;
				m_pLine->GetBounds( x, y, w, t );
				pSlider->GetNobPos( min, max );

				m_pBox->SetBounds( x, y + min, w, ( max - min ) );
			}
		}
		else
		{
			// turn off our images
			SetScrollBarImagesVisible ( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Rich text control that knows how to fill itself with information 
//			that describes a specific item definition.
//-----------------------------------------------------------------------------
CEconItemDetailsRichText::CEconItemDetailsRichText( vgui::Panel *parent, const char *panelName )
:	BaseClass( parent, panelName ),
	m_bAllowItemSetLinks( false ),
	m_bLimitedItem( false ),
	m_hLinkFont( INVALID_FONT )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDetailsRichText::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	const char *pszHighlightColor = inResourceData->GetString( "highlight_color", "Orange" );
	const char *pszItemSetColor = inResourceData->GetString( "itemset_color", "Blue" );
	const char *pszLinkColor = inResourceData->GetString( "link_color", "LightOrange" );
	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	m_colTextHighlight = pScheme->GetColor( pszHighlightColor, Color( 255, 255, 255, 255 ) );
	m_colItemSet = pScheme->GetColor( pszItemSetColor, Color( 255, 255, 255, 255 ) );
	m_colLink = pScheme->GetColor( pszLinkColor, Color( 255, 255, 255, 255 ) );
	m_hLinkFont = pScheme->GetFont( "Link", true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDetailsRichText::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetUnderlineFont( m_hLinkFont );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDetailsRichText::UpdateDetailsForItem( const CEconItemDefinition *pDef )
{
	SetText( "" );

	if ( !m_ToolList.Count() )
	{
		UpdateToolList();
	}

	DataText_AppendStoreFlags( pDef );
	DataText_AppendItemData( pDef );
	DataText_AppendAttributeData( pDef );
	DataText_AppendUsageData( pDef );
	DataText_AppendToolUsage( pDef );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDetailsRichText::AddDataText( const char *pszText, bool bAddPostLines, const wchar_t *wpszArg, const wchar_t *wpszArg2, const int *pItemDefIndex )
{
	static wchar_t wszConstructedString[4096];
	static wchar_t wszText[4096];

	if ( pszText[0] != '#' )
	{
		InsertString( pszText );
		return;
	}

	wchar_t *pLocText = g_pVGuiLocalize->Find( pszText );
	if ( wpszArg && pLocText )
	{
		if ( wpszArg2 )
		{
			g_pVGuiLocalize->ConstructString_safe( wszConstructedString, pLocText, 2, wpszArg, wpszArg2 );
		}
		else
		{
			g_pVGuiLocalize->ConstructString_safe( wszConstructedString, pLocText, 1, wpszArg );
		}
		pLocText = wszConstructedString;
	}

	if ( pLocText )
	{
		enum
		{
			STATE_COLOR_NORMAL = 1,
			STATE_COLOR_HINT,
			STATE_COLOR_ITEMSET,
			STATE_LINK_START,
			STATE_LINK_STOP,
		};

		Color color = GetFgColor();
		Color newColor = color;
		int startIdx = 0;
		int endIdx = 0;
		bool bContinue = true;
		bool bInLink = false;
		while ( bContinue )
		{
			bool bSetText = false;
			bool bEnd = false;

			switch ( pLocText[endIdx] )
			{
			case 0:
				bContinue = false;
				bSetText = true;
				bEnd = true;
				break;
			case STATE_COLOR_NORMAL:
				newColor = GetFgColor();
				bSetText = true;
				break;
			case STATE_COLOR_HINT:
				newColor = m_colTextHighlight;
				bSetText = true;
				break;
			case STATE_COLOR_ITEMSET:
				newColor = m_colItemSet;
				bSetText = true;
				break;
			case STATE_LINK_START:
				bInLink = true;
				break;
			}

			if ( startIdx != endIdx )
			{
				if ( bSetText )
				{
					// copy the colored text to wide
					int len = endIdx - startIdx + 1;
					wcsncpy( wszText, pLocText + startIdx, len );
					wszText[len-1] = 0;

					// If the next character isn't the end of a link, insert the string
					if ( bInLink && pItemDefIndex )
					{
						InsertItemLink( wszText, *pItemDefIndex, &color );				
					}
					else
					{
						InsertColorChange( color );
						InsertString( wszText );
					}
					bInLink = false;
					color = newColor;

					// skip past the color change character
					startIdx = endIdx + 1;
				}
			}
			++endIdx;
		}

		if ( bAddPostLines )
		{
			InsertString( L"\n\n" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDetailsRichText::DataText_AppendUsageData( const CEconItemDefinition *pBaseDef )
{
	// Don't show class/slot usage for class/slot tokens
	if ( pBaseDef->GetItemClass() && ( !V_strcmp( pBaseDef->GetItemClass(), "class_token" ) || !V_strcmp( pBaseDef->GetItemClass(), "slot_token" ) ) )
		return;

#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
	const CTFItemDefinition *pDef = dynamic_cast< const CTFItemDefinition *>( pBaseDef );
	if ( !pDef )
		return;

	// Class usage
	if ( pDef->CanBeUsedByAllClasses() )
	{
		if ( pDef->GetBundleInfo() != NULL )
		{
			AddDataText( "#TF_Armory_Item_ClassUsageAllBundle", false );
		}
		else
		{
			AddDataText( "#TF_Armory_Item_ClassUsageAll", false );
		}
		AddDataText( "\n" );
	}
	else
	{
		bool bFirst = true;
		for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
		{
			if ( pDef->CanBeUsedByClass(i) )
			{
				if ( bFirst )
				{
					bFirst = false;

					if ( pDef->GetBundleInfo() != NULL )
					{
						AddDataText( "#TF_Armory_Item_ClassUsageBundle", false );
					}
					else
					{
						AddDataText( "#TF_Armory_Item_ClassUsage", false );
					}
				}
				else
				{
					AddDataText( ", " );
				}

				const wchar_t *pwszClassName = g_pVGuiLocalize->Find( g_aPlayerClassNames[i] );
				if ( pwszClassName )
				{
					InsertColorChange( m_colTextHighlight );
					InsertString( pwszClassName );
					InsertColorChange( GetFgColor() );
				}
			}
		}
		if ( !bFirst )
		{
			AddDataText( ".\n" );
		}
	}

	// Slot usage. First, find out if everyone uses it in the same slot, or whether it's used in different slots per class
	bool bHasPerClassSlots = false;
	int iDefaultSlot = pDef->GetDefaultLoadoutSlot();
	for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
	{
		if ( !pDef->CanBeUsedByClass(i) )
			continue;

		int iClassSlot = pDef->GetLoadoutSlot(i);
		if ( iClassSlot != iDefaultSlot )
		{
			bHasPerClassSlots = true;
			break;
		}
	}
	// Now print the easy line, or the per-class lines
	if ( !bHasPerClassSlots )
	{
		if ( iDefaultSlot != -1 )
		{
			AddDataText( "#TF_Armory_Item_SlotUsageAll", true, g_pVGuiLocalize->Find( ItemSystem()->GetItemSchema()->GetLoadoutStringsForDisplay( pDef->GetEquipType() )[iDefaultSlot] ) );
		}
	}
	else
	{
		bool bFirst = true;
		for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
		{
			if ( !pDef->CanBeUsedByClass(i) )
				continue;

			if ( bFirst )
			{
				bFirst = false;
				AddDataText( "#TF_Armory_Item_SlotUsageClassHeader", false );
			}
			else
			{
				AddDataText( ", " );
			}

			int iClassSlot = pDef->GetLoadoutSlot(i);
			AddDataText( "#TF_Armory_Item_SlotUsageClass", false, g_pVGuiLocalize->Find( ItemSystem()->GetItemSchema()->GetLoadoutStringsForDisplay( pDef->GetEquipType() )[iClassSlot] ), g_pVGuiLocalize->Find( g_aPlayerClassNames[i] ) );
		}
		if ( !bFirst )
		{
			AddDataText( ".\n\n" );
		}
	}
#endif // #if defined(TF_DLL) || defined(TF_CLIENT_DLL)
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDetailsRichText::DataText_AppendToolUsage( const CEconItemDefinition *pDef )
{
	// Loop through the tools, and list any that can be applied to this item
	bool bFirstTool = true;
	for ( int i = 0; i < m_ToolList.Count(); i++ )
	{
		const GameItemDefinition_t *pToolDef = dynamic_cast<const GameItemDefinition_t *>( GetItemSchema()->GetItemDefinition( m_ToolList[i] ) );

		if ( !CEconSharedToolSupport::ToolCanApplyToDefinition( pToolDef, dynamic_cast<const GameItemDefinition_t *>( pDef ) ) )
			continue;

		if ( bFirstTool )
		{
			bFirstTool = false;
			AddDataText( "#TF_Armory_Item_ToolUsage", false );
		}
		else
		{
			AddDataText( ", " );
		}

		// Create a link to the item
		{
			// we need an econ item view here for just the item name
			CEconItemView tmpTool;
			tmpTool.Init( m_ToolList[i], AE_USE_SCRIPT_VALUE, AE_USE_SCRIPT_VALUE, true );

			InsertItemLink( tmpTool.GetItemName(), pToolDef->GetDefinitionIndex() );
		}
	}
	if ( !bFirstTool )
	{
		AddDataText( ".\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDetailsRichText::DataText_AppendStoreFlags( const CEconItemDefinition *pDef )
{
	if ( !ItemSystem() || !ItemSystem()->GetItemSchema() )
		return;

	const bool bHolidayRestriction = pDef->GetHolidayRestriction() != NULL && V_strlen( pDef->GetHolidayRestriction() ) > 0;
	if ( bHolidayRestriction )
	{
		wchar_t *pRestrictedText = g_pVGuiLocalize->Find( "#Store_HolidayRestrictionText" );

		if ( pRestrictedText )
		{
			InsertColorChange( Color( 200, 80, 60, 255 ) );
			InsertString( pRestrictedText );
			InsertString( L".\n\n" );
		}
	}

	if ( m_bLimitedItem )
	{
		wchar_t *pLocText = bHolidayRestriction
						  ? g_pVGuiLocalize->Find( "#TF_Armory_Item_Limited_Holiday" )
						  : g_pVGuiLocalize->Find( "#TF_Armory_Item_Limited" );
		
		if ( pLocText )
		{
			InsertColorChange( Color( 255, 140, 0, 255 ) );
			InsertString ( pLocText );
			InsertString( L"\n\n" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDetailsRichText::DataText_AppendItemData( const CEconItemDefinition *pDef )
{
	if ( !GetItemSchema() )
		return;

	// Start by looking for a specified armory desc string
	const char *pDesc = pDef->GetArmoryDescString();
	if ( pDesc && pDesc[0] )
	{
		const ArmoryStringDict_t &ArmoryItemData = ItemSystem()->GetItemSchema()->GetArmoryDataItems();

		// Tokenize it, and look for localization strings for each token
		CUtlVector< char * > vecArmoryKeys;
		Q_SplitString( pDesc, " ", vecArmoryKeys );
		FOR_EACH_VEC( vecArmoryKeys, i )
		{
			int iIdx = ArmoryItemData.Find( vecArmoryKeys[i] );
			if ( ArmoryItemData.IsValidIndex( iIdx ) )
			{
				const char *pLoc = ArmoryItemData.Element( iIdx ).Get();
				AddDataText( pLoc );
			}
		}
		vecArmoryKeys.PurgeAndDeleteElements();
	}

	// Is this item part of a set?
	if ( pDef->GetItemSetDefinition() )
	{
		DataText_AppendSetData( pDef );
	}

	if ( pDef->GetBundleInfo() != NULL )
	{
		DataText_AppendBundleData( pDef );
	}

	// Does this item type have data associated with it?
	const ArmoryStringDict_t &ArmoryItemTypeData = GetItemSchema()->GetArmoryDataItemTypes();
	int iIdx = ArmoryItemTypeData.Find( pDef->GetItemTypeName() );
	if ( ArmoryItemTypeData.IsValidIndex( iIdx ) )
	{
		const char *pLoc = ArmoryItemTypeData.Element( iIdx ).Get();
		AddDataText( pLoc );
	}

	// Does this item class have data associated with it?
	const ArmoryStringDict_t &ArmoryItemClassData = GetItemSchema()->GetArmoryDataItemClasses();
	iIdx = pDef->GetItemClass() ? ArmoryItemClassData.Find( pDef->GetItemClass() ) : ArmoryItemClassData.InvalidIndex();
	if ( ArmoryItemClassData.IsValidIndex( iIdx ) )
	{
		if ( !pDef->GetDefinitionKey( "hack_disable_armory_type_desc" ) )
		{
			const char *pLoc = ArmoryItemClassData.Element( iIdx ).Get();
			AddDataText( pLoc );
		}
	}

	// Can this item be earned by an achievement?
	const AchievementAward_t *pAchievementAward = GetItemSchema()->GetAchievementRewardByDefIndex( pDef->GetDefinitionIndex() );
	if( pAchievementAward )
	{
		wchar_t *pszAchName = ACHIEVEMENT_LOCALIZED_NAME_FROM_STR( pAchievementAward->m_sNativeName.String() );
		if ( pszAchName )
		{
			AddDataText( "#TF_Armory_Item_AchievementReward", true, pszAchName );
		}
	}

	// Is this a Holiday item?
	if ( pDef->GetHolidayRestriction() )
	{
		AddDataText( "#TF_Armory_Item_HolidayRestriction" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDetailsRichText::DataText_AppendBundleData( const CEconItemDefinition *pDef )
{
	bool bFirstItem = true;

	const bundleinfo_t *pBundleInfo = pDef->GetBundleInfo();
	FOR_EACH_VEC( pBundleInfo->vecItemDefs, i )
	{
		CEconItemDefinition *pBundledItem = pBundleInfo->vecItemDefs[i];
		if ( pBundledItem )
		{
			if ( bFirstItem )
			{
				bFirstItem = false;
				AddDataText( "#TF_Armory_Item_Bundle", false );
			}
			else
			{
				AddDataText( ", " );
			}

			CEconItemView bundleItemData;
			bundleItemData.Init( pBundledItem->GetDefinitionIndex(), AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );

			InsertItemLink( bundleItemData.GetItemName(), bundleItemData.GetItemDefIndex() );
		}
	}

	if ( !bFirstItem )
	{
		AddDataText( ".\n\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDetailsRichText::DataText_AppendAttributeData( const CEconItemDefinition *pDef )
{
	if ( !ItemSystem() || !ItemSystem()->GetItemSchema() )
		return;

	const ArmoryStringDict_t &ArmoryAttribData = ItemSystem()->GetItemSchema()->GetArmoryDataAttributes();

	CVarBitVec m_AttribsShown;
	m_AttribsShown.Resize( ArmoryAttribData.Count() );
	m_AttribsShown.ClearAll();

	const CUtlVector<static_attrib_t> &vecStaticAttribs = pDef->GetStaticAttributes();
	FOR_EACH_VEC( vecStaticAttribs, i )
	{
		const static_attrib_t &attrib = vecStaticAttribs[i];
		CEconItemAttributeDefinition *pAttributeDef = ItemSystem()->GetStaticDataForAttributeByDefIndex( attrib.iDefIndex );
		if ( !pAttributeDef )
			continue;
		if ( pAttributeDef->IsHidden() )
			continue;

		const char *pDesc = pAttributeDef->GetArmoryDescString();
		if ( !pDesc || !pDesc[0] )
			continue;

		// Tokenize it, and look for localization strings for each token
		CUtlVector< char * > vecArmoryKeys;
		Q_SplitString( pDesc, " ", vecArmoryKeys );
		FOR_EACH_VEC( vecArmoryKeys, iKey )
		{
			int iIdx = ArmoryAttribData.Find( vecArmoryKeys[iKey] );
			if ( ArmoryAttribData.IsValidIndex( iIdx ) )
			{
				if ( m_AttribsShown[iIdx] == false )
				{
					const char *pLoc = ArmoryAttribData.Element( iIdx ).Get();
					AddDataText( pLoc );

					m_AttribsShown.Set( iIdx );
				}
			}
		}

		vecArmoryKeys.PurgeAndDeleteElements();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDetailsRichText::DataText_AppendSetData( const CEconItemDefinition *pDef )
{
	if ( !ItemSystem() || !ItemSystem()->GetItemSchema() )
		return;

	CEconItemSchema *pSchema = ItemSystem()->GetItemSchema();
	if ( pSchema )
	{
		const CEconItemSetDefinition *pItemSet = pDef->GetItemSetDefinition();
		if ( pItemSet )
		{
			// Does this set provide bonus attributes when completely worn?
			if ( pItemSet->m_iAttributes.Count() > 0 )
			{
				// Used for grabbing display colors.
				vgui::HScheme hScheme = vgui::scheme()->GetScheme( "ClientScheme" );
				vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( hScheme );

				Assert( pScheme );

				// Insert the set description
				wchar_t *pLocText = g_pVGuiLocalize->Find( pItemSet->m_pszLocalizedName );
				AddDataText( "#TF_Armory_Item_InSet", false, pLocText, NULL, m_bAllowItemSetLinks ? &pItemSet->m_iBundleItemDef : NULL );

				for ( int i = 0;  i < pItemSet->m_iAttributes.Count(); i++ )
				{
					const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( pItemSet->m_iAttributes[i].m_iAttribDefIndex );
					if ( !pAttrDef )
						continue;

					CEconAttributeDescription AttrDesc( GLocalizationProvider(), pAttrDef, pItemSet->m_iAttributes[i].m_flValue );
					if ( !AttrDesc.GetDescription().IsEmpty() )
					{
						InsertColorChange( pScheme->GetColor( GetColorNameForAttribColor( AttrDesc.GetDefaultColor() ), Color(255, 255, 255, 255) ) );
						AddDataText( "      " );
						InsertString( AttrDesc.GetDescription().Get() );
						AddDataText( "\n" );
					}
				}

				AddDataText( "\n" );
			}
			// This set is visual and provides no additional bonuses when worn completely.
			else
			{
				// Insert the set description
				wchar_t *pLocText = g_pVGuiLocalize->Find( pItemSet->m_pszLocalizedName );
				AddDataText( "#TF_Armory_Item_InSet_NoBonus", false, pLocText, NULL, m_bAllowItemSetLinks ? &pItemSet->m_iBundleItemDef : NULL );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDetailsRichText::UpdateToolList( void )
{
	m_ToolList.Purge();

	// Find all the tool types in our items list
	const CEconItemSchema::ToolsItemDefinitionMap_t &mapItemDefs = ItemSystem()->GetItemSchema()->GetToolsItemDefinitionMap();
	FOR_EACH_MAP( mapItemDefs, i )
	{
		const CEconItemDefinition *pDef = mapItemDefs[i];
		if ( !pDef->GetItemClass() )
			continue;

		if ( !pDef->IsTool() )
			continue;

		const IEconTool *pEconTool = pDef->GetEconTool();
		if ( !pEconTool )
			continue;

		if ( !pEconTool->ShouldDisplayAsUseableOnItemsInArmory() )
			continue;

		// Now make sure it doesn't have the same type as an existing tool
		bool bAlreadyFound = false;
		
		FOR_EACH_VEC( m_ToolList, tool )
		{
			CEconItemDefinition *pOtherDef = ItemSystem()->GetStaticDataForItemByDefIndex( m_ToolList[tool] );
			Assert( pOtherDef );
				
			const IEconTool *pOtherEconTool = pOtherDef->GetEconTool();
			Assert( pOtherEconTool );
				
			bAlreadyFound = !V_strcmp( pEconTool->GetTypeName(), pOtherEconTool->GetTypeName() );
			if ( bAlreadyFound )
			{
				break;
			}
		}
		
		if ( !bAlreadyFound )
		{
			m_ToolList.AddToTail( pDef->GetDefinitionIndex() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDetailsRichText::InsertItemLink( const wchar_t *pwzItemName, int iItemDef, Color *pColorOverride )
{
	char szTmpToolName[256];
	::ILocalize::ConvertUnicodeToANSI(pwzItemName, szTmpToolName, sizeof( szTmpToolName ));
	char szToolStoreURL[256];
	V_snprintf( szToolStoreURL, sizeof( szToolStoreURL ), "<a href=item://%u>%s</a>", iItemDef, szTmpToolName );
	InsertPossibleURLString( szToolStoreURL, pColorOverride ? *pColorOverride : m_colLink, GetFgColor() );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExplanationPopup::CExplanationPopup(Panel *parent, const char *panelName) : vgui::EditablePanel( parent, panelName )
{
	m_pCallout = new CExplanationPopupCalloutArrow( parent );
	m_pCallout->SetVisible( false );
	m_pCallout->SetAutoDelete( false );

	m_szPrevExplanation[0] = '\0';
	m_szNextExplanation[0] = '\0';
	m_bFinishedPopup = false;

	ListenForGameEvent( "gameui_hidden" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExplanationPopup::~CExplanationPopup( void )
{
	m_pCallout->MarkForDeletion();
	m_pCallout = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExplanationPopup::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "close" ) )
	{
		Hide( 0 );
	}
	else if ( !Q_stricmp( command, "nextexplanation" ) )
	{
		Hide( 1 );
	}
	else if ( !Q_stricmp( command, "prevexplanation" ) )
	{
		Hide( -1 );
	}
	else 
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExplanationPopup::Hide( int iExplanationDelta ) 
{
	int iPos = m_iPositionInChain;
	const char *pszMoveTo = NULL;
	if ( iExplanationDelta == -1 )
	{
		if ( !m_szPrevExplanation || m_szPrevExplanation[ 0 ] == '\0' )
			return;

		pszMoveTo = m_szPrevExplanation;
		iPos--;
	}
	else if ( iExplanationDelta == 1 )
	{
		if ( !m_szNextExplanation || m_szNextExplanation[ 0 ] == '\0' )
			return;

		pszMoveTo = m_szNextExplanation;
		iPos++;
	}

	SetVisible( false ); 
	m_pCallout->SetVisible( false ); 
	vgui::ivgui()->RemoveTickSignal( GetVPanel() );

	if ( m_bForceClose )
	{
		TFModalStack()->PopModal( this );
	}

	if ( iExplanationDelta == 0 )
	{
		if ( GetParent() )
		{
			GetParent()->NavigateTo();
		}

		return;
	}

	CExplanationPopup *pPopup = dynamic_cast<CExplanationPopup*>( GetParent()->FindChildByName( pszMoveTo ) );
	if ( pPopup )
	{
		pPopup->Popup( iPos, m_iTotalInChain );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExplanationPopup::Popup( int iPosition, int iTotalPanels )
{
	// Parent this to our parent. Doing it in our constructor doesn't work because
	// the parent passed in there hasn't been initialized properly.
	m_pCallout->SetParent( GetParent() );
	m_pCallout->SetZPos( GetZPos() - 1 );

	if ( m_bForceClose )
	{
		TFModalStack()->PushModal( this );
	}

	// If they don't specify X,Y,W,H, we start tiny on the callout position
	if ( !m_iStartX && !m_iStartY )
	{
		m_iStartX = m_iCalloutInParentsX;
		m_iStartY = m_iCalloutInParentsY;
	}
	if ( !m_iStartW && !m_iStartH )
	{
		m_iStartW = 1;
		m_iStartH = 1;
	}

	// If we weren't given a position, we're the first in a chain. Figure out
	// how many there are in the total chain.
	m_iPositionInChain = iPosition;
	m_iTotalInChain = iTotalPanels;
	if ( !m_iTotalInChain )
	{
		m_iTotalInChain = 0;
		m_iPositionInChain = 1;
		CExplanationPopup *pPopup = this;
		while ( pPopup )
		{
			m_iTotalInChain++;

			const char *pszNext = pPopup->GetNextExplanation();
			if ( !pszNext[0] )
				break;

			const char *pszPrev = pPopup->GetName();
			pPopup = dynamic_cast<CExplanationPopup*>( GetParent()->FindChildByName( pszNext ) );
			if ( pPopup )
			{
				pPopup->SetPrevExplanation( pszPrev );
			}
		}
	}

	// Now assemble our position label 
	char szTmp[16];
	Q_snprintf(szTmp, 16, "%d/%d", m_iPositionInChain, m_iTotalInChain );
	SetDialogVariable( "explanationnumber", szTmp );

	if ( m_bUseResFileForControls )
	{
		Assert( !m_strTitle.IsEmpty() );
		Assert( !m_strBody.IsEmpty() );
		InvalidateLayout( true, true );
		SetDialogVariable( "title", g_pVGuiLocalize->Find( m_strTitle ) );
		SetDialogVariable( "body", g_pVGuiLocalize->Find( m_strBody ) );
		SetControlVisible( "PrevButton", m_iPositionInChain > 1 );
		SetControlVisible( "NextButton", m_iPositionInChain < m_iTotalInChain );
		SetControlVisible( "PositionLabel", m_iTotalInChain > 1 );

		// Set the end height to be just below the body label
		CExLabel* pBodyLabel = FindControl< CExLabel >( "TextLabel" );
		int nContentWide, nContentTall;
		pBodyLabel->GetContentSize( nContentWide, nContentTall );
		m_iEndH = nContentTall + pBodyLabel->GetYPos() + YRES( 30 );
	}

	SetBounds( m_iStartX, m_iStartY, m_iStartW, m_iStartH );
	SetVisible( true );
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_flStartTime = Plat_FloatTime();
	m_flEndTime = m_flStartTime + 0.5;
	m_bFinishedPopup = false;

	// If our endX & endW is going to result in us being off the side of the screen, move back on
	if ( m_iEndX < 0 )
	{
		m_iEndX = XRES(5);
	}
	else if ( (m_iEndX + m_iEndW) > ScreenWidth() )
	{
		m_iEndX = ScreenWidth() - m_iEndW - XRES(5);
	}

	// Figure out what side of the bubble we should have the arrow attached to
	m_iCalloutSide = EXC_SIDE_TOP;
	Vector vecCallout( m_iCalloutInParentsX, m_iCalloutInParentsY, 0 );
	Vector vecMins( m_iEndX, m_iEndY, 0 );
	Vector vecMaxs( m_iEndX + m_iEndW, m_iEndY + m_iEndH, 0 );
	Vector vecPoint;
	CalcClosestPointOnAABB( vecMins, vecMaxs, vecCallout, vecPoint );

	if ( vecPoint.x == vecMins.x && vecCallout.x != vecMins.x )
	{
		m_iCalloutSide = EXC_SIDE_LEFT;
	}
	else if ( vecPoint.y == vecMins.y && vecCallout.y != vecMins.y  )
	{
		m_iCalloutSide = EXC_SIDE_TOP;
	}
	else if ( vecPoint.x == vecMaxs.x && vecCallout.x != vecMaxs.x  )
	{
		m_iCalloutSide = EXC_SIDE_RIGHT;
	}
	else
	{
		m_iCalloutSide = EXC_SIDE_BOTTOM;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExplanationPopup::OnTick( void )
{
	float flElapsed = Plat_FloatTime() - m_flStartTime;
	float flTotal = m_flEndTime - m_flStartTime;
	float flBias = Bias( RemapValClamped( flElapsed, 0.f, flTotal, 0.f, 1.f ), 0.7f );
	flElapsed = flTotal * flBias;

	if ( flElapsed >= flTotal )
	{
		//vgui::ivgui()->RemoveTickSignal( GetVPanel() );
		if ( !m_bFinishedPopup )
		{
			SetBounds( m_iEndX, m_iEndY, m_iEndW, m_iEndH );
			PositionCallout( 1.0 );
			m_bFinishedPopup = true;
		}

		// If we've lost focus, or been hidden, release our modal lock
		if ( !ipanel()->IsFullyVisible( GetVPanel() ))
		{
			Hide(0);
		}
		return;
	}

	int iExpandW = XRES(30);
	int iExpandH = YRES(30);
	int iExpandedW = m_iEndW + iExpandW;
	int iExpandedH = m_iEndH + iExpandH;
	int iExpandedX = m_iEndX - (iExpandW * 0.5);
	int iExpandedY = m_iEndY - (iExpandH * 0.5);

	int iW, iH, iX, iY;
	float flExpandTime = (flTotal * 0.66);

	PositionCallout( RemapVal( flElapsed, 0, flTotal, 0, 1 ) );

// 	iW = RemapValClamped( flElapsed, 0, flTotal, m_iStartW, m_iEndW );
// 	iH = RemapValClamped( flElapsed, 0, flTotal, m_iStartH, m_iEndH );
// 	iX = RemapValClamped( flElapsed, 0, flTotal, m_iStartX, m_iEndX );
// 	iY = RemapValClamped( flElapsed, 0, flTotal, m_iStartY, m_iEndY );
// 	SetBounds( iX, iY, iW, iH );
// 	return;

	if ( flElapsed < flExpandTime )
	{
		// Expand to greater than the end size
		iW = RemapValClamped( flElapsed, 0, flExpandTime, m_iStartW, iExpandedW );
		iH = RemapValClamped( flElapsed, 0, flExpandTime, m_iStartH, iExpandedH );
		iX = RemapValClamped( flElapsed, 0, flExpandTime, m_iStartX, iExpandedX );
		iY = RemapValClamped( flElapsed, 0, flExpandTime, m_iStartY, iExpandedY );
	}
	else
	{
		// Contract to the end size
		iW = RemapValClamped( flElapsed, flExpandTime, flTotal, iExpandedW, m_iEndW );
		iH = RemapValClamped( flElapsed, flExpandTime, flTotal, iExpandedH, m_iEndH );
		iX = RemapValClamped( flElapsed, flExpandTime, flTotal, iExpandedX, m_iEndX );
		iY = RemapValClamped( flElapsed, flExpandTime, flTotal, iExpandedY, m_iEndY );
	}
	SetBounds( iX, iY, iW, iH );
}

void CExplanationPopup::OnKeyCodeTyped( vgui::KeyCode code )
{
	if ( IsVisible() && m_pCallout && m_pCallout->IsVisible() )
	{
		// swallow all keys
		if ( code == KEY_ESCAPE )
		{
			OnCommand( "close" );
			return;
		}
	}

	BaseClass::OnKeyCodePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExplanationPopup::OnKeyCodePressed( vgui::KeyCode code )
{
	if ( IsVisible() && m_pCallout && m_pCallout->IsVisible() )
	{
		ButtonCode_t nButtonCode = GetBaseButtonCode( code );

		// swallow all keys
		if ( nButtonCode == KEY_XBUTTON_B )
		{
			OnCommand( "close" );
			return;
		}
		else if ( nButtonCode == KEY_XBUTTON_LEFT || 
				  nButtonCode == KEY_XSTICK1_LEFT ||
				  nButtonCode == KEY_XSTICK2_LEFT ||
				  code == KEY_LEFT )
		{
			OnCommand( "prevexplanation" );
			return;
		}
		else if ( nButtonCode == KEY_XBUTTON_RIGHT || 
				  nButtonCode == KEY_XSTICK1_RIGHT ||
				  nButtonCode == KEY_XSTICK2_RIGHT ||
				  code == KEY_RIGHT )
		{
			OnCommand( "nextexplanation" );
			return;
		}
	}

	BaseClass::OnKeyCodePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: Keep controls in place as we resize
//-----------------------------------------------------------------------------
void CExplanationPopup::OnSizeChanged( int newWide, int newTall ) 
{
	BaseClass::OnSizeChanged( newWide, newTall );

	if ( !m_bUseResFileForControls )
		return;

	// Prev button
	{
		Panel* pPrev = FindChildByName( "PrevButton" );
		if ( pPrev )
		{
			pPrev->SetPos( 0, GetTall() - pPrev->GetTall() );
		}
	}

	// Next button
	{
		Panel* pNext = FindChildByName( "NextButton" );
		if( pNext )
		{
			pNext->SetPos( GetWide() - pNext->GetWide(), GetTall() - pNext->GetTall() );
		}
	}

	// Position label
	{
		Panel* pPosition = FindChildByName( "PositionLabel" );
		if ( pPosition )
		{
			pPosition->SetPos( GetWide() * 0.5f - pPosition->GetWide() * 0.5f, GetTall() - pPosition->GetTall() ); 
		}
	}

	// Close button
	{
		Panel* pClose = FindChildByName( "CloseButton" );
		if ( pClose )
		{
			pClose->SetPos( GetWide() - pClose->GetWide(), 0 ); 
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExplanationPopup::PositionCallout( float flElapsed )
{
	// Size and position the callout
	if ( !m_pCallout->IsVisible() )
	{
		m_pCallout->SetVisible( true );
	}

	int iCalloutSize = 20;
	int iIndent = 15;

	int iMyPos[2];
	GetPos( iMyPos[0], iMyPos[1] );
	int iMySize[2];
	iMySize[0] = GetWide();
	iMySize[1] = GetTall();

	int iCalloutPos[2];
	iCalloutPos[0] = m_iCalloutInParentsX;
	iCalloutPos[1] = m_iCalloutInParentsY;

	// We need to figure out the three corners of the callout triangle, in parent space
	int iArrowA[2];
	int iArrowB[2];
	int iW, iH, iX, iY;

	// Determine which axis the arrow's extruding along
	int x = (m_iCalloutSide == EXC_SIDE_TOP || m_iCalloutSide == EXC_SIDE_BOTTOM) ? 0 : 1;
	int y = !x;

	// Figure out where the center will be of the arrow on the edge that the arrow's extruding (ensure it's always somewhat indented from the corners)
	iArrowA[x] = iMyPos[x] + clamp( iCalloutPos[x] - iMyPos[x] - XRES(iCalloutSize * 0.5), XRES(iIndent), iMySize[x] - XRES(iIndent) - XRES(iCalloutSize) );
	iArrowB[x] = iArrowA[x] + XRES(iCalloutSize);
	iArrowA[y] = iMyPos[y] + (( iCalloutPos[y] > iMyPos[y] ) ? iMySize[y] : 0);
	iArrowB[y] = iMyPos[y] + (( iCalloutPos[y] > iMyPos[y] ) ? iMySize[y] : 0);

	// Slide the arrow out towards the callout over time.
	for ( int i = 0; i < 2; i++ )
	{
		iCalloutPos[i] = RemapValClamped( flElapsed, 0, 1, iArrowA[i] + (iArrowB[i] - iArrowA[i]), iCalloutPos[i] );
	}

	// Assemble a bounding box that contains the arrow points
	iX = MIN( MIN( iCalloutPos[0], iArrowA[0] ), iArrowB[0] );
	iW = MAX( MAX( iCalloutPos[0], iArrowA[0] ), iArrowB[0] ) - iX;
	iY = MIN( MIN( iCalloutPos[1], iArrowA[1] ), iArrowB[1] );
	iH = MAX( MAX( iCalloutPos[1], iArrowA[1] ), iArrowB[1] ) - iY;
	m_pCallout->SetBounds( iX, iY, iW+1, iH+1 );

	//Msg("CALLOUT: %d %d, %d %d\n", iX,iY,iW,iH );

	// Tell the callout where its points are, so it can draw the triangle (make sure the triangle is facing the camera)
	if ( m_iCalloutSide == EXC_SIDE_TOP || m_iCalloutSide == EXC_SIDE_RIGHT )
	{
		m_pCallout->SetArrowPoints( iCalloutPos[0]-iX, iCalloutPos[1]-iY, iArrowB[0]-iX, iArrowB[1]-iY, iArrowA[0]-iX, iArrowA[1]-iY );
	}
	else
	{
		m_pCallout->SetArrowPoints( iCalloutPos[0]-iX, iCalloutPos[1]-iY, iArrowA[0]-iX, iArrowA[1]-iY, iArrowB[0]-iX, iArrowB[1]-iY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExplanationPopupCalloutArrow::Paint( void )
{
	int x,y;
	vgui::ipanel()->GetAbsPos(GetVPanel(), x,y );

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( materials->FindMaterial( "vgui/callout_tail", TEXTURE_GROUP_OTHER ), NULL );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 1 );

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.TexCoord2f( 0, 1.0,0.5 );
	meshBuilder.Position3f( x + m_iArrowA[0], y + m_iArrowA[1], 1 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.TexCoord2f( 0, 0,1 );
	meshBuilder.Position3f( x + m_iArrowB[0], y + m_iArrowB[1], 1 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.TexCoord2f( 0, 0,0 );
	meshBuilder.Position3f( x + m_iArrowC[0], y + m_iArrowC[1], 1 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

//  	vgui::surface()->DrawSetColor( Color(0,255,0,255) );
//  	vgui::surface()->DrawLine( m_iArrowA[0], m_iArrowA[1], m_iArrowB[0], m_iArrowB[1] );
//  	vgui::surface()->DrawLine( m_iArrowA[0], m_iArrowA[1], m_iArrowC[0], m_iArrowC[1] );
//  	vgui::surface()->DrawLine( m_iArrowB[0], m_iArrowB[1], m_iArrowC[0], m_iArrowC[1] );
//  	vgui::surface()->DrawOutlinedRect( 0,0, GetWide(), GetTall() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExplanationPopup::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_bUseResFileForControls )
	{
		LoadControlSettings( "resource/ui/ExplanationPopup.res" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExplanationPopup::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	Q_strncpy( m_szNextExplanation, inResourceData->GetString( "next_explanation", "" ), sizeof( m_szNextExplanation ) );

	m_strTitle = inResourceData->GetString( "explanation_title", NULL );
	m_strBody = inResourceData->GetString( "explanation_body", NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExplanationPopup::SetPrevExplanation( const char *pszPrev )
{
	m_szPrevExplanation[0] = '\0';
	if ( pszPrev && pszPrev[0] )
	{
		Q_strncpy( m_szPrevExplanation, pszPrev, sizeof( m_szPrevExplanation ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExplanationPopup::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "gameui_hidden") == 0 )
	{
		if ( IsVisible() )
		{
			Hide( 0 );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPanelModalStack g_ModalStack;
CPanelModalStack *TFModalStack( void )
{
	return &g_ModalStack;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPanelModalStack::PushModal( vgui::Panel *pDialog )
{
	VPanelHandle hHandle;
	hHandle.Set( pDialog->GetVPanel() );

	FOR_EACH_VEC( m_pDialogs, i )
	{
		if ( m_pDialogs[i] == hHandle )
			return;
	}

	m_pDialogs.AddToHead( hHandle );		

	vgui::input()->SetAppModalSurface( pDialog->GetVPanel() );
	pDialog->RequestFocus();
	pDialog->MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPanelModalStack::PopModal( vgui::Panel *pDialog )
{
	bool bFound = false;
	FOR_EACH_VEC_BACK( m_pDialogs, i )
	{
		if ( m_pDialogs[i].Get() == pDialog->GetVPanel() )
		{
			PopModal( i );
			bFound = true;
		}
	}

	AssertMsg( bFound, "CPanelModalStack::PopModal() failed to find the given dialog." );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPanelModalStack::PopModal( int iIdx )
{
	bool bRecalcLock = false;

	// Only release the modal lock if we had it
	VPANEL hPanel = vgui::input()->GetAppModalSurface();
	if ( !hPanel || m_pDialogs[iIdx].Get() == hPanel )
	{
		bRecalcLock = true;
	}

	m_pDialogs.Remove(iIdx);

	if ( bRecalcLock )
	{
		if ( m_pDialogs.Count() )
		{
			vgui::input()->SetAppModalSurface( m_pDialogs[0] );
		}
		else
		{
			vgui::input()->SetAppModalSurface( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPanelModalStack::Update( void )
{
	if ( m_pDialogs.Count() <= 0 )
		return;

	// Don't run this logic if the game UI isn't visible
	if ( !enginevgui->IsGameUIVisible() )
		return;

	// Safety check: If the app model surface dialog is in our list, make sure it's usable
	VPANEL hPanel = vgui::input()->GetAppModalSurface();

	FOR_EACH_VEC_BACK( m_pDialogs, i )
	{
		// Pop dialogs that didn't correctly remove themselves on delete
		if ( m_pDialogs[i].Get() == 0 )
		{
			PopModal( i );
			continue;
		}

		if ( m_pDialogs[i].Get() == hPanel )
		{
			Assert( vgui::ipanel()->IsFullyVisible(hPanel) );

			// Backup hack: If our modal window is no longer visible, make it visible
			if ( !vgui::ipanel()->IsFullyVisible(hPanel) )
			{
				vgui::ipanel()->SetVisible( hPanel, true );
				vgui::ipanel()->MoveToFront( hPanel );
				vgui::ipanel()->RequestFocus( hPanel );

				// Make sure all our parents are visible too
				VPANEL hParent = vgui::ipanel()->GetParent( hPanel );
				while ( hParent != INVALID_PANEL )
				{
					vgui::Panel *pParentPanel = vgui::ipanel()->GetPanel(hParent, "ClientDLL");
					if ( !pParentPanel )
						break;

					vgui::ipanel()->SetVisible( hParent, true );
					hParent = vgui::ipanel()->GetParent( hParent );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
vgui::VPanelHandle CPanelModalStack::Top()
{
	if ( m_pDialogs.Count() == 0 )
	{
		return VPanelHandle();	// Defaults to INVALID_PANEL
	}

	return m_pDialogs[0];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPanelModalStack::IsEmpty() const
{
	return m_pDialogs.Count() == 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGenericWaitingDialog::CGenericWaitingDialog( vgui::Panel *pParent )
	: BaseClass( pParent, "GenericWaitingDialog" )
	, m_bAnimateEllipses(false)
	, m_iNumEllipses(0)	  
{
	if ( pParent == NULL )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
		SetScheme(scheme);
		SetProportional( true );
	}
}

void CGenericWaitingDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	LoadControlSettings( GetResFile(), GetResFilePathId() );
}

void CGenericWaitingDialog::Close()
{
	OnCommand( "close" );
}

void CGenericWaitingDialog::OnCommand( const char *command )
{
	bool bClose = false;
	
	if ( !Q_stricmp( command, "close" ) )
	{
		bClose = true;
	}
	else if ( !Q_stricmp( command, "user_close" ) )
	{
		OnUserClose();
		bClose = true;
	}
	
	if ( bClose )
	{
		TFModalStack()->PopModal( this );
		SetVisible( false );
		MarkForDeletion();			
		return;
	}	
	
	BaseClass::OnCommand( command );
}

void CGenericWaitingDialog::OnTick( void )
{
	BaseClass::OnTick();
	
	if ( !IsVisible() )
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}
	
	if ( m_bAnimateEllipses )
	{
		m_iNumEllipses = ((m_iNumEllipses+1) % 4);
		
		switch ( m_iNumEllipses )
		{
		case 3: SetDialogVariable( "ellipses", L"..." ); break;
		case 2: SetDialogVariable( "ellipses", L".." ); break;
		case 1: SetDialogVariable( "ellipses", L"." ); break;
		default: SetDialogVariable( "ellipses", L"" ); break;
		}
	}

	if ( m_timer.HasStarted() )
	{
		// @note Tom Bui: showing 0 is weird, so just show nothing...
		int iSecondsRemaining = (int)m_timer.GetRemainingTime();
		if ( iSecondsRemaining == 0 )
		{
			SetDialogVariable( "duration", "" );
		}
		else
		{
			SetDialogVariable( "duration", iSecondsRemaining );
		}
		if ( m_timer.IsElapsed() )
		{
			OnCommand( "close" );
			OnTimeout();
		}
	}
}

void CGenericWaitingDialog::ShowStatusUpdate( bool bAnimateEllipses, bool bAllowClose, float flMaxWaitTime )
{
	CExButton *pButton = dynamic_cast<CExButton*>( FindChildByName("CloseButton") );
	if ( pButton )
	{
		pButton->SetVisible( bAllowClose );
		pButton->SetEnabled( bAllowClose );
	}
	
	m_bAnimateEllipses = bAnimateEllipses;
	if ( flMaxWaitTime > 0 )
	{
		m_timer.Start( flMaxWaitTime );
		SetDialogVariable( "duration", (int)flMaxWaitTime );
	}
	else
	{
		m_timer.Invalidate();
		SetDialogVariable( "duration", L"" );
	}
	
	if ( m_bAnimateEllipses )
	{
		m_iNumEllipses = 0;
	}
	
	if ( flMaxWaitTime > 0 || m_bAnimateEllipses )
	{
		vgui::ivgui()->AddTickSignal( GetVPanel(), 500 );
	}
	
	SetDialogVariable( "ellipses", L"" );
}

void CGenericWaitingDialog::OnTimeout()
{
}

void CGenericWaitingDialog::OnUserClose()
{
}

static vgui::DHANDLE< CGenericWaitingDialog > g_WaitingDialog;

void ShowWaitingDialog( CGenericWaitingDialog *pWaitingDialog, const char* pUpdateText, bool bAnimate, bool bShowCancel, float flMaxDuration )
{
	CloseWaitingDialog();
	if ( pWaitingDialog )
	{
		g_WaitingDialog = vgui::SETUP_PANEL( pWaitingDialog );
		g_WaitingDialog->SetVisible( true );
		g_WaitingDialog->MakePopup();
		g_WaitingDialog->MoveToFront();
		g_WaitingDialog->SetKeyBoardInputEnabled(true);
		g_WaitingDialog->SetMouseInputEnabled(true);
		TFModalStack()->PushModal( g_WaitingDialog );

		if ( pUpdateText != NULL )
		{
			g_WaitingDialog->SetDialogVariable( "updatetext", g_pVGuiLocalize->Find( pUpdateText ) );
		}
		g_WaitingDialog->ShowStatusUpdate( bAnimate, bShowCancel, flMaxDuration );
	}
}

void CloseWaitingDialog()
{
	if ( g_WaitingDialog.Get() )
	{
		g_WaitingDialog->Close();
		g_WaitingDialog = NULL;
	}
}

//-----------------------------------------------------------------------------
