//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hltvcamera.h"
#include "cdll_client_int.h"
#include "util_shared.h"
#include "prediction.h"
#include "movevars_shared.h"
#include "in_buttons.h"
#include "text_message.h"
#include "vgui_controls/Controls.h"
#include "vgui/ILocalize.h"
#include "vguicenterprint.h"
#include "game/client/iviewport.h"
#include <KeyValues.h>

#ifdef CSTRIKE_DLL
	#include "c_cs_player.h"
#endif

ConVar spec_autodirector( "spec_autodirector", "1", FCVAR_CLIENTDLL | FCVAR_CLIENTCMD_CAN_EXECUTE, "Auto-director chooses best view modes while spectating" );

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CHASE_CAM_DISTANCE		96.0f
#define WALL_OFFSET				6.0f

static Vector WALL_MIN(-WALL_OFFSET,-WALL_OFFSET,-WALL_OFFSET);
static Vector WALL_MAX(WALL_OFFSET,WALL_OFFSET,WALL_OFFSET);
static const ConVar	*tv_transmitall = NULL;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// converts all '\r' characters to '\n', so that the engine can deal with the properly
// returns a pointer to str
static wchar_t* ConvertCRtoNL( wchar_t *str )
{
	for ( wchar_t *ch = str; *ch != 0; ch++ )
		if ( *ch == L'\r' )
			*ch = L'\n';
	return str;
}

static C_HLTVCamera s_HLTVCamera;

C_HLTVCamera *HLTVCamera() { return &s_HLTVCamera; }

C_HLTVCamera::C_HLTVCamera()
{
	Reset();

	m_nNumSpectators = 0;
	m_szTitleText[0] = 0;
}

C_HLTVCamera::~C_HLTVCamera()
{

}

void C_HLTVCamera::Init()
{
	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "hltv_cameraman" );
	ListenForGameEvent( "hltv_fixed" );
	ListenForGameEvent( "hltv_chase" );
	ListenForGameEvent( "hltv_message" );
	ListenForGameEvent( "hltv_title" );
	ListenForGameEvent( "hltv_status" );
	
	Reset();

	m_nNumSpectators = 0;
	m_szTitleText[0] = 0;

	// get a handle to the engine convar
	tv_transmitall = cvar->FindVar( "tv_transmitall" );
}

void C_HLTVCamera::Reset()
{
	m_nCameraMode = OBS_MODE_FIXED;
	m_iCameraMan  = 0;
	m_iTraget1 = m_iTraget2 = 0;
	m_flFOV = 90;
	m_flDistance = m_flLastDistance = 96.0f;
	m_flInertia = 3.0f;
	m_flPhi = 0;
	m_flTheta = 0;
	m_flOffset = 0;
	m_bEntityPacketReceived = false;

	m_vCamOrigin.Init();
	m_aCamAngle.Init();

	m_LastCmd.Reset();
	m_vecVelocity.Init();
}

