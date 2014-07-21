//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef IMESH_H
#define IMESH_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"
#include "materialsystem/imaterial.h"
#include <float.h>
#include <string.h>
#include "tier0/dbg.h"
#include "tier2/meshutils.h"
#include "mathlib/mathlib.h"

#if defined( DX_TO_GL_ABSTRACTION )
// Swap these so that we do color swapping on 10.6.2, which doesn't have EXT_vertex_array_bgra
#define	OPENGL_SWAP_COLORS
#endif

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IMaterial;
class CMeshBuilder;
class IMaterialVar;
typedef uint64 VertexFormat_t;


//-----------------------------------------------------------------------------
// Define this to find write-combine problems
//-----------------------------------------------------------------------------
#ifdef _DEBUG
//#ifndef DEBUG_WRITE_COMBINE
//#define DEBUG_WRITE_COMBINE 1
//#endif
#endif


//-----------------------------------------------------------------------------
// The Vertex Buffer interface
//-----------------------------------------------------------------------------
enum
{
	VERTEX_MAX_TEXTURE_COORDINATES = 8,
	BONE_MATRIX_INDEX_INVALID = 255
};

// Internal maximums for sizes. Don't use directly, use IMaterialSystem::GetMaxToRender()
enum
{
	INDEX_BUFFER_SIZE  = 32768,
	DYNAMIC_VERTEX_BUFFER_MEMORY = ( 1024 + 512 ) * 1024,
	DYNAMIC_VERTEX_BUFFER_MEMORY_SMALL = 384 * 1024, // Only allocate this much during map transitions
};

// Vertex fields must be written in well-defined order to achieve write combining, 
// which is a perf booster
enum WriteCombineOrdering_t
{
	MB_FIELD_NONE = -1,
	MB_FIELD_POSITION = 0,
	MB_FIELD_BONE_WEIGHTS,
	MB_FIELD_BONE_INDEX,
	MB_FIELD_NORMAL,
	MB_FIELD_COLOR,
	MB_FIELD_SPECULAR,
	MB_FIELD_TEXCOORD_FIRST,
	MB_FIELD_TEXCOORD_LAST = MB_FIELD_TEXCOORD_FIRST + VERTEX_MAX_TEXTURE_COORDINATES - 1,
	MB_FIELD_TANGENT_S,
	MB_FIELD_TANGENT_T,
	MB_FIELD_USERDATA,
};

#define MB_FIELD_TEXCOORD( nStage ) ( MB_FIELD_TEXCOORD_FIRST + ( nStage ) )

struct VertexDesc_t
{
	// These can be set to zero if there are pointers to dummy buffers, when the
	// actual buffer format doesn't contain the data but it needs to be safe to
	// use all the CMeshBuilder functions.
	int	m_VertexSize_Position;
	int m_VertexSize_BoneWeight;
	int m_VertexSize_BoneMatrixIndex;
	int	m_VertexSize_Normal;
	int	m_VertexSize_Color;
	int	m_VertexSize_Specular;
	int m_VertexSize_TexCoord[VERTEX_MAX_TEXTURE_COORDINATES];
	int m_VertexSize_TangentS;
	int m_VertexSize_TangentT;
	int m_VertexSize_Wrinkle;

	int m_VertexSize_UserData;

	int m_ActualVertexSize;	// Size of the vertices.. Some of the m_VertexSize_ elements above
							// are set to this value and some are set to zero depending on which
							// fields exist in a buffer's vertex format.

	// The type of compression applied to this vertex data
	VertexCompressionType_t m_CompressionType;

	// Number of bone weights per vertex...
	int m_NumBoneWeights;

	// Pointers to our current vertex data
	float			*m_pPosition;

	float			*m_pBoneWeight;

#ifndef NEW_SKINNING
	unsigned char	*m_pBoneMatrixIndex;
#else
	float			*m_pBoneMatrixIndex;
#endif

	float			*m_pNormal;

	unsigned char	*m_pColor;
	unsigned char	*m_pSpecular;
	float			*m_pTexCoord[VERTEX_MAX_TEXTURE_COORDINATES];

	// Tangent space *associated with one particular set of texcoords*
	float			*m_pTangentS;
	float			*m_pTangentT;

	float			*m_pWrinkle;

	// user data
	float			*m_pUserData;

	// The first vertex index (used for buffered vertex buffers, or cards that don't support stream offset)
	int	m_nFirstVertex;

	// The offset in bytes of the memory we're writing into 
	// from the start of the D3D buffer (will be 0 for static meshes)
	unsigned int	m_nOffset;

#ifdef DEBUG_WRITE_COMBINE
	int m_nLastWrittenField;
	unsigned char* m_pLastWrittenAddress;
#endif
};

struct IndexDesc_t
{
	// Pointers to the index data
	unsigned short	*m_pIndices;

	// The offset in bytes of the memory we're writing into 
	// from the start of the D3D buffer (will be 0 for static meshes)
	unsigned int	m_nOffset;

	// The first index (used for buffered index buffers, or cards that don't support stream offset)
	unsigned int	m_nFirstIndex;

	// 1 if the device is active, 0 if the device isn't active.
	// Faster than doing if checks for null m_pIndices if someone is
	// trying to write the m_pIndices while the device is inactive.
	unsigned char m_nIndexSize;
};


//-----------------------------------------------------------------------------
// The Mesh memory descriptor
//-----------------------------------------------------------------------------
struct MeshDesc_t : public VertexDesc_t, public IndexDesc_t
{
};


//-----------------------------------------------------------------------------
// Standard vertex formats for models
//-----------------------------------------------------------------------------
struct ModelVertexDX7_t
{
	Vector			m_vecPosition;
	Vector2D		m_flBoneWeights;
	unsigned int	m_nBoneIndices;
	Vector			m_vecNormal;
	unsigned int	m_nColor;	// ARGB
	Vector2D		m_vecTexCoord;
};

struct ModelVertexDX8_t	: public ModelVertexDX7_t
{
	Vector4D		m_vecUserData;
};


//-----------------------------------------------------------------------------
// Utility methods for buffer builders
//-----------------------------------------------------------------------------
inline float *OffsetFloatPointer( float *pBufferPointer, int nVertexCount, int vertexSize )
{
	return reinterpret_cast<float *>(
		reinterpret_cast<unsigned char *>(pBufferPointer) + 
		nVertexCount * vertexSize);
}

inline const float *OffsetFloatPointer( const float *pBufferPointer, int nVertexCount, int vertexSize )
{
	return reinterpret_cast<const float*>(
		reinterpret_cast<unsigned char const*>(pBufferPointer) + 
		nVertexCount * vertexSize);
}

inline void IncrementFloatPointer( float* &pBufferPointer, int vertexSize )
{
	pBufferPointer = reinterpret_cast<float*>( reinterpret_cast<unsigned char*>( pBufferPointer ) + vertexSize );
}


//-----------------------------------------------------------------------------
// Used in lists of indexed primitives.
//-----------------------------------------------------------------------------
class CPrimList
{
public:
	CPrimList();
	CPrimList( int nFirstIndex, int nIndexCount );

	int			m_FirstIndex;
	int			m_NumIndices;
};

inline CPrimList::CPrimList()
{
}

inline CPrimList::CPrimList( int nFirstIndex, int nIndexCount )
{
	m_FirstIndex = nFirstIndex;
	m_NumIndices = nIndexCount;
}

abstract_class IVertexBuffer
{
public:
	// Add a virtual destructor to silence the clang warning.
	// This is harmless but not important since the only derived class
	// doesn't have a destructor.
	virtual ~IVertexBuffer() {}

	// NOTE: The following two methods are only valid for static vertex buffers
	// Returns the number of vertices and the format of the vertex buffer
	virtual int VertexCount() const = 0;
	virtual VertexFormat_t GetVertexFormat() const = 0;

	// Is this vertex buffer dynamic?
	virtual bool IsDynamic() const = 0;

	// NOTE: For dynamic vertex buffers only!
	// Casts the memory of the dynamic vertex buffer to the appropriate type
	virtual void BeginCastBuffer( VertexFormat_t format ) = 0;
	virtual void EndCastBuffer() = 0;

	// Returns the number of vertices that can still be written into the buffer
	virtual int GetRoomRemaining() const = 0;

	virtual bool Lock( int nVertexCount, bool bAppend, VertexDesc_t &desc ) = 0;
	virtual void Unlock( int nVertexCount, VertexDesc_t &desc ) = 0;

	// Spews the mesh data
	virtual void Spew( int nVertexCount, const VertexDesc_t &desc ) = 0;

	// Call this in debug mode to make sure our data is good.
	virtual void ValidateData( int nVertexCount, const VertexDesc_t & desc ) = 0;
};

abstract_class IIndexBuffer
{
public:
	// Add a virtual destructor to silence the clang warning.
	// This is harmless but not important since the only derived class
	// doesn't have a destructor.
	virtual ~IIndexBuffer() {}

	// NOTE: The following two methods are only valid for static index buffers
	// Returns the number of indices and the format of the index buffer
	virtual int IndexCount() const = 0;
	virtual MaterialIndexFormat_t IndexFormat() const = 0;

	// Is this index buffer dynamic?
	virtual bool IsDynamic() const = 0;

	// NOTE: For dynamic index buffers only!
	// Casts the memory of the dynamic index buffer to the appropriate type
	virtual void BeginCastBuffer( MaterialIndexFormat_t format ) = 0;
	virtual void EndCastBuffer() = 0;

	// Returns the number of indices that can still be written into the buffer
	virtual int GetRoomRemaining() const = 0;

	// Locks, unlocks the index buffer
	virtual bool Lock( int nMaxIndexCount, bool bAppend, IndexDesc_t &desc ) = 0;
	virtual void Unlock( int nWrittenIndexCount, IndexDesc_t &desc ) = 0;

	// FIXME: Remove this!!
	// Locks, unlocks the index buffer for modify
	virtual void ModifyBegin( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t& desc ) = 0;
	virtual void ModifyEnd( IndexDesc_t& desc ) = 0;

	// Spews the mesh data
	virtual void Spew( int nIndexCount, const IndexDesc_t &desc ) = 0;

	// Ensures the data in the index buffer is valid
	virtual void ValidateData( int nIndexCount, const IndexDesc_t &desc ) = 0;
};


//-----------------------------------------------------------------------------
// Interface to the mesh - needs to contain an IVertexBuffer and an IIndexBuffer to emulate old mesh behavior
//-----------------------------------------------------------------------------
abstract_class IMesh : public IVertexBuffer, public IIndexBuffer
{
public:
	// -----------------------------------

	// Sets/gets the primitive type
	virtual void SetPrimitiveType( MaterialPrimitiveType_t type ) = 0;

	// Draws the mesh
	virtual void Draw( int nFirstIndex = -1, int nIndexCount = 0 ) = 0;

	virtual void SetColorMesh( IMesh *pColorMesh, int nVertexOffset ) = 0;

	// Draw a list of (lists of) primitives. Batching your lists together that use
	// the same lightmap, material, vertex and index buffers with multipass shaders
	// can drastically reduce state-switching overhead.
	// NOTE: this only works with STATIC meshes.
	virtual void Draw( CPrimList *pLists, int nLists ) = 0;

	// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
	virtual void CopyToMeshBuilder( 
		int iStartVert,		// Which vertices to copy.
		int nVerts, 
		int iStartIndex,	// Which indices to copy.
		int nIndices, 
		int indexOffset,	// This is added to each index.
		CMeshBuilder &builder ) = 0;

	// Spews the mesh data
	virtual void Spew( int nVertexCount, int nIndexCount, const MeshDesc_t &desc ) = 0;

	// Call this in debug mode to make sure our data is good.
	virtual void ValidateData( int nVertexCount, int nIndexCount, const MeshDesc_t &desc ) = 0;

	// New version
	// Locks/unlocks the mesh, providing space for nVertexCount and nIndexCount.
	// nIndexCount of -1 means don't lock the index buffer...
	virtual void LockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc ) = 0;
	virtual void ModifyBegin( int nFirstVertex, int nVertexCount, int nFirstIndex, int nIndexCount, MeshDesc_t& desc ) = 0;
	virtual void ModifyEnd( MeshDesc_t& desc ) = 0;
	virtual void UnlockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc ) = 0;

	virtual void ModifyBeginEx( bool bReadOnly, int nFirstVertex, int nVertexCount, int nFirstIndex, int nIndexCount, MeshDesc_t &desc ) = 0;

	virtual void SetFlexMesh( IMesh *pMesh, int nVertexOffset ) = 0;

	virtual void DisableFlexMesh() = 0;

	virtual void MarkAsDrawn() = 0;

	virtual unsigned ComputeMemoryUsed() = 0;
};


#include "meshreader.h"

#define INVALID_BUFFER_OFFSET 0xFFFFFFFFUL

// flags for advancevertex optimization
#define VTX_HAVEPOS 1
#define VTX_HAVENORMAL 2
#define VTX_HAVECOLOR 4
#define VTX_HAVEALL ( VTX_HAVEPOS | VTX_HAVENORMAL | VTX_HAVECOLOR )


//-----------------------------------------------------------------------------
//
// Helper class used to define vertex buffers
//
//-----------------------------------------------------------------------------
class CVertexBuilder : private VertexDesc_t
{
public:
	CVertexBuilder();
	CVertexBuilder( IVertexBuffer *pVertexBuffer, VertexFormat_t fmt = 0 );
	~CVertexBuilder();

	// Begins, ends modification of the index buffer (returns true if the lock succeeded)
	// A lock may not succeed if append is set to true and there isn't enough room
	// NOTE: Append is only used with dynamic index buffers; it's ignored for static buffers
	bool Lock( int nMaxIndexCount, bool bAppend = false );
	void Unlock();

	// Spews the current data
	// NOTE: Can only be called during a lock/unlock block
	void SpewData();

	// Returns the number of indices we can fit into the buffer without needing to discard
	int GetRoomRemaining() const;

	// Binds this vertex buffer
	void Bind( IMatRenderContext *pContext, int nStreamID, VertexFormat_t usage = 0 );

	// Returns the byte offset
	int Offset() const;

	// This must be called before Begin, if a vertex buffer with a compressed format is to be used
	void SetCompressionType( VertexCompressionType_t compressionType );
	void ValidateCompressionType();

	void Begin( IVertexBuffer *pVertexBuffer, int nVertexCount, int *nFirstVertex );
	void Begin( IVertexBuffer *pVertexBuffer, int nVertexCount );

	// Use this when you're done writing
	// Set bDraw to true to call m_pMesh->Draw automatically.
	void End( bool bSpewData = false );

	// Locks the vertex buffer to modify existing data
	// Passing nVertexCount == -1 says to lock all the vertices for modification.
	void BeginModify( IVertexBuffer *pVertexBuffer, int nFirstVertex = 0, int nVertexCount = -1 );
	void EndModify( bool bSpewData = false );

	// returns the number of vertices
	int VertexCount() const;

	// Returns the total number of vertices across all Locks()
	int TotalVertexCount() const;

	// Resets the mesh builder so it points to the start of everything again
	void Reset();

	// Returns the size of the vertex
	int VertexSize() { return m_ActualVertexSize; }

	// returns the data size of a given texture coordinate
	int TextureCoordinateSize( int nTexCoordNumber ) { return m_VertexSize_TexCoord[ nTexCoordNumber ]; }

	// Returns the base vertex memory pointer
	void* BaseVertexData();

	// Selects the nth Vertex and Index 
	void SelectVertex( int idx );

	// Advances the current vertex and index by one
	void AdvanceVertex( void );
	template<int nFlags, int nNumTexCoords> void AdvanceVertexF( void );
	void AdvanceVertices( int nVerts );

	int GetCurrentVertex() const;
	int GetFirstVertex() const;

	// Data retrieval...
	const float *Position() const;

	const float *Normal() const;

	unsigned int Color() const;

	unsigned char *Specular() const;

	const float *TexCoord( int stage ) const;

	const float *TangentS() const;
	const float *TangentT() const;

	const float *BoneWeight() const;
	float Wrinkle() const;

	int NumBoneWeights() const;
#ifndef NEW_SKINNING
	unsigned char *BoneMatrix() const;
#else
	float *BoneMatrix() const;
#endif

	// position setting
	void Position3f( float x, float y, float z );
	void Position3fv( const float *v );

	// normal setting
	void Normal3f( float nx, float ny, float nz );
	void Normal3fv( const float *n );
	void NormalDelta3fv( const float *n );
	void NormalDelta3f( float nx, float ny, float nz );
	// normal setting (templatized for code which needs to support compressed vertices)
	template <VertexCompressionType_t T> void CompressedNormal3f( float nx, float ny, float nz );
	template <VertexCompressionType_t T> void CompressedNormal3fv( const float *n );

	// color setting
	void Color3f( float r, float g, float b );
	void Color3fv( const float *rgb );
	void Color4f( float r, float g, float b, float a );
	void Color4fv( const float *rgba );

	// Faster versions of color
	void Color3ub( unsigned char r, unsigned char g, unsigned char b );
	void Color3ubv( unsigned char const* rgb );
	void Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void Color4ubv( unsigned char const* rgba );

	// specular color setting
	void Specular3f( float r, float g, float b );
	void Specular3fv( const float *rgb );
	void Specular4f( float r, float g, float b, float a );
	void Specular4fv( const float *rgba );

	// Faster version of specular
	void Specular3ub( unsigned char r, unsigned char g, unsigned char b );
	void Specular3ubv( unsigned char const *c );
	void Specular4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void Specular4ubv( unsigned char const *c );

	// texture coordinate setting
	void TexCoord1f( int stage, float s );
	void TexCoord2f( int stage, float s, float t );
	void TexCoord2fv( int stage, const float *st );
	void TexCoord3f( int stage, float s, float t, float u );
	void TexCoord3fv( int stage, const float *stu );
	void TexCoord4f( int stage, float s, float t, float u, float w );
	void TexCoord4fv( int stage, const float *stuv );

	void TexCoordSubRect2f( int stage, float s, float t, float offsetS, float offsetT, float scaleS, float scaleT );
	void TexCoordSubRect2fv( int stage, const float *st, const float *offset, const float *scale );

	// tangent space 
	void TangentS3f( float sx, float sy, float sz );
	void TangentS3fv( const float* s );

	void TangentT3f( float tx, float ty, float tz );
	void TangentT3fv( const float* t );

	// Wrinkle
	void Wrinkle1f( float flWrinkle );

	// bone weights
	void BoneWeight( int idx, float weight );
	// bone weights (templatized for code which needs to support compressed vertices)
	template <VertexCompressionType_t T> void CompressedBoneWeight3fv( const float * pWeights );

	// bone matrix index
	void BoneMatrix( int idx, int matrixIndex );

	// Generic per-vertex data
	void UserData( const float* pData );
	// Generic per-vertex data (templatized for code which needs to support compressed vertices)
	template <VertexCompressionType_t T> void CompressedUserData( const float* pData );

