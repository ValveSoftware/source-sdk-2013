//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_tf_passtime_logic.h"
#include "tf_hud_passtime_ball_offscreen_arrow.h"
#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "tf_controls.h"
#include "view.h"
#include "ivieweffects.h"
#include "viewrender.h"
#include "prediction.h"
#include "tf_gamerules.h"
#include "c_tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PANEL_WIDE			(XRES(48))
#define PANEL_TALL			(XRES(48))
#define PANEL_ARROW_WIDE	(XRES(24))
#define PANEL_ARROW_TALL	(XRES(24))

//-----------------------------------------------------------------------------
static const char *GetBallImageForTeam( int iTeam )
{
	switch( iTeam )
	{
	case TF_TEAM_RED: return "../passtime/hud/passtime_ball_offscreen_red";
	case TF_TEAM_BLUE: return "../passtime/hud/passtime_ball_offscreen_blue";
	default: return "../passtime/hud/passtime_ball";
	};
}


//=============================================================================
// CTFHudPasstimeOffscreenArrow
//=============================================================================


//-----------------------------------------------------------------------------
CTFHudPasstimeOffscreenArrow::CTFHudPasstimeOffscreenArrow( Panel *parent, const char *name ) 
	: EditablePanel( parent, name )
	, m_pArrowMaterial( 0 )
	, m_pImage( 0 )
{
}

