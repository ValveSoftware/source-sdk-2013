// NextBotBodyInterface.cpp
// Control and information about the bot's body state (posture, animation state, etc)
// Author: Michael Booth, April 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "NextBot.h"
#include "NextBotBodyInterface.h"


void IBody::AimHeadTowards( const Vector &lookAtPos, LookAtPriorityType priority, float duration, INextBotReply *replyWhenAimed, const char *reason )
{
	if ( replyWhenAimed )
	{
		replyWhenAimed->OnFail( GetBot(), INextBotReply::FAILED );
	}
}

void IBody::AimHeadTowards( CBaseEntity *subject, LookAtPriorityType priority, float duration, INextBotReply *replyWhenAimed, const char *reason )
{
	if ( replyWhenAimed )
	{
		replyWhenAimed->OnFail( GetBot(), INextBotReply::FAILED );
	}
}

bool IBody::SetPosition( const Vector &pos )
{
	GetBot()->GetEntity()->SetAbsOrigin( pos );
	return true;
}

const Vector &IBody::GetEyePosition( void ) const
{
	static Vector eye;

	eye = GetBot()->GetEntity()->WorldSpaceCenter();

	return eye;
}

const Vector &IBody::GetViewVector( void ) const
{
	static Vector view;
	
	AngleVectors( GetBot()->GetEntity()->EyeAngles(), &view );

	return view;
}

bool IBody::IsHeadAimingOnTarget( void ) const
{
	return false;
}
