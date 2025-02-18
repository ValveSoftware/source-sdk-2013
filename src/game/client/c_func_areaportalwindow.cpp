//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "view.h"
#include "model_types.h"
#include "ivrenderview.h"
#include "engine/ivmodelinfo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define VIEWER_PADDING	80.0f

class C_FuncAreaPortalWindow : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_FuncAreaPortalWindow, C_BaseEntity );


// Overrides.
public:

	virtual void	ComputeFxBlend();
	virtual bool	IsTransparent();
	virtual int		DrawModel( int flags );
	virtual bool	ShouldReceiveProjectedTextures( int flags );

private:

	float			GetDistanceBlend();



public:
	float			m_flFadeStartDist;	// Distance at which it starts fading (when <= this, alpha=m_flTranslucencyLimit).
	float			m_flFadeDist;		// Distance at which it becomes solid.

	// 0-1 value - minimum translucency it's allowed to get to.
	float			m_flTranslucencyLimit;

	int				m_iBackgroundModelIndex;
};



IMPLEMENT_CLIENTCLASS_DT( C_FuncAreaPortalWindow, DT_FuncAreaPortalWindow, CFuncAreaPortalWindow )
	RecvPropFloat( RECVINFO( m_flFadeStartDist ) ),
	RecvPropFloat( RECVINFO( m_flFadeDist ) ),
	RecvPropFloat( RECVINFO( m_flTranslucencyLimit ) ),
	RecvPropInt( RECVINFO( m_iBackgroundModelIndex ) )
END_RECV_TABLE()



void C_FuncAreaPortalWindow::ComputeFxBlend()
{
	// We reset our blend down below so pass anything except 0 to the renderer.
	m_nRenderFXBlend = 255;

#ifdef _DEBUG
	m_nFXComputeFrame = gpGlobals->framecount;
#endif

}


bool C_FuncAreaPortalWindow::IsTransparent()
{
	return true;
}


int C_FuncAreaPortalWindow::DrawModel( int flags )
{
	if ( !m_bReadyToDraw )
		return 0;

	if( !GetModel() )
		return 0;

	// Make sure we're a brush model.
	int modelType = modelinfo->GetModelType( GetModel() );
	if( modelType != mod_brush )
		return 0;

	// Draw the fading version.
	render->SetBlend( GetDistanceBlend() );

	DrawBrushModelMode_t mode = DBM_DRAW_ALL;
	if ( flags & STUDIO_TWOPASS )
	{
		mode = ( flags & STUDIO_TRANSPARENCY ) ? DBM_DRAW_TRANSLUCENT_ONLY : DBM_DRAW_OPAQUE_ONLY;
	}

	render->DrawBrushModelEx( 
		this, 
		(model_t *)GetModel(), 
		GetAbsOrigin(), 
		GetAbsAngles(), 
		mode );

	// Draw the optional foreground model next.
	// Only use the alpha in the texture from the thing in the front.
	if (m_iBackgroundModelIndex >= 0)
	{
		render->SetBlend( 1 );
		model_t *pBackground = ( model_t * )modelinfo->GetModel( m_iBackgroundModelIndex );
		if( pBackground && modelinfo->GetModelType( pBackground ) == mod_brush )
		{
			render->DrawBrushModelEx( 
				this, 
				pBackground, 
				GetAbsOrigin(), 
				GetAbsAngles(), 
				mode );
		}
	}

	return 1;
}


float C_FuncAreaPortalWindow::GetDistanceBlend()
{
	// Get the viewer's distance to us.
	float flDist = CollisionProp()->CalcDistanceFromPoint( CurrentViewOrigin() );
	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	if ( local )
	{
		flDist *= local->GetFOVDistanceAdjustFactor();
	}
	
	return RemapValClamped( flDist, m_flFadeStartDist, m_flFadeDist, m_flTranslucencyLimit, 1 );
}

bool C_FuncAreaPortalWindow::ShouldReceiveProjectedTextures( int flags )
{
	return false;
}


