//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Responsible for drawing the scene
//
//===========================================================================//

#include "cbase.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "rendertexture.h"
#include "view_scene.h"
#include "viewrender.h"
#include "sourcevr/isourcevirtualreality.h"
#include "client_virtualreality.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Convars related to controlling rendering
//-----------------------------------------------------------------------------
ConVar r_updaterefracttexture( "r_updaterefracttexture", "1", FCVAR_CHEAT );
ConVar r_depthoverlay( "r_depthoverlay", "0", FCVAR_CHEAT, "Replaces opaque objects with their grayscaled depth values. r_showz_power scales the output." );


int g_viewscene_refractUpdateFrame = 0;
bool g_bAllowMultipleRefractUpdatesPerScenePerFrame = false;

#if defined( _X360 )
class CAllowMultipleRefractsLogic : public CAutoGameSystem
{
public:
	void LevelInitPreEntity()
	{
		// EP1 core room needs many refract updates per frame to avoid looking broken (ep1_citadel_03)
		// Same with Kleiner's lab (d1_trainstation_05)
		g_bAllowMultipleRefractUpdatesPerScenePerFrame = FStrEq( MapName(), "ep1_citadel_03" ) || FStrEq( MapName(), "d1_trainstation_05" );
	}
};
static CAllowMultipleRefractsLogic s_AllowMultipleRefractsLogic;
#endif

void ViewTransform( const Vector &worldSpace, Vector &viewSpace )
{
	const VMatrix &viewMatrix = engine->WorldToViewMatrix();
	Vector3DMultiplyPosition( viewMatrix, worldSpace, viewSpace );
}



//-----------------------------------------------------------------------------
// Purpose: Transforms a world-space position into a 2D position inside a supplied frustum.
//-----------------------------------------------------------------------------
int FrustumTransform( const VMatrix &worldToSurface, const Vector& point, Vector& screen )
{
	// UNDONE: Clean this up some, handle off-screen vertices
	float w;

	screen.x = worldToSurface[0][0] * point[0] + worldToSurface[0][1] * point[1] + worldToSurface[0][2] * point[2] + worldToSurface[0][3];
	screen.y = worldToSurface[1][0] * point[0] + worldToSurface[1][1] * point[1] + worldToSurface[1][2] * point[2] + worldToSurface[1][3];
	//	z		 = worldToSurface[2][0] * point[0] + worldToSurface[2][1] * point[1] + worldToSurface[2][2] * point[2] + worldToSurface[2][3];
	w		 = worldToSurface[3][0] * point[0] + worldToSurface[3][1] * point[1] + worldToSurface[3][2] * point[2] + worldToSurface[3][3];

	// Just so we have something valid here
	screen.z = 0.0f;

	bool behind;
	if( w < 0.001f )
	{
		behind = true;
		screen.x *= 100000;
		screen.y *= 100000;
	}
	else
	{
		behind = false;
		float invw = 1.0f / w;
		screen.x *= invw;
		screen.y *= invw;
	}

	return behind;
}


//-----------------------------------------------------------------------------
// Purpose: UNDONE: Clean this up some, handle off-screen vertices
// Input  : *point - 
//			*screen - 
// Output : int
//-----------------------------------------------------------------------------
int ScreenTransform( const Vector& point, Vector& screen )
{
	// UNDONE: Clean this up some, handle off-screen vertices
	return FrustumTransform ( engine->WorldToScreenMatrix(), point, screen );
}

//-----------------------------------------------------------------------------
// Purpose: Same as ScreenTransform, but transforms to HUD space.
//			These are totally different things in VR mode!
//-----------------------------------------------------------------------------
int HudTransform( const Vector& point, Vector& screen )
{
	if ( UseVR() )
	{
		return FrustumTransform ( g_ClientVirtualReality.GetHudProjectionFromWorld(), point, screen );
	}
	else
	{
		return FrustumTransform ( engine->WorldToScreenMatrix(), point, screen );
	}
}



void UpdateFullScreenDepthTexture( void )
{
	if( !g_pMaterialSystemHardwareConfig->SupportsPixelShaders_2_b() )
		return;

	ITexture *pDepthTex = GetFullFrameDepthTexture();
	CMatRenderContextPtr pRenderContext( materials );

	if( IsX360() )
	{	
		pRenderContext->CopyRenderTargetToTextureEx( pDepthTex, -1, NULL, NULL );
	}
	else
	{
		pRenderContext->CopyRenderTargetToTextureEx( pDepthTex, 0, NULL, NULL );
	}

	pRenderContext->SetFullScreenDepthTextureValidityFlag( true );

	if( r_depthoverlay.GetBool() )
	{
		IMaterial *pMaterial = materials->FindMaterial( "debug/showz", TEXTURE_GROUP_OTHER, true );
		pMaterial->IncrementReferenceCount();
		IMaterialVar *BaseTextureVar = pMaterial->FindVar( "$basetexture", NULL, false );
		IMaterialVar *pDepthInAlpha = NULL;
		if( IsPC() )
		{
			pDepthInAlpha = pMaterial->FindVar( "$ALPHADEPTH", NULL, false );
			pDepthInAlpha->SetIntValue( 1 );
		}
		
		BaseTextureVar->SetTextureValue( pDepthTex );

		pRenderContext->OverrideDepthEnable( true, false ); //don't write to depth, or else we'll never see translucents
		pRenderContext->DrawScreenSpaceQuad( pMaterial );
		pRenderContext->OverrideDepthEnable( false, true );
		pMaterial->DecrementReferenceCount();
	}
}
