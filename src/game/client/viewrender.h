//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#if !defined( VIEWRENDER_H )
#define VIEWRENDER_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "tier1/utlstack.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "replay/ireplayscreenshotsystem.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ConVar;
class CClientRenderablesList;
class IClientVehicle;
class C_PointCamera;
class C_EnvProjectedTexture;
class IScreenSpaceEffect;
class CClientViewSetup;
class CViewRender;
struct ClientWorldListInfo_t;
class C_BaseEntity;
struct WriteReplayScreenshotParams_t;
class CReplayScreenshotTaker;

#ifdef HL2_EPISODIC
	class CStunEffect;
#endif // HL2_EPISODIC

//-----------------------------------------------------------------------------
// Data specific to intro mode to control rendering.
//-----------------------------------------------------------------------------
struct IntroDataBlendPass_t
{
	int m_BlendMode;
	float m_Alpha; // in [0.0f,1.0f]  This needs to add up to 1.0 for all passes, unless you are fading out.
};

struct IntroData_t
{
	bool	m_bDrawPrimary;
	Vector	m_vecCameraView;
	QAngle	m_vecCameraViewAngles;
	float	m_playerViewFOV;
	CUtlVector<IntroDataBlendPass_t> m_Passes;

	// Fade overriding for the intro
	float	m_flCurrentFadeColor[4];
};

// Robin, make this point at something to get intro mode.
extern IntroData_t *g_pIntroData;

// This identifies the view for certain systems that are unique per view (e.g. pixel visibility)
// NOTE: This is identifying which logical part of the scene an entity is being redered in
// This is not identifying a particular render target necessarily.  This is mostly needed for entities that
// can be rendered more than once per frame (pixel vis queries need to be identified per-render call)
enum view_id_t
{
	VIEW_ILLEGAL = -2,
	VIEW_NONE = -1,
	VIEW_MAIN = 0,
	VIEW_3DSKY = 1,
	VIEW_MONITOR = 2,
	VIEW_REFLECTION = 3,
	VIEW_REFRACTION = 4,
	VIEW_INTRO_PLAYER = 5,
	VIEW_INTRO_CAMERA = 6,
	VIEW_SHADOW_DEPTH_TEXTURE = 7,
	VIEW_SSAO = 8,
	VIEW_ID_COUNT
};
view_id_t CurrentViewID();

//-----------------------------------------------------------------------------
// Purpose: Stored pitch drifting variables
//-----------------------------------------------------------------------------
class CPitchDrift
{
public:
	float		pitchvel;
	bool		nodrift;
	float		driftmove;
	double		laststop;
};



//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
struct ViewCustomVisibility_t
{
	ViewCustomVisibility_t()
	{
		m_nNumVisOrigins = 0;
		m_VisData.m_fDistToAreaPortalTolerance = FLT_MAX; 
		m_iForceViewLeaf = -1;
	}

	void AddVisOrigin( const Vector& origin )
	{
		// Don't allow them to write past array length
		AssertMsg( m_nNumVisOrigins < MAX_VIS_LEAVES, "Added more origins than will fit in the array!" );

		// If the vis origin count is greater than the size of our array, just fail to add this origin
		if ( m_nNumVisOrigins >= MAX_VIS_LEAVES )
			return;

		m_rgVisOrigins[ m_nNumVisOrigins++ ] = origin;
	}

	void ForceVisOverride( VisOverrideData_t& visData )
	{
		m_VisData = visData;
	}

	void ForceViewLeaf ( int iViewLeaf )
	{
		m_iForceViewLeaf = iViewLeaf;
	}

	// Set to true if you want to use multiple origins for doing client side map vis culling
	// NOTE:  In generaly, you won't want to do this, and by default the 3d origin of the camera, as above,
	//  will be used as the origin for vis, too.
	int				m_nNumVisOrigins;
	// Array of origins
	Vector			m_rgVisOrigins[ MAX_VIS_LEAVES ];

