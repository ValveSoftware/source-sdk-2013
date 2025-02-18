//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Hud element that indicates the direction of damage taken by the player
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "text_message.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "IEffects.h"
#include "hudelement.h"
#include "clienteffectprecachesystem.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudDamageIndicator : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudDamageIndicator, vgui::Panel );
public:
	CHudDamageIndicator( const char *pElementName );
	void Init( void );
	void VidInit( void );
	void Reset( void );
	virtual bool ShouldDraw( void );

	// Handler for our message
	void MsgFunc_Damage( bf_read &msg );

private:
	virtual void OnThink();
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	// Painting
	void GetDamagePosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation );
	void DrawDamageIndicator(int x0, int y0, int x1, int y1, float alpha, float flRotation );

private:
	// Indication times

	CPanelAnimationVarAliasType( float, m_flMinimumWidth, "MinimumWidth", "10", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flMaximumWidth, "MaximumWidth", "100", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flMinimumHeight, "MinimumHeight", "20", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flMaximumHeight, "MaximumHeight", "100", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flStartRadius, "StartRadius", "140", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flEndRadius, "EndRadius", "120", "proportional_float" );

	CPanelAnimationVar( float, m_iMaximumDamage, "MaximumDamage", "100" );
	CPanelAnimationVar( float, m_flMinimumTime, "MinimumTime", "1" );
	CPanelAnimationVar( float, m_flMaximumTime, "MaximumTime", "2" );
	CPanelAnimationVar( float, m_flTravelTime, "TravelTime", ".1" );
	CPanelAnimationVar( float, m_flFadeOutPercentage, "FadeOutPercentage", "0.7" );
	CPanelAnimationVar( float, m_flNoise, "Noise", "0.1" );

	// List of damages we've taken
	struct damage_t
	{
		int		iScale;
		float	flLifeTime;
		float	flStartTime;
		Vector	vecDelta;	// Damage origin relative to the player
	};
	CUtlVector<damage_t>	m_vecDamages;

	CMaterialReference m_WhiteAdditiveMaterial;
};

