//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef IDEBUGTEXTUREINFO_H
#define IDEBUGTEXTUREINFO_H
#ifdef _WIN32
#pragma once
#endif


class KeyValues;


// This interface is actually exported by the shader API DLL.
#define DEBUG_TEXTURE_INFO_VERSION "DebugTextureInfo001"


abstract_class IDebugTextureInfo
{
public:
	
	// Use this to turn on the mode where it builds the debug texture list.
	// At the end of the next frame, GetDebugTextureList() will return a valid list of the textures.
	virtual void EnableDebugTextureList( bool bEnable ) = 0;
	
	// If this is on, then it will return all textures that exist, not just the ones that were bound in the last frame.
	// It is required to enable debug texture list to get this.
	virtual void EnableGetAllTextures( bool bEnable ) = 0;

	// Use this to get the results of the texture list.
	// Do NOT release the KeyValues after using them.
	// There will be a bunch of subkeys, each with these values:
	//    Name   - the texture's filename
	//    Binds  - how many times the texture was bound
	//    Format - ImageFormat of the texture
	//    Width  - Width of the texture
	//    Height - Height of the texture
	// It is required to enable debug texture list to get this.
	virtual KeyValues* GetDebugTextureList() = 0;

	// Texture memory usage
	enum TextureMemoryType
	{
		MEMORY_RESERVED_MIN = 0,
		MEMORY_BOUND_LAST_FRAME,		// sums up textures bound last frame
		MEMORY_TOTAL_LOADED,			// total texture memory used
		MEMORY_ESTIMATE_PICMIP_1,		// estimate of running with "picmip 1"
		MEMORY_ESTIMATE_PICMIP_2,		// estimate of running with "picmip 2"
		MEMORY_RESERVED_MAX
	};

	// This returns how much memory was used.
	virtual int GetTextureMemoryUsed( TextureMemoryType eTextureMemory ) = 0;

	// Use this to determine if texture debug info was computed within last numFramesAllowed frames.
	virtual bool IsDebugTextureListFresh( int numFramesAllowed = 1 ) = 0;

	// Enable debug texture rendering when texture binds should not count towards textures
	// used during a frame. Returns the old state of debug texture rendering flag to use
	// it for restoring the mode.
	virtual bool SetDebugTextureRendering( bool bEnable ) = 0;

};


#endif // IDEBUGTEXTUREINFO_H
