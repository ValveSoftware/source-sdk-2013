// ******************************************************
//
// Purpose:
//		-	Connects the shader editor
//		-	Sends data from the main viewsetup
//		-	exposes client callbacks to shaders
// 
// ******************************************************

#include "cbase.h"
#include "client_factorylist.h"
#include "ShaderEditor/IVShaderEditor.h"
#include "ShaderEditor/SEdit_ModelRender.h"
#include "ivrenderview.h"
#include "iviewrender.h"
#include "viewrender.h"
#include "view.h"
#include "view_scene.h"
#include "view_shared.h"
#include "beamdraw.h"
#include "c_sun.h"
#include "tier0/icommandline.h"
#include "rendertexture.h"
#include "c_rope.h"
#include "model_types.h"
#ifdef SWARM_DLL
#include "modelrendersystem.h"
#endif


#if SWARM_DLL
#define Editor_MainViewOrigin MainViewOrigin( 0 )
#define Editor_MainViewForward MainViewForward( 0 )
#else
#define Editor_MainViewOrigin MainViewOrigin()
#define Editor_MainViewForward MainViewForward()
#endif


ShaderEditorHandler __g_ShaderEditorSystem( "ShEditUpdate" );
ShaderEditorHandler *g_ShaderEditorSystem = &__g_ShaderEditorSystem;

CSysModule *shaderEditorModule = NULL;
IVShaderEditor *shaderEdit = NULL;

ShaderEditorHandler::ShaderEditorHandler( char const *name ) : CAutoGameSystemPerFrame( name )
{
	m_bReady = false;
	m_piCurrentViewId = NULL;
}

ShaderEditorHandler::~ShaderEditorHandler()
{
}

const bool ShaderEditorHandler::IsReady()
{
	return m_bReady;
}

bool ShaderEditorHandler::Init()
{
	factorylist_t factories;
	FactoryList_Retrieve( factories );

#ifdef SOURCE_2006
	ConVar *pCVarDev = cvar->FindVar( "developer" );
	bool bShowPrimDebug = pCVarDev != NULL && pCVarDev->GetInt() != 0;
#else
	ConVarRef devEnabled( "developer", true );
	bool bShowPrimDebug = devEnabled.GetInt() != 0;
#endif

	bool bCreateEditor = ( CommandLine() != NULL ) && ( CommandLine()->FindParm( "-shaderedit" ) != 0 );
	SEDIT_SKYMASK_MODE iEnableSkymask = SKYMASK_OFF;

#ifdef SHADEREDITOR_FORCE_ENABLED
	bCreateEditor = true;
	iEnableSkymask = SKYMASK_QUARTER;
#endif

	char modulePath[MAX_PATH*4];
#ifdef SWARM_DLL
	Q_snprintf( modulePath, sizeof( modulePath ), "%s/bin/shadereditor_swarm.dll\0", engine->GetGameDirectory() );
#elif SOURCE_2006
	Q_snprintf( modulePath, sizeof( modulePath ), "%s/bin/shadereditor_2006.dll\0", engine->GetGameDirectory() );
#elif SOURCE_2013
	Q_snprintf( modulePath, sizeof( modulePath ), "%s/bin/shadereditor_2013.dll\0", engine->GetGameDirectory() );
#else
	Q_snprintf( modulePath, sizeof( modulePath ), "%s/bin/shadereditor_2007.dll\0", engine->GetGameDirectory() );
#endif
	shaderEditorModule = Sys_LoadModule( modulePath );
	if ( shaderEditorModule )
	{
		CreateInterfaceFn shaderEditorDLLFactory = Sys_GetFactory( shaderEditorModule );
		shaderEdit = shaderEditorDLLFactory ? ((IVShaderEditor *) shaderEditorDLLFactory( SHADEREDIT_INTERFACE_VERSION, NULL )) : NULL;

		if ( !shaderEdit )
		{
			Warning( "Unable to pull IVShaderEditor interface.\n" );
		}
		else if ( !shaderEdit->Init( factories.appSystemFactory, gpGlobals, sEditMRender,
				bCreateEditor, bShowPrimDebug, iEnableSkymask ) )
		{
			Warning( "Cannot initialize IVShaderEditor.\n" );
			shaderEdit = NULL;
		}
	}
	else
	{
		Warning( "Cannot load shadereditor.dll from %s!\n", modulePath );
	}

	m_bReady = shaderEdit != NULL;

	RegisterCallbacks();
	RegisterViewRenderCallbacks();

	if ( IsReady() )
	{
		shaderEdit->PrecacheData();
	}

	return true;
}

#ifdef SHADEREDITOR_FORCE_ENABLED
CON_COMMAND( sedit_debug_toggle_ppe, "" )
{
	if ( !g_ShaderEditorSystem->IsReady() )
		return Warning( "lib not ready.\n" );

	if ( args.ArgC() < 2 )
		return;

	const int idx = shaderEdit->GetPPEIndex( args[1] );
	if ( idx < 0 )
		return Warning( "can't find ppe named: %s\n", args[1] );

	shaderEdit->SetPPEEnabled( idx, !shaderEdit->IsPPEEnabled( idx ) );
}
#endif

void ShaderEditorHandler::Shutdown()
{
	if ( shaderEdit )
		shaderEdit->Shutdown();
	if ( shaderEditorModule )
		Sys_UnloadModule( shaderEditorModule );
}

void ShaderEditorHandler::Update( float frametime )
{
	if ( IsReady() )
		shaderEdit->OnFrame( frametime );
}

CThreadMutex m_Lock;

void ShaderEditorHandler::PreRender()
{
	if ( IsReady() && view )
	{
		// make sure the class matches
		const CViewSetup *v = view->GetPlayerViewSetup();
		CViewSetup_SEdit_Shared stableVSetup( *v );
		shaderEdit->OnPreRender( &stableVSetup );

		m_Lock.Lock();
		PrepareCallbackData();
		m_Lock.Unlock();
	}
}
void ShaderEditorHandler::PostRender()
{
}
#ifdef SOURCE_2006
void ShaderEditorHandler::CustomViewRender( int *viewId, const VisibleFogVolumeInfo_t &fogVolumeInfo )
#else
void ShaderEditorHandler::CustomViewRender( int *viewId, const VisibleFogVolumeInfo_t &fogVolumeInfo, const WaterRenderInfo_t &waterRenderInfo )
#endif
{
	m_piCurrentViewId = viewId;
	m_tFogVolumeInfo = fogVolumeInfo;

#ifndef SOURCE_2006
	m_tWaterRenderInfo = waterRenderInfo;
#endif

	if ( IsReady() )
		shaderEdit->OnSceneRender();
}
void ShaderEditorHandler::UpdateSkymask( bool bCombineMode )
{
	if ( IsReady() )
		shaderEdit->OnUpdateSkymask( bCombineMode );
}
void ShaderEditorHandler::CustomPostRender()
{
	if ( IsReady() )
		shaderEdit->OnPostRender( true );
}

