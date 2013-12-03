//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef LOCALD3DTYPES_H
#define LOCALD3DTYPES_H

#ifdef _WIN32
#pragma once
#endif

#if defined( DX10 ) && !defined( DX_TO_GL_ABSTRACTION )

#include <d3d10.h>
#include <d3dx10.h>

struct IDirect3D10BaseTexture
{
	ID3D10Resource			 *m_pBaseTexture;
	ID3D10ShaderResourceView *m_pSRView;
	ID3D10RenderTargetView	 *m_pRTView;
};

class CDx10Types
{
public:
	typedef struct IDirect3D10BaseTexture	IDirect3DTexture;
	// FIXME: What is this called now ? 
	// typedef ID3D10TextureCube			IDirect3DCubeTexture;
	typedef ID3D10Texture3D					IDirect3DVolumeTexture;
	typedef ID3D10Device					IDirect3DDevice;
	typedef D3D10_VIEWPORT					D3DVIEWPORT;
	typedef ID3D10Buffer					IDirect3DIndexBuffer;
	typedef ID3D10Buffer					IDirect3DVertexBuffer;
	typedef ID3D10VertexShader				IDirect3DVertexShader;
	typedef ID3D10PixelShader				IDirect3DPixelShader;
	typedef ID3D10ShaderResourceView		IDirect3DSurface;
	typedef ID3DX10Font						ID3DXFont;
	typedef ID3D10Query						ID3DQuery;

	typedef ID3D10Device					*LPDIRECT3DDEVICE;
	typedef ID3D10Buffer					*LPDIRECT3DINDEXBUFFER;
	typedef ID3D10Buffer					*LPDIRECT3DVERTEXBUFFER;
};

#endif // defined( DX10 ) && !defined( DX_TO_GL_ABSTRACTION )


#if !defined( _X360 ) && !defined( DX_TO_GL_ABSTRACTION )
#ifdef _DEBUG
#define D3D_DEBUG_INFO 1
#endif
#endif

struct IDirect3DTexture9;
struct IDirect3DBaseTexture9;
struct IDirect3DCubeTexture9;
struct IDirect3D9;
struct IDirect3DDevice9;
struct IDirect3DSurface9;
struct IDirect3DIndexBuffer9;
struct IDirect3DVertexBuffer9;
struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;
struct IDirect3DVolumeTexture9;

typedef struct _D3DLIGHT9				D3DLIGHT9;
typedef struct _D3DADAPTER_IDENTIFIER9	D3DADAPTER_IDENTIFIER9;
typedef struct _D3DCAPS9				D3DCAPS9;
typedef struct _D3DVIEWPORT9			D3DVIEWPORT9;
typedef struct _D3DMATERIAL9			D3DMATERIAL9;
typedef IDirect3DTexture9				IDirect3DTexture;
typedef IDirect3DBaseTexture9			IDirect3DBaseTexture;
typedef IDirect3DCubeTexture9			IDirect3DCubeTexture;
typedef IDirect3DVolumeTexture9 		IDirect3DVolumeTexture;
typedef IDirect3DDevice9				IDirect3DDevice;
typedef D3DMATERIAL9					D3DMATERIAL;
typedef D3DLIGHT9						D3DLIGHT;
typedef IDirect3DSurface9				IDirect3DSurface;
typedef D3DCAPS9						D3DCAPS;
typedef IDirect3DIndexBuffer9			IDirect3DIndexBuffer;
typedef IDirect3DVertexBuffer9			IDirect3DVertexBuffer;
typedef IDirect3DPixelShader9			IDirect3DPixelShader;
typedef IDirect3DDevice					*LPDIRECT3DDEVICE;
typedef IDirect3DIndexBuffer			*LPDIRECT3DINDEXBUFFER;
typedef IDirect3DVertexBuffer			*LPDIRECT3DVERTEXBUFFER;

class CDx9Types
{
public:
	typedef IDirect3DTexture9				IDirect3DTexture;
	typedef IDirect3DBaseTexture9			IDirect3DBaseTexture;
	typedef IDirect3DCubeTexture9			IDirect3DCubeTexture;
	typedef IDirect3DVolumeTexture9 		IDirect3DVolumeTexture;
	typedef IDirect3DDevice9				IDirect3DDevice;
	typedef D3DMATERIAL9					D3DMATERIAL;
	typedef D3DLIGHT9						D3DLIGHT;
	typedef IDirect3DSurface9				IDirect3DSurface;
	typedef D3DCAPS9						D3DCAPS;
	typedef IDirect3DIndexBuffer9			IDirect3DIndexBuffer;
	typedef IDirect3DVertexBuffer9			IDirect3DVertexBuffer;
	typedef IDirect3DPixelShader9			IDirect3DPixelShader;
	typedef IDirect3DDevice					*LPDIRECT3DDEVICE;
	typedef IDirect3DIndexBuffer			*LPDIRECT3DINDEXBUFFER;
	typedef IDirect3DVertexBuffer			*LPDIRECT3DVERTEXBUFFER;
};

typedef void *HardwareShader_t;

