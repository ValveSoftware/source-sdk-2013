//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Recreates Portal 2 linked_portal_door visual functionality using SDK code only.
//			(basically a combination of point_camera and func_reflective_glass)
//
//===========================================================================//

#include "cbase.h"
#include "modelentities.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CFuncFakeWorldPortal : public CFuncBrush
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CFuncFakeWorldPortal, CFuncBrush );
	DECLARE_SERVERCLASS();

	CFuncFakeWorldPortal()
	{
		// Equivalent to SKYBOX_2DSKYBOX_VISIBLE, the original sky setting
		m_iSkyMode = 2;
	}

public:

	virtual void Spawn( void )
	{
		BaseClass::Spawn();

		if (m_target != NULL_STRING)
		{
			m_hTargetPlane = gEntList.FindEntityByName( NULL, m_target, this );
			if (!m_hTargetPlane)
				Warning("%s: Invalid target plane \"%s\"!\n", GetDebugName(), STRING(m_target));
		}
		else
		{
			Warning("%s: No target plane!\n", GetDebugName());
		}

		if (m_iszFogController != NULL_STRING)
		{
			m_hFogController = gEntList.FindEntityByName( NULL, m_iszFogController, this );
			if (!m_hFogController)
				Warning("%s: Invalid fog controller \"%s\"!\n", GetDebugName(), STRING(m_iszFogController));
		}
	}

	// Input handlers
	void InputSetTargetPlane( inputdata_t &inputdata ) { m_hTargetPlane = inputdata.value.Entity(); if (m_hTargetPlane) { m_target = m_hTargetPlane->GetEntityName(); } }
	void InputSetTargetPlaneAngle( inputdata_t &inputdata ) { Vector vec; inputdata.value.Vector3D(vec); m_PlaneAngles.Init(vec.x, vec.y, vec.z); }
	void InputSetSkyMode( inputdata_t &inputdata ) { m_iSkyMode = inputdata.value.Int(); }
	void InputSetRenderTarget( inputdata_t &inputdata ) { m_iszRenderTarget = inputdata.value.StringID(); }
	void InputSetFogController( inputdata_t &inputdata ) { m_hFogController = inputdata.value.Entity(); if (m_hFogController) { m_iszFogController = m_hFogController->GetEntityName(); } }
	void InputSetScale( inputdata_t &inputdata ) { m_flScale = inputdata.value.Float(); }

private:

	CNetworkHandle( CBaseEntity, m_hTargetPlane );
	CNetworkQAngle( m_PlaneAngles );
	CNetworkVar( int, m_iSkyMode );
	CNetworkVar( float, m_flScale );
	CNetworkVar( string_t, m_iszRenderTarget );

	CNetworkHandle( CBaseEntity, m_hFogController );
	string_t m_iszFogController;
};

// automatically hooks in the system's callbacks
BEGIN_DATADESC( CFuncFakeWorldPortal )

	DEFINE_FIELD( m_hTargetPlane, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_PlaneAngles, FIELD_VECTOR, "PlaneAngles" ),
	DEFINE_KEYFIELD( m_iSkyMode, FIELD_INTEGER, "SkyMode" ),
	DEFINE_KEYFIELD( m_flScale, FIELD_FLOAT, "scale" ),
	DEFINE_KEYFIELD( m_iszRenderTarget, FIELD_STRING, "RenderTarget" ),
	DEFINE_FIELD( m_hFogController, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_iszFogController, FIELD_STRING, "FogController" ),

	DEFINE_INPUTFUNC( FIELD_EHANDLE, "SetTargetPlane", InputSetTargetPlane ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetTargetPlaneAngle", InputSetTargetPlaneAngle ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetSkyMode", InputSetSkyMode ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRenderTarget", InputSetRenderTarget ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "SetFogController", InputSetFogController ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetScale", InputSetScale ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_fake_worldportal, CFuncFakeWorldPortal );

IMPLEMENT_SERVERCLASS_ST( CFuncFakeWorldPortal, DT_FuncFakeWorldPortal )

	SendPropEHandle( SENDINFO( m_hTargetPlane ) ),
	SendPropVector( SENDINFO( m_PlaneAngles ), -1, SPROP_COORD ),
	SendPropInt( SENDINFO( m_iSkyMode ) ),
	SendPropFloat( SENDINFO( m_flScale ) ),
	SendPropStringT( SENDINFO( m_iszRenderTarget ) ),
	SendPropEHandle( SENDINFO( m_hFogController ) ),

END_SEND_TABLE()
