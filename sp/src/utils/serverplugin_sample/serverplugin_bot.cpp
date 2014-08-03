//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Basic BOT handling.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "interface.h"
#include "filesystem.h"
#undef VECTOR_NO_SLOW_OPERATIONS
#include "mathlib/vector.h"

#include "eiface.h"
#include "edict.h"
#include "game/server/iplayerinfo.h"
#include "igameevents.h"
#include "convar.h"
#include "vstdlib/random.h"
#include "../../game/shared/in_buttons.h"
#include "../../game/shared/shareddefs.h"
//#include "../../game_shared/util_shared.h"
#include "engine/IEngineTrace.h"

extern IBotManager *botmanager; 
extern IUniformRandomStream *randomStr;
extern IPlayerInfoManager *playerinfomanager; 
extern IVEngineServer	*engine; 
extern IEngineTrace *enginetrace;
extern IPlayerInfoManager *playerinfomanager; // game dll interface to interact with players
extern IServerPluginHelpers *helpers; // special 3rd party plugin helpers from the engine

extern CGlobalVars *gpGlobals;

ConVar bot_forcefireweapon( "plugin_bot_forcefireweapon", "", 0, "Force bots with the specified weapon to fire." );
ConVar bot_forceattack2( "plugin_bot_forceattack2", "0", 0, "When firing, use attack2." );
ConVar bot_forceattackon( "plugin_bot_forceattackon", "0", 0, "When firing, don't tap fire, hold it down." );
ConVar bot_flipout( "plugin_bot_flipout", "0", 0, "When on, all bots fire their guns." );
ConVar bot_changeclass( "plugin_bot_changeclass", "0", 0, "Force all bots to change to the specified class." );
static ConVar bot_mimic( "plugin_bot_mimic", "0", 0, "Bot uses usercmd of player by index." );
static ConVar bot_mimic_yaw_offset( "plugin_bot_mimic_yaw_offset", "0", 0, "Offsets the bot yaw." );

ConVar bot_sendcmd( "plugin_bot_sendcmd", "", 0, "Forces bots to send the specified command." );
ConVar bot_crouch( "plugin_bot_crouch", "0", 0, "Bot crouches" );


// This is our bot class.
class CPluginBot
{
public:
	CPluginBot() :
		m_bBackwards(0),
		m_flNextTurnTime(0),
		m_bLastTurnToRight(0),
		m_flNextStrafeTime(0),
		m_flSideMove(0),
		m_ForwardAngle(),
		m_LastAngles()
		{
		}

	bool			m_bBackwards;

	float			m_flNextTurnTime;
	bool			m_bLastTurnToRight;

	float			m_flNextStrafeTime;
	float			m_flSideMove;

	QAngle			m_ForwardAngle;
	QAngle			m_LastAngles;

	IBotController	*m_BotInterface;
	IPlayerInfo		*m_PlayerInfo;
	edict_t			*m_BotEdict;
};

CUtlVector<CPluginBot> s_Bots;

void Bot_Think( CPluginBot *pBot );

// Handler for the "bot" command.
void BotAdd_f()
{
	if ( !botmanager )
		return;

	static int s_BotNum = 0;
	char botName[64];
	Q_snprintf( botName, sizeof(botName), "Bot_%i", s_BotNum );
	s_BotNum++;

	edict_t *botEdict = botmanager->CreateBot( botName );
	if ( botEdict )
	{
		int botIndex = s_Bots.AddToTail();
		CPluginBot & bot = s_Bots[ botIndex ];
		bot.m_BotInterface = botmanager->GetBotController( botEdict );
		bot.m_PlayerInfo = playerinfomanager->GetPlayerInfo( botEdict );
		bot.m_BotEdict = botEdict;
		Assert( bot.m_BotInterface );
	}
}

ConCommand cc_Bot( "plugin_bot_add", BotAdd_f, "Add a bot." );


//-----------------------------------------------------------------------------
// Purpose: Run through all the Bots in the game and let them think.
//-----------------------------------------------------------------------------
void Bot_RunAll( void )
{
	if ( !botmanager )
		return;

	for ( int i = 0; i < s_Bots.Count(); i++ )
	{
		CPluginBot & bot = s_Bots[i];
		if ( bot.m_BotEdict->IsFree() || !bot.m_BotEdict->GetUnknown()|| !bot.m_PlayerInfo->IsConnected() )
		{
			s_Bots.Remove(i);
			--i;
		}
		else
		{
			Bot_Think( &bot );
		}
	}
}

