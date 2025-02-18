//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//==================================================================================================

#ifndef DXABSTRACT_H
#define DXABSTRACT_H
#ifdef _WIN32
#pragma once
#endif

#include "togl/rendermechanism.h"

#include "materialsystem/ishader.h"

// Uncomment this on Windows if you want to compile the Windows GL version.
// #undef USE_ACTUAL_DX

#ifdef USE_ACTUAL_DX

#ifndef WIN32
#error sorry man
#endif
#ifdef _X360
#include "d3d9.h"
#include "d3dx9.h"
#else
#include <windows.h>
#include "../../dx9sdk/include/d3d9.h"
#include "../../dx9sdk/include/d3dx9.h"
#endif
typedef HWND VD3DHWND;

#else

#ifdef WIN32
#error Gl on win32?
#endif

#include "tier0/platform.h"

#ifndef DX_TO_GL_ABSTRACTION
#define DX_TO_GL_ABSTRACTION
#endif

#include "bitmap/imageformat.h"
#include "togl/rendermechanism.h"

#ifdef OSX
extern "C" void Debugger(void);
#endif

// turn this on to get refcount logging from IUnknown
#define	IUNKNOWN_ALLOC_SPEW 0
#define	IUNKNOWN_ALLOC_SPEW_MARK_ALL 0	


// ------------------------------------------------------------------------------------------------------------------------------ //
// DEFINES
// ------------------------------------------------------------------------------------------------------------------------------ //

typedef void* VD3DHWND;
typedef void* VD3DHANDLE;


TOGL_INTERFACE void toglGetClientRect( VD3DHWND hWnd, RECT *destRect );

struct TOGL_CLASS IUnknown
{
	int	m_refcount[2];
	bool m_mark;
		
	IUnknown( void )
	{
		m_refcount[0] = 1;
		m_refcount[1] = 0;
		m_mark = (IUNKNOWN_ALLOC_SPEW_MARK_ALL != 0);	// either all are marked, or only the ones that have SetMark(true) called on them

		#if IUNKNOWN_ALLOC_SPEW
			if (m_mark)
			{
				GLMPRINTF(("-A- IUnew (%08x) refc -> (%d,%d) ",this,m_refcount[0],m_refcount[1]));
			}
		#endif
	};
				
	virtual	~IUnknown( void )
	{
		#if IUNKNOWN_ALLOC_SPEW
			if (m_mark)
			{
				GLMPRINTF(("-A- IUdel (%08x) ",this ));
			}
		#endif
	};
		
	void	AddRef( int which=0, char *comment = NULL )
	{
		Assert( which >= 0 );
		Assert( which < 2 );
		m_refcount[which]++;
			
		#if IUNKNOWN_ALLOC_SPEW
			if (m_mark)
			{
				GLMPRINTF(("-A- IUAddRef  (%08x,%d) refc -> (%d,%d) [%s]",this,which,m_refcount[0],m_refcount[1],comment?comment:"..."))	;
				if (!comment)
				{
					GLMPRINTF((""))	;	// place to hang a breakpoint
				}
			}
		#endif
	};
		
	ULONG __stdcall	Release( int which=0, char *comment = NULL )
	{
		Assert( which >= 0 );
		Assert( which < 2 );
			
		//int oldrefcs[2] = { m_refcount[0], m_refcount[1] };
		bool deleting = false;
			
		m_refcount[which]--;
		if ( (!m_refcount[0]) && (!m_refcount[1]) )
		{
			deleting = true;
		}
			
		#if IUNKNOWN_ALLOC_SPEW
			if (m_mark)
			{
				GLMPRINTF(("-A- IURelease (%08x,%d) refc -> (%d,%d) [%s] %s",this,which,m_refcount[0],m_refcount[1],comment?comment:"...",deleting?"->DELETING":""));
				if (!comment)
				{
					GLMPRINTF((""))	;	// place to hang a breakpoint
				}
			}
		#endif

		if (deleting)
		{
			if (m_mark)
			{
				GLMPRINTF((""))	;		// place to hang a breakpoint
			}
			delete this;
			return 0;
		}
		else
		{
			return m_refcount[0];
		}
	};
		
