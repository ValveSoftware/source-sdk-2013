//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Client handler implementations for instruction players how to play
//
//=============================================================================//

#include "cbase.h"

#include "c_baselesson.h"
#include "c_gameinstructor.h"

#include "hud_locator_target.h"
#include "c_world.h"
#include "iinput.h"
#include "ammodef.h"
#include "vprof.h"
#include "view.h"
#include "vstdlib/ikeyvaluessystem.h"
#ifdef MAPBASE
#include "usermessages.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Configuración
//=========================================================

#define LESSON_PRIORITY_MAX 1000
#define LESSON_PRIORITY_NONE 0
#define LESSON_MIN_TIME_ON_SCREEN_TO_MARK_DISPLAYED 1.5f
#define LESSON_MIN_TIME_BEFORE_LOCK_ALLOWED 0.1f
#define LESSON_DISTANCE_UPDATE_RATE 0.25f

// See comments in UtlSymbol on why this is useful and how it works
IMPLEMENT_PRIVATE_SYMBOLTYPE( CGameInstructorSymbol );

extern ConVar gameinstructor_verbose;
extern ConVar gameinstructor_verbose_lesson;
extern ConVar gameinstructor_find_errors;

#ifdef MAPBASE
// Mapbase was originally going to use a HL2-style default color (245,232,179).
// This is no longer the case, but mods are free to change this cvar in their config files.
ConVar gameinstructor_default_captioncolor( "gameinstructor_default_captioncolor", "255,255,255", FCVAR_NONE );
ConVar gameinstructor_default_bindingcolor( "gameinstructor_default_bindingcolor", "0,0,0", FCVAR_NONE );
#endif

//
// CGameInstructorLesson
//

Color CBaseLesson::m_rgbaVerboseHeader = Color( 255, 128, 64, 255 );
Color CBaseLesson::m_rgbaVerbosePlain = Color( 64, 128, 255, 255 );
Color CBaseLesson::m_rgbaVerboseName = Color( 255, 255, 255, 255 );
Color CBaseLesson::m_rgbaVerboseOpen = Color( 0, 255, 0, 255 );
Color CBaseLesson::m_rgbaVerboseClose = Color( 255, 0, 0, 255 );
Color CBaseLesson::m_rgbaVerboseSuccess = Color( 255, 255, 0, 255 );
Color CBaseLesson::m_rgbaVerboseUpdate = Color( 255, 0, 255, 255  );


//=========================================================
// Constructor
//=========================================================
CBaseLesson::CBaseLesson( const char *pchName, bool bIsDefaultHolder, bool bIsOpenOpportunity )
{
	COMPILE_TIME_ASSERT( sizeof( CGameInstructorSymbol ) == sizeof( CUtlSymbol ) );

	m_stringName			= pchName;
	m_stringReplaceKey		= "";
	m_bIsDefaultHolder		= bIsDefaultHolder;
	m_bIsOpenOpportunity	= bIsOpenOpportunity;

	Init();
}

//=========================================================
// Destructor
//=========================================================
CBaseLesson::~CBaseLesson()
{
	// Remove from root's children list
	if ( m_pRoot )
		m_pRoot->m_OpenOpportunities.FindAndRemove(this);

	else
	{
		for ( int i = 0; i < m_OpenOpportunities.Count(); ++i )
		{
			// Remove from children if they are still around
			CBaseLesson *pLesson	= m_OpenOpportunities[ i ];
			pLesson->m_pRoot			= NULL;
		}
	}
}

//=========================================================
//=========================================================
void CBaseLesson::AddPrerequisite( const char *pchLessonName )
{
	if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
	{
		ConColorMsg( CBaseLesson::m_rgbaVerboseHeader, "\t%s: ", GetName() );
		ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "Adding prereq " );
		ConColorMsg( CBaseLesson::m_rgbaVerboseOpen, "\"%s\"", pchLessonName );
		ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ".\n" );
	}

	const CBaseLesson *pPrerequisite = GetGameInstructor().GetLesson( pchLessonName );

	if ( !pPrerequisite )
	{
		DevWarning( "Prerequisite %s added by lesson %s doesn't exist!\n", pchLessonName, GetName() );
		return;
	}

	m_Prerequisites.AddToTail(pPrerequisite);
}

//=========================================================
//=========================================================
void CBaseLesson::SetRoot( CBaseLesson *pRoot )
{
	m_pRoot = pRoot;

	if ( m_pRoot->m_OpenOpportunities.Find( this ) == -1 )
		m_pRoot->m_OpenOpportunities.AddToTail( this );
}

//=========================================================
//=========================================================
bool CBaseLesson::ShouldShowSpew()
{
	// @DEBUG
	return true;

	if ( gameinstructor_verbose_lesson.GetString()[ 0 ] == '\0' )
		return false;

	return ( Q_stristr( GetName(), gameinstructor_verbose_lesson.GetString() ) != NULL );
}

//=========================================================
//=========================================================
bool CBaseLesson::NoPriority() const
{
	return ( m_iPriority == LESSON_PRIORITY_NONE );
}

//=========================================================
//=========================================================
bool CBaseLesson::IsLocked() const
{
	if ( m_fLockDuration == 0.0f )
		return false;

	if ( !IsInstructing() || !IsVisible() )
		return false;

	float fLockTime = m_fLockTime;

	if ( fLockTime == 0.0f )
		fLockTime = m_fStartTime;

	return ( gpGlobals->curtime > m_fStartTime + LESSON_MIN_TIME_BEFORE_LOCK_ALLOWED && gpGlobals->curtime < fLockTime + m_fLockDuration );
}

//=========================================================
//=========================================================
bool CBaseLesson::IsLearned() const
{
	if ( m_iDisplayLimit > 0 && m_iDisplayCount >= m_iDisplayLimit )
		return true;

	if ( m_iSuccessLimit > 0 && m_iSuccessCount >= m_iSuccessLimit )
		return true;

	return false;
}

//=========================================================
//=========================================================
bool CBaseLesson::PrerequisitesHaveBeenMet() const
{
	for ( int i = 0; i < m_Prerequisites.Count(); ++i )
	{
		if ( !m_Prerequisites[ i ]->IsLearned() )
		{
			// Failed a prereq
			return false;
		}
	}

	// All prereqs passed
	return true;
}

//=========================================================
//=========================================================
bool CBaseLesson::IsTimedOut()
{
	VPROF_BUDGET( "CBaseLesson::IsTimedOut", "GameInstructor" );

	// Check for no timeout
	if ( m_fTimeout == 0.0f )
		return false;

	float fStartTime = m_fStartTime;

	if ( GetRoot()->IsLearned() )
	{
		if ( !m_bBumpWithTimeoutWhenLearned )
		{
			// Time out instantly if we've learned this and don't want to keep it open for priority bumping
			return true;
		}
		else
		{
			// It'll never be active, so lets use timeout based on when it was initialized
			fStartTime = m_fInitTime;
		}
	}

	if ( !fStartTime )
	{
		if ( !m_bCanTimeoutWhileInactive )
		{
			return false;
		}

		// Not active, so lets use timeout based on when it was initialized
		fStartTime = m_fInitTime;
	}

	bool bTimedOut = ( fStartTime + m_fTimeout < gpGlobals->curtime );

	if ( bTimedOut )
		SetCloseReason( "Timed out." );

	return bTimedOut;
}

//=========================================================
//=========================================================
void CBaseLesson::ResetDisplaysAndSuccesses()
{
	m_iDisplayCount		= 0;
	m_bSuccessCounted	= false;
	m_iSuccessCount		= 0;
}

//=========================================================
//=========================================================
bool CBaseLesson::IncDisplayCount()
{
	if ( m_iDisplayCount < m_iDisplayLimit )
	{
		m_iDisplayCount++;
		return true;
	}

	return false;
}

//=========================================================
//=========================================================
bool CBaseLesson::IncSuccessCount()
{
	if ( m_iSuccessCount < m_iSuccessLimit )
	{
		m_iSuccessCount++;
		return true;
	}

	return false;
}

//=========================================================
//=========================================================
void CBaseLesson::Init()
{
	m_pRoot				= NULL;
	m_bSuccessCounted	= false;

	SetCloseReason( "None given." );

	m_iPriority					= LESSON_PRIORITY_MAX; // Set to invalid value to ensure that it is actually set later on
	m_iInstanceType				= LESSON_INSTANCE_MULTIPLE;
	m_iFixedInstancesMax		= 1;
	m_bReplaceOnlyWhenStopped	= false;
	m_iTeam						= TEAM_ANY;
	m_bOnlyKeyboard				= false;
	m_bOnlyGamepad				= false;

	m_iDisplayLimit				= 0;
	m_iDisplayCount				= 0;
	m_bWasDisplayed				= false;

	m_iSuccessLimit				= 0;
	m_iSuccessCount				= 0;

	m_fLockDuration				= 0.0f;
	m_bCanOpenWhenDead			= false;
	m_bBumpWithTimeoutWhenLearned = false;
	m_bCanTimeoutWhileInactive	= false;
	m_fTimeout					= 0.0f;

	m_fInitTime					= gpGlobals->curtime;
	m_fStartTime				= 0.0f;
	m_fLockTime					= 0.0f;

	m_fUpdateInterval			= 0.5;
	m_bHasPlayedSound			= false;

	m_szStartSound				= "Instructor.LessonStart";
	m_szLessonGroup				= "";

	m_iNumDelayedPlayerSwaps = 0;
}

//=========================================================
//=========================================================
void CBaseLesson::TakePlaceOf( CBaseLesson *pLesson )
{
	// Transfer over marked as displayed so a replaced lesson won't count as an extra display
	m_bWasDisplayed				= pLesson->m_bWasDisplayed;
	pLesson->m_bWasDisplayed		= false;
}

//=========================================================
//=========================================================
void CBaseLesson::MarkSucceeded()
{
	if ( !m_bSuccessCounted )
	{
		GetGameInstructor().MarkSucceeded( GetName() );
		m_bSuccessCounted = true;
	}
}

//=========================================================
//=========================================================
void CBaseLesson::CloseOpportunity( const char *pchReason )
{
	SetCloseReason(pchReason);
	m_bIsOpenOpportunity = false;
}

//=========================================================
//=========================================================
bool CBaseLesson::DoDelayedPlayerSwaps() const
{
	// A bot has swapped places with a player or player with a bot...
	// At the time of the actual swap there was no client representation for the new player...
	// So that swap was queued up and now we're going to make things right!
	while ( m_iNumDelayedPlayerSwaps )
	{
		C_BasePlayer *pNewPlayer = UTIL_PlayerByUserId( m_pDelayedPlayerSwap[ m_iNumDelayedPlayerSwaps - 1 ].iNewUserID );

		if ( !pNewPlayer )
		{
			// There is still no client representation of the new player, we'll have to try again later
			if ( gameinstructor_verbose.GetInt() > 1 )
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tFailed delayed player swap!" );
			
			return false;
		}

		if ( gameinstructor_verbose.GetInt() > 1 )
			ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tSuccessful delayed player swap!" );

		m_pDelayedPlayerSwap[ m_iNumDelayedPlayerSwaps - 1 ].phHandleToChange->Set( pNewPlayer );
		m_iNumDelayedPlayerSwaps--;
	}

	return true;
}


//
// CTextLesson
//

//=========================================================
//=========================================================
void CTextLesson::Init()
{
	m_szDisplayText			= "";
	m_szDisplayParamText	= "";
	m_szBinding				= "";
	m_szGamepadBinding		= "";
}

//=========================================================
//=========================================================
void CTextLesson::Start()
{
	// TODO: Display some text
	//m_szDisplayText
}

//=========================================================
//=========================================================
void CTextLesson::Stop()
{
	// TODO: Clean up text
}

//
// CIconLesson
//

void CIconLesson::Init()
{
	m_hIconTarget			= NULL;
	m_szVguiTargetName		= "";
	m_szVguiTargetLookup	= "";
	m_nVguiTargetEdge		= 0;

	m_hLocatorTarget		= -1;
	m_bFixedPosition		= false;
	m_bNoIconTarget			= false;
	m_bAllowNodrawTarget	= false;

	m_bVisible				= true;
	m_bShowWhenOccluded		= true;
	m_bNoOffscreen			= false;
	m_bForceCaption			= false;

	m_szOnscreenIcon		= "";
	m_szOffscreenIcon		= "";

	m_flUpOffset			= 0.0f;
	m_flRelativeUpOffset	= 0.0f;
	m_fFixedPositionX		= 0.0f;
	m_fFixedPositionY		= 0.0f;

	m_fRange				= 0.0f;
	m_fCurrentDistance		= 0.0f;

	m_fOnScreenStartTime	= 0.0f;
	m_fUpdateDistanceTime	= 0.0f;

	m_iFlags				= LOCATOR_ICON_FX_NONE;
#ifdef MAPBASE
	m_szCaptionColor		= gameinstructor_default_captioncolor.GetString();

	m_iIconTargetPos		= ICON_TARGET_EYE_POSITION;
	m_szHudHint				= "";
#else
	m_szCaptionColor		= "255,255,255";// Default to white
#endif
}

//=========================================================
//=========================================================
void CIconLesson::Start()
{
	if ( !DoDelayedPlayerSwaps() )
		return;

	// Display some text
	C_BaseEntity *pIconTarget = m_hIconTarget.Get();

	if ( !pIconTarget )	
	{
		if ( !m_bNoIconTarget )
		{
			// Wanted one, but couldn't get it
			CloseOpportunity( "Icon Target handle went invalid before the lesson started!" );
		}

		return;
	}
	else
	{
		if ( ( pIconTarget->IsEffectActive( EF_NODRAW ) || pIconTarget->IsDormant() ) && !m_bAllowNodrawTarget )
		{
			// We don't allow no draw entities
			CloseOpportunity( "Icon Target is using effect NODRAW and allow_nodraw_target is false!" );
			return;
		}
	}

	CLocatorTarget *pLocatorTarget = NULL;

	if ( m_hLocatorTarget != -1 )
	{
		// Lets try the handle that we've held on to
		pLocatorTarget = Locator_GetTargetFromHandle( m_hLocatorTarget );

		if ( !pLocatorTarget )
		{
			// It's gone stale, get a new target
			m_hLocatorTarget = Locator_AddTarget();
			pLocatorTarget = Locator_GetTargetFromHandle( m_hLocatorTarget );
		}
	}
	else
	{
		// Get a new target
		m_hLocatorTarget = Locator_AddTarget();
		pLocatorTarget = Locator_GetTargetFromHandle( m_hLocatorTarget );
	}

	if( m_hLocatorTarget == -1 || !pLocatorTarget )
	{
		CloseOpportunity( "Could not get a handle for new locator target. Too many targets in use!" );
		return;
	}

	pLocatorTarget->AddIconEffects( m_iFlags );
	pLocatorTarget->SetCaptionColor( GetCaptionColorString() );
	UpdateLocatorTarget( pLocatorTarget, pIconTarget );

	// Update occlusion data
	Locator_ComputeTargetIconPositionFromHandle( m_hLocatorTarget );
}

//=========================================================
//=========================================================
void CIconLesson::Stop()
{
	if ( !DoDelayedPlayerSwaps() )
		return;

	if ( m_hLocatorTarget != -1 )
		Locator_RemoveTarget( m_hLocatorTarget );

	m_fOnScreenStartTime = 0.0f;
}

//=========================================================
//=========================================================
void CIconLesson::Update()
{
	if ( !DoDelayedPlayerSwaps() )
		return;

	C_BaseEntity *pIconTarget = m_hIconTarget.Get();

	if ( !pIconTarget )
	{
		if ( !m_bNoIconTarget )
		{
			CloseOpportunity( "Lost our icon target handle returned NULL." );
		}

		return;
	}
	else
	{
		if ( ( pIconTarget->IsEffectActive( EF_NODRAW ) || pIconTarget->IsDormant() ) && !m_bAllowNodrawTarget )
		{
			// We don't allow no draw entities
			CloseOpportunity( "Icon Target is using effect NODRAW and allow_nodraw_target is false!" );
			return;
		}
	}

	CLocatorTarget *pLocatorTarget = Locator_GetTargetFromHandle( m_hLocatorTarget );
	if ( !pLocatorTarget )
	{
		// Temp instrumentation to catch a bug - possibly calling Update without having called Start?
		Warning( "Problem in lesson %s: Locator_GetTargetFromHandle returned null for handle %d.\n IsInstanceActive: %s. IsInstructing: %s. IsLearned: %s\n",
				GetName(), m_hLocatorTarget, 
				(IsInstanceActive() ? "yes" : "no"),
				(IsInstructing() ? "yes" : "no"),
				(IsLearned() ? "yes" : "no") );
		CloseOpportunity( "Lost locator target handle." );
		return;
	}

	UpdateLocatorTarget( pLocatorTarget, pIconTarget );
	C_BasePlayer *pLocalPlayer = GetGameInstructor().GetLocalPlayer();

	// Check if it has been onscreen long enough to count as being displayed
	if ( m_fOnScreenStartTime == 0.0f )
	{
		if ( pLocatorTarget->IsOnScreen() && ( IsPresentComplete() || ( pLocatorTarget->GetIconEffectsFlags() & LOCATOR_ICON_FX_STATIC ) ) )
		{
			// Is either static or has finished presenting and is on screen
			m_fOnScreenStartTime = gpGlobals->curtime;
		}
	}
	else
	{
		if ( !pLocatorTarget->IsOnScreen() )
		{
			// Was visible before, but it isn't now
			m_fOnScreenStartTime = 0.0f;
		}
		else if ( gpGlobals->curtime - m_fOnScreenStartTime >= LESSON_MIN_TIME_ON_SCREEN_TO_MARK_DISPLAYED )
		{
			// Lesson on screen long enough to be counted as displayed
			m_bWasDisplayed = true;
		}
	}

	if ( m_fUpdateDistanceTime < gpGlobals->curtime )
	{
		// Update it's distance from the local player
		C_BaseEntity *pTarget = m_hIconTarget.Get();

		if ( !pLocalPlayer || !pTarget || pLocalPlayer == pTarget )
		{
			m_fCurrentDistance = 0.0f;
		}
		else
		{
			m_fCurrentDistance = pLocalPlayer->EyePosition().DistTo( pTarget->WorldSpaceCenter() );
		}

		m_fUpdateDistanceTime = gpGlobals->curtime + LESSON_DISTANCE_UPDATE_RATE;
	}
}

