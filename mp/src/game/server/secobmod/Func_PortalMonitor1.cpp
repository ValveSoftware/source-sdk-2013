//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========
//
// Purpose: Simple model entity that randomly moves and changes direction
//			when activated.
//
//=============================================================================

#include "cbase.h"
#include "point_camera.h"
#include "info_camera_link.h"

class CMyModelEntity1 : public CBaseAnimating
{
public:
	DECLARE_CLASS( CMyModelEntity1, CBaseAnimating );
	DECLARE_DATADESC();

	CMyModelEntity1()
	{
		m_bActive = false;
	}

	void Spawn( void );
	void Precache( void );
	
	virtual void UpdateOnRemove();

private:
	void InputSetCamera(inputdata_t &inputdata);
	void SetCameraByName(const char *szName);
	void ReleaseCameraLink();

	bool	m_bActive;
	float	m_flNextChangeTime;
	EHANDLE m_hInfoCameraLink;
};

//IMPLEMENT_SERVERCLASS_ST( CFuncPortalMonitor2, DT_FuncPortalMonitor2 )
//END_SEND_TABLE()
//!!!!!!!!!!!!!!!!!!!!!!!!!1

LINK_ENTITY_TO_CLASS( my_model_entity1, CMyModelEntity1 );

// Start of our data description for the class
BEGIN_DATADESC( CMyModelEntity1 )
	
	// Save/restore our active state
	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextChangeTime, FIELD_TIME ),
	
	DEFINE_FIELD( m_hInfoCameraLink, FIELD_EHANDLE ),

	// Outputs
	DEFINE_INPUTFUNC( FIELD_STRING, "SetCamera", InputSetCamera ),

END_DATADESC()

// Name of our entity's model
#define	ENTITY_MODEL	"models/obco_portal1.mdl"

//-----------------------------------------------------------------------------
// Purpose: Precache assets used by the entity
//-----------------------------------------------------------------------------
void CMyModelEntity1::Precache( void )
{
	PrecacheModel( ENTITY_MODEL );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the entity's initial state
//-----------------------------------------------------------------------------
void CMyModelEntity1::Spawn( void )
{
	Precache();

	SetModel( ENTITY_MODEL );
	SetSolid( SOLID_NONE );
	UTIL_SetSize( this, -Vector(20,20,20), Vector(20,20,20) );

SetCameraByName("portalcamera2");
}

void CMyModelEntity1::UpdateOnRemove()
{
	ReleaseCameraLink();
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Frees the camera.
//-----------------------------------------------------------------------------
void CMyModelEntity1::ReleaseCameraLink()
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
void CMyModelEntity1::SetCameraByName(const char *szName)
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
void CMyModelEntity1::InputSetCamera(inputdata_t &inputdata)
{
	SetCameraByName( inputdata.value.String() );
}