struct CallbackData_t
{
	void Reset()
	{
		sun_data.Init();
		sun_dir.Init();

		player_speed.Init();
		player_pos.Init();
	};
	Vector4D sun_data;
	Vector sun_dir;

	Vector4D player_speed;
	Vector player_pos;
};

static CallbackData_t clCallback_data;

void ShaderEditorHandler::PrepareCallbackData()
{
	clCallback_data.Reset();

	float flSunAmt_Goal = 0;
	static float s_flSunAmt_Last = 0;

	C_BaseEntity *pEnt = ClientEntityList().FirstBaseEntity();
	while ( pEnt )
	{
		if ( !Q_stricmp( pEnt->GetClassname(), "class C_Sun" ) )
		{
			C_Sun *pSun = ( C_Sun* )pEnt;
			Vector dir = pSun->m_vDirection;
			dir.NormalizeInPlace();

			Vector screen;

			if ( ScreenTransform( Editor_MainViewOrigin + dir * 512, screen ) )
				ScreenTransform( (Editor_MainViewOrigin - dir * 512), screen );

			screen = screen * Vector( 0.5f, -0.5f, 0 ) + Vector( 0.5f, 0.5f, 0 );

			Q_memcpy( clCallback_data.sun_data.Base(), screen.Base(), sizeof(float) * 2 );
			clCallback_data.sun_data[ 2 ] = DotProduct( dir, Editor_MainViewForward );
			clCallback_data.sun_dir = dir;

			trace_t tr;
			UTIL_TraceLine( Editor_MainViewOrigin, Editor_MainViewOrigin + dir * MAX_TRACE_LENGTH, MASK_SOLID, NULL, COLLISION_GROUP_DEBRIS, &tr );
			if ( !tr.DidHitWorld() )
				break;

			if ( tr.surface.flags & SURF_SKY )
				flSunAmt_Goal = 1;

			break;
		}
		pEnt = ClientEntityList().NextBaseEntity( pEnt );
	}

	if ( s_flSunAmt_Last != flSunAmt_Goal )
		s_flSunAmt_Last = Approach( flSunAmt_Goal, s_flSunAmt_Last, gpGlobals->frametime * ( (!!flSunAmt_Goal) ? 4.0f : 0.75f ) );

	clCallback_data.sun_data[ 3 ] = s_flSunAmt_Last;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		Vector velo = pPlayer->GetLocalVelocity();
		clCallback_data.player_speed[ 3 ] = velo.NormalizeInPlace();
		Q_memcpy( clCallback_data.player_speed.Base(), velo.Base(), sizeof(float) * 3 );

		clCallback_data.player_pos = pPlayer->GetLocalOrigin();
	}
}

pFnClCallback_Declare( ClCallback_SunData )
{
	m_Lock.Lock();
	Q_memcpy( pfl4, clCallback_data.sun_data.Base(), sizeof(float) * 4 );
	m_Lock.Unlock();
}

pFnClCallback_Declare( ClCallback_SunDirection )
{
	m_Lock.Lock();
	Q_memcpy( pfl4, clCallback_data.sun_dir.Base(), sizeof(float) * 3 );
	m_Lock.Unlock();
}

pFnClCallback_Declare( ClCallback_PlayerVelocity )
{
	m_Lock.Lock();
	Q_memcpy( pfl4, clCallback_data.player_speed.Base(), sizeof(float) * 4 );
	m_Lock.Unlock();
}

pFnClCallback_Declare( ClCallback_PlayerPos )
{
	m_Lock.Lock();
	Q_memcpy( pfl4, clCallback_data.player_pos.Base(), sizeof(float) * 3 );
	m_Lock.Unlock();
}

void ShaderEditorHandler::RegisterCallbacks()
{
	if ( !IsReady() )
		return;

	// 4 components max
	shaderEdit->RegisterClientCallback( "sun data", ClCallback_SunData, 4 );
	shaderEdit->RegisterClientCallback( "sun dir", ClCallback_SunDirection, 3 );
	shaderEdit->RegisterClientCallback( "local player velocity", ClCallback_PlayerVelocity, 4 );
	shaderEdit->RegisterClientCallback( "local player position", ClCallback_PlayerPos, 3 );

	shaderEdit->LockClientCallbacks();
}

#ifdef SOURCE_2006

void ShaderEditorHandler::RegisterViewRenderCallbacks(){}

#else

extern bool DoesViewPlaneIntersectWater( float waterZ, int leafWaterDataID );

// copy pasta from baseworldview
class CBaseVCallbackView : public CRendering3dView
{
	DECLARE_CLASS( CBaseVCallbackView, CRendering3dView );
protected:

	CBaseVCallbackView( CViewRender *pMainView ) : CRendering3dView( pMainView )
	{
	};

	virtual bool	AdjustView( float waterHeight ){ return false; };

	virtual void CallbackInitRenderList( int viewId )
	{
		BuildRenderableRenderLists( viewId );
	};

	virtual bool ShouldDrawParticles()
	{
		return true;
	};

	virtual bool ShouldDrawRopes()
	{
		return true;
	};

	virtual bool ShouldDrawWorld()
	{
		return true;
	};

	virtual bool ShouldDrawTranslucents()
	{
		return true;
	};

	virtual bool ShouldDrawTranslucentWorld()
	{
		return true;
	};

	void DrawSetup( float waterHeight, int nSetupFlags, float waterZAdjust, int iForceViewLeaf = -1 )
	{
		int savedViewID = g_ShaderEditorSystem->GetViewIdForModify();
		
		g_ShaderEditorSystem->GetViewIdForModify() = VIEW_ILLEGAL;

		render->BeginUpdateLightmaps();

		bool bDrawEntities = ( nSetupFlags & DF_DRAW_ENTITITES ) != 0;
		BuildWorldRenderLists( bDrawEntities, iForceViewLeaf, true, false, NULL );
		PruneWorldListInfo();

		if ( bDrawEntities )
			CallbackInitRenderList( savedViewID );

		render->EndUpdateLightmaps();

		g_ShaderEditorSystem->GetViewIdForModify() = savedViewID;
	};

