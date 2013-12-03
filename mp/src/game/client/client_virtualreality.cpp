//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"

#include "client_virtualreality.h"

#include "materialsystem/itexture.h"
#include "view_shared.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "vgui_controls/Controls.h"
#include "headtrack/isourcevirtualreality.h"
#include "ienginevgui.h"
#include "cdll_client_int.h"
#include "tier0/vprof_telemetry.h"
#include <time.h>

CClientVirtualReality g_ClientVirtualReality;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientVirtualReality, IClientVirtualReality, 
	CLIENTVIRTUALREALITY_INTERFACE_VERSION, g_ClientVirtualReality );


// --------------------------------------------------------------------
// A huge pile of VR convars
// --------------------------------------------------------------------
ConVar vr_moveaim_mode      ( "vr_moveaim_mode",      "3", FCVAR_ARCHIVE, "0=move+shoot from face. 1=move with torso. 2,3,4=shoot with face+mouse cursor. 5+ are probably not that useful." );
ConVar vr_moveaim_mode_zoom ( "vr_moveaim_mode_zoom", "3", FCVAR_ARCHIVE, "0=move+shoot from face. 1=move with torso. 2,3,4=shoot with face+mouse cursor. 5+ are probably not that useful." );

ConVar vr_moveaim_reticle_yaw_limit        ( "vr_moveaim_reticle_yaw_limit",        "10", FCVAR_ARCHIVE, "Beyond this number of degrees, the mouse drags the torso" );
ConVar vr_moveaim_reticle_pitch_limit      ( "vr_moveaim_reticle_pitch_limit",      "30", FCVAR_ARCHIVE, "Beyond this number of degrees, the mouse clamps" );
// Note these are scaled by the zoom factor.
ConVar vr_moveaim_reticle_yaw_limit_zoom   ( "vr_moveaim_reticle_yaw_limit_zoom",   "0", FCVAR_ARCHIVE, "Beyond this number of degrees, the mouse drags the torso" );
ConVar vr_moveaim_reticle_pitch_limit_zoom ( "vr_moveaim_reticle_pitch_limit_zoom", "-1", FCVAR_ARCHIVE, "Beyond this number of degrees, the mouse clamps" );

// This are somewhat obsolete.
ConVar vr_aim_yaw_offset( "vr_aim_yaw_offset", "90", 0, "This value is added to Yaw when returning the vehicle aim angles to Source." );

ConVar vr_stereo_swap_eyes ( "vr_stereo_swap_eyes", "0", 0, "1=swap eyes." );

// Useful for debugging wacky-projection problems, separate from multi-rendering problems.
ConVar vr_stereo_mono_set_eye ( "vr_stereo_mono_set_eye", "0", 0, "0=off, Set all eyes to 1=left, 2=right, 3=middle eye" );

// Useful for examining anims, etc.
ConVar vr_debug_remote_cam( "vr_debug_remote_cam", "0" );
ConVar vr_debug_remote_cam_pos_x( "vr_debug_remote_cam_pos_x", "150.0" );
ConVar vr_debug_remote_cam_pos_y( "vr_debug_remote_cam_pos_y", "0.0" );
ConVar vr_debug_remote_cam_pos_z( "vr_debug_remote_cam_pos_z", "0.0" );
ConVar vr_debug_remote_cam_target_x( "vr_debug_remote_cam_target_x", "0.0" );
ConVar vr_debug_remote_cam_target_y( "vr_debug_remote_cam_target_y", "0.0" );
ConVar vr_debug_remote_cam_target_z( "vr_debug_remote_cam_target_z", "-50.0" );

ConVar vr_translation_limit( "vr_translation_limit", "10.0", 0, "How far the in-game head will translate before being clamped." );

ConVar vr_dont_use_calibration_projection ( "vr_dont_use_calibration_projection", "0", 0, "1=use calibrated rotation, but not projection" );

// HUD config values
ConVar vr_render_hud_in_world( "vr_render_hud_in_world", "1" );
ConVar vr_hud_max_fov( "vr_hud_max_fov", "60", FCVAR_ARCHIVE, "Max FOV of the HUD" );
ConVar vr_hud_forward( "vr_hud_forward", "500", FCVAR_ARCHIVE, "Apparent distance of the HUD in inches" );
ConVar vr_hud_display_ratio( "vr_hud_display_ratio", "0.95", FCVAR_ARCHIVE );

ConVar vr_hud_axis_lock_to_world( "vr_hud_axis_lock_to_world", "0", FCVAR_ARCHIVE, "Bitfield - locks HUD axes to the world - 0=pitch, 1=yaw, 2=roll" );

// Default distance clips through rocketlauncher, heavy's body, etc.
ConVar vr_projection_znear_multiplier( "vr_projection_znear_multiplier", "0.3", 0, "Allows moving the ZNear plane to deal with body clipping" );

ConVar vr_stat_sample_period ( "vr_stat_sample_period", "1", 0, "Frequency with which to sample motion stats" );

// Should the viewmodel (weapon) translate with the HMD, or remain fixed to the in-world body (but still rotate with the head)? Purely a graphics effect - no effect on actual bullet aiming.
// Has no effect in aim modes where aiming is not controlled by the head.
ConVar vr_viewmodel_translate_with_head ( "vr_viewmodel_translate_with_head", "0", 0, "1=translate the viewmodel with the head motion." );

ConVar vr_zoom_multiplier ( "vr_zoom_multiplier", "2.0", FCVAR_ARCHIVE, "When zoomed, how big is the scope on your HUD?" );
ConVar vr_zoom_scope_scale ( "vr_zoom_scope_scale", "6.0", 0, "Something to do with the default scope HUD overlay size." );		// Horrible hack - should work out the math properly, but we need to ship.


ConVar vr_viewmodel_offset_forward( "vr_viewmodel_offset_forward", "-8", 0 );
ConVar vr_viewmodel_offset_forward_large( "vr_viewmodel_offset_forward_large", "-15", 0 );

ConVar vr_ipdtest_left_t ( "vr_ipdtest_left_t", "260", FCVAR_ARCHIVE );
ConVar vr_ipdtest_left_b ( "vr_ipdtest_left_b", "530", FCVAR_ARCHIVE );
ConVar vr_ipdtest_left_i ( "vr_ipdtest_left_i", "550", FCVAR_ARCHIVE );
ConVar vr_ipdtest_left_o ( "vr_ipdtest_left_o", "200", FCVAR_ARCHIVE );
ConVar vr_ipdtest_right_t ( "vr_ipdtest_right_t", "260", FCVAR_ARCHIVE );
ConVar vr_ipdtest_right_b ( "vr_ipdtest_right_b", "530", FCVAR_ARCHIVE );
ConVar vr_ipdtest_right_i ( "vr_ipdtest_right_i", "550", FCVAR_ARCHIVE );
ConVar vr_ipdtest_right_o ( "vr_ipdtest_right_o", "200", FCVAR_ARCHIVE );



// --------------------------------------------------------------------
// Purpose: Cycle through the aim & move modes.
// --------------------------------------------------------------------
void CC_VR_Cycle_Aim_Move_Mode ( const CCommand& args )
{
	int hmmCurrentMode = vr_moveaim_mode.GetInt();
	if ( g_ClientVirtualReality.CurrentlyZoomed() )
	{
		hmmCurrentMode = vr_moveaim_mode_zoom.GetInt();
	}

	hmmCurrentMode++;
	if ( hmmCurrentMode >= HMM_LAST )
	{
		hmmCurrentMode = 0;
	}

	if ( g_ClientVirtualReality.CurrentlyZoomed() )
	{
		vr_moveaim_mode_zoom.SetValue ( hmmCurrentMode );
		Warning ( "Headtrack mode (zoomed) %d\n", hmmCurrentMode );
	}
	else
	{
		vr_moveaim_mode.SetValue ( hmmCurrentMode );
		Warning ( "Headtrack mode %d\n", hmmCurrentMode );
	}
}
static ConCommand vr_cycle_aim_move_mode("vr_cycle_aim_move_mode", CC_VR_Cycle_Aim_Move_Mode, "Cycle through the aim & move modes." );



