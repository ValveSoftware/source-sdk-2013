//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef ISTUDIORENDER_H
#define ISTUDIORENDER_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"
#include "mathlib/vector.h"
#include "mathlib/vector4d.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlvector.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystem.h"
#include "appframework/IAppSystem.h"
#include "datacache/imdlcache.h"
#include "studio.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
struct studiohdr_t;
struct studiomeshdata_t;
class Vector;
struct LightDesc_t;
class IMaterial;
struct studiohwdata_t;
struct Ray_t;
class Vector4D;
class IMaterialSystem;
struct matrix3x4_t;
class IMesh;
struct vertexFileHeader_t;
struct FlashlightState_t;
class VMatrix;
namespace OptimizedModel { struct FileHeader_t; }
class IPooledVBAllocator;

// undone: what's the standard for function type naming?
typedef void (*StudioRender_Printf_t)( PRINTF_FORMAT_STRING const char *fmt, ... );

struct StudioRenderConfig_t
{
	float fEyeShiftX;	// eye X position
	float fEyeShiftY;	// eye Y position
	float fEyeShiftZ;	// eye Z position
	float fEyeSize;		// adjustment to iris textures
	float fEyeGlintPixelWidthLODThreshold;

	int maxDecalsPerModel;
	int drawEntities;
	int skin;
	int fullbright;

	bool bEyeMove : 1;		// look around
	bool bSoftwareSkin : 1;
	bool bNoHardware : 1;
	bool bNoSoftware : 1;
	bool bTeeth : 1;
	bool bEyes : 1;
	bool bFlex : 1;
	bool bWireframe : 1;
	bool bDrawNormals : 1;
	bool bDrawTangentFrame : 1;
	bool bDrawZBufferedWireframe : 1;
	bool bSoftwareLighting : 1;
	bool bShowEnvCubemapOnly : 1;
	bool bWireframeDecals : 1;

	// Reserved for future use
	int m_nReserved[4];
};



//-----------------------------------------------------------------------------
// Studio render interface
//-----------------------------------------------------------------------------
DECLARE_POINTER_HANDLE( StudioDecalHandle_t );
#define STUDIORENDER_DECAL_INVALID  ( (StudioDecalHandle_t)0 )

enum
{
	ADDDECAL_TO_ALL_LODS = -1
};


//-----------------------------------------------------------------------------
// DrawModel flags
//-----------------------------------------------------------------------------
enum
{
	STUDIORENDER_DRAW_ENTIRE_MODEL		= 0,
	STUDIORENDER_DRAW_OPAQUE_ONLY		= 0x01,
	STUDIORENDER_DRAW_TRANSLUCENT_ONLY	= 0x02,
	STUDIORENDER_DRAW_GROUP_MASK		= 0x03,

	STUDIORENDER_DRAW_NO_FLEXES			= 0x04,
	STUDIORENDER_DRAW_STATIC_LIGHTING	= 0x08,

	STUDIORENDER_DRAW_ACCURATETIME		= 0x10,		// Use accurate timing when drawing the model.
	STUDIORENDER_DRAW_NO_SHADOWS		= 0x20,
	STUDIORENDER_DRAW_GET_PERF_STATS	= 0x40,

	STUDIORENDER_DRAW_WIREFRAME			= 0x80,

	STUDIORENDER_DRAW_ITEM_BLINK		= 0x100,

	STUDIORENDER_SHADOWDEPTHTEXTURE		= 0x200,

	STUDIORENDER_SSAODEPTHTEXTURE				= 0x1000,

	STUDIORENDER_GENERATE_STATS					= 0x8000,
};


