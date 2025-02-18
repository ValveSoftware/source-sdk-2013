//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "backgroundpanel.h"

#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Label.h>
#include <vgui/ILocalize.h>
#include "vgui_controls/BuildGroup.h"
#include "vgui_controls/BitmapImagePanel.h"

using namespace vgui;

#define DEBUG_WINDOW_RESIZING 0
#define DEBUG_WINDOW_REPOSITIONING 0

class CaptionLabel : public Label
{
public:
	CaptionLabel(Panel *parent, const char *panelName, const char *text) : Label(parent, panelName, text)
	{
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		Label::ApplySchemeSettings( pScheme );
		SetFont( pScheme->GetFont( "MenuTitle", IsProportional() ) );
	}
};

//-----------------------------------------------------------------------------
// Purpose: transform a normalized value into one that is scaled based the minimum
//          of the horizontal and vertical ratios
//-----------------------------------------------------------------------------
static int GetAlternateProportionalValueFromNormal(int normalizedValue)
{
	int wide, tall;
	GetHudSize( wide, tall );
	int proH, proW;
	surface()->GetProportionalBase( proW, proH );
	double scaleH = (double)tall / (double)proH;
	double scaleW = (double)wide / (double)proW;
	double scale = (scaleW < scaleH) ? scaleW : scaleH;

	return (int)( normalizedValue * scale );
}

//-----------------------------------------------------------------------------
// Purpose: transform a standard scaled value into one that is scaled based the minimum
//          of the horizontal and vertical ratios
//-----------------------------------------------------------------------------
int GetAlternateProportionalValueFromScaled( HScheme hScheme, int scaledValue)
{
	return GetAlternateProportionalValueFromNormal( scheme()->GetProportionalNormalizedValueEx( hScheme,  scaledValue ) );
}

