//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
// identifier was truncated to '255' characters in the debug information
#pragma warning(disable: 4786)

#include "proxyentity.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "bitmap/tgaloader.h"
#include "view.h"
#include "datacache/idatacache.h"
#include "materialsystem/imaterial.h"
#include "vtf/vtf.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CCamoMaterialProxy;

class CCamoTextureRegen : public ITextureRegenerator
{
public:
	CCamoTextureRegen( CCamoMaterialProxy *pProxy ) : m_pProxy(pProxy) {} 
	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect );
	virtual void Release() {}

private:
	CCamoMaterialProxy *m_pProxy;
};

class CCamoMaterialProxy : public CEntityMaterialProxy
{
public:
	CCamoMaterialProxy();
	virtual ~CCamoMaterialProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind(C_BaseEntity *pC_BaseEntity );
	virtual IMaterial *GetMaterial();

	// Procedurally generates the camo texture...
	void GenerateCamoTexture( ITexture* pTexture, IVTFTexture *pVTFTexture );

protected:
#if 0
	virtual void SetInstanceDataSize( int size );
	virtual void *FindInstanceData( C_BaseEntity *pEntity );
	virtual void *AllocateInstanceData( C_BaseEntity *pEntity );
#endif

private:
	void LoadCamoPattern( void );
	void GenerateRandomPointsInNormalizedCube( void );
	void GetColors( Vector &lighting, Vector &base, int index, 
						  const Vector &boxMin, const Vector &boxExtents, 
						  const Vector &forward, const Vector &right, const Vector &up,
						  const Vector& entityPosition );
	// this needs to go in a base class

private:
#if 0
	// stuff that needs to be in a base class.
	struct InstanceData_t
	{
		C_BaseEntity *pEntity;
		void *data;
		struct InstanceData_s *next;
	};
	
	struct CamoInstanceData_t
	{
		int dummy;
	};
#endif
	
	unsigned char *m_pCamoPatternImage;

#if 0
	int m_InstanceDataSize;
	InstanceData_t *m_InstanceDataListHead;
#endif

	IMaterial *m_pMaterial;
	IMaterialVar *m_pCamoTextureVar;
	IMaterialVar *m_pCamoPatternTextureVar;
	Vector *m_pointsInNormalizedBox; // [m_CamoPatternNumColors]

	int m_CamoPatternNumColors;
	int m_CamoPatternWidth;
	int m_CamoPatternHeight;
#if 0
	cache_user_t m_camoImageDataCache;
#endif
	unsigned char m_CamoPalette[256][3];
	// these represent that part of the entitiy's bounding box that we 
	// want to cast rays through to get colors for the camo
	Vector m_SubBoundingBoxMin; // normalized
	Vector m_SubBoundingBoxMax; // normalized

	CCamoTextureRegen m_TextureRegen;
	C_BaseEntity *m_pEnt;
};


void CCamoTextureRegen::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect )
{
	m_pProxy->GenerateCamoTexture( pTexture, pVTFTexture );
}


#pragma warning (disable:4355)

CCamoMaterialProxy::CCamoMaterialProxy() : m_TextureRegen(this)
{
#if 0
	m_InstanceDataSize = 0;
#endif
#if 0
	memset( &m_camoImageDataCache, 0,sizeof( m_camoImageDataCache ) );
#endif
	m_pointsInNormalizedBox = NULL;
#if 0
	m_InstanceDataListHead = NULL;
#endif
	m_pCamoPatternImage = NULL;
	m_pMaterial = NULL;
	m_pCamoTextureVar = NULL;
	m_pCamoPatternTextureVar = NULL;
	m_pointsInNormalizedBox = NULL;
	m_pEnt = NULL;
}

#pragma warning (default:4355)

CCamoMaterialProxy::~CCamoMaterialProxy()
{
#if 0
	InstanceData_t *curr = m_InstanceDataListHead;
	while( curr )
	{
		InstanceData_t *next;
		next = curr->next;
		delete curr;
		curr = next;
	}
	m_InstanceDataListHead = NULL;
#endif

	// Disconnect the texture regenerator...
	if (m_pCamoTextureVar)
	{
		ITexture *pCamoTexture = m_pCamoTextureVar->GetTextureValue();
		if (pCamoTexture)
			pCamoTexture->SetTextureRegenerator( NULL );
	}

	delete m_pCamoPatternImage;
	delete m_pointsInNormalizedBox;
}


#if 0
void CCamoMaterialProxy::SetInstanceDataSize( int size )
{
	m_InstanceDataSize = size;
}
#endif

