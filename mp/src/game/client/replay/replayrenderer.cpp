//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replayrenderer.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterialproxy.h"
#include "replay/vgui/replayrenderoverlay.h"
#include "replay/replay.h"
#include "replay/ireplaymoviemanager.h"
#include "replay/ireplayperformancecontroller.h"
#include "replay/ireplaymovie.h"
#include "replay/ireplaymanager.h"
#include "replay/ienginereplay.h"
#include "replay/iclientreplaycontext.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "replay/replaycamera.h"
#include "bitmap/tgawriter.h"
#include "filesystem.h"

#define REPLAY_RECORDING_ENABLE

#ifdef REPLAY_RECORDING_ENABLE
#include "video/ivideoservices.h"
#endif

#define TMP_WAVE_FILENAME "tmpaudio"

//#define TRACE_REPLAY_STATE_MACHINE

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

extern IReplayMovieManager *g_pReplayMovieManager;
extern IReplayPerformanceController *g_pReplayPerformanceController;

// Map quality index to number of samples
static int s_DoFQualityToSamples[MAX_DOF_QUALITY+1] = {8, 16, 32};//, 64, 128 };

// 4-entry table of values in 2D -1 to +1 range using Poisson disk distribution
static Vector2D g_vJitterTable4[4] =   { Vector2D (0.5318f, -0.6902f ), Vector2D (-0.5123f, 0.8362f ), Vector2D (-0.5193f, -0.2195f ), Vector2D (0.4749f, 0.3478f ) };

// 8-entry table of values in 2D -1 to +1 range using Poisson disk distribution
static Vector2D g_vJitterTable8[8] =   { Vector2D (0.3475f, 0.0042f ),Vector2D (0.8806f, 0.3430f ),Vector2D (-0.0041f, -0.6197f ),Vector2D (0.0472f, 0.4964f ),
	Vector2D (-0.3730f, 0.0874f ),Vector2D (-0.9217f, -0.3177f ),Vector2D (-0.6289f, 0.7388f ),Vector2D (0.5744f, -0.7741f ) };

// 16-entry table of values in 2D -1 to +1 range using Poisson disk distribution (disk size 0.38f)
static Vector2D g_vJitterTable16[16] = { Vector2D (0.0747f, -0.8341f ),Vector2D (-0.9138f, 0.3251f ),Vector2D (0.8667f, -0.3029f ),Vector2D (-0.4642f, 0.2187f ),
	Vector2D (-0.1505f, 0.7320f ),Vector2D (0.7310f, -0.6786f ),Vector2D (0.2859f, -0.3254f ),Vector2D (-0.1311f, -0.2292f ),
	Vector2D (0.3518f, 0.6470f ),Vector2D (-0.7485f, -0.6307f ),Vector2D (0.1687f, 0.1873f ),Vector2D (-0.3604f, -0.7483f ),
	Vector2D (-0.5658f, -0.1521f ),Vector2D (0.7102f, 0.0536f ),Vector2D (-0.6056f, 0.7747f ),Vector2D (0.7793f, 0.6194f ) };

// 32-entry table of values in 2D -1 to +1 range using Poisson disk distribution (disk size 0.28f)
static Vector2D g_vJitterTable32[32] = { Vector2D (0.0854f, -0.0644f ),Vector2D (0.8744f, 0.1665f ),Vector2D (0.2329f, 0.3995f ),Vector2D (-0.7804f, 0.5482f ),
	Vector2D (-0.4577f, 0.7647f ),Vector2D (-0.1936f, 0.5564f ),Vector2D (0.4205f, -0.5768f ),Vector2D (-0.0304f, -0.9050f ),
	Vector2D (-0.5215f, 0.1854f ),Vector2D (0.3161f, -0.2954f ),Vector2D (0.0666f, -0.5564f ),Vector2D (-0.2137f, -0.0072f ),
	Vector2D (-0.4112f, -0.3311f ),Vector2D (0.6438f, -0.2484f ),Vector2D (-0.9055f, -0.0360f ),Vector2D (0.8323f, 0.5268f ),
	Vector2D (0.5592f, 0.3459f ),Vector2D (-0.6797f, -0.5201f ),Vector2D (-0.4325f, -0.8857f ),Vector2D (0.8768f, -0.4197f ),
	Vector2D (0.3090f, -0.8646f ),Vector2D (0.5034f, 0.8603f ),Vector2D (0.3752f, 0.0627f ),Vector2D (-0.0161f, 0.2627f ),
	Vector2D (0.0969f, 0.7054f ),Vector2D (-0.2291f, -0.6595f ),Vector2D (-0.5887f, -0.1100f ),Vector2D (0.7048f, -0.6528f ),
	Vector2D (-0.8438f, 0.2706f ),Vector2D (-0.5061f, 0.4653f ),Vector2D (-0.1245f, -0.3302f ),Vector2D (-0.1801f, 0.8486f )};

//-----------------------------------------------------------------------------

//
// Accumulation material proxy for ping-pong accumulation buffer imp.
//

struct AccumParams_t
{
	ITexture	*m_pTexture0;
	ITexture	*m_pTexture1;
	float		 m_fSampleWeight;
	bool		 m_bClear;
};

class CAccumBuffProxy : public IMaterialProxy
{
public:
	CAccumBuffProxy();
	virtual ~CAccumBuffProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial();

private:
	IMaterialVar *m_pTexture0;
	IMaterialVar *m_pTexture1;
	IMaterialVar *m_pAccumBuffWeights;
};

