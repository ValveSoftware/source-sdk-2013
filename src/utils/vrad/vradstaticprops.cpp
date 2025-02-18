//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//
// This file contains code to allow us to associate client data with bsp leaves.
//
//=============================================================================//

#include "vrad.h"
#include "mathlib/vector.h"
#include "UtlBuffer.h"
#include "utlvector.h"
#include "GameBSPFile.h"
#include "BSPTreeData.h"
#include "VPhysics_Interface.h"
#include "Studio.h"
#include "Optimize.h"
#include "Bsplib.h"
#include "CModel.h"
#include "PhysDll.h"
#include "phyfile.h"
#include "collisionutils.h"
#include "tier1/KeyValues.h"
#include "pacifier.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/hardwareverts.h"
#include "materialsystem/hardwaretexels.h"
#include "byteswap.h"
#include "mpivrad.h"
#include "vtf/vtf.h"
#include "tier1/utldict.h"
#include "tier1/utlsymbol.h"
#include "bitmap/tgawriter.h"

#include "messbuf.h"
#include "vmpi.h"
#include "vmpi_distribute_work.h"


#define ALIGN_TO_POW2(x,y) (((x)+(y-1))&~(y-1))

// identifies a vertex embedded in solid
// lighting will be copied from nearest valid neighbor
struct badVertex_t
{
	int		m_ColorVertex;
	Vector	m_Position;
	Vector	m_Normal;
};

// a final colored vertex
struct colorVertex_t
{
	Vector	m_Color;
	Vector	m_Position;
	bool	m_bValid;
};

// a texel suitable for a model
struct colorTexel_t
{
	Vector		m_Color;
	Vector		m_WorldPosition;
	Vector		m_WorldNormal;
	float		m_fDistanceToTri; // If we are outside of the triangle, how far away is it?
	bool		m_bValid;
	bool		m_bPossiblyInteresting;

};

class CComputeStaticPropLightingResults
{
public:
	~CComputeStaticPropLightingResults()
	{
		m_ColorVertsArrays.PurgeAndDeleteElements();
		m_ColorTexelsArrays.PurgeAndDeleteElements();
	}
	
	CUtlVector< CUtlVector<colorVertex_t>* > m_ColorVertsArrays;
	CUtlVector< CUtlVector<colorTexel_t>* > m_ColorTexelsArrays;
};

//-----------------------------------------------------------------------------
struct Rasterizer
{
	struct Location
	{
		Vector barycentric;
		Vector2D uv;
		bool   insideTriangle;
	};

	Rasterizer(Vector2D t0, Vector2D t1, Vector2D t2, size_t resX, size_t resY)
	: mT0(t0)
	, mT1(t1)
	, mT2(t2)
	, mResX(resX)
	, mResY(resY)
	, mUvStepX(1.0f / resX)
	, mUvStepY(1.0f / resY)
	{ 
		Build();
	}

	CUtlVector< Location >::iterator begin() { return mRasterizedLocations.begin(); }
	CUtlVector< Location >::iterator end() { return mRasterizedLocations.end(); }

	void Build();

	inline size_t GetRow(float y) const { return size_t(y * mResY); }
	inline size_t GetCol(float x) const { return size_t(x * mResX); }

	inline size_t GetLinearPos( const CUtlVector< Location >::iterator& it ) const
	{
		// Given an iterator, return what the linear position in the buffer would be for the data.
		return (size_t)(GetRow(it->uv.y) * mResX)
			 + (size_t)(GetCol(it->uv.x));
	}
	
private:
	const Vector2D mT0, mT1, mT2;
	const size_t mResX, mResY;
	const float mUvStepX, mUvStepY;

	// Right now, we just fill this out and directly iterate over it. 
	// It could be large. This is a memory/speed tradeoff. We could instead generate them
	// on demand. 
	CUtlVector< Location > mRasterizedLocations;
};

//-----------------------------------------------------------------------------
inline Vector ComputeBarycentric( Vector2D _edgeC, Vector2D _edgeA, Vector2D _edgeB, float _dAA, float _dAB, float _dBB, float _invDenom )
{
	float dCA = _edgeC.Dot(_edgeA);
	float dCB = _edgeC.Dot(_edgeB);
	
	Vector retVal;
	retVal.y = (_dBB * dCA - _dAB * dCB) * _invDenom;
	retVal.z = (_dAA * dCB - _dAB * dCA) * _invDenom;
	retVal.x = 1.0f - retVal.y - retVal.z;

	return retVal;
}

//-----------------------------------------------------------------------------
void Rasterizer::Build()
{
	// For now, use the barycentric method. It's easy, I'm lazy. 
	// We can optimize later if it's a performance issue.
	const float baseX = mUvStepX / 2.0f;
	const float baseY = mUvStepY / 2.0f;


	float fMinX = min(min(mT0.x, mT1.x), mT2.x);
	float fMinY = min(min(mT0.y, mT1.y), mT2.y);
	float fMaxX = max(max(mT0.x, mT1.x), mT2.x);
	float fMaxY = max(max(mT0.y, mT1.y), mT2.y);

	// Degenerate. Consider warning about these, but otherwise no problem.
	if (fMinX == fMaxX || fMinY == fMaxY)
		return;

	// Clamp to 0..1
	fMinX = max(0, fMinX);
	fMinY = max(0, fMinY);
	fMaxX = min(1.0f, fMaxX);
	fMaxY = min(1.0f, fMaxY);

	// We puff the interesting area up by 1 so we can hit an inflated region for the necessary bilerp data.
	// If we wanted to support better texturing (almost definitely unnecessary), we'd change this to a larger size.
	const int kFilterSampleRadius = 1;

	int iMinX = GetCol(fMinX) - kFilterSampleRadius;
	int iMinY = GetRow(fMinY) - kFilterSampleRadius;
	int iMaxX = GetCol(fMaxX) + 1 + kFilterSampleRadius;
	int iMaxY = GetRow(fMaxY) + 1 + kFilterSampleRadius;

	// Clamp to valid texture (integer) locations
	iMinX = max(0, iMinX);
	iMinY = max(0, iMinY);
	iMaxX = min(iMaxX, mResX - 1);
	iMaxY = min(iMaxY, mResY - 1);

	// Set the size to be as expected. 
	// TODO: Pass this in from outside to minimize allocations
	int count = (iMaxY - iMinY + 1) 
		      * (iMaxX - iMinX + 1);
	mRasterizedLocations.EnsureCount(count);
	memset( mRasterizedLocations.Base(), 0, mRasterizedLocations.Count() * sizeof( Location ) );
	
	// Computing Barycentrics adapted from here http://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
	Vector2D edgeA = mT1 - mT0;
	Vector2D edgeB = mT2 - mT0;

	float dAA = edgeA.Dot(edgeA);
	float dAB = edgeA.Dot(edgeB);
	float dBB = edgeB.Dot(edgeB);
	float invDenom = 1.0f / (dAA * dBB - dAB * dAB);

	int linearPos = 0; 
	for (int j = iMinY; j <= iMaxY; ++j) {
		for (int i = iMinX; i <= iMaxX; ++i) {
			Vector2D testPt( i * mUvStepX + baseX, j * mUvStepY + baseY );
			Vector barycentric = ComputeBarycentric( testPt - mT0, edgeA, edgeB, dAA, dAB, dBB, invDenom );

			// Test whether the point is inside the triangle. 
			// MCJOHNTODO: Edge rules and whatnot--right now we re-rasterize points on the edge.
			Location& newLoc = mRasterizedLocations[linearPos++];
			newLoc.barycentric = barycentric;
			newLoc.uv = testPt;

			newLoc.insideTriangle = (barycentric.x >= 0.0f && barycentric.x <= 1.0f && barycentric.y >= 0.0f && barycentric.y <= 1.0f && barycentric.z >= 0.0f && barycentric.z <= 1.0f);
		}
	}
}


//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
CUtlSymbolTable g_ForcedTextureShadowsModels;

// DON'T USE THIS FROM WITHIN A THREAD.  THERE IS A THREAD CONTEXT CREATED 
// INSIDE PropTested_t.  USE THAT INSTEAD.
IPhysicsCollision *s_pPhysCollision = NULL;

static void ConvertTexelDataToTexture(unsigned int _resX, unsigned int _resY, ImageFormat _destFmt, const CUtlVector<colorTexel_t>& _srcTexels, CUtlMemory<byte>* _outTexture);

// Such a monstrosity. :(
static void GenerateLightmapSamplesForMesh( const matrix3x4_t& _matPos, const matrix3x4_t& _matNormal, int _iThread, int _skipProp, int _nFlags, int _lightmapResX, int _lightmapResY, 
											studiohdr_t* _pStudioHdr, mstudiomodel_t* _pStudioModel, OptimizedModel::ModelHeader_t* _pVtxModel, int _meshID, 
											CComputeStaticPropLightingResults *_pResults );

// Debug function, converts lightmaps to linear space then dumps them out. 
// TODO: Write out the file in a .dds instead of a .tga, in whatever format we're supposed to use.
static void DumpLightmapLinear( const char* _dstFilename, const CUtlVector<colorTexel_t>& _srcTexels, int _width, int _height );


//-----------------------------------------------------------------------------
// Vrad's static prop manager
//-----------------------------------------------------------------------------

class CVradStaticPropMgr : public IVradStaticPropMgr
{
public:
	// constructor, destructor
	CVradStaticPropMgr();
	virtual ~CVradStaticPropMgr();

	// methods of IStaticPropMgr
	void Init();
	void Shutdown();

	// iterate all the instanced static props and compute their vertex lighting
	void ComputeLighting( int iThread );

private:
	// VMPI stuff.
	static void VMPI_ProcessStaticProp_Static( int iThread, uint64 iStaticProp, MessageBuffer *pBuf );
	static void VMPI_ReceiveStaticPropResults_Static( uint64 iStaticProp, MessageBuffer *pBuf, int iWorker );
	void VMPI_ProcessStaticProp( int iThread, int iStaticProp, MessageBuffer *pBuf );
	void VMPI_ReceiveStaticPropResults( int iStaticProp, MessageBuffer *pBuf, int iWorker );
	
	// local thread version
	static void ThreadComputeStaticPropLighting( int iThread, void *pUserData );
	void ComputeLightingForProp( int iThread, int iStaticProp );

	// Methods associated with unserializing static props
	void UnserializeModelDict( CUtlBuffer& buf );
	void UnserializeModels( CUtlBuffer& buf );
	void UnserializeStaticProps();

	// Creates a collision model
	void CreateCollisionModel( char const* pModelName );

private:
	// Unique static prop models
	struct StaticPropDict_t
	{
		vcollide_t		m_loadedModel;
		CPhysCollide*	m_pModel;
		Vector			m_Mins;			// Bounding box is in local coordinates
		Vector			m_Maxs;
		studiohdr_t*	m_pStudioHdr;
		CUtlBuffer		m_VtxBuf;
		CUtlVector<int>	m_textureShadowIndex;	// each texture has an index if this model casts texture shadows
		CUtlVector<int>	m_triangleMaterialIndex;// each triangle has an index if this model casts texture shadows
	};

	struct MeshData_t
	{
		CUtlVector<Vector>	m_VertexColors;
		CUtlMemory<byte>	m_TexelsEncoded;
		int					m_nLod;
	};

	// A static prop instance
	struct CStaticProp
	{
		Vector					m_Origin;
		QAngle					m_Angles;
		Vector					m_mins;
		Vector					m_maxs;
		Vector					m_LightingOrigin;
		int						m_ModelIdx;
		BSPTreeDataHandle_t		m_Handle;
		CUtlVector<MeshData_t>	m_MeshData;
		int                     m_Flags;
		bool					m_bLightingOriginValid;

		// Note that all lightmaps for a given prop share the same resolution (and format)--and there can be multiple lightmaps
		// per prop (if there are multiple pieces--the watercooler is an example).
		// This is effectively because there's not a good way in hammer for a prop to say "this should be the resolution
		// of each of my sub-pieces."
		ImageFormat				m_LightmapImageFormat;
		unsigned int			m_LightmapImageWidth;
		unsigned int			m_LightmapImageHeight;

	};

	// Enumeration context
	struct EnumContext_t
	{
		PropTested_t* m_pPropTested;
		Ray_t const* m_pRay;
	};

	// The list of all static props
	CUtlVector <StaticPropDict_t>	m_StaticPropDict;
	CUtlVector <CStaticProp>		m_StaticProps;

	bool m_bIgnoreStaticPropTrace;

	void ComputeLighting( CStaticProp &prop, int iThread, int prop_index, CComputeStaticPropLightingResults *pResults );
	void ApplyLightingToStaticProp( int iStaticProp, CStaticProp &prop, const CComputeStaticPropLightingResults *pResults );

	void SerializeLighting();
	void AddPolysForRayTrace();
	void BuildTriList( CStaticProp &prop );
};


//-----------------------------------------------------------------------------
// Expose IVradStaticPropMgr to vrad
//-----------------------------------------------------------------------------

static CVradStaticPropMgr	g_StaticPropMgr;
IVradStaticPropMgr* StaticPropMgr()
{
	return &g_StaticPropMgr;
}


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CVradStaticPropMgr::CVradStaticPropMgr()
{
	// set to ignore static prop traces
	m_bIgnoreStaticPropTrace = false;
}