//-----------------------------------------------------------------------------
// Standard model vertex formats
//-----------------------------------------------------------------------------
// FIXME: remove these (materials/shaders should drive vertex format). Need to
//        list required forcedmaterialoverrides in models/bsps (rather than
//        all models supporting all possible overrides, as they do currently).
#define VERTEX_TEXCOORD0_2D ( ( (uint64) 2 ) << ( TEX_COORD_SIZE_BIT + ( 3*0 ) ) )
enum MaterialVertexFormat_t
{
	MATERIAL_VERTEX_FORMAT_MODEL_SKINNED		= (VertexFormat_t) VERTEX_POSITION | VERTEX_COLOR | VERTEX_NORMAL | VERTEX_TEXCOORD0_2D | VERTEX_BONEWEIGHT(2) | VERTEX_BONE_INDEX | VERTEX_USERDATA_SIZE(4),
	MATERIAL_VERTEX_FORMAT_MODEL_SKINNED_DX7	= (VertexFormat_t) VERTEX_POSITION | VERTEX_COLOR | VERTEX_NORMAL | VERTEX_TEXCOORD0_2D | VERTEX_BONEWEIGHT(2) | VERTEX_BONE_INDEX,
	MATERIAL_VERTEX_FORMAT_MODEL				= (VertexFormat_t) VERTEX_POSITION | VERTEX_COLOR | VERTEX_NORMAL | VERTEX_TEXCOORD0_2D | VERTEX_USERDATA_SIZE(4),
	MATERIAL_VERTEX_FORMAT_MODEL_DX7			= (VertexFormat_t) VERTEX_POSITION | VERTEX_COLOR | VERTEX_NORMAL | VERTEX_TEXCOORD0_2D,
	MATERIAL_VERTEX_FORMAT_COLOR				= (VertexFormat_t) VERTEX_SPECULAR
};


//-----------------------------------------------------------------------------
// What kind of material override is it?
//-----------------------------------------------------------------------------
enum OverrideType_t
{
	OVERRIDE_NORMAL = 0,
	OVERRIDE_BUILD_SHADOWS,
	OVERRIDE_DEPTH_WRITE,
	OVERRIDE_SSAO_DEPTH_WRITE,
};


//-----------------------------------------------------------------------------
// DrawModel info
//-----------------------------------------------------------------------------

// Special flag for studio models that have a compiled in shadow lod version
// It's negative 2 since positive numbers == use a regular slot and -1 means
//  have studiorender compute a value instead
enum
{
	USESHADOWLOD = -2,
};

// beyond this number of materials, you won't get info back from DrawModel
#define MAX_DRAW_MODEL_INFO_MATERIALS 8

struct DrawModelResults_t
{
	int m_ActualTriCount; 
	int m_TextureMemoryBytes;
	int m_NumHardwareBones;
	int m_NumBatches;
	int m_NumMaterials;
	int m_nLODUsed;
	int m_flLODMetric;
	CFastTimer m_RenderTime;
	CUtlVectorFixed<IMaterial *,MAX_DRAW_MODEL_INFO_MATERIALS> m_Materials;
};

struct ColorTexelsInfo_t
{
	int						m_nWidth;
	int						m_nHeight;
	int						m_nMipmapCount;
	ImageFormat				m_ImageFormat;
	int						m_nByteCount;
	byte*					m_pTexelData;
};

struct ColorMeshInfo_t
{
	// A given color mesh can own a unique Mesh, or it can use a shared Mesh
	// (in which case it uses a sub-range defined by m_nVertOffset and m_nNumVerts)
	IMesh				*	m_pMesh;
	IPooledVBAllocator	*	m_pPooledVBAllocator;
	int						m_nVertOffsetInBytes;
	int						m_nNumVerts;
	ITexture			*   m_pLightmap;
	ColorTexelsInfo_t   *   m_pLightmapData;
};

struct DrawModelInfo_t
{
	studiohdr_t		*m_pStudioHdr;
	studiohwdata_t	*m_pHardwareData;
	StudioDecalHandle_t m_Decals;
	int				m_Skin;
	int				m_Body;
	int				m_HitboxSet;
	void			*m_pClientEntity;
	int				m_Lod;
	ColorMeshInfo_t	*m_pColorMeshes;
	bool			m_bStaticLighting;
	Vector			m_vecAmbientCube[6];		// ambient, and lights that aren't in locallight[]
	int				m_nLocalLightCount;
	LightDesc_t		m_LocalLightDescs[4];
};

struct GetTriangles_Vertex_t
{
	Vector m_Position;
	Vector m_Normal;
	Vector4D m_TangentS;
	Vector2D m_TexCoord;
	Vector4D m_BoneWeight;
	int m_BoneIndex[4];
	int m_NumBones;
};

struct GetTriangles_MaterialBatch_t
{
	IMaterial *m_pMaterial;
	CUtlVector<GetTriangles_Vertex_t> m_Verts;
	CUtlVector<int> m_TriListIndices;
};

