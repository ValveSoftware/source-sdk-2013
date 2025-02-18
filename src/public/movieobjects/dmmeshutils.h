//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Mesh Manipulation Utilities
//
//=============================================================================


#ifndef DMMESHUTILS_H
#define DMMESHUTILS_H

#if defined( _WIN32 )
#pragma once
#endif

#include "movieobjects/dmevertexdata.h"
#include "tier1/utlstring.h"
#include "tier1/UtlStringMap.h"
#include "tier1/utlvector.h"

class CDmeMesh;
class CDmMeshComp::CEdge;


//-----------------------------------------------------------------------------
// Mesh Utility function class (more like a namespace)
//-----------------------------------------------------------------------------
class CDmMeshUtils
{
public:

	static bool RemoveLargeAxisAlignedPlanarFaces( CDmeMesh *pMesh );

	static bool RemoveFacesWithMaterial( CDmeMesh *pMesh, const char *pMaterialName );

	static bool RemoveFacesWithMoreThanNVerts( CDmeMesh *pMesh, const int nVertexCount );

	enum Axis_t
	{
		kXAxis,
		kYAxis,
		kZAxis
	};

	static bool Mirror( CDmeMesh *pMesh, int axis = kXAxis );

	static bool RemapMaterial( CDmeMesh *pMesh, const CUtlString &src, const CUtlString &dst );

	static bool RemapMaterial( CDmeMesh *pMesh, const int nMaterialIndex, const CUtlString &dst );

	// Merge the specified mesh onto the first mesh under pRoot that fits it
	static bool Merge( CDmeMesh *pSrcMesh, CDmElement *pRoot );

	static bool Merge( CDmeMesh *pSrcMesh, CDmeMesh *pDstMesh, int nSkinningJointIndex );

	static bool CreateDeltasFromPresets( CDmeMesh *pMesh, CDmeVertexData *pPassedDst, const CUtlStringMap< CUtlString > &presetMap, bool bPurge, const CUtlVector< CUtlString > *pPurgeAllButThese = NULL );

	static bool PurgeUnusedDeltas( CDmeMesh *pMesh );

	enum WrinkleOp
	{
		kReplace,
		kAdd
	};

	static bool CreateWrinkleDeltaFromBaseState( CDmeVertexDeltaData *pDelta, float flScale = 1.0f, WrinkleOp wrinkleOp = kReplace, CDmeMesh *pMesh = NULL, CDmeVertexData *pBind = NULL, CDmeVertexData *pCurrent = NULL );

	static int FindMergeSocket(
		const CUtlVector< CUtlVector< CDmMeshComp::CEdge * > > &srcBorderEdgesList,
		CDmeMesh *pDstMesh );

	static bool Merge( CDmMeshComp &srcComp, const CUtlVector< CDmMeshComp::CEdge * > &edgeList, CDmeMesh *pDstMesh );

protected:
	static const int *BuildDataMirrorMap( CDmeVertexData *pBase, int axis, CDmeVertexData::StandardFields_t standardField, CUtlVector< int > &dataMirrorMap );

	static bool MirrorVertices( CDmeMesh *pMesh, CDmeVertexData *pBase, int axis, CUtlVector< int > &mirrorMap );

	static bool MirrorVertices( CDmeVertexData *pBase, int axis, int nOldVertexCount, int nMirrorCount, const CUtlVector< int > &mirrorMap, const CUtlVector< int > &posMirrorMap, const CUtlVector< int > &normalMirrorMap, const CUtlVector< int > &uvMirrorMap );

	static void MirrorVertices( CDmeVertexData *pBase, FieldIndex_t fieldIndex, int nOldVertexCount, int nMirrorCount, const CUtlVector< int > &baseIndices, const CUtlVector< int > &mirrorMap );

	static bool MirrorDelta( CDmeVertexDeltaData *pDelta, int axis, const CUtlVector< int > &posMirrorMap, const CUtlVector< int > &normalMirrorMap, const CUtlVector< int > &uvMirrorMap );

	static bool PurgeUnusedData( CDmeMesh *pMesh );

	static void CreateDeltasFromPresetGroup( CDmePresetGroup *pPresetGroup, CDmeCombinationOperator * pComboOp, const CUtlVector< CUtlString > * pPurgeAllButThese, CDmeMesh * pMesh, CDmeVertexData * pDst, CUtlStringMap< CUtlString > &conflictingNames, CUtlStringMap< CDmePreset * > &presetMap );

