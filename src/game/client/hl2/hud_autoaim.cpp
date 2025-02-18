//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "c_basehlplayer.h"
#include "view_scene.h"
#include "engine/IEngineSound.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/Panel.h"
#include "vgui/ISurface.h"
#include "iviewrender.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar hud_draw_active_reticle("hud_draw_active_reticle", "0" );
ConVar hud_draw_fixed_reticle("hud_draw_fixed_reticle", "0", FCVAR_ARCHIVE );
ConVar hud_autoaim_scale_icon( "hud_autoaim_scale_icon", "0" );
ConVar hud_autoaim_method( "hud_autoaim_method", "1" );

ConVar hud_reticle_scale("hud_reticle_scale", "1.0" );
ConVar hud_reticle_minalpha( "hud_reticle_minalpha", "125" );
ConVar hud_reticle_maxalpha( "hud_reticle_maxalpha", "255" );
ConVar hud_alpha_speed("hud_reticle_alpha_speed", "700" );
ConVar hud_magnetism("hud_magnetism", "0.3" );

enum 
{
	AUTOAIM_METHOD_RETICLE = 1,
	AUTOAIM_METHOD_DRIFT,
};

using namespace vgui;

class CHUDAutoAim : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHUDAutoAim, vgui::Panel );
public:
	CHUDAutoAim( const char *pElementName );
	virtual ~CHUDAutoAim( void );

	void ApplySchemeSettings( IScheme *scheme );
	void Init( void );
	void VidInit( void );
	bool ShouldDraw( void );
	virtual void OnThink();
	virtual void Paint();

private:
	void ResetAlpha() { m_alpha = 0; }
	void ResetScale() { m_scale = 1.0f; }
	
	void ResetPosition()
	{
		m_vecPos.x = ScreenWidth() / 2;
		m_vecPos.y = ScreenHeight() / 2;
		m_vecPos.z = 0;
	}

	Vector	m_vecPos;
	float	m_alpha;
	float	m_scale;

	float	m_alphaFixed; // alpha value for the fixed element.

	int		m_textureID_ActiveReticle;
	int		m_textureID_FixedReticle;
};

DECLARE_HUDELEMENT( CHUDAutoAim );

CHUDAutoAim::CHUDAutoAim( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HUDAutoAim" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetHiddenBits( HIDEHUD_CROSSHAIR );

	m_textureID_ActiveReticle = -1;
	m_textureID_FixedReticle = -1;
}

CHUDAutoAim::~CHUDAutoAim( void )
{
	if ( vgui::surface() )
	{
		if ( m_textureID_ActiveReticle != -1 )
		{
			vgui::surface()->DestroyTextureID( m_textureID_ActiveReticle );
			m_textureID_ActiveReticle = -1;
		}

		if ( m_textureID_FixedReticle != -1 )
		{
			vgui::surface()->DestroyTextureID( m_textureID_FixedReticle );
			m_textureID_FixedReticle = -1;
		}
	}
}


void CHUDAutoAim::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );
}

void CHUDAutoAim::Init( void )
{
	ResetPosition();
	ResetAlpha();
	ResetScale();
}

void CHUDAutoAim::VidInit( void )
{
	SetAlpha( 255 );
	Init();

	if ( m_textureID_ActiveReticle == -1 )
	{
		m_textureID_ActiveReticle = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_textureID_ActiveReticle, "vgui/hud/autoaim", true, false );
	}

	if ( m_textureID_FixedReticle == -1 )
	{
		m_textureID_FixedReticle = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_textureID_FixedReticle, "vgui/hud/xbox_reticle", true, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHUDAutoAim::ShouldDraw( void )
{	
#ifndef HL1_CLIENT_DLL
	C_BaseHLPlayer *pLocalPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		if( !pLocalPlayer->m_HL2Local.m_bDisplayReticle )
		{
			return false;
		}
	}
#endif

	return ( (hud_draw_fixed_reticle.GetBool() || hud_draw_active_reticle.GetBool()) && CHudElement::ShouldDraw() && !engine->IsDrawingLoadingImage() );
}

#define AUTOAIM_ALPHA_UP_SPEED		1000
#define AUTOAIM_ALPHA_DOWN_SPEED	300
#define AUTOAIM_MAX_ALPHA			120
#define AUTOAIM_MAX_SCALE			1.0f
#define AUTOAIM_MIN_SCALE			0.5f
#define AUTOAIM_SCALE_SPEED			10.0f		
#define AUTOAIM_ONTARGET_CROSSHAIR_SPEED		(ScreenWidth() / 3) // Can cross the whole screen in 3 seconds.
#define AUTOAIM_OFFTARGET_CROSSHAIR_SPEED		(ScreenWidth() / 4)

