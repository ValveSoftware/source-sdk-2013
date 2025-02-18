//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "particles_simple.h"
#include "iviewrender.h"
#include "proxyentity.h"
#include "materialsystem/imaterialvar.h"
#include "model_types.h"
#include "engine/ivmodelinfo.h"
#include "clienteffectprecachesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_NUM_PANELS 16

extern IVDebugOverlay *debugoverlay;


enum WinSide_t 
{
	WIN_SIDE_BOTTOM,
	WIN_SIDE_RIGHT,
	WIN_SIDE_TOP,
	WIN_SIDE_LEFT,
};

enum WinEdge_t 
{
	EDGE_NOT	= -1,		// No edge
	EDGE_NONE,				/* No edge on both sides	/##\  */
	EDGE_FULL,				// Edge on both sides		|##| 
	EDGE_LEFT,				/* Edge is on left only		|##\  */
	EDGE_RIGHT,				// Edge is on right only	/##|
};

#define STYLE_HIGHLIGHT = -1;

#define NUM_EDGE_TYPES		4
#define NUM_EDGE_STYLES		3


//==================================================
// C_BreakableSurface
//==================================================

//-----------------------------------------------------------------------------
// All the information associated with a particular handle
//-----------------------------------------------------------------------------
struct Panel_t
{
	char			m_nWidth;
	char			m_nHeight;
	char			m_nSide;
	char			m_nEdgeType;
	char			m_nStyle;
};

struct EdgeTexture_t
{
	int					m_nRenderIndex;
	int					m_nStyle;
	CMaterialReference	m_pMaterialEdge;
	CTextureReference	m_pMaterialEdgeTexture;
};

// Bits for m_nPanelBits
#define BITS_PANEL_IS_SOLID		(1<<0)
#define BITS_PANEL_IS_STALE		(1<<1)


class C_BreakableSurface : public C_BaseEntity, public IBrushRenderer
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_BreakableSurface, C_BaseEntity );
	DECLARE_DATADESC();

	int				m_nNumWide;
	int				m_nNumHigh;
	float			m_flPanelWidth;
	float			m_flPanelHeight;
	Vector			m_vNormal;
	Vector			m_vCorner;
	bool			m_bIsBroken;
	int				m_nSurfaceType;

						
	// This is the texture we're going to use to multiply by the cracked base texture
	ITexture*	m_pCurrentDetailTexture;

	// Stores linked list of edges to render
	CUtlLinkedList< Panel_t, unsigned short >		m_RenderList;


	C_BreakableSurface();
	~C_BreakableSurface();

public:
	void		InitMaterial(WinEdge_t nEdgeType, int nEdgeStyle, char const* pMaterialName);
	virtual void		OnDataChanged( DataUpdateType_t updateType );
	virtual void		OnPreDataChanged( DataUpdateType_t updateType );

	bool		IsTransparent( void );
	bool		HavePanel(int nWidth, int nHeight);
	bool		RenderBrushModelSurface( IClientEntity* pBaseEntity, IBrushSurface* pBrushSurface ); 
	int			DrawModel( int flags );
	void		DrawSolidBlocks( IBrushSurface* pBrushSurface );

	virtual void	OnRestore();

	virtual bool	ShouldReceiveProjectedTextures( int flags );

private:
	// One bit per pane
	CNetworkArray( bool, m_RawPanelBitVec, MAX_NUM_PANELS * MAX_NUM_PANELS );
	bool m_PrevRawPanelBitVec[ MAX_NUM_PANELS * MAX_NUM_PANELS ];
	
	// 2 bits of flags and 2 bits of edge type
	byte		m_nPanelBits[MAX_NUM_PANELS][MAX_NUM_PANELS];	//UNDONE: allocate this dynamically?
	CMaterialReference	m_pMaterialBox;	
	EdgeTexture_t	m_pSolid;
	EdgeTexture_t	m_pEdge[NUM_EDGE_TYPES][NUM_EDGE_STYLES];

	inline bool InLegalRange(int nWidth, int nHeight);
	inline bool	IsPanelSolid(int nWidth, int nHeight);
	inline bool	IsPanelStale(int nWidth, int nHeight);
	inline void	SetPanelSolid(int nWidth, int nHeight, bool value);
	inline void	SetPanelStale(int nWidth, int nHeight, bool value);

	void		DrawOneEdge( IBrushSurface* pBrushSurface, IMesh* pMesh, 
					CMeshBuilder *pMeshBuilder, const Vector &vStartPos,  
					const Vector &vWStep, const Vector &vHstep, WinSide_t nEdge);
	void		DrawOneHighlight( IBrushSurface* pBrushSurface, IMesh* pMesh, 
					CMeshBuilder *pMeshBuilder, const Vector &vStartPos,  
					const Vector &vWStep, const Vector &vHstep, WinSide_t nEdge);
	void		DrawOneBlock(IBrushSurface* pBrushSurface, IMesh* pMesh, 
					CMeshBuilder *pMeshBuilder, const Vector &vPosition, 
					const Vector &vWidth, const Vector &vHeight);

	void		DrawRenderList( IBrushSurface* pBrushSurface);
	void		DrawRenderListHighlights( IBrushSurface* pBrushSurface );
	int			FindRenderPanel(int nWidth, int nHeight, WinSide_t nSide);
	void		AddToRenderList(int nWidth, int nHeight, WinSide_t nSide, WinEdge_t nEdgeType, int forceStyle);
	int			FindFirstRenderTexture(WinEdge_t nEdgeType, int nStyle);

	inline void SetStyleType( int w, int h, int type )
	{
		Assert( type < NUM_EDGE_STYLES );
		Assert( type >= 0 );
		// Clear old value
		m_nPanelBits[ w ][ h ] &= 0xF0; // ( ~0x03 << 2 ); Left shifting a negative value has undefined behavior. Use the constant 0xF0 instead.
		// Insert new value
		m_nPanelBits[ w ][ h ] |= ( type << 2 );
	}

	inline int GetStyleType( int w, int h )
	{
		int value = m_nPanelBits[ w ][ h ];
		value = ( value >> 2 ) & 0x03;
		Assert( value < NUM_EDGE_STYLES );
		return value;
	}

	// Gets at the cracked version of the material
	void FindCrackedMaterial();

	CMaterialReference	m_pCrackedMaterial;
	CTextureReference	m_pMaterialBoxTexture;


	void		UpdateEdgeType(int nWidth, int nHeight, int forceStyle = -1 );
};

