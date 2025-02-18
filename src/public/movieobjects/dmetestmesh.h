//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing a mesh
//
//=============================================================================

#ifndef DMETESTMESH_H
#define DMETESTMESH_H

#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmeshape.h"
#include "datacache/imdlcache.h"

#include "mathlib/vector.h"
#include <string>
#include <vector>


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeTransform;
class IMorph;
class IMaterial;
struct SubdivMesh_t;
class IMesh;
class CDmeDrawSettings;


//-----------------------------------------------------------------------------
// First attempt at making a hacky SMD loader - clean this up later
//-----------------------------------------------------------------------------
struct skinning_info_t
{
	skinning_info_t() : index( -1 ), weight( 0.0f )
	{
	}
	skinning_info_t( int i, float w ) : index( i ), weight( w )
	{
	}
	int index;
	float weight;
	bool operator<( const skinning_info_t &info )
	{
		return weight < info.weight;
	}
};

struct vertex_t
{
	Vector coord;
	Vector normal;
	Vector2D texcoord;
	std::vector< skinning_info_t > skinning;

	static float normal_tolerance;

	bool operator==( const vertex_t &vert )
	{
		return
//			skinning == vert.skinning && // TODO - the original studiomdl doesn't do this, but...
			coord == vert.coord &&
			texcoord == vert.texcoord &&
			DotProduct( normal, vert.normal ) > normal_tolerance;
	}
};

struct submesh_t
{
	submesh_t( const std::string &texture_name ) : texname( texture_name )
	{
	}
	std::string texname;
	std::vector< int > indices;
	std::vector< vertex_t > vertices;
	std::vector< CDmeTransform* > bones;
};


//-----------------------------------------------------------------------------
// A class representing a mesh
//-----------------------------------------------------------------------------
class CDmeTestMesh : public CDmeShape
{
	DEFINE_ELEMENT( CDmeTestMesh, CDmeShape );

public:
	virtual void Draw( const matrix3x4_t& shapeToWorld, CDmeDrawSettings *pDrawSettings = NULL );

	static CDmeTestMesh *ReadMeshFromSMD( char *pFilename, DmFileId_t fileid );

	virtual void Resolve();

private:
	// Addref/Release the MDL handle
	void ReferenceMDL( const char *pMDLName );
	void UnreferenceMDL();

	// Returns a mask indicating which bones to set up
	int BoneMask( );

	// Sets up the bones
	void SetUpBones( CDmeTransform *pTransform, int nMaxBoneCount, matrix3x4_t *pBoneToWorld );

	// For testing vertex textures
	void LoadMorphData( const char *pMorphFile, int nVertexCount );
	void UnloadMorphData();
	void LoadModelMatrix( CDmeTransform *pTransform );
	void DrawBox( CDmeTransform *pTransform );

	// Draws a subdivided box
	void DrawSubdivMesh( const SubdivMesh_t &mesh );
	void DrawSubdividedBox();

	// Creates/destroys the subdiv control cage
	void CreateControlCage( );
	void DestroyControlCage( );

	// Creates/destroys the morphed mesh
	void CreateMesh( );
	void DestroyMesh( );

	MDLHandle_t m_MDLHandle;
	IMaterial *m_pMaterial;
	IMesh *m_pMesh;
	IMorph *m_pMorph;
	SubdivMesh_t *m_pControlCage;

	//-----------------------------------------------------------------------------
	// First attempt at making a hacky SMD loader - clean this up later
	//-----------------------------------------------------------------------------

	std::vector< submesh_t* > m_submeshes;
	std::vector< CDmeTransform* > m_bones;
};

#endif // DMETESTMESH_H
