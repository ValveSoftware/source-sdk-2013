//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_taunt.h
// Stand still and play a taunt animation
// Michael Booth, November 2009

#ifndef TF_BOT_TAUNT_H
#define TF_BOT_TAUNT_H

//-----------------------------------------------------------------------------
class CTFBotTaunt : public Action< CTFBot >
{
public:
	CTFBotTaunt( CTFPlayer *partner );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "Taunt"; };

private:
	CHandle< CTFPlayer > m_partner;
	CountdownTimer m_tauntTimer;
	CountdownTimer m_tauntEndTimer;
	CountdownTimer m_tauntStopCheckTimer;
	bool m_didTaunt;
	float m_currTauntTurnSpeed;
	float m_targetTauntYaw;
	bool m_hasTauntYaw;
};


#endif // TF_BOT_TAUNT_H