BEGIN_DATADESC( C_BreakableSurface )

	DEFINE_ARRAY( m_nPanelBits, FIELD_CHARACTER, MAX_NUM_PANELS * MAX_NUM_PANELS ),

//	DEFINE_FIELD( m_nNumWide, FIELD_INTEGER ),
//	DEFINE_FIELD( m_nNumHigh, FIELD_INTEGER ),
//	DEFINE_FIELD( m_flPanelWidth, FIELD_FLOAT ),
//	DEFINE_FIELD( m_flPanelHeight, FIELD_FLOAT ),
//	DEFINE_FIELD( m_vNormal, FIELD_VECTOR ),
//	DEFINE_FIELD( m_vCorner, FIELD_VECTOR ),
//	DEFINE_FIELD( m_bIsBroken, FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_nSurfaceType, FIELD_INTEGER ),
	// DEFINE_FIELD( m_pCurrentDetailTexture, ITexture* ),
	// DEFINE_FIELD( m_RenderList, CUtlLinkedList < Panel_t , unsigned short > ),
	// DEFINE_FIELD( m_pMaterialBox, CMaterialReference ),
	// DEFINE_FIELD( m_pSolid, EdgeTexture_t ),
	// DEFINE_ARRAY( m_pEdge, EdgeTexture_t, NUM_EDGE_TYPES][NUM_EDGE_STYLES ),
	// DEFINE_FIELD( m_pCrackedMaterial, CMaterialReference ),
	// DEFINE_FIELD( m_pMaterialBoxTexture, CTextureReference ),

END_DATADESC()

bool C_BreakableSurface::InLegalRange(int nWidth, int nHeight)
{ 
	return (nWidth < m_nNumWide && nHeight < m_nNumHigh && 
		nWidth >=0 && nHeight >= 0 );
}

bool C_BreakableSurface::IsPanelSolid(int nWidth, int nHeight)
{ 
	return ( BITS_PANEL_IS_SOLID & m_nPanelBits[nWidth][nHeight] )!=0 ;
}

bool C_BreakableSurface::IsPanelStale(int nWidth, int nHeight)
{ 
	return ( BITS_PANEL_IS_STALE & m_nPanelBits[nWidth][nHeight] )!=0 ; 
}

void C_BreakableSurface::SetPanelSolid(int nWidth, int nHeight, bool value)
{ 
	if ( !InLegalRange( nWidth, nHeight ) )
		return;

	if ( value )
	{
		m_nPanelBits[nWidth][nHeight] |= BITS_PANEL_IS_SOLID;
	}
	else
	{
		m_nPanelBits[nWidth][nHeight] &= ~BITS_PANEL_IS_SOLID;
	}
}

void C_BreakableSurface::SetPanelStale(int nWidth, int nHeight, bool value)
{ 
	if ( !InLegalRange( nWidth, nHeight) )
		return;

	if ( value )
	{
		m_nPanelBits[nWidth][nHeight] |= BITS_PANEL_IS_STALE;
	}
	else
	{
		m_nPanelBits[nWidth][nHeight] &= ~BITS_PANEL_IS_STALE;
	}
}

void C_BreakableSurface::OnRestore()
{
	BaseClass::OnRestore();

	// FIXME:  This restores the general state, but not the random edge bits
	//  those would need to be serialized separately...
	// traverse everthing and restore bits
	// Initialize panels
	for (int w=0;w<m_nNumWide;w++)
	{ 
		for (int h=0;h<m_nNumHigh;h++)
		{
			// Force recomputation
			SetPanelSolid(w,h,IsPanelSolid(w,h));
			SetPanelStale(w,h,true);
			UpdateEdgeType( w, h, GetStyleType(w,h ) );
		}
	}
}


//Receive datatable
IMPLEMENT_CLIENTCLASS_DT( C_BreakableSurface, DT_BreakableSurface, CBreakableSurface )
	RecvPropInt( RECVINFO( m_nNumWide ) ),
	RecvPropInt( RECVINFO( m_nNumHigh ) ),
	RecvPropFloat( RECVINFO( m_flPanelWidth) ),
	RecvPropFloat( RECVINFO( m_flPanelHeight) ),
	RecvPropVector( RECVINFO( m_vNormal ) ),
	RecvPropVector( RECVINFO( m_vCorner ) ),
	RecvPropInt( RECVINFO( m_bIsBroken )),
	RecvPropInt( RECVINFO( m_nSurfaceType )),
	RecvPropArray3( RECVINFO_ARRAY(m_RawPanelBitVec), RecvPropInt( RECVINFO( m_RawPanelBitVec[ 0 ] ))),

END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Gets at the cracked version of the material
//-----------------------------------------------------------------------------
void C_BreakableSurface::FindCrackedMaterial()
{
	m_pCrackedMaterial = 0;

	// First time we've seen it, get the material on the brush model
	int materialCount = modelinfo->GetModelMaterialCount( const_cast<model_t*>(GetModel()) );
	if( materialCount != 1 )
	{
		Warning( "Encountered func_breakablesurf that has a material applied to more than one surface!\n" );
		m_pCrackedMaterial.Init( "debug/debugempty", TEXTURE_GROUP_OTHER );
		return;
	}

	// Get at the first material; even if there are more than one.
	IMaterial* pMaterial;
	modelinfo->GetModelMaterials( const_cast<model_t*>(GetModel()), 1, &pMaterial );

	// The material should point to a cracked version of itself
	bool foundVar;
	IMaterialVar* pCrackName = pMaterial->FindVar( "$crackmaterial", &foundVar, false );
	if (foundVar)
	{
		m_pCrackedMaterial.Init( pCrackName->GetStringValue(), TEXTURE_GROUP_CLIENT_EFFECTS );
	}
	else
	{
		m_pCrackedMaterial.Init( pMaterial );
	}
}


