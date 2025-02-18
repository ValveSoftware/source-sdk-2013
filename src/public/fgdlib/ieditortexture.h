//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines the interface a given texture for the 3D renderer. Current
//			implementations are for world textures (WADTexture.cpp) and sprite
//			textures (Texture.cpp).
//
//=============================================================================

#ifndef IEDITORTEXTURE_H
#define IEDITORTEXTURE_H

#ifdef _WIN32
#pragma once
#endif


#include <utlvector.h>


class CDC;
class CPalette;
class IMaterial;

// FGDLIB:
#define DEFAULT_TEXTURE_SCALE			0.25
#define DEFAULT_LIGHTMAP_SCALE			16


//
// Set your texture ID to this in your implementation's constructor.
//
#define TEXTURE_ID_NONE	-1


//
// Texture formats. hack: MUST correlate with radio buttons in IDD_OPTIONS_CONFIGS.
//
enum TEXTUREFORMAT
{
	tfNone = -1,
	tfWAD = 0,
	tfWAL = 1,
	tfWAD3 = 2,
	tfWAD4 = 3,
	tfWAD5 = 4,
	tfVMT = 5,
	tfSprite = 6	// dvs: not sure if I want to do it this way
};


//
// Flags for DrawTexData_t.
//
#define drawCaption			0x01
#define	drawResizeAlways	0x02
#define	drawIcons			0x04
#define	drawErrors			0x08
#define	drawUsageCount		0x10


struct DrawTexData_t
{
	int nFlags;
	int nUsageCount;
};


class IEditorTexture
{
	public:

		virtual ~IEditorTexture(void)
		{
		}

		//
		// dvs: remove one of these
		//
		virtual int GetImageWidth( void ) const = 0;
		virtual int GetImageHeight( void ) const = 0;

		virtual int GetWidth( void ) const = 0;
		virtual int GetHeight( void ) const = 0;

		virtual float GetDecalScale( void ) const = 0;

		//
		// dvs: Try to remove as many of these as possible:
		//
		virtual const char *GetName( void ) const = 0;
		virtual int GetShortName( char *szShortName ) const = 0;
		virtual int GetKeywords( char *szKeywords ) const = 0;
		// FGDLIB:
		//virtual void Draw(CDC *pDC, RECT &rect, int iFontHeight, int iIconHeight, DrawTexData_t &DrawTexData) = 0;
		virtual TEXTUREFORMAT GetTextureFormat( void ) const = 0;
		virtual int GetSurfaceAttributes( void ) const = 0;
		virtual int GetSurfaceContents(void ) const = 0;
		virtual int GetSurfaceValue( void ) const = 0;
		virtual CPalette *GetPalette( void ) const = 0;
		virtual bool HasData( void ) const = 0;
		virtual bool HasPalette( void ) const = 0;
		virtual bool Load( void ) = 0; // ensure that texture is loaded. could this be done internally?
		virtual void Reload( void ) = 0; // The texture changed
		virtual bool IsLoaded( void ) const = 0;
		virtual const char *GetFileName( void ) const = 0;

		virtual bool IsWater( void ) const = 0;

		//-----------------------------------------------------------------------------
		// Purpose: 
		// Input  : pData - 
		// Output : 
		//-----------------------------------------------------------------------------
		virtual int GetImageDataRGB( void *pData = NULL ) = 0;

		//-----------------------------------------------------------------------------
		// Purpose: 
		// Input  : pData - 
		// Output : 
		//-----------------------------------------------------------------------------
		virtual int GetImageDataRGBA( void *pData = NULL ) = 0;

		//-----------------------------------------------------------------------------
		// Purpose: Returns true if this texture has an alpha component, false if not.
		//-----------------------------------------------------------------------------
		virtual bool HasAlpha( void ) const = 0;

		//-----------------------------------------------------------------------------
		// Purpose: Returns whether this texture is a dummy texture or not. Dummy textures
		//			serve as placeholders for textures that were found in the map, but
		//			not in the WAD (or the materials tree). The dummy texture enables us
		//			to bind the texture, find it by name, etc.
		//-----------------------------------------------------------------------------
		virtual bool IsDummy( void ) const = 0; // dvs: perhaps not the best name?

		//-----------------------------------------------------------------------------
		// Purpose: Returns the unique texture ID for this texture object. The texture ID
		//			identifies the texture object across all renderers, and is assigned
		//			by the first renderer that actually binds the texture thru BindTexture.
		//
		//			Only the renderer ever needs to call SetTextureID.
		//-----------------------------------------------------------------------------
		virtual int GetTextureID( void ) const = 0;

		//-----------------------------------------------------------------------------
		// Purpose: Sets the unique texture ID for this texture object. The texture ID
		//			identifies the texture object across all renderers, and is assigned
		//			by the first renderer that actually binds the texture thru BindTexture.
		//
		//			Only the renderer should ever call SetTextureID!
		//-----------------------------------------------------------------------------
		virtual void SetTextureID( int nTextureID ) = 0;

		//-----------------------------------------------------------------------------
		// Returns the material system material associated with a texture
		//-----------------------------------------------------------------------------

		virtual IMaterial* GetMaterial() { return 0; }
};


typedef CUtlVector<IEditorTexture *> EditorTextureList_t;


#endif // IEDITORTEXTURE_H
