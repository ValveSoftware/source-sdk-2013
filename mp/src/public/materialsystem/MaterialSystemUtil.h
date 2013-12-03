//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//
#ifndef MATERIALSYSTEMUTIL_H
#define MATERIALSYSTEMUTIL_H

#ifdef _WIN32
#pragma once
#endif

#include "bitmap/imageformat.h" //ImageFormat enum definition
#include "materialsystem/imaterialsystem.h"  // RenderTargetSizeMode_t and MaterialRenderTargetDepth_t definition

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMaterial;
class ITexture;
class KeyValues;

class KeyValues;


//-----------------------------------------------------------------------------
// Little utility class to deal with material references
//-----------------------------------------------------------------------------
class CMaterialReference
{
public:
	// constructor, destructor
	CMaterialReference( char const* pMaterialName = 0, const char *pTextureGroupName = 0, bool bComplain = true );
	~CMaterialReference();

	// Attach to a material
	void Init( const char* pMaterialName, const char *pTextureGroupName, bool bComplain = true );
	void Init( const char *pMaterialName, KeyValues *pVMTKeyValues );
	void Init( IMaterial* pMaterial );
	void Init( CMaterialReference& ref );
	void Init( const char *pMaterialName, const char *pTextureGroupName, KeyValues *pVMTKeyValues );

	// Detach from a material
	void Shutdown();
	bool IsValid() { return m_pMaterial != 0; }

	// Automatic casts to IMaterial
	operator IMaterial*() { return m_pMaterial; }
	operator IMaterial*() const { return m_pMaterial; }
	operator IMaterial const*() const { return m_pMaterial; }
	IMaterial* operator->() { return m_pMaterial; }
	
private:
	IMaterial* m_pMaterial;
};

//-----------------------------------------------------------------------------
// Little utility class to deal with texture references
//-----------------------------------------------------------------------------
class CTextureReference
{
public:
	// constructor, destructor
	CTextureReference( );
	CTextureReference( const CTextureReference &ref );
	~CTextureReference();

	// Attach to a texture
	void Init( char const* pTexture, const char *pTextureGroupName, bool bComplain = true );
	void InitProceduralTexture( const char *pTextureName, const char *pTextureGroupName, int w, int h, ImageFormat fmt, int nFlags );
	void InitRenderTarget( int w, int h, RenderTargetSizeMode_t sizeMode, ImageFormat fmt, MaterialRenderTargetDepth_t depth, bool bHDR, char *pStrOptionalName = NULL );
#if defined( _X360 )
	// used when RT coupling is disparate (texture is DDR based, surface is EDRAM based)
	void InitRenderTargetTexture( int width, int height, RenderTargetSizeMode_t sizeMode, ImageFormat fmt, MaterialRenderTargetDepth_t depth, bool bHDR, char *pStrOptionalName = NULL );
	void InitRenderTargetSurface( int width, int height, ImageFormat fmt, bool bSameAsTexture );
#endif
	void Init( ITexture* pTexture );

	// Detach from a texture
	void Shutdown( bool bDeleteIfUnReferenced = false );
	bool IsValid() { return m_pTexture != 0; }

	// Automatic casts to ITexture
	operator ITexture*() { return m_pTexture; }
	operator ITexture const*() const { return m_pTexture; }
	ITexture* operator->() { return m_pTexture; }

	// Assignment operator
	void operator=( CTextureReference &ref );

private:
	ITexture* m_pTexture;
};


#endif // !MATERIALSYSTEMUTIL_H
