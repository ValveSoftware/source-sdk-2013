//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing an abstract shape (ie drawable object)
//
//=============================================================================

#ifndef DMEFACESET_H
#define DMEFACESET_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"
#include "tier1/utlvector.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeMaterial;


//-----------------------------------------------------------------------------
// A class representing a face of a polygonal mesh
//-----------------------------------------------------------------------------
class CDmeFaceSet : public CDmElement
{
	DEFINE_ELEMENT( CDmeFaceSet, CDmElement );

public:
	// material accessors
	CDmeMaterial *GetMaterial();
	void SetMaterial( CDmeMaterial *pMaterial );

	// Total number of indices in the face set including the -1 end of face designators
	int NumIndices() const;
	const int *GetIndices() const;
	int AddIndices( int nCount );
	void SetIndex( int i, int nValue );
	void SetIndices( int nFirstIndex, int nCount, int *pIndices );
	int GetIndex( int i ) const;

	// Returns the number of vertices in the next polygon
	int GetNextPolygonVertexCount( int nFirstIndex ) const;

	// Returns the number of triangulated indices total
	int GetTriangulatedIndexCount() const;

	// Total number of indices in the face set excluding the -1 end of face designators
	int GetIndexCount() const;

	// Removes multiple faces from the face set
	void RemoveMultiple( int elem, int num );

	// Returns the number of faces in total... This should be the number of -1 indices in the face set
	// Which should equal  NumIndices() - GetIndexCount() but this function accounts for
	// empty faces (which aren't counted as faces) and a missing -1 terminator at the end
	int GetFaceCount() const;

private:
	CDmaArray< int > m_indices;
	CDmaElement< CDmeMaterial > m_material;
};


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline int CDmeFaceSet::NumIndices() const
{
	return m_indices.Count();
}

inline const int *CDmeFaceSet::GetIndices() const
{
	return m_indices.Base();
}

inline int CDmeFaceSet::GetIndex( int i ) const
{
	return m_indices[i];
}


#endif // DMEFACESET_H