void CIconLesson::UpdateInactive()
{
	if ( m_fUpdateDistanceTime < gpGlobals->curtime )
	{
		if ( !DoDelayedPlayerSwaps() )
		{
			return;
		}

		C_BaseEntity *pIconTarget = m_hIconTarget.Get();

		if ( !pIconTarget )
		{
			if ( !m_bNoIconTarget )
			{
				CloseOpportunity( "Lost our icon target handle returned NULL." );
			}

			m_fCurrentDistance = 0.0f;
			return;
		}
		else
		{
			if ( ( pIconTarget->IsEffectActive( EF_NODRAW ) || pIconTarget->IsDormant() ) && !m_bAllowNodrawTarget )
			{
				// We don't allow no draw entities
				CloseOpportunity( "Icon Target is using effect NODRAW and allow_nodraw_target is false!" );
				return;
			}
		}

		// Update it's distance from the local player
		C_BasePlayer *pLocalPlayer = GetGameInstructor().GetLocalPlayer();

		if ( !pLocalPlayer || pLocalPlayer == pIconTarget )
		{
			m_fCurrentDistance = 0.0f;
		}
		else
		{
			m_fCurrentDistance = pLocalPlayer->EyePosition().DistTo( pIconTarget->WorldSpaceCenter() );
		}

#ifdef MAPBASE
		if (m_szHudHint.String()[0] != '\0' && GetRoot()->IsLearned())
		{
			DevMsg("Showing hint\n");
			CUtlBuffer msg_data;
			msg_data.PutChar( 1 );
			msg_data.PutString( m_szHudHint.String() );
			usermessages->DispatchUserMessage( usermessages->LookupUserMessage( "KeyHintText" ), bf_read( msg_data.Base(), msg_data.TellPut() ) );
		}
#endif

		m_fUpdateDistanceTime = gpGlobals->curtime + LESSON_DISTANCE_UPDATE_RATE;
	}
}

bool CIconLesson::ShouldDisplay() const
{
	VPROF_BUDGET( "CIconLesson::ShouldDisplay", "GameInstructor" );

	if ( !DoDelayedPlayerSwaps() )
	{
		return false;
	}

	if ( m_fRange > 0.0f && m_fCurrentDistance > m_fRange )
	{
		// Distance to target is more than the max range
		return false;
	}

	if ( !m_bShowWhenOccluded && m_hLocatorTarget >= 0 )
	{
		CLocatorTarget *pLocatorTarget = Locator_GetTargetFromHandle( m_hLocatorTarget );

		if ( pLocatorTarget && pLocatorTarget->IsOccluded() )
		{
			// Target is occluded and doesn't want to be shown when occluded
			return false;
		}
	}

	// Ok to display
	return true;
}

bool CIconLesson::IsVisible() const
{
	VPROF_BUDGET( "CIconLesson::IsVisible", "GameInstructor" );

	if( m_hLocatorTarget == -1 )
	{
		// If it doesn't want a target, it's "visible" otherwise we'll have to call it invisible
		return m_bNoIconTarget;
	}

	CLocatorTarget *pLocatorTarget = Locator_GetTargetFromHandle( m_hLocatorTarget );
	if ( !pLocatorTarget )
	{
		return false;
	}

	return pLocatorTarget->IsVisible();
}

void CIconLesson::SwapOutPlayers( int iOldUserID, int iNewUserID )
{
	BaseClass::SwapOutPlayers( iOldUserID, iNewUserID );

	if ( m_bNoIconTarget )
		return;

	// Get the player pointers from the user IDs
	C_BasePlayer *pOldPlayer = UTIL_PlayerByUserId( iOldUserID );
	C_BasePlayer *pNewPlayer = UTIL_PlayerByUserId( iNewUserID );

	if ( pOldPlayer == m_hIconTarget.Get() )
	{
		if ( pNewPlayer )
		{
			m_hIconTarget = pNewPlayer;
		}
		else
		{
			if ( m_iNumDelayedPlayerSwaps < MAX_DELAYED_PLAYER_SWAPS )
			{
				m_pDelayedPlayerSwap[ m_iNumDelayedPlayerSwaps ].phHandleToChange = &m_hIconTarget;
				m_pDelayedPlayerSwap[ m_iNumDelayedPlayerSwaps ].iNewUserID = iNewUserID;
				++m_iNumDelayedPlayerSwaps;
			}
		}
	}
}

void CIconLesson::TakePlaceOf( CBaseLesson *pLesson )
{
	BaseClass::TakePlaceOf( pLesson );

	const CIconLesson *pIconLesson = dynamic_cast<const CIconLesson*>( pLesson );

	if ( pIconLesson )
	{
		if ( pIconLesson->m_hLocatorTarget != -1 )
		{
			CLocatorTarget *pLocatorTarget = Locator_GetTargetFromHandle( pIconLesson->m_hLocatorTarget );

			if ( pLocatorTarget )
			{
				// This one draw right to the hud... use it's icon target handle
				m_hLocatorTarget = pIconLesson->m_hLocatorTarget;
			}
		}

		m_fOnScreenStartTime = pIconLesson->m_fOnScreenStartTime;
	}
}

void CIconLesson::SetLocatorBinding( CLocatorTarget * pLocatorTarget )
{
	if ( IsX360() /*|| input->ControllerModeActive()*/ )
	{
		// Try to use gamepad bindings first
		if ( m_szGamepadBinding.String()[ 0 ] != '\0' )
		{
			// Found gamepad binds!
			pLocatorTarget->SetBinding( m_szGamepadBinding.String() );
		}
		else
		{
			// No gamepad binding, so fallback to the regular binding
			pLocatorTarget->SetBinding( m_szBinding.String() );
		}
	}
	else
	{
		// Always use the regular binding when the gamepad is disabled
		pLocatorTarget->SetBinding( m_szBinding.String() );
	}
}

bool CIconLesson::IsPresentComplete()
{
	if ( m_hLocatorTarget == -1 )
		return false;

	CLocatorTarget *pLocatorTarget = Locator_GetTargetFromHandle( m_hLocatorTarget );

	if ( !pLocatorTarget )
		return false;

	return !pLocatorTarget->IsPresenting();
}

void CIconLesson::PresentStart()
{
	if ( m_hLocatorTarget == -1 )
		return;

	CLocatorTarget *pLocatorTarget = Locator_GetTargetFromHandle( m_hLocatorTarget );

	if ( !pLocatorTarget )
		return;

	pLocatorTarget->StartPresent();
}

void CIconLesson::PresentEnd()
{
	if ( m_hLocatorTarget == -1 )
		return;

	CLocatorTarget *pLocatorTarget = Locator_GetTargetFromHandle( m_hLocatorTarget );

	if ( !pLocatorTarget )
		return;

	pLocatorTarget->EndPresent();
}

void CIconLesson::UpdateLocatorTarget( CLocatorTarget *pLocatorTarget, C_BaseEntity *pIconTarget )
{
	if ( m_bFixedPosition )
	{
		pLocatorTarget->m_bOriginInScreenspace = true;
		pLocatorTarget->m_vecOrigin.x = m_fFixedPositionX;
		pLocatorTarget->m_vecOrigin.y = m_fFixedPositionY;
		pLocatorTarget->SetVguiTargetName( m_szVguiTargetName.String() );
		pLocatorTarget->SetVguiTargetLookup( m_szVguiTargetLookup.String() );
		pLocatorTarget->SetVguiTargetEdge( m_nVguiTargetEdge );
	}
	else
	{
		pLocatorTarget->m_bOriginInScreenspace = false;
#ifdef MAPBASE
		pLocatorTarget->m_vecOrigin = GetIconTargetPosition( pIconTarget ) + MainViewUp() * m_flRelativeUpOffset + Vector( 0.0f, 0.0f, m_flUpOffset );
#else
		pLocatorTarget->m_vecOrigin = pIconTarget->EyePosition() + MainViewUp() * m_flRelativeUpOffset + Vector( 0.0f, 0.0f, m_flUpOffset );
#endif
		pLocatorTarget->SetVguiTargetName( "" );
	}

	const char *pchDisplayParamText = m_szDisplayParamText.String();
#ifdef INFESTED_DLL
	char szCustomName[ 256 ];
#endif

	// Check if the parameter is the be the player display name
	if ( Q_stricmp( pchDisplayParamText, "use_name" ) == 0 )
	{
		// Fix up the player display name
		C_BasePlayer *pPlayer = ToBasePlayer( pIconTarget );
		if ( pPlayer )
		{
			pchDisplayParamText = pPlayer->GetPlayerName();
		}
		else
		{
			bool bNoName = true;

#ifdef INFESTED_DLL
			C_ASW_Marine *pMarine = dynamic_cast< C_ASW_Marine* >( pIconTarget );
			if ( pMarine )
			{
				C_ASW_Marine_Resource *pMR = pMarine->GetMarineResource();
				if ( pMR )
				{
					pMR->GetDisplayName( szCustomName, sizeof( szCustomName ) );
					pchDisplayParamText = szCustomName;
					bNoName = false;
				}
			}
#endif

			if ( bNoName )
			{
				// It's not a player!
				pchDisplayParamText = "";
			}
		}
	}

	pLocatorTarget->SetCaptionText( m_szDisplayText.String(), pchDisplayParamText );
	SetLocatorBinding( pLocatorTarget );
	pLocatorTarget->SetOnscreenIconTextureName( m_szOnscreenIcon.String() );
	pLocatorTarget->SetOffscreenIconTextureName( m_szOffscreenIcon.String() );
	pLocatorTarget->SetVisible( m_bVisible );

	C_BasePlayer *pLocalPlayer = GetGameInstructor().GetLocalPlayer();

	if( !m_bFixedPosition && 
		( ( pLocalPlayer != NULL && pLocalPlayer == m_hIconTarget ) || 
		  GetClientWorldEntity() == m_hIconTarget ) )
	{
		// Mark this icon as a static icon that draws in a fixed 
		// location on the hud rather than tracking an object
		// in 3D space.
		pLocatorTarget->AddIconEffects( LOCATOR_ICON_FX_STATIC );
	}
	else
	{
		pLocatorTarget->AddIconEffects( LOCATOR_ICON_FX_NONE );
	}

	if ( m_bNoOffscreen )
	{
		pLocatorTarget->AddIconEffects( LOCATOR_ICON_FX_NO_OFFSCREEN );
	}
	else
	{
		pLocatorTarget->RemoveIconEffects( LOCATOR_ICON_FX_NO_OFFSCREEN );
	}

	if( m_bForceCaption || IsLocked() )
	{
		pLocatorTarget->AddIconEffects( LOCATOR_ICON_FX_FORCE_CAPTION );
	}
	else
	{
		pLocatorTarget->RemoveIconEffects( LOCATOR_ICON_FX_FORCE_CAPTION );
	}

	pLocatorTarget->Update();

	if ( pLocatorTarget->m_bIsDrawing )
	{
		if ( !m_bHasPlayedSound )
		{
			GetGameInstructor().PlaySound( m_szStartSound.String() );
			m_bHasPlayedSound = true;
		}
	}
}

#ifdef MAPBASE
Vector CIconLesson::GetIconTargetPosition( C_BaseEntity *pIconTarget )
{
	switch (m_iIconTargetPos)
	{
		default:
		case ICON_TARGET_EYE_POSITION:
			return pIconTarget->EyePosition();

		case ICON_TARGET_ORIGIN:
			return pIconTarget->GetAbsOrigin();

		case ICON_TARGET_CENTER:
			return pIconTarget->WorldSpaceCenter();
	}
}
#endif

//
// CScriptedIconLesson
//

// Linking variables to scriptable entries is done here!
// The first parameter correlates to the case insensitive string name read from scripts.
// This macro generates code that passes this consistent variable data in to other macros
#define LESSON_VARIABLE_FACTORY \
	LESSON_VARIABLE_MACRO_EHANDLE( VOID, m_hLocalPlayer, EHANDLE )	\
																	\
	LESSON_VARIABLE_MACRO_EHANDLE( LOCAL_PLAYER, m_hLocalPlayer, EHANDLE )	\
	LESSON_VARIABLE_MACRO( OUTPUT, m_fOutput, float )						\
																			\
	LESSON_VARIABLE_MACRO_EHANDLE( ENTITY1, m_hEntity1, EHANDLE )				\
	LESSON_VARIABLE_MACRO_EHANDLE( ENTITY2, m_hEntity2, EHANDLE )				\
	LESSON_VARIABLE_MACRO_STRING( STRING1, m_szString1, CGameInstructorSymbol )	\
	LESSON_VARIABLE_MACRO_STRING( STRING2, m_szString2, CGameInstructorSymbol )	\
	LESSON_VARIABLE_MACRO( INTEGER1, m_iInteger1, int )							\
	LESSON_VARIABLE_MACRO( INTEGER2, m_iInteger2, int )							\
	LESSON_VARIABLE_MACRO( FLOAT1, m_fFloat1, float )							\
	LESSON_VARIABLE_MACRO( FLOAT2, m_fFloat2, float )							\
																				\
	LESSON_VARIABLE_MACRO_EHANDLE( ICON_TARGET, m_hIconTarget, EHANDLE )							\
	LESSON_VARIABLE_MACRO_STRING( VGUI_TARGET_NAME, m_szVguiTargetName, CGameInstructorSymbol )		\
	LESSON_VARIABLE_MACRO_STRING( VGUI_TARGET_LOOKUP, m_szVguiTargetLookup, CGameInstructorSymbol )	\
	LESSON_VARIABLE_MACRO( VGUI_TARGET_EDGE, m_nVguiTargetEdge, int )								\
	LESSON_VARIABLE_MACRO( FIXED_POSITION_X, m_fFixedPositionX, float )								\
	LESSON_VARIABLE_MACRO( FIXED_POSITION_Y, m_fFixedPositionY, float )								\
	LESSON_VARIABLE_MACRO_BOOL( FIXED_POSITION, m_bFixedPosition, bool )							\
	LESSON_VARIABLE_MACRO_BOOL( NO_ICON_TARGET, m_bNoIconTarget, bool )								\
	LESSON_VARIABLE_MACRO_BOOL( ALLOW_NODRAW_TARGET, m_bAllowNodrawTarget, bool )					\
	LESSON_VARIABLE_MACRO_BOOL( VISIBLE, m_bVisible, bool )											\
	LESSON_VARIABLE_MACRO_BOOL( SHOW_WHEN_OCCLUDED, m_bShowWhenOccluded, bool )						\
	LESSON_VARIABLE_MACRO_BOOL( NO_OFFSCREEN, m_bNoOffscreen, bool )								\
	LESSON_VARIABLE_MACRO_BOOL( FORCE_CAPTION, m_bForceCaption, bool )								\
	LESSON_VARIABLE_MACRO_STRING( ONSCREEN_ICON, m_szOnscreenIcon, CGameInstructorSymbol )			\
	LESSON_VARIABLE_MACRO_STRING( OFFSCREEN_ICON, m_szOffscreenIcon, CGameInstructorSymbol )		\
	LESSON_VARIABLE_MACRO( ICON_OFFSET, m_flUpOffset, float )										\
	LESSON_VARIABLE_MACRO( ICON_RELATIVE_OFFSET, m_flRelativeUpOffset, float )						\
	LESSON_VARIABLE_MACRO( RANGE, m_fRange, float )													\
																									\
	LESSON_VARIABLE_MACRO( FLAGS, m_iFlags, int )											\
	LESSON_VARIABLE_MACRO_STRING( CAPTION_COLOR, m_szCaptionColor, CGameInstructorSymbol )	\
	LESSON_VARIABLE_MACRO_STRING( GROUP, m_szLessonGroup, CGameInstructorSymbol )			\
																							\
	LESSON_VARIABLE_MACRO_STRING( CAPTION, m_szDisplayText, CGameInstructorSymbol )				\
	LESSON_VARIABLE_MACRO_STRING( CAPTION_PARAM, m_szDisplayParamText, CGameInstructorSymbol )	\
	LESSON_VARIABLE_MACRO_STRING( BINDING, m_szBinding, CGameInstructorSymbol )					\
	LESSON_VARIABLE_MACRO_STRING( GAMEPAD_BINDING, m_szGamepadBinding, CGameInstructorSymbol )	\
																								\
	LESSON_VARIABLE_MACRO( PRIORITY, m_iPriority, int )										\
	LESSON_VARIABLE_MACRO_STRING( REPLACE_KEY, m_stringReplaceKey, CGameInstructorSymbol )	\
																							\
	LESSON_VARIABLE_MACRO( LOCK_DURATION, m_fLockDuration, float )										\
	LESSON_VARIABLE_MACRO_BOOL( CAN_OPEN_WHEN_DEAD, m_bCanOpenWhenDead, bool )							\
	LESSON_VARIABLE_MACRO_BOOL( BUMP_WITH_TIMEOUT_WHEN_LEARNED, m_bBumpWithTimeoutWhenLearned, bool )	\
	LESSON_VARIABLE_MACRO_BOOL( CAN_TIMEOUT_WHILE_INACTIVE, m_bCanTimeoutWhileInactive, bool )			\
	LESSON_VARIABLE_MACRO( TIMEOUT, m_fTimeout, float )													\
	LESSON_VARIABLE_MACRO( UPDATE_INTERVAL, m_fUpdateInterval, float )									\
	LESSON_VARIABLE_MACRO_STRING( START_SOUND, m_szStartSound, CGameInstructorSymbol )					\
																										\
	LESSON_VARIABLE_MACRO( ICON_TARGET_POS, m_iIconTargetPos, int )								\
	LESSON_VARIABLE_MACRO_STRING( HUD_HINT_AFTER_LEARNED, m_szHudHint, CGameInstructorSymbol )					\


