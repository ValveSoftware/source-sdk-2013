//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Displays easy, flexible VGui text. Mapbase equivalent of point_worldtext.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "panelmetaclassmgr.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/IVGui.h>
#include "ienginevgui.h"
#include "c_vguiscreen.h"
#include "vgui_bitmapbutton.h"
#include "vgui_bitmappanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// vgui_text_display
//-----------------------------------------------------------------------------
class C_VGuiTextDisplay : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_VGuiTextDisplay, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_VGuiTextDisplay();
	~C_VGuiTextDisplay();

	virtual void PostDataUpdate( DataUpdateType_t updateType );

	bool IsEnabled( void ) const { return m_bEnabled; }

	const char *GetDisplayText( void ) const { return m_szDisplayText; }
	const char *GetFontName( void ) const { return m_szFont; }
	int GetResolution( void ) const { return m_iResolution; }
	vgui::Label::Alignment GetContentAlignment() const { return m_iContentAlignment; }

	bool NeedsTextUpdate() { return m_bTextNeedsUpdate; }
	void UpdatedText() { m_bTextNeedsUpdate = false; }

private:
	bool	m_bEnabled;
	char	m_szDisplayText[256];
	vgui::Label::Alignment	m_iContentAlignment;
	char	m_szFont[64];
	int		m_iResolution;

	bool	m_bTextNeedsUpdate;
};

IMPLEMENT_CLIENTCLASS_DT( C_VGuiTextDisplay, DT_VGuiTextDisplay, CVGuiTextDisplay )
	RecvPropBool( RECVINFO( m_bEnabled ) ),
	RecvPropString( RECVINFO( m_szDisplayText ) ),
	RecvPropInt( RECVINFO( m_iContentAlignment ) ),
	RecvPropString( RECVINFO( m_szFont ) ),
	RecvPropInt( RECVINFO( m_iResolution ) ),
END_RECV_TABLE()

C_VGuiTextDisplay::C_VGuiTextDisplay()
{
}

C_VGuiTextDisplay::~C_VGuiTextDisplay()
{
}

void C_VGuiTextDisplay::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	// For now, always update
	m_bTextNeedsUpdate = true;
}

using namespace vgui;

//-----------------------------------------------------------------------------
// Control screen 
//-----------------------------------------------------------------------------
class C_TextDisplayPanel : public CVGuiScreenPanel
{
	DECLARE_CLASS( C_TextDisplayPanel, CVGuiScreenPanel );

public:
	C_TextDisplayPanel( vgui::Panel *parent, const char *panelName );
	~C_TextDisplayPanel( void );

	virtual void ApplySchemeSettings( IScheme *pScheme );

	void UpdateText();

	virtual bool Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData );
	virtual void OnTick( void );
	virtual void Paint( void );

private:

	CHandle<C_VGuiScreen>	m_hVGUIScreen;
	CHandle<C_VGuiTextDisplay>	m_hScreenEntity;

	// VGUI specifics
	Label			*m_pDisplayTextLabel;
};

DECLARE_VGUI_SCREEN_FACTORY( C_TextDisplayPanel, "text_display_panel" );

CUtlVector <C_TextDisplayPanel *>	g_TextDisplays;

//-----------------------------------------------------------------------------
// Constructor: 
//-----------------------------------------------------------------------------
C_TextDisplayPanel::C_TextDisplayPanel( vgui::Panel *parent, const char *panelName )
: BaseClass( parent, "C_TextDisplayPanel"/*, vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/WorldTextPanel.res", "WorldTextPanel" )*/ ) 
{
	// Add ourselves to the global list of movie displays
	g_TextDisplays.AddToTail( this );
}

