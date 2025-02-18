//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef LOGICRELAY_H
#define LOGICRELAY_H

#include "cbase.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"

class CLogicRelay : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLogicRelay, CLogicalEntity );

	CLogicRelay();

	void Activate();
	void Think();

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputEnableRefire( inputdata_t &inputdata );  // Private input handler, not in FGD
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputTrigger( inputdata_t &inputdata );
	void InputCancelPending( inputdata_t &inputdata );

	DECLARE_DATADESC();

	// Outputs
	COutputEvent m_OnTrigger;
	COutputEvent m_OnSpawn;

	bool IsDisabled( void ){ return m_bDisabled; }
	
private:

	bool m_bDisabled;
	bool m_bWaitForRefire;			// Set to disallow a refire while we are waiting for our outputs to finish firing.
};

#endif //LOGICRELAY_H
