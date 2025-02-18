//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing a mesh
//
//=============================================================================

#ifndef DMEMESH_H
#define DMEMESH_H

#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmeshape.h"
#include "movieobjects/dmevertexdata.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "mathlib/vector.h"
#include "tier1/utllinkedlist.h"
#include "Color.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmElement;
class CDmeFaceSet;
class CDmeVertexData;
class IMaterial;
class IMorph;
class IMesh;
class Vector;
class Vector4D;
class Color;
class CDmeDag;
class CMeshBuilder;
class CDmeCombinationOperator;
class CDmeSingleIndexedComponent;
class CDmeDrawSettings;
class CDmMeshComp;

//-----------------------------------------------------------------------------
// Mesh weights
//-----------------------------------------------------------------------------
enum MeshDeltaWeightType_t
{
	MESH_DELTA_WEIGHT_FIRST = 0,

	MESH_DELTA_WEIGHT_NORMAL = 0,
	MESH_DELTA_WEIGHT_LAGGED,

	MESH_DELTA_WEIGHT_TYPE_COUNT,
};


//-----------------------------------------------------------------------------
// Mesh representation
//-----------------------------------------------------------------------------
class CDmeMesh : public CDmeShape
{
DEFINE_ELEMENT( CDmeMesh, CDmeShape );

public:
	// resolve internal data from changed attributes
	virtual void OnAttributeChanged( CDmAttribute *pAttribute );

	void GetBoundingSphere( Vector &c, float &r, CDmeVertexData *pPassedBase, CDmeSingleIndexedComponent *pPassedSelection ) const;

	virtual void GetBoundingSphere( Vector &c, float &r ) const { return GetBoundingSphere( c, r, NULL, NULL ); }

	void GetBoundingBox( Vector &min, Vector &max, CDmeVertexData *pPassedBase /* = NULL */, CDmeSingleIndexedComponent *pPassedSelection /* = NULL */ ) const;
	
	virtual void GetBoundingBox( Vector &min, Vector &max ) const { return GetBoundingBox( min, max, NULL, NULL ); }

	// accessors
	int FaceSetCount() const;
	CDmeFaceSet *GetFaceSet( int nFaceSetIndex );
	const CDmeFaceSet *GetFaceSet( int nFaceSetIndex ) const;
	void AddFaceSet( CDmeFaceSet *faceSet );
	void RemoveFaceSet( int nFaceSetIndex );

	// Base states
	int BaseStateCount() const;

	CDmeVertexData *GetBaseState( int nBaseIndex ) const;

	CDmeVertexData *FindBaseState( const char *pStateName ) const;

	CDmeVertexData *FindOrCreateBaseState( const char *pStateName );
	bool DeleteBaseState( const char *pStateName );

	// Selects a particular base state to be current state
	void SetCurrentBaseState( const char *pStateName );
	CDmeVertexData *GetCurrentBaseState();
	const CDmeVertexData *GetCurrentBaseState() const;

	bool SetBindBaseState( CDmeVertexData *pBaseState );
	CDmeVertexData *GetBindBaseState();
	const CDmeVertexData *GetBindBaseState() const;

	// Draws the mesh
	void Draw( const matrix3x4_t &shapeToWorld, CDmeDrawSettings *pDrawSettings = NULL );

	// Compute triangulated indices
	void ComputeTriangulatedIndices( const CDmeVertexData *pBaseState, CDmeFaceSet *pFaceSet, int nFirstIndex, int *pIndices, int nOutCount );

	// Compute a default per-vertex tangent given normal data + uv data for all vertex data referenced by this mesh
	void ComputeDefaultTangentData( bool bSmoothTangents = false );

	// Compute a default per-vertex tangent given normal data + uv data
	void ComputeDefaultTangentData( CDmeVertexData *pVertexData, bool bSmoothTangents = false );