	// Fast Vertex! No need to call advance vertex, and no random access allowed. 
	// WARNING - these are low level functions that are intended only for use
	// in the software vertex skinner.
	void FastVertex( const ModelVertexDX7_t &vertex );
	void FastVertexSSE( const ModelVertexDX7_t &vertex );

	// store 4 dx7 vertices fast. for special sse dx7 pipeline
	void Fast4VerticesSSE( 
		ModelVertexDX7_t const *vtx_a,
		ModelVertexDX7_t const *vtx_b,
		ModelVertexDX7_t const *vtx_c,
		ModelVertexDX7_t const *vtx_d);

	void FastVertex( const ModelVertexDX8_t &vertex );
	void FastVertexSSE( const ModelVertexDX8_t &vertex );

	// Add number of verts and current vert since FastVertex routines do not update.
	void FastAdvanceNVertices( int n );	

#if defined( _X360 )
	void VertexDX8ToX360( const ModelVertexDX8_t &vertex );
#endif

	// FIXME: Remove! Backward compat so we can use this from a CMeshBuilder.
	void AttachBegin( IMesh* pMesh, int nMaxVertexCount, const MeshDesc_t &desc );
	void AttachEnd();
	void AttachBeginModify( IMesh* pMesh, int nFirstVertex, int nVertexCount, const MeshDesc_t &desc );
	void AttachEndModify();

private:
	// The vertex buffer we're modifying
	IVertexBuffer *m_pVertexBuffer;

	// Used to make sure Begin/End calls and BeginModify/EndModify calls match.
	bool m_bModify;

	// Max number of indices and vertices
	int m_nMaxVertexCount;

	// Number of indices and vertices
	int m_nVertexCount;

	// The current vertex and index
	mutable int m_nCurrentVertex;

	// Optimization: Pointer to the current pos, norm, texcoord, and color
	mutable float			*m_pCurrPosition;
	mutable float			*m_pCurrNormal;
	mutable float			*m_pCurrTexCoord[VERTEX_MAX_TEXTURE_COORDINATES];
	mutable unsigned char	*m_pCurrColor;

	// Total number of vertices appended
	int m_nTotalVertexCount;

	// First vertex buffer offset + index
	unsigned int m_nBufferOffset;
	unsigned int m_nBufferFirstVertex;

#if ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 )
	// Debug checks to make sure we write userdata4/tangents AFTER normals
	bool m_bWrittenNormal   : 1;
	bool m_bWrittenUserData : 1;
#endif

	friend class CMeshBuilder;
};


//-----------------------------------------------------------------------------
//
// Inline methods of CVertexBuilder
//
//-----------------------------------------------------------------------------
inline CVertexBuilder::CVertexBuilder()
{
	m_pVertexBuffer = NULL;
	m_nBufferOffset = INVALID_BUFFER_OFFSET;
	m_nBufferFirstVertex = 0;
	m_nVertexCount = 0;
	m_nCurrentVertex = 0;
	m_nMaxVertexCount = 0;
	m_nTotalVertexCount = 0;
	m_CompressionType = VERTEX_COMPRESSION_INVALID;

#ifdef _DEBUG
	m_pCurrPosition = NULL;
	m_pCurrNormal = NULL;
	m_pCurrColor = NULL;
	memset( m_pCurrTexCoord, 0, sizeof( m_pCurrTexCoord ) );
	m_bModify = false;
#endif
}

inline CVertexBuilder::CVertexBuilder( IVertexBuffer *pVertexBuffer, VertexFormat_t fmt )
{
	m_pVertexBuffer = pVertexBuffer;
	m_nBufferOffset = INVALID_BUFFER_OFFSET;
	m_nBufferFirstVertex = 0;
	m_nVertexCount = 0;
	m_nCurrentVertex = 0;
	m_nMaxVertexCount = 0;
	m_nTotalVertexCount = 0;
	m_CompressionType = VERTEX_COMPRESSION_INVALID;

	if ( m_pVertexBuffer->IsDynamic() )
	{
		m_pVertexBuffer->BeginCastBuffer( fmt );
	}
	else
	{
		Assert( m_pVertexBuffer->GetVertexFormat() == fmt );
	}

#ifdef _DEBUG
	m_pCurrPosition = NULL;
	m_pCurrNormal = NULL;
	m_pCurrColor = NULL;
	memset( m_pCurrTexCoord, 0, sizeof( m_pCurrTexCoord ) );
	m_bModify = false;
#endif
}

inline CVertexBuilder::~CVertexBuilder()
{
	if ( m_pVertexBuffer && m_pVertexBuffer->IsDynamic() )
	{
		m_pVertexBuffer->EndCastBuffer();
	}
}

//-----------------------------------------------------------------------------
// Begins, ends modification of the index buffer
//-----------------------------------------------------------------------------
inline bool CVertexBuilder::Lock( int nMaxVertexCount, bool bAppend )
{
	Assert( m_pVertexBuffer );
	m_bModify = false;
	m_nMaxVertexCount = nMaxVertexCount;
	bool bFirstLock = ( m_nBufferOffset == INVALID_BUFFER_OFFSET );
	if ( bFirstLock )
	{
		bAppend = false;
	}
	if ( !bAppend )
	{
		m_nTotalVertexCount = 0;
	}

	// Lock the vertex buffer
	if ( !m_pVertexBuffer->Lock( m_nMaxVertexCount, bAppend, *this ) )
	{
		m_nMaxVertexCount = 0;
		return false;
	}

	Reset();

	if ( bFirstLock )
	{
		m_nBufferOffset = m_nOffset;
		m_nBufferFirstVertex = m_nFirstVertex;
	}

	return true;
}

inline void CVertexBuilder::Unlock()
{
	Assert( !m_bModify && m_pVertexBuffer );

#ifdef _DEBUG
	m_pVertexBuffer->ValidateData( m_nVertexCount, *this );
#endif

	m_pVertexBuffer->Unlock( m_nVertexCount, *this );
	m_nTotalVertexCount += m_nVertexCount;

	m_nMaxVertexCount = 0;

#ifdef _DEBUG
	// Null out our data...
	m_pCurrPosition = NULL;
	m_pCurrNormal = NULL;
	m_pCurrColor = NULL;
	memset( m_pCurrTexCoord, 0, sizeof( m_pCurrTexCoord ) );
	memset( static_cast<VertexDesc_t*>( this ), 0, sizeof(VertexDesc_t) );
#endif
}

inline void CVertexBuilder::SpewData()
{
	m_pVertexBuffer->Spew( m_nVertexCount, *this );
}


//-----------------------------------------------------------------------------
// Binds this vertex buffer
//-----------------------------------------------------------------------------
inline void CVertexBuilder::Bind( IMatRenderContext *pContext, int nStreamID, VertexFormat_t usage )
{
	if ( m_pVertexBuffer && ( m_nBufferOffset != INVALID_BUFFER_OFFSET ) )
	{
		pContext->BindVertexBuffer( nStreamID, m_pVertexBuffer, m_nBufferOffset,
			m_nFirstVertex, m_nTotalVertexCount, usage ? usage : m_pVertexBuffer->GetVertexFormat() );
	}
	else
	{
		pContext->BindVertexBuffer( nStreamID, NULL, 0, 0, 0, 0 );
	}
}


//-----------------------------------------------------------------------------
// Returns the byte offset
//-----------------------------------------------------------------------------
inline int CVertexBuilder::Offset() const
{
	return m_nBufferOffset;
}

inline int CVertexBuilder::GetFirstVertex() const
{
	return m_nBufferFirstVertex;
}

//-----------------------------------------------------------------------------
// Specify the type of vertex compression that this CMeshBuilder will perform
//-----------------------------------------------------------------------------
inline void CVertexBuilder::SetCompressionType( VertexCompressionType_t compressionType )
{
	// The real purpose of this method is to allow us to emit a Warning in Begin()
	m_CompressionType = compressionType;
}

inline void CVertexBuilder::ValidateCompressionType()
{
#ifdef _DEBUG
	VertexCompressionType_t vbCompressionType = CompressionType( m_pVertexBuffer->GetVertexFormat() );
	if ( vbCompressionType != VERTEX_COMPRESSION_NONE )
	{
		Assert( m_CompressionType == vbCompressionType );
		if ( m_CompressionType != vbCompressionType )
		{
			Warning( "ERROR: CVertexBuilder::SetCompressionType() must be called to specify the same vertex compression type (%s) as the vertex buffer being modified."
					 "Junk vertices will be rendered, or there will be a crash in CVertexBuilder!\n",
					  vbCompressionType == VERTEX_COMPRESSION_ON ? "VERTEX_COMPRESSION_ON" : "VERTEX_COMPRESSION_NONE" );
		}
		// Never use vertex compression for dynamic VBs (the conversions can really hurt perf)
		Assert(	!m_pVertexBuffer->IsDynamic() );
	}
#endif
}

inline void CVertexBuilder::Begin( IVertexBuffer *pVertexBuffer, int nVertexCount )
{
	Assert( pVertexBuffer && (!m_pVertexBuffer) );

	m_pVertexBuffer = pVertexBuffer;
	m_bModify = false;

	m_nMaxVertexCount = nVertexCount;
	m_nVertexCount = 0;

	// Make sure SetCompressionType was called correctly, if this VB is compressed
	ValidateCompressionType();

	// Lock the vertex and index buffer
	m_pVertexBuffer->Lock( m_nMaxVertexCount, false, *this );

	// Point to the start of the buffers..
	Reset();
}


//-----------------------------------------------------------------------------
// Use this when you're done modifying the mesh
//-----------------------------------------------------------------------------
inline void CVertexBuilder::End( bool bSpewData )
{
	// Make sure they called Begin()
	Assert( !m_bModify );

	if ( bSpewData )
	{
		m_pVertexBuffer->Spew( m_nVertexCount, *this );
	}

#ifdef _DEBUG
	m_pVertexBuffer->ValidateData( m_nVertexCount, *this );
#endif

	// Unlock our buffers
	m_pVertexBuffer->Unlock( m_nVertexCount, *this );

	m_pVertexBuffer = 0;
	m_nMaxVertexCount = 0;

	m_CompressionType = VERTEX_COMPRESSION_INVALID;

#ifdef _DEBUG
	// Null out our pointers...
	m_pCurrPosition = NULL;
	m_pCurrNormal = NULL;
	m_pCurrColor = NULL;
	memset( m_pCurrTexCoord, 0, sizeof( m_pCurrTexCoord ) );
	memset( static_cast< VertexDesc_t* >( this ), 0, sizeof(VertexDesc_t) );
#endif
}


//-----------------------------------------------------------------------------
// FIXME: Remove! Backward compat so we can use this from a CMeshBuilder.
//-----------------------------------------------------------------------------
inline void CVertexBuilder::AttachBegin( IMesh* pMesh, int nMaxVertexCount, const MeshDesc_t &desc )
{
	VertexCompressionType_t compressionType = m_CompressionType;

	m_pVertexBuffer = pMesh;
	memcpy( static_cast<VertexDesc_t*>( this ), static_cast<const VertexDesc_t*>( &desc ), sizeof(VertexDesc_t) );
	m_nMaxVertexCount = nMaxVertexCount;
	m_NumBoneWeights = m_NumBoneWeights == 0 ? 0 : 2;	// Two weights if any
	m_nVertexCount = 0;
	m_bModify = false;

	if ( compressionType != VERTEX_COMPRESSION_INVALID )
		m_CompressionType = compressionType;

	// Make sure SetCompressionType was called correctly, if this VB is compressed
	ValidateCompressionType();

	if ( m_nBufferOffset == INVALID_BUFFER_OFFSET )
	{
		m_nTotalVertexCount = 0;
		m_nBufferOffset = static_cast< const VertexDesc_t* >( &desc )->m_nOffset;
		m_nBufferFirstVertex = desc.m_nFirstVertex;
	}
}

inline void CVertexBuilder::AttachEnd()
{
	// Make sure they called Begin()
	Assert( !m_bModify );

	m_nMaxVertexCount = 0;
	m_pVertexBuffer = NULL;

	m_CompressionType = VERTEX_COMPRESSION_INVALID;

#ifdef _DEBUG
	// Null out our pointers...
	m_pCurrPosition = NULL;
	m_pCurrNormal = NULL;
	m_pCurrColor = NULL;
	memset( m_pCurrTexCoord, 0, sizeof( m_pCurrTexCoord ) );
	memset( static_cast<VertexDesc_t*>( this ), 0, sizeof(VertexDesc_t) );
#endif
}

inline void CVertexBuilder::AttachBeginModify( IMesh* pMesh, int nFirstVertex, int nVertexCount, const MeshDesc_t &desc )
{
	Assert( pMesh && (!m_pVertexBuffer) );

	m_pVertexBuffer = pMesh;
	memcpy( static_cast<VertexDesc_t*>( this ), static_cast<const VertexDesc_t*>( &desc ), sizeof(VertexDesc_t) );
	m_nMaxVertexCount = m_nVertexCount = nVertexCount;
	m_NumBoneWeights = m_NumBoneWeights == 0 ? 0 : 2;	// Two weights if any
	m_bModify = true;

	// Make sure SetCompressionType was called correctly, if this VB is compressed
	ValidateCompressionType();
}

inline void CVertexBuilder::AttachEndModify()
{
	Assert( m_pVertexBuffer );
	Assert( m_bModify );	// Make sure they called BeginModify.

	m_pVertexBuffer = 0;
	m_nMaxVertexCount = 0;

	m_CompressionType = VERTEX_COMPRESSION_INVALID;

#ifdef _DEBUG
	// Null out our pointers...
	m_pCurrPosition = NULL;
	m_pCurrNormal = NULL;
	m_pCurrColor = NULL;
	memset( m_pCurrTexCoord, 0, sizeof( m_pCurrTexCoord ) );
	memset( static_cast<VertexDesc_t*>( this ), 0, sizeof(VertexDesc_t) );
#endif
}


//-----------------------------------------------------------------------------
// Computes the first min non-null address
//-----------------------------------------------------------------------------
inline unsigned char* FindMinAddress( void *pAddress1, void *pAddress2, int nAddress2Size )
{
	if ( nAddress2Size == 0 )
		return (unsigned char*)pAddress1;
	if ( !pAddress1 )
		return (unsigned char*)pAddress2;
	return ( pAddress1 < pAddress2 ) ? (unsigned char*)pAddress1 : (unsigned char*)pAddress2;
}

//-----------------------------------------------------------------------------
// Resets the vertex buffer builder so it points to the start of everything again
//-----------------------------------------------------------------------------
inline void CVertexBuilder::Reset()
{
	m_nCurrentVertex = 0;

	m_pCurrPosition = m_pPosition;
	m_pCurrNormal = m_pNormal;
	for ( int i = 0; i < NELEMS( m_pCurrTexCoord ); i++ )
	{
		m_pCurrTexCoord[i] = m_pTexCoord[i];
	}
	m_pCurrColor = m_pColor;

#if ( defined( _DEBUG ) && ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 ) )
	m_bWrittenNormal   = false;
	m_bWrittenUserData = false;
#endif

#ifdef DEBUG_WRITE_COMBINE
	// Logic for m_pLastWrittenAddress is tricky. It really wants the min of the
	// non-null address pointers.
	m_nLastWrittenField = MB_FIELD_NONE;
	m_pLastWrittenAddress = NULL;
	m_pLastWrittenAddress =	FindMinAddress( m_pLastWrittenAddress, m_pPosition, m_VertexSize_Position );
	m_pLastWrittenAddress =	FindMinAddress( m_pLastWrittenAddress, m_pBoneWeight, m_VertexSize_BoneWeight );
	m_pLastWrittenAddress =	FindMinAddress( m_pLastWrittenAddress, m_pBoneMatrixIndex, m_VertexSize_BoneMatrixIndex );
	m_pLastWrittenAddress =	FindMinAddress( m_pLastWrittenAddress, m_pNormal, m_VertexSize_Normal );
	m_pLastWrittenAddress =	FindMinAddress( m_pLastWrittenAddress, m_pColor, m_VertexSize_Color );
	m_pLastWrittenAddress =	FindMinAddress( m_pLastWrittenAddress, m_pSpecular, m_VertexSize_Specular );
	for ( int i = 0; i < VERTEX_MAX_TEXTURE_COORDINATES; ++i )
	{
		m_pLastWrittenAddress =	FindMinAddress( m_pLastWrittenAddress, m_pTexCoord[i], m_VertexSize_TexCoord[i] );
	}
	m_pLastWrittenAddress =	FindMinAddress( m_pLastWrittenAddress, m_pTangentS, m_VertexSize_TangentS );
	m_pLastWrittenAddress =	FindMinAddress( m_pLastWrittenAddress, m_pTangentT, m_VertexSize_TangentT );
	m_pLastWrittenAddress =	FindMinAddress( m_pLastWrittenAddress, m_pUserData, m_VertexSize_UserData );
#endif
}


//-----------------------------------------------------------------------------
// returns the number of vertices
//-----------------------------------------------------------------------------
inline int CVertexBuilder::VertexCount() const
{
	return m_nVertexCount;
}


//-----------------------------------------------------------------------------
// Returns the total number of vertices across all Locks()
//-----------------------------------------------------------------------------
inline int CVertexBuilder::TotalVertexCount() const
{
	return m_nTotalVertexCount;
}


//-----------------------------------------------------------------------------
// Returns the base vertex memory pointer
//-----------------------------------------------------------------------------
inline void* CVertexBuilder::BaseVertexData()
{
	// FIXME: If there's no position specified, we need to find
	// the base address 
	Assert( m_pPosition );
	return m_pPosition;
}


//-----------------------------------------------------------------------------
// Selects the current vertex
//-----------------------------------------------------------------------------
inline void CVertexBuilder::SelectVertex( int nIndex )
{
	// NOTE: This index is expected to be relative 
	Assert( (nIndex >= 0) && (nIndex < m_nMaxVertexCount) );
	m_nCurrentVertex = nIndex;

	m_pCurrPosition = OffsetFloatPointer( m_pPosition, m_nCurrentVertex, m_VertexSize_Position );
	m_pCurrNormal = OffsetFloatPointer( m_pNormal, m_nCurrentVertex, m_VertexSize_Normal );

	COMPILE_TIME_ASSERT( VERTEX_MAX_TEXTURE_COORDINATES == 8 );
	m_pCurrTexCoord[0] = OffsetFloatPointer( m_pTexCoord[0], m_nCurrentVertex, m_VertexSize_TexCoord[0] );
	m_pCurrTexCoord[1] = OffsetFloatPointer( m_pTexCoord[1], m_nCurrentVertex, m_VertexSize_TexCoord[1] );
	m_pCurrTexCoord[2] = OffsetFloatPointer( m_pTexCoord[2], m_nCurrentVertex, m_VertexSize_TexCoord[2] );
	m_pCurrTexCoord[3] = OffsetFloatPointer( m_pTexCoord[3], m_nCurrentVertex, m_VertexSize_TexCoord[3] );
	m_pCurrTexCoord[4] = OffsetFloatPointer( m_pTexCoord[4], m_nCurrentVertex, m_VertexSize_TexCoord[4] );
	m_pCurrTexCoord[5] = OffsetFloatPointer( m_pTexCoord[5], m_nCurrentVertex, m_VertexSize_TexCoord[5] );
	m_pCurrTexCoord[6] = OffsetFloatPointer( m_pTexCoord[6], m_nCurrentVertex, m_VertexSize_TexCoord[6] );
	m_pCurrTexCoord[7] = OffsetFloatPointer( m_pTexCoord[7], m_nCurrentVertex, m_VertexSize_TexCoord[7] );
	m_pCurrColor = m_pColor + m_nCurrentVertex * m_VertexSize_Color;

#if ( defined( _DEBUG ) && ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 ) )
	m_bWrittenNormal   = false;
	m_bWrittenUserData = false;
#endif
}


