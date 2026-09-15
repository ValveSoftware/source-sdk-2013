//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "matsys_controls/vtfpreviewpanel.h"
#include "matsys_controls/matsyscontrols.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imesh.h"
#include "tier1/KeyValues.h"


using namespace vgui;


#define FOV 90.0f
#define ZNEAR 0.1f
#define ZFAR 2000.0f
#define ROTATION_SPEED 120.0f		// degrees/sec

//-----------------------------------------------------------------------------
//
// VTF Preview panel
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------
CVTFPreviewPanel::CVTFPreviewPanel( vgui::Panel *pParent, const char *pName ) :
	BaseClass( pParent, pName )
{
	SetVTF( "//platform/materials/vgui/vtfnotloaded", true );
	m_nTextureID = MatSystemSurface()->CreateNewTextureID( false );
}

CVTFPreviewPanel::~CVTFPreviewPanel()
{
	if ( vgui::surface() && m_nTextureID != -1 )
	{
		vgui::surface()->DestroyTextureID( m_nTextureID );
		m_nTextureID = -1;
	}
}


//-----------------------------------------------------------------------------
// Sets the current VTF
//-----------------------------------------------------------------------------
void CVTFPreviewPanel::SetVTF( const char *pFullPath, bool bLoadImmediately )
{
	m_PreviewTexture.Init( pFullPath, "editor texture" );
	m_VTFName = pFullPath;

	KeyValues *pVMTKeyValues = new KeyValues( "UnlitGeneric" );
	if ( m_PreviewTexture->IsCubeMap() )
	{
		pVMTKeyValues->SetString( "$envmap", pFullPath );
	}
	else if ( m_PreviewTexture->IsNormalMap() )
	{
		pVMTKeyValues->SetString( "$bumpmap", pFullPath );
	}
	else
	{
		pVMTKeyValues->SetString( "$basetexture", pFullPath );
	}
	pVMTKeyValues->SetInt( "$nocull", 1 );
	pVMTKeyValues->SetInt( "$nodebug", 1 );
	m_PreviewMaterial.Init( MaterialSystem()->CreateMaterial( pFullPath, pVMTKeyValues ));

	MatSystemSurface()->DrawSetTextureMaterial( m_nTextureID, m_PreviewMaterial );

	// Reset the camera direction
	m_vecCameraDirection.Init( 1.0f, 0.0f, 0.0f );
	m_flLastRotationTime = Plat_FloatTime();
}


//-----------------------------------------------------------------------------
// Gets the current VTF
//-----------------------------------------------------------------------------
const char *CVTFPreviewPanel::GetVTF() const
{
	return m_VTFName;
}