void C_HLTVCamera::CalcChaseCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
	bool bManual = !spec_autodirector.GetBool();	// chase camera controlled manually
	
 	Vector targetOrigin1, targetOrigin2, cameraOrigin, forward;

 	if ( m_iTraget1 == 0 )
		return;

	// get primary target, also translates to ragdoll
	C_BaseEntity *target1 = GetPrimaryTarget();

 	if ( !target1 ) 
		return;
	
	if ( target1->IsAlive() && target1->IsDormant() )
		return;

	targetOrigin1 = target1->GetRenderOrigin();

	if ( !target1->IsAlive() )
	{
		targetOrigin1 += VEC_DEAD_VIEWHEIGHT;
	}
	else if ( target1->GetFlags() & FL_DUCKING )
	{
		targetOrigin1 += VEC_DUCK_VIEW;
	}
	else
	{
		targetOrigin1 += VEC_VIEW;
	}

	// get secondary target if set
	C_BaseEntity *target2 = NULL;

	if ( m_iTraget2 > 0 && (m_iTraget2 != m_iTraget1) && !bManual )
	{
		target2 = ClientEntityList().GetBaseEntity( m_iTraget2 );

		// if target is out PVS and not dead, it's not valid
		if ( target2 && target2->IsDormant() && target2->IsAlive() )
			target2 = NULL;

		if ( target2 )
		{
			targetOrigin2 = target2->GetRenderOrigin();

			if ( !target2->IsAlive() )
			{
				targetOrigin2 += VEC_DEAD_VIEWHEIGHT;
			}
			else if ( target2->GetFlags() & FL_DUCKING )
			{
				targetOrigin2 += VEC_DUCK_VIEW;
			}
			else
			{
				targetOrigin2 += VEC_VIEW;
			}
		}
	}

		// apply angle offset & smoothing
	QAngle angleOffset(  m_flPhi, m_flTheta, 0 );
	QAngle cameraAngles = m_aCamAngle;

	if ( bManual )
	{
		// let spectator choose the view angles
 		engine->GetViewAngles( cameraAngles );
	}
	else if ( target2 )
	{
		// look into direction of second target
 		forward = targetOrigin2 - targetOrigin1;
        VectorAngles( forward, cameraAngles );
        cameraAngles.z = 0; // no ROLL
	}
	else if ( m_iTraget2 == 0 || m_iTraget2 == m_iTraget1)
	{
		// look into direction where primary target is looking
		cameraAngles = target1->EyeAngles();
		cameraAngles.x = 0; // no PITCH
		cameraAngles.z = 0; // no ROLL
	}
	else
	{
		// target2 is missing, just keep angelsm, reset offset
		angleOffset.Init();
	}

	if ( !bManual )
	{
		if ( !target1->IsAlive() )
		{
			angleOffset.x = 15;
		}

		cameraAngles += angleOffset;
	}

	AngleVectors( cameraAngles, &forward );

	VectorNormalize( forward );

	// calc optimal camera position
	VectorMA(targetOrigin1, -m_flDistance, forward, cameraOrigin );

 	targetOrigin1.z += m_flOffset; // add offset

	// clip against walls
  	trace_t trace;
	C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
	UTIL_TraceHull( targetOrigin1, cameraOrigin, WALL_MIN, WALL_MAX, MASK_SOLID, target1, COLLISION_GROUP_NONE, &trace );
	C_BaseEntity::PopEnableAbsRecomputations();

  	float dist = VectorLength( trace.endpos -  targetOrigin1 );

	// grow distance by 32 unit a second
  	m_flLastDistance += gpGlobals->frametime * 32.0f; 

  	if ( dist > m_flLastDistance )
	{
		VectorMA(targetOrigin1, -m_flLastDistance, forward, cameraOrigin );
	}
 	else
	{
		cameraOrigin = trace.endpos;
		m_flLastDistance = dist;
	}
	
  	if ( target2 )
	{
		// if we have 2 targets look at point between them
		forward = (targetOrigin1+targetOrigin2)/2 - cameraOrigin;
 		QAngle angle;
		VectorAngles( forward, angle );
		cameraAngles.y = angle.y;
		
		NormalizeAngles( cameraAngles );
		cameraAngles.x = clamp( cameraAngles.x, -60.f, 60.f );

		SmoothCameraAngle( cameraAngles );
	}
	else
	{
		SetCameraAngle( cameraAngles );
	}
 	
	VectorCopy( cameraOrigin, m_vCamOrigin );
	VectorCopy( m_aCamAngle, eyeAngles );
	VectorCopy( m_vCamOrigin, eyeOrigin );
}

int C_HLTVCamera::GetMode()
{
	if ( m_iCameraMan > 0 )
	{
		C_BasePlayer *pCameraMan = UTIL_PlayerByIndex( m_iCameraMan );

		if ( pCameraMan )
			return pCameraMan->GetObserverMode();
	}

	return m_nCameraMode;	
}

C_BaseEntity* C_HLTVCamera::GetPrimaryTarget()
{
	if ( m_iCameraMan > 0 )
	{
		C_BasePlayer *pCameraMan = UTIL_PlayerByIndex( m_iCameraMan );
		
		if ( pCameraMan )
		{
			return pCameraMan->GetObserverTarget();
		}
	}

	if ( m_iTraget1 <= 0 )
		return NULL;

	C_BaseEntity* target = ClientEntityList().GetEnt( m_iTraget1 );

	return target;
}

C_BaseEntity *C_HLTVCamera::GetCameraMan()
{
	return ClientEntityList().GetEnt( m_iCameraMan );
}

