//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef C_MERASMUS_DANCER_H
#define C_MERASMUS_DANCER_H

#include "c_baseanimating.h"

//--------------------------------------------------------------------------------------------------------

class C_MerasmusDancer : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_MerasmusDancer, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_MerasmusDancer();
	virtual ~C_MerasmusDancer();

private:
	C_MerasmusDancer( const C_MerasmusDancer & );				// not defined, not accessible
};

#endif // C_MERASMUS_DANCER_H
