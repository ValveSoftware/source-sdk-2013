//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Places "detail" objects which are client-only renderable things
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#include <windows.h>
#include "vbsp.h"
#include "bsplib.h"
#include "KeyValues.h"
#include "utlsymbol.h"
#include "utlvector.h"
#include <io.h>
#include "bspfile.h"
#include "utilmatlib.h"
#include "gamebspfile.h"
#include "mathlib/VMatrix.h"
#include "materialpatch.h"
#include "pacifier.h"
#include "vstdlib/random.h"
#include "builddisp.h"
#include "disp_vbsp.h"
#include "UtlBuffer.h"
#include "CollisionUtils.h"
#include <float.h>
#include "UtlLinkedList.h"
#include "byteswap.h"
#include "writebsp.h"

//-----------------------------------------------------------------------------
// Information about particular detail object types
//-----------------------------------------------------------------------------
enum
{
	MODELFLAG_UPRIGHT = 0x1,
};

struct DetailModel_t
{
	CUtlSymbol	m_ModelName;
	float		m_Amount;
	float		m_MinCosAngle;
	float		m_MaxCosAngle;
	int			m_Flags;
	int			m_Orientation;
	int			m_Type;
	Vector2D	m_Pos[2];
	Vector2D	m_Tex[2];
	float		m_flRandomScaleStdDev;
	unsigned char m_ShapeSize;
	unsigned char m_ShapeAngle;
	unsigned char m_SwayAmount;
};

struct DetailObjectGroup_t
{
	float	m_Alpha;
	CUtlVector< DetailModel_t >	m_Models;
};

struct DetailObject_t
{
	CUtlSymbol m_Name;
	float	m_Density;
	CUtlVector< DetailObjectGroup_t >	m_Groups;

	bool operator==(const DetailObject_t& src ) const
	{
		return src.m_Name == m_Name;
	}
};

static CUtlVector<DetailObject_t>	s_DetailObjectDict;


//-----------------------------------------------------------------------------
// Error checking.. make sure the model is valid + is a static prop
//-----------------------------------------------------------------------------
struct StaticPropLookup_t
{
	CUtlSymbol	m_ModelName;
	bool		m_IsValid;
};

static bool StaticLess( StaticPropLookup_t const& src1, StaticPropLookup_t const& src2 )
{
	return src1.m_ModelName < src2.m_ModelName;
}

static CUtlRBTree< StaticPropLookup_t, unsigned short > s_StaticPropLookup( 0, 32, StaticLess );


//-----------------------------------------------------------------------------
// These puppies are used to construct the game lumps
//-----------------------------------------------------------------------------
static CUtlVector<DetailObjectDictLump_t>	s_DetailObjectDictLump;
static CUtlVector<DetailObjectLump_t>		s_DetailObjectLump;
static CUtlVector<DetailSpriteDictLump_t>	s_DetailSpriteDictLump;


