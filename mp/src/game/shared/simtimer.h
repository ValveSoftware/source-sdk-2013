//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef SIMTIMER_H
#define SIMTIMER_H

#if defined( _WIN32 )
#pragma once
#endif

#define ST_EPS 0.001

#define DEFINE_SIMTIMER( type, name ) 	DEFINE_EMBEDDED( type, name )

//-----------------------------------------------------------------------------

class CSimpleSimTimer
{
public:
	CSimpleSimTimer()
	 : m_next( -1 )
	{ 
	}

	void Force()
	{
		m_next = -1;
	}

	bool Expired() const
	{
		return ( gpGlobals->curtime - m_next > -ST_EPS );
	}

	float Delay( float delayTime )
	{
		return (m_next += delayTime);
	}
	
	float GetNext() const
	{
		return m_next;
	}

	void Set( float interval )
	{
		m_next = gpGlobals->curtime + interval;
	}

	void Set( float minInterval, float maxInterval )
	{ 
		if ( maxInterval > 0.0 )
			m_next = gpGlobals->curtime + random->RandomFloat( minInterval, maxInterval );
		else
			m_next = gpGlobals->curtime + minInterval;
	}

	float GetRemaining() const
	{
		float result = m_next - gpGlobals->curtime;
		if (result < 0 )
			return 0;
		return result;
	}

	DECLARE_SIMPLE_DATADESC();
	
protected:
	float m_next;
};

//-----------------------------------------------------------------------------

class CSimTimer : public CSimpleSimTimer
{
public:
	CSimTimer( float interval = 0.0, bool startExpired = true )	
	{ 
		Set( interval, startExpired );
	}
	
	void Set( float interval, bool startExpired = true )
	{ 
		m_interval = interval;
		m_next = (startExpired) ? -1.0 : gpGlobals->curtime + m_interval;
	}

	void Reset( float interval = -1.0 )
	{
		if ( interval == -1.0 )
		{
			m_next = gpGlobals->curtime + m_interval;
		}
		else
		{
			m_next = gpGlobals->curtime + interval;
		}
	}

	float GetInterval() const
	{
		return m_interval;
	}

	DECLARE_SIMPLE_DATADESC();
	
private:
	float m_interval;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

class CRandSimTimer : public CSimpleSimTimer
{
public:
	CRandSimTimer( float minInterval = 0.0, float maxInterval = 0.0, bool startExpired = true )	
	{ 
		Set( minInterval, maxInterval, startExpired );
	}
	
	void Set( float minInterval, float maxInterval = 0.0, bool startExpired = true )
	{ 
		m_minInterval = minInterval;
		m_maxInterval = maxInterval;
		
		if (startExpired)
		{
			m_next = -1;
		}
		else
		{
			if ( m_maxInterval == 0 )
				m_next = gpGlobals->curtime + m_minInterval;
			else
				m_next = gpGlobals->curtime + random->RandomFloat( m_minInterval, m_maxInterval );
		}
	}

	void Reset()
	{
		if ( m_maxInterval == 0 )
			m_next = gpGlobals->curtime + m_minInterval;
		else
			m_next = gpGlobals->curtime + random->RandomFloat( m_minInterval, m_maxInterval );
	}

	float GetMinInterval() const
	{
		return m_minInterval;
	}

	float GetMaxInterval() const
	{
		return m_maxInterval;
	}

	DECLARE_SIMPLE_DATADESC();
	
private:
	float m_minInterval;
	float m_maxInterval;
};

//-----------------------------------------------------------------------------

class CStopwatchBase  : public CSimpleSimTimer
{
public:
	CStopwatchBase()	
	{ 
		m_fIsRunning = false;
	}

	bool IsRunning() const
	{
		return m_fIsRunning;
	}
	
	void Stop()
	{
		m_fIsRunning = false;
	}

	bool Expired() const
	{
		return ( m_fIsRunning && CSimpleSimTimer::Expired() );
	}
	
	DECLARE_SIMPLE_DATADESC();
	
protected:
	bool m_fIsRunning;
	
};

//-------------------------------------
class CSimpleStopwatch  : public CStopwatchBase
{
public:
	void Start( float minCountdown, float maxCountdown = 0.0 )
	{ 
		m_fIsRunning = true;
		CSimpleSimTimer::Set( minCountdown, maxCountdown );
	}

	void Stop()
	{
		m_fIsRunning = false;
	}

	bool Expired() const
	{
		return ( m_fIsRunning && CSimpleSimTimer::Expired() );
	}
};
//-------------------------------------

class CStopwatch : public CStopwatchBase
{
public:
	CStopwatch ( float interval = 0.0 )
	{ 
		Set( interval );
	}
	
	void Set( float interval )
	{ 
		m_interval = interval;
	}

	void Start( float intervalOverride )
	{ 
		m_fIsRunning = true;
		m_next = gpGlobals->curtime + intervalOverride;
	}

	void Start()
	{
		Start( m_interval );
	}
	
	float GetInterval() const
	{
		return m_interval;
	}

	DECLARE_SIMPLE_DATADESC();
	
private:
	float m_interval;
};

//-------------------------------------

class CRandStopwatch : public CStopwatchBase
{
public:
	CRandStopwatch( float minInterval = 0.0, float maxInterval = 0.0 )	
	{ 
		Set( minInterval, maxInterval );
	}
	
	void Set( float minInterval, float maxInterval = 0.0 )
	{ 
		m_minInterval = minInterval;
		m_maxInterval = maxInterval;
	}

	void Start( float minOverride, float maxOverride = 0.0 )
	{ 
		m_fIsRunning = true;
		if ( maxOverride == 0 )
			m_next = gpGlobals->curtime + minOverride;
		else
			m_next = gpGlobals->curtime + random->RandomFloat( minOverride, maxOverride );
	}

	void Start()
	{
		Start( m_minInterval, m_maxInterval );
	}
	
	float GetInterval() const
	{
		return m_minInterval;
	}

	float GetMinInterval() const
	{
		return m_minInterval;
	}

	float GetMaxInterval() const
	{
		return m_maxInterval;
	}

	DECLARE_SIMPLE_DATADESC();
	
private:
	float m_minInterval;
	float m_maxInterval;
};

//-----------------------------------------------------------------------------

class CThinkOnceSemaphore
{
public:
	CThinkOnceSemaphore()
	 :	m_lastTime( -1 )
	{
	}

	bool EnterThink()
	{
		if ( m_lastTime == gpGlobals->curtime )
			return false;
		m_lastTime = gpGlobals->curtime;
		return true;
	}

	bool DidThink() const
	{
		return ( gpGlobals->curtime == m_lastTime );

	}

	void SetDidThink()
	{
		m_lastTime = gpGlobals->curtime;
	}

private:
	float m_lastTime;
};

//-----------------------------------------------------------------------------

#endif // SIMTIMER_H
