//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "matsys_controls/mdlpanel.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/imesh.h"
#include "vgui/IVGui.h"
#include "tier1/KeyValues.h"
#include "vgui_controls/Frame.h"
#include "tier1/convar.h"
#include "tier0/dbg.h"
#include "tier1/fmtstr.h"
#include "istudiorender.h"
#include "matsys_controls/matsyscontrols.h"
#include "vcollide.h"
#include "vcollide_parse.h"
#include "bone_setup.h"
#include "vphysics_interface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CMDLPanel );

static const int THUMBNAIL_SAFE_ZONE_SIZE = 512;
static const int THUMBNAIL_SAFE_ZONE_HEIGHT = 92;
static const float THUMBNAIL_SAFE_ZONE_HEIGHT_SCALE = (float)THUMBNAIL_SAFE_ZONE_HEIGHT / THUMBNAIL_SAFE_ZONE_SIZE;

//-----------------------------------------------------------------------------
// Purpose: Keeps a global clock to autoplay sequences to run from
//			Also deals with speedScale changes
//-----------------------------------------------------------------------------
float GetAutoPlayTime( void )
{
	static int g_prevTicks;
	static float g_time;

	int ticks = Plat_MSTime();

	// limit delta so that float time doesn't overflow
	if (g_prevTicks == 0)
	{
		g_prevTicks = ticks;
	}

	g_time += ( ticks - g_prevTicks ) / 1000.0f;
	g_prevTicks = ticks;

	return g_time;
}


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CMDLPanel::CMDLPanel( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
{
	SetVisible( true );

	// Used to poll input
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	// Deal with the default cubemap
	ITexture *pCubemapTexture = vgui::MaterialSystem()->FindTexture( "editor/cubemap", NULL, true );
	m_DefaultEnvCubemap.Init( pCubemapTexture );
	pCubemapTexture = vgui::MaterialSystem()->FindTexture( "editor/cubemap.hdr", NULL, true );
	m_DefaultHDREnvCubemap.Init( pCubemapTexture );

	SetIdentityMatrix( m_RootMDL.m_MDLToWorld );
	m_RootMDL.m_pStudioHdr = NULL;
	m_RootMDL.m_unMdlCacheSerial = 0;
	m_bDrawCollisionModel = false;
	m_bWireFrame = false;
	m_bGroundGrid = false;
	m_bLockView = false;
	m_bLookAtCamera = true;
	m_bThumbnailSafeZone = false;
	m_nNumSequenceLayers = 0;
	ResetAnimationEventState( &m_EventState );
}

CMDLPanel::~CMDLPanel()
{
	m_aMergeMDLs.Purge();
	m_DefaultEnvCubemap.Shutdown( );
	m_DefaultHDREnvCubemap.Shutdown();
	if ( m_RootMDL.m_pStudioHdr )
	{
		delete m_RootMDL.m_pStudioHdr;
		m_RootMDL.m_pStudioHdr = NULL;
	}
}


//-----------------------------------------------------------------------------
// Scheme settings
//-----------------------------------------------------------------------------
void CMDLPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetBackgroundColor( GetBgColor() );
	SetBorder( pScheme->GetBorder( "MenuBorder") );
}


//-----------------------------------------------------------------------------
// Rendering options
//-----------------------------------------------------------------------------
void CMDLPanel::SetCollsionModel( bool bVisible )
{
	m_bDrawCollisionModel = bVisible;
}

void CMDLPanel::SetGroundGrid( bool bVisible )
{
	m_bGroundGrid = bVisible;
}

void CMDLPanel::SetWireFrame( bool bVisible )
{
	m_bWireFrame = bVisible;
}

void CMDLPanel::SetLockView( bool bLocked )
{
	m_bLockView = bLocked;
}

void CMDLPanel::SetLookAtCamera( bool bLookAtCamera )
{
	m_bLookAtCamera = bLookAtCamera;
}

void CMDLPanel::SetIgnoreDoubleClick( bool bState )
{
	m_bIgnoreDoubleClick = bState;
}

void CMDLPanel::SetThumbnailSafeZone( bool bVisible )
{
	m_bThumbnailSafeZone = bVisible;
}

