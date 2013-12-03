//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Physics constraint entities
//
// $NoKeywords: $
//===========================================================================//

#ifndef PHYSCONSTRAINT_H
#define PHYSCONSTRAINT_H
#ifdef _WIN32
#pragma once
#endif

#include "vphysics/constraints.h"

struct hl_constraint_info_t
{
	hl_constraint_info_t() 
	{ 
		pObjects[0] = pObjects[1] = NULL;
		pGroup = NULL;
		anchorPosition[0].Init();
		anchorPosition[1].Init();
		swapped = false; 
		massScale[0] = massScale[1] = 1.0f;
	}
	Vector			anchorPosition[2];
	IPhysicsObject	*pObjects[2];
	IPhysicsConstraintGroup *pGroup;
	float			massScale[2];
	bool			swapped;
};

abstract_class CPhysConstraint : public CLogicalEntity
{
	DECLARE_CLASS( CPhysConstraint, CLogicalEntity );
public:

	CPhysConstraint();
	~CPhysConstraint();

	DECLARE_DATADESC();

	void Spawn( void );
	void Precache( void );
	void Activate( void );

	void ClearStaticFlag( IPhysicsObject *pObj );

	virtual void Deactivate();

	void OnBreak( void );

	void InputBreak( inputdata_t &inputdata );

	void InputOnBreak( inputdata_t &inputdata );

	void InputTurnOn( inputdata_t &inputdata );

	void InputTurnOff( inputdata_t &inputdata );

	int DrawDebugTextOverlays();

	void DrawDebugGeometryOverlays();

	void GetBreakParams( constraint_breakableparams_t &params, const hl_constraint_info_t &info );

	// the notify system calls this on the constrained entities - used to detect & follow teleports
	void NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params );

	// gets called at setup time on first init and restore
	virtual void OnConstraintSetup( hl_constraint_info_t &info ); 

	// return the internal constraint object (used by sound gadgets)
	inline IPhysicsConstraint *GetPhysConstraint() { return m_pConstraint; }

	string_t GetNameAttach1( void ){ return m_nameAttach1; }
	string_t GetNameAttach2( void ){ return m_nameAttach2; }

protected:	
	void GetConstraintObjects( hl_constraint_info_t &info );
	void SetupTeleportationHandling( hl_constraint_info_t &info );
	bool ActivateConstraint( void );
	virtual IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info ) = 0;

	IPhysicsConstraint	*m_pConstraint;

	// These are "template" values used to construct the hinge
	string_t		m_nameAttach1;
	string_t		m_nameAttach2;
	string_t		m_breakSound;
	string_t		m_nameSystem;
	float			m_forceLimit;
	float			m_torqueLimit;
	unsigned int	m_teleportTick;
	float			m_minTeleportDistance;

	COutputEvent	m_OnBreak;
};

//-----------------------------------------------------------------------------
// Purpose: Fixed breakable constraint
//-----------------------------------------------------------------------------
class CPhysFixed : public CPhysConstraint
{
	DECLARE_CLASS( CPhysFixed, CPhysConstraint );
public:
	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );

	// just for debugging - move to the position of the reference entity
	void MoveToRefPosition()
	{
		if ( m_pConstraint )
		{
			matrix3x4_t xformRef;
			m_pConstraint->GetConstraintTransform( &xformRef, NULL );
			IPhysicsObject *pObj = m_pConstraint->GetReferenceObject();
			if ( pObj && pObj->IsMoveable() )
			{
				Vector pos, posWorld;
				MatrixPosition( xformRef, pos );
				pObj->LocalToWorld(&posWorld, pos);
				SetAbsOrigin(posWorld);
			}
		}
	}
	int DrawDebugTextOverlays()
	{
		if ( m_debugOverlays & OVERLAY_TEXT_BIT )
		{
			MoveToRefPosition();
		}
		return BaseClass::DrawDebugTextOverlays();
	}
	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			MoveToRefPosition();
		}
		BaseClass::DrawDebugGeometryOverlays();
	}
};

#endif // PHYSCONSTRAINT_H

