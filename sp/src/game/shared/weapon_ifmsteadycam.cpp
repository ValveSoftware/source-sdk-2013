//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "weapon_ifmsteadycam.h"
#include "in_buttons.h"
#include "usercmd.h"
#include "dt_shared.h"

#ifdef CLIENT_DLL
#include "vgui_controls/Controls.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "vgui/ILocalize.h"
#include "vgui/VGUI.h"
#include "tier1/KeyValues.h"
#include "toolframework/itoolframework.h"
#endif

//-----------------------------------------------------------------------------
// CWeaponIFMSteadyCam tables.
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( WeaponIFMSteadyCam, DT_WeaponIFMSteadyCam )
LINK_ENTITY_TO_CLASS( weapon_ifm_steadycam, CWeaponIFMSteadyCam );
#if !( defined( TF_CLIENT_DLL ) || defined( TF_DLL ) )
PRECACHE_WEAPON_REGISTER( weapon_ifm_steadycam );
#endif

BEGIN_NETWORK_TABLE( CWeaponIFMSteadyCam, DT_WeaponIFMSteadyCam )	
END_NETWORK_TABLE()

#ifdef CLIENT_DLL

BEGIN_PREDICTION_DATA( CWeaponIFMSteadyCam ) 
	DEFINE_PRED_FIELD( m_bIsLocked, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_bInSpringMode, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_bInDirectMode, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_vecOffset, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_hLockTarget, FIELD_EHANDLE, 0 ),
	DEFINE_PRED_FIELD( m_vec2DVelocity, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_vecActualViewOffset, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_vecViewOffset, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_flFOVOffsetY, FIELD_FLOAT, 0 ),
END_PREDICTION_DATA()

#endif


#ifdef GAME_DLL

BEGIN_DATADESC( CWeaponIFMSteadyCam )
	DEFINE_FIELD( m_hLockTarget, FIELD_EHANDLE ),
END_DATADESC()

#endif


//-----------------------------------------------------------------------------
// CWeaponIFMSteadyCam implementation. 
//-----------------------------------------------------------------------------
CWeaponIFMSteadyCam::CWeaponIFMSteadyCam()
{
#ifdef CLIENT_DLL
	m_bIsLocked = false;
	m_bInDirectMode = false;
	m_bInSpringMode = true;
	m_vec2DVelocity.Init();
	m_vecActualViewOffset.Init();
	m_vecViewOffset.Init();
	m_flFOVOffsetY = 0.0f;
	m_vecOffset.Init();
	m_hFont = vgui::INVALID_FONT;
	m_nTextureId = -1;
#endif
}

CWeaponIFMSteadyCam::~CWeaponIFMSteadyCam()
{
#ifdef CLIENT_DLL
	if ( vgui::surface() && m_nTextureId != -1 )
	{
		vgui::surface()->DestroyTextureID( m_nTextureId );
		m_nTextureId = -1;
	}
#endif
}


//-----------------------------------------------------------------------------
//
// Specific methods on the client 
//
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL


//-----------------------------------------------------------------------------
// Computes a matrix given a forward direction
//-----------------------------------------------------------------------------
void CWeaponIFMSteadyCam::MatrixFromForwardDirection( const Vector &vecForward, matrix3x4_t &mat )
{
	// Convert desired to quaternion
	Vector vecLeft( -vecForward.y, vecForward.x, 0.0f );
	if ( VectorNormalize( vecLeft ) < 1e-3 )
	{
		vecLeft.Init( 1.0f, 0.0f, 0.0f );
	}

	Vector vecUp;
	CrossProduct( vecForward, vecLeft, vecUp );
	MatrixInitialize( mat, m_vecRelativePosition, vecForward, vecLeft, vecUp );
}

	
//-----------------------------------------------------------------------------
// Updates the relative orientation of the camera, spring mode
//-----------------------------------------------------------------------------
void CWeaponIFMSteadyCam::ComputeMouseRay( const VMatrix &steadyCamToPlayer, Vector &vecForward )
{
	// Create a ray in steadycam space
	float flMaxD = 1.0f / tan( M_PI * m_flFOV / 360.0f );

	// Remap offsets into normalized space
	int w, h;
	GetViewportSize( w, h );

	float flViewX = ( w != 0 ) ? m_vecViewOffset.x / ( w / 2 ) : 0.0f;
	float flViewY = ( h != 0 ) ? m_vecViewOffset.y / ( h / 2 ) : 0.0f;

	flViewX *= flMaxD;
	flViewY *= flMaxD;
				    
	Vector vecSelectionDir( 1.0f, -flViewX, -flViewY );
	VectorNormalize( vecSelectionDir );

	// Rotate the ray into player coordinates
	Vector3DMultiply( steadyCamToPlayer, vecSelectionDir, vecForward );
}


