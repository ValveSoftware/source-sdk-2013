//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENV_SPEAKER_H
#define ENV_SPEAKER_H
#ifdef _WIN32
#pragma once
#endif

// ===================================================================================
//
// Speaker class. Used for announcements per level, for door lock/unlock spoken voice. 
//

class CSpeaker : public CPointEntity
{
public:
	DECLARE_CLASS( CSpeaker, CPointEntity );

	void Spawn( void );
	void Precache( void );
	
	DECLARE_DATADESC();

	virtual int	ObjectCaps( void ) { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

	virtual IResponseSystem *GetResponseSystem() { return m_pInstancedResponseSystem; }

	virtual int	Save( ISave &save );
	virtual int	Restore( IRestore &restore );

protected:

	void SpeakerThink( void );

#ifdef MAPBASE
	EHANDLE			m_hTarget;
	virtual void	InputSetTarget( inputdata_t &inputdata ) { BaseClass::InputSetTarget(inputdata); m_hTarget = NULL; }
	CBaseEntity		*GetTarget();
	virtual void	DispatchResponse( const char *conceptName );
#endif

	void InputToggle( inputdata_t &inputdata );

	float	m_delayMin;
	float	m_delayMax;

	string_t	m_iszRuleScriptFile;
	string_t	m_iszConcept;
	IResponseSystem *m_pInstancedResponseSystem;

public:

	void InputTurnOff( inputdata_t &inputdata );
	void InputTurnOn( inputdata_t &inputdata );

#ifdef MAPBASE
	COutputString m_OnSpeak;
#endif
};

#endif // ENV_SPEAKER_H