	void	SetMark( bool markValue, char *comment=NULL )
	{
		#if IUNKNOWN_ALLOC_SPEW
			if (!m_mark && markValue)	// leading edge detect
			{
				// print the same thing that the constructor would have printed if it had been marked from the beginning
				// i.e. it's anticipated that callers asking for marking will do so right at create time
				GLMPRINTF(("-A- IUSetMark (%08x) refc -> (%d,%d) (%s) ",this,m_refcount[0],m_refcount[1],comment?comment:"..."));
			}
		#endif

		m_mark = markValue;
	}
};


// ------------------------------------------------------------------------------------------------------------------------------ //
// INTERFACES
// ------------------------------------------------------------------------------------------------------------------------------ //

struct TOGL_CLASS IDirect3DResource9 : public IUnknown
{
	IDirect3DDevice9	*m_device;		// parent device
	D3DRESOURCETYPE		m_restype;
	
	DWORD SetPriority(DWORD PriorityNew);
};

struct TOGL_CLASS IDirect3DBaseTexture9 : public IDirect3DResource9						// "A Texture.."
{	
	D3DSURFACE_DESC			m_descZero;			// desc of top level.
	CGLMTex					*m_tex;				// a CGLMTex can represent all forms of tex
	int						m_srgbFlipCount;

	virtual					~IDirect3DBaseTexture9();
    D3DRESOURCETYPE			GetType();
    DWORD					GetLevelCount();
	HRESULT					GetLevelDesc(UINT Level,D3DSURFACE_DESC *pDesc);
};

struct TOGL_CLASS IDirect3DTexture9 : public IDirect3DBaseTexture9							// "Texture 2D"
{	
	IDirect3DSurface9		*m_surfZero;			// surf of top level.

	virtual					~IDirect3DTexture9();

    HRESULT					LockRect(UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags);
    HRESULT					UnlockRect(UINT Level);
    HRESULT					GetSurfaceLevel(UINT Level,IDirect3DSurface9** ppSurfaceLevel);
};

struct TOGL_CLASS IDirect3DCubeTexture9 : public IDirect3DBaseTexture9						// "Texture Cube Map"
{
	IDirect3DSurface9		*m_surfZero[6];			// surfs of top level.

	virtual					~IDirect3DCubeTexture9();

    HRESULT					GetCubeMapSurface(D3DCUBEMAP_FACES FaceType,UINT Level,IDirect3DSurface9** ppCubeMapSurface);
    HRESULT					GetLevelDesc(UINT Level,D3DSURFACE_DESC *pDesc);
};

struct TOGL_CLASS IDirect3DVolumeTexture9 : public IDirect3DBaseTexture9					// "Texture 3D"
{
	IDirect3DSurface9		*m_surfZero;			// surf of top level.
	D3DVOLUME_DESC			m_volDescZero;			// volume desc top level

	virtual					~IDirect3DVolumeTexture9();

    HRESULT					LockBox(UINT Level,D3DLOCKED_BOX* pLockedVolume,CONST D3DBOX* pBox,DWORD Flags);
    HRESULT					UnlockBox(UINT Level);
	HRESULT					GetLevelDesc( UINT level, D3DVOLUME_DESC *pDesc );
};


// for the moment, a "D3D surface" is modeled as a GLM tex, a face, and a mip.
// no Create method, these are filled in by the various create surface methods.	

struct TOGL_CLASS IDirect3DSurface9 : public IDirect3DResource9
{
	virtual					~IDirect3DSurface9();

    HRESULT					LockRect(D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags);
    HRESULT					UnlockRect();
	HRESULT					GetDesc(D3DSURFACE_DESC *pDesc);

	D3DSURFACE_DESC			m_desc;
	CGLMTex					*m_tex;
	int						m_face;
	int						m_mip;
};



struct TOGL_CLASS IDirect3D9 : public IUnknown
{
public:
	virtual	~IDirect3D9();

	UINT	GetAdapterCount();			//cheese: returns 1

