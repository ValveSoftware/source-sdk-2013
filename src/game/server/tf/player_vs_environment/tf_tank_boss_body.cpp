//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#include "NextBot.h"
#include "tf_tank_boss.h"
#include "tf_tank_boss_body.h"


//-------------------------------------------------------------------------------------------
CTFTankBossBody::CTFTankBossBody( INextBot *bot ) : IBody( bot )
{
}


//-------------------------------------------------------------------------------------------
bool CTFTankBossBody::StartSequence( const char *name )
{
	CTFTankBoss *me = (CTFTankBoss *)GetBot()->GetEntity();

	int animSequence = me->LookupSequence( name );

	if ( animSequence )
	{
		me->SetSequence( animSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0 );
		me->ResetSequenceInfo();

		return true;
	}

	return false;
}

void CTFTankBossBody::SetSkin( int nSkin )
{
	CTFTankBoss *me = (CTFTankBoss *)GetBot()->GetEntity();
	me->m_nSkin = nSkin;
}


//-------------------------------------------------------------------------------------------
void CTFTankBossBody::Update( void )
{
	CTFTankBoss *me = (CTFTankBoss *)GetBot()->GetEntity();

	// move the animation ahead in time	
	me->StudioFrameAdvance();
	me->DispatchAnimEvents( me );
}


//---------------------------------------------------------------------------------------------
// return the bot's collision mask (hack until we get a general hull trace abstraction here or in the locomotion interface)
unsigned int CTFTankBossBody::GetSolidMask( void ) const
{
	return CONTENTS_SOLID;
}