// --------------------------------------------------------------------
// Purpose: Returns true if the matrix is orthonormal
// --------------------------------------------------------------------
bool IsOrthonormal ( VMatrix Mat, float fTolerance )
{
	float LenFwd = Mat.GetForward().Length();
	float LenUp = Mat.GetUp().Length();
	float LenLeft = Mat.GetLeft().Length();
	float DotFwdUp = Mat.GetForward().Dot ( Mat.GetUp() );
	float DotUpLeft = Mat.GetUp().Dot ( Mat.GetLeft() );
	float DotLeftFwd = Mat.GetLeft().Dot ( Mat.GetForward() );
	if ( fabsf ( LenFwd - 1.0f ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( LenUp - 1.0f ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( LenLeft - 1.0f ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( DotFwdUp ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( DotUpLeft ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( DotLeftFwd ) > fTolerance )
	{
		return false;
	}
	return true;
}


// --------------------------------------------------------------------
// Purpose: Computes the FOV from the projection matrix
// --------------------------------------------------------------------
void CalcFovFromProjection ( float *pFov, const VMatrix &proj )
{
	// The projection matrices should be of the form:
	// p0  0   z1 p1 
	// 0   p2  z2 p3
	// 0   0   z3 1
	// (p0 = X fov, p1 = X offset, p2 = Y fov, p3 = Y offset )
	// TODO: cope with more complex projection matrices?
	float xscale  = proj.m[0][0];
	Assert ( proj.m[0][1] == 0.0f );
	float xoffset = proj.m[0][2];
	Assert ( proj.m[0][3] == 0.0f );
	Assert ( proj.m[1][0] == 0.0f );
	float yscale  = proj.m[1][1];
	float yoffset = proj.m[1][2];
	Assert ( proj.m[1][3] == 0.0f );
	// Row 2 determines Z-buffer values - don't care about those for now.
	Assert ( proj.m[3][0] == 0.0f );
	Assert ( proj.m[3][1] == 0.0f );
	Assert ( proj.m[3][2] == -1.0f );
	Assert ( proj.m[3][3] == 0.0f );

	// The math here:
	// A view-space vector (x,y,z,1) is transformed by the projection matrix
	// / xscale   0     xoffset  0 \
	// |    0   yscale  yoffset  0 |
	// |    ?     ?        ?     ? |
	// \    0     0       -1     0 /
	//
	// Then the result is normalized (i.e. divide by w) and the result clipped to the [-1,+1] unit cube.
	// (ignore Z for now, and the clipping is slightly different).
	// So, we want to know what vectors produce a clip value of -1 and +1 in each direction, e.g. in the X direction:
	//    +-1 = ( xscale*x + xoffset*z ) / (-1*z)
	//        = xscale*(x/z) + xoffset            (I flipped the signs of both sides)
	// => (+-1 - xoffset)/xscale = x/z
	// ...and x/z is tan(theta), and theta is the half-FOV.

	float fov_px = 2.0f * RAD2DEG ( atanf ( fabsf ( (  1.0f - xoffset ) / xscale ) ) );
	float fov_nx = 2.0f * RAD2DEG ( atanf ( fabsf ( ( -1.0f - xoffset ) / xscale ) ) );
	float fov_py = 2.0f * RAD2DEG ( atanf ( fabsf ( (  1.0f - yoffset ) / yscale ) ) );
	float fov_ny = 2.0f * RAD2DEG ( atanf ( fabsf ( ( -1.0f - yoffset ) / yscale ) ) );

	*pFov = Max ( Max ( fov_px, fov_nx ), Max ( fov_py, fov_ny ) );
	// FIXME: hey you know, I could do the Max() series before I call all those expensive atanf()s...
}


// --------------------------------------------------------------------
// construction/destruction
// --------------------------------------------------------------------
CClientVirtualReality::CClientVirtualReality()
{
	m_PlayerTorsoOrigin.Init();
	m_PlayerTorsoAngle.Init();
	m_WorldFromWeapon.Identity();
	m_WorldFromMidEye.Identity();
	
	m_bOverrideTorsoAngle = false;
	m_OverrideTorsoOffset.Init();

	// Also reset our model of the player's torso orientation
	m_PlayerTorsoAngle.Init ( 0.0f, 0.0f, 0.0f );

	m_WorldZoomScale = 1.0f;
	m_hmmMovementActual = HMM_SHOOTFACE_MOVEFACE;
	m_iAlignTorsoAndViewToWeaponCountdown = 0;

	m_rtLastMotionSample = 0;
	m_bMotionUpdated = false;

	m_bIpdTestEnabled = false;

	// Needs to be after the tracker has initted.
	m_bIpdTestEnabled = false;
	m_IpdTestControl = 0;
}

CClientVirtualReality::~CClientVirtualReality()
{
}


// --------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------
bool			CClientVirtualReality::Connect( CreateInterfaceFn factory )
{
	if ( !factory )
		return false;

	if ( !BaseClass::Connect( factory ) )
		return false;

	return true;
}


// --------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------
void			CClientVirtualReality::Disconnect()
{
	BaseClass::Disconnect();
}


// --------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------
void *			CClientVirtualReality::QueryInterface( const char *pInterfaceName )
{
	CreateInterfaceFn factory = Sys_GetFactoryThis();	// This silly construction is necessary
	return factory( pInterfaceName, NULL );				// to prevent the LTCG compiler from crashing.
}


// --------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------
InitReturnVal_t	CClientVirtualReality::Init()
{
	InitReturnVal_t nRetVal = BaseClass::Init();
	if ( nRetVal != INIT_OK )
		return nRetVal;

	return INIT_OK;
}


// --------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------
void			CClientVirtualReality::Shutdown()
{
	BaseClass::Shutdown();
}


// --------------------------------------------------------------------
// Purpose: Draws the main menu in Stereo
// --------------------------------------------------------------------
void CClientVirtualReality::DrawMainMenu()
{
	// have to draw the UI in stereo via the render texture or it won't fuse properly

	// Draw it into the render target first
	ITexture *pTexture = materials->FindTexture( "_rt_gui", NULL, false );
	Assert( pTexture );
	if( !pTexture) 
		return;

	CMatRenderContextPtr pRenderContext( materials );
	int viewActualWidth = pTexture->GetActualWidth();
	int viewActualHeight = pTexture->GetActualHeight();

	int viewWidth, viewHeight;
	vgui::surface()->GetScreenSize( viewWidth, viewHeight );

	// clear depth in the backbuffer before we push the render target
	pRenderContext->ClearBuffers( false, true, true );

	// constrain where VGUI can render to the view
	pRenderContext->PushRenderTargetAndViewport( pTexture, NULL, 0, 0, viewActualWidth, viewActualHeight );
	pRenderContext->OverrideAlphaWriteEnable( true, true );

	// clear the render target 
	pRenderContext->ClearColor4ub( 0, 0, 0, 0 );
	pRenderContext->ClearBuffers( true, false );

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "VGui_DrawHud", __FUNCTION__ );

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

	// paint the main menu and cursor
	render->VGui_Paint( (PaintMode_t) ( PAINT_UIPANELS | PAINT_CURSOR ) );

	pRenderContext->OverrideAlphaWriteEnable( false, true );
	pRenderContext->PopRenderTargetAndViewport();
	pRenderContext->Flush();

	int leftX, leftY, leftW, leftH, rightX, rightY, rightW, rightH;
	g_pSourceVR->GetViewportBounds( ISourceVirtualReality::VREye_Left, &leftX, &leftY, &leftW, &leftH );
	g_pSourceVR->GetViewportBounds( ISourceVirtualReality::VREye_Right, &rightX, &rightY, &rightW, &rightH );


	// render the main view
	CViewSetup viewEye[STEREO_EYE_MAX];
	viewEye[ STEREO_EYE_MONO ].zNear = 0.1;
	viewEye[ STEREO_EYE_MONO ].zFar = 10000.f;
	viewEye[ STEREO_EYE_MONO ].angles.Init();
	viewEye[ STEREO_EYE_MONO ].origin.Zero();
	viewEye[ STEREO_EYE_MONO ].x = viewEye[ STEREO_EYE_MONO ].m_nUnscaledX =  leftX;
	viewEye[ STEREO_EYE_MONO ].y = viewEye[ STEREO_EYE_MONO ].m_nUnscaledY = leftY;
	viewEye[ STEREO_EYE_MONO ].width = viewEye[ STEREO_EYE_MONO ].m_nUnscaledWidth = leftW;
	viewEye[ STEREO_EYE_MONO ].height = viewEye[ STEREO_EYE_MONO ].m_nUnscaledHeight = leftH;

	viewEye[STEREO_EYE_LEFT] = viewEye[STEREO_EYE_RIGHT] = viewEye[ STEREO_EYE_MONO ] ;
	viewEye[STEREO_EYE_LEFT].m_eStereoEye = STEREO_EYE_LEFT;
	viewEye[STEREO_EYE_RIGHT].x = rightX;
	viewEye[STEREO_EYE_RIGHT].y = rightY;
	viewEye[STEREO_EYE_RIGHT].m_eStereoEye = STEREO_EYE_RIGHT;

	// let headtrack.dll tell us where to put the cameras
	ProcessCurrentTrackingState( 75.f );
	Vector vViewModelOrigin;
	QAngle qViewModelAngles;
	OverrideView( &viewEye[ STEREO_EYE_MONO ] , &vViewModelOrigin, &qViewModelAngles, HMM_NOOVERRIDE );
	g_ClientVirtualReality.OverrideStereoView( &viewEye[ STEREO_EYE_MONO ] , &viewEye[STEREO_EYE_LEFT], &viewEye[STEREO_EYE_RIGHT] );

	// render both eyes
	for( int nView = STEREO_EYE_LEFT; nView <= STEREO_EYE_RIGHT; nView++ )
	{
		// clear happens here probably
		render->Push3DView( viewEye[nView], VIEW_CLEAR_DEPTH|VIEW_CLEAR_COLOR, NULL, NULL );

		RenderHUDQuad( true,  false );

		render->PopView( NULL );
	}

	vrect_t rect;
	rect.x = rect.y = 0;
	rect.width = leftW*2;
	rect.height = leftH;
	PostProcessFrame( &rect );
}



// --------------------------------------------------------------------
// Purpose:
//		Offset the incoming view appropriately.
//		Set up the "middle eye" from that.
// --------------------------------------------------------------------
bool CClientVirtualReality::OverrideView ( CViewSetup *pViewMiddle, Vector *pViewModelOrigin, QAngle *pViewModelAngles, HeadtrackMovementMode_t hmmMovementOverride )
{
	if( !UseVR() )
	{
		return false;
	}

	if ( hmmMovementOverride == HMM_NOOVERRIDE )
	{
		if ( CurrentlyZoomed() )
		{
			m_hmmMovementActual = static_cast<HeadtrackMovementMode_t>( vr_moveaim_mode_zoom.GetInt() );
		}
		else
		{
			m_hmmMovementActual = static_cast<HeadtrackMovementMode_t>( vr_moveaim_mode.GetInt() );
		}
	}
	else
	{
		m_hmmMovementActual = hmmMovementOverride;
	}


	// Incoming data may or may not be useful - it is the origin and aim of the "player", i.e. where bullets come from.
	// In some modes it is an independent thing, guided by the mouse & keyboard = useful.
	// In other modes it's just where the HMD was pointed last frame, modified slightly by kbd+mouse.
	// In those cases, we should use our internal reference (which keeps track thanks to OverridePlayerMotion)
	QAngle originalMiddleAngles = pViewMiddle->angles;
	Vector originalMiddleOrigin = pViewMiddle->origin;

	// Figure out the in-game "torso" concept, which corresponds to the player's physical torso.
	m_PlayerTorsoOrigin = pViewMiddle->origin;

	// Ignore what was passed in - it's just the direction the weapon is pointing, which was determined by last frame's HMD orientation!
	// Instead use our cached value.
	QAngle torsoAngles = m_PlayerTorsoAngle;

	VMatrix worldFromTorso;
	AngleMatrix ( torsoAngles, worldFromTorso.As3x4() );
	worldFromTorso.SetTranslation ( m_PlayerTorsoOrigin );

	//// Scale translation e.g. to allow big in-game leans with only a small head movement.
	//// Clamp HMD movement to a reasonable amount to avoid wallhacks, vis problems, etc.
	float limit = vr_translation_limit.GetFloat();
	VMatrix matMideyeZeroFromMideyeCurrent = g_pSourceVR->GetMideyePose();
	Vector viewTranslation = matMideyeZeroFromMideyeCurrent.GetTranslation();
	if ( viewTranslation.IsLengthGreaterThan ( limit ) )
	{
		viewTranslation.NormalizeInPlace();
		viewTranslation *= limit;
		matMideyeZeroFromMideyeCurrent.SetTranslation( viewTranslation );
	}

	// Now figure out the three principal matrices: m_TorsoFromMideye, m_WorldFromMidEye, m_WorldFromWeapon
	// m_TorsoFromMideye is done so that OverridePlayerMotion knows what to do with WASD.

	switch ( m_hmmMovementActual )
	{
	case HMM_SHOOTFACE_MOVEFACE:
	case HMM_SHOOTFACE_MOVETORSO:
		// Aim point is down your nose, i.e. same as the view angles.
		m_TorsoFromMideye = matMideyeZeroFromMideyeCurrent;
		m_WorldFromMidEye = worldFromTorso * matMideyeZeroFromMideyeCurrent;
		m_WorldFromWeapon = m_WorldFromMidEye;
		break;

	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEFACE:
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEMOUSE:
	case HMM_SHOOTMOUSE_MOVEFACE:
	case HMM_SHOOTMOVEMOUSE_LOOKFACE:
		// Aim point is independent of view - leave it as it was, just copy it into m_WorldFromWeapon for our use.
		m_TorsoFromMideye = matMideyeZeroFromMideyeCurrent;
		m_WorldFromMidEye = worldFromTorso * matMideyeZeroFromMideyeCurrent;
		AngleMatrix ( originalMiddleAngles, m_WorldFromWeapon.As3x4() );
		m_WorldFromWeapon.SetTranslation ( originalMiddleOrigin );
		break;

	case HMM_SHOOTMOVELOOKMOUSE:
		// HMD is ignored completely, mouse does everything.
		m_PlayerTorsoAngle = originalMiddleAngles;

		AngleMatrix ( originalMiddleAngles, worldFromTorso.As3x4() );
		worldFromTorso.SetTranslation ( m_PlayerTorsoOrigin );

		m_TorsoFromMideye.Identity();
		m_WorldFromMidEye = worldFromTorso;
		m_WorldFromWeapon = worldFromTorso;
		break;

	case HMM_SHOOTMOVELOOKMOUSEFACE:
		// mouse does everything, and then we add head tracking on top of that
		worldFromTorso = worldFromTorso * matMideyeZeroFromMideyeCurrent; 

		m_TorsoFromMideye = matMideyeZeroFromMideyeCurrent;
		m_WorldFromWeapon = worldFromTorso;
		m_WorldFromMidEye = worldFromTorso;
		break;

	default: Assert ( false ); break;
	}

	// Finally convert back to origin+angles that the game understands.
	pViewMiddle->origin = m_WorldFromMidEye.GetTranslation();
	VectorAngles ( m_WorldFromMidEye.GetForward(), m_WorldFromMidEye.GetUp(), pViewMiddle->angles );

	*pViewModelAngles = pViewMiddle->angles;
	if ( vr_viewmodel_translate_with_head.GetBool() )
	{
		*pViewModelOrigin = pViewMiddle->origin;
	}
	else
	{
		*pViewModelOrigin = originalMiddleOrigin;
	}

	m_WorldFromMidEyeNoDebugCam = m_WorldFromMidEye;
	if ( vr_debug_remote_cam.GetBool() )
	{
		Vector vOffset ( vr_debug_remote_cam_pos_x.GetFloat(), vr_debug_remote_cam_pos_y.GetFloat(), vr_debug_remote_cam_pos_z.GetFloat() );
		Vector vLookat ( vr_debug_remote_cam_target_x.GetFloat(), vr_debug_remote_cam_target_y.GetFloat(), vr_debug_remote_cam_target_z.GetFloat() );
		pViewMiddle->origin += vOffset;
		Vector vView = vLookat - vOffset;
		VectorAngles ( vView, m_WorldFromMidEye.GetUp(), pViewMiddle->angles );

		AngleMatrix ( pViewMiddle->angles, m_WorldFromMidEye.As3x4() );

		m_WorldFromMidEye.SetTranslation ( pViewMiddle->origin );
		m_TorsoFromMideye.Identity();
	}

	// set the near clip plane so the local player clips less
	pViewMiddle->zNear *= vr_projection_znear_multiplier.GetFloat();

	return true;
}


// --------------------------------------------------------------------
// Purpose:
//		In some aim/move modes, the HUD aim reticle lags because it's
//		using slightly stale data. This will feed it the newest data. 
// --------------------------------------------------------------------
bool CClientVirtualReality::OverrideWeaponHudAimVectors ( Vector *pAimOrigin, Vector *pAimDirection )
{
	if( !UseVR() )
	{
		return false;
	}

	Assert ( pAimOrigin != NULL );
	Assert ( pAimDirection != NULL );

	// So give it some nice high-fps numbers, not the low-fps ones we get from the game.
	*pAimOrigin = m_WorldFromWeapon.GetTranslation();
	*pAimDirection = m_WorldFromWeapon.GetForward();

	return true;
}


// --------------------------------------------------------------------
// Purpose:
//		Set up the left and right eyes from the middle eye if stereo is on.
//		Advise calling soonish after OverrideView().
// --------------------------------------------------------------------
bool CClientVirtualReality::OverrideStereoView( CViewSetup *pViewMiddle, CViewSetup *pViewLeft, CViewSetup *pViewRight  )
{
	// Everything in here is in Source coordinate space.
	if( !UseVR() )
	{
		return false;
	}

	if ( vr_stereo_swap_eyes.GetBool() )
	{
		// Windows likes to randomly rename display numbers which causes eye-swaps, so this tries to cope with that.
		CViewSetup *pViewTemp = pViewLeft;
		pViewLeft = pViewRight;
		pViewRight = pViewTemp;
	}

	// Move eyes to calibrated positions.
	VMatrix worldFromLeftEye  = m_WorldFromMidEye * g_pSourceVR->GetMidEyeFromLeft();
	VMatrix worldFromRightEye = m_WorldFromMidEye * g_pSourceVR->GetMidEyeFromRight();

	Assert ( IsOrthonormal ( worldFromLeftEye, 0.001f ) );
	Assert ( IsOrthonormal ( worldFromRightEye, 0.001f ) );

	Vector rightFromLeft = worldFromRightEye.GetTranslation() - worldFromLeftEye.GetTranslation();
	//float calibratedIPD = rightFromLeft.Length();		// THIS IS NOT CORRECT. The positions of the virtual cameras do have any real physical "meaning" with the way we currently calibrate.
	float calibratedIPD = g_pSourceVR->GetDisplaySeparationMM() / 25.4f;

	// Scale the eyes closer/further to fit the desired IPD.
	// (the calibrated distance is the IPD of whoever calibrated it!)
	float desiredIPD = g_pSourceVR->GetUserIPDMM() / 25.4f;
	if ( calibratedIPD < 0.000001f )
	{
		// No HMD, or a monocular HMD.
	}
	else
	{
		float scale = 0.5f * ( desiredIPD - calibratedIPD ) / calibratedIPD;
		worldFromLeftEye.SetTranslation  ( worldFromLeftEye.GetTranslation()  - ( scale * rightFromLeft ) );
		worldFromRightEye.SetTranslation ( worldFromRightEye.GetTranslation() + ( scale * rightFromLeft ) );
	}

	Assert ( IsOrthonormal ( worldFromLeftEye, 0.001f ) );
	Assert ( IsOrthonormal ( worldFromRightEye, 0.001f ) );

	// Finally convert back to origin+angles.
	pViewLeft->origin  = worldFromLeftEye.GetTranslation();
	VectorAngles ( worldFromLeftEye.GetForward(),  worldFromLeftEye.GetUp(),  pViewLeft->angles );
	pViewRight->origin = worldFromRightEye.GetTranslation();
	VectorAngles ( worldFromRightEye.GetForward(), worldFromRightEye.GetUp(), pViewRight->angles );

	// Find the projection matrices.

	// TODO: this isn't the fastest thing in the world. Cache them?
	float headtrackFovScale = m_WorldZoomScale;
	pViewLeft->m_bViewToProjectionOverride = true;
	pViewRight->m_bViewToProjectionOverride = true;
	g_pSourceVR->GetEyeProjectionMatrix (  &pViewLeft->m_ViewToProjection, ISourceVirtualReality::VREye_Left,  pViewMiddle->zNear, pViewMiddle->zFar, 1.0f/headtrackFovScale );
	g_pSourceVR->GetEyeProjectionMatrix ( &pViewRight->m_ViewToProjection, ISourceVirtualReality::VREye_Right, pViewMiddle->zNear, pViewMiddle->zFar, 1.0f/headtrackFovScale );

	// And bodge together some sort of average for our cyclops friends.
	pViewMiddle->m_bViewToProjectionOverride = true;
	for ( int i = 0; i < 4; i++ )
	{
		for ( int j = 0; j < 4; j++ )
		{
			pViewMiddle->m_ViewToProjection.m[i][j] = (pViewLeft->m_ViewToProjection.m[i][j] + pViewRight->m_ViewToProjection.m[i][j] ) * 0.5f;
		}
	}

	if ( vr_dont_use_calibration_projection.GetBool() )
	{
		pViewLeft  ->m_bViewToProjectionOverride = false;
		pViewRight ->m_bViewToProjectionOverride = false;
		pViewMiddle->m_bViewToProjectionOverride = false;
	}

	switch ( vr_stereo_mono_set_eye.GetInt() )
	{
	case 0:
		// ... nothing.
		break;
	case 1:
		// Override all eyes with left
		*pViewMiddle = *pViewLeft;
		*pViewRight = *pViewLeft;
		pViewRight->m_eStereoEye = STEREO_EYE_RIGHT;
		break;
	case 2:
		// Override all eyes with right
		*pViewMiddle = *pViewRight;
		*pViewLeft = *pViewRight;
		pViewLeft->m_eStereoEye = STEREO_EYE_LEFT;
		break;
	case 3:
		// Override all eyes with middle
		*pViewRight = *pViewMiddle;
		*pViewLeft = *pViewMiddle;
		pViewLeft->m_eStereoEye = STEREO_EYE_LEFT;
		pViewRight->m_eStereoEye = STEREO_EYE_RIGHT;
		break;
	}

	// To make culling work correctly, calculate the widest FOV of each projection matrix.
	CalcFovFromProjection ( &(pViewLeft  ->fov), pViewLeft  ->m_ViewToProjection );
	CalcFovFromProjection ( &(pViewRight ->fov), pViewRight ->m_ViewToProjection );
	CalcFovFromProjection ( &(pViewMiddle->fov), pViewMiddle->m_ViewToProjection );

	// remember the view angles so we can limit the weapon to something near those
	m_PlayerViewAngle = pViewMiddle->angles;
	m_PlayerViewOrigin = pViewMiddle->origin;



	// Figure out the HUD vectors and frustum.

	// The aspect ratio of the HMD may be something bizarre (e.g. Rift is 640x800), and the pixels may not be square, so don't use that!
	static const float fAspectRatio = 4.f/3.f;
	float fHFOV = m_fHudHorizontalFov;
	float fVFOV = m_fHudHorizontalFov / fAspectRatio;

	const float fHudForward = vr_hud_forward.GetFloat();
	m_fHudHalfWidth = tan( DEG2RAD( fHFOV * 0.5f ) ) * fHudForward * m_WorldZoomScale;
	m_fHudHalfHeight = tan( DEG2RAD( fVFOV * 0.5f ) ) * fHudForward * m_WorldZoomScale;

	QAngle HudAngles;
	VMatrix HudUpCorrection;
	switch ( m_hmmMovementActual )
	{
	case HMM_SHOOTFACE_MOVETORSO:
		// Put the HUD in front of the player's torso.
		// This helps keep you oriented about where "forwards" is, which is otherwise surprisingly tricky!
		// TODO: try preserving roll and/or pitch from the view?
		HudAngles = m_PlayerTorsoAngle;
		HudUpCorrection.Identity();
		break;
	case HMM_SHOOTFACE_MOVEFACE:
	case HMM_SHOOTMOUSE_MOVEFACE:
	case HMM_SHOOTMOVEMOUSE_LOOKFACE:
	case HMM_SHOOTMOVELOOKMOUSE:
	case HMM_SHOOTMOVELOOKMOUSEFACE:
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEFACE:
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEMOUSE:
		// Put the HUD in front of wherever the player is looking.
		HudAngles = m_PlayerViewAngle;
		HudUpCorrection = g_pSourceVR->GetHudUpCorrection();
		break;
	default: Assert ( false ); break;
	}

	// This is a bitfield. A set bit means lock to the world, a clear bit means don't.
	int iVrHudAxisLockToWorld = vr_hud_axis_lock_to_world.GetInt();
	if ( ( iVrHudAxisLockToWorld & (1<<ROLL) ) != 0 )
	{
		HudAngles[ROLL] = 0.0f;
	}
	if ( ( iVrHudAxisLockToWorld & (1<<PITCH) ) != 0 )
	{
		HudAngles[PITCH] = 0.0f;
	}
	if ( ( iVrHudAxisLockToWorld & (1<<YAW) ) != 0 )
	{
		// Locking the yaw to the world is not particularly helpful, so what it actually means is lock it to the weapon.
		QAngle aimAngles;
		MatrixAngles( m_WorldFromWeapon.As3x4(), aimAngles );
		HudAngles[YAW] = aimAngles[YAW];
	}
	AngleMatrix ( HudAngles, m_WorldFromHud.As3x4() );
	m_WorldFromHud.SetTranslation ( m_PlayerViewOrigin );
	m_WorldFromHud = m_WorldFromHud * HudUpCorrection;

	// Remember in source X forwards, Y left, Z up.
	// We need to transform to a more conventional X right, Y up, Z backwards before doing the projection.
	VMatrix WorldFromHudView;
	WorldFromHudView./*X vector*/SetForward ( -m_WorldFromHud.GetLeft() );
	WorldFromHudView./*Y vector*/SetLeft    ( m_WorldFromHud.GetUp() );
	WorldFromHudView./*Z vector*/SetUp      ( -m_WorldFromHud.GetForward() );
	WorldFromHudView.SetTranslation         ( m_PlayerViewOrigin );

	VMatrix HudProjection;
	HudProjection.Identity();
	HudProjection.m[0][0] = fHudForward / m_fHudHalfWidth;
	HudProjection.m[1][1] = fHudForward / m_fHudHalfHeight;
	// Z vector is not used/valid, but w is for projection.
	HudProjection.m[3][2] = -1.0f;

	// This will transform a world point into a homogeneous vector that
	//  when projected (i.e. divide by w) maps to HUD space [-1,1]
	m_HudProjectionFromWorld = HudProjection * WorldFromHudView.InverseTR();

	return true;
}


// --------------------------------------------------------------------
// Purpose: Updates player orientation, position and motion according
//			to HMD status.
// --------------------------------------------------------------------
bool CClientVirtualReality::OverridePlayerMotion( float flInputSampleFrametime, const QAngle &oldAngles, const QAngle &curAngles, const Vector &curMotion, QAngle *pNewAngles, Vector *pNewMotion )
{
	Assert ( pNewAngles != NULL );
	Assert ( pNewMotion != NULL );
	*pNewAngles = curAngles;
	*pNewMotion = curMotion;

	if ( !UseVR() )
	{
		return false;
	}


	m_bMotionUpdated = true;

	// originalAngles tells us what the weapon angles were before whatever mouse, joystick, etc thing changed them - called "old"
	// curAngles holds the new weapon angles after mouse, joystick, etc. applied.
	// We need to compute what weapon angles WE want and return them in *pNewAngles - called "new"



	VMatrix worldFromTorso;

	// Whatever position is already here (set up by OverrideView) needs to be preserved.
	Vector vWeaponOrigin = m_WorldFromWeapon.GetTranslation();

	switch ( m_hmmMovementActual )
	{
	case HMM_SHOOTFACE_MOVEFACE:
	case HMM_SHOOTFACE_MOVETORSO:
		{
			// Figure out what changes were made to the WEAPON by mouse/joystick/etc
			VMatrix worldFromOldWeapon, worldFromCurWeapon;
			AngleMatrix ( oldAngles, worldFromOldWeapon.As3x4() );
			AngleMatrix ( curAngles, worldFromCurWeapon.As3x4() );

			// We ignore mouse pitch, the mouse can't do rolls, so it's just yaw changes.
			if( !m_bOverrideTorsoAngle )
			{
				m_PlayerTorsoAngle[YAW] += curAngles[YAW] - oldAngles[YAW];
				m_PlayerTorsoAngle[ROLL] = 0.0f;
				m_PlayerTorsoAngle[PITCH] = 0.0f;
			}

			AngleMatrix ( m_PlayerTorsoAngle, worldFromTorso.As3x4() );

			// Weapon view = mideye view, so apply that to the torso to find the world view direction.
			m_WorldFromWeapon = worldFromTorso * m_TorsoFromMideye;

			// ...and we return this new weapon direction as the player's orientation.
			MatrixAngles( m_WorldFromWeapon.As3x4(), *pNewAngles );

			// Restore the translation.
			m_WorldFromWeapon.SetTranslation ( vWeaponOrigin );
		}
		break;
	case HMM_SHOOTMOVELOOKMOUSEFACE:
	case HMM_SHOOTMOVEMOUSE_LOOKFACE:
	case HMM_SHOOTMOVELOOKMOUSE:
		{
			// The mouse just controls the weapon directly.
			*pNewAngles = curAngles;
			*pNewMotion = curMotion;

			// Move the torso by the yaw angles - torso should not have roll or pitch or you'll make folks ill.
			if( !m_bOverrideTorsoAngle && m_hmmMovementActual != HMM_SHOOTMOVELOOKMOUSEFACE )
			{
				m_PlayerTorsoAngle[YAW] = curAngles[YAW];
				m_PlayerTorsoAngle[ROLL] = 0.0f;
				m_PlayerTorsoAngle[PITCH] = 0.0f;
			}

			// Let every other system know.
			AngleMatrix( *pNewAngles, m_WorldFromWeapon.As3x4() );
			AngleMatrix( m_PlayerTorsoAngle, worldFromTorso.As3x4() );
			// Restore the translation.
			m_WorldFromWeapon.SetTranslation ( vWeaponOrigin );
		}
		break;
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEFACE:
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEMOUSE:
		{
			// The mouse controls the weapon directly.
			*pNewAngles = curAngles;
			*pNewMotion = curMotion;

			float fReticleYawLimit = vr_moveaim_reticle_yaw_limit.GetFloat();
			float fReticlePitchLimit = vr_moveaim_reticle_pitch_limit.GetFloat();

			if ( CurrentlyZoomed() )
			{
				fReticleYawLimit = vr_moveaim_reticle_yaw_limit_zoom.GetFloat() * m_WorldZoomScale;
				fReticlePitchLimit = vr_moveaim_reticle_pitch_limit_zoom.GetFloat() * m_WorldZoomScale;
				if ( fReticleYawLimit > 180.0f )
				{
					fReticleYawLimit = 180.0f;
				}
				if ( fReticlePitchLimit > 180.0f )
				{
					fReticlePitchLimit = 180.0f;
				}
			}

			if ( fReticlePitchLimit >= 0.0f )
			{
				// Clamp pitch to within the limits.
				(*pNewAngles)[PITCH] = Clamp ( curAngles[PITCH], m_PlayerViewAngle[PITCH] - fReticlePitchLimit, m_PlayerViewAngle[PITCH] + fReticlePitchLimit );
			}

			// For yaw the concept here is the torso stays within a set number of degrees of the weapon in yaw.
			// However, with drifty tracking systems (e.g. IMUs) the concept of "torso" is hazy.
			// Really it's just a mechanism to turn the view without moving the head - its absolute
			// orientation is not that useful.
			// So... if the mouse is to the right greater than the chosen angle from the view, and then
			// moves more right, it will drag the torso (and thus the view) right, so it stays on the edge of the view.
			// But if it moves left towards the view, it does no dragging.
			// Note that if the mouse does not move, but the view moves, it will NOT drag at all.
			// This allows people to mouse-aim within their view, but also to flick-turn with the mouse,
			// and to flick-glance with the head.
			if ( fReticleYawLimit >= 0.0f )
			{
				float fViewToWeaponYaw = AngleDiff ( curAngles[YAW], m_PlayerViewAngle[YAW] );
				float fWeaponYawMovement = AngleDiff ( curAngles[YAW], oldAngles[YAW] );
				if ( fViewToWeaponYaw > fReticleYawLimit )
				{
					if ( fWeaponYawMovement > 0.0f )
					{
						m_PlayerTorsoAngle[YAW] += fWeaponYawMovement;
					}
				}
				else if ( fViewToWeaponYaw < -fReticleYawLimit )
				{
					if ( fWeaponYawMovement < 0.0f )
					{
						m_PlayerTorsoAngle[YAW] += fWeaponYawMovement;
					}
				}
			}

			// Let every other system know.
			AngleMatrix( *pNewAngles, m_WorldFromWeapon.As3x4() );
			AngleMatrix( m_PlayerTorsoAngle, worldFromTorso.As3x4() );
			// Restore the translation.
			m_WorldFromWeapon.SetTranslation ( vWeaponOrigin );
		}
		break;
	case HMM_SHOOTMOUSE_MOVEFACE:
		{
			(*pNewAngles)[PITCH] = clamp( (*pNewAngles)[PITCH], m_PlayerViewAngle[PITCH]-15.f, m_PlayerViewAngle[PITCH]+15.f );

			float fDiff = AngleDiff( (*pNewAngles)[YAW], m_PlayerViewAngle[YAW] );

			if( fDiff > 15.f )
			{
				(*pNewAngles)[YAW] = AngleNormalize( m_PlayerViewAngle[YAW] + 15.f );
				if( !m_bOverrideTorsoAngle )
					m_PlayerTorsoAngle[ YAW ] += fDiff - 15.f;
			}
			else if( fDiff < -15.f )
			{
				(*pNewAngles)[YAW] = AngleNormalize( m_PlayerViewAngle[YAW] - 15.f );
				if( !m_bOverrideTorsoAngle )
					m_PlayerTorsoAngle[ YAW ] += fDiff + 15.f;
			}
			else
			{
				m_PlayerTorsoAngle[ YAW ] += AngleDiff( curAngles[YAW], oldAngles[YAW] ) /2.f;
			}

			AngleMatrix( *pNewAngles, m_WorldFromWeapon.As3x4() );
			AngleMatrix( m_PlayerTorsoAngle, worldFromTorso.As3x4() );
			// Restore the translation.
			m_WorldFromWeapon.SetTranslation ( vWeaponOrigin );
		}
		break;
	default: Assert ( false ); break;
	}

	// Figure out player motion.
	switch ( m_hmmMovementActual )
	{
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEFACE:
		{
			// The motion passed in is meant to be relative to the face, so jimmy it to be relative to the new weapon aim.
			VMatrix mideyeFromWorld = m_WorldFromMidEye.InverseTR();
			VMatrix newMidEyeFromWeapon = mideyeFromWorld * m_WorldFromWeapon;
			newMidEyeFromWeapon.SetTranslation ( Vector ( 0.0f, 0.0f, 0.0f ) );
			*pNewMotion = newMidEyeFromWeapon * curMotion;
		}
		break;
	case HMM_SHOOTFACE_MOVETORSO:
		{
			// The motion passed in is meant to be relative to the torso, so jimmy it to be relative to the new weapon aim.
			VMatrix torsoFromWorld = worldFromTorso.InverseTR();
			VMatrix newTorsoFromWeapon = torsoFromWorld * m_WorldFromWeapon;
			newTorsoFromWeapon.SetTranslation ( Vector ( 0.0f, 0.0f, 0.0f ) );
			*pNewMotion = newTorsoFromWeapon * curMotion;
		}
		break;
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEMOUSE:
	case HMM_SHOOTMOVELOOKMOUSEFACE:
	case HMM_SHOOTFACE_MOVEFACE:
	case HMM_SHOOTMOUSE_MOVEFACE:
	case HMM_SHOOTMOVEMOUSE_LOOKFACE:
	case HMM_SHOOTMOVELOOKMOUSE:
		// Motion is meant to be relative to the weapon, so we're fine.
		*pNewMotion = curMotion;
		break;
	default: Assert ( false ); break;
	}

	// If the game told us to, recenter the torso yaw to match the weapon
	if ( m_iAlignTorsoAndViewToWeaponCountdown > 0 )
	{
		m_iAlignTorsoAndViewToWeaponCountdown--;

		// figure out the angles from the torso to the head
		QAngle torsoFromHeadAngles;
		MatrixAngles( m_TorsoFromMideye.As3x4(), torsoFromHeadAngles );

		QAngle weaponAngles;
		MatrixAngles( m_WorldFromWeapon.As3x4(), weaponAngles );
		m_PlayerTorsoAngle[ YAW ] = weaponAngles[ YAW ] - torsoFromHeadAngles[ YAW ] ;
		NormalizeAngles( m_PlayerTorsoAngle );
	}

	// remember the motion for stat tracking
	m_PlayerLastMovement = *pNewMotion;



	return true;
}


// --------------------------------------------------------------------
// Purpose: Collects convar and HMD state once a session
// --------------------------------------------------------------------
bool CClientVirtualReality::CollectSessionStartStats( KeyValues *pkvStats )
{
	pkvStats->SetName( "TF2VRSessionDetails" );

	CUtlString sSerialNumber = g_pSourceVR->GetDisplaySerialNumber();
	if( sSerialNumber.IsValid() && !sSerialNumber.IsEmpty() )
	{
		pkvStats->SetString( "SerialNumber", sSerialNumber.Get() );
	}
	CUtlString sModelNumber = g_pSourceVR->GetDisplayModelNumber();
	if( sModelNumber.IsValid() && !sModelNumber.IsEmpty() )
	{
		pkvStats->SetString( "ModelNumberID", sModelNumber.Get() );
	}

	pkvStats->SetFloat( "vr_separation_user_inches", g_pSourceVR->GetUserIPDMM() / 25.4f );
	//pkvStats->SetFloat( "vr_separation_toein_pixels", vr_separation_toein_pixels.GetFloat() );
	//pkvStats->SetInt( "vr_moveaim_mode", vr_moveaim_mode.GetInt() );
	//pkvStats->SetFloat( "vr_moveaim_reticle_yaw_limit", vr_moveaim_reticle_yaw_limit.GetFloat() );
	//pkvStats->SetFloat( "vr_moveaim_reticle_pitch_limit", vr_moveaim_reticle_pitch_limit.GetFloat() );
	//pkvStats->SetInt( "vr_moveaim_mode_zoom", vr_moveaim_mode_zoom.GetInt() );
	//pkvStats->SetFloat( "vr_moveaim_reticle_yaw_limit_zoom", vr_moveaim_reticle_yaw_limit_zoom.GetFloat() );
	//pkvStats->SetFloat( "vr_moveaim_reticle_pitch_limit_zoom", vr_moveaim_reticle_pitch_limit_zoom.GetFloat() );
	//pkvStats->SetFloat( "vr_hud_max_fov", vr_hud_max_fov.GetFloat() );
	//pkvStats->SetFloat( "vr_hud_forward", vr_hud_forward.GetFloat() );
	//pkvStats->SetFloat( "vr_neckmodel_up", vr_neckmodel_up.GetFloat() );
	//pkvStats->SetFloat( "vr_neckmodel_forwards", vr_neckmodel_forwards.GetFloat() );
	//pkvStats->SetInt( "vr_hud_axis_lock_to_world", vr_hud_axis_lock_to_world.GetInt() );

	//pkvStats->SetInt( "vr_ipdtest_left_t", vr_ipdtest_left_t.GetInt() );
	//pkvStats->SetInt( "vr_ipdtest_left_b", vr_ipdtest_left_b.GetInt() );
	//pkvStats->SetInt( "vr_ipdtest_left_i", vr_ipdtest_left_i.GetInt() );
	//pkvStats->SetInt( "vr_ipdtest_left_o", vr_ipdtest_left_o.GetInt() );
	//pkvStats->SetInt( "vr_ipdtest_right_t", vr_ipdtest_right_t.GetInt() );
	//pkvStats->SetInt( "vr_ipdtest_right_b", vr_ipdtest_right_b.GetInt() );
	//pkvStats->SetInt( "vr_ipdtest_right_i", vr_ipdtest_right_i.GetInt() );
	//pkvStats->SetInt( "vr_ipdtest_right_o", vr_ipdtest_right_o.GetInt() );

	return true;
}


// --------------------------------------------------------------------
// Purpose: Collects view stats every so often
// --------------------------------------------------------------------
bool CClientVirtualReality::CollectPeriodicStats( KeyValues *pkvStats )
{
	// maybe we haven't even been called to get tracking data
	if( !m_bMotionUpdated )
		return false;
	m_bMotionUpdated = false;

	uint32 unPeriod = (uint32) vr_stat_sample_period.GetInt();
	if( unPeriod == 0 )
		return false; // periodic stats are turned off

	RTime32 rtCurrent = time(NULL);
	if( rtCurrent == m_rtLastMotionSample && ( rtCurrent - m_rtLastMotionSample ) < unPeriod )
		return false; // it isn't time to report yet

	pkvStats->SetName( "TF2VRMotionSample" );

	pkvStats->SetInt( "SampleTime", rtCurrent );

	Vector vPos;
	QAngle viewAngles;
	MatrixAngles( m_WorldFromMidEye.As3x4(), viewAngles, vPos );

	pkvStats->SetFloat( "LookYaw", viewAngles[YAW] );
	pkvStats->SetFloat( "LookPitch", viewAngles[PITCH] );
	pkvStats->SetFloat( "LookRoll", viewAngles[ROLL] );
	pkvStats->SetFloat( "PositionX", vPos.x );
	pkvStats->SetFloat( "PositionY", vPos.y );
	pkvStats->SetFloat( "PositionZ", vPos.z );

	pkvStats->SetFloat( "VelocityX", m_PlayerLastMovement.x );
	pkvStats->SetFloat( "VelocityY", m_PlayerLastMovement.y );
	pkvStats->SetFloat( "VelocityZ", m_PlayerLastMovement.z );

	QAngle aimAngles;
	MatrixAngles( m_WorldFromWeapon.As3x4(), aimAngles );

	pkvStats->SetFloat( "AimYaw", aimAngles[YAW] );
	pkvStats->SetFloat( "AimPitch", aimAngles[PITCH] );

	m_rtLastMotionSample = rtCurrent;

	return true;
}


// --------------------------------------------------------------------
// Purpose: Returns true if the world is zoomed
// --------------------------------------------------------------------
bool CClientVirtualReality::CurrentlyZoomed()
{
	return ( m_WorldZoomScale != 1.0f );
}


// --------------------------------------------------------------------
// Purpose: Tells the headtracker to keep the torso angle of the player
//			fixed at this point until the game tells us something 
//			different.
// --------------------------------------------------------------------
void CClientVirtualReality::OverrideTorsoTransform( const Vector & position, const QAngle & angles )
{
	if( m_iAlignTorsoAndViewToWeaponCountdown > 0 )
	{
		m_iAlignTorsoAndViewToWeaponCountdown--;

		// figure out the angles from the torso to the head
		QAngle torsoFromHeadAngles;
		MatrixAngles( m_TorsoFromMideye.As3x4(), torsoFromHeadAngles );

		// this is how far off the torso we actually set will need to be to keep the current "forward"
		// vector while the torso angle is being overridden.
		m_OverrideTorsoOffset[ YAW ] = -torsoFromHeadAngles[ YAW ];
	}

	m_bOverrideTorsoAngle = true;
	m_OverrideTorsoAngle = angles + m_OverrideTorsoOffset;

	// overriding pitch and roll isn't allowed to avoid making people sick
	m_OverrideTorsoAngle[ PITCH ] = 0;
	m_OverrideTorsoAngle[ ROLL ] = 0;

	NormalizeAngles( m_OverrideTorsoAngle );
	
	m_PlayerTorsoAngle = m_OverrideTorsoAngle;
}


// --------------------------------------------------------------------
// Purpose: Tells the headtracker to resume using its own notion of 
//			where the torso is pointed.
// --------------------------------------------------------------------
void CClientVirtualReality::CancelTorsoTransformOverride()
{
	m_bOverrideTorsoAngle = false;
}


// --------------------------------------------------------------------
// Purpose: Returns the bounds in world space where the game should 
//			position the HUD.
// --------------------------------------------------------------------
void CClientVirtualReality::GetHUDBounds( Vector *pViewer, Vector *pUL, Vector *pUR, Vector *pLL, Vector *pLR )
{
	Vector vHalfWidth = m_WorldFromHud.GetLeft() * -m_fHudHalfWidth;
	Vector vHalfHeight = m_WorldFromHud.GetUp() * m_fHudHalfHeight;
	Vector vHUDOrigin = m_PlayerViewOrigin + m_WorldFromHud.GetForward() * vr_hud_forward.GetFloat();

	*pViewer = m_PlayerViewOrigin;
	*pUL = vHUDOrigin - vHalfWidth + vHalfHeight;
	*pUR = vHUDOrigin + vHalfWidth + vHalfHeight;
	*pLL = vHUDOrigin - vHalfWidth - vHalfHeight;
	*pLR = vHUDOrigin + vHalfWidth - vHalfHeight;
}


// --------------------------------------------------------------------
// Purpose: Renders the HUD in the world.
// --------------------------------------------------------------------
void CClientVirtualReality::RenderHUDQuad( bool bBlackout, bool bTranslucent )
{
	Vector vHead, vUL, vUR, vLL, vLR;
	GetHUDBounds ( &vHead, &vUL, &vUR, &vLL, &vLR );

	CMatRenderContextPtr pRenderContext( materials );

	{
		IMaterial *mymat = NULL;
		if ( bTranslucent )
		{
			mymat = materials->FindMaterial( "vgui/inworldui", TEXTURE_GROUP_VGUI );
		}
		else
		{
			mymat = materials->FindMaterial( "vgui/inworldui_opaque", TEXTURE_GROUP_VGUI );
		}
		Assert( !mymat->IsErrorMaterial() );

		IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, mymat );

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 );

		meshBuilder.Position3fv (vLR.Base() );
		meshBuilder.TexCoord2f( 0, 1, 1 );
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

		meshBuilder.Position3fv (vLL.Base());
		meshBuilder.TexCoord2f( 0, 0, 1 );
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

		meshBuilder.Position3fv (vUR.Base());
		meshBuilder.TexCoord2f( 0, 1, 0 );
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

		meshBuilder.Position3fv (vUL.Base());
		meshBuilder.TexCoord2f( 0, 0, 0 );
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

		meshBuilder.End();
		pMesh->Draw();
	}

	if( bBlackout )
	{
		Vector vbUL, vbUR, vbLL, vbLR;
		// "Reflect" the HUD bounds through the viewer to find the ones behind the head.
		vbUL = 2 * vHead - vLR;
		vbUR = 2 * vHead - vLL;
		vbLL = 2 * vHead - vUR;
		vbLR = 2 * vHead - vUL;

		IMaterial *mymat = materials->FindMaterial( "vgui/black", TEXTURE_GROUP_VGUI );
		IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, mymat );

		// Tube around the outside.
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 8 );

		meshBuilder.Position3fv (vLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbLR.Base() );
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vLL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbLL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vUL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbUL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vUR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbUR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.End();
		pMesh->Draw();

		// Cap behind the viewer.
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 );

		meshBuilder.Position3fv (vbUR.Base() );
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbUL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbLL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.End();
		pMesh->Draw();
	}
}


