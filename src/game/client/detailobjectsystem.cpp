//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws grasses and other small objects  
//
// $Revision: $
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "detailobjectsystem.h"
#include "gamebspfile.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlmap.h"
#include "view.h"
#include "clientmode.h"
#include "iviewrender.h"
#include "bsptreedata.h"
#include "tier0/vprof.h"
#include "engine/ivmodelinfo.h"
#include "materialsystem/imesh.h"
#include "model_types.h"
#include "env_detail_controller.h"
#include "tier0/icommandline.h"
#include "c_world.h"

#include "tier0/valve_minmax_off.h"
#include <algorithm>
#include "tier0/valve_minmax_on.h"

#if defined(DOD_DLL) || defined(CSTRIKE_DLL)
#define USE_DETAIL_SHAPES
#endif

#ifdef USE_DETAIL_SHAPES
#include "engine/ivdebugoverlay.h"
#include "playerenumerator.h"
#endif

#include "materialsystem/imaterialsystemhardwareconfig.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DETAIL_SPRITE_MATERIAL		"detail/detailsprites"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
struct model_t;


ConVar cl_detaildist( "cl_detaildist", "1200", 0, "Distance at which detail props are no longer visible" );
ConVar cl_detailfade( "cl_detailfade", "400", 0, "Distance across which detail props fade in" );
#if defined( USE_DETAIL_SHAPES ) 
ConVar cl_detail_max_sway( "cl_detail_max_sway", "0", FCVAR_ARCHIVE, "Amplitude of the detail prop sway" );
ConVar cl_detail_avoid_radius( "cl_detail_avoid_radius", "0", FCVAR_ARCHIVE, "radius around detail sprite to avoid players" );
ConVar cl_detail_avoid_force( "cl_detail_avoid_force", "0", FCVAR_ARCHIVE, "force with which to avoid players ( in units, percentage of the width of the detail sprite )" );
ConVar cl_detail_avoid_recover_speed( "cl_detail_avoid_recover_speed", "0", FCVAR_ARCHIVE, "how fast to recover position after avoiding players" );
#endif

// Per detail instance information
struct DetailModelAdvInfo_t
{
	// precaculated angles for shaped sprites
	Vector m_vecAnglesForward[3];
	Vector m_vecAnglesRight[3];		// better to save this mem and calc per sprite ?
	Vector m_vecAnglesUp[3];

	// direction we're avoiding the player
	Vector m_vecCurrentAvoid;

	// yaw to sway on
	float m_flSwayYaw;

	// size of the shape
	float m_flShapeSize;

	int m_iShapeAngle;
	float m_flSwayAmount;

};

class CDetailObjectSystemPerLeafData
{
	unsigned short	m_FirstDetailProp;
	unsigned short	m_DetailPropCount;
	int				m_DetailPropRenderFrame;

	CDetailObjectSystemPerLeafData( void )
	{
		m_FirstDetailProp = 0;
		m_DetailPropCount = 0;
		m_DetailPropRenderFrame = -1;
	}
};

//-----------------------------------------------------------------------------
// Detail models
//-----------------------------------------------------------------------------
struct SptrintInfo_t
{
	unsigned short	m_nSpriteIndex;
	float16			m_flScale;
};

class CDetailModel : public IClientUnknown, public IClientRenderable
{
	DECLARE_CLASS_NOBASE( CDetailModel );

public:
	CDetailModel();
	~CDetailModel();


	// Initialization
	bool InitCommon( int index, const Vector& org, const QAngle& angles );
	bool Init( int index, const Vector& org, const QAngle& angles, model_t* pModel, 
		ColorRGBExp32 lighting, int lightstyle, unsigned char lightstylecount, int orientation );

	bool InitSprite( int index, bool bFlipped, const Vector& org, const QAngle& angles,
					 unsigned short nSpriteIndex, 
					 ColorRGBExp32 lighting, int lightstyle, unsigned char lightstylecount,
					 int orientation, float flScale, unsigned char type,
					 unsigned char shapeAngle, unsigned char shapeSize, unsigned char swayAmount );

	void SetAlpha( unsigned char alpha ) { m_Alpha = alpha; }


	// IClientUnknown overrides.
public:

	virtual IClientUnknown*		GetIClientUnknown()		{ return this; }
	virtual ICollideable*		GetCollideable()		{ return 0; }		// Static props DO implement this.
	virtual IClientNetworkable*	GetClientNetworkable()	{ return 0; }
	virtual IClientRenderable*	GetClientRenderable()	{ return this; }
	virtual IClientEntity*		GetIClientEntity()		{ return 0; }
	virtual C_BaseEntity*		GetBaseEntity()			{ return 0; }
	virtual IClientThinkable*	GetClientThinkable()	{ return 0; }


	// IClientRenderable overrides.
public:

	virtual int					GetBody() { return 0; }
	virtual const Vector&		GetRenderOrigin( );
	virtual const QAngle&		GetRenderAngles( );
	virtual const matrix3x4_t &	RenderableToWorldTransform();
	virtual bool				ShouldDraw();
	virtual bool				IsTwoPass( void ) { return false; }
	virtual void				OnThreadedDrawSetup() {}
	virtual bool				IsTransparent( void );
	virtual const model_t*		GetModel( ) const;
	virtual int					DrawModel( int flags );
	virtual void				ComputeFxBlend( );
	virtual int					GetFxBlend( );
	virtual bool				SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime );
	virtual void				SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );
	virtual bool				UsesFlexDelayedWeights() { return false; }
	virtual void				DoAnimationEvents( void );
	virtual void				GetRenderBounds( Vector& mins, Vector& maxs );
	virtual IPVSNotify*			GetPVSNotifyInterface();
	virtual void				GetRenderBoundsWorldspace( Vector& mins, Vector& maxs );
	virtual bool				ShouldReceiveProjectedTextures( int flags );
	virtual bool				GetShadowCastDistance( float *pDist, ShadowType_t shadowType ) const			{ return false; }
	virtual bool				GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const	{ return false; }
	virtual bool				UsesPowerOfTwoFrameBufferTexture();
	virtual bool				UsesFullFrameBufferTexture();
	virtual bool				IgnoresZBuffer( void ) const { return false; }
	virtual bool				LODTest() { return true; }

	virtual ClientShadowHandle_t	GetShadowHandle() const;
	virtual ClientRenderHandle_t&	RenderHandle();
	virtual void				GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType );
	virtual bool IsShadowDirty( )			     { return false; }
	virtual void MarkShadowDirty( bool bDirty )  {}
	virtual IClientRenderable *GetShadowParent() { return NULL; }
	virtual IClientRenderable *FirstShadowChild(){ return NULL; }
	virtual IClientRenderable *NextShadowPeer()  { return NULL; }
	virtual ShadowType_t		ShadowCastType() { return SHADOWS_NONE; }
	virtual void CreateModelInstance()			 {}
	virtual ModelInstanceHandle_t GetModelInstance() { return MODEL_INSTANCE_INVALID; }
	virtual int					LookupAttachment( const char *pAttachmentName ) { return -1; }
	virtual bool				GetAttachment( int number, matrix3x4_t &matrix );
	virtual	bool				GetAttachment( int number, Vector &origin, QAngle &angles );
	virtual float *				GetRenderClipPlane() { return NULL; }
	virtual int					GetSkin() { return 0; }
	virtual void				RecordToolMessage() {}

	void GetColorModulation( float* color );

	// Computes the render angles for screen alignment
	void ComputeAngles( void );

	// Calls the correct rendering func
	void DrawSprite( CMeshBuilder &meshBuilder );

	// Returns the number of quads the sprite will draw
	int QuadsToDraw() const;

	// Draw functions for the different types of sprite
	void DrawTypeSprite( CMeshBuilder &meshBuilder );


#ifdef USE_DETAIL_SHAPES
	void DrawTypeShapeCross( CMeshBuilder &meshBuilder );
	void DrawTypeShapeTri( CMeshBuilder &meshBuilder );

	// check for players nearby and angle away from them
	void UpdatePlayerAvoid( void );

	void InitShapedSprite( unsigned char shapeAngle, unsigned char shapeSize, unsigned char swayAmount );
	void InitShapeTri();
	void InitShapeCross();

	void DrawSwayingQuad( CMeshBuilder &meshBuilder, Vector vecOrigin, Vector vecSway, Vector2D texul, Vector2D texlr, unsigned char *color,
		Vector width, Vector height );
#endif

	int GetType() const { return m_Type; }
	unsigned char GetAlpha() const { return m_Alpha; }

	bool IsDetailModelTranslucent();

	// IHandleEntity stubs.
public:
	virtual void SetRefEHandle( const CBaseHandle &handle )	{ Assert( false ); }
	virtual const CBaseHandle& GetRefEHandle() const		{ Assert( false ); return *((CBaseHandle*)0); }

	//---------------------------------
	struct LightStyleInfo_t
	{
		unsigned int	m_LightStyle:24;
		unsigned int	m_LightStyleCount:8;
	};

protected:
	Vector	m_Origin;
	QAngle	m_Angles;

	ColorRGBExp32	m_Color;

	unsigned char	m_Orientation:2;
	unsigned char	m_Type:2;
	unsigned char	m_bHasLightStyle:1;
	unsigned char	m_bFlipped:1;

	unsigned char	m_Alpha;

	static CUtlMap<CDetailModel *, LightStyleInfo_t> gm_LightStylesMap;

#pragma warning( disable : 4201 ) //warning C4201: nonstandard extension used : nameless struct/union
	union
	{
		model_t* m_pModel;
		SptrintInfo_t m_SpriteInfo;
	};
#pragma warning( default : 4201 )

#ifdef USE_DETAIL_SHAPES
	// pointer to advanced properties
	DetailModelAdvInfo_t *m_pAdvInfo;
#endif
};

static ConVar mat_fullbright( "mat_fullbright", "0", FCVAR_CHEAT ); // hook into engine's cvars..
extern ConVar r_DrawDetailProps;


//-----------------------------------------------------------------------------
// Dictionary for detail sprites
//-----------------------------------------------------------------------------
struct DetailPropSpriteDict_t 
{
	Vector2D	m_UL;		// Coordinate of upper left
	Vector2D	m_LR;		// Coordinate of lower right
	Vector2D	m_TexUL;	// Texcoords of upper left
	Vector2D	m_TexLR;	// Texcoords of lower left
};

struct FastSpriteX4_t
{
	// mess with this structure without care and you'll be in a world of trouble. layout matters.
	FourVectors m_Pos;
	fltx4 m_HalfWidth;
	fltx4 m_Height;
	uint8 m_RGBColor[4][4];
	DetailPropSpriteDict_t *m_pSpriteDefs[4];

	void ReplicateFirstEntryToOthers( void )
	{
		m_HalfWidth = ReplicateX4( SubFloat( m_HalfWidth, 0 ) );
		m_Height = ReplicateX4( SubFloat( m_Height, 0 ) );

		for( int i = 1; i < 4; i++ )
			for( int j = 0; j < 4; j++ )
			{
				m_RGBColor[i][j] = m_RGBColor[0][j];
			}
		m_Pos.x = ReplicateX4( SubFloat( m_Pos.x, 0 ) );
		m_Pos.y = ReplicateX4( SubFloat( m_Pos.y, 0 ) );
		m_Pos.z = ReplicateX4( SubFloat( m_Pos.z, 0 ) );
	}

};


struct FastSpriteQuadBuildoutBufferX4_t
{
	// mess with this structure without care and you'll be in a world of trouble. layout matters.
	FourVectors m_Coords[4];
	uint8 m_RGBColor[4][4];
	fltx4 m_Alpha;
	DetailPropSpriteDict_t *m_pSpriteDefs[4];
};

struct FastSpriteQuadBuildoutBufferNonSIMDView_t
{
	// mess with this structure without care and you'll be in a world of trouble. layout matters.
	float m_flX0[4], m_flY0[4], m_flZ0[4];
	float m_flX1[4], m_flY1[4], m_flZ1[4];
	float m_flX2[4], m_flY2[4], m_flZ2[4];
	float m_flX3[4], m_flY3[4], m_flZ3[4];

	uint8 m_RGBColor[4][4];
	float m_Alpha[4];
	DetailPropSpriteDict_t *m_pSpriteDefs[4];
};


class CFastDetailLeafSpriteList : public CClientLeafSubSystemData
{
	friend class CDetailObjectSystem;
	int m_nNumSprites;
	int m_nNumSIMDSprites;									// #sprites/4, rounded up
	// simd pointers into larger array - don't free individually or you will be sad
	FastSpriteX4_t *m_pSprites;

	// state for partially drawn sprite lists
	int m_nNumPendingSprites;
	int m_nStartSpriteIndex;

	CFastDetailLeafSpriteList( void )
	{
		m_nNumPendingSprites = 0;
		m_nStartSpriteIndex = 0;
	}

};




//-----------------------------------------------------------------------------
// Responsible for managing detail objects
//-----------------------------------------------------------------------------
class CDetailObjectSystem : public IDetailObjectSystem, public ISpatialLeafEnumerator
{
public:
	char const *Name() { return "DetailObjectSystem"; }

	// constructor, destructor
	CDetailObjectSystem();
	~CDetailObjectSystem();

	bool IsPerFrame() { return false; }

