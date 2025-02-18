//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Various material proxies for TF
//
//=====================================================================================//

#include "cbase.h"

#include "filesystem.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "pixelwriter.h"
#include "toolframework_client.h"
#include "econ_gcmessages.h"
#include "tf_item_inventory.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

// @note Currently not used since we cache the image on disk as a VTF
class CRGBAImageTextureRegenerator : public ITextureRegenerator
{
public:
	CRGBAImageTextureRegenerator() {}
	virtual ~CRGBAImageTextureRegenerator() {}

	// @return true if the image retrieval was successful, false otherwise
	virtual bool GetRGBAImage( uint8 *pubDest, int nDestBufferSize ) = 0;

	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pRect )
	{
		const int width = 64;
		const int height = 64;

		// if the width or height mismatches, we abort
		int nWidth = pVTFTexture->Width();
		int nHeight = pVTFTexture->Height();
		if ( nWidth != width || nHeight != height )
		{
			return;
		}

		// retrieve the image
		int destBufferSize = width * height * 4;
		byte *rgbaAvatarImage = (byte*)malloc( destBufferSize );
		if ( GetRGBAImage( rgbaAvatarImage, destBufferSize ) )
		{
			// Set up the pixel writer to write into the VTF texture
			CPixelWriter pixelWriter;		
			pixelWriter.SetPixelMemory( pVTFTexture->Format(), pVTFTexture->ImageData(), pVTFTexture->RowSizeInBytes( 0 ) );

			// only write the pixels requested
			for (int y = pRect->y; y < pRect->height; ++y)
			{
				pixelWriter.Seek( pRect->x, y );
				for (int x = pRect->x; x < pRect->width; ++x)
				{
					int idx = ( y * width + x ) * 4;
					byte *rgba = &rgbaAvatarImage[idx];
					byte r = rgba[0];
					byte g = rgba[1];
					byte b = rgba[2];
					byte a = rgba[3];
					pixelWriter.WritePixel( r, g, b, a );
				}
			}
		}

		// cleanup after ourselves
		free( rgbaAvatarImage );
	}

	virtual void Release()
	{
	}
};

//-----------------------------------------------------------------------------

/* @note Tom Bui: Here for reference
class CPlayerSteamAvatarTextureRegenerator : public CRGBAImageTextureRegenerator
{
public:
	CPlayerSteamAvatarTextureRegenerator( const CSteamID &steamID ) 
		: CRGBAImageTextureRegenerator()
		, m_steamID(steamID) 
		{}
	virtual ~CPlayerSteamAvatarTextureRegenerator() {}

	virtual bool GetRGBAImage( uint8 *pubDest, int nDestBufferSize )
	{
		int iAvatar = steamapicontext->SteamFriends()->GetFriendAvatar( m_steamID, k_EAvatarSize64x64 );
		return steamapicontext->SteamUtils()->GetImageRGBA( iAvatar, pubDest, nDestBufferSize );
	}

private:
	CSteamID m_steamID;
};
*/
