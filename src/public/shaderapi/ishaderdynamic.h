//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef ISHADERDYNAMIC_H
#define ISHADERDYNAMIC_H

#ifdef _WIN32
#pragma once
#endif

#include "shaderapi/shareddefs.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystem.h"
#include "tier0/basetypes.h"


typedef int ShaderAPITextureHandle_t;

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class CMeshBuilder;
class IMaterialVar;
struct LightDesc_t; 


//-----------------------------------------------------------------------------
// State from ShaderAPI used to select proper vertex and pixel shader combos
//-----------------------------------------------------------------------------
struct LightState_t
{
	int  m_nNumLights;
	bool m_bAmbientLight;
	bool m_bStaticLightVertex;
	bool m_bStaticLightTexel;
	inline int HasDynamicLight() { return (m_bAmbientLight || (m_nNumLights > 0)) ? 1 : 0; }
};


//-----------------------------------------------------------------------------
// Color correction info
//-----------------------------------------------------------------------------
struct ShaderColorCorrectionInfo_t
{
	bool m_bIsEnabled;
	int m_nLookupCount;
	float m_flDefaultWeight;
	float m_pLookupWeights[4];
};


//-----------------------------------------------------------------------------
// the 3D shader API interface
// This interface is all that shaders see.
//-----------------------------------------------------------------------------
enum StandardTextureId_t
{
	// Lightmaps
	TEXTURE_LIGHTMAP = 0,
	TEXTURE_LIGHTMAP_FULLBRIGHT,
	TEXTURE_LIGHTMAP_BUMPED,
	TEXTURE_LIGHTMAP_BUMPED_FULLBRIGHT,

	// Flat colors
	TEXTURE_WHITE,
	TEXTURE_BLACK,
	TEXTURE_GREY,
	TEXTURE_GREY_ALPHA_ZERO,

	// Normalmaps
	TEXTURE_NORMALMAP_FLAT,

	// Normalization
	TEXTURE_NORMALIZATION_CUBEMAP,
	TEXTURE_NORMALIZATION_CUBEMAP_SIGNED,

	// Frame-buffer textures
	TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0,
	TEXTURE_FRAME_BUFFER_FULL_TEXTURE_1,

	// Color correction
	TEXTURE_COLOR_CORRECTION_VOLUME_0,
	TEXTURE_COLOR_CORRECTION_VOLUME_1,
	TEXTURE_COLOR_CORRECTION_VOLUME_2,
	TEXTURE_COLOR_CORRECTION_VOLUME_3,

	// An alias to the Back Frame Buffer
	TEXTURE_FRAME_BUFFER_ALIAS,

	// Noise for shadow mapping algorithm
	TEXTURE_SHADOW_NOISE_2D,

	// A texture in which morph data gets accumulated (vs30, fast vertex textures required)
	TEXTURE_MORPH_ACCUMULATOR,

	// A texture which contains morph weights
	TEXTURE_MORPH_WEIGHTS,

	// A snapshot of the frame buffer's depth. Currently only valid on the 360
	TEXTURE_FRAME_BUFFER_FULL_DEPTH,

	// A snapshot of the frame buffer's depth. Currently only valid on the 360
	TEXTURE_IDENTITY_LIGHTWARP,

	// Equivalent to the debug material for mat_luxels, in convenient texture form.
	TEXTURE_DEBUG_LUXELS,

	TEXTURE_MAX_STD_TEXTURES = 32
};

//-----------------------------------------------------------------------------
// Viewport structure
//-----------------------------------------------------------------------------
#define SHADER_VIEWPORT_VERSION 1
struct ShaderViewport_t
{
	int m_nVersion;
	int m_nTopLeftX;
	int m_nTopLeftY;
	int m_nWidth;
	int m_nHeight;
	float m_flMinZ;
	float m_flMaxZ;

	ShaderViewport_t() : m_nVersion( SHADER_VIEWPORT_VERSION ) {}

	void Init()
	{
		memset( this, 0, sizeof(ShaderViewport_t) );
		m_nVersion = SHADER_VIEWPORT_VERSION;
	}

	void Init( int x, int y, int nWidth, int nHeight, float flMinZ = 0.0f, float flMaxZ = 1.0f )
	{
		m_nVersion = SHADER_VIEWPORT_VERSION;
		m_nTopLeftX = x; m_nTopLeftY = y; m_nWidth = nWidth; m_nHeight = nHeight;
		m_flMinZ = flMinZ;
		m_flMaxZ = flMaxZ;
	}
};