//-----------------------------------------------------------------------------

CAccumBuffProxy::CAccumBuffProxy()
{
	m_pTexture0			= NULL;
	m_pTexture1			= NULL;
	m_pAccumBuffWeights	= NULL;
}

CAccumBuffProxy::~CAccumBuffProxy()
{
}

bool CAccumBuffProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool foundVar;

	// Grab the Material variables for the accumulation shader
	m_pTexture0 = pMaterial->FindVar( "$TEXTURE0", &foundVar, false );
	if( !foundVar )
		return false;

	m_pTexture1 = pMaterial->FindVar( "$TEXTURE1", &foundVar, false );
	if( !foundVar )
		return false;

	m_pAccumBuffWeights = pMaterial->FindVar( "$WEIGHTS", &foundVar, false );
	if( !foundVar )
		return false;

	return true;
}

void CAccumBuffProxy::OnBind( void *pC_BaseEntity )
{
	AccumParams_t *pAccumParams = (AccumParams_t *) pC_BaseEntity;

	if( !m_pTexture0 || !m_pTexture1 || !m_pAccumBuffWeights )
	{
		return;
	}

	m_pTexture0->SetTextureValue( pAccumParams->m_pTexture0 ); 
	m_pTexture1->SetTextureValue( pAccumParams->m_pTexture1 ); 

	// If we're just using this material to do a clear to black...
	if ( pAccumParams->m_bClear )
	{
		m_pAccumBuffWeights->SetVecValue( 0.0f, 0.0f, 0.0f, 0.0f );
	}
	else
	{
		m_pAccumBuffWeights->SetVecValue( pAccumParams->m_fSampleWeight, 1.0f - pAccumParams->m_fSampleWeight, 0.0f, 0.0f );
	}
}

IMaterial *CAccumBuffProxy::GetMaterial()
{
	return m_pAccumBuffWeights ? m_pAccumBuffWeights->GetOwningMaterial() : NULL;
}

//-----------------------------------------------------------------------------