void CHUDAutoAim::OnThink()
{
	int wide, tall;
	GetSize( wide, tall );

	BaseClass::OnThink();

	// Get the HL2 player
	C_BaseHLPlayer *pLocalPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer == NULL )
	{
		// Just turn the autoaim crosshair off.
		ResetPosition();
		ResetAlpha();
		ResetScale();

		m_alphaFixed = 0.0f;
		return;
	}

	// Get the autoaim target.
	CBaseEntity *pTarget = pLocalPlayer->m_HL2Local.m_hAutoAimTarget.Get();

	// Fixed element stuff
	float flFixedAlphaGoal;

	if( pTarget )
	{
		flFixedAlphaGoal = hud_reticle_maxalpha.GetFloat();
	}
	else
	{
		flFixedAlphaGoal = hud_reticle_minalpha.GetFloat();
	}

	if( pLocalPlayer->m_HL2Local.m_bZooming || pLocalPlayer->m_HL2Local.m_bWeaponLowered )
	{
		flFixedAlphaGoal = 0.0f;
	}

	m_alphaFixed = Approach( flFixedAlphaGoal, m_alphaFixed, (hud_alpha_speed.GetFloat() * gpGlobals->frametime) );
	

	switch( hud_autoaim_method.GetInt() )
	{
	case AUTOAIM_METHOD_RETICLE:
		{
			if( pLocalPlayer->m_HL2Local.m_hAutoAimTarget.Get() && pLocalPlayer->m_HL2Local.m_bStickyAutoAim )
			{
				if( !pLocalPlayer->IsInAVehicle() )
				{
					Vector vecLook;
					pLocalPlayer->EyeVectors( &vecLook, NULL, NULL );

					Vector vecMove = pLocalPlayer->GetAbsVelocity();
					float flSpeed = VectorNormalize( vecMove );
					float flDot = DotProduct( vecLook, vecMove );

					if( flSpeed >= 100 && fabs(flDot) <= 0.707f )
					{
						QAngle viewangles;
						QAngle targetangles;
						QAngle delta;

						engine->GetViewAngles( viewangles );

						Vector vecDir = pLocalPlayer->m_HL2Local.m_vecAutoAimPoint - pLocalPlayer->EyePosition();
						VectorNormalize(vecDir);
						VectorAngles( vecDir, targetangles );

						float magnetism = hud_magnetism.GetFloat();

						delta[0] = ApproachAngle( targetangles[0], viewangles[0], magnetism );
						delta[1] = ApproachAngle( targetangles[1], viewangles[1], magnetism );
						delta[2] = targetangles[2];

						//viewangles[PITCH] = clamp( viewangles[ PITCH ], -cl_pitchup.GetFloat(), cl_pitchdown.GetFloat() );
						engine->SetViewAngles( delta );
					}
				}
			}

#if 0
			bool doScaling = hud_autoaim_scale_icon.GetBool();

			// These are the X & Y coords of where the crosshair should be. Default to 
			// returning to the center of the screen if there is no target.
			int goalx = ScreenWidth() / 2;
			int goaly = ScreenHeight() / 2;
			int goalalpha = 0;
			float goalscale = AUTOAIM_MIN_SCALE;
			float speed = AUTOAIM_OFFTARGET_CROSSHAIR_SPEED;

			if( pTarget )
			{
				// Get the autoaim crosshair onto the target.
				Vector screen;

				// Center the crosshair on the entity.
				if( doScaling )
				{
					// Put the crosshair over the center of the target.
					ScreenTransform( pTarget->WorldSpaceCenter(), screen );
				}
				else
				{
					// Put the crosshair exactly where the player is aiming.
					ScreenTransform( pLocalPlayer->m_HL2Local.m_vecAutoAimPoint, screen );
				}

				// Set Goal Position and speed.
				goalx += 0.5f * screen[0] * ScreenWidth() + 0.5f;
				goaly -= 0.5f * screen[1] * ScreenHeight() + 0.5f;
				speed = AUTOAIM_ONTARGET_CROSSHAIR_SPEED;

				goalalpha = AUTOAIM_MAX_ALPHA;

				if( doScaling )
				{
					// Scale the crosshair to envelope the entity's bounds on screen.
					Vector vecMins, vecMaxs;
					Vector vecScreenMins, vecScreenMaxs;

					// Get mins and maxs in world space
					vecMins = pTarget->GetAbsOrigin() + pTarget->WorldAlignMins();
					vecMaxs = pTarget->GetAbsOrigin() + pTarget->WorldAlignMaxs();

					// Project them to screen
					ScreenTransform( vecMins, vecScreenMins );
					ScreenTransform( vecMaxs, vecScreenMaxs );

					vecScreenMins.y = (ScreenWidth()/2) - 0.5f * vecScreenMins.y * ScreenWidth() + 0.5f;
					vecScreenMaxs.y = (ScreenWidth()/2) - 0.5f * vecScreenMaxs.y * ScreenWidth() + 0.5f;

					float screenSize = vecScreenMins.y - vecScreenMaxs.y;

					// Set goal scale
					goalscale = screenSize / 64.0f; // 64 is the width of the crosshair art.
				}
				else
				{
					goalscale = 1.0f;
				}
			}

			// Now approach the goal, alpha, and scale
			Vector vecGoal( goalx, goaly, 0 );
			Vector vecDir = vecGoal - m_vecPos;
			float flDistRemaining = VectorNormalize( vecDir );
			m_vecPos += vecDir * min(flDistRemaining, (speed * gpGlobals->frametime) );

			// Lerp and Clamp scale
			float scaleDelta = fabs( goalscale - m_scale );
			float scaleMove = MIN( AUTOAIM_SCALE_SPEED * gpGlobals->frametime, scaleDelta );
			if( m_scale < goalscale )
			{
				m_scale += scaleMove;
			}
			else if( m_scale > goalscale )
			{
				m_scale -= scaleMove;
			}
			if( m_scale > AUTOAIM_MAX_SCALE )
			{
				m_scale = AUTOAIM_MAX_SCALE;
			}
			else if( m_scale < AUTOAIM_MIN_SCALE )
			{
				m_scale = AUTOAIM_MIN_SCALE;
			}

			if( goalalpha > m_alpha )
			{
				m_alpha += AUTOAIM_ALPHA_UP_SPEED * gpGlobals->frametime;
			}
			else if( goalalpha < m_alpha )
			{
				m_alpha -= AUTOAIM_ALPHA_DOWN_SPEED * gpGlobals->frametime;
			}

			// Clamp alpha
			if( m_alpha < 0 )
			{
				m_alpha = 0;
			}
			else if( m_alpha > AUTOAIM_MAX_ALPHA )
			{
				m_alpha = AUTOAIM_MAX_ALPHA;
			}
#endif
		}
		break;

	case AUTOAIM_METHOD_DRIFT:
		{
			if( pLocalPlayer->m_HL2Local.m_hAutoAimTarget.Get() )
			{
				QAngle viewangles;

				engine->GetViewAngles( viewangles );
				
				Vector vecDir = pLocalPlayer->m_HL2Local.m_vecAutoAimPoint - pLocalPlayer->EyePosition();
				VectorNormalize(vecDir);

				VectorAngles( vecDir, viewangles );

				//viewangles[PITCH] = clamp( viewangles[ PITCH ], -cl_pitchup.GetFloat(), cl_pitchdown.GetFloat() );
				engine->SetViewAngles( viewangles );
			}
		}
		break;
	}
}