//-----------------------------------------------------------------------------
// Updates the relative orientation of the camera, spring mode
//-----------------------------------------------------------------------------
void CWeaponIFMSteadyCam::UpdateDirectRelativeOrientation()
{
	// Compute a player to steadycam matrix
	VMatrix steadyCamToPlayer;
	MatrixFromAngles( m_angRelativeAngles, steadyCamToPlayer );
	MatrixSetColumn( steadyCamToPlayer, 3, m_vecRelativePosition );

	// Compute a forward direction
	Vector vecCurrentForward;
	MatrixGetColumn( steadyCamToPlayer, 0, &vecCurrentForward );

	// Before any updating occurs, sample the current 
	// world-space direction of the mouse
	Vector vecDesiredDirection;
	ComputeMouseRay( steadyCamToPlayer, vecDesiredDirection );

	// rebuild a roll-less orientation based on that direction vector
	matrix3x4_t mat;
	MatrixFromForwardDirection( vecDesiredDirection, mat );
	MatrixAngles( mat, m_angRelativeAngles );
	Assert( m_angRelativeAngles.IsValid() );

	m_vecActualViewOffset -= m_vecViewOffset;
	m_vecViewOffset.Init();
}

	
//-----------------------------------------------------------------------------
// Updates the relative orientation of the camera  when locked
//-----------------------------------------------------------------------------
void CWeaponIFMSteadyCam::UpdateLockedRelativeOrientation()
{
	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;
	
	Vector vecDesiredDirection = m_vecOffset;
	CBaseEntity *pLock = m_hLockTarget.Get();
	if ( pLock )
	{
		vecDesiredDirection += pLock->GetAbsOrigin();
	}

	Vector vecAbsOrigin;
	QAngle angAbsRotation;
	ComputeAbsCameraTransform( vecAbsOrigin, angAbsRotation );
	vecDesiredDirection -= vecAbsOrigin;
	VectorNormalize( vecDesiredDirection );

	matrix3x4_t mat;
	MatrixFromForwardDirection( vecDesiredDirection, mat );
	MatrixAngles( mat, m_angRelativeAngles );
}


//-----------------------------------------------------------------------------
// Updates the relative orientation of the camera
//-----------------------------------------------------------------------------
static ConVar ifm_steadycam_rotaterate( "ifm_steadycam_rotaterate", "60", FCVAR_ARCHIVE );
static ConVar ifm_steadycam_zoomspeed( "ifm_steadycam_zoomspeed", "1.0", FCVAR_ARCHIVE );
static ConVar ifm_steadycam_zoomdamp( "ifm_steadycam_zoomdamp", "0.95", FCVAR_ARCHIVE );
static ConVar ifm_steadycam_armspeed( "ifm_steadycam_armspeed", "0.5", FCVAR_ARCHIVE );
static ConVar ifm_steadycam_rotatedamp( "ifm_steadycam_rotatedamp", "0.95", FCVAR_ARCHIVE );
static ConVar ifm_steadycam_mousefactor( "ifm_steadycam_mousefactor", "1.0", FCVAR_ARCHIVE );
static ConVar ifm_steadycam_mousepower( "ifm_steadycam_mousepower", "1.0", FCVAR_ARCHIVE );