EXPOSE_INTERFACE( CAccumBuffProxy, IMaterialProxy, "accumbuff4sample" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------

CReplayRenderer::CReplayRenderer( CReplayRenderOverlay *pOverlay )
:	m_bIsAudioSyncFrame( false ),
	m_pRenderOverlay( pOverlay ),
	m_nCurrentPingPong( 0 ),
	m_nCurSample( 0 ),
	m_nTimeStep( 0 ),
	m_curSampleTime( 0 ),
	m_nFrame( 0 ),
	m_nNumJitterSamples( 0 ),
	m_iTgaFrame( 0 ),
	m_pLayoffBuf( NULL ),
	m_pMovie( NULL ),
	m_pMovieMaker( NULL ),
	m_pJitterTable( NULL ),
	m_pViewmodelFov( NULL ),
	m_pDefaultFov( NULL ),
	m_bCacheFullSceneState( false ),
	m_bShutterClosed( false ),
	m_bForceCheapDoF( false )
{
}

CReplayRenderer::~CReplayRenderer()
{
}

const CReplayPerformance *CReplayRenderer::GetPerformance() const
{
	CReplay *pReplay = g_pReplayManager->GetPlayingReplay();
	if ( !pReplay )
		return NULL;

	return m_RenderParams.m_iPerformance >= 0 ? pReplay->GetPerformance( m_RenderParams.m_iPerformance ) : NULL;
}

const char *CReplayRenderer::GetMovieFilename() const
{
	if ( !m_pMovie ) 
		return NULL;

	return m_pMovie->GetMovieFilename();
}

// -------------------------------------------------------------------
// Functions used by audio engine to distinguish between sub-frames
//  rendered for motion blur, and the actual frames being recorded
// -------------------------------------------------------------------
void CReplayRenderer::SetAudioSyncFrame( bool isSync )
{
	m_bIsAudioSyncFrame = isSync;
}

bool CReplayRenderer::IsAudioSyncFrame()
{
	return m_bIsAudioSyncFrame;
}

float CReplayRenderer::GetRecordingFrameDuration()
{
	double actualFPS = m_RenderParams.m_Settings.m_FPS.GetFPS();
	if ( actualFPS <= 0.0 )
	{
		Assert( false );
		return 30.0f;
	}
	
	double interval = 1.0 / actualFPS;
	
	return (float) interval;
}

bool CReplayRenderer::SetupRenderer( RenderMovieParams_t &params, IReplayMovie *pMovie )
{
	// Cache render parameters
	V_memcpy( &m_RenderParams, &params, sizeof( params ) );

	// Cache movie
	m_pMovie = pMovie;

	// Reset current frame
	m_nFrame = 0;
	m_nTimeStep = 0;
	m_nCurSample = 0;
	m_iTgaFrame = 0;
	m_curSampleTime = DmeTime_t(0);

	m_pViewmodelFov = ( ConVar * )cvar->FindVar( "viewmodel_fov" );
	m_pDefaultFov = ( ConVar * )cvar->FindVar( "default_fov" );

	InitBuffers( params );

#ifdef REPLAY_RECORDING_ENABLE
	// Record directly to a .wav file if desired via 'startmovie' and write out TGA's
	if ( params.m_bExportRaw )
	{
		// Create the temporary wave file
		g_pEngineClientReplay->Wave_CreateTmpFile( TMP_WAVE_FILENAME );

		// Create the path for the movie
		m_fmtTgaRenderDirName = g_pClientReplayContext->GetMovieManager()->GetRawExportDir();

		g_pFullFileSystem->CreateDirHierarchy( m_fmtTgaRenderDirName.Access() );
	}
	else
	{
		// Record to a movie using video services.
		if ( !g_pVideo )
			return false;

#ifdef USE_WEBM_FOR_REPLAY
		m_pMovieMaker = g_pVideo->CreateVideoRecorder( VideoSystem::WEBM );
#else
		m_pMovieMaker = g_pVideo->CreateVideoRecorder( VideoSystem::QUICKTIME );
#endif		
		if ( !m_pMovieMaker )
			return false;

		CFmtStr fmtMovieFullFilename( "%s%s", g_pReplayMovieManager->GetRenderDir(), pMovie->GetMovieFilename() );
		
		bool bSuccess = false;
		if ( m_pMovieMaker->CreateNewMovieFile( fmtMovieFullFilename.Access(), true ) )
		{
			const ReplayRenderSettings_t &Settings = params.m_Settings;
			
#ifndef USE_WEBM_FOR_REPLAY
			ConVarRef QTEncodeGamma( "video_quicktime_encode_gamma" );
			VideoEncodeGamma_t encodeGamma = ( QTEncodeGamma.IsValid() ) ? (VideoEncodeGamma_t) QTEncodeGamma.GetInt() : VideoEncodeGamma::GAMMA_2_2;
#else
			VideoEncodeGamma_t encodeGamma = VideoEncodeGamma::GAMMA_2_2;
#endif
			
			if ( m_pMovieMaker->SetMovieVideoParameters( Settings.m_Codec, Settings.m_nEncodingQuality, (int)Settings.m_nWidth, (int)Settings.m_nHeight, Settings.m_FPS, encodeGamma ) )
			{
				if ( m_pMovieMaker->SetMovieSourceImageParameters( VideoEncodeSourceFormat::BGRA_32BIT, (int)Settings.m_nWidth, (int)Settings.m_nHeight ) )
				{
					AudioEncodeOptions_t audioOptions =  AudioEncodeOptions::USE_AUDIO_ENCODE_GROUP_SIZE | AudioEncodeOptions::GROUP_SIZE_IS_VIDEO_FRAME |
														 AudioEncodeOptions::LIMIT_AUDIO_TRACK_TO_VIDEO_DURATION | AudioEncodeOptions::PAD_AUDIO_WITH_SILENCE ;
					
					if ( m_pMovieMaker->SetMovieSourceAudioParameters( AudioEncodeSourceFormat::AUDIO_16BIT_PCMStereo, 44100, audioOptions ) )
					{
						bSuccess = true;
					}
				}
			}
		}
		
		if ( !bSuccess )
		{
			g_pVideo->DestroyVideoRecorder( m_pMovieMaker );
			m_pMovieMaker = NULL;
			return false;
		}
	}

	SetupJitterTable();
#endif

	m_pRenderOverlay->Show();
	
	return true;
}

bool CReplayRenderer::SetupJitterTable()
{
	const int nNumSamples = NumMotionBlurTimeSteps();

	switch ( nNumSamples )
	{
	case 4:		m_pJitterTable = g_vJitterTable4; break;
	case 8:		m_pJitterTable = g_vJitterTable8; break;
	case 16:	m_pJitterTable = g_vJitterTable16; break;
	case 32:	m_pJitterTable = g_vJitterTable32; break;
//	case 64:	m_pJitterTable = g_vJitterTable64; break;
//	case 128:	m_pJitterTable = g_vJitterTable128; break;
	default:	return false;
	}

	m_nNumJitterSamples = nNumSamples;

	return true;
}

void CReplayRenderer::InitBuffers( const RenderMovieParams_t &params )
{
	const ReplayRenderSettings_t &Settings = params.m_Settings;

	Assert( m_pLayoffBuf == NULL );
	m_pLayoffBuf = new BGRA8888_t[ Settings.m_nWidth * Settings.m_nHeight ];

	CFmtStr fmtHostFramerateCmd( "host_framerate %f\n", params.m_flEngineFps );
	engine->ClientCmd_Unrestricted( fmtHostFramerateCmd.Access() );

    g_pMaterialSystem->BeginRenderTargetAllocation();								// Begin allocating RTs which IFM can scribble into

	// Offscreen surface for rendering individual samples
	ImageFormat AccumSampleFormat = (g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_FLOAT) ? IMAGE_FORMAT_RGBA16161616F : g_pMaterialSystem->GetBackBufferFormat();
	m_AccumBuffSample.Init(
		g_pMaterialSystem->CreateNamedRenderTargetTextureEx2(
			"_rt_Replay_Accum_Sample", Settings.m_nWidth, Settings.m_nHeight, RT_SIZE_OFFSCREEN,
			AccumSampleFormat, MATERIAL_RT_DEPTH_SHARED, TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_POINTSAMPLE
		)
	);

	// Ping-Pong textures for accumulating result prior to final tone map
	ImageFormat PingPongFormat = IMAGE_FORMAT_BGR888;
	m_AccumBuffPingPong[0].Init(g_pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_Replay_Ping", Settings.m_nWidth, Settings.m_nHeight, RT_SIZE_OFFSCREEN,
		PingPongFormat, MATERIAL_RT_DEPTH_NONE, TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_POINTSAMPLE ));
	m_AccumBuffPingPong[1].Init(g_pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_Replay_Pong", Settings.m_nWidth, Settings.m_nHeight, RT_SIZE_OFFSCREEN,
		PingPongFormat, MATERIAL_RT_DEPTH_NONE, TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_POINTSAMPLE ));

	// LDR final result of either HDR or LDR rendering
	m_LayoffResult.Init(g_pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_LayoffResult", Settings.m_nWidth, Settings.m_nHeight, RT_SIZE_OFFSCREEN,
		g_pMaterialSystem->GetBackBufferFormat(), MATERIAL_RT_DEPTH_SHARED, TEXTUREFLAGS_BORDER | TEXTUREFLAGS_POINTSAMPLE ));

    g_pMaterialSystem->EndRenderTargetAllocation();								// Begin allocating RTs which IFM can scribble into

	KeyValues *pVMTKeyValues = new KeyValues( "accumbuff4sample" );
	pVMTKeyValues->SetString( "$TEXTURE0", m_AccumBuffSample->GetName() ); // Dummy
	pVMTKeyValues->SetString( "$TEXTURE1", m_AccumBuffSample->GetName() ); // Dummy
	pVMTKeyValues->SetString( "$TEXTURE2", m_AccumBuffSample->GetName() ); // Dummy
	pVMTKeyValues->SetString( "$TEXTURE3", m_AccumBuffSample->GetName() ); // Dummy
	pVMTKeyValues->SetString( "$WEIGHTS", "[0.25 0.75 0.0 0.0]" );
	pVMTKeyValues->SetInt( "$nocull", 1 );
	KeyValues *pProxiesKV = pVMTKeyValues->FindKey( "proxies", true );	// create a subkey
	pProxiesKV->FindKey( "accumbuff4sample", true ); // create
	m_FourSampleResolveMatRef.Init( "accumbuff4sample", pVMTKeyValues );
	m_FourSampleResolveMatRef->Refresh();
}

