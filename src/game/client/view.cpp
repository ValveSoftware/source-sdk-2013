//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "view.h"
#include "iviewrender.h"
#include "iviewrender_beams.h"
#include "view_shared.h"
#include "ivieweffects.h"
#include "iinput.h"
#include "iclientmode.h"
#include "prediction.h"
#include "viewrender.h"
#include "c_te_legacytempents.h"
#include "cl_mat_stub.h"
#include "tier0/vprof.h"
#include "iclientvehicle.h"
#include "engine/IEngineTrace.h"
#include "mathlib/vmatrix.h"
#include "rendertexture.h"
#include "c_world.h"
#include <KeyValues.h>
#include "igameevents.h"
#include "smoke_fog_overlay.h"
#include "bitmap/tgawriter.h"
#include "hltvcamera.h"
#if defined( REPLAY_ENABLED )
#include "replay/replaycamera.h"
#include "replay/replay_screenshot.h"
#endif
#include "input.h"
#include "filesystem.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/materialsystem_config.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "toolframework_client.h"
#include "tier0/icommandline.h"
#include "ienginevgui.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include "ScreenSpaceEffects.h"
#include "sourcevr/isourcevirtualreality.h"
#include "client_virtualreality.h"

#if defined( REPLAY_ENABLED )
#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#endif

#if defined( HL2_CLIENT_DLL ) || defined( CSTRIKE_DLL ) || defined( TF_CLIENT_DLL )
#define USE_MONITORS
#endif

#ifdef PORTAL
#include "c_prop_portal.h" //portal surface rendering functions
#endif

	
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
		  
void ToolFramework_AdjustEngineViewport( int& x, int& y, int& width, int& height );
bool ToolFramework_SetupEngineView( Vector &origin, QAngle &angles, float &fov );
bool ToolFramework_SetupEngineMicrophone( Vector &origin, QAngle &angles );


extern ConVar default_fov;
extern bool g_bRenderingScreenshot;

#if !defined( _X360 )
#define SAVEGAME_SCREENSHOT_WIDTH	180
#define SAVEGAME_SCREENSHOT_HEIGHT	100
#else
#define SAVEGAME_SCREENSHOT_WIDTH	128
#define SAVEGAME_SCREENSHOT_HEIGHT	128
#endif

#ifndef _XBOX
extern ConVar sensitivity;
#endif

ConVar zoom_sensitivity_ratio( "zoom_sensitivity_ratio", "1.0", FCVAR_ARCHIVE, "Additional mouse sensitivity scale factor applied when FOV is zoomed in." );

CViewRender g_DefaultViewRender;
IViewRender *view = NULL;	// set in cldll_client_init.cpp if no mod creates their own

bool g_bRenderingCameraView = false;


// These are the vectors for the "main" view - the one the player is looking down.
// For stereo views, they are the vectors for the middle eye.
static Vector g_vecRenderOrigin(0,0,0);
static QAngle g_vecRenderAngles(0,0,0);
static Vector g_vecPrevRenderOrigin(0,0,0);	// Last frame's render origin
static QAngle g_vecPrevRenderAngles(0,0,0); // Last frame's render angles
static Vector g_vecVForward(0,0,0), g_vecVRight(0,0,0), g_vecVUp(0,0,0);
static VMatrix g_matCamInverse;

extern ConVar cl_forwardspeed;

static ConVar v_centermove( "v_centermove", "0.15");
static ConVar v_centerspeed( "v_centerspeed","500" );

#ifdef TF_CLIENT_DLL
// 54 degrees approximates a 35mm camera - we determined that this makes the viewmodels
// and motions look the most natural.
ConVar v_viewmodel_fov( "viewmodel_fov", "54", FCVAR_ARCHIVE, "Sets the field-of-view for the viewmodel.", true, 0.1, true, 179.9, true, 54, true, 70, NULL );
#else
ConVar v_viewmodel_fov( "viewmodel_fov", "54", FCVAR_CHEAT, "Sets the field-of-view for the viewmodel.", true, 0.1, true, 179.9 );
#endif
ConVar mat_viewportscale( "mat_viewportscale", "1.0", FCVAR_ARCHIVE, "Scale down the main viewport (to reduce GPU impact on CPU profiling)", true, (1.0f / 640.0f), true, 1.0f );
ConVar mat_viewportupscale( "mat_viewportupscale", "1", FCVAR_ARCHIVE, "Scale the viewport back up" );
ConVar cl_leveloverview( "cl_leveloverview", "0", FCVAR_CHEAT );

static ConVar r_mapextents( "r_mapextents", "16384", FCVAR_CHEAT, 
						   "Set the max dimension for the map.  This determines the far clipping plane" );

// UNDONE: Delete this or move to the material system?
ConVar	gl_clear( "gl_clear", "0");
ConVar	gl_clear_randomcolor( "gl_clear_randomcolor", "0", FCVAR_CHEAT, "Clear the back buffer to random colors every frame. Helps spot open seams in geometry." );

static ConVar r_farz( "r_farz", "-1", FCVAR_CHEAT, "Override the far clipping plane. -1 means to use the value in env_fog_controller." );
static ConVar cl_demoviewoverride( "cl_demoviewoverride", "0", 0, "Override view during demo playback" );


void SoftwareCursorChangedCB( IConVar *pVar, const char *pOldValue, float fOldValue )
{
	ConVar *pConVar = (ConVar *)pVar;
	vgui::surface()->SetSoftwareCursor( pConVar->GetBool() || UseVR() );
}
static ConVar cl_software_cursor ( "cl_software_cursor", "0", FCVAR_ARCHIVE, "Switches the game to use a larger software cursor instead of the normal OS cursor", SoftwareCursorChangedCB );


