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
#include "tf_controls.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/PropertyPage.h"
#include "econ_item_system.h"
#include "iachievementmgr.h"
#include "clientmode_tf.h"
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/MessageBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/TextEntry.h>
#include <../common/GameUI/cvarslider.h>
#include "filesystem.h"
#include "hud_controlpointicons.h"

using namespace vgui;

wchar_t* LocalizeNumberWithToken( const char* pszLocToken, int nValue )
{
	static wchar_t wszOutString[ 256 ];
	wchar_t wszCount[ 16 ];
	_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", nValue );
	const wchar_t *wpszFormat = g_pVGuiLocalize->Find( pszLocToken );
	g_pVGuiLocalize->ConstructString_safe( wszOutString, wpszFormat, 1, wszCount );
	
	return wszOutString;
}

wchar_t* LocalizeNumberWithToken( const char* pszLocToken, int nValue1, int nValue2 )
{
	static wchar_t wszOutString[ 256 ];
	wchar_t wszCount1[ 16 ];
	wchar_t wszCount2[ 16 ];
	_snwprintf( wszCount1, ARRAYSIZE( wszCount1 ), L"%d", nValue1 );
	_snwprintf( wszCount2, ARRAYSIZE( wszCount2 ), L"%d", nValue2 );
	const wchar_t *wpszFormat = g_pVGuiLocalize->Find( pszLocToken );
	g_pVGuiLocalize->ConstructString_safe( wszOutString, wpszFormat, 2, wszCount1, wszCount2 );

	return wszOutString;
}

void GetPlayerNameForSteamID( wchar_t *wCharPlayerName, int nBufSizeBytes, const CSteamID &steamID )
{
	const char *pszName = steamapicontext->SteamFriends()->GetFriendPersonaName( steamID );
	V_UTF8ToUnicode( pszName, wCharPlayerName, nBufSizeBytes );
}

bool BGeneralPaintSetup( const Color& color )
{
	static int snWhiteTextureID = -1;
	if ( snWhiteTextureID == -1 )
	{
		snWhiteTextureID = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( snWhiteTextureID, "vgui/white" , true, false);
		if (snWhiteTextureID == -1)
			return false;

	}

	vgui::surface()->DrawSetTexture( snWhiteTextureID );
	vgui::surface()->DrawSetColor( color );

	return true;
}

void DrawColoredCircle( float flXPos, float flYPos, float flRadius, const Color& color )
{
	if ( !BGeneralPaintSetup( color ) )
		return;

	// Create a circle by creating verts
	const int nNumSmallSegments = 20;
	const int nNumLargeSements = 50;
	int nNumSegments = RemapValClamped( flRadius, 50.f, 300.f, (float)nNumSmallSegments, (float)nNumLargeSements );

	surface()->DrawOutlinedCircle( flXPos, flYPos, flRadius, nNumSegments );
}

void DrawFilledColoredCircle( float flXPos, float flYPos, float flRadius, const Color& color )
{
	if ( !BGeneralPaintSetup( color ) )
		return;

	// Create a circle by creating verts
	const int nNumSmallSegments = 20;
	const int nNumLargeSements = 50;
	int nNumSegments = RemapValClamped( flRadius, 50.f, 300.f, (float)nNumSmallSegments, (float)nNumLargeSements );

	vgui::Vertex_t verts[ nNumLargeSements ];

	float invDelta = 2.0f * M_PI / nNumSegments;
	for ( int i = 0; i < nNumSegments; ++i )
	{
		float flRadians = i * invDelta;
		float ca = cos( flRadians );
		float sa = sin( flRadians );

		// Rotate it around the circle
		verts[i].m_Position.x = flXPos + (flRadius * ca);
		verts[i].m_Position.y = flYPos + (flRadius * sa);
		verts[i].m_TexCoord.x = 0.5f * (ca + 1.0f);
		verts[i].m_TexCoord.y = 0.5f * (sa + 1.0f);
	}

	surface()->DrawTexturedPolygon( nNumSegments, verts, false );
}

void DrawFilledColoredCircleSegment( float flXPos, float flYPos, float flRadiusOuter, float flRadiusInner, const Color& color, float flStartAngle, float flEndAngle, bool bCW /* = true */ )
{
	DrawFilledColoredCircleSegment( flXPos, flYPos, flRadiusOuter, flRadiusInner, color, flStartAngle, flEndAngle, flStartAngle, flEndAngle, bCW );
}

void DrawFilledColoredCircleSegment( float flXPos, float flYPos, float flRadiusOuter, float flRadiusInner, const Color& color, float flStartAngleOuter, float flEndAngleOuter, float flStartAngleInner, float flEndAngleInner, bool bCW /*= true*/ )
{
	if ( !BGeneralPaintSetup( color ) )
		return;

	if ( flEndAngleOuter < flStartAngleOuter )
	{
		flEndAngleOuter += 360.f;
	}

	if ( flEndAngleInner < flStartAngleInner )
	{
		flEndAngleInner += 360.f;
	}

	// Create a circle by creating verts
	const int nNumSmallSegments = 20;
	const int nNumLargeSements = 50;
	float flAngleDelta[2];
	flAngleDelta[ 0 ] = flEndAngleOuter - flStartAngleOuter;
	flAngleDelta[ 1 ] = flEndAngleInner - flStartAngleInner;

	//if ( flAngleDelta > 180.f )
	//{
	//	DrawFilledColoredCircleSegment( flXPos, flYPos, flRadiusOuter, flRadiusInner, color, flStartAngle, flStartAngle + 180.f, bCW );
	//	flStartAngle += 180.f;
	//	flAngleDelta -= 180.f;
	//}

	if ( !bCW )
	{
		flStartAngleOuter = flStartAngleOuter - flAngleDelta[ 0 ];
		flEndAngleOuter = flStartAngleOuter + flAngleDelta[ 0 ];

		flStartAngleInner = flStartAngleInner - flAngleDelta[ 1 ];
		flEndAngleInner = flStartAngleInner + flAngleDelta[ 1 ];
	}

	int nNumSegments = RemapValClamped( flRadiusOuter, 50.f, 300.f, (float)nNumSmallSegments, (float)nNumLargeSements );
	nNumSegments = RemapValClamped( Max( flAngleDelta[ 0 ], flAngleDelta[ 1 ] ), 1, 360, 1, nNumSegments );

	float flStartRadAngle[ 2 ];
	float flDeltaRadAngle[ 2 ];

	flStartRadAngle[ 0 ] = DEG2RAD(flStartAngleOuter);
	flDeltaRadAngle[ 0 ] = DEG2RAD(flAngleDelta[ 0 ]);
	flStartRadAngle[ 1 ] = DEG2RAD(flStartAngleInner);
	flDeltaRadAngle[ 1 ] = DEG2RAD(flAngleDelta[ 1 ]);

	vgui::Vertex_t verts[ 4 ];

	float invDelta[ 2 ] = { flDeltaRadAngle[ 0 ] / nNumSegments, flDeltaRadAngle[ 1 ] / nNumSegments };
	float fl90 = DEG2RAD( 90 );

	auto lambdaCompVerts = [&]( float flAngOuter, float flAngInner, vgui::Vertex_t& vert0, vgui::Vertex_t& vert1 )
	{
		float ca = cos( flAngOuter - fl90 );
		float sa = sin( flAngOuter - fl90 );

		// Rotate it around the circle
		vert0.m_Position.x = flXPos + (flRadiusOuter * ca);
		vert0.m_Position.y = flYPos + (flRadiusOuter * sa);
		vert0.m_TexCoord.x = 0.5f * (ca + 1.0f);
		vert0.m_TexCoord.y = 0.5f * (sa + 1.0f);

		ca = cos( flAngInner - fl90 );
		sa = sin( flAngInner - fl90 );

		vert1.m_Position.x = flXPos + (flRadiusInner * ca);
		vert1.m_Position.y = flYPos + (flRadiusInner * sa);
		vert1.m_TexCoord.x = 0.5f * (ca + 1.0f);
		vert1.m_TexCoord.y = 0.5f * (sa + 1.0f);
	};

	// Seed 2 and 3 so the loop below works
	lambdaCompVerts( flStartRadAngle[ 0 ], flStartRadAngle[ 1 ], verts[ 2 ], verts[ 3 ] ); 

	for ( int i = 0; i <= nNumSegments; ++i )
	{
		// Use the previous leading-edge verts as the start-edge verts for
		// the next section.
		verts[ 0 ] = verts[ 3 ];
		verts[ 1 ] = verts[ 2 ];
		// Compute the leading-edge verts
		lambdaCompVerts( flStartRadAngle[ 0 ] + i * invDelta[ 0 ],
						 flStartRadAngle[ 1 ] + i * invDelta[ 1 ],
						 verts[ 2 ], verts[ 3 ] );

		surface()->DrawTexturedPolygon( 4, verts, true );
	}
}

void BrigthenColor( Color& color, int nBrigthenAmount )
{
	color.SetColor( Min( 255, color.r() + nBrigthenAmount ),
					Min( 255, color.g() + nBrigthenAmount ),
					Min( 255, color.b() + nBrigthenAmount ),
					color.a() );
};

