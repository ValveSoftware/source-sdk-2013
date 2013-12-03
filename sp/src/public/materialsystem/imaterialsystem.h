//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef IMATERIALSYSTEM_H
#define IMATERIALSYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#define OVERBRIGHT 2.0f
#define OO_OVERBRIGHT ( 1.0f / 2.0f )
#define GAMMA 2.2f
#define TEXGAMMA 2.2f

#include "tier1/interface.h"
#include "tier1/refcount.h"
#include "mathlib/vector.h"
#include "mathlib/vector4d.h"
#include "mathlib/vmatrix.h"
#include "appframework/IAppSystem.h"
#include "bitmap/imageformat.h"
#include "texture_group_names.h"
#include "vtf/vtf.h"
#include "materialsystem/deformations.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/IColorCorrection.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IMaterial;
class IMesh;
class IVertexBuffer;
class IIndexBuffer;
struct MaterialSystem_Config_t;
class VMatrix;
struct matrix3x4_t;
class ITexture;
struct MaterialSystemHardwareIdentifier_t;
class KeyValues;
class IShader;
class IVertexTexture;
class IMorph;
class IMatRenderContext;
class ICallQueue;
struct MorphWeight_t;
class IFileList;


//-----------------------------------------------------------------------------
// The vertex format type
//-----------------------------------------------------------------------------
typedef uint64 VertexFormat_t;

//-----------------------------------------------------------------------------
// important enumeration
//-----------------------------------------------------------------------------

// NOTE NOTE NOTE!!!!  If you up this, grep for "NEW_INTERFACE" to see if there is anything
// waiting to be enabled during an interface revision.
#define MATERIAL_SYSTEM_INTERFACE_VERSION "VMaterialSystem080"

#ifdef POSIX
#define ABSOLUTE_MINIMUM_DXLEVEL 90
#else
#define ABSOLUTE_MINIMUM_DXLEVEL 80
#endif

enum ShaderParamType_t 
{ 
	SHADER_PARAM_TYPE_TEXTURE, 
	SHADER_PARAM_TYPE_INTEGER,
	SHADER_PARAM_TYPE_COLOR,
	SHADER_PARAM_TYPE_VEC2,
	SHADER_PARAM_TYPE_VEC3,
	SHADER_PARAM_TYPE_VEC4,
	SHADER_PARAM_TYPE_ENVMAP,	// obsolete
	SHADER_PARAM_TYPE_FLOAT,
	SHADER_PARAM_TYPE_BOOL,
	SHADER_PARAM_TYPE_FOURCC,
	SHADER_PARAM_TYPE_MATRIX,
	SHADER_PARAM_TYPE_MATERIAL,
	SHADER_PARAM_TYPE_STRING,
};

enum MaterialMatrixMode_t
{
	MATERIAL_VIEW = 0,
	MATERIAL_PROJECTION,

	// Texture matrices
	MATERIAL_TEXTURE0,
	MATERIAL_TEXTURE1,
	MATERIAL_TEXTURE2,
	MATERIAL_TEXTURE3,
	MATERIAL_TEXTURE4,
	MATERIAL_TEXTURE5,
	MATERIAL_TEXTURE6,
	MATERIAL_TEXTURE7,

	MATERIAL_MODEL,

	// Total number of matrices
	NUM_MATRIX_MODES = MATERIAL_MODEL+1,

	// Number of texture transforms
	NUM_TEXTURE_TRANSFORMS = MATERIAL_TEXTURE7 - MATERIAL_TEXTURE0 + 1
};

// FIXME: How do I specify the actual number of matrix modes?
const int NUM_MODEL_TRANSFORMS = 53;
const int MATERIAL_MODEL_MAX = MATERIAL_MODEL + NUM_MODEL_TRANSFORMS;

enum MaterialPrimitiveType_t 
{ 
	MATERIAL_POINTS			= 0x0,
	MATERIAL_LINES,
	MATERIAL_TRIANGLES,
	MATERIAL_TRIANGLE_STRIP,
	MATERIAL_LINE_STRIP,
	MATERIAL_LINE_LOOP,	// a single line loop
	MATERIAL_POLYGON,	// this is a *single* polygon
	MATERIAL_QUADS,
	MATERIAL_INSTANCED_QUADS, // (X360) like MATERIAL_QUADS, but uses vertex instancing

	// This is used for static meshes that contain multiple types of
	// primitive types.	When calling draw, you'll need to specify
	// a primitive type.
	MATERIAL_HETEROGENOUS
};

enum MaterialPropertyTypes_t
{
	MATERIAL_PROPERTY_NEEDS_LIGHTMAP = 0,					// bool
	MATERIAL_PROPERTY_OPACITY,								// int (enum MaterialPropertyOpacityTypes_t)
	MATERIAL_PROPERTY_REFLECTIVITY,							// vec3_t
	MATERIAL_PROPERTY_NEEDS_BUMPED_LIGHTMAPS				// bool
};

// acceptable property values for MATERIAL_PROPERTY_OPACITY
enum MaterialPropertyOpacityTypes_t
{
	MATERIAL_ALPHATEST = 0,
	MATERIAL_OPAQUE,
	MATERIAL_TRANSLUCENT
};

enum MaterialBufferTypes_t
{
	MATERIAL_FRONT = 0,
	MATERIAL_BACK
};

enum MaterialCullMode_t
{
	MATERIAL_CULLMODE_CCW,	// this culls polygons with counterclockwise winding
	MATERIAL_CULLMODE_CW	// this culls polygons with clockwise winding
};

enum MaterialIndexFormat_t
{
	MATERIAL_INDEX_FORMAT_UNKNOWN = -1,
	MATERIAL_INDEX_FORMAT_16BIT = 0,
	MATERIAL_INDEX_FORMAT_32BIT,
};

enum MaterialFogMode_t
{
	MATERIAL_FOG_NONE,
	MATERIAL_FOG_LINEAR,
	MATERIAL_FOG_LINEAR_BELOW_FOG_Z,
};

enum MaterialHeightClipMode_t
{
	MATERIAL_HEIGHTCLIPMODE_DISABLE,
	MATERIAL_HEIGHTCLIPMODE_RENDER_ABOVE_HEIGHT,
	MATERIAL_HEIGHTCLIPMODE_RENDER_BELOW_HEIGHT
};

enum MaterialNonInteractiveMode_t
{
	MATERIAL_NON_INTERACTIVE_MODE_NONE = -1,
	MATERIAL_NON_INTERACTIVE_MODE_STARTUP = 0,
	MATERIAL_NON_INTERACTIVE_MODE_LEVEL_LOAD,

	MATERIAL_NON_INTERACTIVE_MODE_COUNT,
};


//-----------------------------------------------------------------------------
// Special morph used in decalling pass
//-----------------------------------------------------------------------------
#define MATERIAL_MORPH_DECAL ( (IMorph*)1 )


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

enum MaterialThreadMode_t
{
	MATERIAL_SINGLE_THREADED,
	MATERIAL_QUEUED_SINGLE_THREADED,
	MATERIAL_QUEUED_THREADED
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

enum MaterialContextType_t
{
	MATERIAL_HARDWARE_CONTEXT,
	MATERIAL_QUEUED_CONTEXT,
	MATERIAL_NULL_CONTEXT
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

enum MaterialFindContext_t
{
	MATERIAL_FINDCONTEXT_NONE,
	MATERIAL_FINDCONTEXT_ISONAMODEL,
};

//-----------------------------------------------------------------------------
// Light structure
//-----------------------------------------------------------------------------
#include "mathlib/lightdesc.h"

#if 0
enum LightType_t
{
	MATERIAL_LIGHT_DISABLE = 0,
	MATERIAL_LIGHT_POINT,
	MATERIAL_LIGHT_DIRECTIONAL,
	MATERIAL_LIGHT_SPOT,
};

enum LightType_OptimizationFlags_t
{
	LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0 = 1,
	LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1 = 2,
	LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2 = 4,
};


struct LightDesc_t 
{
	LightType_t		m_Type;
	Vector			m_Color;
	Vector	m_Position;
	Vector  m_Direction;
	float   m_Range;
	float   m_Falloff;
	float   m_Attenuation0;
	float   m_Attenuation1;
	float   m_Attenuation2;
	float   m_Theta;
	float   m_Phi;
	// These aren't used by DX8. . used for software lighting.
	float	m_ThetaDot;
	float	m_PhiDot;
	unsigned int	m_Flags;