//-----------------------------------------------------------------------------
// Advances vertex after you're done writing to it.
//-----------------------------------------------------------------------------

template<int nFlags, int nNumTexCoords> FORCEINLINE void CVertexBuilder::AdvanceVertexF()
{
	if ( ++m_nCurrentVertex > m_nVertexCount )
	{
		m_nVertexCount = m_nCurrentVertex;
	}

	if ( nFlags & VTX_HAVEPOS )
		IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position );
	if ( nFlags & VTX_HAVENORMAL )
		IncrementFloatPointer( m_pCurrNormal, m_VertexSize_Normal );
	if ( nFlags & VTX_HAVECOLOR )
		m_pCurrColor += m_VertexSize_Color;

	COMPILE_TIME_ASSERT( VERTEX_MAX_TEXTURE_COORDINATES == 8 );
	if ( nNumTexCoords > 0 )
		IncrementFloatPointer( m_pCurrTexCoord[0], m_VertexSize_TexCoord[0] );
	if ( nNumTexCoords > 1 )
		IncrementFloatPointer( m_pCurrTexCoord[1], m_VertexSize_TexCoord[1] );
	if ( nNumTexCoords > 2 )
		IncrementFloatPointer( m_pCurrTexCoord[2], m_VertexSize_TexCoord[2] );
	if ( nNumTexCoords > 3 )
		IncrementFloatPointer( m_pCurrTexCoord[3], m_VertexSize_TexCoord[3] );
	if ( nNumTexCoords > 4 )
		IncrementFloatPointer( m_pCurrTexCoord[4], m_VertexSize_TexCoord[4] );
	if ( nNumTexCoords > 5 )
		IncrementFloatPointer( m_pCurrTexCoord[5], m_VertexSize_TexCoord[5] );
	if ( nNumTexCoords > 6 )
		IncrementFloatPointer( m_pCurrTexCoord[6], m_VertexSize_TexCoord[6] );
	if ( nNumTexCoords > 7 )
		IncrementFloatPointer( m_pCurrTexCoord[7], m_VertexSize_TexCoord[7] );

#if ( defined( _DEBUG ) && ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 ) )
	m_bWrittenNormal   = false;
	m_bWrittenUserData = false;
#endif
}

inline void CVertexBuilder::AdvanceVertex()
{
	AdvanceVertexF<VTX_HAVEALL, 8>();
}


inline void CVertexBuilder::AdvanceVertices( int nVerts )
{
	m_nCurrentVertex += nVerts;
	if ( m_nCurrentVertex > m_nVertexCount )
	{
		m_nVertexCount = m_nCurrentVertex;
	}

	IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position*nVerts );
	IncrementFloatPointer( m_pCurrNormal, m_VertexSize_Normal*nVerts );

	COMPILE_TIME_ASSERT( VERTEX_MAX_TEXTURE_COORDINATES == 8 );
	IncrementFloatPointer( m_pCurrTexCoord[0], m_VertexSize_TexCoord[0]*nVerts );
	IncrementFloatPointer( m_pCurrTexCoord[1], m_VertexSize_TexCoord[1]*nVerts );
	IncrementFloatPointer( m_pCurrTexCoord[2], m_VertexSize_TexCoord[2]*nVerts );
	IncrementFloatPointer( m_pCurrTexCoord[3], m_VertexSize_TexCoord[3]*nVerts );
	IncrementFloatPointer( m_pCurrTexCoord[4], m_VertexSize_TexCoord[4]*nVerts );
	IncrementFloatPointer( m_pCurrTexCoord[5], m_VertexSize_TexCoord[5]*nVerts );
	IncrementFloatPointer( m_pCurrTexCoord[6], m_VertexSize_TexCoord[6]*nVerts );
	IncrementFloatPointer( m_pCurrTexCoord[7], m_VertexSize_TexCoord[7]*nVerts );
	m_pCurrColor += m_VertexSize_Color*nVerts;

#if ( defined( _DEBUG ) && ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 ) )
	m_bWrittenNormal   = false;
	m_bWrittenUserData = false;
#endif
}


//-----------------------------------------------------------------------------
// For use with the FastVertex methods, advances the current vertex by N
//-----------------------------------------------------------------------------
inline void CVertexBuilder::FastAdvanceNVertices( int n )
{
	m_nCurrentVertex += n;
	m_nVertexCount = m_nCurrentVertex;
}



#ifndef COMPILER_MSVC64
// Implement for 64-bit Windows if needed.
//-----------------------------------------------------------------------------
// Fast Vertex! No need to call advance vertex, and no random access allowed
//-----------------------------------------------------------------------------
inline void CVertexBuilder::FastVertex( const ModelVertexDX7_t &vertex )
{
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE ); // FIXME: support compressed verts if needed
	Assert( m_nCurrentVertex < m_nMaxVertexCount );

#if defined( _WIN32 ) && !defined( _X360 )
	const void *pRead = &vertex;
	void *pCurrPos = m_pCurrPosition;

	__asm
	{
		mov esi, pRead
			mov edi, pCurrPos

			movq mm0, [esi + 0]
		movq mm1, [esi + 8]
		movq mm2, [esi + 16]
		movq mm3, [esi + 24]
		movq mm4, [esi + 32]
		movq mm5, [esi + 40]

		movntq [edi + 0], mm0
			movntq [edi + 8], mm1
			movntq [edi + 16], mm2
			movntq [edi + 24], mm3
			movntq [edi + 32], mm4
			movntq [edi + 40], mm5

			emms
	}
#elif defined(GNUC)
	const void *pRead = &vertex;
	void *pCurrPos = m_pCurrPosition;
	__asm__ __volatile__ (
						  "movq (%0), %%mm0\n"
						  "movq 8(%0), %%mm1\n"
						  "movq 16(%0), %%mm2\n"
						  "movq 24(%0), %%mm3\n"
						  "movq 32(%0), %%mm4\n"
						  "movq 40(%0), %%mm5\n"
						  "movntq %%mm0, (%1)\n"
						  "movntq %%mm1, 8(%1)\n"
						  "movntq %%mm2, 16(%1)\n"
						  "movntq %%mm3, 24(%1)\n"
						  "movntq %%mm4, 32(%1)\n"
						  "movntq %%mm5, 40(%1)\n"
						  "emms\n"
						  :: "r" (pRead), "r" (pCurrPos) : "memory");
#else
	Error( "Implement CMeshBuilder::FastVertex(dx7) ");
#endif

	IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position );
	//m_nVertexCount = ++m_nCurrentVertex;

#if ( defined( _DEBUG ) && ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 ) )
	m_bWrittenNormal   = false;
	m_bWrittenUserData = false;
#endif
}

inline void CVertexBuilder::FastVertexSSE( const ModelVertexDX7_t &vertex )
{
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE ); // FIXME: support compressed verts if needed
	Assert( m_nCurrentVertex < m_nMaxVertexCount );

#if defined( _WIN32 ) && !defined( _X360 )
	const void *pRead = &vertex;
	void *pCurrPos = m_pCurrPosition;
	__asm
	{
		mov esi, pRead
			mov edi, pCurrPos

			movaps xmm0, [esi + 0]
		movaps xmm1, [esi + 16]
		movaps xmm2, [esi + 32]

		movntps [edi + 0], xmm0
			movntps [edi + 16], xmm1
			movntps [edi + 32], xmm2
	}
#elif defined(GNUC)
	const char *pRead = (char *)&vertex;
	char *pCurrPos = (char *)m_pCurrPosition;
	__m128 m1 = _mm_load_ps( (float *)pRead );
	__m128 m2 = _mm_load_ps( (float *)(pRead + 16) );
	__m128 m3 = _mm_load_ps( (float *)(pRead + 32) );
	_mm_stream_ps( (float *)pCurrPos, m1 );
	_mm_stream_ps( (float *)(pCurrPos + 16), m2 );
	_mm_stream_ps( (float *)(pCurrPos + 32), m3 );
#else
	Error( "Implement CMeshBuilder::FastVertexSSE(dx7)" );
#endif

	IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position );
	//m_nVertexCount = ++m_nCurrentVertex;

#if ( defined( _DEBUG ) && ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 ) )
	m_bWrittenNormal   = false;
	m_bWrittenUserData = false;
#endif
}

inline void CVertexBuilder::Fast4VerticesSSE( 
	ModelVertexDX7_t const *vtx_a,
	ModelVertexDX7_t const *vtx_b,
	ModelVertexDX7_t const *vtx_c,
	ModelVertexDX7_t const *vtx_d)
{
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE ); // FIXME: support compressed verts if needed
	Assert( m_nCurrentVertex < m_nMaxVertexCount-3 );

#if defined( _WIN32 ) && !defined( _X360 )
	void *pCurrPos = m_pCurrPosition;
	__asm
	{
		mov esi, vtx_a
			mov ecx, vtx_b

			mov edi, pCurrPos
			nop

			movaps xmm0, [esi + 0]
		movaps xmm1, [esi + 16]
		movaps xmm2, [esi + 32]
		movaps xmm3, [ecx + 0]
		movaps xmm4, [ecx + 16]
		movaps xmm5, [ecx + 32]

		mov esi, vtx_c
			mov ecx, vtx_d

			movntps [edi + 0], xmm0
			movntps [edi + 16], xmm1
			movntps [edi + 32], xmm2
			movntps [edi + 48], xmm3
			movntps [edi + 64], xmm4
			movntps [edi + 80], xmm5

			movaps xmm0, [esi + 0]
		movaps xmm1, [esi + 16]
		movaps xmm2, [esi + 32]
		movaps xmm3, [ecx + 0]
		movaps xmm4, [ecx + 16]
		movaps xmm5, [ecx + 32]

		movntps [edi + 0+96], xmm0
			movntps [edi + 16+96], xmm1
			movntps [edi + 32+96], xmm2
			movntps [edi + 48+96], xmm3
			movntps [edi + 64+96], xmm4
			movntps [edi + 80+96], xmm5

	}
#else
	Error( "Implement CMeshBuilder::Fast4VerticesSSE\n");
#endif
	IncrementFloatPointer( m_pCurrPosition, 4*m_VertexSize_Position );

#if ( defined( _DEBUG ) && ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 ) )
	m_bWrittenNormal   = false;
	m_bWrittenUserData = false;
#endif
}

inline void CVertexBuilder::FastVertex( const ModelVertexDX8_t &vertex )
{
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE ); // FIXME: support compressed verts if needed
	Assert( m_nCurrentVertex < m_nMaxVertexCount );

#if defined( _WIN32 ) && !defined( _X360 )
	const void *pRead = &vertex;
	void *pCurrPos = m_pCurrPosition;
	__asm
	{
		mov esi, pRead
			mov edi, pCurrPos

			movq mm0, [esi + 0]
		movq mm1, [esi + 8]
		movq mm2, [esi + 16]
		movq mm3, [esi + 24]
		movq mm4, [esi + 32]
		movq mm5, [esi + 40]
		movq mm6, [esi + 48]
		movq mm7, [esi + 56]

		movntq [edi + 0], mm0
			movntq [edi + 8], mm1
			movntq [edi + 16], mm2
			movntq [edi + 24], mm3
			movntq [edi + 32], mm4
			movntq [edi + 40], mm5
			movntq [edi + 48], mm6
			movntq [edi + 56], mm7

			emms
	}
#elif defined(GNUC)
	const void *pRead = &vertex;
	void *pCurrPos = m_pCurrPosition;
	__asm__ __volatile__ (
						  "movq (%0), %%mm0\n"
						  "movq 8(%0), %%mm1\n"
						  "movq 16(%0), %%mm2\n"
						  "movq 24(%0), %%mm3\n"
						  "movq 32(%0), %%mm4\n"
						  "movq 40(%0), %%mm5\n"
						  "movq 48(%0), %%mm6\n"
						  "movq 56(%0), %%mm7\n"
						  "movntq %%mm0, (%1)\n"
						  "movntq %%mm1, 8(%1)\n"
						  "movntq %%mm2, 16(%1)\n"
						  "movntq %%mm3, 24(%1)\n"
						  "movntq %%mm4, 32(%1)\n"
						  "movntq %%mm5, 40(%1)\n"
						  "movntq %%mm6, 48(%1)\n"
						  "movntq %%mm7, 56(%1)\n"
						  "emms\n"
						  :: "r" (pRead), "r" (pCurrPos) : "memory");
#else
	Error( "Implement CMeshBuilder::FastVertex(dx8)" );
#endif

	IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position );
	//	m_nVertexCount = ++m_nCurrentVertex;

#if ( defined( _DEBUG ) && ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 ) )
	m_bWrittenNormal   = false;
	m_bWrittenUserData = false;
#endif
}

inline void CVertexBuilder::FastVertexSSE( const ModelVertexDX8_t &vertex )
{
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE ); // FIXME: support compressed verts if needed
	Assert( m_nCurrentVertex < m_nMaxVertexCount );

#if defined( _WIN32 ) && !defined( _X360 )
	const void *pRead = &vertex;
	void *pCurrPos = m_pCurrPosition;
	__asm
	{
		mov esi, pRead
			mov edi, pCurrPos

			movaps xmm0, [esi + 0]
		movaps xmm1, [esi + 16]
		movaps xmm2, [esi + 32]
		movaps xmm3, [esi + 48]

		movntps [edi + 0], xmm0
			movntps [edi + 16], xmm1
			movntps [edi + 32], xmm2
			movntps [edi + 48], xmm3
	}
#elif defined(GNUC)
	const void *pRead = &vertex;
	void *pCurrPos = m_pCurrPosition;
	__asm__ __volatile__ (
						  "movaps (%0), %%xmm0\n"
						  "movaps 16(%0), %%xmm1\n"
						  "movaps 32(%0), %%xmm2\n"
						  "movaps 48(%0), %%xmm3\n"
						  "movntps %%xmm0, (%1)\n"
						  "movntps %%xmm1, 16(%1)\n"
						  "movntps %%xmm2, 32(%1)\n"
						  "movntps %%xmm3, 48(%1)\n"						  
						  :: "r" (pRead), "r" (pCurrPos) : "memory");
#else
	Error( "Implement CMeshBuilder::FastVertexSSE((dx8)" );
#endif

	IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position );
	//	m_nVertexCount = ++m_nCurrentVertex;

#if ( defined( _DEBUG ) && ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 ) )
	m_bWrittenNormal   = false;
	m_bWrittenUserData = false;
#endif
}
#endif // COMPILER_MSVC64


//-----------------------------------------------------------------------------
// Returns the current vertex
//-----------------------------------------------------------------------------
inline int CVertexBuilder::GetCurrentVertex() const
{
	return m_nCurrentVertex;
}


//-----------------------------------------------------------------------------
// Copies a vertex into the x360 format
//-----------------------------------------------------------------------------
#if defined( _X360 )
inline void CVertexBuilder::VertexDX8ToX360( const ModelVertexDX8_t &vertex )
{
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE ); // FIXME: support compressed verts if needed
	Assert( m_nCurrentVertex < m_nMaxVertexCount );

	// get the start of the data
	unsigned char *pDst = (unsigned char*)m_pCurrPosition;

	Assert( m_VertexSize_Position > 0 ); // Assume position is always present
	Assert( GetVertexElementSize( VERTEX_ELEMENT_POSITION, VERTEX_COMPRESSION_NONE ) == sizeof( vertex.m_vecPosition ) );
	memcpy( pDst, vertex.m_vecPosition.Base(), sizeof( vertex.m_vecPosition ) );
	pDst += sizeof( vertex.m_vecPosition );

	if ( m_VertexSize_BoneWeight )
	{
		Assert( vertex.m_flBoneWeights[0] >= 0 && vertex.m_flBoneWeights[0] <= 1.0f );
		Assert( vertex.m_flBoneWeights[1] >= 0 && vertex.m_flBoneWeights[1] <= 1.0f );
		Assert( GetVertexElementSize( VERTEX_ELEMENT_BONEWEIGHTS2, VERTEX_COMPRESSION_NONE ) == sizeof( vertex.m_flBoneWeights ) );
		memcpy( pDst, vertex.m_flBoneWeights.Base(), sizeof( vertex.m_flBoneWeights ) );
		pDst += sizeof( vertex.m_flBoneWeights );

		if ( m_VertexSize_BoneMatrixIndex )
		{
			Assert( GetVertexElementSize( VERTEX_ELEMENT_BONEINDEX, VERTEX_COMPRESSION_NONE ) == sizeof( vertex.m_nBoneIndices ) );
			*(unsigned int*)pDst = vertex.m_nBoneIndices;
			pDst += sizeof( vertex.m_nBoneIndices );
		}
	}

	if ( m_VertexSize_Normal )
	{
		Assert( GetVertexElementSize( VERTEX_ELEMENT_NORMAL, VERTEX_COMPRESSION_NONE ) == sizeof( vertex.m_vecNormal ) );
		memcpy( pDst, vertex.m_vecNormal.Base(), sizeof( vertex.m_vecNormal ) );
		pDst += sizeof( vertex.m_vecNormal );
	}

	if ( m_VertexSize_Color )
	{
		Assert( GetVertexElementSize( VERTEX_ELEMENT_COLOR, VERTEX_COMPRESSION_NONE ) == sizeof( vertex.m_nColor ) );
		*(unsigned int*)pDst = vertex.m_nColor;
		pDst += sizeof( vertex.m_nColor );
	}

	if ( m_VertexSize_TexCoord[0] )
	{
		Assert( GetVertexElementSize( VERTEX_ELEMENT_TEXCOORD2D_0, VERTEX_COMPRESSION_NONE ) == sizeof( vertex.m_vecTexCoord ) );
		memcpy( pDst, vertex.m_vecTexCoord.Base(), sizeof( vertex.m_vecTexCoord ) );
		pDst += sizeof( vertex.m_vecTexCoord );
	}

	if ( m_VertexSize_UserData )
	{
		Assert( GetVertexElementSize( VERTEX_ELEMENT_USERDATA4, VERTEX_COMPRESSION_NONE ) == sizeof( vertex.m_vecUserData ) );
		memcpy( pDst, vertex.m_vecUserData.Base(), sizeof( vertex.m_vecUserData ) );
		pDst += sizeof( vertex.m_vecUserData );
	}

	// ensure code is synced with the mesh builder that established the offsets
	Assert( pDst - (unsigned char*)m_pCurrPosition == m_VertexSize_Position );

	IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position );