//-----------------------------------------------------------------------------
// Gets at the base texture 
//-----------------------------------------------------------------------------

static ITexture* GetBaseTexture( IMaterial* pMaterial )
{
	bool foundVar;
	IMaterialVar* pTextureVar = pMaterial->FindVar( "$basetexture", &foundVar, false );
	if (!foundVar)
		return 0;

	return pTextureVar->GetTextureValue();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_BreakableSurface::InitMaterial(WinEdge_t nEdgeType, int nEdgeStyle, char const* pMaterialName)
{
	m_pEdge[nEdgeType][nEdgeStyle].m_nRenderIndex  = m_RenderList.InvalidIndex();
	m_pEdge[nEdgeType][nEdgeStyle].m_nStyle		   = nEdgeStyle;
	m_pEdge[nEdgeType][nEdgeStyle].m_pMaterialEdge.Init(pMaterialName, TEXTURE_GROUP_CLIENT_EFFECTS);
	m_pEdge[nEdgeType][nEdgeStyle].m_pMaterialEdgeTexture.Init( GetBaseTexture( m_pEdge[nEdgeType][nEdgeStyle].m_pMaterialEdge ) );
}

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

C_BreakableSurface::C_BreakableSurface()
{
	m_vNormal.Init();
	m_vCorner.Init();
	m_bIsBroken = false;

	m_pCurrentDetailTexture = NULL;

	Q_memset( m_PrevRawPanelBitVec, 0xff, sizeof( m_PrevRawPanelBitVec ) );
}

C_BreakableSurface::~C_BreakableSurface()
{
}

void C_BreakableSurface::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Initialize panels
		m_nNumWide = MAX_NUM_PANELS;
		m_nNumHigh = MAX_NUM_PANELS;
		for (int w=0;w<MAX_NUM_PANELS;w++)
		{ 
			for (int h=0;h<MAX_NUM_PANELS;h++)
			{
				SetPanelSolid(w,h,true);
				SetPanelStale(w,h,false);

				m_RawPanelBitVec.Set( w + h * MAX_NUM_PANELS, true );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_BreakableSurface::OnDataChanged( DataUpdateType_t updateType )
{
	C_BaseEntity::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Get at the cracked material
		FindCrackedMaterial();

		// Use same solid box for all breakable surfaces
		m_pMaterialBox.Init( "models/brokenglass/glassbroken_solid", TEXTURE_GROUP_MODEL );
		m_pMaterialBoxTexture.Init( GetBaseTexture( m_pMaterialBox ) );

		// NOTE: If you add or change this list of materials, change
		// the precache list on func_breakablesurf.cpp on the server.

		// Load the edge types and styles for the specific surface type
		if (m_nSurfaceType == SHATTERSURFACE_TILE)
		{
			InitMaterial(EDGE_NONE,	0,"models/brokentile/tilebroken_03a");
			InitMaterial(EDGE_FULL,	0,"models/brokentile/tilebroken_03b");
			InitMaterial(EDGE_LEFT,	0,"models/brokentile/tilebroken_03c");
			InitMaterial(EDGE_RIGHT,0,"models/brokentile/tilebroken_03d");

			InitMaterial(EDGE_NONE,	1,"models/brokentile/tilebroken_02a");
			InitMaterial(EDGE_FULL,	1,"models/brokentile/tilebroken_02b");
			InitMaterial(EDGE_LEFT,	1,"models/brokentile/tilebroken_02c");
			InitMaterial(EDGE_RIGHT,1,"models/brokentile/tilebroken_02d");

			InitMaterial(EDGE_NONE,	2,"models/brokentile/tilebroken_01a");
			InitMaterial(EDGE_FULL,	2,"models/brokentile/tilebroken_01b");
			InitMaterial(EDGE_LEFT,	2,"models/brokentile/tilebroken_01c");
			InitMaterial(EDGE_RIGHT,2,"models/brokentile/tilebroken_01d");
		}
		else 
		{
			InitMaterial(EDGE_NONE,	0,"models/brokenglass/glassbroken_03a");
			InitMaterial(EDGE_FULL,	0,"models/brokenglass/glassbroken_03b");
			InitMaterial(EDGE_LEFT,	0,"models/brokenglass/glassbroken_03c");
			InitMaterial(EDGE_RIGHT,0,"models/brokenglass/glassbroken_03d");

			InitMaterial(EDGE_NONE,	1,"models/brokenglass/glassbroken_02a");
			InitMaterial(EDGE_FULL,	1,"models/brokenglass/glassbroken_02b");
			InitMaterial(EDGE_LEFT,	1,"models/brokenglass/glassbroken_02c");
			InitMaterial(EDGE_RIGHT,1,"models/brokenglass/glassbroken_02d");

			InitMaterial(EDGE_NONE,	2,"models/brokenglass/glassbroken_01a");
			InitMaterial(EDGE_FULL,	2,"models/brokenglass/glassbroken_01b");
			InitMaterial(EDGE_LEFT,	2,"models/brokenglass/glassbroken_01c");
			InitMaterial(EDGE_RIGHT,2,"models/brokenglass/glassbroken_01d");
		}
	}
	
	bool changed = false;

	for ( int j = 0; j < m_nNumHigh; j++ )
	{
		for ( int i = 0; i < m_nNumWide; i++ )
		{
			int offset = i + j * m_nNumWide;

			bool newVal = m_RawPanelBitVec[ offset ];
			bool oldVal = m_PrevRawPanelBitVec[ offset ];

			if ( newVal != oldVal )
			{
				changed = true;
			}

			SetPanelSolid(i,j,newVal);

			if ( !newVal && changed )
			{
					
				// Mark these panels and being stale (need edge type updated)
				// We update them in one fell swoop rather than as each panel
				// is updated, so we don't have to do duplicate operations
				SetPanelStale(i,	j ,true);
				SetPanelStale(i,   j+1,true);
				SetPanelStale(i,   j-1,true);
				SetPanelStale(i-1, j  ,true);
				SetPanelStale(i+1, j  ,true);
				SetPanelStale(i+1, j+1,true);
				SetPanelStale(i-1, j+1,true);
				SetPanelStale(i+1, j-1,true);
				SetPanelStale(i-1, j-1,true);
			}
		}
	}

	if ( changed )
	{
		for (int width=0;width<m_nNumWide;width++)
		{
			for (int height=0;height<m_nNumHigh;height++)
			{
				if ( IsPanelStale(width,height) )
				{
					UpdateEdgeType( width, height );
				}
			}
		}
	}

	Q_memcpy( m_PrevRawPanelBitVec, m_RawPanelBitVec.Base(), sizeof( m_PrevRawPanelBitVec ) );
}

bool C_BreakableSurface::IsTransparent( void )
{
	// Not an identity brush if it's broken
	if (m_bIsBroken)
		return true;

	return C_BaseEntity::IsTransparent();
}
  

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
int C_BreakableSurface::DrawModel( int flags )
{
	if ( !m_bReadyToDraw )
		return 0;

	// If I'm not broken draw normally
	if (m_bIsBroken)
		render->InstallBrushSurfaceRenderer( this );

	// If it's broken, always draw it translucent
	BaseClass::DrawModel( m_bIsBroken ? flags | STUDIO_TRANSPARENCY : flags );

	// Remove our nonstandard brush surface renderer...
	render->InstallBrushSurfaceRenderer( 0 );

	return 0;
}

bool C_BreakableSurface::RenderBrushModelSurface( IClientEntity* pBaseEntity, IBrushSurface* pBrushSurface )
{
	// If tile draw highlight for grout
	if (m_nSurfaceType == SHATTERSURFACE_TILE)
	{
		DrawRenderListHighlights(pBrushSurface);
	}
	DrawSolidBlocks(pBrushSurface);
	DrawRenderList(pBrushSurface);

	// Don't draw decals
	return false;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_BreakableSurface::DrawRenderList(IBrushSurface* pBrushSurface)
{
	// Get width and height steps
	QAngle vAngles;
	VectorAngles(-1*m_vNormal,vAngles);
	Vector vWidthStep,vHeightStep;
	AngleVectors(vAngles,NULL,&vWidthStep,&vHeightStep);
	vWidthStep	*= m_flPanelWidth;
	vHeightStep *= m_flPanelHeight;

	CMeshBuilder	pMeshBuilder;
	IMesh*			pMesh			= NULL;
	int				nCurStyle		= -1;
	int				nCurEdgeType	= -1;
	CMatRenderContextPtr pRenderContext( materials );
	for( unsigned short i = m_RenderList.Head(); i != m_RenderList.InvalidIndex(); i = m_RenderList.Next(i) )
	{
	
		if (nCurStyle		!= m_RenderList[i].m_nStyle		||
			nCurEdgeType	!= m_RenderList[i].m_nEdgeType	)
		{
			nCurStyle	 = m_RenderList[i].m_nStyle;
			nCurEdgeType = m_RenderList[i].m_nEdgeType;

			m_pCurrentDetailTexture = m_pEdge[nCurEdgeType][nCurStyle].m_pMaterialEdgeTexture;
			pRenderContext->Flush(false);
			pRenderContext->Bind(m_pCrackedMaterial, (IClientRenderable*)this);
			pMesh = pRenderContext->GetDynamicMesh( );
		}

		Vector vRenderPos = m_vCorner + 
							(m_RenderList[i].m_nWidth*vWidthStep)	+ 
							(m_RenderList[i].m_nHeight*vHeightStep);

		DrawOneEdge(pBrushSurface, pMesh,&pMeshBuilder,vRenderPos,vWidthStep,vHeightStep,(WinSide_t)m_RenderList[i].m_nSide);
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_BreakableSurface::DrawRenderListHighlights(IBrushSurface* pBrushSurface)
{
	// Get width and height steps
	QAngle vAngles;
	VectorAngles(-1*m_vNormal,vAngles);
	Vector vWidthStep,vHeightStep;
	AngleVectors(vAngles,NULL,&vWidthStep,&vHeightStep);
	vWidthStep	*= m_flPanelWidth;
	vHeightStep *= m_flPanelHeight;


	CMeshBuilder	pMeshBuilder;
	IMesh*			pMesh			= NULL;
	int				nCurStyle		= -1;
	int				nCurEdgeType	= -1;
	CMatRenderContextPtr pRenderContext( materials );
	for( unsigned short i = m_RenderList.Head(); i != m_RenderList.InvalidIndex(); i = m_RenderList.Next(i) )
	{
	
		if (nCurStyle		!= m_RenderList[i].m_nStyle		||
			nCurEdgeType	!= m_RenderList[i].m_nEdgeType	)
		{
			nCurStyle	 = m_RenderList[i].m_nStyle;
			nCurEdgeType = m_RenderList[i].m_nEdgeType;
			
			IMaterial *pMat = m_pEdge[nCurEdgeType][nCurStyle].m_pMaterialEdge;
			pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMat );
		}

		Vector vRenderPos = m_vCorner + 
							(m_RenderList[i].m_nWidth*vWidthStep)	+ 
							(m_RenderList[i].m_nHeight*vHeightStep) +
							(0.30*m_vNormal);

		DrawOneHighlight(pBrushSurface, pMesh,&pMeshBuilder,vRenderPos,vWidthStep,vHeightStep,(WinSide_t)m_RenderList[i].m_nSide);
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool C_BreakableSurface::HavePanel(int nWidth, int nHeight)
{
	// If I'm off the edge, always give support
	if (!InLegalRange(nWidth,nHeight))
	{
		return true;
	}
	return (IsPanelSolid(nWidth,nHeight));
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_BreakableSurface::UpdateEdgeType(int nWidth, int nHeight, int forceStyle /*=-1*/ )
{
	Assert( forceStyle < NUM_EDGE_STYLES );

	// -----------------------------------
	//  Check edge conditions
	// -----------------------------------
	if (!InLegalRange(nWidth,nHeight))
	{
		return;
	}

	// ----------------------------------
	//  If solid has no edges
	// ----------------------------------
	if (IsPanelSolid(nWidth,nHeight))
	{
		return;
	}
	
	// Panel is no longer stale
	SetPanelStale(nWidth, nHeight,false);

	// ----------------------------------
	//  Set edge type base on neighbors
	// ----------------------------------
	bool bUp		= HavePanel(nWidth,   nHeight+1);
	bool bDown		= HavePanel(nWidth,   nHeight-1);
	bool bLeft		= HavePanel(nWidth-1, nHeight  );
	bool bRight		= HavePanel(nWidth+1, nHeight  );

	bool bUpLeft	= HavePanel(nWidth-1,  nHeight+1);
	bool bUpRight	= HavePanel(nWidth+1,  nHeight+1);
	bool bDownLeft	= HavePanel(nWidth-1,  nHeight-1);
	bool bDownRight	= HavePanel(nWidth+1,  nHeight-1);

	//-------------
	// Top
	//-------------
	if (bUp)
	{
		bool bLeftEdge		= !bLeft  && bUpLeft;
		bool bRightEdge		= !bRight && bUpRight;

		if (bLeftEdge && bRightEdge)
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_TOP, EDGE_FULL, forceStyle );
		}
		else if (bLeftEdge)
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_TOP, EDGE_LEFT, forceStyle );
		}
		else if (bRightEdge)
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_TOP, EDGE_RIGHT, forceStyle );
		}
		else
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_TOP, EDGE_NONE, forceStyle );
		}
	}
	else
	{
		AddToRenderList(nWidth, nHeight, WIN_SIDE_TOP, EDGE_NOT, forceStyle );
	}
	//-------------
	// Bottom
	//-------------
	if (bDown)
	{
		bool bLeftEdge		= !bLeft  && bDownLeft;
		bool bRightEdge		= !bRight && bDownRight;

		if (bLeftEdge && bRightEdge)
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_BOTTOM, EDGE_FULL, forceStyle );
		}
		else if (bLeftEdge)
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_BOTTOM, EDGE_RIGHT, forceStyle );
		}
		else if (bRightEdge)
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_BOTTOM, EDGE_LEFT, forceStyle );
		}
		else
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_BOTTOM, EDGE_NONE, forceStyle );
		}
	}
	else
	{
		AddToRenderList(nWidth, nHeight, WIN_SIDE_BOTTOM, EDGE_NOT, forceStyle );
	}
	//-------------
	// Left
	//-------------
	if (bLeft)
	{
		bool bTop		= !bUp 	 && bUpLeft;
		bool bBottom	= !bDown && bDownLeft;

		if (bTop && bBottom)
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_LEFT, EDGE_FULL, forceStyle );
		}
		else if (bTop)
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_LEFT, EDGE_RIGHT, forceStyle );
		}
		else if (bBottom)
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_LEFT, EDGE_LEFT, forceStyle );
		}
		else
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_LEFT, EDGE_NONE, forceStyle );
		}
	}
	else
	{
		AddToRenderList(nWidth, nHeight, WIN_SIDE_LEFT, EDGE_NOT, forceStyle );
	}
	//-------------
	// Right
	//-------------
	if (bRight)
	{
		bool bTop		= !bUp 	 && bUpRight;
		bool bBottom	= !bDown && bDownRight;

		if (bTop && bBottom)
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_RIGHT, EDGE_FULL, forceStyle );
		}
		else if (bTop)
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_RIGHT, EDGE_LEFT, forceStyle );
		}
		else if (bBottom)
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_RIGHT, EDGE_RIGHT, forceStyle );
		}
		else
		{
			AddToRenderList(nWidth, nHeight, WIN_SIDE_RIGHT, EDGE_NONE, forceStyle );
		}
	}
	else
	{
		AddToRenderList(nWidth, nHeight, WIN_SIDE_RIGHT, EDGE_NOT, forceStyle );
	}
}