static Vector s_DemoView;
static QAngle s_DemoAngle;

static void CalcDemoViewOverride( Vector &origin, QAngle &angles )
{
	engine->SetViewAngles( s_DemoAngle );

	input->ExtraMouseSample( gpGlobals->absoluteframetime, true );

	engine->GetViewAngles( s_DemoAngle );

	Vector forward, right, up;

	AngleVectors( s_DemoAngle, &forward, &right, &up );

	float speed = gpGlobals->absoluteframetime * cl_demoviewoverride.GetFloat() * 320;
	
	s_DemoView += speed * input->KeyState (&in_forward) * forward  ;
	s_DemoView -= speed * input->KeyState (&in_back) * forward ;

	s_DemoView += speed * input->KeyState (&in_moveright) * right ;
	s_DemoView -= speed * input->KeyState (&in_moveleft) * right ;

	origin = s_DemoView;
	angles = s_DemoAngle;
}



// Selects the relevant member variable to update. You could do it manually, but...
// We always set up the MONO eye, even when doing stereo, and it's set up to be mid-way between the left and right,
// so if you don't really care about L/R (e.g. culling, sound, etc), just use MONO.
CViewSetup &CViewRender::GetView(StereoEye_t eEye)
{
	if ( eEye == STEREO_EYE_MONO )
    {
		return m_View;
    }
	else if ( eEye == STEREO_EYE_RIGHT )
    {
        return m_ViewRight;
    }
	else
    {
        Assert ( eEye == STEREO_EYE_LEFT );
		return m_ViewLeft;
    }
}

const CViewSetup &CViewRender::GetView(StereoEye_t eEye) const
{
    return (const_cast<CViewRender*>(this))->GetView ( eEye );
}


//-----------------------------------------------------------------------------
// Accessors to return the main view (where the player's looking)
//-----------------------------------------------------------------------------
const Vector &MainViewOrigin()
{
	return g_vecRenderOrigin;
}

const QAngle &MainViewAngles()
{
	return g_vecRenderAngles;
}

const Vector &MainViewForward()
{
	return g_vecVForward;
}

const Vector &MainViewRight()
{
	return g_vecVRight;
}

const Vector &MainViewUp()
{
	return g_vecVUp;
}

const VMatrix &MainWorldToViewMatrix()
{
	return g_matCamInverse;
}

const Vector &PrevMainViewOrigin()
{
	return g_vecPrevRenderOrigin;
}

const QAngle &PrevMainViewAngles()
{
	return g_vecPrevRenderAngles;
}

//-----------------------------------------------------------------------------
// Compute the world->camera transform
//-----------------------------------------------------------------------------
void ComputeCameraVariables( const Vector &vecOrigin, const QAngle &vecAngles, 
	Vector *pVecForward, Vector *pVecRight, Vector *pVecUp, VMatrix *pMatCamInverse )
{
	// Compute view bases
	AngleVectors( vecAngles, pVecForward, pVecRight, pVecUp );

	for (int i = 0; i < 3; ++i)
	{
		(*pMatCamInverse)[0][i] = (*pVecRight)[i];	
		(*pMatCamInverse)[1][i] = (*pVecUp)[i];	
		(*pMatCamInverse)[2][i] = -(*pVecForward)[i];	
		(*pMatCamInverse)[3][i] = 0.0F;
	}
	(*pMatCamInverse)[0][3] = -DotProduct( *pVecRight, vecOrigin );
	(*pMatCamInverse)[1][3] = -DotProduct( *pVecUp, vecOrigin );
	(*pMatCamInverse)[2][3] =  DotProduct( *pVecForward, vecOrigin );
	(*pMatCamInverse)[3][3] = 1.0F;
}