//-----------------------------------------------------------------------------
// Draw a sphere
//-----------------------------------------------------------------------------
void CVTFPreviewPanel::RenderSphere( const Vector &vCenter, float flRadius, int nTheta, int nPhi )
{
	CMatRenderContextPtr pRenderContext( MaterialSystem() );

	int nVertices =  nTheta * nPhi;
	int nIndices = 2 * ( nTheta + 1 ) * ( nPhi - 1 );
	
	pRenderContext->FogMode( MATERIAL_FOG_NONE );
	pRenderContext->SetNumBoneWeights( 0 );
	pRenderContext->Bind( m_PreviewMaterial );

	IMesh* pMesh = pRenderContext->GetDynamicMesh();

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, nVertices, nIndices );

	//
	// Build the index buffer.
	//
	int i, j;
	for ( i = 0; i < nPhi; ++i )
	{
		for ( j = 0; j < nTheta; ++j )
		{
			float u = j / ( float )(nTheta - 1);
			float v = i / ( float )(nPhi - 1);
			float theta = ( j != nTheta-1 ) ? 2.0f * M_PI * u : 0.0f;
			float phi = M_PI * v;

			Vector vecPos;
			vecPos.x = flRadius * sin(phi) * cos(theta);
			vecPos.y = flRadius * cos(phi);
			vecPos.z = -flRadius * sin(phi) * sin(theta); 
			    
			Vector vecNormal = vecPos;
			VectorNormalize( vecNormal );

			Vector4D vecTangentS;
			Vector vecTangentT;
			vecTangentS.Init( vecPos.z, -vecPos.x, 0.0f, 1.0f );
			if ( VectorNormalize( vecTangentS.AsVector3D() ) == 0.0f )
			{
				vecTangentS.Init( 1.0f, 0.0f, 0.0f, 1.0f );
			}

			CrossProduct( vecNormal, vecTangentS.AsVector3D(), vecTangentT );

			unsigned char red = (int)( u * 255.0f );
			unsigned char green = (int)( v * 255.0f );
			unsigned char blue = (int)( v * 255.0f );
			unsigned char alpha = (int)( v * 255.0f );

			vecPos += vCenter;

			float u1, u2, v1, v2;
			u1 = u2 = u;
			v1 = v2 = v;

			meshBuilder.Position3fv( vecPos.Base() );
			meshBuilder.Normal3fv( vecNormal.Base() );
			meshBuilder.Color4ub( red, green, blue, alpha );
			meshBuilder.TexCoord2f( 0, u, v );
			meshBuilder.TexCoord2f( 1, u1, v1 );
			meshBuilder.TexCoord2f( 2, u2, v2 );
			meshBuilder.TangentS3fv( vecTangentS.Base() );
			meshBuilder.TangentT3fv( vecTangentT.Base() );
			meshBuilder.BoneWeight( 0, 1.0f );
			meshBuilder.BoneMatrix( 0, 0 );
			meshBuilder.UserData( vecTangentS.Base() );
			meshBuilder.AdvanceVertex();
		}
	}

	//
	// Emit the triangle strips.
	//
	int idx = 0;
	for ( i = 0; i < nPhi - 1; ++i )
	{
		for ( j = 0; j < nTheta; ++j )
		{
			idx = nTheta * i + j;

			meshBuilder.FastIndex( idx );
			meshBuilder.FastIndex( idx + nTheta );
		}

		//
		// Emit a degenerate triangle to skip to the next row without
		// a connecting triangle.
		//
		if ( i < nPhi - 2 )
		{
			meshBuilder.FastIndex( idx + 1 );
			meshBuilder.FastIndex( idx + 1 + nTheta );
		}
	}

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Paints a regular texture
//-----------------------------------------------------------------------------
void CVTFPreviewPanel::PaintStandardTexture( void )
{	 
	int x, y, w, h;
	x = y = 0;
	GetSize( w, h );
	vgui::surface()->DrawSetTexture( m_nTextureID );
	vgui::surface()->DrawSetColor( 255, 255, 255, 255 );

	// Get the aspect ratio of the texture
	int tw = m_PreviewTexture->GetActualWidth();
 	int th = m_PreviewTexture->GetActualHeight();

	if ( th > 0 && h > 0 )
	{
		float screenaspect = (float)tw / (float)th;
		float aspect = (float)w / (float)h;

		float ratio = screenaspect / aspect;

		// Screen is wider, need bars at top and bottom
		if ( ratio > 1.0f )
		{
			int usetall = (float)w / screenaspect;
			y = ( h - usetall ) / 2;
			h = usetall;
		}
		// Screen is narrower, need bars at left/right
		else
		{
			int usewide = (float)h * screenaspect;
			x = ( w - usewide ) / 2;
			w = usewide;
		}
	}

	vgui::surface()->DrawTexturedRect( x, y, x+w, y+h );
}


//-----------------------------------------------------------------------------
// Paints a normalmap texture
//-----------------------------------------------------------------------------
void CVTFPreviewPanel::PaintNormalMapTexture( void )
{
}


//-----------------------------------------------------------------------------
// Paints a volume texture
//-----------------------------------------------------------------------------
void CVTFPreviewPanel::PaintVolumeTexture( void )
{
}


//-----------------------------------------------------------------------------
// Paints a cubemap texture
//-----------------------------------------------------------------------------
void CVTFPreviewPanel::PaintCubeTexture( void )
{
	float flNewTime = Plat_FloatTime();

	// Circle the camera around the origin
	VMatrix rot;
	MatrixBuildRotateZ( rot, ROTATION_SPEED * (flNewTime - m_flLastRotationTime ) );
	Vector vecTemp;
	Vector3DMultiply( rot, m_vecCameraDirection, vecTemp );
	m_vecCameraDirection = vecTemp;
	m_flLastRotationTime = flNewTime;

 	LookAt( vec3_origin, 12.0f );

	// Draw a sphere at the origin
	RenderSphere( vec3_origin, 10.0f,  20, 20 );
}


