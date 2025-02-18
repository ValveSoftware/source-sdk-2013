//========= Copyright Valve Corporation, All rights reserved. ============//
// headless_hatman_dying.h
// The HHH in the process of dying
// Michael Booth, October 2010

#ifndef HEADLESS_HATMAN_DYING_H
#define HEADLESS_HATMAN_DYING_H


//---------------------------------------------------------------------------------------------
class CHeadlessHatmanDying : public Action< CHeadlessHatman >
{
public:
	virtual ActionResult< CHeadlessHatman >	OnStart( CHeadlessHatman *me, Action< CHeadlessHatman > *priorAction );
	virtual ActionResult< CHeadlessHatman >	Update( CHeadlessHatman *me, float interval );
	virtual const char *GetName( void ) const	{ return "Dying"; }		// return name of this action
};


#endif // HEADLESS_HATMAN_DYING_H
