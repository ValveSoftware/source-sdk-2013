//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"
#include "vgui_schemevisualizer.h"
#include "vgui/IBorder.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/ComboBox.h"
#include "KeyValues.h"
#include "fmtstr.h"

//----------------------------------------------------------------------------------------

using namespace vgui;

//----------------------------------------------------------------------------------------

class CBorderVisualizerPanel : public Panel
{
	DECLARE_CLASS_SIMPLE( CBorderVisualizerPanel, Panel );
public:
	CBorderVisualizerPanel( Panel *pParent, const char *pName, IBorder *pBorder );

private:
	virtual void Paint();

	IBorder *m_pBorder;
};

//----------------------------------------------------------------------------------------

CBorderVisualizerPanel::CBorderVisualizerPanel( Panel *pParent, const char *pName, IBorder *pBorder )
:	Panel( pParent, pName ),
	m_pBorder( pBorder )
{
	SetBgColor( Color( 255, 0, 0, 255 ) );
}

void CBorderVisualizerPanel::Paint()
{
	BaseClass::Paint();

	surface()->PushMakeCurrent( GetVPanel(), false );
	m_pBorder->Paint( GetVPanel() );
	surface()->PopMakeCurrent( GetVPanel() );
}

//----------------------------------------------------------------------------------------

class CColorVisualizerPanel : public Panel
{
	DECLARE_CLASS_SIMPLE( CColorVisualizerPanel, Panel );
public:
	CColorVisualizerPanel( Panel *pParent, const char *pName, const Color &color );

private:
	virtual void Paint();

	Color m_Color;
};

//----------------------------------------------------------------------------------------

CColorVisualizerPanel::CColorVisualizerPanel( Panel *pParent, const char *pName, const Color &color )
:	Panel( pParent, pName ),
	m_Color( color )
{
}

void CColorVisualizerPanel::Paint()
{
	BaseClass::Paint();

	int nHorzBuffer = XRES( 2 );
	int nVertBuffer = YRES( 2 );

	surface()->DrawSetColor( m_Color );
	surface()->DrawFilledRect( nHorzBuffer, nVertBuffer, GetWide() - 2 * nHorzBuffer, GetTall() - 2 * nVertBuffer );
}

//----------------------------------------------------------------------------------------

CSchemeVisualizer::CSchemeVisualizer( vgui::Panel *pParent, IScheme *pViewScheme, const char *pSchemeName )
:	vgui::Frame( pParent, "SchemeVisualizer" ),
	m_pViewScheme( pViewScheme ),
	m_pList( NULL ),
	m_nListDataType( LISTDATATYPE_INVALID )
{
	CFmtStr fmtTitle( "Scheme Visualizer - scheme: \"%s\"", pSchemeName );
	SetTitle( fmtTitle.Access(), true );
	SetTitleBarVisible( true );
	SetMoveable( true );
	SetCloseButtonVisible( true );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );

	m_pListDataTypeCombo = new ComboBox( this, "DataTypeCombo", 3, false );
	m_aComboDataTypeToItemIDMap[ LISTDATATYPE_BORDERS ] = m_pListDataTypeCombo->AddItem( "Borders", NULL );
	m_aComboDataTypeToItemIDMap[ LISTDATATYPE_FONTS ] = m_pListDataTypeCombo->AddItem( "Fonts", NULL );
	m_aComboDataTypeToItemIDMap[ LISTDATATYPE_COLORS ] = m_pListDataTypeCombo->AddItem( "Colors", NULL );
	m_pListDataTypeCombo->SilentActivateItemByRow( 0 );
	m_pListDataTypeCombo->AddActionSignalTarget( this );

	m_nSelectedComboItem = m_aComboDataTypeToItemIDMap[ LISTDATATYPE_BORDERS ];

	UpdateList( LISTDATATYPE_BORDERS );

	ivgui()->AddTickSignal( GetVPanel(), 100 );
}

CSchemeVisualizer::~CSchemeVisualizer()
{
	ivgui()->RemoveTickSignal( GetVPanel() );
}

void CSchemeVisualizer::PerformLayout()
{
	BaseClass::PerformLayout();

	const int nComboWidth = XRES( 50 );
	m_pListDataTypeCombo->SetBounds(
		GetWide() - nComboWidth - XRES( 10 ),
		YRES( 2 ),
		nComboWidth,
		YRES( 8 )
	);

	const int nHorzBuffer = XRES( 2 );
	const int nVertBuffer = YRES( 10 );
	m_pList->SetBounds( nHorzBuffer, nVertBuffer, GetWide() - 2 * nHorzBuffer, GetTall() - 1.5f * nVertBuffer );
}

void CSchemeVisualizer::UpdateList( ListDataType_t nType )
{
	Assert( nType != m_nListDataType );

	// Cache off type
	m_nListDataType = nType;

	// Clear the list
	if ( m_pList )
	{
		m_pList->MarkForDeletion();
	}
	m_pList = new PanelListPanel( this, "ListPanel" );
	m_pList->SetBgColor( Color( 0, 255, 0, 255 ) );
	m_pList->SetPaintBackgroundEnabled( true );
	InvalidateLayout( true, false );

	// Set default column width - may be changed depending on type
	m_pList->SetFirstColumnWidth( XRES( 50 ) );

	switch( nType )
	{
	case LISTDATATYPE_BORDERS:	AddBordersToList(); break;
	case LISTDATATYPE_FONTS:	AddFontsToList(); break;
	case LISTDATATYPE_COLORS:	AddColorsToList(); break;
	}
}