    HRESULT GetDeviceCaps				(UINT Adapter,D3DDEVTYPE DeviceType,D3DCAPS9* pCaps);
    HRESULT GetAdapterIdentifier		(UINT Adapter,DWORD Flags,D3DADAPTER_IDENTIFIER9* pIdentifier);
    HRESULT CheckDeviceFormat			(UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,DWORD Usage,D3DRESOURCETYPE RType,D3DFORMAT CheckFormat);
    UINT	GetAdapterModeCount			(UINT Adapter,D3DFORMAT Format);
    HRESULT EnumAdapterModes			(UINT Adapter,D3DFORMAT Format,UINT Mode,D3DDISPLAYMODE* pMode);
    HRESULT CheckDeviceType				(UINT Adapter,D3DDEVTYPE DevType,D3DFORMAT AdapterFormat,D3DFORMAT BackBufferFormat,BOOL bWindowed);
    HRESULT GetAdapterDisplayMode		(UINT Adapter,D3DDISPLAYMODE* pMode);
    HRESULT CheckDepthStencilMatch		(UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,D3DFORMAT RenderTargetFormat,D3DFORMAT DepthStencilFormat);
    HRESULT CheckDeviceMultiSampleType	(UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT SurfaceFormat,BOOL Windowed,D3DMULTISAMPLE_TYPE MultiSampleType,DWORD* pQualityLevels);

    HRESULT CreateDevice				(UINT Adapter,D3DDEVTYPE DeviceType,VD3DHWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface);
};

struct TOGL_CLASS IDirect3DSwapChain9 : public IUnknown
{
};



	//	typedef enum D3DDECLUSAGE
	//	{
	//		D3DDECLUSAGE_POSITION = 0,
	//		D3DDECLUSAGE_BLENDWEIGHT = 1,
	//		D3DDECLUSAGE_BLENDINDICES = 2,
	//		D3DDECLUSAGE_NORMAL = 3,
	//		D3DDECLUSAGE_PSIZE = 4,
	//		D3DDECLUSAGE_TEXCOORD = 5,
	//		D3DDECLUSAGE_TANGENT = 6,
	//		D3DDECLUSAGE_BINORMAL = 7,
	//		D3DDECLUSAGE_TESSFACTOR = 8,
	//		D3DDECLUSAGE_POSITIONT = 9,
	//		D3DDECLUSAGE_COLOR = 10,
	//		D3DDECLUSAGE_FOG = 11,
	//		D3DDECLUSAGE_DEPTH = 12,
	//		D3DDECLUSAGE_SAMPLE = 13,
	//	} D3DDECLUSAGE, *LPD3DDECLUSAGE;
	//	Constants
	//	
	//	D3DDECLUSAGE_POSITION
	//	Position data ranging from (-1,-1) to (1,1). Use D3DDECLUSAGE_POSITION with
	//	a usage index of 0 to specify untransformed position for fixed function
	//	vertex processing and the n-patch tessellator. Use D3DDECLUSAGE_POSITION
	//	with a usage index of 1 to specify untransformed position in the fixed
	//	function vertex shader for vertex tweening.
	//	
	//	D3DDECLUSAGE_BLENDWEIGHT
	//	Blending weight data. Use D3DDECLUSAGE_BLENDWEIGHT with a usage index of 0
	//	to specify the blend weights used in indexed and nonindexed vertex
	//	blending.
	//	
	//	D3DDECLUSAGE_BLENDINDICES
	//	Blending indices data. Use D3DDECLUSAGE_BLENDINDICES with a usage index of
	//	0 to specify matrix indices for indexed paletted skinning.
	//	
	//	D3DDECLUSAGE_NORMAL
	//	Vertex normal data. Use D3DDECLUSAGE_NORMAL with a usage index of 0 to
	//	specify vertex normals for fixed function vertex processing and the n-patch
	//	tessellator. Use D3DDECLUSAGE_NORMAL with a usage index of 1 to specify
	//	vertex normals for fixed function vertex processing for vertex tweening.
	//	
	//	D3DDECLUSAGE_PSIZE
	//	Point size data. Use D3DDECLUSAGE_PSIZE with a usage index of 0 to specify
	//	the point-size attribute used by the setup engine of the rasterizer to
	//	expand a point into a quad for the point-sprite functionality.
	//	
	//	D3DDECLUSAGE_TEXCOORD
	//	Texture coordinate data. Use D3DDECLUSAGE_TEXCOORD, n to specify texture
	//	coordinates in fixed function vertex processing and in pixel shaders prior
	//	to ps_3_0. These can be used to pass user defined data.
	//	
	//	D3DDECLUSAGE_TANGENT
	//	Vertex tangent data.
	//	
	//	D3DDECLUSAGE_BINORMAL
	//	Vertex binormal data.
	//	
	//	D3DDECLUSAGE_TESSFACTOR
	//	Single positive floating point value. Use D3DDECLUSAGE_TESSFACTOR with a
	//	usage index of 0 to specify a tessellation factor used in the tessellation
	//	unit to control the rate of tessellation. For more information about the
	//	data type, see D3DDECLTYPE_FLOAT1.
	//	
	//	D3DDECLUSAGE_POSITIONT
	//	Vertex data contains transformed position data ranging from (0,0) to
	//	(viewport width, viewport height). Use D3DDECLUSAGE_POSITIONT with a usage
	//	index of 0 to specify transformed position. When a declaration containing
	//	this is set, the pipeline does not perform vertex processing.
	//	
	//	D3DDECLUSAGE_COLOR
	//	Vertex data contains diffuse or specular color. Use D3DDECLUSAGE_COLOR with
	//	a usage index of 0 to specify the diffuse color in the fixed function
	//	vertex shader and pixel shaders prior to ps_3_0. Use D3DDECLUSAGE_COLOR
	//	with a usage index of 1 to specify the specular color in the fixed function
	//	vertex shader and pixel shaders prior to ps_3_0.
	//	
	//	D3DDECLUSAGE_FOG
	//	Vertex data contains fog data. Use D3DDECLUSAGE_FOG with a usage index of 0
	//	to specify a fog blend value used after pixel shading finishes. This
	//	applies to pixel shaders prior to version ps_3_0.
	//	
	//	D3DDECLUSAGE_DEPTH
	//	Vertex data contains depth data.
	//	
	//	D3DDECLUSAGE_SAMPLE
	//	Vertex data contains sampler data. Use D3DDECLUSAGE_SAMPLE with a usage
	//	index of 0 to specify the displacement value to look up. It can be used
	//	only with D3DDECLUSAGE_LOOKUPPRESAMPLED or D3DDECLUSAGE_LOOKUP.

	//note the form of the list terminator..

	//	#define D3DDECL_END() {0xFF,0,D3DDECLTYPE_UNUSED,0,0,0}
	//	typedef struct _D3DVERTEXELEMENT9
	//	{
	//		WORD    Stream;     // Stream index
	//		WORD    Offset;     // Offset in the stream in bytes
	//		BYTE    Type;       // Data type
	//		BYTE    Method;     // Processing method
	//		BYTE    Usage;      // Semantics
	//		BYTE    UsageIndex; // Semantic index
	//	} D3DVERTEXELEMENT9, *LPD3DVERTEXELEMENT9;