// Create keyvalues name symbol
#define LESSON_VARIABLE_SYMBOL( _varEnum, _varName, _varType ) static int g_n##_varEnum##Symbol;

#define LESSON_VARIABLE_INIT_SYMBOL( _varEnum, _varName, _varType ) g_n##_varEnum##Symbol = KeyValuesSystem()->GetSymbolForString( #_varEnum );

#define LESSON_SCRIPT_STRING_ADD_TO_MAP( _varEnum, _varName, _varType ) g_NameToTypeMap.Insert( #_varEnum, LESSON_VARIABLE_##_varEnum## );

// Create enum value
#define LESSON_VARIABLE_ENUM( _varEnum, _varName, _varType ) LESSON_VARIABLE_##_varEnum##,

// Init info call
#define LESSON_VARIABLE_INIT_INFO_CALL( _varEnum, _varName, _varType ) g_pLessonVariableInfo[ LESSON_VARIABLE_##_varEnum## ].Init_##_varEnum##();

// Init info
#define LESSON_VARIABLE_INIT_INFO( _varEnum, _varName, _varType ) \
	void Init_##_varEnum##() \
	{ \
		iOffset = offsetof( CScriptedIconLesson, CScriptedIconLesson::##_varName## ); \
		varType = LessonParamTypeFromString( #_varType ); \
	}

#define LESSON_VARIABLE_INIT_INFO_BOOL( _varEnum, _varName, _varType ) \
	void Init_##_varEnum##() \
	{ \
		iOffset = offsetof( CScriptedIconLesson, CScriptedIconLesson::##_varName## ); \
		varType = FIELD_BOOLEAN; \
	}

#define LESSON_VARIABLE_INIT_INFO_EHANDLE( _varEnum, _varName, _varType ) \
	void Init_##_varEnum##() \
	{ \
		iOffset = offsetof( CScriptedIconLesson, CScriptedIconLesson::##_varName## ); \
		varType = FIELD_EHANDLE; \
	}

#define LESSON_VARIABLE_INIT_INFO_STRING( _varEnum, _varName, _varType ) \
	void Init_##_varEnum##() \
	{ \
		iOffset = offsetof( CScriptedIconLesson, CScriptedIconLesson::##_varName## ); \
		varType = FIELD_STRING; \
	}

// Copy defaults into this scripted lesson into a new one
#define LESSON_VARIABLE_DEFAULT( _varEnum, _varName, _varType ) ( _varName = m_pDefaultHolder->_varName );

// Copy a variable from this scripted lesson into a new one
#define LESSON_VARIABLE_COPY( _varEnum, _varName, _varType ) ( pOpenLesson->_varName = _varName );

// Return the first param if pchName is the same as the second param
#define LESSON_SCRIPT_STRING( _type, _string ) \
	if ( Q_stricmp( pchName, _string ) == 0 )\
	{\
		return _type;\
	}

// Wrapper for using this macro in the factory
#define LESSON_SCRIPT_STRING_GENERAL( _varEnum, _varName, _varType ) LESSON_SCRIPT_STRING( LESSON_VARIABLE_##_varEnum##, #_varEnum )

// Process the element action on this variable 
#define PROCESS_LESSON_ACTION( _varEnum, _varName, _varType ) \
	case LESSON_VARIABLE_##_varEnum##:\
		return ProcessElementAction( pLessonElement->iAction, pLessonElement->bNot, #_varName, _varName, &pLessonElement->szParam, eventParam_float );

#define PROCESS_LESSON_ACTION_EHANDLE( _varEnum, _varName, _varType ) \
	case LESSON_VARIABLE_##_varEnum##:\
		return ProcessElementAction( pLessonElement->iAction, pLessonElement->bNot, #_varName, _varName, &pLessonElement->szParam, eventParam_float, eventParam_BaseEntity, eventParam_string );

#define PROCESS_LESSON_ACTION_STRING( _varEnum, _varName, _varType ) \
	case LESSON_VARIABLE_##_varEnum##:\
		return ProcessElementAction( pLessonElement->iAction, pLessonElement->bNot, #_varName, &_varName, &pLessonElement->szParam, eventParam_string );

// Init the variable from the script (or a convar)
#define LESSON_VARIABLE_INIT( _varEnum, _varName, _varType ) \
	else if ( g_n##_varEnum##Symbol == pSubKey->GetNameSymbol() ) \
	{ \
		const char *pchParam = pSubKey->GetString(); \
		if ( pchParam && StringHasPrefix( pchParam, "convar " ) ) \
		{ \
			ConVarRef tempCVar( pchParam + Q_strlen( "convar " ) ); \
			if ( tempCVar.IsValid() ) \
			{ \
				_varName = static_cast<_varType>( tempCVar.GetFloat() ); \
			} \
			else \
			{ \
				_varName = static_cast<_varType>( 0.0f ); \
			} \
		} \
		else \
		{ \
			_varName = static_cast<_varType>( pSubKey->GetFloat() ); \
		} \
	}

#define LESSON_VARIABLE_INIT_BOOL( _varEnum, _varName, _varType ) \
	else if ( Q_stricmp( #_varEnum, pSubKey->GetName() ) == 0 ) \
	{ \
		_varName = pSubKey->GetBool(); \
	}

#define LESSON_VARIABLE_INIT_EHANDLE( _varEnum, _varName, _varType ) \
	else if ( g_n##_varEnum##Symbol == pSubKey->GetNameSymbol() ) \
	{ \
		DevWarning( "Can't initialize an EHANDLE from the instructor lesson script." ); \
	}

#define LESSON_VARIABLE_INIT_STRING( _varEnum, _varName, _varType ) \
	else if ( g_n##_varEnum##Symbol == pSubKey->GetNameSymbol() ) \
	{ \
		const char *pchParam = pSubKey->GetString(); \
		if ( pchParam && StringHasPrefix( pchParam, "convar " ) ) \
		{ \
			ConVarRef tempCVar( pchParam + Q_strlen( "convar " ) ); \
			if ( tempCVar.IsValid() ) \
			{ \
				_varName = tempCVar.GetString(); \
			} \
			else \
			{ \
				_varName = ""; \
			} \
		} \
		else \
		{ \
			_varName = pSubKey->GetString(); \
		} \
	}

// Gets a scripted variable by offset and casts it to the proper type
#define LESSON_VARIABLE_GET_FROM_OFFSET( _type, _offset ) *static_cast<_type*>( static_cast<void*>( static_cast<byte*>( static_cast<void*>( this ) ) + _offset ) )


// Enum of scripted variables
enum LessonVariable
{
	// Run enum macros on all scriptable variables (see: LESSON_VARIABLE_FACTORY definition)
#define LESSON_VARIABLE_MACRO			LESSON_VARIABLE_ENUM
#define LESSON_VARIABLE_MACRO_BOOL		LESSON_VARIABLE_ENUM
#define LESSON_VARIABLE_MACRO_EHANDLE	LESSON_VARIABLE_ENUM
#define LESSON_VARIABLE_MACRO_STRING	LESSON_VARIABLE_ENUM
	LESSON_VARIABLE_FACTORY
#undef LESSON_VARIABLE_MACRO
#undef LESSON_VARIABLE_MACRO_BOOL
#undef LESSON_VARIABLE_MACRO_EHANDLE
#undef LESSON_VARIABLE_MACRO_STRING

	LESSON_VARIABLE_TOTAL
};

// Declare the keyvalues symbols for the keynames
#define LESSON_VARIABLE_MACRO			LESSON_VARIABLE_SYMBOL
#define LESSON_VARIABLE_MACRO_BOOL		LESSON_VARIABLE_SYMBOL
#define LESSON_VARIABLE_MACRO_EHANDLE	LESSON_VARIABLE_SYMBOL
#define LESSON_VARIABLE_MACRO_STRING	LESSON_VARIABLE_SYMBOL
	LESSON_VARIABLE_FACTORY
#undef LESSON_VARIABLE_MACRO
#undef LESSON_VARIABLE_MACRO_BOOL
#undef LESSON_VARIABLE_MACRO_EHANDLE
#undef LESSON_VARIABLE_MACRO_STRING

// String lookup prototypes
LessonVariable LessonVariableFromString( const char *pchName, bool bWarnOnInvalidNames = true );
_fieldtypes LessonParamTypeFromString( const char *pchName );
int LessonActionFromString( const char *pchName );


// This is used to get type info an variable offsets from the variable enumerated value
class LessonVariableInfo
{
public:

	LessonVariableInfo() 
		: iOffset( 0 ), varType( FIELD_VOID )
	{
	}

	// Run init info macros on all scriptable variables (see: LESSON_VARIABLE_FACTORY definition)
#define LESSON_VARIABLE_MACRO			LESSON_VARIABLE_INIT_INFO
#define LESSON_VARIABLE_MACRO_BOOL		LESSON_VARIABLE_INIT_INFO_BOOL
#define LESSON_VARIABLE_MACRO_EHANDLE	LESSON_VARIABLE_INIT_INFO_EHANDLE
#define LESSON_VARIABLE_MACRO_STRING	LESSON_VARIABLE_INIT_INFO_STRING
	LESSON_VARIABLE_FACTORY
#undef LESSON_VARIABLE_MACRO
#undef LESSON_VARIABLE_MACRO_BOOL
#undef LESSON_VARIABLE_MACRO_EHANDLE
#undef LESSON_VARIABLE_MACRO_STRING

public:

	int iOffset;
	_fieldtypes varType;
};

LessonVariableInfo g_pLessonVariableInfo[ LESSON_VARIABLE_TOTAL ];


const LessonVariableInfo *GetLessonVariableInfo( int iLessonVariable )
{
	Assert( iLessonVariable >= 0 && iLessonVariable < LESSON_VARIABLE_TOTAL );

	if ( g_pLessonVariableInfo[ 0 ].varType == FIELD_VOID )
	{
		// Run init info call macros on all scriptable variables (see: LESSON_VARIABLE_FACTORY definition)
#define LESSON_VARIABLE_MACRO			LESSON_VARIABLE_INIT_INFO_CALL
#define LESSON_VARIABLE_MACRO_BOOL		LESSON_VARIABLE_INIT_INFO_CALL
#define LESSON_VARIABLE_MACRO_EHANDLE	LESSON_VARIABLE_INIT_INFO_CALL
#define LESSON_VARIABLE_MACRO_STRING	LESSON_VARIABLE_INIT_INFO_CALL
		LESSON_VARIABLE_FACTORY
#undef LESSON_VARIABLE_MACRO
#undef LESSON_VARIABLE_MACRO_BOOL
#undef LESSON_VARIABLE_MACRO_EHANDLE
#undef LESSON_VARIABLE_MACRO_STRING
	}

	return &(g_pLessonVariableInfo[ iLessonVariable ]);
}

static CUtlDict< LessonVariable, int > g_NameToTypeMap;
static CUtlDict< fieldtype_t, int > g_TypeToParamTypeMap;
CUtlDict< int, int > CScriptedIconLesson::LessonActionMap;

CScriptedIconLesson::~CScriptedIconLesson()
{
	if ( m_pDefaultHolder )
	{
		delete m_pDefaultHolder;
		m_pDefaultHolder = NULL;
	}
}


void CScriptedIconLesson::Init()
{
	m_hLocalPlayer.Set( NULL );
	m_fOutput = 0.0f;
	m_hEntity1.Set( NULL );
	m_hEntity2.Set( NULL );
	m_szString1 = "";
	m_szString2 = "";
	m_iInteger1 = 0;
	m_iInteger2 = 0;
	m_fFloat1	= 0.0f;
	m_fFloat2	= 0.0f;

	m_fUpdateEventTime	= 0.0f;
	m_pDefaultHolder	= NULL;
	m_iScopeDepth		= 0;

	if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
	{
		ConColorMsg( CBaseLesson::m_rgbaVerboseHeader, "GAME INSTRUCTOR: " );
		ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "Initializing scripted lesson " );
		ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"", GetName() );
		ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "...\n" );
	}

	if ( !IsDefaultHolder() )
	{
		if ( !IsOpenOpportunity() )
		{
			// Initialize from the key value file
			InitFromKeys( GetGameInstructor().GetScriptKeys() );

			if ( m_iPriority >= LESSON_PRIORITY_MAX )
			{
				DevWarning( "Priority level not set for lesson: %s\n", GetName() );
			}

			// We use this to remember variable defaults to be reset before each open attempt
			m_pDefaultHolder = new CScriptedIconLesson( GetName(), true, false );
			CScriptedIconLesson *pOpenLesson = m_pDefaultHolder;

			// Run copy macros on all default scriptable variables (see: LESSON_VARIABLE_FACTORY definition)
#define LESSON_VARIABLE_MACRO			LESSON_VARIABLE_COPY
#define LESSON_VARIABLE_MACRO_BOOL		LESSON_VARIABLE_COPY
#define LESSON_VARIABLE_MACRO_EHANDLE	LESSON_VARIABLE_COPY
#define LESSON_VARIABLE_MACRO_STRING	LESSON_VARIABLE_COPY
			LESSON_VARIABLE_FACTORY
#undef LESSON_VARIABLE_MACRO
#undef LESSON_VARIABLE_MACRO_BOOL
#undef LESSON_VARIABLE_MACRO_EHANDLE
#undef LESSON_VARIABLE_MACRO_STRING

			// Listen for open events
			for ( int iLessonEvent = 0; iLessonEvent < m_OpenEvents.Count(); ++iLessonEvent )
			{
				const LessonEvent_t *pLessonEvent = &(m_OpenEvents[ iLessonEvent ]);
				ListenForGameEvent( pLessonEvent->szEventName.String() );

				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tListen for open event " );
					ConColorMsg( CBaseLesson::m_rgbaVerboseOpen, "\"%s\"", pLessonEvent->szEventName.String());
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ".\n" );
				}
			}

			// Listen for close events
			for ( int iLessonEvent = 0; iLessonEvent < m_CloseEvents.Count(); ++iLessonEvent )
			{
				const LessonEvent_t *pLessonEvent = &(m_CloseEvents[ iLessonEvent ]);
				ListenForGameEvent( pLessonEvent->szEventName.String() );

				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tListen for close event " );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\"%s\"", pLessonEvent->szEventName.String());
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ".\n" );
				}
			}

			// Listen for success events
			for ( int iLessonEvent = 0; iLessonEvent < m_SuccessEvents.Count(); ++iLessonEvent )
			{
				const LessonEvent_t *pLessonEvent = &(m_SuccessEvents[ iLessonEvent ]);
				ListenForGameEvent( pLessonEvent->szEventName.String());

				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tListen for success event " );
					ConColorMsg( CBaseLesson::m_rgbaVerboseSuccess, "\"%s\"", pLessonEvent->szEventName.String());
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ".\n" );
				}
			}
		}
		else
		{
			// This is an open lesson! Get the root for reference
			const CScriptedIconLesson *pLesson = static_cast<const CScriptedIconLesson *>( GetGameInstructor().GetLesson( GetName() ) );
			SetRoot( const_cast<CScriptedIconLesson *>( pLesson ) );
		}
	}
}

void CScriptedIconLesson::InitPrerequisites()
{
	if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
	{
		ConColorMsg( CBaseLesson::m_rgbaVerboseHeader, "GAME INSTRUCTOR: " );
		ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "Initializing prereqs for scripted lesson " );
		ConColorMsg( CBaseLesson::m_rgbaVerboseOpen, "\"%s\"", GetName() );
		ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "...\n" );
	}

	for ( int iPrerequisit = 0; iPrerequisit < m_PrerequisiteNames.Count(); ++iPrerequisit )
	{
		const char *pPrerequisiteName = m_PrerequisiteNames[ iPrerequisit ].String();
		AddPrerequisite( pPrerequisiteName );
	}
}

void CScriptedIconLesson::OnOpen()
{
	VPROF_BUDGET( "CScriptedIconLesson::OnOpen", "GameInstructor" );

	if ( !DoDelayedPlayerSwaps() )
	{
		return;
	}

	const CScriptedIconLesson *pLesson = static_cast<const CScriptedIconLesson *>( GetRoot() );

	// Process all update events
	for ( int iLessonEvent = 0; iLessonEvent < pLesson->m_OnOpenEvents.Count(); ++iLessonEvent )
	{
		const LessonEvent_t *pLessonEvent = &(pLesson->m_OnOpenEvents[ iLessonEvent ]);

		if ( gameinstructor_verbose.GetInt() > 1 && ShouldShowSpew() )
		{
			ConColorMsg( Color( 255, 128, 64, 255 ), "GAME INSTRUCTOR: " );
			ConColorMsg( Color( 64, 128, 255, 255 ), "OnOpen event " );
			ConColorMsg( Color( 0, 255, 0, 255 ), "\"%s\"", pLessonEvent->szEventName.String());
			ConColorMsg( Color( 64, 128, 255, 255 ), "received for lesson \"%s\"...\n", GetName() );
		}

		ProcessElements( NULL, &(pLessonEvent->elements) );
	}

	BaseClass::OnOpen();
}

void CScriptedIconLesson::Update()
{
	VPROF_BUDGET( "CScriptedIconLesson::Update", "GameInstructor" );

	if ( !DoDelayedPlayerSwaps() )
	{
		return;
	}

	const CScriptedIconLesson *pLesson = static_cast<const CScriptedIconLesson *>( GetRoot() );

	if ( gpGlobals->curtime >= m_fUpdateEventTime )
	{
		bool bShowSpew = ( gameinstructor_verbose.GetInt() > 1 && ShouldShowSpew() );

		int iVerbose = gameinstructor_verbose.GetInt();
		if ( gameinstructor_verbose.GetInt() == 1 )
		{
			// Force the verbose level from 1 to 0 for update events
			gameinstructor_verbose.SetValue( 0 );
		}

		// Process all update events
		for ( int iLessonEvent = 0; iLessonEvent < pLesson->m_UpdateEvents.Count(); ++iLessonEvent )
		{
			const LessonEvent_t *pLessonEvent = &(pLesson->m_UpdateEvents[ iLessonEvent ]);

			if ( bShowSpew )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerboseHeader, "GAME INSTRUCTOR: " );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "Update event " );
				ConColorMsg( CBaseLesson::m_rgbaVerboseUpdate, "\"%s\"", pLessonEvent->szEventName.String());
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "received for lesson \"%s\"...\n", GetName() );
			}

			ProcessElements( NULL, &(pLessonEvent->elements) );
		}

		gameinstructor_verbose.SetValue( iVerbose );

		// Wait before doing update events again
		m_fUpdateEventTime = gpGlobals->curtime + m_fUpdateInterval;
	}

	BaseClass::Update();
}