//-----------------------------------------------------------------------------
// Stores the clip
//-----------------------------------------------------------------------------
void CMDLPanel::SetMDL( MDLHandle_t handle, void *pProxyData )
{
	m_RootMDL.m_MDL.SetMDL( handle );
	if ( m_RootMDL.m_pStudioHdr )
	{
		delete m_RootMDL.m_pStudioHdr;
	}
	m_RootMDL.m_pStudioHdr = new CStudioHdr( m_RootMDL.m_MDL.GetStudioHdr(), g_pMDLCache );
	m_RootMDL.m_MDL.m_pProxyData = pProxyData;

	Vector vecMins, vecMaxs;
	GetMDLBoundingBox( &vecMins, &vecMaxs, handle, m_RootMDL.m_MDL.m_nSequence );

	m_RootMDL.m_MDL.m_bWorldSpaceViewTarget = false;
	m_RootMDL.m_MDL.m_vecViewTarget.Init( 100.0f, 0.0f, vecMaxs.z );

	m_RootMDL.m_flCycleStartTime = 0.f;

	// Set the pose parameters to the default for the mdl
	SetPoseParameters( NULL, 0 );
	
	// Clear any sequence layers
	SetSequenceLayers( NULL, 0 );

	ResetAnimationEventState( &m_EventState );
}

//-----------------------------------------------------------------------------
// An MDL was selected
//-----------------------------------------------------------------------------
void CMDLPanel::SetMDL( const char *pMDLName, void *pProxyData )
{
	MDLHandle_t hMDLFindResult = vgui::MDLCache()->FindMDL( pMDLName );
	MDLHandle_t hMDL = pMDLName ? hMDLFindResult : MDLHANDLE_INVALID;
	if ( vgui::MDLCache()->IsErrorModel( hMDL ) )
	{
		hMDL = MDLHANDLE_INVALID;
	}

	SetMDL( hMDL, pProxyData );

	// FindMDL takes a reference and the the CMDL will also hold a reference for as long as it sticks around. Release the FindMDL reference.
	int nRef = vgui::MDLCache()->Release( hMDLFindResult );
	(void)nRef; // Avoid unreferenced variable warning
	AssertMsg( hMDL == MDLHANDLE_INVALID || nRef > 0, "CMDLPanel::SetMDL referenced a model that has a zero ref count." );
}