void CReplayRenderer::ShutdownRenderer()
{
	if ( m_LayoffResult.IsValid() )
	{
		m_LayoffResult.Shutdown( true );
	}

	if ( m_AccumBuffSample.IsValid() )
	{
		m_AccumBuffSample.Shutdown( true );
	}

	for ( int i = 0; i < 2; ++i )
	{
		if ( m_AccumBuffPingPong[i].IsValid() )
		{
			m_AccumBuffPingPong[i].Shutdown( true );
		}
	}

	delete [] m_pLayoffBuf;
	m_pLayoffBuf = NULL;

#ifdef REPLAY_RECORDING_ENABLE
	if ( m_pMovieMaker )
	{
		m_pMovieMaker->FinishMovie( true );

		if ( g_pVideo )
		{
			g_pVideo->DestroyVideoRecorder( m_pMovieMaker );
		}

		m_pMovieMaker = NULL;
		m_pRenderOverlay->Hide();
	}
	else
#endif
	if ( m_RenderParams.m_bExportRaw )
	{
		// Mimicking what "startmovie" does here.
		g_pEngineClientReplay->Wave_FixupTmpFile( TMP_WAVE_FILENAME );

		// Move the temp wave file to the destination dir
		CFmtStr fmtTmpFilename( "%s%c%s.wav", engine->GetGameDirectory(), CORRECT_PATH_SEPARATOR, TMP_WAVE_FILENAME );
		CFmtStr fmtDstFilename( "%s%s", m_fmtTgaRenderDirName.Access(), "audio.wav" );
		g_pFullFileSystem->RenameFile( fmtTmpFilename.Access(), fmtDstFilename.Access() );
	}

	// Reset framerate
	engine->ClientCmd_Unrestricted( "host_framerate 0" );

	// Notify of performance end
	g_pReplayPerformanceController->Stop();
}

void CReplayRenderer::DrawResolvingQuad( int nWidth, int nHeight )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	CMeshBuilder meshBuilder;

	// Epsilons for 1:1 texel to pixel mapping
	float fWidthEpsilon = IsOSX() ? 0.0f : 0.5f / ((float) nWidth);
	float fHeightEpsilon = IsOSX() ? 0.0f : 0.5f / ((float) nHeight);

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( -1.0f, 1.0f, 0.5f );	// Upper left
	meshBuilder.TexCoord2f( 0, 0.0f + fWidthEpsilon, 0.0f + fHeightEpsilon );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( -1.0f,  -1.0f, 0.5f );	// Lower left
	meshBuilder.TexCoord2f( 0, 0.0f + fWidthEpsilon, 1.0f + fHeightEpsilon );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( 1.0f, -1.0f, 0.5f );	// Lower right
	meshBuilder.TexCoord2f( 0, 1.0f + fWidthEpsilon, 1.0f + fHeightEpsilon );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( 1.0f, 1.0f, 0.5f );		// Upper right
	meshBuilder.TexCoord2f( 0, 1.0f + fWidthEpsilon, 0.0f + fHeightEpsilon );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

void CReplayRenderer::BeginRenderingSample( int nSample, int x, int y, int nWidth, int nHeight, float fTonemapScale )
{
	// Always start on ping-pong buffer zero
	if ( nSample == 0 )
	{
		m_nCurrentPingPong = 0;
	}

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->PushRenderTargetAndViewport( m_AccumBuffSample, x, y, nWidth, nHeight );
}