//--------------------------------------------------------------------------------
// Purpose : Return index to panel in render list that meets these qualifications
// Input   :
// Output  :
//--------------------------------------------------------------------------------
int C_BreakableSurface::FindRenderPanel(int nWidth, int nHeight, WinSide_t nWinSide)
{
	for( unsigned short i = m_RenderList.Head(); i != m_RenderList.InvalidIndex(); i = m_RenderList.Next(i) )
	{
		if (m_RenderList[i].m_nSide			== nWinSide	&&
			m_RenderList[i].m_nWidth		== nWidth		&&
			m_RenderList[i].m_nHeight		== nHeight)
		{
			return i;
		}
	}
	return m_RenderList.InvalidIndex();
}

//----------------------------------------------------------------------------------
// Purpose : Returns first element in render list with the same edge type and style
// Input   :
// Output  :
//----------------------------------------------------------------------------------
int C_BreakableSurface::FindFirstRenderTexture(WinEdge_t nEdgeType, int nStyle)
{
	for( unsigned short i = m_RenderList.Head(); i != m_RenderList.InvalidIndex(); i = m_RenderList.Next(i) )
	{
		if (m_RenderList[i].m_nStyle		== nStyle		&&
			m_RenderList[i].m_nEdgeType		== nEdgeType	)
		{
			return i;
		}
	}
	return m_RenderList.InvalidIndex();
}