void CWeaponIFMSteadyCam::UpdateRelativeOrientation()
{
	if ( m_bIsLocked )
		return;

	if ( m_bInDirectMode )
	{
		UpdateDirectRelativeOrientation();
		return;
	}

	if ( ( m_vecViewOffset.x == 0.0f ) && ( m_vecViewOffset.y == 0.0f ) )
		return;

	// Compute a player to steadycam matrix
	VMatrix steadyCamToPlayer;
	MatrixFromAngles( m_angRelativeAngles, steadyCamToPlayer );
	MatrixSetColumn( steadyCamToPlayer, 3, m_vecRelativePosition );

	Vector vecCurrentForward;
	MatrixGetColumn( steadyCamToPlayer, 0, &vecCurrentForward );

	// Create a ray in steadycam space
	float flMaxD = 1.0f / tan( M_PI * m_flFOV / 360.0f );

	// Remap offsets into normalized space
	float flViewX = m_vecViewOffset.x / ( 384 / 2 );
	float flViewY = m_vecViewOffset.y / ( 288 / 2 );

	flViewX *= flMaxD * ifm_steadycam_mousefactor.GetFloat();
	flViewY *= flMaxD * ifm_steadycam_mousefactor.GetFloat();
				    
	Vector vecSelectionDir( 1.0f, -flViewX, -flViewY );
	VectorNormalize( vecSelectionDir );

	// Rotate the ray into player coordinates
	Vector vecDesiredDirection;
	Vector3DMultiply( steadyCamToPlayer, vecSelectionDir, vecDesiredDirection );

	float flDot = DotProduct( vecDesiredDirection, vecCurrentForward );
	flDot = clamp( flDot, -1.0f, 1.0f );
	float flAngle = 180.0f * acos( flDot ) / M_PI;
	if ( flAngle < 1e-3 )
	{
		matrix3x4_t mat;
		MatrixFromForwardDirection( vecDesiredDirection, mat );
		MatrixAngles( mat, m_angRelativeAngles );
		return;
	}

	Vector vecAxis;
	CrossProduct( vecCurrentForward, vecDesiredDirection, vecAxis );
	VectorNormalize( vecAxis );
	
	float flRotateRate = ifm_steadycam_rotaterate.GetFloat();
	if ( flRotateRate < 1.0f )
	{
		flRotateRate = 1.0f;
	}

	float flRateFactor = flAngle / flRotateRate;
	flRateFactor *= flRateFactor * flRateFactor;
	float flRate = flRateFactor * 30.0f;
	float flMaxAngle = gpGlobals->frametime * flRate;
	flAngle = clamp( flAngle, 0.0f, flMaxAngle );

	Vector vecNewForard;
	VMatrix rotation;
	MatrixBuildRotationAboutAxis( rotation, vecAxis, flAngle ); 
	Vector3DMultiply( rotation, vecCurrentForward, vecNewForard );

	matrix3x4_t mat;
	MatrixFromForwardDirection( vecNewForard, mat );
	MatrixAngles( mat, m_angRelativeAngles );

	Assert( m_angRelativeAngles.IsValid() );
}
	
	
//-----------------------------------------------------------------------------
// Toggles to springy camera
//-----------------------------------------------------------------------------
void CWeaponIFMSteadyCam::ToggleDirectMode()
{
	m_vecViewOffset.Init();
	m_vecActualViewOffset.Init();
	m_vec2DVelocity.Init();
	m_bInDirectMode = !m_bInDirectMode;
}

	
//-----------------------------------------------------------------------------
// Targets the camera to always look at a point
//-----------------------------------------------------------------------------
void CWeaponIFMSteadyCam::LockCamera()
{
	m_vecViewOffset.Init();
	m_vecActualViewOffset.Init();
	m_vec2DVelocity.Init();

	m_bIsLocked = !m_bIsLocked;
	if ( !m_bIsLocked )
	{
		UpdateLockedRelativeOrientation();
		return;
	}

	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;
	
	Vector vTraceStart, vTraceEnd, vTraceDir;
	QAngle angles;
	BaseClass::ComputeAbsCameraTransform( vTraceStart, angles );
	AngleVectors( angles, &vTraceDir );
	VectorMA( vTraceStart, 10000.0f, vTraceDir, vTraceEnd);

	trace_t tr;
	UTIL_TraceLine( vTraceStart, vTraceEnd, MASK_ALL, GetPlayerOwner(), COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction == 1.0f )
	{
		m_bIsLocked = false;
		UpdateLockedRelativeOrientation();
		return;
	}

	m_hLockTarget = tr.m_pEnt;
	m_vecOffset = tr.endpos;
	if ( tr.m_pEnt )
	{
		m_vecOffset -= tr.m_pEnt->GetAbsOrigin();
	}
}


