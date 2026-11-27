//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_freeze_input.cpp
// Don't do anything until the TF_COND_FREEZE_INPUT condition expires
// Community-contributed, June 2025

#include "cbase.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_freeze_input.h"


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotFreezeInput::Update( CTFBot *me, float interval )
{
	if ( !me->m_Shared.InCond( TF_COND_FREEZE_INPUT ) )
	{
		return Done( "No longer frozen by TF_COND" );
	}

	return Continue();
}
