//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef NPC_SecobModportal1_H
#define NPC_SecobModportal1_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"

//=========================================================
//	>> CSecobModportal1
//=========================================================
class CNPC_SecobModportal1 : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_SecobModportal1, CAI_BaseNPC );

public:
	CNPC_SecobModportal1(void);
	~CNPC_SecobModportal1();

	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void Activate( void );
	virtual void OnRestore( void );

	virtual float GetAutoAimRadius() { return m_fAutoaimRadius; }

	Class_T Classify( void );
	void	Event_Killed( const CTakeDamageInfo &info );
	void	DecalTrace( trace_t *pTrace, char const *decalName );
	void	ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName );
	bool	IsLightDamage( const CTakeDamageInfo &info );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	int		OnTakeDamage( const CTakeDamageInfo &info );
	bool	UsePerfectAccuracy( void ) { return m_bPerfectAccuracy; }

	bool	TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr ) { return false; } // force traces to test against hull
	
	void	SecobModportal1Think( void );
	void		Touch( CBaseEntity *pOther );
	bool	CanBecomeRagdoll( void );

	void	SetPainPartner( CBaseEntity *pOther );
	void	InputTargeted( inputdata_t &inputdata );
	void	InputReleased( inputdata_t &inputdata );
	bool	CanBecomeServerRagdoll( void ) { return false;	}

	bool	CanBeAnEnemyOf( CBaseEntity *pEnemy );


protected:

	EHANDLE			m_hPainPartner;	//Entity that the SecobModportal1 will pass any damage it take to
	COutputEvent	m_OnTargeted;
	COutputEvent	m_OnReleased;
	bool			m_bPerfectAccuracy;	// Entities that shoot at me should be perfectly accurate
	float			m_fAutoaimRadius;	// How much to influence player's autoaim.
	float			m_flMinDistValidEnemy;

	DECLARE_DATADESC();
};

int FindSecobModportal1sInCone( CBaseEntity **pList, int listMax, const Vector &coneOrigin, const Vector &coneAxis, float coneAngleCos, float coneLength );

#define SF_SecobModportal1_NONSOLID		(1 << 16)
#define SF_SecobModportal1_NODAMAGE		(1 << 17)
#define	SF_SecobModportal1_ENEMYDAMAGEONLY	(1 << 18)
#define	SF_SecobModportal1_BLEED			(1 << 19)
#define SF_SecobModportal1_PERFECTACC		(1 << 20)
#define SF_SecobModportal1_VPHYSICSSHADOW  (1 << 21)


#endif	// NPC_SecobModportal1_H