bool R_CullSphere(
	VPlane const *pPlanes,
	int nPlanes,
	Vector const *pCenter,
	float radius)
{
	for(int i=0; i < nPlanes; i++)
		if(pPlanes[i].DistTo(*pCenter) < -radius)
			return true;
	
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void StartPitchDrift( void )
{
	view->StartPitchDrift();
}

static ConCommand centerview( "centerview", StartPitchDrift );

extern ConVar default_fov;



//-----------------------------------------------------------------------------
// Purpose: Initializes all view systems
//-----------------------------------------------------------------------------
void CViewRender::Init( void )
{
	memset( &m_PitchDrift, 0, sizeof( m_PitchDrift ) );

	m_bDrawOverlay = false;

	m_pDrawEntities		= cvar->FindVar( "r_drawentities" );
	m_pDrawBrushModels	= cvar->FindVar( "r_drawbrushmodels" );

	beams->InitBeams();
	tempents->Init();

	m_TranslucentSingleColor.Init( "debug/debugtranslucentsinglecolor", TEXTURE_GROUP_OTHER );
	m_ModulateSingleColor.Init( "engine/modulatesinglecolor", TEXTURE_GROUP_OTHER );
	
	extern CMaterialReference g_material_WriteZ;
	g_material_WriteZ.Init( "engine/writez", TEXTURE_GROUP_OTHER );

	// FIXME:  
	QAngle angles;
	engine->GetViewAngles( angles );
	AngleVectors( angles, &m_vecLastFacing );

#if defined( REPLAY_ENABLED )
	m_pReplayScreenshotTaker = NULL;
#endif

#if defined( CSTRIKE_DLL )
	m_flLastFOV = default_fov.GetFloat();
#endif

}

//-----------------------------------------------------------------------------
// Purpose: Called once per level change
//-----------------------------------------------------------------------------
void CViewRender::LevelInit( void )
{
	beams->ClearBeams();
	tempents->Clear();

	m_BuildWorldListsNumber = 0;
	m_BuildRenderableListsNumber = 0;

	for( int i=0; i < STEREO_EYE_MAX; i++ )
	{
		m_rbTakeFreezeFrame[ i ] = false;
	}
	m_flFreezeFrameUntil = 0;

	// Clear our overlay materials
	m_ScreenOverlayMaterial.Init( NULL );

	// Init all IScreenSpaceEffects
	g_pScreenSpaceEffects->InitScreenSpaceEffects( );
}

//-----------------------------------------------------------------------------
// Purpose: Called at shutdown
//-----------------------------------------------------------------------------
void CViewRender::Shutdown( void )
{
	m_TranslucentSingleColor.Shutdown( );
	m_ModulateSingleColor.Shutdown( );
	m_ScreenOverlayMaterial.Shutdown();
	m_UnderWaterOverlayMaterial.Shutdown();
	beams->ShutdownBeams();
	tempents->Shutdown();
}


//-----------------------------------------------------------------------------
// Returns the worldlists build number
//-----------------------------------------------------------------------------

int CViewRender::BuildWorldListsNumber( void ) const
{
	return m_BuildWorldListsNumber;
}

//-----------------------------------------------------------------------------
// Purpose: Start moving pitch toward ideal
//-----------------------------------------------------------------------------
void CViewRender::StartPitchDrift (void)
{
	if ( m_PitchDrift.laststop == gpGlobals->curtime )
	{
		// Something else is blocking the drift.
		return;		
	}

	if ( m_PitchDrift.nodrift || !m_PitchDrift.pitchvel )
	{
		m_PitchDrift.pitchvel	= v_centerspeed.GetFloat();
		m_PitchDrift.nodrift	= false;
		m_PitchDrift.driftmove	= 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::StopPitchDrift (void)
{
	m_PitchDrift.laststop	= gpGlobals->curtime;
	m_PitchDrift.nodrift	= true;
	m_PitchDrift.pitchvel	= 0;
}

//-----------------------------------------------------------------------------
// Purpose: Moves the client pitch angle towards cl.idealpitch sent by the server.
// If the user is adjusting pitch manually, either with lookup/lookdown,
//   mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
//-----------------------------------------------------------------------------
void CViewRender::DriftPitch (void)
{
	float		delta, move;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

#if defined( REPLAY_ENABLED )
	if ( engine->IsHLTV() || g_pEngineClientReplay->IsPlayingReplayDemo() || ( player->GetGroundEntity() == NULL ) || engine->IsPlayingDemo() )
#else
	if ( engine->IsHLTV() || ( player->GetGroundEntity() == NULL ) || engine->IsPlayingDemo() )
#endif
	{
		m_PitchDrift.driftmove = 0;
		m_PitchDrift.pitchvel = 0;
		return;
	}

	// Don't count small mouse motion
	if ( m_PitchDrift.nodrift )
	{
		if ( fabs( input->GetLastForwardMove() ) < cl_forwardspeed.GetFloat() )
		{
			m_PitchDrift.driftmove = 0;
		}
		else
		{
			m_PitchDrift.driftmove += gpGlobals->frametime;
		}
	
		if ( m_PitchDrift.driftmove > v_centermove.GetFloat() )
		{
			StartPitchDrift ();
		}
		return;
	}
	
	// How far off are we
	delta = prediction->GetIdealPitch() - player->GetAbsAngles()[ PITCH ];
	if ( !delta )
	{
		m_PitchDrift.pitchvel = 0;
		return;
	}

	// Determine movement amount
	move = gpGlobals->frametime * m_PitchDrift.pitchvel;
	// Accelerate
	m_PitchDrift.pitchvel += gpGlobals->frametime * v_centerspeed.GetFloat();
	
	// Move predicted pitch appropriately
	if (delta > 0)
	{
		if ( move > delta )
		{
			m_PitchDrift.pitchvel = 0;
			move = delta;
		}
		player->SetLocalAngles( player->GetLocalAngles() + QAngle( move, 0, 0 ) );
	}
	else if ( delta < 0 )
	{
		if ( move > -delta )
		{
			m_PitchDrift.pitchvel = 0;
			move = -delta;
		}
		player->SetLocalAngles( player->GetLocalAngles() - QAngle( move, 0, 0 ) );
	}
}



StereoEye_t		CViewRender::GetFirstEye() const
{
	if( UseVR() )
		return STEREO_EYE_LEFT;
	else
		return STEREO_EYE_MONO;
}

StereoEye_t		CViewRender::GetLastEye() const
{
	if( UseVR() )
		return STEREO_EYE_RIGHT;
	else
		return STEREO_EYE_MONO;
}




// This is called by cdll_client_int to setup view model origins. This has to be done before
// simulation so entities can access attachment points on view models during simulation.
void CViewRender::OnRenderStart()
{
	VPROF_("CViewRender::OnRenderStart", 2, VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, false, 0);

    SetUpViews();

	// Adjust mouse sensitivity based upon the current FOV
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( player )
	{
		default_fov.SetValue( player->m_iDefaultFOV );

		//Update our FOV, including any zooms going on
		int iDefaultFOV = default_fov.GetInt();
		int	localFOV	= player->GetFOV();
		int min_fov		= player->GetMinFOV();

		// Don't let it go too low
		localFOV = MAX( min_fov, localFOV );

		gHUD.m_flFOVSensitivityAdjust = 1.0f;
#ifndef _XBOX
		if ( gHUD.m_flMouseSensitivityFactor )
		{
			gHUD.m_flMouseSensitivity = sensitivity.GetFloat() * gHUD.m_flMouseSensitivityFactor;
		}
		else
#endif
		{
			// No override, don't use huge sensitivity
			if ( localFOV == iDefaultFOV )
			{
#ifndef _XBOX
				// reset to saved sensitivity
				gHUD.m_flMouseSensitivity = 0;
#endif
			}
			else
			{  
				// Set a new sensitivity that is proportional to the change from the FOV default and scaled
				//  by a separate compensating factor
				if ( iDefaultFOV == 0 )
				{
					Assert(0); // would divide by zero, something is broken with iDefatulFOV
					iDefaultFOV = 1;
				}
				gHUD.m_flFOVSensitivityAdjust = 
					((float)localFOV / (float)iDefaultFOV) * // linear fov downscale
					zoom_sensitivity_ratio.GetFloat(); // sensitivity scale factor
#ifndef _XBOX
				gHUD.m_flMouseSensitivity = gHUD.m_flFOVSensitivityAdjust * sensitivity.GetFloat(); // regular sensitivity
#endif
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const CViewSetup
//-----------------------------------------------------------------------------
const CViewSetup *CViewRender::GetViewSetup( void ) const
{
	return &m_CurrentView;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const CViewSetup
//-----------------------------------------------------------------------------
const CViewSetup *CViewRender::GetPlayerViewSetup( void ) const
{   
    const CViewSetup &viewEye = GetView ( STEREO_EYE_MONO );
    return &viewEye;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::DisableVis( void )
{
	m_bForceNoVis = true;
}

#ifdef DBGFLAG_ASSERT
static Vector s_DbgSetupOrigin;
static QAngle s_DbgSetupAngles;
#endif

//-----------------------------------------------------------------------------
// Gets znear + zfar
//-----------------------------------------------------------------------------
float CViewRender::GetZNear()
{
	return VIEW_NEARZ;
}

float CViewRender::GetZFar()
{
	// Initialize view structure with default values
	float farZ;
	if ( r_farz.GetFloat() < 1 )
	{
		// Use the far Z from the map's parameters.
		farZ = r_mapextents.GetFloat() * 1.73205080757f;
		
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if( pPlayer && pPlayer->GetFogParams() )
		{
			if ( pPlayer->GetFogParams()->farz > 0 )
			{
				farZ = pPlayer->GetFogParams()->farz;
			}
		}
	}
	else
	{
		farZ = r_farz.GetFloat();
	}

	return farZ;
}


//-----------------------------------------------------------------------------
// Sets up the view parameters
//-----------------------------------------------------------------------------
void CViewRender::SetUpViews()
{
	VPROF("CViewRender::SetUpViews");

	// Initialize view structure with default values
	float farZ = GetZFar();

    // Set up the mono/middle view.
    CViewSetup &viewEye = m_View;

	viewEye.zFar				= farZ;
	viewEye.zFarViewmodel	    = farZ;
	// UNDONE: Make this farther out? 
	//  closest point of approach seems to be view center to top of crouched box
	viewEye.zNear			    = GetZNear();
	viewEye.zNearViewmodel	    = 1;
	viewEye.fov				= default_fov.GetFloat();

	viewEye.m_bOrtho			= false;
	viewEye.m_bViewToProjectionOverride = false;
	viewEye.m_eStereoEye		= STEREO_EYE_MONO;

	// Enable spatial partition access to edicts
	::partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	// You in-view weapon aim.
	bool bCalcViewModelView = false;
	Vector ViewModelOrigin;
	QAngle ViewModelAngles;

	if ( engine->IsHLTV() )
	{
		HLTVCamera()->CalcView( viewEye.origin, viewEye.angles, viewEye.fov );
	}
#if defined( REPLAY_ENABLED )
	else if ( g_pEngineClientReplay->IsPlayingReplayDemo() )
	{
		ReplayCamera()->CalcView( viewEye.origin, viewEye.angles, viewEye.fov );
	}
#endif
	else
	{
		// FIXME: Are there multiple views? If so, then what?
		// FIXME: What happens when there's no player?
		if (pPlayer)
		{
			pPlayer->CalcView( viewEye.origin, viewEye.angles, viewEye.zNear, viewEye.zFar, viewEye.fov );

			// If we are looking through another entities eyes, then override the angles/origin for view
			int viewentity = render->GetViewEntity();

			if ( !g_nKillCamMode && (pPlayer->entindex() != viewentity) )
			{
				C_BaseEntity *ve = cl_entitylist->GetEnt( viewentity );
				if ( ve )
				{
					VectorCopy( ve->GetAbsOrigin(), viewEye.origin );
					VectorCopy( ve->GetAbsAngles(), viewEye.angles );
				}
			}

			// There is a viewmodel.
			bCalcViewModelView = true;
			ViewModelOrigin = viewEye.origin;
			ViewModelAngles = viewEye.angles;
		}
		else
		{
			viewEye.origin.Init();
			viewEye.angles.Init();
		}

		// Even if the engine is paused need to override the view
		// for keeping the camera control during pause.
		g_pClientMode->OverrideView( &viewEye );
	}

	// give the toolsystem a chance to override the view
	ToolFramework_SetupEngineView( viewEye.origin, viewEye.angles, viewEye.fov );

	if ( engine->IsPlayingDemo() )
	{
		if ( cl_demoviewoverride.GetFloat() > 0.0f )
		{
			// Retreive view angles from engine ( could have been set in IN_AdjustAngles above )
			CalcDemoViewOverride( viewEye.origin, viewEye.angles );
		}
		else
		{
			s_DemoView = viewEye.origin;
			s_DemoAngle = viewEye.angles;
		}
	}

	//Find the offset our current FOV is from the default value
	float fDefaultFov = default_fov.GetFloat();
	float flFOVOffset = fDefaultFov - viewEye.fov;

	//Adjust the viewmodel's FOV to move with any FOV offsets on the viewer's end
	viewEye.fovViewmodel = g_pClientMode->GetViewModelFOV() - flFOVOffset;

	if ( UseVR() )
	{
		// Let the headtracking read the status of the HMD, etc.
		// This call can go almost anywhere, but it needs to know the player FOV for sniper weapon zoom, etc
		if ( flFOVOffset == 0.0f )
		{
			g_ClientVirtualReality.ProcessCurrentTrackingState ( 0.0f );
		}
		else
		{
			g_ClientVirtualReality.ProcessCurrentTrackingState ( viewEye.fov );
		}

		HeadtrackMovementMode_t hmmOverrideMode = g_pClientMode->ShouldOverrideHeadtrackControl();
		g_ClientVirtualReality.OverrideView( &m_View, &ViewModelOrigin, &ViewModelAngles, hmmOverrideMode );

		// left and right stereo views should default to being the same as the mono/middle view
		m_ViewLeft = m_View;
		m_ViewRight = m_View;
		m_ViewLeft.m_eStereoEye = STEREO_EYE_LEFT;
		m_ViewRight.m_eStereoEye = STEREO_EYE_RIGHT;

		g_ClientVirtualReality.OverrideStereoView( &m_View, &m_ViewLeft, &m_ViewRight );
	}
	else
	{
		// left and right stereo views should default to being the same as the mono/middle view
		m_ViewLeft = m_View;
		m_ViewRight = m_View;
		m_ViewLeft.m_eStereoEye = STEREO_EYE_LEFT;
		m_ViewRight.m_eStereoEye = STEREO_EYE_RIGHT;
	}

	if ( bCalcViewModelView )
	{
		Assert ( pPlayer != NULL );
		pPlayer->CalcViewModelView ( ViewModelOrigin, ViewModelAngles );
	}

	// Disable spatial partition access
	::partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );

	// Enable access to all model bones
	C_BaseAnimating::PopBoneAccess( "OnRenderStart->CViewRender::SetUpView" ); // pops the (true, false) bone access set in OnRenderStart
	C_BaseAnimating::PushAllowBoneAccess( true, true, "CViewRender::SetUpView->OnRenderEnd" ); // pop is in OnRenderEnd()

	// Compute the world->main camera transform
    // This is only done for the main "middle-eye" view, not for the various other views.
	ComputeCameraVariables( viewEye.origin, viewEye.angles,
		&g_vecVForward, &g_vecVRight, &g_vecVUp, &g_matCamInverse );

	// set up the hearing origin...
	AudioState_t audioState;
	audioState.m_Origin = viewEye.origin;
	audioState.m_Angles = viewEye.angles;
	audioState.m_bIsUnderwater = pPlayer && pPlayer->AudioStateIsUnderwater( viewEye.origin );

	ToolFramework_SetupAudioState( audioState );

    // TomF: I wonder when the audio tools modify this, if ever...
    Assert ( viewEye.origin == audioState.m_Origin );
    Assert ( viewEye.angles == audioState.m_Angles );
	viewEye.origin = audioState.m_Origin;
	viewEye.angles = audioState.m_Angles;

	engine->SetAudioState( audioState );

	g_vecPrevRenderOrigin = g_vecRenderOrigin;
	g_vecPrevRenderAngles = g_vecRenderAngles;
	g_vecRenderOrigin = viewEye.origin;
	g_vecRenderAngles = viewEye.angles;

#ifdef DBGFLAG_ASSERT
	s_DbgSetupOrigin = viewEye.origin;
	s_DbgSetupAngles = viewEye.angles;
#endif
}




void CViewRender::WriteSaveGameScreenshotOfSize( const char *pFilename, int width, int height, bool bCreatePowerOf2Padded/*=false*/,
												 bool bWriteVTF/*=false*/ )
{
#ifndef _X360
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();

	g_bRenderingScreenshot = true;

	// Push back buffer on the stack with small viewport
	pRenderContext->PushRenderTargetAndViewport( NULL, 0, 0, width, height );

	// render out to the backbuffer
    CViewSetup viewSetup = GetView ( STEREO_EYE_MONO );
	viewSetup.x = 0;
	viewSetup.y = 0;
	viewSetup.width = width;
	viewSetup.height = height;
	viewSetup.fov = ScaleFOVByWidthRatio( viewSetup.fov, ( (float)width / (float)height ) / ( 4.0f / 3.0f ) );
	viewSetup.m_bRenderToSubrectOfLargerScreen = true;

	// draw out the scene
	// Don't draw the HUD or the viewmodel
	RenderView( viewSetup, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, 0 );

	// get the data from the backbuffer and save to disk
	// bitmap bits
	unsigned char *pImage = ( unsigned char * )malloc( width * height * 3 );

	// Get Bits from the material system
	pRenderContext->ReadPixels( 0, 0, width, height, pImage, IMAGE_FORMAT_RGB888 );

	// Some stuff to be setup dependent on padded vs. not padded
	int nSrcWidth, nSrcHeight;
	unsigned char *pSrcImage;

	// Create a padded version if necessary
	unsigned char *pPaddedImage = NULL;
	if ( bCreatePowerOf2Padded )
	{
		// Setup dimensions as needed
		int nPaddedWidth = SmallestPowerOfTwoGreaterOrEqual( width );
		int nPaddedHeight = SmallestPowerOfTwoGreaterOrEqual( height );

		// Allocate
		int nPaddedImageSize = nPaddedWidth * nPaddedHeight * 3;
		pPaddedImage = ( unsigned char * )malloc( nPaddedImageSize );
		
		// Zero out the entire thing
		V_memset( pPaddedImage, 255, nPaddedImageSize );

		// Copy over each row individually
		for ( int nRow = 0; nRow < height; ++nRow )
		{
			unsigned char *pDst = pPaddedImage + 3 * ( nRow * nPaddedWidth );
			const unsigned char *pSrc = pImage + 3 * ( nRow * width );
			V_memcpy( pDst, pSrc, 3 * width );
		}

		// Setup source data
		nSrcWidth = nPaddedWidth;
		nSrcHeight = nPaddedHeight;
		pSrcImage = pPaddedImage;
	}
	else
	{
		// Use non-padded info
		nSrcWidth = width;
		nSrcHeight = height;
		pSrcImage = pImage;
	}

	// allocate a buffer to write the tga into
	CUtlBuffer buffer;

	bool bWriteResult;
	if ( bWriteVTF )
	{
		// Create and initialize a VTF texture
		IVTFTexture *pVTFTexture = CreateVTFTexture();
		const int nFlags = TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOLOD | TEXTUREFLAGS_SRGB;
		if ( pVTFTexture->Init( nSrcWidth, nSrcHeight, 1, IMAGE_FORMAT_RGB888, nFlags, 1, 1 ) )
		{
			// Copy the image data over to the VTF
			unsigned char *pDestBits = pVTFTexture->ImageData();
			int nDstSize = nSrcWidth * nSrcHeight * 3;
			V_memcpy( pDestBits, pSrcImage, nDstSize );

			// Allocate output buffer
			int iMaxVTFSize = 1024 + ( nSrcWidth * nSrcHeight * 3 );
			void *pVTF = malloc( iMaxVTFSize );
			buffer.SetExternalBuffer( pVTF, iMaxVTFSize, 0 );

			// Serialize to the buffer
			bWriteResult = pVTFTexture->Serialize( buffer );
		
			// Free the VTF texture
			DestroyVTFTexture( pVTFTexture );
		}
		else
		{
			bWriteResult = false;
		}
	}
	else
	{
		// Write TGA format to buffer
		int iMaxTGASize = 1024 + ( nSrcWidth * nSrcHeight * 4 );
		void *pTGA = malloc( iMaxTGASize );
		buffer.SetExternalBuffer( pTGA, iMaxTGASize, 0 );

		bWriteResult = TGAWriter::WriteToBuffer( pSrcImage, buffer, nSrcWidth, nSrcHeight, IMAGE_FORMAT_RGB888, IMAGE_FORMAT_RGB888 );
	}

	if ( !bWriteResult )
	{
		Error( "Couldn't write bitmap data snapshot.\n" );
	}
	
	free( pImage );
	free( pPaddedImage );

	// async write to disk (this will take ownership of the memory)
	char szPathedFileName[_MAX_PATH];
	Q_snprintf( szPathedFileName, sizeof(szPathedFileName), "//MOD/%s", pFilename );

	filesystem->AsyncWrite( szPathedFileName, buffer.Base(), buffer.TellPut(), true );

	// restore our previous state
	pRenderContext->PopRenderTargetAndViewport();
	
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	g_bRenderingScreenshot = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: takes a screenshot for the replay system
//-----------------------------------------------------------------------------
void CViewRender::WriteReplayScreenshot( WriteReplayScreenshotParams_t &params )
{
#if defined( REPLAY_ENABLED )
	if ( !m_pReplayScreenshotTaker )
		return;

	m_pReplayScreenshotTaker->TakeScreenshot( params );
#endif
}

void CViewRender::UpdateReplayScreenshotCache()
{
#if defined( REPLAY_ENABLED )
	// Delete the old one
	delete m_pReplayScreenshotTaker;

	// Create a new one
	m_pReplayScreenshotTaker = new CReplayScreenshotTaker( this, GetView ( STEREO_EYE_MONO ) );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: takes a screenshot of the save game
//-----------------------------------------------------------------------------
void CViewRender::WriteSaveGameScreenshot( const char *pFilename )
{
	WriteSaveGameScreenshotOfSize( pFilename, SAVEGAME_SCREENSHOT_WIDTH, SAVEGAME_SCREENSHOT_HEIGHT );
}


float ScaleFOVByWidthRatio( float fovDegrees, float ratio )
{
	float halfAngleRadians = fovDegrees * ( 0.5f * M_PI / 180.0f );
	float t = tan( halfAngleRadians );
	t *= ratio;
	float retDegrees = ( 180.0f / M_PI ) * atan( t );
	return retDegrees * 2.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Sets view parameters for level overview mode
// Input  : *rect - 
//-----------------------------------------------------------------------------
void CViewRender::SetUpOverView()
{
	static int oldCRC = 0;

    CViewSetup &viewEye = GetView ( STEREO_EYE_MONO );

	viewEye.m_bOrtho = true;

	float aspect = (float)viewEye.width/(float)viewEye.height;

	int size_y = 1024.0f * cl_leveloverview.GetFloat(); // scale factor, 1024 = OVERVIEW_MAP_SIZE
	int	size_x = size_y * aspect;	// standard screen aspect 

	viewEye.origin.x -= size_x / 2;
	viewEye.origin.y += size_y / 2;

	viewEye.m_OrthoLeft   = 0;
	viewEye.m_OrthoTop    = -size_y;
	viewEye.m_OrthoRight  = size_x;
	viewEye.m_OrthoBottom = 0;

	viewEye.angles = QAngle( 90, 90, 0 );

	// simple movement detector, show position if moved
	int newCRC = viewEye.origin.x + viewEye.origin.y + viewEye.origin.z;
	if ( newCRC != oldCRC )
	{
		Msg( "Overview: scale %.2f, pos_x %.0f, pos_y %.0f\n", cl_leveloverview.GetFloat(),
			viewEye.origin.x, viewEye.origin.y );
		oldCRC = newCRC;
	}

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->ClearColor4ub( 0, 255, 0, 255 );

	// render->DrawTopView( true );
}

//-----------------------------------------------------------------------------
// Purpose: Render current view into specified rectangle
// Input  : *rect - is computed by CVideoMode_Common::GetClientViewRect()
//-----------------------------------------------------------------------------
void CViewRender::Render( vrect_t *rect )
{
	Assert(s_DbgSetupOrigin == m_View.origin);
	Assert(s_DbgSetupAngles == m_View.angles);

	VPROF_BUDGET( "CViewRender::Render", "CViewRender::Render" );
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	vrect_t vr = *rect;

	// Stub out the material system if necessary.
	CMatStubHandler matStub;

	engine->EngineStats_BeginFrame();
	
	// Assume normal vis
	m_bForceNoVis			= false;
	
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();


    // Set for console commands, etc.
    render->SetMainView ( m_View.origin, m_View.angles );

    for( StereoEye_t eEye = GetFirstEye(); eEye <= GetLastEye(); eEye = (StereoEye_t)(eEye+1) )
	{
		CViewSetup &viewEye = GetView( eEye );


	    static ConVarRef sv_restrict_aspect_ratio_fov( "sv_restrict_aspect_ratio_fov" );
	    float aspectRatio = engine->GetScreenAspectRatio() * 0.75f;	 // / (4/3)
	    float limitedAspectRatio = aspectRatio;
	    if ( ( sv_restrict_aspect_ratio_fov.GetInt() > 0 && engine->IsWindowedMode() && gpGlobals->maxClients > 1 ) ||
		    sv_restrict_aspect_ratio_fov.GetInt() == 2 )
	    {
		    limitedAspectRatio = MIN( aspectRatio, 1.85f * 0.75f ); // cap out the FOV advantage at a 1.85:1 ratio (about the widest any legit user should be)
	    }

		viewEye.fov = ScaleFOVByWidthRatio( viewEye.fov, limitedAspectRatio );
		viewEye.fovViewmodel = ScaleFOVByWidthRatio( viewEye.fovViewmodel, aspectRatio );

	    // Let the client mode hook stuff.
	    g_pClientMode->PreRender(&viewEye );

	    g_pClientMode->AdjustEngineViewport( vr.x, vr.y, vr.width, vr.height );

	    ToolFramework_AdjustEngineViewport( vr.x, vr.y, vr.width, vr.height );

	    float flViewportScale = mat_viewportscale.GetFloat();

		viewEye.m_nUnscaledX = vr.x;
		viewEye.m_nUnscaledY = vr.y;
		viewEye.m_nUnscaledWidth = vr.width;
		viewEye.m_nUnscaledHeight = vr.height;

        switch( eEye )
		{
			case STEREO_EYE_MONO:
			{
#if 0
                // Good test mode for debugging viewports that are not full-size.
	            view.width			= vr.width * flViewportScale * 0.75f;
	            view.height			= vr.height * flViewportScale * 0.75f;
	            view.x				= vr.x + view.width * 0.10f;
	            view.y				= vr.y + view.height * 0.20f;
#else
				viewEye.x				= vr.x * flViewportScale;
				viewEye.y				= vr.y * flViewportScale;
				viewEye.width			= vr.width * flViewportScale;
				viewEye.height			= vr.height * flViewportScale;
#endif
			    float engineAspectRatio = engine->GetScreenAspectRatio();
				viewEye.m_flAspectRatio	= ( engineAspectRatio > 0.0f ) ? engineAspectRatio : ( (float)viewEye.width / (float)viewEye.height );
			}
			break;

			case STEREO_EYE_RIGHT:
			case STEREO_EYE_LEFT:
			{
				g_pSourceVR->GetViewportBounds( (ISourceVirtualReality::VREye)(eEye - 1 ), &viewEye.x, &viewEye.y, &viewEye.width, &viewEye.height );
				viewEye.m_nUnscaledWidth = viewEye.width;
				viewEye.m_nUnscaledHeight = viewEye.height;
				viewEye.m_nUnscaledX = viewEye.x;
				viewEye.m_nUnscaledY = viewEye.y;
			}
			break;

            default:
                Assert ( false );
                break;
		}

		// if we still don't have an aspect ratio, compute it from the view size
		if( viewEye.m_flAspectRatio <= 0.f )
			viewEye.m_flAspectRatio	= (float)viewEye.width / (float)viewEye.height;

	    int nClearFlags = VIEW_CLEAR_DEPTH | VIEW_CLEAR_STENCIL;

	    if( gl_clear_randomcolor.GetBool() )
	    {
		    CMatRenderContextPtr pRenderContext( materials );
		    pRenderContext->ClearColor3ub( rand()%256, rand()%256, rand()%256 );
		    pRenderContext->ClearBuffers( true, false, false );
		    pRenderContext->Release();
	    }
	    else if ( gl_clear.GetBool() )
	    {
		    nClearFlags |= VIEW_CLEAR_COLOR;
	    }
	    else if ( IsPosix() )
	    {
		    MaterialAdapterInfo_t adapterInfo;
		    materials->GetDisplayAdapterInfo( materials->GetCurrentAdapter(), adapterInfo );

		    // On Posix, on ATI, we always clear color if we're antialiasing
		    if ( adapterInfo.m_VendorID == 0x1002 )
		    {
			    if ( g_pMaterialSystem->GetCurrentConfigForVideoCard().m_nAASamples > 1 )
			    {
				    nClearFlags |= VIEW_CLEAR_COLOR;
			    }
		    }
	    }

	    // Determine if we should draw view model ( client mode override )
	    bool drawViewModel = g_pClientMode->ShouldDrawViewModel();

	    if ( cl_leveloverview.GetFloat() > 0 )
	    {
		    SetUpOverView();		
		    nClearFlags |= VIEW_CLEAR_COLOR;
		    drawViewModel = false;
	    }

	    // Apply any player specific overrides
	    if ( pPlayer )
	    {
		    // Override view model if necessary
		    if ( !pPlayer->m_Local.m_bDrawViewmodel )
		    {
			    drawViewModel = false;
		    }
	    }

	    int flags = 0;
		if( eEye == STEREO_EYE_MONO || eEye == STEREO_EYE_LEFT || ( g_ClientVirtualReality.ShouldRenderHUDInWorld() ) )
		{
			flags = RENDERVIEW_DRAWHUD;
		}
	    if ( drawViewModel )
	    {
		    flags |= RENDERVIEW_DRAWVIEWMODEL;
	    }
		if( eEye == STEREO_EYE_RIGHT )
		{
			// we should use the monitor view from the left eye for both eyes
			flags |= RENDERVIEW_SUPPRESSMONITORRENDERING;
		}

	    RenderView( viewEye, nClearFlags, flags );

		if ( UseVR() )
		{
			bool bDoUndistort = ! engine->IsTakingScreenshot();

			if ( bDoUndistort )
			{
				g_ClientVirtualReality.PostProcessFrame( eEye );
			}

			// logic here all cloned from code in viewrender.cpp around RenderHUDQuad:

			// figure out if we really want to draw the HUD based on freeze cam
			bool bInFreezeCam = ( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM );

			// draw the HUD after the view model so its "I'm closer" depth queues work right.
			if( !bInFreezeCam && g_ClientVirtualReality.ShouldRenderHUDInWorld() )
			{
				// TODO - a bit of a shonky test - basically trying to catch the main menu, the briefing screen, the loadout screen, etc.
				bool bTranslucent = !g_pMatSystemSurface->IsCursorVisible();
				g_ClientVirtualReality.OverlayHUDQuadWithUndistort( viewEye, bDoUndistort, g_pClientMode->ShouldBlackoutAroundHUD(), bTranslucent );
			}
		}
    }


	// TODO: should these be inside or outside the stereo eye stuff?
	g_pClientMode->PostRender();
	engine->EngineStats_EndFrame();

#if !defined( _X360 )
	// Stop stubbing the material system so we can see the budget panel
	matStub.End();
#endif


	// Draw all of the UI stuff "fullscreen"
    // (this is not health, ammo, etc. Nor is it pre-game briefing interface stuff - this is the stuff that appears when you hit Esc in-game)
	// In stereo mode this is rendered inside of RenderView so it goes into the render target
	if( !g_ClientVirtualReality.ShouldRenderHUDInWorld() )
	{
		CViewSetup view2d;
		view2d.x				= rect->x;
		view2d.y				= rect->y;
		view2d.width			= rect->width;
		view2d.height			= rect->height;

		render->Push2DView( view2d, 0, NULL, GetFrustum() );
		render->VGui_Paint( PAINT_UIPANELS | PAINT_CURSOR );
		render->PopView( GetFrustum() );
	}


}




static void GetPos( const CCommand &args, Vector &vecOrigin, QAngle &angles )
{
	vecOrigin = MainViewOrigin();
	angles = MainViewAngles();
	if ( args.ArgC() == 2 && atoi( args[1] ) == 2 )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer )
		{
			vecOrigin = pPlayer->GetAbsOrigin();
			angles = pPlayer->GetAbsAngles();
		}
	}
}

CON_COMMAND( spec_pos, "dump position and angles to the console" )
{
	Vector vecOrigin;
	QAngle angles;
	GetPos( args, vecOrigin, angles );
	Warning( "spec_goto %.1f %.1f %.1f %.1f %.1f\n", vecOrigin.x, vecOrigin.y, 
		vecOrigin.z, angles.x, angles.y );
}

CON_COMMAND( getpos, "dump position and angles to the console" )
{
	Vector vecOrigin;
	QAngle angles;
	GetPos( args, vecOrigin, angles );

	const char *pCommand1 = "setpos";
	const char *pCommand2 = "setang";
	if ( args.ArgC() == 2 && atoi( args[1] ) == 2 )
	{
		pCommand1 = "setpos_exact";
		pCommand2 = "setang_exact";
	}

	Warning( "%s %f %f %f;", pCommand1, vecOrigin.x, vecOrigin.y, vecOrigin.z );
	Warning( "%s %f %f %f\n", pCommand2, angles.x, angles.y, angles.z );
}

