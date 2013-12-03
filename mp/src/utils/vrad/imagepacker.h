//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef IMAGEPACKER_H
#define IMAGEPACKER_H

#ifdef _WIN32
#pragma once
#endif

#define MAX_MAX_LIGHTMAP_WIDTH 2048


//-----------------------------------------------------------------------------
// This packs a single lightmap
//-----------------------------------------------------------------------------
class CImagePacker
{
public:
	bool Reset( int maxLightmapWidth, int maxLightmapHeight );
	bool AddBlock( int width, int height, int *returnX, int *returnY );

protected:
	int GetMaxYIndex( int firstX, int width );

	int m_MaxLightmapWidth;
	int m_MaxLightmapHeight;
	int m_pLightmapWavefront[MAX_MAX_LIGHTMAP_WIDTH];
	int m_AreaUsed;
	int m_MinimumHeight;

	// For optimization purposes:
	// These store the width + height of the first image
	// that was unable to be stored in this image
	int m_MaxBlockWidth;
	int m_MaxBlockHeight;
};


#endif // IMAGEPACKER_H