	LightDesc_t() {}

private:
	// No copy constructors allowed
	LightDesc_t(const LightDesc_t& vOther);
};
#endif

#define CREATERENDERTARGETFLAGS_HDR				0x00000001
#define CREATERENDERTARGETFLAGS_AUTOMIPMAP		0x00000002
#define CREATERENDERTARGETFLAGS_UNFILTERABLE_OK 0x00000004
// XBOX ONLY:
#define CREATERENDERTARGETFLAGS_NOEDRAM			0x00000008 // inhibit allocation in 360 EDRAM
#define CREATERENDERTARGETFLAGS_TEMP			0x00000010 // only allocates memory upon first resolve, destroyed at level end


//-----------------------------------------------------------------------------
// allowed stencil operations. These match the d3d operations
//-----------------------------------------------------------------------------
enum StencilOperation_t 
{
#if !defined( _X360 )
	STENCILOPERATION_KEEP = 1,
	STENCILOPERATION_ZERO = 2,
	STENCILOPERATION_REPLACE = 3,
	STENCILOPERATION_INCRSAT = 4,
	STENCILOPERATION_DECRSAT = 5,
	STENCILOPERATION_INVERT = 6,
	STENCILOPERATION_INCR = 7,
	STENCILOPERATION_DECR = 8,
#else
	STENCILOPERATION_KEEP = D3DSTENCILOP_KEEP,
	STENCILOPERATION_ZERO = D3DSTENCILOP_ZERO,
	STENCILOPERATION_REPLACE = D3DSTENCILOP_REPLACE,
	STENCILOPERATION_INCRSAT = D3DSTENCILOP_INCRSAT,
	STENCILOPERATION_DECRSAT = D3DSTENCILOP_DECRSAT,
	STENCILOPERATION_INVERT = D3DSTENCILOP_INVERT,
	STENCILOPERATION_INCR = D3DSTENCILOP_INCR,
	STENCILOPERATION_DECR = D3DSTENCILOP_DECR,
#endif
	STENCILOPERATION_FORCE_DWORD = 0x7fffffff
};

enum StencilComparisonFunction_t 
{
#if !defined( _X360 )
	STENCILCOMPARISONFUNCTION_NEVER = 1,
	STENCILCOMPARISONFUNCTION_LESS = 2,
	STENCILCOMPARISONFUNCTION_EQUAL = 3,
	STENCILCOMPARISONFUNCTION_LESSEQUAL = 4,
	STENCILCOMPARISONFUNCTION_GREATER = 5,
	STENCILCOMPARISONFUNCTION_NOTEQUAL = 6,
	STENCILCOMPARISONFUNCTION_GREATEREQUAL = 7,
	STENCILCOMPARISONFUNCTION_ALWAYS = 8,
#else
	STENCILCOMPARISONFUNCTION_NEVER = D3DCMP_NEVER,
	STENCILCOMPARISONFUNCTION_LESS = D3DCMP_LESS,
	STENCILCOMPARISONFUNCTION_EQUAL = D3DCMP_EQUAL,
	STENCILCOMPARISONFUNCTION_LESSEQUAL = D3DCMP_LESSEQUAL,
	STENCILCOMPARISONFUNCTION_GREATER = D3DCMP_GREATER,
	STENCILCOMPARISONFUNCTION_NOTEQUAL = D3DCMP_NOTEQUAL,
	STENCILCOMPARISONFUNCTION_GREATEREQUAL = D3DCMP_GREATEREQUAL,
	STENCILCOMPARISONFUNCTION_ALWAYS = D3DCMP_ALWAYS,
#endif

	STENCILCOMPARISONFUNCTION_FORCE_DWORD = 0x7fffffff
};


//-----------------------------------------------------------------------------
// Enumeration for the various fields capable of being morphed
//-----------------------------------------------------------------------------
enum MorphFormatFlags_t
{
	MORPH_POSITION	= 0x0001,	// 3D
	MORPH_NORMAL	= 0x0002,	// 3D
	MORPH_WRINKLE	= 0x0004,	// 1D
	MORPH_SPEED		= 0x0008,	// 1D
	MORPH_SIDE		= 0x0010,	// 1D
};


//-----------------------------------------------------------------------------
// The morph format type
//-----------------------------------------------------------------------------
typedef unsigned int MorphFormat_t;


//-----------------------------------------------------------------------------
// Standard lightmaps
//-----------------------------------------------------------------------------
enum StandardLightmap_t
{
	MATERIAL_SYSTEM_LIGHTMAP_PAGE_WHITE = -1,
	MATERIAL_SYSTEM_LIGHTMAP_PAGE_WHITE_BUMP = -2,
	MATERIAL_SYSTEM_LIGHTMAP_PAGE_USER_DEFINED = -3
};


struct MaterialSystem_SortInfo_t
{
	IMaterial	*material;
	int			lightmapPageID;
};


#define MAX_FB_TEXTURES 4

//-----------------------------------------------------------------------------
// Information about each adapter
//-----------------------------------------------------------------------------
enum
{
	MATERIAL_ADAPTER_NAME_LENGTH = 512
};

struct MaterialAdapterInfo_t
{
	char m_pDriverName[MATERIAL_ADAPTER_NAME_LENGTH];
	unsigned int m_VendorID;
	unsigned int m_DeviceID;
	unsigned int m_SubSysID;
	unsigned int m_Revision;
	int m_nDXSupportLevel;			// This is the *preferred* dx support level
	int m_nMaxDXSupportLevel;
	unsigned int m_nDriverVersionHigh;
	unsigned int m_nDriverVersionLow;
};


//-----------------------------------------------------------------------------
// Video mode info..
//-----------------------------------------------------------------------------
struct MaterialVideoMode_t
{
	int m_Width;			// if width and height are 0 and you select 
	int m_Height;			// windowed mode, it'll use the window size
	ImageFormat m_Format;	// use ImageFormats (ignored for windowed mode)
	int m_RefreshRate;		// 0 == default (ignored for windowed mode)
};

// fixme: should move this into something else.
struct FlashlightState_t
{
	FlashlightState_t()
	{
		m_bEnableShadows = false;						// Provide reasonable defaults for shadow depth mapping parameters
		m_bDrawShadowFrustum = false;
		m_flShadowMapResolution = 1024.0f;
		m_flShadowFilterSize = 3.0f;
		m_flShadowSlopeScaleDepthBias = 16.0f;
		m_flShadowDepthBias = 0.0005f;
		m_flShadowJitterSeed = 0.0f;
		m_flShadowAtten = 0.0f;
		m_bScissor = false; 
		m_nLeft = -1;
		m_nTop = -1;
		m_nRight = -1;
		m_nBottom = -1;
		m_nShadowQuality = 0;
	}

	Vector m_vecLightOrigin;
	Quaternion m_quatOrientation;
	float m_NearZ;
	float m_FarZ;
	float m_fHorizontalFOVDegrees;
	float m_fVerticalFOVDegrees;
	float m_fQuadraticAtten;
	float m_fLinearAtten;
	float m_fConstantAtten;
	float m_Color[4];
	ITexture *m_pSpotlightTexture;
	int m_nSpotlightTextureFrame;

	// Shadow depth mapping parameters
	bool  m_bEnableShadows;
	bool  m_bDrawShadowFrustum;
	float m_flShadowMapResolution;
	float m_flShadowFilterSize;
	float m_flShadowSlopeScaleDepthBias;
	float m_flShadowDepthBias;
	float m_flShadowJitterSeed;
	float m_flShadowAtten;
	int   m_nShadowQuality;

	// Getters for scissor members
	bool DoScissor() { return m_bScissor; }
	int GetLeft()	 { return m_nLeft; }
	int GetTop()	 { return m_nTop; }
	int GetRight()	 { return m_nRight; }
	int GetBottom()	 { return m_nBottom; }

private:

	friend class CShadowMgr;