DECLARE_HUDELEMENT( CHudDamageIndicator );
DECLARE_HUD_MESSAGE( CHudDamageIndicator, Damage );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudDamageIndicator::CHudDamageIndicator( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass(NULL, "HudDamageIndicator")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH );

	m_WhiteAdditiveMaterial.Init( "VGUI/damageindicator", TEXTURE_GROUP_VGUI ); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDamageIndicator::Init( void )
{
	HOOK_HUD_MESSAGE( CHudDamageIndicator, Damage );
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDamageIndicator::Reset( void )
{
	m_vecDamages.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDamageIndicator::VidInit( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDamageIndicator::OnThink()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudDamageIndicator::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
		return false;

	// Don't draw if we don't have any damage to indicate
	if ( !m_vecDamages.Count() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Convert a damage position in world units to the screen's units
//-----------------------------------------------------------------------------
void CHudDamageIndicator::GetDamagePosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation )
{
	// Player Data
	Vector playerPosition = MainViewOrigin();
	QAngle playerAngles = MainViewAngles();

	Vector forward, right, up(0,0,1);
	AngleVectors (playerAngles, &forward, NULL, NULL );
	forward.z = 0;
	VectorNormalize(forward);
	CrossProduct( up, forward, right );
	float front = DotProduct(vecDelta, forward);
	float side = DotProduct(vecDelta, right);
	*xpos = flRadius * -side;
	*ypos = flRadius * -front;

	// Get the rotation (yaw)
	*flRotation = atan2(*xpos,*ypos) + M_PI;
	*flRotation *= 180 / M_PI;

	float yawRadians = -(*flRotation) * M_PI / 180.0f;
	float ca = cos( yawRadians );
	float sa = sin( yawRadians );
				 
	// Rotate it around the circle
	*xpos = (int)((ScreenWidth() / 2) + (flRadius * sa));
	*ypos = (int)((ScreenHeight() / 2) - (flRadius * ca));
}

//-----------------------------------------------------------------------------
// Purpose: Draw a single damage indicator
//-----------------------------------------------------------------------------
void CHudDamageIndicator::DrawDamageIndicator(int x0, int y0, int x1, int y1, float alpha, float flRotation )
{
	CMatRenderContextPtr pRenderContext( materials );
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_WhiteAdditiveMaterial );

	// Get the corners, since they're being rotated
	int wide = x1 - x0;
	int tall = y1 - y0;
	Vector2D vecCorners[4];
	Vector2D center( x0 + (wide * 0.5f), y0 + (tall * 0.5f) );
	float yawRadians = -flRotation * M_PI / 180.0f;
	Vector2D axis[2];
	axis[0].x = cos(yawRadians);
	axis[0].y = sin(yawRadians);
	axis[1].x = -axis[0].y;
	axis[1].y = axis[0].x;
	Vector2DMA( center, -0.5f * wide, axis[0], vecCorners[0] );
	Vector2DMA( vecCorners[0], -0.5f * tall, axis[1], vecCorners[0] );
	Vector2DMA( vecCorners[0], wide, axis[0], vecCorners[1] );
	Vector2DMA( vecCorners[1], tall, axis[1], vecCorners[2] );
	Vector2DMA( vecCorners[0], tall, axis[1], vecCorners[3] );

	// Draw the sucker
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	int iAlpha = alpha * 255;
	meshBuilder.Color4ub( 255,255,255, iAlpha );
	meshBuilder.TexCoord2f( 0,0,0 );
	meshBuilder.Position3f( vecCorners[0].x,vecCorners[0].y,0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255,255,255, iAlpha );
	meshBuilder.TexCoord2f( 0,1,0 );
	meshBuilder.Position3f( vecCorners[1].x,vecCorners[1].y,0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255,255,255, iAlpha );
	meshBuilder.TexCoord2f( 0,1,1 );
	meshBuilder.Position3f( vecCorners[2].x,vecCorners[2].y,0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255,255,255, iAlpha );
	meshBuilder.TexCoord2f( 0,0,1 );
	meshBuilder.Position3f( vecCorners[3].x,vecCorners[3].y,0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDamageIndicator::Paint()
{
	// Iterate backwards, because we might remove them as we go
	int iSize = m_vecDamages.Count();
	for (int i = iSize-1; i >= 0; i--)
	{
		// Scale size to the damage
		float clampedDamage = clamp( (float) m_vecDamages[i].iScale, 0.f, m_iMaximumDamage );

		int iWidth = RemapVal(clampedDamage, 0, m_iMaximumDamage, m_flMinimumWidth, m_flMaximumWidth) * 0.5;
		int iHeight = RemapVal(clampedDamage, 0, m_iMaximumDamage, m_flMinimumHeight, m_flMaximumHeight) * 0.5;

		// Find the place to draw it
		float xpos, ypos;
		float flRotation;
		float flTimeSinceStart = ( gpGlobals->curtime - m_vecDamages[i].flStartTime );
		float flRadius = RemapVal( MIN( flTimeSinceStart, m_flTravelTime ), 0, m_flTravelTime, m_flStartRadius, m_flEndRadius );
		GetDamagePosition( m_vecDamages[i].vecDelta, flRadius, &xpos, &ypos, &flRotation );

		// Calculate life left
		float flLifeLeft = ( m_vecDamages[i].flLifeTime - gpGlobals->curtime );
		if ( flLifeLeft > 0 )
		{
			float flPercent = flTimeSinceStart / (m_vecDamages[i].flLifeTime - m_vecDamages[i].flStartTime);
			float alpha;
			if ( flPercent <= m_flFadeOutPercentage )
			{
				alpha = 1.0;
			}
			else
			{
				alpha = 1.0 - RemapVal( flPercent, m_flFadeOutPercentage, 1.0, 0.0, 1.0 );
			}
			DrawDamageIndicator( xpos-iWidth, ypos-iHeight, xpos+iWidth, ypos+iHeight, alpha, flRotation );
		}
		else
		{
			m_vecDamages.Remove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for Damage message
//-----------------------------------------------------------------------------
void CHudDamageIndicator::MsgFunc_Damage( bf_read &msg )
{
	damage_t damage;
	damage.iScale = msg.ReadShort();
	msg.ReadLong();	// Read & ignored
	if ( !msg.ReadOneBit() )
		return;

	if ( damage.iScale > m_iMaximumDamage )
	{
		damage.iScale = m_iMaximumDamage;
	}
	Vector vecOrigin;
	msg.ReadBitVec3Coord( vecOrigin );

	damage.flStartTime = gpGlobals->curtime;
	damage.flLifeTime = gpGlobals->curtime + RemapVal(damage.iScale, 0, m_iMaximumDamage, m_flMinimumTime, m_flMaximumTime);

	if ( vecOrigin == vec3_origin )
	{
		vecOrigin = MainViewOrigin();
	}

	damage.vecDelta = (vecOrigin - MainViewOrigin());
	VectorNormalize( damage.vecDelta );

	// Add some noise
	damage.vecDelta[0] += random->RandomFloat( -m_flNoise, m_flNoise );
	damage.vecDelta[1] += random->RandomFloat( -m_flNoise, m_flNoise );
	damage.vecDelta[2] += random->RandomFloat( -m_flNoise, m_flNoise );
	VectorNormalize( damage.vecDelta );

	m_vecDamages.AddToTail( damage );
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudDamageIndicator::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);

	// set our size
	int screenWide, screenTall;
	int x, y;
	GetPos(x, y);
	GetHudSize(screenWide, screenTall);
	SetBounds(0, y, screenWide, screenTall - y);
}
