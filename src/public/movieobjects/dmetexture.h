//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing a procedural texture
//
//=============================================================================

#ifndef DMETEXTURE_H
#define DMETEXTURE_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "movieobjects/dmeimage.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ITexture;
class IMesh;
enum ImageFormat;
class IVTFTexture;


//-----------------------------------------------------------------------------
// Compression types
//-----------------------------------------------------------------------------
enum DmeTextureCompress_t
{
	DMETEXTURE_COMPRESS_DEFAULT = 0,
	DMETEXTURE_COMPRESS_NONE,
	DMETEXTURE_COMPRESS_DXT1,
	DMETEXTURE_COMPRESS_DXT5,
};


//-----------------------------------------------------------------------------
// Filter types
//-----------------------------------------------------------------------------
enum DmeTextureFilter_t
{
	DMETEXTURE_FILTER_DEFAULT = 0,
	DMETEXTURE_FILTER_ANISOTROPIC,
	DMETEXTURE_FILTER_TRILINEAR,
	DMETEXTURE_FILTER_BILINEAR,
	DMETEXTURE_FILTER_POINT,
};


//-----------------------------------------------------------------------------
// Mipmap types
//-----------------------------------------------------------------------------
enum DmeTextureMipmap_t
{
	DMETEXTURE_MIPMAP_DEFAULT = 0,
	DMETEXTURE_MIPMAP_ALL_LEVELS,
	DMETEXTURE_MIPMAP_NONE,
};


//-----------------------------------------------------------------------------
// A base class for textures
//-----------------------------------------------------------------------------
class CDmeBaseTexture : public CDmElement
{
	DEFINE_ELEMENT( CDmeBaseTexture, CDmElement );

public:
	ITexture *GetCachedTexture();

	// Compression type
	void SetCompressionType( DmeTextureCompress_t type );
	DmeTextureCompress_t GetCompressionType() const;

	// Filter type
	void SetFilterType( DmeTextureFilter_t type );
	DmeTextureFilter_t GetFilterType() const;

	// Mipmap type
	void SetMipmapType( DmeTextureMipmap_t type );
	DmeTextureMipmap_t GetMipmapType() const;

public:
	CDmAttributeVar<bool> m_bClampS;
	CDmAttributeVar<bool> m_bClampT;
	CDmAttributeVar<bool> m_bClampU;
	CDmAttributeVar<bool> m_bNoDebugOverride;
	CDmAttributeVar<bool> m_bNoLod;
	CDmAttributeVar<bool> m_bNiceFiltered;
	CDmAttributeVar<bool> m_bNormalMap;
	CDmAttributeVar<float> m_flBumpScale;

protected:
	// Computes texture flags
	int CalcTextureFlags( int nDepth ) const;

	// Computes the desired texture format based on flags
	ImageFormat ComputeDesiredImageFormat( ImageFormat srcFormat, int nWidth, int nHeight, int nDepth, int nFlags );

	CDmAttributeVar<int> m_nCompressType;
	CDmAttributeVar<int> m_nFilterType;
	CDmAttributeVar<int> m_nMipmapType;

	// Computed values
	CTextureReference m_Texture;
	IVTFTexture *m_pVTFTexture;
	Vector m_vecReflectivity;
};


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline void CDmeBaseTexture::SetCompressionType( DmeTextureCompress_t type )
{
	m_nCompressType = type;
}

inline DmeTextureCompress_t CDmeBaseTexture::GetCompressionType() const
{
	return (DmeTextureCompress_t)m_nCompressType.Get();
}

inline void CDmeBaseTexture::SetFilterType( DmeTextureFilter_t type )
{
	m_nFilterType = type;
}

inline DmeTextureFilter_t CDmeBaseTexture::GetFilterType() const
{
	return (DmeTextureFilter_t)m_nFilterType.Get();
}

inline void CDmeBaseTexture::SetMipmapType( DmeTextureMipmap_t type )
{
	m_nMipmapType = type;
}

inline DmeTextureMipmap_t CDmeBaseTexture::GetMipmapType() const
{
	return (DmeTextureMipmap_t)m_nMipmapType.Get();
}


//-----------------------------------------------------------------------------
// A class representing a texture
//-----------------------------------------------------------------------------
class CDmeTexture : public CDmeBaseTexture
{
	DEFINE_ELEMENT( CDmeTexture, CDmeBaseTexture );

public:
	virtual void Resolve();

private:
	// Array of images in an animated texture
	CDmAttributeVarElementArray< CDmeImage > m_Images;
};


//-----------------------------------------------------------------------------
// A class representing a cube texture
//-----------------------------------------------------------------------------
class CDmeCubeTexture : public CDmeBaseTexture
{
	DEFINE_ELEMENT( CDmeCubeTexture, CDmeBaseTexture );

public:
	virtual void Resolve();

private:
	// Array of images in an animated texture
	CDmAttributeVarElementArray< CDmeImage > m_ImagesPosX;
	CDmAttributeVarElementArray< CDmeImage > m_ImagesNegX;
	CDmAttributeVarElementArray< CDmeImage > m_ImagesPosY;
	CDmAttributeVarElementArray< CDmeImage > m_ImagesNegY;
	CDmAttributeVarElementArray< CDmeImage > m_ImagesPosZ;
	CDmAttributeVarElementArray< CDmeImage > m_ImagesNegZ;
};


#endif // DMETEXTURE_H