void C_HLTVCamera::CalcInEyeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
	C_BasePlayer *pPlayer = UTIL_PlayerByIndex( m_iTraget1 );

	if ( !pPlayer )
		return;

	if ( !pPlayer->IsAlive() )
	{
		// if dead, show from 3rd person
		CalcChaseCamView( eyeOrigin, eyeAngles, fov );
		return;
	}

	m_aCamAngle	= pPlayer->EyeAngles();
	m_vCamOrigin = pPlayer->GetAbsOrigin();
	m_flFOV = pPlayer->GetFOV();

	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		m_vCamOrigin += VEC_DUCK_VIEW;
	}
	else
	{
		m_vCamOrigin += VEC_VIEW;
	}

	eyeOrigin = m_vCamOrigin;
	eyeAngles = m_aCamAngle;
	fov = m_flFOV;

	pPlayer->CalcViewModelView( eyeOrigin, eyeAngles);

	C_BaseViewModel *pViewModel = pPlayer->GetViewModel( 0 );

	if ( pViewModel )
	{
		Assert( pViewModel->GetOwner() == pPlayer );
		pViewModel->UpdateVisibility();
	}
}

void C_HLTVCamera::Accelerate( Vector& wishdir, float wishspeed, float accel )
{
	float addspeed, accelspeed, currentspeed;

	// See if we are changing direction a bit
	currentspeed =m_vecVelocity.Dot(wishdir);

	// Reduce wishspeed by the amount of veer.
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of acceleration.
	accelspeed = accel * gpGlobals->frametime * wishspeed;

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust velocity.
	for (int i=0 ; i<3 ; i++)
	{
		m_vecVelocity[i] += accelspeed * wishdir[i];	
	}
}


// movement code is a copy of CGameMovement::FullNoClipMove()
void C_HLTVCamera::CalcRoamingView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	// only if PVS isn't locked by auto-director
	if ( !IsPVSLocked() )
	{

		Vector wishvel;
		Vector forward, right, up;
		Vector wishdir;
		float wishspeed;
		float factor = sv_specspeed.GetFloat();
		float maxspeed = sv_maxspeed.GetFloat() * factor;

		AngleVectors ( m_LastCmd.viewangles, &forward, &right, &up);  // Determine movement angles

		if ( m_LastCmd.buttons & IN_SPEED )
		{
			factor /= 2.0f;
		}

		// Copy movement amounts
		float fmove = m_LastCmd.forwardmove * factor;
		float smove = m_LastCmd.sidemove * factor;

		VectorNormalize (forward);  // Normalize remainder of vectors
		VectorNormalize (right);    // 

		for (int i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
			wishvel[i] = forward[i]*fmove + right[i]*smove;
		wishvel[2] += m_LastCmd.upmove * factor;

		VectorCopy (wishvel, wishdir);   // Determine magnitude of speed of move
		wishspeed = VectorNormalize(wishdir);

		//
		// Clamp to server defined max speed
		//
		if (wishspeed > maxspeed )
		{
			VectorScale (wishvel, maxspeed/wishspeed, wishvel);
			wishspeed = maxspeed;
		}

		if ( sv_specaccelerate.GetFloat() > 0.0 )
		{
			// Set move velocity
			Accelerate ( wishdir, wishspeed, sv_specaccelerate.GetFloat() );

			float spd = VectorLength( m_vecVelocity );
			if (spd < 1.0f)
			{
				m_vecVelocity.Init();
			}
			else
			{
				// Bleed off some speed, but if we have less than the bleed
				//  threshold, bleed the threshold amount.
				float control = (spd < maxspeed/4.0) ? maxspeed/4.0 : spd;

				float friction = sv_friction.GetFloat();

				// Add the amount to the drop amount.
				float drop = control * friction * gpGlobals->frametime;

				// scale the velocity
				float newspeed = spd - drop;
				if (newspeed < 0)
					newspeed = 0;

				// Determine proportion of old speed we are using.
				newspeed /= spd;
				VectorScale( m_vecVelocity, newspeed, m_vecVelocity );
			}
		}
		else
		{
			VectorCopy( wishvel, m_vecVelocity );
		}

		// Just move ( don't clip or anything )
		VectorMA( m_vCamOrigin, gpGlobals->frametime, m_vecVelocity, m_vCamOrigin );
		
		// get camera angle directly from engine
		 engine->GetViewAngles( m_aCamAngle );

		// Zero out velocity if in noaccel mode
		if ( sv_specaccelerate.GetFloat() < 0.0f )
		{
			m_vecVelocity.Init();
		}
	}

	eyeOrigin = m_vCamOrigin;
	eyeAngles = m_aCamAngle;
	fov = m_flFOV;
}