	// Delta states
	int DeltaStateCount() const;
	CDmeVertexDeltaData *GetDeltaState( int nDeltaIndex ) const;
	CDmeVertexDeltaData *FindDeltaState( const char *pDeltaName ) const;
	CDmeVertexDeltaData *FindOrCreateDeltaState( const char *pDeltaName );
	bool DeleteDeltaState( const char *pDeltaName );
	bool ResetDeltaState( const char *pDeltaName );
	int FindDeltaStateIndex( const char *pDeltaName ) const;
	void SetDeltaStateWeight( int nDeltaIndex, MeshDeltaWeightType_t type, float flMorphWeight );
	void SetDeltaStateWeight( int nDeltaIndex, MeshDeltaWeightType_t type, float flLeftWeight, float flRightWeight );
	CDmeVertexDeltaData *ModifyOrCreateDeltaStateFromBaseState( const char *pDeltaName, CDmeVertexData *pPassedBase = NULL, bool absolute = false );

	// Sets all of the data in the current base state to be the bind state plus the corrected delta, if delta is NULL then it's set to the bind state
	bool SetBaseStateToDelta( const CDmeVertexDeltaData *pDelta, CDmeVertexData *pPassedBase = NULL );

	// Selects the vertices from the delta that change position
	void SelectVerticesFromDelta( CDmeVertexDeltaData *pDelta, CDmeSingleIndexedComponent *pSelection );

	// Selects all the vertices in the mesh
	void SelectAllVertices( CDmeSingleIndexedComponent *pSelection, CDmeVertexData *pPassedBase = NULL );

	enum SelectHalfType_t
	{
		kLeft,
		kRight
	};

	// Selects all the vertices in the mesh
	void SelectHalfVertices( SelectHalfType_t selectHalfType, CDmeSingleIndexedComponent *pSelection, CDmeVertexData *pPassedBase = NULL );

	// Add the delta into the vertex data state weighted by the weight and masked by the weight map
	bool AddMaskedDelta(
		CDmeVertexDeltaData *pDelta,
		CDmeVertexData *pDst = NULL,
		float weight = 1.0f,
		const CDmeSingleIndexedComponent *pMask = NULL );

	// Interpolate between the current state and the specified delta by the specified percentage masked by the selection
	bool InterpMaskedDelta(
		CDmeVertexDeltaData *pDelta,
		CDmeVertexData *pDst = NULL,
		float weight = 1.0f,
		const CDmeSingleIndexedComponent *pMask = NULL );

	// Grows the selection by a specified amount
	void GrowSelection( int nSize, CDmeSingleIndexedComponent *pSelection, CDmMeshComp *pPassedMeshComp );

	// Shrinks the selection by a specified amount
	void ShrinkSelection( int nSize, CDmeSingleIndexedComponent *pSelection, CDmMeshComp *pPassedMeshComp );

	enum Falloff_t
	{
		STRAIGHT = 0,
		LINEAR = STRAIGHT,
		BELL,
		SMOOTH = BELL,
		SPIKE,
		DOME
	};

	enum Distance_t
	{
		DIST_ABSOLUTE = 0,
		DIST_RELATIVE,
		DIST_DEFAULT
	};

	CDmeSingleIndexedComponent *FeatherSelection( float falloffDistance, Falloff_t falloffType, Distance_t distanceType, CDmeSingleIndexedComponent *pSelection, CDmMeshComp *pPassedMeshComp );

	// Computes new normal deltas for all states based on position deltas
	void ComputeDeltaStateNormals();

	struct DeltaComputation_t 
	{
		int m_nDeltaIndex;
		int m_nDimensionality;
		CUtlVector<int> m_DependentDeltas;
	};

	// Construct list of all n-1 -> 1 dimensional delta states that will be active when this delta state is active
	void ComputeDependentDeltaStateList( CUtlVector< DeltaComputation_t > &compList );

