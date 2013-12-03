//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a panel which is rendered on top of an entity
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "vgui_EntityPanel.h"
#include "ienginevgui.h"
#include "c_BaseTFPlayer.h"
#include "clientmode_commander.h"
#include "hud_commander_statuspanel.h"
#include <KeyValues.h>
#include "commanderoverlaypanel.h"
#include <vgui/IVGui.h>
#include "cdll_util.h"
#include "view.h"

//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------

CEntityPanel::CEntityPanel( vgui::Panel *pParent, const char *panelName )
: BaseClass( pParent, panelName )
{
	SetPaintBackgroundEnabled( false );
	m_pBaseEntity = NULL;

	// FIXME: ComputeParent is yucky... can we be rid of it?
	if (!pParent)
	{
		ComputeParent();
	}

	m_szMouseOverText[ 0 ] = 0;

	// Send mouse inputs (but not cursorenter/exit for now) up to parent
	SetReflectMouse( true );

	m_bShowInNormal = false;

	m_flScale = 0;
	m_OffsetX = 0;
	m_OffsetY = 0;
}

//-----------------------------------------------------------------------------
// Attach to a new entity
//-----------------------------------------------------------------------------
void CEntityPanel::SetEntity( C_BaseEntity* pEntity )
{
	m_pBaseEntity = pEntity;

	// Recompute position
	OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntityPanel::ComputeParent( void )
{
	vgui::VPANEL parent = NULL;

	if ( IsLocalPlayerInTactical() || !m_bShowInNormal )
	{
		CClientModeCommander *commander = ( CClientModeCommander * )ClientModeCommander();
		Assert( commander );
		parent = commander->GetCommanderOverlayPanel()->GetVPanel();
	}
	else
	{
		parent = enginevgui->GetPanel( PANEL_CLIENTDLL );
	}
	if ( !GetParent() || ( GetParent()->GetVPanel() != parent ) )
	{
		SetParent( parent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Compute the size of the panel based upon the commander's zoom level
//-----------------------------------------------------------------------------
void CEntityPanel::ComputeAndSetSize( void )
{
	m_flScale = 1.0;

	// Scale the image
	// Use different scales in tactical / normal
	if ( IsLocalPlayerInTactical() )
	{
		CClientModeCommander *commander = ( CClientModeCommander * )ClientModeCommander();
		Assert( commander );
		float flZoom = commander->GetCommanderOverlayPanel()->GetZoom();

		// Scale our size
		m_flScale = 0.75 + (0.25 * (1.0 - flZoom)); // 1/2 size at max zoomed out, full size by half zoomed in
	}
	else if ( m_pBaseEntity )
	{
		// Get distance to entity
		float flDistance = (m_pBaseEntity->GetRenderOrigin() - MainViewOrigin()).Length();
		flDistance *= 2;
  		m_flScale = 0.25 + MAX( 0, 2.0 - (flDistance / 2048) );
	}

	// Update the size
	int w = m_iOrgWidth * m_flScale;
	int h = m_iOrgHeight * m_flScale;
	SetSize( w,h );

	// Update the offsets too
	m_OffsetX = m_iOrgOffsetX * m_flScale;
	m_OffsetY = m_iOrgOffsetY * m_flScale;
}

//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------
bool CEntityPanel::Init( ::KeyValues* pInitData, C_BaseEntity* pEntity )
{
	Assert( pInitData && pEntity );
	m_pBaseEntity = pEntity;

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

	const char *mouseover = pInitData->GetString( "mousehint", "" );
	if ( mouseover && mouseover[ 0 ] )
	{
		Q_strncpy( m_szMouseOverText, mouseover, sizeof( m_szMouseOverText ) );
	}

	SetSize( w, h );

	m_iOrgWidth = w;
	m_iOrgHeight = h;
	m_iOrgOffsetX = m_OffsetX;
	m_iOrgOffsetY = m_OffsetY;

	// we need updating
	vgui::ivgui()->AddTickSignal( GetVPanel() );
	return true;
}



//-----------------------------------------------------------------------------
// Purpose: Determine where our entity is in screen space.
//-----------------------------------------------------------------------------
void CEntityPanel::GetEntityPosition( int& sx, int& sy )
{
	if (!m_pBaseEntity)
	{
		sx = sy = -1.0f;
		return;
	}

	GetTargetInScreenSpace( m_pBaseEntity, sx, sy );
}


//-----------------------------------------------------------------------------
// Should we draw?.
//-----------------------------------------------------------------------------
bool CEntityPanel::ShouldDraw()
{
	return ( ( IsLocalPlayerInTactical() && ClientModeCommander()->ShouldDrawEntity( m_pBaseEntity ) ) || 
			 ( !IsLocalPlayerInTactical() && m_bShowInNormal) );
}


//-----------------------------------------------------------------------------
// called when we're ticked...
//-----------------------------------------------------------------------------
void CEntityPanel::OnTick()
{
	// Determine if panel parent should switch
	ComputeParent();

	// If we should shouldn't draw, don't bother with any of this
	if ( !ShouldDraw() )
		return;

	// Update our current position
	int sx, sy;
	GetEntityPosition( sx, sy );

	// Recalculate our size
	ComputeAndSetSize();

	// Set the position
	SetPos( (int)(sx + m_OffsetX + 0.5f), (int)(sy + m_OffsetY + 0.5f));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntityPanel::OnCursorEntered()
{
	if ( m_szMouseOverText[ 0 ] )
	{
		StatusPrint( TYPE_HINT, "%s", m_szMouseOverText );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntityPanel::OnCursorExited()
{
	if ( m_szMouseOverText[ 0 ] )
	{
		StatusClear();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CEntityPanel::GetMouseOverText( void )
{
	return m_szMouseOverText;
}
