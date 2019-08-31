//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef LOGICRANDOMOUTPUTS_H
#define LOGICRANDOMOUTPUTS_H

#include "cbase.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"

#define NUM_RANDOM_OUTPUTS 8

class CLogicRandomOutputs : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLogicRandomOutputs, CLogicalEntity );

	CLogicRandomOutputs();

	void Activate();
	void Think();
	virtual bool KeyValue( const char *szKeyName, const char *szValue );
#ifdef MAPBASE
	virtual bool GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen );
#endif

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputEnableRefire( inputdata_t &inputdata );  // Private input handler, not in FGD
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputTrigger( inputdata_t &inputdata );
	void InputCancelPending( inputdata_t &inputdata );

	DECLARE_DATADESC();

	// Outputs
	COutputEvent m_Output[ NUM_RANDOM_OUTPUTS ];
	COutputEvent m_OnSpawn;

	float m_flOnTriggerChance[ NUM_RANDOM_OUTPUTS ];
	
private:

	bool m_bDisabled;
	bool m_bWaitForRefire;			// Set to disallow a refire while we are waiting for our outputs to finish firing.
};

#endif //LOGICRANDOMOUTPUTS_H