struct GetTriangles_Output_t
{
	CUtlVector<GetTriangles_MaterialBatch_t> m_MaterialBatches;
	matrix3x4_t m_PoseToWorld[MAXSTUDIOBONES];
};


struct model_array_instance_t 
{
	matrix3x4_t		modelToWorld;

	// UNDONE: Per instance lighting values?
};

//-----------------------------------------------------------------------------
// Cache Callback Function
// implementation can either statically persist data (tools) or lru cache (engine) it.
// caller returns base pointer to resident data.
// code expectes data to be dynamic and invokes cache callback prior to iterative access.
// virtualModel is member passed in via studiohdr_t and passed back for model identification.
//-----------------------------------------------------------------------------
#define STUDIO_DATA_CACHE_INTERFACE_VERSION "VStudioDataCache005"
 
abstract_class IStudioDataCache : public IAppSystem
{
public:
	virtual bool VerifyHeaders( studiohdr_t *pStudioHdr ) = 0;
	virtual vertexFileHeader_t *CacheVertexData( studiohdr_t *pStudioHdr ) = 0;
};


//-----------------------------------------------------------------------------
// Studio render interface
//-----------------------------------------------------------------------------
#define STUDIO_RENDER_INTERFACE_VERSION "VStudioRender025"

abstract_class IStudioRender : public IAppSystem
{
public:
	virtual void BeginFrame( void ) = 0;
	virtual void EndFrame( void ) = 0;

	// Used for the mat_stub console command.
	virtual void Mat_Stub( IMaterialSystem *pMatSys ) = 0;

	// Updates the rendering configuration 
	virtual void UpdateConfig( const StudioRenderConfig_t& config ) = 0;
	virtual void GetCurrentConfig( StudioRenderConfig_t& config ) = 0;

	// Load, unload model data
	virtual bool LoadModel( studiohdr_t *pStudioHdr, void *pVtxData, studiohwdata_t	*pHardwareData ) = 0;
	virtual void UnloadModel( studiohwdata_t *pHardwareData ) = 0;

	// Refresh the studiohdr since it was lost...
	virtual void RefreshStudioHdr( studiohdr_t* pStudioHdr, studiohwdata_t* pHardwareData ) = 0;

	// This is needed to do eyeglint and calculate the correct texcoords for the eyes.
	virtual void SetEyeViewTarget( const studiohdr_t *pStudioHdr, int nBodyIndex, const Vector& worldPosition ) = 0;
		
	// Methods related to lighting state
	// NOTE: SetAmbientLightColors assumes that the arraysize is the same as 
	// returned from GetNumAmbientLightSamples
	virtual int GetNumAmbientLightSamples() = 0;
	virtual const Vector *GetAmbientLightDirections() = 0;
	virtual void SetAmbientLightColors( const Vector4D *pAmbientOnlyColors ) = 0;
	virtual void SetAmbientLightColors( const Vector *pAmbientOnlyColors ) = 0;
	virtual void SetLocalLights( int numLights, const LightDesc_t *pLights ) = 0;

	// Sets information about the camera location + orientation
	virtual void SetViewState( const Vector& viewOrigin, const Vector& viewRight, 
		const Vector& viewUp, const Vector& viewPlaneNormal ) = 0;
	
	// Allocates flex weights for use in rendering
	// NOTE: Pass in a non-null second parameter to lock delayed flex weights
	virtual void LockFlexWeights( int nWeightCount, float **ppFlexWeights, float **ppFlexDelayedWeights = NULL ) = 0;
	virtual void UnlockFlexWeights() = 0;
	
	// Used to allocate bone matrices to be used to pass into DrawModel
	virtual matrix3x4_t* LockBoneMatrices( int nBoneCount ) = 0;
	virtual void UnlockBoneMatrices() = 0;
	
	// LOD stuff
	virtual int GetNumLODs( const studiohwdata_t &hardwareData ) const = 0;
	virtual float GetLODSwitchValue( const studiohwdata_t &hardwareData, int lod ) const = 0;
	virtual void SetLODSwitchValue( studiohwdata_t &hardwareData, int lod, float switchValue ) = 0;

	// Sets the color/alpha modulation
	virtual void SetColorModulation( float const* pColor ) = 0;
	virtual void SetAlphaModulation( float flAlpha ) = 0;
	
	// Draws the model
	virtual void DrawModel( DrawModelResults_t *pResults, const DrawModelInfo_t& info, 
		matrix3x4_t *pBoneToWorld, float *pFlexWeights, float *pFlexDelayedWeights, const Vector &modelOrigin, int flags = STUDIORENDER_DRAW_ENTIRE_MODEL ) = 0;

	// Methods related to static prop rendering
	virtual void DrawModelStaticProp( const DrawModelInfo_t& drawInfo, const matrix3x4_t &modelToWorld, int flags = STUDIORENDER_DRAW_ENTIRE_MODEL ) = 0;
	virtual void DrawStaticPropDecals( const DrawModelInfo_t &drawInfo, const matrix3x4_t &modelToWorld ) = 0;
	virtual void DrawStaticPropShadows( const DrawModelInfo_t &drawInfo, const matrix3x4_t &modelToWorld, int flags ) = 0;

	// Causes a material to be used instead of the materials the model was compiled with
	virtual void ForcedMaterialOverride( IMaterial *newMaterial, OverrideType_t nOverrideType = OVERRIDE_NORMAL ) = 0;

	// Create, destroy list of decals for a particular model
	virtual StudioDecalHandle_t CreateDecalList( studiohwdata_t *pHardwareData ) = 0;
	virtual void DestroyDecalList( StudioDecalHandle_t handle ) = 0;

	// Add decals to a decal list by doing a planar projection along the ray
	// The BoneToWorld matrices must be set before this is called
	virtual void AddDecal( StudioDecalHandle_t handle, studiohdr_t *pStudioHdr, matrix3x4_t *pBoneToWorld, 
		const Ray_t & ray, const Vector& decalUp, IMaterial* pDecalMaterial, float radius, int body, bool noPokethru = false, int maxLODToDecal = ADDDECAL_TO_ALL_LODS ) = 0;

	// Compute the lighting at a point and normal
	virtual void ComputeLighting( const Vector* pAmbient, int lightCount,
		LightDesc_t* pLights, const Vector& pt, const Vector& normal, Vector& lighting ) = 0;

	// Compute the lighting at a point, constant directional component is passed
	// as flDirectionalAmount
	virtual void ComputeLightingConstDirectional( const Vector* pAmbient, int lightCount,
		LightDesc_t* pLights, const Vector& pt, const Vector& normal, Vector& lighting, float flDirectionalAmount ) = 0;

	// Shadow state (affects the models as they are rendered)
	virtual void AddShadow( IMaterial* pMaterial, void* pProxyData, FlashlightState_t *m_pFlashlightState = NULL, VMatrix *pWorldToTexture = NULL, ITexture *pFlashlightDepthTexture = NULL ) = 0;
	virtual void ClearAllShadows() = 0;

	// Gets the model LOD; pass in the screen size in pixels of a sphere 
	// of radius 1 that has the same origin as the model to get the LOD out...
	virtual int ComputeModelLod( studiohwdata_t* pHardwareData, float unitSphereSize, float *pMetric = NULL ) = 0;

	// Return a number that is usable for budgets, etc.
	// Things that we care about:
	// 1) effective triangle count (factors in batch sizes, state changes, etc)
	// 2) texture memory usage
	// Get Triangles returns the LOD used
	virtual void GetPerfStats( DrawModelResults_t *pResults, const DrawModelInfo_t &info, CUtlBuffer *pSpewBuf = NULL ) const = 0;
	virtual void GetTriangles( const DrawModelInfo_t& info, matrix3x4_t *pBoneToWorld, GetTriangles_Output_t &out ) = 0;

	// Returns materials used by a particular model
	virtual int GetMaterialList( studiohdr_t *pStudioHdr, int count, IMaterial** ppMaterials ) = 0;
	virtual int GetMaterialListFromBodyAndSkin( MDLHandle_t studio, int nSkin, int nBody, int nCountOutputMaterials, IMaterial** ppOutputMaterials ) = 0;
	// draw an array of models with the same state
	virtual void DrawModelArray( const DrawModelInfo_t &drawInfo, int arrayCount, model_array_instance_t *pInstanceData, int instanceStride, int flags = STUDIORENDER_DRAW_ENTIRE_MODEL ) = 0;
};

extern IStudioRender *g_pStudioRender;

#endif // ISTUDIORENDER_H