#define MAX_D3DVERTEXELEMENTS	16

struct TOGL_CLASS IDirect3DVertexDeclaration9 : public IUnknown
{
//public:
	uint					m_elemCount;
	D3DVERTEXELEMENT9_GL	m_elements[ MAX_D3DVERTEXELEMENTS ];

	virtual					~IDirect3DVertexDeclaration9();
};

struct TOGL_CLASS IDirect3DQuery9 : public IDirect3DResource9	//was IUnknown
{
//public:
	D3DQUERYTYPE			m_type;		// D3DQUERYTYPE_OCCLUSION or D3DQUERYTYPE_EVENT
	GLMContext				*m_ctx;
	CGLMQuery				*m_query;
	
	virtual					~IDirect3DQuery9();

    HRESULT					Issue(DWORD dwIssueFlags);
    HRESULT					GetData(void* pData,DWORD dwSize,DWORD dwGetDataFlags);
};

struct TOGL_CLASS IDirect3DVertexBuffer9 : public IDirect3DResource9	//was IUnknown
{
//public:
	GLMContext				*m_ctx;
	CGLMBuffer				*m_vtxBuffer;
	D3DVERTEXBUFFER_DESC	m_vtxDesc;		// to satisfy GetDesc

	virtual					~IDirect3DVertexBuffer9();
    HRESULT					Lock(UINT OffsetToLock,UINT SizeToLock,void** ppbData,DWORD Flags);
    HRESULT					Unlock();
	HRESULT					UnlockActualSize( uint nActualSize, const void *pActualData = NULL );

};

struct TOGL_CLASS IDirect3DIndexBuffer9 : public IDirect3DResource9	//was IUnknown
{
//public:
	GLMContext				*m_ctx;
	CGLMBuffer				*m_idxBuffer;
	D3DINDEXBUFFER_DESC		m_idxDesc;		// to satisfy GetDesc
	
	virtual					~IDirect3DIndexBuffer9();

    HRESULT					Lock(UINT OffsetToLock,UINT SizeToLock,void** ppbData,DWORD Flags);
    HRESULT					Unlock();
	HRESULT					UnlockActualSize( uint nActualSize, const void *pActualData = NULL );
    HRESULT					GetDesc(D3DINDEXBUFFER_DESC *pDesc);
};