void CReplayRenderer::ResolveSamples( int nSample, DmeTime_t frametime, int x, int y, int nWidth, int nHeight, bool bLayoffResult, float flBloomScale )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	// Render resolving quad to current ping-pong buffer
	AccumParams_t accParms = {
		m_AccumBuffSample,
		m_AccumBuffPingPong[ ( m_nCurrentPingPong + 1 ) % 2 ],
		1.0f / (float)( nSample + 1 ),
		false
	};
	pRenderContext->Bind( m_FourSampleResolveMatRef, &accParms );
	pRenderContext->PushRenderTargetAndViewport( m_AccumBuffPingPong[m_nCurrentPingPong], x, y, nWidth, nHeight );
		DrawResolvingQuad( nWidth, nHeight );
	pRenderContext->PopRenderTargetAndViewport();

	// If we want to show accumulated result to user...
	if ( bLayoffResult )
	{
		accParms.m_pTexture0 = m_AccumBuffPingPong[m_nCurrentPingPong];
		accParms.m_pTexture1 = m_AccumBuffPingPong[m_nCurrentPingPong];
		accParms.m_fSampleWeight = 1.0f;
		accParms.m_bClear = false;
		pRenderContext->Bind( m_FourSampleResolveMatRef, &accParms );

		pRenderContext->PushRenderTargetAndViewport( m_LayoffResult, x, y, nWidth, nHeight );
			DrawResolvingQuad( nWidth, nHeight );
		pRenderContext->PopRenderTargetAndViewport();
	}

	m_nCurrentPingPong = (m_nCurrentPingPong + 1) % 2;		// Flip the ping-pong buffers
}

bool CReplayRenderer::IsHDR() const
{
	return g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_FLOAT;
}

float CReplayRenderer::GetViewModelFOVOffset()
{
//	float flVMDefaultFov = m_pViewmodelFov ? m_pViewmodelFov->GetFloat() : 54.0f;
	float flVMDefaultFov = 54.0f;
	float flDefaultFov = m_pDefaultFov ? m_pDefaultFov->GetFloat() : 75.0f;

	return flVMDefaultFov - flDefaultFov;
}

void CReplayRenderer::SetupSampleView( int x, int y, int w, int h, int nSample, CViewSetup& viewSetup )
{
	// Frustum stuff

	// FIXME: This currently matches the client DLL for HL2
	// but we probably need a way of getting this state from the client DLL
	viewSetup.zNear = 3;
	viewSetup.zFar = 16384.0f * 1.73205080757f;
	viewSetup.x = x;
	viewSetup.y = y;
	viewSetup.width = w;
	viewSetup.height = h;
	viewSetup.m_flAspectRatio = (float)viewSetup.width / (float)viewSetup.height;

	const float fov = viewSetup.fov;
	float fHalfAngleRadians = DEG2RAD( 0.5f * fov );
	float t = tan( fHalfAngleRadians ) * (viewSetup.m_flAspectRatio / ( 4.0f / 3.0f ));
	viewSetup.fov = RAD2DEG( 2.0f * atan( t ) );

	viewSetup.fovViewmodel = viewSetup.fov + GetViewModelFOVOffset();
	viewSetup.zNearViewmodel = 1;
	viewSetup.zFarViewmodel = viewSetup.zFar;

	viewSetup.m_bOrtho = false; 
	viewSetup.m_bRenderToSubrectOfLargerScreen = true;

	SetupDOFMatrixSkewView( viewSetup.origin, viewSetup.angles, nSample, viewSetup );	// Sheared matrix method more comparable to image-space DoF approximation

	// Only have the engine do bloom and tone mapping if not HDR
	viewSetup.m_bDoBloomAndToneMapping = !IsHDR();

	viewSetup.m_bCacheFullSceneState = m_bCacheFullSceneState;
}