//-----------------------------------------------------------------------------
// Purpose: Returns a model bounding box.
//-----------------------------------------------------------------------------
bool CMDLPanel::GetBoundingBox( Vector &vecBoundsMin, Vector &vecBoundsMax )
{
	// Check to see if we have a valid model to look at.
	if ( m_RootMDL.m_MDL.GetMDL() == MDLHANDLE_INVALID )
		return false;

	GetMDLBoundingBox( &vecBoundsMin, &vecBoundsMax, m_RootMDL.m_MDL.GetMDL(), m_RootMDL.m_MDL.m_nSequence );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a more accurate bounding sphere
//-----------------------------------------------------------------------------
bool CMDLPanel::GetBoundingSphere( Vector &vecCenter, float &flRadius )
{
	// Check to see if we have a valid model to look at.
	if ( m_RootMDL.m_MDL.GetMDL() == MDLHANDLE_INVALID )
		return false;

	Vector vecEngineCenter;
	GetMDLBoundingSphere( &vecEngineCenter, &flRadius, m_RootMDL.m_MDL.GetMDL(), m_RootMDL.m_MDL.m_nSequence );
	VectorTransform( vecEngineCenter, m_RootMDL.m_MDLToWorld, vecCenter );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMDLPanel::SetModelAnglesAndPosition(  const QAngle &angRot, const Vector &vecPos )
{
	SetIdentityMatrix( m_RootMDL.m_MDLToWorld );
	AngleMatrix( angRot, vecPos, m_RootMDL.m_MDLToWorld );
}

//-----------------------------------------------------------------------------
// Sets the camera to look at the model
//-----------------------------------------------------------------------------
void CMDLPanel::LookAtMDL()
{
	// Check to see if we have a valid model to look at.
	if ( m_RootMDL.m_MDL.GetMDL() == MDLHANDLE_INVALID )
		return;

	if ( m_bLockView )
		return;

	float flRadius;
	Vector vecCenter;
	GetBoundingSphere( vecCenter, flRadius );
	LookAt( vecCenter, flRadius );
}


//-----------------------------------------------------------------------------
// FIXME: This should be moved into studiorender
//-----------------------------------------------------------------------------
static ConVar	r_showenvcubemap( "r_showenvcubemap", "0", FCVAR_CHEAT );
static ConVar	r_eyegloss		( "r_eyegloss", "1", FCVAR_ARCHIVE ); // wet eyes
static ConVar	r_eyemove		( "r_eyemove", "1", FCVAR_ARCHIVE ); // look around
static ConVar	r_eyeshift_x	( "r_eyeshift_x", "0", FCVAR_ARCHIVE ); // eye X position
static ConVar	r_eyeshift_y	( "r_eyeshift_y", "0", FCVAR_ARCHIVE ); // eye Y position
static ConVar	r_eyeshift_z	( "r_eyeshift_z", "0", FCVAR_ARCHIVE ); // eye Z position
static ConVar	r_eyesize		( "r_eyesize", "0", FCVAR_ARCHIVE ); // adjustment to iris textures
static ConVar	mat_softwareskin( "mat_softwareskin", "0", FCVAR_CHEAT );
static ConVar	r_nohw			( "r_nohw", "0", FCVAR_CHEAT );
static ConVar	r_nosw			( "r_nosw", "0", FCVAR_CHEAT );
static ConVar	r_teeth			( "r_teeth", "1" );
static ConVar	r_drawentities	( "r_drawentities", "1", FCVAR_CHEAT );
static ConVar	r_flex			( "r_flex", "1" );
static ConVar	r_eyes			( "r_eyes", "1" );
static ConVar	r_skin			( "r_skin","0", FCVAR_CHEAT );
static ConVar	r_maxmodeldecal ( "r_maxmodeldecal", "50" );
static ConVar	r_modelwireframedecal ( "r_modelwireframedecal", "0", FCVAR_CHEAT );
static ConVar	mat_normals		( "mat_normals", "0", FCVAR_CHEAT );
static ConVar	r_eyeglintlodpixels ( "r_eyeglintlodpixels", "0", FCVAR_CHEAT );
static ConVar	r_rootlod		( "r_rootlod", "0" );

static StudioRenderConfig_t s_StudioRenderConfig;

void CMDLPanel::UpdateStudioRenderConfig( void )
{
	memset( &s_StudioRenderConfig, 0, sizeof(s_StudioRenderConfig) );

	s_StudioRenderConfig.bEyeMove = !!r_eyemove.GetInt();
	s_StudioRenderConfig.fEyeShiftX = r_eyeshift_x.GetFloat();
	s_StudioRenderConfig.fEyeShiftY = r_eyeshift_y.GetFloat();
	s_StudioRenderConfig.fEyeShiftZ = r_eyeshift_z.GetFloat();
	s_StudioRenderConfig.fEyeSize = r_eyesize.GetFloat();	
	if( mat_softwareskin.GetInt() || m_bWireFrame )
	{
		s_StudioRenderConfig.bSoftwareSkin = true;
	}
	else
	{
		s_StudioRenderConfig.bSoftwareSkin = false;
	}
	s_StudioRenderConfig.bNoHardware = !!r_nohw.GetInt();
	s_StudioRenderConfig.bNoSoftware = !!r_nosw.GetInt();
	s_StudioRenderConfig.bTeeth = !!r_teeth.GetInt();
	s_StudioRenderConfig.drawEntities = r_drawentities.GetInt();
	s_StudioRenderConfig.bFlex = !!r_flex.GetInt();
	s_StudioRenderConfig.bEyes = !!r_eyes.GetInt();
	s_StudioRenderConfig.bWireframe = m_bWireFrame;
	s_StudioRenderConfig.bDrawNormals = mat_normals.GetBool();
	s_StudioRenderConfig.skin = r_skin.GetInt();
	s_StudioRenderConfig.maxDecalsPerModel = r_maxmodeldecal.GetInt();
	s_StudioRenderConfig.bWireframeDecals = r_modelwireframedecal.GetInt() != 0;
	
	s_StudioRenderConfig.fullbright = false;
	s_StudioRenderConfig.bSoftwareLighting = false;

	s_StudioRenderConfig.bShowEnvCubemapOnly = r_showenvcubemap.GetInt() ? true : false;
	s_StudioRenderConfig.fEyeGlintPixelWidthLODThreshold = r_eyeglintlodpixels.GetFloat();

	StudioRender()->UpdateConfig( s_StudioRenderConfig );
}

void CMDLPanel::DrawCollisionModel()
{
	if ( m_RootMDL.m_MDL.GetMDL() == MDLHANDLE_INVALID )
	{
		return;
	}

	vcollide_t *pCollide = MDLCache()->GetVCollide( m_RootMDL.m_MDL.GetMDL() );

	if ( !pCollide || pCollide->solidCount <= 0 )
		return;
	
	static color32 color = {255,0,0,0};

	IVPhysicsKeyParser *pParser = g_pPhysicsCollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

	matrix3x4_t pBoneToWorld[MAXSTUDIOBONES];
	m_RootMDL.m_MDL.SetUpBones( m_RootMDL.m_MDLToWorld, MAXSTUDIOBONES, pBoneToWorld );

	// PERFORMANCE: Just parse the script each frame.  It's fast enough for tools.  If you need
	// this to go faster then cache off the bone index mapping in an array like HLMV does
	while ( !pParser->Finished() )
	{
		const char *pBlock = pParser->GetCurrentBlockName();
		if ( !stricmp( pBlock, "solid" ) )
		{
			solid_t solid;

			pParser->ParseSolid( &solid, NULL );
			int boneIndex = Studio_BoneIndexByName( &studioHdr, solid.name );
			Vector *outVerts;
			int vertCount = g_pPhysicsCollision->CreateDebugMesh( pCollide->solids[solid.index], &outVerts );

			if ( vertCount )
			{
				CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
				pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );
				// NOTE: assumes these have been set up already by the model render code
				// So this is a little bit of a back door to a cache of the bones
				// this code wouldn't work unless you draw the model this frame before calling
				// this routine.  CMDLPanel always does this, but it's worth noting.
				// A better solution would be to move the ragdoll visulization into the CDmeMdl
				// and either draw it there or make it queryable and query/draw here.
				matrix3x4_t xform;
				SetIdentityMatrix( xform );
				if ( boneIndex >= 0 )
				{
					MatrixCopy( pBoneToWorld[ boneIndex ], xform );
				}
				IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_Wireframe );

				CMeshBuilder meshBuilder;
				meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, vertCount/3 );

				for ( int j = 0; j < vertCount; j++ )
				{
					Vector out;
					VectorTransform( outVerts[j].Base(), xform, out.Base() );
					meshBuilder.Position3fv( out.Base() );
					meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
					meshBuilder.TexCoord2f( 0, 0, 0 );
					meshBuilder.AdvanceVertex();
				}
				meshBuilder.End();
				pMesh->Draw();
			}

			g_pPhysicsCollision->DestroyDebugMesh( vertCount, outVerts );
		}
		else
		{
			pParser->SkipBlock();
		}
	}
	g_pPhysicsCollision->VPhysicsKeyParserDestroy( pParser );
}