// --------------------------------------------------------------------
// Purpose: Gets the amount of zoom to apply
// --------------------------------------------------------------------
float CClientVirtualReality::GetZoomedModeMagnification()
{
	return m_WorldZoomScale * vr_zoom_scope_scale.GetFloat();
}


// --------------------------------------------------------------------
// Purpose: Does some client-side tracking work and then tells headtrack
//			to do its own work.
// --------------------------------------------------------------------
bool CClientVirtualReality::ProcessCurrentTrackingState( float fGameFOV )
{
	// Figure out the current HUD FOV.
	m_fHudHorizontalFov = g_pSourceVR->GetHorizontalFOVDegrees() * vr_hud_display_ratio.GetFloat();
	if( m_fHudHorizontalFov > vr_hud_max_fov.GetFloat() )
	{
		m_fHudHorizontalFov = vr_hud_max_fov.GetFloat();
	}


	m_WorldZoomScale = 1.0f;
	if ( fGameFOV != 0.0f )
	{
		// To compensate for the lack of pixels on most HUDs, let's grow this a bit.
		// Remember that MORE zoom equals LESS fov!
		fGameFOV *= ( 1.0f / vr_zoom_multiplier.GetFloat() );
		fGameFOV = Min ( fGameFOV, 170.0f );

		// The game has overridden the FOV, e.g. because of a sniper scope. So we need to match this view with whatever actual FOV the HUD has.
		float wantedGameTanfov = tanf ( DEG2RAD ( fGameFOV * 0.5f ) );
		// OK, so now in stereo mode, we're going to also draw an overlay, but that overlay usually covers more of the screen (because in a good HMD usually our actual FOV is much wider)
		float overlayActualPhysicalTanfov = tanf ( DEG2RAD ( m_fHudHorizontalFov * 0.5f ) );
		// Therefore... (remembering that a zoom > 1.0 means you zoom *out*)
		m_WorldZoomScale = wantedGameTanfov / overlayActualPhysicalTanfov;
	}

	return g_pSourceVR->SampleTrackingState( fGameFOV, 0.f /* seconds to predict */ );
}


