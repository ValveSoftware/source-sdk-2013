//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef MERASMUS_EMERGE_H
#define MERASMUS_EMERGE_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CMerasmusReveal : public Action< CMerasmus >
{
public:
	virtual ActionResult< CMerasmus >	OnStart( CMerasmus *me, Action< CMerasmus > *priorAction );
	virtual ActionResult< CMerasmus >	Update( CMerasmus *me, float interval );
	virtual const char *GetName( void ) const	{ return "Reveal"; }		// return name of this action
};


#endif // MERASMUS_EMERGE_H