	bool m_bScissor; 
	int m_nLeft;
	int m_nTop;
	int m_nRight;
	int m_nBottom;
};

//-----------------------------------------------------------------------------
// Flags to be used with the Init call
//-----------------------------------------------------------------------------
enum MaterialInitFlags_t
{
	MATERIAL_INIT_ALLOCATE_FULLSCREEN_TEXTURE = 0x2,
	MATERIAL_INIT_REFERENCE_RASTERIZER	= 0x4,
};

//-----------------------------------------------------------------------------
// Flags to specify type of depth buffer used with RT
//-----------------------------------------------------------------------------

// GR - this is to add RT with no depth buffer bound

enum MaterialRenderTargetDepth_t
{
	MATERIAL_RT_DEPTH_SHARED   = 0x0,
	MATERIAL_RT_DEPTH_SEPARATE = 0x1,
	MATERIAL_RT_DEPTH_NONE     = 0x2,
	MATERIAL_RT_DEPTH_ONLY	   = 0x3,
};

//-----------------------------------------------------------------------------
// A function to be called when we need to release all vertex buffers
// NOTE: The restore function will tell the caller if all the vertex formats
// changed so that it can flush caches, etc. if it needs to (for dxlevel support)
//-----------------------------------------------------------------------------
enum RestoreChangeFlags_t
{
	MATERIAL_RESTORE_VERTEX_FORMAT_CHANGED = 0x1,
};


// NOTE: All size modes will force the render target to be smaller than or equal to
// the size of the framebuffer.
enum RenderTargetSizeMode_t
{
	RT_SIZE_NO_CHANGE=0,			// Only allowed for render targets that don't want a depth buffer
	// (because if they have a depth buffer, the render target must be less than or equal to the size of the framebuffer).
	RT_SIZE_DEFAULT=1,				// Don't play with the specified width and height other than making sure it fits in the framebuffer.
	RT_SIZE_PICMIP=2,				// Apply picmip to the render target's width and height.
	RT_SIZE_HDR=3,					// frame_buffer_width / 4
	RT_SIZE_FULL_FRAME_BUFFER=4,	// Same size as frame buffer, or next lower power of 2 if we can't do that.
	RT_SIZE_OFFSCREEN=5,			// Target of specified size, don't mess with dimensions
	RT_SIZE_FULL_FRAME_BUFFER_ROUNDED_UP=6, // Same size as the frame buffer, rounded up if necessary for systems that can't do non-power of two textures.
	RT_SIZE_REPLAY_SCREENSHOT = 7,	// Rounded down to power of 2, essentially...
	RT_SIZE_LITERAL = 8				// Use the size passed in. Don't clamp it to the frame buffer size. Really.
};

typedef void (*MaterialBufferReleaseFunc_t)( );
typedef void (*MaterialBufferRestoreFunc_t)( int nChangeFlags );	// see RestoreChangeFlags_t
typedef void (*ModeChangeCallbackFunc_t)( void );

typedef int VertexBufferHandle_t;
typedef unsigned short MaterialHandle_t;

DECLARE_POINTER_HANDLE( OcclusionQueryObjectHandle_t );
#define INVALID_OCCLUSION_QUERY_OBJECT_HANDLE ( (OcclusionQueryObjectHandle_t)0 )

class IMaterialProxyFactory;
class ITexture;
class IMaterialSystemHardwareConfig;
class CShadowMgr;

DECLARE_POINTER_HANDLE( MaterialLock_t );

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

abstract_class IMaterialSystem : public IAppSystem
{
public:

	// Placeholder for API revision
	virtual bool Connect( CreateInterfaceFn factory ) = 0;
	virtual void Disconnect() = 0;
	virtual void *QueryInterface( const char *pInterfaceName ) = 0;
	virtual InitReturnVal_t Init() = 0;
	virtual void Shutdown() = 0;

	//---------------------------------------------------------
	// Initialization and shutdown
	//---------------------------------------------------------

	// Call this to initialize the material system
	// returns a method to create interfaces in the shader dll
	virtual CreateInterfaceFn	Init( char const* pShaderAPIDLL, 
		IMaterialProxyFactory *pMaterialProxyFactory,
		CreateInterfaceFn fileSystemFactory,
		CreateInterfaceFn cvarFactory=NULL ) = 0;

	// Call this to set an explicit shader version to use 
	// Must be called before Init().
	virtual void				SetShaderAPI( char const *pShaderAPIDLL ) = 0;

	// Must be called before Init(), if you're going to call it at all...
	virtual void				SetAdapter( int nAdapter, int nFlags ) = 0;

	// Call this when the mod has been set up, which may occur after init
	// At this point, the game + gamebin paths have been set up
	virtual void				ModInit() = 0;
	virtual void				ModShutdown() = 0;

	//---------------------------------------------------------
	//
	//---------------------------------------------------------
	virtual void					SetThreadMode( MaterialThreadMode_t mode, int nServiceThread = -1 ) = 0;
	virtual MaterialThreadMode_t	GetThreadMode( ) = 0;
	virtual bool					IsRenderThreadSafe( ) = 0;
	virtual void					ExecuteQueued() = 0;

	//---------------------------------------------------------
	// Config management
	//---------------------------------------------------------

	virtual IMaterialSystemHardwareConfig *GetHardwareConfig( const char *pVersion, int *returnCode ) = 0;


	// Call this before rendering each frame with the current config
	// for the material system.
	// Will do whatever is necessary to get the material system into the correct state
	// upon configuration change. .doesn't much else otherwise.
	virtual bool				UpdateConfig( bool bForceUpdate ) = 0;

	// Force this to be the config; update all material system convars to match the state
	// return true if lightmaps need to be redownloaded
	virtual bool				OverrideConfig( const MaterialSystem_Config_t &config, bool bForceUpdate ) = 0;

	// Get the current config for this video card (as last set by UpdateConfig)
	virtual const MaterialSystem_Config_t &GetCurrentConfigForVideoCard() const = 0;

	// Gets *recommended* configuration information associated with the display card, 
	// given a particular dx level to run under. 
	// Use dxlevel 0 to use the recommended dx level.
	// The function returns false if an invalid dxlevel was specified

	// UNDONE: To find out all convars affected by configuration, we'll need to change
	// the dxsupport.pl program to output all column headers into a single keyvalue block
	// and then we would read that in, and send it back to the client
	virtual bool				GetRecommendedConfigurationInfo( int nDXLevel, KeyValues * pKeyValues ) = 0;


	// -----------------------------------------------------------
	// Device methods
	// -----------------------------------------------------------

	// Gets the number of adapters...
	virtual int					GetDisplayAdapterCount() const = 0;

	// Returns the current adapter in use
	virtual int					GetCurrentAdapter() const = 0;

	// Returns info about each adapter
	virtual void				GetDisplayAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const = 0;

	// Returns the number of modes
	virtual int					GetModeCount( int adapter ) const = 0;

	// Returns mode information..
	virtual void				GetModeInfo( int adapter, int mode, MaterialVideoMode_t& info ) const = 0;

	virtual void				AddModeChangeCallBack( ModeChangeCallbackFunc_t func ) = 0;

	// Returns the mode info for the current display device
	virtual void				GetDisplayMode( MaterialVideoMode_t& mode ) const = 0;

	// Sets the mode...
	virtual bool				SetMode( void* hwnd, const MaterialSystem_Config_t &config ) = 0;

	virtual bool				SupportsMSAAMode( int nMSAAMode ) = 0;

	// FIXME: REMOVE! Get video card identitier
	virtual const MaterialSystemHardwareIdentifier_t &GetVideoCardIdentifier( void ) const = 0;

	// Use this to spew information about the 3D layer 
	virtual void				SpewDriverInfo() const = 0;

	virtual void				GetDXLevelDefaults(uint &max_dxlevel,uint &recommended_dxlevel) = 0;

	// Get the image format of the back buffer. . useful when creating render targets, etc.
	virtual void				GetBackBufferDimensions( int &width, int &height) const = 0;
	virtual ImageFormat			GetBackBufferFormat() const = 0;

	virtual bool				SupportsHDRMode( HDRType_t nHDRModede ) = 0;


	// -----------------------------------------------------------
	// Window methods
	// -----------------------------------------------------------

	// Creates/ destroys a child window
	virtual bool				AddView( void* hwnd ) = 0;
	virtual void				RemoveView( void* hwnd ) = 0;

	// Sets the view
	virtual void				SetView( void* hwnd ) = 0;


	// -----------------------------------------------------------
	// Control flow
	// -----------------------------------------------------------

	virtual void				BeginFrame( float frameTime ) = 0;
	virtual void				EndFrame( ) = 0;
	virtual void				Flush( bool flushHardware = false ) = 0;

	/// FIXME: This stuff needs to be cleaned up and abstracted.
	// Stuff that gets exported to the launcher through the engine
	virtual void				SwapBuffers( ) = 0;

	// Flushes managed textures from the texture cacher
	virtual void				EvictManagedResources() = 0;

	virtual void				ReleaseResources(void) = 0;
	virtual void				ReacquireResources(void ) = 0;


	// -----------------------------------------------------------
	// Device loss/restore
	// -----------------------------------------------------------

	// Installs a function to be called when we need to release vertex buffers + textures
	virtual void				AddReleaseFunc( MaterialBufferReleaseFunc_t func ) = 0;
	virtual void				RemoveReleaseFunc( MaterialBufferReleaseFunc_t func ) = 0;

	// Installs a function to be called when we need to restore vertex buffers
	virtual void				AddRestoreFunc( MaterialBufferRestoreFunc_t func ) = 0;
	virtual void				RemoveRestoreFunc( MaterialBufferRestoreFunc_t func ) = 0;

	// Release temporary HW memory...
	virtual void				ResetTempHWMemory( bool bExitingLevel = false ) = 0;

	// For dealing with device lost in cases where SwapBuffers isn't called all the time (Hammer)
	virtual void				HandleDeviceLost() = 0;


	// -----------------------------------------------------------
	// Shaders
	// -----------------------------------------------------------

	// Used to iterate over all shaders for editing purposes
	// GetShaders returns the number of shaders it actually found
	virtual int					ShaderCount() const = 0;
	virtual int					GetShaders( int nFirstShader, int nMaxCount, IShader **ppShaderList ) const = 0;

	// FIXME: Is there a better way of doing this?
	// Returns shader flag names for editors to be able to edit them
	virtual int					ShaderFlagCount() const = 0;
	virtual const char *		ShaderFlagName( int nIndex ) const = 0;

	// Gets the actual shader fallback for a particular shader
	virtual void				GetShaderFallback( const char *pShaderName, char *pFallbackShader, int nFallbackLength ) = 0;


	// -----------------------------------------------------------
	// Material proxies
	// -----------------------------------------------------------

	virtual IMaterialProxyFactory *GetMaterialProxyFactory() = 0;

	// Sets the material proxy factory. Calling this causes all materials to be uncached.
	virtual void				SetMaterialProxyFactory( IMaterialProxyFactory* pFactory ) = 0;


	// -----------------------------------------------------------
	// Editor mode
	// -----------------------------------------------------------

	// Used to enable editor materials. Must be called before Init.
	virtual void				EnableEditorMaterials() = 0;


	// -----------------------------------------------------------
	// Stub mode mode
	// -----------------------------------------------------------

	// Force it to ignore Draw calls.
	virtual void				SetInStubMode( bool bInStubMode ) = 0;


	//---------------------------------------------------------
	// Debug support
	//---------------------------------------------------------

	virtual void				DebugPrintUsedMaterials( const char *pSearchSubString, bool bVerbose ) = 0;
	virtual void				DebugPrintUsedTextures( void ) = 0;

	virtual void				ToggleSuppressMaterial( char const* pMaterialName ) = 0;
	virtual void				ToggleDebugMaterial( char const* pMaterialName ) = 0;


	//---------------------------------------------------------
	// Misc features
	//---------------------------------------------------------
	//returns whether fast clipping is being used or not - needed to be exposed for better per-object clip behavior
	virtual bool				UsingFastClipping( void ) = 0;

	virtual int					StencilBufferBits( void ) = 0; //number of bits per pixel in the stencil buffer


	//---------------------------------------------------------
	// Material and texture management
	//---------------------------------------------------------

	// uncache all materials. .  good for forcing reload of materials.
	virtual void				UncacheAllMaterials( ) = 0;

	// Remove any materials from memory that aren't in use as determined
	// by the IMaterial's reference count.
	virtual void				UncacheUnusedMaterials( bool bRecomputeStateSnapshots = false ) = 0;

	// Load any materials into memory that are to be used as determined
	// by the IMaterial's reference count.
	virtual void				CacheUsedMaterials( ) = 0;

	// Force all textures to be reloaded from disk.
	virtual void				ReloadTextures( ) = 0;

	// Reloads materials
	virtual void				ReloadMaterials( const char *pSubString = NULL ) = 0;

	// Create a procedural material. The keyvalues looks like a VMT file
	virtual IMaterial *			CreateMaterial( const char *pMaterialName, KeyValues *pVMTKeyValues ) = 0;

	// Find a material by name.
	// The name of a material is a full path to 
	// the vmt file starting from "hl2/materials" (or equivalent) without
	// a file extension.
	// eg. "dev/dev_bumptest" refers to somethign similar to:
	// "d:/hl2/hl2/materials/dev/dev_bumptest.vmt"
	//
	// Most of the texture groups for pTextureGroupName are listed in texture_group_names.h.
	// 
	// Note: if the material can't be found, this returns a checkerboard material. You can 
	// find out if you have that material by calling IMaterial::IsErrorMaterial().
	// (Or use the global IsErrorMaterial function, which checks if it's null too).
	virtual IMaterial *			FindMaterial( char const* pMaterialName, const char *pTextureGroupName, bool complain = true, const char *pComplainPrefix = NULL ) = 0;

	// Query whether a material is loaded (eg, whether FindMaterial will be nonblocking)
	virtual bool				IsMaterialLoaded( char const* pMaterialName ) = 0;

	//---------------------------------
	// This is the interface for knowing what materials are available
	// is to use the following functions to get a list of materials.  The
	// material names will have the full path to the material, and that is the 
	// only way that the directory structure of the materials will be seen through this
	// interface.
	// NOTE:  This is mostly for worldcraft to get a list of materials to put
	// in the "texture" browser.in Worldcraft
	virtual MaterialHandle_t	FirstMaterial() const = 0;

	// returns InvalidMaterial if there isn't another material.
	// WARNING: you must call GetNextMaterial until it returns NULL, 
	// otherwise there will be a memory leak.
	virtual MaterialHandle_t	NextMaterial( MaterialHandle_t h ) const = 0;

	// This is the invalid material
	virtual MaterialHandle_t	InvalidMaterial() const = 0;

	// Returns a particular material
	virtual IMaterial*			GetMaterial( MaterialHandle_t h ) const = 0;

	// Get the total number of materials in the system.  These aren't just the used
	// materials, but the complete collection.
	virtual int					GetNumMaterials( ) const = 0;

	//---------------------------------

	virtual void				SetAsyncTextureLoadCache( void* hFileCache ) = 0;

	virtual ITexture *			FindTexture( char const* pTextureName, const char *pTextureGroupName, bool complain = true, int nAdditionalCreationFlags = 0  ) = 0;

	// Checks to see if a particular texture is loaded
	virtual bool				IsTextureLoaded( char const* pTextureName ) const = 0;

	// Creates a procedural texture
	virtual ITexture *			CreateProceduralTexture( const char	*pTextureName, 
		const char *pTextureGroupName, 
		int w, 
		int h, 
		ImageFormat fmt, 
		int nFlags ) = 0;

	//
	// Render targets
	//
	virtual void				BeginRenderTargetAllocation() = 0;
	virtual void				EndRenderTargetAllocation() = 0; // Simulate an Alt-Tab in here, which causes a release/restore of all resources

	// Creates a render target
	// If depth == true, a depth buffer is also allocated. If not, then
	// the screen's depth buffer is used.
	// Creates a texture for use as a render target
	virtual ITexture *			CreateRenderTargetTexture( int w, 
		int h, 
		RenderTargetSizeMode_t sizeMode,	// Controls how size is generated (and regenerated on video mode change).
		ImageFormat	format, 
		MaterialRenderTargetDepth_t depth = MATERIAL_RT_DEPTH_SHARED ) = 0;

	virtual ITexture *			CreateNamedRenderTargetTextureEx(  const char *pRTName,				// Pass in NULL here for an unnamed render target.
		int w, 
		int h, 
		RenderTargetSizeMode_t sizeMode,	// Controls how size is generated (and regenerated on video mode change).
		ImageFormat format, 
		MaterialRenderTargetDepth_t depth = MATERIAL_RT_DEPTH_SHARED, 
		unsigned int textureFlags = TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		unsigned int renderTargetFlags = 0 ) = 0;

	virtual ITexture *			CreateNamedRenderTargetTexture( const char *pRTName, 
		int w, 
		int h, 
		RenderTargetSizeMode_t sizeMode,	// Controls how size is generated (and regenerated on video mode change).
		ImageFormat format, 
		MaterialRenderTargetDepth_t depth = MATERIAL_RT_DEPTH_SHARED, 
		bool bClampTexCoords = true, 
		bool bAutoMipMap = false ) = 0;

	// Must be called between the above Begin-End calls!
	virtual ITexture *			CreateNamedRenderTargetTextureEx2( const char *pRTName,				// Pass in NULL here for an unnamed render target.
		int w, 
		int h, 
		RenderTargetSizeMode_t sizeMode,	// Controls how size is generated (and regenerated on video mode change).
		ImageFormat format, 
		MaterialRenderTargetDepth_t depth = MATERIAL_RT_DEPTH_SHARED, 
		unsigned int textureFlags = TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		unsigned int renderTargetFlags = 0 ) = 0;

	// -----------------------------------------------------------
	// Lightmaps
	// -----------------------------------------------------------

	// To allocate lightmaps, sort the whole world by material twice.
	// The first time through, call AllocateLightmap for every surface.
	// that has a lightmap.
	// The second time through, call AllocateWhiteLightmap for every 
	// surface that expects to use shaders that expect lightmaps.
	virtual void				BeginLightmapAllocation( ) = 0;
	virtual void				EndLightmapAllocation( ) = 0;

	// returns the sorting id for this surface
	virtual int 				AllocateLightmap( int width, int height, 
		int offsetIntoLightmapPage[2],
		IMaterial *pMaterial ) = 0;
	// returns the sorting id for this surface
	virtual int					AllocateWhiteLightmap( IMaterial *pMaterial ) = 0;

	// lightmaps are in linear color space
	// lightmapPageID is returned by GetLightmapPageIDForSortID
	// lightmapSize and offsetIntoLightmapPage are returned by AllocateLightmap.
	// You should never call UpdateLightmap for a lightmap allocated through
	// AllocateWhiteLightmap.
	virtual void				UpdateLightmap( int lightmapPageID, int lightmapSize[2],
		int offsetIntoLightmapPage[2], 
		float *pFloatImage, float *pFloatImageBump1,
		float *pFloatImageBump2, float *pFloatImageBump3 ) = 0;

	// fixme: could just be an array of ints for lightmapPageIDs since the material
	// for a surface is already known.
	virtual int					GetNumSortIDs( ) = 0;
	virtual void				GetSortInfo( MaterialSystem_SortInfo_t *sortInfoArray ) = 0;

	// Read the page size of an existing lightmap by sort id (returned from AllocateLightmap())
	virtual void				GetLightmapPageSize( int lightmap, int *width, int *height ) const = 0;

	virtual void				ResetMaterialLightmapPageInfo() = 0;



	virtual void				ClearBuffers( bool bClearColor, bool bClearDepth, bool bClearStencil = false ) = 0;

	// -----------------------------------------------------------
	// X360 specifics
	// -----------------------------------------------------------

#if defined( _X360 )
	virtual void				ListUsedMaterials( void ) = 0;
	virtual HXUIFONT			OpenTrueTypeFont( const char *pFontname, int tall, int style ) = 0;
	virtual void				CloseTrueTypeFont( HXUIFONT hFont ) = 0;
	virtual bool				GetTrueTypeFontMetrics( HXUIFONT hFont, XUIFontMetrics *pFontMetrics, XUICharMetrics charMetrics[256] ) = 0;
	// Render a sequence of characters and extract the data into a buffer
	// For each character, provide the width+height of the font texture subrect,
	// an offset to apply when rendering the glyph, and an offset into a buffer to receive the RGBA data
	virtual bool				GetTrueTypeGlyphs( HXUIFONT hFont, int numChars, wchar_t *pWch, int *pOffsetX, int *pOffsetY, int *pWidth, int *pHeight, unsigned char *pRGBA, int *pRGBAOffset ) = 0;
	virtual void				PersistDisplay() = 0;
	virtual void				*GetD3DDevice() = 0;
	virtual bool				OwnGPUResources( bool bEnable ) = 0;
#endif

	// -----------------------------------------------------------
	// Access the render contexts
	// -----------------------------------------------------------
	virtual IMatRenderContext *	GetRenderContext() = 0;

	virtual bool				SupportsShadowDepthTextures( void ) = 0;
	virtual void				BeginUpdateLightmaps( void ) = 0;
	virtual void				EndUpdateLightmaps( void ) = 0;

	// -----------------------------------------------------------
	// Methods to force the material system into non-threaded, non-queued mode
	// -----------------------------------------------------------
	virtual MaterialLock_t		Lock() = 0;
	virtual void				Unlock( MaterialLock_t ) = 0;

	// Vendor-dependent shadow depth texture format
	virtual ImageFormat			GetShadowDepthTextureFormat() = 0;

	virtual bool				SupportsFetch4( void ) = 0;

	// Create a custom render context. Cannot be used to create MATERIAL_HARDWARE_CONTEXT
	virtual IMatRenderContext *CreateRenderContext( MaterialContextType_t type ) = 0;

	// Set a specified render context to be the global context for the thread. Returns the prior context.
	virtual IMatRenderContext *SetRenderContext( IMatRenderContext * ) = 0;

	virtual bool				SupportsCSAAMode( int nNumSamples, int nQualityLevel ) = 0;

	virtual void				RemoveModeChangeCallBack( ModeChangeCallbackFunc_t func ) = 0;

	// Finds or create a procedural material.
	virtual IMaterial *			FindProceduralMaterial( const char *pMaterialName, const char *pTextureGroupName, KeyValues *pVMTKeyValues ) = 0;

	virtual ImageFormat			GetNullTextureFormat() = 0;

	virtual void				AddTextureAlias( const char *pAlias, const char *pRealName ) = 0;
	virtual void				RemoveTextureAlias( const char *pAlias ) = 0;

	// returns a lightmap page ID for this allocation, -1 if none available
	// frameID is a number that should be changed every frame to prevent locking any textures that are
	// being used to draw in the previous frame
	virtual int					AllocateDynamicLightmap( int lightmapSize[2], int *pOutOffsetIntoPage, int frameID ) = 0;

	virtual void				SetExcludedTextures( const char *pScriptName ) = 0;
	virtual void				UpdateExcludedTextures( void ) = 0;

	virtual bool				IsInFrame( ) const = 0;

	virtual void				CompactMemory() = 0;

	// For sv_pure mode. The filesystem figures out which files the client needs to reload to be "pure" ala the server's preferences.
	virtual void ReloadFilesInList( IFileList *pFilesToReload ) = 0;
	virtual	bool				AllowThreading( bool bAllow, int nServiceThread ) = 0;

	// Extended version of FindMaterial().
	// Contains context in so it can make decisions (i.e. if it's a model, ignore certain cheat parameters)
	virtual IMaterial *			FindMaterialEx( char const* pMaterialName, const char *pTextureGroupName, int nContext, bool complain = true, const char *pComplainPrefix = NULL ) = 0;

#ifdef DX_TO_GL_ABSTRACTION
	virtual void				DoStartupShaderPreloading( void ) = 0;
#endif	

	// Sets the override sizes for all render target size tests. These replace the frame buffer size.
	// Set them when you are rendering primarily to something larger than the frame buffer (as in VR mode).
	virtual void				SetRenderTargetFrameBufferSizeOverrides( int nWidth, int nHeight ) = 0;

	// Returns the (possibly overridden) framebuffer size for render target sizing.
	virtual void				GetRenderTargetFrameBufferDimensions( int & nWidth, int & nHeight ) = 0;

	// returns the display device name that matches the adapter index we were started with
	virtual char *GetDisplayDeviceName() const = 0;
};


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
abstract_class IMatRenderContext : public IRefCounted
{
public:
	virtual void				BeginRender() = 0;
	virtual void				EndRender() = 0;

	virtual void				Flush( bool flushHardware = false ) = 0;

	virtual void				BindLocalCubemap( ITexture *pTexture ) = 0;

	// pass in an ITexture (that is build with "rendertarget" "1") or
	// pass in NULL for the regular backbuffer.
	virtual void				SetRenderTarget( ITexture *pTexture ) = 0;
	virtual ITexture *			GetRenderTarget( void ) = 0;

	virtual void				GetRenderTargetDimensions( int &width, int &height) const = 0;

	// Bind a material is current for rendering.
	virtual void				Bind( IMaterial *material, void *proxyData = 0 ) = 0;
	// Bind a lightmap page current for rendering.  You only have to 
	// do this for materials that require lightmaps.
	virtual void				BindLightmapPage( int lightmapPageID ) = 0;

	// inputs are between 0 and 1
	virtual void				DepthRange( float zNear, float zFar ) = 0;

	virtual void				ClearBuffers( bool bClearColor, bool bClearDepth, bool bClearStencil = false ) = 0;

	// read to a unsigned char rgb image.
	virtual void				ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat ) = 0;