//-----------------------------------------------------------------------------
// Gets the abs orientation of the camera
//-----------------------------------------------------------------------------
void CWeaponIFMSteadyCam::ComputeAbsCameraTransform( Vector &vecAbsOrigin, QAngle &angAbsRotation )
{
	CBaseEntity *pLock = m_bIsLocked ? m_hLockTarget.Get() : NULL;
	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( !pLock || !pPlayer )
	{
		BaseClass::ComputeAbsCameraTransform( vecAbsOrigin, angAbsRotation );
		return;
	}
	
	Vector vecDesiredDirection = m_vecOffset;
	if ( pLock )
	{
		vecDesiredDirection += pLock->GetAbsOrigin();
	}

	BaseClass::ComputeAbsCameraTransform( vecAbsOrigin, angAbsRotation );
	vecDesiredDirection -= vecAbsOrigin;
	VectorNormalize( vecDesiredDirection );

	matrix3x4_t mat;
	MatrixFromForwardDirection( vecDesiredDirection, mat );
	MatrixAngles( mat, angAbsRotation );
}


//-----------------------------------------------------------------------------
// Computes the view offset from the actual view offset
//-----------------------------------------------------------------------------
static ConVar ifm_steadycam_2dspringconstant( "ifm_steadycam_2dspringconstant", "33.0", FCVAR_ARCHIVE );
static ConVar ifm_steadycam_2ddragconstant( "ifm_steadycam_2ddragconstant", "11.0", FCVAR_ARCHIVE );

void CWeaponIFMSteadyCam::ComputeViewOffset()
{
	// Update 2D spring
	if ( !m_bInSpringMode )
	{
		m_vecViewOffset = m_vecActualViewOffset;
		return;
	}

	Vector2D dir;
	Vector2DSubtract( m_vecViewOffset.AsVector2D(), m_vecActualViewOffset.AsVector2D(), dir );
	float flDist = Vector2DNormalize( dir );

	Vector2D vecForce;
	Vector2DMultiply( dir, -flDist * ifm_steadycam_2dspringconstant.GetFloat(), vecForce );
	Vector2DMA( vecForce, -ifm_steadycam_2ddragconstant.GetFloat(), m_vec2DVelocity.AsVector2D(), vecForce ); 

	Vector2DMA( m_vecViewOffset.AsVector2D(), gpGlobals->frametime, m_vec2DVelocity.AsVector2D(), m_vecViewOffset.AsVector2D() ); 
	Vector2DMA( m_vec2DVelocity.AsVector2D(), gpGlobals->frametime, vecForce, m_vec2DVelocity.AsVector2D() ); 
}


//-----------------------------------------------------------------------------
// Camera control
//-----------------------------------------------------------------------------
static ConVar ifm_steadycam_noise( "ifm_steadycam_noise", "0.0", FCVAR_ARCHIVE | FCVAR_REPLICATED );
static ConVar ifm_steadycam_sensitivity( "ifm_steadycam_sensitivity", "1.0", FCVAR_ARCHIVE | FCVAR_REPLICATED );

