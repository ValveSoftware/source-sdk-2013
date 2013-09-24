
#include "cbase.h"
#include "Gstring/vgui/hud_waterfx.h"
#include "text_message.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include "vguimatsurface/IMatSystemSurface.h"
#include "Gstring/vgui/vUtil.h"
#include "Gstring/gstring_cvars.h"
#include "view_scene.h"
#include "Gstring/cFrametimeHelper.h"

using namespace vgui;

DECLARE_HUDELEMENT( CHudWaterEffects );

ConVar gstring_vgui_rain_size( "gstring_vgui_rain_size", "4.0" ); // Nicolas: 1.2 or 2.5 looks more realistic. //
ConVar gstring_vgui_rain_length( "gstring_vgui_rain_length", "0.3" ); // Nicolas: 0.5 or 1.5 looks more realistic. //

CHudWaterEffects::CHudWaterEffects( const char *pElementName ) : CHudElement( pElementName ), BaseClass(NULL, "HudWaterFx")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_flRainOverlay_Alpha = 0.0f;

	m_pParticleParent_Emerge = new CVGUIParticleContainer( this );
	m_pParticleParent_Drops = new CVGUIParticleContainer( this );

	m_pParticleParent_Emerge->AddGlobalOperator( new vParticleOperator_Movement() );
	m_pParticleParent_Emerge->AddGlobalOperator( new vParticleOperator_Gravity( Vector2D( 0, 600.0f ) ) );

	vParticleOperatorBase *fadeAlpha = new vParticleOperator_AlphaFade(0);
	fadeAlpha->GetImpulseGenerator()->impulse_bias = 0.05f;

	m_pParticleParent_Drops->AddGlobalOperator( new vParticleOperator_Movement() );
	m_pParticleParent_Drops->AddGlobalOperator( fadeAlpha );
	m_pParticleParent_Drops->AddGlobalOperator( new vParticleOperator_GravityWorld( 2200.0f ) ); // 2
	m_pParticleParent_Drops->AddGlobalOperator( new vParticleOperator_Movement_SpeedClamp( 3000.0f ) ); // 15
	
	Reset();
}

void CHudWaterEffects::Init( void )
{
}

void CHudWaterEffects::PostDLLInit()
{
	SetupVGUITex( "effects/waterblur", m_iTexture_WaterOverlay );
	SetupVGUITex( "effects/water_wet", m_iTexture_RainOverlay );
}

void CHudWaterEffects::Reset( void )
{
	FlushAllParticles();

	m_flRainRegisterTimer = 0;
	m_flDropSpawnTimer = 0;
	m_flDropAmountMultiplier = 1.0f;
	m_bSubmerged_Last = false;
	m_flRainOverlay_Alpha = 0;
}

void CHudWaterEffects::FlushAllParticles()
{
	m_pParticleParent_Emerge->FlushParticles();
	m_pParticleParent_Drops->FlushParticles();
}

bool CHudWaterEffects::ShouldDraw( void )
{
	if ( !cvar_gstring_enable_hud.GetBool() )
		return false;

	return CHudElement::ShouldDraw();
}

void CHudWaterEffects::OnThink()
{
	C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
	if ( !pLocal )
		return;

	bool m_bSubmerged_Cur = enginetrace->GetPointContents( pLocal->EyePosition() ) & MASK_WATER;
	if ( m_bSubmerged_Cur != m_bSubmerged_Last )
	{
		if ( m_bSubmerged_Cur )
			FlushAllParticles();
		else
			PlayerEffect_Emerge();
	}

	m_bSubmerged_Last = m_bSubmerged_Cur;

	if ( m_flRainRegisterTimer > CFrameTimeHelper::GetCurrentTime() &&
		m_flDropSpawnTimer < CFrameTimeHelper::GetCurrentTime() &&
		!m_bSubmerged_Last )
	{
		float dot_world_z = DotProduct( MainViewForward(), Vector( 0, 0, 1 ) );
		dot_world_z = RemapValClamped( dot_world_z, -0.4f, 1, 0, 1 );

		float flWaitingTime = 0.5f - 0.4f * dot_world_z;
		m_flDropSpawnTimer = CFrameTimeHelper::GetCurrentTime() + flWaitingTime;

		if ( dot_world_z > 0 )
		{
			int numToSpawn = RemapValClamped( dot_world_z, -0.707f, 1, 1, 6 );

			for ( int i = 0; i < numToSpawn; i++ )
				PlayerEffect_WaterDrop();
		}
	}

	float rainTargetAlpha = (m_flRainRegisterTimer > CFrameTimeHelper::GetCurrentTime()) ? 1.0f : 0.0f;
	m_flRainOverlay_Alpha = Approach( rainTargetAlpha, m_flRainOverlay_Alpha, CFrameTimeHelper::GetFrameTime() * 1.0f );
}