//-----------------------------------------------------------------------------
// Purpose: moves and resizes a single control
//-----------------------------------------------------------------------------
static void RepositionControl( Panel *pPanel )
{
	int x, y, w, h;
	pPanel->GetBounds(x, y, w, h);

#if DEBUG_WINDOW_RESIZING
	int x1, y1, w1, h1;
	pPanel->GetBounds(x1, y1, w1, h1);
	int x2, y2, w2, h2;
	x2 = scheme()->GetProportionalNormalizedValueEx( pPanel->GetScheme(),  x1 );
	y2 = scheme()->GetProportionalNormalizedValueEx( pPanel->GetScheme(),  y1 );
	w2 = scheme()->GetProportionalNormalizedValueEx( pPanel->GetScheme(),  w1 );
	h2 = scheme()->GetProportionalNormalizedValueEx( pPanel->GetScheme(),  h1 );
#endif

	x = GetAlternateProportionalValueFromScaled(pPanel->GetScheme(),x);
	y = GetAlternateProportionalValueFromScaled(pPanel->GetScheme(),y);
	w = GetAlternateProportionalValueFromScaled(pPanel->GetScheme(),w);
	h = GetAlternateProportionalValueFromScaled(pPanel->GetScheme(),h);

	pPanel->SetBounds(x, y, w, h);

#if DEBUG_WINDOW_RESIZING
	DevMsg( "Resizing '%s' from (%d,%d) %dx%d to (%d,%d) %dx%d -- initially was (%d,%d) %dx%d\n",
		pPanel->GetName(), x1, y1, w1, h1, x, y, w, h, x2, y2, w2, h2 );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Sets colors etc for background image panels
//-----------------------------------------------------------------------------
void ApplyBackgroundSchemeSettings( EditablePanel *pWindow, vgui::IScheme *pScheme )
{
	Color bgColor = Color( 255, 255, 255, pScheme->GetColor( "BgColor", Color( 0, 0, 0, 0 ) )[3] );
	Color fgColor = pScheme->GetColor( "FgColor", Color( 0, 0, 0, 0 ) );

	if ( !pWindow )
		return;

	CBitmapImagePanel *pBitmapPanel;

	// corners --------------------------------------------
	pBitmapPanel = dynamic_cast< CBitmapImagePanel * >(pWindow->FindChildByName( "TopLeftPanel" ));
	if ( pBitmapPanel )
	{
		pBitmapPanel->setImageColor( bgColor );
	}
	pBitmapPanel = dynamic_cast< CBitmapImagePanel * >(pWindow->FindChildByName( "TopRightPanel" ));
	if ( pBitmapPanel )
	{
		pBitmapPanel->setImageColor( bgColor );
	}
	pBitmapPanel = dynamic_cast< CBitmapImagePanel * >(pWindow->FindChildByName( "BottomLeftPanel" ));
	if ( pBitmapPanel )
	{
		pBitmapPanel->setImageColor( bgColor );
	}
	pBitmapPanel = dynamic_cast< CBitmapImagePanel * >(pWindow->FindChildByName( "BottomRightPanel" ));
	if ( pBitmapPanel )
	{
		pBitmapPanel->setImageColor( bgColor );
	}

	// background -----------------------------------------
	pBitmapPanel = dynamic_cast< CBitmapImagePanel * >(pWindow->FindChildByName( "TopSolid" ));
	if ( pBitmapPanel )
	{
		pBitmapPanel->setImageColor( bgColor );
	}
	pBitmapPanel = dynamic_cast< CBitmapImagePanel * >(pWindow->FindChildByName( "UpperMiddleSolid" ));
	if ( pBitmapPanel )
	{
		pBitmapPanel->setImageColor( bgColor );
	}
	pBitmapPanel = dynamic_cast< CBitmapImagePanel * >(pWindow->FindChildByName( "LowerMiddleSolid" ));
	if ( pBitmapPanel )
	{
		pBitmapPanel->setImageColor( bgColor );
	}
	pBitmapPanel = dynamic_cast< CBitmapImagePanel * >(pWindow->FindChildByName( "BottomSolid" ));
	if ( pBitmapPanel )
	{
		pBitmapPanel->setImageColor( bgColor );
	}

	// Logo -----------------------------------------------
	pBitmapPanel = dynamic_cast< CBitmapImagePanel * >(pWindow->FindChildByName( "ExclamationPanel" ));
	if ( pBitmapPanel )
	{
		pBitmapPanel->setImageColor( fgColor );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Re-aligns background image panels so they are touching.
//-----------------------------------------------------------------------------
static void FixupBackgroundPanels( EditablePanel *pWindow, int offsetX, int offsetY )
{
	if ( !pWindow )
		return;

	int screenWide, screenTall;
	pWindow->GetSize( screenWide, screenTall );

	int inset = GetAlternateProportionalValueFromNormal( 20 );
	int cornerSize = GetAlternateProportionalValueFromNormal( 10 );

	int titleHeight = GetAlternateProportionalValueFromNormal( 42 );
	int mainHeight = GetAlternateProportionalValueFromNormal( 376 );

	int logoSize = titleHeight;

	int captionInset = GetAlternateProportionalValueFromNormal( 76 );

	Panel *pPanel;

	// corners --------------------------------------------
	pPanel = pWindow->FindChildByName( "TopLeftPanel" );
	if ( pPanel )
	{
		pPanel->SetZPos( -20 );
		pPanel->SetBounds( offsetX + inset, offsetY + inset, cornerSize, cornerSize );
	}

	pPanel = pWindow->FindChildByName( "TopRightPanel" );
	if ( pPanel )
	{
		pPanel->SetZPos( -20 );
		pPanel->SetBounds( screenWide - offsetX - inset - cornerSize, offsetY + inset, cornerSize, cornerSize );
	}

	pPanel = pWindow->FindChildByName( "BottomLeftPanel" );
	if ( pPanel )
	{
		pPanel->SetZPos( -20 );
		pPanel->SetBounds( offsetX + inset, screenTall - offsetY - inset - cornerSize, cornerSize, cornerSize );
	}

	pPanel = pWindow->FindChildByName( "BottomRightPanel" );
	if ( pPanel )
	{
		pPanel->SetZPos( -20 );
		pPanel->SetBounds( screenWide - offsetX - inset - cornerSize, screenTall - offsetY - inset - cornerSize, cornerSize, cornerSize );
	}

	// background -----------------------------------------
	pPanel = pWindow->FindChildByName( "TopSolid" );
	if ( pPanel )
	{
		pPanel->SetZPos( -20 );
		pPanel->SetBounds( offsetX + inset + cornerSize, offsetY + inset, screenWide - 2*offsetX - 2*inset - 2*cornerSize, cornerSize );
	}

	pPanel = pWindow->FindChildByName( "UpperMiddleSolid" );
	if ( pPanel )
	{
		pPanel->SetZPos( -20 );
		pPanel->SetBounds( offsetX + inset, offsetY + inset + cornerSize, screenWide - 2*offsetX - 2*inset, titleHeight );
	}

	pPanel = pWindow->FindChildByName( "LowerMiddleSolid" );
	if ( pPanel )
	{
		pPanel->SetZPos( -20 );
		pPanel->SetBounds( offsetX + inset + cornerSize, screenTall - offsetY - inset - cornerSize, screenWide - 2*offsetX - 2*inset - 2*cornerSize, cornerSize );
	}

	pPanel = pWindow->FindChildByName( "BottomSolid" );
	if ( pPanel )
	{
		pPanel->SetZPos( -20 );
		pPanel->SetBounds( offsetX + inset, screenTall - offsetY - inset - cornerSize - mainHeight, screenWide - 2*offsetX - 2*inset, mainHeight );
	}

	// transparent border ---------------------------------
	pPanel = pWindow->FindChildByName( "TopClear" );
	if ( pPanel )
	{
		pPanel->SetZPos( -20 );
		pPanel->SetBounds( 0, 0, screenWide, offsetY + inset );
	}

	pPanel = pWindow->FindChildByName( "BottomClear" );
	if ( pPanel )
	{
		pPanel->SetZPos( -20 );
		pPanel->SetBounds( 0, screenTall - offsetY - inset, screenWide, offsetY + inset );
	}

	pPanel = pWindow->FindChildByName( "LeftClear" );
	if ( pPanel )
	{
		pPanel->SetZPos( -20 );
		pPanel->SetBounds( 0, offsetY + inset, offsetX + inset, screenTall - 2*offsetY - 2*inset );
	}

	pPanel = pWindow->FindChildByName( "RightClear" );
	if ( pPanel )
	{
		pPanel->SetZPos( -20 );
		pPanel->SetBounds( screenWide - offsetX - inset, offsetY + inset, offsetX + inset, screenTall - 2*offsetY - 2*inset );
	}

	// Logo -----------------------------------------------
	int logoInset = (cornerSize + titleHeight - logoSize)/2;
	pPanel = pWindow->FindChildByName( "ExclamationPanel" );
	if ( pPanel )
	{
		pPanel->SetZPos( -19 ); // higher than the background
		pPanel->SetBounds( offsetX + inset + logoInset, offsetY + inset + logoInset, logoSize, logoSize );
	}

	// Title caption --------------------------------------
	pPanel = dynamic_cast< Label * >(pWindow->FindChildByName( "CaptionLabel" ));
	if ( pPanel )
	{
		pPanel->SetZPos( -19 ); // higher than the background
		pPanel->SetBounds( offsetX + captionInset/*inset + 2*logoInset + logoSize*/, offsetY + inset + logoInset, screenWide, logoSize );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Creates background image panels
//-----------------------------------------------------------------------------
void CreateBackground( EditablePanel *pWindow )
{
	// corners --------------------------------------------
	new CBitmapImagePanel( pWindow, "TopLeftPanel", "gfx/vgui/round_corner_nw" );
	new CBitmapImagePanel( pWindow, "TopRightPanel", "gfx/vgui/round_corner_ne" );
	new CBitmapImagePanel( pWindow, "BottomLeftPanel", "gfx/vgui/round_corner_sw" );
	new CBitmapImagePanel( pWindow, "BottomRightPanel", "gfx/vgui/round_corner_se" );

	// background -----------------------------------------
	new CBitmapImagePanel( pWindow, "TopSolid", "gfx/vgui/solid_background" );
	new CBitmapImagePanel( pWindow, "UpperMiddleSolid", "gfx/vgui/solid_background" );
	new CBitmapImagePanel( pWindow, "LowerMiddleSolid", "gfx/vgui/solid_background" );
	new CBitmapImagePanel( pWindow, "BottomSolid", "gfx/vgui/solid_background" );

	// transparent border ---------------------------------
	new CBitmapImagePanel( pWindow, "TopClear", "gfx/vgui/trans_background" );
	new CBitmapImagePanel( pWindow, "BottomClear", "gfx/vgui/trans_background" );
	new CBitmapImagePanel( pWindow, "LeftClear", "gfx/vgui/trans_background" );
	new CBitmapImagePanel( pWindow, "RightClear", "gfx/vgui/trans_background" );

	// Logo -----------------------------------------------
	new CBitmapImagePanel( pWindow, "ExclamationPanel", "gfx/vgui/hl2mp_logo" );

	// Title caption --------------------------------------
	Panel *pPanel = dynamic_cast< Label * >(pWindow->FindChildByName( "CaptionLabel" ));
	if ( !pPanel )
		new CaptionLabel( pWindow, "CaptionLabel", "" );
}

void ResizeWindowControls( EditablePanel *pWindow, int tall, int wide, int offsetX, int offsetY )
{
	if (!pWindow || !pWindow->GetBuildGroup() || !pWindow->GetBuildGroup()->GetPanelList())
		return;

	CUtlVector<PHandle> *panelList = pWindow->GetBuildGroup()->GetPanelList();
	CUtlVector<Panel *> resizedPanels;
	CUtlVector<Panel *> movedPanels;

	// Resize to account for 1.25 aspect ratio (1280x1024) screens
	{
		for ( int i = 0; i < panelList->Size(); ++i )
		{
			PHandle handle = (*panelList)[i];

			Panel *panel = handle.Get();

			bool found = false;
			for ( int j = 0; j < resizedPanels.Size(); ++j )
			{
				if (panel == resizedPanels[j])
					found = true;
			}

			if (!panel || found)
			{
				continue;
			}

			resizedPanels.AddToTail( panel ); // don't move a panel more than once

			if ( panel != pWindow )
			{
				RepositionControl( panel );
			}
		}
	}

	// and now re-center them.  Woohoo!
	for ( int i = 0; i < panelList->Size(); ++i )
	{
		PHandle handle = (*panelList)[i];

		Panel *panel = handle.Get();

		bool found = false;
		for ( int j = 0; j < movedPanels.Size(); ++j )
		{
			if (panel == movedPanels[j])
				found = true;
		}

		if (!panel || found)
		{
			continue;
		}

		movedPanels.AddToTail( panel ); // don't move a panel more than once

		if ( panel != pWindow )
		{
			int x, y;

			panel->GetPos( x, y );
			panel->SetPos( x + offsetX, y + offsetY );

#if DEBUG_WINDOW_REPOSITIONING
			DevMsg( "Repositioning '%s' from (%d,%d) to (%d,%d) -- a distance of (%d,%d)\n",
				panel->GetName(), x, y, x + offsetX, y + offsetY, offsetX, offsetY );
#endif
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resizes windows to fit completely on-screen (for 1280x1024), and
//          centers them on the screen.  Sub-controls are also resized and moved.
//-----------------------------------------------------------------------------
void LayoutBackgroundPanel( EditablePanel *pWindow )
{
	if ( !pWindow )
		return;

	int screenW, screenH;
	GetHudSize( screenW, screenH );

	int wide, tall;
	pWindow->GetSize( wide, tall );

	int offsetX = 0;
	int offsetY = 0;

	// Slide everything over to the center
	pWindow->SetBounds( 0, 0, screenW, screenH );

	if ( wide != screenW || tall != screenH )
	{
		wide = GetAlternateProportionalValueFromScaled(pWindow->GetScheme(), wide);
		tall = GetAlternateProportionalValueFromScaled(pWindow->GetScheme(), tall);

		offsetX = (screenW - wide)/2;
		offsetY = (screenH - tall)/2;

		ResizeWindowControls( pWindow, tall, wide, offsetX, offsetY );
	}

	// now that the panels are moved/resized, look for some bg panels, and re-align them
	FixupBackgroundPanels( pWindow, offsetX, offsetY );
}

//-----------------------------------------------------------------------------

