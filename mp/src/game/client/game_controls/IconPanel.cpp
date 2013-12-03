//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "IconPanel.h"
#include "KeyValues.h"

DECLARE_BUILD_FACTORY( CIconPanel );

CIconPanel::CIconPanel( vgui::Panel *parent, const char *name ) : vgui::Panel( parent, name )
{
	m_szIcon[0] = '\0';
	m_icon = NULL;
	m_bScaleImage = false;
}

void CIconPanel::ApplySettings( KeyValues *inResourceData )
{
	Q_strncpy( m_szIcon, inResourceData->GetString( "icon", "" ), sizeof( m_szIcon ) );

	m_icon = gHUD.GetIcon( m_szIcon );

	m_bScaleImage = inResourceData->GetInt("scaleImage", 0);

	BaseClass::ApplySettings( inResourceData );
}

void CIconPanel::SetIcon( const char *szIcon )
{
	Q_strncpy( m_szIcon, szIcon, sizeof(m_szIcon) );

	m_icon = gHUD.GetIcon( m_szIcon );
}

void CIconPanel::Paint()
{
	BaseClass::Paint();

	if ( m_icon )
	{
		int x, y, w, h;
		GetBounds( x, y, w, h );

		if ( m_bScaleImage )
		{
			m_icon->DrawSelf( 0, 0, w, h, m_IconColor );
		}
		else
		{
			m_icon->DrawSelf( 0, 0, m_IconColor );
		}
	}	
}

void CIconPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
    
	if ( m_szIcon[0] != '\0' )
	{
		m_icon = gHUD.GetIcon( m_szIcon );
	}

	SetFgColor( pScheme->GetColor( "FgColor", Color( 255, 255, 255, 255 ) ) );
}