	// Construct list of all > n dimensional delta states that when active have the specified state as a dependent
	bool ComputeSuperiorDeltaStateList( const char *pDeltaName, CUtlVector< int > &superiorDeltaStates );

	void SetDeltaNormalDataFromActualNormals( int nDeltaIndex, const CUtlVector<int> &deltaStateList, int nNormalCount, Vector *pNormals );

	void ComputeAllCorrectedPositionsFromActualPositions();

	// Computes adds a delta to the passed data weighted by the passed weight
	template < class T_t > void AddDelta(
		const CDmeVertexDeltaData *pDelta, T_t *pFullData, int nFullData, FieldIndex_t fieldIndex, float weight = 1.0f, const CDmeSingleIndexedComponent *pMask = NULL );

	template < class T_t > void AddDelta(
		const CDmeVertexDeltaData *pDelta, T_t *pFullData, int nFullData, CDmeVertexData::StandardFields_t standardField, float weight = 1.0f, const CDmeSingleIndexedComponent *pMask = NULL );

	bool SetBaseStateToDeltas( CDmeVertexData *pPassedBase = NULL );

	template < class T_t >
	bool SetBaseDataToDeltas( CDmeVertexData *pBase, CDmeVertexData::StandardFields_t nStandardField, CDmrArrayConst< T_t > &srcData, CDmrArray< T_t > &dstData, bool bDoStereo, bool bDoLag );

	// Replace all instances of a material with a different material
	void ReplaceMaterial( const char *pOldMaterialName, const char *pNewMaterialName );

	// makes all the normals in the mesh unit length
	void NormalizeNormals();

	// Collapses redundant normals in the model
	// flNormalBlend is the maximum difference in the dot product between two normals to consider them
	// to be the same normal, a value of cos( DEG2RAD( 2.0 ) ) is the default studiomdl uses, for example
	void CollapseRedundantNormals( float flNormalBlend );

	// SWIG errors on the parsing of something in the private section of DmeMesh, it isn't exposed by SWIG anyway, so have SWIG ignore it
#ifndef SWIG
	template < class T_t > static int GenerateCompleteDataForDelta( const CDmeVertexDeltaData *pDelta, T_t *pFullData, int nFullData, CDmeVertexData::StandardFields_t standardField );

private:
	friend class CDmMeshComp;

	struct FaceSet_t
	{
		FaceSet_t() : m_bBuilt(false) {}
		IMesh *m_pMesh;
		bool m_bBuilt;
	};

	struct Triangle_t
	{
		int m_nIndex[3];
		Vector m_vecTangentS;
		Vector m_vecTangentT;
	};

	struct RenderVertexDelta_t
	{
		Vector m_vecDeltaPosition;
		Vector m_vecDeltaNormal;
		Vector2D m_vecDeltaUV;
		Vector4D m_vecDeltaColor;
		float m_flDeltaWrinkle;
	};

	VertexFormat_t ComputeHwMeshVertexFormat( void );
	IMorph *CreateHwMorph( IMaterial *pMTL );
	IMesh *CreateHwMesh( CDmeFaceSet *pFaceSet );

	// Draws the mesh when it uses too many bones
	void DrawDynamicMesh( CDmeFaceSet *pFaceSet, matrix3x4_t *pPoseToWorld, bool bHasActiveDeltaStates, CDmeDrawSettings *pDrawSettings = NULL );

	// Build a map from vertex index to a list of triangles that share the vert.
	void BuildTriangleMap( const CDmeVertexData *pBaseState, CDmeFaceSet* pFaceSet, CUtlVector<Triangle_t>& triangles, CUtlVector< CUtlVector<int> >* pVertToTriMap = NULL );

	// Computes tangent space data for triangles
	void ComputeTriangleTangets( const CDmeVertexData *pVertexData, CUtlVector<Triangle_t>& triangles );