// --------------------------------------------------------------------
// Purpose: Returns the projection matrix to use for the HUD
// --------------------------------------------------------------------
const VMatrix &CClientVirtualReality::GetHudProjectionFromWorld()
{
	// This matrix will transform a world-space position into a homogenous HUD-space vector.
	// So if you divide x+y by w, you will get the position on the HUD in [-1,1] space.
	return m_HudProjectionFromWorld;
}


// --------------------------------------------------------------------
// Purpose: Returns the aim vector relative to the torso
// --------------------------------------------------------------------
void CClientVirtualReality::GetTorsoRelativeAim( Vector *pPosition, QAngle *pAngles )
{
	MatrixAngles( m_TorsoFromMideye.As3x4(), *pAngles, *pPosition );
	pAngles->y += vr_aim_yaw_offset.GetFloat();
}


// --------------------------------------------------------------------
// Purpose: Returns distance of the HUD in front of the eyes.
// --------------------------------------------------------------------
float CClientVirtualReality::GetHUDDistance()
{
	return vr_hud_forward.GetFloat();
}


// --------------------------------------------------------------------
// Purpose: Returns true if the HUD should be rendered into a render 
//			target and then into the world on a quad.
// --------------------------------------------------------------------
bool CClientVirtualReality::ShouldRenderHUDInWorld()
{
	return UseVR() && vr_render_hud_in_world.GetBool();
}