//-----------------------------------------------------------------------------
// Purpose: Clean up the movie
//-----------------------------------------------------------------------------
C_TextDisplayPanel::~C_TextDisplayPanel( void )
{
	// Remove ourselves from the global list of movie displays
	g_TextDisplays.FindAndRemove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Setup our scheme
//-----------------------------------------------------------------------------
void C_TextDisplayPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	/*
	m_pDisplayTextLabel->SetFgColor( Color( 255, 255, 255, 255 ) );
	m_pDisplayTextLabel->SetText( "" );
	m_pDisplayTextLabel->SetVisible( false );
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TextDisplayPanel::UpdateText()
{
	color32 clr = m_hScreenEntity->GetRenderColor();

	m_pDisplayTextLabel->SetFgColor( Color( clr.r, clr.g, clr.b, clr.a ) );
	m_pDisplayTextLabel->SetText( m_hScreenEntity->GetDisplayText() );

	//SetSize( m_hScreenEntity->GetTextSize(), m_hScreenEntity->GetTextSize() );
	//m_pDisplayTextLabel->SetSize( m_hScreenEntity->GetTextSize(), m_hScreenEntity->GetTextSize() );

	Label::Alignment iAlignment = m_hScreenEntity->GetContentAlignment();

	switch (iAlignment)
	{
		// Use a special scaling method when using a south alignment
		case Label::Alignment::a_southwest:
		case Label::Alignment::a_south:
		case Label::Alignment::a_southeast:
			int lW, lT;
			m_pDisplayTextLabel->GetContentSize( lW, lT );
			SetSize( m_hScreenEntity->GetResolution(), lT );
			m_pDisplayTextLabel->SetSize( m_hScreenEntity->GetResolution(), lT );

			float sW, sT;
			m_hVGUIScreen->GetSize( sW, sT );
			//Msg( "Screen width: %f, new height: %f\n", sW, sW * (lT / m_hScreenEntity->GetResolution()) );
			m_hVGUIScreen->SetHeight( sW * ((float)lT / (float)m_hScreenEntity->GetResolution()) );
			m_hVGUIScreen->SetPixelHeight( lT );
			break;

		default:
			SetSize( m_hScreenEntity->GetResolution(), m_hScreenEntity->GetResolution() );
			m_pDisplayTextLabel->SetSize( m_hScreenEntity->GetResolution(), m_hScreenEntity->GetResolution() );
			break;
	}

	m_pDisplayTextLabel->SetContentAlignment( iAlignment );

	bool bWrap = true;
	bool bCenterWrap = false;
	switch (iAlignment)
	{
		// Center wrap if centered
		case Label::Alignment::a_north:
		case Label::Alignment::a_center:
		case Label::Alignment::a_south:
			bCenterWrap = true;
			break;

		// HACKHACK: Don't wrap if using an east alignment
		case Label::Alignment::a_northeast:
		case Label::Alignment::a_east:
		case Label::Alignment::a_southeast:
			bWrap = false;
			break;
	}

	m_pDisplayTextLabel->SetWrap( bWrap );
	m_pDisplayTextLabel->SetCenterWrap( bCenterWrap );

	//Msg( "Resolution is %i\n", m_hScreenEntity->GetResolution() );

	const char *pszFontName = m_hScreenEntity->GetFontName();
	if (pszFontName && pszFontName[0] != '\0')
	{
		HFont font = scheme()->GetIScheme( GetScheme() )->GetFont( pszFontName );
		m_pDisplayTextLabel->SetFont( font );
	}

	m_pDisplayTextLabel->SetVisible( true );
}

//-----------------------------------------------------------------------------
// Initialization 
//-----------------------------------------------------------------------------
bool C_TextDisplayPanel::Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData )
{
	if ( !BaseClass::Init( pKeyValues, pInitData ) )
		return false;

	// Make sure we get ticked...
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_pDisplayTextLabel = dynamic_cast<vgui::Label*>(FindChildByName( "TextDisplay" ));

	// Save this for simplicity later on
	m_hVGUIScreen = dynamic_cast<C_VGuiScreen *>( GetEntity() );
	if ( m_hVGUIScreen != NULL )
	{
		// Also get the associated entity
		m_hScreenEntity = dynamic_cast<C_VGuiTextDisplay *>(m_hVGUIScreen->GetOwnerEntity());
		UpdateText();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Update the display string
//-----------------------------------------------------------------------------
void C_TextDisplayPanel::OnTick()
{
	if (m_hScreenEntity->NeedsTextUpdate())
	{
		UpdateText();
		m_hScreenEntity->UpdatedText();
	}

	BaseClass::OnTick();
}

ConVar r_vguitext_bg( "r_vguitext_bg", "0" );

//-----------------------------------------------------------------------------
// Purpose: Update and draw the frame
//-----------------------------------------------------------------------------
void C_TextDisplayPanel::Paint( void )
{
	// Black out the background (we could omit drawing under the video surface, but this is straight-forward)
	if ( r_vguitext_bg.GetBool() )
	{
		surface()->DrawSetColor( 0, 0, 0, 255 );
		surface()->DrawFilledRect( 0, 0, GetWide(), GetTall() );

		//surface()->DrawSetColor( 64, 64, 64, 255 );
		//surface()->DrawFilledRect( 0, 0, m_pDisplayTextLabel->GetWide(), m_pDisplayTextLabel->GetTall() );
	}

	// Parent's turn
	BaseClass::Paint();
}