bool Bot_RunMimicCommand( CBotCmd& cmd )
{
	if ( bot_mimic.GetInt() <= 0 )
		return false;

	if ( bot_mimic.GetInt() > gpGlobals->maxClients )
		return false;

	IPlayerInfo *playerInfo = playerinfomanager->GetPlayerInfo( engine->PEntityOfEntIndex( bot_mimic.GetInt() ) ); 
	if ( !playerInfo )
		return false;

	cmd = playerInfo->GetLastUserCommand();
	cmd.viewangles[YAW] += bot_mimic_yaw_offset.GetFloat();

	if( bot_crouch.GetInt() )
		cmd.buttons |= IN_DUCK;

	return true;
}

void Bot_UpdateStrafing( CPluginBot *pBot, CBotCmd &cmd )
{
	if ( gpGlobals->curtime >= pBot->m_flNextStrafeTime )
	{
		pBot->m_flNextStrafeTime = gpGlobals->curtime + 1.0f;

		if ( randomStr->RandomInt( 0, 5 ) == 0 )
		{
			pBot->m_flSideMove = -600.0f + 1200.0f * randomStr->RandomFloat( 0, 2 );
		}
		else
		{
			pBot->m_flSideMove = 0;
		}
		cmd.sidemove = pBot->m_flSideMove;

		if ( randomStr->RandomInt( 0, 20 ) == 0 )
		{
			pBot->m_bBackwards = true;
		}
		else
		{
			pBot->m_bBackwards = false;
		}
	}
}

void Bot_UpdateDirection( CPluginBot *pBot )
{
	float angledelta = 15.0;

	int maxtries = (int)360.0/angledelta;

	if ( pBot->m_bLastTurnToRight )
	{
		angledelta = -angledelta;
	}

	QAngle angle( pBot->m_BotInterface->GetLocalAngles() );

	trace_t trace;
	Vector vecSrc, vecEnd, forward;
	while ( --maxtries >= 0 )
	{
		AngleVectors( angle, &forward );

		vecSrc =  pBot->m_BotInterface->GetLocalOrigin() + Vector( 0, 0, 36 ); 
		vecEnd = vecSrc + forward * 10;

		Ray_t ray;
		ray.Init( vecSrc, vecEnd, 	Vector(-16, -16, 0 ), Vector( 16,  16,  72 ) );
		CTraceFilterWorldAndPropsOnly traceFilter;
		enginetrace->TraceRay( ray, MASK_PLAYERSOLID, &traceFilter, &trace );

		if ( trace.fraction == 1.0 )
		{
			if ( gpGlobals->curtime < pBot->m_flNextTurnTime )
			{
				break;
			}
		}

		angle.y += angledelta;

		if ( angle.y > 180 )
			angle.y -= 360;
		else if ( angle.y < -180 )
			angle.y += 360;

		pBot->m_flNextTurnTime = gpGlobals->curtime + 2.0;
		pBot->m_bLastTurnToRight = randomStr->RandomInt( 0, 1 ) == 0 ? true : false;

		pBot->m_ForwardAngle = angle;
		pBot->m_LastAngles = angle;
	}
	
	pBot->m_BotInterface->SetLocalAngles( angle );
}


void Bot_FlipOut( CPluginBot *pBot, CBotCmd &cmd )
{
	if ( bot_flipout.GetInt() > 0 && !pBot->m_PlayerInfo->IsDead() )
	{
		if ( bot_forceattackon.GetBool() || (RandomFloat(0.0,1.0) > 0.5) )
		{
			cmd.buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
		}

		if ( bot_flipout.GetInt() >= 2 )
		{
			QAngle angOffset = RandomAngle( -1, 1 );

			pBot->m_LastAngles += angOffset;

			for ( int i = 0 ; i < 2; i++ )
			{
				if ( fabs( pBot->m_LastAngles[ i ] - pBot->m_ForwardAngle[ i ] ) > 15.0f )
				{
					if ( pBot->m_LastAngles[ i ] > pBot->m_ForwardAngle[ i ] )
					{
						pBot->m_LastAngles[ i ] = pBot->m_ForwardAngle[ i ] + 15;
					}
					else
					{
						pBot->m_LastAngles[ i ] = pBot->m_ForwardAngle[ i ] - 15;
					}
				}
			}

			pBot->m_LastAngles[ 2 ] = 0;

			pBot->m_BotInterface->SetLocalAngles( pBot->m_LastAngles );
		}
	}
}


