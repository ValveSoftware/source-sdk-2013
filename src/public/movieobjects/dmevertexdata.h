//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing vertex data
//
//=============================================================================

#ifndef DMEVERTEXDATA_H
#define DMEVERTEXDATA_H

#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"
#include "mathlib/vector.h"
#include "Color.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class Vector;
class Vector4D;
class Color;


//-----------------------------------------------------------------------------
// Used to represent fields
//-----------------------------------------------------------------------------
typedef int FieldIndex_t;


class CDmeVertexDataBase : public CDmElement
{
	DEFINE_ELEMENT( CDmeVertexDataBase, CDmElement );

public:
	// NOTE: If you add fields to this, add to g_pStandardFieldNames in dmevertexdata.cpp
	enum StandardFields_t
	{
		FIELD_POSITION,
		FIELD_NORMAL,
		FIELD_TANGENT,
		FIELD_TEXCOORD,
		FIELD_COLOR,
		FIELD_JOINT_WEIGHTS,
		FIELD_JOINT_INDICES,
		FIELD_BALANCE,			// Used by left/right delta states
		FIELD_MORPH_SPEED,		// Used to author morph speeds
		FIELD_WRINKLE,			// Used to author morphed wrinklemaps
		FIELD_WEIGHT,			// Weight is just the different between the base position and the delta position
		STANDARD_FIELD_COUNT,
	};

	// resolve internal data from changed attributes
	virtual void Resolve();

	// Returns the number of joints per vertex
	int JointCount() const;

	// Vertex accessors
	int VertexCount() const;
	const Vector& GetPosition( int nVertexIndex ) const;
	const Vector& GetNormal( int nVertexIndex ) const;
	const Vector2D& GetTexCoord( int nVertexIndex ) const;
	const Vector4D& GetTangent( int nVertexIndex ) const;
	const Color& GetColor( int nVertexIndex ) const;
	const float *GetJointWeights( int nVertexIndex ) const;
	const float *GetJointPositionWeights( int nPositionIndex ) const;
	const int *GetJointIndices( int nVertexIndex ) const;
	const int *GetJointPositionIndices( int nPositionIndex ) const;
	float GetBalance( int nVertexIndex ) const;
	float GetMorphSpeed( int nVertexIndex ) const;
	float GetWrinkle( int nVertexIndex ) const;
	float GetWeight( int nVertexIndex ) const;

	// Returns indices into the various fields
	int GetPositionIndex( int nVertexIndex ) const;
	int GetNormalIndex( int nVertexIndex ) const;
	int GetTangentIndex( int nVertexIndex ) const;
	int GetTexCoordIndex( int nVertexIndex ) const;
	int GetColorIndex( int nVertexIndex ) const;
	int GetBalanceIndex( int nVertexIndex ) const;
	int GetMorphSpeedIndex( int nVertexIndex ) const;
	int GetWrinkleIndex( int nVertexIndex ) const;
	int GetWeightIndex( int nVertexIndex ) const;

	// Creates a new vertex field. NOTE: This cannot be used to create joint weights + indices
	template< class T >
	FieldIndex_t CreateField( const char *pFieldName );
	FieldIndex_t CreateField( const char *pFieldName, DmAttributeType_t type );
	FieldIndex_t CreateField( StandardFields_t fieldId );

	// Use this to create vertex fields for joint weights + indices
	void CreateJointWeightsAndIndices( int nJointCount, FieldIndex_t *pJointWeightsField, FieldIndex_t *pJointIndicesField );

	// Returns the field index of a particular field
	FieldIndex_t FindFieldIndex( const char *pFieldName ) const;
	FieldIndex_t FindFieldIndex( StandardFields_t nFieldIndex ) const;

	// Adds a new vertex, returns the vertex index
	// NOTE: This will also add vertex indices for DmeMeshDeltaData
	int AddVertexData( FieldIndex_t nFieldIndex, int nCount );

	// Sets vertex data
	void SetVertexData( FieldIndex_t nFieldIndex, int nFirstVertex, int nCount, DmAttributeType_t valueType, const void *pData );
	void SetVertexIndices( FieldIndex_t nFieldIndex, int nFirstIndex, int nCount, const int *pIndices );

