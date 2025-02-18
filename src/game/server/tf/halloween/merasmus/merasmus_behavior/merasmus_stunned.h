//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef TF_MERASMUS_STUNNED_H
#define TF_MERASMUS_STUNNED_H

class CMerasmusStunned : public Action< CMerasmus >
{
public:
	virtual ActionResult< CMerasmus > OnStart( CMerasmus *me, Action< CMerasmus > *priorAction );
	virtual ActionResult< CMerasmus > Update( CMerasmus *me, float interval );
	virtual void OnEnd( CMerasmus *me, Action< CMerasmus > *nextAction );

	virtual const char *GetName( void ) const	{ return "Stunned!"; }		// return name of this action
private:
	enum StunStage_t
	{
		STUN_BEGIN,
		STUN_MID,
		STUN_END
	};
	StunStage_t m_nStunStage;
	CountdownTimer m_stunAnimationTimer;
	CountdownTimer m_stunFinishTimer;
};

#endif //TF_MERASMUS_STUNNED_H
