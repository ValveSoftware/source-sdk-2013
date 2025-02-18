//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2_playerlocaldata.h"
#include "hl2_player.h"
#include "mathlib/mathlib.h"
#include "entitylist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SEND_TABLE_NOBASE( CHL2PlayerLocalData, DT_HL2Local )
	// misyl: SPROP_NOSCALE looks a bit overkill here, but I get pred errors without it.
	// Getting this as accurate as impossible is important... If we get out of sync with
	// this, we can get pred errors going in/out of sprint which is SUPER JANKY!!!
	SendPropFloat( SENDINFO(m_flSuitPower), -1, SPROP_NOSCALE, 0.0, 100.0 ),
	SendPropFloat( SENDINFO(m_flSuitPowerLoad), -1, SPROP_NOSCALE, 0.0, 100.0 ),
	SendPropFloat( SENDINFO(m_flTimeAllSuitDevicesOff), -1, SPROP_NOSCALE ),
	SendPropInt( SENDINFO(m_bNewSprinting), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_bZooming), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_bitsActiveDevices), MAX_SUIT_DEVICES, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_iSquadMemberCount) ),
	SendPropInt( SENDINFO(m_iSquadMedicCount) ),
	SendPropBool( SENDINFO(m_fSquadInFollowMode) ),
	SendPropBool( SENDINFO(m_bWeaponLowered) ),
	SendPropEHandle( SENDINFO(m_hAutoAimTarget) ),
	SendPropVector( SENDINFO(m_vecAutoAimPoint) ),
	SendPropEHandle( SENDINFO(m_hLadder) ),
	SendPropBool( SENDINFO(m_bDisplayReticle) ),
	SendPropBool( SENDINFO(m_bStickyAutoAim) ),
	SendPropBool( SENDINFO(m_bAutoAimTarget) ),
#ifdef HL2_EPISODIC
	SendPropFloat( SENDINFO(m_flFlashBattery) ),
	SendPropVector( SENDINFO(m_vecLocatorOrigin) ),
#endif
	SendPropDataTable( SENDINFO_DT( m_LadderMove ), &REFERENCE_SEND_TABLE( DT_LadderMove ), SendProxy_SendLocalDataTable ),
END_SEND_TABLE()

BEGIN_SIMPLE_DATADESC( CHL2PlayerLocalData )
	DEFINE_FIELD( m_flSuitPower, FIELD_FLOAT ),
	DEFINE_FIELD( m_flSuitPowerLoad, FIELD_FLOAT ),
	DEFINE_FIELD( m_flTimeAllSuitDevicesOff, FIELD_FLOAT ),
	DEFINE_FIELD( m_bZooming, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bNewSprinting, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bitsActiveDevices, FIELD_INTEGER ),
	DEFINE_FIELD( m_iSquadMemberCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_iSquadMedicCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_fSquadInFollowMode, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bWeaponLowered, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDisplayReticle, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bStickyAutoAim, FIELD_BOOLEAN ),
#ifdef HL2_EPISODIC
	DEFINE_FIELD( m_flFlashBattery, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecLocatorOrigin, FIELD_POSITION_VECTOR ),
#endif
	// Ladder related stuff
	DEFINE_FIELD( m_hLadder, FIELD_EHANDLE ),
	DEFINE_EMBEDDED( m_LadderMove ),
END_DATADESC()

CHL2PlayerLocalData::CHL2PlayerLocalData()
{
	m_flSuitPower = 0.0f;
	m_flSuitPowerLoad = 0.0f;
	m_flTimeAllSuitDevicesOff = 0.0f;
	m_bZooming = false;
	m_bNewSprinting = false;
	m_bWeaponLowered = false;
	m_hAutoAimTarget.Set(NULL);
	m_hLadder.Set(NULL);
	m_vecAutoAimPoint.GetForModify().Init();
	m_bDisplayReticle = false;
#ifdef HL2_EPISODIC
	m_flFlashBattery = 0.0f;
#endif
}

