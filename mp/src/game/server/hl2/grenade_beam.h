//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Projectile shot by mortar synth
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	GRENADEBEAM_H
#define	GRENADEBEAM_H

#include "basegrenade_shared.h"

#define GRENADEBEAM_MAXBEAMS 2
#define GRENADEBEAM_MAXHITS  GRENADEBEAM_MAXBEAMS-1

class CGrenadeBeam;
class CBeam;

// End of the grenade beam
class CGrenadeBeamChaser : public CBaseAnimating
{
public:
	DECLARE_CLASS( CGrenadeBeamChaser, CBaseAnimating );
	DECLARE_DATADESC();

	static CGrenadeBeamChaser* ChaserCreate( CGrenadeBeam *pTarget );

	void			Spawn( void );
	void 			ChaserThink();

	CGrenadeBeam*	m_pTarget;
};

class CGrenadeBeam : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenadeBeam, CBaseGrenade );
	DECLARE_DATADESC();

	static CGrenadeBeam* Create( CBaseEntity* pOwner, const Vector &vStart);

public:
	void		Spawn( void );
	void		Precache( void );
	void		Format( color32 clrColor, float flWidth);
	void 		GrenadeBeamTouch( CBaseEntity *pOther );
	void 		KillBeam();
	void		CreateBeams(void);
	void		UpdateBeams(void);
	//void		DebugBeams(void);
	void		GetChaserTargetPos(Vector *vPosition);
	void		GetNextTargetPos(Vector *vPosition);
	int			UpdateTransmitState(void);
	void		Shoot(Vector vDirection, float flSpeed, float flLifetime, float flLag, float flDamage );

	Vector		m_vLaunchPos;
	float		m_flBeamWidth;
	float		m_flBeamSpeed;
	float		m_flBeamLag;
	float		m_flLaunchTime;
	float		m_flLastTouchTime;
	EHANDLE		m_hBeamChaser;
	int			m_nNumHits;
	Vector		m_pHitLocation[GRENADEBEAM_MAXHITS];
	CBeam*		m_pBeam[GRENADEBEAM_MAXBEAMS];
};

#endif	//GRENADEBEAM_H