void CWeaponIFMSteadyCam::ItemPostFrame()
{
	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;
	
	float flSensitivity = ifm_steadycam_sensitivity.GetFloat();

	Vector2D vecOldActualViewOffset = m_vecActualViewOffset.AsVector2D();
	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		const CUserCmd *pUserCmd = pPlayer->GetCurrentUserCommand();
		m_vecActualViewOffset.x += pUserCmd->mousedx * flSensitivity;
		m_vecActualViewOffset.y += pUserCmd->mousedy * flSensitivity;
	}
	else
	{
		if ( !m_bIsLocked && !m_bInDirectMode )
		{
			float flDamp = ifm_steadycam_rotatedamp.GetFloat();
			m_vecActualViewOffset.x *= flDamp;
			m_vecActualViewOffset.y *= flDamp;
		}
	}
	
	// Add noise
	if ( !m_bIsLocked )
	{
		float flNoise = ifm_steadycam_noise.GetFloat();
		if ( flNoise > 0.0f )
		{
			CUniformRandomStream stream;
			stream.SetSeed( (int)(gpGlobals->curtime * 100) );

			CGaussianRandomStream gauss( &stream );
			float dx = gauss.RandomFloat( 0.0f, flNoise );
			float dy = gauss.RandomFloat( 0.0f, flNoise );

			m_vecActualViewOffset.x += dx;
			m_vecActualViewOffset.y += dy;
		}
	}

	ComputeViewOffset();

	if ( pPlayer->m_nButtons & IN_ZOOM )
	{
		const CUserCmd *pUserCmd = pPlayer->GetCurrentUserCommand();
		m_flFOVOffsetY += pUserCmd->mousedy * flSensitivity;
	}
	else
	{
		float flDamp = ifm_steadycam_zoomdamp.GetFloat();
		m_flFOVOffsetY *= flDamp;
	}			    
	m_flFOV += m_flFOVOffsetY * ifm_steadycam_zoomspeed.GetFloat() / 1000.0f;
	m_flFOV = clamp( m_flFOV, 0.5f, 160.0f ); 

	if ( pPlayer->m_nButtons & IN_WALK )
	{
		const CUserCmd *pUserCmd = pPlayer->GetCurrentUserCommand();
		m_flArmLength -= ifm_steadycam_armspeed.GetFloat() * pUserCmd->mousedy;
	}

	if ( pPlayer->GetImpulse() == 87 )
	{
		ToggleDirectMode();
	}

	if ( pPlayer->GetImpulse() == 89 )
	{
		m_bInSpringMode = !m_bInSpringMode;
	}

	if ( pPlayer->m_afButtonPressed & IN_USE )
	{
		LockCamera();
	}

	if ( pPlayer->m_afButtonPressed & IN_ATTACK2 )
	{
		m_bFullScreen = !m_bFullScreen;
	}

	if ( pPlayer->GetImpulse() == 88 )
	{
		// Make the view angles exactly match the player
		m_vecViewOffset.Init();
		m_vecActualViewOffset.Init();
		m_vecOffset.Init();
		m_vec2DVelocity.Init();
		m_hLockTarget.Set( NULL );
		m_flArmLength = 0.0f;
		if ( m_bIsLocked )
		{
			LockCamera();
		}
		m_angRelativeAngles = pPlayer->EyeAngles();
		m_flFOV = pPlayer->GetFOV();
	}

	UpdateRelativeOrientation();
	TransmitRenderInfo();
}


//-----------------------------------------------------------------------------
// Records the state for the IFM
//-----------------------------------------------------------------------------
void CWeaponIFMSteadyCam::GetToolRecordingState( KeyValues *msg )
{
	BaseClass::GetToolRecordingState( msg );

	static CameraRecordingState_t state;
	state.m_flFOV = m_flFOV;
	ComputeAbsCameraTransform( state.m_vecEyePosition, state.m_vecEyeAngles );
	msg->SetPtr( "camera", &state );
}


//-----------------------------------------------------------------------------
// Slams view angles if the mouse is down
//-----------------------------------------------------------------------------
void CWeaponIFMSteadyCam::CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles )
{
	BaseClass::CreateMove( flInputSampleTime, pCmd, vecOldViewAngles );
	
	// Block angular movement when IN_ATTACK is pressed
	if ( pCmd->buttons & (IN_ATTACK | IN_WALK | IN_ZOOM) )
	{
		VectorCopy( vecOldViewAngles, pCmd->viewangles );
	}
}

	
//-----------------------------------------------------------------------------
// Purpose: Draw the weapon's crosshair
//-----------------------------------------------------------------------------
void CWeaponIFMSteadyCam::DrawArmLength( int x, int y, int w, int h, Color clr )
{
	// Draw a readout for the arm length
	if ( m_hFont == vgui::INVALID_FONT )
	{
		vgui::HScheme hScheme = vgui::scheme()->GetScheme( "ClientScheme" );
		vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( hScheme );
		m_hFont	= pScheme->GetFont("DefaultVerySmall", false );	
		Assert( m_hFont != vgui::INVALID_FONT );
	}
	
	// Create our string
	char szString[256];
	Q_snprintf( szString, sizeof(szString), "Arm Length: %.2f\n", m_flArmLength );

	// Convert it to localize friendly unicode
	wchar_t wcString[256];
	g_pVGuiLocalize->ConvertANSIToUnicode( szString, wcString, sizeof(wcString) );

	int tw, th;
	vgui::surface()->GetTextSize( m_hFont, wcString, tw, th );

	vgui::surface()->DrawSetTextFont( m_hFont ); // set the font	
	vgui::surface()->DrawSetTextColor( clr ); // white
	vgui::surface()->DrawSetTextPos( x + w - tw - 10, y + 10 ); // x,y position

	vgui::surface()->DrawPrintText( wcString, wcslen(wcString) ); // print text
}