CVradStaticPropMgr::~CVradStaticPropMgr()
{
}

//-----------------------------------------------------------------------------
// Makes sure the studio model is a static prop
//-----------------------------------------------------------------------------

bool IsStaticProp( studiohdr_t* pHdr )
{
	if (!(pHdr->flags & STUDIOHDR_FLAGS_STATIC_PROP))
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Load a file into a Utlbuf
//-----------------------------------------------------------------------------
static bool LoadFile( char const* pFileName, CUtlBuffer& buf )
{
	if ( !g_pFullFileSystem )
		return false;

	return g_pFullFileSystem->ReadFile( pFileName, NULL, buf );
}


//-----------------------------------------------------------------------------
// Constructs the file name from the model name
//-----------------------------------------------------------------------------
static char const* ConstructFileName( char const* pModelName )
{
	static char buf[1024];
	sprintf( buf, "%s%s", gamedir, pModelName );
	return buf;
}


//-----------------------------------------------------------------------------
// Computes a convex hull from a studio mesh
//-----------------------------------------------------------------------------
static CPhysConvex* ComputeConvexHull( mstudiomesh_t* pMesh, studiohdr_t *pStudioHdr  )
{
	const mstudio_meshvertexdata_t *vertData = pMesh->GetVertexData( (void *)pStudioHdr );
	Assert( vertData ); // This can only return NULL on X360 for now

	// Generate a list of all verts in the mesh
	Vector** ppVerts = (Vector**)_alloca(pMesh->numvertices * sizeof(Vector*) );
	for (int i = 0; i < pMesh->numvertices; ++i)
	{
		ppVerts[i] = vertData->Position(i);
	}

	// Generate a convex hull from the verts
	return s_pPhysCollision->ConvexFromVerts( ppVerts, pMesh->numvertices );
}


//-----------------------------------------------------------------------------
// Computes a convex hull from the studio model
//-----------------------------------------------------------------------------
CPhysCollide* ComputeConvexHull( studiohdr_t* pStudioHdr )
{
	CUtlVector<CPhysConvex*>	convexHulls;

	for (int body = 0; body < pStudioHdr->numbodyparts; ++body )
	{
		mstudiobodyparts_t *pBodyPart = pStudioHdr->pBodypart( body );
		for( int model = 0; model < pBodyPart->nummodels; ++model )
		{
			mstudiomodel_t *pStudioModel = pBodyPart->pModel( model );
			for( int mesh = 0; mesh < pStudioModel->nummeshes; ++mesh )
			{
				// Make a convex hull for each mesh
				// NOTE: This won't work unless the model has been compiled
				// with $staticprop
				mstudiomesh_t *pStudioMesh = pStudioModel->pMesh( mesh );
				convexHulls.AddToTail( ComputeConvexHull( pStudioMesh, pStudioHdr ) );
			}
		}
	}

	// Convert an array of convex elements to a compiled collision model
	// (this deletes the convex elements)
	return s_pPhysCollision->ConvertConvexToCollide( convexHulls.Base(), convexHulls.Size() );
}


//-----------------------------------------------------------------------------
// Load studio model vertex data from a file...
//-----------------------------------------------------------------------------

bool LoadStudioModel( char const* pModelName, CUtlBuffer& buf )
{
	// No luck, gotta build it	
	// Construct the file name...
	if (!LoadFile( pModelName, buf ))
	{
		Warning("Error! Unable to load model \"%s\"\n", pModelName );
		return false;
	}

	// Check that it's valid
	if (strncmp ((const char *) buf.PeekGet(), "IDST", 4) &&
		strncmp ((const char *) buf.PeekGet(), "IDAG", 4))
	{
		Warning("Error! Invalid model file \"%s\"\n", pModelName );
		return false;
	}

	studiohdr_t* pHdr = (studiohdr_t*)buf.PeekGet();

	Studio_ConvertStudioHdrToNewVersion( pHdr );

	if (pHdr->version != STUDIO_VERSION)
	{
		Warning("Error! Invalid model version \"%s\"\n", pModelName );
		return false;
	}

	if (!IsStaticProp(pHdr))
	{
		Warning("Error! To use model \"%s\"\n"
			"      as a static prop, it must be compiled with $staticprop!\n", pModelName );
		return false;
	}

	// ensure reset
	pHdr->pVertexBase = NULL;
	pHdr->pIndexBase  = NULL;

	return true;
}

bool LoadStudioCollisionModel( char const* pModelName, CUtlBuffer& buf )
{
	char tmp[1024];
	Q_strncpy( tmp, pModelName, sizeof( tmp ) );
	Q_SetExtension( tmp, ".phy", sizeof( tmp ) );
	// No luck, gotta build it	
	if (!LoadFile( tmp, buf ))
	{
		// this is not an error, the model simply has no PHY file
		return false;
	}

	phyheader_t *header = (phyheader_t *)buf.PeekGet();

	if ( header->size != sizeof(*header) || header->solidCount <= 0 )
		return false;

	return true;
}

bool LoadVTXFile( char const* pModelName, const studiohdr_t *pStudioHdr, CUtlBuffer& buf )
{
	char	filename[MAX_PATH];

	// construct filename
	Q_StripExtension( pModelName, filename, sizeof( filename ) );
	strcat( filename, ".dx80.vtx" );

	if ( !LoadFile( filename, buf ) )
	{
		Warning( "Error! Unable to load file \"%s\"\n", filename );
		return false;
	}

	OptimizedModel::FileHeader_t* pVtxHdr = (OptimizedModel::FileHeader_t *)buf.Base();

	// Check that it's valid
	if ( pVtxHdr->version != OPTIMIZED_MODEL_FILE_VERSION )
	{
		Warning( "Error! Invalid VTX file version: %d, expected %d \"%s\"\n", pVtxHdr->version, OPTIMIZED_MODEL_FILE_VERSION, filename );
		return false;
	}
	if ( pVtxHdr->checkSum != pStudioHdr->checksum )
	{
		Warning( "Error! Invalid VTX file checksum: %d, expected %d \"%s\"\n", pVtxHdr->checkSum, pStudioHdr->checksum, filename );
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Gets a vertex position from a strip index
//-----------------------------------------------------------------------------
inline static Vector* PositionFromIndex( const mstudio_meshvertexdata_t *vertData, mstudiomesh_t* pMesh, OptimizedModel::StripGroupHeader_t* pStripGroup, int i )
{
	OptimizedModel::Vertex_t* pVert = pStripGroup->pVertex( i );
	return vertData->Position( pVert->origMeshVertID );
}


//-----------------------------------------------------------------------------
// Purpose: Writes a glview text file containing the collision surface in question
// Input  : *pCollide - 
//			*pFilename - 
//-----------------------------------------------------------------------------
void DumpCollideToGlView( vcollide_t *pCollide, const char *pFilename )
{
	if ( !pCollide )
		return;

	Msg("Writing %s...\n", pFilename );

	FILE *fp = fopen( pFilename, "w" );
	for (int i = 0; i < pCollide->solidCount; ++i)
	{
		Vector *outVerts;
		int vertCount = s_pPhysCollision->CreateDebugMesh( pCollide->solids[i], &outVerts );
		int triCount = vertCount / 3;
		int vert = 0;

		unsigned char r = (i & 1) * 64 + 64;
		unsigned char g = (i & 2) * 64 + 64;
		unsigned char b = (i & 4) * 64 + 64;

		float fr = r / 255.0f;
		float fg = g / 255.0f;
		float fb = b / 255.0f;

		for ( int i = 0; i < triCount; i++ )
		{
			fprintf( fp, "3\n" );
			fprintf( fp, "%6.3f %6.3f %6.3f %.2f %.3f %.3f\n", 
				outVerts[vert].x, outVerts[vert].y, outVerts[vert].z, fr, fg, fb );
			vert++;
			fprintf( fp, "%6.3f %6.3f %6.3f %.2f %.3f %.3f\n", 
				outVerts[vert].x, outVerts[vert].y, outVerts[vert].z, fr, fg, fb );
			vert++;
			fprintf( fp, "%6.3f %6.3f %6.3f %.2f %.3f %.3f\n", 
				outVerts[vert].x, outVerts[vert].y, outVerts[vert].z, fr, fg, fb );
			vert++;
		}
		s_pPhysCollision->DestroyDebugMesh( vertCount, outVerts );
	}
	fclose( fp );
}


static bool PointInTriangle( const Vector2D &p, const Vector2D &v0, const Vector2D &v1, const Vector2D &v2 )
{
	float coords[3];
	GetBarycentricCoords2D( v0, v1, v2, p, coords );
	for ( int i = 0; i < 3; i++ )
	{
		if ( coords[i] < 0.0f || coords[i] > 1.0f )
			return false;
	}
	float sum = coords[0] + coords[1] + coords[2];
	if ( sum > 1.0f )
		return false;
	return true;
}

bool LoadFileIntoBuffer( CUtlBuffer &buf, const char *pFilename )
{
	FileHandle_t fileHandle = g_pFileSystem->Open( pFilename, "rb" );
	if ( !fileHandle )
		return false;

	// Get the file size
	int texSize = g_pFileSystem->Size( fileHandle );
	buf.EnsureCapacity( texSize );
	int nBytesRead = g_pFileSystem->Read( buf.Base(), texSize, fileHandle );
	g_pFileSystem->Close( fileHandle );
	buf.SeekPut( CUtlBuffer::SEEK_HEAD, nBytesRead );
	buf.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
	return true;
}

// keeps a list of all textures that cast shadows via alpha channel
class CShadowTextureList
{
public:
	// This loads a vtf and converts it to RGB8888 format
	unsigned char *LoadVTFRGB8888( const char *pName, int *pWidth, int *pHeight, bool *pClampU, bool *pClampV )
	{
		char szPath[MAX_PATH];
		Q_strncpy( szPath, "materials/", sizeof( szPath ) );
		Q_strncat( szPath, pName, sizeof( szPath ), COPY_ALL_CHARACTERS );
		Q_strncat( szPath, ".vtf", sizeof( szPath ), COPY_ALL_CHARACTERS );
		Q_FixSlashes( szPath, CORRECT_PATH_SEPARATOR );

		CUtlBuffer buf;
		if ( !LoadFileIntoBuffer( buf, szPath ) )
			return NULL;
		IVTFTexture *pTex = CreateVTFTexture();
		if (!pTex->Unserialize( buf ))
			return NULL;
		Msg("Loaded alpha texture %s\n", szPath );
		unsigned char *pSrcImage = pTex->ImageData( 0, 0, 0, 0, 0, 0 );
		int iWidth = pTex->Width();
		int iHeight = pTex->Height();
		ImageFormat dstFormat = IMAGE_FORMAT_RGBA8888;
		ImageFormat srcFormat = pTex->Format();
		*pClampU = (pTex->Flags() & TEXTUREFLAGS_CLAMPS) ? true : false;
		*pClampV = (pTex->Flags() & TEXTUREFLAGS_CLAMPT) ? true : false;
		unsigned char *pDstImage = new unsigned char[ImageLoader::GetMemRequired( iWidth, iHeight, 1, dstFormat, false )];

		if( !ImageLoader::ConvertImageFormat( pSrcImage, srcFormat, 
			pDstImage, dstFormat, iWidth, iHeight, 0, 0 ) )
		{
			delete[] pDstImage;
			return NULL;
		}

		*pWidth = iWidth;
		*pHeight = iHeight;
		return pDstImage;
	}

	// Checks the database for the material and loads if necessary
	// returns true if found and pIndex will be the index, -1 if no alpha shadows
	bool FindOrLoadIfValid( const char *pMaterialName, int *pIndex )
	{
		*pIndex = -1;
		int index = m_Textures.Find(pMaterialName);
		bool bFound = false;
		if ( index != m_Textures.InvalidIndex() )
		{
			bFound = true;
			*pIndex = index;
		}
		else
		{
			KeyValues *pVMT = new KeyValues("vmt");
			CUtlBuffer buf(0,0,CUtlBuffer::TEXT_BUFFER);
			LoadFileIntoBuffer( buf, pMaterialName );
			if ( pVMT->LoadFromBuffer( pMaterialName, buf ) )
			{
				bFound = true;
				if ( pVMT->FindKey("$translucent") || pVMT->FindKey("$alphatest") )
				{
					KeyValues *pBaseTexture = pVMT->FindKey("$basetexture");
					if ( pBaseTexture )
					{
						const char *pBaseTextureName = pBaseTexture->GetString();
						if ( pBaseTextureName )
						{
							int w, h;
							bool bClampU = false;
							bool bClampV = false;
							unsigned char *pImageBits = LoadVTFRGB8888( pBaseTextureName, &w, &h, &bClampU, &bClampV );
							if ( pImageBits )
							{
								int index = m_Textures.Insert( pMaterialName );
								m_Textures[index].InitFromRGB8888( w, h, pImageBits );
								*pIndex = index;
								if ( pVMT->FindKey("$nocull") )
								{
									// UNDONE: Support this? Do we need to emit two triangles?
									m_Textures[index].allowBackface = true;
								}
								m_Textures[index].clampU = bClampU;
								m_Textures[index].clampV = bClampV;
								delete[] pImageBits;
							}
						}
					}
				}

			}
			pVMT->deleteThis();
		}

		return bFound;
	}


	// iterate the textures for the model and load each one into the database
	// this is used on models marked to cast texture shadows
	void LoadAllTexturesForModel( studiohdr_t *pHdr, int *pTextureList )
	{
		for ( int i = 0; i < pHdr->numtextures; i++ )
		{
			int textureIndex = -1;
			// try to add each texture to the transparent shadow manager
			char szPath[MAX_PATH];

			// iterate quietly through all specified directories until a valid material is found
			for ( int j = 0; j < pHdr->numcdtextures; j++ )
			{
				Q_strncpy( szPath, "materials/", sizeof( szPath ) );
				Q_strncat( szPath, pHdr->pCdtexture( j ), sizeof( szPath ) );
				const char *textureName = pHdr->pTexture( i )->pszName();
				Q_strncat( szPath, textureName, sizeof( szPath ), COPY_ALL_CHARACTERS );
				Q_strncat( szPath, ".vmt", sizeof( szPath ), COPY_ALL_CHARACTERS );
				Q_FixSlashes( szPath, CORRECT_PATH_SEPARATOR );
				if ( FindOrLoadIfValid( szPath, &textureIndex ) )
					break;
			}

			pTextureList[i] = textureIndex;
		}
	}
	
	int AddMaterialEntry( int shadowTextureIndex, const Vector2D &t0, const Vector2D &t1, const Vector2D &t2 )
	{
		int index = m_MaterialEntries.AddToTail();
		m_MaterialEntries[index].textureIndex = shadowTextureIndex;
		m_MaterialEntries[index].uv[0] = t0;
		m_MaterialEntries[index].uv[1] = t1;
		m_MaterialEntries[index].uv[2] = t2;
		return index;
	}

	// HACKHACK: Compute the average coverage for this triangle by sampling the AABB of its texture space
	float ComputeCoverageForTriangle( int shadowTextureIndex, const Vector2D &t0, const Vector2D &t1, const Vector2D &t2 )
	{
		float umin = min(t0.x, t1.x);
		umin = min(umin, t2.x);
		float umax = max(t0.x, t1.x);
		umax = max(umax, t2.x);

		float vmin = min(t0.y, t1.y);
		vmin = min(vmin, t2.y);
		float vmax = max(t0.y, t1.y);
		vmax = max(vmax, t2.y);

		// UNDONE: Do something about tiling
		umin = clamp(umin, 0, 1);
		umax = clamp(umax, 0, 1);
		vmin = clamp(vmin, 0, 1);
		vmax = clamp(vmax, 0, 1);
		Assert(umin>=0.0f && umax <= 1.0f);
		Assert(vmin>=0.0f && vmax <= 1.0f);
		const alphatexture_t &tex = m_Textures.Element(shadowTextureIndex);
		int u0 = umin * (tex.width-1);
		int u1 = umax * (tex.width-1);
		int v0 = vmin * (tex.height-1);
		int v1 = vmax * (tex.height-1);

		int total = 0;
		int count = 0;
		for ( int v = v0; v <= v1; v++ )
		{
			int row = (v * tex.width);
			for ( int u = u0; u <= u1; u++ )
			{
				total += tex.pAlphaTexels[row + u];
				count++;
			}
		}
		if ( count )
		{
			float coverage = float(total) / (count * 255.0f);
			return coverage;
		}
		return 1.0f;
	}
	
	int SampleMaterial( int materialIndex, const Vector &coords, bool bBackface )
	{
		const materialentry_t &mat = m_MaterialEntries[materialIndex];
		const alphatexture_t &tex = m_Textures.Element(m_MaterialEntries[materialIndex].textureIndex);
		if ( bBackface && !tex.allowBackface )
			return 0;
		Vector2D uv = coords.x * mat.uv[0] + coords.y * mat.uv[1] + coords.z * mat.uv[2];
		int u = RoundFloatToInt( uv[0] * tex.width );
		int v = RoundFloatToInt( uv[1] * tex.height );
		
		// asume power of 2, clamp or wrap
		// UNDONE: Support clamp?  This code should work
#if 0
		u = tex.clampU ? clamp(u,0,(tex.width-1)) : (u & (tex.width-1));
		v = tex.clampV ? clamp(v,0,(tex.height-1)) : (v & (tex.height-1));
#else
		// for now always wrap
		u &= (tex.width-1);
		v &= (tex.height-1);
#endif

		return tex.pAlphaTexels[v * tex.width + u];
	}

	struct alphatexture_t 
	{
		short width;
		short height;
		bool allowBackface;
		bool clampU;
		bool clampV;
		unsigned char *pAlphaTexels;

		void InitFromRGB8888( int w, int h, unsigned char *pTexels )
		{
			width = w;
			height = h;
			pAlphaTexels = new unsigned char[w*h];
			for ( int i = 0; i < h; i++ )
			{
				for ( int j = 0; j < w; j++ )
				{
					int index = (i*w) + j;
					pAlphaTexels[index] = pTexels[index*4 + 3];
				}
			}
		}
	};
	struct materialentry_t
	{
		int textureIndex;
		Vector2D uv[3];
	};
	// this is the list of textures we've loaded
	// only load each one once
	CUtlDict< alphatexture_t, unsigned short > m_Textures;
	CUtlVector<materialentry_t> m_MaterialEntries;
};

// global to keep the shadow-casting texture list and their alpha bits
CShadowTextureList g_ShadowTextureList;

float ComputeCoverageFromTexture( float b0, float b1, float b2, int32 hitID )
{
	const float alphaScale = 1.0f / 255.0f;
	// UNDONE: Pass ray down to determine backfacing?
	//Vector normal( tri.m_flNx, tri.m_flNy, tri.m_flNz );
	//bool bBackface = DotProduct(delta, tri.N) > 0 ? true : false;
	Vector coords(b0,b1,b2);
	return alphaScale * g_ShadowTextureList.SampleMaterial( g_RtEnv.GetTriangleMaterial(hitID), coords, false );
}

// this is here to strip models/ or .mdl or whatnot
void CleanModelName( const char *pModelName, char *pOutput, int outLen )
{
	// strip off leading models/ if it exists
	const char *pModelDir = "models/";
	int modelLen = Q_strlen(pModelDir);

	if ( !Q_strnicmp(pModelName, pModelDir, modelLen ) )
	{
		pModelName += modelLen;
	}
	Q_strncpy( pOutput, pModelName, outLen );

	// truncate any .mdl extension
	char *dot = strchr(pOutput,'.');
	if ( dot )
	{
		*dot = 0;
	}

}


void ForceTextureShadowsOnModel( const char *pModelName )
{
	char buf[1024];
	CleanModelName( pModelName, buf, sizeof(buf) );
	if ( !g_ForcedTextureShadowsModels.Find(buf).IsValid())
	{
		g_ForcedTextureShadowsModels.AddString(buf);
	}
}

bool IsModelTextureShadowsForced( const char *pModelName )
{
	char buf[1024];
	CleanModelName( pModelName, buf, sizeof(buf) );
	return g_ForcedTextureShadowsModels.Find(buf).IsValid();
}


//-----------------------------------------------------------------------------
// Creates a collision model (based on the render geometry!)
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::CreateCollisionModel( char const* pModelName )
{
	CUtlBuffer buf;
	CUtlBuffer bufvtx;
	CUtlBuffer bufphy;

	int i = m_StaticPropDict.AddToTail();
	m_StaticPropDict[i].m_pModel = NULL;
	m_StaticPropDict[i].m_pStudioHdr = NULL;

	if ( !LoadStudioModel( pModelName, buf ) )
	{
		VectorCopy( vec3_origin, m_StaticPropDict[i].m_Mins );
		VectorCopy( vec3_origin, m_StaticPropDict[i].m_Maxs );
		return;
	}

	studiohdr_t* pHdr = (studiohdr_t*)buf.Base();

	VectorCopy( pHdr->hull_min, m_StaticPropDict[i].m_Mins );
	VectorCopy( pHdr->hull_max, m_StaticPropDict[i].m_Maxs );

	if ( LoadStudioCollisionModel( pModelName, bufphy ) )
	{
		phyheader_t header;
		bufphy.Get( &header, sizeof(header) );

		vcollide_t *pCollide = &m_StaticPropDict[i].m_loadedModel;
		s_pPhysCollision->VCollideLoad( pCollide, header.solidCount, (const char *)bufphy.PeekGet(), bufphy.TellPut() - bufphy.TellGet() );
		m_StaticPropDict[i].m_pModel = m_StaticPropDict[i].m_loadedModel.solids[0];

		/*
		static int propNum = 0;
		char tmp[128];
		sprintf( tmp, "staticprop%03d.txt", propNum );
		DumpCollideToGlView( pCollide, tmp );
		++propNum;
		*/
	}
	else
	{
		// mark this as unused
		m_StaticPropDict[i].m_loadedModel.solidCount = 0;

		// CPhysCollide* pPhys = CreatePhysCollide( pHdr, pVtxHdr );
		m_StaticPropDict[i].m_pModel = ComputeConvexHull( pHdr );
	}

	// clone it
	m_StaticPropDict[i].m_pStudioHdr = (studiohdr_t *)malloc( buf.Size() );
	memcpy( m_StaticPropDict[i].m_pStudioHdr, (studiohdr_t*)buf.Base(), buf.Size() );

	if ( !LoadVTXFile( pModelName, m_StaticPropDict[i].m_pStudioHdr, m_StaticPropDict[i].m_VtxBuf ) )
	{
		// failed, leave state identified as disabled
		m_StaticPropDict[i].m_VtxBuf.Purge();
	}

	if ( g_bTextureShadows )
	{
		if ( (pHdr->flags & STUDIOHDR_FLAGS_CAST_TEXTURE_SHADOWS) || IsModelTextureShadowsForced(pModelName) )
		{
			m_StaticPropDict[i].m_textureShadowIndex.RemoveAll();
			m_StaticPropDict[i].m_triangleMaterialIndex.RemoveAll();
			m_StaticPropDict[i].m_textureShadowIndex.AddMultipleToTail( pHdr->numtextures );
			g_ShadowTextureList.LoadAllTexturesForModel( pHdr, m_StaticPropDict[i].m_textureShadowIndex.Base() );
		}
	}
}


//-----------------------------------------------------------------------------
// Unserialize static prop model dictionary
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::UnserializeModelDict( CUtlBuffer& buf )
{
	int count = buf.GetInt();
	while ( --count >= 0 )
	{
		StaticPropDictLump_t lump;
		buf.Get( &lump, sizeof(StaticPropDictLump_t) );
		
		CreateCollisionModel( lump.m_Name );
	}
}

void CVradStaticPropMgr::UnserializeModels( CUtlBuffer& buf )
{
	int count = buf.GetInt();


	m_StaticProps.AddMultipleToTail(count);
	for ( int i = 0; i < count; ++i )				  
	{
		StaticPropLump_t lump;
		buf.Get( &lump, sizeof(StaticPropLump_t) );
		
		VectorCopy( lump.m_Origin, m_StaticProps[i].m_Origin );
		VectorCopy( lump.m_Angles, m_StaticProps[i].m_Angles );
		VectorCopy( lump.m_LightingOrigin, m_StaticProps[i].m_LightingOrigin );
		m_StaticProps[i].m_bLightingOriginValid = ( lump.m_Flags & STATIC_PROP_USE_LIGHTING_ORIGIN ) > 0;
		m_StaticProps[i].m_ModelIdx = lump.m_PropType;
		m_StaticProps[i].m_Handle = TREEDATA_INVALID_HANDLE;
		m_StaticProps[i].m_Flags = lump.m_Flags;

		// Changed this from using DXT1 to RGB888 because the compression artifacts were pretty nasty. 
		// TODO: Consider changing back or basing this on user selection in hammer.
		m_StaticProps[i].m_LightmapImageFormat = IMAGE_FORMAT_RGB888;
		m_StaticProps[i].m_LightmapImageWidth = lump.m_nLightmapResolutionX;
		m_StaticProps[i].m_LightmapImageHeight = lump.m_nLightmapResolutionY;
	}
}

//-----------------------------------------------------------------------------
// Unserialize static props
//-----------------------------------------------------------------------------

void CVradStaticPropMgr::UnserializeStaticProps()
{
	// Unserialize static props, insert them into the appropriate leaves
	GameLumpHandle_t handle = g_GameLumps.GetGameLumpHandle( GAMELUMP_STATIC_PROPS );
	int size = g_GameLumps.GameLumpSize( handle );
	if (!size)
		return;

	if ( g_GameLumps.GetGameLumpVersion( handle ) != GAMELUMP_STATIC_PROPS_VERSION )
	{
		Error( "Cannot load the static props... encountered a stale map version. Re-vbsp the map." );
	}

	if ( g_GameLumps.GetGameLump( handle ) )
	{
		CUtlBuffer buf( g_GameLumps.GetGameLump(handle), size, CUtlBuffer::READ_ONLY );
		UnserializeModelDict( buf );

		// Skip the leaf list data
		int count = buf.GetInt();
		buf.SeekGet( CUtlBuffer::SEEK_CURRENT, count * sizeof(StaticPropLeafLump_t) );

		UnserializeModels( buf );
	}
}

//-----------------------------------------------------------------------------
// Level init, shutdown
//-----------------------------------------------------------------------------

void CVradStaticPropMgr::Init()
{
	CreateInterfaceFn physicsFactory = GetPhysicsFactory();
	if ( !physicsFactory )
		Error( "Unable to load vphysics DLL." );
		
	s_pPhysCollision = (IPhysicsCollision *)physicsFactory( VPHYSICS_COLLISION_INTERFACE_VERSION, NULL );
	if( !s_pPhysCollision )
	{
		Error( "Unable to get '%s' for physics interface.", VPHYSICS_COLLISION_INTERFACE_VERSION );
		return;
	}

	// Read in static props that have been compiled into the bsp file
	UnserializeStaticProps();
}

void CVradStaticPropMgr::Shutdown()
{

	// Remove all static prop model data
	for (int i = m_StaticPropDict.Size(); --i >= 0; )
	{
		studiohdr_t *pStudioHdr = m_StaticPropDict[i].m_pStudioHdr;
		if ( pStudioHdr )
		{
			if ( pStudioHdr->pVertexBase )
			{
				free( pStudioHdr->pVertexBase );
			}
			free( pStudioHdr );
		}
	}

	m_StaticProps.Purge();
	m_StaticPropDict.Purge();
}

void ComputeLightmapColor( dface_t* pFace, Vector &color )
{
	texinfo_t* pTex = &texinfo[pFace->texinfo];
	if ( pTex->flags & SURF_SKY )
	{
		// sky ambient already accounted for in direct component
		return;
	}
}

bool PositionInSolid( Vector &position )
{
	int ndxLeaf = PointLeafnum( position );
	if ( dleafs[ndxLeaf].contents & CONTENTS_SOLID )
	{
		// position embedded in solid
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Trace from a vertex to each direct light source, accumulating its contribution.
//-----------------------------------------------------------------------------
void ComputeDirectLightingAtPoint( Vector &position, Vector &normal, Vector &outColor, int iThread,
								   int static_prop_id_to_skip=-1, int nLFlags = 0)
{
	SSE_sampleLightOutput_t	sampleOutput;

	outColor.Init();

	// Iterate over all direct lights and accumulate their contribution
	int cluster = ClusterFromPoint( position );
	for ( directlight_t *dl = activelights; dl != NULL; dl = dl->next )
	{
		if ( dl->light.style )
		{
			// skip lights with style
			continue;
		}

		// is this lights cluster visible?
		if ( !PVSCheck( dl->pvs, cluster ) )
			continue;

		// push the vertex towards the light to avoid surface acne
		Vector adjusted_pos = position;
		float flEpsilon = 0.0;

		if  (dl->light.type != emit_skyambient)
		{
			// push towards the light
			Vector fudge;
			if ( dl->light.type == emit_skylight )
				fudge = -( dl->light.normal);
			else
			{
				fudge = dl->light.origin-position;
				VectorNormalize( fudge );
			}
			fudge *= 4.0;
			adjusted_pos += fudge;
		}
		else 
		{
			// push out along normal
			adjusted_pos += 4.0 * normal;
//			flEpsilon = 1.0;
		}

		FourVectors adjusted_pos4;
		FourVectors normal4;
		adjusted_pos4.DuplicateVector( adjusted_pos );
		normal4.DuplicateVector( normal );

		GatherSampleLightSSE( sampleOutput, dl, -1, adjusted_pos4, &normal4, 1, iThread, nLFlags | GATHERLFLAGS_FORCE_FAST,
		                      static_prop_id_to_skip, flEpsilon );
		
		VectorMA( outColor, sampleOutput.m_flFalloff.m128_f32[0] * sampleOutput.m_flDot[0].m128_f32[0], dl->light.intensity, outColor );
	}
}

//-----------------------------------------------------------------------------
// Takes the results from a ComputeLighting call and applies it to the static prop in question.
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::ApplyLightingToStaticProp( int iStaticProp, CStaticProp &prop, const CComputeStaticPropLightingResults *pResults )
{
	if ( pResults->m_ColorVertsArrays.Count() == 0 && pResults->m_ColorTexelsArrays.Count() == 0 )
		return;

	StaticPropDict_t &dict = m_StaticPropDict[prop.m_ModelIdx];
	studiohdr_t	*pStudioHdr = dict.m_pStudioHdr;
	OptimizedModel::FileHeader_t *pVtxHdr = (OptimizedModel::FileHeader_t *)dict.m_VtxBuf.Base();
	Assert( pStudioHdr && pVtxHdr );

	int iCurColorVertsArray = 0;
	int iCurColorTexelsArray = 0;

	for ( int bodyID = 0; bodyID < pStudioHdr->numbodyparts; ++bodyID )
	{
		OptimizedModel::BodyPartHeader_t* pVtxBodyPart = pVtxHdr->pBodyPart( bodyID );
		mstudiobodyparts_t *pBodyPart = pStudioHdr->pBodypart( bodyID );

		for ( int modelID = 0; modelID < pBodyPart->nummodels; ++modelID )
		{
			OptimizedModel::ModelHeader_t* pVtxModel = pVtxBodyPart->pModel( modelID );
			mstudiomodel_t *pStudioModel = pBodyPart->pModel( modelID );
						
			const CUtlVector<colorVertex_t> *colorVerts = pResults->m_ColorVertsArrays.Count() ? pResults->m_ColorVertsArrays[iCurColorVertsArray++] : nullptr;
			const CUtlVector<colorTexel_t> *colorTexels = pResults->m_ColorTexelsArrays.Count() ? pResults->m_ColorTexelsArrays[iCurColorTexelsArray++] : nullptr;
			
			for ( int nLod = 0; nLod < pVtxHdr->numLODs; nLod++ )
			{
				OptimizedModel::ModelLODHeader_t *pVtxLOD = pVtxModel->pLOD( nLod );

				for ( int nMesh = 0; nMesh < pStudioModel->nummeshes; ++nMesh )
				{
					mstudiomesh_t* pMesh = pStudioModel->pMesh( nMesh );
					OptimizedModel::MeshHeader_t* pVtxMesh = pVtxLOD->pMesh( nMesh );

					for ( int nGroup = 0; nGroup < pVtxMesh->numStripGroups; ++nGroup )
					{
						OptimizedModel::StripGroupHeader_t* pStripGroup = pVtxMesh->pStripGroup( nGroup );
						int nMeshIdx = prop.m_MeshData.AddToTail();

						if (colorVerts)
						{
							prop.m_MeshData[nMeshIdx].m_VertexColors.AddMultipleToTail( pStripGroup->numVerts );
							prop.m_MeshData[nMeshIdx].m_nLod = nLod;

							for ( int nVertex = 0; nVertex < pStripGroup->numVerts; ++nVertex )
							{
								int nIndex = pMesh->vertexoffset + pStripGroup->pVertex( nVertex )->origMeshVertID;

								Assert( nIndex < pStudioModel->numvertices );
								prop.m_MeshData[nMeshIdx].m_VertexColors[nVertex] = (*colorVerts)[nIndex].m_Color;
							}
						}

						if (colorTexels)
						{
							// TODO: Consider doing this work in the worker threads, because then we distribute it.
							ConvertTexelDataToTexture(prop.m_LightmapImageWidth, prop.m_LightmapImageHeight, prop.m_LightmapImageFormat, (*colorTexels), &prop.m_MeshData[nMeshIdx].m_TexelsEncoded);

							if (g_bDumpPropLightmaps)
							{
								char buffer[_MAX_PATH];
								V_snprintf( 
									buffer, 
									_MAX_PATH - 1, 
									"staticprop_lightmap_%d_%.0f_%.0f_%.0f_%s_%d_%d_%d_%d_%d.tga", 
									iStaticProp, 
									prop.m_Origin.x, 
									prop.m_Origin.y,
									prop.m_Origin.z,
									dict.m_pStudioHdr->pszName(), 
									bodyID, 
									modelID, 
									nLod, 
									nMesh, 
									nGroup 
								);

								for ( int i = 0; buffer[i]; ++i ) 
								{
									if (buffer[i] == '/' || buffer[i] == '\\')
										buffer[i] = '-';
								}
								DumpLightmapLinear( buffer, (*colorTexels), prop.m_LightmapImageWidth, prop.m_LightmapImageHeight );
							}
						}
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Trace rays from each unique vertex, accumulating direct and indirect
// sources at each ray termination. Use the winding data to distribute the unique vertexes
// into the rendering layout.
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::ComputeLighting( CStaticProp &prop, int iThread, int prop_index, CComputeStaticPropLightingResults *pResults )
{
	CUtlVector<badVertex_t>		badVerts;

	StaticPropDict_t &dict = m_StaticPropDict[prop.m_ModelIdx];
	studiohdr_t	*pStudioHdr = dict.m_pStudioHdr;
	OptimizedModel::FileHeader_t *pVtxHdr = (OptimizedModel::FileHeader_t *)dict.m_VtxBuf.Base();
	if ( !pStudioHdr || !pVtxHdr )
	{
		// must have model and its verts for lighting computation
		// game will fallback to fullbright
		return;
	}

	const bool withVertexLighting = (prop.m_Flags & STATIC_PROP_NO_PER_VERTEX_LIGHTING) == 0;
	const bool withTexelLighting = (prop.m_Flags & STATIC_PROP_NO_PER_TEXEL_LIGHTING) == 0;

	if (!withVertexLighting && !withTexelLighting)
		return;

	const int skip_prop = (g_bDisablePropSelfShadowing || (prop.m_Flags & STATIC_PROP_NO_SELF_SHADOWING)) ? prop_index : -1;
	const int nFlags = ( prop.m_Flags & STATIC_PROP_IGNORE_NORMALS ) ? GATHERLFLAGS_IGNORE_NORMALS : 0;

	VMPI_SetCurrentStage( "ComputeLighting" );

	matrix3x4_t	matPos, matNormal;
	AngleMatrix(prop.m_Angles, prop.m_Origin, matPos);
	AngleMatrix(prop.m_Angles, matNormal);
	
	for ( int bodyID = 0; bodyID < pStudioHdr->numbodyparts; ++bodyID )
	{
		OptimizedModel::BodyPartHeader_t* pVtxBodyPart = pVtxHdr->pBodyPart( bodyID );
		mstudiobodyparts_t *pBodyPart = pStudioHdr->pBodypart( bodyID );

		for ( int modelID = 0; modelID < pBodyPart->nummodels; ++modelID )
		{
			OptimizedModel::ModelHeader_t* pVtxModel = pVtxBodyPart->pModel(modelID);
			mstudiomodel_t *pStudioModel = pBodyPart->pModel( modelID );

			if (withTexelLighting)
			{
				CUtlVector<colorTexel_t> *pColorTexelArray = new CUtlVector<colorTexel_t>;
				pResults->m_ColorTexelsArrays.AddToTail(pColorTexelArray);
			}
			
			// light all unique vertexes
			CUtlVector<colorVertex_t> *pColorVertsArray = new CUtlVector<colorVertex_t>;
			pResults->m_ColorVertsArrays.AddToTail( pColorVertsArray );
						
			CUtlVector<colorVertex_t> &colorVerts = *pColorVertsArray; 
			colorVerts.EnsureCount( pStudioModel->numvertices );
			memset( colorVerts.Base(), 0, colorVerts.Count() * sizeof(colorVertex_t) );

			int numVertexes = 0;
			for ( int meshID = 0; meshID < pStudioModel->nummeshes; ++meshID )
			{
				mstudiomesh_t *pStudioMesh = pStudioModel->pMesh( meshID );
				const mstudio_meshvertexdata_t *vertData = pStudioMesh->GetVertexData((void *)pStudioHdr);

				Assert(vertData); // This can only return NULL on X360 for now
				
				// TODO: Move this into its own function. In fact, refactor this whole function.
				if (withTexelLighting)
				{
					GenerateLightmapSamplesForMesh( matPos, matNormal, iThread, skip_prop, nFlags, prop.m_LightmapImageWidth, prop.m_LightmapImageHeight, pStudioHdr, pStudioModel, pVtxModel, meshID, pResults );
				}

				// If we do lightmapping, we also do vertex lighting as a potential fallback. This may change.
				for ( int vertexID = 0; vertexID < pStudioMesh->numvertices; ++vertexID )
				{
					Vector sampleNormal;
					Vector samplePosition;
					// transform position and normal into world coordinate system
					VectorTransform(*vertData->Position(vertexID), matPos, samplePosition);
					VectorTransform(*vertData->Normal(vertexID), matNormal, sampleNormal);

					if ( PositionInSolid( samplePosition ) )
					{
						// vertex is in solid, add to the bad list, and recover later
						badVertex_t badVertex;
						badVertex.m_ColorVertex = numVertexes;
						badVertex.m_Position = samplePosition;
						badVertex.m_Normal = sampleNormal;
						badVerts.AddToTail( badVertex );			
					}
					else
					{
						Vector direct_pos=samplePosition;
							
						

						Vector directColor(0,0,0);
						ComputeDirectLightingAtPoint( direct_pos,
														sampleNormal, directColor, iThread,
														skip_prop, nFlags );
						Vector indirectColor(0,0,0);

						if (g_bShowStaticPropNormals)
						{
							directColor= sampleNormal;
							directColor += Vector(1.0,1.0,1.0);
							directColor *= 50.0;
						}
						else
						{
							if (numbounce >= 1)
								ComputeIndirectLightingAtPoint( 
									samplePosition, sampleNormal, 
									indirectColor, iThread, true,
									( prop.m_Flags & STATIC_PROP_IGNORE_NORMALS) != 0 );
						}
						
						colorVerts[numVertexes].m_bValid = true;
						colorVerts[numVertexes].m_Position = samplePosition;
						VectorAdd( directColor, indirectColor, colorVerts[numVertexes].m_Color );
					}
					
					numVertexes++;
				}
			}
			
			// color in the bad vertexes
			// when entire model has no lighting origin and no valid neighbors
			// must punt, leave black coloring
			if ( badVerts.Count() && ( prop.m_bLightingOriginValid || badVerts.Count() != numVertexes ) )
			{
				for ( int nBadVertex = 0; nBadVertex < badVerts.Count(); nBadVertex++ )
				{		
					Vector bestPosition;
					if ( prop.m_bLightingOriginValid )
					{
						// use the specified lighting origin
						VectorCopy( prop.m_LightingOrigin, bestPosition );
					}
					else
					{
						// find the closest valid neighbor
						int best = 0;
						float closest = FLT_MAX;
						for ( int nColorVertex = 0; nColorVertex < numVertexes; nColorVertex++ )
						{
							if ( !colorVerts[nColorVertex].m_bValid )
							{
								// skip invalid neighbors
								continue;
							}
							Vector delta;
							VectorSubtract( colorVerts[nColorVertex].m_Position, badVerts[nBadVertex].m_Position, delta );
							float distance = VectorLength( delta );
							if ( distance < closest )
							{
								closest = distance;
								best    = nColorVertex;
							}
						}

						// use the best neighbor as the direction to crawl
						VectorCopy( colorVerts[best].m_Position, bestPosition );
					}

					// crawl toward best position
					// sudivide to determine a closer valid point to the bad vertex, and re-light
					Vector midPosition;
					int numIterations = 20;
					while ( --numIterations > 0 )
					{
						VectorAdd( bestPosition, badVerts[nBadVertex].m_Position, midPosition );
						VectorScale( midPosition, 0.5f, midPosition );
						if ( PositionInSolid( midPosition ) )
							break;
						bestPosition = midPosition;
					}

					// re-light from better position
					Vector directColor;
					ComputeDirectLightingAtPoint( bestPosition, badVerts[nBadVertex].m_Normal, directColor, iThread );

					Vector indirectColor;
					ComputeIndirectLightingAtPoint( bestPosition, badVerts[nBadVertex].m_Normal,
													indirectColor, iThread, true );

					// save results, not changing valid status
					// to ensure this offset position is not considered as a viable candidate
					colorVerts[badVerts[nBadVertex].m_ColorVertex].m_Position = bestPosition;
					VectorAdd( directColor, indirectColor, colorVerts[badVerts[nBadVertex].m_ColorVertex].m_Color );
				}
			}
			
			// discard bad verts
			badVerts.Purge();
		}
	}
}

//-----------------------------------------------------------------------------
// Write the lighitng to bsp pak lump
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::SerializeLighting()
{
	char		filename[MAX_PATH];
	CUtlBuffer	utlBuf;

	// illuminate them all
	int count = m_StaticProps.Count();
	if ( !count )
	{
		// nothing to do
		return;
	}

	char mapName[MAX_PATH];
	Q_FileBase( source, mapName, sizeof( mapName ) );

	int size;
	for (int i = 0; i < count; ++i)
	{
		// no need to write this file if we didn't compute the data
		// props marked this way will not load the info anyway 
		if ( m_StaticProps[i].m_Flags & STATIC_PROP_NO_PER_VERTEX_LIGHTING )
			continue;

		if (g_bHDR)
		{
			sprintf( filename, "sp_hdr_%d.vhv", i );
		}
		else
		{
			sprintf( filename, "sp_%d.vhv", i );
		}

		int totalVertexes = 0;
		for ( int j=0; j<m_StaticProps[i].m_MeshData.Count(); j++ )
		{
			totalVertexes += m_StaticProps[i].m_MeshData[j].m_VertexColors.Count();
		}

		// allocate a buffer with enough padding for alignment
		size = sizeof( HardwareVerts::FileHeader_t ) + 
				m_StaticProps[i].m_MeshData.Count()*sizeof(HardwareVerts::MeshHeader_t) +
				totalVertexes*4 + 2*512;
		utlBuf.EnsureCapacity( size );
		Q_memset( utlBuf.Base(), 0, size );

		HardwareVerts::FileHeader_t *pVhvHdr = (HardwareVerts::FileHeader_t *)utlBuf.Base();

		// align to start of vertex data
		unsigned char *pVertexData = (unsigned char *)(sizeof( HardwareVerts::FileHeader_t ) + m_StaticProps[i].m_MeshData.Count()*sizeof(HardwareVerts::MeshHeader_t));
		pVertexData = (unsigned char*)pVhvHdr + ALIGN_TO_POW2( (unsigned int)pVertexData, 512 );
		
		// construct header
		pVhvHdr->m_nVersion     = VHV_VERSION;
		pVhvHdr->m_nChecksum    = m_StaticPropDict[m_StaticProps[i].m_ModelIdx].m_pStudioHdr->checksum;
		pVhvHdr->m_nVertexFlags = VERTEX_COLOR;
		pVhvHdr->m_nVertexSize  = 4;
		pVhvHdr->m_nVertexes    = totalVertexes;
		pVhvHdr->m_nMeshes      = m_StaticProps[i].m_MeshData.Count();

		for (int n=0; n<pVhvHdr->m_nMeshes; n++)
		{
			// construct mesh dictionary
			HardwareVerts::MeshHeader_t *pMesh = pVhvHdr->pMesh( n );
			pMesh->m_nLod      = m_StaticProps[i].m_MeshData[n].m_nLod;
			pMesh->m_nVertexes = m_StaticProps[i].m_MeshData[n].m_VertexColors.Count();
			pMesh->m_nOffset   = (unsigned int)pVertexData - (unsigned int)pVhvHdr; 

			// construct vertexes
			for (int k=0; k<pMesh->m_nVertexes; k++)
			{
				Vector &vertexColor = m_StaticProps[i].m_MeshData[n].m_VertexColors[k];

				ColorRGBExp32 rgbColor;
				VectorToColorRGBExp32( vertexColor, rgbColor );
				unsigned char dstColor[4];
				ConvertRGBExp32ToRGBA8888( &rgbColor, dstColor );

				// b,g,r,a order
				pVertexData[0] = dstColor[2];
				pVertexData[1] = dstColor[1];
				pVertexData[2] = dstColor[0];
				pVertexData[3] = dstColor[3];
				pVertexData += 4;
			}
		}

		// align to end of file
		pVertexData = (unsigned char *)((unsigned int)pVertexData - (unsigned int)pVhvHdr);
		pVertexData = (unsigned char*)pVhvHdr + ALIGN_TO_POW2( (unsigned int)pVertexData, 512 );

		AddBufferToPak( GetPakFile(), filename, (void*)pVhvHdr, pVertexData - (unsigned char*)pVhvHdr, false );
	}

	for (int i = 0; i < count; ++i)
	{
		const int kAlignment = 512;
		// no need to write this file if we didn't compute the data
		// props marked this way will not load the info anyway 
		if (m_StaticProps[i].m_Flags & STATIC_PROP_NO_PER_TEXEL_LIGHTING)
			continue;

		sprintf(filename, "texelslighting_%d.ppl", i);

		ImageFormat fmt = m_StaticProps[i].m_LightmapImageFormat;

		unsigned int totalTexelSizeBytes = 0;
		for (int j = 0; j < m_StaticProps[i].m_MeshData.Count(); j++)
		{
			totalTexelSizeBytes += m_StaticProps[i].m_MeshData[j].m_TexelsEncoded.Count();
		}

		// allocate a buffer with enough padding for alignment
		size = sizeof(HardwareTexels::FileHeader_t) 
			 + m_StaticProps[i].m_MeshData.Count() * sizeof(HardwareTexels::MeshHeader_t) 
			 + totalTexelSizeBytes
			 + 2 * kAlignment;
		
		utlBuf.EnsureCapacity(size);
		Q_memset(utlBuf.Base(), 0, size);

		HardwareTexels::FileHeader_t *pVhtHdr = (HardwareTexels::FileHeader_t *)utlBuf.Base();

		// align start of texel data
		unsigned char *pTexelData = (unsigned char *)(sizeof(HardwareTexels::FileHeader_t) + m_StaticProps[i].m_MeshData.Count() * sizeof(HardwareTexels::MeshHeader_t));
		pTexelData = (unsigned char*)pVhtHdr + ALIGN_TO_POW2((unsigned int)pTexelData, kAlignment);

		pVhtHdr->m_nVersion	    = VHT_VERSION;
		pVhtHdr->m_nChecksum    = m_StaticPropDict[m_StaticProps[i].m_ModelIdx].m_pStudioHdr->checksum;
		pVhtHdr->m_nTexelFormat = fmt;
		pVhtHdr->m_nMeshes      = m_StaticProps[i].m_MeshData.Count();

		for (int n = 0; n < pVhtHdr->m_nMeshes; n++)
		{
			HardwareTexels::MeshHeader_t *pMesh = pVhtHdr->pMesh(n);
			pMesh->m_nLod = m_StaticProps[i].m_MeshData[n].m_nLod;
			pMesh->m_nOffset = (unsigned int)pTexelData - (unsigned int)pVhtHdr;
			pMesh->m_nBytes = m_StaticProps[i].m_MeshData[n].m_TexelsEncoded.Count();
			pMesh->m_nWidth = m_StaticProps[i].m_LightmapImageWidth;
			pMesh->m_nHeight = m_StaticProps[i].m_LightmapImageHeight;

			Q_memcpy(pTexelData, m_StaticProps[i].m_MeshData[n].m_TexelsEncoded.Base(), m_StaticProps[i].m_MeshData[n].m_TexelsEncoded.Count());
			pTexelData += m_StaticProps[i].m_MeshData[n].m_TexelsEncoded.Count();
		}

		pTexelData = (unsigned char *)((unsigned int)pTexelData - (unsigned int)pVhtHdr);
		pTexelData = (unsigned char*)pVhtHdr + ALIGN_TO_POW2((unsigned int)pTexelData, kAlignment);

		AddBufferToPak(GetPakFile(), filename, (void*)pVhtHdr, pTexelData - (unsigned char*)pVhtHdr, false);
	}
}

void CVradStaticPropMgr::VMPI_ProcessStaticProp_Static( int iThread, uint64 iStaticProp, MessageBuffer *pBuf )
{
	g_StaticPropMgr.VMPI_ProcessStaticProp( iThread, iStaticProp, pBuf );
}

void CVradStaticPropMgr::VMPI_ReceiveStaticPropResults_Static( uint64 iStaticProp, MessageBuffer *pBuf, int iWorker )
{
	g_StaticPropMgr.VMPI_ReceiveStaticPropResults( iStaticProp, pBuf, iWorker );
}
	
//-----------------------------------------------------------------------------
// Called on workers to do the computation for a static prop and send
// it to the master.
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::VMPI_ProcessStaticProp( int iThread, int iStaticProp, MessageBuffer *pBuf )
{
	// Compute the lighting.
	CComputeStaticPropLightingResults results;
	ComputeLighting( m_StaticProps[iStaticProp], iThread, iStaticProp, &results );

	VMPI_SetCurrentStage( "EncodeLightingResults" );
	
	// Encode the results.
	int nLists = results.m_ColorVertsArrays.Count();
	pBuf->write( &nLists, sizeof( nLists ) );
	
	for ( int i=0; i < nLists; i++ )
	{
		CUtlVector<colorVertex_t> &curList = *results.m_ColorVertsArrays[i];
		int count = curList.Count();
		pBuf->write( &count, sizeof( count ) );
		pBuf->write( curList.Base(), curList.Count() * sizeof( colorVertex_t ) );
	}

	nLists = results.m_ColorTexelsArrays.Count();
	pBuf->write(&nLists, sizeof(nLists));

	for (int i = 0; i < nLists; i++)
	{
		CUtlVector<colorTexel_t> &curList = *results.m_ColorTexelsArrays[i];
		int count = curList.Count();
		pBuf->write(&count, sizeof(count));
		pBuf->write(curList.Base(), curList.Count() * sizeof(colorTexel_t));
	}
}

//-----------------------------------------------------------------------------
// Called on the master when a worker finishes processing a static prop.
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::VMPI_ReceiveStaticPropResults( int iStaticProp, MessageBuffer *pBuf, int iWorker )
{
	// Read in the results.
	CComputeStaticPropLightingResults results;
	
	int nLists;
	pBuf->read( &nLists, sizeof( nLists ) );
	
	for ( int i=0; i < nLists; i++ )
	{
		CUtlVector<colorVertex_t> *pList = new CUtlVector<colorVertex_t>;
		results.m_ColorVertsArrays.AddToTail( pList );
		
		int count;
		pBuf->read( &count, sizeof( count ) );
		pList->SetSize( count );
		pBuf->read( pList->Base(), count * sizeof( colorVertex_t ) );
	}

	pBuf->read(&nLists, sizeof(nLists));

	for (int i = 0; i < nLists; i++)
	{
		CUtlVector<colorTexel_t> *pList = new CUtlVector<colorTexel_t>;
		results.m_ColorTexelsArrays.AddToTail(pList);

		int count;
		pBuf->read(&count, sizeof(count));
		pList->SetSize(count);
		pBuf->read(pList->Base(), count * sizeof(colorTexel_t));
	}
	
	// Apply the results.
	ApplyLightingToStaticProp( iStaticProp, m_StaticProps[iStaticProp], &results );
}


void CVradStaticPropMgr::ComputeLightingForProp( int iThread, int iStaticProp )
{
	// Compute the lighting.
	CComputeStaticPropLightingResults results;
	ComputeLighting( m_StaticProps[iStaticProp], iThread, iStaticProp, &results );
	ApplyLightingToStaticProp( iStaticProp, m_StaticProps[iStaticProp], &results );
}

void CVradStaticPropMgr::ThreadComputeStaticPropLighting( int iThread, void *pUserData )
{
	while (1)
	{
		int j = GetThreadWork ();
		if (j == -1)
			break;
		CComputeStaticPropLightingResults results;
		g_StaticPropMgr.ComputeLightingForProp( iThread, j );
	}
}

//-----------------------------------------------------------------------------
// Computes lighting for the static props.
// Must be after all other surface lighting has been computed for the indirect sampling.
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::ComputeLighting( int iThread )
{
	// illuminate them all
	int count = m_StaticProps.Count();
	if ( !count )
	{
		// nothing to do
		return;
	}

	StartPacifier( "Computing static prop lighting : " );

	// ensure any traces against us are ignored because we have no inherit lighting contribution
	m_bIgnoreStaticPropTrace = true;

	if ( g_bUseMPI )
	{
		// Distribute the work among the workers.
		VMPI_SetCurrentStage( "CVradStaticPropMgr::ComputeLighting" );
		
		DistributeWork( 
			count, 
			VMPI_DISTRIBUTEWORK_PACKETID,
			&CVradStaticPropMgr::VMPI_ProcessStaticProp_Static, 
			&CVradStaticPropMgr::VMPI_ReceiveStaticPropResults_Static );
	}
	else
	{
		RunThreadsOn(count, true, ThreadComputeStaticPropLighting);
	}

	// restore default
	m_bIgnoreStaticPropTrace = false;

	// save data to bsp
	SerializeLighting();

	EndPacifier( true );
}

//-----------------------------------------------------------------------------
// Adds all static prop polys to the ray trace store.
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::AddPolysForRayTrace( void )
{
	int count = m_StaticProps.Count();
	if ( !count )
	{
		// nothing to do
		return;
	}

	// Triangle coverage of 1 (full coverage)
	Vector fullCoverage;
	fullCoverage.x = 1.0f;

	for ( int nProp = 0; nProp < count; ++nProp )
	{
		CStaticProp &prop = m_StaticProps[nProp];
		StaticPropDict_t &dict = m_StaticPropDict[prop.m_ModelIdx];

		if ( prop.m_Flags & STATIC_PROP_NO_SHADOW )
			continue;

		// If not using static prop polys, use AABB
		if ( !g_bStaticPropPolys )
		{
			if ( dict.m_pModel )
			{
				VMatrix xform;
				xform.SetupMatrixOrgAngles ( prop.m_Origin, prop.m_Angles );
				ICollisionQuery *queryModel = s_pPhysCollision->CreateQueryModel( dict.m_pModel );
				for ( int nConvex = 0; nConvex < queryModel->ConvexCount(); ++nConvex )
				{
					for ( int nTri = 0; nTri < queryModel->TriangleCount( nConvex ); ++nTri )
					{
						Vector verts[3];
						queryModel->GetTriangleVerts( nConvex, nTri, verts );
						for ( int nVert = 0; nVert < 3; ++nVert )
							verts[nVert] = xform.VMul4x3(verts[nVert]);
						g_RtEnv.AddTriangle ( TRACE_ID_STATICPROP | nProp, verts[0], verts[1], verts[2], fullCoverage );
					}
				}
				s_pPhysCollision->DestroyQueryModel( queryModel );
			}
			else
			{
				VectorAdd ( dict.m_Mins, prop.m_Origin, prop.m_mins );
				VectorAdd ( dict.m_Maxs, prop.m_Origin, prop.m_maxs );
				g_RtEnv.AddAxisAlignedRectangularSolid ( TRACE_ID_STATICPROP | nProp, prop.m_mins, prop.m_maxs, fullCoverage );
			}
			
			continue;
		}

		studiohdr_t	*pStudioHdr = dict.m_pStudioHdr;
		OptimizedModel::FileHeader_t *pVtxHdr = (OptimizedModel::FileHeader_t *)dict.m_VtxBuf.Base();
		if ( !pStudioHdr || !pVtxHdr )
		{
			// must have model and its verts for decoding triangles
			return;
		}
		// only init the triangle table the first time
		bool bInitTriangles = dict.m_triangleMaterialIndex.Count() ? false : true;
		int triangleIndex = 0;

		// meshes are deeply hierarchial, divided between three stores, follow the white rabbit
		// body parts -> models -> lod meshes -> strip groups -> strips
		// the vertices and indices are pooled, the trick is knowing the offset to determine your indexed base 
		for ( int bodyID = 0; bodyID < pStudioHdr->numbodyparts; ++bodyID )
		{
			OptimizedModel::BodyPartHeader_t* pVtxBodyPart = pVtxHdr->pBodyPart( bodyID );
			mstudiobodyparts_t *pBodyPart = pStudioHdr->pBodypart( bodyID );

			for ( int modelID = 0; modelID < pBodyPart->nummodels; ++modelID )
			{
				OptimizedModel::ModelHeader_t* pVtxModel = pVtxBodyPart->pModel( modelID );
				mstudiomodel_t *pStudioModel = pBodyPart->pModel( modelID );

				// assuming lod 0, could iterate if required
				int nLod = 0;
				OptimizedModel::ModelLODHeader_t *pVtxLOD = pVtxModel->pLOD( nLod );

				for ( int nMesh = 0; nMesh < pStudioModel->nummeshes; ++nMesh )
				{
					// check if this mesh's material is in the no shadow material name list
					mstudiomesh_t* pMesh = pStudioModel->pMesh( nMesh );
					mstudiotexture_t *pTxtr=pStudioHdr->pTexture(pMesh->material);
					//printf("mat idx=%d mat name=%s\n",pMesh->material,pTxtr->pszName());
					bool bSkipThisMesh = false;
					for(int check=0; check<g_NonShadowCastingMaterialStrings.Count(); check++)
					{
						if ( Q_stristr( pTxtr->pszName(),
										g_NonShadowCastingMaterialStrings[check] ) )
						{
							//printf("skip mat name=%s\n",pTxtr->pszName());
							bSkipThisMesh = true;
							break;
						}
					}
					if ( bSkipThisMesh)
						continue;

					int shadowTextureIndex = -1;
					if ( dict.m_textureShadowIndex.Count() )
					{
						shadowTextureIndex = dict.m_textureShadowIndex[pMesh->material];
					}


					OptimizedModel::MeshHeader_t* pVtxMesh = pVtxLOD->pMesh( nMesh );
					const mstudio_meshvertexdata_t *vertData = pMesh->GetVertexData( (void *)pStudioHdr );
					Assert( vertData ); // This can only return NULL on X360 for now

					for ( int nGroup = 0; nGroup < pVtxMesh->numStripGroups; ++nGroup )
					{
						OptimizedModel::StripGroupHeader_t* pStripGroup = pVtxMesh->pStripGroup( nGroup );

						int nStrip;
						for ( nStrip = 0; nStrip < pStripGroup->numStrips; nStrip++ )
						{
							OptimizedModel::StripHeader_t *pStrip = pStripGroup->pStrip( nStrip );

							if ( pStrip->flags & OptimizedModel::STRIP_IS_TRILIST )
							{
								for ( int i = 0; i < pStrip->numIndices; i += 3 )
								{
									int idx = pStrip->indexOffset + i;

									unsigned short i1 = *pStripGroup->pIndex( idx );
									unsigned short i2 = *pStripGroup->pIndex( idx + 1 );
									unsigned short i3 = *pStripGroup->pIndex( idx + 2 );

									int vertex1 = pStripGroup->pVertex( i1 )->origMeshVertID;
									int vertex2 = pStripGroup->pVertex( i2 )->origMeshVertID;
									int vertex3 = pStripGroup->pVertex( i3 )->origMeshVertID;

									// transform position into world coordinate system
									matrix3x4_t	matrix;
									AngleMatrix( prop.m_Angles, prop.m_Origin, matrix );

									Vector position1;
									Vector position2;
									Vector position3;
									VectorTransform( *vertData->Position( vertex1 ), matrix, position1 );
									VectorTransform( *vertData->Position( vertex2 ), matrix, position2 );
									VectorTransform( *vertData->Position( vertex3 ), matrix, position3 );
									unsigned short flags = 0;
									int materialIndex = -1;
									Vector color = vec3_origin;
									if ( shadowTextureIndex >= 0 )
									{
										if ( bInitTriangles )
										{
											// add texture space and texture index to material database
											// now
											float coverage = g_ShadowTextureList.ComputeCoverageForTriangle(shadowTextureIndex, *vertData->Texcoord(vertex1), *vertData->Texcoord(vertex2), *vertData->Texcoord(vertex3) );
											if ( coverage < 1.0f )
											{
												materialIndex = g_ShadowTextureList.AddMaterialEntry( shadowTextureIndex, *vertData->Texcoord(vertex1), *vertData->Texcoord(vertex2), *vertData->Texcoord(vertex3) );
												color.x = coverage;
											}
											else
											{
												materialIndex = -1;
											}
											dict.m_triangleMaterialIndex.AddToTail(materialIndex);
										}
										else
										{
											materialIndex = dict.m_triangleMaterialIndex[triangleIndex];
											triangleIndex++;
										}
										if ( materialIndex >= 0 )
										{
											flags = FCACHETRI_TRANSPARENT;
										}
									}
// 		printf( "\ngl 3\n" );
// 		printf( "gl %6.3f %6.3f %6.3f 1 0 0\n", XYZ(position1));
// 		printf( "gl %6.3f %6.3f %6.3f 0 1 0\n", XYZ(position2));
// 		printf( "gl %6.3f %6.3f %6.3f 0 0 1\n", XYZ(position3));
									g_RtEnv.AddTriangle( TRACE_ID_STATICPROP | nProp,
														 position1, position2, position3,
														 color, flags, materialIndex);
								}
							}
							else
							{
								// all tris expected to be discrete tri lists
								// must fixme if stripping ever occurs
								printf( "unexpected strips found\n" );
								Assert( 0 );
								return;
							}
						}
					}
				}
			}
		}
	}
}

struct tl_tri_t
{
	Vector	p0;
	Vector	p1;
	Vector	p2;
	Vector	n0;
	Vector	n1;
	Vector	n2;

	bool operator == (const tl_tri_t &t) const 
	{ 
		return ( p0 == t.p0 && 
				p1 == t.p1 && 
				p2 == t.p2 &&
				n0 == t.n0 &&
				n1 == t.n1 &&
				n2 == t.n2 );
	}
};

struct tl_vert_t
{
	Vector m_position;
	CUtlLinkedList< tl_tri_t, int > m_triList;
};

void AddTriVertsToList( CUtlVector< tl_vert_t > &triListVerts, int vertIndex, Vector vertPosition, Vector p0, Vector p1, Vector p2, Vector n0, Vector n1, Vector n2 )
{
	tl_tri_t tlTri;

	tlTri.p0 = p0;
	tlTri.p1 = p1;
	tlTri.p2 = p2;
	tlTri.n0 = n0;
	tlTri.n1 = n1;
	tlTri.n2 = n2;

	triListVerts.EnsureCapacity( vertIndex+1 );

	triListVerts[vertIndex].m_position = vertPosition;

	int index = triListVerts[vertIndex].m_triList.Find( tlTri );
	if ( !triListVerts[vertIndex].m_triList.IsValidIndex( index ) )
	{
		// not in list, add to list of triangles
		triListVerts[vertIndex].m_triList.AddToTail( tlTri );
	}
}

//-----------------------------------------------------------------------------
// Builds a list of tris for every vertex
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::BuildTriList( CStaticProp &prop )
{
	// the generated list will consist of a list of verts
	// each vert will have a linked list of triangles that it belongs to
	CUtlVector< tl_vert_t >	triListVerts;

	StaticPropDict_t &dict = m_StaticPropDict[prop.m_ModelIdx];
	studiohdr_t	*pStudioHdr = dict.m_pStudioHdr;
	OptimizedModel::FileHeader_t *pVtxHdr = (OptimizedModel::FileHeader_t *)dict.m_VtxBuf.Base();
	if ( !pStudioHdr || !pVtxHdr )
	{
		// must have model and its verts for decoding triangles
		return;
	}

	// meshes are deeply hierarchial, divided between three stores, follow the white rabbit
	// body parts -> models -> lod meshes -> strip groups -> strips
	// the vertices and indices are pooled, the trick is knowing the offset to determine your indexed base 
	for ( int bodyID = 0; bodyID < pStudioHdr->numbodyparts; ++bodyID )
	{
		OptimizedModel::BodyPartHeader_t* pVtxBodyPart = pVtxHdr->pBodyPart( bodyID );
		mstudiobodyparts_t *pBodyPart = pStudioHdr->pBodypart( bodyID );

		for ( int modelID = 0; modelID < pBodyPart->nummodels; ++modelID )
		{
			OptimizedModel::ModelHeader_t* pVtxModel = pVtxBodyPart->pModel( modelID );
			mstudiomodel_t *pStudioModel = pBodyPart->pModel( modelID );

			// get the specified lod, assuming lod 0
			int nLod = 0;
			OptimizedModel::ModelLODHeader_t *pVtxLOD = pVtxModel->pLOD( nLod );

			// must reset because each model has their own vertexes [0..n]
			// in order for this to be monolithic for the entire prop the list must be segmented
			triListVerts.Purge();

			for ( int nMesh = 0; nMesh < pStudioModel->nummeshes; ++nMesh )
			{
				mstudiomesh_t* pMesh = pStudioModel->pMesh( nMesh );
				OptimizedModel::MeshHeader_t* pVtxMesh = pVtxLOD->pMesh( nMesh );
				const mstudio_meshvertexdata_t *vertData = pMesh->GetVertexData( (void *)pStudioHdr );
				Assert( vertData ); // This can only return NULL on X360 for now

				for ( int nGroup = 0; nGroup < pVtxMesh->numStripGroups; ++nGroup )
				{
					OptimizedModel::StripGroupHeader_t* pStripGroup = pVtxMesh->pStripGroup( nGroup );

					int nStrip;
					for ( nStrip = 0; nStrip < pStripGroup->numStrips; nStrip++ )
					{
						OptimizedModel::StripHeader_t *pStrip = pStripGroup->pStrip( nStrip );

						if ( pStrip->flags & OptimizedModel::STRIP_IS_TRILIST )
						{
							for ( int i = 0; i < pStrip->numIndices; i += 3 )
							{
								int idx = pStrip->indexOffset + i;

								unsigned short i1 = *pStripGroup->pIndex( idx );
								unsigned short i2 = *pStripGroup->pIndex( idx + 1 );
								unsigned short i3 = *pStripGroup->pIndex( idx + 2 );

								int vertex1 = pStripGroup->pVertex( i1 )->origMeshVertID;
								int vertex2 = pStripGroup->pVertex( i2 )->origMeshVertID;
								int vertex3 = pStripGroup->pVertex( i3 )->origMeshVertID;

								// transform position into world coordinate system
								matrix3x4_t	matrix;
								AngleMatrix( prop.m_Angles, prop.m_Origin, matrix );

								Vector position1;
								Vector position2;
								Vector position3;
								VectorTransform( *vertData->Position( vertex1 ), matrix, position1 );
								VectorTransform( *vertData->Position( vertex2 ), matrix, position2 );
								VectorTransform( *vertData->Position( vertex3 ), matrix, position3 );

								Vector normal1;
								Vector normal2;
								Vector normal3;
								VectorTransform( *vertData->Normal( vertex1 ), matrix, normal1 );
								VectorTransform( *vertData->Normal( vertex2 ), matrix, normal2 );
								VectorTransform( *vertData->Normal( vertex3 ), matrix, normal3 );

								AddTriVertsToList( triListVerts, pMesh->vertexoffset + vertex1, position1, position1, position2, position3, normal1, normal2, normal3 );
								AddTriVertsToList( triListVerts, pMesh->vertexoffset + vertex2, position2, position1, position2, position3, normal1, normal2, normal3 );
								AddTriVertsToList( triListVerts, pMesh->vertexoffset + vertex3, position3, position1, position2, position3, normal1, normal2, normal3 );
							}
						}
						else
						{
							// all tris expected to be discrete tri lists
							// must fixme if stripping ever occurs
							printf( "unexpected strips found\n" );
							Assert( 0 );
							return;
						}
					}
				}
			}
		}
	}
}

const vertexFileHeader_t * mstudiomodel_t::CacheVertexData( void *pModelData )
{
	studiohdr_t *pActiveStudioHdr = static_cast<studiohdr_t *>(pModelData);
	Assert( pActiveStudioHdr );

	if ( pActiveStudioHdr->pVertexBase )
	{
		return (vertexFileHeader_t *)pActiveStudioHdr->pVertexBase;
	}

	// mandatory callback to make requested data resident
	// load and persist the vertex file
	char fileName[MAX_PATH];
	strcpy( fileName, "models/" );	
	strcat( fileName, pActiveStudioHdr->pszName() );
	Q_StripExtension( fileName, fileName, sizeof( fileName ) );
	strcat( fileName, ".vvd" );

	// load the model
	FileHandle_t fileHandle = g_pFileSystem->Open( fileName, "rb" );
	if ( !fileHandle )
	{
		Error( "Unable to load vertex data \"%s\"\n", fileName );
	}

	// Get the file size
	int vvdSize = g_pFileSystem->Size( fileHandle );
	if ( vvdSize == 0 )
	{
		g_pFileSystem->Close( fileHandle );
		Error( "Bad size for vertex data \"%s\"\n", fileName );
	}

	vertexFileHeader_t *pVvdHdr = (vertexFileHeader_t *)malloc( vvdSize );
	g_pFileSystem->Read( pVvdHdr, vvdSize, fileHandle );
	g_pFileSystem->Close( fileHandle );

	// check header
	if ( pVvdHdr->id != MODEL_VERTEX_FILE_ID )
	{
		Error("Error Vertex File %s id %d should be %d\n", fileName, pVvdHdr->id, MODEL_VERTEX_FILE_ID);
	}
	if ( pVvdHdr->version != MODEL_VERTEX_FILE_VERSION )
	{
		Error("Error Vertex File %s version %d should be %d\n", fileName, pVvdHdr->version, MODEL_VERTEX_FILE_VERSION);
	}
	if ( pVvdHdr->checksum != pActiveStudioHdr->checksum )
	{
		Error("Error Vertex File %s checksum %d should be %d\n", fileName, pVvdHdr->checksum, pActiveStudioHdr->checksum);
	}

	// need to perform mesh relocation fixups
	// allocate a new copy
	vertexFileHeader_t *pNewVvdHdr = (vertexFileHeader_t *)malloc( vvdSize );
	if ( !pNewVvdHdr )
	{
		Error( "Error allocating %d bytes for Vertex File '%s'\n", vvdSize, fileName );
	}

	// load vertexes and run fixups
	Studio_LoadVertexes( pVvdHdr, pNewVvdHdr, 0, true );

	// discard original
	free( pVvdHdr );
	pVvdHdr = pNewVvdHdr;

	pActiveStudioHdr->pVertexBase = (void*)pVvdHdr;
	return pVvdHdr;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
struct ColorTexelValue
{
	Vector mLinearColor;	// Linear color value for this texel
	bool mValidData;		// Whether there is valid data in this texel.
	size_t mTriangleIndex;	// Which triangle we used to generate the texel.
};

// ------------------------------------------------------------------------------------------------
inline int ComputeLinearPos( int _x, int _y, int _resX, int _resY )
{
	return Min( Max( 0, _y ), _resY - 1 ) * _resX
		 + Min( Max( 0, _x ), _resX - 1 );
}

// ------------------------------------------------------------------------------------------------
inline float ComputeBarycentricDistanceToTri( Vector _barycentricCoord, Vector2D _v[3] )
{
	Vector2D realPos = _barycentricCoord.x * _v[0]
		             + _barycentricCoord.y * _v[1]
					 + _barycentricCoord.z * _v[2];

	int minIndex = 0;
	float minVal = _barycentricCoord[0];
	for (int i = 1; i < 3; ++i) {
		if (_barycentricCoord[i] < minVal) {
			minVal = _barycentricCoord[i];
			minIndex = i;
		}
	}

	Vector2D& first  = _v[ (minIndex + 1) % 3];
	Vector2D& second = _v[ (minIndex + 2) % 3];

	return CalcDistanceToLineSegment2D( realPos, first, second );
}

// ------------------------------------------------------------------------------------------------
static void GenerateLightmapSamplesForMesh( const matrix3x4_t& _matPos, const matrix3x4_t& _matNormal, int _iThread, int _skipProp, int _flags, int _lightmapResX, int _lightmapResY, studiohdr_t* _pStudioHdr, mstudiomodel_t* _pStudioModel, OptimizedModel::ModelHeader_t* _pVtxModel, int _meshID, CComputeStaticPropLightingResults *_outResults )
{
	// Could iterate and gen this if needed.
	int nLod = 0;

	OptimizedModel::ModelLODHeader_t *pVtxLOD = _pVtxModel->pLOD(nLod);

	CUtlVector<colorTexel_t> &colorTexels = (*_outResults->m_ColorTexelsArrays.Tail());
	const int cTotalPixelCount = _lightmapResX * _lightmapResY;
	colorTexels.EnsureCount(cTotalPixelCount);
	memset(colorTexels.Base(), 0, colorTexels.Count() * sizeof(colorTexel_t));

	for (int i = 0; i < colorTexels.Count(); ++i) {
		colorTexels[i].m_fDistanceToTri = FLT_MAX;	
	}

	mstudiomesh_t* pMesh = _pStudioModel->pMesh(_meshID);
	OptimizedModel::MeshHeader_t* pVtxMesh = pVtxLOD->pMesh(_meshID);
	const mstudio_meshvertexdata_t *vertData = pMesh->GetVertexData((void *)_pStudioHdr);
	Assert(vertData); // This can only return NULL on X360 for now

	for (int nGroup = 0; nGroup < pVtxMesh->numStripGroups; ++nGroup)
	{
		OptimizedModel::StripGroupHeader_t* pStripGroup = pVtxMesh->pStripGroup(nGroup);

		int nStrip;
		for (nStrip = 0; nStrip < pStripGroup->numStrips; nStrip++)
		{
			OptimizedModel::StripHeader_t *pStrip = pStripGroup->pStrip(nStrip);

			// If this hits, re-factor the code to iterate over triangles, and build the triangles
			// from the underlying structures.
			Assert((pStrip->flags & OptimizedModel::STRIP_IS_TRISTRIP) == 0);

			if (pStrip->flags & OptimizedModel::STRIP_IS_TRILIST)
			{
				for (int i = 0; i < pStrip->numIndices; i += 3)
				{
					int idx = pStrip->indexOffset + i;

					unsigned short i1 = *pStripGroup->pIndex(idx);
					unsigned short i2 = *pStripGroup->pIndex(idx + 1);
					unsigned short i3 = *pStripGroup->pIndex(idx + 2);

					int vertex1 = pStripGroup->pVertex(i1)->origMeshVertID;
					int vertex2 = pStripGroup->pVertex(i2)->origMeshVertID;
					int vertex3 = pStripGroup->pVertex(i3)->origMeshVertID;

					Vector modelPos[3] = {
						*vertData->Position(vertex1),
						*vertData->Position(vertex2),
						*vertData->Position(vertex3)
					};

					Vector modelNormal[3] = {
						*vertData->Normal(vertex1),
						*vertData->Normal(vertex2),
						*vertData->Normal(vertex3)
					};

					Vector worldPos[3];
					Vector worldNormal[3];

					VectorTransform(modelPos[0], _matPos, worldPos[0]);
					VectorTransform(modelPos[1], _matPos, worldPos[1]);
					VectorTransform(modelPos[2], _matPos, worldPos[2]);

					VectorTransform(modelNormal[0], _matNormal, worldNormal[0]);
					VectorTransform(modelNormal[1], _matNormal, worldNormal[1]);
					VectorTransform(modelNormal[2], _matNormal, worldNormal[2]);

					Vector2D texcoord[3] = { 
						*vertData->Texcoord(vertex1),
						*vertData->Texcoord(vertex2),
						*vertData->Texcoord(vertex3)
					};

					Rasterizer rasterizer(texcoord[0], texcoord[1], texcoord[2],
					                      _lightmapResX, _lightmapResY);

					for (auto it = rasterizer.begin(); it != rasterizer.end(); ++it)
					{
						size_t linearPos = rasterizer.GetLinearPos(it);
						Assert(linearPos < cTotalPixelCount);

						if ( colorTexels[linearPos].m_bValid )
						{
							continue;
						}						

						float ourDistancetoTri = ComputeBarycentricDistanceToTri( it->barycentric, texcoord );

						bool doWrite =  it->insideTriangle
							        || !colorTexels[linearPos].m_bPossiblyInteresting
									||  colorTexels[linearPos].m_fDistanceToTri > ourDistancetoTri;

						if (doWrite)
						{
							Vector itWorldPos = worldPos[0] * it->barycentric.x
											  + worldPos[1] * it->barycentric.y
											  + worldPos[2] * it->barycentric.z;

							Vector itWorldNormal = worldNormal[0] * it->barycentric.x
												 + worldNormal[1] * it->barycentric.y
												 + worldNormal[2] * it->barycentric.z;
							itWorldNormal.NormalizeInPlace();

							colorTexels[linearPos].m_WorldPosition = itWorldPos;
							colorTexels[linearPos].m_WorldNormal = itWorldNormal;
							colorTexels[linearPos].m_bValid = it->insideTriangle;
							colorTexels[linearPos].m_bPossiblyInteresting = true;
							colorTexels[linearPos].m_fDistanceToTri = ourDistancetoTri;
						}
					}
				}
			}
		}
	}

	// Process neighbors to the valid region. Walk through the existing array, look for samples that
	// are not valid but are adjacent to valid samples. Works if we are only bilinearly sampling
	// on the other side.
	// First attempt: Just pretend the triangle was larger and cast a ray from this new world pos 
	// as above.
	int linearPos = 0;
	for ( int j = 0; j < _lightmapResY; ++j )
	{
		for (int i = 0; i < _lightmapResX; ++i )
		{
			bool shouldProcess = colorTexels[linearPos].m_bValid;
			// Are any of the eight neighbors valid??
			if ( colorTexels[linearPos].m_bPossiblyInteresting )
			{
				// Look at our neighborhood (3x3 centerd on us). 
				shouldProcess = shouldProcess
				             || colorTexels[ComputeLinearPos( i - 1, j - 1, _lightmapResX, _lightmapResY )].m_bValid  // TL
							 || colorTexels[ComputeLinearPos( i    , j - 1, _lightmapResX, _lightmapResY )].m_bValid  // T
							 || colorTexels[ComputeLinearPos( i + 1, j - 1, _lightmapResX, _lightmapResY )].m_bValid  // TR

							 || colorTexels[ComputeLinearPos( i - 1, j    , _lightmapResX, _lightmapResY )].m_bValid  // L
							 || colorTexels[ComputeLinearPos( i + 1, j    , _lightmapResX, _lightmapResY )].m_bValid  // R

							 || colorTexels[ComputeLinearPos( i - 1, j + 1, _lightmapResX, _lightmapResY )].m_bValid  // BL
							 || colorTexels[ComputeLinearPos( i    , j + 1, _lightmapResX, _lightmapResY )].m_bValid  // B
							 || colorTexels[ComputeLinearPos( i + 1, j + 1, _lightmapResX, _lightmapResY )].m_bValid; // BR
			}

			if (shouldProcess)
			{
				Vector directColor(0, 0, 0),
					   indirectColor(0, 0, 0);


				ComputeDirectLightingAtPoint( colorTexels[linearPos].m_WorldPosition, colorTexels[linearPos].m_WorldNormal, directColor, _iThread, _skipProp, _flags);

				if (numbounce >= 1) {
					ComputeIndirectLightingAtPoint( colorTexels[linearPos].m_WorldPosition, colorTexels[linearPos].m_WorldNormal, indirectColor, _iThread, true, (_flags & GATHERLFLAGS_IGNORE_NORMALS) != 0 );
				}

				VectorAdd(directColor, indirectColor, colorTexels[linearPos].m_Color);
			}

			++linearPos;
		}
	}
}

// ------------------------------------------------------------------------------------------------
static int GetTexelCount(unsigned int _resX, unsigned int _resY, bool _mipmaps)
{
	// Because they are unsigned, this is a != check--but if we were to change to ints, this would be
	// the right assert (and it's no worse than != now). 
	Assert(_resX > 0 && _resY > 0);

	if (_mipmaps == false)
		return _resX * _resY;

	int retVal = 0;
	while (_resX > 1 || _resY > 1) 
	{
		retVal += _resX * _resY;
		_resX = max(1, _resX >> 1);
		_resY = max(1, _resY >> 1);
	}

	// Add in the 1x1 mipmap level, which wasn't hit above. This could be done in the initializer of 
	// retVal, but it's more obvious here. 
	retVal += 1;

	return retVal;
}

// ------------------------------------------------------------------------------------------------
static void FilterFineMipmap(unsigned int _resX, unsigned int _resY, const CUtlVector<colorTexel_t>& _srcTexels, CUtlVector<Vector>* _outLinear)
{
	Assert(_outLinear);
	// We can't filter in place, so go ahead and create a linear buffer here.
	CUtlVector<Vector> filterSrc;
	filterSrc.EnsureCount(_srcTexels.Count());

	for (int i = 0; i < _srcTexels.Count(); ++i)
	{
		ColorRGBExp32 rgbColor;
		VectorToColorRGBExp32(_srcTexels[i].m_Color, rgbColor);
		ConvertRGBExp32ToLinear( &rgbColor, &(filterSrc[i]) );
	}

	const int cRadius = 1;
	const float cOneOverDiameter = 1.0f / pow(2.0f * cRadius + 1.0f, 2.0f) ;
	// Filter here.
	for (int j = 0; j < _resY; ++j) 
	{
		for (int i = 0; i < _resX; ++i)
		{
			Vector value(0, 0, 0);
			int thisIndex = ComputeLinearPos(i, j, _resX, _resY);

			if (!_srcTexels[thisIndex].m_bValid)
			{
				(*_outLinear)[thisIndex] = filterSrc[thisIndex];
				continue;
			}

			// TODO: Check ASM for this, unroll by hand if needed.
			for ( int offsetJ = -cRadius; offsetJ <= cRadius; ++offsetJ )
			{
				for ( int offsetI = -cRadius; offsetI <= cRadius; ++offsetI )
				{
					int finalIndex = ComputeLinearPos( i + offsetI, j + offsetJ, _resX, _resY );
					if ( !_srcTexels[finalIndex].m_bValid )
					{
						finalIndex = thisIndex;
					}
						
					value += filterSrc[finalIndex];
				}
			}

			(*_outLinear)[thisIndex] = value * cOneOverDiameter;
		}
	}
}

// ------------------------------------------------------------------------------------------------
static void BuildFineMipmap(unsigned int _resX, unsigned int _resY, bool _applyFilter, const CUtlVector<colorTexel_t>& _srcTexels, CUtlVector<RGB888_t>* _outTexelsRGB888, CUtlVector<Vector>* _outLinear)
{
	// At least one of these needs to be non-null, otherwise what are we doing here?
	Assert(_outTexelsRGB888 || _outLinear);
	Assert(!_applyFilter || _outLinear);
	Assert(_srcTexels.Count() == GetTexelCount(_resX, _resY, false));

	int texelCount = GetTexelCount(_resX, _resY, true);

	if (_outTexelsRGB888)
		(*_outTexelsRGB888).EnsureCount(texelCount);

	if (_outLinear)
		(*_outLinear).EnsureCount(GetTexelCount(_resX, _resY, false));

	// This code can take awhile, so minimize the branchiness of the inner-loop. 
	if (_applyFilter)
	{

		FilterFineMipmap(_resX, _resY, _srcTexels, _outLinear);

		if ( _outTexelsRGB888 )
		{
			for (int i = 0; i < _srcTexels.Count(); ++i) 
			{
				RGBA8888_t encodedColor;

				Vector linearColor = (*_outLinear)[i];

				ConvertLinearToRGBA8888( &linearColor, (unsigned char*)&encodedColor );
				(*_outTexelsRGB888)[i].r = encodedColor.r;
				(*_outTexelsRGB888)[i].g = encodedColor.g;
				(*_outTexelsRGB888)[i].b = encodedColor.b;
			}
		}
	}
	else
	{
		for (int i = 0; i < _srcTexels.Count(); ++i) 
		{
			ColorRGBExp32 rgbColor;
			RGBA8888_t encodedColor;
			VectorToColorRGBExp32(_srcTexels[i].m_Color, rgbColor);
			ConvertRGBExp32ToRGBA8888(&rgbColor, (unsigned char*)&encodedColor, (_outLinear ? (&(*_outLinear)[i]) : NULL) );
			// We drop alpha on the floor here, if this were to fire we'd need to consider using a different compressed format.
			Assert(encodedColor.a == 0xFF);

			if (_outTexelsRGB888)
			{
				(*_outTexelsRGB888)[i].r = encodedColor.r;
				(*_outTexelsRGB888)[i].g = encodedColor.g;
				(*_outTexelsRGB888)[i].b = encodedColor.b;
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
static void FilterCoarserMipmaps(unsigned int _resX, unsigned int _resY, CUtlVector<Vector>* _scratchLinear, CUtlVector<RGB888_t> *_outTexelsRGB888)
{
	Assert(_outTexelsRGB888);

	int srcResX = _resX;
	int srcResY = _resY;
	int dstResX = max(1, (srcResX >> 1));
	int dstResY = max(1, (srcResY >> 1));
	int dstOffset = GetTexelCount(srcResX, srcResY, false);

	// Build mipmaps here, after being converted to linear space. 
	// TODO: Should do better filtering for downsampling. But this will work for now.
	while (srcResX > 1 || srcResY > 1)
	{
		for (int j = 0; j < srcResY; j += 2) {
			for (int i = 0; i < srcResX; i += 2) {
				int srcCol0 = i;
				int srcCol1 = i + 1 > srcResX - 1 ? srcResX - 1 : i + 1;
				int srcRow0 = j;
				int srcRow1 = j + 1 > srcResY - 1 ? srcResY - 1 : j + 1;;

				int dstCol = i >> 1;
				int dstRow = j >> 1;


				const Vector& tl = (*_scratchLinear)[srcCol0 + (srcRow0 * srcResX)];
				const Vector& tr = (*_scratchLinear)[srcCol1 + (srcRow0 * srcResX)];
				const Vector& bl = (*_scratchLinear)[srcCol0 + (srcRow1 * srcResX)];
				const Vector& br = (*_scratchLinear)[srcCol1 + (srcRow1 * srcResX)];

				Vector sample = (tl + tr + bl + br) / 4.0f;

				ConvertLinearToRGBA8888(&sample, (unsigned char*)&(*_outTexelsRGB888)[dstOffset + dstCol + dstRow * dstResX]);

				// Also overwrite the srcBuffer to filter the next loop. This is safe because we won't be reading this source value
				// again during this mipmap level.
				(*_scratchLinear)[dstCol + dstRow * dstResX] = sample;
			}
		}

		srcResX = dstResX;
		srcResY = dstResY;
		dstResX = max(1, (srcResX >> 1));
		dstResY = max(1, (srcResY >> 1));
		dstOffset += GetTexelCount(srcResX, srcResY, false);
	}
}

// ------------------------------------------------------------------------------------------------
static void ConvertToDestinationFormat(unsigned int _resX, unsigned int _resY, ImageFormat _destFmt, const CUtlVector<RGB888_t>& _scratchRBG888, CUtlMemory<byte>* _outTexture)
{
	const ImageFormat cSrcImageFormat = IMAGE_FORMAT_RGB888;

	// Converts from the scratch RGB888 buffer, which should be fully filled out to the output texture.
	int destMemoryUsage = ImageLoader::GetMemRequired(_resX, _resY, 1, _destFmt, true);
	(*_outTexture).EnsureCapacity(destMemoryUsage);

	int srcResX = _resX;
	int srcResY = _resY;
	int srcOffset = 0;
	int dstOffset = 0;

	// The usual case--that they'll be different.
	if (cSrcImageFormat != _destFmt)
	{
		while (srcResX > 1 || srcResY > 1)
		{
			// Convert this mipmap level.
			ImageLoader::ConvertImageFormat((unsigned char*)(&_scratchRBG888[srcOffset]), cSrcImageFormat, (*_outTexture).Base() + dstOffset, _destFmt, srcResX, srcResY);

			// Then update offsets for the next mipmap level.
			srcOffset += GetTexelCount(srcResX, srcResY, false);
			dstOffset += ImageLoader::GetMemRequired(srcResX, srcResY, 1, _destFmt, false);

			srcResX = max(1, (srcResX >> 1));
			srcResY = max(1, (srcResY >> 1));
		}

		// Do the 1x1 level also.
		ImageLoader::ConvertImageFormat((unsigned char*)_scratchRBG888.Base() + srcOffset, cSrcImageFormat, (*_outTexture).Base() + dstOffset, _destFmt, srcResX, srcResY);
	} else {
		// But sometimes (particularly for debugging) they will be the same.
		Q_memcpy( (*_outTexture).Base(), _scratchRBG888.Base(), destMemoryUsage );
	}
}

// ------------------------------------------------------------------------------------------------
static void ConvertTexelDataToTexture(unsigned int _resX, unsigned int _resY, ImageFormat _destFmt, const CUtlVector<colorTexel_t>& _srcTexels, CUtlMemory<byte>* _outTexture)
{
	Assert(_outTexture);
	Assert(_srcTexels.Count() == _resX * _resY);

	CUtlVector<RGB888_t> scratchRGB888;
	CUtlVector<Vector> scratchLinear;

	BuildFineMipmap(_resX, _resY, true, _srcTexels, &scratchRGB888, &scratchLinear);
	FilterCoarserMipmaps(_resX, _resY, &scratchLinear, &scratchRGB888 );
	ConvertToDestinationFormat(_resX, _resY, _destFmt, scratchRGB888, _outTexture);
}

// ------------------------------------------------------------------------------------------------
static void DumpLightmapLinear( const char* _dstFilename, const CUtlVector<colorTexel_t>& _srcTexels, int _width, int _height )
{
	CUtlVector< Vector > linearFloats;
	CUtlVector< BGR888_t > linearBuffer;
	BuildFineMipmap( _width, _height, true, _srcTexels, NULL, &linearFloats );
	linearBuffer.SetCount( linearFloats.Count() );

	for ( int i = 0; i < linearFloats.Count(); ++i ) {
		linearBuffer[i].b = RoundFloatToByte(linearFloats[i].z * 255.0f);
		linearBuffer[i].g = RoundFloatToByte(linearFloats[i].y * 255.0f);
		linearBuffer[i].r = RoundFloatToByte(linearFloats[i].x * 255.0f);
	}
	
	TGAWriter::WriteTGAFile( _dstFilename, _width, _height, IMAGE_FORMAT_BGR888, (uint8*)(linearBuffer.Base()), _width * ImageLoader::SizeInBytes(IMAGE_FORMAT_BGR888) );
}