	void DrawExecute( float waterHeight, view_id_t viewID, float waterZAdjust )
	{
		// ClientWorldListInfo_t is defined in viewrender.cpp...
		//g_pClientShadowMgr->ComputeShadowTextures( *this, m_pWorldListInfo->m_LeafCount, m_pWorldListInfo->m_pLeafList );

		engine->Sound_ExtraUpdate();

		int savedViewID = g_ShaderEditorSystem->GetViewIdForModify();
		g_ShaderEditorSystem->GetViewIdForModify() = viewID;

		int iDrawFlagsBackup = m_DrawFlags;
		m_DrawFlags |= m_pMainView->GetBaseDrawFlags();

		PushView( waterHeight );

		CMatRenderContextPtr pRenderContext( materials );

		ITexture *pSaveFrameBufferCopyTexture = pRenderContext->GetFrameBufferCopyTexture( 0 );
		if ( engine->GetDXSupportLevel() >= 80 )
		{
			pRenderContext->SetFrameBufferCopyTexture( GetPowerOfTwoFrameBufferTexture() );
		}

		pRenderContext.SafeRelease();

		static ConVarRef translucentNoWorld( "r_drawtranslucentworld" );
		const int tnoWorldSaved = translucentNoWorld.GetInt();
		translucentNoWorld.SetValue( ShouldDrawWorld() ? 0 : 1 );

		if ( m_DrawFlags & DF_DRAW_ENTITITES )
		{
			if ( ShouldDrawWorld() )
				DrawWorld( waterZAdjust );

			DrawOpaqueRenderables_Custom( false );

			if ( ShouldDrawTranslucents() && ShouldDrawTranslucentWorld() )
				DrawTranslucentRenderables( false, false );
			else if ( ShouldDrawTranslucents() )
				DrawTranslucentRenderablesNoWorld( false );
			else if ( ShouldDrawTranslucentWorld() )
				DrawTranslucentWorldInLeaves( false );
		}
		else if ( ShouldDrawWorld() )
		{
			DrawWorld( waterZAdjust );

			if ( ShouldDrawTranslucentWorld() )
				DrawTranslucentWorldInLeaves( false );
		}

		translucentNoWorld.SetValue( tnoWorldSaved );

		if ( CurrentViewID() != VIEW_MAIN && CurrentViewID() != VIEW_INTRO_CAMERA )
			PixelVisibility_EndCurrentView();

		pRenderContext.GetFrom( materials );
		pRenderContext->SetFrameBufferCopyTexture( pSaveFrameBufferCopyTexture );
		PopView();

		m_DrawFlags = iDrawFlagsBackup;

		g_ShaderEditorSystem->GetViewIdForModify() = savedViewID;
	};

	virtual void	PushView( float waterHeight )
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

		if ( ( m_DrawFlags & DF_CLIP_Z ) )
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

		if ( m_ClearFlags & ( VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR | VIEW_CLEAR_STENCIL ) )
		{
			if ( m_ClearFlags & VIEW_CLEAR_OBEY_STENCIL )
			{
				pRenderContext->ClearBuffersObeyStencil( (m_ClearFlags & VIEW_CLEAR_COLOR) != 0, (m_ClearFlags & VIEW_CLEAR_DEPTH) != 0 );
			}
			else
			{
				pRenderContext->ClearBuffers( (m_ClearFlags & VIEW_CLEAR_COLOR) != 0, (m_ClearFlags & VIEW_CLEAR_DEPTH) != 0, (m_ClearFlags & VIEW_CLEAR_STENCIL) != 0 );
			}
		}