void CMDLPanel::SetupRenderState( int nDisplayWidth, int nDisplayHeight )
{
	int iWidth = nDisplayWidth;
	int iHeight = nDisplayHeight;
	if ( m_bThumbnailSafeZone )
	{
		iWidth = THUMBNAIL_SAFE_ZONE_SIZE;
		iHeight = THUMBNAIL_SAFE_ZONE_SIZE;
	}
	BaseClass::SetupRenderState( iWidth, iHeight );
}

//-----------------------------------------------------------------------------
// paint it!
//-----------------------------------------------------------------------------
void CMDLPanel::OnPaint3D()
{
	if ( m_RootMDL.m_MDL.GetMDL() == MDLHANDLE_INVALID )
		return;

	// FIXME: Move this call into DrawModel in StudioRender
	StudioRenderConfig_t oldStudioRenderConfig;
	StudioRender()->GetCurrentConfig( oldStudioRenderConfig );

	UpdateStudioRenderConfig();

	CMatRenderContextPtr pRenderContext( vgui::MaterialSystem() );
	if ( vgui::MaterialSystemHardwareConfig()->GetHDRType() == HDR_TYPE_NONE )
	{
		ITexture *pMyCube = HasLightProbe() ? GetLightProbeCubemap( false ) : m_DefaultEnvCubemap;
		pRenderContext->BindLocalCubemap( pMyCube );
	}
	else
	{
		ITexture *pMyCube = HasLightProbe() ? GetLightProbeCubemap( true ) : m_DefaultHDREnvCubemap;
		pRenderContext->BindLocalCubemap( pMyCube );
	}

	PrePaint3D( pRenderContext );

	if ( m_bGroundGrid )
	{
		DrawGrid();
	}

	if ( m_bLookAtCamera )
	{
		matrix3x4_t worldToCamera;
		ComputeCameraTransform( &worldToCamera );

		Vector vecPosition;
		MatrixGetColumn( worldToCamera, 3, vecPosition );
		m_RootMDL.m_MDL.m_bWorldSpaceViewTarget = true;
		m_RootMDL.m_MDL.m_vecViewTarget = vecPosition;
	}

	// Draw the MDL
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

	SetupFlexWeights();

	matrix3x4_t *pBoneToWorld = g_pStudioRender->LockBoneMatrices( studioHdr.numbones() );
	m_RootMDL.m_MDL.SetUpBones( m_RootMDL.m_MDLToWorld, studioHdr.numbones(), pBoneToWorld, m_PoseParameters, m_SequenceLayers, m_nNumSequenceLayers );
	g_pStudioRender->UnlockBoneMatrices();

	IMaterial* pOverrideMaterial = GetOverrideMaterial( m_RootMDL.m_MDL.GetMDL() );
	if ( pOverrideMaterial != NULL )
		g_pStudioRender->ForcedMaterialOverride( pOverrideMaterial );

	m_RootMDL.m_MDL.Draw( m_RootMDL.m_MDLToWorld, pBoneToWorld );

	if ( pOverrideMaterial != NULL )
		g_pStudioRender->ForcedMaterialOverride( NULL );

	pOverrideMaterial = NULL;

	// Draw the merge MDLs.
	matrix3x4_t matMergeBoneToWorld[MAXSTUDIOBONES];
	int nMergeCount = m_aMergeMDLs.Count();
	for ( int iMerge = 0; iMerge < nMergeCount; ++iMerge )
	{
		if ( m_aMergeMDLs[iMerge].m_bDisabled )
			continue;

		// Get the merge studio header.
		CStudioHdr *pMergeHdr = m_aMergeMDLs[iMerge].m_pStudioHdr;
		matrix3x4_t *pMergeBoneToWorld = &matMergeBoneToWorld[0];

		// If we have a valid mesh, bonemerge it. If we have an invalid mesh we can't bonemerge because
		// it'll crash trying to pull data from the missing header.
		if ( pMergeHdr != NULL )
		{
			CStudioHdr &mergeHdr = *pMergeHdr;
			m_aMergeMDLs[iMerge].m_MDL.SetupBonesWithBoneMerge( &mergeHdr, pMergeBoneToWorld, &studioHdr, pBoneToWorld, m_RootMDL.m_MDLToWorld );		

			pOverrideMaterial = GetOverrideMaterial( m_aMergeMDLs[iMerge].m_MDL.GetMDL() );
			if ( pOverrideMaterial != NULL ) 
				g_pStudioRender->ForcedMaterialOverride( pOverrideMaterial );

			m_aMergeMDLs[iMerge].m_MDL.Draw( m_aMergeMDLs[iMerge].m_MDLToWorld, pMergeBoneToWorld );

			if ( pOverrideMaterial != NULL )
				g_pStudioRender->ForcedMaterialOverride( NULL );

			// Notify of model render
			RenderingMergedModel( pRenderContext, &mergeHdr, m_aMergeMDLs[iMerge].m_MDL.GetMDL(), pMergeBoneToWorld );
		}
	}

	RenderingRootModel( pRenderContext, &studioHdr, m_RootMDL.m_MDL.GetMDL(), pBoneToWorld );

	PostPaint3D( pRenderContext );

	if ( m_bDrawCollisionModel )
	{
		DrawCollisionModel();
	}

	pRenderContext->Flush();
	StudioRender()->UpdateConfig( oldStudioRenderConfig );
}