	// Build a map from vertex index to a list of triangles that share the vert.
	void ComputeAverageTangent( CDmeVertexData *pVertexData, bool bSmoothTangents, CUtlVector< CUtlVector<int> >& vertToTriMap, CUtlVector<Triangle_t>& triangles );

	// Do we have active delta state data?
	bool HasActiveDeltaStates() const;

	// Adds deltas into a delta mesh
	template< class T > bool AddVertexDelta( CDmeVertexData *pBaseState, void *pVertexData, int nStride, CDmeVertexDataBase::StandardFields_t fieldId, int nIndex, bool bDoLag );
	template< class T > bool AddStereoVertexDelta( CDmeVertexData *pBaseState, void *pVertexData, int nStride, CDmeVertexDataBase::StandardFields_t fieldId, int nIndex, bool bDoLag );
	void AddTexCoordDelta( RenderVertexDelta_t *pRenderDelta, float flWeight, CDmeVertexDeltaData *pDeltaState );
	void AddColorDelta( RenderVertexDelta_t *pRenderDelta, float flWeight, CDmeVertexDeltaData *pDeltaState );

	// Builds deltas based on the current deltas, returns true if there was delta wrinkle data
	bool BuildDeltaMesh( int nVertices, RenderVertexDelta_t *pDelta );

	// Builds a map from vertex index to all triangles that use it
	void BuildVertToTriMap( const CDmeVertexData *pVertexData, CUtlVector<Triangle_t> &triangles, CUtlVector< CUtlVector<int> > &vertToTriMap );

	// Compute the dimensionality of the delta state (how many inputs affect it)
	int ComputeDeltaStateDimensionality( int nDeltaIndex );

	// Discovers the atomic controls used by the various delta states 
	void BuildAtomicControlLists( int nCount, DeltaComputation_t *pInfo, CUtlVector< CUtlVector< int > > &deltaStateUsage );

	// Computes the aggregate position for all vertices after applying a set of delta states
	void AddDelta( CDmeVertexData *pBaseState, Vector *pDeltaPosition, int nDeltaStateIndex, CDmeVertexData::StandardFields_t fieldId );

	// Converts pose-space normals into deltas appropriate for correction delta states
	void ComputeCorrectedNormalsFromActualNormals( const CUtlVector<int> &deltaStateList, int nNormalCount, Vector *pNormals );

	// Copies the corrected normal data into a delta state
	void SetDeltaNormalData( int nDeltaIndex, int nNormalCount, Vector *pNormals );
	// Renders normals 
	void RenderNormals( matrix3x4_t *pPoseToWorld, RenderVertexDelta_t *pDelta );

	// Writes triangulated indices for a face set into a meshbuilder
	void WriteTriangluatedIndices( const CDmeVertexData *pBaseState, CDmeFaceSet *pFaceSet, CMeshBuilder &meshBuilder );

	// Initializes the normal material
	static void InitializeNormalMaterial();

	// Sort function 
	static int DeltaStateLessFunc( const void * lhs, const void * rhs );

	// Computes a list of the delta states ordered by dimensionality
	void ComputeDeltaStateComputationList( CUtlVector< DeltaComputation_t > &compList );

	// Compute the number of combinations of n items taken k at a time nCk - Probably doesn't belong here but it useful for combos
	static void Combinations( int n, int k, CUtlVector< CUtlVector< int > > &combos, int *pTmpArray = NULL, int start = 0, int currentK = 0 );

	// Splits the passed delta state name on '_' and finds all of the control Delta states which make up the name
	bool GetControlDeltaIndices( CDmeVertexDeltaData *pDeltaState, CUtlVector< int > &controlDeltaIndices ) const;

	// Splits the passed delta state name on '_' and finds all of the control Delta states which make up the name
	bool GetControlDeltaIndices( const char *pDeltaStateName, CUtlVector< int > &controlDeltaIndices ) const;