	// Sets lighting
	virtual void				SetAmbientLight( float r, float g, float b ) = 0;
	virtual void				SetLight( int lightNum, const LightDesc_t& desc ) = 0;

	// The faces of the cube are specified in the same order as cubemap textures
	virtual void				SetAmbientLightCube( Vector4D cube[6] ) = 0;

	// Blit the backbuffer to the framebuffer texture
	virtual void				CopyRenderTargetToTexture( ITexture *pTexture ) = 0;

	// Set the current texture that is a copy of the framebuffer.
	virtual void				SetFrameBufferCopyTexture( ITexture *pTexture, int textureIndex = 0 ) = 0;
	virtual ITexture		   *GetFrameBufferCopyTexture( int textureIndex ) = 0;

	//
	// end vertex array api
	//

	// matrix api
	virtual void				MatrixMode( MaterialMatrixMode_t matrixMode ) = 0;
	virtual void				PushMatrix( void ) = 0;
	virtual void				PopMatrix( void ) = 0;
	virtual void				LoadMatrix( VMatrix const& matrix ) = 0;
	virtual void				LoadMatrix( matrix3x4_t const& matrix ) = 0;
	virtual void				MultMatrix( VMatrix const& matrix ) = 0;
	virtual void				MultMatrix( matrix3x4_t const& matrix ) = 0;
	virtual void				MultMatrixLocal( VMatrix const& matrix ) = 0;
	virtual void				MultMatrixLocal( matrix3x4_t const& matrix ) = 0;
	virtual void				GetMatrix( MaterialMatrixMode_t matrixMode, VMatrix *matrix ) = 0;
	virtual void				GetMatrix( MaterialMatrixMode_t matrixMode, matrix3x4_t *matrix ) = 0;
	virtual void				LoadIdentity( void ) = 0;
	virtual void				Ortho( double left, double top, double right, double bottom, double zNear, double zFar ) = 0;
	virtual void				PerspectiveX( double fovx, double aspect, double zNear, double zFar ) = 0;
	virtual void				PickMatrix( int x, int y, int width, int height ) = 0;
	virtual void				Rotate( float angle, float x, float y, float z ) = 0;
	virtual void				Translate( float x, float y, float z ) = 0;
	virtual void				Scale( float x, float y, float z ) = 0;
	// end matrix api