struct TOGL_CLASS IDirect3DPixelShader9 : public IDirect3DResource9	//was IUnknown
{
//public:
	CGLMProgram				*m_pixProgram;
	uint					m_pixHighWater;		// count of active constant slots referenced by shader.
	uint					m_pixSamplerMask;	// (1<<n) mask of samplers referemnced by this pixel shader
												// this can help FlushSamplers avoid SRGB flipping on textures not being referenced...

	virtual					~IDirect3DPixelShader9();
};

struct TOGL_CLASS IDirect3DVertexShader9 : public IDirect3DResource9	//was IUnknown
{
//public:
	CGLMProgram				*m_vtxProgram;
	uint					m_vtxHighWater;		// count of active constant slots referenced by shader.
	unsigned char			m_vtxAttribMap[16];	// high nibble is usage, low nibble is usageindex, array position is attrib number

	virtual					~IDirect3DVertexShader9();

};

struct TOGL_CLASS ID3DXMatrixStack : public IUnknown
{
//public:
	CUtlVector<D3DMATRIX>	m_stack;
	int						m_stackTop;	// top of stack is at the highest index, this is that index.  push increases, pop decreases.
	
	HRESULT	Create( void );
	
    D3DXMATRIX* GetTop();
	void Push();
	void Pop();
	void LoadIdentity();
	void LoadMatrix( const D3DXMATRIX *pMat );
	void MultMatrix( const D3DXMATRIX *pMat );
	void MultMatrixLocal( const D3DXMATRIX *pMat );
    HRESULT ScaleLocal(FLOAT x, FLOAT y, FLOAT z);

	// Left multiply the current matrix with the computed rotation
    // matrix, counterclockwise about the given axis with the given angle.
    // (rotation is about the local origin of the object)
    HRESULT RotateAxisLocal(CONST D3DXVECTOR3* pV, FLOAT Angle);

	// Left multiply the current matrix with the computed translation
    // matrix. (transformation is about the local origin of the object)
    HRESULT TranslateLocal(FLOAT x, FLOAT y, FLOAT z);
};
typedef ID3DXMatrixStack* LPD3DXMATRIXSTACK;

struct TOGL_CLASS IDirect3DDevice9 : public IUnknown
{
public:
	// members
	
	IDirect3DDevice9Params	m_params;						// mirror of the creation inputs

	// D3D flavor stuff
	IDirect3DSurface9			*m_rtSurfaces[16];				// current color RT surfaces. [0] is initially == m_defaultColorSurface
	IDirect3DSurface9			*m_dsSurface;					// current DS RT surface. can be changed!
	
	IDirect3DSurface9			*m_defaultColorSurface;			// default color surface.
	IDirect3DSurface9			*m_defaultDepthStencilSurface;	// queried by GetDepthStencilSurface.
	
	IDirect3DVertexDeclaration9	*m_vertDecl;					// Set by SetVertexDeclaration...
	D3DStreamDesc				m_streams[ D3D_MAX_STREAMS ];	// Set by SetStreamSource..
	D3DIndexDesc				m_indices;						// Set by SetIndices..
	
	IDirect3DVertexShader9		*m_vertexShader;				// Set by SetVertexShader...
	IDirect3DPixelShader9		*m_pixelShader;					// Set by SetPixelShader...

	IDirect3DBaseTexture9		*m_textures[16];				// set by SetTexture... NULL if stage inactive
	D3DSamplerDesc				m_samplers[16];					// set by SetSamplerState..
	// GLM flavor stuff
	GLMContext					*m_ctx;
	CGLMFBO						*m_drawableFBO;					// this FBO should have all the attachments set to match m_rtSurfaces and m_dsSurface.
	
	// GL state 
	struct 
	{
		// render state buckets
		GLAlphaTestEnable_t			m_AlphaTestEnable;
		GLAlphaTestFunc_t			m_AlphaTestFunc;

		GLAlphaToCoverageEnable_t	m_AlphaToCoverageEnable;
		
		GLDepthTestEnable_t			m_DepthTestEnable;
		GLDepthMask_t				m_DepthMask;
		GLDepthFunc_t				m_DepthFunc;