//-----------------------------------------------------------------------------
// Sets the camera to look at the the thing we're spinning around
//-----------------------------------------------------------------------------
void CVTFPreviewPanel::LookAt( const Vector &vecLookAt, float flRadius )
{
	// Compute the distance to the camera for the object based on its
	// radius and fov.

	// since tan( fov/2 ) = f/d
	// cos( fov/2 ) = r / r' where r = sphere radius, r' = perp distance from sphere center to max extent of camera
	// d/f = r'/d' where d' is distance of camera to sphere
	// d' = r' / tan( fov/2 ) * r' = r / ( cos (fov/2) * tan( fov/2 ) ) = r / sin( fov/2 )
	float flFOVx = FOV;

	// Compute fov/2 in radians
	flFOVx *= M_PI / 360.0f;

	// Compute an effective fov	based on the aspect ratio 
	// if the height is smaller than the width
	int w, h;
	GetSize( w, h );
	if ( h < w )
	{
		flFOVx = atan( h * tan( flFOVx ) / w );
	}

	float flDistance = flRadius / sin( flFOVx );

	Vector vecMDLOrigin = vecLookAt;
	Vector vecCameraOrigin;
	VectorMA( vecMDLOrigin, -flDistance, m_vecCameraDirection, vecCameraOrigin );

	CMatRenderContextPtr pRenderContext( MaterialSystem() );
	QAngle angles;
	VectorAngles( m_vecCameraDirection, angles );

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->LoadIdentity();

	// convert from a right handed system to a left handed system
	// since dx for wants it that way.  
//	pRenderContext->Scale( 1.0f, 1.0f, -1.0f );

	pRenderContext->Rotate( -90,  1, 0, 0 );	    // put Z going up
	pRenderContext->Rotate( 90,  0, 0, 1 );	    // put Z going up
	pRenderContext->Rotate( -angles[2],  1, 0, 0 );
	pRenderContext->Rotate( -angles[0],  0, 1, 0 );
	pRenderContext->Rotate( -angles[1],  0, 0, 1 );
	pRenderContext->Translate( -vecCameraOrigin[0],  -vecCameraOrigin[1],  -vecCameraOrigin[2] );
}


//-----------------------------------------------------------------------------
// Set up a projection matrix for a 90 degree fov
//-----------------------------------------------------------------------------
void CVTFPreviewPanel::SetupProjectionMatrix( int nWidth, int nHeight )
{
	CMatRenderContextPtr pRenderContext( MaterialSystem() );
	VMatrix proj;
	float flFOV = FOV;
	float flZNear = ZNEAR;
	float flZFar = ZFAR;
	float flApsectRatio = (nHeight != 0.0f) ? (float)nWidth / (float)nHeight : 100.0f;

#if 1
	float halfWidth = tan( flFOV * M_PI / 360.0 );
	float halfHeight = halfWidth / flApsectRatio;
#else
	float halfHeight = tan( flFOV * M_PI / 360.0 );
	float halfWidth = flApsectRatio * halfHeight;
#endif
	memset( proj.Base(), 0, sizeof( proj ) );
	proj[0][0]  = 1.0f / halfWidth;
	proj[1][1]  = 1.0f / halfHeight;
	proj[2][2] = flZFar / ( flZNear - flZFar );
	proj[3][2] = -1.0f;
	proj[2][3] = flZNear * flZFar / ( flZNear - flZFar );

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->LoadMatrix( proj );
}


//-----------------------------------------------------------------------------
// Paints the texture
//-----------------------------------------------------------------------------
void CVTFPreviewPanel::Paint( void )
{
	if ( !m_PreviewTexture->IsCubeMap() && /*!m_PreviewTexture->IsNormalMap() &&*/ !m_PreviewTexture->IsVolumeTexture() )
	{
		PaintStandardTexture();
		return;
	}

	CMatRenderContextPtr pRenderContext( MaterialSystem() );
	int w, h;
	GetSize( w, h );
	vgui::MatSystemSurface()->Begin3DPaint( 0, 0, w, h );

	pRenderContext->ClearColor4ub( 76, 88, 68, 255 ); 
	pRenderContext->ClearBuffers( true, true );
				   
	SetupProjectionMatrix( w, h );

	if ( m_PreviewTexture->IsCubeMap() )
	{
		PaintCubeTexture();
	}
	else if ( m_PreviewTexture->IsNormalMap() )
	{
		PaintNormalMapTexture();
	}
	else if ( m_PreviewTexture->IsVolumeTexture() )
	{
		PaintVolumeTexture();
	}

	vgui::MatSystemSurface()->End3DPaint( );
}