void CSchemeVisualizer::AddBordersToList()
{
	const int nBorderCount = m_pViewScheme->GetBorderCount();
	for ( int i = 0; i < nBorderCount; ++i )
	{
		IBorder *pCurBorder = m_pViewScheme->GetBorderAtIndex( i );
		CFmtStr fmtName( "BorderPanel_%s", pCurBorder->GetName() );
		CBorderVisualizerPanel *pNewBorderPanel = new CBorderVisualizerPanel( m_pList, fmtName.Access(), pCurBorder );
		pNewBorderPanel->SetSize( m_pList->GetWide(), YRES( 45 ) );
		m_pList->AddItem( new Label( NULL, "Label", pCurBorder->GetName() ), pNewBorderPanel );
	}
}

void CSchemeVisualizer::AddFontsToList()
{
#ifdef POSIX
	const char strOAccent[] = { (char)0xc3, (char)0x93, 0x00 };	// UTF-8 for U+00D3 (LATIN CAPITAL LETTER O WITH ACUTE)
	const char strSkull[] = { (char)0xe2, (char)0x98, (char)0xa0, 0x00 };
#else
	const uint8 strOAccent[] = { 0xd3, 0x00	};
	const char strSkull[] = "";
#endif
	// Stick an intl character in here to test accents (O')
	CFmtStr fmtText( "ABCDEFGHIJKLMN%sPQRSTUVWXYZ%sabcdefhijklmnopqrstuvwxyz0123456789!@#$%%^&*()-_=+", strOAccent, strSkull );

	const int nFontCount = m_pViewScheme->GetFontCount();

	for ( int i = 0; i < nFontCount; ++i )
	{
		HFont hCurFont = m_pViewScheme->GetFontAtIndex( i );
		const char *pCurFontName = m_pViewScheme->GetFontName( hCurFont );
		CFmtStr fmtName( "FontPanel_%s", pCurFontName );

		Label *pNewFontLabel = new Label( m_pList, fmtName.Access(), fmtText.Access() );
		pNewFontLabel->SetFont( hCurFont );
		pNewFontLabel->SizeToContents();
		pNewFontLabel->SetWide( m_pList->GetWide() );
		m_pList->AddItem( new Label( NULL, "Label", pCurFontName ), pNewFontLabel );
	}
}

void CSchemeVisualizer::AddColorsToList()
{
	KeyValues *pColorData = (KeyValues *)m_pViewScheme->GetColorData();
	FOR_EACH_SUBKEY( pColorData, pCurColor )
	{
		const char *pCurColorName = pCurColor->GetName();
		CFmtStr fmtName( "ColorPanel_%s", pCurColorName );

		int r = 0, g = 0, b = 0, a = 0;
		const char *pCurColorRGBA = pCurColor->GetString();
		if ( sscanf( pCurColorRGBA, "%d %d %d %d", &r, &g, &b, &a) < 3 )
		{
			Warning( "Skipping color \"%s\"\n", pCurColorRGBA );
			continue;
		}

		CColorVisualizerPanel *pNewColorPanel = new CColorVisualizerPanel( m_pList, fmtName.Access(), Color(r,g,b,a) );
		pNewColorPanel->SetSize( m_pList->GetWide(), YRES( 25 ) );
		m_pList->AddItem( new Label( NULL, "Label", pCurColorName ), pNewColorPanel );
	}
}

//----------------------------------------------------------------------------------------

static CSchemeVisualizer *g_pSchemeVisualizer = NULL;

//----------------------------------------------------------------------------------------

CON_COMMAND( showschemevisualizer, "Show borders, fonts and colors for a particular scheme.  The default is ClientScheme.res" )
{
	if ( g_pSchemeVisualizer )
	{
		g_pSchemeVisualizer->MarkForDeletion();
		g_pSchemeVisualizer = NULL;
	}

	// Load a scheme - defaults to "ClientScheme"
	const char *pSchemeName = "ClientScheme";
	if ( args.ArgC() == 2 )
	{
		pSchemeName = args.Arg( 1 );
	}
	IScheme *pScheme = scheme()->GetIScheme( scheme()->GetScheme( pSchemeName ) );

	Msg( "Using scheme %s...\n", pSchemeName );

	g_pSchemeVisualizer = vgui::SETUP_PANEL( new CSchemeVisualizer( NULL, pScheme, pSchemeName ) );
	g_pSchemeVisualizer->InvalidateLayout( false, true );

	engine->ClientCmd_Unrestricted( "gameui_activate" );

	const int nWidth = XRES( 300 );
	const int nHeight = YRES( 300 );

	g_pSchemeVisualizer->SetBounds(
		( ScreenWidth() - nWidth ) / 2,
		( ScreenHeight() - nHeight ) / 2,
		nWidth,
		nHeight
	);

	g_pSchemeVisualizer->Activate();
}

void CSchemeVisualizer::OnTick()
{
	const int nItemID = m_pListDataTypeCombo->GetActiveItem();
	if ( m_nSelectedComboItem == nItemID )
		return;

	// Cache
	m_nSelectedComboItem = nItemID;

	// Figure out which type was selected
	ListDataType_t nType = LISTDATATYPE_INVALID;
	for ( int i = 0; i < (int)NUM_TYPES; ++i )
	{
		if ( nItemID == m_aComboDataTypeToItemIDMap[ i ] )
		{
			nType = (ListDataType_t)i;
			break;
		}
	}

	AssertMsg( nType != LISTDATATYPE_INVALID, "Couldn't find item ID in list - this should never happen." );

	// Update the list now
	UpdateList( nType );
}
