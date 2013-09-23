
#include "cbase.h"
#include "hud_nightvision.h"
#include "Gstring/vgui/vUtil.h"
#include "vgui_controls/animationcontroller.h"
#include "iclientmode.h"

#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"

#include "../c_gstring_player.h"
#include "../gstring_postprocess.h"
#include "Gstring/gstring_cvars.h"


using namespace vgui;

DECLARE_HUDELEMENT( CHudNightvision );

CHudNightvision::CHudNightvision( const char *pElementName )
	: CHudElement( pElementName )
	, BaseClass( NULL, pElementName )
	, m_bWasPainting( false )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
}

CHudNightvision::~CHudNightvision()
{
}

void CHudNightvision::Init()
{
}

void CHudNightvision::PostDLLInit()
{
}

void CHudNightvision::LevelInit()
{
	Reset();
}

void CHudNightvision::Reset()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "nightvision_reset" );
	m_bWasPainting = false;
}

bool CHudNightvision::ShouldDraw()
{
	if ( !cvar_gstring_enable_hud.GetBool() )
		return false;

	return CHudElement::ShouldDraw();
}

void CHudNightvision::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	int w, t;
	GetHudSize( w, t );
	SetSize( w, t );
}

void CHudNightvision::OnThink()
{
	C_GstringPlayer *pPlayer = LocalGstringPlayer();
	const bool bIsPainting = pPlayer && pPlayer->IsNightvisionActive();

	if ( bIsPainting != m_bWasPainting )
	{
		m_bWasPainting = bIsPainting;

		const char *pszSequenceName = bIsPainting ? "nightvision_fadein" : "nightvision_fadeout";

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pszSequenceName );
	}

	SetNightvisionParams( m_flFade, m_flNightvisionAmount, m_flOverbright );
}