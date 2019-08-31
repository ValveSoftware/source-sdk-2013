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

#ifdef MAPBASE

// I was originally going to add something similar to that queue thing to logic_relay directly, but I later decided to relegate it to a derivative entity.
#define RELAY_QUEUE_SYSTEM 0

#endif

class CLogicRelay : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLogicRelay, CLogicalEntity );

	CLogicRelay();

	void Activate();
	void Think();

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
#ifdef MAPBASE
	void InputEnableRefire( inputdata_t &inputdata );
#else
	void InputEnableRefire( inputdata_t &inputdata );  // Private input handler, not in FGD
#endif
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputTrigger( inputdata_t &inputdata );
#ifdef MAPBASE
	void InputTriggerWithParameter( inputdata_t &inputdata );
#endif
	void InputCancelPending( inputdata_t &inputdata );

	DECLARE_DATADESC();

	// Outputs
	COutputEvent m_OnTrigger;
#ifdef MAPBASE
	COutputVariant m_OnTriggerParameter;
#endif
	COutputEvent m_OnSpawn;

	bool IsDisabled( void ){ return m_bDisabled; }
	
private:

	bool m_bDisabled;
	bool m_bWaitForRefire;			// Set to disallow a refire while we are waiting for our outputs to finish firing.
#if RELAY_QUEUE_SYSTEM
	bool m_bQueueTrigger;
	bool m_bQueueWaiting;
	float m_flRefireTime;
#endif
};

#ifdef MAPBASE
struct LogicRelayQueueInfo_t
{
	DECLARE_SIMPLE_DATADESC();

	bool TriggerWithParameter;
	CBaseEntity *pActivator;
	variant_t value;
	int outputID;
};

//#define LOGIC_RELAY_QUEUE_LIMIT 16

class CLogicRelayQueue : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLogicRelayQueue, CLogicalEntity );

	CLogicRelayQueue();

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputEnableRefire( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputTrigger( inputdata_t &inputdata );
	void InputTriggerWithParameter( inputdata_t &inputdata );
	void InputCancelPending( inputdata_t &inputdata );
	void InputClearQueue( inputdata_t &inputdata );

	DECLARE_DATADESC();

	// Outputs
	COutputEvent m_OnTrigger;
	COutputVariant m_OnTriggerParameter;

	bool IsDisabled( void ){ return m_bDisabled; }

	void HandleNextQueueItem();
	void AddQueueItem(CBaseEntity *pActivator, int outputID, variant_t &value);
	void AddQueueItem(CBaseEntity *pActivator, int outputID);

	int		DrawDebugTextOverlays( void );
	
private:

	bool m_bDisabled;
	bool m_bWaitForRefire;			// Set to disallow a refire while we are waiting for our outputs to finish firing.

	int m_iMaxQueueItems;
	bool m_bDontQueueWhenDisabled; // Don't add to queue while disabled, only when waiting for refire
	CUtlVector<LogicRelayQueueInfo_t> m_QueueItems;
};
#endif

#endif //LOGICRELAY_H