void CScriptedIconLesson::SwapOutPlayers( int iOldUserID, int iNewUserID )
{
	BaseClass::SwapOutPlayers( iOldUserID, iNewUserID );

	// Get the player pointers from the user IDs
	C_BasePlayer *pOldPlayer = UTIL_PlayerByUserId( iOldUserID );
	C_BasePlayer *pNewPlayer = UTIL_PlayerByUserId( iNewUserID );

	if ( pOldPlayer == m_hEntity1.Get() )
	{
		if ( pNewPlayer )
		{
			m_hEntity1 = pNewPlayer;
		}
		else
		{
			if ( m_iNumDelayedPlayerSwaps < MAX_DELAYED_PLAYER_SWAPS )
			{
				m_pDelayedPlayerSwap[ m_iNumDelayedPlayerSwaps ].phHandleToChange = &m_hEntity1;
				m_pDelayedPlayerSwap[ m_iNumDelayedPlayerSwaps ].iNewUserID = iNewUserID;
				++m_iNumDelayedPlayerSwaps;
			}
		}
	}

	if ( pOldPlayer == m_hEntity2.Get() )
	{
		if ( pNewPlayer )
		{
			m_hEntity2 = pNewPlayer;
		}
		else
		{

			if ( m_iNumDelayedPlayerSwaps < MAX_DELAYED_PLAYER_SWAPS )
			{
				m_pDelayedPlayerSwap[ m_iNumDelayedPlayerSwaps ].phHandleToChange = &m_hEntity2;
				m_pDelayedPlayerSwap[ m_iNumDelayedPlayerSwaps ].iNewUserID = iNewUserID;
				++m_iNumDelayedPlayerSwaps;
			}
		}
	}
}

void CScriptedIconLesson::FireGameEvent( IGameEvent *event )
{
	VPROF_BUDGET( "CScriptedIconLesson::FireGameEvent", "GameInstructor" );

	if ( m_bDisabled )
		return;

	if ( !DoDelayedPlayerSwaps() )
	{
		return;
	}

	if ( !C_BasePlayer::GetLocalPlayer() )
		return;

	// Check that this lesson is allowed for the current input device
	if( m_bOnlyKeyboard /*&& input->ControllerModeActive()*/ )
		return;

	if( m_bOnlyGamepad /*&& !input->ControllerModeActive()*/ )
		return;

	// Check that this lesson is for the proper team
	CBasePlayer *pLocalPlayer = GetGameInstructor().GetLocalPlayer();

	if ( m_iTeam != TEAM_ANY && pLocalPlayer && pLocalPlayer->GetTeamNumber() != m_iTeam )
	{
		// This lesson is intended for a different team
		return;
	}

	const char *name = event->GetName();

	// Open events run on the root
	ProcessOpenGameEvents( this, name, event );

	// Close and success events run on the children
	const CUtlVector < CBaseLesson * > *pChildren = GetChildren();
	for ( int iChild = 0; iChild < pChildren->Count(); ++iChild )
	{
		CScriptedIconLesson *pScriptedChild = dynamic_cast<CScriptedIconLesson*>( (*pChildren)[ iChild ] );
		
		pScriptedChild->ProcessCloseGameEvents( this, name, event );
		pScriptedChild->ProcessSuccessGameEvents( this, name, event );
	}
}

void CScriptedIconLesson::ProcessOpenGameEvents( const CScriptedIconLesson *pRootLesson, const char *name, IGameEvent *event )
{
	if ( pRootLesson->InstanceType() == LESSON_INSTANCE_SINGLE_OPEN && GetGameInstructor().IsLessonOfSameTypeOpen( this ) )
	{
		// We don't want more than one of this type, and there is already one open
		if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
		{
			ConColorMsg( CBaseLesson::m_rgbaVerboseHeader, "GAME INSTRUCTOR: " );
			ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "Opportunity " );
			ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\"%s\" ", pRootLesson->GetName() );
			ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "open events NOT processed (there is already an open lesson of this type).\n" );
		}

		return;
	}

	for ( int iLessonEvent = 0; iLessonEvent < pRootLesson->m_OpenEvents.Count(); ++iLessonEvent )
	{
		const LessonEvent_t *pLessonEvent = &(pRootLesson->m_OpenEvents[ iLessonEvent ]);

		if ( Q_strcmp( name, pLessonEvent->szEventName.String()) == 0 )
		{
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerboseHeader, "GAME INSTRUCTOR: " );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "Open event " );
				ConColorMsg( CBaseLesson::m_rgbaVerboseOpen, "\"%s\"", pLessonEvent->szEventName.String());
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "received for lesson \"%s\"...\n", GetName() );
			}

			if ( m_pDefaultHolder )
			{
				// Run copy from default macros on all scriptable variables (see: LESSON_VARIABLE_FACTORY definition)
#define LESSON_VARIABLE_MACRO			LESSON_VARIABLE_DEFAULT
#define LESSON_VARIABLE_MACRO_BOOL		LESSON_VARIABLE_DEFAULT
#define LESSON_VARIABLE_MACRO_EHANDLE	LESSON_VARIABLE_DEFAULT
#define LESSON_VARIABLE_MACRO_STRING	LESSON_VARIABLE_DEFAULT
				LESSON_VARIABLE_FACTORY
#undef LESSON_VARIABLE_MACRO
#undef LESSON_VARIABLE_MACRO_BOOL
#undef LESSON_VARIABLE_MACRO_EHANDLE
#undef LESSON_VARIABLE_MACRO_STRING
			}

			if ( ProcessElements( event, &(pLessonEvent->elements) ) )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerboseOpen, "\tAll elements returned true. Opening!\n" );
				}

				MEM_ALLOC_CREDIT();
				CScriptedIconLesson *pOpenLesson = new CScriptedIconLesson( GetName(), false, true );

				// Run copy macros on all scriptable variables (see: LESSON_VARIABLE_FACTORY definition)
#define LESSON_VARIABLE_MACRO			LESSON_VARIABLE_COPY
#define LESSON_VARIABLE_MACRO_BOOL		LESSON_VARIABLE_COPY
#define LESSON_VARIABLE_MACRO_EHANDLE	LESSON_VARIABLE_COPY
#define LESSON_VARIABLE_MACRO_STRING	LESSON_VARIABLE_COPY
				LESSON_VARIABLE_FACTORY
#undef LESSON_VARIABLE_MACRO
#undef LESSON_VARIABLE_MACRO_BOOL
#undef LESSON_VARIABLE_MACRO_EHANDLE
#undef LESSON_VARIABLE_MACRO_STRING

				if ( GetGameInstructor().OpenOpportunity( pOpenLesson ) )
				{
					pOpenLesson->OnOpen();

					if ( pRootLesson->InstanceType() == LESSON_INSTANCE_SINGLE_OPEN )
					{
						// This one is open and we only want one! So, we're done.
						// Other open events may be listening for the same events... skip them!
						return;
					}
				}
			}
		}
	}
}

void CScriptedIconLesson::ProcessCloseGameEvents( const CScriptedIconLesson *pRootLesson, const char *name, IGameEvent *event )
{
	for ( int iLessonEvent = 0; iLessonEvent < pRootLesson->m_CloseEvents.Count() && IsOpenOpportunity(); ++iLessonEvent )
	{
		const LessonEvent_t *pLessonEvent = &(pRootLesson->m_CloseEvents[ iLessonEvent ]);

		if ( Q_strcmp( name, pLessonEvent->szEventName.String()) == 0 )
		{
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerboseHeader, "GAME INSTRUCTOR: " );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "Close event " );
				ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\"%s\"", pLessonEvent->szEventName.String());
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "received for lesson \"%s\"...\n", GetName() );
			}

			if ( ProcessElements( event, &(pLessonEvent->elements) ) )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tAll elements returned true. Closing!\n" );
				}

				CloseOpportunity( "Close event elements completed." );
			}
		}
	}
}

void CScriptedIconLesson::ProcessSuccessGameEvents( const CScriptedIconLesson *pRootLesson, const char *name, IGameEvent *event )
{
	for ( int iLessonEvent = 0; iLessonEvent < pRootLesson->m_SuccessEvents.Count(); ++iLessonEvent )
	{
		const LessonEvent_t *pLessonEvent = &(pRootLesson->m_SuccessEvents[ iLessonEvent ]);

		if ( Q_strcmp( name, pLessonEvent->szEventName.String()) == 0 )
		{
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerboseHeader, "GAME INSTRUCTOR: " );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "Success event " );
				ConColorMsg( CBaseLesson::m_rgbaVerboseSuccess, "\"%s\"", pLessonEvent->szEventName.String());
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "received for lesson \"%s\"...\n", GetName() );
			}

			if ( ProcessElements( event, &(pLessonEvent->elements) ) )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerboseSuccess, "\tAll elements returned true. Succeeding!\n" );
				}

				MarkSucceeded();
			}
		}
	}
}

LessonVariable LessonVariableFromString( const char *pchName, bool bWarnOnInvalidNames )
{
	int slot = g_NameToTypeMap.Find( pchName );
	if ( slot != g_NameToTypeMap.InvalidIndex() )
		return g_NameToTypeMap[ slot ];

	if ( bWarnOnInvalidNames )
	{
		AssertMsg( 0, "Invalid scripted lesson variable!" );
		DevWarning( "Invalid scripted lesson variable: %s\n", pchName );
	}
	
	return LESSON_VARIABLE_TOTAL;
}

_fieldtypes LessonParamTypeFromString( const char *pchName )
{
	int slot = g_TypeToParamTypeMap.Find( pchName );
	if ( slot != g_TypeToParamTypeMap.InvalidIndex() )
		return g_TypeToParamTypeMap[ slot ];

	DevWarning( "Invalid scripted lesson variable/param type: %s\n", pchName );
	return FIELD_VOID;
}

int LessonActionFromString( const char *pchName )
{
	int slot = CScriptedIconLesson::LessonActionMap.Find( pchName );
	if ( slot != CScriptedIconLesson::LessonActionMap.InvalidIndex() )
		return CScriptedIconLesson::LessonActionMap[ slot ];

	DevWarning( "Invalid scripted lesson action: %s\n", pchName );
	return LESSON_ACTION_NONE;
}

void CScriptedIconLesson::InitElementsFromKeys( CUtlVector< LessonElement_t > *pLessonElements, KeyValues *pKey )
{
	KeyValues *pSubKey = NULL;
	for ( pSubKey = pKey->GetFirstSubKey(); pSubKey; pSubKey = pSubKey->GetNextKey() )
	{
		char szSubKeyName[ 256 ];
		Q_strcpy( szSubKeyName, pSubKey->GetName() );

		char *pchToken = strtok( szSubKeyName, " " );
		LessonVariable iVariable = LessonVariableFromString( pchToken );

		pchToken = strtok ( NULL, "" );
		int iAction = LESSON_ACTION_NONE;
		bool bNot = false;
		bool bOptionalParam = false;
		
		if ( !pchToken || pchToken[ 0 ] == '\0' )
		{
			DevWarning( "No action specified for variable: \"%s\"\n", pSubKey->GetName() );
		}
		else
		{
			if ( pchToken[ 0 ] == '?' )
			{
				pchToken++;
				bOptionalParam = true;
			}

			if ( pchToken[ 0 ] == '!' )
			{
				pchToken++;
				bNot = true;
			}
			
			iAction = LessonActionFromString( pchToken );
		}

		Q_strcpy( szSubKeyName, pSubKey->GetString() );

		pchToken = strtok( szSubKeyName, " " );
		_fieldtypes paramType = LessonParamTypeFromString( pchToken );

		char *pchParam = "";

		if ( paramType != FIELD_VOID )
		{
			pchToken = strtok ( NULL, "" );
			pchParam = pchToken;
		}

		if ( !pchParam )
		{
			DevWarning( "No parameter specified for action: \"%s\"\n", pSubKey->GetName() );
		}
		else
		{
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t\tElement \"%s %s\" added.\n", pSubKey->GetName(), pSubKey->GetString() );
			}

			// See if our param is a scripted var
			LessonVariable iParamVarIndex = LessonVariableFromString( pchParam, false );

			pLessonElements->AddToTail( LessonElement_t( iVariable, iAction, bNot, bOptionalParam, pchParam, iParamVarIndex, paramType ) );
		}
	}
}

void CScriptedIconLesson::InitElementsFromElements( CUtlVector< LessonElement_t > *pLessonElements, const CUtlVector< LessonElement_t > *pLessonElements2 )
{
	for ( int i = 0; i < pLessonElements2->Count(); ++i )
	{
		pLessonElements->AddToTail( LessonElement_t( (*pLessonElements2)[ i ] ) );
	}
}