//------------------------------------------------------------------------------
// Purpose : Add a edge to be rendered to the render list
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_BreakableSurface::AddToRenderList(int nWidth, int nHeight, WinSide_t nSide, WinEdge_t nEdgeType, int forceStyle )
{
	// -----------------------------------------------------
	// Try to find old panel
	int nOldPanelIndex = FindRenderPanel(nWidth,nHeight,nSide);

	// -----------------------------------------------------
	// If I have an old panel, get it's style and remove it
	// otherwise randomly chose a style
	int nStyle;
	if (m_RenderList.IsValidIndex(nOldPanelIndex) )
	{
		nStyle = m_RenderList[nOldPanelIndex].m_nStyle;
		m_RenderList.Remove(nOldPanelIndex);
	}
	else
	{
		nStyle = random->RandomInt(0,NUM_EDGE_STYLES-1);
	}
		
	if ( forceStyle != -1 )
	{
		nStyle = forceStyle;
	}

	// -----------------------------------------------------
	// If my new panel has an edge, add it to render list
	if (nEdgeType != EDGE_NOT)
	{
		// Renderlist is sorted by texture type.  Find first element
		// that shares the same texture as the new panel
		unsigned short nTexIndex = FindFirstRenderTexture(nEdgeType, nStyle);

		// If texture was already in list, add after last use
		unsigned short nNewIndex;
		if (m_RenderList.IsValidIndex(nTexIndex))
		{
			nNewIndex = m_RenderList.InsertAfter(nTexIndex);
		}
		// Otherwise add to send of render list
		else
		{
			nNewIndex = m_RenderList.AddToTail();
		}

		// Now fill out my data
		m_RenderList[nNewIndex].m_nHeight	= nHeight;
		m_RenderList[nNewIndex].m_nWidth	= nWidth;
		m_RenderList[nNewIndex].m_nEdgeType	= nEdgeType;
		m_RenderList[nNewIndex].m_nSide		= nSide;
		m_RenderList[nNewIndex].m_nStyle	= nStyle;

		Assert( nStyle < NUM_EDGE_STYLES );
		SetStyleType( nWidth, nHeight, nStyle );
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_BreakableSurface::DrawSolidBlocks(IBrushSurface* pBrushSurface)
{
	CMatRenderContextPtr pRenderContext( materials );

	m_pCurrentDetailTexture = m_pMaterialBoxTexture;

	// Gotta flush (in a non-stalling way) because we effectively 
	// have a new material due to the new base texture
	pRenderContext->Flush(false);
	pRenderContext->Bind(m_pCrackedMaterial, (IClientRenderable*)this);
	IMesh* pMesh = pRenderContext->GetDynamicMesh( );
	CMeshBuilder pMeshBuilder;

	// ---------------
	// Create panels
	// ---------------
	QAngle vAngles;
	VectorAngles(-1*m_vNormal,vAngles);
	Vector vWidthStep,vHeightStep;
	AngleVectors(vAngles,NULL,&vWidthStep,&vHeightStep);
	vWidthStep	*= m_flPanelWidth;
	vHeightStep *= m_flPanelHeight;

	Vector vCurPos = m_vCorner;
	for (int width=0;width<m_nNumWide;width++)
	{
		int height;
		int nHCount = 0;
		for (height=0;height<m_nNumHigh;height++)
		{
			// Keep count of how many panes there are in a row
			if (IsPanelSolid(width,height))
			{
				nHCount++;
			}

			// Drow the strip and start counting again
			else if (nHCount > 0)
			{
				vCurPos = m_vCorner + vWidthStep*width + vHeightStep*(height-nHCount);
				DrawOneBlock(pBrushSurface, pMesh, &pMeshBuilder, vCurPos,vWidthStep,vHeightStep*nHCount);
				nHCount = 0;
			}
		}
		if (nHCount)
		{
			vCurPos = m_vCorner + vWidthStep*width + vHeightStep*(height-nHCount);
			DrawOneBlock(pBrushSurface, pMesh, &pMeshBuilder, vCurPos,vWidthStep,vHeightStep*nHCount);
		}
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_BreakableSurface::DrawOneBlock(IBrushSurface* pBrushSurface, IMesh* pMesh, 
	CMeshBuilder *pMeshBuilder,  const Vector &vCurPos, const Vector &vWidthStep, 
	const Vector &vHeightStep)
{
	pMeshBuilder->Begin( pMesh, MATERIAL_QUADS, 1 );

	Vector2D texCoord, lightCoord;
	pBrushSurface->ComputeTextureCoordinate( vCurPos, texCoord );
	pBrushSurface->ComputeLightmapCoordinate( vCurPos, lightCoord );

	pMeshBuilder->Position3f( vCurPos.x, vCurPos.y, vCurPos.z );
	pMeshBuilder->Color4ub( 255, 255, 255, 255 );
	pMeshBuilder->TexCoord2f( 0, 0, 1 );
	pMeshBuilder->TexCoord2fv( 1, lightCoord.Base() );
	pMeshBuilder->TexCoord2fv( 2, texCoord.Base() );
	pMeshBuilder->Normal3fv( m_vNormal.Base() );
 	pMeshBuilder->AdvanceVertex();

	Vector vNextPos = vCurPos + vWidthStep;

	pBrushSurface->ComputeTextureCoordinate( vNextPos, texCoord );
	pBrushSurface->ComputeLightmapCoordinate( vNextPos, lightCoord );

	pMeshBuilder->Position3f( vNextPos.x, vNextPos.y, vNextPos.z );
	pMeshBuilder->Color4ub( 255, 255, 255, 255 );
	pMeshBuilder->TexCoord2f( 0, 0, 0 );
	pMeshBuilder->TexCoord2fv( 1, lightCoord.Base() );
	pMeshBuilder->TexCoord2fv( 2, texCoord.Base() );
 	pMeshBuilder->Normal3fv( m_vNormal.Base() );
	pMeshBuilder->AdvanceVertex();

	vNextPos = vNextPos + vHeightStep;

	pBrushSurface->ComputeTextureCoordinate( vNextPos, texCoord );
	pBrushSurface->ComputeLightmapCoordinate( vNextPos, lightCoord );

	pMeshBuilder->Position3f( vNextPos.x , vNextPos.y, vNextPos.z );
	pMeshBuilder->Color4ub( 255, 255, 255, 255 );
	pMeshBuilder->TexCoord2f( 0, 1, 0 );
	pMeshBuilder->TexCoord2fv( 1, lightCoord.Base() );
	pMeshBuilder->TexCoord2fv( 2, texCoord.Base() );
	pMeshBuilder->Normal3fv( m_vNormal.Base() );
 	pMeshBuilder->AdvanceVertex();

	vNextPos = vNextPos - vWidthStep;

	pBrushSurface->ComputeTextureCoordinate( vNextPos, texCoord );
	pBrushSurface->ComputeLightmapCoordinate( vNextPos, lightCoord );

	pMeshBuilder->Position3f( vNextPos.x, vNextPos.y , vNextPos.z);
	pMeshBuilder->Color4ub( 255, 255, 255, 255 );
	pMeshBuilder->TexCoord2f( 0, 1, 1 );
	pMeshBuilder->TexCoord2fv( 1, lightCoord.Base() );
	pMeshBuilder->TexCoord2fv( 2, texCoord.Base() );
	pMeshBuilder->Normal3fv( m_vNormal.Base() );
 	pMeshBuilder->AdvanceVertex();

	pMeshBuilder->End();
	pMesh->Draw();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_BreakableSurface::DrawOneEdge( IBrushSurface* pBrushSurface, IMesh* pMesh, 
	CMeshBuilder *pMeshBuilder,  const Vector &vStartPos,  const Vector &vWStep, 
	const Vector &vHStep, WinSide_t nEdge )
{
	pMeshBuilder->Begin( pMesh, MATERIAL_QUADS, 1 );

	Vector2D texCoord, lightCoord;
	pBrushSurface->ComputeTextureCoordinate( vStartPos, texCoord );
	pBrushSurface->ComputeLightmapCoordinate( vStartPos, lightCoord );

	pMeshBuilder->Position3f( vStartPos.x, vStartPos.y, vStartPos.z);
	pMeshBuilder->Normal3fv( m_vNormal.Base() );
	pMeshBuilder->Color4ub( 255, 255, 255, 255 );
	pMeshBuilder->TexCoord2fv( 1, lightCoord.Base() );
	pMeshBuilder->TexCoord2fv( 2, texCoord.Base() );
	switch (nEdge)
	{
		case WIN_SIDE_RIGHT:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 0.0f );
			break;
		case WIN_SIDE_BOTTOM:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 1.0f );
			break;
		case WIN_SIDE_LEFT:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 1.0f );
			break;
		case WIN_SIDE_TOP:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 0.0f );
			break;
	}
 	pMeshBuilder->AdvanceVertex();
	
	Vector vNextPos = vStartPos + vWStep;

	pBrushSurface->ComputeTextureCoordinate( vNextPos, texCoord );
	pBrushSurface->ComputeLightmapCoordinate( vNextPos, lightCoord );

	pMeshBuilder->Position3f( vNextPos.x , vNextPos.y , vNextPos.z);
	pMeshBuilder->Normal3fv( m_vNormal.Base() );
	pMeshBuilder->Color4ub( 255, 255, 255, 255 );
	pMeshBuilder->TexCoord2fv( 1, lightCoord.Base() );
	pMeshBuilder->TexCoord2fv( 2, texCoord.Base() );
	switch (nEdge)
	{
		case WIN_SIDE_RIGHT:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 1.0f );
			break;
		case WIN_SIDE_BOTTOM:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 1.0f );
			break;
		case WIN_SIDE_LEFT:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 0.0f );
			break;
		case WIN_SIDE_TOP:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 0.0f );
			break;
	}
 	pMeshBuilder->AdvanceVertex();

	vNextPos = vNextPos + vHStep;

	pBrushSurface->ComputeTextureCoordinate( vNextPos, texCoord );
	pBrushSurface->ComputeLightmapCoordinate( vNextPos, lightCoord );

	pMeshBuilder->Position3f( vNextPos.x, vNextPos.y, vNextPos.z );
	pMeshBuilder->Normal3fv( m_vNormal.Base() );
	pMeshBuilder->Color4ub( 255, 255, 255, 255 );
	pMeshBuilder->TexCoord2fv( 1, lightCoord.Base() );
	pMeshBuilder->TexCoord2fv( 2, texCoord.Base() );
	switch (nEdge)
	{
		case WIN_SIDE_RIGHT:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 1.0f );
			break;
		case WIN_SIDE_BOTTOM:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 0.0f );
			break;
		case WIN_SIDE_LEFT:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 0.0f );
			break;
		case WIN_SIDE_TOP:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 1.0f );
			break;
	}
 	pMeshBuilder->AdvanceVertex();

	vNextPos = vNextPos - vWStep;

	pBrushSurface->ComputeTextureCoordinate( vNextPos, texCoord );
	pBrushSurface->ComputeLightmapCoordinate( vNextPos, lightCoord );

	pMeshBuilder->Position3f( vNextPos.x, vNextPos.y, vNextPos.z );
	pMeshBuilder->Normal3fv( m_vNormal.Base() );
	pMeshBuilder->Color4ub( 255, 255, 255, 255 );
	pMeshBuilder->TexCoord2fv( 1, lightCoord.Base() );
	pMeshBuilder->TexCoord2fv( 2, texCoord.Base() );
	switch (nEdge)
	{
		case WIN_SIDE_RIGHT:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 0.0f );
			break;
		case WIN_SIDE_BOTTOM:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 0.0f );
			break;
		case WIN_SIDE_LEFT:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 1.0f );
			break;
		case WIN_SIDE_TOP:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 1.0f );
			break;
	}
 	pMeshBuilder->AdvanceVertex();

	pMeshBuilder->End();
	pMesh->Draw();
}