// --------------------------------------------------------------------
// Purpose: Lets headtrack tweak the view model origin and angles to match 
//			aim angles and handle strange viewmode FOV stuff
// --------------------------------------------------------------------
void CClientVirtualReality::OverrideViewModelTransform( Vector & vmorigin, QAngle & vmangles, bool bUseLargeOverride ) 
{
	Vector vForward, vRight, vUp;
	AngleVectors( vmangles, &vForward, &vRight, &vUp );

	float fForward = bUseLargeOverride ? vr_viewmodel_offset_forward_large.GetFloat() : vr_viewmodel_offset_forward.GetFloat();

	vmorigin += vForward * fForward;
	MatrixAngles( m_WorldFromWeapon.As3x4(), vmangles );
}


// --------------------------------------------------------------------
// Purpose: Tells the head tracker to reset the torso position in case
//			we're on a drifty tracker.
// --------------------------------------------------------------------
void CClientVirtualReality::AlignTorsoAndViewToWeapon()
{
	if( !UseVR() )
		return;

	VRTrackerState_t state = g_pSourceVR->GetTrackerState();
	if( state.bWillDriftInYaw )
	{
		m_iAlignTorsoAndViewToWeaponCountdown = 2;
	}
}


// --------------------------------------------------------------------
// Purpose: Lets VR do stuff at the very end of the rendering process
// --------------------------------------------------------------------
void CClientVirtualReality::PostProcessFrame( const vrect_t *SrcRect )
{
	if( !UseVR() )
		return;

	g_pSourceVR->DoDistortionProcessing( SrcRect );

	if ( m_bIpdTestEnabled )
	{
		DrawIpdCalibration ( SrcRect );
	}
}