void CScriptedIconLesson::InitFromKeys( KeyValues *pKey )
{
	if ( !pKey )
		return;

	static int s_nInstanceTypeSymbol = KeyValuesSystem()->GetSymbolForString( "instance_type" );
	static int s_nReplaceKeySymbol = KeyValuesSystem()->GetSymbolForString( "replace_key" );
	static int s_nFixedInstancesMaxSymbol = KeyValuesSystem()->GetSymbolForString( "fixed_instances_max" );
	static int s_nReplaceOnlyWhenStopped = KeyValuesSystem()->GetSymbolForString( "replace_only_when_stopped" );
	static int s_nTeamSymbol = KeyValuesSystem()->GetSymbolForString( "team" );
	static int s_nOnlyKeyboardSymbol = KeyValuesSystem()->GetSymbolForString( "only_keyboard" );
	static int s_nOnlyGamepadSymbol = KeyValuesSystem()->GetSymbolForString( "only_gamepad" );
	static int s_nDisplayLimitSymbol = KeyValuesSystem()->GetSymbolForString( "display_limit" );
	static int s_nSuccessLimitSymbol = KeyValuesSystem()->GetSymbolForString( "success_limit" );
	static int s_nPreReqSymbol = KeyValuesSystem()->GetSymbolForString( "prereq" );
	static int s_nOpenSymbol = KeyValuesSystem()->GetSymbolForString( "open" );
	static int s_nCloseSymbol = KeyValuesSystem()->GetSymbolForString( "close" );
	static int s_nSuccessSymbol = KeyValuesSystem()->GetSymbolForString( "success" );
	static int s_nOnOpenSymbol = KeyValuesSystem()->GetSymbolForString( "onopen" );
	static int s_nUpdateSymbol = KeyValuesSystem()->GetSymbolForString( "update" );

	KeyValues *pSubKey = NULL;
	for ( pSubKey = pKey->GetFirstSubKey(); pSubKey; pSubKey = pSubKey->GetNextKey() )
	{
		if ( pSubKey->GetNameSymbol() == s_nInstanceTypeSymbol )
		{
			m_iInstanceType = LessonInstanceType( pSubKey->GetInt() );
		}
		else if ( pSubKey->GetNameSymbol() == s_nReplaceKeySymbol )
		{
			m_stringReplaceKey = pSubKey->GetString();
		}
		else if ( pSubKey->GetNameSymbol() == s_nFixedInstancesMaxSymbol )
		{
			m_iFixedInstancesMax = pSubKey->GetInt();
		}
		else if ( pSubKey->GetNameSymbol() == s_nReplaceOnlyWhenStopped )
		{
			m_bReplaceOnlyWhenStopped = pSubKey->GetBool();
		}
		else if ( pSubKey->GetNameSymbol() == s_nTeamSymbol )
		{
			m_iTeam = pSubKey->GetInt();
		}
		else if ( pSubKey->GetNameSymbol() == s_nOnlyKeyboardSymbol )
		{
			m_bOnlyKeyboard = pSubKey->GetBool();
		}
		else if ( pSubKey->GetNameSymbol() == s_nOnlyGamepadSymbol )
		{
			m_bOnlyGamepad = pSubKey->GetBool();
		}
		else if ( pSubKey->GetNameSymbol() == s_nDisplayLimitSymbol )
		{
			m_iDisplayLimit = pSubKey->GetInt();
		}
		else if ( pSubKey->GetNameSymbol() == s_nSuccessLimitSymbol )
		{
			m_iSuccessLimit = pSubKey->GetInt();
		}
		else if ( pSubKey->GetNameSymbol() == s_nPreReqSymbol )
		{
			CGameInstructorSymbol pName;
			pName = pSubKey->GetString();
			m_PrerequisiteNames.AddToTail( pName );
		}
		else if ( pSubKey->GetNameSymbol() == s_nOpenSymbol )
		{
			KeyValues *pEventKey = NULL;
			for ( pEventKey = pSubKey->GetFirstTrueSubKey(); pEventKey; pEventKey = pEventKey->GetNextTrueSubKey() )
			{
				LessonEvent_t *pLessonEvent = AddOpenEvent();
				pLessonEvent->szEventName = pEventKey->GetName();

				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tAdding open event " );
					ConColorMsg( CBaseLesson::m_rgbaVerboseOpen, "\"%s\" ", pLessonEvent->szEventName.String());
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "...\n" );
				}

				InitElementsFromKeys( &(pLessonEvent->elements), pEventKey );
			}
		}
		else if ( pSubKey->GetNameSymbol() == s_nCloseSymbol )
		{
			KeyValues *pEventKey = NULL;
			for ( pEventKey = pSubKey->GetFirstTrueSubKey(); pEventKey; pEventKey = pEventKey->GetNextTrueSubKey() )
			{
				LessonEvent_t *pLessonEvent = AddCloseEvent();
				pLessonEvent->szEventName = pEventKey->GetName();

				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tAdding close event " );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\"%s\" ", pLessonEvent->szEventName.String());
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "...\n" );
				}

				InitElementsFromKeys( &(pLessonEvent->elements), pEventKey );
			}
		}
		else if ( pSubKey->GetNameSymbol() == s_nSuccessSymbol )
		{
			KeyValues *pEventKey = NULL;
			for ( pEventKey = pSubKey->GetFirstTrueSubKey(); pEventKey; pEventKey = pEventKey->GetNextTrueSubKey() )
			{
				LessonEvent_t *pLessonEvent = AddSuccessEvent();
				pLessonEvent->szEventName = pEventKey->GetName();

				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tAdding success event " );
					ConColorMsg( CBaseLesson::m_rgbaVerboseSuccess, "\"%s\" ", pLessonEvent->szEventName.String());
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "...\n" );
				}

				InitElementsFromKeys( &(pLessonEvent->elements), pEventKey );
			}
		}
		else if ( pSubKey->GetNameSymbol() == s_nOnOpenSymbol )
		{
			KeyValues *pEventKey = NULL;
			for ( pEventKey = pSubKey->GetFirstTrueSubKey(); pEventKey; pEventKey = pEventKey->GetNextTrueSubKey() )
			{
				LessonEvent_t *pLessonEvent = AddOnOpenEvent();
				pLessonEvent->szEventName = pEventKey->GetName();

				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tAdding onopen event " );
					ConColorMsg( CBaseLesson::m_rgbaVerboseOpen, "\"%s\" ", pLessonEvent->szEventName.String());
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "...\n" );
				}

				InitElementsFromKeys( &(pLessonEvent->elements), pEventKey );
			}
		}
		else if ( pSubKey->GetNameSymbol() == s_nUpdateSymbol )
		{
			KeyValues *pEventKey = NULL;
			for ( pEventKey = pSubKey->GetFirstTrueSubKey(); pEventKey; pEventKey = pEventKey->GetNextTrueSubKey() )
			{
				LessonEvent_t *pLessonEvent = AddUpdateEvent();
				pLessonEvent->szEventName = pEventKey->GetName();

				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tAdding update event " );
					ConColorMsg( CBaseLesson::m_rgbaVerboseUpdate, "\"%s\" ", pLessonEvent->szEventName.String());
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "...\n" );
				}

				InitElementsFromKeys( &(pLessonEvent->elements), pEventKey );
			}
		}

		// Run intialize from key macros on all scriptable variables (see: LESSON_VARIABLE_FACTORY definition)
#define LESSON_VARIABLE_MACRO			LESSON_VARIABLE_INIT
#define LESSON_VARIABLE_MACRO_BOOL		LESSON_VARIABLE_INIT_BOOL
#define LESSON_VARIABLE_MACRO_EHANDLE	LESSON_VARIABLE_INIT_EHANDLE
#define LESSON_VARIABLE_MACRO_STRING	LESSON_VARIABLE_INIT_STRING
		LESSON_VARIABLE_FACTORY
#undef LESSON_VARIABLE_MACRO
#undef LESSON_VARIABLE_MACRO_BOOL
#undef LESSON_VARIABLE_MACRO_EHANDLE
#undef LESSON_VARIABLE_MACRO_STRING
	}
}

bool CScriptedIconLesson::ProcessElements( IGameEvent *event, const CUtlVector< LessonElement_t > *pElements )
{
	VPROF_BUDGET( "CScriptedIconLesson::ProcessElements", "GameInstructor" );

	m_hLocalPlayer = GetGameInstructor().GetLocalPlayer();

	bool bSuccess = true;
	int nContinueScope = -1;
	m_iScopeDepth = 0;

	if ( gameinstructor_find_errors.GetBool() )
	{
		// Just run them all to check for errors!
		for ( int iElement = 0; iElement < pElements->Count(); ++iElement )
		{
			ProcessElement( event, &((*pElements)[ iElement ] ), false );
		}

		return false;
	}

	// Process each element until a step fails
	for ( int iElement = 0; iElement < pElements->Count(); ++iElement )
	{
		if ( nContinueScope == m_iScopeDepth )
		{
			nContinueScope = -1;
		}

		if ( !ProcessElement( event, &((*pElements)[ iElement ]), nContinueScope != -1 ) )
		{
			// This element failed
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tPrevious element returned false.\n" );
			}

			nContinueScope = m_iScopeDepth - 1;

			if ( nContinueScope < 0 )
			{
				// No outer scope to worry about, we're done
				bSuccess = false;
				break;
			}
		}
	}

	return bSuccess;
}

bool CScriptedIconLesson::ProcessElement( IGameEvent *event, const LessonElement_t *pLessonElement, bool bInFailedScope )
{
	VPROF_BUDGET( "CScriptedIconLesson::ProcessElement", "GameInstructor" );

	if ( pLessonElement->iAction == LESSON_ACTION_SCOPE_IN )
	{
		// Special case for closing (we don't need variables for this)
		if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
		{
			ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tScopeIn()\n" );
		}

		m_iScopeDepth++;
		return true;
	}
	else if ( pLessonElement->iAction == LESSON_ACTION_SCOPE_OUT )
	{
		// Special case for closing (we don't need variables for this)
		if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
		{
			ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tScopeOut()\n" );
		}

		m_iScopeDepth--;
		return true;
	}

	if ( bInFailedScope )
	{
		// Only scope upkeep is done when we're in a failing scope... bail!
		return true;
	}

	if ( pLessonElement->iAction == LESSON_ACTION_CLOSE )
	{
		// Special case for closing (we don't need variables for this)
		if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
		{
			ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tCloseOpportunity()\n" );
		}

		CloseOpportunity( "Close action." );
		return true;
	}
	else if ( pLessonElement->iAction == LESSON_ACTION_SUCCESS )
	{
		// Special case for succeeding (we don't need variables for this)
		if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
		{
			ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tMarkSucceeded()\n" );
		}

		MarkSucceeded();
		return true;
	}
	else if ( pLessonElement->iAction == LESSON_ACTION_LOCK )
	{
		// Special case for setting the starting point for the lesson to stay locked from (we don't need variables for this)
		if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
		{
			ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tm_fLockTime = gpGlobals->curtime\n" );
		}

		m_fLockTime = gpGlobals->curtime;
		return true;
	}
	else if ( pLessonElement->iAction == LESSON_ACTION_PRESENT_COMPLETE )
	{
		// Special case for checking presentation status (we don't need variables for this)
		bool bPresentComplete = IsPresentComplete();

		if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
		{
			ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tIsPresentComplete() " );
			ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%s ", ( bPresentComplete ) ? ( "true" ) : ( "false" ) );
			ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( pLessonElement->bNot ) ? ( "!= true\n" ) : ( "== true\n" ) );
		}

		return ( pLessonElement->bNot ) ? ( !bPresentComplete ) : ( bPresentComplete );
	}
	else if ( pLessonElement->iAction == LESSON_ACTION_PRESENT_START )
	{
		// Special case for setting presentation status (we don't need variables for this)
		if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
		{
			ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tPresentStart()\n" );
		}

		PresentStart();
		return true;
	}
	else if ( pLessonElement->iAction == LESSON_ACTION_PRESENT_END )
	{
		// Special case for setting presentation status (we don't need variables for this)
		if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
		{
			ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tPresentEnd()\n" );
		}

		PresentEnd();
		return true;
	}

	// These values temporarily hold the parameter's value
	const char *pParamName = pLessonElement->szParam.String();
	float eventParam_float = 0.0f;
	char eventParam_string[ 256 ];
	eventParam_string[ 0 ] = '\0';
	C_BaseEntity *eventParam_BaseEntity = NULL;

	// Get the value from the event parameter based on its type
	switch ( pLessonElement->paramType )
	{
	case FIELD_FLOAT:
		if ( pLessonElement->iParamVarIndex < LESSON_VARIABLE_TOTAL )
		{
			// The parameter is a scripted var
			const LessonVariableInfo *pInfo = GetLessonVariableInfo( pLessonElement->iParamVarIndex );

			switch ( pInfo->varType )
			{
			case FIELD_FLOAT:
				eventParam_float = LESSON_VARIABLE_GET_FROM_OFFSET( float, pInfo->iOffset );
				break;
			case FIELD_INTEGER:
				eventParam_float = static_cast<float>( LESSON_VARIABLE_GET_FROM_OFFSET( int, pInfo->iOffset ) );
				break;
			case FIELD_BOOLEAN:
				eventParam_float = static_cast<float>( LESSON_VARIABLE_GET_FROM_OFFSET( bool, pInfo->iOffset ) );
				break;
			case FIELD_STRING:
				eventParam_float = static_cast<float>( atoi( &LESSON_VARIABLE_GET_FROM_OFFSET( CGameInstructorSymbol, pInfo->iOffset )->String() ) );
				break;
			case FIELD_EHANDLE:
			case FIELD_FUNCTION:
				DevWarning( "Can't use this variable type with this parameter type in lesson script.\n" );
				break;
			}
		}
		else if ( event && !(event->IsEmpty( pParamName )) )
		{
			eventParam_float = event->GetFloat( pParamName );
		}
		else if ( pLessonElement->bOptionalParam )
		{
			// We don't want to interpret this and not finding the param is still ok
			return true;
		}
		else if ( ( pParamName[ 0 ] >= '0' && pParamName[ 0 ] <= '9' ) || pParamName[ 0 ] == '-' || pParamName[ 0 ] == '.' )
		{
			// This param doesn't exist, try parsing the string
			eventParam_float = Q_atof( pParamName );
		}
		else
		{
			DevWarning( "Invalid event field name and not a float \"%s\".\n", pParamName );
			return false;
		}
		break;

	case FIELD_INTEGER:
		if ( pLessonElement->iParamVarIndex < LESSON_VARIABLE_TOTAL )
		{
			// The parameter is a scripted var
			const LessonVariableInfo *pInfo = GetLessonVariableInfo( pLessonElement->iParamVarIndex );

			switch ( pInfo->varType )
			{
			case FIELD_FLOAT:
				eventParam_float = static_cast<int>( LESSON_VARIABLE_GET_FROM_OFFSET( float, pInfo->iOffset ) );
				break;
			case FIELD_INTEGER:
				eventParam_float = LESSON_VARIABLE_GET_FROM_OFFSET( int, pInfo->iOffset );
				break;
			case FIELD_BOOLEAN:
				eventParam_float = static_cast<int>( LESSON_VARIABLE_GET_FROM_OFFSET( bool, pInfo->iOffset ) );
				break;
			case FIELD_STRING:
				eventParam_float = atof( &LESSON_VARIABLE_GET_FROM_OFFSET( CGameInstructorSymbol, pInfo->iOffset )->String() );
				break;
			case FIELD_EHANDLE:
			case FIELD_FUNCTION:
				DevWarning( "Can't use this variable type with this parameter type in lesson script.\n" );
				break;
			}
		}
		else if ( event && !(event->IsEmpty( pParamName )) )
		{
			eventParam_float = static_cast<float>( event->GetInt( pParamName ) );
		}
		else if ( pLessonElement->bOptionalParam )
		{
			// We don't want to interpret this and not finding the param is still ok
			return true;
		}
		else if ( ( pParamName[ 0 ] >= '0' && pParamName[ 0 ] <= '9' ) || pParamName[ 0 ] == '-' )
		{
			// This param doesn't exist, try parsing the string
			eventParam_float = static_cast<float>( Q_atoi( pParamName ) );
		}
		else
		{
			DevWarning( "Invalid event field name and not an integer \"%s\".\n", pParamName );
			return false;
		}
		break;

	case FIELD_STRING:
		if ( pLessonElement->iParamVarIndex < LESSON_VARIABLE_TOTAL )
		{
			// The parameter is a scripted var
			const LessonVariableInfo *pInfo = GetLessonVariableInfo( pLessonElement->iParamVarIndex );

			switch ( pInfo->varType )
			{
			case FIELD_STRING:
				Q_strncpy( eventParam_string, &LESSON_VARIABLE_GET_FROM_OFFSET( CGameInstructorSymbol, pInfo->iOffset )->String(), sizeof( eventParam_string ) );
				break;
			case FIELD_FLOAT:
				Q_snprintf( eventParam_string, sizeof( eventParam_string ), "%f", LESSON_VARIABLE_GET_FROM_OFFSET( float, pInfo->iOffset ) );
				break;
			case FIELD_INTEGER:
				Q_snprintf( eventParam_string, sizeof( eventParam_string ), "%i", LESSON_VARIABLE_GET_FROM_OFFSET( int, pInfo->iOffset ) );
				break;
			case FIELD_BOOLEAN:
			case FIELD_EHANDLE:
			case FIELD_FUNCTION:
				DevWarning( "Can't use this variable type with this parameter type in lesson script.\n" );
				break;
			}
		}
		else
		{
			const char *pchEventString = NULL;

			if ( event && !(event->IsEmpty( pParamName )) )
			{
				pchEventString = event->GetString( pParamName );
			}

			if ( pchEventString && pchEventString[0] )
			{
				Q_strcpy( eventParam_string, pchEventString );
			}
			else if ( pLessonElement->bOptionalParam )
			{
				// We don't want to interpret this and not finding the param is still ok
				return true;
			}
			else
			{
				// This param doesn't exist, try parsing the string
				Q_strncpy( eventParam_string, pParamName, sizeof( eventParam_string ) );
			}
		}
		break;

	case FIELD_BOOLEAN:
		if ( pLessonElement->iParamVarIndex < LESSON_VARIABLE_TOTAL )
		{
			// The parameter is a scripted var
			const LessonVariableInfo *pInfo = GetLessonVariableInfo( pLessonElement->iParamVarIndex );

			switch ( pInfo->varType )
			{
			case FIELD_FLOAT:
				eventParam_float = ( ( LESSON_VARIABLE_GET_FROM_OFFSET( float, pInfo->iOffset ) ) ? ( 1.0f ) : ( 0.0f ) );
				break;
			case FIELD_INTEGER:
				eventParam_float = ( ( LESSON_VARIABLE_GET_FROM_OFFSET( int, pInfo->iOffset ) ) ? ( 1.0f ) : ( 0.0f ) );
				break;
			case FIELD_BOOLEAN:
				eventParam_float = ( ( LESSON_VARIABLE_GET_FROM_OFFSET( bool, pInfo->iOffset ) ) ? ( 1.0f ) : ( 0.0f ) );
				break;
			case FIELD_EHANDLE:
			case FIELD_STRING:
			case FIELD_FUNCTION:
				DevWarning( "Can't use this variable type with this parameter type in lesson script.\n" );
				break;
			}
		}
		else if ( event && !(event->IsEmpty( pParamName )) )
		{
			eventParam_float = ( event->GetBool( pParamName ) ) ? ( 1.0f ) : ( 0.0f );
		}
		else if ( pLessonElement->bOptionalParam )
		{
			// We don't want to interpret this and not finding the param is still ok
			return true;
		}
		else if ( pParamName[ 0 ] == '0' || pParamName[ 0 ] == '1' )
		{
			// This param doesn't exist, try parsing the string
			eventParam_float = Q_atof( pParamName ) != 0.0f;
		}
		else
		{
			DevWarning( "Invalid event field name and not an boolean \"%s\".\n", pParamName );
			return false;
		}
		break;

	case FIELD_CUSTOM:
		if ( pLessonElement->iParamVarIndex < LESSON_VARIABLE_TOTAL )
		{
			// The parameter is a scripted var
			const LessonVariableInfo *pInfo = GetLessonVariableInfo( pLessonElement->iParamVarIndex );

			switch ( pInfo->varType )
			{
			case FIELD_EHANDLE:
				eventParam_BaseEntity = ( LESSON_VARIABLE_GET_FROM_OFFSET( EHANDLE, pInfo->iOffset ) ).Get();
				if ( !eventParam_BaseEntity )
				{
					if ( pLessonElement->bOptionalParam )
					{
						// Not having an entity is fine
						return true;
					}

					if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
					{
						ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tPlayer param \"%s\" returned NULL.\n", pParamName );
					}
					return false;
				}
				break;
			case FIELD_FLOAT:
			case FIELD_INTEGER:
			case FIELD_BOOLEAN:
			case FIELD_STRING:
			case FIELD_FUNCTION:
				DevWarning( "Can't use this variable type with this parameter type in lesson script.\n" );
				break;
			}
		}
		else if ( event && !(event->IsEmpty( pParamName )) )
		{
			eventParam_BaseEntity = UTIL_PlayerByUserId( event->GetInt( pParamName ) );
			if ( !eventParam_BaseEntity )
			{
				if ( pLessonElement->bOptionalParam )
				{
					// Not having an entity is fine
					return true;
				}

				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tPlayer param \"%s\" returned NULL.\n", pParamName );
				}
				return false;
			}
		}
		else if ( pLessonElement->bOptionalParam )
		{
			// We don't want to interpret this and not finding the param is still ok
			return true;
		}
		else if ( Q_stricmp( pParamName, "null" ) == 0 )
		{
			// They explicitly want a null pointer
			eventParam_BaseEntity = NULL;
		}
		else
		{
			DevWarning( "Invalid event field name \"%s\".\n", pParamName );
			return false;
		}
		break;

	case FIELD_EHANDLE:
		if ( pLessonElement->iParamVarIndex < LESSON_VARIABLE_TOTAL )
		{
			// The parameter is a scripted var
			const LessonVariableInfo *pInfo = GetLessonVariableInfo( pLessonElement->iParamVarIndex );

			switch ( pInfo->varType )
			{
			case FIELD_EHANDLE:
				eventParam_BaseEntity = ( LESSON_VARIABLE_GET_FROM_OFFSET( EHANDLE, pInfo->iOffset ) ).Get();
				if ( !eventParam_BaseEntity )
				{
					if ( pLessonElement->bOptionalParam )
					{
						// Not having an entity is fine
						return true;
					}

					if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
					{
						ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tEntity param \"%s\" returned NULL.\n", pParamName );
					}
					return false;
				}
				break;
			case FIELD_FLOAT:
			case FIELD_INTEGER:
			case FIELD_BOOLEAN:
			case FIELD_STRING:
			case FIELD_FUNCTION:
				DevWarning( "Can't use this variable type with this parameter type in lesson script.\n" );
				break;
			}
		}
		else if ( event && !(event->IsEmpty( pParamName ))  )
		{
			int iEntID = event->GetInt( pParamName );
			if ( iEntID >= NUM_ENT_ENTRIES )
			{
				AssertMsg( 0, "Invalid entity ID used in game event field!" );
				DevWarning( "Invalid entity ID used in game event (%s) for param (%s).", event->GetName(), pParamName );
				return false;
			}

			eventParam_BaseEntity = C_BaseEntity::Instance( iEntID );
			if ( !eventParam_BaseEntity )
			{
				if ( pLessonElement->bOptionalParam )
				{
					// Not having an entity is fine
					return true;
				}

				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tEntity param \"%s\" returned NULL.\n", pParamName );
				}
				return false;
			}
		}
		else if ( pLessonElement->bOptionalParam )
		{
			// We don't want to interpret this and not finding the param is still ok
			return true;
		}
		else if ( Q_stricmp( pParamName, "null" ) == 0 )
		{
			// They explicitly want a null pointer
			eventParam_BaseEntity = NULL;
		}
		else if ( Q_stricmp( pParamName, "world" ) == 0 )
		{
			// They explicitly want the world
			eventParam_BaseEntity = GetClientWorldEntity();
		}
		else
		{
			DevWarning( "Invalid event field name \"%s\".\n", pParamName );
			return false;
		}
		break;

	case FIELD_EMBEDDED:
		{
			// The parameter is a convar
			ConVarRef tempCVar( pParamName );
			if ( tempCVar.IsValid() )
			{
				eventParam_float = tempCVar.GetFloat();
				Q_strncpy( eventParam_string, tempCVar.GetString(), sizeof( eventParam_string ) );
			}
			else
			{
				DevWarning( "Invalid convar name \"%s\".\n", pParamName );
				return false;
			}
		}
		break;
	}

	// Do the action to the specified variable
	switch ( pLessonElement->iVariable )
	{
		// Run process action macros on all scriptable variables (see: LESSON_VARIABLE_FACTORY definition)
#define LESSON_VARIABLE_MACRO			PROCESS_LESSON_ACTION
#define LESSON_VARIABLE_MACRO_BOOL		PROCESS_LESSON_ACTION
#define LESSON_VARIABLE_MACRO_EHANDLE	PROCESS_LESSON_ACTION_EHANDLE
#define LESSON_VARIABLE_MACRO_STRING	PROCESS_LESSON_ACTION_STRING
		LESSON_VARIABLE_FACTORY;
#undef LESSON_VARIABLE_MACRO
#undef LESSON_VARIABLE_MACRO_BOOL
#undef LESSON_VARIABLE_MACRO_EHANDLE
#undef LESSON_VARIABLE_MACRO_STRING
	}

	return true;
}