#if 0
void *CCamoMaterialProxy::FindInstanceData( C_BaseEntity *pEntity )
{
	InstanceData_t *curr = m_InstanceDataListHead;
	while( curr )
	{
		if( pEntity == curr->pEntity )
		{
			return curr->data;
		}
		curr = curr->next;
	}
	return NULL;
}
#endif

#if 0
void *CCamoMaterialProxy::AllocateInstanceData( C_BaseEntity *pEntity )
{
	InstanceData_t *newData = new InstanceData_t;
	newData->pEntity = pEntity;
	newData->next = m_InstanceDataListHead;
	m_InstanceDataListHead = newData;
	newData->data = new unsigned char[m_InstanceDataSize];
	return newData->data;
}
#endif

bool CCamoMaterialProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	return false; // hack!  Need to make sure that the TGA loader has a valid filesystem before trying
			// to load the camo pattern.

#if 0
	// set how big our instance data is.
	SetInstanceDataSize( sizeof( CamoInstanceData_t ) );
#endif
	// remember what material we belong to.
	m_pMaterial = pMaterial;
	// get pointers to material vars.
	bool found;
	m_pCamoTextureVar = m_pMaterial->FindVar( "$baseTexture", &found );
	if( !found )
	{
		m_pCamoTextureVar = NULL;
		return false;
	}
	ITexture *pCamoTexture = m_pCamoTextureVar->GetTextureValue();
	if (pCamoTexture)
		pCamoTexture->SetTextureRegenerator( &m_TextureRegen );
	
	// Need to get the palettized texture to create the procedural texture from
	// somewhere.
	m_pCamoPatternTextureVar = m_pMaterial->FindVar( "$camoPatternTexture", &found );
	if( !found )
	{
		m_pCamoTextureVar = NULL;
		return false;
	}
	
	IMaterialVar *subBoundingBoxMinVar, *subBoundingBoxMaxVar;

	subBoundingBoxMinVar = m_pMaterial->FindVar( "$camoBoundingBoxMin", &found, false );
	if( !found )
	{
		m_SubBoundingBoxMin = Vector( 0.0f, 0.0f, 0.0f );
	}
	else
	{
		subBoundingBoxMinVar->GetVecValue( m_SubBoundingBoxMin.Base(), 3 );
	}

	subBoundingBoxMaxVar = m_pMaterial->FindVar( "$camoBoundingBoxMax", &found, false );
	if( !found )
	{
		m_SubBoundingBoxMax = Vector( 1.0f, 1.0f, 1.0f );
	}
	else
	{
		subBoundingBoxMaxVar->GetVecValue( m_SubBoundingBoxMax.Base(), 3 );
	}
	
	LoadCamoPattern();
	GenerateRandomPointsInNormalizedCube();

	return true;
}

void CCamoMaterialProxy::GetColors( Vector &diffuseColor, Vector &baseColor, int index, 
										  const Vector &boxMin, const Vector &boxExtents, 
										  const Vector &forward, const Vector &right, const Vector &up,
										  const Vector& entityPosition )
{
	Vector position, transformedPosition;
	
	// hack
//	m_pointsInNormalizedBox[index] = Vector( 0.5f, 0.5f, 1.0f );
	
	position[0] = m_pointsInNormalizedBox[index][0] * boxExtents[0] + boxMin[0];
	position[1] = m_pointsInNormalizedBox[index][1] * boxExtents[1] + boxMin[1];
	position[2] = m_pointsInNormalizedBox[index][2] * boxExtents[2] + boxMin[2];
	transformedPosition[0] = right[0] * position[0] + forward[0] * position[1] + up[0] * position[2];
	transformedPosition[1] = right[1] * position[0] + forward[1] * position[1] + up[1] * position[2];
	transformedPosition[2] = right[2] * position[0] + forward[2] * position[1] + up[2] * position[2];
	transformedPosition = transformedPosition + entityPosition;
	Vector direction = transformedPosition - CurrentViewOrigin();
	VectorNormalize( direction );
	direction = direction * ( COORD_EXTENT * 1.74f );
	Vector endPoint = position + direction;
	
	// baseColor is already in gamma space
//	engine->TraceLineMaterialAndLighting( g_vecInstantaneousRenderOrigin, endPoint, diffuseColor, baseColor );
	engine->TraceLineMaterialAndLighting( transformedPosition, endPoint, diffuseColor, baseColor );

	// hack - optimize! - convert from linear to gamma space - this should be hidden
	diffuseColor[0] = pow( diffuseColor[0], 1.0f / 2.2f );
	diffuseColor[1] = pow( diffuseColor[1], 1.0f / 2.2f );
	diffuseColor[2] = pow( diffuseColor[2], 1.0f / 2.2f );

#if 0
	Msg( "%f %f %f\n", 
		diffuseColor[0], 
		diffuseColor[1], 
		diffuseColor[2] );
#endif
	
#if 0
	float max;
	max = diffuseColor[0];
	if( diffuseColor[1] > max )
	{
		max = diffuseColor[1];
	}
	if( diffuseColor[2] > max )
	{
		max = diffuseColor[2];
	}
	if( max > 1.0f )
	{
		max = 1.0f / max;
		diffuseColor = diffuseColor * max;
	}
#else
	if( diffuseColor[0] > 1.0f )
	{
		diffuseColor[0] = 1.0f;
	}
	if( diffuseColor[1] > 1.0f )
	{
		diffuseColor[1] = 1.0f;
	}
	if( diffuseColor[2] > 1.0f )
	{
		diffuseColor[2] = 1.0f;
	}
#endif
	// hack
	//baseColor = Vector( 1.0f, 1.0f, 1.0f );
	//diffuseColor = Vector( 1.0f, 1.0f, 1.0f );
}


