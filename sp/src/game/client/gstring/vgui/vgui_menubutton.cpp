
#include "cbase.h"
#include "tier1/KeyValues.h"
#include "Gstring/gstring_cvars.h"
#include "Gstring/vgui/vgui_gstringMain.h"
#include "ienginevgui.h"

#include "vgui_controls/panel.h"
#include <vgui/ISurface.h>
#include <vgui/IInput.h>

#include "vgui_controls/checkbutton.h"
#include "vgui_controls/combobox.h"
#include "vgui_controls/slider.h"
#include "vgui_controls/button.h"
#include "vgui_controls/Label.h"


using namespace vgui;


CVGUIMenuButton::CVGUIMenuButton( vgui::Panel *parent, const char *pszText, const char *pszCmd, int order )
	: Button( parent, "", pszText, parent, pszCmd )
{
	//Set3DComponentState( true ); // TODO:

	m_pTextBlur = new CVGUIMenuLabel(this, pszText );

	SetupVGUITex( "vgui/menu/menu_line", m_iBgTexture );
	m_iOrderIndex = order;

	m_flBlurStrengthAmount = 0;
	m_flBackgroundAlpha = 0;

	//SetDepressedSound( "ui/buttonclick.wav" );
	SetReleasedSound( "ui/buttonclickrelease.wav" );
	SetArmedSound( "ui/buttonrollover.wav" );
}

CVGUIMenuButton::~CVGUIMenuButton()
{
}

int CVGUIMenuButton::GetOrderIndex()
{
	return m_iOrderIndex;
}

void CVGUIMenuButton::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	//Color grey_dark( 96,96,96,255 );
	//Color grey_light( 200,200,200,255 );
	//Color white( 255,255,255,255 );
	Color blank(0,0,0,0);

	Color grey_dark( 96,47,6,255 );
	Color grey_light( 200,180,100,255 );
	Color white( 255,148,32,255 );

	SetDefaultColor( grey_light, blank );
	SetDepressedColor( grey_dark, blank );
	SetArmedColor( white, blank );
	SetPaintBorderEnabled( false );

	SetFont( pScheme->GetFont( "Menu3DItem", true ) );
	m_pTextBlur->SetFont( pScheme->GetFont( "Menu3DItemBlur", true ) );

	SetContentAlignment( Label::a_center );
	m_pTextBlur->SetContentAlignment( Label::a_center );

	m_pTextBlur->SetMouseInputEnabled( false );
	m_pTextBlur->SetKeyBoardInputEnabled( false );
}

void CVGUIMenuButton::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, t;
	GetSize( w, t );
	m_pTextBlur->SetBounds( 0, 0, w, t );
}

void CVGUIMenuButton::OnThink()
{
	float flBlurGoal = IsArmed() ? 1 : 0;
	float flBGAlphaGoal = IsArmed() ? 1 : 0;
	float flBGAlphaSpeed = IsArmed() ? 4.0f : 2.0f;

	if ( abs(m_flBlurStrengthAmount-flBlurGoal) < 0.01f )
		m_flBlurStrengthAmount = flBlurGoal;
	else
		m_flBlurStrengthAmount += (flBlurGoal - m_flBlurStrengthAmount) *
		min( 1.0f, CFrameTimeHelper::GetFrameTime() );

	if ( abs(m_flBackgroundAlpha-flBGAlphaGoal) < 0.01f )
		m_flBackgroundAlpha = flBGAlphaGoal;
	else
		m_flBackgroundAlpha += (flBGAlphaGoal - m_flBackgroundAlpha) *
		min( 1.0f, CFrameTimeHelper::GetFrameTime() * flBGAlphaSpeed );

	m_pTextBlur->SetAlpha( m_flBlurStrengthAmount * 255 );
}

void CVGUIMenuButton::Paint()
{
	int w, t;
	GetSize( w, t );

	int brightness = 64;

	Paint3DAdvanceDepth();
	surface()->DrawSetColor( 1.0f * brightness, 0.8f * brightness, 0.5f * brightness, m_flBackgroundAlpha * 192 );
	surface()->DrawSetTexture( m_iBgTexture );
	surface()->DrawTexturedRect( 0, 0, w, t );

	Paint3DAdvanceDepth();
	BaseClass::Paint();
}