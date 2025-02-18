//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef MERASMUS_DYING_H
#define MERASMUS_DYING_H


//---------------------------------------------------------------------------------------------
class CMerasmusDying : public Action< CMerasmus >
{
public:
	virtual ActionResult< CMerasmus >	OnStart( CMerasmus *me, Action< CMerasmus > *priorAction );
	virtual ActionResult< CMerasmus >	Update( CMerasmus *me, float interval );
	virtual const char *GetName( void ) const	{ return "Dying"; }		// return name of this action
};


#endif // MERASMUS_DYING_H