	// Init, shutdown
	bool Init()
	{
		m_flDefaultFadeStart = cl_detailfade.GetFloat();
		m_flDefaultFadeEnd = cl_detaildist.GetFloat();
		return true;
	}
	void PostInit() {}
	void Shutdown() {}

	// Level init, shutdown
	void LevelInitPreEntity();
	void LevelInitPostEntity();
	void LevelShutdownPreEntity();
	void LevelShutdownPostEntity();

	void OnSave() {}
	void OnRestore() {}
	void SafeRemoveIfDesired() {}

    // Gets a particular detail object
	IClientRenderable* GetDetailModel( int idx );

	// Prepares detail for rendering 
	void BuildDetailObjectRenderLists( const Vector &vViewOrigin );

	// Renders all opaque detail objects in a particular set of leaves
	void RenderOpaqueDetailObjects( int nLeafCount, LeafIndex_t *pLeafList );

	// Renders all translucent detail objects in a particular set of leaves
	void RenderTranslucentDetailObjects( const Vector &viewOrigin, const Vector &viewForward, const Vector &viewRight, const Vector &viewUp, int nLeafCount, LeafIndex_t *pLeafList );

	// Renders all translucent detail objects in a particular leaf up to a particular point
	void RenderTranslucentDetailObjectsInLeaf( const Vector &viewOrigin, const Vector &viewForward, const Vector &viewRight, const Vector &viewUp, int nLeaf, const Vector *pVecClosestPoint );
	void RenderFastTranslucentDetailObjectsInLeaf( const Vector &viewOrigin, const Vector &viewForward, const Vector &viewRight, const Vector &viewUp, int nLeaf, const Vector *pVecClosestPoint );



	// Call this before rendering translucent detail objects
	void BeginTranslucentDetailRendering( );

	// Method of ISpatialLeafEnumerator
	bool EnumerateLeaf( int leaf, int context );

	DetailPropLightstylesLump_t& DetailLighting( int i ) { return m_DetailLighting[i]; }
	DetailPropSpriteDict_t& DetailSpriteDict( int i ) { return m_DetailSpriteDict[i]; }

private:
	struct DetailModelDict_t
	{
		model_t* m_pModel;
	};

	struct EnumContext_t
	{
		Vector m_vViewOrigin;
		int	m_BuildWorldListNumber;
	};

	struct SortInfo_t
	{
		int m_nIndex;
		float m_flDistance;
	};

	int BuildOutSortedSprites( CFastDetailLeafSpriteList *pData,
							   Vector const &viewOrigin,
							   Vector const &viewForward,
							   Vector const &viewRight,
							   Vector const &viewUp );

	void RenderFastSprites( const Vector &viewOrigin, const Vector &viewForward, const Vector &viewRight, const Vector &viewUp, int nLeafCount, LeafIndex_t const * pLeafList );

	void UnserializeFastSprite( FastSpriteX4_t *pSpritex4, int nSubField, DetailObjectLump_t const &lump, bool bFlipped, Vector const &posOffset );

	// Unserialization
	void ScanForCounts( CUtlBuffer& buf, int *pNumOldStyleObjects, 
						int *pNumFastSpritesToAllocate, int *nMaxOldInLeaf,
						int *nMaxFastInLeaf ) const;

	void UnserializeModelDict( CUtlBuffer& buf );
	void UnserializeDetailSprites( CUtlBuffer& buf );
	void UnserializeModels( CUtlBuffer& buf );
	void UnserializeModelLighting( CUtlBuffer& buf );

	Vector GetSpriteMiddleBottomPosition( DetailObjectLump_t const &lump ) const;
	// Count the number of detail sprites in the leaf list
	int CountSpritesInLeafList( int nLeafCount, LeafIndex_t *pLeafList ) const;

	// Count the number of detail sprite quads in the leaf list
	int CountSpriteQuadsInLeafList( int nLeafCount, LeafIndex_t *pLeafList ) const;

	int CountFastSpritesInLeafList( int nLeafCount, LeafIndex_t const *pLeafList, int *nMaxInLeaf ) const;

	void FreeSortBuffers( void );

	// Sorts sprites in back-to-front order
	static bool SortLessFunc( const SortInfo_t &left, const SortInfo_t &right );
	int SortSpritesBackToFront( int nLeaf, const Vector &viewOrigin, const Vector &viewForward, SortInfo_t *pSortInfo );

	// For fast detail object insertion
	IterationRetval_t EnumElement( int userId, int context );

	CUtlVector<DetailModelDict_t>			m_DetailObjectDict;
	CUtlVector<CDetailModel>				m_DetailObjects;
	CUtlVector<DetailPropSpriteDict_t>		m_DetailSpriteDict;
	CUtlVector<DetailPropSpriteDict_t>		m_DetailSpriteDictFlipped;
	CUtlVector<DetailPropLightstylesLump_t>	m_DetailLighting;
	FastSpriteX4_t *m_pFastSpriteData;

	// Necessary to get sprites to batch correctly
	CMaterialReference m_DetailSpriteMaterial;
	CMaterialReference m_DetailWireframeMaterial;

	// State stored off for rendering detail sprites in a single leaf
	int m_nSpriteCount;
	int m_nFirstSprite;
	int m_nSortedLeaf;
	int m_nSortedFastLeaf;
	SortInfo_t *m_pSortInfo;
	SortInfo_t *m_pFastSortInfo;
	FastSpriteQuadBuildoutBufferX4_t *m_pBuildoutBuffer;

	float m_flDefaultFadeStart;
	float m_flDefaultFadeEnd;


	// pre calcs for the current render frame
	float m_flCurMaxSqDist;
	float m_flCurFadeSqDist;
	float m_flCurFalloffFactor;

};


//-----------------------------------------------------------------------------
// System for dealing with detail objects
//-----------------------------------------------------------------------------
static CDetailObjectSystem s_DetailObjectSystem;

IDetailObjectSystem* DetailObjectSystem()
{
	return &s_DetailObjectSystem;
}



//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------

CUtlMap<CDetailModel *, CDetailModel::LightStyleInfo_t> CDetailModel::gm_LightStylesMap( DefLessFunc( CDetailModel * ) );

bool CDetailModel::InitCommon( int index, const Vector& org, const QAngle& angles )
{
	VectorCopy( org, m_Origin );
	VectorCopy( angles, m_Angles );
	m_Alpha = 255;

	return true;
}


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------

// NOTE: If DetailPropType_t enum changes, change CDetailModel::QuadsToDraw
static int s_pQuadCount[4] =
{
	0, //DETAIL_PROP_TYPE_MODEL
	1, //DETAIL_PROP_TYPE_SPRITE
	4, //DETAIL_PROP_TYPE_SHAPE_CROSS
	3, //DETAIL_PROP_TYPE_SHAPE_TRI
};

inline int CDetailModel::QuadsToDraw() const
{
	return s_pQuadCount[m_Type];
}

	
//-----------------------------------------------------------------------------
// Data accessors
//-----------------------------------------------------------------------------
const Vector& CDetailModel::GetRenderOrigin( void )
{
	return m_Origin;
}

const QAngle& CDetailModel::GetRenderAngles( void )
{
	return m_Angles;
}

const matrix3x4_t &CDetailModel::RenderableToWorldTransform()
{
	// Setup our transform.
	static matrix3x4_t mat;
	AngleMatrix( GetRenderAngles(), GetRenderOrigin(), mat );
	return mat;
}

bool CDetailModel::GetAttachment( int number, matrix3x4_t &matrix )
{
	MatrixCopy( RenderableToWorldTransform(), matrix );
	return true;
}

bool CDetailModel::GetAttachment( int number, Vector &origin, QAngle &angles )
{
	origin = m_Origin;
	angles = m_Angles;
	return true;
}

bool CDetailModel::IsTransparent( void )
{
	return (m_Alpha < 255) || modelinfo->IsTranslucent(m_pModel);
}

bool CDetailModel::ShouldDraw()
{
	// Don't draw in commander mode
	return g_pClientMode->ShouldDrawDetailObjects();
}

void CDetailModel::GetRenderBounds( Vector& mins, Vector& maxs )
{
	int nModelType = modelinfo->GetModelType( m_pModel );
	if (nModelType == mod_studio || nModelType == mod_brush)
	{
		modelinfo->GetModelRenderBounds( GetModel(), mins, maxs );
	}
	else
	{
		mins.Init( 0,0,0 );
		maxs.Init( 0,0,0 );
	}
}

IPVSNotify* CDetailModel::GetPVSNotifyInterface()
{
	return NULL;
}

void CDetailModel::GetRenderBoundsWorldspace( Vector& mins, Vector& maxs )
{
	DefaultRenderBoundsWorldspace( this, mins, maxs );
}

bool CDetailModel::ShouldReceiveProjectedTextures( int flags )
{
	return false;
}

bool CDetailModel::UsesPowerOfTwoFrameBufferTexture()
{
	return false;
}

bool CDetailModel::UsesFullFrameBufferTexture()
{
	return false;
}

void CDetailModel::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	GetRenderBounds( mins, maxs );
}

ClientShadowHandle_t CDetailModel::GetShadowHandle() const
{
	return CLIENTSHADOW_INVALID_HANDLE;
}

ClientRenderHandle_t& CDetailModel::RenderHandle()
{
	AssertMsg( 0, "CDetailModel has no render handle" );
	return *((ClientRenderHandle_t*)NULL);
}	


//-----------------------------------------------------------------------------
// Render setup
//-----------------------------------------------------------------------------
bool CDetailModel::SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )
{
	if (!m_pModel)
		return false;

	// Setup our transform.
	matrix3x4_t parentTransform;
	const QAngle &vRenderAngles = GetRenderAngles();
	const Vector &vRenderOrigin = GetRenderOrigin();
	AngleMatrix( vRenderAngles, parentTransform );
	parentTransform[0][3] = vRenderOrigin.x;
	parentTransform[1][3] = vRenderOrigin.y;
	parentTransform[2][3] = vRenderOrigin.z;

	// Just copy it on down baby
	studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( m_pModel );
	for (int i = 0; i < pStudioHdr->numbones; i++) 
	{
		MatrixCopy( parentTransform, pBoneToWorldOut[i] );
	}

	return true;
}

void	CDetailModel::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
}

void	CDetailModel::DoAnimationEvents( void )
{
}


//-----------------------------------------------------------------------------
// Render baby!
//-----------------------------------------------------------------------------
const model_t* CDetailModel::GetModel( ) const
{
	return m_pModel;
}

int CDetailModel::DrawModel( int flags )
{
	if ((m_Alpha == 0) || (!m_pModel))
		return 0;

	int drawn = modelrender->DrawModel( 
		flags, 
		this,
		MODEL_INSTANCE_INVALID,
		-1,		// no entity index
		m_pModel,
		m_Origin,
		m_Angles,
		0,	// skin
		0,	// body
		0  // hitboxset
		);
	return drawn;
}


//-----------------------------------------------------------------------------
// Determine alpha and blend amount for transparent objects based on render state info
//-----------------------------------------------------------------------------
void CDetailModel::ComputeFxBlend( )
{
	// Do nothing, it's already calculate in our m_Alpha
}

int CDetailModel::GetFxBlend( )
{
	return m_Alpha;
}