		pRenderContext->SetHeightClipMode( clipMode );
		if ( clipMode != MATERIAL_HEIGHTCLIPMODE_DISABLE )
		{
			pRenderContext->SetHeightClipZ( waterHeight );
		}
	};

	virtual void	PopView()
	{
		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->SetHeightClipMode( MATERIAL_HEIGHTCLIPMODE_DISABLE );
	};

	void DrawOpaqueRenderables_Custom( bool bShadowDepth )
	{
		//if( !r_drawopaquerenderables.GetBool() )
		//	return;

		if( !m_pMainView->ShouldDrawEntities() )
			return;

		render->SetBlend( 1 );

		const bool bRopes = ShouldDrawRopes();
		const bool bParticles = ShouldDrawParticles();

		//
		// Prepare to iterate over all leaves that were visible, and draw opaque things in them.	
		//
		if ( bRopes )
			RopeManager()->ResetRenderCache();
		if ( bParticles )
			g_pParticleSystemMgr->ResetRenderCache();

#ifdef SWARM_DLL

		extern ConVar cl_modelfastpath;
		extern ConVar r_drawothermodels;

		// Categorize models by type
		int nOpaqueRenderableCount = m_pRenderablesList->m_RenderGroupCounts[RENDER_GROUP_OPAQUE];
		CUtlVector< CClientRenderablesList::CEntry* > brushModels( (CClientRenderablesList::CEntry **)stackalloc( nOpaqueRenderableCount * sizeof( CClientRenderablesList::CEntry* ) ), nOpaqueRenderableCount );
		CUtlVector< CClientRenderablesList::CEntry* > staticProps( (CClientRenderablesList::CEntry **)stackalloc( nOpaqueRenderableCount * sizeof( CClientRenderablesList::CEntry* ) ), nOpaqueRenderableCount );
		CUtlVector< CClientRenderablesList::CEntry* > otherRenderables( (CClientRenderablesList::CEntry **)stackalloc( nOpaqueRenderableCount * sizeof( CClientRenderablesList::CEntry* ) ), nOpaqueRenderableCount );
		CClientRenderablesList::CEntry *pOpaqueList = m_pRenderablesList->m_RenderGroups[RENDER_GROUP_OPAQUE];
		for ( int i = 0; i < nOpaqueRenderableCount; ++i )
		{
			switch( pOpaqueList[i].m_nModelType )
			{
			case RENDERABLE_MODEL_BRUSH:		brushModels.AddToTail( &pOpaqueList[i] ); break; 
			case RENDERABLE_MODEL_STATIC_PROP:	staticProps.AddToTail( &pOpaqueList[i] ); break; 
			default:							otherRenderables.AddToTail( &pOpaqueList[i] ); break; 
			}
		}

		//
		// First do the brush models
		//
		DrawOpaqueRenderables_DrawBrushModels( brushModels.Count(), brushModels.Base(), bShadowDepth );

		// Move all static props to modelrendersystem
		bool bUseFastPath = ( cl_modelfastpath.GetInt() != 0 );

		//
		// Sort everything that's not a static prop
		//
		int nStaticPropCount = staticProps.Count();
		int numOpaqueEnts = otherRenderables.Count();
		CUtlVector< CClientRenderablesList::CEntry* > arrRenderEntsNpcsFirst( (CClientRenderablesList::CEntry **)stackalloc( numOpaqueEnts * sizeof( CClientRenderablesList::CEntry ) ), numOpaqueEnts );
		CUtlVector< ModelRenderSystemData_t > arrModelRenderables( (ModelRenderSystemData_t *)stackalloc( ( numOpaqueEnts + nStaticPropCount ) * sizeof( ModelRenderSystemData_t ) ), numOpaqueEnts + nStaticPropCount );

		// Queue up RENDER_GROUP_OPAQUE_ENTITY entities to be rendered later.
		CClientRenderablesList::CEntry *itEntity;
		if( r_drawothermodels.GetBool() )
		{
			for ( int i = 0; i < numOpaqueEnts; ++i )
			{
				itEntity = otherRenderables[i];
				if ( !itEntity->m_pRenderable )
					continue;

				IClientUnknown *pUnknown = itEntity->m_pRenderable->GetIClientUnknown();
				IClientModelRenderable *pModelRenderable = pUnknown->GetClientModelRenderable();
				C_BaseEntity *pEntity = pUnknown->GetBaseEntity();

				// FIXME: Strangely, some static props are in the non-static prop bucket
				// which is what the last case in this if statement is for
				if ( bUseFastPath && pModelRenderable )
				{
					ModelRenderSystemData_t data;
					data.m_pRenderable = itEntity->m_pRenderable;
					data.m_pModelRenderable = pModelRenderable;
					data.m_InstanceData = itEntity->m_InstanceData;
					arrModelRenderables.AddToTail( data );
					otherRenderables.FastRemove( i );
					--i; --numOpaqueEnts;
					continue;
				}

				if ( !pEntity )
					continue;

				if ( pEntity->IsNPC() )
				{
					arrRenderEntsNpcsFirst.AddToTail( itEntity );
					otherRenderables.FastRemove( i );
					--i; --numOpaqueEnts;
					continue;
				}
			}
		}

		// Queue up the static props to be rendered later.
		for ( int i = 0; i < nStaticPropCount; ++i )
		{
			itEntity = staticProps[i];
			if ( !itEntity->m_pRenderable )
				continue;

			IClientUnknown *pUnknown = itEntity->m_pRenderable->GetIClientUnknown();
			IClientModelRenderable *pModelRenderable = pUnknown->GetClientModelRenderable();
			if ( !bUseFastPath || !pModelRenderable )
				continue;

			ModelRenderSystemData_t data;
			data.m_pRenderable = itEntity->m_pRenderable;
			data.m_pModelRenderable = pModelRenderable;
			data.m_InstanceData = itEntity->m_InstanceData;
			arrModelRenderables.AddToTail( data );

			staticProps.FastRemove( i );
			--i; --nStaticPropCount;
		}

		//
		// Draw model renderables now (ie. models that use the fast path)
		//					 
		DrawOpaqueRenderables_ModelRenderables( arrModelRenderables.Count(), arrModelRenderables.Base(), bShadowDepth );

		// Turn off z pass here. Don't want non-fastpath models with potentially large dynamic VB requirements overwrite
		// stuff in the dynamic VB ringbuffer. We're calling End360ZPass again in DrawExecute, but that's not a problem.
		// Begin360ZPass/End360ZPass don't have to be matched exactly.
		End360ZPass();

		//
		// Draw static props + opaque entities that aren't using the fast path.
		//
		DrawOpaqueRenderables_Range( otherRenderables.Count(), otherRenderables.Base(), bShadowDepth );
		DrawOpaqueRenderables_DrawStaticProps( staticProps.Count(), staticProps.Base(), bShadowDepth );

		//
		// Draw NPCs now
		//
		DrawOpaqueRenderables_NPCs( arrRenderEntsNpcsFirst.Count(), arrRenderEntsNpcsFirst.Base(), bShadowDepth );
#else

		bool const bDrawopaquestaticpropslast = false; //r_drawopaquestaticpropslast.GetBool();

	
		//
		// First do the brush models
		//
		{
			CClientRenderablesList::CEntry *pEntitiesBegin, *pEntitiesEnd;
			pEntitiesBegin = m_pRenderablesList->m_RenderGroups[RENDER_GROUP_OPAQUE_BRUSH];
			pEntitiesEnd = pEntitiesBegin + m_pRenderablesList->m_RenderGroupCounts[RENDER_GROUP_OPAQUE_BRUSH];
			DrawOpaqueRenderables_DrawBrushModels( pEntitiesBegin, pEntitiesEnd, bShadowDepth );
		}


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
			}

			for ( int bucket = 0; bucket < RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS; ++ bucket )
			{
				if ( bDrawopaquestaticpropslast )
				{
					DrawOpaqueRenderables_Range( pEnts[bucket][0], pEnts[bucket][1], bShadowDepth );
					DrawOpaqueRenderables_DrawStaticProps( pProps[bucket][0], pProps[bucket][1], bShadowDepth );
				}
				else
				{
					DrawOpaqueRenderables_Range( pEnts[bucket][0], pEnts[bucket][1], bShadowDepth );
					DrawOpaqueRenderables_DrawStaticProps( pProps[bucket][0], pProps[bucket][1], bShadowDepth );
				}
			}
		}

		//
		// Draw NPCs now
		//
		DrawOpaqueRenderables_Range( arrRenderEntsNpcsFirst.Base(), arrRenderEntsNpcsFirst.Base() + numNpcs, bShadowDepth );
#endif
		//
		// Ropes and particles
		//
		if ( bRopes )
			RopeManager()->DrawRenderCache( bShadowDepth );
		if ( bParticles )
			g_pParticleSystemMgr->DrawRenderCache( bShadowDepth );
	};

#ifdef SWARM_DLL
	void	DrawOpaqueRenderables_ModelRenderables( int nCount, ModelRenderSystemData_t* pModelRenderables, bool bShadowDepth )
	{
		g_pModelRenderSystem->DrawModels( pModelRenderables, nCount, bShadowDepth ? MODEL_RENDER_MODE_SHADOW_DEPTH : MODEL_RENDER_MODE_NORMAL );
	}
	void DrawOpaqueRenderables_NPCs( int nCount, CClientRenderablesList::CEntry **ppEntities, bool bShadowDepth )
	{
		DrawOpaqueRenderables_Range( nCount, ppEntities, bShadowDepth );
	}
	void DrawRenderable( IClientRenderable *pEnt, int flags, const RenderableInstance_t &instance )
	{
		extern ConVar r_entityclips;
		float *pRenderClipPlane = NULL;
		if( r_entityclips.GetBool() )
			pRenderClipPlane = pEnt->GetRenderClipPlane();

		if( pRenderClipPlane )	
		{
			CMatRenderContextPtr pRenderContext( materials );
			if( !materials->UsingFastClipping() ) //do NOT change the fast clip plane mid-scene, depth problems result. Regular user clip planes are fine though
				pRenderContext->PushCustomClipPlane( pRenderClipPlane );
#if DEBUG
			else
				AssertMsg( 0, "can't link DrawClippedDepthBox externally so you either have to cope with even more redundancy or move all this crap to viewrender" );
#endif
			//	DrawClippedDepthBox( pEnt, pRenderClipPlane );
			Assert( view->GetCurrentlyDrawingEntity() == NULL );
			view->SetCurrentlyDrawingEntity( pEnt->GetIClientUnknown()->GetBaseEntity() );
			bool bBlockNormalDraw = false; //BlurTest( pEnt, flags, true, instance );
			if( !bBlockNormalDraw )
				pEnt->DrawModel( flags, instance );
			//BlurTest( pEnt, flags, false, instance );
			view->SetCurrentlyDrawingEntity( NULL );

			if( !materials->UsingFastClipping() )	
				pRenderContext->PopCustomClipPlane();
		}
		else
		{
			Assert( view->GetCurrentlyDrawingEntity() == NULL );
			view->SetCurrentlyDrawingEntity( pEnt->GetIClientUnknown()->GetBaseEntity() );
			bool bBlockNormalDraw = false; //BlurTest( pEnt, flags, true, instance );
			if( !bBlockNormalDraw )
				pEnt->DrawModel( flags, instance );
			//BlurTest( pEnt, flags, false, instance );
			view->SetCurrentlyDrawingEntity( NULL );
		}
	};
	void DrawOpaqueRenderable( IClientRenderable *pEnt, bool bTwoPass, bool bShadowDepth )
	{
		ASSERT_LOCAL_PLAYER_RESOLVABLE();
		float color[3];

		Assert( !IsSplitScreenSupported() || pEnt->ShouldDrawForSplitScreenUser( GET_ACTIVE_SPLITSCREEN_SLOT() ) );
		Assert( (pEnt->GetIClientUnknown() == NULL) || (pEnt->GetIClientUnknown()->GetIClientEntity() == NULL) || (pEnt->GetIClientUnknown()->GetIClientEntity()->IsBlurred() == false) );
		pEnt->GetColorModulation( color );
		render->SetColorModulation(	color );

		int flags = STUDIO_RENDER;
		if ( bTwoPass )
		{
			flags |= STUDIO_TWOPASS;
		}

		if ( bShadowDepth )
		{
			flags |= STUDIO_SHADOWDEPTHTEXTURE;
		}

		RenderableInstance_t instance;
		instance.m_nAlpha = 255;
		DrawRenderable( pEnt, flags, instance );
	};