//-----------------------------------------------------------------------------
// Calibration UI
//-----------------------------------------------------------------------------


// These control the conversion of IPD from pixels to inches.
ConVar vr_ipdtest_interp_ipd_start_pixels ( "vr_ipdtest_interp_ipd_start_pixels", "491.0", 0 );
ConVar vr_ipdtest_interp_ipd_start_inches ( "vr_ipdtest_interp_ipd_start_inches", "2.717", 0 );	// 69mm
ConVar vr_ipdtest_interp_ipd_end_pixels   ( "vr_ipdtest_interp_ipd_end_pixels",   "602.0", 0 );
ConVar vr_ipdtest_interp_ipd_end_inches   ( "vr_ipdtest_interp_ipd_end_inches",   "2.205", 0 );	// 56mm

// These numbers need to be filled in from physical tests. Right now they are placeholder.
ConVar vr_ipdtest_interp_relief_start_pixels ( "vr_ipdtest_interp_relief_start_pixels", "400.0", 0 );
ConVar vr_ipdtest_interp_relief_start_inches ( "vr_ipdtest_interp_relief_start_inches", "0.0", 0 );
ConVar vr_ipdtest_interp_relief_end_pixels   ( "vr_ipdtest_interp_relief_end_pixels",   "600.0", 0 );
ConVar vr_ipdtest_interp_relief_end_inches   ( "vr_ipdtest_interp_relief_end_inches",   "1.0", 0 );




