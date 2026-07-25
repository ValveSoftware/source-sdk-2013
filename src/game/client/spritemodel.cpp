//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "enginesprite.h"
#include "hud.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "c_sprite.h"
#include "tier1/callqueue.h"
#include "tier1/KeyValues.h"
#include "tier2/tier2.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Sprites are clipped to this rectangle (x,y,width,height) if ScissorTest is enabled
static int scissor_x = 0;
static int scissor_y = 0;
static int scissor_width = 0;
static int scissor_height = 0;
static bool giScissorTest = false;

//-----------------------------------------------------------------------------
// Purpose: 
// Set the scissor
//  the coordinate system for gl is upsidedown (inverted-y) as compared to software, so the
//  specified clipping rect must be flipped
// Input  : x - 
//			y - 
//			width - 
//			height - 
//-----------------------------------------------------------------------------
void EnableScissorTest( int x, int y, int width, int height )
{
	x = clamp( x, 0, ScreenWidth() );
	y = clamp( y, 0, ScreenHeight() );
	width = clamp( width, 0, ScreenWidth() - x );
	height = clamp( height, 0, ScreenHeight() - y );

	scissor_x = x;
	scissor_width = width;
	scissor_y = y;
	scissor_height = height;

	giScissorTest = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DisableScissorTest( void )
{
	scissor_x = 0;
	scissor_width = 0;
	scissor_y = 0;
	scissor_height = 0;
	
	giScissorTest = false;
}

//-----------------------------------------------------------------------------
// Purpose: Verify that this is a valid, properly ordered rectangle.
// Input  : *prc - 
// Output : int
//-----------------------------------------------------------------------------
static int ValidateWRect(const wrect_t *prc)
{

	if (!prc)
		return false;

	if ((prc->left >= prc->right) || (prc->top >= prc->bottom))
	{
		//!!!UNDONE Dev only warning msg
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: classic interview question
// Input  : *prc1 - 
//			*prc2 - 
//			*prc - 
// Output : int
//-----------------------------------------------------------------------------
static int IntersectWRect(const wrect_t *prc1, const wrect_t *prc2, wrect_t *prc)
{	
	wrect_t rc;

	if (!prc)
		prc = &rc;

	prc->left = MAX(prc1->left, prc2->left);
	prc->right = MIN(prc1->right, prc2->right);

	if (prc->left < prc->right)
	{
		prc->top = MAX(prc1->top, prc2->top);
		prc->bottom = MIN(prc1->bottom, prc2->bottom);

		if (prc->top < prc->bottom)
			return 1;

	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : x - 
//			y - 
//			width - 
//			height - 
//			u0 - 
//			v0 - 
//			u1 - 
//			v1 - 
// Output : static bool
//-----------------------------------------------------------------------------
static bool Scissor( int& x, int& y, int& width, int& height, float& u0, float& v0, float& u1, float& v1 )
{
	// clip sub rect to sprite
	if ((width == 0) || (height == 0))
		return false;

	if ((x + width <= scissor_x) || (x >= scissor_x + scissor_width) ||
		(y + height <= scissor_y) || (y >= scissor_y + scissor_height))
		return false;

	float dudx = (u1-u0) / width;
	float dvdy = (v1-v0) / height;
	if (x < scissor_x)
	{
		u0 += (scissor_x - x) * dudx;
		width -= scissor_x - x;
		x = scissor_x;
	}

	if (x + width > scissor_x + scissor_width)
	{
		u1 -= (x + width - (scissor_x + scissor_width)) * dudx;
		width = scissor_x + scissor_width - x;
	}

	if (y < scissor_y)
	{
		v0 += (scissor_y - y) * dvdy;
		height -= scissor_y - y;
		y = scissor_y;
	}

	if (y + height > scissor_y + scissor_height)
	{
		v1 -= (y + height - (scissor_y + scissor_height)) * dvdy;
		height = scissor_y + scissor_height - y;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSprite - 
//			frame - 
//			*pfLeft - 
//			*pfRight - 
//			*pfTop - 
//			*pfBottom - 
//			*pw - 
//			*ph - 
//			*prcSubRect - 
// Output : static void
//-----------------------------------------------------------------------------
static void AdjustSubRect(CEngineSprite *pSprite, int frame, float *pfLeft, float *pfRight, float *pfTop, 
						  float *pfBottom, int *pw, int *ph, const wrect_t *prcSubRect)
{
	wrect_t rc;
	float f;

	if (!ValidateWRect(prcSubRect))
		return;

	// clip sub rect to sprite

	rc.top = rc.left = 0;
	rc.right = *pw;
	rc.bottom = *ph;

	if (!IntersectWRect(prcSubRect, &rc, &rc))
		return;

	*pw = rc.right - rc.left;
	*ph = rc.bottom - rc.top;

	f = 1.0 / (float)pSprite->GetWidth();
	*pfLeft = ((float)rc.left + 0.5) * f;
	*pfRight = ((float)rc.right - 0.5) * f;

	f = 1.0 / (float)pSprite->GetHeight();
	*pfTop = ((float)rc.top + 0.5) * f;
	*pfBottom = ((float)rc.bottom - 0.5) * f;

	return;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static unsigned int spriteOriginCache = 0;
static unsigned int spriteOrientationCache = 0;
bool CEngineSprite::Init( const char *pName )
{
	m_VideoMaterial = NULL;
	for ( int i = 0; i < kRenderModeCount; ++i )
	{
		m_material[ i ] = NULL;
	}

	m_width = m_height = m_numFrames = 1;

	Assert( g_pVideo != NULL );
	
	if ( g_pVideo != NULL && g_pVideo->LocateVideoSystemForPlayingFile( pName ) != VideoSystem::NONE ) 
	{
		m_VideoMaterial = g_pVideo->CreateVideoMaterial( pName, pName, "GAME", VideoPlaybackFlags::DEFAULT_MATERIAL_OPTIONS, VideoSystem::DETERMINE_FROM_FILE_EXTENSION, false ); 
		
		if ( m_VideoMaterial == NULL )
			return false;

		IMaterial *pMaterial = m_VideoMaterial->GetMaterial();
		m_VideoMaterial->GetVideoImageSize( &m_width, &m_height );
		m_numFrames = m_VideoMaterial->GetFrameCount();
		for ( int i = 0; i < kRenderModeCount; ++i )
		{
			m_material[i] = pMaterial;
			pMaterial->IncrementReferenceCount();
		}
	}
	else
	{
		char pTemp[MAX_PATH];
		char pMaterialName[MAX_PATH];
		char pMaterialPath[MAX_PATH];
		Q_StripExtension( pName, pTemp, sizeof(pTemp) );
		Q_strlower( pTemp );
		Q_FixSlashes( pTemp, '/' );

		// Check to see if this is a UNC-specified material name
		bool bIsUNC = pTemp[0] == '/' && pTemp[1] == '/' && pTemp[2] != '/';
		if ( !bIsUNC )
		{
			Q_strncpy( pMaterialName, "materials/", sizeof(pMaterialName) );
			Q_strncat( pMaterialName, pTemp, sizeof(pMaterialName), COPY_ALL_CHARACTERS );
		}
		else
		{
			Q_strncpy( pMaterialName, pTemp, sizeof(pMaterialName) );
		}
		Q_strncpy( pMaterialPath, pMaterialName, sizeof(pMaterialPath) );
		Q_SetExtension( pMaterialPath, ".vmt", sizeof(pMaterialPath) );

		KeyValuesAD kv( "vmt" );
		if ( !kv->LoadFromFile( g_pFullFileSystem, pMaterialPath, "GAME" ) )
		{
			Warning( "Unable to load sprite material %s!\n", pMaterialPath );
			return false;
		}

		for ( int i = 0; i < kRenderModeCount; ++i )
		{	
			if ( i == kRenderNone || i == kRenderEnvironmental )
			{
				m_material[i] = NULL;
				continue;
			}

			Q_snprintf( pMaterialPath, sizeof(pMaterialPath), "%s_rendermode_%d", pMaterialName, i );
			KeyValues *pMaterialKV = kv->MakeCopy();
			pMaterialKV->SetInt( "$spriteRenderMode", i );
			m_material[i] = g_pMaterialSystem->FindProceduralMaterial( pMaterialPath, TEXTURE_GROUP_CLIENT_EFFECTS, pMaterialKV );
			m_material[ i ]->IncrementReferenceCount();
		}

		m_width = m_material[0]->GetMappingWidth();
		m_height = m_material[0]->GetMappingHeight();
		m_numFrames = m_material[0]->GetNumAnimationFrames();
	}

	for ( int i = 0; i < kRenderModeCount; ++i )
	{
		if ( i == kRenderNone || i == kRenderEnvironmental )
			continue;

		if ( !m_material[i] )
			return false;
	}

	IMaterialVar *orientationVar = m_material[0]->FindVarFast( "$spriteorientation", &spriteOrientationCache );
	m_orientation = orientationVar ? orientationVar->GetIntValue() : C_SpriteRenderer::SPR_VP_PARALLEL_UPRIGHT;

	IMaterialVar *originVar = m_material[0]->FindVarFast( "$spriteorigin", &spriteOriginCache );
	Vector origin, originVarValue;
	if( !originVar || ( originVar->GetType() != MATERIAL_VAR_TYPE_VECTOR ) )
	{
		origin[0] = -m_width * 0.5f;
		origin[1] = m_height * 0.5f;
	}
	else
	{
		originVar->GetVecValue( &originVarValue[0], 3 );
		origin[0] = -m_width * originVarValue[0];
		origin[1] = m_height * originVarValue[1];
	}

	up = origin[1];
	down = origin[1] - m_height;
	left = origin[0];
	right = m_width + origin[0];

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEngineSprite::Shutdown( void )
{
	if ( g_pVideo != NULL && m_VideoMaterial != NULL )
	{
		g_pVideo->DestroyVideoMaterial( m_VideoMaterial );
		m_VideoMaterial = NULL;
	}

	UnloadMaterial();
}



//-----------------------------------------------------------------------------
// Is the sprite a video sprite?
//-----------------------------------------------------------------------------
bool CEngineSprite::IsVideo()
{
	return ( m_VideoMaterial != NULL );
}

//-----------------------------------------------------------------------------
// Returns the texture coordinate range	used to draw the sprite
//-----------------------------------------------------------------------------
void CEngineSprite::GetTexCoordRange( float *pMinU, float *pMinV, float *pMaxU, float *pMaxV )
{
	*pMaxU = 1.0f; 
	*pMaxV = 1.0f;
	if ( IsVideo() )
	{
		m_VideoMaterial->GetVideoTexCoordRange( pMaxU, pMaxV );
	}
	
	float flOOWidth = ( m_width != 0 ) ? 1.0f / m_width : 1.0f;
	float flOOHeight = ( m_height!= 0 ) ? 1.0f / m_height : 1.0f;

	*pMinU = 0.5f * flOOWidth; 
	*pMinV = 0.5f * flOOHeight;
	*pMaxU = (*pMaxU) - (*pMinU);
	*pMaxV = (*pMaxV) - (*pMinV);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEngineSprite::SetColor( float r, float g, float b )
{
	Assert( (r >= 0.0) && (g >= 0.0) && (b >= 0.0) );
	Assert( (r <= 1.0) && (g <= 1.0) && (b <= 1.0) );
	m_hudSpriteColor[0] = r;
	m_hudSpriteColor[1] = g;
	m_hudSpriteColor[2] = b;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEngineSprite::GetHUDSpriteColor( float* color )
{
	VectorCopy( m_hudSpriteColor, color );
}


//-----------------------------------------------------------------------------
// Returns the material 
//-----------------------------------------------------------------------------
static unsigned int frameCache = 0;
IMaterial *CEngineSprite::GetMaterial( RenderMode_t nRenderMode, int nFrame ) 
{
	if ( nRenderMode == kRenderNone || nRenderMode == kRenderEnvironmental )
		return NULL;

	if ( IsVideo() )
	{
		m_VideoMaterial->SetFrame( nFrame );
	}
	
	IMaterial *pMaterial = m_material[nRenderMode];
	if ( pMaterial )
	{
		IMaterialVar* pFrameVar = pMaterial->FindVarFast( "$frame", &frameCache );
		if ( pFrameVar )
		{
			pFrameVar->SetIntValue( nFrame );
		}
	}

	return pMaterial;
} 

void CEngineSprite::SetFrame( RenderMode_t nRenderMode, int nFrame )
{
	if ( IsVideo() )
	{
		m_VideoMaterial->SetFrame( nFrame );
		return;
	}


	IMaterial *pMaterial = m_material[nRenderMode];
	if ( !pMaterial )
		return;

	IMaterialVar* pFrameVar = pMaterial->FindVarFast( "$frame", &frameCache );
	if ( pFrameVar )
	{
		pFrameVar->SetIntValue( nFrame );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CEngineSprite::GetOrientation( void )
{
	return m_orientation;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEngineSprite::UnloadMaterial( void )
{
	for ( int i = 0; i < kRenderModeCount; ++i )
	{
		if( m_material[i] )
		{
			m_material[i]->DecrementReferenceCount();
			m_material[i] = NULL;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEngineSprite::DrawFrame( RenderMode_t nRenderMode, int frame, int x, int y, const wrect_t *prcSubRect )
{
	DrawFrameOfSize( nRenderMode, frame, x, y, GetWidth(), GetHeight(), prcSubRect );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frame - 
//			x - 
//			y - 
//			*prcSubRect - 
//-----------------------------------------------------------------------------
void CEngineSprite::DrawFrameOfSize( RenderMode_t nRenderMode, int frame, int x, int y, int iWidth, int iHeight, const wrect_t *prcSubRect )
{
	// FIXME: If we ever call this with AVIs, need to have it call GetTexCoordRange and make that work
	Assert( !IsVideo() );
	float fLeft = 0;
	float fRight = 1;
	float fTop = 0;
	float fBottom = 1;

	if ( prcSubRect )
	{
		AdjustSubRect( this, frame, &fLeft, &fRight, &fTop, &fBottom, &iWidth, &iHeight, prcSubRect );
	}

	if ( giScissorTest && !Scissor( x, y, iWidth, iHeight, fLeft, fTop, fRight, fBottom ) )
		return;

	SetFrame( nRenderMode, frame );

	CMatRenderContextPtr pRenderContext( materials );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, GetMaterial( nRenderMode ) );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	float color[3];
	GetHUDSpriteColor( color );
	
	meshBuilder.Color3fv( color );
	meshBuilder.TexCoord2f( 0, fLeft, fTop );
	meshBuilder.Position3f( x, y, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3fv( color );
	meshBuilder.TexCoord2f( 0, fRight, fTop );
	meshBuilder.Position3f( x + iWidth, y, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3fv( color );
	meshBuilder.TexCoord2f( 0, fRight, fBottom );
	meshBuilder.Position3f( x + iWidth, y + iHeight, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3fv( color );
	meshBuilder.TexCoord2f( 0, fLeft, fBottom );
	meshBuilder.Position3f( x, y + iHeight, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}