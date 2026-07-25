//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "c_tf_player.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "client_virtualreality.h"
#include "sourcevr/isourcevirtualreality.h"
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>

//for screenfade
#include "ivieweffects.h"
#include "shake.h"
#include "view_scene.h"

#include "tf_weapon_sniperrifle.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Purpose: Figure out where the sniper scope should be drawn.
//-----------------------------------------------------------------------------
void WhereToDrawSniperScope ( int *pX, int *pY, int screenWide, int screenTall )
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer != NULL )
	{
		// These are the correct values to use, but they lag the high-speed view data...
		Vector vecStart = pPlayer->Weapon_ShootPosition();
		Vector vecAimDirection = pPlayer->GetAutoaimVector( 1.0f );
		// ...so in some aim modes, they get zapped by something completely up-to-date.
		g_ClientVirtualReality.OverrideWeaponHudAimVectors ( &vecStart, &vecAimDirection );

		Vector vAimPoint;
		// Here we just put the aim point a set distance from the viewer - same depth as the HUD.
		float fVrHudDistance = g_ClientVirtualReality.GetHUDDistance();
		vAimPoint = vecStart + vecAimDirection * fVrHudDistance;

		Vector screen;
		screen.Init();
		ScreenTransform(vAimPoint, screen);

		// screen[0][1] are in range (-1, 1)
		float x = 0.5 * ( 1.0f + screen[0] ) * screenWide + 0.5;
		float y = 0.5 * ( 1.0f - screen[1] ) * screenTall + 0.5;

		*pX = (int)( x + 0.5f );
		*pY = (int)( y + 0.5f );
	}
}



//-----------------------------------------------------------------------------
// Purpose: Draws the sniper chargeup meter
//-----------------------------------------------------------------------------
class CHudScopeCharge : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudScopeCharge, vgui::Panel );

public:
	CHudScopeCharge( const char *pElementName );
	virtual ~CHudScopeCharge( void );

	void	Init( void );

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void Paint( void );

private:
	int m_iChargeupTexture;
	int m_iChargeupTextureWidth;
	CPanelAnimationVarAliasType( float, m_iChargeup_xpos, "chargeup_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_iChargeup_ypos, "chargeup_ypos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_iChargeup_wide, "chargeup_wide", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_iChargeup_tall, "chargeup_tall", "0", "proportional_float" );

	bool m_bJarateMode;
};

DECLARE_HUDELEMENT_DEPTH( CHudScopeCharge, 100 );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudScopeCharge::CHudScopeCharge( const char *pElementName ) : CHudElement(pElementName), BaseClass(NULL, "HudScopeCharge")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_PLAYERDEAD );

	m_bJarateMode = false;

	m_iChargeupTexture = -1;
}

