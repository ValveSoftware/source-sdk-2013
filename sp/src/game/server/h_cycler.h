//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef H_CYCLER_H
#define H_CYCLER_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCycler : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CCycler, CAI_BaseNPC );

	void GenericCyclerSpawn(char *szModel, Vector vecMin, Vector vecMax);
	virtual int	ObjectCaps( void ) { return (BaseClass::ObjectCaps() | FCAP_IMPULSE_USE); }
	int OnTakeDamage( const CTakeDamageInfo &info );
	void Spawn( void );
	void Precache( void );
	void Think( void );
	//void Pain( float flDamage );
	void Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	// Don't treat as a live target
	virtual bool IsAlive( void ) { return false; }

	// Inputs
	void	InputSetSequence( inputdata_t &inputdata );

	DECLARE_DATADESC();

	int			m_animate;
};

#endif // H_CYCLER_H
