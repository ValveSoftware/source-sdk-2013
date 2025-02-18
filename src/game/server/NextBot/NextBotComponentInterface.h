// NextBotComponentInterface.h
// Interface for all components
// Author: Michael Booth, May 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_COMPONENT_INTERFACE_H_
#define _NEXT_BOT_COMPONENT_INTERFACE_H_

#include "NextBotEventResponderInterface.h"
#include "vscript_server.h"

class INextBot;
class Path;
class CGameTrace;
class CTakeDamageInfo;


//--------------------------------------------------------------------------------------------------------------------------
/**
 * Various processes can invoke a "reply" (ie: callback) via instances of this interface
 */
class INextBotReply
{
public:
	virtual void OnSuccess( INextBot *bot )  { }						// invoked when process completed successfully

	enum FailureReason
	{
		DENIED,
		INTERRUPTED,
		FAILED
	};
	virtual void OnFail( INextBot *bot, FailureReason reason ) { }		// invoked when process failed
};


//--------------------------------------------------------------------------------------------------------------------------
/**
 * Next Bot component interface
 */
class INextBotComponent : public INextBotEventResponder
{
public:
	INextBotComponent( INextBot *bot );
	virtual ~INextBotComponent();

	virtual void Reset( void )	{ m_lastUpdateTime = 0; m_curInterval = TICK_INTERVAL; }				// reset to initial state
	virtual void Update( void ) = 0;									// update internal state
	virtual void Upkeep( void ) { }										// lightweight update guaranteed to occur every server tick

	inline bool ComputeUpdateInterval();								// return false is no time has elapsed (interval is zero)
	inline float GetUpdateInterval();

	virtual INextBot *GetBot( void ) const  { return m_bot; }

	//- Script access to component functions ------------------------------------------------------------------
	DECLARE_ENT_SCRIPTDESC();
	HSCRIPT GetScriptInstance();
	
private:
	float m_lastUpdateTime;
	float m_curInterval;

	friend class INextBot;
	
	INextBot *m_bot;
	INextBotComponent *m_nextComponent;									// simple linked list of components in the bot

	HSCRIPT	m_hScriptInstance;
};


inline bool INextBotComponent::ComputeUpdateInterval() 
{ 
	if ( m_lastUpdateTime ) 
	{ 
		float interval = gpGlobals->curtime - m_lastUpdateTime;

		const float minInterval = 0.0001f;
		if ( interval > minInterval )
		{
			m_curInterval = interval;
			m_lastUpdateTime = gpGlobals->curtime;
			return true;
		}

		return false;
	}

	// First update - assume a reasonable interval.
	// We need the very first update to do work, for cases
	// where the bot was just created and we need to propagate
	// an event to it immediately.
	m_curInterval = 0.033f;
	m_lastUpdateTime = gpGlobals->curtime - m_curInterval;

	return true;
}

inline float INextBotComponent::GetUpdateInterval() 
{ 
	return m_curInterval; 
}

inline HSCRIPT ToHScript( INextBotComponent *pNextBotComponent )
{
	return ( pNextBotComponent ) ? pNextBotComponent->GetScriptInstance() : NULL;
}

template <> ScriptClassDesc_t *GetScriptDesc<INextBotComponent>( INextBotComponent * );
inline INextBotComponent *ToNextBotComponent( HSCRIPT hScript )
{
	return ( IsValid( hScript ) ) ? (INextBotComponent *)g_pScriptVM->GetInstanceValue( hScript, GetScriptDescForClass( INextBotComponent ) ) : NULL;
}

#endif // _NEXT_BOT_COMPONENT_INTERFACE_H_