//-----------------------------------------------------------------------------
CTFHudPasstimeOffscreenArrow::~CTFHudPasstimeOffscreenArrow()
{
	if ( m_pArrowMaterial )
	{
		m_pArrowMaterial->DecrementReferenceCount();
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeOffscreenArrow::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/HudPasstimeOffscreenArrow.res" );
	SetBounds( 0,0, PANEL_WIDE, PANEL_TALL );
	SetVisible( true );

	if ( m_pArrowMaterial )
	{
		m_pArrowMaterial->DecrementReferenceCount();
	}
	m_pArrowMaterial = materials->FindMaterial( "HUD/medic_arrow", TEXTURE_GROUP_VGUI );
	m_pArrowMaterial->IncrementReferenceCount();

	m_pImage = FindControl<vgui::ImagePanel>( "Image" );
	if ( m_pImage )
	{
		m_pImage->SetVisible( true );
		m_pImage->SetSize( PANEL_WIDE, PANEL_TALL );
		m_pImage->SetPos( (GetWide() - m_pImage->GetWide()) * 0.5, (GetTall() - m_pImage->GetTall()) * 0.5 );
	}
}

//-----------------------------------------------------------------------------
// TODO what's the difference between Paint and PaintBackground?
extern int HudTransform( const Vector& point, Vector& screen );
void CTFHudPasstimeOffscreenArrow::PaintBackground()
{
	//	BaseClass::PaintBackground(); 
	if ( !g_pPasstimeLogic )
	{
		return;
	}

	//
	// Let subclasses determine where to point and how to look
	//
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BaseEntity *pEnt = PreparePaint( m_pImage, pLocalPlayer );

	//
	// Check if everything is valid, friendly, and visible.
	//
	{
		if ( !pEnt || !pLocalPlayer )
		{
			SetAlpha( 0 );
			return;
		}

		const bool bTargetVisible = (pEnt->GetEffects() & EF_NODRAW) != 0;
		const bool bLocalPlayerTarget = (pEnt == pLocalPlayer);
		if ( bTargetVisible || bLocalPlayerTarget )
		{
			SetAlpha( 0 );
			return;
		}
	}

	// 
	// Determine if the arrow should even be visible
	// HudTransform works ok as long as the point is on-screen, otherwise it's useless.
	// Calling HudTransform here does a lot of redundant work, but I don't care right now.
	//
	Vector vecEntPos = pEnt->WorldSpaceCenter();
	{
		Vector vecActualProjection;
		const bool bBehind = HudTransform( vecEntPos, vecActualProjection );
		const bool bCenterOfScreen =
			(vecActualProjection.x < 1.0f) && (vecActualProjection.x > -1.0f) 
			&& (vecActualProjection.y < 1.0f) && (vecActualProjection.y > -1.0f);
		if ( !bBehind && bCenterOfScreen )
		{
			SetAlpha( 0 );
			return;
		}
	}

	// Definitely visible

	// 
	// Move the target into view space.
	// The screen is in the y/z plane.
	//
	Vector vecLocalTarget;
	{
		VMatrix mWorldToView( SetupMatrixIdentity() );
		const VMatrix mTemp( SetupMatrixOrgAngles( CurrentViewOrigin(), CurrentViewAngles() ) );
		MatrixInverseTR( mTemp, mWorldToView );
		Vector3DMultiplyPosition( mWorldToView, vecEntPos, vecLocalTarget );
	}

	//
	// Calc the direction in viewspace from the view origin to the target.
	// Since all we want is a direction on-screen that goes toward the object, we
	// don't need to actually project it. it's transformed to view space to get a direction.
	// vecHudDir is always a point on the exact edge of the screen.
	//
	float flArrowAngle;
	{
		const Vector vecHudDir = Vector( -vecLocalTarget.y / engine->GetScreenAspectRatio(), -vecLocalTarget.z, 0 ).Normalized();
		flArrowAngle = atan2( vecHudDir.y, vecHudDir.x );
		// put it in the range 0 to 2PI
		if ( flArrowAngle > (M_PI_F * 2.0f) )
			flArrowAngle -= (M_PI_F * 2.0f);
		else if ( flArrowAngle < 0 )
			flArrowAngle += (M_PI_F * 2.0f);
	}

	//
	// Build the mesh for the arrow, because vgui can't rotate
	// TODO probably don't need to build this mesh every frame
	// The code that draws the arrow mesh has to use the current vgui
	//  position or there will be a one-frame position lag between the two.
	//
	{
		int iX = PANEL_ARROW_WIDE / 2;
		int iY = PANEL_ARROW_TALL / 2;

		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->Bind( m_pArrowMaterial );

		IMesh* pMesh = pRenderContext->GetDynamicMesh( true );
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Position3f( -iX, -iY, 0 );
		meshBuilder.TexCoord2f( 0, 0, 0 );
		meshBuilder.Color4ub( 255, 255, 255, GetAlpha() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( iX, -iY, 0 );
		meshBuilder.TexCoord2f( 0, 1, 0 );
		meshBuilder.Color4ub( 255, 255, 255, GetAlpha() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( iX, iY, 0 );
		meshBuilder.TexCoord2f( 0, 1, 1 );
		meshBuilder.Color4ub( 255, 255, 255, GetAlpha() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( -iX, iY, 0 );
		meshBuilder.TexCoord2f( 0, 0, 1 );
		meshBuilder.Color4ub( 255, 255, 255, GetAlpha() );
		meshBuilder.AdvanceVertex();
		meshBuilder.End();

		// Determine a scale factor based on how close the player is to the ball.  Scale is between 1-2.
		Vector vecPlayerPos = pLocalPlayer->GetNetworkOrigin();
		float flDistanceToTargetSq = (float)(vecEntPos.DistToSqr( vecPlayerPos ));
		float flMaxDistanceSq = 1000000.0f;
		flDistanceToTargetSq = Min( flDistanceToTargetSq, flMaxDistanceSq );
		float flDistanceScale = (flDistanceToTargetSq / flMaxDistanceSq) + 1.0f;

		GetPos( iX, iY );
		iX += GetWide() / 2;
		iY += GetTall() / 2;
		pRenderContext->PushMatrix();
		pRenderContext->Translate( iX, iY, 0 );
		pRenderContext->Rotate( RAD2DEG( flArrowAngle ), 0, 0, 1 );
		pRenderContext->Scale( flDistanceScale, 1.0f, 1.0f );
		pRenderContext->Translate( GetWide() / 2, 0, 0 );
		pMesh->Draw();
		pRenderContext->PopMatrix();
	}

	//
	// Given the angle, calc a position on the screen for the indicator.
	// This finds the point at which vecHudDir intersects with the edges of the screen.
	// Roughly.
	//
	Vector vecHudPoint;
	{
		Assert( flArrowAngle >= 0 && flArrowAngle < (M_PI_F * 2.0f) );
		vecHudPoint.y = tanf( flArrowAngle );
		if ( fabsf( vecHudPoint.y ) <= 1 )
		{
			// point will be either along the left or right edge.
			// y coord is between top and bottom edges, decide whether it lies on
			// the left or right side of the screen.
			const float fHalfPi = M_PI_F / 2.0f;
			if ( (flArrowAngle < fHalfPi) || (flArrowAngle > (M_PI_F + fHalfPi)) )
				vecHudPoint.Init( 1, vecHudPoint.y );
			else
				vecHudPoint.Init( -1, -vecHudPoint.y );
		}
		else
		{
			// point will be either along the top or bottom edge.
			// y coord is out of bounds, clamp it and compute x coord
			if ( flArrowAngle < M_PI_F )
				vecHudPoint.Init( (1 / vecHudPoint.y), 1 );
			else
				vecHudPoint.Init( (-1 / vecHudPoint.y), -1 );
		}
		vecHudPoint.x *= 0.8f; // bring toward center.
		vecHudPoint.y *= 0.5f;
	}

	//
	// Convert to hud coordinates and reposition the vgui control
	//
	{
		const int iX = (0.5f * ( 1.0f + vecHudPoint.x ) * ScreenWidth()) - (GetWide() / 2);
		const int iY = (0.5f * ( 1.0f + vecHudPoint.y ) * ScreenHeight()) - (GetTall() / 2);

#ifdef DEBUG
		// This block avoids calling SetPos with invalid values while debugging,
		// because otherwise Paint will never be called again.
		const int min = 8;
		const int maxx = ScreenWidth() - 8;
		const int maxy = ScreenHeight() - 8;
		if ( iX > min && iX < maxx && iY > min && iY < maxy )
#endif
			SetPos( iX, iY );
	}
}


//=============================================================================
// CTFHudPasstimeBallOffscreenArrow
//=============================================================================


//-----------------------------------------------------------------------------
CTFHudPasstimeBallOffscreenArrow::CTFHudPasstimeBallOffscreenArrow( vgui::Panel *pParent )
	: BaseClass( pParent, "PasstimeBallOffscreenArrow" )
{
}

//-----------------------------------------------------------------------------
C_BaseEntity *CTFHudPasstimeBallOffscreenArrow::PreparePaint( 
	vgui::ImagePanel *pImage, C_TFPlayer *pLocalPlayer ) 
{
	if ( !g_pPasstimeLogic ) 
	{
		return NULL;
	}

	C_PasstimeBall *pBall = g_pPasstimeLogic->GetBall();
	C_BaseEntity *pTarget = 0;
	bool bHomingActive = false;
	bool bHaveTarget = g_pPasstimeLogic->GetBallReticleTarget( &pTarget, &bHomingActive );
	if ( !pImage || !pBall || !pLocalPlayer || !bHaveTarget )
	{
		return NULL;
	}

	if ( pLocalPlayer->m_Shared.IsTargetedForPasstimePass() || bHomingActive )
	{
		SetAlpha( (int)( (fmodf( gpGlobals->curtime * 3.0f, 1.0f )) * 255 ) );
	}
	else
	{
		SetAlpha( 128 );
	}

	if ( pImage )
	{
		// setimage will ignore redundant calls
		if ( pTarget )
		{
			pImage->SetImage( GetBallImageForTeam( pTarget->GetTeamNumber() ) );
		}
		else
		{
			pImage->SetImage( "../passtime/hud/passtime_ball" );
		}
	}

	return pTarget;
}


//=============================================================================
// CTFHudPasstimePlayerOffscreenArrow
//=============================================================================


//-----------------------------------------------------------------------------
CTFHudPasstimePlayerOffscreenArrow::CTFHudPasstimePlayerOffscreenArrow( vgui::Panel *pParent, int iPlayerIndex )
	: BaseClass( pParent, "PasstimePlayerOffscreenArrow" )
	, m_iPlayerIndex( iPlayerIndex )
{
}

//-----------------------------------------------------------------------------
C_BaseEntity *CTFHudPasstimePlayerOffscreenArrow::PreparePaint( 
	vgui::ImagePanel *pImage, C_TFPlayer *pLocalPlayer ) 
{
	if ( !pImage || (!pLocalPlayer->m_Shared.HasPasstimeBall() && !pLocalPlayer->IsObserver()) )
	{
		return NULL;
	}

	C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( m_iPlayerIndex ) );
	if ( !pPlayer || (pPlayer == pLocalPlayer) || (pPlayer->m_Shared.AskForBallTime() < gpGlobals->curtime) )
	{
		return NULL;
	}

	SetAlpha( 128 );
	pImage->SetImage( "../passtime/hud/passtime_pass_to_me_prompt" );
	return pPlayer;
}