void CHUDAutoAim::Paint()
{
	if( hud_draw_active_reticle.GetBool() )
	{
		int xCenter = m_vecPos.x;
		int yCenter = m_vecPos.y;

		int width, height;
		float xMod, yMod;

		vgui::surface()->DrawSetTexture( m_textureID_ActiveReticle );
		vgui::surface()->DrawSetColor( 255, 255, 255, m_alpha );
		vgui::surface()->DrawGetTextureSize( m_textureID_ActiveReticle, width, height );

		float uv1 = 0.5f / width, uv2 = 1.0f - uv1;

		vgui::Vertex_t vert[4];	

		Vector2D uv11( uv1, uv1 );
		Vector2D uv12( uv1, uv2 );
		Vector2D uv21( uv2, uv1 );
		Vector2D uv22( uv2, uv2 );

		xMod = width;
		yMod = height;

		xMod *= m_scale;
		yMod *= m_scale;

		xMod /= 2;
		yMod /= 2;

		vert[0].Init( Vector2D( xCenter + xMod, yCenter + yMod ), uv21 );
		vert[1].Init( Vector2D( xCenter - xMod, yCenter + yMod ), uv11 );
		vert[2].Init( Vector2D( xCenter - xMod, yCenter - yMod ), uv12 );
		vert[3].Init( Vector2D( xCenter + xMod, yCenter - yMod ), uv22 );
		vgui::surface()->DrawTexturedPolygon( 4, vert );
	}

	if( hud_draw_fixed_reticle.GetBool() )
	{
		int width, height;
		float xMod, yMod;

		vgui::surface()->DrawSetTexture( m_textureID_FixedReticle );
		vgui::surface()->DrawGetTextureSize( m_textureID_FixedReticle, width, height );

		int xCenter = ScreenWidth() / 2;
		int yCenter = ScreenHeight() / 2;

		vgui::Vertex_t vert[4];	

		Vector2D uv11( 0, 0 );
		Vector2D uv12( 0, 1 );
		Vector2D uv21( 1, 0 );
		Vector2D uv22( 1, 1 );

		xMod = width;
		yMod = height;

		xMod /= 2;
		yMod /= 2;

		vert[0].Init( Vector2D( xCenter + xMod, yCenter + yMod ), uv21 );
		vert[1].Init( Vector2D( xCenter - xMod, yCenter + yMod ), uv11 );
		vert[2].Init( Vector2D( xCenter - xMod, yCenter - yMod ), uv12 );
		vert[3].Init( Vector2D( xCenter + xMod, yCenter - yMod ), uv22 );

		Color	clr;
		clr = gHUD.m_clrNormal;
		int r,g,b,a;
		clr.GetColor( r,g,b,a );

		C_BaseHLPlayer *pLocalPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
		if( pLocalPlayer && pLocalPlayer->m_HL2Local.m_hAutoAimTarget.Get() )
		{
			r = 250; 
			g = 138;
			b = 4;
		}

		clr.SetColor( r,g,b,m_alphaFixed);

		vgui::surface()->DrawSetColor( clr );
		vgui::surface()->DrawTexturedPolygon( 4, vert );
	}
}