//-----------------------------------------------------------------------------
// Parses the key-value pairs in the detail.rad file
//-----------------------------------------------------------------------------
static void ParseDetailGroup( int detailId, KeyValues* pGroupKeyValues )
{
	// Sort the group by alpha
	float alpha = pGroupKeyValues->GetFloat( "alpha", 1.0f );
	
	int i = s_DetailObjectDict[detailId].m_Groups.Count();
	while ( --i >= 0 )
	{
		if (alpha > s_DetailObjectDict[detailId].m_Groups[i].m_Alpha)
			break;
	}

	// Insert after the first guy who's more transparent that we are!
	i = s_DetailObjectDict[detailId].m_Groups.InsertAfter(i);
	DetailObjectGroup_t& group = s_DetailObjectDict[detailId].m_Groups[i];

	group.m_Alpha = alpha;

	// Add in all the model groups
	KeyValues* pIter = pGroupKeyValues->GetFirstSubKey();
	float totalAmount = 0.0f;
	while( pIter )
	{
		if (pIter->GetFirstSubKey())
		{
			int i = group.m_Models.AddToTail();

			DetailModel_t &model = group.m_Models[i];

			model.m_ModelName = pIter->GetString( "model", 0 );
			if (model.m_ModelName != UTL_INVAL_SYMBOL)
			{
				model.m_Type = DETAIL_PROP_TYPE_MODEL;
			}
			else
			{
				const char *pSpriteData = pIter->GetString( "sprite", 0 );
				if (pSpriteData)
				{
					const char *pProcModelType = pIter->GetString( "sprite_shape", 0 );

					if ( pProcModelType )
					{
						if ( !Q_stricmp( pProcModelType, "cross" ) )
						{
							model.m_Type = DETAIL_PROP_TYPE_SHAPE_CROSS;
						}
						else if ( !Q_stricmp( pProcModelType, "tri" ) )
						{
							model.m_Type = DETAIL_PROP_TYPE_SHAPE_TRI;
						}
						else
							model.m_Type = DETAIL_PROP_TYPE_SPRITE;
					}					
					else
					{
						// card sprite
                        model.m_Type = DETAIL_PROP_TYPE_SPRITE;
					}

					model.m_Tex[0].Init();
					model.m_Tex[1].Init();

					float x = 0, y = 0, flWidth = 64, flHeight = 64, flTextureSize = 512;
					int nValid = sscanf( pSpriteData, "%f %f %f %f %f", &x, &y, &flWidth, &flHeight, &flTextureSize ); 
					if ( (nValid != 5) || (flTextureSize == 0) )
					{
						Error( "Invalid arguments to \"sprite\" in detail.vbsp (model %s)!\n", model.m_ModelName.String() );
					}

					model.m_Tex[0].x = ( x + 0.5f ) / flTextureSize;
					model.m_Tex[0].y = ( y + 0.5f ) / flTextureSize;
					model.m_Tex[1].x = ( x + flWidth - 0.5f ) / flTextureSize;
					model.m_Tex[1].y = ( y + flHeight - 0.5f ) / flTextureSize;

					model.m_Pos[0].Init( -10, 20 );
					model.m_Pos[1].Init( 10, 0 );

					pSpriteData = pIter->GetString( "spritesize", 0 );
					if (pSpriteData)
					{
						sscanf( pSpriteData, "%f %f %f %f", &x, &y, &flWidth, &flHeight );

						float ox = flWidth * x;
						float oy = flHeight * y;

						model.m_Pos[0].x = -ox;
						model.m_Pos[0].y = flHeight - oy;
						model.m_Pos[1].x = flWidth - ox;
						model.m_Pos[1].y = -oy;
					}

					model.m_flRandomScaleStdDev = pIter->GetFloat( "spriterandomscale", 0.0f );

					// sway is a percent of max sway, cl_detail_max_sway
					float flSway = clamp( pIter->GetFloat( "sway", 0.0f ), 0.0, 1.0 );
					model.m_SwayAmount = (unsigned char)( 255.0 * flSway );

					// shape angle
					// for the tri shape, this is the angle each side is fanned out
					model.m_ShapeAngle = pIter->GetInt( "shape_angle", 0 );

					// shape size
					// for the tri shape, this is the distance from the origin to the center of a side
					float flShapeSize = clamp( pIter->GetFloat( "shape_size", 0.0f ), 0.0, 1.0 );
					model.m_ShapeSize = (unsigned char)( 255.0 * flShapeSize );
				}
			}

			model.m_Amount = pIter->GetFloat( "amount", 1.0 ) + totalAmount;
			totalAmount = model.m_Amount;

			model.m_Flags = 0;
			if (pIter->GetInt( "upright", 0 ))
			{
				model.m_Flags |= MODELFLAG_UPRIGHT;
			}

			// These are used to prevent emission on steep surfaces
			float minAngle = pIter->GetFloat( "minAngle", 180 );
			float maxAngle = pIter->GetFloat( "maxAngle", 180 );
			model.m_MinCosAngle = cos(minAngle * M_PI / 180.f);
			model.m_MaxCosAngle = cos(maxAngle * M_PI / 180.f);
			model.m_Orientation = pIter->GetInt( "detailOrientation", 0 );

			// Make sure minAngle < maxAngle
			if ( model.m_MinCosAngle < model.m_MaxCosAngle)
			{
				model.m_MinCosAngle = model.m_MaxCosAngle;
			}
		}
		pIter = pIter->GetNextKey();
	}

	// renormalize the amount if the total > 1
	if (totalAmount > 1.0f)
	{
		for (i = 0; i < group.m_Models.Count(); ++i)
		{
			group.m_Models[i].m_Amount /= totalAmount;
		}
	}
}


