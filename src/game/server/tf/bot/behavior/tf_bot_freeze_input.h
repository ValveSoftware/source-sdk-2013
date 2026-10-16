//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_freeze_input.h
// Don't do anything until the TF_COND_FREEZE_INPUT condition expires
// Community-contributed, June 2025

#ifndef TF_BOT_FREEZE_INPUT_H
#define TF_BOT_FREEZE_INPUT_H


//-----------------------------------------------------------------------------
class CTFBotFreezeInput : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "FreezeInput"; };
};


#endif // TF_BOT_FREEZE_INPUT_H
