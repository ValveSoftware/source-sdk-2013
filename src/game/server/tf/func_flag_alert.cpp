//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "func_flag_alert.h"
#include "entity_capture_flag.h"
#include "tf_player.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( func_flag_alert, CFuncFlagAlertZone );

BEGIN_DATADESC( CFuncFlagAlertZone )

	DEFINE_KEYFIELD( m_bPlaySound,	FIELD_BOOLEAN,	"playsound" ),
	DEFINE_KEYFIELD( m_nAlertDelay,	FIELD_INTEGER,	"alert_delay" ),

	DEFINE_OUTPUT( m_OnTriggeredByTeam1,	"OnTriggeredByTeam1" ),
	DEFINE_OUTPUT( m_OnTriggeredByTeam2,	"OnTriggeredByTeam2" ),

END_DATADESC();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncFlagAlertZone::CFuncFlagAlertZone()
{
	for ( int i = 0 ; i < TF_TEAM_COUNT ; i++ )
	{
		m_flNextAlertTime[i] = 0.0f;
	}
	
	m_bPlaySound = true;
	m_nAlertDelay = 10;
}

//-----------------------------------------------------------------------------`
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncFlagAlertZone::Spawn( void )
{
	AddSpawnFlags( SF_TRIGGER_ALLOW_ALL );

	BaseClass::Spawn();
	InitTrigger();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncFlagAlertZone::StartTouch( CBaseEntity *pOther )
{
	if ( !m_bDisabled )
	{
		if ( pOther && pOther->IsPlayer() && ( pOther->GetTeamNumber() != GetTeamNumber() ) )
		{
			CTFPlayer *pPlayer = ToTFPlayer( pOther );
			if ( pPlayer && pPlayer->HasTheFlag() )
			{
				int iTeamNum = pPlayer->GetTeamNumber();

				if ( gpGlobals->curtime > m_flNextAlertTime[iTeamNum] )
				{
					if ( m_bPlaySound && TFGameRules() )
					{
						int iBroadcastTeam = ( iTeamNum == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED;
						TFGameRules()->BroadcastSound( iBroadcastTeam, "Announcer.SecurityAlert" );
					}

					switch( iTeamNum )
					{
					case TF_TEAM_RED:
						m_OnTriggeredByTeam1.FireOutput( this, this );
						break;
					case TF_TEAM_BLUE:
						m_OnTriggeredByTeam2.FireOutput( this, this );
						break;
					default:
						break;
					}

					m_flNextAlertTime[iTeamNum] = gpGlobals->curtime + m_nAlertDelay;
				}
			}
		}
	}

	BaseClass::StartTouch( pOther );
}
