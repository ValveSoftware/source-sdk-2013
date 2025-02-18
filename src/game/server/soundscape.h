//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SOUNDSCAPE_H
#define SOUNDSCAPE_H
#ifdef _WIN32
#pragma once
#endif

class CEnvSoundscape;

struct ss_update_t
{
	CBasePlayer *pPlayer;
	CEnvSoundscape	*pCurrentSoundscape;
	Vector		playerPosition;
	float		currentDistance;
	int			traceCount;
	bool		bInRange;
};

class CEnvSoundscape : public CServerOnlyPointEntity
{
public:
	DECLARE_CLASS( CEnvSoundscape, CServerOnlyPointEntity );
	DECLARE_DATADESC();

	CEnvSoundscape();
	~CEnvSoundscape();

	bool KeyValue( const char *szKeyName, const char *szValue );
	void Spawn( void );
	void Precache( void );
	void UpdateForPlayer( ss_update_t &update );
	void WriteAudioParamsTo( audioparams_t &audio );
	bool InRangeOfPlayer( CBasePlayer *pPlayer );
	void DrawDebugGeometryOverlays( void );

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggleEnabled( inputdata_t &inputdata );

	string_t GetSoundscapeName() const {return m_soundscapeName;}


private:

	bool IsEnabled( void ) const;
	void Disable( void );
	void Enable( void );


public:
	COutputEvent	m_OnPlay;
	float	m_flRadius;
	string_t m_soundscapeName;
	int		m_soundscapeIndex;
	int		m_soundscapeEntityId;
	string_t m_positionNames[NUM_AUDIO_LOCAL_SOUNDS];
	
	// If this is set, then this soundscape ignores all its parameters and uses
	// those of this soundscape.
	CHandle<CEnvSoundscape> m_hProxySoundscape;


private:

	bool	m_bDisabled;
};


class CEnvSoundscapeProxy : public CEnvSoundscape
{
public:
	DECLARE_CLASS( CEnvSoundscapeProxy, CEnvSoundscape );
	DECLARE_DATADESC();

	CEnvSoundscapeProxy();
	virtual void Activate();

	// Here just to stop it falling back to CEnvSoundscape's, and
	// printing bogus errors about missing soundscapes.
	virtual void Precache() { return; }

private:
	string_t m_MainSoundscapeName;
};


class CEnvSoundscapeTriggerable : public CEnvSoundscape
{
friend class CTriggerSoundscape;

public:
	DECLARE_CLASS( CEnvSoundscapeTriggerable, CEnvSoundscape );
	DECLARE_DATADESC();

	CEnvSoundscapeTriggerable();
	
	// Overrides the base class's think and prevents it from running at all.
	virtual void Think();


private:

	// Passed through from CTriggerSoundscape.
	void DelegateStartTouch( CBaseEntity *pEnt );
	void DelegateEndTouch( CBaseEntity *pEnt );
};


#endif // SOUNDSCAPE_H