		GLClipPlaneEnable_t			m_ClipPlaneEnable[kGLMUserClipPlanes];
		GLClipPlaneEquation_t		m_ClipPlaneEquation[kGLMUserClipPlanes];
		
		GLColorMaskSingle_t			m_ColorMaskSingle;
		GLColorMaskMultiple_t		m_ColorMaskMultiple;
		
		GLCullFaceEnable_t			m_CullFaceEnable;
		GLCullFrontFace_t			m_CullFrontFace;
		GLPolygonMode_t				m_PolygonMode;
		GLDepthBias_t				m_DepthBias;
		GLScissorEnable_t			m_ScissorEnable;
		GLScissorBox_t				m_ScissorBox;
		GLViewportBox_t				m_ViewportBox;
		GLViewportDepthRange_t		m_ViewportDepthRange;

		GLBlendEnable_t				m_BlendEnable;
		GLBlendFactor_t				m_BlendFactor;
		GLBlendEquation_t			m_BlendEquation;
		GLBlendColor_t				m_BlendColor;
		GLBlendEnableSRGB_t			m_BlendEnableSRGB;

		GLStencilTestEnable_t		m_StencilTestEnable;
		GLStencilFunc_t				m_StencilFunc;
		GLStencilOp_t				m_StencilOp;
		GLStencilWriteMask_t		m_StencilWriteMask;

		GLClearColor_t				m_ClearColor;
		GLClearDepth_t				m_ClearDepth;
		GLClearStencil_t			m_ClearStencil;
		
		bool						m_FogEnable;			// not really pushed to GL, just latched here
		
		// samplers
		GLMTexSamplingParams		m_samplers[ 16 ];
		
		// bindings...hmmm...		

		// dirty-bits
		uint						m_stateDirtyMask;		// covers the state blocks, indexed by 1<<n, n = EGLMStateBlockType
		uint						m_samplerDirtyMask;		// covers the samplers, indexed 1<<n, n = sampler index
	}	gl;
	
	// methods

public:
	virtual					~IDirect3DDevice9();
	
	// Create call invoked from IDirect3D9
	HRESULT	Create( IDirect3DDevice9Params *params );
	
	//
	// Basics
	//
	HRESULT Reset(D3DPRESENT_PARAMETERS* pPresentationParameters);
	HRESULT SetViewport(CONST D3DVIEWPORT9* pViewport);
	HRESULT GetViewport(D3DVIEWPORT9* pViewport);
    HRESULT BeginScene();
	HRESULT Clear(DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil);
    HRESULT EndScene();
    HRESULT Present(CONST RECT* pSourceRect,CONST RECT* pDestRect,VD3DHWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);

	// textures
	HRESULT CreateTexture(UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,VD3DHANDLE* pSharedHandle, char *debugLabel=NULL);
    HRESULT CreateCubeTexture(UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,VD3DHANDLE* pSharedHandle, char *debugLabel=NULL);
    HRESULT CreateVolumeTexture(UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,VD3DHANDLE* pSharedHandle, char *debugLabel=NULL);
	
	HRESULT SetTexture(DWORD Stage,IDirect3DBaseTexture9* pTexture);
    HRESULT GetTexture(DWORD Stage,IDirect3DBaseTexture9** ppTexture);

