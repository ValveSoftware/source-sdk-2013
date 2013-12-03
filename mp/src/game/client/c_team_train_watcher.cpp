//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates train data for escort game type
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_team_train_watcher.h"
#include "igameevents.h"
#include "c_team_objectiveresource.h"

#ifdef TF_CLIENT_DLL
#include "tf_shareddefs.h"
#include "teamplayroundbased_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_TeamTrainWatcher, DT_TeamTrainWatcher, CTeamTrainWatcher)

	RecvPropFloat( RECVINFO( m_flTotalProgress ) ),
	RecvPropInt( RECVINFO( m_iTrainSpeedLevel ) ),
	RecvPropFloat( RECVINFO( m_flRecedeTime ) ),
	RecvPropInt( RECVINFO( m_nNumCappers ) ),
#ifdef GLOWS_ENABLE
	RecvPropEHandle( RECVINFO( m_hGlowEnt ) ),
#endif // GLOWS_ENABLE

END_RECV_TABLE()

CUtlVector< CHandle<C_TeamTrainWatcher> > g_hTrainWatchers;

C_TeamTrainWatcher::C_TeamTrainWatcher()
{
	// force updates when we get our baseline
	m_iTrainSpeedLevel = -2;
	m_flTotalProgress = -1;
	m_flRecedeTime = -1;

#ifdef GLOWS_ENABLE
	m_pGlowEffect = NULL;
	m_hGlowEnt = NULL;
	m_hOldGlowEnt = NULL;
#endif // GLOWS_ENABLE
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TeamTrainWatcher::~C_TeamTrainWatcher()
{
#ifdef GLOWS_ENABLE
	DestroyGlowEffect();
#endif // GLOWS_ENABLE
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TeamTrainWatcher::ClientThink()
{
#ifdef GLOWS_ENABLE
	if ( IsDormant() || ( m_hGlowEnt.Get() == NULL ) )
	{
		DestroyGlowEffect();
		m_hOldGlowEnt = NULL;
		m_hGlowEnt = NULL;
	}
#endif // GLOWS_ENABLE
}

#ifdef GLOWS_ENABLE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TeamTrainWatcher::UpdateGlowEffect( void )
{
	// destroy the existing effect
	if ( m_pGlowEffect )
	{
		DestroyGlowEffect();
	}

	// create a new effect if we have a cart
	if ( m_hGlowEnt )
	{
		float r, g, b;
		TeamplayRoundBasedRules()->GetTeamGlowColor( GetTeamNumber(), r, g, b );
		m_pGlowEffect = new CGlowObject( m_hGlowEnt, Vector( r, g, b ), 1.0, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TeamTrainWatcher::DestroyGlowEffect( void )
{
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
}
#endif // GLOWS_ENABLE

void C_TeamTrainWatcher::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldTrainSpeedLevel = m_iTrainSpeedLevel;
	m_flOldProgress = m_flTotalProgress;
	m_flOldRecedeTime = m_flRecedeTime;
	m_nOldNumCappers = m_nNumCappers;
#ifdef GLOWS_ENABLE
	m_hOldGlowEnt = m_hGlowEnt;
#endif // GLOWS_ENABLE
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TeamTrainWatcher::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	if ( m_iOldTrainSpeedLevel != m_iTrainSpeedLevel || m_nOldNumCappers != m_nNumCappers )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "escort_speed" );
		if ( event )
		{
			event->SetInt( "team", GetTeamNumber() );
			event->SetInt( "speed", m_iTrainSpeedLevel );
			event->SetInt( "players", m_nNumCappers );
			gameeventmanager->FireEventClientSide( event );
		}
	}

	if ( m_flOldProgress != m_flTotalProgress )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "escort_progress" );
		if ( event )
		{
			event->SetInt( "team", GetTeamNumber() );
			event->SetFloat( "progress", m_flTotalProgress );

			if ( m_flOldProgress <= -1 )
			{
				event->SetBool( "reset", true );
			}

			gameeventmanager->FireEventClientSide( event );
		}

		// check to see if the train is now on a hill
		if ( ObjectiveResource() )
		{
			int nNumHills = ObjectiveResource()->GetNumNodeHillData( GetTeamNumber() );
			if ( nNumHills > 0 )
			{
				float flStart, flEnd;
				for ( int i = 0 ; i < nNumHills ; i++ )
				{
					ObjectiveResource()->GetHillData( GetTeamNumber(), i, flStart, flEnd );
					if ( m_flTotalProgress >= flStart && m_flTotalProgress<= flEnd )
					{
						ObjectiveResource()->SetTrainOnHill( GetTeamNumber(), i, true );
					}
					else
					{
						ObjectiveResource()->SetTrainOnHill( GetTeamNumber(), i, false );
					}
				}
			}
		}
	}

	if ( m_flOldRecedeTime != m_flRecedeTime )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "escort_recede" );
		if ( event )
		{
			event->SetInt( "team", GetTeamNumber() );
			event->SetFloat( "recedetime", m_flRecedeTime );
			gameeventmanager->FireEventClientSide( event );
		}
	}
#ifdef GLOWS_ENABLE
	if ( m_hOldGlowEnt != m_hGlowEnt )
	{
		UpdateGlowEffect();
	}
#endif // GLOWS_ENABLE
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TeamTrainWatcher::Spawn( void )
{
	BaseClass::Spawn();

	if ( g_hTrainWatchers.Find( this ) == g_hTrainWatchers.InvalidIndex() )
	{
		g_hTrainWatchers.AddToTail( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TeamTrainWatcher::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

	g_hTrainWatchers.FindAndRemove( this );
}