#if ( defined( _DEBUG ) && ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 ) )
	m_bWrittenNormal   = false;
	m_bWrittenUserData = false;
#endif
}
#endif


//-----------------------------------------------------------------------------
// Data retrieval...
//-----------------------------------------------------------------------------
inline const float* CVertexBuilder::Position() const
{
	// FIXME: add a templatized accessor (return type varies to ensure calling code is updated appropriately)
	//        for code that needs to access compressed data (and/or a return-by-value templatized accessor)
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE );
	Assert( m_nCurrentVertex < m_nMaxVertexCount );
	return m_pCurrPosition;
}

inline const float* CVertexBuilder::Normal() const
{
	// FIXME: add a templatized accessor (return type varies to ensure calling code is updated appropriately)
	//        for code that needs to access compressed data (and/or a return-by-value templatized accessor)
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE );
	Assert( m_nCurrentVertex < m_nMaxVertexCount );
	return m_pCurrNormal;
}

inline unsigned int CVertexBuilder::Color() const
{
	// FIXME: add a templatized accessor (return type varies to ensure calling code is updated appropriately)
	//        for code that needs to access compressed data (and/or a return-by-value templatized accessor)
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE );
	// Swizzle it so it returns the same format as accepted by Color4ubv - rgba
	Assert( m_nCurrentVertex < m_nMaxVertexCount );
	unsigned int color;
	if ( IsPC() || !IsX360() )
	{
		color = (m_pCurrColor[3] << 24) | (m_pCurrColor[0] << 16) | (m_pCurrColor[1] << 8) | (m_pCurrColor[2]);
	}
	else
	{
		// in memory as argb, back to rgba
		color = (m_pCurrColor[1] << 24) | (m_pCurrColor[2] << 16) | (m_pCurrColor[3] << 8) | (m_pCurrColor[0]);
	}
	return color;
}

inline unsigned char *CVertexBuilder::Specular() const
{
	// FIXME: add a templatized accessor (return type varies to ensure calling code is updated appropriately)
	//        for code that needs to access compressed data (and/or a return-by-value templatized accessor)
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE );
	Assert( m_nCurrentVertex < m_nMaxVertexCount );
	return m_pSpecular + m_nCurrentVertex * m_VertexSize_Specular;
}

inline const float* CVertexBuilder::TexCoord( int stage ) const
{
	// FIXME: add a templatized accessor (return type varies to ensure calling code is updated appropriately)
	//        for code that needs to access compressed data (and/or a return-by-value templatized accessor)
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE );
	Assert( m_nCurrentVertex < m_nMaxVertexCount );
	return m_pCurrTexCoord[stage];
}

inline const float* CVertexBuilder::TangentS() const
{
	// FIXME: add a templatized accessor (return type varies to ensure calling code is updated appropriately)
	//        for code that needs to access compressed data (and/or a return-by-value templatized accessor)
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE );
	Assert( m_nCurrentVertex < m_nMaxVertexCount );
	return OffsetFloatPointer( m_pTangentS, m_nCurrentVertex, m_VertexSize_TangentS );
}

inline const float* CVertexBuilder::TangentT() const
{
	// FIXME: add a templatized accessor (return type varies to ensure calling code is updated appropriately)
	//        for code that needs to access compressed data (and/or a return-by-value templatized accessor)
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE );
	Assert( m_nCurrentVertex < m_nMaxVertexCount );
	return OffsetFloatPointer( m_pTangentT, m_nCurrentVertex, m_VertexSize_TangentT );
}

inline float CVertexBuilder::Wrinkle() const
{
	// FIXME: add a templatized accessor (return type varies to ensure calling code is updated appropriately)
	//        for code that needs to access compressed data (and/or a return-by-value templatized accessor)
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE );
	Assert( m_nCurrentVertex < m_nMaxVertexCount );
	return *OffsetFloatPointer( m_pWrinkle, m_nCurrentVertex, m_VertexSize_Wrinkle );
}

inline const float* CVertexBuilder::BoneWeight() const
{
	// FIXME: add a templatized accessor (return type varies to ensure calling code is updated appropriately)
	//        for code that needs to access compressed data (and/or a return-by-value templatized accessor)
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE );
	Assert( m_nCurrentVertex < m_nMaxVertexCount );
	return OffsetFloatPointer( m_pBoneWeight, m_nCurrentVertex, m_VertexSize_BoneWeight );
}

inline int CVertexBuilder::NumBoneWeights() const
{
	return m_NumBoneWeights;
}

#ifndef NEW_SKINNING
inline unsigned char* CVertexBuilder::BoneMatrix() const
{
	// FIXME: add a templatized accessor (return type varies to ensure calling code is updated appropriately)
	//        for code that needs to access compressed data (and/or a return-by-value templatized accessor)
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE );
	Assert( m_nCurrentVertex < m_nMaxVertexCount );
	return m_pBoneMatrixIndex + m_nCurrentVertex * m_VertexSize_BoneMatrixIndex;
}
#else
inline float* CVertexBuilder::BoneMatrix() const
{
	// FIXME: add a templatized accessor (return type varies to ensure calling code is updated appropriately)
	//        for code that needs to access compressed data (and/or a return-by-value templatized accessor)
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE );
	Assert( m_nCurrentVertex < m_nMaxVertexCount );
	return m_pBoneMatrixIndex + m_nCurrentVertex * m_VertexSize_BoneMatrixIndex;
}
#endif


//-----------------------------------------------------------------------------
// Position setting methods
//-----------------------------------------------------------------------------
inline void	CVertexBuilder::Position3f( float x, float y, float z )
{
	Assert( m_pPosition && m_pCurrPosition );
	Assert( IsFinite(x) && IsFinite(y) && IsFinite(z) );
	float *pDst = m_pCurrPosition;
	*pDst++ = x;
	*pDst++ = y;
	*pDst = z;
}

inline void	CVertexBuilder::Position3fv( const float *v )
{
	Assert(v);
	Assert( m_pPosition && m_pCurrPosition );

	float *pDst = m_pCurrPosition;
	*pDst++ = *v++;
	*pDst++ = *v++;
	*pDst = *v;
}


//-----------------------------------------------------------------------------
// Normal setting methods
//-----------------------------------------------------------------------------
inline void	CVertexBuilder::Normal3f( float nx, float ny, float nz )
{
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE ); // Use the templatized version if you want to support compression
	Assert( m_pNormal );
	Assert( IsFinite(nx) && IsFinite(ny) && IsFinite(nz) );
	Assert( nx >= -1.05f && nx <= 1.05f );
	Assert( ny >= -1.05f && ny <= 1.05f );
	Assert( nz >= -1.05f && nz <= 1.05f );

	float *pDst = m_pCurrNormal;
	*pDst++ = nx;
	*pDst++ = ny;
	*pDst = nz;
}

inline void	CVertexBuilder::Normal3fv( const float *n )
{
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE ); // Use the templatized version if you want to support compression
	Assert( n );
	Assert( m_pNormal && m_pCurrNormal );
	Assert( IsFinite(n[0]) && IsFinite(n[1]) && IsFinite(n[2]) );
	Assert( n[0] >= -1.05f && n[0] <= 1.05f );
	Assert( n[1] >= -1.05f && n[1] <= 1.05f );
	Assert( n[2] >= -1.05f && n[2] <= 1.05f );

	float *pDst = m_pCurrNormal;
	*pDst++ = *n++;
	*pDst++ = *n++;
	*pDst = *n;
}

inline void	CVertexBuilder::NormalDelta3f( float nx, float ny, float nz )
{
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE ); // Use the templatized version if you want to support compression
	Assert( m_pNormal );
	Assert( IsFinite(nx) && IsFinite(ny) && IsFinite(nz) );

	float *pDst = m_pCurrNormal;
	*pDst++ = nx;
	*pDst++ = ny;
	*pDst = nz;
}

inline void	CVertexBuilder::NormalDelta3fv( const float *n )
{
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE ); // Use the templatized version if you want to support compression
	Assert( n );
	Assert( m_pNormal && m_pCurrNormal );
	Assert( IsFinite(n[0]) && IsFinite(n[1]) && IsFinite(n[2]) );

	float *pDst = m_pCurrNormal;
	*pDst++ = *n++;
	*pDst++ = *n++;
	*pDst = *n;
}

//-----------------------------------------------------------------------------
// Templatized normal setting methods which support compressed vertices
//-----------------------------------------------------------------------------
template <VertexCompressionType_t T> inline void CVertexBuilder::CompressedNormal3f( float nx, float ny, float nz )
{
	Assert( T == m_CompressionType );
	Assert( m_pNormal && m_pCurrNormal );
	Assert( IsFinite(nx) && IsFinite(ny) && IsFinite(nz) );
	Assert( nx >= -1.05f && nx <= 1.05f );
	Assert( ny >= -1.05f && ny <= 1.05f );
	Assert( nz >= -1.05f && nz <= 1.05f );
	// FIXME: studiorender is passing in non-unit normals
	//float lengthSqd = nx*nx + ny*ny + nz*nz;
	//Assert( lengthSqd >= 0.95f && lengthSqd <= 1.05f );

	if ( T == VERTEX_COMPRESSION_ON )
	{
#if		( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_SEPARATETANGENTS_SHORT2 )
		PackNormal_SHORT2( nx, ny, nz, (unsigned int *)m_pCurrNormal );

#else //( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 )
		// NOTE: write the normal into the lower 16 bits of a word, clearing the top 16 bits - a userdata4
		//       tangent must be written into the upper 16 bits by CompressedUserData() *AFTER* this.
#ifdef _DEBUG
		Assert( m_bWrittenUserData == false );
		m_bWrittenNormal = true;
#endif
		PackNormal_UBYTE4( nx, ny, nz, (unsigned int *)m_pCurrNormal );
#endif
	}
	else
	{
		float *pDst = m_pCurrNormal;
		*pDst++ = nx;
		*pDst++ = ny;
		*pDst = nz;
	}
}

template <VertexCompressionType_t T> inline void CVertexBuilder::CompressedNormal3fv( const float *n )
{
	Assert( n );
	CompressedNormal3f<T>( n[0], n[1], n[2] );
}


//-----------------------------------------------------------------------------
// Color setting methods
//-----------------------------------------------------------------------------
inline void	CVertexBuilder::Color3f( float r, float g, float b )
{
	Assert( m_pColor && m_pCurrColor );
	Assert( IsFinite(r) && IsFinite(g) && IsFinite(b) );
	Assert( (r >= 0.0) && (g >= 0.0) && (b >= 0.0) );
	Assert( (r <= 1.0) && (g <= 1.0) && (b <= 1.0) );

#ifdef OPENGL_SWAP_COLORS
	int col = (FastFToC(r)) | (FastFToC(g) << 8) | (FastFToC(b) << 16) | 0xFF000000;
#else
	int col = (FastFToC(b)) | (FastFToC(g) << 8) | (FastFToC(r) << 16) | 0xFF000000;
#endif
	*(int*)m_pCurrColor = col;
}

inline void	CVertexBuilder::Color3fv( const float *rgb )
{
	Assert(rgb);
	Assert( m_pColor && m_pCurrColor );
	Assert( IsFinite(rgb[0]) && IsFinite(rgb[1]) && IsFinite(rgb[2]) );
	Assert( (rgb[0] >= 0.0) && (rgb[1] >= 0.0) && (rgb[2] >= 0.0) );
	Assert( (rgb[0] <= 1.0) && (rgb[1] <= 1.0) && (rgb[2] <= 1.0) );

#ifdef OPENGL_SWAP_COLORS	
	int col = (FastFToC(rgb[0])) | (FastFToC(rgb[1]) << 8) | (FastFToC(rgb[2]) << 16) | 0xFF000000;
#else
	int col = (FastFToC(rgb[2])) | (FastFToC(rgb[1]) << 8) | (FastFToC(rgb[0]) << 16) | 0xFF000000;
#endif
	*(int*)m_pCurrColor = col;
}

inline void	CVertexBuilder::Color4f( float r, float g, float b, float a )
{
	Assert( m_pColor && m_pCurrColor );
	Assert( IsFinite(r) && IsFinite(g) && IsFinite(b) && IsFinite(a) );
	Assert( (r >= 0.0) && (g >= 0.0) && (b >= 0.0) && (a >= 0.0) );
	Assert( (r <= 1.0) && (g <= 1.0) && (b <= 1.0) && (a <= 1.0) );

#ifdef OPENGL_SWAP_COLORS
	int col = (FastFToC(r)) | (FastFToC(g) << 8) | (FastFToC(b) << 16) | (FastFToC(a) << 24);
#else
	int col = (FastFToC(b)) | (FastFToC(g) << 8) | (FastFToC(r) << 16) | (FastFToC(a) << 24);
#endif
	*(int*)m_pCurrColor = col;
}

inline void	CVertexBuilder::Color4fv( const float *rgba )
{
	Assert(rgba);
	Assert( m_pColor && m_pCurrColor );
	Assert( IsFinite(rgba[0]) && IsFinite(rgba[1]) && IsFinite(rgba[2]) && IsFinite(rgba[3]) );
	Assert( (rgba[0] >= 0.0) && (rgba[1] >= 0.0) && (rgba[2] >= 0.0) && (rgba[3] >= 0.0) );
	Assert( (rgba[0] <= 1.0) && (rgba[1] <= 1.0) && (rgba[2] <= 1.0) && (rgba[3] <= 1.0) );

#ifdef OPENGL_SWAP_COLORS
	int col = (FastFToC(rgba[0])) | (FastFToC(rgba[1]) << 8) | (FastFToC(rgba[2]) << 16) | (FastFToC(rgba[3]) << 24);
#else
	int col = (FastFToC(rgba[2])) | (FastFToC(rgba[1]) << 8) | (FastFToC(rgba[0]) << 16) | (FastFToC(rgba[3]) << 24);
#endif
	*(int*)m_pCurrColor = col;
}


//-----------------------------------------------------------------------------
// Faster versions of color
//-----------------------------------------------------------------------------

// note that on the OSX target (OpenGL) whenever there is vertex data being written as bytes - they need to be written in R,G,B,A memory order

inline void CVertexBuilder::Color3ub( unsigned char r, unsigned char g, unsigned char b )
{
	Assert( m_pColor && m_pCurrColor );
	#ifdef OPENGL_SWAP_COLORS
		int col = r | (g << 8) | (b << 16) | 0xFF000000;	// r, g, b, a in memory
	#else
		int col = b | (g << 8) | (r << 16) | 0xFF000000;
	#endif
	
	*(int*)m_pCurrColor = col;
}

inline void CVertexBuilder::Color3ubv( unsigned char const* rgb )
{
	Assert(rgb);
	Assert( m_pColor && m_pCurrColor );
	#ifdef OPENGL_SWAP_COLORS
		int col = rgb[0] | (rgb[1] << 8) | (rgb[2] << 16) | 0xFF000000;	// r, g, b, a in memory
	#else
		int col = rgb[2] | (rgb[1] << 8) | (rgb[0] << 16) | 0xFF000000;
	#endif

	*(int*)m_pCurrColor = col;
}

inline void CVertexBuilder::Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	Assert( m_pColor && m_pCurrColor );
	#ifdef OPENGL_SWAP_COLORS
		int col = r | (g << 8) | (b << 16) | (a << 24);	// r, g, b, a in memory
	#else
		int col = b | (g << 8) | (r << 16) | (a << 24);
	#endif

	*(int*)m_pCurrColor = col;
}

inline void CVertexBuilder::Color4ubv( unsigned char const* rgba )
{
	Assert( rgba );
	Assert( m_pColor && m_pCurrColor );
	#ifdef OPENGL_SWAP_COLORS
		int col = rgba[0] | (rgba[1] << 8) | (rgba[2] << 16) | (rgba[3] << 24);	// r, g, b, a in memory
	#else
		int col = rgba[2] | (rgba[1] << 8) | (rgba[0] << 16) | (rgba[3] << 24);
	#endif
	*(int*)m_pCurrColor = col;
}

inline void	CVertexBuilder::Specular3f( float r, float g, float b )
{
	Assert( m_pSpecular );
	Assert( IsFinite(r) && IsFinite(g) && IsFinite(b) );
	Assert( (r >= 0.0) && (g >= 0.0) && (b >= 0.0) );
	Assert( (r <= 1.0) && (g <= 1.0) && (b <= 1.0) );

	unsigned char* pSpecular = &m_pSpecular[m_nCurrentVertex * m_VertexSize_Specular];
#ifdef OPENGL_SWAP_COLORS
	int col = (FastFToC(r)) | (FastFToC(g) << 8) | (FastFToC(b) << 16) | 0xFF000000;
#else
	int col = (FastFToC(b)) | (FastFToC(g) << 8) | (FastFToC(r) << 16) | 0xFF000000;
#endif
	*(int*)pSpecular = col;
}

inline void	CVertexBuilder::Specular3fv( const float *rgb )
{
	Assert(rgb);
	Assert( m_pSpecular );
	Assert( IsFinite(rgb[0]) && IsFinite(rgb[1]) && IsFinite(rgb[2]) );
	Assert( (rgb[0] >= 0.0) && (rgb[1] >= 0.0) && (rgb[2] >= 0.0) );
	Assert( (rgb[0] <= 1.0) && (rgb[1] <= 1.0) && (rgb[2] <= 1.0) );

	unsigned char* pSpecular = &m_pSpecular[m_nCurrentVertex * m_VertexSize_Specular];
#ifdef OPENGL_SWAP_COLORS
	int col = (FastFToC(rgb[0])) | (FastFToC(rgb[1]) << 8) | (FastFToC(rgb[2]) << 16) | 0xFF000000;
#else
	int col = (FastFToC(rgb[2])) | (FastFToC(rgb[1]) << 8) | (FastFToC(rgb[0]) << 16) | 0xFF000000;
#endif
	*(int*)pSpecular = col;
}