	// Builds a complete list of all of the delta states expressed as the control indices
	bool BuildCompleteDeltaStateControlList( CUtlVector< CUtlVector< int > > &deltaStateControlList ) const;

	// Given a list of control indices and a complete list of control indices for each delta state, returns the delta index or -1 if it doesn't exist
	int FindDeltaIndexFromControlIndices( const CUtlVector< int > &controlIndices, const CUtlVector< CUtlVector< int > > &controlList ) const;

	// Builds a list of all of the dependent delta states that do not already exist
	bool BuildMissingDependentDeltaList( CDmeVertexDeltaData *pDeltaState, CUtlVector< int > &controlIndices, CUtlVector< CUtlVector< int > > &dependentStates ) const;

	static void ComputeCorrectedPositionsFromActualPositions( const CUtlVector< int > &deltaStateList, int nPositionCount, Vector *pPositions );

	template < class T_t > void AddCorrectedDelta(
		CDmrArray< T_t > &baseDataArray,
		const CUtlVector< int > &baseIndices,
		const DeltaComputation_t &deltaComputation,
		const char *pFieldName,
		float weight = 1.0f,
		const CDmeSingleIndexedComponent *pMask = NULL );

	template < class T_t > void AddCorrectedDelta(
		CUtlVector< T_t > &baseData,
		const CUtlVector< int > &baseIndices,
		const DeltaComputation_t &deltaComputation,
		const char *pFieldName,
		float weight = 1.0f,
		const CDmeSingleIndexedComponent *pMask = NULL );

	// Add the delta into the vertex data state weighted by the weight and masked by the weight map
	bool AddCorrectedMaskedDelta(
		CDmeVertexDeltaData *pDelta,
		CDmeVertexData *pDst = NULL,
		float weight = 1.0f,
		const CDmeSingleIndexedComponent *pMask = NULL );

	template < class T_t > void AddRawDelta(
		CDmeVertexDeltaData *pDelta,
		CDmrArray< T_t > &baseDataArray,
		FieldIndex_t nDeltaFieldIndex,
		float weight = 1.0f,
		const CDmeSingleIndexedComponent *pMask = NULL );

	template < class T_t > void AddRawDelta(
		CDmeVertexDeltaData *pDelta,
		CUtlVector< T_t > &baseData,
		FieldIndex_t nDeltaFieldIndex,
		float weight = 1.0f,
		const CDmeSingleIndexedComponent *pMask = NULL );

	friend class CDmxEdit;
	bool RemoveBaseState( CDmeVertexData *pBase ); 
	CDmeVertexData *FindOrAddBaseState( CDmeVertexData *pBase );

	// CFalloff functors map [0, 1] values to [0, 1] values
	template < int T >
	class CFalloff
	{
	public:
		virtual inline float operator()( float x ) { return 1 - x; }
	};

	template<>
	class CFalloff< CDmeMesh::LINEAR >
	{
	public:
		virtual inline float operator()( float x ) { return 1 - x; }
	};

	template<>
	class CFalloff< CDmeMesh::SMOOTH >
	{
	public:
		virtual inline float operator()( float x ) {
			return ( cosf( x * M_PI ) + 1.0f ) / 2.0f;
		}
	};

	template<>
	class CFalloff< CDmeMesh::DOME >
	{
	public:
		virtual inline float operator()( float x ) {
			return ( cosf( x * M_PI / 2.0 ) );
		}
	};

	template<>
	class CFalloff< CDmeMesh::SPIKE >
	{
	public:
		virtual inline float operator()( float x ) {
			return ( 1.0f - cosf( ( 1.0f - x ) * M_PI / 2.0 ) );
		}
	};

	// Feather's the selection by a specified amount, creates a new CDmeSingleIndexedComponent or NULL if error
	template < int T >
	CDmeSingleIndexedComponent *FeatherSelection( float fFalloffDistance, Distance_t distanceType, CDmeSingleIndexedComponent *pSelection, CDmMeshComp *pPassedMeshComp );