//-----------------------------------------------------------------------------
// Procedurally generates the camo texture...
//-----------------------------------------------------------------------------
void CCamoMaterialProxy::GenerateCamoTexture( ITexture* pTexture, IVTFTexture *pVTFTexture )
{
	if (!m_pEnt)
		return;

#if 0
	CamoInstanceData_t *pInstanceData;
	pInstanceData = ( CamoInstanceData_t * )FindInstanceData( pEnt );
	if( !pInstanceData )
	{
		pInstanceData = ( CamoInstanceData_t * )AllocateInstanceData( pEnt );
		if( !pInstanceData )
		{
			return;
		}
		// init the instance data
	}
#endif

	Vector entityPosition;
	entityPosition = m_pEnt->GetAbsOrigin();

	QAngle entityAngles;
	entityAngles = m_pEnt->GetAbsAngles();

	// Get the bounding box for the entity
	Vector mins, maxs;
	mins = m_pEnt->WorldAlignMins();
	maxs = m_pEnt->WorldAlignMaxs();
	
	Vector traceDirection;
	Vector traceEnd;
	trace_t	traceResult;
	
	Vector forward, right, up;
	AngleVectors( entityAngles, &forward, &right, &up );
	
	Vector position, transformedPosition;
	Vector maxsMinusMins = maxs - mins;

	Vector diffuseColor[256];
	Vector baseColor;

	unsigned char camoPalette[256][3];
	// Calculate the camo palette
	//Msg( "start of loop\n" );
	int i;
	for( i = 0; i < m_CamoPatternNumColors; i++ )
	{
		GetColors( diffuseColor[i], baseColor, i,
			mins, maxsMinusMins, forward, right, up, entityPosition );
#if 1
		camoPalette[i][0] = diffuseColor[i][0] * baseColor[0] * 255.0f;
		camoPalette[i][1] = diffuseColor[i][1] * baseColor[1] * 255.0f;
		camoPalette[i][2] = diffuseColor[i][2] * baseColor[2] * 255.0f;
#endif
#if 0
		camoPalette[i][0] = baseColor[0] * 255.0f;
		camoPalette[i][1] = baseColor[1] * 255.0f;
		camoPalette[i][2] = baseColor[2] * 255.0f;
#endif
#if 0
		camoPalette[i][0] = diffuseColor[i][0] * 255.0f;
		camoPalette[i][1] = diffuseColor[i][1] * 255.0f;
		camoPalette[i][2] = diffuseColor[i][2] * 255.0f;
#endif
	}
	
	int width = pVTFTexture->Width();
	int height = pVTFTexture->Height();
	if( width != m_CamoPatternWidth || height != m_CamoPatternHeight )
	{
		return;
	}
	
	unsigned char *imageData = pVTFTexture->ImageData( 0, 0, 0 );
	enum ImageFormat imageFormat = pVTFTexture->Format();
	if( imageFormat != IMAGE_FORMAT_RGB888 )
	{
		return;
	}
	// optimize
#if 1
	int x, y;
	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++ )
		{
			int offset = 3 * ( x + y * width );
			assert( offset < width * height * 3 );
			int paletteID = m_pCamoPatternImage[x + y * width];
			assert( paletteID < 256 );
#if 1
			imageData[offset + 0] = camoPalette[paletteID][0];
			imageData[offset + 1] = camoPalette[paletteID][1];
			imageData[offset + 2] = camoPalette[paletteID][2];
#else
			imageData[offset] = 255;
			imageData[offset + 1] = 0;
			imageData[offset + 2] = 0;
#endif
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Called when the texture is bound...
//-----------------------------------------------------------------------------
void CCamoMaterialProxy::OnBind( C_BaseEntity *pEntity )
{
	if( !m_pCamoTextureVar )
	{
		return;
	}
	
	m_pEnt = pEntity;	
	ITexture *pCamoTexture = m_pCamoTextureVar->GetTextureValue();
	pCamoTexture->Download();

	// Mark it so it doesn't get regenerated on task switch
	m_pEnt = NULL;
}

void CCamoMaterialProxy::LoadCamoPattern( void )
{
#if 0
	// hack - need to figure out a name to attach that isn't too long.
	m_pCamoPatternImage = 
		( unsigned char * )datacache->FindByName( &m_camoImageDataCache, "camopattern" );
	
	if( m_pCamoPatternImage )
	{
		// is already in the cache.
		return m_pCamoPatternImage;
	}
#endif
	
	enum ImageFormat indexImageFormat;
	int indexImageSize;
#ifndef _XBOX
	float dummyGamma;
	if( !TGALoader::GetInfo( m_pCamoPatternTextureVar->GetStringValue(), 
		&m_CamoPatternWidth, &m_CamoPatternHeight, &indexImageFormat, &dummyGamma ) )
	{
		//Warning( "Can't get tga info for hl2/materials/models/combine_elite/camo7paletted.tga for camo material\n" );
		m_pCamoTextureVar = NULL;
		return;
	}
#else
	// xboxissue - no tga support, why implemented this way
	Assert( 0 );
	m_pCamoTextureVar = NULL;
	return;
#endif
	
	if( indexImageFormat != IMAGE_FORMAT_I8 )
	{
		//	Warning( "Camo material texture hl2/materials/models/combine_elite/camo7paletted.tga must be 8-bit greyscale\n" );
		m_pCamoTextureVar = NULL;
		return;
	}
	
	indexImageSize = ImageLoader::GetMemRequired( m_CamoPatternWidth, m_CamoPatternHeight, 1, indexImageFormat, false );
#if 0
	m_pCamoPatternImage = ( unsigned char * )
		datacache->Alloc( &m_camoImageDataCache, indexImageSize, "camopattern" );
#endif
	m_pCamoPatternImage = ( unsigned char * )new unsigned char[indexImageSize];
	if( !m_pCamoPatternImage )
	{
		m_pCamoTextureVar = NULL;
		return;
	}
	
#ifndef _XBOX
	if( !TGALoader::Load( m_pCamoPatternImage, m_pCamoPatternTextureVar->GetStringValue(),
		m_CamoPatternWidth, m_CamoPatternHeight, IMAGE_FORMAT_I8, dummyGamma, false ) )
	{
		//			Warning( "camo texture hl2/materials/models/combine_elite/camo7paletted.tga must be grey-scale" );
		m_pCamoTextureVar = NULL;
		return;
	}
#else
	// xboxissue - no tga support, why is the camo done this way?
	Assert( 0 );
#endif
	
	bool colorUsed[256];
	int colorRemap[256];
	// count the number of colors used in the image.
	int i;
	for( i = 0; i < 256; i++ )
	{
		colorUsed[i] = false;
	}
	for( i = 0; i < indexImageSize; i++ )
	{
		colorUsed[m_pCamoPatternImage[i]] = true;
	}
	m_CamoPatternNumColors = 0;
	for( i = 0; i < 256; i++ )
	{
		if( colorUsed[i] )
		{
			colorRemap[i] = m_CamoPatternNumColors;
			m_CamoPatternNumColors++;
		}
	}
	// remap the color to the beginning of the palette.
	for( i = 0; i < indexImageSize; i++ )
	{
		m_pCamoPatternImage[i] = colorRemap[m_pCamoPatternImage[i]];
		// hack
//		m_pCamoPatternImage[i] = 0;
	}
}

void CCamoMaterialProxy::GenerateRandomPointsInNormalizedCube( void )
{
	m_pointsInNormalizedBox = new Vector[m_CamoPatternNumColors];
	if( !m_pointsInNormalizedBox )
	{
		m_pCamoTextureVar = NULL;
		return;
	}
	
	int i;
	for( i = 0; i < m_CamoPatternNumColors; i++ )
	{
		m_pointsInNormalizedBox[i][0] = random->RandomFloat( m_SubBoundingBoxMin[0], m_SubBoundingBoxMax[0] );
		m_pointsInNormalizedBox[i][1] = random->RandomFloat( m_SubBoundingBoxMin[1], m_SubBoundingBoxMax[1] );
		m_pointsInNormalizedBox[i][2] = random->RandomFloat( m_SubBoundingBoxMin[2], m_SubBoundingBoxMax[2] );
	}
}

IMaterial *CCamoMaterialProxy::GetMaterial()
{
	return m_pMaterial;
}

EXPOSE_INTERFACE( CCamoMaterialProxy, IMaterialProxy, "Camo" IMATERIAL_PROXY_INTERFACE_VERSION );
