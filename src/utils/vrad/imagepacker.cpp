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

#include "vrad.h"
#include "imagepacker.h"


bool CImagePacker::Reset( int maxLightmapWidth, int maxLightmapHeight )
{
	int i;
	
	Assert( maxLightmapWidth <= MAX_MAX_LIGHTMAP_WIDTH );
	
	m_MaxLightmapWidth = maxLightmapWidth;
	m_MaxLightmapHeight = maxLightmapHeight;
	
	m_MaxBlockWidth = maxLightmapWidth + 1;
	m_MaxBlockHeight = maxLightmapHeight + 1;

	m_AreaUsed = 0;
	m_MinimumHeight = -1;
	for( i = 0; i < m_MaxLightmapWidth; i++ )
    {
		m_pLightmapWavefront[i] = -1;
    }
	return true;
}


inline int CImagePacker::GetMaxYIndex( int firstX, int width )
{
	int maxY = -1;
	int maxYIndex = 0;
	for( int x = firstX; x < firstX + width; ++x )
	{
		// NOTE: Want the equals here since we'll never be able to fit
		// in between the multiple instances of maxY
		if( m_pLightmapWavefront[x] >= maxY )
		{
			maxY = m_pLightmapWavefront[x];
			maxYIndex = x;
		}
	}
	return maxYIndex;
}


bool CImagePacker::AddBlock( int width, int height, int *returnX, int *returnY )
{
	// If we've already determined that a block this big couldn't fit
	// then blow off checking again...
	if ( ( width >= m_MaxBlockWidth ) && ( height >= m_MaxBlockHeight ) )
		return false;

	int bestX = -1;	
	int maxYIdx;
	int outerX = 0;
	int outerMinY = m_MaxLightmapHeight;
	int lastX = m_MaxLightmapWidth - width;
	int lastMaxYVal = -2;
	while (outerX <= lastX)
	{
		// Skip all tiles that have the last Y value, these
		// aren't going to change our min Y value
		if (m_pLightmapWavefront[outerX] == lastMaxYVal)
		{
			++outerX;
			continue;
		}

		maxYIdx = GetMaxYIndex( outerX, width );
		lastMaxYVal = m_pLightmapWavefront[maxYIdx];
		if (outerMinY > lastMaxYVal)
		{
			outerMinY = lastMaxYVal;
			bestX = outerX;
		}
		outerX = maxYIdx + 1;
	}
	
	if( bestX == -1 )
	{
		// If we failed to add it, remember the block size that failed
		// *only if both dimensions are smaller*!!
		// Just because a 1x10 block failed, doesn't mean a 10x1 block will fail
		if ( ( width <= m_MaxBlockWidth ) && ( height <= m_MaxBlockHeight )	)
		{
			m_MaxBlockWidth = width;
			m_MaxBlockHeight = height;
		}

		return false;
	}
	
	// Set the return positions for the block.
	*returnX = bestX;
	*returnY = outerMinY + 1;
	
	// Check if it actually fit height-wise.
	// hack
	//  if( *returnY + height > maxLightmapHeight )
	if( *returnY + height >= m_MaxLightmapHeight - 1 )
	{
		if ( ( width <= m_MaxBlockWidth ) && ( height <= m_MaxBlockHeight )	)
		{
			m_MaxBlockWidth = width;
			m_MaxBlockHeight = height;
		}

		return false;
	}
						   
	// It fit!
	// Keep up with the smallest possible size for the image so far.
	if( *returnY + height > m_MinimumHeight )
		m_MinimumHeight = *returnY + height;
	
	// Update the wavefront info.
	int x;
	for( x = bestX; x < bestX + width; x++ )
    {
		m_pLightmapWavefront[x] = outerMinY + height;
    }
	
	//  AddBlockToLightmapImage( *returnX, *returnY, width, height );
	m_AreaUsed += width * height;

	return true;
}