inline void	CVertexBuilder::Specular4f( float r, float g, float b, float a )
{
	Assert( m_pSpecular );
	Assert( IsFinite(r) && IsFinite(g) && IsFinite(b) && IsFinite(a) );
	Assert( (r >= 0.0) && (g >= 0.0) && (b >= 0.0) && (a >= 0.0) );
	Assert( (r <= 1.0) && (g <= 1.0) && (b <= 1.0) && (a <= 1.0f) );

	unsigned char* pSpecular = &m_pSpecular[m_nCurrentVertex * m_VertexSize_Specular];
#ifdef OPENGL_SWAP_COLORS
	int col = (FastFToC(r)) | (FastFToC(g) << 8) | (FastFToC(b) << 16) | (FastFToC(a) << 24);
#else
	int col = (FastFToC(b)) | (FastFToC(g) << 8) | (FastFToC(r) << 16) | (FastFToC(a) << 24);
#endif
	*(int*)pSpecular = col;
}

inline void	CVertexBuilder::Specular4fv( const float *rgb )
{
	Assert(rgb);
	Assert( m_pSpecular );
	Assert( IsFinite(rgb[0]) && IsFinite(rgb[1]) && IsFinite(rgb[2]) && IsFinite(rgb[3]) );
	Assert( (rgb[0] >= 0.0) && (rgb[1] >= 0.0) && (rgb[2] >= 0.0) && (rgb[3] >= 0.0) );
	Assert( (rgb[0] <= 1.0) && (rgb[1] <= 1.0) && (rgb[2] <= 1.0) && (rgb[3] <= 1.0) );

	unsigned char* pSpecular = &m_pSpecular[m_nCurrentVertex * m_VertexSize_Specular];
#ifdef OPENGL_SWAP_COLORS
	int col = (FastFToC(rgb[0])) | (FastFToC(rgb[1]) << 8) | (FastFToC(rgb[2]) << 16) | (FastFToC(rgb[3]) << 24);
#else
	int col = (FastFToC(rgb[2])) | (FastFToC(rgb[1]) << 8) | (FastFToC(rgb[0]) << 16) | (FastFToC(rgb[3]) << 24);
#endif
	*(int*)pSpecular = col;
}

inline void CVertexBuilder::Specular3ub( unsigned char r, unsigned char g, unsigned char b )
{
	Assert( m_pSpecular );
	unsigned char *pSpecular = &m_pSpecular[m_nCurrentVertex * m_VertexSize_Specular];

	#ifdef OPENGL_SWAP_COLORS
		int col = r | (g << 8) | (b << 16) | 0xFF000000;	// r, g, b, a in memory
	#else
		int col = b | (g << 8) | (r << 16) | 0xFF000000;
	#endif
	
	*(int*)pSpecular = col;
}

inline void CVertexBuilder::Specular3ubv( unsigned char const *c )
{
	Assert( m_pSpecular );
	unsigned char *pSpecular = &m_pSpecular[m_nCurrentVertex * m_VertexSize_Specular];

	#ifdef OPENGL_SWAP_COLORS
		int col = c[0] | (c[1] << 8) | (c[2] << 16) | 0xFF000000;	// r, g, b, a in memory
	#else
		int col = c[2] | (c[1] << 8) | (c[0] << 16) | 0xFF000000;
	#endif
	
	*(int*)pSpecular = col;
}

inline void CVertexBuilder::Specular4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	Assert( m_pSpecular );
	unsigned char *pSpecular = &m_pSpecular[m_nCurrentVertex * m_VertexSize_Specular];

	#ifdef OPENGL_SWAP_COLORS
		int col = r | (g << 8) | (b << 16) | (a << 24);	// r, g, b, a in memory
	#else
		int col = b | (g << 8) | (r << 16) | (a << 24);
	#endif

	*(int*)pSpecular = col;
}

inline void CVertexBuilder::Specular4ubv( unsigned char const *c )
{
	Assert( m_pSpecular );
	unsigned char *pSpecular = &m_pSpecular[m_nCurrentVertex * m_VertexSize_Specular];

	#ifdef OPENGL_SWAP_COLORS
		int col = c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24);
	#else
		int col = c[2] | (c[1] << 8) | (c[0] << 16) | (c[3] << 24);
	#endif
	
	*(int*)pSpecular = col;
}


//-----------------------------------------------------------------------------
// Texture coordinate setting methods
//-----------------------------------------------------------------------------
inline void CVertexBuilder::TexCoord1f( int nStage, float s )
{
	Assert( m_pTexCoord[nStage] && m_pCurrTexCoord[nStage] );
	Assert( IsFinite(s) );

	float *pDst = m_pCurrTexCoord[nStage];
	*pDst = s;
}

inline void	CVertexBuilder::TexCoord2f( int nStage, float s, float t )
{
	Assert( m_pTexCoord[nStage] && m_pCurrTexCoord[nStage] );
	Assert( IsFinite(s) && IsFinite(t) );

	float *pDst = m_pCurrTexCoord[nStage];
	*pDst++ = s;
	*pDst = t;
}

inline void	CVertexBuilder::TexCoord2fv( int nStage, const float *st )
{
	Assert(st);
	Assert( m_pTexCoord[nStage] && m_pCurrTexCoord[nStage] );
	Assert( IsFinite(st[0]) && IsFinite(st[1]) );

	float *pDst = m_pCurrTexCoord[nStage];
	*pDst++ = *st++;
	*pDst = *st;
}

inline void	CVertexBuilder::TexCoord3f( int stage, float s, float t, float u )
{
	// Tried to add too much!
	Assert( m_pTexCoord[stage] && m_pCurrTexCoord[stage] );
	Assert( IsFinite(s) && IsFinite(t)  && IsFinite(u) );
	float *pDst = m_pCurrTexCoord[stage];
	*pDst++ = s;
	*pDst++ = t;
	*pDst = u;
}

inline void	CVertexBuilder::TexCoord3fv( int stage, const float *stu )
{
	Assert(stu);
	Assert( m_pTexCoord[stage] && m_pCurrTexCoord[stage] );
	Assert( IsFinite(stu[0]) && IsFinite(stu[1]) && IsFinite(stu[2]) );

	float *pDst = m_pCurrTexCoord[stage];
	*pDst++ = *stu++;
	*pDst++ = *stu++;
	*pDst = *stu;
}

inline void	CVertexBuilder::TexCoord4f( int stage, float s, float t, float u, float v )
{
	// Tried to add too much!
	Assert( m_pTexCoord[stage] && m_pCurrTexCoord[stage] );
	Assert( IsFinite(s) && IsFinite(t)  && IsFinite(u) );
	float *pDst = m_pCurrTexCoord[stage];
	*pDst++ = s;
	*pDst++ = t;
	*pDst++ = u;
	*pDst = v;
}

inline void	CVertexBuilder::TexCoord4fv( int stage, const float *stuv )
{
	Assert(stuv);
	Assert( m_pTexCoord[stage] && m_pCurrTexCoord[stage] );
	Assert( IsFinite(stuv[0]) && IsFinite(stuv[1]) && IsFinite(stuv[2]) );

	float *pDst = m_pCurrTexCoord[stage];
	*pDst++ = *stuv++;
	*pDst++ = *stuv++;
	*pDst++ = *stuv++;
	*pDst = *stuv;
}


inline void CVertexBuilder::TexCoordSubRect2f( int stage, float s, float t, float offsetS, float offsetT, float scaleS, float scaleT )
{
	Assert( m_pTexCoord[stage] && m_pCurrTexCoord[stage] );
	Assert( IsFinite(s) && IsFinite(t) );

	float *pDst = m_pCurrTexCoord[stage];
	*pDst++ = ( s * scaleS ) + offsetS;
	*pDst = ( t * scaleT ) + offsetT;
}

inline void CVertexBuilder::TexCoordSubRect2fv( int stage, const float *st, const float *offset, const float *scale )
{
	Assert(st);
	Assert( m_pTexCoord[stage] && m_pCurrTexCoord[stage] );
	Assert( IsFinite(st[0]) && IsFinite(st[1]) );

	float *pDst = m_pCurrTexCoord[stage];
	*pDst++ = ( *st++ * *scale++ ) + *offset++;
	*pDst = ( *st * *scale ) + *offset;
}


//-----------------------------------------------------------------------------
// Tangent space setting methods
//-----------------------------------------------------------------------------
inline void CVertexBuilder::TangentS3f( float sx, float sy, float sz )
{
	Assert( m_pTangentS );
	Assert( IsFinite(sx) && IsFinite(sy) && IsFinite(sz) );

	float* pTangentS = OffsetFloatPointer( m_pTangentS, m_nCurrentVertex, m_VertexSize_TangentS );
	*pTangentS++ = sx;
	*pTangentS++ = sy;
	*pTangentS = sz;
}

inline void CVertexBuilder::TangentS3fv( const float* s )
{
	Assert( s );
	Assert( m_pTangentS );
	Assert( IsFinite(s[0]) && IsFinite(s[1]) && IsFinite(s[2]) );

	float* pTangentS = OffsetFloatPointer( m_pTangentS, m_nCurrentVertex, m_VertexSize_TangentS );
	*pTangentS++ = *s++;
	*pTangentS++ = *s++;
	*pTangentS = *s;
}

inline void CVertexBuilder::TangentT3f( float tx, float ty, float tz )
{
	Assert( m_pTangentT );
	Assert( IsFinite(tx) && IsFinite(ty) && IsFinite(tz) );

	float* pTangentT = OffsetFloatPointer( m_pTangentT, m_nCurrentVertex, m_VertexSize_TangentT );
	*pTangentT++ = tx;
	*pTangentT++ = ty;
	*pTangentT = tz;
}

inline void CVertexBuilder::TangentT3fv( const float* t )
{
	Assert( t );
	Assert( m_pTangentT );
	Assert( IsFinite(t[0]) && IsFinite(t[1]) && IsFinite(t[2]) );

	float* pTangentT = OffsetFloatPointer( m_pTangentT, m_nCurrentVertex, m_VertexSize_TangentT );
	*pTangentT++ = *t++;
	*pTangentT++ = *t++;
	*pTangentT = *t;
}


//-----------------------------------------------------------------------------
// Wrinkle setting methods
//-----------------------------------------------------------------------------
inline void CVertexBuilder::Wrinkle1f( float flWrinkle )
{
	Assert( m_pWrinkle );
	Assert( IsFinite(flWrinkle) );

	float *pWrinkle = OffsetFloatPointer( m_pWrinkle, m_nCurrentVertex, m_VertexSize_Wrinkle );
	*pWrinkle = flWrinkle;
}


//-----------------------------------------------------------------------------
// Bone weight setting methods
//-----------------------------------------------------------------------------
inline void CVertexBuilder::BoneWeight( int idx, float weight )
{
	Assert( m_pBoneWeight );
	Assert( IsFinite( weight ) );
	Assert( idx >= 0 );
	AssertOnce( m_NumBoneWeights == 2 );

	// This test is here because we store N-1 bone weights (the Nth is computed in
	// the vertex shader as "1 - C", where C is the sum of the (N-1) other weights)
	if ( idx < m_NumBoneWeights )
	{
		float* pBoneWeight = OffsetFloatPointer( m_pBoneWeight, m_nCurrentVertex, m_VertexSize_BoneWeight );
		pBoneWeight[idx] = weight;
	}
}

static int sg_IndexSwap[4] = { 2, 1, 0, 3 };

inline void CVertexBuilder::BoneMatrix( int idx, int matrixIdx )
{
	Assert( m_pBoneMatrixIndex );
	Assert( idx >= 0 );
	Assert( idx < 4 );

	// garymcthack
	if ( matrixIdx == BONE_MATRIX_INDEX_INVALID )
	{
		matrixIdx = 0;
	}
	Assert( (matrixIdx >= 0) && (matrixIdx < 53) );

#ifdef OPENGL_SWAP_COLORS
	idx = sg_IndexSwap[idx];
#endif
	
#ifndef NEW_SKINNING
	unsigned char* pBoneMatrix = &m_pBoneMatrixIndex[m_nCurrentVertex * m_VertexSize_BoneMatrixIndex];
	if ( IsX360() )
	{
		// store sequentially as wzyx order, gpu delivers as xyzw
		idx = 3-idx;
	}
	pBoneMatrix[idx] = (unsigned char)matrixIdx;
#else
	float* pBoneMatrix = &m_pBoneMatrixIndex[m_nCurrentVertex * m_VertexSize_BoneMatrixIndex];
	pBoneMatrix[idx] = matrixIdx;
#endif
}

//-----------------------------------------------------------------------------
// Templatized bone weight setting methods which support compressed vertices
//-----------------------------------------------------------------------------
template <VertexCompressionType_t T> inline void CVertexBuilder::CompressedBoneWeight3fv( const float * pWeights )
{
	Assert( T == m_CompressionType );
	Assert( m_pBoneWeight );
	Assert( pWeights );

	float *pDestWeights = OffsetFloatPointer( m_pBoneWeight, m_nCurrentVertex, m_VertexSize_BoneWeight );

	if ( T == VERTEX_COMPRESSION_ON )
	{
		// Quantize to 15 bits per weight (we use D3DDECLTYPE_SHORT2)
		// NOTE: we perform careful normalization (weights sum to 1.0f in the vertex shader), so
		// as to avoid cracking at boundaries between meshes with different numbers of weights
		// per vertex. For example, (1) needs to yield the same normalized weights as (1,0),
		// and (0.5,0.49) needs to normalize the same normalized weights as (0.5,0.49,0).
		// The key is that values which are *computed* in the shader (e.g. the second weight
		// in a 2-weight mesh) must exactly equal values which are *read* from the vertex
		// stream (e.g. the second weight in a 3-weight mesh).

		// Only 1 or 2 weights (SHORT2N) supported for compressed verts so far
		Assert( m_NumBoneWeights <= 2 );

		const int WEIGHT0_SHIFT = IsX360() ? 16 : 0;
		const int WEIGHT1_SHIFT = IsX360() ? 0 : 16;
		unsigned int *weights = (unsigned int *)pDestWeights;

		// We scale our weights so that they sum to 32768, then subtract 1 (which gets added
		// back in the shader), because dividing by 32767 introduces nasty rounding issues.
		Assert( IsFinite( pWeights[0] ) && ( pWeights[0] >= 0.0f ) && ( pWeights[0] <= 1.0f ) );
		unsigned int weight0 = Float2Int( pWeights[0] * 32768.0f );
		*weights = ( 0x0000FFFF & (weight0 - 1) ) << WEIGHT0_SHIFT;

#ifdef DEBUG
		if ( m_NumBoneWeights == 1 )
		{
			// Double-check the validity of the values that were passed in
			Assert( IsFinite( pWeights[1] ) && ( pWeights[1] >= 0.0f ) && ( pWeights[1] <= 1.0f ) );
			unsigned int weight1 = Float2Int( pWeights[1] * 32768.0f );
			Assert( ( weight0 + weight1 ) <= 32768 );
		}
#endif

		if ( m_NumBoneWeights > 1 )
		{
			// This path for 3 weights per vert (2 are stored and the 3rd is computed
			// in the shader - we do post-quantization normalization here in such a
			// way as to avoid mesh-boundary cracking)
			Assert( m_NumBoneWeights == 2 );
			Assert( IsFinite( pWeights[1] ) && ( pWeights[1] >= 0.0f ) && ( pWeights[1] <= 1.0f ) );
			Assert( IsFinite( pWeights[2] ) && ( pWeights[2] >= 0.0f ) && ( pWeights[2] <= 1.0f ) );
			unsigned int weight1 = Float2Int( pWeights[1] * 32768.0f );
			unsigned int weight2 = Float2Int( pWeights[2] * 32768.0f );
			Assert( ( weight0 + weight1 + weight2 ) <= 32768 );
			unsigned int residual = 32768 - ( weight0 + weight1 + weight2 );
			weight1 += residual; // Normalize
			*weights |= ( 0x0000FFFF & ( weight1 - 1 ) ) << WEIGHT1_SHIFT;
		}
	}
	else	// Uncompressed path
	{
		pDestWeights[0] = pWeights[0];
		pDestWeights[1] = pWeights[1];
	}
}

//-----------------------------------------------------------------------------
// Generic per-vertex data setting method
//-----------------------------------------------------------------------------
inline void CVertexBuilder::UserData( const float* pData )
{
	Assert( m_CompressionType == VERTEX_COMPRESSION_NONE ); // Use the templatized version if you want to support compression
	Assert( pData );

	int userDataSize = 4; // garymcthack
	float *pUserData = OffsetFloatPointer( m_pUserData, m_nCurrentVertex, m_VertexSize_UserData );
	memcpy( pUserData, pData, sizeof( float ) * userDataSize );
}

//-----------------------------------------------------------------------------
// Templatized generic per-vertex data setting method which supports compressed vertices
//-----------------------------------------------------------------------------
template <VertexCompressionType_t T> inline void CVertexBuilder::CompressedUserData( const float* pData )
{
	Assert( T == m_CompressionType );
	Assert( pData );
	// This is always in fact a tangent vector, not generic 'userdata'
	Assert( IsFinite(pData[0]) && IsFinite(pData[1]) && IsFinite(pData[2]) );
	Assert( pData[0] >= -1.05f && pData[0] <= 1.05f );
	Assert( pData[1] >= -1.05f && pData[1] <= 1.05f );
	Assert( pData[2] >= -1.05f && pData[2] <= 1.05f );
	Assert( pData[3] == +1.0f  || pData[3] == -1.0f );
	// FIXME: studiorender is passing in non-unit normals
	//float lengthSqd = pData[0]*pData[0] + pData[1]*pData[1] + pData[2]*pData[2];
	//Assert( lengthSqd >= 0.95f && lengthSqd <= 1.05f );

	if ( T == VERTEX_COMPRESSION_ON )
	{
		float binormalSign = pData[3];

#if		( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_SEPARATETANGENTS_SHORT2 )
		float *pUserData = OffsetFloatPointer( m_pUserData, m_nCurrentVertex, m_VertexSize_UserData );
		PackNormal_SHORT2( pData, (unsigned int *)pUserData, binormalSign );
#else //( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 ) 
		// FIXME: add a combined CompressedNormalAndTangent() accessor, to avoid reading back from write-combined memory here
		// The normal should have already been written into the lower 16
		// bits - here, we OR in the tangent into the upper 16 bits
		unsigned int existingNormalData = *(unsigned int *)m_pCurrNormal;
		Assert( ( existingNormalData & 0xFFFF0000 ) == 0 );
#ifdef _DEBUG
		Assert( m_bWrittenNormal == true );
		m_bWrittenUserData = true;
#endif
		bool bIsTangent = true;
		unsigned int tangentData = 0;
		PackNormal_UBYTE4( pData, &tangentData, bIsTangent, binormalSign );
		*(unsigned int *)m_pCurrNormal = existingNormalData | tangentData;
#endif
	}
	else
	{
		int userDataSize = 4; // garymcthack
		float *pUserData = OffsetFloatPointer( m_pUserData, m_nCurrentVertex, m_VertexSize_UserData );
		memcpy( pUserData, pData, sizeof( float ) * userDataSize );
	}
}