void Bot_HandleSendCmd( CPluginBot *pBot )
{
	if ( strlen( bot_sendcmd.GetString() ) > 0 )
	{
		//send the cmd from this bot
		helpers->ClientCommand( pBot->m_BotEdict, bot_sendcmd.GetString() );

		bot_sendcmd.SetValue("");
	}
}


// If bots are being forced to fire a weapon, see if I have it
void Bot_ForceFireWeapon( CPluginBot *pBot, CBotCmd &cmd )
{
	if ( Q_strlen( bot_forcefireweapon.GetString() ) > 0 )
	{
		pBot->m_BotInterface->SetActiveWeapon( bot_forcefireweapon.GetString() );
		bot_forcefireweapon.SetValue( "" );
		// Start firing
		// Some weapons require releases, so randomise firing
		if ( bot_forceattackon.GetBool() || (RandomFloat(0.0,1.0) > 0.5) )
		{
			cmd.buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
		}
	}
}


void Bot_SetForwardMovement( CPluginBot *pBot, CBotCmd &cmd )
{
	if ( !pBot->m_BotInterface->IsEFlagSet(EFL_BOT_FROZEN) )
	{
		if ( pBot->m_PlayerInfo->GetHealth() == 100 )
		{
			cmd.forwardmove = 600 * ( pBot->m_bBackwards ? -1 : 1 );
			if ( pBot->m_flSideMove != 0.0f )
			{
				cmd.forwardmove *= randomStr->RandomFloat( 0.1, 1.0f );
			}
		}
		else
		{
			// Stop when shot
			cmd.forwardmove = 0;
		}
	}
}


void Bot_HandleRespawn( CPluginBot *pBot, CBotCmd &cmd )
{
	// Wait for Reinforcement wave
	if ( pBot->m_PlayerInfo->IsDead() )
	{
		if ( pBot->m_PlayerInfo->GetTeamIndex() == 0 )
		{
			helpers->ClientCommand( pBot->m_BotEdict, "joingame" );
			helpers->ClientCommand( pBot->m_BotEdict, "jointeam 3" );
			helpers->ClientCommand( pBot->m_BotEdict, "joinclass 0" );
		}
	}
}


//-----------------------------------------------------------------------------
// Run this Bot's AI for one frame.
//-----------------------------------------------------------------------------
void Bot_Think( CPluginBot *pBot )
{
	CBotCmd cmd;
	Q_memset( &cmd, 0, sizeof( cmd ) );
	
	// Finally, override all this stuff if the bot is being forced to mimic a player.
	if ( !Bot_RunMimicCommand( cmd ) )
	{
		cmd.sidemove = pBot->m_flSideMove;

		if ( !pBot->m_PlayerInfo->IsDead() )
		{
			Bot_SetForwardMovement( pBot, cmd );

			// Only turn if I haven't been hurt
			if ( !pBot->m_BotInterface->IsEFlagSet(EFL_BOT_FROZEN) && pBot->m_PlayerInfo->GetHealth() == 100 )
			{
				Bot_UpdateDirection( pBot );
				Bot_UpdateStrafing( pBot, cmd );
			}

			// Handle console settings.
			Bot_ForceFireWeapon( pBot, cmd );
			Bot_HandleSendCmd( pBot );
		}
		else
		{
			Bot_HandleRespawn( pBot, cmd );
		}

		Bot_FlipOut( pBot, cmd );

		cmd.viewangles = pBot->m_BotInterface->GetLocalAngles();
		cmd.upmove = 0;
		cmd.impulse = 0;
	}

	pBot->m_BotInterface->RunPlayerMove( &cmd );
}