	// Removes all vertex data associated with a particular field
	void RemoveAllVertexData( FieldIndex_t nFieldIndex );

	// Returns arbitrary vertex + index data
	CDmAttribute* GetVertexData( FieldIndex_t nFieldIndex );
	const CDmAttribute* GetVertexData( FieldIndex_t nFieldIndex ) const;
	CDmAttribute* GetIndexData( FieldIndex_t nFieldIndex );
	const CDmAttribute* GetIndexData( FieldIndex_t nFieldIndex ) const;

	// Returns well-known vertex data
	const CUtlVector<Vector> &GetPositionData( ) const;
	const CUtlVector<Vector> &GetNormalData( ) const;
	const CUtlVector<Vector4D> &GetTangentData( ) const;
	const CUtlVector<Vector2D> &GetTextureCoordData( ) const;
	const CUtlVector<Color> &GetColorData( ) const;
	const float *GetJointWeightData( int nDataIndex ) const;
	const int *GetJointIndexData( int nDataIndex ) const;
	const CUtlVector<float> &GetBalanceData( ) const;
	const CUtlVector<float> &GetMorphSpeedData( ) const;
	const CUtlVector<float> &GetWrinkleData( ) const;
	const CUtlVector<float> &GetWeightData( ) const;

	// Returns well-known index data
	const CUtlVector<int> &GetVertexIndexData( FieldIndex_t nFieldIndex ) const;
	const CUtlVector<int> &GetVertexIndexData( StandardFields_t fieldId ) const;

	// Do we have skinning data?
	bool HasSkinningData() const;
	
	// Do we need tangent data? (Utility method for applications to know if they should call ComputeDefaultTangentData)
	bool NeedsTangentData() const;

	// Should we flip the V coordinates?
	bool IsVCoordinateFlipped() const;
	void FlipVCoordinate( bool bFlip );

	// Returns an inverse map from vertex data index to vertex index
	const CUtlVector< int > &FindVertexIndicesFromDataIndex( FieldIndex_t nFieldIndex, int nDataIndex );
	const CUtlVector< int > &FindVertexIndicesFromDataIndex( StandardFields_t nFieldIndex, int nDataIndex );

	int FieldCount() const;

	const char *FieldName( int i ) const;

	void CopyFrom( CDmeVertexDataBase *pSrc );

	void CopyTo( CDmeVertexDataBase *pDst ) const;

protected:
	struct FieldInfo_t
	{
		CUtlString m_Name;
		CDmAttribute *m_pVertexData;
		CDmAttribute* m_pIndexData;
		CUtlVector< CUtlVector< int > > m_InverseMap;
		bool m_bInverseMapDirty;
	};

	// Derived classes must inherit
	virtual bool IsVertexDeltaData() const { Assert(0); return false; }

	// Computes the vertex count ( min of the index buffers )
	void ComputeFieldInfo();

	// Computes the vertex count ( min of the index buffers )
	void ComputeVertexCount();

	// Updates info for fast lookups for well-known fields
	void UpdateStandardFieldInfo( int nFieldIndex, const char *pFieldName, DmAttributeType_t attrType );

	// Adds a field to the vertex format
	void FindOrAddVertexField( const char *pFieldName );

	// Returns the index of a particular field
	int GetFieldIndex( int nVertexIndex, StandardFields_t nFieldIndex ) const;
	// List of names of attributes containing vertex data
	CDmaStringArray m_VertexFormat;

	CDmaVar< int > m_nJointCount;
	CDmaVar< bool > m_bFlipVCoordinates;
	CUtlVector< FieldInfo_t > m_FieldInfo;
	FieldIndex_t m_pStandardFieldIndex[STANDARD_FIELD_COUNT];
	int m_nVertexCount;
};


//-----------------------------------------------------------------------------
// Creates a particular vertex data field + associated index field
//-----------------------------------------------------------------------------
template< class T >
inline FieldIndex_t CDmeVertexDataBase::CreateField( const char *pFieldName )
{
	return CreateField( pFieldName, CDmAttributeInfo< CUtlVector<T> >::AttributeType() );
}