//-----------------------------------------------------------------------------
//
// Helper class used to define index buffers
//
//-----------------------------------------------------------------------------
class CIndexBuilder : private IndexDesc_t
{
public:
	CIndexBuilder();
	CIndexBuilder( IIndexBuffer *pIndexBuffer, MaterialIndexFormat_t fmt = MATERIAL_INDEX_FORMAT_UNKNOWN );
	~CIndexBuilder();

	// Begins, ends modification of the index buffer (returns true if the lock succeeded)
	// A lock may not succeed if append is set to true and there isn't enough room
	// NOTE: Append is only used with dynamic index buffers; it's ignored for static buffers
	bool Lock( int nMaxIndexCount, int nIndexOffset, bool bAppend = false );
	void Unlock();

	// Spews the current data
	// NOTE: Can only be called during a lock/unlock block
	void SpewData();

	// Returns the number of indices we can fit into the buffer without needing to discard
	int GetRoomRemaining() const;

	// Binds this index buffer
	void Bind( IMatRenderContext *pContext );

	// Returns the byte offset
	int Offset() const;

	// Begins, ends modification of the index buffer
	// NOTE: IndexOffset is the number to add to all indices written into the buffer;
	// useful when using dynamic vertex buffers.
	void Begin( IIndexBuffer *pIndexBuffer, int nMaxIndexCount, int nIndexOffset = 0 );
	void End( bool bSpewData = false );

	// Locks the index buffer to modify existing data
	// Passing nVertexCount == -1 says to lock all the vertices for modification.
	// Pass 0 for nIndexCount to not lock the index buffer.
	void BeginModify( IIndexBuffer *pIndexBuffer, int nFirstIndex = 0, int nIndexCount = 0, int nIndexOffset = 0 );
	void EndModify( bool bSpewData = false );

	// returns the number of indices
	int	IndexCount() const;

	// Returns the total number of indices across all Locks()
	int TotalIndexCount() const;

	// Resets the mesh builder so it points to the start of everything again
	void Reset();

	// Selects the nth Index 
	void SelectIndex( int nBufferIndex );

	// Advances the current index by one
	void AdvanceIndex();
	void AdvanceIndices( int nIndexCount );

	int GetCurrentIndex();
	int GetFirstIndex() const;

	unsigned short const* Index() const;

	// Used to define the indices (only used if you aren't using primitives)
	void Index( unsigned short nIndex );

	// Fast Index! No need to call advance index, and no random access allowed
	void FastIndex( unsigned short nIndex );

	// NOTE: This version is the one you really want to achieve write-combining;
	// Write combining only works if you write in 4 bytes chunks.
	void FastIndex2( unsigned short nIndex1, unsigned short nIndex2 );

	// Generates indices for a particular primitive type
	void GenerateIndices( MaterialPrimitiveType_t primitiveType, int nIndexCount );

	// FIXME: Remove! Backward compat so we can use this from a CMeshBuilder.
	void AttachBegin( IMesh* pMesh, int nMaxIndexCount, const MeshDesc_t &desc );
	void AttachEnd();
	void AttachBeginModify( IMesh* pMesh, int nFirstIndex, int nIndexCount, const MeshDesc_t &desc );
	void AttachEndModify();

	void FastTriangle( int startVert );
	void FastQuad( int startVert );
	void FastPolygon( int startVert, int numTriangles );
	void FastPolygonList( int startVert, int *pVertexCount, int polygonCount );
	void FastIndexList( const unsigned short *pIndexList, int startVert, int indexCount );

private:
	// The mesh we're modifying
	IIndexBuffer	*m_pIndexBuffer;

	// Max number of indices
	int				m_nMaxIndexCount;

	// Number of indices
	int				m_nIndexCount;

	// Offset to add to each index as it's written into the buffer
	int				m_nIndexOffset;

	// The current index
	mutable int		m_nCurrentIndex;

	// Total number of indices appended
	int				m_nTotalIndexCount;

	// First index buffer offset + first index
	unsigned int	m_nBufferOffset;
	unsigned int	m_nBufferFirstIndex;

	// Used to make sure Begin/End calls and BeginModify/EndModify calls match.
	bool			m_bModify;
};


//-----------------------------------------------------------------------------
//
// Inline methods related to CIndexBuilder
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
inline CIndexBuilder::CIndexBuilder() : m_pIndexBuffer(0), m_nIndexCount(0), 
	m_nCurrentIndex(0),	m_nMaxIndexCount(0)
{
	m_nTotalIndexCount = 0;
	m_nBufferOffset = INVALID_BUFFER_OFFSET;
	m_nBufferFirstIndex = 0;
#ifdef _DEBUG
	m_bModify = false;
#endif
}

inline CIndexBuilder::CIndexBuilder( IIndexBuffer *pIndexBuffer, MaterialIndexFormat_t fmt )
{
	m_pIndexBuffer = pIndexBuffer;
	m_nBufferOffset = INVALID_BUFFER_OFFSET;
	m_nBufferFirstIndex = 0;
	m_nIndexCount = 0;
	m_nCurrentIndex = 0;
	m_nMaxIndexCount = 0;
	m_nTotalIndexCount = 0;
	if ( m_pIndexBuffer->IsDynamic() )
	{
		m_pIndexBuffer->BeginCastBuffer( fmt );
	}
	else
	{
		Assert( m_pIndexBuffer->IndexFormat() == fmt );
	}
#ifdef _DEBUG
	m_bModify = false;
#endif
}

inline CIndexBuilder::~CIndexBuilder()
{
	if ( m_pIndexBuffer && m_pIndexBuffer->IsDynamic() )
	{
		m_pIndexBuffer->EndCastBuffer();
	}
}


//-----------------------------------------------------------------------------
// Begins, ends modification of the index buffer
//-----------------------------------------------------------------------------
inline bool CIndexBuilder::Lock( int nMaxIndexCount, int nIndexOffset, bool bAppend )
{
 	Assert( m_pIndexBuffer );
	m_bModify = false;
	m_nIndexOffset = nIndexOffset;
	m_nMaxIndexCount = nMaxIndexCount;
	bool bFirstLock = ( m_nBufferOffset == INVALID_BUFFER_OFFSET );
	if ( bFirstLock )
	{
		bAppend = false;
	}
	if ( !bAppend )
	{
		m_nTotalIndexCount = 0;
	}
	Reset();

	// Lock the index buffer
	if ( !m_pIndexBuffer->Lock( m_nMaxIndexCount, bAppend, *this ) )
	{
		m_nMaxIndexCount = 0;
		return false;
	}

	if ( bFirstLock )
	{
		m_nBufferOffset = m_nOffset;
		m_nBufferFirstIndex = m_nFirstIndex;
	}

	return true;
}

inline void CIndexBuilder::Unlock()
{
	Assert( !m_bModify && m_pIndexBuffer );

	m_pIndexBuffer->Unlock( m_nIndexCount, *this );
	m_nTotalIndexCount += m_nIndexCount;

	m_nMaxIndexCount = 0;

#ifdef _DEBUG
	// Null out our data...
	memset( (IndexDesc_t*)this, 0, sizeof(IndexDesc_t) );
#endif
}

inline void CIndexBuilder::SpewData()
{
	m_pIndexBuffer->Spew( m_nIndexCount, *this );
}


//-----------------------------------------------------------------------------
// Binds this index buffer
//-----------------------------------------------------------------------------
inline void CIndexBuilder::Bind( IMatRenderContext *pContext )
{
	if ( m_pIndexBuffer && ( m_nBufferOffset != INVALID_BUFFER_OFFSET ) )
	{
		pContext->BindIndexBuffer( m_pIndexBuffer, m_nBufferOffset );
	}
	else
	{
		pContext->BindIndexBuffer( NULL, 0 );
	}
}


//-----------------------------------------------------------------------------
// Returns the byte offset
//-----------------------------------------------------------------------------
inline int CIndexBuilder::Offset() const
{
	return m_nBufferOffset;
}

inline int CIndexBuilder::GetFirstIndex() const
{
	return m_nBufferFirstIndex;
}


//-----------------------------------------------------------------------------
// Begins, ends modification of the index buffer
//-----------------------------------------------------------------------------
inline void CIndexBuilder::Begin( IIndexBuffer *pIndexBuffer, int nMaxIndexCount, int nIndexOffset )
{
	Assert( pIndexBuffer && (!m_pIndexBuffer) );

	m_pIndexBuffer = pIndexBuffer;
	m_nIndexCount = 0;
	m_nMaxIndexCount = nMaxIndexCount;
	m_nIndexOffset = nIndexOffset;

	m_bModify = false;

	// Lock the index buffer
	m_pIndexBuffer->Lock( m_nMaxIndexCount, false, *this );

	// Point to the start of the buffers..
	Reset();
}

inline void CIndexBuilder::End( bool bSpewData )
{
	// Make sure they called Begin()
	Assert( !m_bModify );

	if ( bSpewData )
	{
		m_pIndexBuffer->Spew( m_nIndexCount, *this );
	}

	// Unlock our buffers
	m_pIndexBuffer->Unlock( m_nIndexCount, *this );

	m_pIndexBuffer = 0;
	m_nMaxIndexCount = 0;

#ifdef _DEBUG
	// Null out our data...
	memset( (IndexDesc_t*)this, 0, sizeof(IndexDesc_t) );
#endif
}


//-----------------------------------------------------------------------------
// Begins, ends modification of an existing index buffer which has already been filled out
//-----------------------------------------------------------------------------
inline void CIndexBuilder::BeginModify( IIndexBuffer* pIndexBuffer, int nFirstIndex, int nIndexCount, int nIndexOffset )
{
	m_pIndexBuffer = pIndexBuffer;
	m_nIndexCount = nIndexCount;
	m_nMaxIndexCount = nIndexCount;
	m_nIndexOffset = nIndexOffset;
	m_bModify = true;

	// Lock the vertex and index buffer
	m_pIndexBuffer->ModifyBegin( false, nFirstIndex, nIndexCount, *this );

	// Point to the start of the buffers..
	Reset();
}

inline void CIndexBuilder::EndModify( bool bSpewData )
{
	Assert( m_pIndexBuffer );
	Assert( m_bModify );	// Make sure they called BeginModify.

	if ( bSpewData )
	{
		m_pIndexBuffer->Spew( m_nIndexCount, *this );
	}

	// Unlock our buffers
	m_pIndexBuffer->ModifyEnd( *this );

	m_pIndexBuffer = 0;
	m_nMaxIndexCount = 0;

#ifdef _DEBUG
	// Null out our data...
	memset( (IndexDesc_t*)this, 0, sizeof(IndexDesc_t) );
#endif
}


//-----------------------------------------------------------------------------
// FIXME: Remove! Backward compat so we can use this from a CMeshBuilder.
//-----------------------------------------------------------------------------
inline void CIndexBuilder::AttachBegin( IMesh* pMesh, int nMaxIndexCount, const MeshDesc_t &desc )
{
	m_pIndexBuffer = pMesh;
	m_nIndexCount = 0;
	m_nMaxIndexCount = nMaxIndexCount;

	m_bModify = false;

	// Copy relevant data from the mesh desc
	m_nIndexOffset = desc.m_nFirstVertex;
	m_pIndices = desc.m_pIndices;
	m_nIndexSize = desc.m_nIndexSize;

	// Point to the start of the buffers..
	Reset();
}

inline void CIndexBuilder::AttachEnd()
{
	Assert( m_pIndexBuffer );
	Assert( !m_bModify );	// Make sure they called AttachBegin.

	m_pIndexBuffer = 0;
	m_nMaxIndexCount = 0;

#ifdef _DEBUG
	// Null out our data...
	memset( (IndexDesc_t*)this, 0, sizeof(IndexDesc_t) );
#endif
}

inline void CIndexBuilder::AttachBeginModify( IMesh* pMesh, int nFirstIndex, int nIndexCount, const MeshDesc_t &desc )
{
	m_pIndexBuffer = pMesh;
	m_nIndexCount = nIndexCount;
	m_nMaxIndexCount = nIndexCount;
	m_bModify = true;

	// Copy relevant data from the mesh desc
	m_nIndexOffset = desc.m_nFirstVertex;
	m_pIndices = desc.m_pIndices;
	m_nIndexSize = desc.m_nIndexSize;

	// Point to the start of the buffers..
	Reset();
}

inline void CIndexBuilder::AttachEndModify()
{
	Assert( m_pIndexBuffer );
	Assert( m_bModify );	// Make sure they called AttachBeginModify.

	m_pIndexBuffer = 0;
	m_nMaxIndexCount = 0;

#ifdef _DEBUG
	// Null out our data...
	memset( (IndexDesc_t*)this, 0, sizeof(IndexDesc_t) );
#endif
}


//-----------------------------------------------------------------------------
// Resets the index buffer builder so it points to the start of everything again
//-----------------------------------------------------------------------------
inline void CIndexBuilder::Reset()
{
	m_nCurrentIndex = 0;
}


//-----------------------------------------------------------------------------
// returns the number of indices
//-----------------------------------------------------------------------------
inline int CIndexBuilder::IndexCount() const
{
	return m_nIndexCount;
}


//-----------------------------------------------------------------------------
// Returns the total number of indices across all Locks()
//-----------------------------------------------------------------------------
inline int CIndexBuilder::TotalIndexCount() const
{
	return m_nTotalIndexCount;
}


//-----------------------------------------------------------------------------
// Advances the current index
//-----------------------------------------------------------------------------
inline void CIndexBuilder::AdvanceIndex()
{
	m_nCurrentIndex += m_nIndexSize;
	if ( m_nCurrentIndex > m_nIndexCount )
	{
		m_nIndexCount = m_nCurrentIndex; 
	}
}

inline void CIndexBuilder::AdvanceIndices( int nIndices )
{
	m_nCurrentIndex += nIndices * m_nIndexSize;
	if ( m_nCurrentIndex > m_nIndexCount )
	{
		m_nIndexCount = m_nCurrentIndex; 
	}
}


//-----------------------------------------------------------------------------
// Returns the current index
//-----------------------------------------------------------------------------
inline int CIndexBuilder::GetCurrentIndex()
{
	return m_nCurrentIndex;
}

inline unsigned short const* CIndexBuilder::Index() const
{
	Assert( m_nCurrentIndex < m_nMaxIndexCount );
	return &m_pIndices[m_nCurrentIndex];
}

inline void CIndexBuilder::SelectIndex( int nIndex )
{
	Assert( ( nIndex >= 0 ) && ( nIndex < m_nIndexCount ) );
	m_nCurrentIndex = nIndex * m_nIndexSize;
}


//-----------------------------------------------------------------------------
// Used to write data into the index buffer
//-----------------------------------------------------------------------------
inline void CIndexBuilder::Index( unsigned short nIndex )
{
	Assert( m_pIndices );
	Assert( m_nCurrentIndex < m_nMaxIndexCount );
	m_pIndices[ m_nCurrentIndex ] = (unsigned short)( m_nIndexOffset + nIndex );
}

// Fast Index! No need to call advance index
inline void CIndexBuilder::FastIndex( unsigned short nIndex )
{
	Assert( m_pIndices );
	Assert( m_nCurrentIndex < m_nMaxIndexCount );
	m_pIndices[m_nCurrentIndex] = (unsigned short)( m_nIndexOffset + nIndex );
	m_nCurrentIndex += m_nIndexSize;
	m_nIndexCount = m_nCurrentIndex;	
}

inline void CIndexBuilder::FastTriangle( int startVert )
{
	startVert += m_nIndexOffset;
	m_pIndices[m_nCurrentIndex+0] = startVert;
	m_pIndices[m_nCurrentIndex+1] = startVert + 1;
	m_pIndices[m_nCurrentIndex+2] = startVert + 2;

	AdvanceIndices(3);
}

inline void CIndexBuilder::FastQuad( int startVert )
{
	startVert += m_nIndexOffset;
	m_pIndices[m_nCurrentIndex+0] = startVert;
	m_pIndices[m_nCurrentIndex+1] = startVert + 1;
	m_pIndices[m_nCurrentIndex+2] = startVert + 2;
	m_pIndices[m_nCurrentIndex+3] = startVert;
	m_pIndices[m_nCurrentIndex+4] = startVert + 2;
	m_pIndices[m_nCurrentIndex+5] = startVert + 3;
	AdvanceIndices(6);
}

inline void CIndexBuilder::FastPolygon( int startVert, int triangleCount )
{
	unsigned short *pIndex = &m_pIndices[m_nCurrentIndex];
	startVert += m_nIndexOffset;
	if ( !IsX360() )
	{
		// NOTE: IndexSize is 1 or 0 (0 for alt-tab)
		// This prevents us from writing into bogus memory
		Assert( m_nIndexSize == 0 || m_nIndexSize == 1 );
		triangleCount *= m_nIndexSize;
	}
	for ( int v = 0; v < triangleCount; ++v )
	{
		*pIndex++ = startVert;
		*pIndex++ = startVert + v + 1;
		*pIndex++ = startVert + v + 2;
	}
	AdvanceIndices(triangleCount*3);
}

inline void CIndexBuilder::FastPolygonList( int startVert, int *pVertexCount, int polygonCount )
{
	unsigned short *pIndex = &m_pIndices[m_nCurrentIndex];
	startVert += m_nIndexOffset;
	int indexOut = 0;

	if ( !IsX360() )
	{
		// NOTE: IndexSize is 1 or 0 (0 for alt-tab)
		// This prevents us from writing into bogus memory
		Assert( m_nIndexSize == 0 || m_nIndexSize == 1 );
		polygonCount *= m_nIndexSize;
	}

	for ( int i = 0; i < polygonCount; i++ )
	{
		int vertexCount = pVertexCount[i];
		int triangleCount = vertexCount-2;
		for ( int v = 0; v < triangleCount; ++v )
		{
			*pIndex++ = startVert;
			*pIndex++ = startVert + v + 1;
			*pIndex++ = startVert + v + 2;
		}
		startVert += vertexCount;
		indexOut += triangleCount * 3;
	}
	AdvanceIndices(indexOut);
}

inline void CIndexBuilder::FastIndexList( const unsigned short *pIndexList, int startVert, int indexCount )
{
	unsigned short *pIndexOut = &m_pIndices[m_nCurrentIndex];
	startVert += m_nIndexOffset;
	if ( !IsX360() )
	{
		// NOTE: IndexSize is 1 or 0 (0 for alt-tab)
		// This prevents us from writing into bogus memory
		Assert( m_nIndexSize == 0 || m_nIndexSize == 1 );
		indexCount *= m_nIndexSize;
	}
	for ( int i = 0; i < indexCount; ++i )
	{
		pIndexOut[i] = startVert + pIndexList[i];
	}
	AdvanceIndices(indexCount);
}


