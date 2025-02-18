//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Panel.h>
#include <vgui/IPanel.h>
#include "vgui_int.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define ANIM_INFO_START_Y 50
#define ANIM_INFO_WIDTH XRES( 300 )

class CHudAnimationInfo : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudAnimationInfo, vgui::Panel );

public:

	CHudAnimationInfo( const char *pElementName );

	virtual bool ShouldDraw();

	// vgui::Panel overrides.
	virtual void Paint( void );

	virtual void ApplySchemeSettings( IScheme *scheme );

	void SetWatch( Panel *element )
	{
		m_pWatch = element;
	}

protected:

	void PaintMappingInfo( int& x, int& y, Panel *element, PanelAnimationMap *map );
	void PaintString( int& x, int &y, const char *sz, Color *pLegendColor );

	CPanelAnimationVar( vgui::HFont, m_LabelFont, "LabelFont", "DebugFixed" );
	CPanelAnimationVar( vgui::HFont, m_ItemFont, "ItemFont", "DebugFixedSmall" );

	CPanelAnimationVar( Color, m_LabelColor, "LabelColor", "DebugLabel" );
	CPanelAnimationVar( Color, m_ItemColor, "ItemColor", "DebugText");

	Panel *m_pWatch;
};

DECLARE_HUDELEMENT( CHudAnimationInfo );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pElementName - 
//			*panelName - 
//-----------------------------------------------------------------------------
CHudAnimationInfo::CHudAnimationInfo( const char *pElementName )
 : CHudElement( pElementName ), BaseClass( NULL, "HudAnimationInfo" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetActive( true );

	m_pWatch = NULL;

	// Make sure we render on top of other hud elements since we are debugging info for them.
	SetZPos( 100 );
}

void CHudAnimationInfo::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_LabelFont = scheme->GetFont( "DebugFixed", true );
	m_ItemFont= scheme->GetFont( "DebugFixedSmall", true );

	m_LabelColor = scheme->GetColor( "DebugLabel", GetFgColor() );
	m_ItemColor= scheme->GetColor( "DebugText", GetFgColor() );

	SetPaintBackgroundEnabled( false );
}

bool CHudAnimationInfo::ShouldDraw()
{
	return ( m_pWatch && CHudElement::ShouldDraw() );
}

void CHudAnimationInfo::PaintString( int& x, int &y, const char *sz, Color *pLegendColor )
{
	surface()->DrawSetTextFont( m_ItemFont );
	surface()->DrawSetTextPos( x, y );

	wchar_t szconverted[ 512 ];

	g_pVGuiLocalize->ConvertANSIToUnicode( "O->", szconverted, sizeof(szconverted)  );
		
	if ( pLegendColor )
	{
		surface()->DrawSetTextColor( *pLegendColor );
	}
	else
	{
		surface()->DrawSetTextColor( Color( 0, 0, 0, 0 ) );
	}

	surface()->DrawPrintText( szconverted, wcslen( szconverted ) );

	g_pVGuiLocalize->ConvertANSIToUnicode( sz, szconverted, sizeof(szconverted)  );

	surface()->DrawSetTextColor( m_ItemColor );
	surface()->DrawPrintText( szconverted, wcslen( szconverted ) );

	int fontHeight = surface()->GetFontTall( m_ItemFont );

	y += fontHeight;

	if ( y + fontHeight >= ScreenHeight() )
	{
		y = ANIM_INFO_START_Y;
		x += ANIM_INFO_WIDTH;
	}
}