//-----------------------------------------------------------------------------
// The Shader interface versions
//-----------------------------------------------------------------------------
#define SHADERDYNAMIC_INTERFACE_VERSION		"ShaderDynamic001"
abstract_class IShaderDynamicAPI
{
public:

	virtual void SetViewports( int nCount, const ShaderViewport_t* pViewports ) = 0;
	virtual int GetViewports( ShaderViewport_t* pViewports, int nMax ) const = 0;

	// returns the current time in seconds....
	virtual double CurrentTime() const = 0;

	// Gets the lightmap dimensions
	virtual void GetLightmapDimensions( int *w, int *h ) = 0;

	// Scene fog state.
	// This is used by the shaders for picking the proper vertex shader for fogging based on dynamic state.
	virtual MaterialFogMode_t GetSceneFogMode( ) = 0;
	virtual void GetSceneFogColor( unsigned char *rgb ) = 0;

	// stuff related to matrix stacks
	virtual void MatrixMode( MaterialMatrixMode_t matrixMode ) = 0;
	virtual void PushMatrix() = 0;
	virtual void PopMatrix() = 0;
	virtual void LoadMatrix( float *m ) = 0;
	virtual void MultMatrix( float *m ) = 0;
	virtual void MultMatrixLocal( float *m ) = 0;
	virtual void GetMatrix( MaterialMatrixMode_t matrixMode, float *dst ) = 0;
	virtual void LoadIdentity( void ) = 0;
	virtual void LoadCameraToWorld( void ) = 0;
	virtual void Ortho( double left, double right, double bottom, double top, double zNear, double zFar ) = 0;
	virtual void PerspectiveX( double fovx, double aspect, double zNear, double zFar ) = 0;
	virtual	void PickMatrix( int x, int y, int width, int height ) = 0;
	virtual void Rotate( float angle, float x, float y, float z ) = 0;
	virtual void Translate( float x, float y, float z ) = 0;
	virtual void Scale( float x, float y, float z ) = 0;
	virtual void ScaleXY( float x, float y ) = 0;

	// Sets the color to modulate by
	virtual void Color3f( float r, float g, float b ) = 0;
	virtual void Color3fv( float const* pColor ) = 0;
	virtual void Color4f( float r, float g, float b, float a ) = 0;
	virtual void Color4fv( float const* pColor ) = 0;

	virtual void Color3ub( unsigned char r, unsigned char g, unsigned char b ) = 0;
	virtual void Color3ubv( unsigned char const* pColor ) = 0;
	virtual void Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a ) = 0;
	virtual void Color4ubv( unsigned char const* pColor ) = 0;

	// Sets the constant register for vertex and pixel shaders
	virtual void SetVertexShaderConstant( int var, float const* pVec, int numConst = 1, bool bForce = false ) = 0;
	virtual void SetPixelShaderConstant( int var, float const* pVec, int numConst = 1, bool bForce = false ) = 0;

	// Sets the default *dynamic* state
	virtual void SetDefaultState() = 0;

	// Get the current camera position in world space.
	virtual void GetWorldSpaceCameraPosition( float* pPos ) const = 0;

	virtual int GetCurrentNumBones( void ) const = 0;
	virtual int GetCurrentLightCombo( void ) const = 0;

	virtual MaterialFogMode_t GetCurrentFogType( void ) const = 0;

	// fixme: move this to shadow state
	virtual void SetTextureTransformDimension( TextureStage_t textureStage, int dimension, bool projected ) = 0;
	virtual void DisableTextureTransform( TextureStage_t textureStage ) = 0;
	virtual void SetBumpEnvMatrix( TextureStage_t textureStage, float m00, float m01, float m10, float m11 ) = 0;

	// Sets the vertex and pixel shaders
	virtual void SetVertexShaderIndex( int vshIndex = -1 ) = 0;
	virtual void SetPixelShaderIndex( int pshIndex = 0 ) = 0;

	// Get the dimensions of the back buffer.
	virtual void GetBackBufferDimensions( int& width, int& height ) const = 0;

	// FIXME: The following 6 methods used to live in IShaderAPI
	// and were moved for stdshader_dx8. Let's try to move them back!

	// Get the lights
	virtual int GetMaxLights( void ) const = 0;
	virtual const LightDesc_t& GetLight( int lightNum ) const = 0;

	virtual void SetPixelShaderFogParams( int reg ) = 0;

	// Render state for the ambient light cube
	virtual void SetVertexShaderStateAmbientLightCube() = 0;
	virtual void SetPixelShaderStateAmbientLightCube( int pshReg, bool bForceToBlack = false ) = 0;
	virtual void CommitPixelShaderLighting( int pshReg ) = 0;

	// Use this to get the mesh builder that allows us to modify vertex data
	virtual CMeshBuilder* GetVertexModifyBuilder() = 0;
	virtual bool InFlashlightMode() const = 0;
	virtual const FlashlightState_t &GetFlashlightState( VMatrix &worldToTexture ) const = 0;
	virtual bool InEditorMode() const = 0;

	// Gets the bound morph's vertex format; returns 0 if no morph is bound
	virtual MorphFormat_t GetBoundMorphFormat() = 0;

	// Binds a standard texture
	virtual void BindStandardTexture( Sampler_t sampler, StandardTextureId_t id ) = 0;

	virtual ITexture *GetRenderTargetEx( int nRenderTargetID ) = 0;

	virtual void SetToneMappingScaleLinear( const Vector &scale ) = 0;
	virtual const Vector &GetToneMappingScaleLinear( void ) const = 0;
	virtual float GetLightMapScaleFactor( void ) const = 0;

	virtual void LoadBoneMatrix( int boneIndex, const float *m ) = 0;

	virtual void PerspectiveOffCenterX( double fovx, double aspect, double zNear, double zFar, double bottom, double top, double left, double right ) = 0;

	virtual void SetFloatRenderingParameter(int parm_number, float value) = 0;

	virtual void SetIntRenderingParameter(int parm_number, int value) = 0 ;
	virtual void SetVectorRenderingParameter(int parm_number, Vector const &value) = 0 ;

	virtual float GetFloatRenderingParameter(int parm_number) const = 0 ;

	virtual int GetIntRenderingParameter(int parm_number) const = 0 ;

	virtual Vector GetVectorRenderingParameter(int parm_number) const = 0 ;

	// stencil buffer operations.
	virtual void SetStencilEnable(bool onoff) = 0;
	virtual void SetStencilFailOperation(StencilOperation_t op) = 0;
	virtual void SetStencilZFailOperation(StencilOperation_t op) = 0;
	virtual void SetStencilPassOperation(StencilOperation_t op) = 0;
	virtual void SetStencilCompareFunction(StencilComparisonFunction_t cmpfn) = 0;
	virtual void SetStencilReferenceValue(int ref) = 0;
	virtual void SetStencilTestMask(uint32 msk) = 0;
	virtual void SetStencilWriteMask(uint32 msk) = 0;
	virtual void ClearStencilBufferRectangle( int xmin, int ymin, int xmax, int ymax,int value) = 0;

	virtual void GetDXLevelDefaults(uint &max_dxlevel,uint &recommended_dxlevel) = 0;

	virtual const FlashlightState_t &GetFlashlightStateEx( VMatrix &worldToTexture, ITexture **pFlashlightDepthTexture ) const = 0;

	virtual float GetAmbientLightCubeLuminance() = 0;

	virtual void GetDX9LightState( LightState_t *state ) const = 0;
	virtual int GetPixelFogCombo( ) = 0; //0 is either range fog, or no fog simulated with rigged range fog values. 1 is height fog

	virtual void BindStandardVertexTexture( VertexTextureSampler_t sampler, StandardTextureId_t id ) = 0;

	// Is hardware morphing enabled?
	virtual bool IsHWMorphingEnabled( ) const = 0;

	virtual void GetStandardTextureDimensions( int *pWidth, int *pHeight, StandardTextureId_t id ) = 0;

	virtual void SetBooleanVertexShaderConstant( int var, BOOL const* pVec, int numBools = 1, bool bForce = false ) = 0;
	virtual void SetIntegerVertexShaderConstant( int var, int const* pVec, int numIntVecs = 1, bool bForce = false ) = 0;
	virtual void SetBooleanPixelShaderConstant( int var, BOOL const* pVec, int numBools = 1, bool bForce = false ) = 0;
	virtual void SetIntegerPixelShaderConstant( int var, int const* pVec, int numIntVecs = 1, bool bForce = false ) = 0;
	
	//Are we in a configuration that needs access to depth data through the alpha channel later?
	virtual bool ShouldWriteDepthToDestAlpha( void ) const = 0;


	// deformations
	virtual void PushDeformation( DeformationBase_t const *Deformation ) = 0;
	virtual void PopDeformation( ) = 0;
	virtual int GetNumActiveDeformations() const =0;


	// for shaders to set vertex shader constants. returns a packed state which can be used to set
	// the dynamic combo. returns # of active deformations
	virtual int GetPackedDeformationInformation( int nMaskOfUnderstoodDeformations,
												 float *pConstantValuesOut,
												 int nBufferSize,
												 int nMaximumDeformations,
												 int *pNumDefsOut ) const = 0;

	// This lets the lower level system that certain vertex fields requested 
	// in the shadow state aren't actually being read given particular state
	// known only at dynamic state time. It's here only to silence warnings.
	virtual void MarkUnusedVertexFields( unsigned int nFlags, int nTexCoordCount, bool *pUnusedTexCoords ) = 0;


	virtual void ExecuteCommandBuffer( uint8 *pCmdBuffer ) =0;

	// interface for mat system to tell shaderapi about standard texture handles
	virtual void SetStandardTextureHandle( StandardTextureId_t nId, ShaderAPITextureHandle_t nHandle ) =0;

	// Interface for mat system to tell shaderapi about color correction
	virtual void GetCurrentColorCorrection( ShaderColorCorrectionInfo_t* pInfo ) = 0;

	virtual void SetPSNearAndFarZ( int pshReg ) = 0;

	virtual void SetDepthFeatheringPixelShaderConstant( int iConstant, float fDepthBlendScale ) = 0;
};
// end class IShaderDynamicAPI

//-----------------------------------------------------------------------------
// Software vertex shaders
//-----------------------------------------------------------------------------
typedef void (*SoftwareVertexShader_t)( CMeshBuilder& meshBuilder, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI );


#endif // ISHADERDYNAMIC_H