	// The view data overrides for visibility calculations with area portals
	VisOverrideData_t m_VisData;

	// The starting leaf to determing which area to start in when performing area portal culling on the engine
	// Default behavior is to use the leaf the camera position is in.
	int				m_iForceViewLeaf;
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
struct WaterRenderInfo_t
{
	bool m_bCheapWater : 1;
	bool m_bReflect : 1;
	bool m_bRefract : 1;
	bool m_bReflectEntities : 1;
	bool m_bDrawWaterSurface : 1;
	bool m_bOpaqueWater : 1;

};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CBase3dView : public CRefCounted<>,
					protected CViewSetup
{
	DECLARE_CLASS_NOBASE( CBase3dView );
public:
	CBase3dView( CViewRender *pMainView );

	VPlane *		GetFrustum();
	virtual int		GetDrawFlags() { return 0; }

#ifdef PORTAL
	virtual	void	EnableWorldFog() {};
#endif

protected:
	// @MULTICORE (toml 8/11/2006): need to have per-view frustum. Change when move view stack to client
	VPlane			*m_Frustum;
	CViewRender *m_pMainView;
};

//-----------------------------------------------------------------------------
// Base class for 3d views
//-----------------------------------------------------------------------------
class CRendering3dView : public CBase3dView
{
	DECLARE_CLASS( CRendering3dView, CBase3dView );
public:
	CRendering3dView( CViewRender *pMainView );
	virtual ~CRendering3dView() { ReleaseLists(); }

	void Setup( const CViewSetup &setup );

	// What are we currently rendering? Returns a combination of DF_ flags.
	virtual int		GetDrawFlags();

	virtual void	Draw() {};

protected:

	// Fog setup
	void			EnableWorldFog( void );
	void			SetFogVolumeState( const VisibleFogVolumeInfo_t &fogInfo, bool bUseHeightFog );

	// Draw setup
	void			SetupRenderablesList( int viewID );

	void			UpdateRenderablesOpacity();

	// If iForceViewLeaf is not -1, then it uses the specified leaf as your starting area for setting up area portal culling.
	// This is used by water since your reflected view origin is often in solid space, but we still want to treat it as though
	// the first portal we're looking out of is a water portal, so our view effectively originates under the water.
	void			BuildWorldRenderLists( bool bDrawEntities, int iForceViewLeaf = -1, bool bUseCacheIfEnabled = true, bool bShadowDepth = false, float *pReflectionWaterHeight = NULL );

	// Purpose: Builds render lists for renderables. Called once for refraction, once for over water
	void			BuildRenderableRenderLists( int viewID );

	// More concise version of the above BuildRenderableRenderLists().  Called for shadow depth map rendering
	void			BuildShadowDepthRenderableRenderLists();

	void			DrawWorld( float waterZAdjust );

	// Draws all opaque/translucent renderables in leaves that were rendered
	void			DrawOpaqueRenderables( ERenderDepthMode DepthMode );
	void			DrawTranslucentRenderables( bool bInSkybox, bool bShadowDepth );

	// Renders all translucent entities in the render list
	void			DrawTranslucentRenderablesNoWorld( bool bInSkybox );

	// Draws translucent renderables that ignore the Z buffer
	void			DrawNoZBufferTranslucentRenderables( void );

	// Renders all translucent world surfaces in a particular set of leaves
	void			DrawTranslucentWorldInLeaves( bool bShadowDepth );

	// Renders all translucent world + detail objects in a particular set of leaves
	void			DrawTranslucentWorldAndDetailPropsInLeaves( int iCurLeaf, int iFinalLeaf, int nEngineDrawFlags, int &nDetailLeafCount, LeafIndex_t* pDetailLeafList, bool bShadowDepth );

	// Purpose: Computes the actual world list info based on the render flags
	void			PruneWorldListInfo();

#ifdef PORTAL
	virtual bool	ShouldDrawPortals() { return true; }
#endif