//-----------------------------------------------------------------------------
// Parses the key-value pairs in the detail.vbsp file
//-----------------------------------------------------------------------------
static void ParseDetailObjectFile( KeyValues& keyValues )
{
	// Iterate over all detail object groups...
	KeyValues* pIter;
	for( pIter = keyValues.GetFirstSubKey(); pIter; pIter = pIter->GetNextKey() )
	{
		if (!pIter->GetFirstSubKey())
			continue;

		int i = s_DetailObjectDict.AddToTail( );
		s_DetailObjectDict[i].m_Name = pIter->GetName() ;
		s_DetailObjectDict[i].m_Density = pIter->GetFloat( "density", 0.0f );

		// Iterate over all detail object groups...
		KeyValues* pIterGroups = pIter->GetFirstSubKey();
		while( pIterGroups )
		{
			if (pIterGroups->GetFirstSubKey())
			{
				ParseDetailGroup( i, pIterGroups );
			}
			pIterGroups = pIterGroups->GetNextKey();
		}
	}
}


//-----------------------------------------------------------------------------
// Finds the name of the detail.vbsp file to use
//-----------------------------------------------------------------------------
static const char *FindDetailVBSPName( void )
{
	for( int i = 0; i < num_entities; i++ )
	{
		char* pEntity = ValueForKey( &entities[i], "classname" );
		if ( !strcmp( pEntity, "worldspawn" ) )
		{
			const char *pDetailVBSP = ValueForKey( &entities[i], "detailvbsp" );
			if ( !pDetailVBSP || !pDetailVBSP[0] ) 
			{
				pDetailVBSP = "detail.vbsp";
			}
			return pDetailVBSP;
		}
	}
	return "detail.vbsp";
}


//-----------------------------------------------------------------------------
// Loads up the detail object dictionary
//-----------------------------------------------------------------------------
void LoadEmitDetailObjectDictionary( const char* pGameDir )
{
	// Set the required global lights filename and try looking in qproject
	const char *pDetailVBSP = FindDetailVBSPName();
	KeyValues * values = new KeyValues( pDetailVBSP );
	if ( values->LoadFromFile( g_pFileSystem, pDetailVBSP ) )
	{
		ParseDetailObjectFile( *values );
	}
	values->deleteThis();
}


//-----------------------------------------------------------------------------
// Selects a detail group
//-----------------------------------------------------------------------------
static int SelectGroup( const DetailObject_t& detail, float alpha )
{
	// Find the two groups whose alpha we're between...
	int start, end;
	for ( start = 0; start < detail.m_Groups.Count() - 1; ++start )
	{
		if (alpha < detail.m_Groups[start+1].m_Alpha)
			break;
	}

	end = start + 1;
	if (end >= detail.m_Groups.Count())
		--end;

	if (start == end)
		return start;

	// Figure out how far we are between start and end...
	float dist = 0.0f;
	float dAlpha = (detail.m_Groups[end].m_Alpha - detail.m_Groups[start].m_Alpha);
	if (dAlpha != 0.0f)
	{
		dist = (alpha - detail.m_Groups[start].m_Alpha) / dAlpha;
	}

	// Pick a number, any number...
	float r = rand() / (float)VALVE_RAND_MAX;

	// When dist == 0, we *always* want start.
	// When dist == 1, we *always* want end
	// That's why this logic looks a little reversed
	return (r > dist) ? start : end;
}


