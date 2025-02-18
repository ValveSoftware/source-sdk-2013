//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef ITEXTURE_H
#define ITEXTURE_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "bitmap/imageformat.h" // ImageFormat defn.

class IVTFTexture;
class ITexture;
struct Rect_t;

//-----------------------------------------------------------------------------
// This will get called on procedural textures to re-fill the textures
// with the appropriate bit pattern. Calling Download() will also
// cause this interface to be called. It will also be called upon
// mode switch, or on other occasions where the bits are discarded.
//-----------------------------------------------------------------------------
abstract_class ITextureRegenerator
{
public:
	// This will be called when the texture bits need to be regenerated.
	// Use the VTFTexture interface, which has been set up with the
	// appropriate texture size + format
	// The rect specifies which part of the texture needs to be updated
	// You can choose to update all of the bits if you prefer
	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pRect ) = 0;

	// This will be called when the regenerator needs to be deleted
	// which will happen when the texture is destroyed
	virtual void Release() = 0;

	// (erics): This should have a virtual destructor, but would be ABI breaking (non-versioned interface implemented
	//          by the game)
//	virtual ~ITextureRegenerator(){}
};

abstract_class ITexture
{
public:
	// Various texture polling methods
	virtual const char *GetName( void ) const = 0;
	virtual int GetMappingWidth() const = 0;
	virtual int GetMappingHeight() const = 0;
	virtual int GetActualWidth() const = 0;
	virtual int GetActualHeight() const = 0;
	virtual int GetNumAnimationFrames() const = 0;
	virtual bool IsTranslucent() const = 0;
	virtual bool IsMipmapped() const = 0;

	virtual void GetLowResColorSample( float s, float t, float *color ) const = 0;

	// Gets texture resource data of the specified type.
	// Params:
	//		eDataType		type of resource to retrieve.
	//		pnumBytes		on return is the number of bytes available in the read-only data buffer or is undefined
	// Returns:
	//		pointer to the resource data, or NULL
	virtual void *GetResourceData( uint32 eDataType, size_t *pNumBytes ) const = 0;

	// Methods associated with reference count
	virtual void IncrementReferenceCount( void ) = 0;
	virtual void DecrementReferenceCount( void ) = 0;

	inline void AddRef() { IncrementReferenceCount(); }
	inline void Release() { DecrementReferenceCount(); }

	// Used to modify the texture bits (procedural textures only)
	virtual void SetTextureRegenerator( ITextureRegenerator *pTextureRegen ) = 0;

	// Reconstruct the texture bits in HW memory

	// If rect is not specified, reconstruct all bits, otherwise just
	// reconstruct a subrect.
	virtual void Download( Rect_t *pRect = 0, int nAdditionalCreationFlags = 0 ) = 0;

	// Uses for stats. . .get the approximate size of the texture in it's current format.
	virtual int GetApproximateVidMemBytes( void ) const = 0;

	// Returns true if the texture data couldn't be loaded.
	virtual bool IsError() const = 0;

	// NOTE: Stuff after this is added after shipping HL2.

	// For volume textures
	virtual bool IsVolumeTexture() const = 0;
	virtual int GetMappingDepth() const = 0;
	virtual int GetActualDepth() const = 0;

	virtual ImageFormat GetImageFormat() const = 0;
	virtual NormalDecodeMode_t GetNormalDecodeMode() const = 0;

	// Various information about the texture
	virtual bool IsRenderTarget() const = 0;
	virtual bool IsCubeMap() const = 0;
	virtual bool IsNormalMap() const = 0;
	virtual bool IsProcedural() const = 0;

	virtual void DeleteIfUnreferenced() = 0;

#if defined( _X360 )
	virtual bool ClearTexture( int r, int g, int b, int a ) = 0;
	virtual bool CreateRenderTargetSurface( int width, int height, ImageFormat format, bool bSameAsTexture ) = 0;
#endif

	// swap everything except the name with another texture
	virtual void SwapContents( ITexture *pOther ) = 0;

	// Retrieve the vtf flags mask
	virtual unsigned int GetFlags( void ) const = 0;

	// Force LOD override (automatically downloads the texture)
	virtual void ForceLODOverride( int iNumLodsOverrideUpOrDown ) = 0;

	// Save texture to a file.
	virtual bool SaveToFile( const char *fileName ) = 0;

	// Copy this texture, which must be a render target or a renderable texture, to the destination texture, 
	// which must have been created with the STAGING bit.
	virtual void CopyToStagingTexture( ITexture* pDstTex ) = 0;

	// Set that this texture should return true for the call "IsError"
	virtual void SetErrorTexture( bool bIsErrorTexture ) = 0;
};


inline bool IsErrorTexture( ITexture *pTex )
{
	return !pTex || pTex->IsError();
}


#endif // ITEXTURE_H