void CReplayRenderer::SetupDOFMatrixSkewView( const Vector &pos, const QAngle &angles, int nSample, CViewSetup& viewSetup )
{
	Vector vPosition = pos;

	matrix3x4_t matViewMatrix;												// Get transform
	AngleMatrix( angles, matViewMatrix );

	Vector vViewDirection, vViewLeft, vViewUp;
	MatrixGetColumn( matViewMatrix, 0, vViewDirection );
	MatrixGetColumn( matViewMatrix, 1, vViewLeft );
	MatrixGetColumn( matViewMatrix, 2, vViewUp );

	// Be sure these are normalized
	vViewDirection.NormalizeInPlace();
	vViewLeft.NormalizeInPlace();
	vViewUp.NormalizeInPlace();

	// Set up a non-skewed off-center projection matrix to start with...  (Posters already have this set up)
	viewSetup.m_bOffCenter				= true;
	viewSetup.m_flOffCenterBottom		= 0.0f;
	viewSetup.m_flOffCenterTop			= 1.0f;
	viewSetup.m_flOffCenterLeft			= 0.0f;
	viewSetup.m_flOffCenterRight		= 1.0f;

	if ( IsAntialiasingEnabled() && !IsDepthOfFieldEnabled() && !m_bForceCheapDoF )		// AA jitter but no DoF
	{
		Vector2D vAAJitter = m_pJitterTable[nSample % m_nNumJitterSamples];
		const float fHalfPixelRadius = 0.65;
		viewSetup.m_flOffCenterBottom += (vAAJitter.y / (float) viewSetup.height) * fHalfPixelRadius;
		viewSetup.m_flOffCenterTop    += (vAAJitter.y / (float) viewSetup.height) * fHalfPixelRadius;
		viewSetup.m_flOffCenterLeft   += (vAAJitter.x / (float) viewSetup.width)  * fHalfPixelRadius;
		viewSetup.m_flOffCenterRight  += (vAAJitter.x / (float) viewSetup.width)  * fHalfPixelRadius;

		viewSetup.origin = vPosition;
	}

#if 0
	if ( IsDepthOfFieldEnabled() || m_bForceCheapDoF )											// DoF (independent of AA jitter)
	{
		// Try to match the amount of blurriness from legacy fulcrum method
		const float flDoFHack = 0.0008f;
		Vector2D vDoFJitter = DepthOfFieldJitter( nSample ) * pCamera->GetAperture() * flDoFHack;

		float fov43 = pCamera->GetFOVx();
		float fHalfAngleRadians43 = DEG2RAD( 0.5f * fov43 );
		float t = tan( fHalfAngleRadians43 ) * (viewSetup.m_flAspectRatio / ( 4.0f / 3.0f ));

		float flZFocalWidth = t * pCamera->GetFocalDistance() * 2.0f;									// Width of Viewport at Focal plane
		Vector2D vFocalZJitter = vDoFJitter * flZFocalWidth;

		viewSetup.m_flOffCenterBottom += vDoFJitter.y;
		viewSetup.m_flOffCenterTop    += vDoFJitter.y;
		viewSetup.m_flOffCenterLeft   += vDoFJitter.x;
		viewSetup.m_flOffCenterRight  += vDoFJitter.x;

		viewSetup.origin = vPosition + vViewLeft * vFocalZJitter.x - vViewUp * vFocalZJitter.y * (1.0f / viewSetup.m_flAspectRatio);

		if ( !m_bForceCheapDoF )
		{
			Vector2D vAAJitter = g_vJitterTable32[nSample % 32];										// Jitter in addition to DoF offset
			const float fHalfPixelRadius = 0.6f;
			viewSetup.m_flOffCenterBottom += (vAAJitter.y / (float) viewSetup.height) * fHalfPixelRadius;
			viewSetup.m_flOffCenterTop    += (vAAJitter.y / (float) viewSetup.height) * fHalfPixelRadius;
			viewSetup.m_flOffCenterLeft   += (vAAJitter.x / (float) viewSetup.width)  * fHalfPixelRadius;
			viewSetup.m_flOffCenterRight  += (vAAJitter.x / (float) viewSetup.width)  * fHalfPixelRadius;
		}
	}
#endif

	MatrixAngles( matViewMatrix, viewSetup.angles );
}

int CReplayRenderer::GetMotionBlurQuality() const
{
	return m_RenderParams.m_Settings.m_nMotionBlurQuality;
}

int CReplayRenderer::GetDepthOfFieldQuality() const
{
	if ( !IsDepthOfFieldEnabled() )
		return 0;

	return MAX_DOF_QUALITY;
}

/*static*/ int CReplayRenderer::GetNumMotionBlurTimeSteps( int nQuality )
{
	Assert( nQuality >= 0 && nQuality <= MAX_MOTION_BLUR_QUALITY	);

	// Map {0, 1, 2, 3, 4} to {8, 16, 32, 64, 128 }
	return (int) pow(2.0f, nQuality+2 );
}

int CReplayRenderer::NumMotionBlurTimeSteps() const
{
	return ( IsMotionBlurEnabled() ) ? GetNumMotionBlurTimeSteps( GetMotionBlurQuality() ) : 1;
}

bool CReplayRenderer::IsMotionBlurEnabled() const
{
	return m_RenderParams.m_Settings.m_bMotionBlurEnabled;
}

bool CReplayRenderer::IsDepthOfFieldEnabled() const
{
	return false;
}

bool CReplayRenderer::IsAntialiasingEnabled() const
{
	return m_RenderParams.m_Settings.m_bAAEnabled;
}

void CReplayRenderer::ComputeSampleCounts( int *pNSamplesPerTimeStep, int *pNTotalSamples ) const
{
	*pNSamplesPerTimeStep = *pNTotalSamples = 1;

	if ( IsMotionBlurEnabled() )
	{
		*pNTotalSamples *= NumMotionBlurTimeSteps();
	}

	if ( IsDepthOfFieldEnabled() )
	{
		*pNTotalSamples *= s_DoFQualityToSamples[GetDepthOfFieldQuality()];
		*pNSamplesPerTimeStep *= s_DoFQualityToSamples[GetDepthOfFieldQuality()];
	}
}

float CReplayRenderer::GetFramerate() const
{
	return m_RenderParams.m_Settings.m_FPS.GetFPS();
}

double CReplayRenderer::GetShutterSpeed() const
{
	return 0.5 / m_RenderParams.m_Settings.m_FPS.GetFPS();
}

#ifdef TRACE_REPLAY_STATE_MACHINE
static int nFramesSent = 0;
#endif