	// Sets/gets the viewport
	virtual void				Viewport( int x, int y, int width, int height ) = 0;
	virtual void				GetViewport( int& x, int& y, int& width, int& height ) const = 0;

	// The cull mode
	virtual void				CullMode( MaterialCullMode_t cullMode ) = 0;

	// end matrix api

	// This could easily be extended to a general user clip plane
	virtual void				SetHeightClipMode( MaterialHeightClipMode_t nHeightClipMode ) = 0;
	// garymcthack : fog z is always used for heightclipz for now.
	virtual void				SetHeightClipZ( float z ) = 0;

	// Fog methods...
	virtual void				FogMode( MaterialFogMode_t fogMode ) = 0;
	virtual void				FogStart( float fStart ) = 0;
	virtual void				FogEnd( float fEnd ) = 0;
	virtual void				SetFogZ( float fogZ ) = 0;
	virtual MaterialFogMode_t	GetFogMode( void ) = 0;

	virtual void				FogColor3f( float r, float g, float b ) = 0;
	virtual void				FogColor3fv( float const* rgb ) = 0;
	virtual void				FogColor3ub( unsigned char r, unsigned char g, unsigned char b ) = 0;
	virtual void				FogColor3ubv( unsigned char const* rgb ) = 0;

	virtual void				GetFogColor( unsigned char *rgb ) = 0;

	// Sets the number of bones for skinning
	virtual void				SetNumBoneWeights( int numBones ) = 0;

	// Creates/destroys Mesh
	virtual IMesh* CreateStaticMesh( VertexFormat_t fmt, const char *pTextureBudgetGroup, IMaterial * pMaterial = NULL ) = 0;
	virtual void DestroyStaticMesh( IMesh* mesh ) = 0;

	// Gets the dynamic mesh associated with the currently bound material
	// note that you've got to render the mesh before calling this function 
	// a second time. Clients should *not* call DestroyStaticMesh on the mesh 
	// returned by this call.
	// Use buffered = false if you want to not have the mesh be buffered,
	// but use it instead in the following pattern:
	//		meshBuilder.Begin
	//		meshBuilder.End
	//		Draw partial
	//		Draw partial
	//		Draw partial
	//		meshBuilder.Begin
	//		meshBuilder.End
	//		etc
	// Use Vertex or Index Override to supply a static vertex or index buffer
	// to use in place of the dynamic buffers.
	//
	// If you pass in a material in pAutoBind, it will automatically bind the
	// material. This can be helpful since you must bind the material you're
	// going to use BEFORE calling GetDynamicMesh.
	virtual IMesh* GetDynamicMesh( 
		bool buffered = true, 
		IMesh* pVertexOverride = 0,	
		IMesh* pIndexOverride = 0, 
		IMaterial *pAutoBind = 0 ) = 0;