	// render targets, color and depthstencil, surfaces, blit
    HRESULT CreateRenderTarget(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,VD3DHANDLE* pSharedHandle, char *debugLabel=NULL);
    HRESULT SetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget);
    HRESULT GetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget);

    HRESULT CreateOffscreenPlainSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,VD3DHANDLE* pSharedHandle);

    HRESULT CreateDepthStencilSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,VD3DHANDLE* pSharedHandle);
    HRESULT SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil);
    HRESULT GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface);

	HRESULT GetRenderTargetData(IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface);	// ? is anyone using this ?
    HRESULT GetFrontBufferData(UINT iSwapChain,IDirect3DSurface9* pDestSurface);
    HRESULT StretchRect(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter);

	// pixel shaders
    HRESULT CreatePixelShader(CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader, const char *pShaderName, char *debugLabel = NULL);
	HRESULT SetPixelShader(IDirect3DPixelShader9* pShader);
    HRESULT SetPixelShaderConstantF(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
    HRESULT SetPixelShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
    HRESULT SetPixelShaderConstantI(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);

	// vertex shaders
    HRESULT CreateVertexShader(CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader, const char *pShaderName, char *debugLabel = NULL);
    HRESULT SetVertexShader(IDirect3DVertexShader9* pShader);
    HRESULT SetVertexShaderConstantF(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
    HRESULT SetVertexShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
    HRESULT SetVertexShaderConstantI(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);

	// POSIX only - preheating for a specific vertex/pixel shader pair - trigger GLSL link inside GLM
	HRESULT LinkShaderPair( IDirect3DVertexShader9* vs, IDirect3DPixelShader9* ps );
	HRESULT QueryShaderPair( int index, GLMShaderPairInfo *infoOut );
	
	// vertex buffers
    HRESULT CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl);
	HRESULT SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl);

    HRESULT SetFVF(DWORD FVF);		// we might not be using these ?
	HRESULT GetFVF(DWORD* pFVF);

    HRESULT CreateVertexBuffer(UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,VD3DHANDLE* pSharedHandle);
    HRESULT SetStreamSource(UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride);

	// index buffers
    HRESULT CreateIndexBuffer(UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,VD3DHANDLE* pSharedHandle);
    HRESULT SetIndices(IDirect3DIndexBuffer9* pIndexData);

	// response to retired objects (when refcount goes to zero and they self-delete..)
	void	ReleasedTexture			( IDirect3DBaseTexture9 *baseTex );			// called from texture destructor - need to scrub samplers	
	void	ReleasedSurface			( IDirect3DSurface9 *surface );				// called from any surface destructor - need to scrub RT table if an RT
	void	ReleasedPixelShader		( IDirect3DPixelShader9 *pixelShader );		// called from IDirect3DPixelShader9 destructor
	void	ReleasedVertexShader	( IDirect3DVertexShader9 *vertexShader );	// called from IDirect3DVertexShader9 destructor
	void	ReleasedVertexBuffer	( IDirect3DVertexBuffer9 *vertexBuffer );	// called from IDirect3DVertexBuffer9 destructor
	void	ReleasedIndexBuffer		( IDirect3DIndexBuffer9 *indexBuffer );		// called from IDirect3DIndexBuffer9 destructor
	void	ReleasedQuery			( IDirect3DQuery9 *query );					// called from IDirect3DQuery9 destructor

	// State management.
	HRESULT SetRenderStateInline(D3DRENDERSTATETYPE State,DWORD Value);
	HRESULT SetRenderStateConstInline(D3DRENDERSTATETYPE State,DWORD Value);
	HRESULT SetRenderState(D3DRENDERSTATETYPE State,DWORD Value);
	HRESULT SetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value);


	// Flushing changes to GL
	HRESULT FlushStates( uint mask );
	HRESULT FlushSamplers( uint mask );		// push SetRenderState and SetSamplerState changes
	HRESULT FlushIndexBindings( void );		// push index buffer (set index ptr)
	HRESULT	FlushVertexBindings( uint baseVertexIndex );	// push vertex streams (set attrib ptrs)
	HRESULT FlushGLM( void );
	
	// Draw.
    HRESULT DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
    HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount);
	HRESULT DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);

	// misc
    BOOL ShowCursor(BOOL bShow);
    HRESULT ValidateDevice(DWORD* pNumPasses);
    HRESULT SetMaterial(CONST D3DMATERIAL9* pMaterial);
    HRESULT LightEnable(DWORD Index,BOOL Enable);
    HRESULT SetScissorRect(CONST RECT* pRect);
	HRESULT CreateQuery(D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery);
    HRESULT GetDeviceCaps(D3DCAPS9* pCaps);
    HRESULT TestCooperativeLevel();
    HRESULT EvictManagedResources();
    HRESULT SetLight(DWORD Index,CONST D3DLIGHT9*);
    void SetGammaRamp(UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp);

	void SaveGLState();
	void RestoreGLState();

	// Talk to JasonM about this one. It's tricky in GL.
    HRESULT SetClipPlane(DWORD Index,CONST float* pPlane);

	//
	//
	// **** FIXED FUNCTION STUFF - None of this stuff needs support in GL.
	//
	//
    HRESULT SetTransform(D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix);
    HRESULT SetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value);	

	void AcquireThreadOwnership( );
	void ReleaseThreadOwnership( );
	inline DWORD GetCurrentOwnerThreadId() const { return m_ctx->m_nCurOwnerThreadId; }

};

struct ID3DXInclude
{
    virtual HRESULT Open(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) = 0;
    virtual HRESULT Close(LPCVOID pData) = 0;
};
typedef ID3DXInclude* LPD3DXINCLUDE;