//-----------------------------------------------------------------------------
// Selects a detail object
//-----------------------------------------------------------------------------
static int SelectDetail( DetailObjectGroup_t const& group )
{
	// Pick a number, any number...
	float r = rand() / (float)VALVE_RAND_MAX;

	// Look through the list of models + pick the one associated with this number
	for ( int i = 0; i < group.m_Models.Count(); ++i )
	{
		if (r <= group.m_Models[i].m_Amount)
			return i;
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Adds a detail dictionary element (expected to oftentimes be shared)
//-----------------------------------------------------------------------------
static int AddDetailDictLump( const char* pModelName )
{
	DetailObjectDictLump_t dictLump;
	Q_strncpy( dictLump.m_Name, pModelName, DETAIL_NAME_LENGTH );

	for (int i = s_DetailObjectDictLump.Count(); --i >= 0; )
	{
		if (!memcmp(&s_DetailObjectDictLump[i], &dictLump, sizeof(dictLump) ))
			return i;
	}

	return s_DetailObjectDictLump.AddToTail( dictLump );
}

static int AddDetailSpriteDictLump( const Vector2D *pPos, const Vector2D *pTex )
{
	DetailSpriteDictLump_t dictLump;
	dictLump.m_UL = pPos[0];
	dictLump.m_LR = pPos[1];
	dictLump.m_TexUL = pTex[0];
	dictLump.m_TexLR = pTex[1];

	for (int i = s_DetailSpriteDictLump.Count(); --i >= 0; )
	{
		if (!memcmp(&s_DetailSpriteDictLump[i], &dictLump, sizeof(dictLump) ))
			return i;
	}

	return s_DetailSpriteDictLump.AddToTail( dictLump );
}


//-----------------------------------------------------------------------------
// Computes the leaf that the detail lies in
//-----------------------------------------------------------------------------
static int ComputeDetailLeaf( const Vector& pt )
{
	int node = 0;
	while( node >= 0 )
	{
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		if (DotProduct(pt, pPlane->normal) < pPlane->dist)
			node = pNode->children[1];
		else
			node = pNode->children[0];
	}

	return - node - 1;
}


//-----------------------------------------------------------------------------
// Make sure the details are compiled with static prop
//-----------------------------------------------------------------------------
static bool IsModelValid( const char* pModelName )
{
	StaticPropLookup_t lookup;
	lookup.m_ModelName = pModelName;

	int i = s_StaticPropLookup.Find( lookup );
	if (i != s_StaticPropLookup.InvalidIndex() )
		return s_StaticPropLookup[i].m_IsValid;

	CUtlBuffer buf;
	lookup.m_IsValid = LoadStudioModel( pModelName, "detail_prop", buf );
	if (!lookup.m_IsValid)
	{
		Warning("Error loading studio model \"%s\"!\n", pModelName );
	}

	s_StaticPropLookup.Insert( lookup );
	return lookup.m_IsValid;
}


//-----------------------------------------------------------------------------
// Add a detail to the lump.
//-----------------------------------------------------------------------------
static int s_nDetailOverflow = 0;
static void AddDetailToLump( const char* pModelName, const Vector& pt, const QAngle& angles, int nOrientation )
{
	Assert( pt.IsValid() && angles.IsValid() );

	// Make sure the model is valid...
	if (!IsModelValid(pModelName))
		return;

	if (s_DetailObjectLump.Count() == 65535)
	{
		++s_nDetailOverflow;
		return;
	}

	// Insert an element into the object dictionary if it aint there...
	int i = s_DetailObjectLump.AddToTail( );

	DetailObjectLump_t& objectLump = s_DetailObjectLump[i];
	objectLump.m_DetailModel = AddDetailDictLump( pModelName ); 
	VectorCopy( angles, objectLump.m_Angles );
	VectorCopy( pt, objectLump.m_Origin );
	objectLump.m_Leaf = ComputeDetailLeaf(pt);
	objectLump.m_Lighting.r = 255;
	objectLump.m_Lighting.g = 255;
	objectLump.m_Lighting.b = 255;
	objectLump.m_Lighting.exponent = 0;
	objectLump.m_LightStyles = 0;
	objectLump.m_LightStyleCount = 0;
	objectLump.m_Orientation = nOrientation;
	objectLump.m_Type = DETAIL_PROP_TYPE_MODEL;
}


//-----------------------------------------------------------------------------
// Add a detail sprite to the lump.
//-----------------------------------------------------------------------------
static void AddDetailSpriteToLump( const Vector &vecOrigin, const QAngle &vecAngles, int nOrientation,
								  const Vector2D *pPos, const Vector2D *pTex, float flScale, int iType,
									int iShapeAngle = 0, int iShapeSize = 0, int iSwayAmount = 0 )
{
	// Insert an element into the object dictionary if it aint there...
	int i = s_DetailObjectLump.AddToTail( );

	if (i >= 65535)
	{
		Error( "Error! Too many detail props emitted on this map! (64K max!)n" );
	}

	DetailObjectLump_t& objectLump = s_DetailObjectLump[i];
	objectLump.m_DetailModel = AddDetailSpriteDictLump( pPos, pTex ); 
	VectorCopy( vecAngles, objectLump.m_Angles );
	VectorCopy( vecOrigin, objectLump.m_Origin );
	objectLump.m_Leaf = ComputeDetailLeaf(vecOrigin);
	objectLump.m_Lighting.r = 255;
	objectLump.m_Lighting.g = 255;
	objectLump.m_Lighting.b = 255;
	objectLump.m_Lighting.exponent = 0;
	objectLump.m_LightStyles = 0;
	objectLump.m_LightStyleCount = 0;
	objectLump.m_Orientation = nOrientation;
	objectLump.m_Type = iType;
	objectLump.m_flScale = flScale;
	objectLump.m_ShapeAngle = iShapeAngle;
	objectLump.m_ShapeSize = iShapeSize;
	objectLump.m_SwayAmount = iSwayAmount;
}

static void AddDetailSpriteToLump( const Vector &vecOrigin, const QAngle &vecAngles, DetailModel_t const& model, float flScale )
{
	AddDetailSpriteToLump( vecOrigin,
		vecAngles,
		model.m_Orientation,
		model.m_Pos,
		model.m_Tex,
		flScale,
		model.m_Type,
		model.m_ShapeAngle,
		model.m_ShapeSize,
		model.m_SwayAmount );
}

//-----------------------------------------------------------------------------
// Got a detail! Place it on the surface...
//-----------------------------------------------------------------------------
// BUGBUG: When the global optimizer is on, "normal" gets trashed in this function
// (only when not in the debugger?)
// Printing the values of normal at the bottom of the function fixes it as does
// disabling global optimizations.
static void PlaceDetail( DetailModel_t const& model, const Vector& pt, const Vector& normal )
{
	// But only place it on the surface if it meets the angle constraints...
	float cosAngle = normal.z;

	// Never emit if the angle's too steep
	if (cosAngle < model.m_MaxCosAngle)
		return;

	// If it's between min + max, flip a coin...
	if (cosAngle < model.m_MinCosAngle)
	{
		float probability = (cosAngle - model.m_MaxCosAngle) / 
			(model.m_MinCosAngle - model.m_MaxCosAngle);

		float t = rand() / (float)VALVE_RAND_MAX;
		if (t > probability)
			return;
	}

	// Compute the orientation of the detail
	QAngle angles;
	if (model.m_Flags & MODELFLAG_UPRIGHT)
	{
		// If it's upright, we just select a random yaw
		angles.Init( 0, 360.0f * rand() / (float)VALVE_RAND_MAX, 0.0f );
	}
	else
	{
		// It's not upright, so it must conform to the ground. Choose
		// a random orientation based on the surface normal

		Vector zaxis;
		VectorCopy( normal, zaxis );
		VectorNormalize( zaxis );

		// Choose any two arbitrary axes which are perpendicular to the normal
		Vector xaxis( 1, 0, 0 );
		if (fabs(xaxis.Dot(zaxis)) - 1.0 > -1e-3)
			xaxis.Init( 0, 1, 0 );
		Vector yaxis;
		CrossProduct( zaxis, xaxis, yaxis );
		VectorNormalize( yaxis );
		CrossProduct( yaxis, zaxis, xaxis );
		VectorNormalize( xaxis );
		VMatrix matrix;
		matrix.SetBasisVectors( xaxis, yaxis, zaxis );
		matrix.SetTranslation( vec3_origin );

		float rotAngle = 360.0f * rand() / (float)VALVE_RAND_MAX;
		VMatrix rot = SetupMatrixAxisRot( Vector( 0, 0, 1 ), rotAngle );
		matrix = matrix * rot;

		MatrixToAngles( matrix, angles );
	}

	// FIXME: We may also want a purely random rotation too

	// Insert an element into the object dictionary if it aint there...
	switch ( model.m_Type )
	{
	case DETAIL_PROP_TYPE_MODEL:
		AddDetailToLump( model.m_ModelName.String(), pt, angles, model.m_Orientation );
		break;

	// Sprites and procedural models made from sprites
	case DETAIL_PROP_TYPE_SPRITE:
	default:
		{
			float flScale = 1.0f;
			if ( model.m_flRandomScaleStdDev != 0.0f ) 
			{
				flScale = fabs( RandomGaussianFloat( 1.0f, model.m_flRandomScaleStdDev ) );
			}

			AddDetailSpriteToLump( pt, angles, model, flScale );
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Places Detail Objects on a face
//-----------------------------------------------------------------------------
static void EmitDetailObjectsOnFace( dface_t* pFace, DetailObject_t& detail )
{
	if (pFace->numedges < 3)
		return;

	// We're going to pick a bunch of random points, and then probabilistically
	// decide whether or not to plant a detail object there.

	// Turn the face into a bunch of polygons, and compute the area of each
	int* pSurfEdges = &dsurfedges[pFace->firstedge];
	int vertexIdx = (pSurfEdges[0] < 0);
	int firstVertexIndex = dedges[abs(pSurfEdges[0])].v[vertexIdx];
	dvertex_t* pFirstVertex = &dvertexes[firstVertexIndex];
	for (int i = 1; i < pFace->numedges - 1; ++i )
	{
		int vertexIdx = (pSurfEdges[i] < 0);
		dedge_t* pEdge = &dedges[abs(pSurfEdges[i])];

		// Compute two triangle edges
		Vector e1, e2;
		VectorSubtract( dvertexes[pEdge->v[vertexIdx]].point, pFirstVertex->point, e1 );
		VectorSubtract( dvertexes[pEdge->v[1 - vertexIdx]].point, pFirstVertex->point, e2 );

		// Compute the triangle area
		Vector areaVec;
		CrossProduct( e1, e2, areaVec );
		float normalLength = areaVec.Length();
		float area = 0.5f * normalLength;

		// Compute the number of samples to take
		int numSamples = area * detail.m_Density * 0.000001;

		// Now take a sample, and randomly place an object there
		for (int i = 0; i < numSamples; ++i )
		{
			// Create a random sample...
			float u = rand() / (float)VALVE_RAND_MAX;
			float v = rand() / (float)VALVE_RAND_MAX;
			if (v > 1.0f - u)
			{
				u = 1.0f - u;
				v = 1.0f - v;
				assert( u + v <= 1.0f );
			}

			// Compute alpha
			float alpha = 1.0f;

			// Select a group based on the alpha value
			int group = SelectGroup( detail, alpha );

			// Now that we've got a group, choose a detail
			int model = SelectDetail( detail.m_Groups[group] );
			if (model < 0)
				continue;

			// Got a detail! Place it on the surface...
			Vector pt, normal;
			VectorMA( pFirstVertex->point, u, e1, pt );
			VectorMA( pt, v, e2, pt );
			VectorDivide( areaVec, -normalLength, normal );

			PlaceDetail( detail.m_Groups[group].m_Models[model], pt, normal );
		}
	}
}


//-----------------------------------------------------------------------------
// Places Detail Objects on a face
//-----------------------------------------------------------------------------
static float ComputeDisplacementFaceArea( dface_t* pFace )
{
	float area = 0.0f;

	// Compute the area of the base face
	int* pSurfEdges = &dsurfedges[pFace->firstedge];
	int vertexIdx = (pSurfEdges[0] < 0);
	int firstVertexIndex = dedges[abs(pSurfEdges[0])].v[vertexIdx];
	dvertex_t* pFirstVertex = &dvertexes[firstVertexIndex];
	for (int i = 1; i <= 2; ++i )
	{
		int vertexIdx = (pSurfEdges[i] < 0);
		dedge_t* pEdge = &dedges[abs(pSurfEdges[i])];

		// Compute two triangle edges
		Vector e1, e2;
		VectorSubtract( dvertexes[pEdge->v[vertexIdx]].point, pFirstVertex->point, e1 );
		VectorSubtract( dvertexes[pEdge->v[1 - vertexIdx]].point, pFirstVertex->point, e2 );

		// Compute the triangle area
		Vector areaVec;
		CrossProduct( e1, e2, areaVec );
		float normalLength = areaVec.Length();
		area += 0.5f * normalLength;
	}

	return area;
}


//-----------------------------------------------------------------------------
// Places Detail Objects on a face
//-----------------------------------------------------------------------------
static void EmitDetailObjectsOnDisplacementFace( dface_t* pFace, 
						DetailObject_t& detail, CCoreDispInfo& coreDispInfo )
{
	assert(pFace->numedges == 4);

	// We're going to pick a bunch of random points, and then probabilistically
	// decide whether or not to plant a detail object there.

	// Compute the area of the base face
	float area = ComputeDisplacementFaceArea( pFace );

	// Compute the number of samples to take
	int numSamples = area * detail.m_Density * 0.000001;

	// Now take a sample, and randomly place an object there
	for (int i = 0; i < numSamples; ++i )
	{
		// Create a random sample...
		float u = rand() / (float)VALVE_RAND_MAX;
		float v = rand() / (float)VALVE_RAND_MAX;

		// Compute alpha
		float alpha;
		Vector pt, normal;
		coreDispInfo.GetPositionOnSurface( u, v, pt, &normal, &alpha );
		alpha /= 255.0f;

		// Select a group based on the alpha value
		int group = SelectGroup( detail, alpha );

		// Now that we've got a group, choose a detail
		int model = SelectDetail( detail.m_Groups[group] );
		if (model < 0)
			continue;

		// Got a detail! Place it on the surface...
		PlaceDetail( detail.m_Groups[group].m_Models[model], pt, normal );
	}
}


//-----------------------------------------------------------------------------
// Sort detail objects by leaf
//-----------------------------------------------------------------------------
static int SortFunc( const void *arg1, const void *arg2 )
{
	int nDelta = ((DetailObjectLump_t*)arg1)->m_Leaf - ((DetailObjectLump_t*)arg2)->m_Leaf;
	if ( nDelta < 0 )
		return -1;
	if ( nDelta > 0 )
		return 1;
	return 0;
}


//-----------------------------------------------------------------------------
// Places Detail Objects in the lump
//-----------------------------------------------------------------------------
static void SetLumpData( )
{
	// Sort detail props by leaf
	qsort( s_DetailObjectLump.Base(), s_DetailObjectLump.Count(), sizeof(DetailObjectLump_t), SortFunc );

	GameLumpHandle_t handle = g_GameLumps.GetGameLumpHandle(GAMELUMP_DETAIL_PROPS);
	if (handle != g_GameLumps.InvalidGameLump())
	{
		g_GameLumps.DestroyGameLump(handle);
	}
	int nDictSize = s_DetailObjectDictLump.Count() * sizeof(DetailObjectDictLump_t);
	int nSpriteDictSize = s_DetailSpriteDictLump.Count() * sizeof(DetailSpriteDictLump_t);
	int nObjSize = s_DetailObjectLump.Count() * sizeof(DetailObjectLump_t);
	int nSize = nDictSize + nSpriteDictSize + nObjSize + (3 * sizeof(int));

	handle = g_GameLumps.CreateGameLump( GAMELUMP_DETAIL_PROPS, nSize, 0, GAMELUMP_DETAIL_PROPS_VERSION );

	// Serialize the data
	CUtlBuffer buf( g_GameLumps.GetGameLump(handle), nSize );
	buf.PutInt( s_DetailObjectDictLump.Count() );
	if (nDictSize)
	{
		buf.Put( s_DetailObjectDictLump.Base(), nDictSize );
	}
	buf.PutInt( s_DetailSpriteDictLump.Count() );
	if (nSpriteDictSize)
	{
		buf.Put( s_DetailSpriteDictLump.Base(), nSpriteDictSize );
	}
	buf.PutInt( s_DetailObjectLump.Count() );
	if (nObjSize)
	{
		buf.Put( s_DetailObjectLump.Base(), nObjSize );
	}
}


//-----------------------------------------------------------------------------
// Places Detail Objects in the level
//-----------------------------------------------------------------------------
void EmitDetailModels()
{
	StartPacifier("Placing detail props : ");

	// Place stuff on each face
	dface_t* pFace = dfaces;
	for (int j = 0; j < numfaces; ++j)
	{
		UpdatePacifier( (float)j / (float)numfaces );

		// Get at the material associated with this face
		texinfo_t* pTexInfo = &texinfo[pFace[j].texinfo];
		dtexdata_t* pTexData = GetTexData( pTexInfo->texdata );

		// Try to get at the material
		bool found;
		MaterialSystemMaterial_t handle = 
			FindOriginalMaterial( TexDataStringTable_GetString( pTexData->nameStringTableID ), 
						  &found, false );
		if (!found)
			continue;

		// See if its got any detail objects on it
		const char* pDetailType = GetMaterialVar( handle, "%detailtype" );
		if (!pDetailType)
			continue;

		// Get the detail type...
		DetailObject_t search;
		search.m_Name = pDetailType;
		int objectType = s_DetailObjectDict.Find(search);
		if (objectType < 0)
		{
			Warning("Material %s uses unknown detail object type %s!\n",	
				TexDataStringTable_GetString( pTexData->nameStringTableID ),
				pDetailType);
			continue;
		}

		// Emit objects on a particular face
		DetailObject_t& detail = s_DetailObjectDict[objectType];

		// Initialize the Random Number generators for detail prop placement based on the hammer Face num.
		int	detailpropseed = dfaceids[j].hammerfaceid;
#ifdef WARNSEEDNUMBER
		Warning( "[%d]\n",detailpropseed );
#endif
		srand( detailpropseed );
		RandomSeed( detailpropseed );

		if (pFace[j].dispinfo < 0)
		{
			EmitDetailObjectsOnFace( &pFace[j], detail );
		}
		else
		{
			// Get a CCoreDispInfo. All we need is the triangles and lightmap texture coordinates.
			mapdispinfo_t *pMapDisp = &mapdispinfo[pFace[j].dispinfo];
			CCoreDispInfo coreDispInfo;
			DispMapToCoreDispInfo( pMapDisp, &coreDispInfo, NULL, NULL );

			EmitDetailObjectsOnDisplacementFace( &pFace[j], detail, coreDispInfo );
		}
	}

	// Emit specifically specified detail props
	Vector origin;
	QAngle angles;
	Vector2D pos[2];
	Vector2D tex[2];
	for (int i = 0; i < num_entities; ++i)
	{
		char* pEntity = ValueForKey(&entities[i], "classname");
		if (!strcmp(pEntity, "detail_prop") || !strcmp(pEntity, "prop_detail"))
		{
			GetVectorForKey( &entities[i], "origin", origin );
			GetAnglesForKey( &entities[i], "angles", angles );
			char* pModelName = ValueForKey( &entities[i], "model" );
			int nOrientation = IntForKey( &entities[i], "detailOrientation" );

			AddDetailToLump( pModelName, origin, angles, nOrientation );

			// strip this ent from the .bsp file
			entities[i].epairs = 0;
			continue;
		}

		if (!strcmp(pEntity, "prop_detail_sprite"))
		{
			GetVectorForKey( &entities[i], "origin", origin );
			GetAnglesForKey( &entities[i], "angles", angles );
			int nOrientation = IntForKey( &entities[i], "detailOrientation" );
			GetVector2DForKey( &entities[i], "position_ul", pos[0] );
			GetVector2DForKey( &entities[i], "position_lr", pos[1] );
			GetVector2DForKey( &entities[i], "tex_ul", tex[0] );
			GetVector2DForKey( &entities[i], "tex_size", tex[1] );
			float flTextureSize = FloatForKey( &entities[i], "tex_total_size" );

			tex[1].x += tex[0].x - 0.5f;
			tex[1].y += tex[0].y - 0.5f;
			tex[0].x += 0.5f;
			tex[0].y += 0.5f;
			tex[0] /= flTextureSize;
			tex[1] /= flTextureSize;

			AddDetailSpriteToLump( origin, angles, nOrientation, pos, tex, 1.0f, DETAIL_PROP_TYPE_SPRITE );

			// strip this ent from the .bsp file
			entities[i].epairs = 0;
			continue;
		}
	}

	EndPacifier( true );
}


//-----------------------------------------------------------------------------
// Places Detail Objects in the level
//-----------------------------------------------------------------------------
void EmitDetailObjects()
{
	EmitDetailModels();

	// Done! Now lets add the lumps (destroy previous ones)
	SetLumpData( );

	if ( s_nDetailOverflow != 0 )
	{
		Warning( "Error! Too many detail props on this map. %d were not emitted!\n", s_nDetailOverflow );
	}
}
