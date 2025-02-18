//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Hardware Verts
//
// Contains data purposely formatted for a dma copy into a D3D Vertex Buffer.
// The file is divided into two partitions, the foremost contains the static
// portion (header), the latter contains the streamable compliant portion.
// The streamable component starts and ends on a sector (512) aligned boundary.
// The header identifies the vertex format of the data and the atomic sizes of each component.
// The hierarchial mesh is flattened for dma but the vertex counts are available
// per mesh to transfer each mesh individually.
//=============================================================================//

#ifndef HARDWAREVERTS_H
#define HARDWAREVERTS_H

#ifdef _WIN32
#pragma once
#endif

#include "datamap.h"

// valve hardware vertexes
#define VHV_VERSION	2

namespace HardwareVerts
{

#pragma pack(1)

struct MeshHeader_t
{
	DECLARE_BYTESWAP_DATADESC();

	// this mesh is part of this lod
	unsigned int m_nLod;

	// this mesh has this many vertexes
	unsigned int m_nVertexes;

	// starting at this offset
	unsigned int m_nOffset;

	unsigned int m_nUnused[4];
};

struct FileHeader_t
{
	DECLARE_BYTESWAP_DATADESC();

	// file version as defined by VHV_VERSION
	int m_nVersion;

	// must match checkSum in the .mdl header
	unsigned int m_nChecksum;

	// a vertex consists of these components
	uint32 m_nVertexFlags;

	// the byte size of a single vertex
	// this won't be adequate, need some concept of byte format i.e. rgbexp32 vs rgba8888
	unsigned int m_nVertexSize;

	// total number of vertexes
	unsigned int m_nVertexes;

	int m_nMeshes;
	inline MeshHeader_t *pMesh( int nMesh ) const 
	{
		return (MeshHeader_t *)(((byte *)this) + sizeof(FileHeader_t)) + nMesh;
	};

	inline void *pVertexBase( int nMesh ) const 
	{
		return (void *)((byte *)this + pMesh( nMesh )->m_nOffset);
	};

	unsigned int m_nUnused[4];
};

#pragma pack()

}; // end namespace

#endif // HARDWAREVERTS_H