//-----------------------------------------------------------------------------
// NOTE: This version is the one you really want to achieve write-combining;
// Write combining only works if you write in 4 bytes chunks.
//-----------------------------------------------------------------------------
inline void CIndexBuilder::FastIndex2( unsigned short nIndex1, unsigned short nIndex2 )
{
	Assert( m_pIndices );
	Assert( m_nCurrentIndex < m_nMaxIndexCount - 1 );
//	Assert( ( (int)( &m_pIndices[m_nCurrentIndex] ) & 0x3 ) == 0 );

#ifndef _X360
	unsigned int nIndices = ( (unsigned int)nIndex1 + m_nIndexOffset ) | ( ( (unsigned int)nIndex2 + m_nIndexOffset ) << 16 );
#else
	unsigned int nIndices = ( (unsigned int)nIndex2 + m_nIndexOffset ) | ( ( (unsigned int)nIndex1 + m_nIndexOffset ) << 16 );
#endif

	*(int*)( &m_pIndices[m_nCurrentIndex] ) = nIndices;
	m_nCurrentIndex += m_nIndexSize + m_nIndexSize;
	m_nIndexCount = m_nCurrentIndex;	
}


//-----------------------------------------------------------------------------
// Generates indices for a particular primitive type
//-----------------------------------------------------------------------------
inline void CIndexBuilder::GenerateIndices( MaterialPrimitiveType_t primitiveType, int nIndexCount )
{
	// FIXME: How to make this work with short vs int sized indices?
	// Don't generate indices if we've got an empty buffer
	if ( m_nIndexSize == 0 )
		return;

	int nMaxIndices = m_nMaxIndexCount - m_nCurrentIndex;
	nIndexCount = Min( nMaxIndices, nIndexCount );
	if ( nIndexCount == 0 )
		return;

	unsigned short *pIndices = &m_pIndices[m_nCurrentIndex];

	switch( primitiveType )
	{
	case MATERIAL_INSTANCED_QUADS:
		Assert(0); // Shouldn't get here (this primtype is unindexed)
		break;
	case MATERIAL_QUADS:
		GenerateQuadIndexBuffer( pIndices, nIndexCount, m_nIndexOffset );
		break;
	case MATERIAL_POLYGON:
		GeneratePolygonIndexBuffer( pIndices, nIndexCount, m_nIndexOffset );
		break;
	case MATERIAL_LINE_STRIP:
		GenerateLineStripIndexBuffer( pIndices, nIndexCount, m_nIndexOffset );
		break;
	case MATERIAL_LINE_LOOP:
		GenerateLineLoopIndexBuffer( pIndices, nIndexCount, m_nIndexOffset );
		break;
	case MATERIAL_POINTS:
		Assert(0); // Shouldn't get here (this primtype is unindexed)
		break;
	default:
		GenerateSequentialIndexBuffer( pIndices, nIndexCount, m_nIndexOffset );
		break;
	}

	AdvanceIndices( nIndexCount );
}


//-----------------------------------------------------------------------------
//
// Helper class used to define meshes
//
//-----------------------------------------------------------------------------
//class CMeshBuilder : private MeshDesc_t
// hack fixme
class CMeshBuilder : public MeshDesc_t
{
public:
	CMeshBuilder();
	~CMeshBuilder() { Assert(!m_pMesh); }		// if this fires you did a Begin() without an End()

	operator CIndexBuilder&() { return m_IndexBuilder; }

	// This must be called before Begin, if a vertex buffer with a compressed format is to be used
	void SetCompressionType( VertexCompressionType_t compressionType );

	// Locks the vertex buffer
	// (*cannot* use the Index() call below)
	void Begin( IMesh *pMesh, MaterialPrimitiveType_t type, int numPrimitives );

	// Locks the vertex buffer, can specify arbitrary index lists
	// (must use the Index() call below)
	void Begin( IMesh *pMesh, MaterialPrimitiveType_t type, int nVertexCount, int nIndexCount, int *nFirstVertex );
	void Begin( IMesh *pMesh, MaterialPrimitiveType_t type, int nVertexCount, int nIndexCount );

	// forward compat
	void Begin( IVertexBuffer *pVertexBuffer, MaterialPrimitiveType_t type, int numPrimitives );
	void Begin( IVertexBuffer *pVertexBuffer, IIndexBuffer *pIndexBuffer, MaterialPrimitiveType_t type, int nVertexCount, int nIndexCount, int *nFirstVertex );
	void Begin( IVertexBuffer *pVertexBuffer, IIndexBuffer *pIndexBuffer, MaterialPrimitiveType_t type, int nVertexCount, int nIndexCount );

	// Use this when you're done writing
	// Set bDraw to true to call m_pMesh->Draw automatically.
	void End( bool bSpewData = false, bool bDraw = false );

	// Locks the vertex buffer to modify existing data
	// Passing nVertexCount == -1 says to lock all the vertices for modification.
	// Pass 0 for nIndexCount to not lock the index buffer.
	void BeginModify( IMesh *pMesh, int nFirstVertex = 0, int nVertexCount = -1, int nFirstIndex = 0, int nIndexCount = 0 );
	void EndModify( bool bSpewData = false );

	// A helper method since this seems to be done a whole bunch.
	void DrawQuad( IMesh* pMesh, const float *v1, const float *v2, 
		const float *v3, const float *v4, unsigned char const *pColor, bool wireframe = false );

	// returns the number of indices and vertices
	int VertexCount() const;
	int	IndexCount() const;

	// Resets the mesh builder so it points to the start of everything again
	void Reset();

	// Returns the size of the vertex
	int VertexSize() { return m_ActualVertexSize; }

	// returns the data size of a given texture coordinate
	int TextureCoordinateSize( int nTexCoordNumber ) { return m_VertexSize_TexCoord[ nTexCoordNumber ]; }

	// Returns the base vertex memory pointer
	void* BaseVertexData();

	// Selects the nth Vertex and Index 
	void SelectVertex( int idx );
	void SelectIndex( int idx );

	// Given an index, point to the associated vertex
	void SelectVertexFromIndex( int idx );

	// Advances the current vertex and index by one
	void AdvanceVertex();
	template<int nFlags, int nNumTexCoords> void AdvanceVertexF();
	void AdvanceVertices( int nVerts );
	void AdvanceIndex();
	void AdvanceIndices( int nIndices );

	int GetCurrentVertex();
	int GetCurrentIndex();

	// Data retrieval...
	const float *Position() const;

	const float *Normal() const;

	unsigned int Color() const;

	unsigned char *Specular() const;

	const float *TexCoord( int stage ) const;

	const float *TangentS() const;
	const float *TangentT() const;

	const float *BoneWeight() const;
	float Wrinkle() const;

	int NumBoneWeights() const;
#ifndef NEW_SKINNING
	unsigned char *BoneMatrix() const;
#else
	float *BoneMatrix() const;
#endif
	unsigned short const *Index() const; 

	// position setting
	void Position3f( float x, float y, float z );
	void Position3fv( const float *v );

	// normal setting
	void Normal3f( float nx, float ny, float nz );
	void Normal3fv( const float *n );
	void NormalDelta3fv( const float *n );
	void NormalDelta3f( float nx, float ny, float nz );

	// normal setting (templatized for code which needs to support compressed vertices)
	template <VertexCompressionType_t T> void CompressedNormal3f( float nx, float ny, float nz );
	template <VertexCompressionType_t T> void CompressedNormal3fv( const float *n );

	// color setting
	void Color3f( float r, float g, float b );
	void Color3fv( const float *rgb );
	void Color4f( float r, float g, float b, float a );
	void Color4fv( const float *rgba );

	// Faster versions of color
	void Color3ub( unsigned char r, unsigned char g, unsigned char b );
	void Color3ubv( unsigned char const* rgb );
	void Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void Color4ubv( unsigned char const* rgba );

	// specular color setting
	void Specular3f( float r, float g, float b );
	void Specular3fv( const float *rgb );
	void Specular4f( float r, float g, float b, float a );
	void Specular4fv( const float *rgba );

	// Faster version of specular
	void Specular3ub( unsigned char r, unsigned char g, unsigned char b );
	void Specular3ubv( unsigned char const *c );
	void Specular4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void Specular4ubv( unsigned char const *c );

	// texture coordinate setting
	void TexCoord1f( int stage, float s );
	void TexCoord2f( int stage, float s, float t );
	void TexCoord2fv( int stage, const float *st );
	void TexCoord3f( int stage, float s, float t, float u );
	void TexCoord3fv( int stage, const float *stu );
	void TexCoord4f( int stage, float s, float t, float u, float w );
	void TexCoord4fv( int stage, const float *stuv );

	void TexCoordSubRect2f( int stage, float s, float t, float offsetS, float offsetT, float scaleS, float scaleT );
	void TexCoordSubRect2fv( int stage, const float *st, const float *offset, const float *scale );

	// tangent space 
	void TangentS3f( float sx, float sy, float sz );
	void TangentS3fv( const float *s );

	void TangentT3f( float tx, float ty, float tz );
	void TangentT3fv( const float *t );

	// Wrinkle
	void Wrinkle1f( float flWrinkle );

	// bone weights
	void BoneWeight( int idx, float weight );
	// bone weights (templatized for code which needs to support compressed vertices)
	template <VertexCompressionType_t T> void CompressedBoneWeight3fv( const float * pWeights );

	// bone matrix index
	void BoneMatrix( int idx, int matrixIndex );

	// Generic per-vertex data
	void UserData( const float *pData );
	// Generic per-vertex data (templatized for code which needs to support compressed vertices)
	template <VertexCompressionType_t T> void CompressedUserData( const float* pData );

	// Used to define the indices (only used if you aren't using primitives)
	void Index( unsigned short index );

	// NOTE: Use this one to get write combining! Much faster than the other version of FastIndex
	// Fast Index! No need to call advance index, and no random access allowed
	void FastIndex2( unsigned short nIndex1, unsigned short nIndex2 );

	// Fast Index! No need to call advance index, and no random access allowed
	void FastIndex( unsigned short index );

	// Fast Vertex! No need to call advance vertex, and no random access allowed. 
	// WARNING - these are low level functions that are intended only for use
	// in the software vertex skinner.
	void FastVertex( const ModelVertexDX7_t &vertex );
	void FastVertexSSE( const ModelVertexDX7_t &vertex );

	// store 4 dx7 vertices fast. for special sse dx7 pipeline
	void Fast4VerticesSSE( 
		ModelVertexDX7_t const *vtx_a,
		ModelVertexDX7_t const *vtx_b,
		ModelVertexDX7_t const *vtx_c,
		ModelVertexDX7_t const *vtx_d);

	void FastVertex( const ModelVertexDX8_t &vertex );
	void FastVertexSSE( const ModelVertexDX8_t &vertex );

	// Add number of verts and current vert since FastVertexxx routines do not update.
	void FastAdvanceNVertices(int n);	

#if defined( _X360 )
	void VertexDX8ToX360( const ModelVertexDX8_t &vertex );
#endif

private:
	// Computes number of verts and indices 
	void ComputeNumVertsAndIndices( int *pMaxVertices, int *pMaxIndices, 
		MaterialPrimitiveType_t type, int nPrimitiveCount );
	int IndicesFromVertices( MaterialPrimitiveType_t type, int nVertexCount ); 

	// The mesh we're modifying
	IMesh *m_pMesh;

	MaterialPrimitiveType_t m_Type;

	// Generate indices?
	bool m_bGenerateIndices;

	CIndexBuilder	m_IndexBuilder;
	CVertexBuilder	m_VertexBuilder;
};


//-----------------------------------------------------------------------------
// Forward compat
//-----------------------------------------------------------------------------
inline void CMeshBuilder::Begin( IVertexBuffer* pVertexBuffer, MaterialPrimitiveType_t type, int numPrimitives )
{
	Assert( 0 );
	//	Begin( pVertexBuffer->GetMesh(), type, numPrimitives );
}

inline void CMeshBuilder::Begin( IVertexBuffer* pVertexBuffer, IIndexBuffer *pIndexBuffer, MaterialPrimitiveType_t type, int nVertexCount, int nIndexCount, int *nFirstVertex )
{
	Assert( 0 );
	//	Begin( pVertexBuffer->GetMesh(), type, nVertexCount, nIndexCount, nFirstVertex );
}