void C_HLTVCamera::CalcFixedView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	eyeOrigin = m_vCamOrigin;
	eyeAngles = m_aCamAngle;
	fov = m_flFOV;

	if ( m_iTraget1 == 0 )
		return;

 	C_BaseEntity * target = ClientEntityList().GetBaseEntity( m_iTraget1 );
	
	if ( target && target->IsAlive() )
	{
		// if we're chasing a target, change viewangles
		QAngle angle;
		VectorAngles( (target->GetAbsOrigin()+VEC_VIEW) - m_vCamOrigin, angle );
		SmoothCameraAngle( angle );
	}
}

void C_HLTVCamera::PostEntityPacketReceived()
{
	m_bEntityPacketReceived = true;
}

void C_HLTVCamera::FixupMovmentParents()
{
	// Find resource zone
	
	for (	ClientEntityHandle_t e = ClientEntityList().FirstHandle();
			e != ClientEntityList().InvalidHandle(); e = ClientEntityList().NextHandle( e ) )
	{
		C_BaseEntity *ent = C_BaseEntity::Instance( e );

		if ( !ent )
			continue;

		ent->HierarchyUpdateMoveParent();
	}
}

void C_HLTVCamera::CalcView(Vector& origin, QAngle& angles, float& fov)
{
	if ( m_bEntityPacketReceived )
	{
		// try to fixup movment pareents
		FixupMovmentParents();
		m_bEntityPacketReceived = false;
	}

	if ( m_iCameraMan > 0 )
	{
		C_BasePlayer *pCameraMan = UTIL_PlayerByIndex( m_iCameraMan );
		if ( pCameraMan )
		{
			float zNear,zFar;
			pCameraMan->CalcView( origin, angles, zNear, zFar, fov );
			pCameraMan->CalcViewModelView( origin, angles );
			return;
		}
	}

	switch ( m_nCameraMode )
	{
		case OBS_MODE_ROAMING	:	CalcRoamingView( origin, angles, fov );
									break;

		case OBS_MODE_FIXED		:	CalcFixedView( origin, angles, fov );
									break;

		case OBS_MODE_IN_EYE	:	CalcInEyeCamView( origin, angles, fov );
									break;

		case OBS_MODE_CHASE		:	CalcChaseCamView( origin, angles, fov  );
									break;
	}
}

void C_HLTVCamera::SetMode(int iMode)
{
	if ( m_nCameraMode == iMode )
		return;

    Assert( iMode > OBS_MODE_NONE && iMode <= LAST_PLAYER_OBSERVERMODE );

	int iOldMode = m_nCameraMode;
	m_nCameraMode = iMode;

	IGameEvent *event = gameeventmanager->CreateEvent( "hltv_changed_mode" );
	if ( event )
	{
		event->SetInt( "oldmode", iOldMode );
		event->SetInt( "newmode", m_nCameraMode );
		event->SetInt( "obs_target", m_iTraget1 );
		gameeventmanager->FireEventClientSide( event );
	}
}

void C_HLTVCamera::SetPrimaryTarget( int nEntity ) 
{
 	if ( m_iTraget1 == nEntity )
		return;

	int iOldTarget = m_iTraget1;
	m_iTraget1 = nEntity;

	if ( GetMode() == OBS_MODE_ROAMING )
	{
		Vector vOrigin;
		QAngle aAngles;
		float flFov;

		CalcChaseCamView( vOrigin,  aAngles, flFov );
	}
	else if ( GetMode() == OBS_MODE_CHASE )
	{
		C_BaseEntity* target = ClientEntityList().GetEnt( m_iTraget1 );
		if ( target )
		{
			QAngle eyeAngle = target->EyeAngles();
			prediction->SetViewAngles( eyeAngle );
		}
	}

	m_flLastDistance = m_flDistance;
	m_flLastAngleUpdateTime = -1;

	IGameEvent *event = gameeventmanager->CreateEvent( "hltv_changed_target" );
	if ( event )
	{
		event->SetInt( "mode", m_nCameraMode );
		event->SetInt( "old_target", iOldTarget );
		event->SetInt( "obs_target", m_iTraget1 );
		gameeventmanager->FireEventClientSide( event );
	}
}