struct TOGL_CLASS ID3DXBuffer : public IUnknown
{
    void* GetBufferPointer();
    DWORD GetBufferSize();
};

typedef ID3DXBuffer* LPD3DXBUFFER;

class TOGL_CLASS ID3DXConstantTable : public IUnknown
{
};
typedef ID3DXConstantTable* LPD3DXCONSTANTTABLE;



// ------------------------------------------------------------------------------------------------------------------------------ //
// D3DX stuff.
// ------------------------------------------------------------------------------------------------------------------------------ //

TOGL_INTERFACE const char* D3DXGetPixelShaderProfile( IDirect3DDevice9 *pDevice );


TOGL_INTERFACE D3DXMATRIX* D3DXMatrixMultiply( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM1, CONST D3DXMATRIX *pM2 );
TOGL_INTERFACE D3DXVECTOR3* D3DXVec3TransformCoord( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DXMATRIX *pM );

TOGL_INTERFACE  HRESULT D3DXCreateMatrixStack( DWORD Flags, LPD3DXMATRIXSTACK* ppStack);
TOGL_INTERFACE void D3DXMatrixIdentity( D3DXMATRIX * );

TOGL_INTERFACE D3DXINLINE D3DXVECTOR3* D3DXVec3Subtract( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
    pOut->x = pV1->x - pV2->x;
    pOut->y = pV1->y - pV2->y;
    pOut->z = pV1->z - pV2->z;
    return pOut;
}

TOGL_INTERFACE D3DXINLINE D3DXVECTOR3* D3DXVec3Cross( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
    D3DXVECTOR3 v;

    v.x = pV1->y * pV2->z - pV1->z * pV2->y;
    v.y = pV1->z * pV2->x - pV1->x * pV2->z;
    v.z = pV1->x * pV2->y - pV1->y * pV2->x;

    *pOut = v;
    return pOut;
}

TOGL_INTERFACE D3DXINLINE FLOAT D3DXVec3Dot( CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
    return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z;
}

TOGL_INTERFACE D3DXMATRIX* D3DXMatrixInverse( D3DXMATRIX *pOut, FLOAT *pDeterminant, CONST D3DXMATRIX *pM );

TOGL_INTERFACE D3DXMATRIX* D3DXMatrixTranspose( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM );

TOGL_INTERFACE D3DXPLANE* D3DXPlaneNormalize( D3DXPLANE *pOut, CONST D3DXPLANE *pP);

TOGL_INTERFACE D3DXVECTOR4* D3DXVec4Transform( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV, CONST D3DXMATRIX *pM );


TOGL_INTERFACE D3DXVECTOR4* D3DXVec4Normalize( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV );

TOGL_INTERFACE D3DXMATRIX* D3DXMatrixTranslation( D3DXMATRIX *pOut, FLOAT x, FLOAT y, FLOAT z );

// Build an ortho projection matrix. (right-handed)
TOGL_INTERFACE D3DXMATRIX* D3DXMatrixOrthoOffCenterRH( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn,FLOAT zf );

TOGL_INTERFACE D3DXMATRIX* D3DXMatrixPerspectiveRH( D3DXMATRIX *pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf );

TOGL_INTERFACE D3DXMATRIX* D3DXMatrixPerspectiveOffCenterRH( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf );

// Transform a plane by a matrix.  The vector (a,b,c) must be normal.
// M should be the inverse transpose of the transformation desired.
TOGL_INTERFACE D3DXPLANE* D3DXPlaneTransform( D3DXPLANE *pOut, CONST D3DXPLANE *pP, CONST D3DXMATRIX *pM );

TOGL_INTERFACE IDirect3D9 *Direct3DCreate9(UINT SDKVersion);

TOGL_INTERFACE void D3DPERF_SetOptions( DWORD dwOptions );

TOGL_INTERFACE HRESULT D3DXCompileShader(
        LPCSTR                          pSrcData,
        UINT                            SrcDataLen,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        LPCSTR                          pFunctionName,
        LPCSTR                          pProfile,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs,
        LPD3DXCONSTANTTABLE*            ppConstantTable);


#endif // USE_ACTUAL_DX

// fake D3D usage constant for SRGB tex creation
#define D3DUSAGE_TEXTURE_SRGB (0x80000000L)

#endif // DXABSTRACT_H
