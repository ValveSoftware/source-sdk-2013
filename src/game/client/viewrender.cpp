//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Responsible for drawing the scene
//
//===========================================================================//

#include "cbase.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "ivieweffects.h"
#include "iinput.h"
#include "model_types.h"
#include "clientsideeffects.h"
#include "particlemgr.h"
#include "viewrender.h"
#include "iclientmode.h"
#include "voice_status.h"
#include "glow_overlay.h"
#include "materialsystem/imesh.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterialsystem.h"
#include "detailobjectsystem.h"
#include "tier0/vprof.h"
#include "tier1/mempool.h"
#include "vstdlib/jobthread.h"
#include "datacache/imdlcache.h"
#include "engine/IEngineTrace.h"
#include "engine/ivmodelinfo.h"
#include "tier0/icommandline.h"
#include "view_scene.h"
#include "particles_ez.h"
#include "engine/IStaticPropMgr.h"
#include "engine/ivdebugoverlay.h"
#include "c_pixel_visibility.h"
#include "clienteffectprecachesystem.h"
#include "c_rope.h"
#include "c_effects.h"
#include "smoke_fog_overlay.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "vgui_int.h"
#include "ienginevgui.h"
#include "ScreenSpaceEffects.h"
#include "toolframework_client.h"
#include "c_func_reflective_glass.h"
#include "KeyValues.h"
#include "renderparm.h"
#include "studio_stats.h"
#include "con_nprint.h"
#include "clientmode_shared.h"
#include "sourcevr/isourcevirtualreality.h"
#include "client_virtualreality.h"
#ifdef TF_CLIENT_DLL
#include "tf/c_tf_player.h"
#endif

#ifdef PORTAL
//#include "C_Portal_Player.h"
#include "portal_render_targets.h"
#include "PortalRender.h"
#endif
#if defined( HL2_CLIENT_DLL ) || defined( CSTRIKE_DLL ) || defined( TF_CLIENT_DLL )
#define USE_MONITORS
#endif
#include "rendertexture.h"
#include "viewpostprocess.h"
#include "viewdebug.h"

#if defined USES_ECON_ITEMS
#include "econ_wearable.h"
#endif

#ifdef USE_MONITORS
#include "c_point_camera.h"
#endif // USE_MONITORS

// Projective textures
#include "C_Env_Projected_Texture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static void testfreezeframe_f( void )
{
	view->FreezeFrame( 3.0 );
}
static ConCommand test_freezeframe( "test_freezeframe", testfreezeframe_f, "Test the freeze frame code.", FCVAR_CHEAT );

//-----------------------------------------------------------------------------

static ConVar r_visocclusion( "r_visocclusion", "0", FCVAR_CHEAT );
extern ConVar r_flashlightdepthtexture;
extern ConVar vcollide_wireframe;
extern ConVar r_depthoverlay;
extern ConVar mat_viewportscale;
extern ConVar mat_viewportupscale;
extern bool g_bDumpRenderTargets;

//-----------------------------------------------------------------------------
// Convars related to controlling rendering
//-----------------------------------------------------------------------------
static ConVar cl_maxrenderable_dist("cl_maxrenderable_dist", "3000", FCVAR_CHEAT, "Max distance from the camera at which things will be rendered" );

ConVar r_entityclips( "r_entityclips", "1" ); //FIXME: Nvidia drivers before 81.94 on cards that support user clip planes will have problems with this, require driver update? Detect and disable?

// Matches the version in the engine
static ConVar r_drawopaqueworld( "r_drawopaqueworld", "1", FCVAR_CHEAT );
static ConVar r_drawtranslucentworld( "r_drawtranslucentworld", "1", FCVAR_CHEAT );
static ConVar r_3dsky( "r_3dsky","1", 0, "Enable the rendering of 3d sky boxes" );
static ConVar r_skybox( "r_skybox","1", FCVAR_CHEAT, "Enable the rendering of sky boxes" );
#ifdef TF_CLIENT_DLL
ConVar r_drawviewmodel( "r_drawviewmodel","1", FCVAR_DONTRECORD );
#else
ConVar r_drawviewmodel( "r_drawviewmodel","1", FCVAR_CHEAT );
#endif
static ConVar r_drawtranslucentrenderables( "r_drawtranslucentrenderables", "1", FCVAR_CHEAT );
static ConVar r_drawopaquerenderables( "r_drawopaquerenderables", "1", FCVAR_CHEAT );
static ConVar r_threaded_renderables( "r_threaded_renderables", "0" );

// FIXME: This is not static because we needed to turn it off for TF2 playtests
ConVar r_DrawDetailProps( "r_DrawDetailProps", "1", FCVAR_NONE, "0=Off, 1=Normal, 2=Wireframe" );

ConVar r_worldlistcache( "r_worldlistcache", "1" );

//-----------------------------------------------------------------------------
// Convars related to fog color
//-----------------------------------------------------------------------------
static ConVar fog_override( "fog_override", "0", FCVAR_CHEAT );
// set any of these to use the maps fog
static ConVar fog_start( "fog_start", "-1", FCVAR_CHEAT );
static ConVar fog_end( "fog_end", "-1", FCVAR_CHEAT );
static ConVar fog_color( "fog_color", "-1 -1 -1", FCVAR_CHEAT );
static ConVar fog_enable( "fog_enable", "-1", FCVAR_CHEAT );
static ConVar fog_startskybox( "fog_startskybox", "-1", FCVAR_CHEAT );
static ConVar fog_endskybox( "fog_endskybox", "-1", FCVAR_CHEAT );
static ConVar fog_maxdensityskybox( "fog_maxdensityskybox", "-1", FCVAR_CHEAT );
static ConVar fog_colorskybox( "fog_colorskybox", "-1 -1 -1", FCVAR_CHEAT );
static ConVar fog_enableskybox( "fog_enableskybox", "-1", FCVAR_CHEAT );
static ConVar fog_maxdensity( "fog_maxdensity", "-1", FCVAR_CHEAT );
static ConVar fog_radial( "fog_radial", "-1", FCVAR_CHEAT );
static ConVar fog_radialskybox( "fog_radialskybox", "-1", FCVAR_CHEAT );


//-----------------------------------------------------------------------------
// Water-related convars
//-----------------------------------------------------------------------------
static ConVar r_debugcheapwater( "r_debugcheapwater", "0", FCVAR_CHEAT );
#ifndef _X360
static ConVar r_waterforceexpensive( "r_waterforceexpensive", "0", FCVAR_ARCHIVE );
#endif
static ConVar r_waterforcereflectentities( "r_waterforcereflectentities", "0", FCVAR_ALLOWED_IN_COMPETITIVE );
static ConVar r_WaterDrawRefraction( "r_WaterDrawRefraction", "1", 0, "Enable water refraction" );
static ConVar r_WaterDrawReflection( "r_WaterDrawReflection", "1", 0, "Enable water reflection" );
static ConVar r_ForceWaterLeaf( "r_ForceWaterLeaf", "1", 0, "Enable for optimization to water - considers view in leaf under water for purposes of culling" );
static ConVar mat_drawwater( "mat_drawwater", "1", FCVAR_CHEAT );
static ConVar mat_clipz( "mat_clipz", "1" );


//-----------------------------------------------------------------------------
// Other convars
//-----------------------------------------------------------------------------
static ConVar r_screenfademinsize( "r_screenfademinsize", "0" );
static ConVar r_screenfademaxsize( "r_screenfademaxsize", "0" );
static ConVar cl_drawmonitors( "cl_drawmonitors", "1" );
static ConVar r_eyewaterepsilon( "r_eyewaterepsilon", "10.0f", FCVAR_CHEAT );

#ifdef TF_CLIENT_DLL
static ConVar pyro_dof( "pyro_dof", "1", FCVAR_ARCHIVE );
#endif

extern ConVar cl_leveloverview;

extern ConVar localplayer_visionflags;

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
static Vector g_vecCurrentRenderOrigin(0,0,0);
static QAngle g_vecCurrentRenderAngles(0,0,0);
static Vector g_vecCurrentVForward(0,0,0), g_vecCurrentVRight(0,0,0), g_vecCurrentVUp(0,0,0);
static VMatrix g_matCurrentCamInverse;
bool s_bCanAccessCurrentView = false;
IntroData_t *g_pIntroData = NULL;
static bool	g_bRenderingView = false;			// For debugging...
static int g_CurrentViewID = VIEW_NONE;
bool g_bRenderingScreenshot = false;


#define FREEZECAM_SNAPSHOT_FADE_SPEED 340
float g_flFreezeFlash = 0.0f;

//-----------------------------------------------------------------------------

CON_COMMAND( r_cheapwaterstart,  "" )
{
	if( args.ArgC() == 2 )
	{
		float dist = atof( args[ 1 ] );
		view->SetCheapWaterStartDistance( dist );
	}
	else
	{
		float start, end;
		view->GetWaterLODParams( start, end );
		Warning( "r_cheapwaterstart: %f\n", start );
	}
}

CON_COMMAND( r_cheapwaterend,  "" )
{
	if( args.ArgC() == 2 )
	{
		float dist = atof( args[ 1 ] );
		view->SetCheapWaterEndDistance( dist );
	}
	else
	{
		float start, end;
		view->GetWaterLODParams( start, end );
		Warning( "r_cheapwaterend: %f\n", end );
	}
}



//-----------------------------------------------------------------------------
// Describes a pruned set of leaves to be rendered this view. Reference counted
// because potentially shared by a number of views
//-----------------------------------------------------------------------------
struct ClientWorldListInfo_t : public CRefCounted1<WorldListInfo_t>
{
	ClientWorldListInfo_t() 
	{ 
		memset( (WorldListInfo_t *)this, 0, sizeof(WorldListInfo_t) ); 
		m_pActualLeafIndex = NULL;
		m_bPooledAlloc = false;
	}

	// Allocate a list intended for pruning
	static ClientWorldListInfo_t *AllocPooled( const ClientWorldListInfo_t &exemplar );

	// Because we remap leaves to eliminate unused leaves, we need a remap
	// when drawing translucent surfaces, which requires the *original* leaf index
	// using m_pActualLeafMap[ remapped leaf index ] == actual leaf index
	LeafIndex_t *m_pActualLeafIndex;

private:
	virtual bool OnFinalRelease();

	bool m_bPooledAlloc;
	static CObjectPool<ClientWorldListInfo_t> gm_Pool;
};


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

class CWorldListCache
{
public:
	CWorldListCache()
	{

	}
	void Flush()
	{
		for ( int i = m_Entries.FirstInorder(); i != m_Entries.InvalidIndex(); i = m_Entries.NextInorder( i ) )
		{
			delete m_Entries[i];
		}
		m_Entries.RemoveAll();
	}

	bool Find( const CViewSetup &viewSetup, IWorldRenderList **ppList, ClientWorldListInfo_t **ppListInfo )
	{
		Entry_t lookup( viewSetup );

		int i = m_Entries.Find( &lookup );

		if ( i != m_Entries.InvalidIndex() )
		{
			Entry_t *pEntry = m_Entries[i];
			*ppList = InlineAddRef( pEntry->pList );
			*ppListInfo = InlineAddRef( pEntry->pListInfo );
			return true;
		}

		return false;
	}

	void Add( const CViewSetup &viewSetup, IWorldRenderList *pList, ClientWorldListInfo_t *pListInfo )
	{
		m_Entries.Insert( new Entry_t( viewSetup, pList, pListInfo ) );
	}

private:
	struct Entry_t
	{
		Entry_t( const CViewSetup &viewSetup, IWorldRenderList *pList = NULL, ClientWorldListInfo_t *pListInfo = NULL ) :
			pList( ( pList ) ? InlineAddRef( pList ) : NULL ),
			pListInfo( ( pListInfo ) ? InlineAddRef( pListInfo ) : NULL )
		{
            // @NOTE (toml 8/18/2006): because doing memcmp, need to fill all of the fields and the padding!
			memset( &m_bOrtho, 0, offsetof(Entry_t, pList ) - offsetof(Entry_t, m_bOrtho ) );
			m_bOrtho = viewSetup.m_bOrtho;			
			m_OrthoLeft = viewSetup.m_OrthoLeft;		
			m_OrthoTop = viewSetup.m_OrthoTop;
			m_OrthoRight = viewSetup.m_OrthoRight;
			m_OrthoBottom = viewSetup.m_OrthoBottom;
			fov = viewSetup.fov;				
			origin = viewSetup.origin;					
			angles = viewSetup.angles;				
			zNear = viewSetup.zNear;			
			zFar = viewSetup.zFar;			
			m_flAspectRatio = viewSetup.m_flAspectRatio;
			m_bOffCenter = viewSetup.m_bOffCenter;
			m_flOffCenterTop = viewSetup.m_flOffCenterTop;
			m_flOffCenterBottom = viewSetup.m_flOffCenterBottom;
			m_flOffCenterLeft = viewSetup.m_flOffCenterLeft;
			m_flOffCenterRight = viewSetup.m_flOffCenterRight;
		}

		~Entry_t()
		{
			if ( pList )
				pList->Release();
			if ( pListInfo )
				pListInfo->Release();
		}

		// The fields from CViewSetup that would actually affect the list
		float	m_OrthoLeft;		
		float	m_OrthoTop;
		float	m_OrthoRight;
		float	m_OrthoBottom;
		float	fov;				
		Vector	origin;					
		QAngle	angles;				
		float	zNear;			
		float	zFar;			
		float	m_flAspectRatio;
		float	m_flOffCenterTop;
		float	m_flOffCenterBottom;
		float	m_flOffCenterLeft;
		float	m_flOffCenterRight;
		bool	m_bOrtho;			
		bool	m_bOffCenter;

		IWorldRenderList *pList;
		ClientWorldListInfo_t *pListInfo;
	};

	class CEntryComparator
	{
	public:
		CEntryComparator( int ) {}
		bool operator!() const { return false; }
		bool operator()( const Entry_t *lhs, const Entry_t *rhs ) const 
		{ 
			return ( memcmp( lhs, rhs, sizeof(Entry_t) - ( sizeof(Entry_t) - offsetof(Entry_t, pList ) ) ) < 0 );
		}
	};

	CUtlRBTree<Entry_t *, unsigned short, CEntryComparator> m_Entries;
};

CWorldListCache g_WorldListCache;

//-----------------------------------------------------------------------------
// Standard 3d skybox view
//-----------------------------------------------------------------------------
class CSkyboxView : public CRendering3dView
{
	DECLARE_CLASS( CSkyboxView, CRendering3dView );
public:
	CSkyboxView(CViewRender *pMainView) : 
		CRendering3dView( pMainView ),
		m_pSky3dParams( NULL )
	  {
	  }

	bool			Setup( const CViewSetup &view, int *pClearFlags, SkyboxVisibility_t *pSkyboxVisible );
	void			Draw();

protected:

#ifdef PORTAL
	virtual bool ShouldDrawPortals() { return false; }
#endif

	virtual SkyboxVisibility_t	ComputeSkyboxVisibility();

	bool			GetSkyboxFogEnable();

	void			Enable3dSkyboxFog( void );
	void			DrawInternal( view_id_t iSkyBoxViewID, bool bInvokePreAndPostRender, ITexture *pRenderTarget, ITexture *pDepthTarget );

	sky3dparams_t *	PreRender3dSkyboxWorld( SkyboxVisibility_t nSkyboxVisible );

	sky3dparams_t *m_pSky3dParams;
};

//-----------------------------------------------------------------------------
// 3d skybox view when drawing portals
//-----------------------------------------------------------------------------
#ifdef PORTAL
class CPortalSkyboxView : public CSkyboxView
{
	DECLARE_CLASS( CPortalSkyboxView, CSkyboxView );
public:
	CPortalSkyboxView(CViewRender *pMainView) : 
	  CSkyboxView( pMainView ),
		  m_pRenderTarget( NULL )
	  {}

	  bool			Setup( const CViewSetup &view, int *pClearFlags, SkyboxVisibility_t *pSkyboxVisible, ITexture *pRenderTarget = NULL );

	  //Skybox drawing through portals with workarounds to fix area bits, position/scaling, view id's..........
	  void			Draw();

private:
	virtual SkyboxVisibility_t	ComputeSkyboxVisibility();

	ITexture *m_pRenderTarget;
};
#endif


//-----------------------------------------------------------------------------
// Shadow depth texture
//-----------------------------------------------------------------------------
class CShadowDepthView : public CRendering3dView
{
	DECLARE_CLASS( CShadowDepthView, CRendering3dView );
public:
	CShadowDepthView(CViewRender *pMainView) : CRendering3dView( pMainView ) {}

	void Setup( const CViewSetup &shadowViewIn, ITexture *pRenderTarget, ITexture *pDepthTexture );
	void Draw();

private:
	ITexture *m_pRenderTarget;
	ITexture *m_pDepthTexture;
};

//-----------------------------------------------------------------------------
// Freeze frame. Redraws the frame at which it was enabled.
//-----------------------------------------------------------------------------
class CFreezeFrameView : public CRendering3dView
{
	DECLARE_CLASS( CFreezeFrameView, CRendering3dView );
public:
	CFreezeFrameView(CViewRender *pMainView) : CRendering3dView( pMainView ) {}

	void Setup( const CViewSetup &view );
	void Draw();

private:
	CMaterialReference m_pFreezeFrame;
	CMaterialReference m_TranslucentSingleColor;
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CBaseWorldView : public CRendering3dView
{
	DECLARE_CLASS( CBaseWorldView, CRendering3dView );
protected:
	CBaseWorldView(CViewRender *pMainView) : CRendering3dView( pMainView ) {}

	virtual bool	AdjustView( float waterHeight );

	void			DrawSetup( float waterHeight, int flags, float waterZAdjust, int iForceViewLeaf = -1 );
	void			DrawExecute( float waterHeight, view_id_t viewID, float waterZAdjust );

	virtual void	PushView( float waterHeight );
	virtual void	PopView();

	void			SSAO_DepthPass();
	void			DrawDepthOfField();
};


//-----------------------------------------------------------------------------
// Draws the scene when there's no water or only cheap water
//-----------------------------------------------------------------------------
class CSimpleWorldView : public CBaseWorldView
{
	DECLARE_CLASS( CSimpleWorldView, CBaseWorldView );
public:
	CSimpleWorldView(CViewRender *pMainView) : CBaseWorldView( pMainView ) {}

	void			Setup( const CViewSetup &view, int nClearFlags, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& info, ViewCustomVisibility_t *pCustomVisibility = NULL );
	void			Draw();

private: 
	VisibleFogVolumeInfo_t m_fogInfo;

};


//-----------------------------------------------------------------------------
// Base class for scenes with water
//-----------------------------------------------------------------------------
class CBaseWaterView : public CBaseWorldView
{
	DECLARE_CLASS( CBaseWaterView, CBaseWorldView );
public:
	CBaseWaterView(CViewRender *pMainView) : 
		CBaseWorldView( pMainView ),
		m_SoftwareIntersectionView( pMainView )
	{}

	//	void Setup( const CViewSetup &, const WaterRenderInfo_t& info );

protected:
	void			CalcWaterEyeAdjustments( const VisibleFogVolumeInfo_t &fogInfo, float &newWaterHeight, float &waterZAdjust, bool bSoftwareUserClipPlane );

	class CSoftwareIntersectionView : public CBaseWorldView
	{
		DECLARE_CLASS( CSoftwareIntersectionView, CBaseWorldView );
	public:
		CSoftwareIntersectionView(CViewRender *pMainView) : CBaseWorldView( pMainView ) {}

		void Setup( bool bAboveWater );
		void Draw();

	private: 
		CBaseWaterView *GetOuter() { return GET_OUTER( CBaseWaterView, m_SoftwareIntersectionView ); }
	};

	friend class CSoftwareIntersectionView;

	CSoftwareIntersectionView m_SoftwareIntersectionView;

	WaterRenderInfo_t m_waterInfo;
	float m_waterHeight;
	float m_waterZAdjust;
	bool m_bSoftwareUserClipPlane;
	VisibleFogVolumeInfo_t m_fogInfo;
};


//-----------------------------------------------------------------------------
// Scenes above water
//-----------------------------------------------------------------------------
class CAboveWaterView : public CBaseWaterView
{
	DECLARE_CLASS( CAboveWaterView, CBaseWaterView );
public:
	CAboveWaterView(CViewRender *pMainView) : 
		CBaseWaterView( pMainView ),
		m_ReflectionView( pMainView ),
		m_RefractionView( pMainView ),
		m_IntersectionView( pMainView )
	{}

	void Setup(  const CViewSetup &view, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& waterInfo );
	void			Draw();

	class CReflectionView : public CBaseWorldView
	{
		DECLARE_CLASS( CReflectionView, CBaseWorldView );
	public:
		CReflectionView(CViewRender *pMainView) : CBaseWorldView( pMainView ) {}

		void Setup( bool bReflectEntities );
		void Draw();

	private:
		CAboveWaterView *GetOuter() { return GET_OUTER( CAboveWaterView, m_ReflectionView ); }
	};

	class CRefractionView : public CBaseWorldView
	{
		DECLARE_CLASS( CRefractionView, CBaseWorldView );
	public:
		CRefractionView(CViewRender *pMainView) : CBaseWorldView( pMainView ) {}

		void Setup();
		void Draw();

	private:
		CAboveWaterView *GetOuter() { return GET_OUTER( CAboveWaterView, m_RefractionView ); }
	};

	class CIntersectionView : public CBaseWorldView
	{
		DECLARE_CLASS( CIntersectionView, CBaseWorldView );
	public:
		CIntersectionView(CViewRender *pMainView) : CBaseWorldView( pMainView ) {}

		void Setup();
		void Draw();

	private:
		CAboveWaterView *GetOuter() { return GET_OUTER( CAboveWaterView, m_IntersectionView ); }
	};


	friend class CRefractionView;
	friend class CReflectionView;
	friend class CIntersectionView;

	bool m_bViewIntersectsWater;

	CReflectionView m_ReflectionView;
	CRefractionView m_RefractionView;
	CIntersectionView m_IntersectionView;
};


//-----------------------------------------------------------------------------
// Scenes below water
//-----------------------------------------------------------------------------
class CUnderWaterView : public CBaseWaterView
{
	DECLARE_CLASS( CUnderWaterView, CBaseWaterView );
public:
	CUnderWaterView(CViewRender *pMainView) : 
		CBaseWaterView( pMainView ),
		m_RefractionView( pMainView )
	{}

	void			Setup( const CViewSetup &view, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& info );
	void			Draw();

	class CRefractionView : public CBaseWorldView
	{
		DECLARE_CLASS( CRefractionView, CBaseWorldView );
	public:
		CRefractionView(CViewRender *pMainView) : CBaseWorldView( pMainView ) {}

		void Setup();
		void Draw();

	private:
		CUnderWaterView *GetOuter() { return GET_OUTER( CUnderWaterView, m_RefractionView ); }
	};

	friend class CRefractionView;

	bool m_bDrawSkybox; // @MULTICORE (toml 8/17/2006): remove after setup hoisted

	CRefractionView m_RefractionView;
};


//-----------------------------------------------------------------------------
// Scenes containing reflective glass
//-----------------------------------------------------------------------------
class CReflectiveGlassView : public CSimpleWorldView
{
	DECLARE_CLASS( CReflectiveGlassView, CSimpleWorldView );
public:
	CReflectiveGlassView( CViewRender *pMainView ) : BaseClass( pMainView )
	{
	}

	virtual bool AdjustView( float flWaterHeight );
	virtual void PushView( float waterHeight );
	virtual void PopView( );
	void Setup( const CViewSetup &view, int nClearFlags, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t &waterInfo, const cplane_t &reflectionPlane );
	void Draw();

	cplane_t m_ReflectionPlane;
};

class CRefractiveGlassView : public CSimpleWorldView
{
	DECLARE_CLASS( CRefractiveGlassView, CSimpleWorldView );
public:
	CRefractiveGlassView( CViewRender *pMainView ) : BaseClass( pMainView )
	{
	}

	virtual bool AdjustView( float flWaterHeight );
	virtual void PushView( float waterHeight );
	virtual void PopView( );
	void Setup( const CViewSetup &view, int nClearFlags, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t &waterInfo, const cplane_t &reflectionPlane );
	void Draw();

	cplane_t m_ReflectionPlane;
};


//-----------------------------------------------------------------------------
// Computes draw flags for the engine to build its world surface lists
//-----------------------------------------------------------------------------
static inline unsigned long BuildEngineDrawWorldListFlags( unsigned nDrawFlags )
{
	unsigned long nEngineFlags = 0;

	if ( nDrawFlags & DF_DRAWSKYBOX )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_SKYBOX;
	}

	if ( nDrawFlags & DF_RENDER_ABOVEWATER )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_STRICTLYABOVEWATER;
		nEngineFlags |= DRAWWORLDLISTS_DRAW_INTERSECTSWATER;
	}

	if ( nDrawFlags & DF_RENDER_UNDERWATER )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_STRICTLYUNDERWATER;
		nEngineFlags |= DRAWWORLDLISTS_DRAW_INTERSECTSWATER;
	}

	if ( nDrawFlags & DF_RENDER_WATER )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_WATERSURFACE;
	}

	if( nDrawFlags & DF_CLIP_SKYBOX )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_CLIPSKYBOX;
	}

	if( nDrawFlags & DF_SHADOW_DEPTH_MAP )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_SHADOWDEPTH;
	}

	if( nDrawFlags & DF_RENDER_REFRACTION )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_REFRACTION;
	}

	if( nDrawFlags & DF_RENDER_REFLECTION )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_REFLECTION;
	}

	if( nDrawFlags & DF_SSAO_DEPTH_PASS )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_SSAO | DRAWWORLDLISTS_DRAW_STRICTLYUNDERWATER | DRAWWORLDLISTS_DRAW_INTERSECTSWATER | DRAWWORLDLISTS_DRAW_STRICTLYABOVEWATER ;
		nEngineFlags &= ~( DRAWWORLDLISTS_DRAW_WATERSURFACE | DRAWWORLDLISTS_DRAW_REFRACTION | DRAWWORLDLISTS_DRAW_REFLECTION );
	}

	return nEngineFlags;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static void SetClearColorToFogColor()
{
	unsigned char ucFogColor[3];
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->GetFogColor( ucFogColor );
	if( g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER )
	{
		// @MULTICORE (toml 8/16/2006): Find a way to not do this twice in eye above water case
		float scale = LinearToGammaFullRange( pRenderContext->GetToneMappingScaleLinear().x );
		ucFogColor[0] *= scale;
		ucFogColor[1] *= scale;
		ucFogColor[2] *= scale;
	}
	pRenderContext->ClearColor4ub( ucFogColor[0], ucFogColor[1], ucFogColor[2], 255 );
}

//-----------------------------------------------------------------------------
// Precache of necessary materials
//-----------------------------------------------------------------------------

#ifdef HL2_CLIENT_DLL
CLIENTEFFECT_REGISTER_BEGIN( PrecacheViewRender )
	CLIENTEFFECT_MATERIAL( "scripted/intro_screenspaceeffect" )
CLIENTEFFECT_REGISTER_END()
#endif

CLIENTEFFECT_REGISTER_BEGIN( PrecachePostProcessingEffects )
	CLIENTEFFECT_MATERIAL( "dev/blurfiltery_and_add_nohdr" )
	CLIENTEFFECT_MATERIAL( "dev/blurfilterx" )
	CLIENTEFFECT_MATERIAL( "dev/blurfilterx_nohdr" )
	CLIENTEFFECT_MATERIAL( "dev/blurfiltery" )
	CLIENTEFFECT_MATERIAL( "dev/blurfiltery_nohdr" )
	CLIENTEFFECT_MATERIAL( "dev/bloomadd" )
	CLIENTEFFECT_MATERIAL( "dev/downsample" )
	#ifdef CSTRIKE_DLL
		CLIENTEFFECT_MATERIAL( "dev/downsample_non_hdr_cstrike" )
	#else
		CLIENTEFFECT_MATERIAL( "dev/downsample_non_hdr" )
	#endif
	CLIENTEFFECT_MATERIAL( "dev/no_pixel_write" )
	CLIENTEFFECT_MATERIAL( "dev/lumcompare" )
	CLIENTEFFECT_MATERIAL( "dev/floattoscreen_combine" )
	CLIENTEFFECT_MATERIAL( "dev/copyfullframefb_vanilla" )
	CLIENTEFFECT_MATERIAL( "dev/copyfullframefb" )
	CLIENTEFFECT_MATERIAL( "dev/engine_post" )
	CLIENTEFFECT_MATERIAL( "dev/motion_blur" )
	CLIENTEFFECT_MATERIAL( "dev/upscale" )