#else
	void DrawOpaqueRenderable( IClientRenderable *pEnt, bool bTwoPass, bool bShadowDepth )
	{
		float color[3];

		pEnt->GetColorModulation( color );
		render->SetColorModulation(	color );

		int flags = STUDIO_RENDER;
		if ( bTwoPass )
		{
			flags |= STUDIO_TWOPASS;
		}

		if ( bShadowDepth )
		{
			flags |= STUDIO_SHADOWDEPTHTEXTURE;
		}

		float *pRenderClipPlane = NULL;
		if( true ) //r_entityclips.GetBool() )
			pRenderClipPlane = pEnt->GetRenderClipPlane();

		if( pRenderClipPlane )	
		{
			CMatRenderContextPtr pRenderContext( materials );
			if( !materials->UsingFastClipping() ) //do NOT change the fast clip plane mid-scene, depth problems result. Regular user clip planes are fine though
				pRenderContext->PushCustomClipPlane( pRenderClipPlane );
#if DEBUG
			else
				AssertMsg( 0, "can't link DrawClippedDepthBox externally so you either have to cope with even more redundancy or move all this crap to viewrender" );
#endif
			//	DrawClippedDepthBox( pEnt, pRenderClipPlane );
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
	};
#endif

#ifdef SWARM_DLL
	void DrawOpaqueRenderables_DrawBrushModels( int nCount, CClientRenderablesList::CEntry **ppEntities, bool bShadowDepth )
	{
		for( int i = 0; i < nCount; ++i )
			DrawOpaqueRenderable( ppEntities[i]->m_pRenderable, false, bShadowDepth );
	};
#else
	void DrawOpaqueRenderables_DrawBrushModels( CClientRenderablesList::CEntry *pEntitiesBegin, CClientRenderablesList::CEntry *pEntitiesEnd, bool bShadowDepth )
	{
		for( CClientRenderablesList::CEntry *itEntity = pEntitiesBegin; itEntity < pEntitiesEnd; ++ itEntity )
			DrawOpaqueRenderable( itEntity->m_pRenderable, false, bShadowDepth );
	};
#endif

#ifdef SWARM_DLL
	void DrawOpaqueRenderables_DrawStaticProps( int nCount, CClientRenderablesList::CEntry **ppEntities, bool bShadowDepth )
	{
		if ( nCount == 0 )
			return;

		float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		render->SetColorModulation(	one );
		render->SetBlend( 1.0f );
	
		const int MAX_STATICS_PER_BATCH = 512;
		IClientRenderable *pStatics[ MAX_STATICS_PER_BATCH ];
		RenderableInstance_t pInstances[ MAX_STATICS_PER_BATCH ];
	
		int numScheduled = 0, numAvailable = MAX_STATICS_PER_BATCH;

		for( int i = 0; i < nCount; ++i )
		{
			CClientRenderablesList::CEntry *itEntity = ppEntities[i];
			if ( itEntity->m_pRenderable )
				NULL;
			else
				continue;

			pInstances[ numScheduled ] = itEntity->m_InstanceData;
			pStatics[ numScheduled ++ ] = itEntity->m_pRenderable;
			if ( -- numAvailable > 0 )
				continue; // place a hint for compiler to predict more common case in the loop
		
			staticpropmgr->DrawStaticProps( pStatics, pInstances, numScheduled, bShadowDepth, vcollide_wireframe.GetBool() );
			numScheduled = 0;
			numAvailable = MAX_STATICS_PER_BATCH;
		}
	
		if ( numScheduled )
			staticpropmgr->DrawStaticProps( pStatics, pInstances, numScheduled, bShadowDepth, vcollide_wireframe.GetBool() );
	}
#else
	void DrawOpaqueRenderables_DrawStaticProps( CClientRenderablesList::CEntry *pEntitiesBegin, CClientRenderablesList::CEntry *pEntitiesEnd, bool bShadowDepth )
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
				NULL;
			else
				continue;

			pStatics[ numScheduled ++ ] = itEntity->m_pRenderable;
			if ( -- numAvailable > 0 )
				continue; // place a hint for compiler to predict more common case in the loop
		
			staticpropmgr->DrawStaticProps( pStatics, numScheduled, bShadowDepth, vcollide_wireframe.GetBool() );
			numScheduled = 0;
			numAvailable = MAX_STATICS_PER_BATCH;
		}
	
		if ( numScheduled )
			staticpropmgr->DrawStaticProps( pStatics, numScheduled, bShadowDepth, vcollide_wireframe.GetBool() );
	};
#endif

#ifdef SWARM_DLL
	void DrawOpaqueRenderables_Range( int nCount, CClientRenderablesList::CEntry **ppEntities, bool bShadowDepth )
	{
		for ( int i = 0; i < nCount; ++i )
		{
			CClientRenderablesList::CEntry *itEntity = ppEntities[i]; 
			if ( itEntity->m_pRenderable )
				DrawOpaqueRenderable( itEntity->m_pRenderable, ( itEntity->m_TwoPass != 0 ), bShadowDepth );
		}
	};
