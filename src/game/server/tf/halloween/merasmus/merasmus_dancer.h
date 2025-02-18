//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Teleport vortex for the Eyeball Boss
//
//=============================================================================//
#ifndef MERASMUS_DANCE_H
#define MERASMUS_DANCE_H

#ifdef _WIN32
#pragma once
#endif

#include "baseanimating.h"

//=============================================================================
//
// Non-AI version of Merasmus that can be spawned during the dance spell.
//
class CMerasmusDancer : public CBaseAnimating
{
	DECLARE_CLASS( CMerasmusDancer, CBaseAnimating );
	DECLARE_SERVERCLASS();

public:
					CMerasmusDancer();
	virtual			~CMerasmusDancer();

	void			Dance();
	void			Vanish();
	void			BlastOff();

private:
	virtual void	Spawn();
	virtual void	Precache();

	void			HideStaff();
	void			PlaySequence( const char *pSeqName );
	void			PlayActivity( int iActivity );
	void			DanceThink();

	bool			ShouldDelete() const;

	bool			m_bEmitParticleEffect;
	CountdownTimer	m_DieCountdownTimer;
};

#endif // MERASMUS_DANCE_H