CHudScopeCharge::~CHudScopeCharge( void )
{
	if ( vgui::surface() && m_iChargeupTexture != -1 )
	{
		vgui::surface()->DestroyTextureID( m_iChargeupTexture );
		m_iChargeupTexture = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: standard hud element init function
//-----------------------------------------------------------------------------
void CHudScopeCharge::Init( void )
{
	if ( m_iChargeupTexture == -1 )
	{
		m_iChargeupTexture = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_iChargeupTexture, "HUD/sniperscope_numbers", true, false);
	}

	// Get the texture size
	int ignored;
	surface()->DrawGetTextureSize( m_iChargeupTexture, m_iChargeupTextureWidth, ignored );
}

//-----------------------------------------------------------------------------
// Purpose: sets scheme colors
//-----------------------------------------------------------------------------
void CHudScopeCharge::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	if ( UseVR() )
	{
		// Force it to go direct to the framebuffer.
		SetForceStereoRenderToFrameBuffer( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: draws the zoom effect
//-----------------------------------------------------------------------------
void CHudScopeCharge::Paint( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( GetSpectatorTarget() != 0 && GetSpectatorMode() == OBS_MODE_IN_EYE )
	{
		pPlayer = (C_TFPlayer *)UTIL_PlayerByIndex( GetSpectatorTarget() );
	}

	if ( !pPlayer )
		return;

	if ( !pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
		return;

	// Make sure the current weapon is a sniper rifle
	CTFSniperRifle *pWeapon = assert_cast<CTFSniperRifle*>(pPlayer->GetActiveTFWeapon());
	if ( !pWeapon )
		return;

	if ( pWeapon->IsJarateRifle() && !m_bJarateMode )
	{
		vgui::surface()->DrawSetTextureFile(m_iChargeupTexture, "HUD/sniperscope_numbers_jar", true, false);
		m_bJarateMode = true;
	}
	else if ( !pWeapon->IsJarateRifle() && m_bJarateMode )
	{
		vgui::surface()->DrawSetTextureFile(m_iChargeupTexture, "HUD/sniperscope_numbers", true, false);
		m_bJarateMode = false;
	}

	// Actual charge value is set through a material proxy in the sniper rifle class

	int wide, tall;
	GetSize( wide, tall );

	int x = 0;
	int y = 0;
	bool bDisableClipping = false;
	if ( UseVR() )
	{
		int vx, vy, vw, vh;
		vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );

		int screenWide = vw;
		int screenTall = vh;
		bDisableClipping = true;
		WhereToDrawSniperScope ( &x, &y, screenWide, screenTall );

		// The origin is wherever the property file put it, so (0,0) means "the right place, if the scope is in the middle of the screen"
		// So offset from there. But additional fun - the rendering coordinates are in the UI space, not the actual screen space.
		int UiScreenWide, UiScreenTall;
		GetHudSize(UiScreenWide, UiScreenTall);
		x -= ( UiScreenWide / 2 );
		y -= ( UiScreenTall / 2 );
	}

	if( bDisableClipping )
		g_pMatSystemSurface->DisableClipping( true );
	vgui::surface()->DrawSetColor(255,255,255,255);
	vgui::surface()->DrawSetTexture(m_iChargeupTexture);
	vgui::surface()->DrawTexturedRect( x, y, x+wide, y+tall );
	if( bDisableClipping )
		g_pMatSystemSurface->DisableClipping( false );
}

//-----------------------------------------------------------------------------
// Purpose: Draws the zoom screen
//-----------------------------------------------------------------------------
class CHudScope : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudScope, vgui::Panel );

public:
	CHudScope( const char *pElementName );
	virtual ~CHudScope( void );
	
	void	Init( void );

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void Paint( void );
	virtual bool ShouldDraw( void );
	virtual bool CanAnimate() const OVERRIDE { return false; }

private:
	int m_iScopeTexture[4];
	int m_iScopeTextureAlt[4];
	bool m_bAltScopeMode;
};

DECLARE_HUDELEMENT_DEPTH( CHudScope, 100 );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudScope::CHudScope( const char *pElementName ) : CHudElement(pElementName), BaseClass(NULL, "HudScope")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	
	SetHiddenBits( HIDEHUD_PLAYERDEAD );

	for ( int i = 0; i < ARRAYSIZE( m_iScopeTexture ); i++ )
	{
		m_iScopeTexture[ i ] = -1;
	}

	for ( int i = 0; i < ARRAYSIZE( m_iScopeTextureAlt ); i++ )
	{
		m_iScopeTextureAlt[i] = -1;
	}

	m_bAltScopeMode = false;
}