//-----------------------------------------------------------------------------
// Sets the current LOD
//-----------------------------------------------------------------------------
void CMDLPanel::SetLOD( int nLOD )
{
	m_RootMDL.m_MDL.m_nLOD = nLOD;

	int nMergeCount = m_aMergeMDLs.Count();
	for ( int iMerge = 0; iMerge < nMergeCount; ++iMerge )
	{
		m_aMergeMDLs[iMerge].m_MDL.m_nLOD = nLOD;
	}
}


//-----------------------------------------------------------------------------
// Sets the current sequence
//-----------------------------------------------------------------------------
void CMDLPanel::SetSequence( int nSequence, bool bResetSequence )
{
	m_RootMDL.m_MDL.m_nSequence = nSequence;

	if ( bResetSequence )
	{
		m_RootMDL.m_flCycleStartTime = GetAutoPlayTime();
	}
}


//-----------------------------------------------------------------------------
// Set the current pose parameters. If NULL the pose parameters will be reset
// to the default values.
//-----------------------------------------------------------------------------
void CMDLPanel::SetPoseParameters( const float *pPoseParameters, int nCount )
{
	if ( pPoseParameters )
	{
		int nParameters = MIN( MAXSTUDIOPOSEPARAM, nCount );
		for ( int iParam = 0; iParam < nParameters; ++iParam )
		{
			m_PoseParameters[ iParam ] = pPoseParameters[ iParam ];
		}
	}
	else if ( m_RootMDL.m_MDL.GetMDL() != MDLHANDLE_INVALID )
	{
		CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
		Studio_CalcDefaultPoseParameters( &studioHdr, m_PoseParameters, MAXSTUDIOPOSEPARAM );
	}
}