inline void CMeshBuilder::Begin( IVertexBuffer* pVertexBuffer, IIndexBuffer *pIndexBuffer, MaterialPrimitiveType_t type, int nVertexCount, int nIndexCount )
{
	Assert( 0 );
	//	Begin( pVertexBuffer->GetMesh(), type, nVertexCount, nIndexCount );
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
inline CMeshBuilder::CMeshBuilder()	: m_pMesh(0), m_bGenerateIndices(false)
{
}


//-----------------------------------------------------------------------------
// Computes the number of verts and indices based on primitive type and count
//-----------------------------------------------------------------------------
inline void CMeshBuilder::ComputeNumVertsAndIndices( int *pMaxVertices, int *pMaxIndices, 
													MaterialPrimitiveType_t type, int nPrimitiveCount )
{
	switch(type)
	{
	case MATERIAL_POINTS:
		*pMaxVertices = *pMaxIndices = nPrimitiveCount;
		break;

	case MATERIAL_LINES:
		*pMaxVertices = *pMaxIndices = nPrimitiveCount * 2;
		break;

	case MATERIAL_LINE_STRIP:
		*pMaxVertices = nPrimitiveCount + 1;
		*pMaxIndices = nPrimitiveCount * 2;
		break;

	case MATERIAL_LINE_LOOP:
		*pMaxVertices = nPrimitiveCount;
		*pMaxIndices = nPrimitiveCount * 2;
		break;

	case MATERIAL_TRIANGLES:
		*pMaxVertices = *pMaxIndices = nPrimitiveCount * 3;
		break;

	case MATERIAL_TRIANGLE_STRIP:
		*pMaxVertices = *pMaxIndices = nPrimitiveCount + 2;
		break;

	case MATERIAL_QUADS:
		*pMaxVertices = nPrimitiveCount * 4;
		*pMaxIndices = nPrimitiveCount * 6;
		break;

	case MATERIAL_INSTANCED_QUADS:
		*pMaxVertices = nPrimitiveCount;
		*pMaxIndices = 0; // This primtype is unindexed
		break;

	case MATERIAL_POLYGON:
		*pMaxVertices = nPrimitiveCount;
		*pMaxIndices = (nPrimitiveCount - 2) * 3;
		break;

	default:
		Assert(0);
	}

	// FIXME: need to get this from meshdx8.cpp, or move it to somewhere common
	Assert( *pMaxVertices <= 32768 );
	Assert( *pMaxIndices <= 32768 );
}


inline int CMeshBuilder::IndicesFromVertices( MaterialPrimitiveType_t type, int nVertexCount )
{
	switch( type )
	{
	case MATERIAL_QUADS:
		Assert( (nVertexCount & 0x3) == 0 );
		return (nVertexCount * 6) / 4;

	case MATERIAL_INSTANCED_QUADS:
		// This primtype is unindexed
		return 0;

	case MATERIAL_POLYGON:
		Assert( nVertexCount >= 3 );
		return (nVertexCount - 2) * 3;

	case MATERIAL_LINE_STRIP:
		Assert( nVertexCount >= 2 );
		return (nVertexCount - 1) * 2;

	case MATERIAL_LINE_LOOP:
		Assert( nVertexCount >= 3 );
		return nVertexCount * 2;

	default:
		return nVertexCount;
	}
}

//-----------------------------------------------------------------------------
// Specify the type of vertex compression that this CMeshBuilder will perform
//-----------------------------------------------------------------------------
inline void CMeshBuilder::SetCompressionType( VertexCompressionType_t vertexCompressionType )
{
	m_VertexBuilder.SetCompressionType( vertexCompressionType );
}

//-----------------------------------------------------------------------------
// Begins modifying the mesh
//-----------------------------------------------------------------------------
inline void CMeshBuilder::Begin( IMesh *pMesh, MaterialPrimitiveType_t type, int numPrimitives )
{
	Assert( pMesh && (!m_pMesh) );
	Assert( type != MATERIAL_HETEROGENOUS );

	m_pMesh = pMesh;
	m_bGenerateIndices = true;
	m_Type = type;

	int nMaxVertexCount, nMaxIndexCount;
	ComputeNumVertsAndIndices( &nMaxVertexCount, &nMaxIndexCount, type, numPrimitives );

	switch( type )
	{
	case MATERIAL_INSTANCED_QUADS:
		m_pMesh->SetPrimitiveType( MATERIAL_INSTANCED_QUADS );
		break;

	case MATERIAL_QUADS:
	case MATERIAL_POLYGON:
		m_pMesh->SetPrimitiveType( MATERIAL_TRIANGLES );
		break;

	case MATERIAL_LINE_STRIP:
	case MATERIAL_LINE_LOOP:
		m_pMesh->SetPrimitiveType( MATERIAL_LINES );
		break;

	default:
		m_pMesh->SetPrimitiveType( type );
	}

	// Lock the mesh
	m_pMesh->LockMesh( nMaxVertexCount, nMaxIndexCount, *this );

	m_IndexBuilder.AttachBegin( pMesh, nMaxIndexCount, *this );
	m_VertexBuilder.AttachBegin( pMesh, nMaxVertexCount, *this );

	// Point to the start of the index and vertex buffers
	Reset();
}

inline void CMeshBuilder::Begin( IMesh *pMesh, MaterialPrimitiveType_t type, int nVertexCount, int nIndexCount, int *nFirstVertex )
{
	Begin( pMesh, type, nVertexCount, nIndexCount );

	*nFirstVertex = m_VertexBuilder.m_nFirstVertex * m_VertexBuilder.VertexSize();
}

inline void CMeshBuilder::Begin( IMesh* pMesh, MaterialPrimitiveType_t type, int nVertexCount, int nIndexCount )
{
	Assert( pMesh && (!m_pMesh) );

	// NOTE: We can't specify the indices when we use quads, polygons, or
	// linestrips; they aren't actually directly supported by 
	// the material system
	Assert( (type != MATERIAL_QUADS) && (type != MATERIAL_INSTANCED_QUADS) && (type != MATERIAL_POLYGON) &&
		(type != MATERIAL_LINE_STRIP) && (type != MATERIAL_LINE_LOOP));

	// Dx8 doesn't support indexed points...
	Assert( type != MATERIAL_POINTS );

	m_pMesh = pMesh;
	m_bGenerateIndices = false;
	m_Type = type;

	// Set the primitive type
	m_pMesh->SetPrimitiveType( type );

	// Lock the vertex and index buffer
	m_pMesh->LockMesh( nVertexCount, nIndexCount, *this );

	m_IndexBuilder.AttachBegin( pMesh, nIndexCount, *this );
	m_VertexBuilder.AttachBegin( pMesh, nVertexCount, *this );

	// Point to the start of the buffers..
	Reset();
}


//-----------------------------------------------------------------------------
// Use this when you're done modifying the mesh
//-----------------------------------------------------------------------------
inline void CMeshBuilder::End( bool bSpewData, bool bDraw )
{
	if ( m_bGenerateIndices )
	{
		int nIndexCount = IndicesFromVertices( m_Type, m_VertexBuilder.VertexCount() );
		m_IndexBuilder.GenerateIndices( m_Type, nIndexCount );
	}

	if ( bSpewData )
	{
		m_pMesh->Spew( m_VertexBuilder.VertexCount(), m_IndexBuilder.IndexCount(), *this );
	}

#ifdef _DEBUG
	m_pMesh->ValidateData( m_VertexBuilder.VertexCount(), m_IndexBuilder.IndexCount(), *this );
#endif

	// Unlock our buffers
	m_pMesh->UnlockMesh( m_VertexBuilder.VertexCount(), m_IndexBuilder.IndexCount(), *this );

	m_IndexBuilder.AttachEnd();
	m_VertexBuilder.AttachEnd();

	if ( bDraw )
	{
		m_pMesh->Draw();
	}

	m_pMesh = 0;

#ifdef _DEBUG
	memset( (MeshDesc_t*)this, 0, sizeof(MeshDesc_t) );
#endif
}


//-----------------------------------------------------------------------------
// Locks the vertex buffer to modify existing data
//-----------------------------------------------------------------------------
inline void CMeshBuilder::BeginModify( IMesh* pMesh, int nFirstVertex, int nVertexCount, int nFirstIndex, int nIndexCount )
{
	Assert( pMesh && (!m_pMesh) );

	if (nVertexCount < 0)
	{
		nVertexCount = pMesh->VertexCount();
	}

	m_pMesh = pMesh;
	m_bGenerateIndices = false;

	// Locks mesh for modifying
	pMesh->ModifyBeginEx( false, nFirstVertex, nVertexCount, nFirstIndex, nIndexCount, *this );

	m_IndexBuilder.AttachBeginModify( pMesh, nFirstIndex, nIndexCount, *this );
	m_VertexBuilder.AttachBeginModify( pMesh, nFirstVertex, nVertexCount, *this );

	// Point to the start of the buffers..
	Reset();
}

inline void CMeshBuilder::EndModify( bool bSpewData )
{
	Assert( m_pMesh );

	if (bSpewData)
	{
		m_pMesh->Spew( m_VertexBuilder.VertexCount(), m_IndexBuilder.IndexCount(), *this );
	}
#ifdef _DEBUG
	m_pMesh->ValidateData( m_VertexBuilder.VertexCount(), m_IndexBuilder.IndexCount(), *this );
#endif

	// Unlocks mesh
	m_pMesh->ModifyEnd( *this );
	m_pMesh = 0;

	m_IndexBuilder.AttachEndModify();
	m_VertexBuilder.AttachEndModify();

#ifdef _DEBUG
	// Null out our pointers...
	memset( (MeshDesc_t*)this, 0, sizeof(MeshDesc_t) );
#endif
}


//-----------------------------------------------------------------------------
// Resets the mesh builder so it points to the start of everything again
//-----------------------------------------------------------------------------
inline void CMeshBuilder::Reset()
{
	m_IndexBuilder.Reset();
	m_VertexBuilder.Reset();
}


//-----------------------------------------------------------------------------
// Selects the current Vertex and Index 
//-----------------------------------------------------------------------------
FORCEINLINE void CMeshBuilder::SelectVertex( int nIndex )
{
	m_VertexBuilder.SelectVertex( nIndex );
}

inline void CMeshBuilder::SelectVertexFromIndex( int idx )
{
	// NOTE: This index is expected to be relative 
	int vertIdx = idx - m_nFirstVertex;
	SelectVertex( vertIdx );
}

FORCEINLINE void CMeshBuilder::SelectIndex( int idx )
{
	m_IndexBuilder.SelectIndex( idx );
}


//-----------------------------------------------------------------------------
// Advances the current vertex and index by one
//-----------------------------------------------------------------------------
template<int nFlags, int nNumTexCoords> FORCEINLINE void CMeshBuilder::AdvanceVertexF()
{
	m_VertexBuilder.AdvanceVertexF<nFlags, nNumTexCoords>();
}
FORCEINLINE void CMeshBuilder::AdvanceVertex()
{
	m_VertexBuilder.AdvanceVertex();
}

FORCEINLINE void CMeshBuilder::AdvanceVertices( int nVertexCount )
{
	m_VertexBuilder.AdvanceVertices( nVertexCount );
}

FORCEINLINE void CMeshBuilder::AdvanceIndex()
{
	m_IndexBuilder.AdvanceIndex();
}

FORCEINLINE void CMeshBuilder::AdvanceIndices( int nIndices )
{
	m_IndexBuilder.AdvanceIndices( nIndices );
}

FORCEINLINE int CMeshBuilder::GetCurrentVertex()
{
	return m_VertexBuilder.GetCurrentVertex();
}

FORCEINLINE int CMeshBuilder::GetCurrentIndex()
{
	return m_IndexBuilder.GetCurrentIndex();
}


//-----------------------------------------------------------------------------
// A helper method since this seems to be done a whole bunch.
//-----------------------------------------------------------------------------
inline void CMeshBuilder::DrawQuad( IMesh* pMesh, const float* v1, const float* v2, 
								   const float* v3, const float* v4, unsigned char const* pColor, bool wireframe )
{
	if (!wireframe)
	{
		Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 );

		Position3fv (v1);
		Color4ubv( pColor );
		AdvanceVertexF<VTX_HAVEPOS | VTX_HAVECOLOR, 0>();

		Position3fv (v2);
		Color4ubv( pColor );
		AdvanceVertexF<VTX_HAVEPOS | VTX_HAVECOLOR, 0>();

		Position3fv (v4);
		Color4ubv( pColor );
		AdvanceVertexF<VTX_HAVEPOS | VTX_HAVECOLOR, 0>();

		Position3fv (v3);
		Color4ubv( pColor );
		AdvanceVertexF<VTX_HAVEPOS | VTX_HAVECOLOR, 0>();
	}
	else
	{
		Begin( pMesh, MATERIAL_LINE_LOOP, 4 );
		Position3fv (v1);
		Color4ubv( pColor );
		AdvanceVertexF<VTX_HAVEPOS | VTX_HAVECOLOR, 0>();

		Position3fv (v2);
		Color4ubv( pColor );
		AdvanceVertexF<VTX_HAVEPOS | VTX_HAVECOLOR, 0>();

		Position3fv (v3);
		Color4ubv( pColor );
		AdvanceVertexF<VTX_HAVEPOS | VTX_HAVECOLOR, 0>();

		Position3fv (v4);
		Color4ubv( pColor );
		AdvanceVertexF<VTX_HAVEPOS | VTX_HAVECOLOR, 0>();
	}

	End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// returns the number of indices and vertices
//-----------------------------------------------------------------------------
FORCEINLINE int CMeshBuilder::VertexCount() const
{
	return m_VertexBuilder.VertexCount();
}

FORCEINLINE int CMeshBuilder::IndexCount() const
{
	return m_IndexBuilder.IndexCount();
}


//-----------------------------------------------------------------------------
// Returns the base vertex memory pointer
//-----------------------------------------------------------------------------
FORCEINLINE void* CMeshBuilder::BaseVertexData()
{
	return m_VertexBuilder.BaseVertexData();
}

//-----------------------------------------------------------------------------
// Data retrieval...
//-----------------------------------------------------------------------------
FORCEINLINE const float* CMeshBuilder::Position() const
{
	return m_VertexBuilder.Position();
}

FORCEINLINE const float* CMeshBuilder::Normal()	const
{
	return m_VertexBuilder.Normal();
}

FORCEINLINE unsigned int CMeshBuilder::Color() const
{
	return m_VertexBuilder.Color();
}

FORCEINLINE unsigned char *CMeshBuilder::Specular() const
{
	return m_VertexBuilder.Specular();
}

FORCEINLINE const float* CMeshBuilder::TexCoord( int nStage ) const
{
	return m_VertexBuilder.TexCoord( nStage );
}

FORCEINLINE const float* CMeshBuilder::TangentS() const
{
	return m_VertexBuilder.TangentS();
}

FORCEINLINE const float* CMeshBuilder::TangentT() const
{
	return m_VertexBuilder.TangentT();
}

FORCEINLINE float CMeshBuilder::Wrinkle() const
{
	return m_VertexBuilder.Wrinkle();
}

FORCEINLINE const float* CMeshBuilder::BoneWeight() const
{
	return m_VertexBuilder.BoneWeight();
}

FORCEINLINE int CMeshBuilder::NumBoneWeights() const
{
	return m_VertexBuilder.NumBoneWeights();
}

FORCEINLINE unsigned short const* CMeshBuilder::Index() const
{
	return m_IndexBuilder.Index();
}


//-----------------------------------------------------------------------------
// Index
//-----------------------------------------------------------------------------
FORCEINLINE void CMeshBuilder::Index( unsigned short idx )
{
	m_IndexBuilder.Index( idx );
}


//-----------------------------------------------------------------------------
// Fast Index! No need to call advance index
//-----------------------------------------------------------------------------
FORCEINLINE void CMeshBuilder::FastIndex( unsigned short idx )
{
	m_IndexBuilder.FastIndex( idx );
}

// NOTE: Use this one to get write combining! Much faster than the other version of FastIndex
// Fast Index! No need to call advance index, and no random access allowed
FORCEINLINE void CMeshBuilder::FastIndex2( unsigned short nIndex1, unsigned short nIndex2 )
{
	m_IndexBuilder.FastIndex2( nIndex1, nIndex2 );
}

//-----------------------------------------------------------------------------
// For use with the FastVertex methods, advances the current vertex by N
//-----------------------------------------------------------------------------
FORCEINLINE void CMeshBuilder::FastAdvanceNVertices( int nVertexCount )
{
	m_VertexBuilder.FastAdvanceNVertices( nVertexCount );
}


//-----------------------------------------------------------------------------
// Fast Vertex! No need to call advance vertex, and no random access allowed
//-----------------------------------------------------------------------------
FORCEINLINE void CMeshBuilder::FastVertex( const ModelVertexDX7_t &vertex )
{
	m_VertexBuilder.FastVertex( vertex );
}

FORCEINLINE void CMeshBuilder::FastVertexSSE( const ModelVertexDX7_t &vertex )
{
	m_VertexBuilder.FastVertexSSE( vertex );
}

FORCEINLINE void CMeshBuilder::Fast4VerticesSSE( 
	const ModelVertexDX7_t *vtx_a, const ModelVertexDX7_t *vtx_b,
	const ModelVertexDX7_t *vtx_c, const ModelVertexDX7_t *vtx_d )
{
	m_VertexBuilder.Fast4VerticesSSE( vtx_a, vtx_b, vtx_c, vtx_d );
}

FORCEINLINE void CMeshBuilder::FastVertex( const ModelVertexDX8_t &vertex )
{
	m_VertexBuilder.FastVertex( vertex );
}

FORCEINLINE void CMeshBuilder::FastVertexSSE( const ModelVertexDX8_t &vertex )
{
	m_VertexBuilder.FastVertexSSE( vertex );
}

//-----------------------------------------------------------------------------
// Copies a vertex into the x360 format
//-----------------------------------------------------------------------------
#if defined( _X360 )
inline void CMeshBuilder::VertexDX8ToX360( const ModelVertexDX8_t &vertex )
{
	m_VertexBuilder.VertexDX8ToX360( vertex );
}
#endif

//-----------------------------------------------------------------------------
// Vertex field setting methods
//-----------------------------------------------------------------------------
FORCEINLINE void CMeshBuilder::Position3f( float x, float y, float z )
{
	m_VertexBuilder.Position3f( x, y, z );
}

FORCEINLINE void CMeshBuilder::Position3fv( const float *v )
{
	m_VertexBuilder.Position3fv( v );
}

FORCEINLINE void CMeshBuilder::Normal3f( float nx, float ny, float nz )
{
	m_VertexBuilder.Normal3f( nx, ny, nz );
}

FORCEINLINE void CMeshBuilder::Normal3fv( const float *n )
{
	m_VertexBuilder.Normal3fv( n );
}

FORCEINLINE void CMeshBuilder::NormalDelta3f( float nx, float ny, float nz )
{
	m_VertexBuilder.NormalDelta3f( nx, ny, nz );
}

FORCEINLINE void CMeshBuilder::NormalDelta3fv( const float *n )
{
	m_VertexBuilder.NormalDelta3fv( n );
}

FORCEINLINE void CMeshBuilder::Color3f( float r, float g, float b )
{
	m_VertexBuilder.Color3f( r, g, b );
}

FORCEINLINE void CMeshBuilder::Color3fv( const float *rgb )
{
	m_VertexBuilder.Color3fv( rgb );
}

FORCEINLINE void CMeshBuilder::Color4f( float r, float g, float b, float a )
{
	m_VertexBuilder.Color4f( r, g ,b, a );
}

FORCEINLINE void CMeshBuilder::Color4fv( const float *rgba )
{
	m_VertexBuilder.Color4fv( rgba );
}

FORCEINLINE void CMeshBuilder::Color3ub( unsigned char r, unsigned char g, unsigned char b )
{
	m_VertexBuilder.Color3ub( r, g, b );
}

FORCEINLINE void CMeshBuilder::Color3ubv( unsigned char const* rgb )
{
	m_VertexBuilder.Color3ubv( rgb );
}

FORCEINLINE void CMeshBuilder::Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	m_VertexBuilder.Color4ub( r, g, b, a );
}

FORCEINLINE void CMeshBuilder::Color4ubv( unsigned char const* rgba )
{
	m_VertexBuilder.Color4ubv( rgba );
}

FORCEINLINE void CMeshBuilder::Specular3f( float r, float g, float b )
{
	m_VertexBuilder.Specular3f( r, g, b );
}

FORCEINLINE void CMeshBuilder::Specular3fv( const float *rgb )
{
	m_VertexBuilder.Specular3fv( rgb );
}

FORCEINLINE void CMeshBuilder::Specular4f( float r, float g, float b, float a )
{
	m_VertexBuilder.Specular4f( r, g, b, a );
}

FORCEINLINE void CMeshBuilder::Specular4fv( const float *rgba )
{
	m_VertexBuilder.Specular4fv( rgba );
}

FORCEINLINE void CMeshBuilder::Specular3ub( unsigned char r, unsigned char g, unsigned char b )
{
	m_VertexBuilder.Specular3ub( r, g, b );
}

FORCEINLINE void CMeshBuilder::Specular3ubv( unsigned char const *c )
{
	m_VertexBuilder.Specular3ubv( c );
}

FORCEINLINE void CMeshBuilder::Specular4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	m_VertexBuilder.Specular4ub( r, g, b, a );
}

FORCEINLINE void CMeshBuilder::Specular4ubv( unsigned char const *c )
{
	m_VertexBuilder.Specular4ubv( c );
}

FORCEINLINE void CMeshBuilder::TexCoord1f( int nStage, float s )
{
	m_VertexBuilder.TexCoord1f( nStage, s );
}

FORCEINLINE void CMeshBuilder::TexCoord2f( int nStage, float s, float t )
{
	m_VertexBuilder.TexCoord2f( nStage, s, t );
}

FORCEINLINE void CMeshBuilder::TexCoord2fv( int nStage, const float *st )
{
	m_VertexBuilder.TexCoord2fv( nStage, st );
}

FORCEINLINE void CMeshBuilder::TexCoord3f( int nStage, float s, float t, float u )
{
	m_VertexBuilder.TexCoord3f( nStage, s, t, u );
}

FORCEINLINE void CMeshBuilder::TexCoord3fv( int nStage, const float *stu )
{
	m_VertexBuilder.TexCoord3fv( nStage, stu );
}

FORCEINLINE void CMeshBuilder::TexCoord4f( int nStage, float s, float t, float u, float v )
{
	m_VertexBuilder.TexCoord4f( nStage, s, t, u, v );
}

FORCEINLINE void CMeshBuilder::TexCoord4fv( int nStage, const float *stuv )
{
	m_VertexBuilder.TexCoord4fv( nStage, stuv );
}

FORCEINLINE void CMeshBuilder::TexCoordSubRect2f( int nStage, float s, float t, float offsetS, float offsetT, float scaleS, float scaleT )
{
	m_VertexBuilder.TexCoordSubRect2f( nStage, s, t, offsetS, offsetT, scaleS, scaleT );
}

FORCEINLINE void CMeshBuilder::TexCoordSubRect2fv( int nStage, const float *st, const float *offset, const float *scale )
{
	m_VertexBuilder.TexCoordSubRect2fv( nStage, st, offset, scale );
}

FORCEINLINE void CMeshBuilder::TangentS3f( float sx, float sy, float sz )
{
	m_VertexBuilder.TangentS3f( sx, sy, sz );
}

FORCEINLINE void CMeshBuilder::TangentS3fv( const float* s )
{
	m_VertexBuilder.TangentS3fv( s );
}

FORCEINLINE void CMeshBuilder::TangentT3f( float tx, float ty, float tz )
{
	m_VertexBuilder.TangentT3f( tx, ty, tz );
}

FORCEINLINE void CMeshBuilder::TangentT3fv( const float* t )
{
	m_VertexBuilder.TangentT3fv( t );
}

FORCEINLINE void CMeshBuilder::Wrinkle1f( float flWrinkle )
{
	m_VertexBuilder.Wrinkle1f( flWrinkle );
}

FORCEINLINE void CMeshBuilder::BoneWeight( int nIndex, float flWeight )
{
	m_VertexBuilder.BoneWeight( nIndex, flWeight );
}

template <VertexCompressionType_t T> FORCEINLINE void CMeshBuilder::CompressedBoneWeight3fv( const float * pWeights )
{
	m_VertexBuilder.CompressedBoneWeight3fv<T>( pWeights );
}

FORCEINLINE void CMeshBuilder::BoneMatrix( int nIndex, int nMatrixIdx )
{
	m_VertexBuilder.BoneMatrix( nIndex, nMatrixIdx );
}

FORCEINLINE void CMeshBuilder::UserData( const float* pData )
{
	m_VertexBuilder.UserData( pData );
}

template <VertexCompressionType_t T> FORCEINLINE void CMeshBuilder::CompressedUserData( const float* pData )
{
	m_VertexBuilder.CompressedUserData<T>( pData );
}

//-----------------------------------------------------------------------------
// Templatized vertex field setting methods which support compression
//-----------------------------------------------------------------------------

template <VertexCompressionType_t T> FORCEINLINE void CMeshBuilder::CompressedNormal3f( float nx, float ny, float nz )
{
	m_VertexBuilder.CompressedNormal3f<T>( nx, ny, nz );
}

template <VertexCompressionType_t T> FORCEINLINE void CMeshBuilder::CompressedNormal3fv( const float *n )
{
	m_VertexBuilder.CompressedNormal3fv<T>( n );
}

#endif // IMESH_H