CHudScope::~CHudScope( void )
{
	if ( vgui::surface() )
	{
		for ( int i = 0; i < ARRAYSIZE( m_iScopeTexture ); i++ )
		{
			if ( m_iScopeTexture[ i ] != -1 )
			{
				vgui::surface()->DestroyTextureID( m_iScopeTexture[ i ] );
				m_iScopeTexture[ i ] = -1;
			}
		}

		for ( int i = 0; i < ARRAYSIZE( m_iScopeTextureAlt ); i++ )
		{
			if ( m_iScopeTextureAlt[i] != -1 )
			{
				vgui::surface()->DestroyTextureID( m_iScopeTextureAlt[i] );
				m_iScopeTextureAlt[i] = -1;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: standard hud element init function
//-----------------------------------------------------------------------------
void CHudScope::Init( void )
{
	for ( int i = 0; i < ARRAYSIZE( m_iScopeTexture ); i++ )
	{
		if ( m_iScopeTexture[ i ] == -1 )
		{
			m_iScopeTexture[ i ] = vgui::surface()->CreateNewTextureID();
		}
	}

	vgui::surface()->DrawSetTextureFile( m_iScopeTexture[0], "HUD/scope_sniper_ul", true, false );
	vgui::surface()->DrawSetTextureFile( m_iScopeTexture[1], "HUD/scope_sniper_ur", true, false );
	vgui::surface()->DrawSetTextureFile( m_iScopeTexture[2], "HUD/scope_sniper_lr", true, false );
	vgui::surface()->DrawSetTextureFile( m_iScopeTexture[3], "HUD/scope_sniper_ll", true, false );

	for ( int i = 0; i < ARRAYSIZE( m_iScopeTextureAlt ); i++ )
	{
		if ( m_iScopeTextureAlt[i] == -1 )
		{
			m_iScopeTextureAlt[i] = vgui::surface()->CreateNewTextureID();
		}
	}

	vgui::surface()->DrawSetTextureFile( m_iScopeTextureAlt[0], "HUD/scope_sniper_alt_ul", true, false );
	vgui::surface()->DrawSetTextureFile( m_iScopeTextureAlt[1], "HUD/scope_sniper_alt_ur", true, false );
	vgui::surface()->DrawSetTextureFile( m_iScopeTextureAlt[2], "HUD/scope_sniper_alt_lr", true, false );
	vgui::surface()->DrawSetTextureFile( m_iScopeTextureAlt[3], "HUD/scope_sniper_alt_ll", true, false );

	// remove ourselves from the global group so the scoreboard doesn't hide us
	UnregisterForRenderGroup( "global" );
}

//-----------------------------------------------------------------------------
// Purpose: sets scheme colors
//-----------------------------------------------------------------------------
void CHudScope::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	if ( UseVR() )
	{
		// Make it fill the screen.
		int iViewportWidth, iViewportHeight;
		g_pSourceVR->GetViewportBounds( ISourceVirtualReality::VREye_Left, NULL, NULL, &iViewportWidth, &iViewportHeight );
	    SetSize ( iViewportWidth, iViewportHeight );
		SetBounds ( 0, 0, iViewportWidth, iViewportHeight );
		// Force it to go direct to the framebuffer.
		SetForceStereoRenderToFrameBuffer( true );
	}
	else
	{
		int screenWide, screenTall;
		GetHudSize(screenWide, screenTall);
		SetBounds(0, 0, screenWide, screenTall);
	}

	// Move behind the spectator GUI, so we can be visible at the same time.
	SetZPos( -1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudScope::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( GetSpectatorTarget() != 0 && GetSpectatorMode() == OBS_MODE_IN_EYE )
	{
		pPlayer = (C_TFPlayer *)UTIL_PlayerByIndex( GetSpectatorTarget() );
	}

	if ( !pPlayer || !pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
		return false;

	if ( pPlayer->GetActiveTFWeapon() )
	{
		m_bAltScopeMode = ( pPlayer->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_SNIPERRIFLE_CLASSIC );
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: draws the zoom effect
//-----------------------------------------------------------------------------
void CHudScope::Paint( void )
{
	// We need to update the refraction texture so the scope can refract it
	UpdateRefractTexture();

	int screenWide;
	int screenTall;

	GetHudSize(screenWide, screenTall);

	// calculate the bounds in which we should draw the scope
	int xMid = screenWide / 2;
	int yMid = screenTall / 2;

	// width of the drawn scope. in widescreen, we draw the sides with primitives
	int wide, tall;
	if( screenWide > screenTall )
	{
		wide = ( screenTall * 4 ) / 3;
		tall = screenTall;
	}
	else
	{
		wide = screenWide;
		tall = ( screenWide * 3 ) / 4;
	}

	bool bDisableClipping = false;
	if ( UseVR() )
	{
		int vx, vy, vw, vh;
		vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );

		screenWide = vw;
		screenTall = vh;
		// This is actually awful - the scope is drawn in HUD-space, which is this completely
		// artifical 640*480 space that we invent for VR and which doesn't even have square pixels. Ugh.
		// Hacked good enough to ship. TODO: don't hack it.
		float fMagnification = 1.0f / g_ClientVirtualReality.GetZoomedModeMagnification();
		wide = (int)( fMagnification * (float)screenWide );
		tall = ( wide * 3 ) / 4;

		bDisableClipping = true;
		WhereToDrawSniperScope ( &xMid, &yMid, screenWide, screenTall );
	}

	int xLeft = xMid - wide/2;
	int xRight = xMid + wide/2;
	int yTop = yMid - tall/2;
	int yBottom = yMid + tall/2;

	float uv1 = 0.5f / 256.0f, uv2 = 1.0f - uv1;

	vgui::Vertex_t vert[4];	
	
	Vector2D uv11( uv1, uv1 );
	Vector2D uv12( uv1, uv2 );
	Vector2D uv21( uv2, uv1 );
	Vector2D uv22( uv2, uv2 );

	vgui::surface()->DrawSetColor(0,0,0,255);

	if( bDisableClipping )
		g_pMatSystemSurface->DisableClipping( true );

	//upper left
	vgui::surface()->DrawSetTexture( m_bAltScopeMode ? m_iScopeTextureAlt[0] : m_iScopeTexture[0] );
	vert[0].Init( Vector2D( xLeft, yTop ), uv11 );
	vert[1].Init( Vector2D( xMid,  yTop ), uv21 );
	vert[2].Init( Vector2D( xMid,  yMid ), uv22 );
	vert[3].Init( Vector2D( xLeft, yMid ), uv12 );
	vgui::surface()->DrawTexturedPolygon( 4, vert );

	// top right
	if ( m_bAltScopeMode )
	{
		vgui::surface()->DrawSetTexture( m_iScopeTextureAlt[1] );
		vert[0].Init( Vector2D( xMid, yTop ), uv11 );
		vert[1].Init( Vector2D( xRight, yTop ), uv21 );
		vert[2].Init( Vector2D( xRight, yMid ), uv22 );
		vert[3].Init( Vector2D( xMid, yMid ), uv12 );
		vgui::surface()->DrawTexturedPolygon( 4, vert );
	}
	else
	{
		vgui::surface()->DrawSetTexture( m_iScopeTexture[1] );
		vert[0].Init( Vector2D( xMid - 1, yTop ), uv11 );
		vert[1].Init( Vector2D( xRight,   yTop ), uv21 );
		vert[2].Init( Vector2D( xRight,   yMid + 1 ), uv22 );
		vert[3].Init( Vector2D( xMid - 1, yMid + 1 ), uv12 );
		vgui::surface()->DrawTexturedPolygon( 4, vert );
	}

	// bottom right
	vgui::surface()->DrawSetTexture( m_bAltScopeMode ? m_iScopeTextureAlt[2] : m_iScopeTexture[2] );
	vert[0].Init( Vector2D( xMid,   yMid ), uv11 );
	vert[1].Init( Vector2D( xRight, yMid ), uv21 );
	vert[2].Init( Vector2D( xRight, yBottom ), uv22 );
	vert[3].Init( Vector2D( xMid,   yBottom ), uv12 );
	vgui::surface()->DrawTexturedPolygon( 4, vert );

	// bottom left
	vgui::surface()->DrawSetTexture( m_bAltScopeMode ? m_iScopeTextureAlt[3] : m_iScopeTexture[3] );
	vert[0].Init( Vector2D( xLeft, yMid ), uv11 );
	vert[1].Init( Vector2D( xMid,  yMid ), uv21 );
	vert[2].Init( Vector2D( xMid,  yBottom ), uv22 );
	vert[3].Init( Vector2D( xLeft, yBottom), uv12 );
	vgui::surface()->DrawTexturedPolygon( 4, vert );

	if ( xLeft > 0 )
	{
		// Left block
		vgui::surface()->DrawFilledRect( 0, 0, xLeft, screenTall );
	}
	if ( screenWide > xRight )
	{
		// Right block
		vgui::surface()->DrawFilledRect( xRight, 0, screenWide, screenTall );
	}
	if ( yTop > 0 )
	{
		// top block
		vgui::surface()->DrawFilledRect( 0, 0, screenWide, yTop );
	}
	if ( screenTall > yBottom )
	{
		// bottom block
		vgui::surface()->DrawFilledRect( 0, yBottom, screenWide, screenTall );
	}

	if( bDisableClipping )
		g_pMatSystemSurface->DisableClipping( false );

}