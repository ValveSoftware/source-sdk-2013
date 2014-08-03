//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "c_vguiscreen.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include "clientmode_hlnormal.h"
#include "tier1/utllinkedlist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Amount of time before breen teleports away
//-----------------------------------------------------------------------------
class C_InfoTeleporterCountdown : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_InfoTeleporterCountdown, C_BaseEntity );
	DECLARE_CLIENTCLASS();

public:
	C_InfoTeleporterCountdown();
	~C_InfoTeleporterCountdown();

	virtual bool ShouldDraw() { return false; }

private:
	bool m_bCountdownStarted;
	bool m_bDisabled;
	float m_flStartTime;
	float m_flTimeRemaining;

	friend class CTeleportCountdownScreen;
};


//-----------------------------------------------------------------------------
// Global list of teleporters
//-----------------------------------------------------------------------------
CUtlFixedLinkedList<C_InfoTeleporterCountdown *> g_InfoTeleporterCountdownList;


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_InfoTeleporterCountdown, DT_InfoTeleporterCountdown, CInfoTeleporterCountdown )
	RecvPropInt( RECVINFO( m_bCountdownStarted ) ),
	RecvPropInt( RECVINFO( m_bDisabled ) ),
	RecvPropTime( RECVINFO( m_flStartTime ) ),
	RecvPropFloat( RECVINFO( m_flTimeRemaining ) ),	
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
C_InfoTeleporterCountdown::C_InfoTeleporterCountdown()
{		
	g_InfoTeleporterCountdownList.AddToTail( this );
}

C_InfoTeleporterCountdown::~C_InfoTeleporterCountdown()
{
	g_InfoTeleporterCountdownList.FindAndRemove( this );
}


//-----------------------------------------------------------------------------
//
// In-game vgui panel which shows the teleporter countdown
//
//-----------------------------------------------------------------------------
class CTeleportCountdownScreen : public CVGuiScreenPanel
{
	DECLARE_CLASS( CTeleportCountdownScreen, CVGuiScreenPanel );

public:
	CTeleportCountdownScreen( vgui::Panel *parent, const char *panelName );

	virtual bool Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData );
	virtual void OnTick();

private:
	vgui::Label *m_pTimeRemainingTitleLabel;
	vgui::Label *m_pTimeRemainingLabel;
	vgui::Label *m_pMalfunctionLabel;
};


//-----------------------------------------------------------------------------
// Standard VGUI panel for objects 
//-----------------------------------------------------------------------------
DECLARE_VGUI_SCREEN_FACTORY( CTeleportCountdownScreen, "teleport_countdown_screen" );


//-----------------------------------------------------------------------------
// Constructor: 
//-----------------------------------------------------------------------------
CTeleportCountdownScreen::CTeleportCountdownScreen( vgui::Panel *parent, const char *panelName )
	: BaseClass( parent, panelName, g_hVGuiCombineScheme ) 
{
}


//-----------------------------------------------------------------------------
// Initialization 
//-----------------------------------------------------------------------------
bool CTeleportCountdownScreen::Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData )
{
	// Load all of the controls in
	if ( !BaseClass::Init(pKeyValues, pInitData) )
		return false;

	// Make sure we get ticked...
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	// Grab ahold of certain well-known controls
	// NOTE: it is valid for these controls to not exist!
	m_pTimeRemainingTitleLabel = dynamic_cast<vgui::Label*>(FindChildByName( "TimeRemainingTitle" ));
	m_pTimeRemainingLabel = dynamic_cast<vgui::Label*>(FindChildByName( "TimeRemaining" ));
	m_pMalfunctionLabel = dynamic_cast<vgui::Label*>( FindChildByName( "MalfunctionLabel" ) );
	
	return true;
}


//-----------------------------------------------------------------------------
// Frame-based update
//-----------------------------------------------------------------------------
void CTeleportCountdownScreen::OnTick()
{
	BaseClass::OnTick();

	// Find the active info teleporter countdown
	C_InfoTeleporterCountdown *pActiveCountdown = NULL;
	for ( int i = g_InfoTeleporterCountdownList.Head(); i != g_InfoTeleporterCountdownList.InvalidIndex();
		i = g_InfoTeleporterCountdownList.Next(i) )
	{
		if ( g_InfoTeleporterCountdownList[i]->m_bCountdownStarted )
		{
			pActiveCountdown = g_InfoTeleporterCountdownList[i];
			break;
		}
	}

	if ( !GetEntity() || !pActiveCountdown )
	{
		m_pTimeRemainingTitleLabel->SetVisible( false );
		m_pTimeRemainingLabel->SetVisible( false );
		m_pMalfunctionLabel->SetVisible( false );

		return;
	}

	// Make the appropriate labels visible
	bool bMalfunction = pActiveCountdown->m_bDisabled;
	m_pTimeRemainingTitleLabel->SetVisible( !bMalfunction );
	m_pTimeRemainingLabel->SetVisible( !bMalfunction );

	// This will make it flash
	m_pMalfunctionLabel->SetVisible( bMalfunction && (((int)(gpGlobals->curtime) & 0x1) == 0x1) );

	// Update the time remaining
	if ( !bMalfunction )
	{
		char buf[32];
		if (m_pTimeRemainingLabel)
		{
			float dt = gpGlobals->curtime - pActiveCountdown->m_flStartTime;
			if ( dt < 0.0f )
			{
				dt = 0.0f;
			}

			int nTimeRemaining = (int)(pActiveCountdown->m_flTimeRemaining - dt + 0.5f); 
			if ( nTimeRemaining < 0 )
			{
				nTimeRemaining = 0;
			}

			Q_snprintf( buf, sizeof( buf ), "%d", nTimeRemaining );
			m_pTimeRemainingLabel->SetText( buf );
		}
	}
}