#ifdef TF_CLIENT_DLL
	CLIENTEFFECT_MATERIAL( "dev/pyro_blur_filter_y" )
	CLIENTEFFECT_MATERIAL( "dev/pyro_blur_filter_x" )
	CLIENTEFFECT_MATERIAL( "dev/pyro_dof" )
	CLIENTEFFECT_MATERIAL( "dev/pyro_vignette_border" )
	CLIENTEFFECT_MATERIAL( "dev/pyro_vignette" )
	CLIENTEFFECT_MATERIAL( "dev/pyro_post" )
#endif

CLIENTEFFECT_REGISTER_END_CONDITIONAL( engine->GetDXSupportLevel() >= 90 )

//-----------------------------------------------------------------------------
// Accessors to return the current view being rendered
//-----------------------------------------------------------------------------
const Vector &CurrentViewOrigin()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentRenderOrigin;
}

const QAngle &CurrentViewAngles()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentRenderAngles;
}

const Vector &CurrentViewForward()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentVForward;
}

const Vector &CurrentViewRight()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentVRight;
}

const Vector &CurrentViewUp()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentVUp;
}

const VMatrix &CurrentWorldToViewMatrix()
{
	Assert( s_bCanAccessCurrentView );
	return g_matCurrentCamInverse;
}


//-----------------------------------------------------------------------------
// Methods to set the current view/guard access to view parameters
//-----------------------------------------------------------------------------
void AllowCurrentViewAccess( bool allow )
{
	s_bCanAccessCurrentView = allow;
}

bool IsCurrentViewAccessAllowed()
{
	return s_bCanAccessCurrentView;
}

void SetupCurrentView( const Vector &vecOrigin, const QAngle &angles, view_id_t viewID )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// Store off view origin and angles
	g_vecCurrentRenderOrigin = vecOrigin;
	g_vecCurrentRenderAngles = angles;

	// Compute the world->main camera transform
	ComputeCameraVariables( vecOrigin, angles, 
		&g_vecCurrentVForward, &g_vecCurrentVRight, &g_vecCurrentVUp, &g_matCurrentCamInverse );

	g_CurrentViewID = viewID;
	s_bCanAccessCurrentView = true;

	// Cache off fade distances
	float flScreenFadeMinSize, flScreenFadeMaxSize;
	view->GetScreenFadeDistances( &flScreenFadeMinSize, &flScreenFadeMaxSize );
	modelinfo->SetViewScreenFadeRange( flScreenFadeMinSize, flScreenFadeMaxSize );

	CMatRenderContextPtr pRenderContext( materials );
#ifdef PORTAL
	if ( g_pPortalRender->GetViewRecursionLevel() == 0 )
	{
		pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA, ((viewID == VIEW_MAIN) || (viewID == VIEW_3DSKY)) ? 1 : 0 );
	}
#else
	pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA, ((viewID == VIEW_MAIN) || (viewID == VIEW_3DSKY)) ? 1 : 0 );
#endif
}

view_id_t CurrentViewID()
{
	Assert( g_CurrentViewID != VIEW_ILLEGAL );
	return ( view_id_t )g_CurrentViewID;
}

//-----------------------------------------------------------------------------
// Purpose: Portal views are considered 'Main' views. This function tests a view id 
//			against all view ids used by portal renderables, as well as the main view.
//-----------------------------------------------------------------------------
bool IsMainView ( view_id_t id )
{
#if defined(PORTAL)
	return ( (id == VIEW_MAIN) || g_pPortalRender->IsPortalViewID( id ) );
#else
	return (id == VIEW_MAIN);
#endif
}

