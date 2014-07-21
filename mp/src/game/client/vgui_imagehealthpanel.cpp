//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a panel which is rendered image on top of an entity
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "vgui_imagehealthpanel.h"
#include "commanderoverlay.h"
#include <KeyValues.h>
#include "mapdata.h"

bool IsLocalPlayerInTactical( void );

//-----------------------------------------------------------------------------
// Class factory
//-----------------------------------------------------------------------------

DECLARE_OVERLAY_FACTORY( CEntityImageHealthPanel, "image_health" );


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CEntityImageHealthPanel::CEntityImageHealthPanel( vgui::Panel *parent, const char *panelName )
: BaseClass( parent, "CEntityImageHealthPanel" )
{
	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEntityImageHealthPanel::~CEntityImageHealthPanel()
{
	if ( m_pImagePanel )
	{
		delete m_pImagePanel;
		m_pImagePanel = NULL;
	}
}

//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------
bool CEntityImageHealthPanel::Init( KeyValues* pInitData, C_BaseEntity* pEntity )
{
	if ( !pInitData )
		return false;
	if ( !BaseClass::Init( pInitData, pEntity ) )
		return false;

	m_CommanderHealthBar = NULL;
	m_NormalHealthBar = NULL;
	m_ResourceLevelBar = NULL;
	m_pImagePanel = NULL;

	// Health bar when we're in commander view
	KeyValues *pHealth = pInitData->FindKey("health_commander");
	if ( pHealth )
	{
		m_CommanderHealthBar = new CHealthBarPanel();
		if (!m_CommanderHealthBar->Init( pHealth ))
			return false;
		m_CommanderHealthBar->SetParent( this );
	}

	// Health bar when we're in normal view
	pHealth = pInitData->FindKey("health_normal");
	if ( pHealth )
	{
		m_NormalHealthBar = new CHealthBarPanel();
		if (!m_NormalHealthBar->Init( pHealth ))
			return false;
		m_NormalHealthBar->SetParent( this );
	}

	// Resource bar for collectors
	KeyValues *pResources = pInitData->FindKey("resource_level");
	if ( pResources )
	{
		m_ResourceLevelBar = new CHealthBarPanel();
		if (!m_ResourceLevelBar->Init( pResources ))
			return false;
		m_ResourceLevelBar->SetParent( this );
	}

	KeyValues *pImage = pInitData->FindKey("Image");
	if ( pImage )
	{
		m_pImagePanel = new CEntityTeamImagePanel( this, "CEntityTeamImagePanel" );
		if ( !m_pImagePanel->Init( pImage, GetEntity() ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Should we draw?.
//-----------------------------------------------------------------------------
bool CEntityImageHealthPanel::ShouldDraw( void )
{
	// Always draw health
	return true;
}

//-----------------------------------------------------------------------------
// called when we're ticked...
//-----------------------------------------------------------------------------
void CEntityImageHealthPanel::OnTick()
{
	// tick the entity panel
	BaseClass::OnTick();

	C_BaseEntity* pBaseEntity = GetEntity();
	if (!pBaseEntity)
		return;
	// Don't draw if I'm not visible in the tactical map
	if ( MapData().IsEntityVisibleToTactical( pBaseEntity ) == false )
		return;

	if ( m_CommanderHealthBar )
		m_CommanderHealthBar->SetHealth( (float)pBaseEntity->GetHealth() / (float)pBaseEntity->GetMaxHealth() );
	if ( m_NormalHealthBar )
		m_NormalHealthBar->SetHealth( (float)pBaseEntity->GetHealth() / (float)pBaseEntity->GetMaxHealth() );

	// Hide the health bar we don't want to see
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( pPlayer && (pBaseEntity->GetTeamNumber() != pPlayer->GetTeamNumber()) )
	{
		if ( m_CommanderHealthBar )
			m_CommanderHealthBar->SetVisible( false );
		if ( m_NormalHealthBar ) 
			m_NormalHealthBar->SetVisible( false );
		if ( m_ResourceLevelBar )
			m_ResourceLevelBar->SetVisible( false );
		if ( m_pImagePanel )
			m_pImagePanel->SetVisible( false );
	}
	else if ( IsLocalPlayerInTactical() )
	{
		if ( m_CommanderHealthBar )
			m_CommanderHealthBar->SetVisible( true );
		if ( m_NormalHealthBar ) 
			m_NormalHealthBar->SetVisible( false );
		if ( m_ResourceLevelBar )
			m_ResourceLevelBar->SetVisible( true );
		if ( m_pImagePanel )
			m_pImagePanel->SetVisible( true );
	}
	else
	{
		if ( m_CommanderHealthBar )
			m_CommanderHealthBar->SetVisible( false );
		if ( m_NormalHealthBar ) 
			m_NormalHealthBar->SetVisible( true );
		if ( m_ResourceLevelBar )
			m_ResourceLevelBar->SetVisible( true );
		if ( m_pImagePanel )
			m_pImagePanel->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Compute the size of the panel based upon the commander's zoom level
//-----------------------------------------------------------------------------
void CEntityImageHealthPanel::ComputeAndSetSize( void )
{
	BaseClass::ComputeAndSetSize();

	// Now update the bars
	if ( m_CommanderHealthBar )
		m_CommanderHealthBar->SetSize( GetWide(), m_CommanderHealthBar->GetTall() );
	if ( m_NormalHealthBar ) 
		m_NormalHealthBar->SetSize( GetWide(), m_CommanderHealthBar->GetTall() );
	if ( m_ResourceLevelBar )
		m_ResourceLevelBar->SetSize( GetWide(), m_CommanderHealthBar->GetTall() );
}