#else
	void DrawOpaqueRenderables_Range( CClientRenderablesList::CEntry *pEntitiesBegin, CClientRenderablesList::CEntry *pEntitiesEnd, bool bShadowDepth )
	{
		for( CClientRenderablesList::CEntry *itEntity = pEntitiesBegin; itEntity < pEntitiesEnd; ++ itEntity )
			if ( itEntity->m_pRenderable )
				DrawOpaqueRenderable( itEntity->m_pRenderable, ( itEntity->m_TwoPass != 0 ), bShadowDepth );
	};
#endif
};


class CSimpleVCallbackView : public CBaseVCallbackView
{
	DECLARE_CLASS( CSimpleVCallbackView, CBaseVCallbackView );
public:
	CSimpleVCallbackView(CViewRender *pMainView) : CBaseVCallbackView( pMainView ) {}

	struct EditorViewSettings
	{
	public:
		bool bDrawPlayers;
		bool bDrawWeapons;
		bool bDrawStaticProps;
		bool bDrawMisc;
		bool bDrawTranslucents;
		bool bDrawWater;
		bool bDrawWorld;
		bool bDrawParticles;
		bool bDrawRopes;
		bool bDrawSkybox;
		bool bClipSkybox;
		bool bClearColor;
		bool bClearDepth;
		bool bClearStencil;
		bool bClearObeyStencil;
		bool bFogOverride;
		bool bFogEnabled;

		int iClearColorR;
		int iClearColorG;
		int iClearColorB;
		int iClearColorA;
		int iFogColorR;
		int iFogColorG;
		int iFogColorB;

		float flFogStart;
		float flFogEnd;
		float flFogDensity;
	};

	EditorViewSettings settings;

	void Setup( const CViewSetup &view, CSimpleVCallbackView::EditorViewSettings settings,
		const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& info )
	{
		this->settings = settings;

		BaseClass::Setup( view );

		m_ClearFlags = (settings.bClearColor ? VIEW_CLEAR_COLOR : 0) |
			(settings.bClearDepth ? VIEW_CLEAR_DEPTH : 0) |
			(settings.bClearStencil ? VIEW_CLEAR_STENCIL : 0) |
			(settings.bClearObeyStencil ? VIEW_CLEAR_OBEY_STENCIL : 0);

		m_DrawFlags = (settings.bDrawPlayers || settings.bDrawStaticProps ||
			settings.bDrawTranslucents || settings.bDrawWeapons ||
			settings.bDrawMisc) ? DF_DRAW_ENTITITES : 0;

		//if ( settings.bDrawWorld )
		{
			if ( !info.m_bOpaqueWater )
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
		}

		if ( info.m_bDrawWaterSurface && settings.bDrawWater )
		{
			m_DrawFlags |= DF_RENDER_WATER;
		}

		if ( !fogInfo.m_bEyeInFogVolume && settings.bDrawSkybox )
		{
			m_DrawFlags |= DF_DRAWSKYBOX;
		}

		if ( settings.bClipSkybox )
			m_DrawFlags |= DF_CLIP_SKYBOX;

		m_pCustomVisibility = NULL;
		m_fogInfo = fogInfo;
	};

	void Draw()
	{
		DrawSetup( 0, m_DrawFlags, 0 );

		CMatRenderContextPtr pRenderContext( materials );

		pRenderContext->ClearColor4ub( (unsigned char)settings.iClearColorR,
			(unsigned char)settings.iClearColorG,
			(unsigned char)settings.iClearColorB,
			(unsigned char)settings.iClearColorA );

		if ( settings.bFogOverride )
		{
			if ( !settings.bFogEnabled )
				pRenderContext->FogMode( MATERIAL_FOG_NONE );
			else
			{
				pRenderContext->FogMode( MATERIAL_FOG_LINEAR );
				pRenderContext->FogColor3ub( (unsigned char)settings.iFogColorR,
					(unsigned char)settings.iFogColorG,
					(unsigned char)settings.iFogColorB );
				pRenderContext->FogStart( settings.flFogStart );
				pRenderContext->FogEnd( settings.flFogEnd );
				pRenderContext->FogMaxDensity( settings.flFogDensity );
			}
		}
		else if ( !m_fogInfo.m_bEyeInFogVolume )
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

		m_pMainView->DisableFog();
	};

	virtual void CallbackInitRenderList( int viewId )
	{
		BaseClass::CallbackInitRenderList( viewId );

		if ( settings.bDrawPlayers && settings.bDrawStaticProps &&
			settings.bDrawTranslucents && settings.bDrawWeapons &&
			settings.bDrawMisc )
			return;

		for ( int i = 0; i < RENDER_GROUP_COUNT; i++ )
		{
#ifndef SWARM_DLL
			const bool bStaticProp = i == 0 || i == 2 || i == 4 || i == 6;
#endif

			for ( int e = 0; e < m_pRenderablesList->m_RenderGroupCounts[i]; e++ )
			{
				CClientRenderablesList::CEntry *pEntry = m_pRenderablesList->m_RenderGroups[i] + e;

				if ( !pEntry || !pEntry->m_pRenderable )
					continue;

#ifdef SWARM_DLL
				const bool bStaticProp = pEntry->m_nModelType == RENDERABLE_MODEL_STATIC_PROP;
#endif

				bool bRemove = false;
				if ( bStaticProp )
					bRemove = !settings.bDrawStaticProps;
				else
				{
					IClientUnknown *pUnknown = pEntry->m_pRenderable->GetIClientUnknown();

					if ( !pUnknown || !pUnknown->GetBaseEntity() )
						continue;

					C_BaseEntity *pEntity = pUnknown->GetBaseEntity();

					if ( pEntity->IsPlayer() )
						bRemove = !settings.bDrawPlayers;
					else if ( dynamic_cast< CBaseCombatWeapon* >( pEntity ) != NULL )
						bRemove = !settings.bDrawWeapons;
#ifdef SWARM_DLL
					else if ( pEntity->ComputeTranslucencyType() != RENDERABLE_IS_OPAQUE )
#else
					else if ( pEntry->m_pRenderable->IsTransparent() )
#endif
						bRemove = !settings.bDrawTranslucents;
					else
						bRemove = !settings.bDrawMisc;
				}

				if ( bRemove )
				{
					pEntry->m_pRenderable = NULL;
#ifndef SWARM_DLL
					pEntry->m_RenderHandle = NULL;
#endif
				}
			}

			int eLast = -1;
			for ( int e = 0; e < m_pRenderablesList->m_RenderGroupCounts[i]; e++ )
			{
				CClientRenderablesList::CEntry *pEntry = m_pRenderablesList->m_RenderGroups[i] + e;

				if ( !pEntry || !pEntry->m_pRenderable
#ifndef SWARM_DLL
					|| !pEntry->m_RenderHandle
#endif
					)
				{
					for ( int e2 = e + 1; e2 < m_pRenderablesList->m_RenderGroupCounts[i]; e2++ )
					{
						CClientRenderablesList::CEntry *pEntry2 = m_pRenderablesList->m_RenderGroups[i] + e2;
						if ( pEntry2 && pEntry2->m_pRenderable
#ifndef SWARM_DLL
							&& pEntry2->m_RenderHandle
#endif
							)
						{
							CClientRenderablesList::CEntry tmp = *pEntry;
							*pEntry = *pEntry2;
							*pEntry2 = tmp;
							break;
						}
					}
				}

				if ( pEntry && pEntry->m_pRenderable
#ifndef SWARM_DLL
					&& pEntry->m_RenderHandle
#endif
					)
					eLast = e;
			}

			m_pRenderablesList->m_RenderGroupCounts[i] = eLast + 1;
		}
	};