//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_BreakableSurface::DrawOneHighlight( IBrushSurface* pBrushSurface, IMesh* pMesh, 
	CMeshBuilder *pMeshBuilder,  const Vector &vStartPos,  const Vector &vWStep, 
	const Vector &vHStep, WinSide_t nEdge )
{
	Vector vColor = Vector(0.41,0.35,0.24);

	pMeshBuilder->Begin( pMesh, MATERIAL_QUADS, 1 );

	Vector2D texCoord, lightCoord;
	pBrushSurface->ComputeTextureCoordinate( vStartPos, texCoord );
	pBrushSurface->ComputeLightmapCoordinate( vStartPos, lightCoord );

	pMeshBuilder->Position3f( vStartPos.x, vStartPos.y, vStartPos.z);
	pMeshBuilder->Normal3fv( m_vNormal.Base() );
	pMeshBuilder->Color4f( vColor[0], vColor[1], vColor[2], 1.0);
	pMeshBuilder->TexCoord2fv( 1, lightCoord.Base() );
	switch (nEdge)
	{
		case WIN_SIDE_RIGHT:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 0.0f );
			break;
		case WIN_SIDE_BOTTOM:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 1.0f );
			break;
		case WIN_SIDE_LEFT:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 1.0f );
			break;
		case WIN_SIDE_TOP:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 0.0f );
			break;
	}
 	pMeshBuilder->AdvanceVertex();
	
	Vector vNextPos = vStartPos + vWStep;

	pBrushSurface->ComputeTextureCoordinate( vNextPos, texCoord );
	pBrushSurface->ComputeLightmapCoordinate( vNextPos, lightCoord );

	pMeshBuilder->Position3f( vNextPos.x , vNextPos.y , vNextPos.z);
	pMeshBuilder->Normal3fv( m_vNormal.Base() );
	pMeshBuilder->Color4f( vColor[0], vColor[1], vColor[2], 1.0);
	pMeshBuilder->TexCoord2fv( 1, lightCoord.Base() );
	switch (nEdge)
	{
		case WIN_SIDE_RIGHT:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 1.0f );
			break;
		case WIN_SIDE_BOTTOM:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 1.0f );
			break;
		case WIN_SIDE_LEFT:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 0.0f );
			break;
		case WIN_SIDE_TOP:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 0.0f );
			break;
	}
 	pMeshBuilder->AdvanceVertex();

	vNextPos = vNextPos + vHStep;

	pBrushSurface->ComputeTextureCoordinate( vNextPos, texCoord );
	pBrushSurface->ComputeLightmapCoordinate( vNextPos, lightCoord );

	pMeshBuilder->Position3f( vNextPos.x, vNextPos.y, vNextPos.z );
	pMeshBuilder->Normal3fv( m_vNormal.Base() );
	pMeshBuilder->Color4f( vColor[0], vColor[1], vColor[2], 1.0);
	pMeshBuilder->TexCoord2fv( 1, lightCoord.Base() );
	switch (nEdge)
	{
		case WIN_SIDE_RIGHT:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 1.0f );
			break;
		case WIN_SIDE_BOTTOM:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 0.0f );
			break;
		case WIN_SIDE_LEFT:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 0.0f );
			break;
		case WIN_SIDE_TOP:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 1.0f );
			break;
	}
 	pMeshBuilder->AdvanceVertex();

	vNextPos = vNextPos - vWStep;

	pBrushSurface->ComputeTextureCoordinate( vNextPos, texCoord );
	pBrushSurface->ComputeLightmapCoordinate( vNextPos, lightCoord );

	pMeshBuilder->Position3f( vNextPos.x, vNextPos.y, vNextPos.z );
	pMeshBuilder->Normal3fv( m_vNormal.Base() );
	pMeshBuilder->Color4f( vColor[0], vColor[1], vColor[2], 1.0);
	pMeshBuilder->TexCoord2fv( 1, lightCoord.Base() );
	switch (nEdge)
	{
		case WIN_SIDE_RIGHT:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 0.0f );
			break;
		case WIN_SIDE_BOTTOM:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 0.0f );
			break;
		case WIN_SIDE_LEFT:
			pMeshBuilder->TexCoord2f( 0, 1.0f, 1.0f );
			break;
		case WIN_SIDE_TOP:
			pMeshBuilder->TexCoord2f( 0, 0.0f, 1.0f );
			break;
	}
 	pMeshBuilder->AdvanceVertex();

	pMeshBuilder->End();
	pMesh->Draw();
}

