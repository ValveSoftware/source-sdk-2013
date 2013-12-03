//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TIMEDEVENT_H
#define TIMEDEVENT_H
#ifdef _WIN32
#pragma once
#endif

// This class triggers events at a specified rate. Just call NextEvent() and do an event until it 
// returns false. For example, if you want to spawn particles 10 times per second, do this:
// pTimer->SetRate(10);
// float tempDelta = fTimeDelta;
// while(pTimer->NextEvent(tempDelta))
//		spawn a particle

class TimedEvent
{
public:
				TimedEvent()
				{
					m_TimeBetweenEvents = -1;
					m_fNextEvent = 0;
				}

	// Rate is in events per second (ie: rate of 15 will trigger 15 events per second).
	inline void	Init(float rate)			
	{
		m_TimeBetweenEvents = 1.0f / rate;
		m_fNextEvent = 0;
	}
	
	inline void ResetRate(float rate)
	{
		m_TimeBetweenEvents = 1.0f / rate;
	}

	inline bool NextEvent(float &curDelta)
	{
		// If this goes off, you didn't call Init().
		Assert( m_TimeBetweenEvents != -1 );

		if(curDelta >= m_fNextEvent)
		{
			curDelta -= m_fNextEvent;
			
			m_fNextEvent = m_TimeBetweenEvents;
			return true;
		}
		else
		{
			m_fNextEvent -= curDelta;
			return false;
		}
	}

private:
	float		m_TimeBetweenEvents;
	float		m_fNextEvent;	// When the next event should be triggered.
};

#endif // TIMEDEVENT_H