void CHudWaterEffects::Paint()
{
	if ( m_bSubmerged_Last ||
		m_pParticleParent_Emerge->GetNumParticles() > 0 ||
		m_pParticleParent_Drops->GetNumParticles() > 0 )
		UpdateScreenEffectTexture();

	if ( !cvar_gstring_drawwatereffects.GetBool() )
		return;

	int wide, tall;
	GetSize(wide, tall);

	if ( m_bSubmerged_Last )
	{
		surface()->DrawSetColor( 255, 255, 255, 255 );
		surface()->DrawSetTexture( m_iTexture_WaterOverlay );
		surface()->DrawTexturedRect( 0, 0, wide, tall );
	}
	else if ( m_flRainOverlay_Alpha > 0 )
	{
		surface()->DrawSetColor( 255, 255, 255, m_flRainOverlay_Alpha * 255 );
		surface()->DrawSetTexture( m_iTexture_RainOverlay );
		surface()->DrawTexturedRect( 0, 0, wide, tall );
	}
}

void CHudWaterEffects::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);

	int wide, tall;
	GetHudSize(wide, tall);
	SetSize(wide, tall);

	m_pParticleParent_Emerge->SetSize( wide, tall );
	m_pParticleParent_Drops->SetSize( wide, tall );
}

void CHudWaterEffects::OnRainHit()
{
	m_flRainRegisterTimer = CFrameTimeHelper::GetCurrentTime() + 1.0f;
}

void CHudWaterEffects::SetDropMultiplier( float m )
{
	m_flDropAmountMultiplier = m;
}

void CHudWaterEffects::PlayerEffect_Emerge()
{
	if ( !cvar_gstring_drawwatereffects.GetBool() )
		return;

	int w, t;
	GetSize( w, t );

	vParticle *p = new vParticle();
	p->SetStartSize_Absolute( w );
	p->vecPos.Init( w/2, t - (w * 1.0f) );
	p->CreateRectRenderer( "effects/water_emerge", 1.0f, 2.0f );
	p->SetLifeDuration( 5.0f );
	p->SetSafeVelocity( Vector2D( 0, 100.0f ) );

	m_pParticleParent_Emerge->AddParticle( p );
}

static const char *g_pszDropMaterials[] = {
	"effects/water_drop_0",
	"effects/water_drop_1",
	"effects/water_drop_2",
};
static const int g_iNumDropMaterials = ARRAYSIZE( g_pszDropMaterials );

void CHudWaterEffects::PlayerEffect_WaterDrop()
{
	if ( !cvar_gstring_drawwatereffects.GetBool() )
		return;

	int w, t;
	GetSize( w, t );

	vParticle *p = new vParticle();
	p->SetStartSize_Relative( RandomFloat( 5.0f, 15.0f ) * gstring_vgui_rain_size.GetFloat() );
	p->vecPos.Init( RandomFloat(0,1) * w, RandomFloat(0,1) * t );
	p->SetLifeDuration( RandomFloat( 4.0f, 6.0f ) * gstring_vgui_rain_length.GetFloat() );
	p->SetRenderer(
		new vParticleRenderer_Trail( g_pszDropMaterials[RandomInt(0,g_iNumDropMaterials-1)],
		RandomInt( 5, 10 ) )
		);
	p->AddParticleOperator( new vParticleOperator_RainSimulation() );
	p->AddParticleOperator( new vParticleOperator_WorldRotationForce( 400.0f ) );

	m_pParticleParent_Drops->AddParticle( p );
}