//-----------------------------------------------------------------------------
// Purpose: Draw the FOV
//-----------------------------------------------------------------------------
void CWeaponIFMSteadyCam::DrawFOV( int x, int y, int w, int h, Color clrEdges, Color clrTriangle )
{
	if ( m_nTextureId == -1 )
	{
		m_nTextureId = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_nTextureId, "vgui/white", true, false );
	}

	// This is the fov
	int nSize = 30;
	int fx = x + w - 10 - nSize;
	int fy = y + h - 10;
	int fh = nSize * cos( M_PI * m_flFOV / 360.0f );
	int fw = nSize * sin( M_PI * m_flFOV / 360.0f );
		  
	vgui::Vertex_t v[3];
	v[0].m_Position.Init( fx, fy );
	v[0].m_TexCoord.Init( 0.0f, 0.0f );
	v[1].m_Position.Init( fx-fw, fy-fh );
	v[1].m_TexCoord.Init( 0.0f, 0.0f );
	v[2].m_Position.Init( fx+fw, fy-fh );
	v[2].m_TexCoord.Init( 0.0f, 0.0f );

	vgui::surface()->DrawSetTexture( m_nTextureId );
	vgui::surface()->DrawSetColor( clrTriangle );
	vgui::surface()->DrawTexturedPolygon( 3, v );

	vgui::surface()->DrawSetColor( clrEdges );
	vgui::surface()->DrawLine( fx, fy, fx - fw, fy - fh );
	vgui::surface()->DrawLine( fx, fy, fx + fw, fy - fh );
}


//-----------------------------------------------------------------------------
// Purpose: Draw the weapon's crosshair
//-----------------------------------------------------------------------------
void CWeaponIFMSteadyCam::DrawCrosshair( void )
{
	BaseClass::DrawCrosshair();

	int x, y, w, h;
	GetOverlayBounds( x, y, w, h );

	// Draw the targeting zone around the crosshair
	int r, g, b, a;
	gHUD.m_clrYellowish.GetColor( r, g, b, a );
		 
	Color gray( 255, 255, 255, 192 );
	Color light( r, g, b, 255 );
	Color dark( r, g, b, 128 );
	Color red( 255, 0, 0, 128 );
	
	DrawArmLength( x, y, w, h, light );
	DrawFOV( x, y, w, h, light, dark );

	int cx, cy;
	cx = x + ( w / 2 );
	cy = y + ( h / 2 );

	// This is the crosshair
	vgui::surface()->DrawSetColor( gray );
	vgui::surface()->DrawFilledRect( cx-10, cy-1, cx-3, cy+1 );
	vgui::surface()->DrawFilledRect( cx+3, cy-1, cx+10, cy+1 );
	vgui::surface()->DrawFilledRect( cx-1, cy-10, cx+1, cy-3 );
	vgui::surface()->DrawFilledRect( cx-1, cy+3, cx+1, cy+10 );

	// This is the yellow aiming dot
	if ( ( m_vecViewOffset.x != 0.0f ) || ( m_vecViewOffset.y != 0.0f ) )
	{
		int ax, ay;
		ax = cx + m_vecViewOffset.x;
		ay = cy + m_vecViewOffset.y;
		vgui::surface()->DrawSetColor( light );
		vgui::surface()->DrawFilledRect( ax-2, ay-2, ax+2, ay+2 );
	}

	// This is the red actual dot
	if ( ( m_vecActualViewOffset.x != 0.0f ) || ( m_vecActualViewOffset.y != 0.0f ) )
	{
		int ax, ay;
		ax = cx + m_vecActualViewOffset.x;
		ay = cy + m_vecActualViewOffset.y;
		vgui::surface()->DrawSetColor( red );
		vgui::surface()->DrawFilledRect( ax-2, ay-2, ax+2, ay+2 );
	}

	// This is the purple fov dot
	if ( m_flFOVOffsetY != 0.0f )
	{
		Color purple( 255, 0, 255, 255 );
		int vy = cy + m_flFOVOffsetY;
		vgui::surface()->DrawSetColor( purple );
		vgui::surface()->DrawFilledRect( cx-2, vy-2, cx+2, vy+2 );
	}
}

#endif // CLIENT_DLL


//-----------------------------------------------------------------------------
//
// Specific methods on the server 
//
//-----------------------------------------------------------------------------
#ifdef GAME_DLL
	
void CWeaponIFMSteadyCam::ItemPostFrame()
{
}

#endif   // GAME_DLL