float Interpolate ( float fIn, float fInStart, float fInEnd, float fOutStart, float fOutEnd )
{
	float fLamdba = ( fIn - fInStart ) / ( fInEnd - fInStart );
	float fOut = fOutStart + fLamdba * ( fOutEnd - fOutStart );
	return fOut;
}

void CClientVirtualReality::RecalcEyeCalibration ( TEyeCalibration *p )
{
	int iDisplayWidth, iDisplayHeight;
	bool bSuccess = g_pSourceVR->GetWindowSize ( &iDisplayWidth, &iDisplayHeight );
	Assert ( bSuccess );
	if ( bSuccess )
	{
		// Eye relief.
		// Many ways to take the average eye size. But since the top edge is hard to find (strains the eyes, and there's problems with glasses), let's just use the difference between left and right.
		p->Left.fSizePixels = (float)( p->Left.iIn - p->Left.iOut );
		p->Right.fSizePixels = (float)( p->Right.iIn - p->Right.iOut );
		// ...not that we have any data yet, nor do we know what to do with it if we had it.
		float fLeftInches = Interpolate ( p->Left.fSizePixels,
			vr_ipdtest_interp_relief_start_pixels.GetFloat(),
			vr_ipdtest_interp_relief_end_pixels.GetFloat(),
			vr_ipdtest_interp_relief_start_inches.GetFloat(),
			vr_ipdtest_interp_relief_end_inches.GetFloat() );
		p->Left.fReliefInches = fLeftInches;
		float fRightInches = Interpolate ( p->Right.fSizePixels,
			vr_ipdtest_interp_relief_start_pixels.GetFloat(),
			vr_ipdtest_interp_relief_end_pixels.GetFloat(),
			vr_ipdtest_interp_relief_start_inches.GetFloat(),
			vr_ipdtest_interp_relief_end_inches.GetFloat() );
		p->Right.fReliefInches = fRightInches;

		// Calculate IPD
		// In and Out are both measured from the nearest edge of the display, i.e. the left ones from the left edge, the right ones from the right edge.
		float fLeftMid = (float)( p->Left.iIn + p->Left.iOut ) * 0.5f;
		float fRightMid = (float)( p->Right.iIn + p->Right.iOut ) * 0.5f;
		// An outside value of 0 is the first actual pixel on the outer edge of the display.
		// So if both values are 0, the two lines are (iDisplayWidth-1) apart.
		float fSeparationInPixels = (float)( iDisplayWidth - 1 ) - fLeftMid - fRightMid;
		float fIpdInches = Interpolate ( fSeparationInPixels,
			vr_ipdtest_interp_ipd_start_pixels.GetFloat(),
			vr_ipdtest_interp_ipd_end_pixels.GetFloat(),
			vr_ipdtest_interp_ipd_start_inches.GetFloat(),
			vr_ipdtest_interp_ipd_end_inches.GetFloat() );
		p->fIpdInches = fIpdInches;
		p->fIpdPixels = fSeparationInPixels;
	}
}


