//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Hardware Texels
//
// Contains texture data that was encoded with the map. The initial use case
// is for per-texel lightmaps to allow static props to match the lighting
// of the surrounding BSP geometry.
// 
//=============================================================================//

#ifndef HARDWARETEXELS_H
#define HARDWARETEXELS_H

#ifdef _WIN32
#pragma once
#endif

#include "bitmap/imageformat.h"
#include "datamap.h"

// valve hardware texels
#define VHT_VERSION	0

namespace HardwareTexels
{
#pragma pack(1)

// ------------------------------------------------------------------------------------------------
struct MeshHeader_t
{
	DECLARE_BYTESWAP_DATADESC();

	// this mesh is part of this lod
	unsigned int m_nLod;

	// starting at this offset
	unsigned int m_nOffset;

	// this mesh consumes this many bytes
	unsigned int m_nBytes;

	// with this width and height
	unsigned int m_nWidth;
	unsigned int m_nHeight;

	unsigned int m_nUnused[3];
};

// ------------------------------------------------------------------------------------------------
struct FileHeader_t
{
	DECLARE_BYTESWAP_DATADESC();

	// file version as defined by VHV_VERSION
	int m_nVersion;

	// must match checkSum in the .mdl header
	unsigned int m_nChecksum;

	// all texels are encoded in the same format
	// This is a value from ImageFormat.
	unsigned int m_nTexelFormat;

	// Number of meshes
	int m_nMeshes;

	inline MeshHeader_t *pMesh( int nMesh ) const 
	{
		Assert(nMesh < m_nMeshes);

		return (MeshHeader_t *)(((byte *)this) + sizeof(FileHeader_t)) + nMesh;
	};

	inline void *pTexelBase( int nMesh ) const 
	{
		return (void *)((byte *)this + pMesh( nMesh )->m_nOffset);
	};

	unsigned int m_nUnused[4];
};

#pragma pack()

}; // end namespace

#endif // HARDWARETEXELS_H

