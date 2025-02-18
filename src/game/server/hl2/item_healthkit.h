//========= Copyright Valve Corporation, All rights reserved. ============//

#pragma once

//-----------------------------------------------------------------------------
// Wall mounted health kit. Heals the player when used.
//-----------------------------------------------------------------------------
class CNewWallHealth : public CBaseAnimating
{
public:
	DECLARE_CLASS( CNewWallHealth, CBaseAnimating );

	void Spawn( );
	void Precache( void );
	int  DrawDebugTextOverlays(void);
	bool CreateVPhysics(void);
	void Off(void);
	void Recharge(void);
	bool KeyValue(  const char *szKeyName, const char *szValue );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() | m_iCaps; }

    int GetJuice() const { return m_iJuice; }

	float m_flNextCharge; 
	int		m_iReactivate ; // DeathMatch Delay until reactvated
	int		m_iJuice;
	int		m_iOn;			// 0 = off, 1 = startup, 2 = going
	float   m_flSoundTime;

	int		m_nState;
	int		m_iCaps;

	COutputFloat m_OutRemainingHealth;
	COutputEvent m_OnPlayerUse;

	void StudioFrameAdvance ( void );

	float m_flJuice;

	DECLARE_DATADESC();
};

//-----------------------------------------------------------------------------
// Wall mounted health kit. Heals the player when used.
//-----------------------------------------------------------------------------
class CWallHealth : public CBaseToggle
{
public:
	DECLARE_CLASS( CWallHealth, CBaseToggle );

	void Spawn( );
	void Precache( void );
	int  DrawDebugTextOverlays(void);
	bool CreateVPhysics(void);
	void Off(void);
	void Recharge(void);
	bool KeyValue(  const char *szKeyName, const char *szValue );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() | m_iCaps; }

    int GetJuice() const { return m_iJuice; }

	float m_flNextCharge; 
	int		m_iReactivate ; // DeathMatch Delay until reactvated
	int		m_iJuice;
	int		m_iOn;			// 0 = off, 1 = startup, 2 = going
	float   m_flSoundTime;

	int		m_nState;
	int		m_iCaps;

	COutputFloat m_OutRemainingHealth;
	COutputEvent m_OnPlayerUse;

	DECLARE_DATADESC();
};
