//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a panel which is rendered image on top of an entity
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "vgui_entityimagepanel.h"
#include <KeyValues.h>
#include "c_BaseTFPlayer.h"
#include <vgui/IVGui.h>
#include "vgui_bitmapimage.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Class factory
//-----------------------------------------------------------------------------
DECLARE_OVERLAY_FACTORY( CEntityImagePanel, "image" );


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CEntityImagePanel::CEntityImagePanel( vgui::Panel *pParent, const char *panelName ) :	
	BaseClass( pParent, panelName ), m_pImage(0)
{
	SetPaintBackgroundEnabled( false );
}

CEntityImagePanel::~CEntityImagePanel()
{
	if (m_pImage)
		delete m_pImage;
}


//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------

bool CEntityImagePanel::Init( KeyValues* pInitData, C_BaseEntity* pEntity )
{
	if (!BaseClass::Init( pInitData, pEntity))
		return false;

	// modulation color
	if (!ParseRGBA( pInitData, "color", m_r, m_g, m_b, m_a ))
		return false;

	// get the size...
	int w, h;
	if (!ParseCoord( pInitData, "offset", m_OffsetX, m_OffsetY ))
		return false;

	if (!ParseCoord( pInitData, "size", w, h ))
		return false;

	char const* pClassImage = pInitData->GetString( "material" );
	if ( !pClassImage || !pClassImage[ 0 ] )
		return false;

	const char *mouseover = pInitData->GetString( "mousehint", "" );
	if ( mouseover && mouseover[ 0 ] )
	{
		Q_strncpy( m_szMouseOverText, mouseover, sizeof( m_szMouseOverText ) );
	}

	// hook in the bitmap
	m_pImage = new BitmapImage( GetVPanel(), pClassImage );

	// Set the size...
	SetSize( w, h );

	m_iOrgWidth = w;
	m_iOrgHeight = h;
	m_iOrgOffsetX = m_OffsetX;
	m_iOrgOffsetY = m_OffsetY;

	// we need updating
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	OnTick();
	return true;
}


//-----------------------------------------------------------------------------
// Should we draw?.
//-----------------------------------------------------------------------------
bool CEntityImagePanel::ShouldDraw()
{
	return ( IsLocalPlayerInTactical() || m_bShowInNormal );
}


//-----------------------------------------------------------------------------
// Draws the puppy
//-----------------------------------------------------------------------------
void CEntityImagePanel::Paint( void )
{
	// Don't draw if I'm not visible in the tactical map
	if ( MapData().IsEntityVisibleToTactical( GetEntity() ) == false )
		return;

	vgui::surface()->DrawSetColor( m_r, m_g, m_b, m_a );

	if ( !m_pImage )
		return;

	Color color;
	color.SetColor( m_r, m_g, m_b, m_a );
	m_pImage->SetColor( color );
	m_pImage->DoPaint( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Class factory CEntityTeamImagePanel
//-----------------------------------------------------------------------------

DECLARE_OVERLAY_FACTORY( CEntityTeamImagePanel, "team_image" );



//-----------------------------------------------------------------------------
// Purpose:
// Constructor:  Same as image panel, except can handle team specific colors/images
// Input  : pEntity - 
//-----------------------------------------------------------------------------
CEntityTeamImagePanel::CEntityTeamImagePanel( vgui::Panel *pParent, const char *panelName ) :	
	BaseClass( pParent, panelName )
{
	SetPaintBackgroundEnabled( false );
	memset( m_Images, 0, sizeof( m_Images ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEntityTeamImagePanel::~CEntityTeamImagePanel( void )
{
	for ( int i = 0 ; i < MAX_TEAMS; i++ )
	{
		if ( m_Images[i].m_pImage )
		{
			delete m_Images[ i ].m_pImage;
		}
	}
	memset( m_Images, 0, sizeof( m_Images ) );
}

//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------
bool CEntityTeamImagePanel::Init( KeyValues* pInitData, C_BaseEntity* pEntity )
{
	if (!BaseClass::Init( pInitData, pEntity))
		return false;

	if ( pInitData->GetInt( "showinnormalmode", 0 ) )
	{
		m_bShowInNormal = true;
	}

	// get the size...
	int w, h;
	if (!ParseCoord( pInitData, "offset", m_OffsetX, m_OffsetY ))
		return false;

	if (!ParseCoord( pInitData, "size", w, h ))
		return false;

	// Set the size...
	SetSize( w, h );

	m_iOrgWidth = w;
	m_iOrgHeight = h;
	m_iOrgOffsetX = m_OffsetX;
	m_iOrgOffsetY = m_OffsetY;

	const char *mouseover = pInitData->GetString( "mousehint", "" );
	if ( mouseover && mouseover[ 0 ] )
	{
		Q_strncpy( m_szMouseOverText, mouseover, sizeof( m_szMouseOverText ) );
	}

	for ( int i = 0 ; i < MAX_TEAMS; i++ )
	{
		char teamname[ 32 ];
		Q_snprintf( teamname, sizeof( teamname ), "Team%i", i );

		memset( &m_Images[ i ], 0, sizeof( m_Images[ i ] ) );

		// Look for team section
		KeyValues *pTeamKV = pInitData->FindKey( teamname );
		if ( !pTeamKV )
			continue;

		// modulation color
		if (!ParseRGBA( pTeamKV, "color", m_Images[i].m_r, m_Images[i].m_g, m_Images[i].m_b, m_Images[i].m_a ))
			return false;

		char const* pClassImage = pTeamKV->GetString( "material" );
		if ( !pClassImage || !pClassImage[ 0 ] )
			return false;

		// hook in the bitmap
		m_Images[ i ].m_pImage = new BitmapImage( GetVPanel(), pClassImage );
	}

	// we need updating
	vgui::ivgui()->AddTickSignal( GetVPanel() );
	return true;
}

//-----------------------------------------------------------------------------
// Draws the puppy
//-----------------------------------------------------------------------------
void CEntityTeamImagePanel::Paint( void )
{
	// Determine team index of underlying entity
	int teamnumber = GetEntity()->GetTeamNumber();
	if ( teamnumber < 0 || teamnumber >= MAX_TEAMS )
	{
		Assert( 0 );
		return;
	}

	if ( !m_Images[ teamnumber ].m_pImage )
		return;

	// Don't draw if I'm not visible in the tactical map
	if ( MapData().IsEntityVisibleToTactical( GetEntity() ) == false )
		return;

	ComputeAndSetSize();

	vgui::surface()->DrawSetColor( m_Images[ teamnumber ].m_r, m_Images[ teamnumber ].m_g, m_Images[ teamnumber ].m_b, m_Images[ teamnumber ].m_b );
	Color color;
	color.SetColor( m_Images[ teamnumber ].m_r, m_Images[ teamnumber ].m_g, m_Images[ teamnumber ].m_b, m_Images[ teamnumber ].m_b );
	m_Images[ teamnumber ].m_pImage->SetColor( color );
	m_Images[ teamnumber ].m_pImage->DoPaint( GetVPanel() );
}