	void ReleaseLists();

	//-----------------------------------------------
	// Combination of DF_ flags.
	int m_DrawFlags;
	int m_ClearFlags;

	IWorldRenderList *m_pWorldRenderList;
	CClientRenderablesList *m_pRenderablesList;
	ClientWorldListInfo_t *m_pWorldListInfo;
	ViewCustomVisibility_t *m_pCustomVisibility;
};


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

class CRenderExecutor
{
	DECLARE_CLASS_NOBASE( CRenderExecutor );
public:
	virtual void AddView( CRendering3dView *pView ) = 0;
	virtual void Execute() = 0;

protected:
	CRenderExecutor( CViewRender *pMainView ) : m_pMainView( pMainView ) {}
	CViewRender *m_pMainView;
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

class CSimpleRenderExecutor : public CRenderExecutor
{
	DECLARE_CLASS( CSimpleRenderExecutor, CRenderExecutor );
public:
	CSimpleRenderExecutor( CViewRender *pMainView ) : CRenderExecutor( pMainView ) {}

	void AddView( CRendering3dView *pView );
	void Execute() {}
};

//-----------------------------------------------------------------------------
// Purpose: Implements the interface to view rendering for the client .dll
//-----------------------------------------------------------------------------

class CViewRender : public IViewRender,
					public IReplayScreenshotSystem
{
	DECLARE_CLASS_NOBASE( CViewRender );
public:
	virtual void	Init( void );
	virtual void	Shutdown( void );

	const CViewSetup *GetPlayerViewSetup( ) const;

	virtual void	StartPitchDrift( void );
	virtual void	StopPitchDrift( void );

	virtual float	GetZNear();
	virtual float	GetZFar();

	virtual void	OnRenderStart();
	void			DriftPitch (void);

	static CViewRender *	GetMainView() { return assert_cast<CViewRender *>( view ); }

	void			AddViewToScene( CRendering3dView *pView ) { m_SimpleExecutor.AddView( pView ); }
protected:
	// Sets up the view parameters for all views (left, middle and right eyes).
    void            SetUpViews();

	// Sets up the view parameters of map overview mode (cl_leveloverview)
	void			SetUpOverView();

	// generates a low-res screenshot for save games
	virtual void	WriteSaveGameScreenshotOfSize( const char *pFilename, int width, int height, bool bCreatePowerOf2Padded = false, bool bWriteVTF = false );
	void			WriteSaveGameScreenshot( const char *filename );

	virtual IReplayScreenshotSystem *GetReplayScreenshotSystem() { return this; }

	// IReplayScreenshot implementation
	virtual void	WriteReplayScreenshot( WriteReplayScreenshotParams_t &params );
	virtual void	UpdateReplayScreenshotCache();

    StereoEye_t		GetFirstEye() const;
    StereoEye_t		GetLastEye() const;
    CViewSetup &    GetView(StereoEye_t eEye);
    const CViewSetup &    GetView(StereoEye_t eEye) const ;


	// This stores all of the view setup parameters that the engine needs to know about.
    // Best way to pick the right one is with ::GetView(), rather than directly.
	CViewSetup		m_View;         // mono <- in stereo mode, this will be between the two eyes and is the "main" view.
	CViewSetup		m_ViewLeft;     // left (unused for mono)
	CViewSetup		m_ViewRight;    // right (unused for mono)

	// Pitch drifting data
	CPitchDrift		m_PitchDrift;

public:
					CViewRender();
	virtual			~CViewRender( void ) {}

// Implementation of IViewRender interface
public:

	void			SetupVis( const CViewSetup& view, unsigned int &visFlags, ViewCustomVisibility_t *pCustomVisibility = NULL );


	// Render functions
	virtual	void	Render( vrect_t *rect );
	virtual void	RenderView( const CViewSetup &view, int nClearFlags, int whatToDraw );
	virtual void	RenderPlayerSprites();
	virtual void	Render2DEffectsPreHUD( const CViewSetup &view );
	virtual void	Render2DEffectsPostHUD( const CViewSetup &view );


	void			DisableFog( void );

	// Called once per level change
	void			LevelInit( void );
	void			LevelShutdown( void );

	// Add entity to transparent entity queue

	bool			ShouldDrawEntities( void );
	bool			ShouldDrawBrushModels( void );

	const CViewSetup *GetViewSetup( ) const;
	
	void			DisableVis( void );

	// Sets up the view model position relative to the local player
	void			MoveViewModels( );

	// Gets the abs origin + angles of the view models
	void			GetViewModelPosition( int nIndex, Vector *pPos, QAngle *pAngle );

	void			SetCheapWaterStartDistance( float flCheapWaterStartDistance );
	void			SetCheapWaterEndDistance( float flCheapWaterEndDistance );

	void			GetWaterLODParams( float &flCheapWaterStartDistance, float &flCheapWaterEndDistance );

	virtual void	QueueOverlayRenderView( const CViewSetup &view, int nClearFlags, int whatToDraw );

	virtual void	GetScreenFadeDistances( float *min, float *max );

	virtual C_BaseEntity *GetCurrentlyDrawingEntity();
	virtual void		  SetCurrentlyDrawingEntity( C_BaseEntity *pEnt );

	virtual bool		UpdateShadowDepthTexture( ITexture *pRenderTarget, ITexture *pDepthTexture, const CViewSetup &shadowView );

	int GetBaseDrawFlags() { return m_BaseDrawFlags; }
	virtual bool ShouldForceNoVis()  { return m_bForceNoVis; }
	int				BuildRenderablesListsNumber() const { return m_BuildRenderableListsNumber; }
	int				IncRenderablesListsNumber()  { return ++m_BuildRenderableListsNumber; }

	int				BuildWorldListsNumber() const;
	int				IncWorldListsNumber() { return ++m_BuildWorldListsNumber; }

	virtual VPlane*	GetFrustum() { return ( m_pActiveRenderer ) ? m_pActiveRenderer->GetFrustum() : m_Frustum; }

	// What are we currently rendering? Returns a combination of DF_ flags.
	virtual int		GetDrawFlags() { return ( m_pActiveRenderer ) ? m_pActiveRenderer->GetDrawFlags() : 0; }

	CBase3dView *	GetActiveRenderer() { return m_pActiveRenderer; }
	CBase3dView *	SetActiveRenderer( CBase3dView *pActiveRenderer ) { CBase3dView *pPrevious = m_pActiveRenderer; m_pActiveRenderer =  pActiveRenderer; return pPrevious; }

	void			FreezeFrame( float flFreezeTime );

	void SetWaterOverlayMaterial( IMaterial *pMaterial )
	{
		m_UnderWaterOverlayMaterial.Init( pMaterial );
	}
private:
	int				m_BuildWorldListsNumber;


	// General draw methods
	// baseDrawFlags is a combination of DF_ defines. DF_MONITOR is passed into here while drawing a monitor.
	void			ViewDrawScene( bool bDrew3dSkybox, SkyboxVisibility_t nSkyboxVisible, const CViewSetup &view, int nClearFlags, view_id_t viewID, bool bDrawViewModel = false, int baseDrawFlags = 0, ViewCustomVisibility_t *pCustomVisibility = NULL );

	void			DrawMonitors( const CViewSetup &cameraView );

	bool			DrawOneMonitor( ITexture *pRenderTarget, int cameraNum, C_PointCamera *pCameraEnt, const CViewSetup &cameraView, C_BasePlayer *localPlayer, 
						int x, int y, int width, int height );

	// Drawing primitives
	bool			ShouldDrawViewModel( bool drawViewmodel );
	void			DrawViewModels( const CViewSetup &view, bool drawViewmodel );

	void			PerformScreenSpaceEffects( int x, int y, int w, int h );

	// Overlays
	void			SetScreenOverlayMaterial( IMaterial *pMaterial );
	IMaterial		*GetScreenOverlayMaterial( );
	void			PerformScreenOverlay( int x, int y, int w, int h );

	void DrawUnderwaterOverlay( void );

	// Water-related methods
	void			DrawWorldAndEntities( bool drawSkybox, const CViewSetup &view, int nClearFlags, ViewCustomVisibility_t *pCustomVisibility = NULL );

	virtual void			ViewDrawScene_Intro( const CViewSetup &view, int nClearFlags, const IntroData_t &introData );

#ifdef PORTAL 
	// Intended for use in the middle of another ViewDrawScene call, this allows stencils to be drawn after opaques but before translucents are drawn in the main view.
	void			ViewDrawScene_PortalStencil( const CViewSetup &view, ViewCustomVisibility_t *pCustomVisibility );
	void			Draw3dSkyboxworld_Portal( const CViewSetup &view, int &nClearFlags, bool &bDrew3dSkybox, SkyboxVisibility_t &nSkyboxVisible, ITexture *pRenderTarget = NULL );
#endif // PORTAL

	// Determines what kind of water we're going to use
	void			DetermineWaterRenderInfo( const VisibleFogVolumeInfo_t &fogVolumeInfo, WaterRenderInfo_t &info );

	bool			UpdateRefractIfNeededByList( CUtlVector< IClientRenderable * > &list );
	void			DrawRenderablesInList( CUtlVector< IClientRenderable * > &list, int flags = 0 );

	// Sets up, cleans up the main 3D view
	void			SetupMain3DView( const CViewSetup &view, int &nClearFlags );
	void			CleanupMain3DView( const CViewSetup &view );


	// This stores the current view
 	CViewSetup		m_CurrentView;

	// VIS Overrides
	// Set to true to turn off client side vis ( !!!! rendering will be slow since everything will draw )
	bool			m_bForceNoVis;	

	// Some cvars needed by this system
	const ConVar	*m_pDrawEntities;
	const ConVar	*m_pDrawBrushModels;

	// Some materials used...
	CMaterialReference	m_TranslucentSingleColor;
	CMaterialReference	m_ModulateSingleColor;
	CMaterialReference	m_ScreenOverlayMaterial;
	CMaterialReference m_UnderWaterOverlayMaterial;

	CMaterialReference	m_ScriptOverlayMaterial;
	char m_szCurrentScriptMaterialName[ MAX_PATH ];

	Vector			m_vecLastFacing;
	float			m_flCheapWaterStartDistance;
	float			m_flCheapWaterEndDistance;

	CViewSetup			m_OverlayViewSetup;
	int					m_OverlayClearFlags;
	int					m_OverlayDrawFlags;
	bool				m_bDrawOverlay;

	int					m_BaseDrawFlags;	// Set in ViewDrawScene and OR'd into m_DrawFlags as it goes.
	C_BaseEntity		*m_pCurrentlyDrawingEntity;

#if defined( CSTRIKE_DLL )
	float				m_flLastFOV;
#endif

#ifdef PORTAL
	friend class CPortalRender; //portal drawing needs muck with views in weird ways
	friend class CPortalRenderable;
#endif
	int				m_BuildRenderableListsNumber;

	friend class CBase3dView;

	Frustum m_Frustum;

	CBase3dView *m_pActiveRenderer;
	CSimpleRenderExecutor m_SimpleExecutor;

	bool			m_rbTakeFreezeFrame[ STEREO_EYE_MAX ];
	float			m_flFreezeFrameUntil;

#if defined( REPLAY_ENABLED )
	CReplayScreenshotTaker	*m_pReplayScreenshotTaker;
#endif
};

#endif // VIEWRENDER_H