//-----------------------------------------------------------------------------
// Purpose: Tests a position for moving a tooltip panel and returns a score.
//			Returns how many pixels of the mouseover panel are covered by the 
//			tooltip panel.  A score of 0 is perfect.
//-----------------------------------------------------------------------------
int AttemptPositionTooltip( const tooltippos_t eTooltipPosition, 
							 Panel* pMouseOverPanel,
							 Panel *pToolTipPanel,
							 int &iXPos, 
							 int &iYPos )
{
	int iPanelX = pMouseOverPanel->GetXPos();
	int iPanelY = pMouseOverPanel->GetYPos();
	pMouseOverPanel->ParentLocalToScreen( iPanelX, iPanelY );

	switch ( eTooltipPosition )
	{
		case TTP_LEFT:
			iXPos = (iPanelX - pToolTipPanel->GetWide() + XRES(18));
			iYPos = iPanelY - YRES(7);
			break;
		case TTP_RIGHT: 
			iXPos = (iPanelX + pMouseOverPanel->GetWide() - XRES(20));
			iYPos = iPanelY - YRES(7);
			break;
		case TTP_LEFT_CENTERED:
			iXPos = (iPanelX - pToolTipPanel->GetWide()) - XRES(4);
			iYPos = (iPanelY - (pToolTipPanel->GetTall() * 0.5));
			break;
		case TTP_RIGHT_CENTERED:
			iXPos = (iPanelX + pMouseOverPanel->GetWide()) + XRES(4);
			iYPos = (iPanelY - (pToolTipPanel->GetTall() * 0.5));
			break;
		case TTP_ABOVE:
			iXPos = (iPanelX + (pMouseOverPanel->GetWide() * 0.5)) - (pToolTipPanel->GetWide() * 0.5);
			iYPos = (iPanelY - pToolTipPanel->GetTall() - YRES(4));
			break;
		case TTP_BELOW:
			iXPos = (iPanelX + (pMouseOverPanel->GetWide() * 0.5)) - (pToolTipPanel->GetWide() * 0.5);
			iYPos = (iPanelY + pMouseOverPanel->GetTall() + YRES(4));
			break;
	}

	int iScreenX, iScreenY;
	surface()->GetScreenSize( iScreenX, iScreenY );

	// Make sure the panel stays on screen
	iXPos = clamp( iXPos, 0, iScreenX - pToolTipPanel->GetWide() );
	iYPos = clamp( iYPos, 0, iScreenY - pToolTipPanel->GetTall() );

	// Detect how much overlap we have in X
	int nXMin = Max( iXPos, pMouseOverPanel->GetXPos() );
	int nXMax = Min( iXPos + pToolTipPanel->GetWide(), pMouseOverPanel->GetXPos() + pMouseOverPanel->GetWide() );
	int nXScore = Max( 0, nXMax - nXMin );

	// Detect overlap in Y
	int nYMin = Max( iYPos, pMouseOverPanel->GetYPos() );
	int nYMax = Min( iYPos + pToolTipPanel->GetTall(), pMouseOverPanel->GetYPos() + pMouseOverPanel->GetTall() );
	int nYScore = Max( 0, nYMax - nYMin );

	return nXScore + nYScore;
}

//-----------------------------------------------------------------------------
// Purpose: Takes a mouse over panel, a tooltip panel and a preferred position
//			and tries to move the tooltip into a nice position next to the
//			mouse over panel.
//-----------------------------------------------------------------------------
void PositionTooltip( const tooltippos_t ePreferredTooltipPosition, 
					  vgui::Panel* pMouseOverPanel,
					  vgui::Panel *pToolTipPanel )
{
	struct PosResult_t
	{
		int nScore;
		int nX;
		int nY;
	};
	PosResult_t arResults[ MAX_POSITIONS ];

	int nBestScore = INT_MAX;
	tooltippos_t eBestType = MAX_POSITIONS;

	float flAverageScore = 0;
	// Try all of the positions and score each by how much they overlap.
	for( int i=0; i < MAX_POSITIONS; ++i )
	{
		arResults[ i ].nScore = AttemptPositionTooltip( (tooltippos_t)i,
														pMouseOverPanel,
														pToolTipPanel,
														arResults[ i ].nX,
														arResults[ i ].nY );
		
		flAverageScore += arResults[ i ].nScore;
		// 0 is a perfect score.
		if ( arResults[ i ].nScore < nBestScore )
		{
			eBestType = (tooltippos_t)i;
			nBestScore = arResults[ i ].nScore;
		}
	}

	flAverageScore /= MAX_POSITIONS;

	Assert( eBestType != MAX_POSITIONS );
	// Go ahead and use their preferred if it's decent
	if( arResults[ ePreferredTooltipPosition ].nScore < ( flAverageScore / 2.f ) )
	{
		pToolTipPanel->SetPos( arResults[ ePreferredTooltipPosition ].nX,
							   arResults[ ePreferredTooltipPosition ].nY );
		return;
	}
	
	// Go with the best we've got
	pToolTipPanel->SetPos( arResults[ eBestType ].nX, arResults[ eBestType ].nY );
}

DECLARE_BUILD_FACTORY( CExCheckButton );
DECLARE_BUILD_FACTORY( CTFFooter );

