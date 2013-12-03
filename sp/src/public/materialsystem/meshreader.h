//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef MESHREADER_H
#define MESHREADER_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// This is used to read vertex and index data out of already-created meshes.
// xbox uses this a lot so it doesn't have to store sysmem backups of the
// vertex data.
//-----------------------------------------------------------------------------
class CBaseMeshReader : protected MeshDesc_t
{
// Initialization.
public:

	CBaseMeshReader();
	~CBaseMeshReader();

	// Use BeginRead/EndRead to initialize the mesh reader.
	void BeginRead( 
		IMesh* pMesh, 
		int firstVertex = 0, 
		int numVertices = 0, 
		int firstIndex = 0, 
		int numIndices = 0 );
		
	void EndRead();

	// PC can use this if it stores its own copy of meshes around, in case
	// locking static buffers is too costly.
	void BeginRead_Direct( const MeshDesc_t &desc, int numVertices, int nIndices );

	// Resets the mesh builder so it points to the start of everything again
	void Reset();
	

protected:
	IMesh *m_pMesh;
	int m_MaxVertices;
	int m_MaxIndices;
};


// A bunch of accessors for the data that CBaseMeshReader sets up.
class CMeshReader : public CBaseMeshReader
{
public:
// Access to vertex data.
public:
	int NumIndices() const;
	unsigned short Index( int index ) const;

	const Vector& Position( int iVertex ) const;

	unsigned int Color( int iVertex ) const;
	
	const float *TexCoord( int iVertex, int stage ) const;
	void TexCoord2f( int iVertex, int stage, float &s, float &t ) const;
	const Vector2D& TexCoordVector2D( int iVertex, int stage ) const;

	int NumBoneWeights() const;
	float Wrinkle( int iVertex ) const;

	const Vector &Normal( int iVertex ) const;
	void Normal( int iVertex, Vector &vNormal ) const;

	const Vector &TangentS( int iVertex ) const;
	const Vector &TangentT( int iVertex ) const;
	float BoneWeight( int iVertex ) const;

#ifdef NEW_SKINNING
	float* BoneMatrix( int iVertex ) const;
#else
	unsigned char* BoneMatrix( int iVertex ) const;
#endif
};


//-----------------------------------------------------------------------------
// CBaseMeshReader implementation.
//-----------------------------------------------------------------------------

inline CBaseMeshReader::CBaseMeshReader()
{
	m_pMesh = NULL;
}

inline CBaseMeshReader::~CBaseMeshReader()
{
	Assert( !m_pMesh );
}

inline void CBaseMeshReader::BeginRead( 
	IMesh* pMesh, 
	int firstVertex, 
	int numVertices, 
	int firstIndex, 
	int numIndices )
{
	Assert( pMesh && (!m_pMesh) );

	if ( numVertices < 0 )
	{
		numVertices = pMesh->VertexCount();
	}

	if ( numIndices < 0 )
	{
		numIndices = pMesh->IndexCount();
	}

	m_pMesh = pMesh;
	m_MaxVertices = numVertices;
	m_MaxIndices = numIndices;

	// UNDONE: support reading from compressed VBs if needed
	VertexCompressionType_t compressionType = CompressionType( pMesh->GetVertexFormat() );
	Assert( compressionType == VERTEX_COMPRESSION_NONE );
	if ( compressionType != VERTEX_COMPRESSION_NONE )
	{
		Warning( "Cannot use CBaseMeshReader with compressed vertices! Will get junk data or a crash.\n" );
	}

	// Locks mesh for modifying
	pMesh->ModifyBeginEx( true, firstVertex, numVertices, firstIndex, numIndices, *this );

	// Point to the start of the buffers..
	Reset();
}

inline void CBaseMeshReader::EndRead()
{
	Assert( m_pMesh );
	m_pMesh->ModifyEnd( *this );
	m_pMesh = NULL;
}