	virtual bool ShouldDrawParticles()
	{
		return settings.bDrawParticles;
	};

	virtual bool ShouldDrawRopes()
	{
		return settings.bDrawRopes;
	};

	virtual bool ShouldDrawWorld()
	{
		return settings.bDrawWorld;
	};

	virtual bool ShouldDrawTranslucents()
	{
		return settings.bDrawTranslucents;
	};

	virtual bool ShouldDrawTranslucentWorld()
	{
		return settings.bDrawWorld && settings.bDrawTranslucents;
	};

private:
	VisibleFogVolumeInfo_t m_fogInfo;

};

#ifdef SWARM_DLL
bool UpdateRefractIfNeededByList( CViewModelRenderablesList::RenderGroups_t &list )
{
	int nCount = list.Count();
	for( int i=0; i < nCount; ++i )
	{
		IClientRenderable *pRenderable = list[i].m_pRenderable;
		Assert( pRenderable );
		if ( pRenderable->GetRenderFlags() & ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB )
		{
			UpdateRefractTexture();
			return true;
		}
	}
	return false;
}
void DrawRenderablesInList( CViewModelRenderablesList::RenderGroups_t &renderGroups, int flags = 0 )
{
	CViewRender *pCView = assert_cast< CViewRender* >( view );
	Assert( pCView->GetCurrentlyDrawingEntity() == NULL );

	ASSERT_LOCAL_PLAYER_RESOLVABLE();
#if defined( DBGFLAG_ASSERT )
	int nSlot = GET_ACTIVE_SPLITSCREEN_SLOT();
#endif
	Assert( pCView->GetCurrentlyDrawingEntity() == NULL );
	int nCount = renderGroups.Count();
	for( int i=0; i < nCount; ++i )
	{
		IClientRenderable *pRenderable = renderGroups[i].m_pRenderable;
		Assert( pRenderable );

		// Non-view models wanting to render in view model list...
		if ( pRenderable->ShouldDraw() )
		{
			Assert( !IsSplitScreenSupported() || pRenderable->ShouldDrawForSplitScreenUser( nSlot ) );
			pCView->SetCurrentlyDrawingEntity( pRenderable->GetIClientUnknown()->GetBaseEntity() );
			pRenderable->DrawModel( STUDIO_RENDER | flags, renderGroups[i].m_InstanceData );
		}
	}
	pCView->SetCurrentlyDrawingEntity( NULL );
}
#else
static inline bool UpdateRefractIfNeededByList( CUtlVector< IClientRenderable * > &list )
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
static inline void DrawRenderablesInList( CUtlVector< IClientRenderable * > &list, int flags = 0 )
{
	CViewRender *pCView = assert_cast< CViewRender* >( view );
	Assert( pCView->GetCurrentlyDrawingEntity() == NULL );

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
			pCView->SetCurrentlyDrawingEntity( pUnk->GetBaseEntity() );
			pRenderable->DrawModel( STUDIO_RENDER | flags );
		}
	}
	pCView->SetCurrentlyDrawingEntity( NULL );
}
#endif


int &ShaderEditorHandler::GetViewIdForModify()
{
	Assert( m_piCurrentViewId != NULL );

	return *m_piCurrentViewId;
}
const VisibleFogVolumeInfo_t &ShaderEditorHandler::GetFogVolumeInfo()
{
	return m_tFogVolumeInfo;
}
const WaterRenderInfo_t &ShaderEditorHandler::GetWaterRenderInfo()
{
	return m_tWaterRenderInfo;
}

pFnVrCallback_Declare( VrCallback_General )
{
	CViewRender *pCView = assert_cast< CViewRender* >( view );
	Assert( pCView->GetViewSetup() != NULL );

	const CViewSetup *setup = pCView->GetViewSetup();

	CSimpleVCallbackView::EditorViewSettings settings;

	settings.bDrawPlayers = pbOptions[0];
	settings.bDrawWeapons = pbOptions[1];
	settings.bDrawStaticProps = pbOptions[2];
	settings.bDrawMisc = pbOptions[3];
	settings.bDrawTranslucents = pbOptions[4];
	settings.bDrawWater = pbOptions[5];
	settings.bDrawWorld = pbOptions[6];
	settings.bDrawParticles = pbOptions[7];
	settings.bDrawRopes = pbOptions[8];
	settings.bDrawSkybox = pbOptions[9];
	settings.bClipSkybox = pbOptions[10];
	settings.bClearColor = pbOptions[11];
	settings.bClearDepth = pbOptions[12];
	settings.bClearStencil = pbOptions[13];
	settings.bClearObeyStencil = pbOptions[14];
	settings.bFogOverride = pbOptions[15];
	settings.bFogEnabled = pbOptions[16];

	settings.iClearColorR = piOptions[0];
	settings.iClearColorG = piOptions[1];
	settings.iClearColorB = piOptions[2];
	settings.iClearColorA = piOptions[3];
	settings.iFogColorR = piOptions[4];
	settings.iFogColorG = piOptions[5];
	settings.iFogColorB = piOptions[6];

	settings.flFogStart = pflOptions[0];
	settings.flFogEnd = pflOptions[1];
	settings.flFogDensity = pflOptions[2];

	if ( settings.flFogEnd < 0 )
		settings.flFogEnd = setup->zFar;

	CRefPtr<CSimpleVCallbackView> pGeneralCallbackView = new CSimpleVCallbackView( pCView );
	pGeneralCallbackView->Setup( *setup, settings,
		g_ShaderEditorSystem->GetFogVolumeInfo(), g_ShaderEditorSystem->GetWaterRenderInfo() );
	pCView->AddViewToScene( pGeneralCallbackView );
}