	static void PurgeUnreferencedDeltas( CDmeMesh *pMesh, CUtlStringMap< CDmePreset * > &presetMap, const CUtlVector< CUtlString > *pPurgeAllButThese, CDmeCombinationOperator *pComboOp );
};


//-----------------------------------------------------------------------------
// Iterate over all of the faces in a mesh.  Use it like this:
//
// for ( CDmMeshFaceIt fIt( pMesh ); !fIt.IsDone(); fIt.Next() )
// {
// }
//-----------------------------------------------------------------------------
class CDmMeshFaceIt
{
public:
	// Constructs a new face iterator for the specified mesh
	CDmMeshFaceIt( const CDmeMesh *pMesh, const CDmeVertexData *pVertexData = NULL );

	// Resets the iterator to the start of the specified mesh or for another iteration of the
	// current mesh if the specified mesh is NULL
	bool Reset( const CDmeMesh *pMesh = NULL, const CDmeVertexData *pVertexData = NULL );

	// Returns the total number of faces in the mesh
	int Count() const;

	// The number of vertices in the face
	int VertexCount() const;

	// Is the iterator at the end of the mesh?
	bool IsDone() const;

	// Move the iterator to the next face
	bool Next();

	// The face index of the current face in the range [ 0, Count() ]
	int Index() const;

	// Returns opposite sense of IsDone(), i.e. !IsDone() so true if iterator still has stuff to do
	operator bool() const { return !IsDone(); }

	// Prefix ++ which just calls Next() for syntax sugar
	CDmMeshFaceIt &operator ++() { Next(); return *this; }

	// Postfix ++ which just calls Next() for syntax sugar
	CDmMeshFaceIt operator ++( int ) { const CDmMeshFaceIt thisCopy( *this ); ++( *this ); return thisCopy; }

	// Gets the vertex indices for the vertices of this face
	bool GetVertexIndices( int *pIndices, int nIndices ) const;

	// Gets the vertex indices for the vertices of this face
	bool GetVertexIndices( CUtlVector< int > &vertexIndices ) const;

	// Gets the mesh relative vertex index given the face relative vertex index
	int GetVertexIndex( int nFaceRelativeVertexIndex ) const;

	// Gets the specified vertex data for the vertices of this face (if it exists) and the type matches
	template < class T_t >
	bool GetVertexData(
		CUtlVector< T_t > &vertexData,
		CDmeVertexDataBase::StandardFields_t nStandardField,
		CDmeVertexData *pPassedBase = NULL ) const;

protected:
	bool SetFaceSet();

	const CDmeMesh *m_pMesh;				// Current Mesh
	const CDmeVertexData *m_pVertexData;	// Current Vertex Data

	int m_nFaceSetCount;			// Number of face sets in current mesh: m_pMesh->FaceSetCount();
	int m_nFaceSetIndex;			// Index of current face set: [ 0, m_nFaceSetCount ]

	const CDmeFaceSet *m_pFaceSet;	// Current face set: m_pMesh->GetFaceSet( m_nFaceSetIndex )

	int m_nFaceSetIndexCount;		// Number of indices in current face set: m_pFaceSet->NumIndices();
	int m_nFaceSetIndexIndex;		// Index of current face set's indices: [ 0, m_nFaceSetIndexCount ]

	int m_nFaceCount;				// Number of faces in current mesh
	int m_nFaceIndex;				// The current face in the iteration [ 0, m_nFaceCount ]
};


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
template < class T_t >
inline bool CDmMeshFaceIt::GetVertexData(
	CUtlVector< T_t > &vertexData,
	CDmeVertexDataBase::StandardFields_t nStandardField,
	CDmeVertexData *pPassedBase /* = NULL */ ) const
{
	vertexData.RemoveAll();

	if ( IsDone() )
		return false;

	CDmeVertexData *pBase = pPassedBase ? pPassedBase : m_pMesh->GetCurrentBaseState();
	if ( !pBase )
		return false;

	const FieldIndex_t nFieldIndex = pBase->FindFieldIndex( nStandardField );
	if ( nFieldIndex < 0 )
		return false;

	CDmAttribute *pDataAttr = pBase->GetVertexData( nFieldIndex );
	if ( pDataAttr->GetType() != CDmAttributeInfo< CUtlVector< T_t > >().AttributeType() )
		return false;

	const CDmrArrayConst< T_t > data( pDataAttr );
	const CUtlVector< int > &indices( pBase->GetVertexIndexData( nFieldIndex ) );
}


#endif // DMMESHUTILS_H
