//========= Copyright Valve Corporation, All rights reserved. ============//
#pragma once

const int SF_PHURT_START_ON			= 1;
const int SF_PHURT_HURT_UBER		= 2;

class CPointHurt : public CPointEntity
{
	DECLARE_CLASS( CPointHurt, CPointEntity );

public:
	void	Spawn( void );
	void	Precache( void );
	void	HurtThink( void );

	// Input handlers
	void InputTurnOn(inputdata_t &inputdata);
	void InputTurnOff(inputdata_t &inputdata);
	void InputToggle(inputdata_t &inputdata);
	void InputHurt(inputdata_t &inputdata);
	
	DECLARE_DATADESC();

	int			m_nDamage;
	int			m_bitsDamageType;
	float		m_flRadius;
	float		m_flDelay;
	string_t	m_strTarget;
	EHANDLE		m_pActivator;
};