inline void CBaseMeshReader::BeginRead_Direct( const MeshDesc_t &desc, int nVertices, int nIndices )
{
	MeshDesc_t *pThis = this;
	*pThis = desc;
	m_MaxVertices = nVertices;
	m_MaxIndices = nIndices;

	// UNDONE: support reading from compressed verts if necessary
	Assert( desc.m_CompressionType == VERTEX_COMPRESSION_NONE );
	if ( desc.m_CompressionType != VERTEX_COMPRESSION_NONE )
	{
		Warning( "Cannot use CBaseMeshReader with compressed vertices!\n" );
	}
}

inline void CBaseMeshReader::Reset()
{
}




// -------------------------------------------------------------------------------------- //
// CMeshReader implementation.
// -------------------------------------------------------------------------------------- //

inline int CMeshReader::NumIndices() const
{
	return m_MaxIndices;
}

inline unsigned short CMeshReader::Index( int index ) const
{
	Assert( (index >= 0) && (index < m_MaxIndices) );
	return m_pIndices[index * m_nIndexSize];
}

inline const Vector& CMeshReader::Position( int iVertex ) const
{
	Assert( iVertex >= 0 && iVertex < m_MaxVertices );
	return *(Vector*)((char*)m_pPosition + iVertex * m_VertexSize_Position);
}

inline unsigned int CMeshReader::Color( int iVertex ) const
{
	Assert( iVertex >= 0 && iVertex < m_MaxVertices );
	unsigned char *pColor = m_pColor + iVertex * m_VertexSize_Color;
	return (pColor[0] << 16) | (pColor[1] << 8) | (pColor[2]) | (pColor[3] << 24);
}

inline const float *CMeshReader::TexCoord( int iVertex, int iStage ) const
{
	Assert( iVertex >= 0 && iVertex < m_MaxVertices );
	return (float*)( (char*)m_pTexCoord[iStage] + iVertex * m_VertexSize_TexCoord[iStage] );
}

inline void CMeshReader::TexCoord2f( int iVertex, int iStage, float &s, float &t ) const
{
	Assert( iVertex >= 0 && iVertex < m_MaxVertices );
	float *p = (float*)( (char*)m_pTexCoord[iStage] + iVertex * m_VertexSize_TexCoord[iStage] );
	s = p[0];
	t = p[1];
}

inline const Vector2D& CMeshReader::TexCoordVector2D( int iVertex, int iStage ) const
{
	Assert( iVertex >= 0 && iVertex < m_MaxVertices );
	Vector2D *p = (Vector2D*)( (char*)m_pTexCoord[iStage] + iVertex * m_VertexSize_TexCoord[iStage] );
	return *p;
}

inline float CMeshReader::Wrinkle( int iVertex ) const
{
	Assert( iVertex >= 0 && iVertex < m_MaxVertices );
	return *(float*)( (char*)m_pWrinkle + iVertex * m_VertexSize_Wrinkle );
}

inline int CMeshReader::NumBoneWeights() const
{
	return m_NumBoneWeights;
}

inline const Vector &CMeshReader::Normal( int iVertex ) const
{
	Assert( iVertex >= 0 && iVertex < m_MaxVertices );
	return *(const Vector *)(const float*)( (char*)m_pNormal + iVertex  * m_VertexSize_Normal );
}

inline void CMeshReader::Normal( int iVertex, Vector &vNormal ) const
{
	Assert( iVertex >= 0 && iVertex < m_MaxVertices );
	const float *p = (const float*)( (char*)m_pNormal + iVertex * m_VertexSize_Normal );
	vNormal.Init( p[0], p[1], p[2] );
}

inline const Vector &CMeshReader::TangentS( int iVertex ) const
{
	Assert( iVertex >= 0 && iVertex < m_MaxVertices );
	return *(const Vector*)( (char*)m_pTangentS + iVertex * m_VertexSize_TangentS );
}

inline const Vector &CMeshReader::TangentT( int iVertex ) const
{
	Assert( iVertex >= 0 && iVertex < m_MaxVertices );
	return *(const Vector*)( (char*)m_pTangentT + iVertex * m_VertexSize_TangentT );
}

inline float CMeshReader::BoneWeight( int iVertex ) const
{
	Assert( iVertex >= 0 && iVertex < m_MaxVertices );
	float *p = (float*)( (char*)m_pBoneWeight + iVertex * m_VertexSize_BoneWeight );
	return *p;
}

#endif // MESHREADER_H