void CReplayRenderer::CompositeAndLayoffFrame( int nFrame )
{
	#ifdef TRACE_REPLAY_STATE_MACHINE
		Msg("CompositeAndLayoffFrame( %3d ) TStep=%d  ...... ", nFrame, m_nTimeStep );
	#endif

	const int nMotionBlurTimeSteps = NumMotionBlurTimeSteps();
	bool bAppendToMovie = false;
	
	// Determine if this is a frame we handle audio on
	
	bool AudioTrigger = (m_nTimeStep == 0) && !m_bShutterClosed;
	SetAudioSyncFrame( AudioTrigger );

	// If we aren't doing motion blur, just render the frame and add it to the video
	if ( !IsMotionBlurEnabled() )
	{
		m_curSampleTime = DmeTime_t( nFrame, GetFramerate() );
		
		#ifdef TRACE_REPLAY_STATE_MACHINE
			Msg( "Rendering Frame at T=%.4f  ", m_curSampleTime.GetSeconds() );
		#endif
		
		RenderLayoffFrame( m_curSampleTime, 0, 1 );			// Just get one frame

		bAppendToMovie = true;
		goto render_to_video;		
	}

	// Shutter closed?
	if ( m_bShutterClosed )
	{
		m_nTimeStep++;
		
		#ifdef TRACE_REPLAY_STATE_MACHINE
			Msg("Shutter Closed...  TStep now %d", m_nTimeStep );
		#endif
		
		// If nMotionBlurTimeSteps subframes have passed, open the shutter for the next frame.
		if ( m_nTimeStep >= nMotionBlurTimeSteps )
		{
			Assert( m_nTimeStep == nMotionBlurTimeSteps );
			
			m_nTimeStep = 0;
			m_bShutterClosed = false;
			
			#ifdef TRACE_REPLAY_STATE_MACHINE
				Msg( ", Shutter OPENED, TStep=0");
			#endif
		}

		#ifdef TRACE_REPLAY_STATE_MACHINE
			ConVarRef HF( "host_framerate" );
			float frameRate = HF.GetFloat();
			Msg( ", DONE, ENgine FPS = %f\n", frameRate );
		#endif
	
		return;
	}

	// scope to avoid compiler warnings
	{
		// Shutter is open, accumulate sub-frames
		int nSamplesPerTimeStep = 1;
		int nNumTotalSamples = 1;
		ComputeSampleCounts( &nSamplesPerTimeStep, &nNumTotalSamples );

		double frameTime = DmeTime_t( nFrame, GetFramerate() ).GetSeconds();
		DmeTime_t timeStepSize( GetShutterSpeed() );
		DmeTime_t remainderStepSize( DmeTime_t( 1, GetFramerate() ) - timeStepSize );

		Assert( timeStepSize.GetSeconds() > 0.0 );

		DmeTime_t curSampleTime( frameTime );
		
		#ifdef TRACE_REPLAY_STATE_MACHINE
			Msg("FrameT=%.4lf   ", frameTime );
		#endif
		

		timeStepSize /= nMotionBlurTimeSteps;
		
		curSampleTime -= timeStepSize * ( nMotionBlurTimeSteps - 1 ) / 2.0f;

		// Loop through all samples for the current timestep, jittering the camera if antialiasing is enabled.
		
		#ifdef TRACE_REPLAY_STATE_MACHINE
			Msg(" Shutter's Open, Rendering %d Sub-Frames ", nSamplesPerTimeStep );
			Msg( "Frame %i: Laying off sub frame at time step %i \n", nFrame, m_nTimeStep );
		#endif 
		
		RenderLayoffFrame( m_curSampleTime, m_nCurSample++, nNumTotalSamples );

		++m_nTimeStep;
		m_curSampleTime += timeStepSize;

		// Catch the very last motionblur timestep and append to movie
		if ( m_nTimeStep == nMotionBlurTimeSteps )
		{
			#ifdef TRACE_REPLAY_STATE_MACHINE
				Msg( "  TStep=Max, Append=TRUE ... ");
			#endif 
			
			m_nTimeStep = 0;
			m_nCurSample = 0;
			m_curSampleTime = curSampleTime;
			m_bShutterClosed = true;			// Close or open the shutter for nMotionBlurTimeSteps subframes
			bAppendToMovie = true;				// Add a frame to the movie we've just closed the shutter
		}
	}

render_to_video:
	// Append the frame to the movie?
	if ( bAppendToMovie )
	{
		#ifdef TRACE_REPLAY_STATE_MACHINE
			Msg(" -- Appending Frame %d to Movie\n", nFramesSent );  nFramesSent++;
		#endif
		
		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
		pRenderContext->PushRenderTargetAndViewport( m_LayoffResult );

		// Add this frame to the movie
		LayoffFrame( nFrame );

		pRenderContext->PopRenderTargetAndViewport();
	}
	
	#ifdef TRACE_REPLAY_STATE_MACHINE
		Msg("\n");
	#endif
}


