//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_hl2_playerlocaldata.h"
#include "dt_recv.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_RECV_TABLE_NOBASE( C_HL2PlayerLocalData, DT_HL2Local )
	RecvPropFloat( RECVINFO(m_flSuitPower) ),
	RecvPropFloat( RECVINFO(m_flSuitPowerLoad) ),
	RecvPropFloat( RECVINFO(m_flTimeAllSuitDevicesOff) ),
	RecvPropInt( RECVINFO(m_bNewSprinting) ),
	RecvPropInt( RECVINFO(m_bZooming) ),
	RecvPropInt( RECVINFO(m_bitsActiveDevices) ),
	RecvPropInt( RECVINFO(m_iSquadMemberCount) ),
	RecvPropInt( RECVINFO(m_iSquadMedicCount) ),
	RecvPropBool( RECVINFO(m_fSquadInFollowMode) ),
	RecvPropBool( RECVINFO(m_bWeaponLowered) ),
	RecvPropEHandle( RECVINFO(m_hAutoAimTarget) ),
	RecvPropVector( RECVINFO(m_vecAutoAimPoint) ),
	RecvPropEHandle( RECVINFO(m_hLadder) ),
	RecvPropBool( RECVINFO(m_bDisplayReticle) ),
	RecvPropBool( RECVINFO(m_bStickyAutoAim) ),
	RecvPropBool( RECVINFO(m_bAutoAimTarget) ),
#ifdef HL2_EPISODIC
	RecvPropFloat( RECVINFO(m_flFlashBattery) ),
	RecvPropVector( RECVINFO(m_vecLocatorOrigin) ),
#endif

	RecvPropDataTable( RECVINFO_DT( m_LadderMove ), 0, &REFERENCE_RECV_TABLE( DT_LadderMove ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE( C_HL2PlayerLocalData )
	DEFINE_PRED_FIELD( m_hLadder, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ), 
	DEFINE_PRED_FIELD( m_flSuitPower, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flSuitPowerLoad, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flTimeAllSuitDevicesOff, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bNewSprinting, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bitsActiveDevices, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_TYPEDESCRIPTION( m_LadderMove, LadderMove_t ),
END_PREDICTION_DATA()

C_HL2PlayerLocalData::C_HL2PlayerLocalData()
{
	m_flSuitPower = 0.0;
	m_flSuitPowerLoad = 0.0f;
	m_flTimeAllSuitDevicesOff = 0.0f;
	m_bZooming = false;
	m_bNewSprinting = false;
	m_iSquadMemberCount = 0;
	m_iSquadMedicCount = 0;
	m_fSquadInFollowMode = false;
	m_bWeaponLowered = false;
	m_hLadder = NULL;
#ifdef HL2_EPISODIC
	m_flFlashBattery = 0.0f;
	m_vecLocatorOrigin = vec3_origin;
#endif
}

