//========= Copyright Valve Corporation, All rights reserved. ============//
// simple_bot.cpp
// A simple bot
// Michael Booth, February 2009

#include "cbase.h"
#include "simple_bot.h"
#include "nav_mesh.h"


//-----------------------------------------------------------------------------------------------------
// Command to add a Simple Bot where your crosshairs are aiming
//-----------------------------------------------------------------------------------------------------
CON_COMMAND_F( simple_bot_add, "Add a simple bot.", FCVAR_CHEAT )
{
	CBasePlayer *player = UTIL_GetCommandClient();
	if ( !player )
	{
		return;
	}

	Vector forward;
	player->EyeVectors( &forward );

	trace_t result;
	UTIL_TraceLine( player->EyePosition(), player->EyePosition() + 999999.9f * forward, MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, player, COLLISION_GROUP_NONE, &result );
	if ( !result.DidHit() )
	{
		return;
	}

	CSimpleBot *bot = static_cast< CSimpleBot * >( CreateEntityByName( "simple_bot" ) );
	if ( bot )
	{
		Vector forward = player->GetAbsOrigin() - result.endpos;
		forward.z = 0.0f;
		forward.NormalizeInPlace();

		QAngle angles;
		VectorAngles( forward, angles );

		bot->SetAbsAngles( angles );
		bot->SetAbsOrigin( result.endpos + Vector( 0, 0, 10.0f ) );

		DispatchSpawn( bot );
	}
}


//-----------------------------------------------------------------------------------------------------
// The Simple Bot
//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( simple_bot, CSimpleBot );

#ifndef TF_DLL
PRECACHE_REGISTER( simple_bot );
#endif


//-----------------------------------------------------------------------------------------------------
CSimpleBot::CSimpleBot()
{
	ALLOCATE_INTENTION_INTERFACE( CSimpleBot );

	m_locomotor = new NextBotGroundLocomotion( this );
}


//-----------------------------------------------------------------------------------------------------
CSimpleBot::~CSimpleBot()
{
	DEALLOCATE_INTENTION_INTERFACE;

	if ( m_locomotor )
		delete m_locomotor;
}

//-----------------------------------------------------------------------------------------------------
void CSimpleBot::Precache()
{
	BaseClass::Precache();

#ifndef DOTA_DLL
	PrecacheModel( "models/humans/group01/female_01.mdl" );
#endif
}


//-----------------------------------------------------------------------------------------------------
void CSimpleBot::Spawn( void )
{
	BaseClass::Spawn();

#ifndef DOTA_DLL
	SetModel( "models/humans/group01/female_01.mdl" );
#endif
}


//---------------------------------------------------------------------------------------------
// The Simple Bot behaviors
//---------------------------------------------------------------------------------------------
/**
 * For use with TheNavMesh->ForAllAreas()
 * Find the Nth area in the sequence
 */
class SelectNthAreaFunctor
{
public:
	SelectNthAreaFunctor( int count )
	{
		m_count = count;
		m_area = NULL;
	}

	bool operator() ( CNavArea *area )
	{
		m_area = area;
		return ( m_count-- > 0 );
	}

	int m_count;
	CNavArea *m_area;
};


//---------------------------------------------------------------------------------------------
/**
 * This action causes the bot to pick a random nav area in the mesh and move to it, then
 * pick another, etc.
 * Actions usually each have their own .cpp/.h file and are organized into folders since there
 * are often many of them. For this example, we're keeping everything to a single .cpp/.h file.
 */
class CSimpleBotRoam : public Action< CSimpleBot >
{
public:
	//----------------------------------------------------------------------------------
	// OnStart is called once when the Action first becomes active
	virtual ActionResult< CSimpleBot >	OnStart( CSimpleBot *me, Action< CSimpleBot > *priorAction )
	{
		// smooth out the bot's path following by moving toward a point farther down the path
		m_path.SetMinLookAheadDistance( 300.0f );

		return Continue();
	}


	//----------------------------------------------------------------------------------
	// Update is called repeatedly (usually once per server frame) while the Action is active
	virtual ActionResult< CSimpleBot >	Update( CSimpleBot *me, float interval )
	{
		if ( m_path.IsValid() && !m_timer.IsElapsed() ) 
		{
			// PathFollower::Update() moves the bot along the path using the bot's ILocomotion and IBody interfaces
			m_path.Update( me );
		}
		else
		{
			SelectNthAreaFunctor pick( RandomInt( 0, TheNavMesh->GetNavAreaCount() - 1 ) );
			TheNavMesh->ForAllAreas( pick );

			if ( pick.m_area )
			{
				CSimpleBotPathCost cost( me );
				m_path.Compute( me, pick.m_area->GetCenter(), cost );
			}

			// follow this path for a random duration (or until we reach the end)
			m_timer.Start( RandomFloat( 5.0f, 10.0f ) );
		}

		return Continue();
	}


	//----------------------------------------------------------------------------------
	// this is an event handler - many more are available (see declaration of Action< Actor > in NextBotBehavior.h)
	virtual EventDesiredResult< CSimpleBot > OnStuck( CSimpleBot *me )
	{
		// we are stuck trying to follow the current path - invalidate it so a new one is chosen
		m_path.Invalidate();

		return TryContinue();
	}


	virtual const char *GetName( void ) const	{ return "Roam"; }		// return name of this action

private:
	PathFollower m_path;
	CountdownTimer m_timer;
};


//---------------------------------------------------------------------------------------------
/**
 * Instantiate the bot's Intention interface and start the initial Action (CSimpleBotRoam in this case)
 */
IMPLEMENT_INTENTION_INTERFACE( CSimpleBot, CSimpleBotRoam )
