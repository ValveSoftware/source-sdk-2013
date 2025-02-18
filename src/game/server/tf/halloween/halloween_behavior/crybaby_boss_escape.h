//========= Copyright Valve Corporation, All rights reserved. ============//
// crybaby_boss_escape.h
// Crybaby Boss runs to the pit in Mann Manor to escape
// Michael Booth, October 2011

#ifndef CRYBABY_ESCAPE_H
#define CRYBABY_ESCAPE_H

#include "Path/NextBotPathFollow.h"

class CCryBabyBossEscape : public Action< CHeadlessHatman >
{
public:
	virtual ActionResult< CHeadlessHatman >	OnStart( CHeadlessHatman *me, Action< CHeadlessHatman > *priorAction );
	virtual ActionResult< CHeadlessHatman >	Update( CHeadlessHatman *me, float interval );

	virtual EventDesiredResult< CHeadlessHatman > OnMoveToSuccess( CHeadlessHatman *me, const Path *path );

	virtual const char *GetName( void ) const	{ return "CryBabyBossEscape"; };

private:
	PathFollower m_path;
	CountdownTimer m_repathTimer;
	CountdownTimer m_footfallTimer;
};


#endif // CRYBABY_ESCAPE_H
