//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef VIRTUALMESH_H
#define VIRTUALMESH_H
#ifdef _WIN32
#pragma once
#endif

// NOTE: These are fixed length to make it easy to fill these out without memory allocation or storage
const int MAX_VIRTUAL_TRIANGLES = 1024;
struct virtualmeshlist_t
{
	Vector			*pVerts;
	int				indexCount;
	int				triangleCount;
	int				vertexCount;
	int				surfacePropsIndex;
	byte			*pHull;
	unsigned short	indices[MAX_VIRTUAL_TRIANGLES*3];
};

struct virtualmeshtrianglelist_t
{
	int				triangleCount;
	unsigned short	triangleIndices[MAX_VIRTUAL_TRIANGLES*3];
};

class IVirtualMeshEvent
{
public:
	virtual void GetVirtualMesh( void *userData, virtualmeshlist_t *pList ) = 0;
	virtual void GetWorldspaceBounds( void *userData, Vector *pMins, Vector *pMaxs ) = 0;
	virtual void GetTrianglesInSphere( void *userData, const Vector &center, float radius, virtualmeshtrianglelist_t *pList ) = 0;
};
struct virtualmeshparams_t
{
	IVirtualMeshEvent	*pMeshEventHandler;
	void				*userData;
	bool				buildOuterHull;
};

#endif // VIRTUALMESH_H
