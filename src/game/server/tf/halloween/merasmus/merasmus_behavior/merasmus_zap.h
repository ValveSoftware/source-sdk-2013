//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef TF_MERASMUS_ZAP_H
#define TF_MERASMUS_ZAP_H

#include "tf_gamerules.h"

class CMerasmusZap : public Action< CMerasmus >
{
public:
	virtual ActionResult< CMerasmus > OnStart( CMerasmus *me, Action< CMerasmus > *priorAction );
	virtual ActionResult< CMerasmus > Update( CMerasmus *me, float interval );

	virtual const char *GetName( void ) const	{ return "Zap!"; }		// return name of this action
private:
	enum SpellType_t
	{
		SPELL_FIRE,
		SPELL_LAUNCH,

		SPELL_COUNT
	};
	SpellType_t m_spellType;
	void PlayCastSound( CMerasmus* me ) const;

	CountdownTimer m_zapTimer;
};

#endif //TF_MERASMUS_ZAP_H