bool CScriptedIconLesson::ProcessElementAction( int iAction, bool bNot, const char *pchVarName, float &fVar, const CGameInstructorSymbol *pchParamName, float fParam )
{
	switch ( iAction )
	{
		case LESSON_ACTION_SET:
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s] = [%s] ", pchVarName, pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f\n", fParam );
			}

			fVar = fParam;
			return true;

		case LESSON_ACTION_ADD:
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s] += [%s] ", pchVarName, pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f\n", fParam );
			}

			fVar += fParam;
			return true;

		case LESSON_ACTION_SUBTRACT:
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s] -= [%s] ", pchVarName, pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f\n", fParam );
			}

			fVar -= fParam;
			return true;

		case LESSON_ACTION_MULTIPLY:
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s] *= [%s] ", pchVarName, pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f\n", fParam );
			}

			fVar *= fParam;
			return true;

		case LESSON_ACTION_IS:
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s] ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f ", fVar );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "!= [%s] " ) : ( "== [%s] " ), pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f\n", fParam );
			}

			return ( bNot ) ? ( fVar != fParam ) : ( fVar == fParam );

		case LESSON_ACTION_LESS_THAN:
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s] ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f ", fVar );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ">= [%s] " ) : ( "< [%s] " ), pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f\n", fParam );
			}

			return ( bNot ) ? ( fVar >= fParam ) : ( fVar < fParam );

		case LESSON_ACTION_HAS_BIT:
		{
			int iTemp1 = static_cast<int>( fVar );
			int iTemp2 = ( 1 << static_cast<int>( fParam ) );

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t([%s] ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "0x%X ", iTemp1 );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "& [%s] ", pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "0x%X", iTemp2 );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ") == 0\n" ) : ( ") != 0\n" ) );
			}

			return ( bNot ) ? ( ( iTemp1 & iTemp2 ) == 0 ) : ( ( iTemp1 & iTemp2 ) != 0 );
		}

		case LESSON_ACTION_BIT_COUNT_IS:
		{
			int iTemp1 = UTIL_CountNumBitsSet( static_cast<unsigned int>( fVar ) );
			int iTemp2 = static_cast<int>( fParam );

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tUTIL_CountNumBitsSet([%s]) ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%i ", iTemp1 );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( " != [%s] " ) : ( " == [%s] " ), pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%i\n", iTemp2 );
			}

			return ( bNot ) ? ( iTemp1 != iTemp2 ) : ( iTemp1 == iTemp2 );
		}

		case LESSON_ACTION_BIT_COUNT_LESS_THAN:
		{
			int iTemp1 = UTIL_CountNumBitsSet( static_cast<unsigned int>( fVar ) );
			int iTemp2 = static_cast<int>( fParam );

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tUTIL_CountNumBitsSet([%s]) ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%i ", iTemp1 );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( " >= [%s] " ) : ( " < [%s] " ), pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%i\n", iTemp2 );
			}

			return ( bNot ) ? ( iTemp1 >= iTemp2 ) : ( iTemp1 < iTemp2 );
		}
	}

	DevWarning( "Invalid lesson action type used with \"%s\" variable type.\n", pchVarName );

	return false;
}

bool CScriptedIconLesson::ProcessElementAction( int iAction, bool bNot, const char *pchVarName, int &iVar, const CGameInstructorSymbol *pchParamName, float fParam )
{
	float fTemp = static_cast<float>( iVar );
	bool bRetVal = ProcessElementAction( iAction, bNot, pchVarName, fTemp, pchParamName, fParam );

	iVar = static_cast<int>( fTemp );
	return bRetVal;
}

bool CScriptedIconLesson::ProcessElementAction( int iAction, bool bNot, const char *pchVarName, bool &bVar, const CGameInstructorSymbol *pchParamName, float fParam )
{
	float fTemp = ( bVar ) ? ( 1.0f ) : ( 0.0f );
	bool bRetVal = ProcessElementAction( iAction, bNot, pchVarName, fTemp, pchParamName, fParam );

	bVar = ( fTemp != 0.0f );
	return bRetVal;
}