void C_HLTVCamera::SpecNextPlayer( bool bInverse )
{
	int start = 1;

	if ( m_iTraget1 > 0 && m_iTraget1 <= gpGlobals->maxClients )
		start = m_iTraget1;

	int index = start;

	while ( true )
	{	
		// got next/prev player
		if ( bInverse )
			index--;
		else
			index++;

		// check bounds
		if ( index < 1 )
			index = gpGlobals->maxClients;
		else if ( index > gpGlobals->maxClients )
			index = 1;

		if ( index == start )
			break; // couldn't find a new valid player

		C_BasePlayer *pPlayer =	UTIL_PlayerByIndex( index );

		if ( !pPlayer )
			continue;

		// only follow living players 
		if ( pPlayer->IsObserver() )
			continue;

		break; // found a new player
	}

	SetPrimaryTarget( index );

	// turn off auto director once user tried to change view settings
	SetAutoDirector( false );
}

void C_HLTVCamera::SpecNamedPlayer( const char *szPlayerName )
{
	for ( int index = 1; index <= gpGlobals->maxClients; ++index )
	{
		C_BasePlayer *pPlayer =	UTIL_PlayerByIndex( index );

		if ( !pPlayer )
			continue;

		if ( !FStrEq( szPlayerName, pPlayer->GetPlayerName() ) )
			continue;

		// only follow living players or dedicated spectators
		if ( pPlayer->IsObserver() && pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
			continue;

		SetPrimaryTarget( index );
		return;
	}
}

void C_HLTVCamera::FireGameEvent( IGameEvent * event)
{
	if ( !engine->IsHLTV() )
		return;	// not in HLTV mode

	const char *type = event->GetName();

	if ( Q_strcmp( "game_newmap", type ) == 0 )
	{
		Reset();	// reset all camera settings

		// show spectator UI
		if ( !gViewPortInterface )
			return;

		if ( engine->IsPlayingDemo() )
        {
			// for demo playback show full menu
			gViewPortInterface->ShowPanel( PANEL_SPECMENU, true );

			SetMode( OBS_MODE_ROAMING );
		}
		else
		{
			// during live broadcast only show black bars
			gViewPortInterface->ShowPanel( PANEL_SPECGUI, true );
		}

		return;
	}

	if ( Q_strcmp( "hltv_message", type ) == 0 )
	{
		wchar_t outputBuf[1024];
		const char *pszText = event->GetString( "text", "" );
		
		char *tmpStr = hudtextmessage->LookupString( pszText );
		const wchar_t *pBuf = g_pVGuiLocalize->Find( tmpStr );
		if ( pBuf )
		{
			// Copy pBuf into szBuf[i].
			int nMaxChars = sizeof( outputBuf ) / sizeof( wchar_t );
			wcsncpy( outputBuf, pBuf, nMaxChars );
			outputBuf[nMaxChars-1] = 0;
		}
		else
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( tmpStr, outputBuf, sizeof(outputBuf) );
		}

		internalCenterPrint->Print( ConvertCRtoNL( outputBuf ) );
		return ;
	}

	if ( Q_strcmp( "hltv_title", type ) == 0 )
	{
		Q_strncpy( m_szTitleText, event->GetString( "text", "" ), sizeof(m_szTitleText) );
		return;
	}

	if ( Q_strcmp( "hltv_status", type ) == 0 )
	{
		int nNumProxies = event->GetInt( "proxies" );
		m_nNumSpectators = event->GetInt( "clients" ) - nNumProxies;
		return;
	}

	// after this only auto-director commands follow
	// don't execute them is autodirector is off and PVS is unlocked
	if ( !spec_autodirector.GetBool() && !IsPVSLocked() )
		return;

	if ( Q_strcmp( "hltv_cameraman", type ) == 0 )
	{
		Reset();

		m_nCameraMode = OBS_MODE_ROAMING;
		m_iCameraMan = event->GetInt( "index" ); 
		
		return;
	}

	if ( Q_strcmp( "hltv_fixed", type ) == 0 )
	{
		m_iCameraMan  = 0;
		
		m_vCamOrigin.x = event->GetInt( "posx" );
		m_vCamOrigin.y = event->GetInt( "posy" );
		m_vCamOrigin.z = event->GetInt( "posz" );

		QAngle angle;
 		angle.x = event->GetInt( "theta" );
		angle.y = event->GetInt( "phi" );
		angle.z = 0; // no roll yet

		if ( m_nCameraMode != OBS_MODE_FIXED )
		{
			SetMode( OBS_MODE_FIXED );
			SetCameraAngle( angle );
			m_flFOV = event->GetFloat( "fov", 90 );
		}

		SetPrimaryTarget( event->GetInt( "target" ) );

		if ( m_iTraget1 == 0 )
		{
			SetCameraAngle( angle );
		}
						
		return;
	}

	if ( Q_strcmp( "hltv_chase", type ) == 0 )
	{
		bool bInEye	= event->GetInt( "ineye" );

		// check if we are already in a player chase mode
		bool bIsInChaseMode = (m_nCameraMode==OBS_MODE_IN_EYE)|| (m_nCameraMode==OBS_MODE_CHASE);

		// if we are in auto director or not in a valid chase mode, set new mode now
		if ( spec_autodirector.GetBool() || !bIsInChaseMode )
		{
			SetMode( bInEye?OBS_MODE_IN_EYE:OBS_MODE_CHASE );
		}

		m_iCameraMan  = 0;
				
		m_iTraget2		= event->GetInt( "target2" );
		m_flDistance	= event->GetFloat( "distance", m_flDistance );
		m_flOffset		= event->GetFloat( "offset", m_flOffset );
		m_flTheta		= event->GetFloat( "theta", m_flTheta );
		m_flPhi			= event->GetFloat( "phi", m_flPhi );
		m_flFOV			= event->GetFloat( "fov", 90 );
 		m_flInertia		= event->GetFloat( "inertia", 30.f ) / 10.f;

		// if inertia is not set use standard value
		if ( m_flInertia <= 0 )
			m_flInertia = 3.0f;

		SetPrimaryTarget( event->GetInt( "target1" ) );
							
		return;
	}
}

