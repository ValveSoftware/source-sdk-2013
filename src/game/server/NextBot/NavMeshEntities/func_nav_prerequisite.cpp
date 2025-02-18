//========= Copyright Valve Corporation, All rights reserved. ============//
// NextBot paths that go through this entity must fulfill the given prerequisites to pass
// Michael Booth, August 2009

#include "cbase.h"
#include "func_nav_prerequisite.h"
#include "ndebugoverlay.h"
#include "modelentities.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


LINK_ENTITY_TO_CLASS( func_nav_prerequisite, CFuncNavPrerequisite );

BEGIN_DATADESC( CFuncNavPrerequisite )
	DEFINE_KEYFIELD( m_task,			FIELD_INTEGER,	"Task" ),
	DEFINE_KEYFIELD( m_taskEntityName,	FIELD_STRING,	"Entity" ),
	DEFINE_KEYFIELD( m_taskValue,		FIELD_FLOAT,	"Value" ),
	DEFINE_KEYFIELD( m_isDisabled,		FIELD_BOOLEAN,	"StartDisabled" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC()

IMPLEMENT_AUTO_LIST( IFuncNavPrerequisiteAutoList );


//-----------------------------------------------------------------------------
CFuncNavPrerequisite::CFuncNavPrerequisite()
{
	m_task = TASK_NONE;
	m_hTaskEntity = NULL;
}


//-----------------------------------------------------------------------------
void CFuncNavPrerequisite::Spawn( void )
{
	AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );

	BaseClass::Spawn();
	InitTrigger();
}


//-----------------------------------------------------------------------------
bool CFuncNavPrerequisite::IsTask( TaskType task ) const
{
	return task == m_task ? true : false;
}


//-----------------------------------------------------------------------------
CBaseEntity *CFuncNavPrerequisite::GetTaskEntity( void )
{
	if ( m_hTaskEntity == NULL )
	{
		m_hTaskEntity = gEntList.FindEntityByName( NULL, m_taskEntityName );
	}
	return m_hTaskEntity;
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavPrerequisite::InputEnable( inputdata_t &inputdata )
{
	m_isDisabled = false;
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavPrerequisite::InputDisable( inputdata_t &inputdata )
{
	m_isDisabled = true;
}