//-----------------------------------------------------------------------------
// Detail models stuff
//-----------------------------------------------------------------------------
CDetailModel::CDetailModel()
{
	m_Color.r = m_Color.g = m_Color.b = 255;
	m_Color.exponent = 0;
	m_bFlipped = 0;
	m_bHasLightStyle = 0;

#ifdef USE_DETAIL_SHAPES
	m_pAdvInfo = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
CDetailModel::~CDetailModel()
{
#ifdef USE_DETAIL_SHAPES
	// delete advanced
	if ( m_pAdvInfo )
	{
		delete m_pAdvInfo;
		m_pAdvInfo = NULL;
	}
#endif

	if ( m_bHasLightStyle )
		gm_LightStylesMap.Remove( this );
}


//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
bool CDetailModel::Init( int index, const Vector& org, const QAngle& angles, 
	model_t* pModel, ColorRGBExp32 lighting, int lightstyle, unsigned char lightstylecount, 
	int orientation)
{
	m_Color = lighting;
	if ( lightstylecount > 0)
	{
		m_bHasLightStyle = 1;
		int iInfo = gm_LightStylesMap.Insert( this );
		if ( lightstyle >= 0x1000000 || lightstylecount >= 100  )
			Error( "Light style overflow\n" );
		gm_LightStylesMap[iInfo].m_LightStyle = lightstyle;
		gm_LightStylesMap[iInfo].m_LightStyleCount = lightstylecount;
	}
	m_Orientation = orientation;
	m_Type = DETAIL_PROP_TYPE_MODEL;
	m_pModel = pModel;
	return InitCommon( index, org, angles );
}

bool CDetailModel::InitSprite( int index, bool bFlipped, const Vector& org, const QAngle& angles, unsigned short nSpriteIndex, 
	ColorRGBExp32 lighting, int lightstyle, unsigned char lightstylecount, int orientation, float flScale,
	unsigned char type, unsigned char shapeAngle, unsigned char shapeSize, unsigned char swayAmount )
{
	m_Color = lighting;
	if ( lightstylecount > 0)
	{
		m_bHasLightStyle = 1;
		int iInfo = gm_LightStylesMap.Insert( this );
		if ( lightstyle >= 0x1000000 || lightstylecount >= 100  )
			Error( "Light style overflow\n" );
		gm_LightStylesMap[iInfo].m_LightStyle = lightstyle;
		gm_LightStylesMap[iInfo].m_LightStyleCount = lightstylecount;
	}
	m_Orientation = orientation;
	m_SpriteInfo.m_nSpriteIndex = nSpriteIndex;
	m_Type = type;
	m_SpriteInfo.m_flScale.SetFloat( flScale );

#ifdef USE_DETAIL_SHAPES
	m_pAdvInfo = NULL;
	Assert( type <= 3 );
	// precalculate angles for shapes
	if ( type == DETAIL_PROP_TYPE_SHAPE_TRI || type == DETAIL_PROP_TYPE_SHAPE_CROSS || swayAmount > 0 )
	{
		m_Angles = angles;
		InitShapedSprite( shapeAngle, shapeSize, swayAmount);
	}

#endif

	m_bFlipped = bFlipped;
	return InitCommon( index, org, angles );
}

#ifdef USE_DETAIL_SHAPES
void CDetailModel::InitShapedSprite( unsigned char shapeAngle, unsigned char shapeSize, unsigned char swayAmount )
{
	// Set up pointer to advanced shape properties object ( per instance )
	Assert( m_pAdvInfo == NULL );
	m_pAdvInfo = new DetailModelAdvInfo_t;
	Assert( m_pAdvInfo );

	if ( m_pAdvInfo )
	{
		m_pAdvInfo->m_iShapeAngle = shapeAngle;
		m_pAdvInfo->m_flSwayAmount = (float)swayAmount / 255.0f;
		m_pAdvInfo->m_flShapeSize = (float)shapeSize / 255.0f;
		m_pAdvInfo->m_vecCurrentAvoid = vec3_origin;
		m_pAdvInfo->m_flSwayYaw = random->RandomFloat( 0, 180 );
	}

	switch ( m_Type )
	{
	case DETAIL_PROP_TYPE_SHAPE_TRI:
		InitShapeTri();
		break;

	case DETAIL_PROP_TYPE_SHAPE_CROSS:
		InitShapeCross();
		break;

	default:	// sprite will get here
		break;
	}
}

void CDetailModel::InitShapeTri( void )
{
	// store the three sets of directions
	matrix3x4_t matrix;

	// Convert roll/pitch only to matrix
	AngleMatrix( m_Angles, matrix );

	// calculate the vectors for the three sides so they can be used in the sorting test
	// as well as in drawing
	for ( int i=0; i<3; i++ )
	{		
		// Convert desired rotation to angles
		QAngle anglesRotated( m_pAdvInfo->m_iShapeAngle, i*120, 0 );

		Vector rotForward, rotRight, rotUp;
		AngleVectors( anglesRotated, &rotForward, &rotRight, &rotUp );

		// Rotate direction vectors
		VectorRotate( rotForward, matrix, m_pAdvInfo->m_vecAnglesForward[i] );
		VectorRotate( rotRight, matrix, m_pAdvInfo->m_vecAnglesRight[i] );
		VectorRotate( rotUp, matrix, m_pAdvInfo->m_vecAnglesUp[i] );
	}
}

void CDetailModel::InitShapeCross( void )
{
	AngleVectors( m_Angles,
		&m_pAdvInfo->m_vecAnglesForward[0],
		&m_pAdvInfo->m_vecAnglesRight[0],
		&m_pAdvInfo->m_vecAnglesUp[0] );
}
#endif

//-----------------------------------------------------------------------------
// Color, alpha modulation
//-----------------------------------------------------------------------------
void CDetailModel::GetColorModulation( float *color )
{
	if (mat_fullbright.GetInt() == 1)
	{
		color[0] = color[1] = color[2] = 1.0f;
		return;
	}

	Vector tmp;
	Vector normal( 1, 0, 0);
	engine->ComputeDynamicLighting( m_Origin, &normal, tmp );

	float val = engine->LightStyleValue( 0 );
	color[0] = tmp[0] + val * TexLightToLinear( m_Color.r, m_Color.exponent );
	color[1] = tmp[1] + val * TexLightToLinear( m_Color.g, m_Color.exponent );
	color[2] = tmp[2] + val * TexLightToLinear( m_Color.b, m_Color.exponent );

	// Add in the lightstyles
	if ( m_bHasLightStyle )
	{
		int iInfo = gm_LightStylesMap.Find( this );
		Assert( iInfo != gm_LightStylesMap.InvalidIndex() );
		if ( iInfo != gm_LightStylesMap.InvalidIndex() )
		{
			int nLightStyles = gm_LightStylesMap[iInfo].m_LightStyleCount;
			int iLightStyle = gm_LightStylesMap[iInfo].m_LightStyle;
			for (int i = 0; i < nLightStyles; ++i)
			{
				DetailPropLightstylesLump_t& lighting = s_DetailObjectSystem.DetailLighting( iLightStyle + i );
				val = engine->LightStyleValue( lighting.m_Style );
				if (val != 0)
				{
					color[0] += val * TexLightToLinear( lighting.m_Lighting.r, lighting.m_Lighting.exponent ); 
					color[1] += val * TexLightToLinear( lighting.m_Lighting.g, lighting.m_Lighting.exponent ); 
					color[2] += val * TexLightToLinear( lighting.m_Lighting.b, lighting.m_Lighting.exponent ); 
				}
			}
		}
	}

	// Gamma correct....
	engine->LinearToGamma( color, color );
}


//-----------------------------------------------------------------------------
// Is the model itself translucent, regardless of modulation?
//-----------------------------------------------------------------------------
bool CDetailModel::IsDetailModelTranslucent()
{
	// FIXME: This is only true for my first pass of this feature
	if (m_Type >= DETAIL_PROP_TYPE_SPRITE)
		return true;

	return modelinfo->IsTranslucent(GetModel());
}


//-----------------------------------------------------------------------------
// Computes the render angles for screen alignment
//-----------------------------------------------------------------------------
void CDetailModel::ComputeAngles( void )
{
	switch( m_Orientation )
	{
	case 0:
		break;

	case 1:
		{
			Vector vecDir;
			VectorSubtract( CurrentViewOrigin(), m_Origin, vecDir );
			VectorAngles( vecDir, m_Angles );
		}
		break;

	case 2:
		{
			Vector vecDir;
			VectorSubtract( CurrentViewOrigin(), m_Origin, vecDir );
			vecDir.z = 0.0f;
			VectorAngles( vecDir, m_Angles );
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Select which rendering func to call
//-----------------------------------------------------------------------------
void CDetailModel::DrawSprite( CMeshBuilder &meshBuilder )
{
	switch( m_Type )
	{
#ifdef USE_DETAIL_SHAPES
	case DETAIL_PROP_TYPE_SHAPE_CROSS:
		DrawTypeShapeCross( meshBuilder );
		break;

	case DETAIL_PROP_TYPE_SHAPE_TRI:
		DrawTypeShapeTri( meshBuilder );
		break;
#endif
	case DETAIL_PROP_TYPE_SPRITE:
		DrawTypeSprite( meshBuilder );
		break;

	default:
		Assert(0);
		break;
	}
}


//-----------------------------------------------------------------------------
// Draws the single sprite type
//-----------------------------------------------------------------------------
void CDetailModel::DrawTypeSprite( CMeshBuilder &meshBuilder )
{
	Assert( m_Type == DETAIL_PROP_TYPE_SPRITE );

	Vector vecColor;
	GetColorModulation( vecColor.Base() );

	unsigned char color[4];
	color[0] = (unsigned char)(vecColor[0] * 255.0f);
	color[1] = (unsigned char)(vecColor[1] * 255.0f);
	color[2] = (unsigned char)(vecColor[2] * 255.0f);
	color[3] = m_Alpha;

	DetailPropSpriteDict_t &dict = s_DetailObjectSystem.DetailSpriteDict( m_SpriteInfo.m_nSpriteIndex );

	Vector vecOrigin, dx, dy;
	AngleVectors( m_Angles, NULL, &dx, &dy );

	Vector2D ul, lr;
	float scale = m_SpriteInfo.m_flScale.GetFloat();
	Vector2DMultiply( dict.m_UL, scale, ul );
	Vector2DMultiply( dict.m_LR, scale, lr );

#ifdef USE_DETAIL_SHAPES
	UpdatePlayerAvoid();

	Vector vecSway = vec3_origin;

	if ( m_pAdvInfo )
	{
		vecSway = m_pAdvInfo->m_vecCurrentAvoid * m_SpriteInfo.m_flScale.GetFloat();
		float flSwayAmplitude = m_pAdvInfo->m_flSwayAmount * cl_detail_max_sway.GetFloat();
		if ( flSwayAmplitude > 0 )
		{
			// sway based on time plus a random seed that is constant for this instance of the sprite
			vecSway += dx * sin(gpGlobals->curtime+m_Origin.x) * flSwayAmplitude;
		}
	}
#endif

	VectorMA( m_Origin, ul.x, dx, vecOrigin );
	VectorMA( vecOrigin, ul.y, dy, vecOrigin );
	dx *= (lr.x - ul.x);
	dy *= (lr.y - ul.y);

	Vector2D texul, texlr;
	texul = dict.m_TexUL;
	texlr = dict.m_TexLR;

	if ( !m_bFlipped )
	{
		texul.x = dict.m_TexLR.x;
		texlr.x = dict.m_TexUL.x;
	}

#ifndef USE_DETAIL_SHAPES
	meshBuilder.Position3fv( vecOrigin.Base() );
#else
	meshBuilder.Position3fv( (vecOrigin+vecSway).Base() );
#endif

	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2fv( 0, texul.Base() );
	meshBuilder.AdvanceVertex();

	vecOrigin += dy;
	meshBuilder.Position3fv( vecOrigin.Base() );
	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2f( 0, texul.x, texlr.y );
	meshBuilder.AdvanceVertex();

	vecOrigin += dx;
	meshBuilder.Position3fv( vecOrigin.Base() );
	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2fv( 0, texlr.Base() );
	meshBuilder.AdvanceVertex();

	vecOrigin -= dy;
#ifndef USE_DETAIL_SHAPES
	meshBuilder.Position3fv( vecOrigin.Base() );
#else
	meshBuilder.Position3fv( (vecOrigin+vecSway).Base() );
#endif
	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2f( 0, texlr.x, texul.y );
	meshBuilder.AdvanceVertex();
}

//-----------------------------------------------------------------------------
// draws a procedural model, cross shape
// two perpendicular sprites
//-----------------------------------------------------------------------------
#ifdef USE_DETAIL_SHAPES
void CDetailModel::DrawTypeShapeCross( CMeshBuilder &meshBuilder )
{
	Assert( m_Type == DETAIL_PROP_TYPE_SHAPE_CROSS );

	Vector vecColor;
	GetColorModulation( vecColor.Base() );

	unsigned char color[4];
	color[0] = (unsigned char)(vecColor[0] * 255.0f);
	color[1] = (unsigned char)(vecColor[1] * 255.0f);
	color[2] = (unsigned char)(vecColor[2] * 255.0f);
	color[3] = m_Alpha;

	DetailPropSpriteDict_t &dict = s_DetailObjectSystem.DetailSpriteDict( m_SpriteInfo.m_nSpriteIndex );

	Vector2D texul, texlr;
	texul = dict.m_TexUL;
	texlr = dict.m_TexLR;

	// What a shameless re-use of bits (m_pModel == 0 when it should be flipped horizontally)
	if ( !m_pModel )
	{
		texul.x = dict.m_TexLR.x;
		texlr.x = dict.m_TexUL.x;
	}

	Vector2D texumid, texlmid;
	texumid.y = texul.y;
	texlmid.y = texlr.y;
	texumid.x = texlmid.x = ( texul.x + texlr.x ) / 2;

	Vector2D texll;
	texll.x = texul.x;
	texll.y = texlr.y;

	Vector2D ul, lr;
	float flScale = m_SpriteInfo.m_flScale.GetFloat();
	Vector2DMultiply( dict.m_UL, flScale, ul );
	Vector2DMultiply( dict.m_LR, flScale, lr );

	float flSizeX = ( lr.x - ul.x ) / 2;
	float flSizeY = ( lr.y - ul.y );

	UpdatePlayerAvoid();

	// sway based on time plus a random seed that is constant for this instance of the sprite
	Vector vecSway = ( m_pAdvInfo->m_vecCurrentAvoid * flSizeX * 2 );
	float flSwayAmplitude = m_pAdvInfo->m_flSwayAmount * cl_detail_max_sway.GetFloat();
	if ( flSwayAmplitude > 0 )
	{
		vecSway += UTIL_YawToVector( m_pAdvInfo->m_flSwayYaw ) * sin(gpGlobals->curtime+m_Origin.x) * flSwayAmplitude;
	}

	Vector vecOrigin;
	VectorMA( m_Origin, ul.y, m_pAdvInfo->m_vecAnglesUp[0], vecOrigin );

	Vector forward, right, up;
	forward = m_pAdvInfo->m_vecAnglesForward[0] * flSizeX;
	right = m_pAdvInfo->m_vecAnglesRight[0] * flSizeX;
	up = m_pAdvInfo->m_vecAnglesUp[0] * flSizeY;

	// figure out drawing order so the branches sort properly
	// do dot products with the forward and right vectors to determine the quadrant the viewer is in
	// assume forward points North , right points East
	/*
			   N
			   |
			3  |  0
		  W---------E
			2  |  1
			   |
			   S
	*/ 
	// eg if they are in quadrant 0, set iBranch to 0, and the draw order will be
	// 0, 1, 2, 3, or South, west, north, east
	Vector viewOffset = CurrentViewOrigin() - m_Origin;
	bool bForward = ( DotProduct( forward, viewOffset ) > 0 );
	bool bRight = ( DotProduct( right, viewOffset ) > 0 );	
	int iBranch = bForward ? ( bRight ? 0 : 3 ) : ( bRight ? 1 : 2 );

	//debugoverlay->AddLineOverlay( m_Origin, m_Origin + right * 20, 255, 0, 0, true, 0.01 );
	//debugoverlay->AddLineOverlay( m_Origin, m_Origin + forward * 20, 0, 0, 255, true, 0.01 );

	int iDrawn = 0;
	while( iDrawn < 4 )
	{
		switch( iBranch )
		{
		case 0:		// south
			DrawSwayingQuad( meshBuilder, vecOrigin, vecSway, texumid, texlr, color, -forward, up );
			break;
		case 1:		// west
			DrawSwayingQuad( meshBuilder, vecOrigin, vecSway, texumid, texll, color, -right, up );
			break;
		case 2:		// north
			DrawSwayingQuad( meshBuilder, vecOrigin, vecSway, texumid, texll, color, forward, up );
			break;
		case 3:		// east
			DrawSwayingQuad( meshBuilder, vecOrigin, vecSway, texumid, texlr, color, right, up );
			break;
		}

		iDrawn++;
		iBranch++;
		if ( iBranch > 3 )
			iBranch = 0;
	}	
}
#endif

//-----------------------------------------------------------------------------
// draws a procedural model, tri shape
//-----------------------------------------------------------------------------
#ifdef USE_DETAIL_SHAPES
void CDetailModel::DrawTypeShapeTri( CMeshBuilder &meshBuilder )
{
	Assert( m_Type == DETAIL_PROP_TYPE_SHAPE_TRI );

	Vector vecColor;
	GetColorModulation( vecColor.Base() );

	unsigned char color[4];
	color[0] = (unsigned char)(vecColor[0] * 255.0f);
	color[1] = (unsigned char)(vecColor[1] * 255.0f);
	color[2] = (unsigned char)(vecColor[2] * 255.0f);
	color[3] = m_Alpha;

	DetailPropSpriteDict_t &dict = s_DetailObjectSystem.DetailSpriteDict( m_SpriteInfo.m_nSpriteIndex );

	Vector2D texul, texlr;
	texul = dict.m_TexUL;
	texlr = dict.m_TexLR;

	// What a shameless re-use of bits (m_pModel == 0 when it should be flipped horizontally)
	if ( !m_pModel )
	{
		texul.x = dict.m_TexLR.x;
		texlr.x = dict.m_TexUL.x;
	}

	Vector2D ul, lr;
	float flScale = m_SpriteInfo.m_flScale.GetFloat();
	Vector2DMultiply( dict.m_UL, flScale, ul );
	Vector2DMultiply( dict.m_LR, flScale, lr );

	// sort the sides relative to the view origin
	Vector viewOffset = CurrentViewOrigin() - m_Origin;

	// three sides, A, B, C, counter-clockwise from A is the unrotated side
	bool bOutsideA = DotProduct( m_pAdvInfo->m_vecAnglesForward[0], viewOffset ) > 0;
	bool bOutsideB = DotProduct( m_pAdvInfo->m_vecAnglesForward[1], viewOffset ) > 0;
	bool bOutsideC = DotProduct( m_pAdvInfo->m_vecAnglesForward[2], viewOffset ) > 0;

	int iBranch = 0;
	if ( bOutsideA && !bOutsideB )
		iBranch = 1;
	else if ( bOutsideB && !bOutsideC )
		iBranch = 2;

	float flHeight, flWidth;
	flHeight = (lr.y - ul.y);
	flWidth = (lr.x - ul.x);

	Vector vecSway;
	Vector vecOrigin;
	Vector vecHeight, vecWidth;

	UpdatePlayerAvoid();

	Vector vecSwayYaw = UTIL_YawToVector( m_pAdvInfo->m_flSwayYaw );
	float flSwayAmplitude = m_pAdvInfo->m_flSwayAmount * cl_detail_max_sway.GetFloat();

	int iDrawn = 0;
	while( iDrawn < 3 )
	{
		vecHeight = m_pAdvInfo->m_vecAnglesUp[iBranch] * flHeight;
		vecWidth = m_pAdvInfo->m_vecAnglesRight[iBranch] * flWidth;

		VectorMA( m_Origin, ul.x, m_pAdvInfo->m_vecAnglesRight[iBranch], vecOrigin );
		VectorMA( vecOrigin, ul.y, m_pAdvInfo->m_vecAnglesUp[iBranch], vecOrigin );
		VectorMA( vecOrigin, m_pAdvInfo->m_flShapeSize*flWidth, m_pAdvInfo->m_vecAnglesForward[iBranch], vecOrigin );

		// sway is calculated per side so they don't sway exactly the same
		Vector vecSway = ( m_pAdvInfo->m_vecCurrentAvoid * flWidth ) + 
			vecSwayYaw * sin(gpGlobals->curtime+m_Origin.x+iBranch) * flSwayAmplitude;

		DrawSwayingQuad( meshBuilder, vecOrigin, vecSway, texul, texlr, color, vecWidth, vecHeight );
		
		iDrawn++;
		iBranch++;
		if ( iBranch > 2 )
			iBranch = 0;
	}	
}
#endif

//-----------------------------------------------------------------------------
// checks for nearby players and pushes the detail to the side
//-----------------------------------------------------------------------------
#ifdef USE_DETAIL_SHAPES
void CDetailModel::UpdatePlayerAvoid( void )
{
	float flForce = cl_detail_avoid_force.GetFloat();

	if ( flForce < 0.1 )
		return;

	if ( m_pAdvInfo == NULL )
		return;

	// get players in a radius
	float flRadius = cl_detail_avoid_radius.GetFloat();
	float flRecoverSpeed = cl_detail_avoid_recover_speed.GetFloat();

	Vector vecAvoid;
	C_BaseEntity *pEnt;

	float flMaxForce = 0;
	Vector vecMaxAvoid(0,0,0);

	CPlayerEnumerator avoid( flRadius, m_Origin );
	::partition->EnumerateElementsInSphere( PARTITION_CLIENT_SOLID_EDICTS, m_Origin, flRadius, false, &avoid );

	// Okay, decide how to avoid if there's anything close by
	int c = avoid.GetObjectCount();
	for ( int i=0; i<c+1; i++ )	// +1 for the local player we tack on the end
	{
		if ( i == c )
		{
			pEnt = C_BasePlayer::GetLocalPlayer();
			if ( !pEnt ) continue;
		}
		else
			pEnt = avoid.GetObject( i );
		
		vecAvoid = m_Origin - pEnt->GetAbsOrigin();
		vecAvoid.z = 0;

		float flDist = vecAvoid.Length2D();

		if ( flDist > flRadius )
			continue;

		float flForceScale = RemapValClamped( flDist, 0, flRadius, flForce, 0.0 );

		if ( flForceScale > flMaxForce )
		{
			flMaxForce = flForceScale;
			vecAvoid.NormalizeInPlace();
			vecAvoid *= flMaxForce;
			vecMaxAvoid = vecAvoid;
		}
	}

	// if we are being moved, move fast. Else we recover at a slow rate
	if ( vecMaxAvoid.Length2D() > m_pAdvInfo->m_vecCurrentAvoid.Length2D() )
		flRecoverSpeed = 10;	// fast approach

	m_pAdvInfo->m_vecCurrentAvoid[0] = Approach( vecMaxAvoid[0], m_pAdvInfo->m_vecCurrentAvoid[0], flRecoverSpeed );
	m_pAdvInfo->m_vecCurrentAvoid[1] = Approach( vecMaxAvoid[1], m_pAdvInfo->m_vecCurrentAvoid[1], flRecoverSpeed );
	m_pAdvInfo->m_vecCurrentAvoid[2] = Approach( vecMaxAvoid[2], m_pAdvInfo->m_vecCurrentAvoid[2], flRecoverSpeed );
}
#endif

//-----------------------------------------------------------------------------
// draws a quad that sways on the top two vertices
// pass vecOrigin as the top left vertex position
//-----------------------------------------------------------------------------
#ifdef USE_DETAIL_SHAPES
void CDetailModel::DrawSwayingQuad( CMeshBuilder &meshBuilder, Vector vecOrigin, Vector vecSway, Vector2D texul, Vector2D texlr, unsigned char *color,
								   Vector width, Vector height )
{
	meshBuilder.Position3fv( (vecOrigin + vecSway).Base() );
	meshBuilder.TexCoord2fv( 0, texul.Base() );
	meshBuilder.Color4ubv( color );
	meshBuilder.AdvanceVertex();

	vecOrigin += height;
	meshBuilder.Position3fv( vecOrigin.Base() );
	meshBuilder.TexCoord2f( 0, texul.x, texlr.y );
	meshBuilder.Color4ubv( color );
	meshBuilder.AdvanceVertex();

	vecOrigin += width;
	meshBuilder.Position3fv( vecOrigin.Base() );
	meshBuilder.TexCoord2fv( 0, texlr.Base() );
	meshBuilder.Color4ubv( color );
	meshBuilder.AdvanceVertex();

	vecOrigin -= height;
	meshBuilder.Position3fv( (vecOrigin + vecSway).Base() );
	meshBuilder.TexCoord2f( 0, texlr.x, texul.y );
	meshBuilder.Color4ubv( color );
	meshBuilder.AdvanceVertex();
}
#endif

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CDetailObjectSystem::CDetailObjectSystem() : m_DetailSpriteDict( 0, 32 ), m_DetailObjectDict( 0, 32 ), m_DetailSpriteDictFlipped( 0, 32 )
{
	m_pFastSpriteData = NULL;
	m_pSortInfo = NULL;
	m_pFastSortInfo = NULL;
	m_pBuildoutBuffer = NULL;
}

void CDetailObjectSystem::FreeSortBuffers( void )
{
	if ( m_pSortInfo )
	{
		MemAlloc_FreeAligned(  m_pSortInfo );
		m_pSortInfo = NULL;
	}
	if ( m_pFastSortInfo )
	{
		MemAlloc_FreeAligned(  m_pFastSortInfo );
		m_pFastSortInfo = NULL;
	}
	if ( m_pBuildoutBuffer )
	{
		MemAlloc_FreeAligned(  m_pBuildoutBuffer );
		m_pBuildoutBuffer = NULL;
	}
}

CDetailObjectSystem::~CDetailObjectSystem()
{
	if ( m_pFastSpriteData )
	{
		MemAlloc_FreeAligned( m_pFastSpriteData );
		m_pFastSpriteData = NULL;
	}
	FreeSortBuffers();

}

	   
//-----------------------------------------------------------------------------
// Level init, shutdown
//-----------------------------------------------------------------------------
void CDetailObjectSystem::LevelInitPreEntity()
{
	// Prepare the translucent detail sprite material; we only have 1!
	m_DetailSpriteMaterial.Init( "detail/detailsprites", TEXTURE_GROUP_OTHER );
	m_DetailWireframeMaterial.Init( "debug/debugspritewireframe", TEXTURE_GROUP_OTHER );

	// Version check
	if (engine->GameLumpVersion( GAMELUMP_DETAIL_PROPS ) < 4)
	{
		Warning("Map uses old detail prop file format.. ignoring detail props\n");
		return;
	}

	MEM_ALLOC_CREDIT();

	// Unserialize
	int size = engine->GameLumpSize( GAMELUMP_DETAIL_PROPS );
	CUtlMemory<unsigned char> fileMemory;
	fileMemory.EnsureCapacity( size );
	if (engine->LoadGameLump( GAMELUMP_DETAIL_PROPS, fileMemory.Base(), size ))
	{
		CUtlBuffer buf( fileMemory.Base(), size, CUtlBuffer::READ_ONLY );
		UnserializeModelDict( buf );

		switch (engine->GameLumpVersion( GAMELUMP_DETAIL_PROPS ) )
		{
		case 4:
			UnserializeDetailSprites( buf );
			UnserializeModels( buf );
			break;
		}
	}

	if ( m_DetailObjects.Count() || m_DetailSpriteDict.Count() )
	{
		// There are detail objects in the level, so precache the material
		PrecacheMaterial( DETAIL_SPRITE_MATERIAL );
		IMaterial *pMat = m_DetailSpriteMaterial;
		// adjust for non-square textures (cropped)
		float flRatio = (float)( pMat->GetMappingWidth() ) / pMat->GetMappingHeight();
		if ( flRatio > 1.0 )
		{
			for( int i = 0; i<m_DetailSpriteDict.Count(); i++ )
			{
				m_DetailSpriteDict[i].m_TexUL.y *= flRatio;
				m_DetailSpriteDict[i].m_TexLR.y *= flRatio;
				m_DetailSpriteDictFlipped[i].m_TexUL.y *= flRatio;
				m_DetailSpriteDictFlipped[i].m_TexLR.y *= flRatio;
			}
		}
	}

	int detailPropLightingLump;
	if( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
	{
		detailPropLightingLump = GAMELUMP_DETAIL_PROP_LIGHTING_HDR;
	}
	else
	{
		detailPropLightingLump = GAMELUMP_DETAIL_PROP_LIGHTING;
	}
	size = engine->GameLumpSize( detailPropLightingLump );

	fileMemory.EnsureCapacity( size );
	if (engine->LoadGameLump( detailPropLightingLump, fileMemory.Base(), size ))
	{
		CUtlBuffer buf( fileMemory.Base(), size, CUtlBuffer::READ_ONLY );
		UnserializeModelLighting( buf );
	}
}


void CDetailObjectSystem::LevelInitPostEntity()
{
	const char *pDetailSpriteMaterial = DETAIL_SPRITE_MATERIAL;
	C_World *pWorld = GetClientWorldEntity();
	if ( pWorld && pWorld->GetDetailSpriteMaterial() && *(pWorld->GetDetailSpriteMaterial()) )
	{
		pDetailSpriteMaterial = pWorld->GetDetailSpriteMaterial(); 
	}
	m_DetailSpriteMaterial.Init( pDetailSpriteMaterial, TEXTURE_GROUP_OTHER );

	if ( GetDetailController() )
	{
		cl_detailfade.SetValue( MIN( m_flDefaultFadeStart, GetDetailController()->m_flFadeStartDist ) );
		cl_detaildist.SetValue( MIN( m_flDefaultFadeEnd, GetDetailController()->m_flFadeEndDist ) );
	}
	else
	{
		// revert to default values if the map doesn't specify
		cl_detailfade.SetValue( m_flDefaultFadeStart );
		cl_detaildist.SetValue( m_flDefaultFadeEnd );
	}
}

void CDetailObjectSystem::LevelShutdownPreEntity()
{
	m_DetailObjects.Purge();
	m_DetailObjectDict.Purge();
	m_DetailSpriteDict.Purge();
	m_DetailSpriteDictFlipped.Purge();
	m_DetailLighting.Purge();
	m_DetailSpriteMaterial.Shutdown();
	if ( m_pFastSpriteData )
	{
		MemAlloc_FreeAligned( m_pFastSpriteData );
		m_pFastSpriteData = NULL;
	}
	FreeSortBuffers();

}

void CDetailObjectSystem::LevelShutdownPostEntity()
{
	m_DetailWireframeMaterial.Shutdown();
}

//-----------------------------------------------------------------------------
// Before each view, blat out the stored detail sprite state
//-----------------------------------------------------------------------------
void CDetailObjectSystem::BeginTranslucentDetailRendering( )
{
	m_nSortedLeaf = -1;
	m_nSortedFastLeaf = -1;
	m_nSpriteCount = m_nFirstSprite = 0;
}


//-----------------------------------------------------------------------------
// Gets a particular detail object
//-----------------------------------------------------------------------------
IClientRenderable* CDetailObjectSystem::GetDetailModel( int idx )
{
	// FIXME: This is necessary because we have intermixed models + sprites
	// in a single list (m_DetailObjects)
	if (m_DetailObjects[idx].GetType() != DETAIL_PROP_TYPE_MODEL)
		return NULL;
	
	return &m_DetailObjects[idx];
}


//-----------------------------------------------------------------------------
// Unserialization
//-----------------------------------------------------------------------------
void CDetailObjectSystem::UnserializeModelDict( CUtlBuffer& buf )
{
	int count = buf.GetInt();
	m_DetailObjectDict.EnsureCapacity( count );
	while ( --count >= 0 )
	{
		DetailObjectDictLump_t lump;
		buf.Get( &lump, sizeof(DetailObjectDictLump_t) );
		
		DetailModelDict_t dict;
		dict.m_pModel = (model_t *)engine->LoadModel( lump.m_Name, true );

		// Don't allow vertex-lit models
		if (modelinfo->IsModelVertexLit(dict.m_pModel))
		{
			Warning("Detail prop model %s is using vertex-lit materials!\nIt must use unlit materials!\n", lump.m_Name );
			dict.m_pModel = (model_t *)engine->LoadModel( "models/error.mdl" );
		}

		m_DetailObjectDict.AddToTail( dict );
	}
}

void CDetailObjectSystem::UnserializeDetailSprites( CUtlBuffer& buf )
{
	int count = buf.GetInt();
	m_DetailSpriteDict.EnsureCapacity( count );
	m_DetailSpriteDictFlipped.EnsureCapacity( count );
	while ( --count >= 0 )
	{
		int i = m_DetailSpriteDict.AddToTail();
		buf.Get( &m_DetailSpriteDict[i], sizeof(DetailSpriteDictLump_t) );
		int flipi = m_DetailSpriteDictFlipped.AddToTail();
		m_DetailSpriteDictFlipped[flipi] = m_DetailSpriteDict[i];
		::V_swap( m_DetailSpriteDictFlipped[flipi].m_TexUL.x, m_DetailSpriteDictFlipped[flipi].m_TexLR.x );
	}
}


void CDetailObjectSystem::UnserializeModelLighting( CUtlBuffer& buf )
{
	int count = buf.GetInt();
	m_DetailLighting.EnsureCapacity( count );
	while ( --count >= 0 )
	{
		int i = m_DetailLighting.AddToTail();
		buf.Get( &m_DetailLighting[i], sizeof(DetailPropLightstylesLump_t) );
	}
}


ConVar cl_detail_multiplier( "cl_detail_multiplier", "1", FCVAR_CHEAT, "extra details to create" );

#define SPRITE_MULTIPLIER  ( cl_detail_multiplier.GetInt() )

ConVar cl_fastdetailsprites( "cl_fastdetailsprites", "1", FCVAR_CHEAT, "whether to use new detail sprite system");

static bool DetailObjectIsFastSprite( DetailObjectLump_t const & lump )
{
	return (
		( cl_fastdetailsprites.GetInt() ) &&
		( lump.m_Type == DETAIL_PROP_TYPE_SPRITE ) &&
		( lump.m_LightStyleCount == 0 ) &&
		( lump.m_Orientation == 2 ) &&
		( lump.m_ShapeAngle == 0 ) &&
		( lump.m_ShapeSize == 0 ) &&
		( lump.m_SwayAmount == 0 ) );
}


void CDetailObjectSystem::ScanForCounts( CUtlBuffer& buf,
										 int *pNumOldStyleObjects,
										 int *pNumFastSpritesToAllocate,
										 int *nMaxNumOldSpritesInLeaf,
										 int *nMaxNumFastSpritesInLeaf
										 ) const
{
	int oldpos = buf.TellGet();								// we need to seek back
	int count = buf.GetInt();

	int nOld = 0;
	int nFast = 0;
	int detailObjectLeaf = -1;

	int nNumOldInLeaf = 0;
	int nNumFastInLeaf = 0;
	int nMaxOld = 0;
	int nMaxFast = 0;
	while ( --count >= 0 )
	{
		DetailObjectLump_t lump;
		buf.Get( &lump, sizeof(DetailObjectLump_t) );
		
		// We rely on the fact that details objects are sorted by leaf in the
		// bsp file for this
		if ( detailObjectLeaf != lump.m_Leaf )
		{
			// need to pad nfast to next sse boundary
			nFast += ( 0 - nFast ) & 3;
			nMaxFast = MAX( nMaxFast, nNumFastInLeaf );
			nMaxOld = MAX( nMaxOld, nNumOldInLeaf );
			nNumOldInLeaf = 0;
			nNumFastInLeaf = 0;
			detailObjectLeaf = lump.m_Leaf;

		}

		if ( DetailObjectIsFastSprite( lump ) )
		{
			nFast += SPRITE_MULTIPLIER;
			nNumFastInLeaf += SPRITE_MULTIPLIER;
		}
		else
		{
			nOld += SPRITE_MULTIPLIER;
			nNumOldInLeaf += SPRITE_MULTIPLIER;
		}
	}

	// need to pad nfast to next sse boundary
	nFast += ( 0 - nFast ) & 3;
	nMaxFast = MAX( nMaxFast, nNumFastInLeaf );
	nMaxOld = MAX( nMaxOld, nNumOldInLeaf );

	buf.SeekGet( CUtlBuffer::SEEK_HEAD, oldpos );
	*pNumFastSpritesToAllocate = nFast;
	*pNumOldStyleObjects = nOld;
	nMaxFast = ( 3 + nMaxFast ) & ~3;
	*nMaxNumOldSpritesInLeaf = nMaxOld;
	*nMaxNumFastSpritesInLeaf = nMaxFast;
	
}

//-----------------------------------------------------------------------------
// Unserialize all models
//-----------------------------------------------------------------------------
void CDetailObjectSystem::UnserializeModels( CUtlBuffer& buf )
{
	int firstDetailObject = 0;
	int detailObjectCount = 0;
	int detailObjectLeaf = -1;

	int nNumOldStyleObjects;
	int nNumFastSpritesToAllocate;
	int nMaxOldInLeaf;
	int nMaxFastInLeaf;
	ScanForCounts( buf, &nNumOldStyleObjects, &nNumFastSpritesToAllocate, &nMaxOldInLeaf, &nMaxFastInLeaf );

	FreeSortBuffers();

	if ( nMaxOldInLeaf )
	{
		m_pSortInfo = reinterpret_cast<SortInfo_t *> (
			MemAlloc_AllocAligned( (3 + nMaxOldInLeaf ) * sizeof( SortInfo_t ), sizeof( fltx4 ) ) );
	}
	if ( nMaxFastInLeaf )
	{
		m_pFastSortInfo = reinterpret_cast<SortInfo_t *> (
			MemAlloc_AllocAligned( (3 + nMaxFastInLeaf ) * sizeof( SortInfo_t ), sizeof( fltx4 ) ) );

		m_pBuildoutBuffer = reinterpret_cast<FastSpriteQuadBuildoutBufferX4_t *> (
			MemAlloc_AllocAligned( 
				( 1 + nMaxFastInLeaf / 4 ) * sizeof( FastSpriteQuadBuildoutBufferX4_t ),
				sizeof( fltx4 ) ) );
	}

	if ( nNumFastSpritesToAllocate )
	{
		Assert( ( nNumFastSpritesToAllocate & 3 ) == 0 );
		Assert( ! m_pFastSpriteData );						// wtf? didn't free?
		m_pFastSpriteData = reinterpret_cast<FastSpriteX4_t *> (
			MemAlloc_AllocAligned( 
				( nNumFastSpritesToAllocate >> 2 ) * sizeof( FastSpriteX4_t ),
				sizeof( fltx4 ) ) );
	}

	m_DetailObjects.EnsureCapacity( nNumOldStyleObjects  );

	int count = buf.GetInt();
	
	int nCurFastObject = 0;
	int nNumFastObjectsInCurLeaf = 0;
	FastSpriteX4_t *pCurFastSpriteOut = m_pFastSpriteData;

	bool bFlipped = true;
	while ( --count >= 0 )
	{
		bFlipped = !bFlipped;
		DetailObjectLump_t lump;
		buf.Get( &lump, sizeof(DetailObjectLump_t) );
		
		// We rely on the fact that details objects are sorted by leaf in the
		// bsp file for this
		if ( detailObjectLeaf != lump.m_Leaf )
		{
			if (detailObjectLeaf != -1)
			{
				if ( nNumFastObjectsInCurLeaf )
				{
					CFastDetailLeafSpriteList *pNew = new CFastDetailLeafSpriteList;
					pNew->m_nNumSprites = nNumFastObjectsInCurLeaf;
					pNew->m_nNumSIMDSprites = ( 3 + nNumFastObjectsInCurLeaf ) >> 2;
					pNew->m_pSprites = pCurFastSpriteOut;
					pCurFastSpriteOut += pNew->m_nNumSIMDSprites;
					ClientLeafSystem()->SetSubSystemDataInLeaf( 
						detailObjectLeaf, CLSUBSYSTEM_DETAILOBJECTS, pNew );
					// round to see boundary
					nCurFastObject += ( 0 - nCurFastObject ) & 3;
					nNumFastObjectsInCurLeaf = 0;
				}
				ClientLeafSystem()->SetDetailObjectsInLeaf( detailObjectLeaf, 
					firstDetailObject, detailObjectCount );
			}

			detailObjectLeaf = lump.m_Leaf;
			firstDetailObject = m_DetailObjects.Count();
			detailObjectCount = 0;
		}

		if ( DetailObjectIsFastSprite( lump ) )
		{
			for( int i =0 ; i < SPRITE_MULTIPLIER ; i++)
			{
				FastSpriteX4_t *pSpritex4 = m_pFastSpriteData +  (nCurFastObject >> 2 );
				int nSubField = ( nCurFastObject & 3 );
				Vector pos(0,0,0);
				if ( i ) 
				{
					pos += RandomVector( -50, 50 );
					pos.z = 0;
				}
				UnserializeFastSprite( pSpritex4, nSubField, lump, bFlipped, pos );
				if ( nSubField == 0 )
					pSpritex4->ReplicateFirstEntryToOthers(); // keep bad numbers out to prevent denormals, etc
				nCurFastObject++;
				nNumFastObjectsInCurLeaf++;
			}
		}
		else
		{
			switch( lump.m_Type )
			{
				case DETAIL_PROP_TYPE_MODEL:
				{
					int newObj = m_DetailObjects.AddToTail();
					m_DetailObjects[newObj].Init(
						newObj, lump.m_Origin, lump.m_Angles, 
						m_DetailObjectDict[lump.m_DetailModel].m_pModel, lump.m_Lighting,
						lump.m_LightStyles, lump.m_LightStyleCount, lump.m_Orientation );
					++detailObjectCount;
				}
				break;

				case DETAIL_PROP_TYPE_SPRITE:
				case DETAIL_PROP_TYPE_SHAPE_CROSS:
				case DETAIL_PROP_TYPE_SHAPE_TRI:
				{
					for( int i=0;i<SPRITE_MULTIPLIER;i++)
					{
						Vector pos = lump.m_Origin;
						if ( i != 0)
						{
							pos += RandomVector( -50, 50 );
							pos. z = lump.m_Origin.z;
						}
						int newObj = m_DetailObjects.AddToTail();
						m_DetailObjects[newObj].InitSprite( 
							newObj, bFlipped, pos, lump.m_Angles, 
							lump.m_DetailModel, lump.m_Lighting,
							lump.m_LightStyles, lump.m_LightStyleCount, lump.m_Orientation, lump.m_flScale,
							lump.m_Type, lump.m_ShapeAngle, lump.m_ShapeSize, lump.m_SwayAmount );
						++detailObjectCount;
					}
				}
				break;
			}
		}
	}

	
	if (detailObjectLeaf != -1)
	{
		if ( nNumFastObjectsInCurLeaf )
		{
			CFastDetailLeafSpriteList *pNew = new CFastDetailLeafSpriteList;
			pNew->m_nNumSprites = nNumFastObjectsInCurLeaf;
			pNew->m_nNumSIMDSprites = ( 3 + nNumFastObjectsInCurLeaf ) >> 2;
			pNew->m_pSprites = pCurFastSpriteOut;
			pCurFastSpriteOut += pNew->m_nNumSIMDSprites;
			ClientLeafSystem()->SetSubSystemDataInLeaf( 
				detailObjectLeaf, CLSUBSYSTEM_DETAILOBJECTS, pNew );
		}
		ClientLeafSystem()->SetDetailObjectsInLeaf( detailObjectLeaf, 
													firstDetailObject, detailObjectCount );
	}
}


Vector CDetailObjectSystem::GetSpriteMiddleBottomPosition( DetailObjectLump_t const &lump ) const
{
	DetailPropSpriteDict_t &dict = s_DetailObjectSystem.DetailSpriteDict( lump.m_DetailModel );

	Vector vecDir;
	QAngle Angles;

	VectorSubtract( lump.m_Origin + Vector(0,-100,0), lump.m_Origin, vecDir );
	vecDir.z = 0.0f;
	VectorAngles( vecDir, Angles );

	Vector vecOrigin, dx, dy;
	AngleVectors( Angles, NULL, &dx, &dy );

	Vector2D ul, lr;
	float scale = lump.m_flScale;
	Vector2DMultiply( dict.m_UL, scale, ul );
	Vector2DMultiply( dict.m_LR, scale, lr );

	VectorMA( lump.m_Origin, ul.x, dx, vecOrigin );
	VectorMA( vecOrigin, ul.y, dy, vecOrigin );
	dx *= (lr.x - ul.x);
	dy *= (lr.y - ul.y);

	Vector2D texul, texlr;
	texul = dict.m_TexUL;
	texlr = dict.m_TexLR;

	return vecOrigin + dy + 0.5 * dx;
}


void CDetailObjectSystem::UnserializeFastSprite( FastSpriteX4_t *pSpritex4, int nSubField, DetailObjectLump_t const &lump, bool bFlipped, Vector const &posOffset )
{
	Vector pos = lump.m_Origin + posOffset;
	pos = GetSpriteMiddleBottomPosition( lump ) + posOffset;

	pSpritex4->m_Pos.X( nSubField ) = pos.x;
	pSpritex4->m_Pos.Y( nSubField ) = pos.y;
	pSpritex4->m_Pos.Z( nSubField ) = pos.z;
	DetailPropSpriteDict_t *pSDef = &m_DetailSpriteDict[lump.m_DetailModel];

	SubFloat( pSpritex4->m_HalfWidth, nSubField ) = 0.5 * lump.m_flScale * ( pSDef->m_LR.x - pSDef->m_UL.x );
	SubFloat( pSpritex4->m_Height, nSubField ) = lump.m_flScale * ( pSDef->m_LR.y - pSDef->m_UL.y );
	if ( !bFlipped )
	{
		pSDef = &m_DetailSpriteDictFlipped[lump.m_DetailModel];
	}
	// do packed color
	ColorRGBExp32 rgbcolor = lump.m_Lighting;
	float color[4];
	color[0] = TexLightToLinear( rgbcolor.r, rgbcolor.exponent );
	color[1] = TexLightToLinear( rgbcolor.g, rgbcolor.exponent );
	color[2] = TexLightToLinear( rgbcolor.b, rgbcolor.exponent );
	color[3] = 255;
	engine->LinearToGamma( color, color );
	pSpritex4->m_RGBColor[nSubField][0] = 255.0 * color[0];
	pSpritex4->m_RGBColor[nSubField][1] = 255.0 * color[1];
	pSpritex4->m_RGBColor[nSubField][2] = 255.0 * color[2];
	pSpritex4->m_RGBColor[nSubField][3] = 255;

	pSpritex4->m_pSpriteDefs[nSubField] = pSDef;
}



//-----------------------------------------------------------------------------
// Renders all opaque detail objects in a particular set of leaves
//-----------------------------------------------------------------------------
void CDetailObjectSystem::RenderOpaqueDetailObjects( int nLeafCount, LeafIndex_t *pLeafList )
{
	// FIXME: Implement!
}


//-----------------------------------------------------------------------------
// Count the number of detail sprites in the leaf list
//-----------------------------------------------------------------------------
int CDetailObjectSystem::CountSpritesInLeafList( int nLeafCount, LeafIndex_t *pLeafList ) const
{
	VPROF_BUDGET( "CDetailObjectSystem::CountSpritesInLeafList", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	int nPropCount = 0;
	int nFirstDetailObject, nDetailObjectCount;
	for ( int i = 0; i < nLeafCount; ++i )
	{
		// FIXME: This actually counts *everything* in the leaf, which is ok for now
		// given how we're using it
		ClientLeafSystem()->GetDetailObjectsInLeaf( pLeafList[i], nFirstDetailObject, nDetailObjectCount );
		nPropCount += nDetailObjectCount;
	}

	return nPropCount;
}

//-----------------------------------------------------------------------------
// Count the number of fast sprites in the leaf list
//-----------------------------------------------------------------------------
int CDetailObjectSystem::CountFastSpritesInLeafList( int nLeafCount, LeafIndex_t const *pLeafList,
													 int *nMaxFoundInLeaf ) const
{
	VPROF_BUDGET( "CDetailObjectSystem::CountSpritesInLeafList", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	int nCount = 0;
	int nMax = 0;
	for ( int i = 0; i < nLeafCount; ++i )
	{
		CFastDetailLeafSpriteList *pData = reinterpret_cast< CFastDetailLeafSpriteList *> (
			ClientLeafSystem()->GetSubSystemDataInLeaf( pLeafList[i], CLSUBSYSTEM_DETAILOBJECTS ) );
		if ( pData )
		{
			nCount += pData->m_nNumSprites;
			nMax = MAX( nMax, pData->m_nNumSprites );
		}
	}
	*nMaxFoundInLeaf = ( nMax + 3 ) & ~3;					// round up
	return nCount;
}


//-----------------------------------------------------------------------------
// Count the number of detail sprite quads in the leaf list
//-----------------------------------------------------------------------------
int CDetailObjectSystem::CountSpriteQuadsInLeafList( int nLeafCount, LeafIndex_t *pLeafList ) const
{
#ifdef USE_DETAIL_SHAPES
	VPROF_BUDGET( "CDetailObjectSystem::CountSpritesInLeafList", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	int nQuadCount = 0;
	int nFirstDetailObject, nDetailObjectCount;
	for ( int i = 0; i < nLeafCount; ++i )
	{
		// FIXME: This actually counts *everything* in the leaf, which is ok for now
		// given how we're using it
		ClientLeafSystem()->GetDetailObjectsInLeaf( pLeafList[i], nFirstDetailObject, nDetailObjectCount );
		for ( int j = 0; j < nDetailObjectCount; ++j )
		{
			nQuadCount += m_DetailObjects[j + nFirstDetailObject].QuadsToDraw();
		}
	}

	return nQuadCount;
#else
	return CountSpritesInLeafList( nLeafCount, pLeafList );
#endif
}


#define TREATASINT(x) ( *(  ( (int32 const *)( &(x) ) ) ) )

//-----------------------------------------------------------------------------
// Sorts sprites in back-to-front order
//-----------------------------------------------------------------------------
inline bool CDetailObjectSystem::SortLessFunc( const CDetailObjectSystem::SortInfo_t &left, const CDetailObjectSystem::SortInfo_t &right )
{
	return TREATASINT( left.m_flDistance ) > TREATASINT( right.m_flDistance );
//	return left.m_flDistance > right.m_flDistance;
}


int CDetailObjectSystem::SortSpritesBackToFront( int nLeaf, const Vector &viewOrigin, const Vector &viewForward, SortInfo_t *pSortInfo )
{
	VPROF_BUDGET( "CDetailObjectSystem::SortSpritesBackToFront", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	int nFirstDetailObject, nDetailObjectCount;
	ClientLeafSystem()->GetDetailObjectsInLeaf( nLeaf, nFirstDetailObject, nDetailObjectCount );

	float flFactor = 1.0f;
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		flFactor = 1.0 / pLocalPlayer->GetFOVDistanceAdjustFactor();
	}

	float flMaxSqDist;
	float flFadeSqDist;
	float flDetailDist = cl_detaildist.GetFloat();

	flMaxSqDist = flDetailDist * flDetailDist;
	flFadeSqDist = flDetailDist - cl_detailfade.GetFloat();
	flMaxSqDist *= flFactor;
	flFadeSqDist *= flFactor;
	if (flFadeSqDist > 0)
	{
		flFadeSqDist *= flFadeSqDist;
	}
	else 
	{
		flFadeSqDist = 0;
	}
	float flFalloffFactor = 255.0f / (flMaxSqDist - flFadeSqDist);

	Vector vecDelta;
	int nCount = 0;
	nDetailObjectCount += nFirstDetailObject;
	for ( int j = nFirstDetailObject; j < nDetailObjectCount; ++j )
	{
		CDetailModel &model = m_DetailObjects[j];

		Vector v;
		VectorSubtract( model.GetRenderOrigin(), viewOrigin, vecDelta );
		float flSqDist = vecDelta.LengthSqr();
		if ( flSqDist >= flMaxSqDist )
			continue;

		if ((flFadeSqDist > 0) && (flSqDist > flFadeSqDist))
		{
			model.SetAlpha( flFalloffFactor * ( flMaxSqDist - flSqDist ) );
		}
		else
		{
			model.SetAlpha( 255 );
		}

		if ( (model.GetType() == DETAIL_PROP_TYPE_MODEL) || (model.GetAlpha() == 0) )
			continue;

		// Perform screen alignment if necessary.
		model.ComputeAngles();
		SortInfo_t *pSortInfoCurrent = &pSortInfo[nCount];

		pSortInfoCurrent->m_nIndex = j;

		// Compute distance from the camera to each object
		pSortInfoCurrent->m_flDistance = flSqDist;
		++nCount;
	}

	if ( nCount )
	{
		VPROF( "CDetailObjectSystem::SortSpritesBackToFront -- Sort" );
		std::make_heap( pSortInfo, pSortInfo + nCount, SortLessFunc ); 
		std::sort_heap( pSortInfo, pSortInfo + nCount, SortLessFunc ); 
	}

	return nCount;
}


#define MAGIC_NUMBER (1<<23)
#ifdef VALVE_BIG_ENDIAN
#define MANTISSA_LSB_OFFSET 3
#else
#define MANTISSA_LSB_OFFSET 0
#endif
static fltx4 Four_MagicNumbers={ MAGIC_NUMBER, MAGIC_NUMBER, MAGIC_NUMBER, MAGIC_NUMBER };
static fltx4 Four_255s={ 255.0, 255.0, 255.0, 255.0 };

static ALIGN16 int32 And255Mask[4] ALIGN16_POST = {0xff,0xff,0xff,0xff};
#define PIXMASK ( * ( reinterpret_cast< fltx4 *>( &And255Mask ) ) )

int CDetailObjectSystem::BuildOutSortedSprites( CFastDetailLeafSpriteList *pData,
												Vector const &viewOrigin,
												Vector const &viewForward,
												Vector const &viewRight,
												Vector const &viewUp )
{
	// part 1 - do all vertex math, fading, etc into a buffer, using as much simd as we can
	int nSIMDSprites = pData->m_nNumSIMDSprites;
	FastSpriteX4_t const *pSprites = pData->m_pSprites;
	SortInfo_t *pOut = m_pFastSortInfo;
	FastSpriteQuadBuildoutBufferX4_t *pQuadBufferOut = m_pBuildoutBuffer;
	int curidx = 0;
	int nLastBfMask = 0;

	FourVectors vecViewPos;
	vecViewPos.DuplicateVector( viewOrigin );
	fltx4 maxsqdist = ReplicateX4( m_flCurMaxSqDist );

	fltx4 falloffFactor = ReplicateX4( 1.0/ ( m_flCurMaxSqDist - m_flCurFadeSqDist ) );
	fltx4 startFade = ReplicateX4( m_flCurFadeSqDist );

	FourVectors vecUp;
	vecUp.DuplicateVector(Vector(0,0,1) );
	FourVectors vecFwd;
	vecFwd.DuplicateVector( viewForward );

	do
	{
		// calculate alpha
		FourVectors ofs = pSprites->m_Pos;
		ofs -= vecViewPos;
		fltx4 ofsDotFwd = ofs * vecFwd;
		fltx4 distanceSquared = ofs * ofs;
		nLastBfMask = TestSignSIMD( OrSIMD( ofsDotFwd, CmpGtSIMD( distanceSquared, maxsqdist ) ) );		//  cull
		if ( nLastBfMask != 0xf )
		{
			FourVectors dx1;
			dx1.x = fnegate( ofs.y );
			dx1.y = ( ofs.x );
			dx1.z = Four_Zeros;
			dx1.VectorNormalizeFast();
				
			FourVectors vecDx = dx1;
			FourVectors vecDy = vecUp;

			FourVectors vecPos0 = pSprites->m_Pos;

			vecDx *= pSprites->m_HalfWidth;
			vecDy *= pSprites->m_Height;
			fltx4 alpha = MulSIMD( falloffFactor, SubSIMD( distanceSquared, startFade ) );
			alpha = SubSIMD( Four_Ones, MinSIMD( MaxSIMD( alpha, Four_Zeros), Four_Ones ) );

			pQuadBufferOut->m_Alpha = AddSIMD( Four_MagicNumbers, 
											   MulSIMD( Four_255s,alpha ) );

			vecPos0 += vecDx;
			pQuadBufferOut->m_Coords[0] = vecPos0;
			vecPos0 -= vecDy;
			pQuadBufferOut->m_Coords[1] = vecPos0;
			vecPos0 -= vecDx;
			vecPos0 -= vecDx;
			pQuadBufferOut->m_Coords[2] = vecPos0;
			vecPos0 += vecDy;
			pQuadBufferOut->m_Coords[3] = vecPos0;

			fltx4 fetch4 = *( ( fltx4 *) ( &pSprites->m_pSpriteDefs[0] ) );
			*( (fltx4 *) ( & ( pQuadBufferOut->m_pSpriteDefs[0] ) ) ) = fetch4;

			fetch4 = *( ( fltx4 *) ( &pSprites->m_RGBColor[0][0] ) );
			*( (fltx4 *) ( & ( pQuadBufferOut->m_RGBColor[0][0] ) ) ) = fetch4;

			//!! bug!! store distance
			// !! speed!! simd?
			pOut[0].m_nIndex = curidx;
			pOut[0].m_flDistance = SubFloat( distanceSquared, 0 );
			pOut[1].m_nIndex = curidx+1;
			pOut[1].m_flDistance = SubFloat( distanceSquared, 1 );
			pOut[2].m_nIndex = curidx+2;
			pOut[2].m_flDistance = SubFloat( distanceSquared, 2 );
			pOut[3].m_nIndex = curidx+3;
			pOut[3].m_flDistance = SubFloat( distanceSquared, 3 );
			curidx += 4;
			pOut += 4;
			pQuadBufferOut++;
		}
		pSprites++;
	} while( --nSIMDSprites );

	// adjust count for tail
	int nCount = pOut - m_pFastSortInfo;
	if ( nLastBfMask != 0xf )						// if last not skipped
		nCount -= ( 0 - pData->m_nNumSprites ) & 3;

	// part 2 - sort
	if ( nCount )
	{
		VPROF( "CDetailObjectSystem::SortSpritesBackToFront -- Sort" );
		std::make_heap( m_pFastSortInfo, m_pFastSortInfo + nCount, SortLessFunc ); 
		std::sort_heap( m_pFastSortInfo, m_pFastSortInfo + nCount, SortLessFunc ); 
	}
	return nCount;
}


void CDetailObjectSystem::RenderFastSprites( const Vector &viewOrigin, const Vector &viewForward, const Vector &viewRight, const Vector &viewUp, int nLeafCount, LeafIndex_t const * pLeafList )
{
	// Here, we must draw all detail objects back-to-front
	// FIXME: Cache off a sorted list so we don't have to re-sort every frame

	// Count the total # of detail quads we possibly could render
	int nMaxInLeaf;

	int nQuadCount = CountFastSpritesInLeafList( nLeafCount, pLeafList, &nMaxInLeaf );
	if ( nQuadCount == 0 )
		return;
	if  ( r_DrawDetailProps.GetInt() == 0 )
		return;


	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	IMaterial *pMaterial = m_DetailSpriteMaterial;
	if ( ShouldDrawInWireFrameMode() || r_DrawDetailProps.GetInt() == 2 )
	{
		pMaterial = m_DetailWireframeMaterial;
	}

	CMeshBuilder meshBuilder;
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );

	int nMaxVerts, nMaxIndices;
	pRenderContext->GetMaxToRender( pMesh, false, &nMaxVerts, &nMaxIndices );
	int nMaxQuadsToDraw = nMaxIndices / 6;
	if ( nMaxQuadsToDraw > nMaxVerts / 4 ) 
	{
		nMaxQuadsToDraw = nMaxVerts / 4;
	}

	if ( nMaxQuadsToDraw == 0 )
		return;

	int nQuadsToDraw = MIN( nQuadCount, nMaxQuadsToDraw );
	int nQuadsRemaining = nQuadsToDraw;

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, nQuadsToDraw );



	// Sort detail sprites in each leaf independently; then render them
	for ( int i = 0; i < nLeafCount; ++i )
	{
		int nLeaf = pLeafList[i];

		CFastDetailLeafSpriteList *pData = reinterpret_cast<CFastDetailLeafSpriteList *> (
			ClientLeafSystem()->GetSubSystemDataInLeaf( nLeaf, CLSUBSYSTEM_DETAILOBJECTS ) );

		if ( pData )
		{
			Assert( pData->m_nNumSprites );					// ptr with no sprites?

			int nCount = BuildOutSortedSprites( pData, viewOrigin, viewForward, viewRight, viewUp );

			// part 3 - stuff the sorted sprites into the vb
			SortInfo_t const *pDraw = m_pFastSortInfo;
			FastSpriteQuadBuildoutBufferNonSIMDView_t const *pQuadBuffer =
				( FastSpriteQuadBuildoutBufferNonSIMDView_t const *) m_pBuildoutBuffer;

			COMPILE_TIME_ASSERT( sizeof( FastSpriteQuadBuildoutBufferNonSIMDView_t ) ==
								 sizeof( FastSpriteQuadBuildoutBufferX4_t ) );

			while( nCount )
			{
				if ( ! nQuadsRemaining )					// no room left?
				{
					meshBuilder.End();
					pMesh->Draw();
					nQuadsRemaining = nQuadsToDraw;
					meshBuilder.Begin( pMesh, MATERIAL_QUADS, nQuadsToDraw );
				}
				int nToDraw = MIN( nCount, nQuadsRemaining );
				nCount -= nToDraw;
				nQuadsRemaining -= nToDraw;
				while( nToDraw-- )
				{
					// draw the sucker
					int nSIMDIdx = pDraw->m_nIndex >> 2;
					int nSubIdx = pDraw->m_nIndex & 3;

					FastSpriteQuadBuildoutBufferNonSIMDView_t const *pquad = pQuadBuffer+nSIMDIdx;

					// voodoo - since everything is in 4s, offset structure pointer by a couple of floats to handle sub-index
					pquad = (FastSpriteQuadBuildoutBufferNonSIMDView_t const *) ( ( (int) ( pquad ) )+ ( nSubIdx << 2 ) );
					uint8 const *pColorsCasted = reinterpret_cast<uint8 const *> ( pquad->m_Alpha );

					uint8 color[4];
					color[0] = pquad->m_RGBColor[0][0];
					color[1] = pquad->m_RGBColor[0][1];
					color[2] = pquad->m_RGBColor[0][2];
					color[3] = pColorsCasted[MANTISSA_LSB_OFFSET];

					DetailPropSpriteDict_t *pDict = pquad->m_pSpriteDefs[0];

					meshBuilder.Position3f( pquad->m_flX0[0], pquad->m_flY0[0], pquad->m_flZ0[0] );
					meshBuilder.Color4ubv( color );
					meshBuilder.TexCoord2f( 0, pDict->m_TexLR.x, pDict->m_TexLR.y );
					meshBuilder.AdvanceVertex();

					meshBuilder.Position3f( pquad->m_flX1[0], pquad->m_flY1[0], pquad->m_flZ1[0] );
					meshBuilder.Color4ubv( color );
					meshBuilder.TexCoord2f( 0, pDict->m_TexLR.x, pDict->m_TexUL.y );
					meshBuilder.AdvanceVertex();

					meshBuilder.Position3f( pquad->m_flX2[0], pquad->m_flY2[0], pquad->m_flZ2[0] );
					meshBuilder.Color4ubv( color );
					meshBuilder.TexCoord2f( 0, pDict->m_TexUL.x, pDict->m_TexUL.y );
					meshBuilder.AdvanceVertex();

					meshBuilder.Position3f( pquad->m_flX3[0], pquad->m_flY3[0], pquad->m_flZ3[0] );
					meshBuilder.Color4ubv( color );
					meshBuilder.TexCoord2f( 0, pDict->m_TexUL.x, pDict->m_TexLR.y );
					meshBuilder.AdvanceVertex();
					pDraw++;
				}
			}
		}
	}
	meshBuilder.End();
	pMesh->Draw();
	pRenderContext->PopMatrix();
}


//-----------------------------------------------------------------------------
// Renders all translucent detail objects in a particular set of leaves
//-----------------------------------------------------------------------------
void CDetailObjectSystem::RenderTranslucentDetailObjects( const Vector &viewOrigin, const Vector &viewForward, const Vector &viewRight, const Vector &viewUp, int nLeafCount, LeafIndex_t *pLeafList )
{
	VPROF_BUDGET( "CDetailObjectSystem::RenderTranslucentDetailObjects", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	if (nLeafCount == 0)
		return;

	// We better not have any partially drawn leaf of detail sprites!
	Assert( m_nSpriteCount == m_nFirstSprite );

	// Here, we must draw all detail objects back-to-front
	RenderFastSprites( viewOrigin, viewForward, viewRight, viewUp, nLeafCount, pLeafList );

	// FIXME: Cache off a sorted list so we don't have to re-sort every frame

	// Count the total # of detail quads we possibly could render
	int nQuadCount = CountSpriteQuadsInLeafList( nLeafCount, pLeafList );
	if ( nQuadCount == 0 )
		return;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	IMaterial *pMaterial = m_DetailSpriteMaterial;
	if ( ShouldDrawInWireFrameMode() || r_DrawDetailProps.GetInt() == 2 )
	{
		pMaterial = m_DetailWireframeMaterial;
	}

	CMeshBuilder meshBuilder;
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );

	int nMaxVerts, nMaxIndices;
	pRenderContext->GetMaxToRender( pMesh, false, &nMaxVerts, &nMaxIndices );
	int nMaxQuadsToDraw = nMaxIndices / 6;
	if ( nMaxQuadsToDraw > nMaxVerts / 4 ) 
	{
		nMaxQuadsToDraw = nMaxVerts / 4;
	}

	if ( nMaxQuadsToDraw == 0 )
		return;

	int nQuadsToDraw = nQuadCount;
	if ( nQuadsToDraw > nMaxQuadsToDraw )
	{
		nQuadsToDraw = nMaxQuadsToDraw;
	}

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, nQuadsToDraw );

	int nQuadsDrawn = 0;
	for ( int i = 0; i < nLeafCount; ++i )
	{
		int nLeaf = pLeafList[i];

		int nFirstDetailObject, nDetailObjectCount;
		ClientLeafSystem()->GetDetailObjectsInLeaf( nLeaf, nFirstDetailObject, nDetailObjectCount );

		// Sort detail sprites in each leaf independently; then render them
		SortInfo_t *pSortInfo = m_pSortInfo;
		int nCount = SortSpritesBackToFront( nLeaf, viewOrigin, viewForward, pSortInfo );

		for ( int j = 0; j < nCount; ++j )
		{
			CDetailModel &model = m_DetailObjects[ pSortInfo[j].m_nIndex ];
			int nQuadsInModel = model.QuadsToDraw();

			// Prevent the batches from getting too large
			if ( nQuadsDrawn + nQuadsInModel > nQuadsToDraw )
			{
				meshBuilder.End();
				pMesh->Draw();

				nQuadCount -= nQuadsDrawn;
				nQuadsToDraw = nQuadCount;
				if (nQuadsToDraw > nMaxQuadsToDraw)
				{
					nQuadsToDraw = nMaxQuadsToDraw;
				}

				meshBuilder.Begin( pMesh, MATERIAL_QUADS, nQuadsToDraw );
				nQuadsDrawn = 0;
			}

			model.DrawSprite( meshBuilder );

			nQuadsDrawn += nQuadsInModel;
		}
	}

	meshBuilder.End();
	pMesh->Draw();

	pRenderContext->PopMatrix();
}


void CDetailObjectSystem::RenderFastTranslucentDetailObjectsInLeaf( const Vector &viewOrigin, const Vector &viewForward, const Vector &viewRight, const Vector &viewUp, int nLeaf, const Vector *pVecClosestPoint )
{
	CFastDetailLeafSpriteList *pData = reinterpret_cast< CFastDetailLeafSpriteList *> (
		ClientLeafSystem()->GetSubSystemDataInLeaf( nLeaf, CLSUBSYSTEM_DETAILOBJECTS ) );
	if ( ! pData )
		return;

	if ( m_nSortedFastLeaf != nLeaf )
	{
		m_nSortedFastLeaf = nLeaf;
		pData->m_nNumPendingSprites = BuildOutSortedSprites( pData, viewOrigin, viewForward, viewRight, viewUp );
		pData->m_nStartSpriteIndex = 0;
	}
	if ( pData->m_nNumPendingSprites == 0 )
		return;

	float flMinDistance = 0.0f;
	if ( pVecClosestPoint )
	{
		Vector vecDelta;
		VectorSubtract( *pVecClosestPoint, viewOrigin, vecDelta );
		flMinDistance = vecDelta.LengthSqr();
	}
		
	if ( m_pFastSortInfo[pData->m_nStartSpriteIndex].m_flDistance < flMinDistance )
		return;

	int nCount = pData->m_nNumPendingSprites;

	if  ( r_DrawDetailProps.GetInt() == 0 )
		return;


	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();
		
	IMaterial *pMaterial = m_DetailSpriteMaterial;
	if ( ShouldDrawInWireFrameMode() || r_DrawDetailProps.GetInt() == 2 )
	{
		pMaterial = m_DetailWireframeMaterial;
	}
		
	CMeshBuilder meshBuilder;
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );

	int nMaxVerts, nMaxIndices;
	pRenderContext->GetMaxToRender( pMesh, false, &nMaxVerts, &nMaxIndices );
	int nMaxQuadsToDraw = nMaxIndices / 6;
	if ( nMaxQuadsToDraw > nMaxVerts / 4 ) 
	{
		nMaxQuadsToDraw = nMaxVerts / 4;
	}
	
	if ( nMaxQuadsToDraw == 0 )
		return;
		
	int nQuadsToDraw = MIN( nCount, nMaxQuadsToDraw );
	int nQuadsRemaining = nQuadsToDraw;
		
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, nQuadsToDraw );

	SortInfo_t const *pDraw = m_pFastSortInfo + pData->m_nStartSpriteIndex;

	FastSpriteQuadBuildoutBufferNonSIMDView_t const *pQuadBuffer =
		( FastSpriteQuadBuildoutBufferNonSIMDView_t const *) m_pBuildoutBuffer;
	
	while( nCount && ( pDraw->m_flDistance >= flMinDistance ) )
	{
		if ( ! nQuadsRemaining )					// no room left?
		{
			meshBuilder.End();
			pMesh->Draw();
			nQuadsRemaining = nQuadsToDraw;
			meshBuilder.Begin( pMesh, MATERIAL_QUADS, nQuadsToDraw );
		}
		int nToDraw = MIN( nCount, nQuadsRemaining );
		nCount -= nToDraw;
		nQuadsRemaining -= nToDraw;
		while( nToDraw-- )
		{
			// draw the sucker
			int nSIMDIdx = pDraw->m_nIndex >> 2;
			int nSubIdx = pDraw->m_nIndex & 3;

			FastSpriteQuadBuildoutBufferNonSIMDView_t const *pquad = pQuadBuffer+nSIMDIdx;

			// voodoo - since everything is in 4s, offset structure pointer by a couple of floats to handle sub-index
			pquad = (FastSpriteQuadBuildoutBufferNonSIMDView_t const *) ( ( (int) ( pquad ) )+ ( nSubIdx << 2 ) );
			uint8 const *pColorsCasted = reinterpret_cast<uint8 const *> ( pquad->m_Alpha );

			uint8 color[4];
			color[0] = pquad->m_RGBColor[0][0];
			color[1] = pquad->m_RGBColor[0][1];
			color[2] = pquad->m_RGBColor[0][2];
			color[3] = pColorsCasted[MANTISSA_LSB_OFFSET];

			DetailPropSpriteDict_t *pDict = pquad->m_pSpriteDefs[0];

			meshBuilder.Position3f( pquad->m_flX0[0], pquad->m_flY0[0], pquad->m_flZ0[0] );
			meshBuilder.Color4ubv( color );
			meshBuilder.TexCoord2f( 0, pDict->m_TexLR.x, pDict->m_TexLR.y );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f( pquad->m_flX1[0], pquad->m_flY1[0], pquad->m_flZ1[0] );
			meshBuilder.Color4ubv( color );
			meshBuilder.TexCoord2f( 0, pDict->m_TexLR.x, pDict->m_TexUL.y );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f( pquad->m_flX2[0], pquad->m_flY2[0], pquad->m_flZ2[0] );
			meshBuilder.Color4ubv( color );
			meshBuilder.TexCoord2f( 0, pDict->m_TexUL.x, pDict->m_TexUL.y );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f( pquad->m_flX3[0], pquad->m_flY3[0], pquad->m_flZ3[0] );
			meshBuilder.Color4ubv( color );
			meshBuilder.TexCoord2f( 0, pDict->m_TexUL.x, pDict->m_TexLR.y );
			meshBuilder.AdvanceVertex();
			pDraw++;
		}
	}
	pData->m_nNumPendingSprites = nCount;
	pData->m_nStartSpriteIndex = pDraw - m_pFastSortInfo;

	meshBuilder.End();
	pMesh->Draw();
	pRenderContext->PopMatrix();
}

//-----------------------------------------------------------------------------
// Renders a subset of the detail objects in a particular leaf (for interleaving with other translucent entities)
//-----------------------------------------------------------------------------
void CDetailObjectSystem::RenderTranslucentDetailObjectsInLeaf( const Vector &viewOrigin, const Vector &viewForward, const Vector &viewRight, const Vector &viewUp, int nLeaf, const Vector *pVecClosestPoint )
{
	VPROF_BUDGET( "CDetailObjectSystem::RenderTranslucentDetailObjectsInLeaf", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );

	RenderFastTranslucentDetailObjectsInLeaf( viewOrigin, viewForward, viewRight, viewUp, nLeaf, pVecClosestPoint );
	// We may have already sorted this leaf. If not, sort the leaf.
	if ( m_nSortedLeaf != nLeaf )
	{
		m_nSortedLeaf = nLeaf;
		m_nSpriteCount = 0;
		m_nFirstSprite = 0;

		// Count the total # of detail sprites we possibly could render
		LeafIndex_t nLeafIndex = nLeaf;
		int nSpriteCount = CountSpritesInLeafList( 1, &nLeafIndex );
		if (nSpriteCount == 0)
			return;

		// Sort detail sprites in each leaf independently; then render them
		m_nSpriteCount = SortSpritesBackToFront( nLeaf, viewOrigin, viewForward, m_pSortInfo );
		Assert( m_nSpriteCount <= nSpriteCount );
	}

	// No more to draw? Bye!
	if ( m_nSpriteCount == m_nFirstSprite )
		return;

	float flMinDistance = 0.0f;
	if ( pVecClosestPoint )
	{
		Vector vecDelta;
		VectorSubtract( *pVecClosestPoint, viewOrigin, vecDelta );
		flMinDistance = vecDelta.LengthSqr();
	}

	if ( m_pSortInfo[m_nFirstSprite].m_flDistance < flMinDistance )
		return;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	IMaterial *pMaterial = m_DetailSpriteMaterial;
	if ( ShouldDrawInWireFrameMode() || r_DrawDetailProps.GetInt() == 2 )
	{
		pMaterial = m_DetailWireframeMaterial;
	}

	CMeshBuilder meshBuilder;
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );

	int nMaxVerts, nMaxIndices;
	pRenderContext->GetMaxToRender( pMesh, false, &nMaxVerts, &nMaxIndices );

	// needs to be * 4 since there are a max of 4 quads per detail object
	int nQuadCount = ( m_nSpriteCount - m_nFirstSprite ) * 4;
	int nMaxQuadsToDraw = nMaxIndices/6;
	if ( nMaxQuadsToDraw > nMaxVerts / 4 )
	{
		nMaxQuadsToDraw = nMaxVerts / 4;
	}

	if ( nMaxQuadsToDraw == 0 )
		return;

	int nQuadsToDraw = nQuadCount;
	if ( nQuadsToDraw > nMaxQuadsToDraw )
	{
		nQuadsToDraw = nMaxQuadsToDraw;
	}

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, nQuadsToDraw );

	int nQuadsDrawn = 0;
	while ( m_nFirstSprite < m_nSpriteCount && m_pSortInfo[m_nFirstSprite].m_flDistance >= flMinDistance )
	{
		CDetailModel &model = m_DetailObjects[m_pSortInfo[m_nFirstSprite].m_nIndex];
		int nQuadsInModel = model.QuadsToDraw();
		if ( nQuadsDrawn + nQuadsInModel > nQuadsToDraw )
		{
			meshBuilder.End();
			pMesh->Draw();

			nQuadCount = ( m_nSpriteCount - m_nFirstSprite ) * 4;
			nQuadsToDraw = nQuadCount;
			if (nQuadsToDraw > nMaxQuadsToDraw)
			{
				nQuadsToDraw = nMaxQuadsToDraw;
			}

			meshBuilder.Begin( pMesh, MATERIAL_QUADS, nQuadsToDraw );
			nQuadsDrawn = 0;
		}

		model.DrawSprite( meshBuilder );
		++m_nFirstSprite;
		nQuadsDrawn += nQuadsInModel;
	}
	meshBuilder.End();
	pMesh->Draw();

 	pRenderContext->PopMatrix();
}


//-----------------------------------------------------------------------------
// Gets called each view
//-----------------------------------------------------------------------------
bool CDetailObjectSystem::EnumerateLeaf( int leaf, int context )
{
	VPROF_BUDGET( "CDetailObjectSystem::EnumerateLeaf", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	Vector v;
	int firstDetailObject, detailObjectCount;

	EnumContext_t* pCtx = (EnumContext_t*)context;
	ClientLeafSystem()->DrawDetailObjectsInLeaf( leaf, pCtx->m_BuildWorldListNumber, 
		firstDetailObject, detailObjectCount );

	// Compute the translucency. Need to do it now cause we need to
	// know that when we're rendering (opaque stuff is rendered first)
	for ( int i = 0; i < detailObjectCount; ++i)
	{
		// Calculate distance (badly)
		CDetailModel& model = m_DetailObjects[firstDetailObject+i];
		VectorSubtract( model.GetRenderOrigin(), pCtx->m_vViewOrigin, v );

		float sqDist = v.LengthSqr();

		model.SetAlpha( 255 );
		if ( sqDist < m_flCurMaxSqDist )
		{
			if ( sqDist > m_flCurFadeSqDist ) 
			{
				model.SetAlpha( m_flCurFalloffFactor * ( m_flCurMaxSqDist - sqDist ) );
			}
			else
			{
				model.SetAlpha( 255 );
			}
			
			// Perform screen alignment if necessary.
			model.ComputeAngles();
		}
		else
		{
			model.SetAlpha( 0 );
		}
	}
	return true;
}


//-----------------------------------------------------------------------------
// Gets called each view
//-----------------------------------------------------------------------------
void CDetailObjectSystem::BuildDetailObjectRenderLists( const Vector &vViewOrigin )
{
	VPROF_BUDGET( "CDetailObjectSystem::BuildDetailObjectRenderLists", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	
	if (!g_pClientMode->ShouldDrawDetailObjects() || (r_DrawDetailProps.GetInt() == 0))
		return;

	// Don't bother doing any of this if the level doesn't have detail props.
	if ( ( ! m_pFastSpriteData ) && ( m_DetailObjects.Count() == 0 ) )
		return;

	EnumContext_t ctx;
	ctx.m_vViewOrigin = vViewOrigin;
 	ctx.m_BuildWorldListNumber = view->BuildWorldListsNumber();

	// We need to recompute translucency information for all detail props
	for (int i = m_DetailObjectDict.Size(); --i >= 0; )
	{
		if (modelinfo->ModelHasMaterialProxy( m_DetailObjectDict[i].m_pModel ))
		{
			modelinfo->RecomputeTranslucency( m_DetailObjectDict[i].m_pModel, 0, 0, NULL );
		}
	}

	float factor = 1.0f;
	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	if ( local )
	{
		factor = local->GetFOVDistanceAdjustFactor();
	}

	// Compute factors to optimize rendering of the detail models
	m_flCurMaxSqDist = cl_detaildist.GetFloat() * cl_detaildist.GetFloat();
	m_flCurFadeSqDist = cl_detaildist.GetFloat() - cl_detailfade.GetFloat();

	m_flCurMaxSqDist /= factor;
	m_flCurFadeSqDist /= factor;

	if ( m_flCurFadeSqDist > 0)
	{
		m_flCurFadeSqDist *= m_flCurFadeSqDist;
	}
	else 
	{
		m_flCurFadeSqDist = 0;
	}
	m_flCurFadeSqDist = MIN( m_flCurFadeSqDist, m_flCurMaxSqDist -1  );
	m_flCurFalloffFactor = 255.0f / ( m_flCurMaxSqDist - m_flCurFadeSqDist );


	ISpatialQuery* pQuery = engine->GetBSPTreeQuery();
	pQuery->EnumerateLeavesInSphere( CurrentViewOrigin(), 
									 cl_detaildist.GetFloat(), this, (int)&ctx );
}

