//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef C_HEADLESS_HATMAN_H
#define C_HEADLESS_HATMAN_H

#include "c_ai_basenpc.h"


//--------------------------------------------------------------------------------------------------------
/**
 * The client-side implementation of the Dark Knight
 */
class C_HeadlessHatman : public C_NextBotCombatCharacter
{
public:
	DECLARE_CLASS( C_HeadlessHatman, C_NextBotCombatCharacter );
	DECLARE_CLIENTCLASS();

	C_HeadlessHatman();
	virtual ~C_HeadlessHatman();

public:	
	virtual void Spawn( void );
	virtual bool IsNextBot() { return true; }

	virtual Vector GetObserverCamOrigin( void );	// Return the origin for player observers tracking this target

	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	virtual void					ClientThink();

private:
	C_HeadlessHatman( const C_HeadlessHatman & );				// not defined, not accessible

	HPARTICLEFFECT		m_ghostEffect;
	HPARTICLEFFECT		m_leftEyeEffect;
	HPARTICLEFFECT		m_rightEyeEffect;
};

#endif // C_HEADLESS_HATMAN_H