void CHudAnimationInfo::PaintMappingInfo( int& x, int& y, Panel *element, PanelAnimationMap *map )
{
	if ( !map )
		return;

	// Draw label
	surface()->DrawSetTextFont( m_LabelFont );
	surface()->DrawSetTextColor( m_LabelColor );
	surface()->DrawSetTextPos( x, y );

	const char *className = "";
	if ( map->pfnClassName )
	{
		className = (*map->pfnClassName)();
	}

	const char *p = className;
	while ( *p )
	{
		surface()->DrawUnicodeChar( *p );
		p++;
	}

	y += surface()->GetFontTall( m_LabelFont ) + 1;

	x += 10;


	int c = map->entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		PanelAnimationMapEntry *e = &map->entries[ i ];

		char sz[ 512 ];
		char value[ 256 ];

		Color col( 0, 0, 0, 0 );
		Color  *pColor = NULL;
		KeyValues *kv = new KeyValues( e->name() );
		if ( element->RequestInfo( kv ) )
		{
			KeyValues *dat = kv->FindKey(e->name());
			if ( dat && dat->GetDataType() == KeyValues::TYPE_COLOR )
			{
				col = dat->GetColor();
				Q_snprintf( value, sizeof( value ), "%i, %i, %i, %i",
					col[0], col[1], col[2], col[3] );
				pColor = &col;
			}
			else
			{
				Q_snprintf( value, sizeof( value ), "%s",
					dat->GetString() );
			}
		}
		else
		{
			Q_strncpy( value, "???", sizeof( value ) );
		}

		Q_snprintf( sz, sizeof( sz ), "%-30s %-20s (%s)",
			e->name(), e->type(), value );

		kv->deleteThis();

		PaintString( x, y, sz, pColor );
	}

	x -= 10;

	if ( map->baseMap )
	{
		PaintMappingInfo( x, y, element, map->baseMap );
	}
}

void CHudAnimationInfo::Paint()
{
	vgui::Panel *panel = m_pWatch;
	if ( !panel )
		return;

	// See if it has any animation info registered
	PanelAnimationMap *map = panel->GetAnimMap();
	if ( !map )
		return;

	int x = 15;
	int y = ANIM_INFO_START_Y;

	PaintMappingInfo( x, y, panel, map );

	x += 10;

	// Paint panel bounds
	int bounds[4];
	panel->GetBounds( bounds[0], bounds[1], bounds[2], bounds[3] );
	char sz[ 256 ];
	Q_snprintf( sz, sizeof( sz ), "%-30s %-20s (%i %i)", "Position", "pos", bounds[0], bounds[1] );
	PaintString( x, y, sz, NULL );
	Q_snprintf( sz, sizeof( sz ), "%-30s %-20s (%i %i)", "Size", "size", bounds[2], bounds[3] );
	PaintString( x, y, sz, NULL );
}


static int HudElementCompletion( const char *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	const char *cmdname = "cl_animationinfo";

	char *substring = (char *)partial;
	if ( Q_strstr( partial, cmdname ) )
	{
		substring = (char *)partial + strlen( cmdname ) + 1;
	}

	int current = 0;

	int c = gHUD.m_HudList.Count();
	int i;
	for ( i = 0; i < c; i++ )
	{
		CHudElement *e = gHUD.m_HudList[ i ];
		if ( !e )
			continue;

		bool add = false;

		// Insert into lookup
		if ( substring[0] )
		{
			if ( !Q_strncasecmp( e->GetName(), substring, strlen( substring ) ) )
			{
				add = true;
			}
		}
		else
		{
			add = true;
		}

		if ( add )
		{
			Q_snprintf( commands[ current ], sizeof( commands[ current ] ), "%s %s", cmdname, e->GetName() );
			current++;
		}
	}

	return current;
}

CON_COMMAND_F_COMPLETION( cl_animationinfo, "Hud element to examine.", 0, HudElementCompletion )
{
	CHudAnimationInfo *info = GET_HUDELEMENT( CHudAnimationInfo );
	Assert( info );
	if ( !info )
		return;

	if ( args.ArgC() != 2 )
	{
		info->SetWatch( NULL );
		return;
	}

	// Find it
	CHudElement *element = NULL;
	
	for ( int i = 0; i < gHUD.m_HudList.Size(); i++ )
	{
		if ( stricmp( gHUD.m_HudList[i]->GetName(), args[1]  ) == 0 )
		{
			element = gHUD.m_HudList[i];
			break;
		}
	}

	if ( element )
	{
		info->SetWatch( dynamic_cast< Panel * >( element ) );
	}
	else 
	{
		VPANEL root = VGui_GetClientDLLRootPanel();
		vgui::Panel *rootPanel = ipanel()->GetPanel( root, info->GetModuleName() );
		Panel *panel = NULL;
		if ( rootPanel )
		{
			panel = rootPanel->FindChildByName( args[1], true );
		}

		if ( panel )
		{
			info->SetWatch( panel );
		}
		else
		{
			Msg( "No such element %s\n", args[1] );
		}
	}
}