void CClientVirtualReality::GetCurrentEyeCalibration ( TEyeCalibration *p )
{
	p->Left.iTop  = vr_ipdtest_left_t.GetInt();
	p->Left.iBot  = vr_ipdtest_left_b.GetInt();
	p->Left.iIn   = vr_ipdtest_left_i.GetInt();
	p->Left.iOut  = vr_ipdtest_left_o.GetInt();
	p->Right.iTop = vr_ipdtest_right_t.GetInt();
	p->Right.iBot = vr_ipdtest_right_b.GetInt();
	p->Right.iIn  = vr_ipdtest_right_i.GetInt();
	p->Right.iOut = vr_ipdtest_right_o.GetInt();
	RecalcEyeCalibration ( p );
	m_IpdTestCurrent = *p;
}

void CClientVirtualReality::SetCurrentEyeCalibration ( TEyeCalibration const &p )
{
	m_IpdTestCurrent = p;
	RecalcEyeCalibration ( &m_IpdTestCurrent );
	g_pSourceVR->SetUserIPDMM( m_IpdTestCurrent.fIpdInches * 25.4f );
	vr_ipdtest_left_t.SetValue  ( m_IpdTestCurrent.Left.iTop  );
	vr_ipdtest_left_b.SetValue  ( m_IpdTestCurrent.Left.iBot  );
	vr_ipdtest_left_i.SetValue  ( m_IpdTestCurrent.Left.iIn   );
	vr_ipdtest_left_o.SetValue  ( m_IpdTestCurrent.Left.iOut  );
	vr_ipdtest_right_t.SetValue ( m_IpdTestCurrent.Right.iTop );
	vr_ipdtest_right_b.SetValue ( m_IpdTestCurrent.Right.iBot );
	vr_ipdtest_right_i.SetValue ( m_IpdTestCurrent.Right.iIn  );
	vr_ipdtest_right_o.SetValue ( m_IpdTestCurrent.Right.iOut );

#ifdef _DEBUG
	Warning ( "                          TBIO: left %d %d %d %d: right %d %d %d %d: %f inches\n",		// Need the spaces to center it so I can read it!
		m_IpdTestCurrent.Left.iTop,
		m_IpdTestCurrent.Left.iBot,
		m_IpdTestCurrent.Left.iIn,
		m_IpdTestCurrent.Left.iOut,
		m_IpdTestCurrent.Right.iTop,
		m_IpdTestCurrent.Right.iBot,
		m_IpdTestCurrent.Right.iIn,
		m_IpdTestCurrent.Right.iOut,
		m_IpdTestCurrent.fIpdInches );
#endif
}

void CClientVirtualReality::SetEyeCalibrationDisplayMisc ( int iEditingNum, bool bVisible )
{
	if( bVisible && !m_bIpdTestEnabled )
	{
		// if we're being shown, read out the current config from the convars
		GetCurrentEyeCalibration ( &m_IpdTestCurrent );
	}

	m_IpdTestControl = iEditingNum;
	m_bIpdTestEnabled = bVisible;
}


void CClientVirtualReality::DrawIpdCalibration ( const vrect_t *SrcRect )
{
	int ControlNum = m_IpdTestControl;
	int WhichEdges = 0;
	bool bShowLeft = false;
	bool bShowRight = false;
	switch ( ControlNum )
	{
	case 0: case 1: case 2: case 3:
		bShowLeft = true;
		WhichEdges = 1 << ControlNum;
		break;
	case 4: case 5: case 6: case 7:
		bShowRight = true;
		WhichEdges = 1 << ControlNum;
		break;
	case 8:
		// Adjust IPD directly.
		bShowLeft = true;
		bShowRight = true;
		WhichEdges = 0x33;
		break;
	case 9:
		// Left relief.
		bShowLeft = true;
		WhichEdges = 0xff;
		break;
	case 10:
		// Right relief.
		bShowRight = true;
		WhichEdges = 0xff;
		break;
	default:
		Assert ( false );
		break;
	}

	CMatRenderContextPtr pRenderContext( materials );

	//pRenderContext->ClearColor4ub ( 0, 0, 0, 0 );
	//pRenderContext->ClearBuffers ( true, true );

	IMaterial *pMaterial = materials->FindMaterial ( "debug/debugtranslucentsinglecolor", TEXTURE_GROUP_OTHER, true );
	pMaterial->ColorModulate( 1.0f, 1.0f, 1.0f );
	pMaterial->AlphaModulate( 1.0f );

	const int Border = 4;
	const int BlueStart = 10;		// Well, it used to be blue, now it isn't.
	const int BlueSize = 30;

	// You want a pure green for the "current" edge so that it has no chromatic aberration smearing (i.e. white is a terrible choice!)
	// The non-current lines can be a different colour because you're not actively tuning them.
#define SET_COLOR1(num) if ( 0 != ( WhichEdges & (1<<(num)) ) ) { pMaterial->ColorModulate( 0.0f, 1.0f, 0.0f ); } else { pMaterial->ColorModulate( 0.25f, 0.25f, 0.25f ); }
#define SET_COLOR2(num) if ( 0 != ( WhichEdges & (1<<(num)) ) ) { pMaterial->ColorModulate( 1.0f, 1.0f, 1.0f ); } else { pMaterial->ColorModulate( 0.0f, 0.0f, 0.0f ); }

	if ( bShowLeft )
	{
		int t = m_IpdTestCurrent.Left.iTop;
		int b = m_IpdTestCurrent.Left.iBot;
		int l = m_IpdTestCurrent.Left.iOut;
		int r = m_IpdTestCurrent.Left.iIn;

		// Render a black rect to enhance contrast.
		pMaterial->ColorModulate( 0.0f, 0.0f, 0.0f );
		pMaterial->AlphaModulate( 1.0f );

		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l-Border-1, t-Border-1, r-l+Border*2+3, Border*2+3, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l-Border-1, b-Border-1, r-l+Border*2+3, Border*2+3, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l-Border-1, t-Border-1, Border*2+3, b-t+Border*2+3, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, r-Border-1, t-Border-1, Border*2+3, b-t+Border*2+3, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );

		int cx = (l+r)/2;
		int cy = (t+b)/2;

		// For each side, draw the line along the side, and also a line "pointing to" it from the middle.
		// Left
		SET_COLOR1(1);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l, t, 1, b-t+1, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		SET_COLOR2(1);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l+BlueStart, cy-1, BlueSize, 3, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );

		// Right
		SET_COLOR1(0);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, r, t, 1, b-t+1, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		SET_COLOR2(0);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, r-BlueStart-BlueSize, cy-1, BlueSize, 3, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );

		// Top
		SET_COLOR1(2);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l, t, r-l+1, 1, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		SET_COLOR2(2);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, cx-1, t+BlueStart, 3, BlueSize, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );

		// Bottom
		SET_COLOR1(3);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l, b, r-l+1, 1, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		SET_COLOR2(3);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, cx-1, b-BlueStart-BlueSize, 3, BlueSize, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
	}

	if ( bShowRight )
	{
		int t = m_IpdTestCurrent.Right.iTop;
		int b = m_IpdTestCurrent.Right.iBot;
		// An outside value of 0 is the first actual pixel on the edge of the display. So if both values are 0, the two lines are (SrcRect->width - 1) apart.
		int l = SrcRect->width - 1 - m_IpdTestCurrent.Right.iIn;
		int r = SrcRect->width - 1 - m_IpdTestCurrent.Right.iOut;

		// Render a black rect to enhance contrast.
		pMaterial->ColorModulate( 0.0f, 0.0f, 0.0f );
		pMaterial->AlphaModulate( 1.0f );

		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l-Border-1, t-Border-1, r-l+Border*2+3, Border*2+3, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l-Border-1, b-Border-1, r-l+Border*2+3, Border*2+3, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l-Border-1, t-Border-1, Border*2+3, b-t+Border*2+3, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, r-Border-1, t-Border-1, Border*2+3, b-t+Border*2+3, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );

		int cx = (l+r)/2;
		int cy = (t+b)/2;

		// For each side, draw the line along the side, and also a line "pointing to" it from the middle.
		// Left
		SET_COLOR1(4);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l, t, 1, b-t+1, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		SET_COLOR2(4);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l+BlueStart, cy-1, BlueSize, 3, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );

		// Right
		SET_COLOR1(5);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, r, t, 1, b-t+1, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		SET_COLOR2(5);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, r-BlueStart-BlueSize, cy-1, BlueSize, 3, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );

		// Top
		SET_COLOR1(6);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l, t, r-l+1, 1, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		SET_COLOR2(6);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, cx-1, t+BlueStart, 3, BlueSize, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );

		// Bottom
		SET_COLOR1(7);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, l, b, r-l+1, 1, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
		SET_COLOR2(7);
		pRenderContext->DrawScreenSpaceRectangle (	pMaterial, cx-1, b-BlueStart-BlueSize, 3, BlueSize, 0.0f, 0.0f, 0.0f, 0.0f, 16, 16 );
	}

	return;
}

