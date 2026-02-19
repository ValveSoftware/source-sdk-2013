//========= Contributed by 4v4 PASS Time developers. ==========================//
//
// Purpose: Shows player speed on the HUD.
// 
//
// $NoKeywords: $
//=============================================================================//

#include "c_tf_player.h"
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "usermessages.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar pf_speedo( "pf_speedo", "1", FCVAR_ARCHIVE, "Enable/Disable the speedometer HUD element" );
ConVar pf_speedo_smoothing( "pf_speedo_smoothing", "1", FCVAR_ARCHIVE, "Enable/disable smoothing for the HUD speedometer." );
ConVar pf_speedo_bar( "pf_speedo_bar", "1", FCVAR_ARCHIVE, "Enable/Disable the speedometer bar HUD element" );
class CHudSpeedo : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudSpeedo, EditablePanel );

	public:
	CHudSpeedo( const char *pElementName );

	virtual void ApplySchemeSettings( IScheme *scheme );
	virtual bool ShouldDraw( void );
	virtual void OnTick( void );
	virtual void Init( void );

	void MsgFunc_P4SS_Speed( bf_read &msg );

	private:
		vgui::Label *m_pSpeedLabel;
		vgui::Label *m_pSpeedLabelShadow;
		float m_fSpeed;
		float m_fSmoothedSpeed;
		vgui::ImagePanel *m_pSpeedBarBG;
		vgui::ImagePanel *m_pSpeedBarFill;
		int m_nBarFullWidth;

    void UpdateSpeedBar( float rawSpeed, float smoothedSpeed );
};

DECLARE_HUDELEMENT( CHudSpeedo );
DECLARE_HUD_MESSAGE( CHudSpeedo, P4SS_Speed );

CHudSpeedo::CHudSpeedo( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudSpeedo" ) {

	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	if ( !m_pSpeedLabel )
	{
		m_pSpeedLabel = new Label( this, "SpeedLabel", "" );
	}

	if ( !m_pSpeedLabelShadow )
	{
		m_pSpeedLabelShadow = new Label( this, "SpeedLabelShadow", "" );
	}

	RegisterForRenderGroup( "inspect_panel" );

	m_fSpeed = 0.0f;
    m_fSmoothedSpeed = 0.0f;

    m_pSpeedBarBG = nullptr;
    m_pSpeedBarFill = nullptr;
    m_nBarFullWidth = 0;
}

void CHudSpeedo::Init( void )
{
	HOOK_HUD_MESSAGE( CHudSpeedo, P4SS_Speed );
};

void CHudSpeedo::MsgFunc_P4SS_Speed( bf_read &msg ) {
	m_fSpeed = msg.ReadFloat();
}

void CHudSpeedo::ApplySchemeSettings( IScheme *pScheme )
{
	LoadControlSettings( "resource/UI/HudSpeedo.res" );
	BaseClass::ApplySchemeSettings( pScheme );

    m_pSpeedBarBG   = dynamic_cast<ImagePanel*>( FindChildByName( "SpeedBarBG" ) );
    m_pSpeedBarFill = dynamic_cast<ImagePanel*>( FindChildByName( "SpeedBarFill" ) );
    if ( m_pSpeedBarBG )
    {
        m_nBarFullWidth = m_pSpeedBarBG->GetWide();
    }
    else if ( m_pSpeedBarFill )
    {
        m_nBarFullWidth = m_pSpeedBarFill->GetWide();
    }
    if ( m_nBarFullWidth <= 0 )
        m_nBarFullWidth = 100; // fallback
}

bool CHudSpeedo::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pf_speedo.GetBool() )
		return false;

	if ( !pPlayer || !pPlayer->IsAlive() )
		return false;

	return CHudElement::ShouldDraw();
}

void CHudSpeedo::UpdateSpeedBar( float rawSpeed, float smoothedSpeed )
{
    if ( !m_pSpeedBarFill )
        return;

    bool bBarEnabled = pf_speedo_bar.GetBool();

    if ( !bBarEnabled )
    {
        if ( m_pSpeedBarBG )   m_pSpeedBarBG->SetVisible( false );
        if ( m_pSpeedBarFill ) m_pSpeedBarFill->SetVisible( false );
        return;
    }

    if ( m_pSpeedBarBG )   m_pSpeedBarBG->SetVisible( true );
    if ( !m_pSpeedBarFill )
        return;
    m_pSpeedBarFill->SetVisible( true );
    float maxSpeed = 1500.0f;

    float ratio = clamp( smoothedSpeed / maxSpeed, 0.0f, 1.0f );
    const int kPad = 2;
    int fillMaxRange = m_nBarFullWidth; // fallback

    if ( m_pSpeedBarBG )
    {
        fillMaxRange = m_pSpeedBarBG->GetWide() - ( kPad * 2 );
        if ( fillMaxRange < 0 ) fillMaxRange = 0;
    }

    int newWide = (int)( fillMaxRange * ratio + 0.5f );

    if ( m_pSpeedBarBG )
    {
        int bgX, bgY, fillX, fillY;
        m_pSpeedBarBG->GetPos( bgX, bgY );
        m_pSpeedBarFill->GetPos( fillX, fillY );

        int desiredFillX = bgX + kPad;
        if ( fillX != desiredFillX )
        {
            m_pSpeedBarFill->SetPos( desiredFillX, fillY );
            fillX = desiredFillX;
        }

        int maxFillWidth = ( bgX + m_pSpeedBarBG->GetWide() - kPad ) - fillX;
        if ( maxFillWidth < 0 ) maxFillWidth = 0;
        if ( newWide > maxFillWidth )
            newWide = maxFillWidth;
    }

    m_pSpeedBarFill->SetWide( newWide );

    Color c;
    const float firstPhaseEnd = 0.75f; 
    if ( ratio < firstPhaseEnd )
    {
        float t = ratio / firstPhaseEnd; 
        int r = 255;
        int g = 255- int( t * (255 - 180) );
        int b = (int)( 255 - t * 255 ); 
        c.SetColor( r, g, b, 255 );
    }
    else
    {
        float t = ( ratio - firstPhaseEnd ) / ( 1.0f - firstPhaseEnd );
        int r = 255;
        int g = 180 - int( t * 180 );
        int b = 0;
        c.SetColor( r, g, b, 255 );
    }
    m_pSpeedBarFill->SetDrawColor( c );
}

void CHudSpeedo::OnTick( void ) 
{
	if ( !IsVisible() )
		return;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

    Vector vel = pPlayer->GetAbsVelocity();
    float rawSpeed = vel.Length2D();

	float fSmoothAmt = 0.1f;

	if ( !pf_speedo_smoothing.GetBool() )
    {
        m_fSmoothedSpeed = rawSpeed;
    }
    else
    {
        float baseAlpha = 1.0f - fSmoothAmt; 
        float scaledAlpha = baseAlpha * ( gpGlobals->frametime * 10.0f ); // normalize vs prior 0.1s ticks
        scaledAlpha = clamp( scaledAlpha, 0.0f, 1.0f );
        m_fSmoothedSpeed += ( rawSpeed - m_fSmoothedSpeed ) * scaledAlpha;
    }

    int displaySpeed = int( m_fSmoothedSpeed + 0.5f );

    char buf[32];
    V_snprintf( buf, sizeof( buf ), "%d", displaySpeed );
    m_pSpeedLabel->SetText( buf );
	m_pSpeedLabelShadow->SetText( buf );
    UpdateSpeedBar( rawSpeed, m_fSmoothedSpeed );
}