	// ------------ New Vertex/Index Buffer interface ----------------------------
	// Do we need support for bForceTempMesh and bSoftwareVertexShader?
	// I don't think we use bSoftwareVertexShader anymore. .need to look into bForceTempMesh.
	virtual IVertexBuffer *CreateStaticVertexBuffer( VertexFormat_t fmt, int nVertexCount, const char *pTextureBudgetGroup ) = 0;
	virtual IIndexBuffer *CreateStaticIndexBuffer( MaterialIndexFormat_t fmt, int nIndexCount, const char *pTextureBudgetGroup ) = 0;
	virtual void DestroyVertexBuffer( IVertexBuffer * ) = 0;
	virtual void DestroyIndexBuffer( IIndexBuffer * ) = 0;
	// Do we need to specify the stream here in the case of locking multiple dynamic VBs on different streams?
	virtual IVertexBuffer *GetDynamicVertexBuffer( int streamID, VertexFormat_t vertexFormat, bool bBuffered = true ) = 0;
	virtual IIndexBuffer *GetDynamicIndexBuffer( MaterialIndexFormat_t fmt, bool bBuffered = true ) = 0;
	virtual void BindVertexBuffer( int streamID, IVertexBuffer *pVertexBuffer, int nOffsetInBytes, int nFirstVertex, int nVertexCount, VertexFormat_t fmt, int nRepetitions = 1 ) = 0;
	virtual void BindIndexBuffer( IIndexBuffer *pIndexBuffer, int nOffsetInBytes ) = 0;
	virtual void Draw( MaterialPrimitiveType_t primitiveType, int firstIndex, int numIndices ) = 0;
	// ------------ End ----------------------------

	// Selection mode methods
	virtual int  SelectionMode( bool selectionMode ) = 0;
	virtual void SelectionBuffer( unsigned int* pBuffer, int size ) = 0;
	virtual void ClearSelectionNames( ) = 0;
	virtual void LoadSelectionName( int name ) = 0;
	virtual void PushSelectionName( int name ) = 0;
	virtual void PopSelectionName() = 0;

	// Sets the Clear Color for ClearBuffer....
	virtual void		ClearColor3ub( unsigned char r, unsigned char g, unsigned char b ) = 0;
	virtual void		ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a ) = 0;

	// Allows us to override the depth buffer setting of a material
	virtual void	OverrideDepthEnable( bool bEnable, bool bDepthEnable ) = 0;

	// FIXME: This is a hack required for NVidia/XBox, can they fix in drivers?
	virtual void	DrawScreenSpaceQuad( IMaterial* pMaterial ) = 0;

	// For debugging and building recording files. This will stuff a token into the recording file,
	// then someone doing a playback can watch for the token.
	virtual void	SyncToken( const char *pToken ) = 0;

	// FIXME: REMOVE THIS FUNCTION!
	// The only reason why it's not gone is because we're a week from ship when I found the bug in it
	// and everything's tuned to use it.
	// It's returning values which are 2x too big (it's returning sphere diameter x2)
	// Use ComputePixelDiameterOfSphere below in all new code instead.
	virtual float	ComputePixelWidthOfSphere( const Vector& origin, float flRadius ) = 0;

	//
	// Occlusion query support
	//

	// Allocate and delete query objects.
	virtual OcclusionQueryObjectHandle_t CreateOcclusionQueryObject( void ) = 0;
	virtual void DestroyOcclusionQueryObject( OcclusionQueryObjectHandle_t ) = 0;

	// Bracket drawing with begin and end so that we can get counts next frame.
	virtual void BeginOcclusionQueryDrawing( OcclusionQueryObjectHandle_t ) = 0;
	virtual void EndOcclusionQueryDrawing( OcclusionQueryObjectHandle_t ) = 0;

	// Get the number of pixels rendered between begin and end on an earlier frame.
	// Calling this in the same frame is a huge perf hit!
	virtual int OcclusionQuery_GetNumPixelsRendered( OcclusionQueryObjectHandle_t ) = 0;

	virtual void SetFlashlightMode( bool bEnable ) = 0;

	virtual void SetFlashlightState( const FlashlightState_t &state, const VMatrix &worldToTexture ) = 0;

	// Gets the current height clip mode
	virtual MaterialHeightClipMode_t GetHeightClipMode( ) = 0;

	// This returns the diameter of the sphere in pixels based on 
	// the current model, view, + projection matrices and viewport.
	virtual float	ComputePixelDiameterOfSphere( const Vector& vecAbsOrigin, float flRadius ) = 0;

	// By default, the material system applies the VIEW and PROJECTION matrices	to the user clip
	// planes (which are specified in world space) to generate projection-space user clip planes
	// Occasionally (for the particle system in hl2, for example), we want to override that
	// behavior and explictly specify a ViewProj transform for user clip planes
	virtual void	EnableUserClipTransformOverride( bool bEnable ) = 0;
	virtual void	UserClipTransform( const VMatrix &worldToView ) = 0;

	virtual bool GetFlashlightMode() const = 0;

	// Used to make the handle think it's never had a successful query before
	virtual void ResetOcclusionQueryObject( OcclusionQueryObjectHandle_t ) = 0;

	// FIXME: Remove
	virtual void Unused3() {}

	// Creates/destroys morph data associated w/ a particular material
	virtual IMorph *CreateMorph( MorphFormat_t format, const char *pDebugName ) = 0;
	virtual void DestroyMorph( IMorph *pMorph ) = 0;

	// Binds the morph data for use in rendering
	virtual void BindMorph( IMorph *pMorph ) = 0;

	// Sets flexweights for rendering
	virtual void SetFlexWeights( int nFirstWeight, int nCount, const MorphWeight_t* pWeights ) = 0;

	// FIXME: Remove
	virtual void Unused4() {};
	virtual void Unused5() {};
	virtual void Unused6() {};
	virtual void Unused7() {};
	virtual void Unused8() {};

	// Read w/ stretch to a host-memory buffer
	virtual void ReadPixelsAndStretch( Rect_t *pSrcRect, Rect_t *pDstRect, unsigned char *pBuffer, ImageFormat dstFormat, int nDstStride ) = 0;

	// Gets the window size
	virtual void GetWindowSize( int &width, int &height ) const = 0;

	// This function performs a texture map from one texture map to the render destination, doing
	// all the necessary pixel/texel coordinate fix ups. fractional values can be used for the
	// src_texture coordinates to get linear sampling - integer values should produce 1:1 mappings
	// for non-scaled operations.
	virtual void DrawScreenSpaceRectangle( 
		IMaterial *pMaterial,
		int destx, int desty,
		int width, int height,
		float src_texture_x0, float src_texture_y0,			// which texel you want to appear at
		// destx/y
		float src_texture_x1, float src_texture_y1,			// which texel you want to appear at
		// destx+width-1, desty+height-1
		int src_texture_width, int src_texture_height,		// needed for fixup
		void *pClientRenderable = NULL,
		int nXDice = 1,
		int nYDice = 1 )=0;

	virtual void LoadBoneMatrix( int boneIndex, const matrix3x4_t& matrix ) = 0;

	// This version will push the current rendertarget + current viewport onto the stack
	virtual void PushRenderTargetAndViewport( ) = 0;

	// This version will push a new rendertarget + a maximal viewport for that rendertarget onto the stack
	virtual void PushRenderTargetAndViewport( ITexture *pTexture ) = 0;

	// This version will push a new rendertarget + a specified viewport onto the stack
	virtual void PushRenderTargetAndViewport( ITexture *pTexture, int nViewX, int nViewY, int nViewW, int nViewH ) = 0;

	// This version will push a new rendertarget + a specified viewport onto the stack
	virtual void PushRenderTargetAndViewport( ITexture *pTexture, ITexture *pDepthTexture, int nViewX, int nViewY, int nViewW, int nViewH ) = 0;

	// This will pop a rendertarget + viewport
	virtual void PopRenderTargetAndViewport( void ) = 0;

	// Binds a particular texture as the current lightmap
	virtual void BindLightmapTexture( ITexture *pLightmapTexture ) = 0;

	// Blit a subrect of the current render target to another texture
	virtual void CopyRenderTargetToTextureEx( ITexture *pTexture, int nRenderTargetID, Rect_t *pSrcRect, Rect_t *pDstRect = NULL ) = 0;
	virtual void CopyTextureToRenderTargetEx( int nRenderTargetID, ITexture *pTexture, Rect_t *pSrcRect, Rect_t *pDstRect = NULL ) = 0;

	// Special off-center perspective matrix for DoF, MSAA jitter and poster rendering
	virtual void PerspectiveOffCenterX( double fovx, double aspect, double zNear, double zFar, double bottom, double top, double left, double right ) = 0;

	// Rendering parameters control special drawing modes withing the material system, shader
	// system, shaders, and engine. renderparm.h has their definitions.
	virtual void SetFloatRenderingParameter(int parm_number, float value) = 0;
	virtual void SetIntRenderingParameter(int parm_number, int value) = 0;
	virtual void SetVectorRenderingParameter(int parm_number, Vector const &value) = 0;

