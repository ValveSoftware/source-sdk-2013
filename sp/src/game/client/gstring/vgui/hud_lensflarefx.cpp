
#include "cbase.h"
#include "Gstring/vgui/hud_lensflarefx.h"
#include "text_message.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include "vguimatsurface/IMatSystemSurface.h"
#include "Gstring/vgui/vUtil.h"
#include "view_scene.h"

#include "Gstring/gstring_postprocess.h"

#include "Gstring/gstring_cvars.h"
#include "Gstring/vgui/vLensflare.h"
#include "Gstring/vgui/vParticleContainer.h"

using namespace vgui;

DECLARE_HUDELEMENT( CHudLensflareEffects );

CHudLensflareEffects::CHudLensflareEffects( const char *pElementName ) : CHudElement( pElementName ), BaseClass(NULL, "HudLensflareFx")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pLensflareRoot = new Panel( this );
}

CHudLensflareEffects::~CHudLensflareEffects()
{
	Assert( m_hLensflares.Count() == 0 );
}

void CHudLensflareEffects::Init( void )
{
}

void CHudLensflareEffects::Reset( void )
{
}

void CHudLensflareEffects::PostDLLInit()
{
}

bool CHudLensflareEffects::ShouldDraw( void )
{
	if ( !cvar_gstring_enable_hud.GetBool() )
		return false;

	return CHudElement::ShouldDraw() && cvar_gstring_drawlensflare.GetBool();
}

void CHudLensflareEffects::OnThink()
{
	m_pLensflareRoot->SetAlpha( 255.0f * GetSceneFadeScalar() );
}

void CHudLensflareEffects::OnSizeChanged(int newWide, int newTall)
{
	BaseClass::OnSizeChanged( newWide, newTall );

	m_pLensflareRoot->SetBounds( 0, 0, newWide, newTall );
}

void CHudLensflareEffects::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);

	int wide, tall;
	GetHudSize(wide, tall);
	SetSize(wide, tall);

	for ( int i = 0; i < m_hLensflares.Count(); i++ )
		m_hLensflares[i]->ResizePanel();
}

vLensflare_Collection *CHudLensflareEffects::LoadLensflare( CGlowOverlay *pSrc, KeyValues *pData )
{
#if _DEBUG
	for ( int i = 0; i < m_hLensflares.Count(); i++ )
		Assert( m_hLensflares[i]->GetSource() != pSrc );
#endif

	vLensflare_Collection *pRet = vLensflare_Collection::InitFromScript( m_pLensflareRoot, pSrc, pData );

	Assert( pRet != NULL );

	m_hLensflares.AddToTail( pRet );

	return pRet;
}

void CHudLensflareEffects::FreeLensflare( vLensflare_Collection **flare )
{
	for ( int i = 0; i < m_hLensflares.Count(); i++ )
	{
		if ( m_hLensflares[i] != (*flare) )
			continue;

		m_hLensflares.Remove( i );

		(*flare)->Destroy();
		*flare = NULL;
		return;
	}

	Assert( 0 );
}