//-----------------------------------------------------------------------------
// Returns a standard field index
//-----------------------------------------------------------------------------
inline FieldIndex_t CDmeVertexDataBase::FindFieldIndex( StandardFields_t nFieldIndex ) const
{	
	return m_pStandardFieldIndex[ nFieldIndex ];
}


//-----------------------------------------------------------------------------
// Vertex field accessors
//-----------------------------------------------------------------------------
inline int CDmeVertexDataBase::VertexCount() const
{
	return m_nVertexCount; 
}


//-----------------------------------------------------------------------------
// Returns the number of joints per vertex
//-----------------------------------------------------------------------------
inline int CDmeVertexDataBase::JointCount() const
{
	return m_nJointCount;
}


//-----------------------------------------------------------------------------
// Should we flip the V coordinates?
//-----------------------------------------------------------------------------
inline bool CDmeVertexDataBase::IsVCoordinateFlipped() const
{
	return m_bFlipVCoordinates;
}

inline void CDmeVertexDataBase::FlipVCoordinate( bool bFlip )
{
	m_bFlipVCoordinates = bFlip;
}


//-----------------------------------------------------------------------------
// Returns arbitrary vertex data
//-----------------------------------------------------------------------------
inline CDmAttribute* CDmeVertexDataBase::GetVertexData( FieldIndex_t nFieldIndex )
{
	return m_FieldInfo[ nFieldIndex ].m_pVertexData;
}

inline const CDmAttribute* CDmeVertexDataBase::GetVertexData( FieldIndex_t nFieldIndex ) const
{
	return m_FieldInfo[ nFieldIndex ].m_pVertexData;
}


//-----------------------------------------------------------------------------
// Returns arbitrary index data
//-----------------------------------------------------------------------------
inline CDmAttribute* CDmeVertexDataBase::GetIndexData( FieldIndex_t nFieldIndex )
{
	return m_FieldInfo[ nFieldIndex ].m_pIndexData;
}

inline const CDmAttribute* CDmeVertexDataBase::GetIndexData( FieldIndex_t nFieldIndex ) const
{
	return m_FieldInfo[ nFieldIndex ].m_pIndexData;
}


//-----------------------------------------------------------------------------
// Utility method for getting at various vertex field indices
//-----------------------------------------------------------------------------
inline int CDmeVertexDataBase::GetFieldIndex( int nVertexIndex, StandardFields_t nFieldId ) const
{
	Assert( nVertexIndex < m_nVertexCount );
	FieldIndex_t nFieldIndex = m_pStandardFieldIndex[nFieldId];
	if ( nFieldIndex < 0 )
		return -1;

	CDmrArrayConst<int> indices( GetIndexData( nFieldIndex ) );
	return indices[ nVertexIndex ];
}


//-----------------------------------------------------------------------------
//
// Vertex Data for base states
//
//-----------------------------------------------------------------------------
class CDmeVertexData : public CDmeVertexDataBase
{
	DEFINE_ELEMENT( CDmeVertexData, CDmeVertexDataBase );

public:
	// Adds a new vertex; creates a new entry in all vertex data fields
	int AddVertexIndices( int nCount );

private:
	virtual bool IsVertexDeltaData() const { return false; }
};


//-----------------------------------------------------------------------------
//
// Vertex Data for delta states
//
//-----------------------------------------------------------------------------
class CDmeVertexDeltaData : public CDmeVertexDataBase
{
	DEFINE_ELEMENT( CDmeVertexDeltaData, CDmeVertexDataBase );

public:
	// Computes wrinkle data from position deltas
	// NOTE: Pass in negative scales to get 'compression', positive to get 'expansion'
	void GenerateWrinkleDelta( CDmeVertexData *pBindState, float flScale, bool bOverwrite );

	// Computes a float map which is the distance between the base and delta position
	// The maximum distance any vertex is moved is returned
	float GenerateWeightDelta( CDmeVertexData *pBindState );

	CDmaVar< bool > m_bCorrected;

private:
	virtual bool IsVertexDeltaData() const { return true; }

	// Computes max positional delta length
	float ComputeMaxDeflection( );
};


#endif // DMEVERTEXDATA_H