	// stencil buffer operations.
	virtual void SetStencilEnable(bool onoff) = 0;
	virtual void SetStencilFailOperation(StencilOperation_t op) = 0;
	virtual void SetStencilZFailOperation(StencilOperation_t op) = 0;
	virtual void SetStencilPassOperation(StencilOperation_t op) = 0;
	virtual void SetStencilCompareFunction(StencilComparisonFunction_t cmpfn) = 0;
	virtual void SetStencilReferenceValue(int ref) = 0;
	virtual void SetStencilTestMask(uint32 msk) = 0;
	virtual void SetStencilWriteMask(uint32 msk) = 0;
	virtual void ClearStencilBufferRectangle(int xmin, int ymin, int xmax, int ymax,int value) =0;	

	virtual void SetRenderTargetEx( int nRenderTargetID, ITexture *pTexture ) = 0;

	// rendering clip planes, beware that only the most recently pushed plane will actually be used in a sizeable chunk of hardware configurations
	// and that changes to the clip planes mid-frame while UsingFastClipping() is true will result unresolvable depth inconsistencies
	virtual void PushCustomClipPlane( const float *pPlane ) = 0;
	virtual void PopCustomClipPlane( void ) = 0;

	// Returns the number of vertices + indices we can render using the dynamic mesh
	// Passing true in the second parameter will return the max # of vertices + indices
	// we can use before a flush is provoked and may return different values 
	// if called multiple times in succession. 
	// Passing false into the second parameter will return
	// the maximum possible vertices + indices that can be rendered in a single batch
	virtual void GetMaxToRender( IMesh *pMesh, bool bMaxUntilFlush, int *pMaxVerts, int *pMaxIndices ) = 0;

	// Returns the max possible vertices + indices to render in a single draw call
	virtual int GetMaxVerticesToRender( IMaterial *pMaterial ) = 0;
	virtual int GetMaxIndicesToRender( ) = 0;
	virtual void DisableAllLocalLights() = 0;
	virtual int CompareMaterialCombos( IMaterial *pMaterial1, IMaterial *pMaterial2, int lightMapID1, int lightMapID2 ) = 0;

	virtual IMesh *GetFlexMesh() = 0;

	virtual void SetFlashlightStateEx( const FlashlightState_t &state, const VMatrix &worldToTexture, ITexture *pFlashlightDepthTexture ) = 0;

	// Returns the currently bound local cubemap
	virtual ITexture *GetLocalCubemap( ) = 0;

	// This is a version of clear buffers which will only clear the buffer at pixels which pass the stencil test
	virtual void ClearBuffersObeyStencil( bool bClearColor, bool bClearDepth ) = 0;

	//enables/disables all entered clipping planes, returns the input from the last time it was called.
	virtual bool EnableClipping( bool bEnable ) = 0;

	//get fog distances entered with FogStart(), FogEnd(), and SetFogZ()
	virtual void GetFogDistances( float *fStart, float *fEnd, float *fFogZ ) = 0;

	// Hooks for firing PIX events from outside the Material System...
	virtual void BeginPIXEvent( unsigned long color, const char *szName ) = 0;
	virtual void EndPIXEvent() = 0;
	virtual void SetPIXMarker( unsigned long color, const char *szName ) = 0;

	// Batch API
	// from changelist 166623:
	// - replaced obtuse material system batch usage with an explicit and easier to thread API
	virtual void BeginBatch( IMesh* pIndices ) = 0;
	virtual void BindBatch( IMesh* pVertices, IMaterial *pAutoBind = NULL ) = 0;
	virtual void DrawBatch(int firstIndex, int numIndices ) = 0;
	virtual void EndBatch() = 0;

	// Raw access to the call queue, which can be NULL if not in a queued mode
	virtual ICallQueue *GetCallQueue() = 0;

	// Returns the world-space camera position
	virtual void GetWorldSpaceCameraPosition( Vector *pCameraPos ) = 0;
	virtual void GetWorldSpaceCameraVectors( Vector *pVecForward, Vector *pVecRight, Vector *pVecUp ) = 0;

	// Tone mapping
	virtual void				ResetToneMappingScale( float monoscale) = 0; 			// set scale to monoscale instantly with no chasing
	virtual void				SetGoalToneMappingScale( float monoscale)  = 0; 			// set scale to monoscale instantly with no chasing

	// call TurnOnToneMapping before drawing the 3d scene to get the proper interpolated brightness
	// value set.
	virtual void				TurnOnToneMapping() = 0;

	// Set a linear vector color scale for all 3D rendering.
	// A value of [1.0f, 1.0f, 1.0f] should match non-tone-mapped rendering.
	virtual void				SetToneMappingScaleLinear( const Vector &scale ) = 0;

	virtual Vector				GetToneMappingScaleLinear( void ) = 0;
	virtual void				SetShadowDepthBiasFactors( float fSlopeScaleDepthBias, float fDepthBias ) = 0;

	// Apply stencil operations to every pixel on the screen without disturbing depth or color buffers
	virtual void				PerformFullScreenStencilOperation( void ) = 0;

	// Sets lighting origin for the current model (needed to convert directional lights to points)
	virtual void				SetLightingOrigin( Vector vLightingOrigin ) = 0;

	// Set scissor rect for rendering
	virtual void				SetScissorRect( const int nLeft, const int nTop, const int nRight, const int nBottom, const bool bEnableScissor ) = 0;

	// Methods used to build the morph accumulator that is read from when HW morph<ing is enabled.
	virtual void				BeginMorphAccumulation() = 0;
	virtual void				EndMorphAccumulation() = 0;
	virtual void				AccumulateMorph( IMorph* pMorph, int nMorphCount, const MorphWeight_t* pWeights ) = 0;

	virtual void				PushDeformation( DeformationBase_t const *Deformation ) = 0;
	virtual void				PopDeformation( ) = 0;
	virtual int					GetNumActiveDeformations() const = 0;

	virtual bool				GetMorphAccumulatorTexCoord( Vector2D *pTexCoord, IMorph *pMorph, int nVertex ) = 0;

	// Version of get dynamic mesh that specifies a specific vertex format
	virtual IMesh*				GetDynamicMeshEx( VertexFormat_t vertexFormat, bool bBuffered = true, 
		IMesh* pVertexOverride = 0,	IMesh* pIndexOverride = 0, IMaterial *pAutoBind = 0 ) = 0;

	virtual void				FogMaxDensity( float flMaxDensity ) = 0;

#if defined( _X360 )
	//Seems best to expose GPR allocation to scene rendering code. 128 total to split between vertex/pixel shaders (pixel will be set to 128 - vertex). Minimum value of 16. More GPR's = more threads.
	virtual void				PushVertexShaderGPRAllocation( int iVertexShaderCount = 64 ) = 0;
	virtual void				PopVertexShaderGPRAllocation( void ) = 0;
#endif

	virtual IMaterial *GetCurrentMaterial() = 0;
	virtual int  GetCurrentNumBones() const = 0;
	virtual void *GetCurrentProxy() = 0;

	// Color correction related methods..
	// Client cannot call IColorCorrectionSystem directly because it is not thread-safe
	// FIXME: Make IColorCorrectionSystem threadsafe?
	virtual void EnableColorCorrection( bool bEnable ) = 0;
	virtual ColorCorrectionHandle_t AddLookup( const char *pName ) = 0;
	virtual bool RemoveLookup( ColorCorrectionHandle_t handle ) = 0;
	virtual void LockLookup( ColorCorrectionHandle_t handle ) = 0;
	virtual void LoadLookup( ColorCorrectionHandle_t handle, const char *pLookupName ) = 0;
	virtual void UnlockLookup( ColorCorrectionHandle_t handle ) = 0;
	virtual void SetLookupWeight( ColorCorrectionHandle_t handle, float flWeight ) = 0;
	virtual void ResetLookupWeights( ) = 0;
	virtual void SetResetable( ColorCorrectionHandle_t handle, bool bResetable ) = 0;

	//There are some cases where it's simply not reasonable to update the full screen depth texture (mostly on PC).
	//Use this to mark it as invalid and use a dummy texture for depth reads.
	virtual void SetFullScreenDepthTextureValidityFlag( bool bIsValid ) = 0;

	// A special path used to tick the front buffer while loading on the 360
	virtual void SetNonInteractivePacifierTexture( ITexture *pTexture, float flNormalizedX, float flNormalizedY, float flNormalizedSize ) = 0;
	virtual void SetNonInteractiveTempFullscreenBuffer( ITexture *pTexture, MaterialNonInteractiveMode_t mode ) = 0;
	virtual void EnableNonInteractiveMode( MaterialNonInteractiveMode_t mode ) = 0;
	virtual void RefreshFrontBufferNonInteractive() = 0;
	// Allocates temp render data. Renderdata goes out of scope at frame end in multicore
	// Renderdata goes out of scope after refcount goes to zero in singlecore.
	// Locking/unlocking increases + decreases refcount
	virtual void *			LockRenderData( int nSizeInBytes ) = 0;
	virtual void			UnlockRenderData( void *pData ) = 0;

	// Typed version. If specified, pSrcData is copied into the locked memory.
	template< class E > E*  LockRenderDataTyped( int nCount, const E* pSrcData = NULL );

	// Temp render data gets immediately freed after it's all unlocked in single core.
	// This prevents it from being freed
	virtual void			AddRefRenderData() = 0;	
	virtual void			ReleaseRenderData() = 0;