// this is a cheap version of FullNoClipMove():
void C_HLTVCamera::CreateMove( CUserCmd *cmd)
{
	if ( cmd )
	{
		m_LastCmd = *cmd;
	}
}

void C_HLTVCamera::SetCameraAngle( QAngle& targetAngle )
{
	m_aCamAngle	= targetAngle;
 	NormalizeAngles( m_aCamAngle );
	m_flLastAngleUpdateTime = gpGlobals->realtime;
}

void C_HLTVCamera::SmoothCameraAngle( QAngle& targetAngle )
{
	if ( m_flLastAngleUpdateTime > 0 )
	{
		float deltaTime = gpGlobals->realtime - m_flLastAngleUpdateTime;

		deltaTime = clamp( deltaTime*m_flInertia, 0.01f, 1.f);

		InterpolateAngles( m_aCamAngle, targetAngle, m_aCamAngle, deltaTime );
	}
	else
	{
		m_aCamAngle = targetAngle;
	}

	m_flLastAngleUpdateTime = gpGlobals->realtime;
}

void C_HLTVCamera::ToggleChaseAsFirstPerson()
{
	if ( GetMode() == OBS_MODE_CHASE )
	{
		SetMode( OBS_MODE_IN_EYE );
	}
	else if ( GetMode() == OBS_MODE_IN_EYE )
	{
		SetMode( OBS_MODE_CHASE );
	}
}

bool C_HLTVCamera::IsPVSLocked()
{
	if ( tv_transmitall != NULL )
	{
		return !tv_transmitall->GetBool();
	}
	else
	{
		//old style, assume locked unless we playback a demo
		return !engine->IsPlayingDemo();
	}
}

void C_HLTVCamera::SetAutoDirector( bool bActive )
{
	spec_autodirector.SetValue( bActive?1:0 );
}