bool CScriptedIconLesson::ProcessElementAction( int iAction, bool bNot, const char *pchVarName, EHANDLE &hVar, const CGameInstructorSymbol *pchParamName, float fParam, C_BaseEntity *pParam, const char *pchParam )
{
	// First try to let the mod act on the action
	/*bool bModHandled = false;
	bool bModReturn = Mod_ProcessElementAction( iAction, bNot, pchVarName, hVar, pchParamName, fParam, pParam, pchParam, bModHandled );

	if ( bModHandled )
	{
		return bModReturn;
	}*/

	C_BaseEntity *pVar = hVar.Get();

	switch ( iAction )
	{
		case LESSON_ACTION_SET:
		{
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s] = [%s]\n", pchVarName, pchParamName->String() );
			}

			hVar = pParam;
			return true;
		}

		case LESSON_ACTION_IS:
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t[%s] != [%s]\n" ) : ( "\t[%s] == [%s]\n" ), pchVarName, pchParamName->String() );
			}

			return ( bNot ) ? ( pVar != pParam ) : ( pVar == pParam );

		case LESSON_ACTION_GET_DISTANCE:
		{
			if ( !pVar || !pParam )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[output] = [%s]->DistTo( [%s] )", pchVarName, pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "...\n" );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle or Param handle returned NULL!\n" );
				}

				return false;
			}
			
			C_BasePlayer *pVarPlayer = ( pVar->IsPlayer() ? static_cast< C_BasePlayer* >( pVar ) : NULL );
			C_BasePlayer *pParamPlayer = ( pParam->IsPlayer() ? static_cast< C_BasePlayer* >( pParam ) : NULL );

			Vector vVarPos = ( pVarPlayer ? pVarPlayer->EyePosition() : pVar->WorldSpaceCenter() );
			Vector vParamPos = ( pParamPlayer ? pParamPlayer->EyePosition() : pParam->WorldSpaceCenter() );

			m_fOutput = vVarPos.DistTo( vParamPos );

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[output] = [%s]->DistTo( [%s] ) ", pchVarName, pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f\n", m_fOutput );
			}

			return true;
		}

		case LESSON_ACTION_GET_ANGULAR_DISTANCE:
		{
			if ( !pVar || !pParam )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[output] = [%s]->AngularDistTo( [%s] )", pchVarName, pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "...\n" );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle or Param handle returned NULL!\n" );
				}

				return false;
			}

			C_BasePlayer *pVarPlayer = ( pVar->IsPlayer() ? static_cast< C_BasePlayer* >( pVar ) : NULL );
			C_BasePlayer *pParamPlayer = ( pParam->IsPlayer() ? static_cast< C_BasePlayer* >( pParam ) : NULL );

			Vector vVarPos = ( pVarPlayer ? pVarPlayer->EyePosition() : pVar->WorldSpaceCenter() );
			Vector vParamPos = ( pParamPlayer ? pParamPlayer->EyePosition() : pParam->WorldSpaceCenter() );

			Vector vVarToParam = vParamPos - vVarPos;
			VectorNormalize( vVarToParam );

			Vector vVarForward;

			if ( pVar->IsPlayer() )
			{
				AngleVectors( static_cast< C_BasePlayer* >( pVar )->EyeAngles(), &vVarForward, NULL, NULL );
			}
			else
			{
				pVar->GetVectors( &vVarForward, NULL, NULL );
			}

			// Set the distance in degrees
			m_fOutput = ( vVarToParam.Dot( vVarForward ) - 1.0f ) * -90.0f;

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[output] = [%s]->AngularDistTo( [%s] ) ", pchVarName, pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f\n", m_fOutput );
			}

			return true;
		}

		case LESSON_ACTION_GET_PLAYER_DISPLAY_NAME:
		{
			int iTemp = static_cast<int>( fParam );

			if ( iTemp <= 0 || iTemp > 2 )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tQ_strcpy( [stringINVALID], [%s]->GetPlayerName() ", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "... " );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")\n" );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tParam selecting string is out of range!\n" );
				}

				return false;
			}

			// Use string2 if it was specified, otherwise, use string1
			CGameInstructorSymbol *pString;
			char const *pchParamNameTemp = NULL;

			if ( iTemp == 2 )
			{
				pString = &m_szString2;
				pchParamNameTemp = "string2";
			}
			else
			{
				pString = &m_szString1;
				pchParamNameTemp = "string1";
			}

			if ( !pVar )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tQ_strcpy( [%s], [%s]->GetPlayerName() ", pchParamNameTemp, pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "... " );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")\n" );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle returned NULL!\n" );
				}

				return false;
			}

			*pString = pVar->GetPlayerName();

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tQ_strcpy( [%s], [%s]->GetPlayerName() ", pchParamNameTemp, pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\" ", pString->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")\n" );
			}

			return true;
		}

		case LESSON_ACTION_CLASSNAME_IS:
		{
			if ( !pVar )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t!FClassnameIs( [%s] " ) : ( "\tFClassnameIs( [%s] " ), pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "..." );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ", [%s] ", pchParamName->String()  );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\" ", pchParam );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")\n" );

					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle returned NULL!\n" );
				}

				return false;
			}

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t!FClassnameIs( [%s] " ) : ( "\tFClassnameIs( [%s] " ), pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%s", pVar->GetClassname() );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ", [%s] ", pchParamName->String()  );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\" ", pchParam );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")\n" );
			}

			return ( bNot ) ? ( !FClassnameIs( pVar, pchParam ) ) : ( FClassnameIs( pVar, pchParam ) );
		}

		case LESSON_ACTION_TEAM_IS:
		{
			int iTemp = static_cast<int>( fParam );

			if ( !pVar )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->GetTeamNumber() ", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "... " );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "!= [%s] " ) : ( "== [%s] " ), pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%i\n", iTemp );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle returned NULL!\n" );
				}

				return false;
			}

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->GetTeamNumber() ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%i ", pVar->GetTeamNumber() );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "!= [%s] " ) : ( "== [%s] " ), pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%i\n", iTemp );
			}

			return ( bNot ) ? ( pVar->GetTeamNumber() != iTemp ) : ( pVar->GetTeamNumber() == iTemp );
		}

		case LESSON_ACTION_MODELNAME_IS:
		{
			C_BaseAnimating *pBaseAnimating = dynamic_cast<C_BaseAnimating *>( pVar );

			if ( !pBaseAnimating )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tQ_stricmp( [%s]->ModelName() ", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "..." );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ", [%s] ", pchParamName->String()  );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\" ", pchParam );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ") != 0\n" ) : ( ") == 0\n" ) );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as BaseAnimating returned NULL!\n" );
				}

				return false;
			}

			const char *pchModelName = "-no model-";
			CStudioHdr *pModel = pBaseAnimating->GetModelPtr();
			if ( pModel )
			{
				const studiohdr_t *pRenderHDR = pModel->GetRenderHdr();
				if ( pRenderHDR )
				{
					pchModelName = pRenderHDR->name;
				}
			}

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tQ_stricmp( [%s]->ModelName() ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%s", pchModelName );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ", [%s] ", pchParamName->String()  );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\" ", pchParam );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ") != 0\n" ) : ( ") == 0\n" ) );
			}

			return ( bNot ) ? ( Q_stricmp( pchModelName, pchParam ) != 0 ) : ( Q_stricmp( pchModelName, pchParam ) == 0 );
		}
		
		case LESSON_ACTION_HEALTH_LESS_THAN:
		{
			int iTemp = static_cast<int>( fParam );

			if ( !pVar )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->GetHealth() ", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "... " );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ">= [%s] " ) : ( "< [%s] " ), pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%i\n", iTemp );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle returned NULL!\n" );
				}

				return false;
			}

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->GetHealth() ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%i ", pVar->GetHealth() );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ">= [%s] " ) : ( "< [%s] " ), pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%i\n", iTemp );
			}

			return ( bNot ) ? ( pVar->GetHealth() >= iTemp ) : ( pVar->GetHealth() < iTemp );
		}

		case LESSON_ACTION_HEALTH_PERCENTAGE_LESS_THAN:
		{
			if ( !pVar )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->HealthFraction() ", pchVarName, pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "... " );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ">= [%s] " ) : ( "< [%s] " ), pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f\n", fParam );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle returned NULL!\n" );
				}

				return false;
			}

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->HealthFraction() ", pchVarName, pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f ", pVar->HealthFraction() );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ">= [%s] " ) : ( "< [%s] " ), pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f\n", fParam );
			}

			float fHealthPercentage = 1.0f;
			
			if ( pVar->GetMaxHealth() != 0.0f )
			{
				fHealthPercentage = pVar->HealthFraction();
			}

			return ( bNot ) ? ( fHealthPercentage >= fParam ) : ( fHealthPercentage < fParam );
		}

		case LESSON_ACTION_GET_ACTIVE_WEAPON:
		{
			int iTemp = static_cast<int>( fParam );

			if ( iTemp <= 0 || iTemp > 2 )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[entityINVALID] = [%s]->GetActiveWeapon()\n", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tParam selecting string is out of range!\n" );
				}

				return false;
			}

			// Use entity2 if it was specified, otherwise, use entity1
			CHandle<C_BaseEntity> *pHandle;

			char const *pchParamNameTemp = NULL;

			if ( iTemp == 2 )
			{
				pHandle = &m_hEntity2;
				pchParamNameTemp = "entity2";
			}
			else
			{
				pHandle = &m_hEntity1;
				pchParamNameTemp = "entity1";
			}

			C_BaseCombatCharacter *pBaseCombatCharacter = NULL;

			if ( pVar )
			{
				pBaseCombatCharacter = pVar->MyCombatCharacterPointer();
			}

			if ( !pBaseCombatCharacter )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s] = [%s]->GetActiveWeapon()", pchParamNameTemp, pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as BaseCombatCharacter returned NULL!\n" );
				}

				return false;
			}

			pHandle->Set( pBaseCombatCharacter->GetActiveWeapon() );

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s] = [%s]->GetActiveWeapon()", pchParamNameTemp, pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"\n", pchParam );
			}

			return true;
		}

		case LESSON_ACTION_WEAPON_IS:
		{
			C_BaseCombatCharacter *pBaseCombatCharacter = NULL;

			if ( pVar )
			{
				pBaseCombatCharacter = pVar->MyCombatCharacterPointer();
			}

			if ( !pBaseCombatCharacter )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->GetActiveWeapon()->GetName() ", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "... " );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "!= [%s] " ) : ( "== [%s] " ), pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"\n", pchParam );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as BaseCombatCharacter returned NULL!\n" );
				}

				return false;
			}

			CBaseCombatWeapon *pBaseCombatWeapon = pBaseCombatCharacter->GetActiveWeapon();

			if ( !pBaseCombatWeapon )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->GetActiveWeapon()->GetName() ", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "... " );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "!= [%s] " ) : ( "== [%s] " ), pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"\n", pchParam );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar GetActiveWeapon returned NULL!\n" );
				}

				return false;
			}

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->GetActiveWeapon()->GetName() ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\" ", pBaseCombatWeapon->GetName() );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "!= [%s] " ) : ( "== [%s] " ), pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"\n", pchParam );
			}

			return ( bNot ) ? ( Q_stricmp( pBaseCombatWeapon->GetName(), pchParam ) != 0 ) : ( Q_stricmp( pBaseCombatWeapon->GetName(), pchParam ) == 0 );
		}

		case LESSON_ACTION_WEAPON_HAS:
		{
			C_BaseCombatCharacter *pBaseCombatCharacter = NULL;

			if ( pVar )
			{
				pBaseCombatCharacter = pVar->MyCombatCharacterPointer();
			}

			if ( !pBaseCombatCharacter )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t![%s]->Weapon_OwnsThisType([%s] " ) : ( "\t[%s]->Weapon_OwnsThisType([%s] " ), pchVarName, pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"", pchParam );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")\n" );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as BaseCombatCharacter returned NULL!\n" );
				}

				return false;
			}

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t![%s]->Weapon_OwnsThisType([%s] " ) : ( "\t[%s]->Weapon_OwnsThisType([%s] " ), pchVarName, pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"", pchParam );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")\n" );
			}

			return ( bNot ) ? ( pBaseCombatCharacter->Weapon_OwnsThisType( pchParam ) == NULL ) : ( pBaseCombatCharacter->Weapon_OwnsThisType( pchParam ) != NULL );
		}

		case LESSON_ACTION_GET_ACTIVE_WEAPON_SLOT:
		{
			C_BaseCombatCharacter *pBaseCombatCharacter = NULL;

			if ( pVar )
			{
				pBaseCombatCharacter = pVar->MyCombatCharacterPointer();
			}

			if ( !pBaseCombatCharacter )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[output] = [%s]->Weapon_GetActiveSlot() ...\n", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as BaseCombatCharacter returned NULL!\n" );
				}

				return false;
			}

			C_BaseCombatWeapon *pWeapon = pBaseCombatCharacter->GetActiveWeapon();

			if ( !pWeapon )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[output] = [%s]->Weapon_GetActiveSlot() ...\n", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar GetActiveWeapon returned NULL!\n" );
				}

				return false;
			}

			// @TODO
			/*m_fOutput = pBaseCombatCharacter->Weapon_GetSlot( pWeapon->GetWpnData().szClassName );

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[output] = [%s]->Weapon_GetSlot() ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f\n", m_fOutput );
			}*/

			return true;
		}

		case LESSON_ACTION_GET_WEAPON_SLOT:
		{
			C_BaseCombatCharacter *pBaseCombatCharacter = NULL;

			if ( pVar )
			{
				pBaseCombatCharacter = pVar->MyCombatCharacterPointer();
			}

			if ( !pBaseCombatCharacter )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[output] = [%s]->Weapon_GetSlot([%s] ", pchVarName, pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"", pchParam );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ") ...\n" );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as BaseCombatCharacter returned NULL!\n" );
				}

				return false;
			}

			// @TODO
			/*m_fOutput = pBaseCombatCharacter->Weapon_GetSlot( pchParam );

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[output] = [%s]->Weapon_GetSlot([%s] ", pchVarName, pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"", pchParam );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ") " );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%f\n", m_fOutput );
			}*/

			return true;
		}

		case LESSON_ACTION_GET_WEAPON_IN_SLOT:
		{
			int nTemp = static_cast<int>( fParam );

			C_BaseCombatCharacter *pBaseCombatCharacter = NULL;

			if ( pVar )
			{
				pBaseCombatCharacter = pVar->MyCombatCharacterPointer();
			}

			if ( !pBaseCombatCharacter )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[entity1] = [%s]->GetWeapon([%s] ", pchVarName, pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%i\"", nTemp );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")\n" );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as BaseCombatCharacter returned NULL!\n" );
				}

				return false;
			}

			m_hEntity1 = pBaseCombatCharacter->GetWeapon( nTemp );

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[entity1] = [%s]->GetWeapon([%s] ", pchVarName, pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%i\"", nTemp );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")\n" );
			}

			return true;
		}

		case LESSON_ACTION_CLIP_PERCENTAGE_LESS_THAN:
		{
			C_BaseCombatCharacter *pBaseCombatCharacter = NULL;

			if ( pVar )
			{
				pBaseCombatCharacter = pVar->MyCombatCharacterPointer();
			}

			if ( !pBaseCombatCharacter )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->GetActiveWeapon()->Clip1Percentage() ", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "... " );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ">= [%s] " ) : ( "< [%s] " ), pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%.1f\n", fParam );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as BaseCombatCharacter returned NULL!\n" );
				}

				return false;
			}

			CBaseCombatWeapon *pBaseCombatWeapon = pBaseCombatCharacter->GetActiveWeapon();

			if ( !pBaseCombatWeapon )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->GetActiveWeapon()->Clip1Percentage() ", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "... " );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ">= [%s] " ) : ( "< [%s] " ), pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%.1f\n", fParam );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar GetActiveWeapon returned NULL!\n" );
				}

				return false;
			}

			float fClip1Percentage = 100.0f;

			if ( pBaseCombatWeapon->UsesClipsForAmmo1() )
			{
				fClip1Percentage = 100.0f * ( static_cast<float>( pBaseCombatWeapon->Clip1() ) / static_cast<float>( pBaseCombatWeapon->GetMaxClip1() ) );
			}

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->GetActiveWeapon()->Clip1Percentage() ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%.1f ", fClip1Percentage );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ">= [%s] " ) : ( "< [%s] " ), pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%.1f\n", fParam );
			}

			return ( bNot ) ? ( fClip1Percentage >= fParam ) : ( fClip1Percentage < fParam );
		}

		case LESSON_ACTION_WEAPON_AMMO_LOW:
		{
			int iTemp = static_cast<int>( fParam );

			C_BasePlayer *pBasePlayer = ToBasePlayer( pVar );

			if ( !pBasePlayer )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->GetWeaponInSlot( ", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%i ", iTemp );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ")->AmmoPercentage() >= 30\n" ) : ( ")->AmmoPercentage() < 30\n" ) );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as BasePlayer returned NULL!\n" );
				}

				return false;
			}

			CBaseCombatWeapon *pBaseCombatWeapon = NULL;

			// Get the weapon in variable slot
			for ( int iWeapon = 0; iWeapon < MAX_WEAPONS; iWeapon++ )
			{
				CBaseCombatWeapon *pBaseCombatWeaponTemp = pBasePlayer->GetWeapon( iWeapon );
				if ( pBaseCombatWeaponTemp )
				{
					if ( pBaseCombatWeaponTemp->GetSlot() == iTemp )
					{
						pBaseCombatWeapon = pBaseCombatWeaponTemp;
						break;
					}
				}
			}

			if ( !pBaseCombatWeapon )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->GetWeaponInSlot( ", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%i ", iTemp );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ")->AmmoPercentage() >= 30\n" ) : ( ")->AmmoPercentage() < 30\n" ) );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar GetActiveWeapon returned NULL!\n" );
				}

				return false;
			}

			// Check if the ammo is full
			int iAmmoType = pBaseCombatWeapon->GetPrimaryAmmoType();
			int iMaxAmmo = GetAmmoDef()->MaxCarry( iAmmoType/*, pBasePlayer*/ );
			int iPlayerAmmo = pBasePlayer->GetAmmoCount( iAmmoType );

			bool bAmmoLow = ( iPlayerAmmo < ( iMaxAmmo / 3 ) );

			if ( bNot )
			{
				bAmmoLow = !bAmmoLow;
			}

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s]->GetWeaponInSlot( ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "%i ", iTemp );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ")->AmmoPercentage() >= 30 " ) : ( ")->AmmoPercentage() < 30 " ) );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bAmmoLow ) ? ( "true\n" ) : ( "false\n" ) );
			}

			return bAmmoLow;
		}

		case LESSON_ACTION_WEAPON_AMMO_FULL:
		{
			int iTemp = static_cast<int>( fParam );

			C_BasePlayer *pBasePlayer = ToBasePlayer( pVar );

			if ( !pBasePlayer )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t![%s]->GetWeaponInSlot( " ) : ( "\t[%s]->GetWeaponInSlot( " ), pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseSuccess, "%i ", iTemp );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")->AmmoFull()\n" );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as BasePlayer returned NULL!\n" );
				}

				return false;
			}

			CBaseCombatWeapon *pBaseCombatWeapon = NULL;

			// Get the weapon in variable slot
			for ( int iWeapon = 0; iWeapon < MAX_WEAPONS; iWeapon++ )
			{
				CBaseCombatWeapon *pBaseCombatWeaponTemp = pBasePlayer->GetWeapon( iWeapon );
				if ( pBaseCombatWeaponTemp )
				{
					if ( pBaseCombatWeaponTemp->GetSlot() == iTemp )
					{
						pBaseCombatWeapon = pBaseCombatWeaponTemp;
						break;
					}
				}
			}

			if ( !pBaseCombatWeapon )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t![%s]->GetWeaponInSlot( " ) : ( "\t[%s]->GetWeaponInSlot( " ), pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseSuccess, "%i ", iTemp );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")->AmmoFull()\n" );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar GetWeaponInSlot returned NULL!\n" );
				}

				return false;
			}

			// Check if the ammo is full
			int iAmmoType = pBaseCombatWeapon->GetPrimaryAmmoType();
			int iMaxAmmo = GetAmmoDef()->MaxCarry( iAmmoType/*, pBasePlayer*/ );
			int iPlayerAmmo = pBasePlayer->GetAmmoCount( iAmmoType );

			bool bAmmoFull = ( iPlayerAmmo >= iMaxAmmo );

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t![%s]->GetWeaponInSlot( " ) : ( "\t[%s]->GetWeaponInSlot( " ), pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseSuccess, "%i ", iTemp );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")->AmmoFull() " );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, ( bAmmoFull ) ? ( "true\n" ) : ( "false\n" ) );
			}

			return ( bNot ) ? ( !bAmmoFull ) : ( bAmmoFull );
		}

		case LESSON_ACTION_WEAPON_AMMO_EMPTY:
		{
			int iTemp = static_cast<int>( fParam );

			C_BasePlayer *pBasePlayer = ToBasePlayer( pVar );

			if ( !pBasePlayer )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t![%s]->GetWeaponInSlot( " ) : ( "\t[%s]->GetWeaponInSlot( " ), pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseSuccess, "%i ", iTemp );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")->AmmoEmpty()\n" );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as BasePlayer returned NULL!\n" );
				}

				return false;
			}

			CBaseCombatWeapon *pBaseCombatWeapon = NULL;

			// Get the weapon in variable slot
			for ( int iWeapon = 0; iWeapon < MAX_WEAPONS; iWeapon++ )
			{
				CBaseCombatWeapon *pBaseCombatWeaponTemp = pBasePlayer->GetWeapon( iWeapon );
				if ( pBaseCombatWeaponTemp )
				{
					if ( pBaseCombatWeaponTemp->GetSlot() == iTemp )
					{
						pBaseCombatWeapon = pBaseCombatWeaponTemp;
						break;
					}
				}
			}

			if ( !pBaseCombatWeapon )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t![%s]->GetWeaponInSlot( " ) : ( "\t[%s]->GetWeaponInSlot( " ), pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseSuccess, "%i ", iTemp );
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")->AmmoEmpty()\n" );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar GetWeaponInSlot returned NULL!\n" );
				}

				return false;
			}

			// Check if the ammo is empty
			int iAmmoType = pBaseCombatWeapon->GetPrimaryAmmoType();
			int iPlayerAmmo = pBasePlayer->GetAmmoCount( iAmmoType );

			bool bAmmoEmpty = ( iPlayerAmmo <= 0 );

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t![%s]->GetWeaponInSlot( " ) : ( "\t[%s]->GetWeaponInSlot( " ), pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseSuccess, "%i ", iTemp );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")->AmmoEmpty() " );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, ( bAmmoEmpty ) ? ( "true" ) : ( "false" ) );
				ConColorMsg(CBaseLesson::m_rgbaVerbosePlain, " )\n" );
			}

			return ( bNot ) ? ( !bAmmoEmpty ) : ( bAmmoEmpty );
		}

		/*case LESSON_ACTION_WEAPON_CAN_USE:
		{
			C_BaseCombatWeapon *pBaseCombatWeapon = dynamic_cast<C_BaseCombatWeapon*>( pParam );
			C_BasePlayer *pBasePlayer = ToBasePlayer( pVar );

			if ( !pBasePlayer )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t![%s]->Weapon_CanUse([%s])\n" ) : ( "\t[%s]->Weapon_CanUse([%s])\n" ), pchVarName, pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as BasePlayer returned NULL!\n" );
				}

				return false;
			}

			if ( !pBaseCombatWeapon )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t![%s]->Weapon_CanUse([%s])\n" ) : ( "\t[%s]->Weapon_CanUse([%s])\n" ), pchVarName, pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tParam BaseCombatWeapon returned NULL!\n" );
				}

				return false;
			}

			bool bCanEquip = pBasePlayer->Weapon_CanUse( pBaseCombatWeapon );

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\t![%s]->Weapon_CanUse([%s]) " ) : ( "\t[%s]->Weapon_CanUse([%s]) " ), pchVarName, pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, ( bCanEquip ) ? ( "true\n" ) : ( "false\n" ) );
			}

			return ( bNot ) ? ( !bCanEquip ) : ( bCanEquip );
		}*/

		case LESSON_ACTION_USE_TARGET_IS:
		{
			C_BasePlayer *pBasePlayer = ToBasePlayer( pVar );

			if ( !pBasePlayer )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\tC_BaseEntity::Instance([%s]->GetUseEntity()) != [%s]\n" ) : ( "\tC_BaseEntity::Instance([%s]->GetUseEntity()) == [%s]\n" ), pchVarName, pchParamName->String() );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as Player returned NULL!\n" );
				}

				return false;
			}

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( "\tC_BaseEntity::Instance([%s]->GetUseEntity()) != [%s]\n" ) : ( "\tC_BaseEntity::Instance([%s]->GetUseEntity()) == [%s]\n" ), pchVarName, pchParamName->String() );
			}

			return ( bNot ) ? ( C_BaseEntity::Instance( pBasePlayer->GetUseEntity() ) != pParam ) : ( C_BaseEntity::Instance( pBasePlayer->GetUseEntity() ) == pParam );
		}

		case LESSON_ACTION_GET_USE_TARGET:
		{
			int iTemp = static_cast<int>( fParam );

			if ( iTemp <= 0 || iTemp > 2 )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[entityINVALID] = C_BaseEntity::Instance([%s]->GetUseEntity())\n", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tParam selecting string is out of range!\n" );
				}

				return false;
			}

			// Use entity2 if it was specified, otherwise, use entity1
			CHandle<C_BaseEntity> *pHandle;
			char const *pchParamNameTemp = NULL;

			if ( iTemp == 2 )
			{
				pHandle = &m_hEntity2;
				pchParamNameTemp = "entity2";
			}
			else
			{
				pHandle = &m_hEntity1;
				pchParamNameTemp = "entity1";
			}

			C_BasePlayer *pBasePlayer = ToBasePlayer( pVar );

			if ( !pBasePlayer )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s] = C_BaseEntity::Instance([%s]->GetUseEntity())\n", pchParamNameTemp, pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as Player returned NULL!\n" );
				}

				return false;
			}

			pHandle->Set( C_BaseEntity::Instance( pBasePlayer->GetUseEntity() ) );

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s] = C_BaseEntity::Instance([%s]->GetUseEntity())\n", pchParamNameTemp, pchVarName );
			}

			return true;
		}

		/*case LESSON_ACTION_GET_POTENTIAL_USE_TARGET:
		{
			int iTemp = static_cast<int>( fParam );

			if ( iTemp <= 0 || iTemp > 2 )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[entityINVALID] = C_BaseEntity::Instance([%s]->GetPotentialUseEntity())\n", pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tParam selecting string is out of range!\n" );
				}

				return false;
			}

			// Use entity2 if it was specified, otherwise, use entity1
			CHandle<C_BaseEntity> *pHandle;
			char const *pchParamNameTemp = NULL;

			if ( iTemp == 2 )
			{
				pHandle = &m_hEntity2;
				pchParamNameTemp = "entity2";
			}
			else
			{
				pHandle = &m_hEntity1;
				pchParamNameTemp = "entity1";
			}

			C_BasePlayer *pBasePlayer = ToBasePlayer( pVar );

			if ( !pBasePlayer )
			{
				if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
				{
					ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s] = C_BaseEntity::Instance([%s]->GetPotentialUseEntity())\n", pchParamNameTemp, pchVarName );
					ConColorMsg( CBaseLesson::m_rgbaVerboseClose, "\tVar handle as Player returned NULL!\n" );
				}

				return false;
			}

			pHandle->Set( C_BaseEntity::Instance( pBasePlayer->GetPotentialUseEntity() ) );

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\t[%s] = C_BaseEntity::Instance([%s]->GetPotentialUseEntity())\n", pchParamNameTemp, pchVarName );
			}

			return true;
		}*/
	}

	DevWarning( "Invalid lesson action type used with \"%s\" variable type.\n", pchVarName );

	return false;
}