pFnVrCallback_Declare( VrCallback_ViewModel )
{
	CViewRender *pCView = assert_cast< CViewRender* >( view );
	Assert( pCView->GetViewSetup() != NULL );

	CMatRenderContextPtr pRenderContext( materials );

	static ConVarRef drawVM( "r_drawviewmodel" );

	const bool bHideVM = pbOptions[0];
	const bool bFogOverride = pbOptions[5];
	const int iClearFlags = (pbOptions[1] ? VIEW_CLEAR_COLOR : 0) |
			(pbOptions[2] ? VIEW_CLEAR_DEPTH : 0) |
			(pbOptions[3] ? VIEW_CLEAR_STENCIL : 0) |
			(pbOptions[4] ? VIEW_CLEAR_OBEY_STENCIL : 0);

	drawVM.SetValue( !bHideVM );

	if ( bFogOverride )
	{
		if ( !pbOptions[6] )
			pCView->DisableFog();
		else
		{
			pRenderContext->FogMode( MATERIAL_FOG_LINEAR );
			pRenderContext->FogColor3ub( (unsigned char)piOptions[4],
				(unsigned char)piOptions[5],
				(unsigned char)piOptions[6] );
			pRenderContext->FogStart( pflOptions[0] );
			pRenderContext->FogEnd( pflOptions[1] );
			pRenderContext->FogMaxDensity( pflOptions[2] );
		}
	}

	int bbx, bby;
	materials->GetBackBufferDimensions( bbx, bby );

	// Restore the matrices
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();

	ITexture *pTex = pRenderContext->GetRenderTarget();
	const CViewSetup &view = *pCView->GetViewSetup();
	CViewSetup viewModelSetup( view );
	viewModelSetup.zNear = view.zNearViewmodel;
	viewModelSetup.zFar = view.zFarViewmodel;
	viewModelSetup.fov = view.fovViewmodel;
#ifdef SWARM_DLL
	viewModelSetup.m_flAspectRatio = engine->GetScreenAspectRatio( view.width, view.height );
#else
	viewModelSetup.m_flAspectRatio = engine->GetScreenAspectRatio();
#endif
	viewModelSetup.width = pTex ? pTex->GetActualWidth() : bbx;
	viewModelSetup.height = pTex ? pTex->GetActualHeight() : bby;

	if ( iClearFlags & VIEW_CLEAR_COLOR )
	{
		pRenderContext->ClearColor4ub( (unsigned char)piOptions[0],
			(unsigned char)piOptions[1],
			(unsigned char)piOptions[2],
			(unsigned char)piOptions[3] );
	}

	render->Push3DView( viewModelSetup, iClearFlags, pTex, pCView->GetFrustum() );
	const bool bUseDepthHack = true;

	float depthmin = 0.0f;
	float depthmax = 1.0f;

	// HACK HACK:  Munge the depth range to prevent view model from poking into walls, etc.
	// Force clipped down range
	if( bUseDepthHack )
		pRenderContext->DepthRange( 0.0f, 0.1f );
	
#ifdef SWARM_DLL
	CViewModelRenderablesList list;
	ClientLeafSystem()->CollateViewModelRenderables( &list );
	CViewModelRenderablesList::RenderGroups_t &opaqueViewModelList = list.m_RenderGroups[ CViewModelRenderablesList::VM_GROUP_OPAQUE ];
	CViewModelRenderablesList::RenderGroups_t &translucentViewModelList = list.m_RenderGroups[ CViewModelRenderablesList::VM_GROUP_TRANSLUCENT ];
#else
	CUtlVector< IClientRenderable * > opaqueViewModelList( 32 );
	CUtlVector< IClientRenderable * > translucentViewModelList( 32 );
	ClientLeafSystem()->CollateViewModelRenderables( opaqueViewModelList, translucentViewModelList );
#endif

	const bool bUpdateRefractForOpaque = UpdateRefractIfNeededByList( opaqueViewModelList );
	DrawRenderablesInList( opaqueViewModelList );

	if ( !bUpdateRefractForOpaque )
		UpdateRefractIfNeededByList( translucentViewModelList );

	DrawRenderablesInList( translucentViewModelList, STUDIO_TRANSPARENCY );

	// Reset the depth range to the original values
	if( bUseDepthHack )
		pRenderContext->DepthRange( depthmin, depthmax );

	render->PopView( pCView->GetFrustum() );

	// Restore the matrices
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();

	if ( bFogOverride )
		pCView->DisableFog();
}


void ShaderEditorHandler::RegisterViewRenderCallbacks()
{
	if ( !IsReady() )
		return;

	const char *boolNames_generalVrc[] = {
		"Draw players",
		"Draw weapons",
		"Draw static props",
		"Draw misc",
		"Draw translucents",
		"Draw water",
		"Draw world",
		"Draw particles",
		"Draw ropes",
		"Draw skybox (2D)",
		"Clip skybox",
		"Clear color",
		"Clear depth",
		"Clear stencil",
		"Clear obey stencil",
		"Fog override",
		"Fog force enabled",
	};
	const bool boolDefaults_generalVrc[] = {
		true,
		true,
		true,
		true,
		true,
		true,
		true,
		true,
		true,
		true,
		false,
		true,
		true,
		false,
		false,
		false,
		true,
	};
	const char *intNames_generalVrc[] = {
		"Clear color R (0-255)",
		"Clear color G (0-255)",
		"Clear color B (0-255)",
		"Clear color A (0-255)",
		"Fog color R (0-255)",
		"Fog color G (0-255)",
		"Fog color B (0-255)",
	};

	const char *floatNames_generalVrc[] = {
		"Fog start (units)",
		"Fog end (units)",
		"Fog density (0-1)",
	};
	const float floatDefaults_generalVrc[] = {
		0,
		2000,
		1,
	};


	const char *boolNames_vmVrc[] = {
		"Hide default viewmodel",
		"Clear color",
		"Clear depth",
		"Clear stencil",
		"Clear obey stencil",
		"Fog override",
		"Fog force enabled",
	};
	const bool boolDefaults_vmVrc[] = {
		false,
		true,
		true,
		false,
		false,
		false,
		true,
	};

	Assert( ARRAYSIZE(boolNames_generalVrc) == ARRAYSIZE(boolDefaults_generalVrc) );
	Assert( ARRAYSIZE(floatNames_generalVrc) == ARRAYSIZE(floatDefaults_generalVrc) );
	Assert( ARRAYSIZE(boolNames_vmVrc) == ARRAYSIZE(boolDefaults_vmVrc) );

	shaderEdit->RegisterViewRenderCallback( "General view", VrCallback_General,
		boolNames_generalVrc, boolDefaults_generalVrc, ARRAYSIZE(boolNames_generalVrc),
		intNames_generalVrc, NULL, ARRAYSIZE(intNames_generalVrc),
		floatNames_generalVrc, floatDefaults_generalVrc, ARRAYSIZE(floatNames_generalVrc) );

	shaderEdit->RegisterViewRenderCallback( "Viewmodel view", VrCallback_ViewModel,
		boolNames_vmVrc, boolDefaults_vmVrc, ARRAYSIZE(boolNames_vmVrc),
		intNames_generalVrc, NULL, ARRAYSIZE(intNames_generalVrc),
		floatNames_generalVrc, floatDefaults_generalVrc, ARRAYSIZE(floatNames_generalVrc) );

	shaderEdit->LockViewRenderCallbacks();
}

#endif