	bool CreateDeltaFieldFromBaseField( CDmeVertexData::StandardFields_t nStandardFieldIndex, const CDmrArrayConst< float > &baseArray, const CDmrArrayConst< float > &bindArray, CDmeVertexDeltaData *pDelta );
	bool CreateDeltaFieldFromBaseField( CDmeVertexData::StandardFields_t nStandardFieldIndex, const CDmrArrayConst< Vector2D > &baseArray, const CDmrArrayConst< Vector2D > &bindArray, CDmeVertexDeltaData *pDelta );
	bool CreateDeltaFieldFromBaseField( CDmeVertexData::StandardFields_t nStandardFieldIndex, const CDmrArrayConst< Vector > &baseArray, const CDmrArrayConst< Vector > &bindArray, CDmeVertexDeltaData *pDelta );

	template< class T_t > bool InterpMaskedData(
		CDmrArray< T_t > &aData,
		const CUtlVector< T_t > &bData,
		float weight,
		const CDmeSingleIndexedComponent *pMask ) const;

	// Interpolate between the current state and the specified delta by the specified percentage masked by the selection
	bool InterpMaskedData(
		CDmeVertexData *paData,
		const CDmeVertexData *pbData,
		float weight,
		const CDmeSingleIndexedComponent *pMask ) const;

	// Find the closest vertex in the specified selection to the passed vertex in the specified base state, if the passed base state is NULL is the current base state
	int ClosestSelectedVertex( int vIndex, CDmeSingleIndexedComponent *pSelection, const CDmeVertexData *pPassedBase = NULL ) const;

	// Return the distance between the two vertices in the specified base state, if the specified base state is NULL the current state is used
	float DistanceBetween( int vIndex0, int vIndex1, const CDmeVertexData *pPassedBase = NULL ) const;

	void DrawWireframeFaceSet( CDmeFaceSet *pFaceSet, matrix3x4_t *pPoseToWorld, bool bHasActiveDeltaStates, CDmeDrawSettings *pDrawSettings );

	void ComputeNormalsFromPositions( CDmeVertexData *pBase, const Vector *pPosition, const CUtlVector<Triangle_t> &triangles, int nNormalCount, Vector *pNormals );

	CDmaElement< CDmeVertexData > m_BindBaseState;
	CDmaElement< CDmeVertexData > m_CurrentBaseState;
	CDmaElementArray< CDmeVertexData > m_BaseStates;
	CDmaElementArray< CDmeVertexDeltaData > m_DeltaStates;
	CDmaElementArray< CDmeFaceSet > m_FaceSets;

	// x is left value, y is right value. If the delta state isn't split, they are the same value
	CDmaArray<Vector2D> m_DeltaStateWeights[MESH_DELTA_WEIGHT_TYPE_COUNT];

	// Cached-off map of fields->
	CUtlVector< FaceSet_t > m_hwFaceSets;
	
	// Normal rendering materials
	static bool s_bNormalMaterialInitialized;
	static CMaterialReference s_NormalMaterial;
	static CMaterialReference s_NormalErrorMaterial;

	static bool s_bMaterialsInitialized;
	static CMaterialReference s_WireframeMaterial;
	static CMaterialReference s_WireframeOnShadedMaterial;

	friend class CRenderInfo;

#endif // ndef SWIG
};


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline int CDmeMesh::BaseStateCount() const
{
	return m_BaseStates.Count();
}


inline CDmeVertexData *CDmeMesh::GetBaseState( int nBaseIndex ) const
{
	return m_BaseStates[ nBaseIndex ];
}


//-----------------------------------------------------------------------------
// Utility method to compute default tangent data on all meshes in the sub-dag hierarchy
//-----------------------------------------------------------------------------
void ComputeDefaultTangentData( CDmeDag *pDag, bool bSmoothTangents );


#endif // DMEMESH_H