//-----------------------------------------------------------------------------
// The vertex and pixel shader type
//-----------------------------------------------------------------------------
typedef int VertexShader_t;
typedef int PixelShader_t;	

//-----------------------------------------------------------------------------
// Bitpattern for an invalid shader
//-----------------------------------------------------------------------------
#define INVALID_SHADER	( 0xFFFFFFFF )
#define INVALID_HARDWARE_SHADER ( NULL )

#define D3DSAMP_NOTSUPPORTED					D3DSAMP_FORCE_DWORD
#define D3DRS_NOTSUPPORTED						D3DRS_FORCE_DWORD

#include "togl/rendermechanism.h"

#if defined( _X360 )

// not supported, keeping for port ease
#define D3DSAMP_SRGBTEXTURE						D3DSAMP_NOTSUPPORTED
#define D3DRS_LIGHTING							D3DRS_NOTSUPPORTED
#define D3DRS_DIFFUSEMATERIALSOURCE				D3DRS_NOTSUPPORTED
#define D3DRS_SPECULARENABLE					D3DRS_NOTSUPPORTED
#define D3DRS_SHADEMODE							D3DRS_NOTSUPPORTED
#define D3DRS_LASTPIXEL							D3DRS_NOTSUPPORTED
#define D3DRS_DITHERENABLE						D3DRS_NOTSUPPORTED
#define D3DRS_FOGENABLE							D3DRS_NOTSUPPORTED
#define D3DRS_FOGCOLOR							D3DRS_NOTSUPPORTED
#define D3DRS_FOGTABLEMODE						D3DRS_NOTSUPPORTED
#define D3DRS_FOGSTART							D3DRS_NOTSUPPORTED
#define D3DRS_FOGEND							D3DRS_NOTSUPPORTED
#define D3DRS_FOGDENSITY						D3DRS_NOTSUPPORTED
#define D3DRS_RANGEFOGENABLE					D3DRS_NOTSUPPORTED
#define D3DRS_TEXTUREFACTOR						D3DRS_NOTSUPPORTED
#define D3DRS_CLIPPING							D3DRS_NOTSUPPORTED
#define D3DRS_AMBIENT							D3DRS_NOTSUPPORTED
#define D3DRS_FOGVERTEXMODE						D3DRS_NOTSUPPORTED
#define D3DRS_COLORVERTEX						D3DRS_NOTSUPPORTED
#define D3DRS_LOCALVIEWER						D3DRS_NOTSUPPORTED
#define D3DRS_NORMALIZENORMALS					D3DRS_NOTSUPPORTED
#define D3DRS_SPECULARMATERIALSOURCE			D3DRS_NOTSUPPORTED
#define D3DRS_AMBIENTMATERIALSOURCE				D3DRS_NOTSUPPORTED
#define D3DRS_EMISSIVEMATERIALSOURCE			D3DRS_NOTSUPPORTED	
#define D3DRS_VERTEXBLEND						D3DRS_NOTSUPPORTED
#define D3DRS_POINTSCALEENABLE					D3DRS_NOTSUPPORTED
#define D3DRS_POINTSCALE_A						D3DRS_NOTSUPPORTED
#define D3DRS_POINTSCALE_B						D3DRS_NOTSUPPORTED
#define D3DRS_POINTSCALE_C						D3DRS_NOTSUPPORTED
#define D3DRS_PATCHEDGESTYLE					D3DRS_NOTSUPPORTED
#define D3DRS_DEBUGMONITORTOKEN					D3DRS_NOTSUPPORTED
#define D3DRS_INDEXEDVERTEXBLENDENABLE			D3DRS_NOTSUPPORTED
#define D3DRS_TWEENFACTOR						D3DRS_NOTSUPPORTED
#define D3DRS_POSITIONDEGREE					D3DRS_NOTSUPPORTED
#define D3DRS_NORMALDEGREE						D3DRS_NOTSUPPORTED
#define D3DRS_ANTIALIASEDLINEENABLE				D3DRS_NOTSUPPORTED
#define D3DRS_ADAPTIVETESS_X					D3DRS_NOTSUPPORTED
#define D3DRS_ADAPTIVETESS_Y					D3DRS_NOTSUPPORTED
#define D3DRS_ADAPTIVETESS_Z					D3DRS_NOTSUPPORTED
#define D3DRS_ADAPTIVETESS_W					D3DRS_NOTSUPPORTED
#define D3DRS_ENABLEADAPTIVETESSELLATION		D3DRS_NOTSUPPORTED
#define D3DRS_SRGBWRITEENABLE					D3DRS_NOTSUPPORTED
#define D3DLOCK_DISCARD							0
#define D3DUSAGE_DYNAMIC						0
#define D3DUSAGE_AUTOGENMIPMAP					0
#define D3DDEVTYPE_REF							D3DDEVTYPE_HAL
#define D3DENUM_WHQL_LEVEL						0
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING		D3DCREATE_HARDWARE_VERTEXPROCESSING
#define D3DDMT_ENABLE							0

typedef enum D3DSHADEMODE
{
	D3DSHADE_FLAT = 0,
	D3DSHADE_GOURAUD = 0,
};

#endif // _X360

#endif // LOCALD3DTYPES_H