	// Returns whether a pointer is render data. NOTE: passing NULL returns true
	virtual bool			IsRenderData( const void *pData ) const = 0;
	virtual void			PrintfVA( char *fmt, va_list vargs ) = 0;
	virtual void			Printf( PRINTF_FORMAT_STRING const char *fmt, ... ) = 0;
	virtual float			Knob( char *knobname, float *setvalue = NULL ) = 0;
	// Allows us to override the alpha write setting of a material
	virtual void OverrideAlphaWriteEnable( bool bEnable, bool bAlphaWriteEnable ) = 0;
	virtual void OverrideColorWriteEnable( bool bOverrideEnable, bool bColorWriteEnable ) = 0;

	virtual void ClearBuffersObeyStencilEx( bool bClearColor, bool bClearAlpha, bool bClearDepth ) = 0;
};

template< class E > inline E* IMatRenderContext::LockRenderDataTyped( int nCount, const E* pSrcData )
{
	int nSizeInBytes = nCount * sizeof(E);
	E *pDstData = (E*)LockRenderData( nSizeInBytes );
	if ( pSrcData && pDstData )
	{
		memcpy( pDstData, pSrcData, nSizeInBytes );
	}
	return pDstData;
}


//-----------------------------------------------------------------------------
// Utility class for addreffing/releasing render data (prevents freeing on single core)
//-----------------------------------------------------------------------------
class CMatRenderDataReference
{
public:
	CMatRenderDataReference();
	CMatRenderDataReference( IMatRenderContext* pRenderContext );
	~CMatRenderDataReference();
	void Lock( IMatRenderContext *pRenderContext );
	void Release();

private:
	IMatRenderContext *m_pRenderContext;
};


inline CMatRenderDataReference::CMatRenderDataReference()
{
	m_pRenderContext = NULL;
}

inline CMatRenderDataReference::CMatRenderDataReference( IMatRenderContext* pRenderContext )
{
	m_pRenderContext = NULL;
	Lock( pRenderContext );
}

inline CMatRenderDataReference::~CMatRenderDataReference()
{
	Release();
}

inline void CMatRenderDataReference::Lock( IMatRenderContext* pRenderContext )
{
	if ( !m_pRenderContext )
	{
		m_pRenderContext = pRenderContext;
		m_pRenderContext->AddRefRenderData( );
	}
}

inline void CMatRenderDataReference::Release()
{
	if ( m_pRenderContext )
	{
		m_pRenderContext->ReleaseRenderData( );
		m_pRenderContext = NULL;
	}
}


//-----------------------------------------------------------------------------
// Utility class for locking/unlocking render data
//-----------------------------------------------------------------------------
template< typename E > 
class CMatRenderData
{
public:
	CMatRenderData( IMatRenderContext* pRenderContext );
	CMatRenderData( IMatRenderContext* pRenderContext, int nCount, const E *pSrcData = NULL );
	~CMatRenderData();
	E* Lock( int nCount, const E* pSrcData = NULL ); 
	void Release();
	bool IsValid() const;
	const E* Base() const;
	E* Base();
	const E& operator[]( int i ) const;
	E& operator[]( int i );

private:
	IMatRenderContext* m_pRenderContext;
	E *m_pRenderData;
	int m_nCount;
	bool m_bNeedsUnlock;
};

template< typename E >
inline CMatRenderData<E>::CMatRenderData( IMatRenderContext* pRenderContext )
{
	m_pRenderContext = pRenderContext;
	m_nCount = 0;
	m_pRenderData = 0;
	m_bNeedsUnlock = false;
}

template< typename E >
inline CMatRenderData<E>::CMatRenderData( IMatRenderContext* pRenderContext, int nCount, const E* pSrcData )
{
	m_pRenderContext = pRenderContext;
	m_nCount = 0;
	m_pRenderData = 0;
	m_bNeedsUnlock = false;
	Lock( nCount, pSrcData );
}

template< typename E >
inline CMatRenderData<E>::~CMatRenderData()
{
	Release();
}

template< typename E >
inline bool CMatRenderData<E>::IsValid() const
{
	return m_pRenderData != NULL;
}

template< typename E >
inline E* CMatRenderData<E>::Lock( int nCount, const E* pSrcData )
{
	m_nCount = nCount;
	if ( pSrcData && m_pRenderContext->IsRenderData( pSrcData ) )
	{
		// Yes, we're const-casting away, but that should be ok since 
		// the src data is render data
		m_pRenderData = const_cast<E*>( pSrcData );
		m_pRenderContext->AddRefRenderData();
		m_bNeedsUnlock = false;
		return m_pRenderData;
	}
	m_pRenderData = m_pRenderContext->LockRenderDataTyped<E>( nCount, pSrcData );
	m_bNeedsUnlock = true;
	return m_pRenderData;
}

template< typename E >
inline void CMatRenderData<E>::Release()
{
	if ( m_pRenderContext && m_pRenderData )
	{
		if ( m_bNeedsUnlock )
		{
			m_pRenderContext->UnlockRenderData( m_pRenderData );
		}
		else
		{
			m_pRenderContext->ReleaseRenderData();
		}
	}
	m_pRenderData = NULL;
	m_nCount = 0;
	m_bNeedsUnlock = false;
}

template< typename E >
inline E* CMatRenderData<E>::Base()
{
	return m_pRenderData;
}

template< typename E >
inline const E* CMatRenderData<E>::Base() const
{
	return m_pRenderData;
}

template< typename E >
inline E& CMatRenderData<E>::operator[]( int i )
{
	Assert( ( i >= 0 ) && ( i < m_nCount ) );
	return m_pRenderData[i];
}

template< typename E >
inline const E& CMatRenderData<E>::operator[]( int i ) const
{
	Assert( ( i >= 0 ) && ( i < m_nCount ) );
	return m_pRenderData[i];
}


//-----------------------------------------------------------------------------

class CMatRenderContextPtr : public CRefPtr<IMatRenderContext>
{
	typedef CRefPtr<IMatRenderContext> BaseClass;
public:
	CMatRenderContextPtr()																					{}
	CMatRenderContextPtr( IMatRenderContext *pInit )			: BaseClass( pInit )						{ if ( BaseClass::m_pObject ) BaseClass::m_pObject->BeginRender(); }
	CMatRenderContextPtr( IMaterialSystem *pFrom )				: BaseClass( pFrom->GetRenderContext() )	{ if ( BaseClass::m_pObject ) BaseClass::m_pObject->BeginRender(); }
	~CMatRenderContextPtr()																					{ if ( BaseClass::m_pObject ) BaseClass::m_pObject->EndRender(); }

	IMatRenderContext *operator=( IMatRenderContext *p )		{ if ( p ) p->BeginRender(); return BaseClass::operator=( p ); }

	void SafeRelease()											{ if ( BaseClass::m_pObject ) BaseClass::m_pObject->EndRender(); BaseClass::SafeRelease(); }
	void AssignAddRef( IMatRenderContext *pFrom )				{ if ( BaseClass::m_pObject ) BaseClass::m_pObject->EndRender(); BaseClass::AssignAddRef( pFrom ); BaseClass::m_pObject->BeginRender(); }

	void GetFrom( IMaterialSystem *pFrom )						{ AssignAddRef( pFrom->GetRenderContext() ); }


private:
	CMatRenderContextPtr( const CMatRenderContextPtr &from );
	void operator=( const CMatRenderContextPtr &from );

};

//-----------------------------------------------------------------------------
// Helper class for begin/end of pix event via constructor/destructor 
//-----------------------------------------------------------------------------
#define PIX_VALVE_ORANGE	0xFFF5940F

class PIXEvent
{
public:
	PIXEvent( IMatRenderContext *pRenderContext, const char *szName, unsigned long color = PIX_VALVE_ORANGE )
	{
		m_pRenderContext = pRenderContext;
		Assert( m_pRenderContext );
		Assert( szName );
		m_pRenderContext->BeginPIXEvent( color, szName );
	}
	~PIXEvent()
	{
		m_pRenderContext->EndPIXEvent();
	}
private:
	IMatRenderContext *m_pRenderContext;
};


// Also be sure to enable PIX_INSTRUMENTATION in shaderdevicedx8.h
//#define PIX_ENABLE 1		// set this to 1 and build engine/studiorender to enable pix events in the engine

#if PIX_ENABLE
#	define PIXEVENT PIXEvent _pixEvent
#else
#	define PIXEVENT 
#endif

//-----------------------------------------------------------------------------

#ifdef MATERIAL_SYSTEM_DEBUG_CALL_QUEUE
#include "tier1/callqueue.h"
#include "tier1/fmtstr.h"
static void DoMatSysQueueMark( IMaterialSystem *pMaterialSystem, const char *psz )
{
	CMatRenderContextPtr pRenderContext( pMaterialSystem );
	if ( pRenderContext->GetCallQueue() ) 
		pRenderContext->GetCallQueue()->QueueCall( Plat_DebugString, CUtlEnvelope<const char *>( psz ) );
}

#define MatSysQueueMark( pMaterialSystem, ...) DoMatSysQueueMark( pMaterialSystem, CFmtStr( __VA_ARGS__ ) )
#else
#define MatSysQueueMark( msg, ...) ((void)0)
#endif

//-----------------------------------------------------------------------------

extern IMaterialSystem *materials;
extern IMaterialSystem *g_pMaterialSystem;

#endif // IMATERIALSYSTEM_H