//-----------------------------------------------------------------------------
// Purpose: Xbox-specific panel that displays button icons text labels
//-----------------------------------------------------------------------------
CTFFooter::CTFFooter( Panel *parent, const char *panelName ) : BaseClass( parent, panelName ) 
{
	SetVisible( true );
	SetAlpha( 0 );

	m_nButtonGap = 32;
	m_ButtonPinRight = 100;
	m_FooterTall = 80;

	m_ButtonOffsetFromTop = 0;
	m_ButtonSeparator = 4;
	m_TextAdjust = 0;

	m_bPaintBackground = false;
	m_bCenterHorizontal = true;

	m_szButtonFont[0] = '\0';
	m_szTextFont[0] = '\0';
	m_szFGColor[0] = '\0';
	m_szBGColor[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFooter::~CTFFooter()
{
	ClearButtons();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_hButtonFont = pScheme->GetFont( ( m_szButtonFont[0] != '\0' ) ? m_szButtonFont : "GameUIButtons" );
	m_hTextFont = pScheme->GetFont( ( m_szTextFont[0] != '\0' ) ? m_szTextFont : "MenuLarge" );

	SetFgColor( pScheme->GetColor( m_szFGColor, Color( 255, 255, 255, 255 ) ) );
	SetBgColor( pScheme->GetColor( m_szBGColor, Color( 0, 0, 0, 255 ) ) );

	int x, y, w, h;
	GetParent()->GetBounds( x, y, w, h );
	SetBounds( x, h - m_FooterTall, w, m_FooterTall );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	// gap between hints
	m_nButtonGap = inResourceData->GetInt( "buttongap", 32 );
	m_ButtonPinRight = inResourceData->GetInt( "button_pin_right", 100 );
	m_FooterTall = inResourceData->GetInt( "tall", 80 );
	m_ButtonOffsetFromTop = inResourceData->GetInt( "buttonoffsety", 0 );
	m_ButtonSeparator = inResourceData->GetInt( "button_separator", 4 );
	m_TextAdjust = inResourceData->GetInt( "textadjust", 0 );

	m_bCenterHorizontal = ( inResourceData->GetInt( "center", 1 ) == 1 );
	m_bPaintBackground = ( inResourceData->GetInt( "paintbackground", 0 ) == 1 );

	// fonts for text and button
	Q_strncpy( m_szTextFont, inResourceData->GetString( "fonttext", "MenuLarge" ), sizeof( m_szTextFont ) );
	Q_strncpy( m_szButtonFont, inResourceData->GetString( "fontbutton", "GameUIButtons" ), sizeof( m_szButtonFont ) );

	// fg and bg colors
	Q_strncpy( m_szFGColor, inResourceData->GetString( "fgcolor", "White" ), sizeof( m_szFGColor ) );
	Q_strncpy( m_szBGColor, inResourceData->GetString( "bgcolor", "Black" ), sizeof( m_szBGColor ) );

	// clear the buttons because we're going to re-add them here
	ClearButtons();

	for ( KeyValues *pButton = inResourceData->GetFirstSubKey(); pButton != NULL; pButton = pButton->GetNextKey() )
	{
		const char *pNameButton = pButton->GetName();

		if ( !Q_stricmp( pNameButton, "button" ) )
		{
			// Add a button to the footer
			const char *pName = pButton->GetString( "name", "NULL" );
			const char *pText = pButton->GetString( "text", "NULL" );
			const char *pIcon = pButton->GetString( "icon", "NULL" );
			AddNewButtonLabel( pName, pText, pIcon );
		}
	}

	InvalidateLayout( false, true ); // force ApplySchemeSettings to run
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::AddNewButtonLabel( const char *name, const char *text, const char *icon )
{
	FooterButton_t *button = new FooterButton_t;

	button->bVisible = true;
	Q_strncpy( button->name, name, sizeof( button->name ) );

	// Button icons are a single character
	wchar_t *pIcon = g_pVGuiLocalize->Find( icon );
	if ( pIcon )
	{
		button->icon[0] = pIcon[0];
		button->icon[1] = '\0';
	}
	else
	{
		button->icon[0] = '\0';
	}

	// Set the help text
	wchar_t *pText = g_pVGuiLocalize->Find( text );
	if ( pText )
	{
		wcsncpy( button->text, pText, wcslen( pText ) + 1 );
	}
	else
	{
		button->text[0] = '\0';
	}

	m_Buttons.AddToTail( button );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::ShowButtonLabel( const char *name, bool show )
{
	for ( int i = 0; i < m_Buttons.Count(); ++i )
	{
		if ( !Q_stricmp( m_Buttons[ i ]->name, name ) )
		{
			m_Buttons[ i ]->bVisible = show;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::PaintBackground( void )
{
	if ( !m_bPaintBackground )
		return;

	BaseClass::PaintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::Paint( void )
{
	// inset from right edge
	int wide = GetWide();

	// center the text within the button
	int buttonHeight = vgui::surface()->GetFontTall( m_hButtonFont );
	int fontHeight = vgui::surface()->GetFontTall( m_hTextFont );
	int textY = ( buttonHeight - fontHeight )/2 + m_TextAdjust;

	if ( textY < 0 )
	{
		textY = 0;
	}

	int y = m_ButtonOffsetFromTop;

	if ( !m_bCenterHorizontal )
	{
		// draw the buttons, right to left
		int x = wide - m_ButtonPinRight;

		vgui::Label label( this, "temp", L"" );
		for ( int i = m_Buttons.Count() - 1 ; i >= 0 ; --i )
		{
			FooterButton_t *pButton = m_Buttons[i];
			if ( !pButton->bVisible )
				continue;

			// Get the string length
			label.SetFont( m_hTextFont );
			label.SetText( pButton->text );
			label.SizeToContents();

			int iTextWidth = label.GetWide();

			if ( iTextWidth == 0 )
				x += m_nButtonGap;	// There's no text, so remove the gap between buttons
			else
				x -= iTextWidth;

			// Draw the string
			vgui::surface()->DrawSetTextFont( m_hTextFont );
			vgui::surface()->DrawSetTextColor( GetFgColor() );
			vgui::surface()->DrawSetTextPos( x, y + textY );
			vgui::surface()->DrawPrintText( pButton->text, wcslen( pButton->text ) );

			// Draw the button
			// back up button width and a little extra to leave a gap between button and text
			x -= ( vgui::surface()->GetCharacterWidth( m_hButtonFont, pButton->icon[0] ) + m_ButtonSeparator );
			vgui::surface()->DrawSetTextFont( m_hButtonFont );
			vgui::surface()->DrawSetTextColor( 255, 255, 255, 255 );
			vgui::surface()->DrawSetTextPos( x, y );
			vgui::surface()->DrawPrintText( pButton->icon, 1 );

			// back up to next string
			x -= m_nButtonGap;
		}
	}
	else
	{
		// center the buttons (as a group)
		int x = wide / 2;
		int totalWidth = 0;
		int i = 0;
		int nButtonCount = 0;

		vgui::Label label( this, "temp", L"" );

		// need to loop through and figure out how wide our buttons and text are (with gaps between) so we can offset from the center
		for ( i = 0; i < m_Buttons.Count(); ++i )
		{
			FooterButton_t *pButton = m_Buttons[i];

			if ( !pButton->bVisible )
				continue;

			// Get the string length
			label.SetFont( m_hTextFont );
			label.SetText( pButton->text );
			label.SizeToContents();

			totalWidth += vgui::surface()->GetCharacterWidth( m_hButtonFont, pButton->icon[0] );
			totalWidth += m_ButtonSeparator;
			totalWidth += label.GetWide();

			nButtonCount++; // keep track of how many active buttons we'll be drawing
		}

		totalWidth += ( nButtonCount - 1 ) * m_nButtonGap; // add in the gaps between the buttons
		x -= ( totalWidth / 2 );

		for ( i = 0; i < m_Buttons.Count(); ++i )
		{
			FooterButton_t *pButton = m_Buttons[i];

			if ( !pButton->bVisible )
				continue;

			// Get the string length
			label.SetFont( m_hTextFont );
			label.SetText( pButton->text );
			label.SizeToContents();

			int iTextWidth = label.GetWide();

			// Draw the icon
			vgui::surface()->DrawSetTextFont( m_hButtonFont );
			vgui::surface()->DrawSetTextColor( 255, 255, 255, 255 );
			vgui::surface()->DrawSetTextPos( x, y );
			vgui::surface()->DrawPrintText( pButton->icon, 1 );
			x += vgui::surface()->GetCharacterWidth( m_hButtonFont, pButton->icon[0] ) + m_ButtonSeparator;

			// Draw the string
			vgui::surface()->DrawSetTextFont( m_hTextFont );
			vgui::surface()->DrawSetTextColor( GetFgColor() );
			vgui::surface()->DrawSetTextPos( x, y + textY );
			vgui::surface()->DrawPrintText( pButton->text, wcslen( pButton->text ) );

			x += iTextWidth + m_nButtonGap;
		}
	}
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::ClearButtons( void )
{
	m_Buttons.PurgeAndDeleteElements();
}

#define OPTIONS_DIR "cfg"
#define DEFAULT_OPTIONS_FILE OPTIONS_DIR "/user_default.scr"
#define OPTIONS_FILE OPTIONS_DIR "/user.scr"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFAdvancedOptionsDialog::CTFAdvancedOptionsDialog(vgui::Panel *parent) : BaseClass(NULL, "TFAdvancedOptionsDialog")
{
	// Need to use the clientscheme (we're not parented to a clientscheme'd panel)
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	m_pListPanel = new vgui::PanelListPanel( this, "PanelListPanel" );

	m_pList = NULL;

	m_pToolTip = new CTFTextToolTip( this );
	m_pToolTipEmbeddedPanel = new vgui::EditablePanel( this, "TooltipPanel" );
	m_pToolTipEmbeddedPanel->SetKeyBoardInputEnabled( false );
	m_pToolTipEmbeddedPanel->SetMouseInputEnabled( false );
	m_pToolTip->SetEmbeddedPanel( m_pToolTipEmbeddedPanel );
	m_pToolTip->SetTooltipDelay( 0 );

	m_pDescription = new CInfoDescription();
	m_pDescription->InitFromFile( DEFAULT_OPTIONS_FILE );
	m_pDescription->InitFromFile( OPTIONS_FILE, false );
	m_pDescription->TransferCurrentValues( NULL );

// 	MoveToCenterOfScreen();
// 	SetSizeable( false );
// 	SetDeleteSelfOnClose( true );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFAdvancedOptionsDialog::~CTFAdvancedOptionsDialog()
{
	delete m_pDescription;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAdvancedOptionsDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings("resource/ui/TFAdvancedOptionsDialog.res");
	m_pListPanel->SetFirstColumnWidth( 0 );

	CreateControls();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAdvancedOptionsDialog::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAdvancedOptionsDialog::OnClose()
{
	BaseClass::OnClose();

	TFModalStack()->PopModal( this );
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CTFAdvancedOptionsDialog::OnCommand( const char *command )
{
	if ( !stricmp( command, "open_chat_filter_settings" ) )
	{
		if ( steamapicontext && steamapicontext->SteamFriends() )
		{
			switch ( GetUniverse() )
			{
			case k_EUniversePublic:
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "https://store.steampowered.com/account/preferences#CommunityContentPreferences" );
				break;
			case k_EUniverseBeta:
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "https://store.beta.steampowered.com/account/preferences#CommunityContentPreferences" );
				break;
			case k_EUniverseDev:
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "https://localhost/store/account/preferences#CommunityContentPreferences" );
				break;
			}
		}
		return;
	}
	else if ( !stricmp( command, "Ok" ) )
	{
		// OnApplyChanges();
		SaveValues();
		OnClose();
		return;
	}
	else if ( !stricmp( command, "Close" ) )
	{
		OnClose();
		return;
	}

	BaseClass::OnCommand( command );
}

void CTFAdvancedOptionsDialog::OnKeyCodeTyped(KeyCode code)
{
	// force ourselves to be closed if the escape key it pressed
	if ( code == KEY_ESCAPE )
	{
		OnClose();
	}
	else
	{
		BaseClass::OnKeyCodeTyped(code);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAdvancedOptionsDialog::OnKeyCodePressed(KeyCode code)
{
	// force ourselves to be closed if the escape key it pressed
	if ( GetBaseButtonCode( code ) == KEY_XBUTTON_B || GetBaseButtonCode( code ) == STEAMCONTROLLER_B || GetBaseButtonCode( code ) == STEAMCONTROLLER_START )
	{
		OnClose();
	}
	else
	{
		BaseClass::OnKeyCodePressed(code);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAdvancedOptionsDialog::GatherCurrentValues()
{
	if ( !m_pDescription )
		return;

	// OK
	CheckButton *pBox;
	TextEntry *pEdit;
	ComboBox *pCombo;
	CCvarSlider *pSlider;

	mpcontrol_t *pList;

	CScriptObject *pObj;
	CScriptListItem *pItem;

	char szValue[256];
	char strValue[ 256 ];

	pList = m_pList;
	while ( pList )
	{
		pObj = pList->pScrObj;

		if ( pObj->type == O_CATEGORY || pObj->type == O_BUTTON )
		{
			pList = pList->next;
			continue;
		}

		if ( !pList->pControl )
		{
			pObj->SetCurValue( pObj->defValue );
			pList = pList->next;
			continue;
		}

		switch ( pObj->type )
		{
		case O_BOOL:
			pBox = (CheckButton *)pList->pControl;
			sprintf( szValue, "%s", pBox->IsSelected() ? "1" : "0" );
			break;
		case O_NUMBER:
			pEdit = ( TextEntry * )pList->pControl;
			pEdit->GetText( strValue, sizeof( strValue ) );
			sprintf( szValue, "%s", strValue );
			break;
		case O_STRING:
			pEdit = ( TextEntry * )pList->pControl;
			pEdit->GetText( strValue, sizeof( strValue ) );
			sprintf( szValue, "%s", strValue );
			break;
		case O_LIST:
			{
				pCombo = (ComboBox *)pList->pControl;
				// pCombo->GetText( strValue, sizeof( strValue ) );
				int activeItem = pCombo->GetActiveItem();

				pItem = pObj->pListItems;
				//			int n = (int)pObj->fdefValue;

				while ( pItem )
				{
					if (!activeItem--)
						break;

					pItem = pItem->pNext;
				}

				if ( pItem )
				{
					sprintf( szValue, "%s", pItem->szValue );
				}
				else  // Couln't find index
				{
					//assert(!("Couldn't find string in list, using default value"));
					sprintf( szValue, "%s", pObj->defValue );
				}
				break;
			}
		case O_SLIDER:
			pSlider = ( CCvarSlider * )pList->pControl;
			sprintf( szValue, "%.2f", pSlider->GetSliderValue() );
			break;
		}

		// Remove double quotes and % characters
		UTIL_StripInvalidCharacters( szValue, sizeof(szValue) );

		V_strcpy_safe( strValue, szValue );

		pObj->SetCurValue( strValue );

		pList = pList->next;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAdvancedOptionsDialog::CreateControls()
{
	DestroyControls();

	// Go through desciption creating controls
	CScriptObject *pObj;

	pObj = m_pDescription->pObjList;

	// Build out the clan dropdown
	CScriptObject *pClanObj = m_pDescription->FindObject( "cl_clanid" );
	ISteamFriends *pFriends = steamapicontext->SteamFriends();
	if ( pFriends && pClanObj )
	{
		pClanObj->RemoveAndDeleteAllItems();
		int iGroupCount = pFriends->GetClanCount();
		pClanObj->AddItem( new CScriptListItem( "#Cstrike_ClanTag_None", "0" ) );
		for ( int k = 0; k < iGroupCount; ++ k )
		{
			CSteamID clanID = pFriends->GetClanByIndex( k );
			const char *pName = pFriends->GetClanName( clanID );
			const char *pTag = pFriends->GetClanTag( clanID );

			char id[12];
			Q_snprintf( id, sizeof( id ), "%d", clanID.GetAccountID() );
			pClanObj->AddItem( new CScriptListItem( CFmtStr( "%s (%s)", pTag, pName ), id ) );
		}
	}

	mpcontrol_t	*pCtrl;

	Button *pButton;
	CheckButton *pBox;
	TextEntry *pEdit;
	ComboBox *pCombo;
	CCvarSlider *pSlider;
	CScriptListItem *pListItem;

	Panel *objParent = m_pListPanel;

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	vgui::HFont hTextFont = pScheme->GetFont( "HudFontSmallestBold", true );
	Color tanDark = pScheme->GetColor( "TanDark", Color(255,0,0,255) );

	while ( pObj )
	{
		if ( pObj->type == O_OBSOLETE )
		{
			pObj = pObj->pNext;
			continue;
		}

		pCtrl = new mpcontrol_t( objParent, "mpcontrol_t" );
		pCtrl->type = pObj->type;

		// Force it to invalidate scheme now, so we can change color afterwards and have it persist
		pCtrl->InvalidateLayout( true, true );

		switch ( pCtrl->type )
		{
		case O_BOOL:
			pBox = new CheckButton( pCtrl, "DescCheckButton", pObj->prompt );
			pBox->SetSelected( pObj->fdefValue != 0.0f ? true : false );

			pCtrl->pControl = (Panel *)pBox;
			pBox->SetFont( hTextFont );

			pBox->InvalidateLayout( true, true );

			pBox->SetFgColor( tanDark );
			pBox->SetDefaultColor( tanDark, pBox->GetBgColor() );
			pBox->SetArmedColor( tanDark, pBox->GetBgColor() );
			pBox->SetDepressedColor( tanDark, pBox->GetBgColor() );
			pBox->SetSelectedColor( tanDark, pBox->GetBgColor() );
			pBox->SetHighlightColor( tanDark );
			pBox->GetCheckImage()->SetColor( tanDark );
			break;
		case O_STRING:
		case O_NUMBER:
			pEdit = new TextEntry( pCtrl, "DescTextEntry");
			pEdit->InsertString(pObj->defValue);
			pCtrl->pControl = (Panel *)pEdit;
			pEdit->SetFont( hTextFont );

			pEdit->InvalidateLayout( true, true );
			pEdit->SetBgColor( Color(0,0,0,255) );
			break;
		case O_LIST:
			{
				pCombo = new ComboBox( pCtrl, "DescComboBox", 5, false );

				// track which row matches the current value
				int iRow = -1;
				int iCount = 0;
				pListItem = pObj->pListItems;
				while ( pListItem )
				{
					if ( iRow == -1 && !Q_stricmp( pListItem->szValue, pObj->curValue ) )
						iRow = iCount;

					pCombo->AddItem( pListItem->szItemText, NULL );
					pListItem = pListItem->pNext;
					++iCount;
				}


				pCombo->ActivateItemByRow( iRow );

				pCtrl->pControl = (Panel *)pCombo;
				pCombo->SetFont( hTextFont );
			}
			break;
		case O_SLIDER:
			pSlider = new CCvarSlider( pCtrl, "DescSlider", "Test", pObj->fMin, pObj->fMax, pObj->cvarname, false );
			pCtrl->pControl = (Panel *)pSlider;
			break;
		case O_BUTTON:
			pButton = new CExButton( pCtrl, "DescButton", pObj->prompt, this, pObj->defValue );
			pButton->SetFont( hTextFont );
			pCtrl->pControl = (Panel *)pButton;
			break;
		case O_CATEGORY:
			pCtrl->SetBorder( pScheme->GetBorder("OptionsCategoryBorder") );
			break;
		default:
			break;
		}

		if ( pCtrl->type != O_BOOL && pCtrl->type != O_BUTTON )
		{
			pCtrl->pPrompt = new vgui::Label( pCtrl, "DescLabel", "" );
			pCtrl->pPrompt->SetContentAlignment( vgui::Label::a_west );
			pCtrl->pPrompt->SetTextInset( 5, 0 );
			pCtrl->pPrompt->SetText( pObj->prompt );
			pCtrl->pPrompt->SetFont( hTextFont );

			pCtrl->pPrompt->InvalidateLayout( true, true );

			if ( pCtrl->type == O_CATEGORY )
			{
				pCtrl->pPrompt->SetFont( pScheme->GetFont( "HudFontSmallBold", true ) );
				pCtrl->pPrompt->SetFgColor( pScheme->GetColor( "TanLight", Color(255,0,0,255) ) );
			}
			else
			{
				pCtrl->pPrompt->SetFgColor( tanDark );
			}
		}

		pCtrl->pScrObj = pObj;

		switch ( pCtrl->type )
		{
		case O_BOOL:
		case O_STRING:
		case O_NUMBER:
		case O_LIST:
		case O_BUTTON:
		case O_CATEGORY:
			pCtrl->SetSize( m_iControlW, m_iControlH );
			break;
		case O_SLIDER:
			pCtrl->SetSize( m_iSliderW, m_iSliderH );
			break;
		default:
			break;
		}

		// Hook up the tooltip, if the entry has one
		if ( pObj->tooltip && pObj->tooltip[0] )
		{
			if ( pCtrl->pPrompt )
			{
				pCtrl->pPrompt->SetTooltip( m_pToolTip, pObj->tooltip );
			}
			else
			{
				pCtrl->SetTooltip( m_pToolTip, pObj->tooltip );
				pCtrl->pControl->SetTooltip( m_pToolTip, pObj->tooltip );
			}
		}

		m_pListPanel->AddItem( NULL, pCtrl );

		// Link it in
		if ( !m_pList )
		{
			m_pList = pCtrl;
			pCtrl->next = NULL;
		}
		else
		{
			mpcontrol_t *p;
			p = m_pList;
			while ( p )
			{
				if ( !p->next )
				{
					p->next = pCtrl;
					pCtrl->next = NULL;
					break;
				}
				p = p->next;
			}
		}

		pObj = pObj->pNext;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAdvancedOptionsDialog::DestroyControls()
{
	mpcontrol_t *p, *n;

	p = m_pList;
	while ( p )
	{
		n = p->next;
		//
		if ( p->pControl )
		{
			p->pControl->MarkForDeletion();
			p->pControl = NULL;
		}
		if ( p->pPrompt )
		{
			p->pPrompt->MarkForDeletion();
			p->pPrompt = NULL;
		}
		delete p;
		p = n;
	}

	m_pList = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAdvancedOptionsDialog::SaveValues() 
{
	// Get the values from the controls:
	GatherCurrentValues();

	// Create the game.cfg file
	if ( m_pDescription )
	{
		FileHandle_t fp;

		// Add settings to config.cfg
		m_pDescription->WriteToConfig();

		g_pFullFileSystem->CreateDirHierarchy( OPTIONS_DIR );
		fp = g_pFullFileSystem->Open( OPTIONS_FILE, "wb" );
		if ( fp )
		{
			m_pDescription->WriteToScriptFile( fp );
			g_pFullFileSystem->Close( fp );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAdvancedOptionsDialog::Deploy( void )
{
	SetVisible( true );
	MakePopup();
	MoveToFront();
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	TFModalStack()->PushModal( this );

	// Center it, keeping requested size
	int x, y, ww, wt, wide, tall;
	vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
	GetSize(wide, tall);
	SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTextToolTip::PerformLayout()
{
	if ( !ShouldLayout() )
		return;

	_isDirty = false;

	// Resize our text labels to fit.
	int iW = 0;
	int iH = 0;
	for (int i = 0; i < m_pEmbeddedPanel->GetChildCount(); i++)
	{
		vgui::Label *pLabel = dynamic_cast<vgui::Label*>( m_pEmbeddedPanel->GetChild(i) );
		if ( !pLabel )
			continue;

		// Only checking to see if we have any text
		char szTmp[2];
		pLabel->GetText( szTmp, sizeof(szTmp) );
		if ( !szTmp[0] )
			continue;

		int iLX, iLY;
		pLabel->GetPos( iLX, iLY );

		int nMaxWide = m_nMaxWide > 0 ? m_nMaxWide : m_pEmbeddedPanel->GetWide() - (iLX * 2);
		pLabel->GetTextImage()->ResizeImageToContentMaxWidth( nMaxWide  );
		pLabel->SizeToContents();
		int nNewWide = Min( nMaxWide, pLabel->GetWide() + (iLX * 2) );
		pLabel->SetWide( nNewWide );
		pLabel->InvalidateLayout(true);

		int iX, iY;
		pLabel->GetPos( iX, iY );
		iW = MAX( iW, ( pLabel->GetWide() + (iX * 2) ) );

		if ( iH == 0 )
		{
			iH += MAX( iH, pLabel->GetTall() + (iY * 2) );
		}
		else
		{
			iH += MAX( iH, pLabel->GetTall() );
		}
	}
	m_pEmbeddedPanel->SetSize( iW, iH );

	m_pEmbeddedPanel->SetVisible(true);

	PositionWindow( m_pEmbeddedPanel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTextToolTip::PositionWindow( Panel *pTipPanel )
{
	int iTipW, iTipH;
	pTipPanel->GetSize( iTipW, iTipH );

	int cursorX, cursorY;
	input()->GetCursorPos(cursorX, cursorY);

	int px, py, wide, tall;
	ipanel()->GetAbsPos( m_pEmbeddedPanel->GetParent()->GetVPanel(), px, py );
	m_pEmbeddedPanel->GetParent()->GetSize(wide, tall);

	if ( !m_pEmbeddedPanel->IsPopup() )
	{
		// Move the cursor into our parent space
		cursorX -= px;
		cursorY -= py;
	}

	if (wide - iTipW > cursorX)
	{
		cursorY += 20;
		// menu hanging right
		if (tall - iTipH > cursorY)
		{
			// menu hanging down
			pTipPanel->SetPos(cursorX, cursorY);
		}
		else
		{
			// menu hanging up
			pTipPanel->SetPos(cursorX, cursorY - iTipH - 20);
		}
	}
	else
	{
		// menu hanging left
		if (tall - iTipH > cursorY)
		{
			// menu hanging down
			pTipPanel->SetPos( Max( 0, cursorX - iTipW ), cursorY);
		}
		else
		{
			// menu hanging up
			pTipPanel->SetPos( Max( 0, cursorX - iTipW ), cursorY - iTipH - 20 );
		}
	}	
}

void CTFTextToolTip::ShowTooltip( Panel *pCurrentPanel )
{
	EditablePanel* pEditableCurrentPanel = dynamic_cast< EditablePanel* >( pCurrentPanel );
	if ( pEditableCurrentPanel )
	{
		KeyValues* pKVVariables = pEditableCurrentPanel->GetDialogVariables();
		const wchar_t *pwszTipText = pKVVariables->GetWString( "tiptext", NULL );
		if ( pwszTipText && *pwszTipText )
		{
			m_pEmbeddedPanel->SetDialogVariable( "tiptext", pwszTipText );
		}
	}

	BaseClass::ShowTooltip( pCurrentPanel );
}

static vgui::DHANDLE<CTFAdvancedOptionsDialog> g_pTFAdvancedOptionsDialog;

//-----------------------------------------------------------------------------
// Purpose: Callback to open the game menus
//-----------------------------------------------------------------------------
void CL_OpenTFAdvancedOptionsDialog( const CCommand &args )
{
	if ( g_pTFAdvancedOptionsDialog.Get() == NULL )
	{
		g_pTFAdvancedOptionsDialog = vgui::SETUP_PANEL( new CTFAdvancedOptionsDialog( NULL ) );
	}

	g_pTFAdvancedOptionsDialog->Deploy();
}

// the console commands
static ConCommand opentf2options( "opentf2options", &CL_OpenTFAdvancedOptionsDialog, "Displays the TF2 Advanced Options dialog." );

//-----------------------------------------------------------------------------
// Purpose: A scroll bar that can have specified width
//-----------------------------------------------------------------------------
class CExScrollBar : public ScrollBar
{
	DECLARE_CLASS_SIMPLE( CExScrollBar, ScrollBar );
public:

	CExScrollBar( Panel *parent, const char *name, bool bVertical )
		: ScrollBar( parent, name, bVertical )
	{}
	
	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE
	{
		// Deliberately skip ScrollBar
		Panel::ApplySchemeSettings( pScheme );
	}
};

DECLARE_BUILD_FACTORY( CExScrollingEditablePanel );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CExScrollingEditablePanel::CExScrollingEditablePanel( Panel *pParent, const char *pszName )
	: EditablePanel( pParent, pszName )
	, m_nLastScrollValue( 0 )
	, m_bUseMouseWheelToScroll( true )
{
	m_pScrollBar = new CExScrollBar( this, "ScrollBar", true );
	m_pScrollBar->SetAutoResize( PIN_TOPRIGHT, Panel::AUTORESIZE_DOWN, 0, 0, 0, 0 );
	m_pScrollBar->AddActionSignalTarget( this );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CExScrollingEditablePanel::~CExScrollingEditablePanel()
{}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CExScrollingEditablePanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pScrollbarKV = inResourceData->FindKey( "Scrollbar" );
	if ( pScrollbarKV )
	{
		m_pScrollBar->ApplySettings( pScrollbarKV );
	}

	m_bUseMouseWheelToScroll = inResourceData->GetBool( "allow_mouse_wheel_to_scroll", true );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CExScrollingEditablePanel::OnSizeChanged( int newWide, int newTall )
{
	BaseClass::OnSizeChanged( newWide, newTall );

	int nDelta = m_nLastScrollValue;
	// Go through all our children and move them BACK into position
	int nNumChildren = GetChildCount();
	for ( int i=0; i < nNumChildren; ++i )
	{
		Panel* pChild = GetChild( i );

		if ( pChild == m_pScrollBar )
			continue;

		EditablePanel* pEditableChild = dynamic_cast< EditablePanel* >( pChild );
		if ( pEditableChild && pEditableChild->ShouldSkipAutoResize() )
			continue;

		int x,y;
		pChild->GetPos( x, y );

		pChild->SetPos( x, y - nDelta );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CExScrollingEditablePanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int nFurthestY = 0;

	// Go through all our children and find the lowest point on the lowest child
	// that we'd need to scroll to
	int nNumChildren = GetChildCount();
	for ( int i=0; i < nNumChildren; ++i )
	{
		Panel* pChild = GetChild( i );
	
		if ( pChild == m_pScrollBar )
			continue;

		// Skip panels that are pinned.  They'll move when their pin sibling moves
		if ( pChild->GetPinSibling().Get() != NULL )
			continue;

		int x,y,wide,tall;
		pChild->GetBounds( x, y, wide, tall );

		// Offset by our scroll value
		y += m_nLastScrollValue;

		if ( m_bRestrictWidth )
		{
			int nMaxWide = Min( x + wide, GetWide() - m_pScrollBar->GetWide() );
			pChild->SetWide( nMaxWide - x );
		}

		nFurthestY = Max( y + tall, nFurthestY );
	}

	int nMaxRange = nFurthestY + m_iBottomBuffer;
	m_pScrollBar->SetRange( 0, nMaxRange );
	m_pScrollBar->SetRangeWindow( GetTall() );

	OnScrollBarSliderMoved();
}

//-----------------------------------------------------------------------------
// Called when the scroll bar moves
//-----------------------------------------------------------------------------
void CExScrollingEditablePanel::OnScrollBarSliderMoved()
{
	// Figure out how far they just scrolled
	int nScrollAmount = m_pScrollBar->GetValue();
	int nDelta = nScrollAmount - m_nLastScrollValue;

	if ( nDelta == 0 )
		return;

	ShiftChildren( nDelta );

	m_nLastScrollValue = nScrollAmount;
}

void CExScrollingEditablePanel::ShiftChildren( int nDistance )
{
	// Go through all our children and move them 
	int nNumChildren = GetChildCount();
	for ( int i=0; i < nNumChildren; ++i )
	{
		Panel* pChild = GetChild( i );

		if ( pChild == m_pScrollBar )
			continue;

		int x,y;
		pChild->GetPos( x, y );

		pChild->SetPos( x, y - nDistance );
	}
}

//-----------------------------------------------------------------------------
// respond to mouse wheel events
//-----------------------------------------------------------------------------
void CExScrollingEditablePanel::OnMouseWheeled( int delta )
{
	if ( !m_bUseMouseWheelToScroll )
	{
		BaseClass::OnMouseWheeled( delta );
		return;
	}

	int val = m_pScrollBar->GetValue();
	val -= ( delta * m_iScrollStep );
	m_pScrollBar->SetValue( val );
}

DECLARE_BUILD_FACTORY( CScrollableList );
//-----------------------------------------------------------------------------
// Clearnup
//-----------------------------------------------------------------------------
CScrollableList::~CScrollableList()
{
	ClearAutoLayoutPanels();
}

void CScrollableList::PerformLayout()
{
	int nYpos = -m_nLastScrollValue;

	for( int i=0; i<m_vecAutoLayoutPanels.Count(); ++i )
	{
		LayoutInfo_t layout = m_vecAutoLayoutPanels[ i ];
		nYpos += layout.m_nGap;

		layout.m_pPanel->SetPos( layout.m_pPanel->GetXPos(), nYpos );

		nYpos += layout.m_pPanel->GetTall();
	}

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Add a panel to the bottom
//-----------------------------------------------------------------------------
void CScrollableList::AddPanel( Panel* pPanel, int nGap )
{
	// We're the captain now
	pPanel->SetParent( this );
	pPanel->SetAutoDelete( false );

	auto idx = m_vecAutoLayoutPanels.AddToTail();
	LayoutInfo_t& layout = m_vecAutoLayoutPanels[ idx ];
	layout.m_pPanel = pPanel;
	layout.m_nGap = nGap;

	// Need to do a perform layout so we get sized correctly
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Delete any panels we own
//-----------------------------------------------------------------------------
void CScrollableList::ClearAutoLayoutPanels()
{
	FOR_EACH_VEC( m_vecAutoLayoutPanels, i )
	{
		m_vecAutoLayoutPanels[ i ].m_pPanel->MarkForDeletion();
	}

	m_vecAutoLayoutPanels.Purge();
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
CExpandablePanel::CExpandablePanel( Panel* pParent, const char* pszName ) 
	: vgui::EditablePanel( pParent, pszName )
	, m_bExpanded( false )
	, m_flAnimEndTime( 0.f )
	, m_flResizeTime( 0.f )
{}


//-----------------------------------------------------------------------------
// Set spcific collapsed state
//-----------------------------------------------------------------------------
void CExpandablePanel::SetCollapsed( bool bCollapsed, bool bInstant /*= false*/ )
{
	if ( bCollapsed == m_bExpanded )
	{
		ToggleCollapse();

		if ( bInstant )
		{
			m_flAnimEndTime = Plat_FloatTime();
		}
	}
}


//-----------------------------------------------------------------------------
// Toggle collapsed state
//-----------------------------------------------------------------------------
void CExpandablePanel::ToggleCollapse()
{
	m_bExpanded = !m_bExpanded;
	// Allow for quick bounce-back if they click while we're already animating
	float flEndTime = RemapValClamped( GetPercentAnimated(), 0.f, 1.f, 0.f, m_flResizeTime );
	m_flAnimEndTime = Plat_FloatTime() + flEndTime;

	OnToggleCollapse( m_bExpanded );
}

//-----------------------------------------------------------------------------
// Read in collapse direction
//-----------------------------------------------------------------------------
void CExpandablePanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	CUtlString strExpandDir( inResourceData->GetString( "expand_direction", "down" ) );
	if ( FStrEq( strExpandDir.Get(), "up" ) )
		m_eExpandDir = EXPAND_UP;
	if ( FStrEq( strExpandDir.Get(), "down" ) )
		m_eExpandDir = EXPAND_DOWN;
	if ( FStrEq( strExpandDir.Get(), "left" ) )
		m_eExpandDir = EXPAND_LEFT;
	if ( FStrEq( strExpandDir.Get(), "right" ) )
		m_eExpandDir = EXPAND_RIGHT;

	// This defaults to true.  REALLY this panel should be firing an action signal
	// when it resizes
	bInvalidateParentOnResize = inResourceData->GetBool( "invalidate_parent_on_resize", true );
}

//-----------------------------------------------------------------------------
// Toggle collapsed state
//-----------------------------------------------------------------------------
void CExpandablePanel::OnCommand( const char *command )
{
	if ( FStrEq( "toggle_collapse", command ) )
	{
		ToggleCollapse();

		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Do collapsing interpolation
//-----------------------------------------------------------------------------
void CExpandablePanel::OnThink()
{
	BaseClass::OnThink();

	float flTimeProgress = Gain( GetPercentAnimated(), 0.8f );
		
	const int& nStartHeight = m_bExpanded ? m_nCollapsedHeight : m_nExpandedHeight;
	const int& nEndHeight = m_bExpanded ? m_nExpandedHeight : m_nCollapsedHeight;
	int nCurrentHeight = RemapValClamped( flTimeProgress, 0.f, 1.f, nStartHeight, nEndHeight );

	if ( nCurrentHeight != GetDimension() )
	{
		SetDimension( nCurrentHeight );
		if ( bInvalidateParentOnResize )
		{
			Panel* pParent = GetParent();
			if ( pParent )
			{
				pParent->InvalidateLayout();
			}
		}
	}
}

int CExpandablePanel::GetDimension()
{
	switch( m_eExpandDir )
	{
	default:
	case EXPAND_UP:
	case EXPAND_DOWN:
		return GetTall();

	case EXPAND_LEFT:
	case EXPAND_RIGHT:
		return GetWide();
	}
}

void CExpandablePanel::SetDimension( int nNewValue )
{
	int nX, nY, nWide, nTall;
	GetBounds( nX, nY, nWide, nTall );

	switch( m_eExpandDir )
	{
		default:
		case EXPAND_UP:
			SetPos( nX, nY - ( nNewValue - nTall ) );
			// fall through
		case EXPAND_DOWN:
			SetTall( nNewValue );
			break;

		case EXPAND_LEFT:
			SetPos( nX - ( nNewValue - nWide ), nY );
			// fall through
		case EXPAND_RIGHT:
			SetWide( nNewValue );
	}
}

//-----------------------------------------------------------------------------
// Where we're at in our interpolation
//-----------------------------------------------------------------------------
float CExpandablePanel::GetPercentAnimated() const
{
	return RemapValClamped( Plat_FloatTime() - ( m_flAnimEndTime - m_flResizeTime ), 0.f, m_flResizeTime, 0.f, 1.f );
}

float CExpandablePanel::GetPercentExpanded() const
{
	return RemapValClamped( (float)const_cast< CExpandablePanel* >(this)->GetTall(), (float)m_nCollapsedHeight, (float)m_nExpandedHeight, 0.f, 1.f );
}

DECLARE_BUILD_FACTORY( CDraggableScrollingPanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDraggableScrollingPanel::CDraggableScrollingPanel( Panel *pParent, const char *pszPanelname )
	: EditablePanel( pParent, pszPanelname )
	, m_iDragStartX( 0 )
	, m_iDragStartY( 0 )
	, m_iDragTotalDistance( 0 )
	, m_bDragging( false )
	, m_flZoom( 1.f )
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	FOR_EACH_VEC_BACK( m_vecChildOriginalData, i )
	{
		bool bFound = false;
		for( int j=0; j < GetChildCount(); ++j )
		{
			if ( m_vecChildOriginalData[ i ].m_pChild == GetChild( j ) )
			{
				bFound = true;
				break;
			}
		}

		if ( !bFound )
		{
			m_vecChildOriginalData.Remove( i );
		}
	}

	m_flMinZoom				= inResourceData->GetFloat( "min_zoom", 1.f );
	m_flMaxZoom				= inResourceData->GetFloat( "max_zoom", 2.f );
	m_flZoom				= inResourceData->GetFloat( "zoom", 1.f );
	m_flMouseWheelZoomRate	= inResourceData->GetFloat( "mouse_wheel_zoom_rate", 0.05f );

	m_iOriginalWide = GetWide();
	m_iOriginalTall = GetTall();

	SetZoomAmount( m_flZoom, GetWide() / 2, GetTall() /  2 );

	KeyValues* pKVPendingChildren = inResourceData->FindKey( "pending_children" );
	if ( pKVPendingChildren )
	{
		FOR_EACH_TRUE_SUBKEY( pKVPendingChildren, pKVChild )
		{
			auto& child = m_vecPendingChildren[ m_vecPendingChildren.AddToTail() ];
			child.m_strName			= pKVChild->GetString( "child_name" );
			child.m_ePinPosition	= (EPinPosition)pKVChild->GetInt( "pin", PIN_CENTER );
			child.m_bScaleWithZoom	= pKVChild->GetBool( "scale", true );
			child.m_bMoveWithDrag	= pKVChild->GetBool( "move", true );
		}

		if ( !m_vecPendingChildren.IsEmpty() )
		{
			vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: When a child is removed, remove the original data for that child
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnChildRemoved( Panel* pChild )
{
	BaseClass::OnChildRemoved( pChild );

	auto idx = m_vecChildOriginalData.FindPredicate( [ & ]( const ChildPositionInfo_t& other )
	{
		return other.m_pChild == pChild;
	} );

	m_vecChildOriginalData.Remove( idx );
}

//-----------------------------------------------------------------------------
// Purpose: Check if our pending children are loaded yer
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnTick()
{
	BaseClass::OnTick();

	if ( BCheckForPendingChildren() )
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Try to setup our pending children
//-----------------------------------------------------------------------------
bool CDraggableScrollingPanel::BCheckForPendingChildren()
{
	FOR_EACH_VEC_BACK( m_vecPendingChildren, i )
	{
		auto& child = m_vecPendingChildren[ i ];
		Panel* pChildPanel = FindChildByName( child.m_strName );
		if ( pChildPanel )
		{
			AddOrUpdateChild( pChildPanel, child.m_bScaleWithZoom, child.m_bMoveWithDrag, child.m_ePinPosition );
			m_vecPendingChildren.Remove( i );
		}
	}

	return m_vecPendingChildren.IsEmpty();
}

//-----------------------------------------------------------------------------
// Purpose: Remember where we started pressing
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnMousePressed( MouseCode code )
{
	input()->SetMouseCapture(GetVPanel());

	m_bDragging = true;
	m_iDragTotalDistance = 0;
	input()->GetCursorPosition( m_iDragStartX, m_iDragStartY );

	PostActionSignal( new KeyValues("DragStart") );
}

//-----------------------------------------------------------------------------
// Purpose: Done dragging
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnMouseReleased( MouseCode code )
{
	input()->SetMouseCapture( NULL );
	m_bDragging = false;
	PostActionSignal( new KeyValues( "DragStop", "dist", m_iDragTotalDistance ) );
}

//-----------------------------------------------------------------------------
// Purpose: Move the panel corresponding to mouse deltas
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::InternalCursorMoved( int x, int y )
{
	if ( !m_bDragging )
		return;

	// How far to go
	int nDeltaX = x - m_iDragStartX;
	int nDeltaY = y - m_iDragStartY;

	m_iDragTotalDistance += abs( nDeltaX );
	m_iDragTotalDistance += abs( nDeltaY );

	// Store where the mouse is now, so we can get the next delta
	m_iDragStartX = x;
	m_iDragStartY = y;

	int nNewX, nNewY;
	GetPos( nNewX, nNewY );

	// Move us, but keep us within parents bounds
	nNewX = clamp( nDeltaX + nNewX, -( GetWide() - GetParent()->GetWide() ), 0 );
	nNewY = clamp( nDeltaY + nNewY, -( GetTall() - GetParent()->GetTall() ), 0 );

	SetPos( nNewX, nNewY );

	// We moved.  Move children
	UpdateChildren();
}

//-----------------------------------------------------------------------------
// Purpose: Zoom based on how much the wheel was wheeled
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnMouseWheeled( int delta )
{
	int nXP, pYP;
	ipanel()->GetAbsPos(GetVParent(), nXP, pYP);

	int nXM, nYM;
	input()->GetCursorPosition( nXM, nYM );

	SetZoomAmount( m_flZoom + delta * m_flMouseWheelZoomRate, nXM - nXP, nYM - pYP );

	BaseClass::OnMouseWheeled(delta);
}

//-----------------------------------------------------------------------------
// Purpose: External slider movement
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnSliderMoved( KeyValues *pParams )
{
	Slider *pSlider = reinterpret_cast<Slider*>( const_cast<KeyValues*>(pParams)->GetPtr("panel") );
	if ( pSlider )
	{
		int nMin, nMax;
		pSlider->GetRange( nMin, nMax );
		SetZoomAmount( RemapVal( (float)pSlider->GetValue(), (float)nMin, (float)nMax, m_flMinZoom, m_flMaxZoom )
					 , GetParent()->GetWide() / 2.f
					 , GetParent()->GetTall() / 2.f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Zoom!  Make our children scale up and move into position.  We use
//			a zoom focus point that we zoom into so if the user uses the mouse
//			wheel to zoom in, the point under the cursor will be maintained
//			as they zoom.
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::SetZoomAmount( float flZoomAmount, int nXZoomFocus, int nYZoomFocus )
{
	float flNewZoom = clamp( flZoomAmount, m_flMinZoom, m_flMaxZoom );
	m_flZoom = flNewZoom;

	// Tell everyone we zoomed
	KeyValues *pParams = new KeyValues( "ZoomChanged" );
	pParams->SetFloat( "zoom", RemapVal( m_flZoom, m_flMinZoom, m_flMaxZoom, 0.f, 1.f ) );
	PostActionSignal( pParams );

	int nXPos, nYPos;
	GetPos( nXPos, nYPos );

	// Maintain focal point as we zoom in/out
	float flXZoomFocalPoint = float( nXZoomFocus - nXPos ) / GetWide();
	float flYZoomFocalPoint = float( nYZoomFocus - nYPos ) / GetTall();

	// Resize ourselves
	SetWide( m_flZoom * m_iOriginalWide );
	SetTall( m_flZoom * m_iOriginalTall );

	nXPos = ( flXZoomFocalPoint * GetWide() ) - nXZoomFocus;
	nYPos = ( flYZoomFocalPoint * GetTall() ) - nYZoomFocus;

	// Make sure we stay within bounds
	nXPos = clamp( -nXPos, -( GetWide() - GetParent()->GetWide() ), 0 );
	nYPos = clamp( -nYPos, -( GetTall() - GetParent()->GetTall() ), 0 );

	SetPos( nXPos, nYPos );

	// Update children since we scaled and possibly moved
	UpdateChildren();
}

const CDraggableScrollingPanel::ChildPositionInfo_t* CDraggableScrollingPanel::GetChildPositionInfo( const Panel* pChildPanel ) const
{
	auto idx =m_vecChildOriginalData.FindPredicate( [ & ]( const ChildPositionInfo_t& other )
	{
		return other.m_pChild == pChildPanel;
	} );

	if ( idx == m_vecChildOriginalData.InvalidIndex() )
		return NULL;

	return &m_vecChildOriginalData[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: Store the original size and position of children so we know how to
//			scale them up and down
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::AddOrUpdateChild( Panel* pChild, bool bScaleWithZoom, bool bMoveWithDrag, EPinPosition ePinPosition )
{
	// Check if we already have this panel and dont do anything if os
	auto idx = m_vecChildOriginalData.InvalidIndex();
	FOR_EACH_VEC( m_vecChildOriginalData, i )
	{
		if ( m_vecChildOriginalData[ i ].m_pChild == pChild )
		{
			idx = i;
			break;
		}
	}

	if ( idx == m_vecChildOriginalData.InvalidIndex() )
	{
		idx = m_vecChildOriginalData.AddToTail();
	}

	// Setup initial data
	ChildPositionInfo_t& info = m_vecChildOriginalData[ idx ];
	info.m_pChild = pChild;
	info.m_bScaleWithZoom = bScaleWithZoom;
	info.m_bMoveWithDrag = bMoveWithDrag;
	info.m_ePinPosition = ePinPosition;

	// Capture their settings now or do we have to wait?
	if ( pChild->IsLayoutInvalid() )
	{
		info.m_bWaitingForSettings = true;
	}
	else
	{
		CaptureChildSettings( pChild );
		UpdateChildren();
	}
}

//-----------------------------------------------------------------------------
// Purpose: See if we were waiting for a child to get their settings applied
//			before we added them as a managed child
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnChildSettingsApplied( KeyValues *pInResourceData, Panel *pChild )
{
	FOR_EACH_VEC( m_vecChildOriginalData, i )
	{
		ChildPositionInfo_t& info = m_vecChildOriginalData[ i ];
		if ( info.m_pChild == pChild && info.m_bWaitingForSettings )
		{
			info.m_bWaitingForSettings = false;
			CaptureChildSettings( pChild );
			return;
		}
	}
}

void CDraggableScrollingPanel::CaptureChildSettings( Panel* pChild )
{
	float flStartWide = GetWide() / m_flZoom;
	float flStartTall = GetTall() / m_flZoom;

	FOR_EACH_VEC( m_vecChildOriginalData, i )
	{
		if ( m_vecChildOriginalData[ i ].m_pChild == pChild )
		{
			ChildPositionInfo_t& info = m_vecChildOriginalData[ i ];
			switch ( info.m_ePinPosition )
			{
				case PIN_CENTER:
				{
					info.m_flX = ( pChild->GetXPos() + ( pChild->GetWide() / 2.f ) ) / flStartWide;
					info.m_flY = ( pChild->GetYPos() + ( pChild->GetTall() / 2.f ) ) / flStartTall;
				}
				break;

				case PIN_TOP_LEFT:
				{
					info.m_flX = pChild->GetXPos() / flStartWide;
					info.m_flY = pChild->GetYPos() / flStartTall;
				}
				break;

				case PIN_TOP_RIGHT:
				{
					info.m_flX = ( pChild->GetXPos() + pChild->GetWide() ) / flStartWide;
					info.m_flY =   pChild->GetYPos() / flStartTall;
				}
				break;

				case PIN_BOTTOM_LEFT:
				{
					info.m_flX =   pChild->GetXPos() / flStartWide;
					info.m_flY = ( pChild->GetYPos() + pChild->GetTall() ) / flStartTall;
				}
				break;

				case PIN_BOTTOM_RIGHT:
				{
					info.m_flX = ( pChild->GetXPos() + pChild->GetWide() ) / flStartWide;
					info.m_flY = ( pChild->GetYPos() + pChild->GetTall() ) / flStartTall;
				}
				break;
			}
			
			info.m_flWide = pChild->GetWide() / flStartWide;
			info.m_flTall = pChild->GetTall() / flStartTall;
			return;
		}
	}

	// Should've been captured above
	Assert( false );
}

//-----------------------------------------------------------------------------
// Purpose: Resize and reposition children based on zoom and drag offset
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::UpdateChildren()
{
	int nXPos, nYPos;
	GetPos( nXPos, nYPos );

	FOR_EACH_VEC( m_vecChildOriginalData, i )
	{
		ChildPositionInfo_t& child = m_vecChildOriginalData[ i ];

		if ( child.m_bWaitingForSettings )
		{
			CaptureChildSettings( child.m_pChild );
			child.m_bWaitingForSettings = false;
		}

		if ( child.m_bScaleWithZoom )
		{
			child.m_pChild->SetWide( child.m_flWide * GetWide() );
			child.m_pChild->SetTall( child.m_flTall * GetTall() );
		}

		if ( child.m_bMoveWithDrag )
		{
			switch ( child.m_ePinPosition )
			{
				case PIN_CENTER:
				{
					child.m_pChild->SetPos( ( child.m_flX * GetWide() ) - ( child.m_pChild->GetWide() / 2.f )
										  , ( child.m_flY * GetTall() ) - ( child.m_pChild->GetTall() / 2.f ) );
				}
				break;

				case PIN_TOP_LEFT:
				{
					child.m_pChild->SetPos( child.m_flX * GetWide()
										  , child.m_flY * GetTall() );
				}
				break;

				case PIN_TOP_RIGHT:
				{
					child.m_pChild->SetPos( ( child.m_flX * GetWide() ) - child.m_pChild->GetWide()
										  , ( child.m_flY * GetTall() ) );
				}
				break;

				case PIN_BOTTOM_LEFT:
				{
					child.m_pChild->SetPos( ( child.m_flX * GetWide() )
										  , ( child.m_flY * GetTall() ) - child.m_pChild->GetTall() );
				}
				break;

				case PIN_BOTTOM_RIGHT:
				{
					child.m_pChild->SetPos( ( child.m_flX * GetWide() ) - ( child.m_pChild->GetWide() )
										  , ( child.m_flY * GetTall() ) - ( child.m_pChild->GetTall() ) );
				}
				break;
			}

			
		}
	}
}

DECLARE_BUILD_FACTORY( CTFLogoPanel );
CTFLogoPanel::CTFLogoPanel( Panel *pParent, const char *pszPanelname )
	: BaseClass( pParent, pszPanelname )
{}

void CTFLogoPanel::PaintTFLogo( float flAngle, const Color& color ) const
{
	const float flTotalRadius = YRES( m_flRadius );
	// I did the math
	const float flOuterToInnerRatio = 0.35f;
	const float flInnerRadius = flTotalRadius * flOuterToInnerRatio;
	constexpr const float flNaturalTiltAngle = 6.7f;
	constexpr const float fl90 = DEG2RAD( 90 );

	// Vgui....
	const float flCenterX = const_cast< CTFLogoPanel* >( this )->GetWide() / 2.f;
	const float flCenterY = const_cast< CTFLogoPanel* >( this )->GetTall() / 2.f;


	auto lambdaDrawSegment = [&]( float flStartAngle, float flEndAngle )
	{
		// Rotate it around the circle
		float flX = flCenterX ;
		float flY = flCenterY ;

		float flMagicOuter = 6.f;
		float flMagicInner = flMagicOuter * ( 1.f / flOuterToInnerRatio );

		DrawFilledColoredCircleSegment( flX,
										flY,
										flTotalRadius,
										flInnerRadius,
										color,
										flStartAngle	+ flMagicOuter,
										flEndAngle		- flMagicOuter,
										flStartAngle	+ flMagicInner,
										flEndAngle		- flMagicInner,
										true );
	};

	

	lambdaDrawSegment( flNaturalTiltAngle + flAngle,
					   flNaturalTiltAngle + flAngle + 90 );

	lambdaDrawSegment( flNaturalTiltAngle + flAngle + 90,
					   flNaturalTiltAngle + flAngle + 180  );

	lambdaDrawSegment( flNaturalTiltAngle + flAngle + 180,
					   flNaturalTiltAngle + flAngle + 270 );

	lambdaDrawSegment( flNaturalTiltAngle + flAngle + 270,
					   flNaturalTiltAngle + flAngle + 360 );

}

void CTFLogoPanel::Paint()
{
	m_flOffsetAngle += gpGlobals->frametime * m_flVelocity;
	PaintTFLogo( m_flOffsetAngle, GetFgColor() );
	BaseClass::Paint();
}

#include "tf_matchmaking_dashboard_parent_manager.h"

class CScrollingIndicatorPanel : public EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CScrollingIndicatorPanel , EditablePanel );
	CScrollingIndicatorPanel( const wchar* pwszText,
							  const char* pszSoundName,
							  float flDelay,
							  int nXTravel,
							  int nYTravel,
							  bool bPositive )
		: BaseClass( NULL, "Indicator" )
		, m_strSound( pszSoundName )
		, m_nXTravel( nXTravel )
		, m_nYTravel( nYTravel )
		, m_bPositive( bPositive )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
		SetProportional( true );
		SetScheme(scheme);

		LoadControlSettings( "resource/ui/XPSourcePanel.res" );
		GetMMDashboardParentManager()->AddPanel( this );
		SetMouseInputEnabled( false );

		PostMessage( GetVPanel(), new KeyValues( "Start" ), flDelay );
		PostMessage( GetVPanel(), new KeyValues( "End" ), flDelay + 3.5f );

		memset( m_wszBuff, 0, sizeof( m_wszBuff ) );

		if( pwszText )
		{
			SetText( pwszText );
		}

		SetAutoDelete( false );
	}

	virtual ~CScrollingIndicatorPanel()
	{
		GetMMDashboardParentManager()->RemovePanel( this );
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE
	{
		BaseClass::ApplySchemeSettings( pScheme );

		
	}

	virtual void PerformLayout() OVERRIDE
	{
		BaseClass::PerformLayout();

		SetDialogVariable( "source", m_wszBuff );
	}

	MESSAGE_FUNC( Start, "Start" )
	{
		SetVisible( true );

		// Do starting stuff
		if ( g_pClientMode && g_pClientMode->GetViewport() && g_pClientMode->GetViewportAnimationController() )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, m_bPositive ? "XPSourceShow_Positive" : "XPSourceShow_Negative", false );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "xpos", GetXPos() + m_nXTravel, 0.f, 3.f, AnimationController::INTERPOLATOR_DEACCEL, 0, true, false );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "ypos", GetYPos() + m_nYTravel, 0.f, 3.f, AnimationController::INTERPOLATOR_DEACCEL, 0, true, false );
		}

		if ( !m_strSound.IsEmpty() )
		{
			PlaySoundEntry( m_strSound );
		}
	}

	MESSAGE_FUNC( End, "End" )
	{
		// We're done!  Delete ourselves
		MarkForDeletion();
	}

	void SetText( const wchar* pwszText )
	{
		_snwprintf( m_wszBuff, ARRAYSIZE( m_wszBuff ), L"%ls", pwszText );
		InvalidateLayout();
	}

private:

	wchar m_wszBuff[ 256 ];
	CUtlString m_strSound;
	bool m_bPositive;
	int m_nXTravel = 0;
	int m_nYTravel = 0;
};

void CreateScrollingIndicator( int nXPos,
							   int nYPos,
							   const wchar* pwszText,
							   const char* pszSoundName,
							   float flDelay,
							   int nXTravel,
							   int nYTravel, 
							   bool bPositive )
{
	CScrollingIndicatorPanel* pPanel = new CScrollingIndicatorPanel( pwszText,
																	 pszSoundName,
																	 flDelay,
																	 nXTravel,
																	 nYTravel, 
																	 bPositive );
	pPanel->MakeReadyForUse();
	pPanel->SetPos( nXPos - pPanel->GetWide() / 2, nYPos );
}

//-----------------------------------------------------------------------------
// Purpose: A label that can have multiple fonts specified and will try to use
//			them in order specified, using the first one that fits.
//-----------------------------------------------------------------------------
class CAutoFittingLabel : public Label
{
	DECLARE_CLASS_SIMPLE( CAutoFittingLabel, Label );
public:

	CAutoFittingLabel( Panel *parent, const char *name )
		: Label( parent, name, (const char*)NULL )
		, m_mapColors( DefLessFunc( int ) )
	{}

	virtual void ApplySettings( KeyValues *inResourceData )
	{
		BaseClass::ApplySettings( inResourceData );
		vgui::IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

		m_vecFonts.Purge();
		KeyValues *pFonts = inResourceData->FindKey( "fonts" );
		if ( pFonts )
		{
			//
			// Get all the fonts
			//
			
			// Old style
			FOR_EACH_TRUE_SUBKEY( pFonts, pFont )
			{
				const HFont& font = pScheme->GetFont( pFont->GetString( "font" ), true );
				m_vecFonts.AddToTail( font );			
			}

			// New style
			FOR_EACH_VALUE( pFonts, pValue )
			{
				const HFont& font = pScheme->GetFont( pValue->GetString(), true );
				m_vecFonts.AddToTail( font );			
			}
		}
		else
		{
			m_vecFonts.AddToTail( GetFont() );
		}

		m_mapColors.Purge();
		KeyValues* pKVColors = inResourceData->FindKey( "Colors" );
		if ( pKVColors )
		{
			FOR_EACH_VALUE( pKVColors, pKVColor )
			{
				m_mapColors.Insert( atoi( pKVColor->GetName() ) ,GetColor( pKVColor->GetString() ) );
			}
		}
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		SetFont( m_vecFonts.Head() );

		// Go through all the fonts and try to find one that fits
		int nIndex = 0;
		GetTextImage()->ResizeImageToContentMaxWidth( GetWide() );
		while ( ( GetTextImage()->IsWrapping() || GetTextImage()->GetEllipsesPosition() ) && nIndex < m_vecFonts.Count() )
		{
			SetFont( m_vecFonts[ nIndex ] );
			GetTextImage()->ResizeImageToContentMaxWidth( GetWide() );

			++nIndex;
		}

		// Go through each character in the buffer and look for color change codes.
		// When a code is found, add a color change into the text image, where we 
		// use the color code as an index into the map.
		const wchar_t* pwszText = GetTextImage()->GetUText();
		int nTextIndex = 0;
		GetTextImage()->ClearColorChangeStream();
		while( pwszText && pwszText[0] )
		{
			auto idx = m_mapColors.Find( pwszText[0] );
			if ( idx != m_mapColors.InvalidIndex() )
			{
				GetTextImage()->AddColorChange( m_mapColors[ idx ], nTextIndex );
			}

			++nTextIndex;
			++pwszText;
		}
	}

private:

	CUtlVector< HFont > m_vecFonts;
	CUtlMap< int, Color > m_mapColors;
};

DECLARE_BUILD_FACTORY( CAutoFittingLabel );

class CGenericSwoop : public CControlPointIconSwoop
{
	DECLARE_CLASS_SIMPLE( CGenericSwoop, CControlPointIconSwoop );
	CGenericSwoop( float flSwoopTime, bool bDown )
		: CControlPointIconSwoop( NULL, "swoop", bDown )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
		SetScheme(scheme);
		SetProportional( true );

		SetZPos( 50000 );
		SetRotation( bDown ? ROTATED_UNROTATED : ROTATED_FLIPPED );
		GetMMDashboardParentManager()->AddPanel( this );
		PostMessage( this, new KeyValues( "StartSwoop" ), flSwoopTime );
	}

	virtual ~CGenericSwoop()
	{
		GetMMDashboardParentManager()->RemovePanel( this );
	}

	MESSAGE_FUNC( MsgStartSwoop, "StartSwoop" )
	{
		StartSwoop();
		PostMessage( this, new KeyValues( "Delete" ), STARTCAPANIM_SWOOP_LENGTH );
	}
};

void CreateSwoop( int nX, int nY, int nWide, int nTall, float flDelay, bool bDown )
{
	CGenericSwoop* pSwoop = new CGenericSwoop( flDelay, bDown );
	pSwoop->MakeReadyForUse();
	pSwoop->SetBounds( nX, nY, nWide, nTall );
}