void FinishCurrentView()
{
	s_bCanAccessCurrentView = false;
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
void CSimpleRenderExecutor::AddView( CRendering3dView *pView )
{
 	CBase3dView *pPrevRenderer = m_pMainView->SetActiveRenderer( pView );
	pView->Draw();
	m_pMainView->SetActiveRenderer( pPrevRenderer );
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CViewRender::CViewRender()
	: m_SimpleExecutor( this )
{
	m_flCheapWaterStartDistance = 0.0f;
	m_flCheapWaterEndDistance = 0.1f;
	m_BaseDrawFlags = 0;
	m_pActiveRenderer = NULL;
	m_pCurrentlyDrawingEntity = NULL;

	m_szCurrentScriptMaterialName[0] = '\0';
}


//-----------------------------------------------------------------------------
// Purpose: Called once per level change
//-----------------------------------------------------------------------------
void CViewRender::LevelShutdown( void )
{
	g_pScreenSpaceEffects->ShutdownScreenSpaceEffects();
	g_CurrentViewID = VIEW_NONE;

	m_ScriptOverlayMaterial.Shutdown();
	m_szCurrentScriptMaterialName[0] = '\0';
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
inline bool CViewRender::ShouldDrawEntities( void )
{
	return ( !m_pDrawEntities || (m_pDrawEntities->GetInt() != 0) );
}


//-----------------------------------------------------------------------------
// Purpose: Check all conditions which would prevent drawing the view model
// Input  : drawViewmodel - 
//			*viewmodel - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CViewRender::ShouldDrawViewModel( bool bDrawViewmodel )
{
	if ( !bDrawViewmodel )
		return false;

	if ( !r_drawviewmodel.GetBool() )
		return false;

	if ( C_BasePlayer::ShouldDrawLocalPlayer() )
		return false;

	if ( !ShouldDrawEntities() )
		return false;

	if ( render->GetViewEntity() > gpGlobals->maxClients )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CViewRender::UpdateRefractIfNeededByList( CUtlVector< IClientRenderable * > &list )
{
	int nCount = list.Count();
	for( int i=0; i < nCount; ++i )
	{
		IClientUnknown *pUnk = list[i]->GetIClientUnknown();
		Assert( pUnk );

		IClientRenderable *pRenderable = pUnk->GetClientRenderable();
		Assert( pRenderable );

		if ( pRenderable->UsesPowerOfTwoFrameBufferTexture() )
		{
			UpdateRefractTexture();
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::DrawRenderablesInList( CUtlVector< IClientRenderable * > &list, int flags )
{
	Assert( m_pCurrentlyDrawingEntity == NULL );
	int nCount = list.Count();
	for( int i=0; i < nCount; ++i )
	{
		IClientUnknown *pUnk = list[i]->GetIClientUnknown();
		Assert( pUnk );

		IClientRenderable *pRenderable = pUnk->GetClientRenderable();
		Assert( pRenderable );

		// Non-view models wanting to render in view model list...
		if ( pRenderable->ShouldDraw() )
		{
			m_pCurrentlyDrawingEntity = pUnk->GetBaseEntity();
			pRenderable->DrawModel( STUDIO_RENDER | flags );
		}
	}
	m_pCurrentlyDrawingEntity = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Actually draw the view model
// Input  : drawViewModel - 
//-----------------------------------------------------------------------------
void CViewRender::DrawViewModels( const CViewSetup &viewRender, bool drawViewmodel )
{
	VPROF( "CViewRender::DrawViewModel" );
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

#ifdef PORTAL //in portal, we'd like a copy of the front buffer without the gun in it for use with the depth doubler
	g_pPortalRender->UpdateDepthDoublerTexture( viewRender );
#endif

	bool bShouldDrawPlayerViewModel = ShouldDrawViewModel( drawViewmodel );
	bool bShouldDrawToolViewModels = ToolsEnabled();

	CMatRenderContextPtr pRenderContext( materials );

	PIXEVENT( pRenderContext, "DrawViewModels" );

	// Restore the matrices
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();

	CViewSetup viewModelSetup( viewRender );
	viewModelSetup.zNear = viewRender.zNearViewmodel;
	viewModelSetup.zFar = viewRender.zFarViewmodel;
	viewModelSetup.fov = viewRender.fovViewmodel;
	viewModelSetup.m_flAspectRatio = engine->GetScreenAspectRatio();

	ITexture *pRTColor = NULL;
	ITexture *pRTDepth = NULL;
	if( viewRender.m_eStereoEye != STEREO_EYE_MONO )
	{
		pRTColor = g_pSourceVR->GetRenderTarget( (ISourceVirtualReality::VREye)(viewRender.m_eStereoEye-1), ISourceVirtualReality::RT_Color );
		pRTDepth = g_pSourceVR->GetRenderTarget( (ISourceVirtualReality::VREye)(viewRender.m_eStereoEye-1), ISourceVirtualReality::RT_Depth );
	}

	// Josh: Reset modulation color + blend
	float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	render->SetColorModulation(	one );
	render->SetBlend( 1.0f );

	render->Push3DView( viewModelSetup, 0, pRTColor, GetFrustum(), pRTDepth );

#ifdef PORTAL //the depth range hack doesn't work well enough for the portal mod (and messing with the depth hack values makes some models draw incorrectly)
				//step up to a full depth clear if we're extremely close to a portal (in a portal environment)
	extern bool LocalPlayerIsCloseToPortal( void ); //defined in C_Portal_Player.cpp, abstracting to a single bool function to remove explicit dependence on c_portal_player.h/cpp, you can define the function as a "return true" in other build configurations at the cost of some perf
	bool bUseDepthHack = !LocalPlayerIsCloseToPortal();
	if( !bUseDepthHack )
		pRenderContext->ClearBuffers( false, true, false );
#else
	const bool bUseDepthHack = true;
#endif

	// FIXME: Add code to read the current depth range
	float depthmin = 0.0f;
	float depthmax = 1.0f;

	// HACK HACK:  Munge the depth range to prevent view model from poking into walls, etc.
	// Force clipped down range
	if( bUseDepthHack )
		pRenderContext->DepthRange( 0.0f, 0.1f );
	
	if ( bShouldDrawPlayerViewModel || bShouldDrawToolViewModels )
	{

		CUtlVector< IClientRenderable * > opaqueViewModelList( 32 );
		CUtlVector< IClientRenderable * > translucentViewModelList( 32 );

		ClientLeafSystem()->CollateViewModelRenderables( opaqueViewModelList, translucentViewModelList );

		if ( ToolsEnabled() && ( !bShouldDrawPlayerViewModel || !bShouldDrawToolViewModels ) )
		{
			int nOpaque = opaqueViewModelList.Count();
			for ( int i = nOpaque-1; i >= 0; --i )
			{
				IClientRenderable *pRenderable = opaqueViewModelList[ i ];
				bool bEntity = pRenderable->GetIClientUnknown()->GetBaseEntity();
				if ( ( bEntity && !bShouldDrawPlayerViewModel ) || ( !bEntity && !bShouldDrawToolViewModels ) )
				{
					opaqueViewModelList.FastRemove( i );
				}
			}

			int nTranslucent = translucentViewModelList.Count();
			for ( int i = nTranslucent-1; i >= 0; --i )
			{
				IClientRenderable *pRenderable = translucentViewModelList[ i ];
				bool bEntity = pRenderable->GetIClientUnknown()->GetBaseEntity();
				if ( ( bEntity && !bShouldDrawPlayerViewModel ) || ( !bEntity && !bShouldDrawToolViewModels ) )
				{
					translucentViewModelList.FastRemove( i );
				}
			}
		}

		if ( !UpdateRefractIfNeededByList( opaqueViewModelList ) )
		{
			UpdateRefractIfNeededByList( translucentViewModelList );
		}

		DrawRenderablesInList( opaqueViewModelList );
		DrawRenderablesInList( translucentViewModelList, STUDIO_TRANSPARENCY );
	}

	// Reset the depth range to the original values
	if( bUseDepthHack )
		pRenderContext->DepthRange( depthmin, depthmax );

	render->PopView( GetFrustum() );

	// Restore the matrices
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CViewRender::ShouldDrawBrushModels( void )
{
	if ( m_pDrawBrushModels && !m_pDrawBrushModels->GetInt() )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Performs screen space effects, if any
//-----------------------------------------------------------------------------
void CViewRender::PerformScreenSpaceEffects( int x, int y, int w, int h )
{
	VPROF("CViewRender::PerformScreenSpaceEffects()");
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// FIXME: Screen-space effects are busted in the editor
	if ( engine->IsHammerRunning() )
		return;

	g_pScreenSpaceEffects->RenderEffects( x, y, w, h );
}


//-----------------------------------------------------------------------------
// Purpose: Sets the screen space effect material (can't be done during rendering)
//-----------------------------------------------------------------------------
void CViewRender::SetScreenOverlayMaterial( IMaterial *pMaterial )
{
	m_ScreenOverlayMaterial.Init( pMaterial );
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
IMaterial *CViewRender::GetScreenOverlayMaterial( )
{
	return m_ScreenOverlayMaterial;
}


//-----------------------------------------------------------------------------
// Purpose: Performs screen space effects, if any
//-----------------------------------------------------------------------------
void CViewRender::PerformScreenOverlay( int x, int y, int w, int h )
{
	VPROF("CViewRender::PerformScreenOverlay()");

	if (m_ScreenOverlayMaterial)
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

		if ( m_ScreenOverlayMaterial->NeedsFullFrameBufferTexture() )
		{
            // FIXME: check with multi/sub-rect renders. Should this be 0,0,w,h instead?
			DrawScreenEffectMaterial( m_ScreenOverlayMaterial, x, y, w, h );
		}
		else if ( m_ScreenOverlayMaterial->NeedsPowerOfTwoFrameBufferTexture() )
		{
			// First copy the FB off to the offscreen texture
			UpdateRefractTexture( x, y, w, h, true );

			// Now draw the entire screen using the material...
			CMatRenderContextPtr pRenderContext( materials );
			ITexture *pTexture = GetPowerOfTwoFrameBufferTexture( );
			int sw = pTexture->GetActualWidth();
			int sh = pTexture->GetActualHeight();
            // Note - don't offset by x,y - already done by the viewport.
			pRenderContext->DrawScreenSpaceRectangle( m_ScreenOverlayMaterial, 0, 0, w, h,
												 0, 0, sw-1, sh-1, sw, sh );
		}
		else
		{
			byte color[4] = { 255, 255, 255, 255 };
			render->ViewDrawFade( color, m_ScreenOverlayMaterial );
		}
	}

	{
		const char *pszScriptMaterial = nullptr;
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
			pszScriptMaterial = pLocalPlayer->GetScriptOverlayMaterial();

		if ( pszScriptMaterial && *pszScriptMaterial )
		{
			if ( !V_strncmp( m_szCurrentScriptMaterialName, pszScriptMaterial, MAX_PATH ) )
			{
				m_ScriptOverlayMaterial.Init( pszScriptMaterial, TEXTURE_GROUP_OTHER, false );
			}

			V_strncpy( m_szCurrentScriptMaterialName, pszScriptMaterial, MAX_PATH );
		}
		else
		{
			m_ScriptOverlayMaterial.Shutdown();
			m_szCurrentScriptMaterialName[0] = '\0';
		}

		if ( m_ScriptOverlayMaterial )
		{
			if ( m_ScriptOverlayMaterial->NeedsFullFrameBufferTexture() )
			{
				// FIXME: check with multi/sub-rect renders. Should this be 0,0,w,h instead?
				DrawScreenEffectMaterial( m_ScriptOverlayMaterial, x, y, w, h );
			}
			else if ( m_ScriptOverlayMaterial->NeedsPowerOfTwoFrameBufferTexture() )
			{
				// First copy the FB off to the offscreen texture
				UpdateRefractTexture( x, y, w, h, true );

				// Now draw the entire screen using the material...
				CMatRenderContextPtr pRenderContext( materials );
				ITexture *pTexture = GetPowerOfTwoFrameBufferTexture( );
				int sw = pTexture->GetActualWidth();
				int sh = pTexture->GetActualHeight();
				// Note - don't offset by x,y - already done by the viewport.
				pRenderContext->DrawScreenSpaceRectangle( m_ScriptOverlayMaterial, 0, 0, w, h,
													 0, 0, sw-1, sh-1, sw, sh );
			}
			else
			{
				byte color[4] = { 255, 255, 255, 255 };
				render->ViewDrawFade( color, m_ScriptOverlayMaterial );
			}
		}
	}
}

void CViewRender::DrawUnderwaterOverlay( void )
{
	IMaterial *pOverlayMat = m_UnderWaterOverlayMaterial;

	if ( pOverlayMat )
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

		CMatRenderContextPtr pRenderContext( materials );

		int x, y, w, h;

		pRenderContext->GetViewport( x, y, w, h );
		if ( pOverlayMat->NeedsFullFrameBufferTexture() )
		{
            // FIXME: check with multi/sub-rect renders. Should this be 0,0,w,h instead?
			DrawScreenEffectMaterial( pOverlayMat, x, y, w, h );
		}
		else if ( pOverlayMat->NeedsPowerOfTwoFrameBufferTexture() )
		{
			// First copy the FB off to the offscreen texture
			UpdateRefractTexture( x, y, w, h, true );

			// Now draw the entire screen using the material...
			CMatRenderContextPtr pRenderContextMat( materials );
			ITexture *pTexture = GetPowerOfTwoFrameBufferTexture( );
			int sw = pTexture->GetActualWidth();
			int sh = pTexture->GetActualHeight();
            // Note - don't offset by x,y - already done by the viewport.
			pRenderContextMat->DrawScreenSpaceRectangle( pOverlayMat, 0, 0, w, h,
													  0, 0, sw-1, sh-1, sw, sh );
		}
		else
		{
            // Note - don't offset by x,y - already done by the viewport.
            // FIXME: actually test this code path.
			pRenderContext->DrawScreenSpaceRectangle( pOverlayMat, 0, 0, w, h,
													  0, 0, 1, 1, 1, 1 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the min/max fade distances
//-----------------------------------------------------------------------------
void CViewRender::GetScreenFadeDistances( float *min, float *max )
{
	if ( min )
	{
		*min = r_screenfademinsize.GetFloat();
	}

	if ( max )
	{
		*max = r_screenfademaxsize.GetFloat();
	}
}

C_BaseEntity *CViewRender::GetCurrentlyDrawingEntity()
{
	return m_pCurrentlyDrawingEntity;
}

void CViewRender::SetCurrentlyDrawingEntity( C_BaseEntity *pEnt )
{
	m_pCurrentlyDrawingEntity = pEnt;
}

bool CViewRender::UpdateShadowDepthTexture( ITexture *pRenderTarget, ITexture *pDepthTexture, const CViewSetup &shadowViewIn )
{
	VPROF_INCREMENT_COUNTER( "shadow depth textures rendered", 1 );

	CMatRenderContextPtr pRenderContext( materials );

	char szPIXEventName[128];
	sprintf( szPIXEventName, "UpdateShadowDepthTexture (%s)", pDepthTexture->GetName() );
	PIXEVENT( pRenderContext, szPIXEventName );

	CRefPtr<CShadowDepthView> pShadowDepthView = new CShadowDepthView( this );
	pShadowDepthView->Setup( shadowViewIn, pRenderTarget, pDepthTexture );
	AddViewToScene( pShadowDepthView );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Renders world and all entities, etc.
//-----------------------------------------------------------------------------
void CViewRender::ViewDrawScene( bool bDrew3dSkybox, SkyboxVisibility_t nSkyboxVisible, const CViewSetup &viewRender, 
								int nClearFlags, view_id_t viewID, bool bDrawViewModel, int baseDrawFlags, ViewCustomVisibility_t *pCustomVisibility )
{
	VPROF( "CViewRender::ViewDrawScene" );
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// this allows the refract texture to be updated once per *scene* on 360
	// (e.g. once for a monitor scene and once for the main scene)
	g_viewscene_refractUpdateFrame = gpGlobals->framecount - 1;

	g_pClientShadowMgr->PreRender();

	// Shadowed flashlights supported on ps_2_b and up...
	if ( r_flashlightdepthtexture.GetBool() && (viewID == VIEW_MAIN) )
	{
		g_pClientShadowMgr->ComputeShadowDepthTextures( viewRender );
	}

	m_BaseDrawFlags = baseDrawFlags;

	SetupCurrentView( viewRender.origin, viewRender.angles, viewID );

	// Invoke pre-render methods
	IGameSystem::PreRenderAllSystems();

	// Start view
	unsigned int visFlags;
	SetupVis( viewRender, visFlags, pCustomVisibility );

	if ( !bDrew3dSkybox && 
		( nSkyboxVisible == SKYBOX_NOT_VISIBLE ) && ( visFlags & IVRenderView::VIEW_SETUP_VIS_EX_RETURN_FLAGS_USES_RADIAL_VIS ) )
	{
		// This covers the case where we don't see a 3dskybox, yet radial vis is clipping
		// the far plane.  Need to clear to fog color in this case.
		nClearFlags |= VIEW_CLEAR_COLOR;
		SetClearColorToFogColor( );
	}

	bool drawSkybox = r_skybox.GetBool();
	if ( bDrew3dSkybox || ( nSkyboxVisible == SKYBOX_NOT_VISIBLE ) )
	{
		drawSkybox = false;
	}

	ParticleMgr()->IncrementFrameCode();

	DrawWorldAndEntities( drawSkybox, viewRender, nClearFlags, pCustomVisibility );

	// Disable fog for the rest of the stuff
	DisableFog();

	// UNDONE: Don't do this with masked brush models, they should probably be in a separate list
	// render->DrawMaskEntities()

	// Here are the overlays...

	CGlowOverlay::DrawOverlays( viewRender.m_bCacheFullSceneState );

	// issue the pixel visibility tests
	if ( IsMainView( CurrentViewID() ) )
	{
		PixelVisibility_EndCurrentView();
	}

	// Draw rain..
	DrawPrecipitation();

	// Make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();

	// Debugging info goes over the top
	CDebugViewRender::Draw3DDebuggingInfo( viewRender );

	// Draw client side effects
	// NOTE: These are not sorted against the rest of the frame
	clienteffects->DrawEffects( gpGlobals->frametime );	

	// Mark the frame as locked down for client fx additions
	SetFXCreationAllowed( false );

	// Invoke post-render methods
	IGameSystem::PostRenderAllSystems();

	FinishCurrentView();

	// Free shadow depth textures for use in future view
	if ( r_flashlightdepthtexture.GetBool() )
	{
		g_pClientShadowMgr->UnlockAllShadowDepthTextures();
	}
}


void CheckAndTransitionColor( float flPercent, float *pColor, float *pLerpToColor )
{
	if ( pLerpToColor[0] != pColor[0] || pLerpToColor[1] != pColor[1] || pLerpToColor[2] != pColor[2] )
	{
		float flDestColor[3];

		flDestColor[0] = pLerpToColor[0];
		flDestColor[1] = pLerpToColor[1];
		flDestColor[2] = pLerpToColor[2];

		pColor[0] = FLerp( pColor[0], flDestColor[0], flPercent );
		pColor[1] = FLerp( pColor[1], flDestColor[1], flPercent );
		pColor[2] = FLerp( pColor[2], flDestColor[2], flPercent );
	}
	else
	{
		pColor[0] = pLerpToColor[0];
		pColor[1] = pLerpToColor[1];
		pColor[2] = pLerpToColor[2];
	}
}

static void GetFogColorTransition( fogparams_t *pFogParams, float *pColorPrimary, float *pColorSecondary )
{
	if ( !pFogParams )
		return;

	if ( pFogParams->lerptime >= gpGlobals->curtime )
	{
		float flPercent = 1.0f - (( pFogParams->lerptime - gpGlobals->curtime ) / pFogParams->duration );

		float flPrimaryColorLerp[3] = { (float)pFogParams->colorPrimaryLerpTo.GetR(), (float)pFogParams->colorPrimaryLerpTo.GetG(), (float)pFogParams->colorPrimaryLerpTo.GetB() };
		float flSecondaryColorLerp[3] = { (float)pFogParams->colorSecondaryLerpTo.GetR(), (float)pFogParams->colorSecondaryLerpTo.GetG(), (float)pFogParams->colorSecondaryLerpTo.GetB() };

		CheckAndTransitionColor( flPercent, pColorPrimary, flPrimaryColorLerp );
		CheckAndTransitionColor( flPercent, pColorSecondary, flSecondaryColorLerp );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the fog color to use in rendering the current frame.
//-----------------------------------------------------------------------------
static void GetFogColor( fogparams_t *pFogParams, float *pColor )
{
	bool bUseOverride = false;

	const char *fogColorString = fog_color.GetString();
	if( fog_override.GetInt() && fogColorString )
	{
		sscanf( fogColorString, "%f%f%f", pColor, pColor+1, pColor+2 );

		bUseOverride = true;
		for ( int i = 0; i < 3; i++ )
		{
			if ( pColor[i] < 0 )
			{
				bUseOverride = false;
			}
		}
	}

	if ( !bUseOverride )
	{
		C_BasePlayer* pbp = C_BasePlayer::GetLocalPlayer();
		if ( !pbp || !pFogParams )
			return;

		float flPrimaryColor[3] = { (float)pFogParams->colorPrimary.GetR(), (float)pFogParams->colorPrimary.GetG(), (float)pFogParams->colorPrimary.GetB() };
		float flSecondaryColor[3] = { (float)pFogParams->colorSecondary.GetR(), (float)pFogParams->colorSecondary.GetG(), (float)pFogParams->colorSecondary.GetB() };

		GetFogColorTransition( pFogParams, flPrimaryColor, flSecondaryColor );

		if( pFogParams->blend )
		{
			//
			// Blend between two fog colors based on viewing angle.
			// The secondary fog color is at 180 degrees to the primary fog color.
			//
			Vector forward;
			pbp->EyeVectors( &forward, NULL, NULL );
			
			Vector vNormalized = pFogParams->dirPrimary;
			VectorNormalize( vNormalized );
			pFogParams->dirPrimary = vNormalized;

			float flBlendFactor = 0.5 * forward.Dot( pFogParams->dirPrimary ) + 0.5;

			// FIXME: convert to linear colorspace
			pColor[0] = flPrimaryColor[0] * flBlendFactor + flSecondaryColor[0] * ( 1 - flBlendFactor );
			pColor[1] = flPrimaryColor[1] * flBlendFactor + flSecondaryColor[1] * ( 1 - flBlendFactor );
			pColor[2] = flPrimaryColor[2] * flBlendFactor + flSecondaryColor[2] * ( 1 - flBlendFactor );
		}
		else
		{
			pColor[0] = flPrimaryColor[0];
			pColor[1] = flPrimaryColor[1];
			pColor[2] = flPrimaryColor[2];
		}
	}

	VectorScale( pColor, 1.0f / 255.0f, pColor );
}


static float GetFogStart( fogparams_t *pFogParams )
{
	if( fog_override.GetInt() )
	{
		if( fog_start.GetInt() != -1 )
		{
			return fog_start.GetFloat();
		}
	}

	if ( !pFogParams )
		return 0.0f;

	if ( pFogParams->lerptime > gpGlobals->curtime )
	{
		if ( pFogParams->start != pFogParams->startLerpTo )
		{
			if ( pFogParams->lerptime > gpGlobals->curtime )
			{
				float flPercent = 1.0f - (( pFogParams->lerptime - gpGlobals->curtime ) / pFogParams->duration );

				return FLerp( pFogParams->start, pFogParams->startLerpTo, flPercent );
			}
			else
			{
				if ( pFogParams->start != pFogParams->startLerpTo )
				{
					pFogParams->start = pFogParams->startLerpTo;
				}
			}
		}
	}

	return pFogParams->start;
}

static float GetFogEnd( fogparams_t *pFogParams )
{
	if( fog_override.GetInt() )
	{
		if( fog_end.GetInt() != -1 )
		{
			return fog_end.GetFloat();
		}
	}

	if ( !pFogParams )
		return 0.0f;

	if ( pFogParams->lerptime > gpGlobals->curtime )
	{
		if ( pFogParams->end != pFogParams->endLerpTo )
		{
			if ( pFogParams->lerptime > gpGlobals->curtime )
			{
				float flPercent = 1.0f - (( pFogParams->lerptime - gpGlobals->curtime ) / pFogParams->duration );

				return FLerp( pFogParams->end, pFogParams->endLerpTo, flPercent );
			}
			else
			{
				if ( pFogParams->end != pFogParams->endLerpTo )
				{
					pFogParams->end = pFogParams->endLerpTo;
				}
			}
		}
	}

	return pFogParams->end;
}

static bool GetFogEnable( fogparams_t *pFogParams )
{
	if ( cl_leveloverview.GetFloat() > 0 )
		return false;

	// Ask the clientmode
	if ( g_pClientMode->ShouldDrawFog() == false )
		return false;

	if( fog_override.GetInt() )
	{
		if ( fog_enable.GetInt() != -1 )
		{
			return fog_enable.GetBool();
		}
	}


	if ( pFogParams )
		return pFogParams->enable != false;

	return false;

}


static float GetFogMaxDensity( fogparams_t *pFogParams )
{
	if ( cl_leveloverview.GetFloat() > 0 )
		return 1.0f;

	// Ask the clientmode
	if ( !g_pClientMode->ShouldDrawFog() )
		return 1.0f;

	if ( fog_override.GetInt() )
	{
		if ( fog_maxdensity.GetInt() != -1 )
			return fog_maxdensity.GetFloat();
	}
	
	if ( !pFogParams )
		return 1.0f;

	return pFogParams->maxdensity;
}

static float GetFogRadial( fogparams_t *pFogParams )
{
	if ( cl_leveloverview.GetFloat() > 0 )
		return false;

	// Ask the clientmode
	if ( !g_pClientMode->ShouldDrawFog() )
		return false;

	if ( fog_override.GetInt() )
	{
		if ( fog_radial.GetInt() != -1 )
			return fog_radial.GetBool();
	}
	
	if ( !pFogParams )
		return false;

	return pFogParams->radial;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the skybox fog color to use in rendering the current frame.
//-----------------------------------------------------------------------------
static void GetSkyboxFogColor( float *pColor )
{			   
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	bool bUseOverride = false;

	const char *fogColorString = fog_colorskybox.GetString();
	if ( fog_override.GetInt() && fogColorString )
	{
		sscanf( fogColorString, "%f%f%f", pColor, pColor + 1, pColor + 2 );

		bUseOverride = true;
		for ( int i = 0; i < 3; i++ )
		{
			if ( pColor[i] < 0 )
			{
				bUseOverride = false;
			}
		}
	}

	if ( !bUseOverride )
	{
		if( local->m_skybox3d.fog.blend )
		{
			//
			// Blend between two fog colors based on viewing angle.
			// The secondary fog color is at 180 degrees to the primary fog color.
			//
			Vector forward;
			pbp->EyeVectors( &forward, NULL, NULL );

			Vector vNormalized = local->m_skybox3d.fog.dirPrimary;
			VectorNormalize( vNormalized );
			local->m_skybox3d.fog.dirPrimary = vNormalized;

			float flBlendFactor = 0.5 * forward.Dot( local->m_skybox3d.fog.dirPrimary ) + 0.5;
						 
			// FIXME: convert to linear colorspace
			pColor[0] = local->m_skybox3d.fog.colorPrimary.GetR() * flBlendFactor + local->m_skybox3d.fog.colorSecondary.GetR() * ( 1 - flBlendFactor );
			pColor[1] = local->m_skybox3d.fog.colorPrimary.GetG() * flBlendFactor + local->m_skybox3d.fog.colorSecondary.GetG() * ( 1 - flBlendFactor );
			pColor[2] = local->m_skybox3d.fog.colorPrimary.GetB() * flBlendFactor + local->m_skybox3d.fog.colorSecondary.GetB() * ( 1 - flBlendFactor );
		}
		else
		{
			pColor[0] = local->m_skybox3d.fog.colorPrimary.GetR();
			pColor[1] = local->m_skybox3d.fog.colorPrimary.GetG();
			pColor[2] = local->m_skybox3d.fog.colorPrimary.GetB();
		}
	}

	VectorScale( pColor, 1.0f / 255.0f, pColor );
}


static float GetSkyboxFogStart( void )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return 0.0f;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	if( fog_override.GetInt() )
	{
		if ( fog_startskybox.GetInt() != -1 )
		{
			return fog_startskybox.GetFloat();
		}
	}

	return local->m_skybox3d.fog.start;
}

static float GetSkyboxFogEnd( void )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return 0.0f;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	if( fog_override.GetInt() )
	{
		if ( fog_endskybox.GetInt() != -1 )
		{
			return fog_endskybox.GetFloat();
		}
	}

	return local->m_skybox3d.fog.end;
}


static float GetSkyboxFogMaxDensity()
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if ( !pbp )
		return 1.0f;

	CPlayerLocalData *local = &pbp->m_Local;

	if ( cl_leveloverview.GetFloat() > 0 )
		return 1.0f;

	// Ask the clientmode
	if ( !g_pClientMode->ShouldDrawFog() )
		return 1.0f;

	if ( fog_override.GetInt() )
	{
		if ( fog_maxdensityskybox.GetInt() != -1 )
		{
			return fog_maxdensityskybox.GetFloat();
		}
	}

	return local->m_skybox3d.fog.maxdensity;
}

static bool GetSkyboxFogRadial()
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if ( !pbp )
		return false;

	CPlayerLocalData *local = &pbp->m_Local;

	if ( cl_leveloverview.GetFloat() > 0 )
		return false;

	// Ask the clientmode
	if ( !g_pClientMode->ShouldDrawFog() )
		return false;

	if ( fog_override.GetInt() )
	{
		if ( fog_radialskybox.GetInt() != -1 )
		{
			return fog_radialskybox.GetBool();
		}
	}

	return local->m_skybox3d.fog.radial;
}



void CViewRender::DisableFog( void )
{
	VPROF("CViewRander::DisableFog()");

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->FogMode( MATERIAL_FOG_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::SetupVis( const CViewSetup& viewRender, unsigned int &visFlags, ViewCustomVisibility_t *pCustomVisibility )
{
	VPROF( "CViewRender::SetupVis" );
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	if ( pCustomVisibility && pCustomVisibility->m_nNumVisOrigins )
	{
		// Pass array or vis origins to merge
		render->ViewSetupVisEx( ShouldForceNoVis(), pCustomVisibility->m_nNumVisOrigins, pCustomVisibility->m_rgVisOrigins, visFlags );
	}
	else
	{
		// Use render origin as vis origin by default
		render->ViewSetupVisEx( ShouldForceNoVis(), 1, &viewRender.origin, visFlags );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Renders voice feedback and other sprites attached to players
// Input  : none
//-----------------------------------------------------------------------------
void CViewRender::RenderPlayerSprites()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	GetClientVoiceMgr()->DrawHeadLabels();
}

//-----------------------------------------------------------------------------
// Sets up, cleans up the main 3D view
//-----------------------------------------------------------------------------
void CViewRender::SetupMain3DView( const CViewSetup &viewRender, int &nClearFlags )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// FIXME: I really want these fields removed from CViewSetup 
	// and passed in as independent flags
	// Clear the color here if requested.

	int nDepthStencilFlags = nClearFlags & ( VIEW_CLEAR_DEPTH | VIEW_CLEAR_STENCIL );
	nClearFlags &= ~( nDepthStencilFlags ); // Clear these flags
	if ( nClearFlags & VIEW_CLEAR_COLOR )
	{
		nClearFlags |= nDepthStencilFlags; // Add them back in if we're clearing color
	}

	// If we are using HDR, we render to the HDR full frame buffer texture
	// instead of whatever was previously the render target
	if( g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_FLOAT )
	{
		render->Push3DView( viewRender, nClearFlags, GetFullFrameFrameBufferTexture( 0 ), GetFrustum() );
	}
	else
	{
		ITexture *pRTColor = NULL;
		ITexture *pRTDepth = NULL;
		if( viewRender.m_eStereoEye != STEREO_EYE_MONO )
		{
			pRTColor = g_pSourceVR->GetRenderTarget( (ISourceVirtualReality::VREye)(viewRender.m_eStereoEye-1), ISourceVirtualReality::RT_Color );
			pRTDepth = g_pSourceVR->GetRenderTarget( (ISourceVirtualReality::VREye)(viewRender.m_eStereoEye-1), ISourceVirtualReality::RT_Depth );
		}

		render->Push3DView( viewRender, nClearFlags, pRTColor, GetFrustum(), pRTDepth );
	}

	// If we didn't clear the depth here, we'll need to clear it later
	nClearFlags ^= nDepthStencilFlags; // Toggle these bits
	if ( nClearFlags & VIEW_CLEAR_COLOR )
	{
		// If we cleared the color here, we don't need to clear it later
		nClearFlags &= ~( VIEW_CLEAR_COLOR | VIEW_CLEAR_FULL_TARGET );
	}
}

void CViewRender::CleanupMain3DView( const CViewSetup &viewRender )
{
	render->PopView( GetFrustum() );
}


//-----------------------------------------------------------------------------
// Queues up an overlay rendering
//-----------------------------------------------------------------------------
void CViewRender::QueueOverlayRenderView( const CViewSetup &viewRender, int nClearFlags, int whatToDraw )
{
	// Can't have 2 in a single scene
	Assert( !m_bDrawOverlay );

    m_bDrawOverlay = true;
	m_OverlayViewSetup = viewRender;
	m_OverlayClearFlags = nClearFlags;
	m_OverlayDrawFlags = whatToDraw;
}

//-----------------------------------------------------------------------------
// Purpose: Force the view to freeze on the next frame for the specified time
//-----------------------------------------------------------------------------
void CViewRender::FreezeFrame( float flFreezeTime )
{
	if ( flFreezeTime == 0 )
	{
		m_flFreezeFrameUntil = 0;
		for( int i=0; i < STEREO_EYE_MAX; i++ )
		{
			m_rbTakeFreezeFrame[ i ] = false;
		}
	}
	else
	{
		if ( m_flFreezeFrameUntil > gpGlobals->curtime )
		{
			m_flFreezeFrameUntil += flFreezeTime;
		}
		else
		{
			m_flFreezeFrameUntil = gpGlobals->curtime + flFreezeTime;
			for( int i=GetFirstEye(); i <= GetLastEye(); i++ )
			{
				m_rbTakeFreezeFrame[ i ] = true;
			}
		}
	}
}

const char *COM_GetModDirectory();


//-----------------------------------------------------------------------------
// Purpose: This renders the entire 3D view and the in-game hud/viewmodel
// Input  : &view - 
//			whatToDraw - 
//-----------------------------------------------------------------------------
// This renders the entire 3D view.
void CViewRender::RenderView( const CViewSetup &viewRender, int nClearFlags, int whatToDraw )
{
	m_UnderWaterOverlayMaterial.Shutdown();					// underwater view will set

	m_CurrentView = viewRender;

	C_BaseAnimating::AutoAllowBoneAccess boneaccess( true, true );
	VPROF( "CViewRender::RenderView" );
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// Don't want TF2 running less than DX 8
	if ( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 80 )
	{
		// We know they were running at least 8.0 when the game started...we check the 
		// value in ClientDLL_Init()...so they must be messing with their DirectX settings.
		if ( ( Q_stricmp( COM_GetModDirectory(), "tf" ) == 0 ) || ( Q_stricmp( COM_GetModDirectory(), "tf_beta" ) == 0 ) )
		{
			static bool bFirstTime = true;
			if ( bFirstTime )
			{
				bFirstTime = false;
				Msg( "This game has a minimum requirement of DirectX 8.0 to run properly.\n" );
			}
			return;
		}
	}

	CMatRenderContextPtr pRenderContext( materials );
	ITexture *saveRenderTarget = pRenderContext->GetRenderTarget();
	pRenderContext.SafeRelease(); // don't want to hold for long periods in case in a locking active share thread mode

	if ( !m_rbTakeFreezeFrame[viewRender.m_eStereoEye ] && m_flFreezeFrameUntil > gpGlobals->curtime )
	{
		CRefPtr<CFreezeFrameView> pFreezeFrameView = new CFreezeFrameView( this );
		pFreezeFrameView->Setup( viewRender );
		AddViewToScene( pFreezeFrameView );

		g_bRenderingView = true;
		s_bCanAccessCurrentView = true;
	}
	else
	{
		g_flFreezeFlash = 0.0f;

		g_pClientShadowMgr->AdvanceFrame();

	#ifdef USE_MONITORS
		if ( cl_drawmonitors.GetBool() && 
			( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() >= 70 ) &&
			( ( whatToDraw & RENDERVIEW_SUPPRESSMONITORRENDERING ) == 0 ) )
		{
			CViewSetup viewMiddle = GetView( STEREO_EYE_MONO );
			DrawMonitors( viewMiddle );	
		}
	#endif

		g_bRenderingView = true;

		// Must be first 
		render->SceneBegin();

		pRenderContext.GetFrom( materials );
		pRenderContext->TurnOnToneMapping();
		pRenderContext.SafeRelease();

		// clear happens here probably
		SetupMain3DView( viewRender, nClearFlags );
			 	  
		bool bDrew3dSkybox = false;
		SkyboxVisibility_t nSkyboxVisible = SKYBOX_NOT_VISIBLE;

		// if the 3d skybox world is drawn, then don't draw the normal skybox
		CSkyboxView *pSkyView = new CSkyboxView( this );
		if ( ( bDrew3dSkybox = pSkyView->Setup( viewRender, &nClearFlags, &nSkyboxVisible ) ) != false )
		{
			AddViewToScene( pSkyView );
		}
		SafeRelease( pSkyView );

		// Force it to clear the framebuffer if they're in solid space.
		if ( ( nClearFlags & VIEW_CLEAR_COLOR ) == 0 )
		{
			if ( enginetrace->GetPointContents( viewRender.origin ) == CONTENTS_SOLID )
			{
				nClearFlags |= VIEW_CLEAR_COLOR;
			}
		}

		// Render world and all entities, particles, etc.
		if( !g_pIntroData )
		{
			ViewDrawScene( bDrew3dSkybox, nSkyboxVisible, viewRender, nClearFlags, VIEW_MAIN, whatToDraw & RENDERVIEW_DRAWVIEWMODEL );
		}
		else
		{
			ViewDrawScene_Intro( viewRender, nClearFlags, *g_pIntroData );
		}

		// We can still use the 'current view' stuff set up in ViewDrawScene
		s_bCanAccessCurrentView = true;


		engine->DrawPortals();

		DisableFog();

		// Finish scene
		render->SceneEnd();

		// Draw lightsources if enabled
		render->DrawLights();

		RenderPlayerSprites();

		// Image-space motion blur
		if ( !building_cubemaps.GetBool() && viewRender.m_bDoBloomAndToneMapping ) // We probably should use a different view. variable here
		{
			static ConVarRef mat_motion_blur_enabled( "mat_motion_blur_enabled" );
			if ( ( mat_motion_blur_enabled.GetInt() ) && ( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() >= 90 ) )
			{
				pRenderContext.GetFrom( materials );
				{
					PIXEVENT( pRenderContext, "DoImageSpaceMotionBlur" );
					DoImageSpaceMotionBlur( viewRender, viewRender.x, viewRender.y, viewRender.width, viewRender.height );
				}
				pRenderContext.SafeRelease();
			}
		}

		GetClientModeNormal()->DoPostScreenSpaceEffects( &viewRender );

		// Now actually draw the viewmodel
		DrawViewModels( viewRender, whatToDraw & RENDERVIEW_DRAWVIEWMODEL );

		DrawUnderwaterOverlay();

		PixelVisibility_EndScene();

		// Draw fade over entire screen if needed
		byte color[4];
		bool blend;
		vieweffects->GetFadeParams( &color[0], &color[1], &color[2], &color[3], &blend );

		// Draw an overlay to make it even harder to see inside smoke particle systems.
		DrawSmokeFogOverlay();

		// Overlay screen fade on entire screen
		IMaterial* pMaterial = blend ? m_ModulateSingleColor : m_TranslucentSingleColor;
		render->ViewDrawFade( color, pMaterial );
		PerformScreenOverlay( viewRender.x, viewRender.y, viewRender.width, viewRender.height );

		// Prevent sound stutter if going slow
		engine->Sound_ExtraUpdate();	
	
		if ( !building_cubemaps.GetBool() && viewRender.m_bDoBloomAndToneMapping )
		{
			pRenderContext.GetFrom( materials );
			{
				PIXEVENT( pRenderContext, "DoEnginePostProcessing" );

				bool bFlashlightIsOn = false;
				C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
				if ( pLocal )
				{
					bFlashlightIsOn = pLocal->IsEffectActive( EF_DIMLIGHT );
				}
				DoEnginePostProcessing( viewRender.x, viewRender.y, viewRender.width, viewRender.height, bFlashlightIsOn );
			}
			pRenderContext.SafeRelease();
		}

		// And here are the screen-space effects

		if ( IsPC() )
		{
			tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "GrabPreColorCorrectedFrame" );

			// Grab the pre-color corrected frame for editing purposes
			engine->GrabPreColorCorrectedFrame( viewRender.x, viewRender.y, viewRender.width, viewRender.height );
		}

		PerformScreenSpaceEffects( 0, 0, viewRender.width, viewRender.height );

		if ( g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER )
		{
			pRenderContext.GetFrom( materials );
			pRenderContext->SetToneMappingScaleLinear(Vector(1,1,1));
			pRenderContext.SafeRelease();
		}

		CleanupMain3DView( viewRender );

		if ( m_rbTakeFreezeFrame[viewRender.m_eStereoEye ] )
		{
			Rect_t rect;
			rect.x = viewRender.x;
			rect.y = viewRender.y;
			rect.width = viewRender.width;
			rect.height = viewRender.height;

			pRenderContext = materials->GetRenderContext();
			if ( IsX360() )
			{
				// 360 doesn't create the Fullscreen texture
				pRenderContext->CopyRenderTargetToTextureEx( GetFullFrameFrameBufferTexture( 1 ), 0, &rect, &rect );
			}
			else
			{
				pRenderContext->CopyRenderTargetToTextureEx( GetFullscreenTexture(), 0, &rect, &rect );
			}
			pRenderContext.SafeRelease();
			m_rbTakeFreezeFrame[viewRender.m_eStereoEye ] = false;
		}

		pRenderContext = materials->GetRenderContext();
		pRenderContext->SetRenderTarget( saveRenderTarget );
		pRenderContext.SafeRelease();

		// Draw the overlay
		if ( m_bDrawOverlay )
		{	   
			tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "DrawOverlay" );

			// This allows us to be ok if there are nested overlay views
			CViewSetup currentView = m_CurrentView;
			CViewSetup tempView = m_OverlayViewSetup;
			tempView.fov = ScaleFOVByWidthRatio( tempView.fov, tempView.m_flAspectRatio / ( 4.0f / 3.0f ) );
			tempView.m_bDoBloomAndToneMapping = false;	// FIXME: Hack to get Mark up and running
			m_bDrawOverlay = false;
			RenderView( tempView, m_OverlayClearFlags, m_OverlayDrawFlags );
			m_CurrentView = currentView;
		}

	}

	if ( mat_viewportupscale.GetBool() && mat_viewportscale.GetFloat() < 1.0f ) 
	{
		CMatRenderContextPtr pRenderContextUpscale( materials );

		ITexture	*pFullFrameFB1 = materials->FindTexture( "_rt_FullFrameFB1", TEXTURE_GROUP_RENDER_TARGET );
		IMaterial	*pCopyMaterial = materials->FindMaterial( "dev/upscale", TEXTURE_GROUP_OTHER );
		pCopyMaterial->IncrementReferenceCount();

		Rect_t	DownscaleRect, UpscaleRect;

		DownscaleRect.x = viewRender.x;
		DownscaleRect.y = viewRender.y;
		DownscaleRect.width = viewRender.width;
		DownscaleRect.height = viewRender.height;

		UpscaleRect.x = viewRender.m_nUnscaledX;
		UpscaleRect.y = viewRender.m_nUnscaledY;
		UpscaleRect.width = viewRender.m_nUnscaledWidth;
		UpscaleRect.height = viewRender.m_nUnscaledHeight;

		pRenderContextUpscale->CopyRenderTargetToTextureEx( pFullFrameFB1, 0, &DownscaleRect, &DownscaleRect );
		pRenderContextUpscale->DrawScreenSpaceRectangle( pCopyMaterial, UpscaleRect.x, UpscaleRect.y, UpscaleRect.width, UpscaleRect.height,
			DownscaleRect.x, DownscaleRect.y, DownscaleRect.x+DownscaleRect.width-1, DownscaleRect.y+DownscaleRect.height-1, 
			pFullFrameFB1->GetActualWidth(), pFullFrameFB1->GetActualHeight() );

		pCopyMaterial->DecrementReferenceCount();
	}

	// if we're in VR mode we might need to override the render target
	if( UseVR() )
	{
		saveRenderTarget = g_pSourceVR->GetRenderTarget( (ISourceVirtualReality::VREye)(viewRender.m_eStereoEye - 1), ISourceVirtualReality::RT_Color );
	}

	// Draw the 2D graphics
	render->Push2DView( viewRender, 0, saveRenderTarget, GetFrustum() );

	Render2DEffectsPreHUD( viewRender );

	if ( whatToDraw & RENDERVIEW_DRAWHUD )
	{
		VPROF_BUDGET( "VGui_DrawHud", VPROF_BUDGETGROUP_OTHER_VGUI );
		int viewWidth = viewRender.m_nUnscaledWidth;
		int viewHeight = viewRender.m_nUnscaledHeight;
		int viewActualWidth = viewRender.m_nUnscaledWidth;
		int viewActualHeight = viewRender.m_nUnscaledHeight;
		int viewX = viewRender.m_nUnscaledX;
		int viewY = viewRender.m_nUnscaledY;
		int viewFramebufferX = 0;
		int viewFramebufferY = 0;
		int viewFramebufferWidth = viewWidth;
		int viewFramebufferHeight = viewHeight;
		bool bClear = false;
		bool bPaintMainMenu = false;
		ITexture *pTexture = NULL;
		if( UseVR() )
		{
			if( g_ClientVirtualReality.ShouldRenderHUDInWorld() )
			{
				pTexture = materials->FindTexture( "_rt_gui", NULL, false );
				if( pTexture )
				{
					bPaintMainMenu = true;
					bClear = true;
					viewX = 0;
					viewY = 0;
					viewActualWidth = pTexture->GetActualWidth();
					viewActualHeight = pTexture->GetActualHeight();

					vgui::surface()->GetScreenSize( viewWidth, viewHeight );

					viewFramebufferX = 0;
					if( viewRender.m_eStereoEye == STEREO_EYE_RIGHT && !saveRenderTarget )
						viewFramebufferX = viewFramebufferWidth;
					viewFramebufferY = 0;
				}
			}
			else
			{
				viewFramebufferX = viewRender.m_eStereoEye == STEREO_EYE_RIGHT ? viewWidth : 0;
				viewFramebufferY = 0;
			}
		}

		// Get the render context out of materials to avoid some debug stuff.
		// WARNING THIS REQUIRES THE .SafeRelease below or it'll never release the ref
		pRenderContext = materials->GetRenderContext();

		// clear depth in the backbuffer before we push the render target
		if( bClear )
		{
			pRenderContext->ClearBuffers( false, true, true );
		}

		// constrain where VGUI can render to the view
		pRenderContext->PushRenderTargetAndViewport( pTexture, NULL, viewX, viewY, viewActualWidth, viewActualHeight );
		// If drawing off-screen, force alpha for that pass
		if (pTexture)
		{
			pRenderContext->OverrideAlphaWriteEnable( true, true );
		}

		// let vgui know where to render stuff for the forced-to-framebuffer panels
		if( UseVR() )
		{
			g_pMatSystemSurface->SetFullscreenViewportAndRenderTarget( viewFramebufferX, viewFramebufferY, viewFramebufferWidth, viewFramebufferHeight, saveRenderTarget );
		}

		// clear the render target if we need to
		if( bClear )
		{
			pRenderContext->ClearColor4ub( 0, 0, 0, 0 );
			pRenderContext->ClearBuffers( true, false );
		}
		pRenderContext.SafeRelease();

		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "VGui_DrawHud", __FUNCTION__ );

		// paint the vgui screen
		VGui_PreRender();

		// Make sure the client .dll root panel is at the proper point before doing the "SolveTraverse" calls
		vgui::VPANEL root = enginevgui->GetPanel( PANEL_CLIENTDLL );
		if ( root != 0 )
		{
			vgui::ipanel()->SetSize( root, viewWidth, viewHeight );
		}
		// Same for client .dll tools
		root = enginevgui->GetPanel( PANEL_CLIENTDLL_TOOLS );
		if ( root != 0 )
		{
			vgui::ipanel()->SetSize( root, viewWidth, viewHeight );
		}

		// The crosshair, etc. needs to get at the current setup stuff
		AllowCurrentViewAccess( true );

		// Draw the in-game stuff based on the actual viewport being used
		render->VGui_Paint( PAINT_INGAMEPANELS );

		// maybe paint the main menu and cursor too if we're in stereo hud mode
		if( bPaintMainMenu )
			render->VGui_Paint( PAINT_UIPANELS | PAINT_CURSOR );

		AllowCurrentViewAccess( false );

		VGui_PostRender();

		g_pClientMode->PostRenderVGui();
		pRenderContext = materials->GetRenderContext();
		if (pTexture)
		{
			pRenderContext->OverrideAlphaWriteEnable( false, true );
		}
		pRenderContext->PopRenderTargetAndViewport();

		if ( UseVR() )
		{
			// figure out if we really want to draw the HUD based on freeze cam
			C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
			bool bInFreezeCam = ( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM );

			// draw the HUD after the view model so its "I'm closer" depth queues work right.
			if( !bInFreezeCam && g_ClientVirtualReality.ShouldRenderHUDInWorld() )
			{
				// Now we've rendered the HUD to its texture, actually get it on the screen.
				// Since we're drawing it as a 3D object, we need correctly set up frustum, etc.
				int ClearFlags = 0;
				SetupMain3DView( viewRender, ClearFlags );

				// TODO - a bit of a shonky test - basically trying to catch the main menu, the briefing screen, the loadout screen, etc.
				bool bTranslucent = !g_pMatSystemSurface->IsCursorVisible();
				g_ClientVirtualReality.RenderHUDQuad( g_pClientMode->ShouldBlackoutAroundHUD(), bTranslucent );
				CleanupMain3DView( viewRender );
			}
		}

		pRenderContext->Flush();
		pRenderContext.SafeRelease();
	}

	CDebugViewRender::Draw2DDebuggingInfo( viewRender );

	Render2DEffectsPostHUD( viewRender );

	g_bRenderingView = false;

	// We can no longer use the 'current view' stuff set up in ViewDrawScene
	s_bCanAccessCurrentView = false;

	if ( IsPC() )
	{
		CDebugViewRender::GenerateOverdrawForTesting();
	}

	render->PopView( GetFrustum() );
	g_WorldListCache.Flush();
}

//-----------------------------------------------------------------------------
// Purpose: Renders extra 2D effects in derived classes while the 2D view is on the stack
//-----------------------------------------------------------------------------
void CViewRender::Render2DEffectsPreHUD( const CViewSetup &viewRender )
{
}

//-----------------------------------------------------------------------------
// Purpose: Renders extra 2D effects in derived classes while the 2D view is on the stack
//-----------------------------------------------------------------------------
void CViewRender::Render2DEffectsPostHUD( const CViewSetup &viewRender )
{
}



//-----------------------------------------------------------------------------
//
// NOTE: Below here is all of the stuff that needs to be done for water rendering
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Determines what kind of water we're going to use
//-----------------------------------------------------------------------------
void CViewRender::DetermineWaterRenderInfo( const VisibleFogVolumeInfo_t &fogVolumeInfo, WaterRenderInfo_t &info )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// By default, assume cheap water (even if there's no water in the scene!)
	info.m_bCheapWater = true;
	info.m_bRefract = false;
	info.m_bReflect = false;
	info.m_bReflectEntities = false;
	info.m_bDrawWaterSurface = false;
	info.m_bOpaqueWater = true;



	IMaterial *pWaterMaterial = fogVolumeInfo.m_pFogVolumeMaterial;
	if (( fogVolumeInfo.m_nVisibleFogVolume == -1 ) || !pWaterMaterial )
		return;


	// Use cheap water if mat_drawwater is set
	info.m_bDrawWaterSurface = mat_drawwater.GetBool();
	if ( !info.m_bDrawWaterSurface )
	{
		info.m_bOpaqueWater = false;
		return;
	}

#ifdef _X360
	bool bForceExpensive = false;
#else
	bool bForceExpensive = r_waterforceexpensive.GetBool();
#endif
	bool bForceReflectEntities = r_waterforcereflectentities.GetBool();

#ifdef PORTAL
	switch( g_pPortalRender->ShouldForceCheaperWaterLevel() )
	{
	case 0: //force cheap water
		info.m_bCheapWater = true;
		return;

	case 1: //downgrade level to "simple reflection"
		bForceExpensive = false;

	case 2: //downgrade level to "reflect world"
		bForceReflectEntities = false;
	
	default:
		break;
	};
#endif

	// Determine if the water surface is opaque or not
	info.m_bOpaqueWater = !pWaterMaterial->IsTranslucent();

	// DX level 70 can't handle anything but cheap water
	if (engine->GetDXSupportLevel() < 80)
		return;

	bool bForceCheap = false;

	// The material can override the default settings though
	IMaterialVar *pForceCheapVar = pWaterMaterial->FindVar( "$forcecheap", NULL, false );
	IMaterialVar *pForceExpensiveVar = pWaterMaterial->FindVar( "$forceexpensive", NULL, false );
	if ( pForceCheapVar && pForceCheapVar->IsDefined() )
	{
		bForceCheap = ( pForceCheapVar->GetIntValueFast() != 0 );
		if ( bForceCheap )
		{
			bForceExpensive = false;
		}
	}
	if ( !bForceCheap && pForceExpensiveVar && pForceExpensiveVar->IsDefined() )
	{
		 bForceExpensive = bForceExpensive || ( pForceExpensiveVar->GetIntValueFast() != 0 );
	}

	bool bDebugCheapWater = r_debugcheapwater.GetBool();
	if( bDebugCheapWater )
	{
		Msg( "Water material: %s dist to water: %f\nforcecheap: %s forceexpensive: %s\n", 
			pWaterMaterial->GetName(), fogVolumeInfo.m_flDistanceToWater, 
			bForceCheap ? "true" : "false", bForceExpensive ? "true" : "false" );
	}

	// Unless expensive water is active, reflections are off.
	bool bLocalReflection;
#ifdef _X360
	if( !r_WaterDrawReflection.GetBool() )
#else
	if( !bForceExpensive || !r_WaterDrawReflection.GetBool() )
#endif
	{
		bLocalReflection = false;
	}
	else
	{
		IMaterialVar *pReflectTextureVar = pWaterMaterial->FindVar( "$reflecttexture", NULL, false );
		bLocalReflection = pReflectTextureVar && (pReflectTextureVar->GetType() == MATERIAL_VAR_TYPE_TEXTURE);
	}

	// Brian says FIXME: I disabled cheap water LOD when local specular is specified.
	// There are very few places that appear to actually
	// take advantage of it (places where water is in the PVS, but outside of LOD range).
	// It was 2 hours before code lock, and I had the choice of either doubling fill-rate everywhere
	// by making cheap water lod actually work (the water LOD wasn't actually rendering!!!)
	// or to just always render the reflection + refraction if there's a local specular specified.
	// Note that water LOD *does* work with refract-only water

	// Gary says: I'm reverting this change so that water LOD works on dx9 for ep2.

	// Check if the water is out of the cheap water LOD range; if so, use cheap water
#ifdef _X360
	if ( !bForceExpensive && ( bForceCheap || ( fogVolumeInfo.m_flDistanceToWater >= m_flCheapWaterEndDistance ) ) )
	{
		return;
	}
#else
	if ( ( (fogVolumeInfo.m_flDistanceToWater >= m_flCheapWaterEndDistance) && !bLocalReflection ) || bForceCheap )
 		return;
#endif
	// Get the material that is for the water surface that is visible and check to see
	// what render targets need to be rendered, if any.
	if ( !r_WaterDrawRefraction.GetBool() )
	{
		info.m_bRefract = false;
	}
	else
	{
		IMaterialVar *pRefractTextureVar = pWaterMaterial->FindVar( "$refracttexture", NULL, false );
		info.m_bRefract = pRefractTextureVar && (pRefractTextureVar->GetType() == MATERIAL_VAR_TYPE_TEXTURE);

		// Refractive water can be seen through
		if ( info.m_bRefract )
		{
			info.m_bOpaqueWater = false;
		}
	}

	info.m_bReflect = bLocalReflection;
	if ( info.m_bReflect )
	{
		if( bForceReflectEntities )
		{
			info.m_bReflectEntities = true;
		}
		else
		{
			IMaterialVar *pReflectEntitiesVar = pWaterMaterial->FindVar( "$reflectentities", NULL, false );
			info.m_bReflectEntities = pReflectEntitiesVar && (pReflectEntitiesVar->GetIntValueFast() != 0);
		}
	}

	info.m_bCheapWater = !info.m_bReflect && !info.m_bRefract;

	if( bDebugCheapWater )
	{
		Warning( "refract: %s reflect: %s\n", info.m_bRefract ? "true" : "false", info.m_bReflect ? "true" : "false" );
	}
}

//-----------------------------------------------------------------------------
// Draws the world and all entities
//-----------------------------------------------------------------------------
void CViewRender::DrawWorldAndEntities( bool bDrawSkybox, const CViewSetup &viewIn, int nClearFlags, ViewCustomVisibility_t *pCustomVisibility )
{
	MDLCACHE_CRITICAL_SECTION();
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	VisibleFogVolumeInfo_t fogVolumeInfo;
#ifdef PORTAL //in portal, we can't use the fog volume for the camera since it's almost never in the same fog volume as what's in front of the portal
	if( g_pPortalRender->GetViewRecursionLevel() == 0 )
	{
		render->GetVisibleFogVolume( viewIn.origin, &fogVolumeInfo );
	}
	else
	{
		render->GetVisibleFogVolume( g_pPortalRender->GetExitPortalFogOrigin(), &fogVolumeInfo );
	}
#else
	render->GetVisibleFogVolume( viewIn.origin, &fogVolumeInfo );
#endif

	WaterRenderInfo_t info;
	DetermineWaterRenderInfo( fogVolumeInfo, info );

	if ( info.m_bCheapWater )
	{		     
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "bCheapWater" );
		cplane_t glassReflectionPlane;
		if ( IsReflectiveGlassInView( viewIn, glassReflectionPlane ) )
		{								    
			CRefPtr<CReflectiveGlassView> pGlassReflectionView = new CReflectiveGlassView( this );
			pGlassReflectionView->Setup( viewIn, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, bDrawSkybox, fogVolumeInfo, info, glassReflectionPlane );
			AddViewToScene( pGlassReflectionView );

			CRefPtr<CRefractiveGlassView> pGlassRefractionView = new CRefractiveGlassView( this );
			pGlassRefractionView->Setup( viewIn, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, bDrawSkybox, fogVolumeInfo, info, glassReflectionPlane );
			AddViewToScene( pGlassRefractionView );
		}

		CRefPtr<CSimpleWorldView> pNoWaterView = new CSimpleWorldView( this );
		pNoWaterView->Setup( viewIn, nClearFlags, bDrawSkybox, fogVolumeInfo, info, pCustomVisibility );
		AddViewToScene( pNoWaterView );
		return;
	}

	Assert( !pCustomVisibility );

	// Blat out the visible fog leaf if we're not going to use it
	if ( !r_ForceWaterLeaf.GetBool() )
	{
		fogVolumeInfo.m_nVisibleFogVolumeLeaf = -1;
	}

	// We can see water of some sort
	if ( !fogVolumeInfo.m_bEyeInFogVolume )
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "CAboveWaterView" );
		CRefPtr<CAboveWaterView> pAboveWaterView = new CAboveWaterView( this );
		pAboveWaterView->Setup( viewIn, bDrawSkybox, fogVolumeInfo, info );
		AddViewToScene( pAboveWaterView );
	}
	else
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "CUnderWaterView" );
		CRefPtr<CUnderWaterView> pUnderWaterView = new CUnderWaterView( this );
		pUnderWaterView->Setup( viewIn, bDrawSkybox, fogVolumeInfo, info );
		AddViewToScene( pUnderWaterView );
	}
}


//-----------------------------------------------------------------------------
// Pushes a water render target
//-----------------------------------------------------------------------------
static Vector SavedLinearLightMapScale(-1,-1,-1);			// x<0 = no saved scale

static void SetLightmapScaleForWater(void)
{
	if (g_pMaterialSystemHardwareConfig->GetHDRType()==HDR_TYPE_INTEGER)
	{
		CMatRenderContextPtr pRenderContext( materials );
		SavedLinearLightMapScale=pRenderContext->GetToneMappingScaleLinear();
		Vector t25=SavedLinearLightMapScale;
		t25*=0.25;
		pRenderContext->SetToneMappingScaleLinear(t25);
	}
}

//-----------------------------------------------------------------------------
// Returns true if the view plane intersects the water
//-----------------------------------------------------------------------------
bool DoesViewPlaneIntersectWater( float waterZ, int leafWaterDataID )
{
	if ( leafWaterDataID == -1 )
		return false;

#ifdef PORTAL //when rendering portal views point/plane intersections just don't cut it.
	if( g_pPortalRender->GetViewRecursionLevel() != 0 )
		return g_pPortalRender->DoesExitPortalViewIntersectWaterPlane( waterZ, leafWaterDataID );
#endif

	CMatRenderContextPtr pRenderContext( materials );
	
	VMatrix viewMatrix, projectionMatrix, viewProjectionMatrix, inverseViewProjectionMatrix;
	pRenderContext->GetMatrix( MATERIAL_VIEW, &viewMatrix );
	pRenderContext->GetMatrix( MATERIAL_PROJECTION, &projectionMatrix );
	MatrixMultiply( projectionMatrix, viewMatrix, viewProjectionMatrix );
	MatrixInverseGeneral( viewProjectionMatrix, inverseViewProjectionMatrix );

	Vector mins, maxs;
	ClearBounds( mins, maxs );
	Vector testPoint[4];
	testPoint[0].Init( -1.0f, -1.0f, 0.0f );
	testPoint[1].Init( -1.0f, 1.0f, 0.0f );
	testPoint[2].Init( 1.0f, -1.0f, 0.0f );
	testPoint[3].Init( 1.0f, 1.0f, 0.0f );
	int i;
	bool bAbove = false;
	bool bBelow = false;
	float fudge = 7.0f;
	for( i = 0; i < 4; i++ )
	{
		Vector worldPos;
		Vector3DMultiplyPositionProjective( inverseViewProjectionMatrix, testPoint[i], worldPos );
		AddPointToBounds( worldPos, mins, maxs );
//		Warning( "viewplanez: %f waterZ: %f\n", worldPos.z, waterZ );
		if( worldPos.z + fudge > waterZ )
		{
			bAbove = true;
		}
		if( worldPos.z - fudge < waterZ )
		{
			bBelow = true;
		}
	}

	// early out if the near plane doesn't cross the z plane of the water.
	if( !( bAbove && bBelow ) )
		return false;

	Vector vecFudge( fudge, fudge, fudge );
	mins -= vecFudge;
	maxs += vecFudge;
	
	// the near plane does cross the z value for the visible water volume.  Call into
	// the engine to find out if the near plane intersects the water volume.
	return render->DoesBoxIntersectWaterVolume( mins, maxs, leafWaterDataID );
} 

#ifdef PORTAL 

//-----------------------------------------------------------------------------
// Purpose: Draw the scene during another draw scene call. We must draw our portals
//			after opaques but before translucents, so this ViewDrawScene resets the view
//			and doesn't flag the rendering as ended when it ends.
// Input  : bDrawSkybox - do we draw the skybox
//			&view - the camera view to render from
//			nClearFlags -  how to clear the buffer
//-----------------------------------------------------------------------------
void CViewRender::ViewDrawScene_PortalStencil( const CViewSetup &viewIn, ViewCustomVisibility_t *pCustomVisibility )
{
	VPROF( "CViewRender::ViewDrawScene_PortalStencil" );

	CViewSetup view( viewIn );

	// Record old view stats
	Vector vecOldOrigin = CurrentViewOrigin();
	QAngle vecOldAngles = CurrentViewAngles();

	int iCurrentViewID = g_CurrentViewID;
	int iRecursionLevel = g_pPortalRender->GetViewRecursionLevel();
	Assert( iRecursionLevel > 0 );

	//get references to reflection textures
	CTextureReference pPrimaryWaterReflectionTexture;
	pPrimaryWaterReflectionTexture.Init( GetWaterReflectionTexture() );
	CTextureReference pReplacementWaterReflectionTexture;
	pReplacementWaterReflectionTexture.Init( portalrendertargets->GetWaterReflectionTextureForStencilDepth( iRecursionLevel ) );

	//get references to refraction textures
	CTextureReference pPrimaryWaterRefractionTexture;
	pPrimaryWaterRefractionTexture.Init( GetWaterRefractionTexture() );
	CTextureReference pReplacementWaterRefractionTexture;
	pReplacementWaterRefractionTexture.Init( portalrendertargets->GetWaterRefractionTextureForStencilDepth( iRecursionLevel ) );


	//swap texture contents for the primary render targets with those we set aside for this recursion level
	if( pReplacementWaterReflectionTexture != NULL )
		pPrimaryWaterReflectionTexture->SwapContents( pReplacementWaterReflectionTexture );

	if( pReplacementWaterRefractionTexture != NULL )
		pPrimaryWaterRefractionTexture->SwapContents( pReplacementWaterRefractionTexture );

	bool bDrew3dSkybox = false;
	SkyboxVisibility_t nSkyboxVisible = SKYBOX_NOT_VISIBLE;
	int iClearFlags = 0;

	Draw3dSkyboxworld_Portal( view, iClearFlags, bDrew3dSkybox, nSkyboxVisible );

	bool drawSkybox = r_skybox.GetBool();
	if ( bDrew3dSkybox || ( nSkyboxVisible == SKYBOX_NOT_VISIBLE ) )
	{
		drawSkybox = false;
	}

	//generate unique view ID's for each stencil view
	view_id_t iNewViewID = (view_id_t)g_pPortalRender->GetCurrentViewId();
	SetupCurrentView( view.origin, view.angles, (view_id_t)iNewViewID );
	
	// update vis data
	unsigned int visFlags;
	SetupVis( view, visFlags, pCustomVisibility );

	VisibleFogVolumeInfo_t fogInfo;
	if( g_pPortalRender->GetViewRecursionLevel() == 0 )
	{
		render->GetVisibleFogVolume( view.origin, &fogInfo );
	}
	else
	{
		render->GetVisibleFogVolume( g_pPortalRender->GetExitPortalFogOrigin(), &fogInfo );
	}

	WaterRenderInfo_t waterInfo;
	DetermineWaterRenderInfo( fogInfo, waterInfo );

	if ( waterInfo.m_bCheapWater )
	{		     
		cplane_t glassReflectionPlane;
		if ( IsReflectiveGlassInView( viewIn, glassReflectionPlane ) )
		{								    
			CRefPtr<CReflectiveGlassView> pGlassReflectionView = new CReflectiveGlassView( this );
			pGlassReflectionView->Setup( viewIn, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR | VIEW_CLEAR_OBEY_STENCIL, drawSkybox, fogInfo, waterInfo, glassReflectionPlane );
			AddViewToScene( pGlassReflectionView );

			CRefPtr<CRefractiveGlassView> pGlassRefractionView = new CRefractiveGlassView( this );
			pGlassRefractionView->Setup( viewIn, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR | VIEW_CLEAR_OBEY_STENCIL, drawSkybox, fogInfo, waterInfo, glassReflectionPlane );
			AddViewToScene( pGlassRefractionView );
		}

		CSimpleWorldView *pClientView = new CSimpleWorldView( this );
		pClientView->Setup( view, VIEW_CLEAR_OBEY_STENCIL, drawSkybox, fogInfo, waterInfo, pCustomVisibility );
		AddViewToScene( pClientView );
		SafeRelease( pClientView );
	}
	else
	{
		// We can see water of some sort
		if ( !fogInfo.m_bEyeInFogVolume )
		{
			CRefPtr<CAboveWaterView> pAboveWaterView = new CAboveWaterView( this );
			pAboveWaterView->Setup( viewIn, drawSkybox, fogInfo, waterInfo );
			AddViewToScene( pAboveWaterView );
		}
		else
		{
			CRefPtr<CUnderWaterView> pUnderWaterView = new CUnderWaterView( this );
			pUnderWaterView->Setup( viewIn, drawSkybox, fogInfo, waterInfo );
			AddViewToScene( pUnderWaterView );
		}
	}

	// Disable fog for the rest of the stuff
	DisableFog();

	CGlowOverlay::DrawOverlays( view.m_bCacheFullSceneState );

	// Draw rain..
	DrawPrecipitation();

	//prerender version only
	// issue the pixel visibility tests
	PixelVisibility_EndCurrentView();

	// Make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();

	// Debugging info goes over the top
	CDebugViewRender::Draw3DDebuggingInfo( view );

	// Return to the previous view
	SetupCurrentView( vecOldOrigin, vecOldAngles, (view_id_t)iCurrentViewID );
	g_CurrentViewID = iCurrentViewID; //just in case the cast to view_id_t screwed up the id #


	//swap back the water render targets
	if( pReplacementWaterReflectionTexture != NULL )
		pPrimaryWaterReflectionTexture->SwapContents( pReplacementWaterReflectionTexture );

	if( pReplacementWaterRefractionTexture != NULL )
		pPrimaryWaterRefractionTexture->SwapContents( pReplacementWaterRefractionTexture );
}

void CViewRender::Draw3dSkyboxworld_Portal( const CViewSetup &view, int &nClearFlags, bool &bDrew3dSkybox, SkyboxVisibility_t &nSkyboxVisible, ITexture *pRenderTarget ) 
{ 
	CRefPtr<CPortalSkyboxView> pSkyView = new CPortalSkyboxView( this ); 
	if ( ( bDrew3dSkybox = pSkyView->Setup( view, &nClearFlags, &nSkyboxVisible, pRenderTarget ) ) == true )
	{
		AddViewToScene( pSkyView );
	}
}

#endif //PORTAL

//-----------------------------------------------------------------------------
// Methods related to controlling the cheap water distance
//-----------------------------------------------------------------------------
void CViewRender::SetCheapWaterStartDistance( float flCheapWaterStartDistance )
{
	m_flCheapWaterStartDistance = flCheapWaterStartDistance;
}

void CViewRender::SetCheapWaterEndDistance( float flCheapWaterEndDistance )
{
	m_flCheapWaterEndDistance = flCheapWaterEndDistance;
}

void CViewRender::GetWaterLODParams( float &flCheapWaterStartDistance, float &flCheapWaterEndDistance )
{
	flCheapWaterStartDistance = m_flCheapWaterStartDistance;
	flCheapWaterEndDistance = m_flCheapWaterEndDistance;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &view - 
//			&introData - 
//-----------------------------------------------------------------------------
void CViewRender::ViewDrawScene_Intro( const CViewSetup &viewRender, int nClearFlags, const IntroData_t &introData )
{
	VPROF( "CViewRender::ViewDrawScene" );

	CMatRenderContextPtr pRenderContext( materials );

	// this allows the refract texture to be updated once per *scene* on 360
	// (e.g. once for a monitor scene and once for the main scene)
	g_viewscene_refractUpdateFrame = gpGlobals->framecount - 1;

	// -----------------------------------------------------------------------
	// Set the clear color to black since we are going to be adding up things
	// in the frame buffer.
	// -----------------------------------------------------------------------
	// Clear alpha to 255 so that masking with the vortigaunts (0) works properly.
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
	
	// -----------------------------------------------------------------------
	// Draw the primary scene and copy it to the first framebuffer texture
	// -----------------------------------------------------------------------	
	unsigned int visFlags;

	// NOTE: We only increment this once since time doesn't move forward.
	ParticleMgr()->IncrementFrameCode();

	if( introData.m_bDrawPrimary  )
	{
		CViewSetup playerView( viewRender );
		playerView.origin = introData.m_vecCameraView;
		playerView.angles = introData.m_vecCameraViewAngles;
		if ( introData.m_playerViewFOV )
		{
			playerView.fov = ScaleFOVByWidthRatio( introData.m_playerViewFOV, engine->GetScreenAspectRatio() / ( 4.0f / 3.0f ) );
		}

		g_pClientShadowMgr->PreRender();

		// Shadowed flashlights supported on ps_2_b and up...
		if ( r_flashlightdepthtexture.GetBool() )
		{
			g_pClientShadowMgr->ComputeShadowDepthTextures( playerView );
		}

		SetupCurrentView( playerView.origin, playerView.angles, VIEW_INTRO_PLAYER );

		// Invoke pre-render methods
		IGameSystem::PreRenderAllSystems();

		// Start view, clear frame/z buffer if necessary
		SetupVis( playerView, visFlags );
		
		render->Push3DView( playerView, VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH, NULL, GetFrustum() );
		DrawWorldAndEntities( true /* drawSkybox */, playerView, VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH  );
		render->PopView( GetFrustum() );

		// Free shadow depth textures for use in future view
		if ( r_flashlightdepthtexture.GetBool() )
		{
			g_pClientShadowMgr->UnlockAllShadowDepthTextures();
		}
	}
	else
	{
		pRenderContext->ClearBuffers( true, true );
	}
	Rect_t actualRect;
	UpdateScreenEffectTexture( 0, viewRender.x, viewRender.y, viewRender.width, viewRender.height, false, &actualRect );

	g_pClientShadowMgr->PreRender();

	// Shadowed flashlights supported on ps_2_b and up...
	if ( r_flashlightdepthtexture.GetBool() )
	{
		g_pClientShadowMgr->ComputeShadowDepthTextures( viewRender );
	}

	// -----------------------------------------------------------------------
	// Draw the secondary scene and copy it to the second framebuffer texture
	// -----------------------------------------------------------------------
	SetupCurrentView( viewRender.origin, viewRender.angles, VIEW_INTRO_CAMERA );

	// Invoke pre-render methods
	IGameSystem::PreRenderAllSystems();

	// Start view, clear frame/z buffer if necessary
	SetupVis( viewRender, visFlags );

	// Clear alpha to 255 so that masking with the vortigaunts (0) works properly.
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );

	DrawWorldAndEntities( true /* drawSkybox */, viewRender, VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH  );

	UpdateScreenEffectTexture( 1, viewRender.x, viewRender.y, viewRender.width, viewRender.height );

	// -----------------------------------------------------------------------
	// Draw quads on the screen for each screenspace pass.
	// -----------------------------------------------------------------------
	// Find the material that we use to render the overlays
	IMaterial *pOverlayMaterial = materials->FindMaterial( "scripted/intro_screenspaceeffect", TEXTURE_GROUP_OTHER );
	IMaterialVar *pModeVar = pOverlayMaterial->FindVar( "$mode", NULL );
	IMaterialVar *pAlphaVar = pOverlayMaterial->FindVar( "$alpha", NULL );

	pRenderContext->ClearBuffers( true, true );
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();
	
	int passID;
	for( passID = 0; passID < introData.m_Passes.Count(); passID++ )
	{
		const IntroDataBlendPass_t& pass = introData.m_Passes[passID];
		if ( pass.m_Alpha == 0 )
			continue;

		// Pick one of the blend modes for the material.
		if ( pass.m_BlendMode >= 0 && pass.m_BlendMode <= 9 )
		{
			pModeVar->SetIntValue( pass.m_BlendMode );
		}
		else
		{
			Assert(0);
		}
		// Set the alpha value for the material.
		pAlphaVar->SetFloatValue( pass.m_Alpha );
		
		// Draw a quad for this pass.
		ITexture *pTexture = GetFullFrameFrameBufferTexture( 0 );
		pRenderContext->DrawScreenSpaceRectangle( pOverlayMaterial, 0, 0, viewRender.width, viewRender.height,
											actualRect.x, actualRect.y, actualRect.x+actualRect.width-1, actualRect.y+actualRect.height-1, 
											pTexture->GetActualWidth(), pTexture->GetActualHeight() );
	}
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();
	
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
	
	// Draw the starfield
	// FIXME
	// blur?
	
	// Disable fog for the rest of the stuff
	DisableFog();
	
	// Here are the overlays...
	CGlowOverlay::DrawOverlays( viewRender.m_bCacheFullSceneState );

	// issue the pixel visibility tests
	PixelVisibility_EndCurrentView();

	// And here are the screen-space effects
	PerformScreenSpaceEffects( 0, 0, viewRender.width, viewRender.height );

	// Make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();

	// Debugging info goes over the top
	CDebugViewRender::Draw3DDebuggingInfo( viewRender );

	// Let the particle manager simulate things that haven't been simulated.
	ParticleMgr()->PostRender();

	FinishCurrentView();

	// Free shadow depth textures for use in future view
	if ( r_flashlightdepthtexture.GetBool() )
	{
		g_pClientShadowMgr->UnlockAllShadowDepthTextures();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets up scene and renders camera view
// Input  : cameraNum - 
//			&cameraView
//			*localPlayer - 
//			x - 
//			y - 
//			width - 
//			height - 
//			highend - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CViewRender::DrawOneMonitor( ITexture *pRenderTarget, int cameraNum, C_PointCamera *pCameraEnt, 
	const CViewSetup &cameraView, C_BasePlayer *localPlayer, int x, int y, int width, int height )
{
#ifdef USE_MONITORS
	VPROF_INCREMENT_COUNTER( "cameras rendered", 1 );
	// Setup fog state for the camera.
	fogparams_t oldFogParams;
	float flOldZFar = 0.0f;

	bool fogEnabled = pCameraEnt->IsFogEnabled();

	CViewSetup monitorView = cameraView;

	fogparams_t *pFogParams = NULL;

	if ( fogEnabled )
	{	
		if ( !localPlayer )
			return false;

		pFogParams = localPlayer->GetFogParams();

		// Save old fog data.
		oldFogParams = *pFogParams;
		flOldZFar = cameraView.zFar;

		pFogParams->enable = true;
		pFogParams->start = pCameraEnt->GetFogStart();
		pFogParams->end = pCameraEnt->GetFogEnd();
		pFogParams->farz = pCameraEnt->GetFogEnd();
		pFogParams->maxdensity = pCameraEnt->GetFogMaxDensity();
		pFogParams->radial = pCameraEnt->GetFogRadial();

		unsigned char r, g, b;
		pCameraEnt->GetFogColor( r, g, b );
		pFogParams->colorPrimary.SetR( r );
		pFogParams->colorPrimary.SetG( g );
		pFogParams->colorPrimary.SetB( b );

		monitorView.zFar = pCameraEnt->GetFogEnd();
	}

	monitorView.width = width;
	monitorView.height = height;
	monitorView.x = x;
	monitorView.y = y;
	monitorView.origin = pCameraEnt->GetAbsOrigin();
	monitorView.angles = pCameraEnt->GetAbsAngles();
	monitorView.fov = pCameraEnt->GetFOV();
	monitorView.m_bOrtho = false;
	monitorView.m_flAspectRatio = pCameraEnt->UseScreenAspectRatio() ? 0.0f : 1.0f;
	monitorView.m_bViewToProjectionOverride = false;

	// @MULTICORE (toml 8/11/2006): this should be a renderer....
	Frustum frustum;
 	render->Push3DView( monitorView, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, pRenderTarget, (VPlane *)frustum );
	ViewDrawScene( false, SKYBOX_2DSKYBOX_VISIBLE, monitorView, 0, VIEW_MONITOR );
 	render->PopView( frustum );

	// Reset the world fog parameters.
	if ( fogEnabled )
	{
		if ( pFogParams )
		{
			*pFogParams = oldFogParams;
		}
		monitorView.zFar = flOldZFar;
	}
#endif // USE_MONITORS
	return true;
}

void CViewRender::DrawMonitors( const CViewSetup &cameraView )
{
#ifdef PORTAL
	g_pPortalRender->DrawPortalsToTextures( this, cameraView );
#endif

#ifdef USE_MONITORS

	// Early out if no cameras
	C_PointCamera *pCameraEnt = GetPointCameraList();
	if ( !pCameraEnt )
		return;

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	g_bRenderingCameraView = true;

	// FIXME: this should check for the ability to do a render target maybe instead.
	// FIXME: shouldn't have to truck through all of the visible entities for this!!!!
	ITexture *pCameraTarget = GetCameraTexture();
	int width = pCameraTarget->GetActualWidth();
	int height = pCameraTarget->GetActualHeight();

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	

#ifdef TF_CLIENT_DLL
	CTFPlayer* pLocalTFPlayer = CTFPlayer::GetLocalTFPlayer();

	bool bNeedToToggleForceDraw = !( pLocalTFPlayer && pLocalTFPlayer->m_Local.m_bForceLocalPlayerDraw );
	bool bNeedToToggleForceDrawBack = false;
#endif

	int cameraNum;
	for ( cameraNum = 0; pCameraEnt != NULL; pCameraEnt = pCameraEnt->m_pNext )
	{
		if ( !pCameraEnt->IsActive() || pCameraEnt->IsDormant() )
			continue;

#ifdef TF_CLIENT_DLL
		if ( bNeedToToggleForceDraw && pLocalTFPlayer )
		{
			pLocalTFPlayer->ForceTempForceDraw( true );

			bNeedToToggleForceDrawBack = true;
			bNeedToToggleForceDraw = false;
		}
#endif

		if ( !DrawOneMonitor( pCameraTarget, cameraNum, pCameraEnt, cameraView, player, 0, 0, width, height ) )
			continue;

		++cameraNum;
	}

#ifdef TF_CLIENT_DLL
	if ( bNeedToToggleForceDrawBack && pLocalTFPlayer )
	{
		pLocalTFPlayer->ForceTempForceDraw( false );
	}
#endif

	if ( IsX360() && cameraNum > 0 )
	{
		// resolve render target to system memory texture
		// resolving *after* all monitors drawn to ensure a single blit using fastest resolve path
		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->PushRenderTargetAndViewport( pCameraTarget );
		pRenderContext->CopyRenderTargetToTextureEx( pCameraTarget, 0, NULL, NULL );
		pRenderContext->PopRenderTargetAndViewport();
	}

	g_bRenderingCameraView = false;

#endif // USE_MONITORS
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

ClientWorldListInfo_t *ClientWorldListInfo_t::AllocPooled( const ClientWorldListInfo_t &exemplar )
{
	size_t nBytes = AlignValue( ( exemplar.m_LeafCount * ((sizeof(LeafIndex_t) * 2) + sizeof(LeafFogVolume_t)) ), 4096 );

	ClientWorldListInfo_t *pResult = gm_Pool.GetObject();

	byte *pMemory = (byte *)pResult->m_pLeafList;

	if ( pMemory )
	{
		// Previously allocated, add a reference. Otherwise comes out of GetObject as a new object with a refcount of 1
		pResult->AddRef();
	}

	if ( !pMemory || _msize( pMemory ) < nBytes )
	{
		pMemory = (byte *)realloc( pMemory, nBytes );
	}

	pResult->m_pLeafList = (LeafIndex_t*)pMemory;
	pResult->m_pLeafFogVolume = (LeafFogVolume_t*)( pMemory + exemplar.m_LeafCount * sizeof(LeafIndex_t) );
	pResult->m_pActualLeafIndex = (LeafIndex_t*)( (byte *)( pResult->m_pLeafFogVolume ) + exemplar.m_LeafCount * sizeof(LeafFogVolume_t) );

	pResult->m_bPooledAlloc = true;

	return pResult;
}

bool ClientWorldListInfo_t::OnFinalRelease()
{
	if ( m_bPooledAlloc )
	{
		Assert( m_pLeafList );
		gm_Pool.PutObject( this );
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CBase3dView::CBase3dView( CViewRender *pMainView ) :
m_pMainView( pMainView ),
m_Frustum( pMainView->m_Frustum )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnt - 
// Output : int
//-----------------------------------------------------------------------------
VPlane* CBase3dView::GetFrustum()
{
	// The frustum is only valid while in a RenderView call.
	// @MULTICORE (toml 8/11/2006): reimplement this when ready -- Assert(g_bRenderingView || g_bRenderingCameraView || g_bRenderingScreenshot);	
	return m_Frustum;
}


CObjectPool<ClientWorldListInfo_t> ClientWorldListInfo_t::gm_Pool;


//-----------------------------------------------------------------------------
// Base class for 3d views
//-----------------------------------------------------------------------------
CRendering3dView::CRendering3dView(CViewRender *pMainView) :
	CBase3dView( pMainView ),
	m_pWorldRenderList( NULL ), 
	m_pRenderablesList( NULL ), 
	m_pWorldListInfo( NULL ), 
	m_pCustomVisibility( NULL ),
	m_DrawFlags( 0 ),
	m_ClearFlags( 0 )
{
}


//-----------------------------------------------------------------------------
// Sort entities in a back-to-front ordering
//-----------------------------------------------------------------------------
void CRendering3dView::Setup( const CViewSetup &setup )
{
	// @MULTICORE (toml 8/15/2006): don't reset if parameters don't require it. For now, just reset
	memcpy( static_cast<CViewSetup *>(this), &setup, sizeof( setup ) );
	ReleaseLists();

	m_pRenderablesList = new CClientRenderablesList; 
	m_pCustomVisibility = NULL;
}


//-----------------------------------------------------------------------------
// Sort entities in a back-to-front ordering
//-----------------------------------------------------------------------------
void CRendering3dView::ReleaseLists()
{
	SafeRelease( m_pWorldRenderList );
	SafeRelease( m_pRenderablesList );
	SafeRelease( m_pWorldListInfo );
	m_pCustomVisibility = NULL;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CRendering3dView::SetupRenderablesList( int viewID )
{
	VPROF( "CViewRender::SetupRenderablesList" );

	// Clear the list.
	int i;
	for( i=0; i < RENDER_GROUP_COUNT; i++ )
	{
		m_pRenderablesList->m_RenderGroupCounts[i] = 0;
	}

	// Now collate the entities in the leaves.
	if( m_pMainView->ShouldDrawEntities() )
	{
		// Precache information used commonly in CollateRenderables
		SetupRenderInfo_t setupInfo;
		setupInfo.m_pWorldListInfo = m_pWorldListInfo;
		setupInfo.m_nRenderFrame = m_pMainView->BuildRenderablesListsNumber();	// only one incremented?
		setupInfo.m_nDetailBuildFrame = m_pMainView->BuildWorldListsNumber();	//
		setupInfo.m_pRenderList = m_pRenderablesList;
		setupInfo.m_bDrawDetailObjects = g_pClientMode->ShouldDrawDetailObjects() && r_DrawDetailProps.GetInt();
		setupInfo.m_bDrawTranslucentObjects = (viewID != VIEW_SHADOW_DEPTH_TEXTURE);

		setupInfo.m_vecRenderOrigin = origin;
		setupInfo.m_vecRenderForward = CurrentViewForward();

		float fMaxDist = cl_maxrenderable_dist.GetFloat();

		// Shadowing light typically has a smaller farz than cl_maxrenderable_dist
		setupInfo.m_flRenderDistSq = (viewID == VIEW_SHADOW_DEPTH_TEXTURE) ? MIN(zFar, fMaxDist) : fMaxDist;
		setupInfo.m_flRenderDistSq *= setupInfo.m_flRenderDistSq;

		ClientLeafSystem()->BuildRenderablesList( setupInfo );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Builds lists of things to render in the world, called once per view
//-----------------------------------------------------------------------------
void CRendering3dView::UpdateRenderablesOpacity()
{
	// Compute the prop opacity based on the view position and fov zoom scale
	float flFactor = 1.0f;
	C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
	if ( pLocal )
	{
		flFactor = pLocal->GetFOVDistanceAdjustFactor();
	}

	if ( cl_leveloverview.GetFloat() > 0 )
	{
		// disable prop fading
		flFactor = -1;
	}

	// When zoomed in, tweak the opacity to stay visible from further away
	staticpropmgr->ComputePropOpacity( origin, flFactor );

	// Build a list of detail props to render
	DetailObjectSystem()->BuildDetailObjectRenderLists( origin );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
// Kinda awkward...three optional parameters at the end...
void CRendering3dView::BuildWorldRenderLists( bool bDrawEntities, int iForceViewLeaf /* = -1 */, 
	bool bUseCacheIfEnabled /* = true */, bool bShadowDepth /* = false */, float *pReflectionWaterHeight /*= NULL*/ )
{
	VPROF_BUDGET( "BuildWorldRenderLists", VPROF_BUDGETGROUP_WORLD_RENDERING );

    // @MULTICORE (toml 8/18/2006): to address....
	extern void UpdateClientRenderableInPVSStatus();
	UpdateClientRenderableInPVSStatus();

	Assert( !m_pWorldRenderList && !m_pWorldListInfo);

	m_pMainView->IncWorldListsNumber();
	// Override vis data if specified this render, otherwise use default behavior with NULL
	VisOverrideData_t* pVisData = ( m_pCustomVisibility && m_pCustomVisibility->m_VisData.m_fDistToAreaPortalTolerance != FLT_MAX ) ?  &m_pCustomVisibility->m_VisData : NULL;
	bool bUseCache = ( bUseCacheIfEnabled && r_worldlistcache.GetBool() );
	if ( !bUseCache || pVisData || !g_WorldListCache.Find( *this, &m_pWorldRenderList, &m_pWorldListInfo ) )
	{
        // @MULTICORE (toml 8/18/2006): when make parallel, will have to change caching to be atomic, where follow ons receive a pointer to a list that is not yet built
		m_pWorldRenderList =  render->CreateWorldList();
		m_pWorldListInfo = new ClientWorldListInfo_t;

		render->BuildWorldLists( m_pWorldRenderList, m_pWorldListInfo, 
			( m_pCustomVisibility ) ? m_pCustomVisibility->m_iForceViewLeaf : iForceViewLeaf, 
			pVisData, bShadowDepth, pReflectionWaterHeight );

		if ( bUseCache && !pVisData )
		{
			g_WorldListCache.Add( *this, m_pWorldRenderList, m_pWorldListInfo );
		}
	}

	if ( bDrawEntities )
	{
		UpdateRenderablesOpacity();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Computes the actual world list info based on the render flags
//-----------------------------------------------------------------------------
void CRendering3dView::PruneWorldListInfo()
{
	// Drawing everything? Just return the world list info as-is 
	int nWaterDrawFlags = m_DrawFlags & (DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER);
	if ( nWaterDrawFlags == (DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER) )
	{
		return;
	}

	ClientWorldListInfo_t *pNewInfo;
	// Only allocate memory if actually will draw something
	if ( m_pWorldListInfo->m_LeafCount > 0 && nWaterDrawFlags )
	{
		pNewInfo = ClientWorldListInfo_t::AllocPooled( *m_pWorldListInfo );
	}
	else
	{
		pNewInfo = new ClientWorldListInfo_t;
	}

	pNewInfo->m_ViewFogVolume = m_pWorldListInfo->m_ViewFogVolume;
	pNewInfo->m_LeafCount = 0;

	// Not drawing anything? Then don't bother with renderable lists
	if ( nWaterDrawFlags != 0 )
	{
		// Create a sub-list based on the actual leaves being rendered
		bool bRenderingUnderwater = (nWaterDrawFlags & DF_RENDER_UNDERWATER) != 0;
		for ( int i = 0; i < m_pWorldListInfo->m_LeafCount; ++i )
		{
			bool bLeafIsUnderwater = ( m_pWorldListInfo->m_pLeafFogVolume[i] != -1 );
			if ( bRenderingUnderwater == bLeafIsUnderwater )
			{
				pNewInfo->m_pLeafList[ pNewInfo->m_LeafCount ] = m_pWorldListInfo->m_pLeafList[ i ];
				pNewInfo->m_pLeafFogVolume[ pNewInfo->m_LeafCount ] = m_pWorldListInfo->m_pLeafFogVolume[ i ];
				pNewInfo->m_pActualLeafIndex[ pNewInfo->m_LeafCount ] = i;
				++pNewInfo->m_LeafCount;
			}
		}
	}

	m_pWorldListInfo->Release();
	m_pWorldListInfo = pNewInfo;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static inline void UpdateBrushModelLightmap( IClientRenderable *pEnt )
{
	model_t *pModel = ( model_t * )pEnt->GetModel();
	render->UpdateBrushModelLightmap( pModel, pEnt );
}


void CRendering3dView::BuildRenderableRenderLists( int viewID )
{
	MDLCACHE_CRITICAL_SECTION();

	if ( viewID != VIEW_SHADOW_DEPTH_TEXTURE )
	{
		render->BeginUpdateLightmaps();
	}

	m_pMainView->IncRenderablesListsNumber();

	ClientWorldListInfo_t& info = *m_pWorldListInfo;

	// For better sorting, find out the leaf *nearest* to the camera
	// and render translucent objects as if they are in that leaf.
	if( m_pMainView->ShouldDrawEntities() && ( viewID != VIEW_SHADOW_DEPTH_TEXTURE ) )
	{
		ClientLeafSystem()->ComputeTranslucentRenderLeaf( 
			info.m_LeafCount, info.m_pLeafList, info.m_pLeafFogVolume, m_pMainView->BuildRenderablesListsNumber(), viewID );
	}

	SetupRenderablesList( viewID );

	if ( viewID == VIEW_MAIN )
	{
		StudioStats_FindClosestEntity( m_pRenderablesList );
	}

	if ( viewID != VIEW_SHADOW_DEPTH_TEXTURE )
	{
		// update lightmap on brush models if necessary
		CClientRenderablesList::CEntry *pEntities = m_pRenderablesList->m_RenderGroups[RENDER_GROUP_OPAQUE_BRUSH];
		int nOpaque = m_pRenderablesList->m_RenderGroupCounts[RENDER_GROUP_OPAQUE_BRUSH];
		int i;
		for( i=0; i < nOpaque; ++i )
		{
			Assert(pEntities[i].m_TwoPass==0);
			UpdateBrushModelLightmap( pEntities[i].m_pRenderable );
		}

		// update lightmap on brush models if necessary
		pEntities = m_pRenderablesList->m_RenderGroups[RENDER_GROUP_TRANSLUCENT_ENTITY];
		int nTranslucent = m_pRenderablesList->m_RenderGroupCounts[RENDER_GROUP_TRANSLUCENT_ENTITY];
		for( i=0; i < nTranslucent; ++i )
		{
			const model_t *pModel = pEntities[i].m_pRenderable->GetModel();
			if( pModel )
			{
				int nModelType = modelinfo->GetModelType( pModel );
				if( nModelType == mod_brush )
				{
					UpdateBrushModelLightmap( pEntities[i].m_pRenderable );
				}
			}
		}

		render->EndUpdateLightmaps();
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CRendering3dView::DrawWorld( float waterZAdjust )
{
	VPROF_INCREMENT_COUNTER( "RenderWorld", 1 );
	VPROF_BUDGET( "DrawWorld", VPROF_BUDGETGROUP_WORLD_RENDERING );
	if( !r_drawopaqueworld.GetBool() )
	{
		return;
	}

	unsigned long engineFlags = BuildEngineDrawWorldListFlags( m_DrawFlags );

	render->DrawWorldLists( m_pWorldRenderList, engineFlags, waterZAdjust );
}


CMaterialReference g_material_WriteZ; //init'ed on by CViewRender::Init()

//-----------------------------------------------------------------------------
// Fakes per-entity clip planes on cards that don't support user clip planes.
//  Achieves the effect by drawing an invisible box that writes to the depth buffer
//  around the clipped area. It's not perfect, but better than nothing.
//-----------------------------------------------------------------------------
static void DrawClippedDepthBox( IClientRenderable *pEnt, float *pClipPlane )
{
//#define DEBUG_DRAWCLIPPEDDEPTHBOX //uncomment to draw the depth box as a colorful box

	static const int iQuads[6][5] = {	{ 0, 4, 6, 2, 0 }, //always an extra copy of first index at end to make some algorithms simpler
										{ 3, 7, 5, 1, 3 },
										{ 1, 5, 4, 0, 1 },
										{ 2, 6, 7, 3, 2 },
										{ 0, 2, 3, 1, 0 },
										{ 5, 7, 6, 4, 5 } };

	static const int iLines[12][2] = {	{ 0, 1 },
										{ 0, 2 },
										{ 0, 4 },
										{ 1, 3 },
										{ 1, 5 },
										{ 2, 3 },
										{ 2, 6 },
										{ 3, 7 },
										{ 4, 6 },
										{ 4, 5 },
										{ 5, 7 },
										{ 6, 7 } };


#ifdef DEBUG_DRAWCLIPPEDDEPTHBOX
	static const float fColors[6][3] = {	{ 1.0f, 0.0f, 0.0f },
											{ 0.0f, 1.0f, 1.0f },
											{ 0.0f, 1.0f, 0.0f },
											{ 1.0f, 0.0f, 1.0f },
											{ 0.0f, 0.0f, 1.0f },
											{ 1.0f, 1.0f, 0.0f } };
#endif

	
	

	Vector vNormal = *(Vector *)pClipPlane;
	float fPlaneDist = pClipPlane[3];

	Vector vMins, vMaxs;
	pEnt->GetRenderBounds( vMins, vMaxs );

	Vector vOrigin = pEnt->GetRenderOrigin();
	QAngle qAngles = pEnt->GetRenderAngles();
	
	Vector vForward, vUp, vRight;
	AngleVectors( qAngles, &vForward, &vRight, &vUp );

	Vector vPoints[8];
	vPoints[0] = vOrigin + (vForward * vMins.x) + (vRight * vMins.y) + (vUp * vMins.z);
	vPoints[1] = vOrigin + (vForward * vMaxs.x) + (vRight * vMins.y) + (vUp * vMins.z);
	vPoints[2] = vOrigin + (vForward * vMins.x) + (vRight * vMaxs.y) + (vUp * vMins.z);
	vPoints[3] = vOrigin + (vForward * vMaxs.x) + (vRight * vMaxs.y) + (vUp * vMins.z);
	vPoints[4] = vOrigin + (vForward * vMins.x) + (vRight * vMins.y) + (vUp * vMaxs.z);
	vPoints[5] = vOrigin + (vForward * vMaxs.x) + (vRight * vMins.y) + (vUp * vMaxs.z);
	vPoints[6] = vOrigin + (vForward * vMins.x) + (vRight * vMaxs.y) + (vUp * vMaxs.z);
	vPoints[7] = vOrigin + (vForward * vMaxs.x) + (vRight * vMaxs.y) + (vUp * vMaxs.z);

	int iClipped[8];
	float fDists[8];
	for( int i = 0; i != 8; ++i )
	{
		fDists[i] = vPoints[i].Dot( vNormal ) - fPlaneDist;
		iClipped[i] = (fDists[i] > 0.0f) ? 1 : 0;
	}

	Vector vSplitPoints[8][8]; //obviously there are only 12 lines, not 64 lines or 64 split points, but the indexing is way easier like this
	int iLineStates[8][8]; //0 = unclipped, 2 = wholly clipped, 3 = first point clipped, 4 = second point clipped

	//categorize lines and generate split points where needed
	for( int i = 0; i != 12; ++i )
	{
		const int *pPoints = iLines[i];
		int iLineState = (iClipped[pPoints[0]] + iClipped[pPoints[1]]);
		if( iLineState != 1 ) //either both points are clipped, or neither are clipped
		{
			iLineStates[pPoints[0]][pPoints[1]] = 
				iLineStates[pPoints[1]][pPoints[0]] = 
					iLineState;
		}
		else
		{
			//one point is clipped, the other is not
			if( iClipped[pPoints[0]] == 1 )
			{
				//first point was clipped, index 1 has the negative distance
				float fInvTotalDist = 1.0f / (fDists[pPoints[0]] - fDists[pPoints[1]]);
				vSplitPoints[pPoints[0]][pPoints[1]] = 
					vSplitPoints[pPoints[1]][pPoints[0]] =
						(vPoints[pPoints[1]] * (fDists[pPoints[0]] * fInvTotalDist)) - (vPoints[pPoints[0]] * (fDists[pPoints[1]] * fInvTotalDist));
				
				Assert( fabs( vNormal.Dot( vSplitPoints[pPoints[0]][pPoints[1]] ) - fPlaneDist ) < 0.01f );

				iLineStates[pPoints[0]][pPoints[1]] = 3;
				iLineStates[pPoints[1]][pPoints[0]] = 4;
			}
			else
			{
				//second point was clipped, index 0 has the negative distance
				float fInvTotalDist = 1.0f / (fDists[pPoints[1]] - fDists[pPoints[0]]);
				vSplitPoints[pPoints[0]][pPoints[1]] = 
					vSplitPoints[pPoints[1]][pPoints[0]] =
						(vPoints[pPoints[0]] * (fDists[pPoints[1]] * fInvTotalDist)) - (vPoints[pPoints[1]] * (fDists[pPoints[0]] * fInvTotalDist));

				Assert( fabs( vNormal.Dot( vSplitPoints[pPoints[0]][pPoints[1]] ) - fPlaneDist ) < 0.01f );

				iLineStates[pPoints[0]][pPoints[1]] = 4;
				iLineStates[pPoints[1]][pPoints[0]] = 3;
			}
		}
	}


	CMatRenderContextPtr pRenderContext( materials );
	
#ifdef DEBUG_DRAWCLIPPEDDEPTHBOX
	pRenderContext->Bind( materials->FindMaterial( "debug/debugvertexcolor", TEXTURE_GROUP_OTHER ), NULL );
#else
	pRenderContext->Bind( g_material_WriteZ, NULL );
#endif

	CMeshBuilder meshBuilder;
	IMesh* pMesh = pRenderContext->GetDynamicMesh( false );
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 18 ); //6 sides, possible one cut per side. Any side is capable of having 3 tri's. Lots of padding for things that aren't possible

	//going to draw as a collection of triangles, arranged as a triangle fan on each side
	for( int i = 0; i != 6; ++i )
	{
		const int *pPoints = iQuads[i];

		//can't start the fan on a wholly clipped line, so seek to one that isn't
		int j = 0;
		do
		{
			if( iLineStates[pPoints[j]][pPoints[j+1]] != 2 ) //at least part of this line will be drawn
				break;

			++j;
		} while( j != 3 );

		if( j == 3 ) //not enough lines to even form a triangle
			continue;

		float *pStartPoint = 0;
		float *pTriangleFanPoints[4]; //at most, one of our fans will have 5 points total, with the first point being stored separately as pStartPoint
		int iTriangleFanPointCount = 1; //the switch below creates the first for sure
		
		//figure out how to start the fan
		switch( iLineStates[pPoints[j]][pPoints[j+1]] )
		{
		case 0: //uncut
			pStartPoint = &vPoints[pPoints[j]].x;
			pTriangleFanPoints[0] = &vPoints[pPoints[j+1]].x;
			break;

		case 4: //second index was clipped
			pStartPoint = &vPoints[pPoints[j]].x;
			pTriangleFanPoints[0] = &vSplitPoints[pPoints[j]][pPoints[j+1]].x;
			break;

		case 3: //first index was clipped
			pStartPoint = &vSplitPoints[pPoints[j]][pPoints[j+1]].x;
			pTriangleFanPoints[0] = &vPoints[pPoints[j + 1]].x;
			break;

		default:
			Assert( false );
			break;
		};

		for( ++j; j != 3; ++j ) //add end points for the rest of the indices, we're assembling a triangle fan
		{
			switch( iLineStates[pPoints[j]][pPoints[j+1]] )
			{
			case 0: //uncut line, normal endpoint
				pTriangleFanPoints[iTriangleFanPointCount] = &vPoints[pPoints[j+1]].x;
				++iTriangleFanPointCount;
				break;

			case 2: //wholly cut line, no endpoint
				break;

			case 3: //first point is clipped, normal endpoint
				//special case, adds start and end point
				pTriangleFanPoints[iTriangleFanPointCount] = &vSplitPoints[pPoints[j]][pPoints[j+1]].x;
				++iTriangleFanPointCount;

				pTriangleFanPoints[iTriangleFanPointCount] = &vPoints[pPoints[j+1]].x;
				++iTriangleFanPointCount;
				break;

			case 4: //second point is clipped
				pTriangleFanPoints[iTriangleFanPointCount] = &vSplitPoints[pPoints[j]][pPoints[j+1]].x;
				++iTriangleFanPointCount;
				break;

			default:
				Assert( false );
				break;
			};
		}
		
		//special case endpoints, half-clipped lines have a connecting line between them and the next line (first line in this case)
		switch( iLineStates[pPoints[j]][pPoints[j+1]] )
		{
		case 3:
		case 4:
			pTriangleFanPoints[iTriangleFanPointCount] = &vSplitPoints[pPoints[j]][pPoints[j+1]].x;
			++iTriangleFanPointCount;
			break;
		};

		Assert( iTriangleFanPointCount <= 4 );

		//add the fan to the mesh
		int iLoopStop = iTriangleFanPointCount - 1;
		for( int k = 0; k != iLoopStop; ++k )
		{
			meshBuilder.Position3fv( pStartPoint );
#ifdef DEBUG_DRAWCLIPPEDDEPTHBOX
			float fHalfColors[3] = { fColors[i][0] * 0.5f, fColors[i][1] * 0.5f, fColors[i][2] * 0.5f };
			meshBuilder.Color3fv( fHalfColors );
#endif
			meshBuilder.AdvanceVertex();
			
			meshBuilder.Position3fv( pTriangleFanPoints[k] );
#ifdef DEBUG_DRAWCLIPPEDDEPTHBOX
			meshBuilder.Color3fv( fColors[i] );
#endif
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pTriangleFanPoints[k+1] );
#ifdef DEBUG_DRAWCLIPPEDDEPTHBOX
			meshBuilder.Color3fv( fColors[i] );
#endif
			meshBuilder.AdvanceVertex();
		}
	}

	meshBuilder.End();
	pMesh->Draw();
	pRenderContext->Flush( false );
}


//-----------------------------------------------------------------------------
// Draws all opaque renderables in leaves that were rendered
//-----------------------------------------------------------------------------
static inline void DrawOpaqueRenderable( IClientRenderable *pEnt, bool bTwoPass, ERenderDepthMode DepthMode, int nDefaultFlags = 0 )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	float color[3];

	pEnt->GetColorModulation( color );
	render->SetColorModulation(	color );

	int flags = nDefaultFlags | STUDIO_RENDER;
	if ( bTwoPass )
	{
		flags |= STUDIO_TWOPASS;
	}

	if ( DepthMode == DEPTH_MODE_SHADOW )
	{
		flags |= STUDIO_SHADOWDEPTHTEXTURE;
	}
	else if ( DepthMode == DEPTH_MODE_SSA0 )
	{
		flags |= STUDIO_SSAODEPTHTEXTURE;
	}

	float *pRenderClipPlane = NULL;
	if( r_entityclips.GetBool() )
		pRenderClipPlane = pEnt->GetRenderClipPlane();

	if( pRenderClipPlane )	
	{
		CMatRenderContextPtr pRenderContext( materials );
		if( !materials->UsingFastClipping() ) //do NOT change the fast clip plane mid-scene, depth problems result. Regular user clip planes are fine though
			pRenderContext->PushCustomClipPlane( pRenderClipPlane );
		else
			DrawClippedDepthBox( pEnt, pRenderClipPlane );
		Assert( view->GetCurrentlyDrawingEntity() == NULL );
		view->SetCurrentlyDrawingEntity( pEnt->GetIClientUnknown()->GetBaseEntity() );
		pEnt->DrawModel( flags );
		view->SetCurrentlyDrawingEntity( NULL );
		if( pRenderClipPlane && !materials->UsingFastClipping() )	
			pRenderContext->PopCustomClipPlane();
	}
	else
	{
		Assert( view->GetCurrentlyDrawingEntity() == NULL );
		view->SetCurrentlyDrawingEntity( pEnt->GetIClientUnknown()->GetBaseEntity() );
		pEnt->DrawModel( flags );
		view->SetCurrentlyDrawingEntity( NULL );
	}
}

//-------------------------------------


ConVar r_drawopaquestaticpropslast( "r_drawopaquestaticpropslast", "0", 0, "Whether opaque static props are rendered after non-npcs" );

#define DEBUG_BUCKETS 0

#if DEBUG_BUCKETS
ConVar r_drawopaque_old( "r_drawopaque_old", "0", 0, "Whether old unbucketed technique is used" );
ConVar r_drawopaquesbucket( "r_drawopaquesbucket", "0", FCVAR_CHEAT, "Draw only specific bucket: positive - props, negative - ents" );
ConVar r_drawopaquesbucket_stats( "r_drawopaquesbucket_stats", "0", FCVAR_CHEAT, "Draw distribution of props/ents in the buckets" );
#endif


static void SetupBonesOnBaseAnimating( C_BaseAnimating *&pBaseAnimating )
{
	pBaseAnimating->SetupBones( NULL, -1, -1, gpGlobals->curtime );
}


static void DrawOpaqueRenderables_DrawBrushModels( CClientRenderablesList::CEntry *pEntitiesBegin, CClientRenderablesList::CEntry *pEntitiesEnd, ERenderDepthMode DepthMode )
{
	for( CClientRenderablesList::CEntry *itEntity = pEntitiesBegin; itEntity < pEntitiesEnd; ++ itEntity )
	{
		Assert( !itEntity->m_TwoPass );
		DrawOpaqueRenderable( itEntity->m_pRenderable, false, DepthMode );
	}
}

static void DrawOpaqueRenderables_DrawStaticProps( CClientRenderablesList::CEntry *pEntitiesBegin, CClientRenderablesList::CEntry *pEntitiesEnd, ERenderDepthMode DepthMode )
{
	if ( pEntitiesEnd == pEntitiesBegin )
		return;

	float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	render->SetColorModulation(	one );
	render->SetBlend( 1.0f );
	
	const int MAX_STATICS_PER_BATCH = 512;
	IClientRenderable *pStatics[ MAX_STATICS_PER_BATCH ];
	
	int numScheduled = 0, numAvailable = MAX_STATICS_PER_BATCH;

	for( CClientRenderablesList::CEntry *itEntity = pEntitiesBegin; itEntity < pEntitiesEnd; ++ itEntity )
	{
		if ( itEntity->m_pRenderable )
			/**/;
		else
			continue;

		if ( g_pStudioStatsEntity != NULL && g_CurrentViewID == VIEW_MAIN && itEntity->m_pRenderable == g_pStudioStatsEntity )
		{
			DrawOpaqueRenderable( itEntity->m_pRenderable, false, DepthMode, STUDIO_GENERATE_STATS );
			continue;
		}

		pStatics[ numScheduled ++ ] = itEntity->m_pRenderable;
		if ( -- numAvailable > 0 )
			continue; // place a hint for compiler to predict more common case in the loop
		
		staticpropmgr->DrawStaticProps( pStatics, numScheduled, DepthMode, vcollide_wireframe.GetBool() );
		numScheduled = 0;
		numAvailable = MAX_STATICS_PER_BATCH;
	}
	
	if ( numScheduled )
		staticpropmgr->DrawStaticProps( pStatics, numScheduled, DepthMode, vcollide_wireframe.GetBool() );
}

static void DrawOpaqueRenderables_Range( CClientRenderablesList::CEntry *pEntitiesBegin, CClientRenderablesList::CEntry *pEntitiesEnd, ERenderDepthMode DepthMode )
{
	for( CClientRenderablesList::CEntry *itEntity = pEntitiesBegin; itEntity < pEntitiesEnd; ++ itEntity )
	{
		if ( itEntity->m_pRenderable )
			DrawOpaqueRenderable( itEntity->m_pRenderable, ( itEntity->m_TwoPass != 0 ), DepthMode );
	}
}

void CRendering3dView::DrawOpaqueRenderables( ERenderDepthMode DepthMode )
{
	VPROF_BUDGET("CViewRender::DrawOpaqueRenderables", "DrawOpaqueRenderables" );

	if( !r_drawopaquerenderables.GetBool() )
		return;

	if( !m_pMainView->ShouldDrawEntities() )
		return;

	render->SetBlend( 1 );

	//
	// Prepare to iterate over all leaves that were visible, and draw opaque things in them.	
	//
	RopeManager()->ResetRenderCache();
	g_pParticleSystemMgr->ResetRenderCache();

	//bool const bDrawopaquestaticpropslast = r_drawopaquestaticpropslast.GetBool();

	
	//
	// First do the brush models
	//
	{
		CClientRenderablesList::CEntry *pEntitiesBegin, *pEntitiesEnd;
		pEntitiesBegin = m_pRenderablesList->m_RenderGroups[RENDER_GROUP_OPAQUE_BRUSH];
		pEntitiesEnd = pEntitiesBegin + m_pRenderablesList->m_RenderGroupCounts[RENDER_GROUP_OPAQUE_BRUSH];
		DrawOpaqueRenderables_DrawBrushModels( pEntitiesBegin, pEntitiesEnd, DepthMode );
	}



#if DEBUG_BUCKETS
	{
		con_nprint_s nxPrn = { 0 };
		nxPrn.index = 16;
		nxPrn.time_to_live = -1;
		nxPrn.color[0] = 0.9f, nxPrn.color[1] = 1.0f, nxPrn.color[2] = 0.9f;
		nxPrn.fixed_width_font = true;

		engine->Con_NXPrintf( &nxPrn, "Draw Opaque Technique : NEW" );
		if ( r_drawopaque_old.GetBool() )
		{

			engine->Con_NXPrintf( &nxPrn, "Draw Opaque Technique : OLD" );

			// now the static props
			{
				for ( int bucket = RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS - 1; bucket -- > 0; )
				{
					CClientRenderablesList::CEntry
						* const pEntitiesBegin = m_pRenderablesList->m_RenderGroups[ RENDER_GROUP_OPAQUE_STATIC_HUGE + 2 * bucket ],
						* const pEntitiesEnd = pEntitiesBegin + m_pRenderablesList->m_RenderGroupCounts[ RENDER_GROUP_OPAQUE_STATIC_HUGE + 2 * bucket ];
					DrawOpaqueRenderables_DrawStaticProps( pEntitiesBegin, pEntitiesEnd, bShadowDepth );
				}
			}

			// now the other opaque entities
			for ( int bucket = RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS - 1; bucket -- > 0; )
			{
				CClientRenderablesList::CEntry
					* const pEntitiesBegin = m_pRenderablesList->m_RenderGroups[ RENDER_GROUP_OPAQUE_ENTITY_HUGE + 2 * bucket ],
					* const pEntitiesEnd = pEntitiesBegin + m_pRenderablesList->m_RenderGroupCounts[ RENDER_GROUP_OPAQUE_ENTITY_HUGE + 2 * bucket ];
				DrawOpaqueRenderables_Range( pEntitiesBegin, pEntitiesEnd, bShadowDepth );
			}

			//
			// Ropes and particles
			//
			RopeManager()->DrawRenderCache( bShadowDepth );
			g_pParticleSystemMgr->DrawRenderCache( bShadowDepth );

			return;
		}
	}
#endif



	//
	// Sort everything that's not a static prop
	//
	int numOpaqueEnts = 0;
	for ( int bucket = 0; bucket < RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS; ++ bucket )
		numOpaqueEnts += m_pRenderablesList->m_RenderGroupCounts[ RENDER_GROUP_OPAQUE_ENTITY_HUGE + 2 * bucket ];

	CUtlVector< C_BaseAnimating * > arrBoneSetupNpcsLast( (C_BaseAnimating **)_alloca( numOpaqueEnts * sizeof( C_BaseAnimating * ) ), numOpaqueEnts, numOpaqueEnts );
	CUtlVector< CClientRenderablesList::CEntry > arrRenderEntsNpcsFirst( (CClientRenderablesList::CEntry *)_alloca( numOpaqueEnts * sizeof( CClientRenderablesList::CEntry ) ), numOpaqueEnts, numOpaqueEnts );
	int numNpcs = 0, numNonNpcsAnimating = 0;

	for ( int bucket = 0; bucket < RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS; ++ bucket )
	{
		for( CClientRenderablesList::CEntry
			* const pEntitiesBegin = m_pRenderablesList->m_RenderGroups[ RENDER_GROUP_OPAQUE_ENTITY_HUGE + 2 * bucket ],
			* const pEntitiesEnd = pEntitiesBegin + m_pRenderablesList->m_RenderGroupCounts[ RENDER_GROUP_OPAQUE_ENTITY_HUGE + 2 * bucket ],
			*itEntity = pEntitiesBegin; itEntity < pEntitiesEnd; ++ itEntity )
		{
			C_BaseEntity *pEntity = itEntity->m_pRenderable ? itEntity->m_pRenderable->GetIClientUnknown()->GetBaseEntity() : NULL;
			if ( pEntity )
			{
				if ( pEntity->IsNPC() )
				{
					C_BaseAnimating *pba = assert_cast<C_BaseAnimating *>( pEntity );
					arrRenderEntsNpcsFirst[ numNpcs ++ ] = *itEntity;
					arrBoneSetupNpcsLast[ numOpaqueEnts - numNpcs ] = pba;
					
					itEntity->m_pRenderable = NULL;		// We will render NPCs separately
					itEntity->m_RenderHandle = NULL;
					
					continue;
				}
				else if ( pEntity->GetBaseAnimating() )
				{
					C_BaseAnimating *pba = assert_cast<C_BaseAnimating *>( pEntity );
					arrBoneSetupNpcsLast[ numNonNpcsAnimating ++ ] = pba;
					// fall through
				}
			}
		}
	}

	if ( 0 && r_threaded_renderables.GetBool() )
	{
		ParallelProcess( "BoneSetupNpcsLast", arrBoneSetupNpcsLast.Base() + numOpaqueEnts - numNpcs, numNpcs, &SetupBonesOnBaseAnimating );
		ParallelProcess( "BoneSetupNpcsLast NonNPCs", arrBoneSetupNpcsLast.Base(), numNonNpcsAnimating, &SetupBonesOnBaseAnimating );
	}


	//
	// Draw static props + opaque entities from the biggest bucket to the smallest
	//
	{
		CClientRenderablesList::CEntry * pEnts[ RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS ][2];
		CClientRenderablesList::CEntry * pProps[ RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS ][2];

		for ( int bucket = 0; bucket < RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS; ++ bucket )
		{
			pEnts[bucket][0] = m_pRenderablesList->m_RenderGroups[ RENDER_GROUP_OPAQUE_ENTITY_HUGE + 2 * bucket ];
			pEnts[bucket][1] = pEnts[bucket][0] + m_pRenderablesList->m_RenderGroupCounts[ RENDER_GROUP_OPAQUE_ENTITY_HUGE + 2 * bucket ];
			
			pProps[bucket][0] = m_pRenderablesList->m_RenderGroups[ RENDER_GROUP_OPAQUE_STATIC_HUGE + 2 * bucket ];
			pProps[bucket][1] = pProps[bucket][0] + m_pRenderablesList->m_RenderGroupCounts[ RENDER_GROUP_OPAQUE_STATIC_HUGE + 2 * bucket ];

			// Render sequence debugging
			#if DEBUG_BUCKETS
 			if ( r_drawopaquesbucket_stats.GetBool() )
			{
				con_nprint_s nxPrn = { 0 };
				nxPrn.index = 20 + bucket * 3;
				nxPrn.time_to_live = -1;
				nxPrn.color[0] = 0.9f, nxPrn.color[1] = 1.0f, nxPrn.color[2] = 0.9f;
				nxPrn.fixed_width_font = true;
				
				if ( bDrawopaquestaticpropslast )
					engine->Con_NXPrintf( &nxPrn, "[ %2d  ]  Ents : %3d", bucket + 1, pEnts[bucket][1] - pEnts[bucket][0] ),
					++ nxPrn.index,
					engine->Con_NXPrintf( &nxPrn, "[ %2d  ]  Props: %3d", bucket + 1, pProps[bucket][1] - pProps[bucket][0] );
				else
					engine->Con_NXPrintf( &nxPrn, "[ %2d  ]  Props: %3d", bucket + 1, pProps[bucket][1] - pProps[bucket][0] ),
					++ nxPrn.index,
					engine->Con_NXPrintf( &nxPrn, "[ %2d  ]  Ents : %3d", bucket + 1, pEnts[bucket][1] - pEnts[bucket][0] );
			}
			#endif
		}


#if DEBUG_BUCKETS
		if ( int iBucket = r_drawopaquesbucket.GetInt() )
		{
			if ( iBucket > 0 && iBucket <= RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS )
			{
				DrawOpaqueRenderables_Range( pEnts[iBucket - 1][0], pEnts[iBucket - 1][1], bShadowDepth );
			}
			if ( iBucket < 0 && iBucket >= -RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS )
			{
				DrawOpaqueRenderables_DrawStaticProps( pProps[- 1 - iBucket][0], pProps[- 1 - iBucket][1], bShadowDepth );
			}
		}
		else
#endif

		for ( int bucket = 0; bucket < RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS; ++ bucket )
		{
			// PVS-Studio pointed out that the two sides of the if/else were identical. Fixing
			// this long-broken behavior would change rendering, so I fixed the code but
			// commented out the new behavior. Uncomment the if statement and else block
			// when needed.
			//if ( bDrawopaquestaticpropslast )
			{
				DrawOpaqueRenderables_Range( pEnts[bucket][0], pEnts[bucket][1], DepthMode );
				DrawOpaqueRenderables_DrawStaticProps( pProps[bucket][0], pProps[bucket][1], DepthMode );
			}
			/*else
			{
				DrawOpaqueRenderables_DrawStaticProps( pProps[bucket][0], pProps[bucket][1], DepthMode );
				DrawOpaqueRenderables_Range( pEnts[bucket][0], pEnts[bucket][1], DepthMode );
			}*/
		}


	}	

	//
	// Draw NPCs now
	//
	DrawOpaqueRenderables_Range( arrRenderEntsNpcsFirst.Base(), arrRenderEntsNpcsFirst.Base() + numNpcs, DepthMode );

	//
	// Ropes and particles
	//
	RopeManager()->DrawRenderCache( DepthMode );
	g_pParticleSystemMgr->DrawRenderCache( DepthMode );
}


//-----------------------------------------------------------------------------
// Renders all translucent world + detail objects in a particular set of leaves
//-----------------------------------------------------------------------------
void CRendering3dView::DrawTranslucentWorldInLeaves( bool bShadowDepth )
{
	VPROF_BUDGET( "CViewRender::DrawTranslucentWorldInLeaves", VPROF_BUDGETGROUP_WORLD_RENDERING );
	const ClientWorldListInfo_t& info = *m_pWorldListInfo;
	for( int iCurLeafIndex = info.m_LeafCount - 1; iCurLeafIndex >= 0; iCurLeafIndex-- )
	{
		int nActualLeafIndex = info.m_pActualLeafIndex ? info.m_pActualLeafIndex[ iCurLeafIndex ] : iCurLeafIndex;
		Assert( nActualLeafIndex != INVALID_LEAF_INDEX );
		if ( render->LeafContainsTranslucentSurfaces( m_pWorldRenderList, nActualLeafIndex, m_DrawFlags ) )
		{
			// Now draw the surfaces in this leaf
			render->DrawTranslucentSurfaces( m_pWorldRenderList, nActualLeafIndex, m_DrawFlags, bShadowDepth );
		}
	}
}


//-----------------------------------------------------------------------------
// Renders all translucent world + detail objects in a particular set of leaves
//-----------------------------------------------------------------------------
void CRendering3dView::DrawTranslucentWorldAndDetailPropsInLeaves( int iCurLeafIndex, int iFinalLeafIndex, int nEngineDrawFlags, int &nDetailLeafCount, LeafIndex_t* pDetailLeafList, bool bShadowDepth )
{
	VPROF_BUDGET( "CViewRender::DrawTranslucentWorldAndDetailPropsInLeaves", VPROF_BUDGETGROUP_WORLD_RENDERING );
	const ClientWorldListInfo_t& info = *m_pWorldListInfo;
	for( ; iCurLeafIndex >= iFinalLeafIndex; iCurLeafIndex-- )
	{
		int nActualLeafIndex = info.m_pActualLeafIndex ? info.m_pActualLeafIndex[ iCurLeafIndex ] : iCurLeafIndex;
		Assert( nActualLeafIndex != INVALID_LEAF_INDEX );
		if ( render->LeafContainsTranslucentSurfaces( m_pWorldRenderList, nActualLeafIndex, nEngineDrawFlags ) )
		{
			// First draw any queued-up detail props from previously visited leaves
			DetailObjectSystem()->RenderTranslucentDetailObjects( CurrentViewOrigin(), CurrentViewForward(), CurrentViewRight(), CurrentViewUp(), nDetailLeafCount, pDetailLeafList );
			nDetailLeafCount = 0;

			// Now draw the surfaces in this leaf
			render->DrawTranslucentSurfaces( m_pWorldRenderList, nActualLeafIndex, nEngineDrawFlags, bShadowDepth );
		}

		// Queue up detail props that existed in this leaf
		if ( ClientLeafSystem()->ShouldDrawDetailObjectsInLeaf( info.m_pLeafList[iCurLeafIndex], m_pMainView->BuildWorldListsNumber() ) )
		{
			pDetailLeafList[nDetailLeafCount] = info.m_pLeafList[iCurLeafIndex];
			++nDetailLeafCount;
		}
	}
}


//-----------------------------------------------------------------------------
// Renders all translucent entities in the render list
//-----------------------------------------------------------------------------
static inline void DrawTranslucentRenderable( IClientRenderable *pEnt, bool twoPass, bool bShadowDepth, bool bIgnoreDepth )
{
	// Determine blending amount and tell engine
	float blend = (float)( pEnt->GetFxBlend() / 255.0f );

	// Totally gone
	if ( blend <= 0.0f )
		return;

	if ( pEnt->IgnoresZBuffer() != bIgnoreDepth )
		return;

	// Tell engine
	render->SetBlend( blend );

	float color[3];
	pEnt->GetColorModulation( color );
	render->SetColorModulation(	color );

	int flags = STUDIO_RENDER | STUDIO_TRANSPARENCY;
	if ( twoPass )
		flags |= STUDIO_TWOPASS;

	if ( bShadowDepth )
		flags |= STUDIO_SHADOWDEPTHTEXTURE;

	float *pRenderClipPlane = NULL;
	if( r_entityclips.GetBool() )
		pRenderClipPlane = pEnt->GetRenderClipPlane();

	if( pRenderClipPlane )	
	{
		CMatRenderContextPtr pRenderContext( materials );
		if( !materials->UsingFastClipping() ) //do NOT change the fast clip plane mid-scene, depth problems result. Regular user clip planes are fine though
			pRenderContext->PushCustomClipPlane( pRenderClipPlane );
		else
			DrawClippedDepthBox( pEnt, pRenderClipPlane );
		Assert( view->GetCurrentlyDrawingEntity() == NULL );
		view->SetCurrentlyDrawingEntity( pEnt->GetIClientUnknown()->GetBaseEntity() );
		pEnt->DrawModel( flags );
		view->SetCurrentlyDrawingEntity( NULL );

		if( pRenderClipPlane && !materials->UsingFastClipping() )	
			pRenderContext->PopCustomClipPlane();
	}
	else
	{
		Assert( view->GetCurrentlyDrawingEntity() == NULL );
		view->SetCurrentlyDrawingEntity( pEnt->GetIClientUnknown()->GetBaseEntity() );
		pEnt->DrawModel( flags );
		view->SetCurrentlyDrawingEntity( NULL );
	}
}


//-----------------------------------------------------------------------------
// Renders all translucent entities in the render list
//-----------------------------------------------------------------------------
void CRendering3dView::DrawTranslucentRenderablesNoWorld( bool bInSkybox )
{
	VPROF( "CViewRender::DrawTranslucentRenderablesNoWorld" );

	if ( !m_pMainView->ShouldDrawEntities() || !r_drawtranslucentrenderables.GetBool() )
		return;

	// Draw the particle singletons.
	DrawParticleSingletons( bInSkybox );

	bool bShadowDepth = (m_DrawFlags & ( DF_SHADOW_DEPTH_MAP | DF_SSAO_DEPTH_PASS ) ) != 0;

	CClientRenderablesList::CEntry *pEntities = m_pRenderablesList->m_RenderGroups[RENDER_GROUP_TRANSLUCENT_ENTITY];
	int iCurTranslucentEntity = m_pRenderablesList->m_RenderGroupCounts[RENDER_GROUP_TRANSLUCENT_ENTITY] - 1;

	while( iCurTranslucentEntity >= 0 )
	{
		IClientRenderable *pRenderable = pEntities[iCurTranslucentEntity].m_pRenderable;
		if ( pRenderable->UsesPowerOfTwoFrameBufferTexture() )
		{
			UpdateRefractTexture();
		}

		if ( pRenderable->UsesFullFrameBufferTexture() )
		{
			UpdateScreenEffectTexture();
		}

		DrawTranslucentRenderable( pRenderable, pEntities[iCurTranslucentEntity].m_TwoPass != 0, bShadowDepth, false );
		--iCurTranslucentEntity;
	}

	// Reset the blend state.
	render->SetBlend( 1 );
}


//-----------------------------------------------------------------------------
// Renders all translucent entities in the render list that ignore the Z buffer
//-----------------------------------------------------------------------------
void CRendering3dView::DrawNoZBufferTranslucentRenderables( void )
{
	VPROF( "CViewRender::DrawNoZBufferTranslucentRenderables" );

	if ( !m_pMainView->ShouldDrawEntities() || !r_drawtranslucentrenderables.GetBool() )
		return;

	bool bShadowDepth = (m_DrawFlags & ( DF_SHADOW_DEPTH_MAP | DF_SSAO_DEPTH_PASS ) ) != 0;

	CClientRenderablesList::CEntry *pEntities = m_pRenderablesList->m_RenderGroups[RENDER_GROUP_TRANSLUCENT_ENTITY];
	int iCurTranslucentEntity = m_pRenderablesList->m_RenderGroupCounts[RENDER_GROUP_TRANSLUCENT_ENTITY] - 1;

	while( iCurTranslucentEntity >= 0 )
	{
		IClientRenderable *pRenderable = pEntities[iCurTranslucentEntity].m_pRenderable;
		if ( pRenderable->UsesPowerOfTwoFrameBufferTexture() )
		{
			UpdateRefractTexture();
		}

		if ( pRenderable->UsesFullFrameBufferTexture() )
		{
			UpdateScreenEffectTexture();
		}

		DrawTranslucentRenderable( pRenderable, pEntities[iCurTranslucentEntity].m_TwoPass != 0, bShadowDepth, true );
		--iCurTranslucentEntity;
	}

	// Reset the blend state.
	render->SetBlend( 1 );
}



//-----------------------------------------------------------------------------
// Renders all translucent world, entities, and detail objects in a particular set of leaves
//-----------------------------------------------------------------------------
void CRendering3dView::DrawTranslucentRenderables( bool bInSkybox, bool bShadowDepth )
{
	const ClientWorldListInfo_t& info = *m_pWorldListInfo;

#ifdef PORTAL //if we're in the portal mod, we need to make a detour so we can render portal views using stencil areas
	if( ShouldDrawPortals() ) //no recursive stencil views during skybox rendering (although we might be drawing a skybox while already in a recursive stencil view)
	{
		int iDrawFlagsBackup = m_DrawFlags;

		if( g_pPortalRender->DrawPortalsUsingStencils( (CViewRender *)m_pMainView ) )// @MULTICORE (toml 8/10/2006): remove this hack cast
		{
			m_DrawFlags = iDrawFlagsBackup;

			//reset visibility
			unsigned int iVisFlags = 0;
			m_pMainView->SetupVis( *this, iVisFlags, m_pCustomVisibility );		

			//recreate drawlists (since I can't find an easy way to backup the originals)
			{
				SafeRelease( m_pWorldRenderList );
				SafeRelease( m_pWorldListInfo );
				BuildWorldRenderLists( ((m_DrawFlags & DF_DRAW_ENTITITES) != 0), m_pCustomVisibility ? m_pCustomVisibility->m_iForceViewLeaf : -1, false );

				AssertMsg( m_DrawFlags & DF_DRAW_ENTITITES, "It shouldn't be possible to get here if this wasn't set, needs special case investigation" );
				for( int i = m_pRenderablesList->m_RenderGroupCounts[RENDER_GROUP_TRANSLUCENT_ENTITY]; --i >= 0; )
				{
					m_pRenderablesList->m_RenderGroups[RENDER_GROUP_TRANSLUCENT_ENTITY][i].m_pRenderable->ComputeFxBlend();
				}
			}

			if( r_depthoverlay.GetBool() )
			{
				CMatRenderContextPtr pRenderContext( materials );
				ITexture *pDepthTex = GetFullFrameDepthTexture();

				IMaterial *pMaterial = materials->FindMaterial( "debug/showz", TEXTURE_GROUP_OTHER, true );
				pMaterial->IncrementReferenceCount();
				IMaterialVar *BaseTextureVar = pMaterial->FindVar( "$basetexture", NULL, false );
				IMaterialVar *pDepthInAlpha = NULL;
				if( IsPC() )
				{
					pDepthInAlpha = pMaterial->FindVar( "$ALPHADEPTH", NULL, false );
					pDepthInAlpha->SetIntValue( 1 );
				}

				BaseTextureVar->SetTextureValue( pDepthTex );

				pRenderContext->OverrideDepthEnable( true, false ); //don't write to depth, or else we'll never see translucents
				pRenderContext->DrawScreenSpaceQuad( pMaterial );
				pRenderContext->OverrideDepthEnable( false, true );
				pMaterial->DecrementReferenceCount();
			}
		}
		else
		{
			//done recursing in, time to go back out and do translucents
			CMatRenderContextPtr pRenderContext( materials );		

			UpdateFullScreenDepthTexture();
		}
	}
#else
	{
		//opaques generally write depth, and translucents generally don't.
		//So immediately after opaques are done is the best time to snap off the depth buffer to a texture.
		switch ( g_CurrentViewID )
		{				 
		case VIEW_MAIN:
#ifdef _X360
		case VIEW_INTRO_CAMERA:
		case VIEW_INTRO_PLAYER:
#endif
			UpdateFullScreenDepthTexture();
			break;

		default:
			materials->GetRenderContext()->SetFullScreenDepthTextureValidityFlag( false );
			break;
		}
	}
#endif

	if ( !r_drawtranslucentworld.GetBool() )
	{
		DrawTranslucentRenderablesNoWorld( bInSkybox );
		return;
	}

	VPROF_BUDGET( "CViewRender::DrawTranslucentRenderables", "DrawTranslucentRenderables" );
	int iPrevLeaf = info.m_LeafCount - 1;
	int nDetailLeafCount = 0;
	LeafIndex_t *pDetailLeafList = (LeafIndex_t*)stackalloc( info.m_LeafCount * sizeof(LeafIndex_t) );

// 	bool bDrawUnderWater = (nFlags & DF_RENDER_UNDERWATER) != 0;
// 	bool bDrawAboveWater = (nFlags & DF_RENDER_ABOVEWATER) != 0;
// 	bool bDrawWater = (nFlags & DF_RENDER_WATER) != 0;
// 	bool bClipSkybox = (nFlags & DF_CLIP_SKYBOX ) != 0;
	unsigned long nEngineDrawFlags = BuildEngineDrawWorldListFlags( m_DrawFlags & ~DF_DRAWSKYBOX );

	DetailObjectSystem()->BeginTranslucentDetailRendering();

	if ( m_pMainView->ShouldDrawEntities() && r_drawtranslucentrenderables.GetBool() )
	{
		MDLCACHE_CRITICAL_SECTION();
		// Draw the particle singletons.
		DrawParticleSingletons( bInSkybox );

		CClientRenderablesList::CEntry *pEntities = m_pRenderablesList->m_RenderGroups[RENDER_GROUP_TRANSLUCENT_ENTITY];
		int iCurTranslucentEntity = m_pRenderablesList->m_RenderGroupCounts[RENDER_GROUP_TRANSLUCENT_ENTITY] - 1;

		bool bRenderingWaterRenderTargets = m_DrawFlags & ( DF_RENDER_REFRACTION | DF_RENDER_REFLECTION );

		while( iCurTranslucentEntity >= 0 )
		{
			// Seek the current leaf up to our current translucent-entity leaf.
			int iThisLeaf = pEntities[iCurTranslucentEntity].m_iWorldListInfoLeaf;

			// First draw the translucent parts of the world up to and including those in this leaf
			DrawTranslucentWorldAndDetailPropsInLeaves( iPrevLeaf, iThisLeaf, nEngineDrawFlags, nDetailLeafCount, pDetailLeafList, bShadowDepth );

			// We're traversing the leaf list backwards to get the appropriate sort ordering (back to front)
			iPrevLeaf = iThisLeaf - 1;

			// Draw all the translucent entities with this leaf.
			int nLeaf = info.m_pLeafList[iThisLeaf];

			bool bDrawDetailProps = ClientLeafSystem()->ShouldDrawDetailObjectsInLeaf( nLeaf, m_pMainView->BuildWorldListsNumber() );
			if ( bDrawDetailProps )
			{
				// Draw detail props up to but not including this leaf
				Assert( nDetailLeafCount > 0 ); 
				--nDetailLeafCount;
				Assert( pDetailLeafList[nDetailLeafCount] == nLeaf );
				DetailObjectSystem()->RenderTranslucentDetailObjects( CurrentViewOrigin(), CurrentViewForward(), CurrentViewRight(), CurrentViewUp(), nDetailLeafCount, pDetailLeafList );

				// Draw translucent renderables in the leaf interspersed with detail props
				for( ;pEntities[iCurTranslucentEntity].m_iWorldListInfoLeaf == iThisLeaf && iCurTranslucentEntity >= 0; --iCurTranslucentEntity )
				{
					IClientRenderable *pRenderable = pEntities[iCurTranslucentEntity].m_pRenderable;

					// Draw any detail props in this leaf that's farther than the entity
					const Vector &vecRenderOrigin = pRenderable->GetRenderOrigin();
					DetailObjectSystem()->RenderTranslucentDetailObjectsInLeaf( CurrentViewOrigin(), CurrentViewForward(), CurrentViewRight(), CurrentViewUp(), nLeaf, &vecRenderOrigin );

					bool bUsesPowerOfTwoFB = pRenderable->UsesPowerOfTwoFrameBufferTexture();
					bool bUsesFullFB       = pRenderable->UsesFullFrameBufferTexture();

					if ( ( bUsesPowerOfTwoFB || bUsesFullFB )&& !bShadowDepth )
					{
						if( bRenderingWaterRenderTargets )
						{
							continue;
						}

						CMatRenderContextPtr pRenderContext( materials );
						ITexture *rt = pRenderContext->GetRenderTarget();

						if ( rt && bUsesFullFB )
						{
							UpdateScreenEffectTexture( 0, 0, 0, rt->GetActualWidth(), rt->GetActualHeight(), true );
						}
						else if ( bUsesPowerOfTwoFB )
						{
							UpdateRefractTexture();
						}

						pRenderContext.SafeRelease();
					}

					// Then draw the translucent renderable
					DrawTranslucentRenderable( pRenderable, (pEntities[iCurTranslucentEntity].m_TwoPass != 0), bShadowDepth, false );
				}

				// Draw all remaining props in this leaf
				DetailObjectSystem()->RenderTranslucentDetailObjectsInLeaf( CurrentViewOrigin(), CurrentViewForward(), CurrentViewRight(), CurrentViewUp(), nLeaf, NULL );
			}
			else
			{
				// Draw queued up detail props (we know that the list of detail leaves won't include this leaf, since ShouldDrawDetailObjectsInLeaf is false)
				// Therefore no fixup on nDetailLeafCount is required as in the above section
				DetailObjectSystem()->RenderTranslucentDetailObjects( CurrentViewOrigin(), CurrentViewForward(), CurrentViewRight(), CurrentViewUp(), nDetailLeafCount, pDetailLeafList );

				for( ;pEntities[iCurTranslucentEntity].m_iWorldListInfoLeaf == iThisLeaf && iCurTranslucentEntity >= 0; --iCurTranslucentEntity )
				{
					IClientRenderable *pRenderable = pEntities[iCurTranslucentEntity].m_pRenderable;

					bool bUsesPowerOfTwoFB = pRenderable->UsesPowerOfTwoFrameBufferTexture();
					bool bUsesFullFB       = pRenderable->UsesFullFrameBufferTexture();

					if ( ( bUsesPowerOfTwoFB || bUsesFullFB )&& !bShadowDepth )
					{
						if( bRenderingWaterRenderTargets )
						{
							continue;
						}

						CMatRenderContextPtr pRenderContext( materials );
						ITexture *rt = pRenderContext->GetRenderTarget();

						if ( rt )
						{
							if ( bUsesFullFB )
							{
								UpdateScreenEffectTexture( 0, 0, 0, rt->GetActualWidth(), rt->GetActualHeight(), true );
							}
							else if ( bUsesPowerOfTwoFB )
							{
								UpdateRefractTexture(0, 0, rt->GetActualWidth(), rt->GetActualHeight());
							}
						}
						else
						{
							if ( bUsesPowerOfTwoFB )
							{
								UpdateRefractTexture();
							}
						}

						pRenderContext.SafeRelease();
					}

					DrawTranslucentRenderable( pRenderable, (pEntities[iCurTranslucentEntity].m_TwoPass != 0), bShadowDepth, false );
				}
			}

			nDetailLeafCount = 0;
		}
	}

	// Draw the rest of the surfaces in world leaves
	DrawTranslucentWorldAndDetailPropsInLeaves( iPrevLeaf, 0, nEngineDrawFlags, nDetailLeafCount, pDetailLeafList, bShadowDepth );

	// Draw any queued-up detail props from previously visited leaves
	DetailObjectSystem()->RenderTranslucentDetailObjects( CurrentViewOrigin(), CurrentViewForward(), CurrentViewRight(), CurrentViewUp(), nDetailLeafCount, pDetailLeafList );

	// Reset the blend state.
	render->SetBlend( 1 );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CRendering3dView::EnableWorldFog( void )
{
	VPROF("CViewRender::EnableWorldFog");
	CMatRenderContextPtr pRenderContext( materials );

	fogparams_t *pFogParams = NULL;
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if ( pbp )
	{
		pFogParams = pbp->GetFogParams();
	}

	if( GetFogEnable( pFogParams ) )
	{
		float fogColor[3];
		GetFogColor( pFogParams, fogColor );
		pRenderContext->FogMode( MATERIAL_FOG_LINEAR );
		pRenderContext->FogColor3fv( fogColor );
		pRenderContext->FogStart( GetFogStart( pFogParams ) );
		pRenderContext->FogEnd( GetFogEnd( pFogParams ) );
		pRenderContext->FogMaxDensity( GetFogMaxDensity( pFogParams ) );
		pRenderContext->FogRadial( GetFogRadial( pFogParams ) );
	}
	else
	{
		pRenderContext->FogMode( MATERIAL_FOG_NONE );
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int CRendering3dView::GetDrawFlags()
{
	return m_DrawFlags;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CRendering3dView::SetFogVolumeState( const VisibleFogVolumeInfo_t &fogInfo, bool bUseHeightFog )
{
	render->SetFogVolumeState( fogInfo.m_nVisibleFogVolume, bUseHeightFog );

#ifdef PORTAL

	//the idea behind fog shifting is this...
	//Normal fog simulates the effect of countless tiny particles between your viewpoint and whatever geometry is rendering.
	//But, when rendering to a portal view, there's a large space between the virtual camera and the portal exit surface.
	//This space isn't supposed to exist, and therefore has none of the tiny particles that make up fog.
	//So, we have to shift fog start/end out to align the distances with the portal exit surface instead of the virtual camera to eliminate fog simulation in the non-space
	if( g_pPortalRender->GetViewRecursionLevel() == 0 )
		return; //rendering one of the primary views, do nothing

	g_pPortalRender->ShiftFogForExitPortalView();

#endif //#ifdef PORTAL
}


//-----------------------------------------------------------------------------
// Standard 3d skybox view
//-----------------------------------------------------------------------------
SkyboxVisibility_t CSkyboxView::ComputeSkyboxVisibility()
{
	return engine->IsSkyboxVisibleFromPoint( origin );
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CSkyboxView::GetSkyboxFogEnable()
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return false;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	if( fog_override.GetInt() )
	{
		if( fog_enableskybox.GetInt() != -1 )
		{
			return fog_enableskybox.GetBool();
		}
	}

	return !!local->m_skybox3d.fog.enable;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CSkyboxView::Enable3dSkyboxFog( void )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	CMatRenderContextPtr pRenderContext( materials );

	if( GetSkyboxFogEnable() )
	{
		float fogColor[3];
		GetSkyboxFogColor( fogColor );
		float scale = 1.0f;
		if ( local->m_skybox3d.scale > 0.0f )
		{
			scale = 1.0f / local->m_skybox3d.scale;
		}
		pRenderContext->FogMode( MATERIAL_FOG_LINEAR );
		pRenderContext->FogColor3fv( fogColor );
		pRenderContext->FogStart( GetSkyboxFogStart() * scale );
		pRenderContext->FogEnd( GetSkyboxFogEnd() * scale );
		pRenderContext->FogMaxDensity( GetSkyboxFogMaxDensity() );
		pRenderContext->FogRadial( GetSkyboxFogRadial() );
	}
	else
	{
		pRenderContext->FogMode( MATERIAL_FOG_NONE );
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
sky3dparams_t *CSkyboxView::PreRender3dSkyboxWorld( SkyboxVisibility_t nSkyboxVisible )
{
	if ( ( nSkyboxVisible != SKYBOX_3DSKYBOX_VISIBLE ) && r_3dsky.GetInt() != 2 )
		return NULL;

	// render the 3D skybox
	if ( !r_3dsky.GetInt() )
		return NULL;

	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();

	// No local player object yet...
	if ( !pbp )
		return NULL;

	CPlayerLocalData* local = &pbp->m_Local;
	if ( local->m_skybox3d.area == 255 )
		return NULL;

	return &local->m_skybox3d;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CSkyboxView::DrawInternal( view_id_t iSkyBoxViewID, bool bInvokePreAndPostRender, ITexture *pRenderTarget, ITexture *pDepthTarget )
{
	unsigned char **areabits = render->GetAreaBits();
	unsigned char *savebits;
	unsigned char tmpbits[ 32 ];
	savebits = *areabits;
	memset( tmpbits, 0, sizeof(tmpbits) );

	// set the sky area bit
	tmpbits[m_pSky3dParams->area>>3] |= 1 << (m_pSky3dParams->area&7);

	*areabits = tmpbits;

	// if you can get really close to the skybox geometry it's possible that you'll be able to clip into it
	// with this near plane.  If so, move it in a bit.  It's at 2.0 to give us more precision.  That means you 
	// need to keep the eye position at least 2 * scale away from the geometry in the skybox
	zNear = 2.0;
	zFar = MAX_TRACE_LENGTH;

	// scale origin by sky scale
	if ( m_pSky3dParams->scale > 0 )
	{
		float scale = 1.0f / m_pSky3dParams->scale;
		VectorScale( origin, scale, origin );
	}
	Enable3dSkyboxFog();
	VectorAdd( origin, m_pSky3dParams->origin, origin );

	// BUGBUG: Fix this!!!  We shouldn't need to call setup vis for the sky if we're connecting
	// the areas.  We'd have to mark all the clusters in the skybox area in the PVS of any 
	// cluster with sky.  Then we could just connect the areas to do our vis.
	//m_bOverrideVisOrigin could hose us here, so call direct
	render->ViewSetupVis( false, 1, &m_pSky3dParams->origin.Get() );
	render->Push3DView( (*this), m_ClearFlags, pRenderTarget, GetFrustum(), pDepthTarget );

	// Store off view origin and angles
	SetupCurrentView( origin, angles, iSkyBoxViewID );

#if defined( _X360 )
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->PushVertexShaderGPRAllocation( 32 );
	pRenderContext.SafeRelease();
#endif

	// Invoke pre-render methods
	if ( bInvokePreAndPostRender )
	{
		IGameSystem::PreRenderAllSystems();
	}

	render->BeginUpdateLightmaps();
	BuildWorldRenderLists( true, true, -1 );
	BuildRenderableRenderLists( iSkyBoxViewID );
	render->EndUpdateLightmaps();

	g_pClientShadowMgr->ComputeShadowTextures( (*this), m_pWorldListInfo->m_LeafCount, m_pWorldListInfo->m_pLeafList );

	DrawWorld( 0.0f );

	// Iterate over all leaves and render objects in those leaves
	DrawOpaqueRenderables( DEPTH_MODE_NORMAL );

	// Iterate over all leaves and render objects in those leaves
	DrawTranslucentRenderables( true, false );
	DrawNoZBufferTranslucentRenderables();

	m_pMainView->DisableFog();

	CGlowOverlay::UpdateSkyOverlays( zFar, m_bCacheFullSceneState );

	PixelVisibility_EndCurrentView();

	// restore old area bits
	*areabits = savebits;

	// Invoke post-render methods
	if( bInvokePreAndPostRender )
	{
		IGameSystem::PostRenderAllSystems();
		FinishCurrentView();
	}

	render->PopView( GetFrustum() );

#if defined( _X360 )
	pRenderContext.GetFrom( materials );
	pRenderContext->PopVertexShaderGPRAllocation();
#endif
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CSkyboxView::Setup( const CViewSetup &viewRender, int *pClearFlags, SkyboxVisibility_t *pSkyboxVisible )
{
	BaseClass::Setup( viewRender );

	// The skybox might not be visible from here
	*pSkyboxVisible = ComputeSkyboxVisibility();
	m_pSky3dParams = PreRender3dSkyboxWorld( *pSkyboxVisible );

	if ( !m_pSky3dParams )
	{
		return false;
	}

	// At this point, we've cleared everything we need to clear
	// The next path will need to clear depth, though.
	m_ClearFlags = *pClearFlags;
	*pClearFlags &= ~( VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH | VIEW_CLEAR_STENCIL | VIEW_CLEAR_FULL_TARGET );
	*pClearFlags |= VIEW_CLEAR_DEPTH; // Need to clear depth after rednering the skybox

	m_DrawFlags = DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER | DF_RENDER_WATER;
	if( r_skybox.GetBool() )
	{
		m_DrawFlags |= DF_DRAWSKYBOX;
	}

	return true;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CSkyboxView::Draw()
{
	VPROF_BUDGET( "CViewRender::Draw3dSkyboxworld", "3D Skybox" );

	ITexture *pRTColor = NULL;
	ITexture *pRTDepth = NULL;
	if( m_eStereoEye != STEREO_EYE_MONO )
	{
		pRTColor = g_pSourceVR->GetRenderTarget( (ISourceVirtualReality::VREye)(m_eStereoEye-1), ISourceVirtualReality::RT_Color );
		pRTDepth = g_pSourceVR->GetRenderTarget( (ISourceVirtualReality::VREye)(m_eStereoEye-1), ISourceVirtualReality::RT_Depth );
	}

	DrawInternal(VIEW_3DSKY, true, pRTColor, pRTDepth );
}


#ifdef PORTAL
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CPortalSkyboxView::Setup( const CViewSetup &view, int *pClearFlags, SkyboxVisibility_t *pSkyboxVisible, ITexture *pRenderTarget )
{
	if ( !BaseClass::Setup( view, pClearFlags, pSkyboxVisible ) )
		return false;

	m_pRenderTarget = pRenderTarget;
	return true;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
SkyboxVisibility_t CPortalSkyboxView::ComputeSkyboxVisibility()
{
	return g_pPortalRender->IsSkyboxVisibleFromExitPortal();
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CPortalSkyboxView::Draw()
{
	AssertMsg( (g_pPortalRender->GetViewRecursionLevel() != 0) && g_pPortalRender->IsRenderingPortal(), "This is designed for through-portal views. Use the regular skybox drawing code for primary views" );

	VPROF_BUDGET( "CViewRender::Draw3dSkyboxworld_Portal", "3D Skybox (portal view)" );

	int iCurrentViewID = g_CurrentViewID;

	Frustum FrustumBackup;
	memcpy( FrustumBackup, GetFrustum(), sizeof( Frustum ) );

	CMatRenderContextPtr pRenderContext( materials );

	bool bClippingEnabled = pRenderContext->EnableClipping( false );

	//NOTE: doesn't magically map to VIEW_3DSKY at (0,0) like PORTAL_VIEWID maps to VIEW_MAIN
	view_id_t iSkyBoxViewID = (view_id_t)g_pPortalRender->GetCurrentSkyboxViewId();

	bool bInvokePreAndPostRender = ( g_pPortalRender->ShouldUseStencilsToRenderPortals() == false );

	DrawInternal( iSkyBoxViewID, bInvokePreAndPostRender, m_pRenderTarget, NULL );

	pRenderContext->EnableClipping( bClippingEnabled );

	memcpy( GetFrustum(), FrustumBackup, sizeof( Frustum ) );
	render->OverrideViewFrustum( FrustumBackup );

	g_CurrentViewID = iCurrentViewID;
}
#endif // PORTAL


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CShadowDepthView::Setup( const CViewSetup &shadowViewIn, ITexture *pRenderTarget, ITexture *pDepthTexture )
{
	BaseClass::Setup( shadowViewIn );
	m_pRenderTarget = pRenderTarget;
	m_pDepthTexture = pDepthTexture;
}


bool DrawingShadowDepthView( void ) //for easy externing
{
	return (CurrentViewID() == VIEW_SHADOW_DEPTH_TEXTURE);
}

bool DrawingMainView() //for easy externing
{
	return (CurrentViewID() == VIEW_MAIN);
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CShadowDepthView::Draw()
{
	VPROF_BUDGET( "CShadowDepthView::Draw", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	// Start view
	unsigned int visFlags;
	m_pMainView->SetupVis( (*this), visFlags );  // @MULTICORE (toml 8/9/2006): Portal problem, not sending custom vis down

	CMatRenderContextPtr pRenderContext( materials );

	pRenderContext->ClearColor3ub(0xFF, 0xFF, 0xFF);

#if defined( _X360 )
	pRenderContext->PushVertexShaderGPRAllocation( 112 ); //almost all work is done in vertex shaders for depth rendering, max out their threads
#endif

	pRenderContext.SafeRelease();

	if( IsPC() )
	{
		render->Push3DView( (*this), VIEW_CLEAR_DEPTH, m_pRenderTarget, GetFrustum(), m_pDepthTexture );
	}
	else if( IsX360() )
	{
		//for the 360, the dummy render target has a separate depth buffer which we Resolve() from afterward
		render->Push3DView( (*this), VIEW_CLEAR_DEPTH, m_pRenderTarget, GetFrustum() );
	}

	SetupCurrentView( origin, angles, VIEW_SHADOW_DEPTH_TEXTURE );

	MDLCACHE_CRITICAL_SECTION();

	{
		VPROF_BUDGET( "BuildWorldRenderLists", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		BuildWorldRenderLists( true, -1, true, true ); // @MULTICORE (toml 8/9/2006): Portal problem, not sending custom vis down
	}

	{
		VPROF_BUDGET( "BuildRenderableRenderLists", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		BuildRenderableRenderLists( CurrentViewID() );
	}

	engine->Sound_ExtraUpdate();	// Make sure sound doesn't stutter

	m_DrawFlags = m_pMainView->GetBaseDrawFlags() | DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER | DF_SHADOW_DEPTH_MAP;	// Don't draw water surface...

	{
		VPROF_BUDGET( "DrawWorld", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		DrawWorld( 0.0f );
	}

	// Draw opaque and translucent renderables with appropriate override materials
	// OVERRIDE_DEPTH_WRITE is OK with a NULL material pointer
	modelrender->ForcedMaterialOverride( NULL, OVERRIDE_DEPTH_WRITE );	

	{
		VPROF_BUDGET( "DrawOpaqueRenderables", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		DrawOpaqueRenderables( DEPTH_MODE_SHADOW );
	}

	modelrender->ForcedMaterialOverride( 0 );

	m_DrawFlags = 0;

	pRenderContext.GetFrom( materials );

	if( IsX360() )
	{
		//Resolve() the depth texture here. Before the pop so the copy will recognize that the resolutions are the same
		pRenderContext->CopyRenderTargetToTextureEx( m_pDepthTexture, -1, NULL, NULL );
	}

	render->PopView( GetFrustum() );

#if defined( _X360 )
	pRenderContext->PopVertexShaderGPRAllocation();
#endif
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CFreezeFrameView::Setup( const CViewSetup &shadowViewIn )
{
	BaseClass::Setup( shadowViewIn );

	KeyValues *pVMTKeyValues = new KeyValues( "UnlitGeneric" );
	pVMTKeyValues->SetString( "$basetexture", IsX360() ? "_rt_FullFrameFB1" : "_rt_FullScreen" );
	pVMTKeyValues->SetInt( "$nocull", 1 );
	pVMTKeyValues->SetInt( "$nofog", 1 );
	pVMTKeyValues->SetInt( "$ignorez", 1 );
	m_pFreezeFrame.Init( "FreezeFrame_FullScreen", TEXTURE_GROUP_OTHER, pVMTKeyValues );
	m_pFreezeFrame->Refresh();

	m_TranslucentSingleColor.Init( "debug/debugtranslucentsinglecolor", TEXTURE_GROUP_OTHER );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFreezeFrameView::Draw( void )
{
	CMatRenderContextPtr pRenderContext( materials );

#if defined( _X360 )
	pRenderContext->PushVertexShaderGPRAllocation( 16 ); //max out pixel shader threads
#endif

	// we might only need half of the texture if we're rendering in stereo
	int nTexX0 = 0, nTexY0 = 0;
	int nTexX1 = width, nTexY1 = height;
	int nTexWidth = width, nTexHeight = height;

	switch( m_eStereoEye )
	{
	case STEREO_EYE_LEFT:
		nTexX1 = width;
		nTexWidth *= 2;
		break;

	case STEREO_EYE_RIGHT:
		nTexX0 = width;
		nTexX1 = width*2;
		nTexWidth *= 2;
		break;
	}

	pRenderContext->DrawScreenSpaceRectangle( m_pFreezeFrame, x, y, width, height,
		nTexX0, nTexY0, nTexX1-1, nTexY1-1, nTexWidth, nTexHeight );

	//Fake a fade during freezeframe view.
	if ( g_flFreezeFlash >= gpGlobals->curtime && engine->IsTakingScreenshot() == false )
	{
		// Overlay screen fade on entire screen
		IMaterial* pMaterial = m_TranslucentSingleColor;

		int iFadeAlpha = FREEZECAM_SNAPSHOT_FADE_SPEED * ( g_flFreezeFlash - gpGlobals->curtime );
		
		iFadeAlpha = MIN( iFadeAlpha, 255 );
		iFadeAlpha = MAX( 0, iFadeAlpha );
		
		pMaterial->AlphaModulate( iFadeAlpha * ( 1.0f / 255.0f ) );
		pMaterial->ColorModulate( 1.0f,	1.0f, 1.0f );
		pMaterial->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, true );

		pRenderContext->DrawScreenSpaceRectangle( pMaterial, x, y, width, height, 0, 0, width-1, height-1, width, height );
	}

#if defined( _X360 )
	pRenderContext->PopVertexShaderGPRAllocation();
#endif
}

//-----------------------------------------------------------------------------
// Pops a water render target
//-----------------------------------------------------------------------------
bool CBaseWorldView::AdjustView( float waterHeight )
{
	if( m_DrawFlags & DF_RENDER_REFRACTION )
	{
		ITexture *pTexture = GetWaterRefractionTexture();

		// Use the aspect ratio of the main view! So, don't recompute it here
		x = y = 0;
		width = pTexture->GetActualWidth();
		height = pTexture->GetActualHeight();

		return true;
	}

	if( m_DrawFlags & DF_RENDER_REFLECTION )
	{
		ITexture *pTexture = GetWaterReflectionTexture();

		// If the main view is overriding the projection matrix (for Stereo or
		// some other nefarious purpose) make sure to include any Y offset in 
		// the custom projection matrix in our reflected overridden projection
		// matrix.
		if( m_bViewToProjectionOverride )
		{
			m_ViewToProjection[1][2] = -m_ViewToProjection[1][2];
		}

		// Use the aspect ratio of the main view! So, don't recompute it here
		x = y = 0;
		width = pTexture->GetActualWidth();
		height = pTexture->GetActualHeight();
		angles[0] = -angles[0];
		angles[2] = -angles[2];
		origin[2] -= 2.0f * ( origin[2] - (waterHeight));
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Pops a water render target
//-----------------------------------------------------------------------------
void CBaseWorldView::PushView( float waterHeight )
{
	float spread = 2.0f;
	if( m_DrawFlags & DF_FUDGE_UP )
	{
		waterHeight += spread;
	}
	else
	{
		waterHeight -= spread;
	}

	MaterialHeightClipMode_t clipMode = MATERIAL_HEIGHTCLIPMODE_DISABLE;
	if ( ( m_DrawFlags & DF_CLIP_Z ) && mat_clipz.GetBool() )
	{
		if( m_DrawFlags & DF_CLIP_BELOW )
		{
			clipMode = MATERIAL_HEIGHTCLIPMODE_RENDER_ABOVE_HEIGHT;
		}
		else
		{
			clipMode = MATERIAL_HEIGHTCLIPMODE_RENDER_BELOW_HEIGHT;
		}
	}

	CMatRenderContextPtr pRenderContext( materials );

	if( m_DrawFlags & DF_RENDER_REFRACTION )
	{
		pRenderContext->SetFogZ( waterHeight );
		pRenderContext->SetHeightClipZ( waterHeight );
		pRenderContext->SetHeightClipMode( clipMode );

		// Have to re-set up the view since we reset the size
		render->Push3DView( *this, m_ClearFlags, GetWaterRefractionTexture(), GetFrustum() );

		return;
	}

	if( m_DrawFlags & DF_RENDER_REFLECTION )
	{
		ITexture *pTexture = GetWaterReflectionTexture();

		pRenderContext->SetFogZ( waterHeight );

		bool bSoftwareUserClipPlane = g_pMaterialSystemHardwareConfig->UseFastClipping();
		if( bSoftwareUserClipPlane && ( origin[2] > waterHeight - r_eyewaterepsilon.GetFloat() ) )
		{
			waterHeight = origin[2] + r_eyewaterepsilon.GetFloat();
		}

		pRenderContext->SetHeightClipZ( waterHeight );
		pRenderContext->SetHeightClipMode( clipMode );

		render->Push3DView( *this, m_ClearFlags, pTexture, GetFrustum() );

		SetLightmapScaleForWater();
		return;
	}

	if ( m_ClearFlags & ( VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR | VIEW_CLEAR_STENCIL ) )
	{
		if ( m_ClearFlags & VIEW_CLEAR_OBEY_STENCIL )
		{
			pRenderContext->ClearBuffersObeyStencil( m_ClearFlags & VIEW_CLEAR_COLOR, m_ClearFlags & VIEW_CLEAR_DEPTH );
		}
		else
		{
			pRenderContext->ClearBuffers( m_ClearFlags & VIEW_CLEAR_COLOR, m_ClearFlags & VIEW_CLEAR_DEPTH, m_ClearFlags & VIEW_CLEAR_STENCIL );
		}
	}

	pRenderContext->SetHeightClipMode( clipMode );
	if ( clipMode != MATERIAL_HEIGHTCLIPMODE_DISABLE )
	{   
		pRenderContext->SetHeightClipZ( waterHeight );
	}
}


//-----------------------------------------------------------------------------
// Pops a water render target
//-----------------------------------------------------------------------------
void CBaseWorldView::PopView()
{
	CMatRenderContextPtr pRenderContext( materials );

	pRenderContext->SetHeightClipMode( MATERIAL_HEIGHTCLIPMODE_DISABLE );
	if( m_DrawFlags & (DF_RENDER_REFRACTION | DF_RENDER_REFLECTION) )
	{
		if ( IsX360() )
		{
			// these renders paths used their surfaces, so blit their results
			if ( m_DrawFlags & DF_RENDER_REFRACTION )
			{
				pRenderContext->CopyRenderTargetToTextureEx( GetWaterRefractionTexture(), NULL, NULL );
			}
			if ( m_DrawFlags & DF_RENDER_REFLECTION )
			{
				pRenderContext->CopyRenderTargetToTextureEx( GetWaterReflectionTexture(), NULL, NULL );
			}
		}

		render->PopView( GetFrustum() );
		if (SavedLinearLightMapScale.x>=0)
		{
			pRenderContext->SetToneMappingScaleLinear(SavedLinearLightMapScale);
			SavedLinearLightMapScale.x=-1;
		}
	}
}


//-----------------------------------------------------------------------------
// Draws the world + entities
//-----------------------------------------------------------------------------
void CBaseWorldView::DrawSetup( float waterHeight, int nSetupFlags, float waterZAdjust, int iForceViewLeaf )
{
	int savedViewID = g_CurrentViewID;
	g_CurrentViewID = VIEW_ILLEGAL;

	bool bViewChanged = AdjustView( waterHeight );

	if ( bViewChanged )
	{
		render->Push3DView( *this, 0, NULL, GetFrustum() );
	}

	render->BeginUpdateLightmaps();

	bool bDrawEntities = ( nSetupFlags & DF_DRAW_ENTITITES ) != 0;
	bool bDrawReflection = ( nSetupFlags & DF_RENDER_REFLECTION ) != 0;
	BuildWorldRenderLists( bDrawEntities, iForceViewLeaf, true, false, bDrawReflection ? &waterHeight : NULL );

	PruneWorldListInfo();

	if ( bDrawEntities )
	{
		BuildRenderableRenderLists( savedViewID );
	}

	render->EndUpdateLightmaps();

	if ( bViewChanged )
	{
		render->PopView( GetFrustum() );
	}

#ifdef TF_CLIENT_DLL
	bool bVisionOverride = ( localplayer_visionflags.GetInt() & ( 0x01 ) ); // Pyro-vision Goggles

	if ( savedViewID == VIEW_MAIN && bVisionOverride && pyro_dof.GetBool() )
	{
		SSAO_DepthPass();
	}
#endif

	g_CurrentViewID = savedViewID;
}


void MaybeInvalidateLocalPlayerAnimation()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( ( pPlayer != NULL ) && pPlayer->InFirstPersonView() )
	{
		// We sometimes need different animation for the main view versus the shadow rendering,
		// so we need to reset the cache to ensure this actually happens.
		pPlayer->InvalidateBoneCache();

		C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
		if ( pWeapon != NULL )
		{
			pWeapon->InvalidateBoneCache();
		}

#if defined USES_ECON_ITEMS
		// ...and all the things you're wearing/holding/etc
		int NumWearables = pPlayer->GetNumWearables();
		for ( int i = 0; i < NumWearables; ++i )
		{
			CEconWearable* pItem = pPlayer->GetWearable ( i );
			if ( pItem != NULL )
			{
				pItem->InvalidateBoneCache();
			}
		}
#endif // USES_ECON_ITEMS

	}
}

void CBaseWorldView::DrawExecute( float waterHeight, view_id_t viewID, float waterZAdjust )
{
	int savedViewID = g_CurrentViewID;

	// @MULTICORE (toml 8/16/2006): rethink how, where, and when this is done...
	g_CurrentViewID = VIEW_SHADOW_DEPTH_TEXTURE;
	MaybeInvalidateLocalPlayerAnimation();
	g_pClientShadowMgr->ComputeShadowTextures( *this, m_pWorldListInfo->m_LeafCount, m_pWorldListInfo->m_pLeafList );
	MaybeInvalidateLocalPlayerAnimation();

	// Make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();

	g_CurrentViewID = viewID;

	// Update our render view flags.
	int iDrawFlagsBackup = m_DrawFlags;
	m_DrawFlags |= m_pMainView->GetBaseDrawFlags();

	PushView( waterHeight );

	CMatRenderContextPtr pRenderContext( materials );

#if defined( _X360 )
	pRenderContext->PushVertexShaderGPRAllocation( 32 );
#endif

	ITexture *pSaveFrameBufferCopyTexture = pRenderContext->GetFrameBufferCopyTexture( 0 );
	if ( engine->GetDXSupportLevel() >= 80 )
	{
		pRenderContext->SetFrameBufferCopyTexture( GetPowerOfTwoFrameBufferTexture() );
	}

	pRenderContext.SafeRelease();

	ERenderDepthMode DepthMode = DEPTH_MODE_NORMAL;

	if ( m_DrawFlags & DF_DRAW_ENTITITES )
	{
		DrawWorld( waterZAdjust );
		DrawOpaqueRenderables( DepthMode );

#ifdef TF_CLIENT_DLL
		bool bVisionOverride = ( localplayer_visionflags.GetInt() & ( 0x01 ) ); // Pyro-vision Goggles

		if ( g_CurrentViewID == VIEW_MAIN && bVisionOverride && pyro_dof.GetBool() ) // Pyro-vision Goggles
		{
			DrawDepthOfField();
		}
#endif
		DrawTranslucentRenderables( false, false );
		DrawNoZBufferTranslucentRenderables();
	}
	else
	{
		DrawWorld( waterZAdjust );

#ifdef TF_CLIENT_DLL
		bool bVisionOverride = ( localplayer_visionflags.GetInt() & ( 0x01 ) ); // Pyro-vision Goggles

		if ( g_CurrentViewID == VIEW_MAIN && bVisionOverride && pyro_dof.GetBool() ) // Pyro-vision Goggles
		{
			DrawDepthOfField();
		}
#endif
		// Draw translucent world brushes only, no entities
		DrawTranslucentWorldInLeaves( false );
	}

	// issue the pixel visibility tests for sub-views
	if ( !IsMainView( CurrentViewID() ) && CurrentViewID() != VIEW_INTRO_CAMERA )
	{
		PixelVisibility_EndCurrentView();
	}

	pRenderContext.GetFrom( materials );
	pRenderContext->SetFrameBufferCopyTexture( pSaveFrameBufferCopyTexture );
	PopView();

	m_DrawFlags = iDrawFlagsBackup;

	g_CurrentViewID = savedViewID;

#if defined( _X360 )
	pRenderContext->PopVertexShaderGPRAllocation();
#endif
}


void CBaseWorldView::SSAO_DepthPass()
{
	if ( !g_pMaterialSystemHardwareConfig->SupportsPixelShaders_2_0() )
	{
		return;
	}

#if 1
	VPROF_BUDGET( "CSimpleWorldView::SSAO_DepthPass", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	int savedViewID = g_CurrentViewID;
	g_CurrentViewID = VIEW_SSAO;

	ITexture *pSSAO = materials->FindTexture( "_rt_ResolvedFullFrameDepth", TEXTURE_GROUP_RENDER_TARGET );

	CMatRenderContextPtr pRenderContext( materials );

	pRenderContext->ClearColor4ub( 255, 255, 255, 255 );

#if defined( _X360 )
	Assert(0); // rebalance this if we ever use this on 360
	pRenderContext->PushVertexShaderGPRAllocation( 112 ); //almost all work is done in vertex shaders for depth rendering, max out their threads
#endif

	pRenderContext.SafeRelease();

	if( IsPC() )
	{
		render->Push3DView( (*this), VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, pSSAO, GetFrustum() );
	}
	else if( IsX360() )
	{
		render->Push3DView( (*this), VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, pSSAO, GetFrustum() );
	}

	MDLCACHE_CRITICAL_SECTION();

	engine->Sound_ExtraUpdate();	// Make sure sound doesn't stutter

	m_DrawFlags |= DF_SSAO_DEPTH_PASS;

	{
		VPROF_BUDGET( "DrawWorld", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		DrawWorld( 0.0f );
	}

	// Draw opaque and translucent renderables with appropriate override materials
	// OVERRIDE_SSAO_DEPTH_WRITE is OK with a NULL material pointer
	modelrender->ForcedMaterialOverride( NULL, OVERRIDE_SSAO_DEPTH_WRITE );	

	{
		VPROF_BUDGET( "DrawOpaqueRenderables", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		DrawOpaqueRenderables( DEPTH_MODE_SSA0 );
	}

#if 0
	if ( m_bRenderFlashlightDepthTranslucents || r_flashlightdepth_drawtranslucents.GetBool() )
	{
		VPROF_BUDGET( "DrawTranslucentRenderables", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		DrawTranslucentRenderables( false, true );
	}
#endif

	modelrender->ForcedMaterialOverride( 0 );

	m_DrawFlags &= ~DF_SSAO_DEPTH_PASS;

	pRenderContext.GetFrom( materials );

	if( IsX360() )
	{
		//Resolve() the depth texture here. Before the pop so the copy will recognize that the resolutions are the same
		pRenderContext->CopyRenderTargetToTextureEx( NULL, -1, NULL, NULL );
	}

	render->PopView( GetFrustum() );

#if defined( _X360 )
	pRenderContext->PopVertexShaderGPRAllocation();
#endif

	pRenderContext.SafeRelease();

	g_CurrentViewID = savedViewID;
#endif
}


void CBaseWorldView::DrawDepthOfField( )
{
	if ( !g_pMaterialSystemHardwareConfig->SupportsPixelShaders_2_0() )
	{
		return;
	}

	CMatRenderContextPtr pRenderContext( materials );

	ITexture *pSmallFB0 = materials->FindTexture( "_rt_smallfb0", TEXTURE_GROUP_RENDER_TARGET );
	ITexture *pSmallFB1 = materials->FindTexture( "_rt_smallfb1", TEXTURE_GROUP_RENDER_TARGET );

	Rect_t	DestRect;
	int w = pSmallFB0->GetActualWidth();
	int h = pSmallFB0->GetActualHeight();
	DestRect.x = 0;
	DestRect.y = 0;
	DestRect.width = w;
	DestRect.height = h;

	pRenderContext->CopyRenderTargetToTextureEx( pSmallFB0, 0, NULL, &DestRect );

	IMaterial *pPyroBlurXMaterial = materials->FindMaterial( "dev/pyro_blur_filter_x", TEXTURE_GROUP_OTHER );
	IMaterial *pPyroBlurYMaterial = materials->FindMaterial( "dev/pyro_blur_filter_y", TEXTURE_GROUP_OTHER );

	pRenderContext->PushRenderTargetAndViewport( pSmallFB1, 0, 0, w, h );
	pRenderContext->DrawScreenSpaceRectangle( pPyroBlurYMaterial, 0, 0, w, h, 0, 0, w - 1, h - 1, w, h );
	pRenderContext->PopRenderTargetAndViewport();

	pRenderContext->PushRenderTargetAndViewport( pSmallFB0, 0, 0, w, h );
	pRenderContext->DrawScreenSpaceRectangle( pPyroBlurXMaterial, 0, 0, w, h, 0, 0, w - 1, h - 1, w, h );
	pRenderContext->PopRenderTargetAndViewport();

	IMaterial *pPyroDepthOfFieldMaterial = materials->FindMaterial( "dev/pyro_dof",  TEXTURE_GROUP_OTHER );

	pRenderContext->DrawScreenSpaceRectangle( pPyroDepthOfFieldMaterial, x, y, width, height, 0, 0, width-1, height-1, width, height );
}

//-----------------------------------------------------------------------------
// Draws the scene when there's no water or only cheap water
//-----------------------------------------------------------------------------
void CSimpleWorldView::Setup( const CViewSetup &simpleView, int nClearFlags, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t &waterInfo, ViewCustomVisibility_t *pCustomVisibility )
{
	BaseClass::Setup( simpleView );

	m_ClearFlags = nClearFlags;
	m_DrawFlags = DF_DRAW_ENTITITES;

	if ( !waterInfo.m_bOpaqueWater )
	{
		m_DrawFlags |= DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER;
	}
	else
	{
		bool bViewIntersectsWater = DoesViewPlaneIntersectWater( fogInfo.m_flWaterHeight, fogInfo.m_nVisibleFogVolume );
		if( bViewIntersectsWater )
		{
			// have to draw both sides if we can see both.
			m_DrawFlags |= DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER;
		}
		else if ( fogInfo.m_bEyeInFogVolume )
		{
			m_DrawFlags |= DF_RENDER_UNDERWATER;
		}
		else
		{
			m_DrawFlags |= DF_RENDER_ABOVEWATER;
		}
	}
	if ( waterInfo.m_bDrawWaterSurface )
	{
		m_DrawFlags |= DF_RENDER_WATER;
	}

	if ( !fogInfo.m_bEyeInFogVolume && bDrawSkybox )
	{
		m_DrawFlags |= DF_DRAWSKYBOX;
	}

	m_pCustomVisibility = pCustomVisibility;
	m_fogInfo = fogInfo;
}


//-----------------------------------------------------------------------------
// Draws the scene when there's no water or only cheap water
//-----------------------------------------------------------------------------
void CSimpleWorldView::Draw()
{
	VPROF( "CViewRender::ViewDrawScene_NoWater" );

	CMatRenderContextPtr pRenderContext( materials );
	PIXEVENT( pRenderContext, "CSimpleWorldView::Draw" );

#if defined( _X360 )
	pRenderContext->PushVertexShaderGPRAllocation( 32 ); //lean toward pixel shader threads
#endif

	pRenderContext.SafeRelease();

	DrawSetup( 0, m_DrawFlags, 0 );

	if ( !m_fogInfo.m_bEyeInFogVolume )
	{
		EnableWorldFog();
	}
	else
	{
		m_ClearFlags |= VIEW_CLEAR_COLOR;

		SetFogVolumeState( m_fogInfo, false );

		pRenderContext.GetFrom( materials );

		unsigned char ucFogColor[3];
		pRenderContext->GetFogColor( ucFogColor );
		pRenderContext->ClearColor4ub( ucFogColor[0], ucFogColor[1], ucFogColor[2], 255 );
	}

	pRenderContext.SafeRelease();

	DrawExecute( 0, CurrentViewID(), 0 );

	pRenderContext.GetFrom( materials );
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );

#if defined( _X360 )
	pRenderContext->PopVertexShaderGPRAllocation();
#endif
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CBaseWaterView::CalcWaterEyeAdjustments( const VisibleFogVolumeInfo_t &fogInfo,
											 float &newWaterHeight, float &waterZAdjust, bool bSoftwareUserClipPlane )
{
	if( !bSoftwareUserClipPlane )
	{
		newWaterHeight = fogInfo.m_flWaterHeight;
		waterZAdjust = 0.0f;
		return;
	}

	newWaterHeight = fogInfo.m_flWaterHeight;
	float eyeToWaterZDelta = origin[2] - fogInfo.m_flWaterHeight;
	float epsilon = r_eyewaterepsilon.GetFloat();
	waterZAdjust = 0.0f;
	if( fabs( eyeToWaterZDelta ) < epsilon )
	{
		if( eyeToWaterZDelta > 0 )
		{
			newWaterHeight = origin[2] - epsilon;
		}
		else
		{
			newWaterHeight = origin[2] + epsilon;
		}
		waterZAdjust = newWaterHeight - fogInfo.m_flWaterHeight;
	}

	//	Warning( "view.origin[2]: %f newWaterHeight: %f fogInfo.m_flWaterHeight: %f waterZAdjust: %f\n", 
	//		( float )view.origin[2], newWaterHeight, fogInfo.m_flWaterHeight, waterZAdjust );
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CBaseWaterView::CSoftwareIntersectionView::Setup( bool bAboveWater )
{
	BaseClass::Setup( *GetOuter() );

	m_DrawFlags = 0;
	m_DrawFlags = ( bAboveWater ) ? DF_RENDER_UNDERWATER : DF_RENDER_ABOVEWATER;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CBaseWaterView::CSoftwareIntersectionView::Draw()
{
	DrawSetup( GetOuter()->m_waterHeight, m_DrawFlags, GetOuter()->m_waterZAdjust );
	DrawExecute( GetOuter()->m_waterHeight, CurrentViewID(), GetOuter()->m_waterZAdjust );
}

//-----------------------------------------------------------------------------
// Draws the scene when the view point is above the level of the water
//-----------------------------------------------------------------------------
void CAboveWaterView::Setup( const CViewSetup &viewRender, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& waterInfo )
{
	BaseClass::Setup( viewRender );

	m_bSoftwareUserClipPlane = g_pMaterialSystemHardwareConfig->UseFastClipping();

	CalcWaterEyeAdjustments( fogInfo, m_waterHeight, m_waterZAdjust, m_bSoftwareUserClipPlane );

	// BROKEN STUFF!
	if ( m_waterZAdjust == 0.0f )
	{
		m_bSoftwareUserClipPlane = false;
	}

	m_DrawFlags = DF_RENDER_ABOVEWATER | DF_DRAW_ENTITITES;
	m_ClearFlags = VIEW_CLEAR_DEPTH;

#ifdef PORTAL
	if( g_pPortalRender->ShouldObeyStencilForClears() )
		m_ClearFlags |= VIEW_CLEAR_OBEY_STENCIL;
#endif

	if ( bDrawSkybox )
	{
		m_DrawFlags |= DF_DRAWSKYBOX;
	}

	if ( waterInfo.m_bDrawWaterSurface )
	{
		m_DrawFlags |= DF_RENDER_WATER;
	}
	if ( !waterInfo.m_bRefract && !waterInfo.m_bOpaqueWater )
	{
		m_DrawFlags |= DF_RENDER_UNDERWATER;
	}

	m_fogInfo = fogInfo;
	m_waterInfo = waterInfo;
}

		 
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::Draw()
{
	VPROF( "CViewRender::ViewDrawScene_EyeAboveWater" );

	// eye is outside of water
	
	CMatRenderContextPtr pRenderContext( materials );
	
	// render the reflection
	if( m_waterInfo.m_bReflect )
	{
		m_ReflectionView.Setup( m_waterInfo.m_bReflectEntities );
		m_pMainView->AddViewToScene( &m_ReflectionView );
	}
	
	bool bViewIntersectsWater = false;

	// render refraction
	if ( m_waterInfo.m_bRefract )
	{
		m_RefractionView.Setup();
		m_pMainView->AddViewToScene( &m_RefractionView );

		if( !m_bSoftwareUserClipPlane )
		{
			bViewIntersectsWater = DoesViewPlaneIntersectWater( m_fogInfo.m_flWaterHeight, m_fogInfo.m_nVisibleFogVolume );
		}
	}
	else if ( !( m_DrawFlags & DF_DRAWSKYBOX ) )
	{
		m_ClearFlags |= VIEW_CLEAR_COLOR;
	}

#ifdef PORTAL
	if( g_pPortalRender->ShouldObeyStencilForClears() )
		m_ClearFlags |= VIEW_CLEAR_OBEY_STENCIL;
#endif

	// NOTE!!!!!  YOU CAN ONLY DO THIS IF YOU HAVE HARDWARE USER CLIP PLANES!!!!!!
	bool bHardwareUserClipPlanes = !g_pMaterialSystemHardwareConfig->UseFastClipping();
	if( bViewIntersectsWater && bHardwareUserClipPlanes )
	{
		// This is necessary to keep the non-water fogged world from drawing underwater in 
		// the case where we want to partially see into the water.
		m_DrawFlags |= DF_CLIP_Z | DF_CLIP_BELOW;
	}

	// render the world
	DrawSetup( m_waterHeight, m_DrawFlags, m_waterZAdjust );
	EnableWorldFog();
	DrawExecute( m_waterHeight, CurrentViewID(), m_waterZAdjust );

	if ( m_waterInfo.m_bRefract )
	{
		if ( m_bSoftwareUserClipPlane )
		{
			m_SoftwareIntersectionView.Setup( true );
			m_SoftwareIntersectionView.Draw( );
		}
		else if ( bViewIntersectsWater )
		{
			m_IntersectionView.Setup();
			m_pMainView->AddViewToScene( &m_IntersectionView );
		}
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::CReflectionView::Setup( bool bReflectEntities )
{
	BaseClass::Setup( *GetOuter() );

	m_ClearFlags = VIEW_CLEAR_DEPTH;

	// NOTE: Clearing the color is unnecessary since we're drawing the skybox
	// and dest-alpha is never used in the reflection
	m_DrawFlags = DF_RENDER_REFLECTION | DF_CLIP_Z | DF_CLIP_BELOW | 
		DF_RENDER_ABOVEWATER;

	// NOTE: This will cause us to draw the 2d skybox in the reflection 
	// (which we want to do instead of drawing the 3d skybox)
	m_DrawFlags |= DF_DRAWSKYBOX;

	if( bReflectEntities )
	{
		m_DrawFlags |= DF_DRAW_ENTITITES;
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::CReflectionView::Draw()
{
#ifdef PORTAL
	g_pPortalRender->WaterRenderingHandler_PreReflection();
#endif

	// Store off view origin and angles and set the new view
	int nSaveViewID = CurrentViewID();
	SetupCurrentView( origin, angles, VIEW_REFLECTION );

	// Disable occlusion visualization in reflection
	bool bVisOcclusion = r_visocclusion.GetInt();
	r_visocclusion.SetValue( 0 );

	DrawSetup( GetOuter()->m_fogInfo.m_flWaterHeight, m_DrawFlags, 0.0f, GetOuter()->m_fogInfo.m_nVisibleFogVolumeLeaf );

	EnableWorldFog();
	DrawExecute( GetOuter()->m_fogInfo.m_flWaterHeight, VIEW_REFLECTION, 0.0f );

	r_visocclusion.SetValue( bVisOcclusion );
	
#ifdef PORTAL
	// deal with stencil
	g_pPortalRender->WaterRenderingHandler_PostReflection();
#endif

	// finish off the view and restore the previous view.
	SetupCurrentView( origin, angles, ( view_id_t )nSaveViewID );

	// This is here for multithreading
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Flush();
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::CRefractionView::Setup()
{
	BaseClass::Setup( *GetOuter() );

	m_ClearFlags = VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH;

	m_DrawFlags = DF_RENDER_REFRACTION | DF_CLIP_Z | 
		DF_RENDER_UNDERWATER | DF_FUDGE_UP | 
		DF_DRAW_ENTITITES ;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::CRefractionView::Draw()
{
#ifdef PORTAL
	g_pPortalRender->WaterRenderingHandler_PreRefraction();
#endif

	// Store off view origin and angles and set the new view
	int nSaveViewID = CurrentViewID();
	SetupCurrentView( origin, angles, VIEW_REFRACTION );

	DrawSetup( GetOuter()->m_waterHeight, m_DrawFlags, GetOuter()->m_waterZAdjust );

	SetFogVolumeState( GetOuter()->m_fogInfo, true );
	SetClearColorToFogColor();
	DrawExecute( GetOuter()->m_waterHeight, VIEW_REFRACTION, GetOuter()->m_waterZAdjust );

#ifdef PORTAL
	// deal with stencil
	g_pPortalRender->WaterRenderingHandler_PostRefraction();
#endif

	// finish off the view.  restore the previous view.
	SetupCurrentView( origin, angles, ( view_id_t )nSaveViewID );

	// This is here for multithreading
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
	pRenderContext->Flush();
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::CIntersectionView::Setup()
{
	BaseClass::Setup( *GetOuter() );
	m_DrawFlags = DF_RENDER_UNDERWATER | DF_CLIP_Z | DF_DRAW_ENTITITES;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::CIntersectionView::Draw()
{
	DrawSetup( GetOuter()->m_fogInfo.m_flWaterHeight, m_DrawFlags, 0 );

	SetFogVolumeState( GetOuter()->m_fogInfo, true );
	SetClearColorToFogColor( );
	DrawExecute( GetOuter()->m_fogInfo.m_flWaterHeight, VIEW_NONE, 0 );
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
}


//-----------------------------------------------------------------------------
// Draws the scene when the view point is under the level of the water
//-----------------------------------------------------------------------------
void CUnderWaterView::Setup( const CViewSetup &underView, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& waterInfo )
{
	BaseClass::Setup( underView );

	m_bSoftwareUserClipPlane = g_pMaterialSystemHardwareConfig->UseFastClipping();

	CalcWaterEyeAdjustments( fogInfo, m_waterHeight, m_waterZAdjust, m_bSoftwareUserClipPlane );

	IMaterial *pWaterMaterial = fogInfo.m_pFogVolumeMaterial;
	if (engine->GetDXSupportLevel() >= 90 )					// screen voerlays underwater are a dx9 feature
	{
		IMaterialVar *pScreenOverlayVar = pWaterMaterial->FindVar( "$underwateroverlay", NULL, false );
		if ( pScreenOverlayVar && ( pScreenOverlayVar->IsDefined() ) )
		{
			char const *pOverlayName = pScreenOverlayVar->GetStringValue();
			if ( pOverlayName[0] != '0' )						// fixme!!!
			{
				IMaterial *pOverlayMaterial = materials->FindMaterial( pOverlayName,  TEXTURE_GROUP_OTHER );
				m_pMainView->SetWaterOverlayMaterial( pOverlayMaterial );
			}
		}
	}
	// NOTE: We're not drawing the 2d skybox under water since it's assumed to not be visible.

	// render the world underwater
	// Clear the color to get the appropriate underwater fog color
	m_DrawFlags = DF_FUDGE_UP | DF_RENDER_UNDERWATER | DF_DRAW_ENTITITES;
	m_ClearFlags = VIEW_CLEAR_DEPTH;

	if( !m_bSoftwareUserClipPlane )
	{
		m_DrawFlags |= DF_CLIP_Z;
	}
	if ( waterInfo.m_bDrawWaterSurface )
	{
		m_DrawFlags |= DF_RENDER_WATER;
	}
	if ( !waterInfo.m_bRefract && !waterInfo.m_bOpaqueWater )
	{
		m_DrawFlags |= DF_RENDER_ABOVEWATER;
	}

	m_fogInfo = fogInfo;
	m_waterInfo = waterInfo;
	m_bDrawSkybox = bDrawSkybox;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CUnderWaterView::Draw()
{
	// FIXME: The 3d skybox shouldn't be drawn when the eye is under water

	VPROF( "CViewRender::ViewDrawScene_EyeUnderWater" );

	CMatRenderContextPtr pRenderContext( materials );

	// render refraction (out of water)
	if ( m_waterInfo.m_bRefract )
	{
		m_RefractionView.Setup( );
		m_pMainView->AddViewToScene( &m_RefractionView );
	}

	if ( !m_waterInfo.m_bRefract )
	{
		SetFogVolumeState( m_fogInfo, true );
		unsigned char ucFogColor[3];
		pRenderContext->GetFogColor( ucFogColor );
		pRenderContext->ClearColor4ub( ucFogColor[0], ucFogColor[1], ucFogColor[2], 255 );
	}

	DrawSetup( m_waterHeight, m_DrawFlags, m_waterZAdjust );
	SetFogVolumeState( m_fogInfo, false );
	DrawExecute( m_waterHeight, CurrentViewID(), m_waterZAdjust );
	m_ClearFlags = 0;

	if( m_waterZAdjust != 0.0f && m_bSoftwareUserClipPlane && m_waterInfo.m_bRefract )
	{
		m_SoftwareIntersectionView.Setup( false );
		m_SoftwareIntersectionView.Draw( );
	}
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );

}



//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CUnderWaterView::CRefractionView::Setup()
{
	BaseClass::Setup( *GetOuter() );
	// NOTE: Refraction renders into the back buffer, over the top of the 3D skybox
	// It is then blitted out into the refraction target. This is so that
	// we only have to set up 3d sky vis once, and only render it once also!
	m_DrawFlags = DF_CLIP_Z | 
		DF_CLIP_BELOW | DF_RENDER_ABOVEWATER | 
		DF_DRAW_ENTITITES;

	m_ClearFlags = VIEW_CLEAR_DEPTH;
	if ( GetOuter()->m_bDrawSkybox )
	{
		m_ClearFlags |= VIEW_CLEAR_COLOR;
		m_DrawFlags |= DF_DRAWSKYBOX | DF_CLIP_SKYBOX;
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CUnderWaterView::CRefractionView::Draw()
{
	CMatRenderContextPtr pRenderContext( materials );
	SetFogVolumeState( GetOuter()->m_fogInfo, true );
	unsigned char ucFogColor[3];
	pRenderContext->GetFogColor( ucFogColor );
	pRenderContext->ClearColor4ub( ucFogColor[0], ucFogColor[1], ucFogColor[2], 255 );

	DrawSetup( GetOuter()->m_waterHeight, m_DrawFlags, GetOuter()->m_waterZAdjust );

	EnableWorldFog();
	DrawExecute( GetOuter()->m_waterHeight, VIEW_REFRACTION, GetOuter()->m_waterZAdjust );

	Rect_t srcRect;
	srcRect.x = x;
	srcRect.y = y;
	srcRect.width = width;
	srcRect.height = height;

	// Optionally write the rendered image to a debug texture
	if ( g_bDumpRenderTargets )
	{
		DumpTGAofRenderTarget( width, height, "WaterRefract" );
	}

	ITexture *pTexture = GetWaterRefractionTexture();
	pRenderContext->CopyRenderTargetToTextureEx( pTexture, 0, &srcRect, NULL );
}


//-----------------------------------------------------------------------------
//
// Reflective glass view starts here
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Draws the scene when the view contains reflective glass
//-----------------------------------------------------------------------------
void CReflectiveGlassView::Setup( const CViewSetup &viewRender, int nClearFlags, bool bDrawSkybox,
	const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t &waterInfo, const cplane_t &reflectionPlane )
{
	BaseClass::Setup( viewRender, nClearFlags, bDrawSkybox, fogInfo, waterInfo, NULL );
	m_ReflectionPlane = reflectionPlane;
}


bool CReflectiveGlassView::AdjustView( float flWaterHeight )
{
	ITexture *pTexture = GetWaterReflectionTexture();
		   
	// Use the aspect ratio of the main view! So, don't recompute it here
	x = y = 0;
	width = pTexture->GetActualWidth();
	height = pTexture->GetActualHeight();

	// Reflect the camera origin + vectors around the reflection plane 
	float flDist = DotProduct( origin, m_ReflectionPlane.normal ) - m_ReflectionPlane.dist;
	VectorMA( origin, - 2.0f * flDist, m_ReflectionPlane.normal, origin );

	Vector vecForward, vecUp;
	AngleVectors( angles, &vecForward, NULL, &vecUp );

	float flDot = DotProduct( vecForward, m_ReflectionPlane.normal );
	VectorMA( vecForward, - 2.0f * flDot, m_ReflectionPlane.normal, vecForward );

	flDot = DotProduct( vecUp, m_ReflectionPlane.normal );
	VectorMA( vecUp, - 2.0f * flDot, m_ReflectionPlane.normal, vecUp );

	VectorAngles( vecForward, vecUp, angles );
	return true;
}

void CReflectiveGlassView::PushView( float waterHeight )
{
	render->Push3DView( *this, m_ClearFlags, GetWaterReflectionTexture(), GetFrustum() );
	 
	Vector4D plane;
	VectorCopy( m_ReflectionPlane.normal, plane.AsVector3D() );
	plane.w = m_ReflectionPlane.dist + 0.1f;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->PushCustomClipPlane( plane.Base() );
}

void CReflectiveGlassView::PopView( )
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->PopCustomClipPlane( );
	render->PopView( GetFrustum() );
}


//-----------------------------------------------------------------------------
// Renders reflective or refractive parts of glass
//-----------------------------------------------------------------------------
void CReflectiveGlassView::Draw()
{
	VPROF( "CReflectiveGlassView::Draw" );

	CMatRenderContextPtr pRenderContext( materials );
	PIXEVENT( pRenderContext, "CReflectiveGlassView::Draw" );

	// Disable occlusion visualization in reflection
	bool bVisOcclusion = r_visocclusion.GetInt();
	r_visocclusion.SetValue( 0 );
				   
	BaseClass::Draw();

	r_visocclusion.SetValue( bVisOcclusion );

	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
	pRenderContext->Flush();
}



//-----------------------------------------------------------------------------
// Draws the scene when the view contains reflective glass
//-----------------------------------------------------------------------------
void CRefractiveGlassView::Setup( const CViewSetup &viewRender, int nClearFlags, bool bDrawSkybox,
	const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t &waterInfo, const cplane_t &reflectionPlane )
{
	BaseClass::Setup( viewRender, nClearFlags, bDrawSkybox, fogInfo, waterInfo, NULL );
	m_ReflectionPlane = reflectionPlane;
}


bool CRefractiveGlassView::AdjustView( float flWaterHeight )
{
	ITexture *pTexture = GetWaterRefractionTexture();

	// Use the aspect ratio of the main view! So, don't recompute it here
	x = y = 0;
	width = pTexture->GetActualWidth();
	height = pTexture->GetActualHeight();
	return true;
}


void CRefractiveGlassView::PushView( float waterHeight )
{
	render->Push3DView( *this, m_ClearFlags, GetWaterRefractionTexture(), GetFrustum() );

	Vector4D plane;
	VectorMultiply( m_ReflectionPlane.normal, -1, plane.AsVector3D() );
	plane.w = -m_ReflectionPlane.dist + 0.1f;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->PushCustomClipPlane( plane.Base() );
}


void CRefractiveGlassView::PopView( )
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->PopCustomClipPlane( );
	render->PopView( GetFrustum() );
}



//-----------------------------------------------------------------------------
// Renders reflective or refractive parts of glass
//-----------------------------------------------------------------------------
void CRefractiveGlassView::Draw()
{
	VPROF( "CRefractiveGlassView::Draw" );

	CMatRenderContextPtr pRenderContext( materials );
	PIXEVENT( pRenderContext, "CRefractiveGlassView::Draw" );

	BaseClass::Draw();

	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
	pRenderContext->Flush();
}
