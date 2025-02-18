//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef POLYHEDRON_H_
#define	POLYHEDRON_H_

#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"



struct Polyhedron_IndexedLine_t
{
	unsigned short iPointIndices[2];
};

struct Polyhedron_IndexedLineReference_t
{
	unsigned short iLineIndex;
	unsigned char iEndPointIndex; //since two polygons reference any one line, one needs to traverse the line backwards, this flags that behavior
};

struct Polyhedron_IndexedPolygon_t
{
	unsigned short iFirstIndex;
	unsigned short iIndexCount;
	Vector polyNormal;
};

class CPolyhedron //made into a class because it's going virtual to support distinctions between temp and permanent versions
{
public:
	Vector *pVertices;
	Polyhedron_IndexedLine_t *pLines;
	Polyhedron_IndexedLineReference_t *pIndices;
	Polyhedron_IndexedPolygon_t *pPolygons;
	
	unsigned short iVertexCount;
	unsigned short iLineCount;
	unsigned short iIndexCount;
	unsigned short iPolygonCount;

	virtual ~CPolyhedron( void ) {};
	virtual void Release( void ) = 0;
	Vector Center( void );
};

class CPolyhedron_AllocByNew : public CPolyhedron
{
public:
	virtual void Release( void );
	static CPolyhedron_AllocByNew *Allocate( unsigned short iVertices, unsigned short iLines, unsigned short iIndices, unsigned short iPolygons ); //creates the polyhedron along with enough memory to hold all it's data in a single allocation

private:
	CPolyhedron_AllocByNew( void ) { }; //CPolyhedron_AllocByNew::Allocate() is the only way to create one of these.
};

CPolyhedron *GeneratePolyhedronFromPlanes( const float *pOutwardFacingPlanes, int iPlaneCount, float fOnPlaneEpsilon, bool bUseTemporaryMemory = false ); //be sure to polyhedron->Release()
CPolyhedron *ClipPolyhedron( const CPolyhedron *pExistingPolyhedron, const float *pOutwardFacingPlanes, int iPlaneCount, float fOnPlaneEpsilon, bool bUseTemporaryMemory = false ); //this does NOT modify/delete the existing polyhedron

CPolyhedron *GetTempPolyhedron( unsigned short iVertices, unsigned short iLines, unsigned short iIndices, unsigned short iPolygons ); //grab the temporary polyhedron. Avoids new/delete for quick work. Can only be in use by one chunk of code at a time


#endif //#ifndef POLYHEDRON_H_

