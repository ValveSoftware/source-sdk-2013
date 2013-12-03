//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_prop_vehicle.h"
#include "c_vehicle_jeep.h"
#include "movevars_shared.h"
#include "view.h"
#include "flashlighteffect.h"
#include "c_baseplayer.h"
#include "c_te_effect_dispatch.h"
#include "hl2_vehicle_radar.h"
#include "usermessages.h"
#include "hud_radar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// Client-side Episodic Jeep (Jalopy) Class
//
class C_PropJeepEpisodic : public C_PropJeep
{

	DECLARE_CLASS( C_PropJeepEpisodic, C_PropJeep );

public:
	DECLARE_CLIENTCLASS();

public:
	C_PropJeepEpisodic();

	void OnEnteredVehicle( C_BasePlayer *pPlayer );
	void Simulate( void );

public:
	int		m_iNumRadarContacts;
	Vector	m_vecRadarContactPos[ RADAR_MAX_CONTACTS ];
	int		m_iRadarContactType[ RADAR_MAX_CONTACTS ];
};
C_PropJeepEpisodic *g_pJalopy = NULL;

IMPLEMENT_CLIENTCLASS_DT( C_PropJeepEpisodic, DT_CPropJeepEpisodic, CPropJeepEpisodic )
	//CNetworkVar( int, m_iNumRadarContacts );
	RecvPropInt( RECVINFO(m_iNumRadarContacts) ),

	//CNetworkArray( Vector, m_vecRadarContactPos, RADAR_MAX_CONTACTS );
	RecvPropArray( RecvPropVector(RECVINFO(m_vecRadarContactPos[0])), m_vecRadarContactPos ),

	//CNetworkArray( int, m_iRadarContactType, RADAR_MAX_CONTACTS );
	RecvPropArray( RecvPropInt( RECVINFO(m_iRadarContactType[0] ) ), m_iRadarContactType ),

END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void __MsgFunc_UpdateJalopyRadar(bf_read &msg) 
{
	// Radar code here!
	if( !GetHudRadar() )
		return;

	// Sometimes we update more quickly when we need to track something in high resolution.
	// Usually we do not, so default to false.
	GetHudRadar()->m_bUseFastUpdate = false;

	for( int i = 0 ; i < g_pJalopy->m_iNumRadarContacts ; i++ )
	{
		if( g_pJalopy->m_iRadarContactType[i] == RADAR_CONTACT_DOG )
		{
			GetHudRadar()->m_bUseFastUpdate = true;
			break;
		}
	}

	float flContactTimeToLive;

	if( GetHudRadar()->m_bUseFastUpdate )
	{
		flContactTimeToLive = RADAR_UPDATE_FREQUENCY_FAST;
	}
	else
	{
		flContactTimeToLive = RADAR_UPDATE_FREQUENCY;
	}

	for( int i = 0 ; i < g_pJalopy->m_iNumRadarContacts ; i++ )
	{
		GetHudRadar()->AddRadarContact( g_pJalopy->m_vecRadarContactPos[i], g_pJalopy->m_iRadarContactType[i], flContactTimeToLive );	
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
C_PropJeepEpisodic::C_PropJeepEpisodic()
{
	if( g_pJalopy == NULL )
	{
		usermessages->HookMessage( "UpdateJalopyRadar", __MsgFunc_UpdateJalopyRadar );
	}

	g_pJalopy = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropJeepEpisodic::Simulate( void )
{
	// Keep trying to hook to the radar.
	if( GetHudRadar() != NULL )
	{
		// This is not our ideal long-term solution. This will only work if you only have 
		// one jalopy in a given level. The Jalopy and the Radar Screen are currently both
		// assumed to be singletons. This is appropriate for EP2, however. (sjb)
		GetHudRadar()->SetVehicle( this );
	}

	BaseClass::Simulate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropJeepEpisodic::OnEnteredVehicle( C_BasePlayer *pPlayer )
{
	BaseClass::OnEnteredVehicle( pPlayer );
}