//-----------------------------------------------------------------------------
// Set a pose parameter by name
//-----------------------------------------------------------------------------
bool CMDLPanel::SetPoseParameterByName( const char *pszName, float fValue )
{
	if ( !m_RootMDL.m_pStudioHdr )
		return false;
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
	int nPoseCount = studioHdr.GetNumPoseParameters();

	for ( int i = 0; i < nPoseCount; ++i )
	{
		const mstudioposeparamdesc_t &Pose = studioHdr.pPoseParameter( i );
		if ( V_strcasecmp( pszName, Pose.pszName() ) == 0 )
		{
			float ctlValue;
			Studio_SetPoseParameter( &studioHdr, i, fValue, ctlValue );
			m_PoseParameters[ i ] = ctlValue;
			return true;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Set the overlay sequence layers
//-----------------------------------------------------------------------------
void CMDLPanel::SetSequenceLayers( const MDLSquenceLayer_t *pSequenceLayers, int nCount )
{
	if ( pSequenceLayers )
	{
		m_nNumSequenceLayers = MIN( MAX_SEQUENCE_LAYERS, nCount );
		for ( int iLayer = 0; iLayer < m_nNumSequenceLayers; ++iLayer )
		{
			m_SequenceLayers[ iLayer ] = pSequenceLayers[ iLayer ];
			ResetAnimationEventState( &m_SequenceLayerEventState[ iLayer ] );
		}
	}
	else
	{
		m_nNumSequenceLayers = 0;
		V_memset( m_SequenceLayers, 0, sizeof( m_SequenceLayers ) );
	}
}

//-----------------------------------------------------------------------------
// Set the current skin
//-----------------------------------------------------------------------------
void CMDLPanel::SetSkin( int nSkin )
{
	m_RootMDL.m_MDL.m_nSkin = nSkin;
}

//-----------------------------------------------------------------------------
// called when we're ticked...
//-----------------------------------------------------------------------------
void CMDLPanel::OnTick()
{
	BaseClass::OnTick();
	if ( m_RootMDL.m_MDL.GetMDL() != MDLHANDLE_INVALID )
	{
		ValidateMDLs();

		m_RootMDL.m_MDL.m_flTime = ( GetAutoPlayTime() - m_RootMDL.m_flCycleStartTime );
	
		DoAnimationEvents();
	}
}

void CMDLPanel::Paint()
{
	BaseClass::Paint();

	if ( m_bThumbnailSafeZone )
	{
		int iWidth, iHeight;
		GetSize( iWidth, iHeight );

		CMatRenderContextPtr pRenderContext( vgui::MaterialSystem() );

		IMaterial *safezone = materials->FindMaterial( "vgui/thumbnails_safezone", TEXTURE_GROUP_VGUI, true );
		if ( safezone )
		{
			safezone->IncrementReferenceCount();
		}

		int screenposx = 0;
		int screenposy = 0;
		LocalToScreen( screenposx, screenposy );

		int iScaledHeight = THUMBNAIL_SAFE_ZONE_HEIGHT_SCALE * iHeight;
		int iRemappedHeight = RemapVal( iScaledHeight, 0, THUMBNAIL_SAFE_ZONE_HEIGHT, 0, iScaledHeight );

		pRenderContext->DrawScreenSpaceRectangle( safezone, screenposx, screenposy, iWidth, iRemappedHeight,
			0, 0,
			THUMBNAIL_SAFE_ZONE_SIZE, THUMBNAIL_SAFE_ZONE_HEIGHT,
			THUMBNAIL_SAFE_ZONE_SIZE, THUMBNAIL_SAFE_ZONE_SIZE );

		screenposx = 0;
		screenposy = iHeight - iRemappedHeight;
		LocalToScreen( screenposx, screenposy );
		pRenderContext->DrawScreenSpaceRectangle( safezone, screenposx, screenposy, iWidth, iRemappedHeight,
			0, 0,
			THUMBNAIL_SAFE_ZONE_SIZE, THUMBNAIL_SAFE_ZONE_HEIGHT,
			THUMBNAIL_SAFE_ZONE_SIZE, THUMBNAIL_SAFE_ZONE_SIZE );

		if ( safezone )
		{
			safezone->DecrementReferenceCount();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMDLPanel::DoAnimationEvents()
{
	if ( !m_RootMDL.m_pStudioHdr )
		return;
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

	// If we don't have any sequences, don't do anything
	if ( studioHdr.GetNumSeq() < 1 )
	{
		Assert( studioHdr.GetNumSeq() >= 1 );
		return;
	}

	// we're holding onto model data that can be forced reloaded when importing workshop items
	// detect the vmodel change here and force an update if so, until we fix the real issue of the studioHdr not being updated itself
	if ( studioHdr.GetVirtualModel() != nullptr && studioHdr.GetVirtualModel() != studioHdr.GetRenderHdr()->GetVirtualModel() )
	{
		studioHdr.ResetVModel( studioHdr.GetRenderHdr()->GetVirtualModel() );
	}

	DoAnimationEvents( &studioHdr, m_RootMDL.m_MDL.m_nSequence, m_RootMDL.m_MDL.m_flTime, false, &m_EventState );

	for ( int i = 0; i < m_nNumSequenceLayers; ++i )
	{
		float flTime = m_RootMDL.m_MDL.m_flTime - m_SequenceLayers[ i ].m_flCycleBeganAt;
		//Plat_DebugString( CFmtStr("Animation: time = %f, started = %f, delta = %f\n",m_RootMDL.m_MDL.m_flTime,m_SequenceLayers[ i ].m_flCycleBeganAt,flTime ) );
		DoAnimationEvents( &studioHdr, m_SequenceLayers[ i ].m_nSequenceIndex, flTime, m_SequenceLayers[ i ].m_bNoLoop, &m_SequenceLayerEventState[ i ] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMDLPanel::DoAnimationEvents( CStudioHdr *pStudioHdr, int nSeqNum, float flTime, bool bNoLoop, MDLAnimEventState_t *pEventState )
{
	if ( nSeqNum < 0 || nSeqNum >= pStudioHdr->GetNumSeq() )
	{
		return;
	}

	mstudioseqdesc_t &seqdesc = pStudioHdr->pSeqdesc( nSeqNum );
	if ( seqdesc.numevents == 0 )
	{
		return;
	}

	mstudioevent_t *pevent = seqdesc.pEvent( 0 );

	int nFrameCount = Studio_MaxFrame( pStudioHdr, nSeqNum, m_PoseParameters );
	if ( nFrameCount == 0 )
	{
		nFrameCount = 1;
	}
	float flEventCycle = ( flTime * m_RootMDL.m_MDL.m_flPlaybackRate ) / nFrameCount;
	//Plat_DebugString( CFmtStr("Event cycle: %f, playback rate: %f, frame count: %d\n", flEventCycle, m_RootMDL.m_MDL.m_flPlaybackRate, nFrameCount ) );
	if ( bNoLoop )
	{
		flEventCycle = MIN(flEventCycle, 1.0f);
	}
	else
	{
		flEventCycle -= (int)(flEventCycle);
	}

	if ( pEventState->m_nEventSequence != nSeqNum )
	{
		pEventState->m_nEventSequence = nSeqNum;
		flEventCycle = 0.0f;
		pEventState->m_flPrevEventCycle = -0.01f; // back up to get 0'th frame animations
	}

	if ( flEventCycle == pEventState->m_flPrevEventCycle )
	{
		return;
	}

	// check for looping
	BOOL bLooped = (flEventCycle < pEventState->m_flPrevEventCycle);

	// This makes sure events that occur at the end of a sequence occur are
	// sent before events that occur at the beginning of a sequence.
	if (bLooped)
	{
		for (int i = 0; i < (int)seqdesc.numevents; i++)
		{
			if ( pevent[i].cycle <= pEventState->m_flPrevEventCycle )
				continue;

			FireEvent( pevent[ i ].pszEventName(), pevent[ i ].pszOptions() );
		}

		// Necessary to get the next loop working
		pEventState->m_flPrevEventCycle = -0.01f;
	}

	for (int i = 0; i < (int)seqdesc.numevents; i++)
	{
		if ( (pevent[i].cycle > pEventState->m_flPrevEventCycle && pevent[i].cycle <= flEventCycle) )
		{
			FireEvent( pevent[ i ].pszEventName(), pevent[ i ].pszOptions() );
		}
	}

	pEventState->m_flPrevEventCycle = flEventCycle;
}

void CMDLPanel::FireEvent( const char *pszEventName, const char *pszEventOptions )
{
	KeyValues* pKVEvent = new KeyValues( "AnimEvent" );
	pKVEvent->SetString( "name", pszEventName );
	pKVEvent->SetString( "options", pszEventOptions );
	PostActionSignal( pKVEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMDLPanel::ResetAnimationEventState( MDLAnimEventState_t *pEventState )
{
	pEventState->m_nEventSequence = -1;
	pEventState->m_flPrevEventCycle = -0.01f;
}

//-----------------------------------------------------------------------------
// input
//-----------------------------------------------------------------------------
void CMDLPanel::OnMouseDoublePressed( vgui::MouseCode code )
{
	if ( m_bIgnoreDoubleClick )
		return;

	float flRadius;
	Vector vecCenter;
	GetBoundingSphere( vecCenter, flRadius );
	LookAt( vecCenter, flRadius );

	BaseClass::OnMouseDoublePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMDLPanel::SetMergeMDL( MDLHandle_t handle, void *pProxyData, int nSkin /*= -1 */ )
{
	// Verify that we have a root model to merge to.
	if ( m_RootMDL.m_MDL.GetMDL() == MDLHANDLE_INVALID )
		return;

	int iIndex = m_aMergeMDLs.AddToTail();
	if ( !m_aMergeMDLs.IsValidIndex( iIndex ) )
		return;

	m_aMergeMDLs[iIndex].m_MDL.SetMDL( handle );

	if ( nSkin != -1 )
	{
		m_aMergeMDLs[iIndex].m_MDL.m_nSkin = nSkin;
	}

	m_aMergeMDLs[iIndex].m_MDL.m_nLOD = m_RootMDL.m_MDL.m_nLOD;

	m_aMergeMDLs[iIndex].m_MDL.m_pProxyData = pProxyData;
	SetIdentityMatrix( m_aMergeMDLs[iIndex].m_MDLToWorld );

	m_aMergeMDLs[iIndex].m_bDisabled = false;

	m_aMergeMDLs[iIndex].m_pStudioHdr = new CStudioHdr( m_aMergeMDLs[iIndex].m_MDL.GetStudioHdr(), g_pMDLCache );

	// Need to invalidate the layout so the panel will adjust is LookAt for the new model.
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
MDLHandle_t CMDLPanel::SetMergeMDL( const char *pMDLName, void *pProxyData, int nSkin /*= -1 */ )
{
	MDLHandle_t hMDLFindResult = vgui::MDLCache()->FindMDL( pMDLName );
	MDLHandle_t hMDL = pMDLName ? hMDLFindResult : MDLHANDLE_INVALID;
	if ( vgui::MDLCache()->IsErrorModel( hMDL ) )
	{
		hMDL = MDLHANDLE_INVALID;
	}

	SetMergeMDL( hMDL, pProxyData, nSkin );

	// FindMDL takes a reference and the the CMDL will also hold a reference for as long as it sticks around. Release the FindMDL reference.
	int nRef = vgui::MDLCache()->Release( hMDLFindResult );
	(void)nRef; // Avoid unreferenced variable warning
	AssertMsg( hMDL == MDLHANDLE_INVALID || nRef > 0, "CMDLPanel::SetMergeMDL referenced a model that has a zero ref count." );

	return hMDL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMDLPanel::GetMergeMDLIndex( void *pProxyData )
{
	int nMergeCount = m_aMergeMDLs.Count();
	for ( int iMerge = 0; iMerge < nMergeCount; ++iMerge )
	{
		if ( m_aMergeMDLs[iMerge].m_MDL.m_pProxyData == pProxyData )
			return iMerge;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMDLPanel::GetMergeMDLIndex( MDLHandle_t handle )
{
	int nMergeCount = m_aMergeMDLs.Count();
	for ( int iMerge = 0; iMerge < nMergeCount; ++iMerge )
	{
		if ( m_aMergeMDLs[iMerge].m_MDL.GetMDL() == handle )
			return iMerge;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMDL *CMDLPanel::GetMergeMDL( MDLHandle_t handle )
{
	int nMergeCount = m_aMergeMDLs.Count();
	for ( int iMerge = 0; iMerge < nMergeCount; ++iMerge )
	{
		if ( m_aMergeMDLs[iMerge].m_MDL.GetMDL() == handle )
			return (&m_aMergeMDLs[iMerge].m_MDL);
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *CMDLPanel::GetMergeMDLStudioHdr( MDLHandle_t handle )
{
	int nMergeCount = m_aMergeMDLs.Count();
	for ( int iMerge = 0; iMerge < nMergeCount; ++iMerge )
	{
		if ( m_aMergeMDLs[ iMerge ].m_MDL.GetMDL() == handle )
			return ( m_aMergeMDLs[ iMerge ].m_pStudioHdr );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMDLPanel::ClearMergeMDLs( void )
{
	const int nMergeCount = m_aMergeMDLs.Count();
	for ( int iMerge = 0; iMerge < nMergeCount; ++iMerge )
	{
		if ( !m_aMergeMDLs[iMerge].m_pStudioHdr )
		{
			continue;
		}
		delete m_aMergeMDLs[iMerge].m_pStudioHdr;
		m_aMergeMDLs[iMerge].m_pStudioHdr = NULL;
	}

	m_aMergeMDLs.Purge();
}

void CMDLPanel::ValidateMDLs()
{
	uint32 uMdlCacheSerial = vgui::ivgui()->GetMdlCacheSerial();

	if ( m_RootMDL.m_pStudioHdr && m_RootMDL.m_unMdlCacheSerial != uMdlCacheSerial )
	{
		m_RootMDL.m_pStudioHdr = new CStudioHdr( m_RootMDL.m_MDL.GetStudioHdr(), g_pMDLCache );
		m_RootMDL.m_unMdlCacheSerial = uMdlCacheSerial;
	}

	const int nMergeCount = m_aMergeMDLs.Count();
	for ( int iMerge = 0; iMerge < nMergeCount; ++iMerge )
	{
		if ( m_aMergeMDLs[iMerge].m_pStudioHdr && m_aMergeMDLs[iMerge].m_unMdlCacheSerial != uMdlCacheSerial )
		{
			m_aMergeMDLs[iMerge].m_pStudioHdr = new CStudioHdr( m_aMergeMDLs[iMerge].m_MDL.GetStudioHdr(), g_pMDLCache );
			m_aMergeMDLs[iMerge].m_unMdlCacheSerial = uMdlCacheSerial;
		}
	}
}
