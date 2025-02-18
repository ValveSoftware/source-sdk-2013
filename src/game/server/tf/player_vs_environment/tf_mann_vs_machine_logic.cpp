//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_mann_vs_macine_logic.h
// Mann Vs Machine game mode singleton manager
// Michael Booth, June 2011

#include "cbase.h"

#include "tf_team.h"
#include "tf_obj_sentrygun.h"
#include "tf_population_manager.h"
#include "bot/map_entities/tf_bot_generator.h"
#include "player_vs_environment/tf_mann_vs_machine_logic.h"
#include "tf_gamerules.h"
#include "tf_objective_resource.h"

CHandle<CMannVsMachineLogic> g_hMannVsMachineLogic;

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
BEGIN_DATADESC( CMannVsMachineLogic )
	DEFINE_THINKFUNC( Update ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_mann_vs_machine, CMannVsMachineLogic );

//-------------------------------------------------------------------------
CMannVsMachineLogic::CMannVsMachineLogic()
{
	InitPopulationManager();

	m_flNextAlarmCheck = 0.0f;
}


//-------------------------------------------------------------------------
CMannVsMachineLogic::~CMannVsMachineLogic()
{
	g_hMannVsMachineLogic = NULL;
}


//-------------------------------------------------------------------------
void CMannVsMachineLogic::Spawn( void )
{
	BaseClass::Spawn();

	SetThink( &CMannVsMachineLogic::Update );
	SetNextThink( gpGlobals->curtime );

	g_hMannVsMachineLogic = this;
}


//-------------------------------------------------------------------------
void CMannVsMachineLogic::SetupOnRoundStart( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsMannVsMachineMode() )
		return;

	TFGameRules()->SetNextMvMPopfile( "" );

	if ( m_populationManager )
	{
		m_populationManager->SetupOnRoundStart();
	}
}


//--------------------------------------------------------------------------------------------------------
void CMannVsMachineLogic::Update( void )
{
	VPROF_BUDGET( "CMannVsMachineLogic::Update", "Game" );

	SetNextThink( gpGlobals->curtime +  0.05f );

	if ( !TFGameRules() || !TFGameRules()->IsMannVsMachineMode() )
		return;

	if ( m_populationManager )
	{
		m_populationManager->Update();
	}

	// we don't need to run this check as often as we're calling our update() function
	if ( m_flNextAlarmCheck < gpGlobals->curtime )
	{
		m_flNextAlarmCheck = gpGlobals->curtime + 0.1;
		for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
		{
			CCaptureFlag *pFlag = static_cast<CCaptureFlag *>( ICaptureFlagAutoList::AutoList()[i] );
			if ( pFlag->IsStolen() )
			{
				for ( int j=0; j<IFlagDetectionZoneAutoList::AutoList().Count(); ++j )
				{
					CFlagDetectionZone *pZone = static_cast<CFlagDetectionZone *>( IFlagDetectionZoneAutoList::AutoList()[j] );
  					if ( !pZone->IsDisabled() && pZone->IsAlarmZone() && pZone->PointIsWithin( pFlag->GetAbsOrigin() ) )
					{
						// Is the alarm currently off?
						if ( TFGameRules()->GetMannVsMachineAlarmStatus() == false )
						{
							IGameEvent *event = gameeventmanager->CreateEvent( "mvm_bomb_alarm_triggered" );
							if ( event )
							{
								gameeventmanager->FireEvent( event );
							}
						}

						TFGameRules()->SetMannVsMachineAlarmStatus( true );
						return;
					}
				}
			}
		}

		TFGameRules()->SetMannVsMachineAlarmStatus( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineLogic::InitPopulationManager( void )
{
	bool bFound = false;
	const char *pszFormat = MVM_POP_FILE_PATH "/%s.pop";
	char szFileName[MAX_PATH] = { 0 };

	// Did they request something specific?
	if ( Q_strlen( TFGameRules()->GetNextMvMPopfile() ) )
	{
		Q_snprintf( szFileName, sizeof( szFileName ), pszFormat, TFGameRules()->GetNextMvMPopfile() );
		if ( g_pFullFileSystem->FileExists( szFileName, "GAME" ) )
		{
			bFound = true;
		}
		else
		{
			// It might need the additional map prefix to find the file
			Q_snprintf( szFileName, sizeof( szFileName ), MVM_POP_FILE_PATH "/%s_%s.pop", STRING( gpGlobals->mapname ), TFGameRules()->GetNextMvMPopfile() );
			if ( g_pFullFileSystem->FileExists( szFileName, "GAME" ) )
			{
				bFound = true;
			}
			if ( !bFound )
			{
				Warning( "Population file '%s' not found", TFGameRules()->GetNextMvMPopfile() );
			}
		}
	}

	// See if we have any default popfiles and use the default-iest one
	if ( !bFound )
	{
		CUtlVector< CUtlString > defaultPopFileList;
		CUtlString defaultPopFileName;
		g_pPopulationManager->FindDefaultPopulationFileShortNames( defaultPopFileList );
		if ( defaultPopFileList.Count() )
		{
			if ( g_pPopulationManager->FindPopulationFileByShortName( defaultPopFileList[0], defaultPopFileName ) && g_pPopulationManager->IsValidPopfile( defaultPopFileName ) )
			{
				V_strncpy( szFileName, defaultPopFileName, sizeof( szFileName ) );
				bFound = true;
			}
		}
	}

	// Use mapname.pop. This won't exist but would've been the highest priority default so you'll at least get an error
	// about the file of last resort.
	if ( !bFound )
	{
		Q_snprintf( szFileName, sizeof( szFileName ), pszFormat, STRING( gpGlobals->mapname ) );
	}

	if ( m_populationManager && V_strcmp( m_populationManager->GetPopulationFilename(), szFileName ) != 0 )
	{
		UTIL_RemoveImmediate( m_populationManager );
		m_populationManager = NULL;
	}

	if ( !m_populationManager )
	{
		m_populationManager = (CPopulationManager *)CreateEntityByName( "info_populator" );
		m_populationManager->SetPopulationFilename( szFileName );
	}
}