void CReplayRenderer::LayoffFrame( int nFrame )
{
	VPROF_BUDGET( "CReplayRenderer::LayoffFrame", VPROF_BUDGETGROUP_REPLAY );
	// FIXME: This is somewhat of a hack to get layoff working again
	// We're rendering into the full preview size, but stretching down to the actual size
	Rect_t srcRect;
	srcRect.x = 0;
	srcRect.y = 0;
	srcRect.width = m_RenderParams.m_Settings.m_nWidth;
	srcRect.height = m_RenderParams.m_Settings.m_nHeight;

	Rect_t dstRect;
	dstRect.x = 0;
	dstRect.y = 0;
	dstRect.width = m_RenderParams.m_Settings.m_nWidth;
	dstRect.height = m_RenderParams.m_Settings.m_nHeight;

	#ifdef TRACE_REPLAY_STATE_MACHINE
		Msg( "laying off movie frame %i\n", nFrame );
	#endif

	CMatRenderContextPtr pRenderContext( materials );
// 	pRenderContext->ReadPixelsAndStretch( &srcRect, &dstRect, (unsigned char*)m_pLayoffBuf, 
// 		IMAGE_FORMAT_BGRA8888, dstRect.width * ImageLoader::SizeInBytes( IMAGE_FORMAT_BGRA8888 ) );

	pRenderContext->ReadPixels( 0, 0, (int) m_RenderParams.m_Settings.m_nWidth, (int) m_RenderParams.m_Settings.m_nHeight, (unsigned char*)m_pLayoffBuf, IMAGE_FORMAT_BGRA8888 );

	static ConVarRef mat_queue_mode( "mat_queue_mode" );

	// Encode the frame
#ifdef REPLAY_RECORDING_ENABLE
	if ( m_RenderParams.m_bExportRaw )
	{
		CUtlBuffer bufOut;
        if ( TGAWriter::WriteToBuffer( (unsigned char *)m_pLayoffBuf, bufOut, m_RenderParams.m_Settings.m_nWidth,
			m_RenderParams.m_Settings.m_nHeight, IMAGE_FORMAT_BGRA8888, IMAGE_FORMAT_RGB888 ) )
		{
			// Format filename and write the TGA
			CFmtStr fmtFilename(
				"%sFrame_%04i.tga",
				m_fmtTgaRenderDirName.Access(),
				m_iTgaFrame++
			);

	        if ( !g_pFullFileSystem->WriteFile( fmtFilename.Access(), NULL, bufOut ) )
	        {
	            Warning( "Couldn't write bitmap data snapshot to file %s.\n", fmtFilename.Access() );
	        }
		}
	}
	else if ( m_pMovieMaker )
	{
		// can't run in any other mode	
		Assert( mat_queue_mode.GetInt() == 0 );
		VPROF_BUDGET( "CReplayRenderer::LayoffFrame - AppendVideoFrame", VPROF_BUDGETGROUP_REPLAY );
		m_pMovieMaker->AppendVideoFrame( m_pLayoffBuf );
	}
#endif
}

void CReplayRenderer::GetViewSetup( CViewSetup &viewsetup )
{
	extern ConVar v_viewmodel_fov;

	viewsetup = *view->GetPlayerViewSetup();

	// HACK: Override the view - this will keep the view from popping if the user toggles the render preview checkbox.
	ReplayCamera()->CalcView( viewsetup.origin, viewsetup.angles, viewsetup.fov );
	viewsetup .fovViewmodel = ScaleFOVByWidthRatio( v_viewmodel_fov.GetFloat(), viewsetup.m_flAspectRatio / ( 4.0f / 3.0f ) );
}

void CReplayRenderer::RenderLayoffFrame( DmeTime_t time, int nCurSample, int nNumTotalSamples )
{
	CViewSetup viewSetup;
	GetViewSetup( viewSetup );

	int x=0, y=0, w=m_RenderParams.m_Settings.m_nWidth, h=m_RenderParams.m_Settings.m_nHeight;

	// FIXME: Using the preview size here is something of a hack
	// to get layoff working again. We're actually going to stretch down from the preview size to layoff size
	// during frame capture
	float fTonemapScale = 0.28f;
	BeginRenderingSample( nCurSample, x, y, w, h, fTonemapScale);

	// Initialize view setup for this sample
	SetupSampleView( 0, 0, w, h, nCurSample, viewSetup );
	
	const int flags = RENDERVIEW_DRAWVIEWMODEL;

	// Tell the engine to tell the client to render the view (sans viewmodel)
	view->RenderView( viewSetup, VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH, flags );

	// Resolve the accumulation buffer samples for display this frame
	float fBloomScale = 0.28f;
	bool bRenderFinalFrame = nCurSample == ( nNumTotalSamples - 1 );
	ResolveSamples( nCurSample, time, 0, 0, w, h, bRenderFinalFrame, fBloomScale );

	// Pop the target
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->PopRenderTargetAndViewport();
}

void CReplayRenderer::EndRendering()
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->PopRenderTargetAndViewport();
}

void CReplayRenderer::ClearToBlack( CTextureReference &buf, int x, int y, int nWidth, int nHeight )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	// Bind the resolving material
	AccumParams_t accParms = { m_AccumBuffSample, m_AccumBuffSample, 0.0f, true }; // true to clear to black
	pRenderContext->Bind( m_FourSampleResolveMatRef, &accParms );

	// Render black quad to the layoff result
	pRenderContext->PushRenderTargetAndViewport( buf, x, y, nWidth, nHeight );
		DrawResolvingQuad( nWidth, nHeight );
	pRenderContext->PopRenderTargetAndViewport();
}

void CReplayRenderer::RenderVideo()
{
#if _DEBUG
	static ConVarRef replay_fake_render( "replay_fake_render" );
	if ( replay_fake_render.IsValid() && replay_fake_render.GetBool() )
		return;
#endif

	if ( !engine->IsInGame() )
		return;

	if ( !m_LayoffResult.IsValid() )
		return;

	CompositeAndLayoffFrame( m_nFrame++ );
}

void CReplayRenderer::RenderAudio( unsigned char *pBuffer, int nSize, int nNumSamples )
{
#ifdef REPLAY_RECORDING_ENABLE
	if ( m_RenderParams.m_bExportRaw )
	{
		g_pEngineClientReplay->Wave_AppendTmpFile( TMP_WAVE_FILENAME, pBuffer, nNumSamples );
	}
	else if ( m_pMovieMaker )
	{
		m_pMovieMaker->AppendAudioSamples( pBuffer, (size_t)nSize );
	}	
#endif
}

//-----------------------------------------------------------------------------

#endif