bool C_BreakableSurface::ShouldReceiveProjectedTextures( int flags )
{
	return false;
}


//------------------------------------------------------------------------------
// A material proxy that resets the texture to use the original surface texture
//------------------------------------------------------------------------------
class CBreakableSurfaceProxy : public CEntityMaterialProxy
{
public:
	CBreakableSurfaceProxy();
	virtual ~CBreakableSurfaceProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( C_BaseEntity *pC_BaseEntity );
	virtual IMaterial *GetMaterial();

private:
	// get at the material whose texture we're going to steal
	void AcquireSourceMaterial( C_BaseEntity* pEnt );

	IMaterialVar* m_BaseTextureVar;
};

CBreakableSurfaceProxy::CBreakableSurfaceProxy()
{
	m_BaseTextureVar = NULL;
}

CBreakableSurfaceProxy::~CBreakableSurfaceProxy()
{
}


bool CBreakableSurfaceProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool foundVar;
	m_BaseTextureVar = pMaterial->FindVar( "$basetexture", &foundVar, false );
	return foundVar;
}

void CBreakableSurfaceProxy::OnBind( C_BaseEntity *pC_BaseEntity )
{
	C_BreakableSurface *pEnt = dynamic_cast< C_BreakableSurface * >(pC_BaseEntity);
	if( !pEnt )
	{
		return;
	}

	// Use the current base texture specified by the suface
	m_BaseTextureVar->SetTextureValue( pEnt->m_pCurrentDetailTexture );
}

IMaterial *CBreakableSurfaceProxy::GetMaterial()
{
	if ( !m_BaseTextureVar )
		return NULL;

	return m_BaseTextureVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CBreakableSurfaceProxy, IMaterialProxy, "BreakableSurface" IMATERIAL_PROXY_INTERFACE_VERSION );