bool CScriptedIconLesson::ProcessElementAction( int iAction, bool bNot, const char *pchVarName, CGameInstructorSymbol *pchVar, const CGameInstructorSymbol *pchParamName, const char *pchParam )
{
	switch ( iAction )
	{
		case LESSON_ACTION_REFERENCE_OPEN:
		{
			const CBaseLesson *pLesson = GetGameInstructor().GetLesson( pchParamName->String() );
			if ( !pLesson )
			{
				DevWarning( "Invalid lesson specified: \"%s\".", pchParamName->String() );
				return false;
			}

			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( Color( 64, 128, 255, 255 ), ( bNot ) ? ( "\t!( [\"%s\"]->IsInstanceActive() " ) : ( "\t( [\"%s\"]->IsInstanceActive() " ), pchParamName->String() );
				ConColorMsg( Color( 255, 255, 255, 255 ), "\"%s\"", (pLesson->IsInstanceActive() ? "true" : "false") );
				ConColorMsg( Color( 64, 128, 255, 255 ), " )\n" );
			}

			return ( bNot ) ? ( !pLesson->IsInstanceActive() ) : ( pLesson->IsInstanceActive() );
		}

		case LESSON_ACTION_SET:
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tQ_strcpy([%s], [%s] ", pchVarName, pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"", pchParam );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")\n" );
			}

			*pchVar = pchParam;
			return true;

		case LESSON_ACTION_ADD:
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tQ_strcat([%s], [%s] ", pchVarName, pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"", pchParam );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ")\n" );
			}

			char szTemp[ 256 ];
			Q_strncpy( szTemp, pchVar->String(), sizeof( szTemp ) );
			Q_strncat( szTemp, pchParam, sizeof( szTemp ) );

			*pchVar = szTemp;
			return true;

		case LESSON_ACTION_IS:
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tQ_strcmp([%s] ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"", pchVar->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ", [%s] ", pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"", pchParam );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ") != 0\n" ) : ( ") == 0\n" ) );
			}

			return ( bNot ) ? ( Q_strcmp( pchVar->String(), pchParam ) != 0 ) : ( Q_strcmp( pchVar->String(), pchParam ) == 0 );

		case LESSON_ACTION_HAS_PREFIX:
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tStringHasPrefix([%s] ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"", pchVar->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ", [%s] ", pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerboseName, "\"%s\"", pchParam );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ") == false\n" ) : ( ") == true\n" ) );
			}

			return ( bNot ) ? ( !StringHasPrefix( pchVar->String(), pchParam ) ) : ( StringHasPrefix( pchVar->String(), pchParam ) );

		case LESSON_ACTION_LESS_THAN:
			if ( gameinstructor_verbose.GetInt() > 0 && ShouldShowSpew() )
			{
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\tQ_strcmp([%s] ", pchVarName );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\"%s\"", pchVar->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ", [%s] ", pchParamName->String() );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, "\"%s\"", pchParam );
				ConColorMsg( CBaseLesson::m_rgbaVerbosePlain, ( bNot ) ? ( ") >= 0\n" ) : ( ") < 0\n" ) );
			}

			return ( bNot ) ? ( Q_strcmp( pchVar->String(), pchParam ) >= 0 ) : ( Q_strcmp( pchVar->String(), pchParam ) < 0 );
	}

	DevWarning( "Invalid lesson action type used with \"%s\" variable type.\n", pchVarName );

	return false;
}

LessonEvent_t * CScriptedIconLesson::AddOpenEvent()
{
	int iNewLessonEvent = m_OpenEvents.AddToTail();
	return &(m_OpenEvents[ iNewLessonEvent ]);
}

LessonEvent_t * CScriptedIconLesson::AddCloseEvent()
{
	int iNewLessonEvent = m_CloseEvents.AddToTail();
	return &(m_CloseEvents[ iNewLessonEvent ]);
}

LessonEvent_t * CScriptedIconLesson::AddSuccessEvent()
{
	int iNewLessonEvent = m_SuccessEvents.AddToTail();
	return &(m_SuccessEvents[ iNewLessonEvent ]);
}

LessonEvent_t * CScriptedIconLesson::AddOnOpenEvent()
{
	int iNewLessonEvent = m_OnOpenEvents.AddToTail();
	return &(m_OnOpenEvents[ iNewLessonEvent ]);
}

LessonEvent_t * CScriptedIconLesson::AddUpdateEvent()
{
	int iNewLessonEvent = m_UpdateEvents.AddToTail();
	return &(m_UpdateEvents[ iNewLessonEvent ]);
}

// Static method to init the keyvalues symbols used for comparisons
void CScriptedIconLesson::PreReadLessonsFromFile()
{
	static bool bFirstTime = true;
	if ( !bFirstTime )
		return;
	bFirstTime = false;

	// Run init info call macros on all scriptable variables (see: LESSON_VARIABLE_FACTORY definition)
#define LESSON_VARIABLE_MACRO			LESSON_VARIABLE_INIT_SYMBOL
#define LESSON_VARIABLE_MACRO_BOOL		LESSON_VARIABLE_INIT_SYMBOL
#define LESSON_VARIABLE_MACRO_EHANDLE	LESSON_VARIABLE_INIT_SYMBOL
#define LESSON_VARIABLE_MACRO_STRING	LESSON_VARIABLE_INIT_SYMBOL
	LESSON_VARIABLE_FACTORY
#undef LESSON_VARIABLE_MACRO
#undef LESSON_VARIABLE_MACRO_BOOL
#undef LESSON_VARIABLE_MACRO_EHANDLE
#undef LESSON_VARIABLE_MACRO_STRING

	// And build the map of variable name to enum
	// Run string to int macros on all scriptable variables (see: LESSON_VARIABLE_FACTORY definition)
#define LESSON_VARIABLE_MACRO			LESSON_SCRIPT_STRING_ADD_TO_MAP
#define LESSON_VARIABLE_MACRO_BOOL		LESSON_SCRIPT_STRING_ADD_TO_MAP
#define LESSON_VARIABLE_MACRO_EHANDLE	LESSON_SCRIPT_STRING_ADD_TO_MAP
#define LESSON_VARIABLE_MACRO_STRING	LESSON_SCRIPT_STRING_ADD_TO_MAP
	LESSON_VARIABLE_FACTORY
#undef LESSON_VARIABLE_MACRO
#undef LESSON_VARIABLE_MACRO_BOOL
#undef LESSON_VARIABLE_MACRO_EHANDLE
#undef LESSON_VARIABLE_MACRO_STRING

	// Set up mapping of field types
	g_TypeToParamTypeMap.Insert( "float", FIELD_FLOAT );
	g_TypeToParamTypeMap.Insert( "string", FIELD_STRING );
	g_TypeToParamTypeMap.Insert( "int", FIELD_INTEGER );
	g_TypeToParamTypeMap.Insert( "integer", FIELD_INTEGER );
	g_TypeToParamTypeMap.Insert( "short", FIELD_INTEGER );
	g_TypeToParamTypeMap.Insert( "long", FIELD_INTEGER );
	g_TypeToParamTypeMap.Insert( "bool", FIELD_BOOLEAN );
	g_TypeToParamTypeMap.Insert( "player", FIELD_CUSTOM );
	g_TypeToParamTypeMap.Insert( "entity", FIELD_EHANDLE );
	g_TypeToParamTypeMap.Insert( "convar", FIELD_EMBEDDED );
	g_TypeToParamTypeMap.Insert( "void", FIELD_VOID );

	// Set up the lesson action map
	
	CScriptedIconLesson::LessonActionMap.Insert( "scope in", LESSON_ACTION_SCOPE_IN );
	CScriptedIconLesson::LessonActionMap.Insert( "scope out", LESSON_ACTION_SCOPE_OUT );
	CScriptedIconLesson::LessonActionMap.Insert( "close", LESSON_ACTION_CLOSE );
	CScriptedIconLesson::LessonActionMap.Insert( "success", LESSON_ACTION_SUCCESS );
	CScriptedIconLesson::LessonActionMap.Insert( "lock", LESSON_ACTION_LOCK );
	CScriptedIconLesson::LessonActionMap.Insert( "present complete", LESSON_ACTION_PRESENT_COMPLETE );
	CScriptedIconLesson::LessonActionMap.Insert( "present start", LESSON_ACTION_PRESENT_START );
	CScriptedIconLesson::LessonActionMap.Insert( "present end", LESSON_ACTION_PRESENT_END );

	CScriptedIconLesson::LessonActionMap.Insert( "reference open", LESSON_ACTION_REFERENCE_OPEN );

	CScriptedIconLesson::LessonActionMap.Insert( "set", LESSON_ACTION_SET );
	CScriptedIconLesson::LessonActionMap.Insert( "add", LESSON_ACTION_ADD );
	CScriptedIconLesson::LessonActionMap.Insert( "subtract", LESSON_ACTION_SUBTRACT );
	CScriptedIconLesson::LessonActionMap.Insert( "multiply", LESSON_ACTION_MULTIPLY );
	CScriptedIconLesson::LessonActionMap.Insert( "is", LESSON_ACTION_IS );
	CScriptedIconLesson::LessonActionMap.Insert( "less than", LESSON_ACTION_LESS_THAN );
	CScriptedIconLesson::LessonActionMap.Insert( "has prefix", LESSON_ACTION_HAS_PREFIX );
	CScriptedIconLesson::LessonActionMap.Insert( "has bit", LESSON_ACTION_HAS_BIT );
	CScriptedIconLesson::LessonActionMap.Insert( "bit count is", LESSON_ACTION_BIT_COUNT_IS );
	CScriptedIconLesson::LessonActionMap.Insert( "bit count less than", LESSON_ACTION_BIT_COUNT_LESS_THAN );

	CScriptedIconLesson::LessonActionMap.Insert( "get distance", LESSON_ACTION_GET_DISTANCE );
	CScriptedIconLesson::LessonActionMap.Insert( "get angular distance", LESSON_ACTION_GET_ANGULAR_DISTANCE );
	CScriptedIconLesson::LessonActionMap.Insert( "get player display name", LESSON_ACTION_GET_PLAYER_DISPLAY_NAME );
	CScriptedIconLesson::LessonActionMap.Insert( "classname is", LESSON_ACTION_CLASSNAME_IS );
	CScriptedIconLesson::LessonActionMap.Insert( "modelname is", LESSON_ACTION_MODELNAME_IS );
	CScriptedIconLesson::LessonActionMap.Insert( "team is", LESSON_ACTION_TEAM_IS );
	CScriptedIconLesson::LessonActionMap.Insert( "health less than", LESSON_ACTION_HEALTH_LESS_THAN );
	CScriptedIconLesson::LessonActionMap.Insert( "health percentage less than", LESSON_ACTION_HEALTH_PERCENTAGE_LESS_THAN );
	CScriptedIconLesson::LessonActionMap.Insert( "get active weapon", LESSON_ACTION_GET_ACTIVE_WEAPON );
	CScriptedIconLesson::LessonActionMap.Insert( "weapon is", LESSON_ACTION_WEAPON_IS );
	CScriptedIconLesson::LessonActionMap.Insert( "weapon has", LESSON_ACTION_WEAPON_HAS );
	CScriptedIconLesson::LessonActionMap.Insert( "get active weapon slot", LESSON_ACTION_GET_ACTIVE_WEAPON_SLOT );
	CScriptedIconLesson::LessonActionMap.Insert( "get weapon slot", LESSON_ACTION_GET_WEAPON_SLOT );
	CScriptedIconLesson::LessonActionMap.Insert( "get weapon in slot", LESSON_ACTION_GET_WEAPON_IN_SLOT );
	CScriptedIconLesson::LessonActionMap.Insert( "clip percentage less than", LESSON_ACTION_CLIP_PERCENTAGE_LESS_THAN);
	CScriptedIconLesson::LessonActionMap.Insert( "weapon ammo low", LESSON_ACTION_WEAPON_AMMO_LOW );
	CScriptedIconLesson::LessonActionMap.Insert( "weapon ammo full", LESSON_ACTION_WEAPON_AMMO_FULL );
	CScriptedIconLesson::LessonActionMap.Insert( "weapon ammo empty", LESSON_ACTION_WEAPON_AMMO_EMPTY );
	CScriptedIconLesson::LessonActionMap.Insert( "weapon can use", LESSON_ACTION_WEAPON_CAN_USE );
	CScriptedIconLesson::LessonActionMap.Insert( "use target is", LESSON_ACTION_USE_TARGET_IS );
	CScriptedIconLesson::LessonActionMap.Insert( "get use target", LESSON_ACTION_GET_USE_TARGET );
	CScriptedIconLesson::LessonActionMap.Insert( "get potential use target", LESSON_ACTION_GET_POTENTIAL_USE_TARGET );

	// Add mod actions to the map
	//Mod_PreReadLessonsFromFile();
}
