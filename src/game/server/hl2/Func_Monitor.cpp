//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "point_camera.h"
#include "modelentities.h"
#include "info_camera_link.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CFuncMonitor : public CFuncBrush
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CFuncMonitor, CFuncBrush );
	DECLARE_SERVERCLASS();

public:
	virtual void Activate();
	virtual void UpdateOnRemove();

private:
	void InputSetCamera(inputdata_t &inputdata);
	void SetCameraByName(const char *szName);
	void ReleaseCameraLink();

	EHANDLE m_hInfoCameraLink;
};

// automatically hooks in the system's callbacks
BEGIN_DATADESC( CFuncMonitor )

	DEFINE_FIELD( m_hInfoCameraLink, FIELD_EHANDLE ),

	// Outputs
	DEFINE_INPUTFUNC( FIELD_STRING, "SetCamera", InputSetCamera ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( func_monitor, CFuncMonitor );


IMPLEMENT_SERVERCLASS_ST( CFuncMonitor, DT_FuncMonitor )
END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Purpose: Called after all entities have spawned and after a load game.
//-----------------------------------------------------------------------------
void CFuncMonitor::Activate()
{
 	BaseClass::Activate();
	SetCameraByName(STRING(m_target));
}

void CFuncMonitor::UpdateOnRemove()
{
	ReleaseCameraLink();
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Frees the camera.
//-----------------------------------------------------------------------------
void CFuncMonitor::ReleaseCameraLink()
{
	if ( m_hInfoCameraLink )
	{
		UTIL_Remove( m_hInfoCameraLink );
		m_hInfoCameraLink = NULL;

		// Keep the target up-to-date for save/load
		m_target = NULL_STRING;
	}
}


//-----------------------------------------------------------------------------
// Sets camera 
//-----------------------------------------------------------------------------
void CFuncMonitor::SetCameraByName(const char *szName)
{
	ReleaseCameraLink();
	CBaseEntity *pBaseEnt = gEntList.FindEntityByName( NULL, szName );
	if( pBaseEnt )
	{
		CPointCamera *pCamera = dynamic_cast<CPointCamera *>( pBaseEnt );
		if( pCamera )
		{
			// Keep the target up-to-date for save/load
			m_target = MAKE_STRING( szName );
			m_hInfoCameraLink = CreateInfoCameraLink( this, pCamera ); 
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncMonitor::InputSetCamera(inputdata_t &inputdata)
{
	SetCameraByName( inputdata.value.String() );
}
