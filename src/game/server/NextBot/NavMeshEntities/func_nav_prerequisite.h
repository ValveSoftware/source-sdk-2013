//========= Copyright Valve Corporation, All rights reserved. ============//
// NextBot paths that go through this entity must fulfill the given prerequisites to pass
// Michael Booth, August 2009

#ifndef FUNC_NAV_PREREQUISITE_H
#define FUNC_NAV_PREREQUISITE_H

#include "triggers.h"

/**
 * NextBot paths that pass through this entity must fulfill the given prerequisites to pass
 */
DECLARE_AUTO_LIST( IFuncNavPrerequisiteAutoList );

class CFuncNavPrerequisite : public CBaseTrigger, public IFuncNavPrerequisiteAutoList
{
	DECLARE_CLASS( CFuncNavPrerequisite, CBaseTrigger );

public:
	CFuncNavPrerequisite();

	DECLARE_DATADESC();

	virtual void Spawn( void );

	enum TaskType
	{
		TASK_NONE = 0,
		TASK_DESTROY_ENTITY = 1,
		TASK_MOVE_TO_ENTITY = 2,
		TASK_WAIT			= 3,
	};

	bool IsTask( TaskType type ) const;
	CBaseEntity *GetTaskEntity( void );
	float GetTaskValue( void ) const;

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	bool IsEnabled( void ) const { return !m_isDisabled; }

protected:
	int m_task;
	string_t m_taskEntityName;
	float m_taskValue;
	bool m_isDisabled;
	EHANDLE m_hTaskEntity;
};

inline float CFuncNavPrerequisite::GetTaskValue( void ) const
{
	return m_taskValue;
}


#endif // FUNC_NAV_PREREQUISITE_H
