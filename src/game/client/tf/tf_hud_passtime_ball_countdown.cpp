//========= Contributed by 4v4 PASS Time developers. ==========================//
//
// Purpose: Shows countdown timer for when the ball
// is about to spawn from the ballspawner
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "usermessages.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include "tf_gamerules.h"
#include "hud_macros.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


class CHudPasstimeHudCountdown : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudPasstimeHudCountdown, EditablePanel );

	public:
	CHudPasstimeHudCountdown( const char *pElementName );

	virtual void ApplySchemeSettings( IScheme *scheme );
	virtual bool ShouldDraw( void );
	virtual void OnTick( void );
	virtual void Init( void );

	void MsgFunc_P4SS_Countdown( bf_read &msg );

	private:
		vgui::Label *m_pCountdownLabel;
		float m_fCountdownTime;
		float m_fCountdownLength;
};

DECLARE_HUDELEMENT( CHudPasstimeHudCountdown );
DECLARE_HUD_MESSAGE( CHudPasstimeHudCountdown, P4SS_Countdown );

CHudPasstimeHudCountdown::CHudPasstimeHudCountdown( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudPasstimeCountdown" ) {

	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	if ( !m_pCountdownLabel )
	{
		m_pCountdownLabel = new Label( this, "CountdownLabel", "" );
	}

	RegisterForRenderGroup( "inspect_panel" );

	m_fCountdownTime = 0.0f;
	m_fCountdownLength = 0.0f;
}

void CHudPasstimeHudCountdown::Init( void )
{
	HOOK_HUD_MESSAGE( CHudPasstimeHudCountdown, P4SS_Countdown );
};

void CHudPasstimeHudCountdown::MsgFunc_P4SS_Countdown( bf_read &msg ) {
	m_fCountdownTime = msg.ReadFloat();
	m_fCountdownLength = msg.ReadFloat();
}

void CHudPasstimeHudCountdown::ApplySchemeSettings( IScheme *pScheme )
{
	LoadControlSettings( "resource/UI/HudPasstimeCountdown.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

bool CHudPasstimeHudCountdown::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( m_fCountdownTime <= 0.0f )
	{
		return false;
	}

	if ( !pPlayer || !pPlayer->IsAlive() )
		return false;

	return CHudElement::ShouldDraw();
}

void CHudPasstimeHudCountdown::OnTick( void ) 
{
	if ( !IsVisible() )
		return;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	if ( m_fCountdownTime > 0.0f )
	{
		char buff[255];
		float f = ( m_fCountdownTime + m_fCountdownLength ) - gpGlobals->curtime;
		int t = (int)(f);

		snprintf( buff, sizeof( buff ), "%d", t );

		char* res = buff;

		m_pCountdownLabel->SetText( res ); 
		if ( f < 0.0f ) { 
			m_fCountdownTime = 0; 
			m_fCountdownLength = 0;
		}
	} 

}
