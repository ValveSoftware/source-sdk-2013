//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <stdarg.h>
#include "engine/IEngineSound.h"
#include "filesystem.h"
#include "igamemovement.h"
#include "engine/IEngineTrace.h"
#include "engine/ivmodelinfo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CMoveData *g_pMoveData;

class CMoveHelperClient : public IMoveHelper
{
public:
					CMoveHelperClient( void );
	virtual			~CMoveHelperClient( void );

	char const*		GetName( EntityHandle_t handle ) const;

	// touch lists
	virtual void	ResetTouchList( void );
	virtual bool	AddToTouched( const trace_t& tr, const Vector& impactvelocity );
	virtual void	ProcessImpacts( void );

	// Numbered line printf
	virtual void	Con_NPrintf( int idx, char const* fmt, ... );
		
	virtual bool	PlayerFallingDamage(void);
	virtual void	PlayerSetAnimation( PLAYER_ANIM eAnim );

	// These have separate server vs client impementations
	virtual void	StartSound( const Vector& origin, int channel, char const* sample, float volume, soundlevel_t soundlevel, int fFlags, int pitch );
	virtual void	StartSound( const Vector& origin, const char *soundname ); 
	virtual void	PlaybackEventFull( int flags, int clientindex, unsigned short eventindex, float delay, Vector& origin, Vector& angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 );
	virtual IPhysicsSurfaceProps *GetSurfaceProps( void );

	virtual bool IsWorldEntity( const CBaseHandle &handle );

private:
	// results, tallied on client and server, but only used by server to run SV_Impact.
	// we store off our velocity in the trace_t structure so that we can determine results
	// of shoving boxes etc. around.
	struct touchlist_t
	{
		Vector	deltavelocity;
		trace_t trace;

		touchlist_t() {}

	private:
		touchlist_t( const touchlist_t &src );
	};

	CUtlVector<touchlist_t>			m_TouchList;
};	

//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------

IMPLEMENT_MOVEHELPER();

static CMoveHelperClient s_MoveHelperClient;


//-----------------------------------------------------------------------------
// Constructor 
//-----------------------------------------------------------------------------
CMoveHelperClient::CMoveHelperClient( void )
{
	SetSingleton( this );
}

CMoveHelperClient::~CMoveHelperClient( void )
{
	SetSingleton( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
char const* CMoveHelperClient::GetName( EntityHandle_t handle ) const
{
	return "";
}

//-----------------------------------------------------------------------------
// Touch list
//-----------------------------------------------------------------------------

void CMoveHelperClient::ResetTouchList( void )
{
	m_TouchList.RemoveAll();
}

//-----------------------------------------------------------------------------
// Adds to the touched list 
//-----------------------------------------------------------------------------

bool CMoveHelperClient::AddToTouched( const trace_t& tr, const Vector& impactvelocity )
{
	int i;

	// Look for duplicates
	for (i = 0; i < m_TouchList.Size(); i++)
	{
		if (m_TouchList[i].trace.m_pEnt == tr.m_pEnt)
		{
			return false;
		}
	}

	i = m_TouchList.AddToTail();
	m_TouchList[i].trace = tr;
	VectorCopy( impactvelocity, m_TouchList[i].deltavelocity );

	return true;
}

void CMoveHelperClient::ProcessImpacts( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// Relink in order to build absorigin and absmin/max to reflect any changes
	//  from prediction.  Relink will early out on SOLID_NOT

	// TODO: Touch triggers on the client
	//pPlayer->PhysicsTouchTriggers();

	// Don't bother if the player ain't solid
	if ( pPlayer->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
		return;

	// Save off the velocity, cause we need to temporarily reset it
	Vector vel = pPlayer->GetAbsVelocity();

	// Touch other objects that were intersected during the movement.
	for (int i = 0 ; i < m_TouchList.Size(); i++)
	{
		// Run the impact function as if we had run it during movement.
		C_BaseEntity *entity = ClientEntityList().GetEnt( m_TouchList[i].trace.m_pEnt->entindex() );
		if ( !entity )
			continue;

		Assert( entity != pPlayer );
		// Don't ever collide with self!!!!
		if ( entity == pPlayer )
			continue;

		// Reconstruct trace results.
		m_TouchList[i].trace.m_pEnt = entity;

		// Use the velocity we had when we collided, so boxes will move, etc.
		pPlayer->SetAbsVelocity( m_TouchList[i].deltavelocity );

		entity->PhysicsImpact( pPlayer, m_TouchList[i].trace );
	}

	// Restore the velocity
	pPlayer->SetAbsVelocity( vel );

	// So no stuff is ever left over, sigh...
	ResetTouchList();
}

void CMoveHelperClient::StartSound( const Vector& origin, const char *soundname )
{
	if ( !soundname )
		return;

	CLocalPlayerFilter filter;
	filter.UsePredictionRules();
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, soundname, &origin );
}


//-----------------------------------------------------------------------------
// Play a sound
//-----------------------------------------------------------------------------

void CMoveHelperClient::StartSound( const Vector& origin, int channel, 
	char const* pSample, float volume, soundlevel_t soundlevel, int fFlags, int pitch )
{
	if ( pSample )
	{
		C_BaseEntity::PrecacheScriptSound( pSample );
		CLocalPlayerFilter filter;
		filter.UsePredictionRules();

		EmitSound_t ep;
		ep.m_nChannel = channel;
		ep.m_pSoundName =  pSample;
		ep.m_flVolume = volume;
		ep.m_SoundLevel = soundlevel;
		ep.m_nPitch = pitch;
		ep.m_pOrigin = &origin;

		C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, ep );
	}
}

//-----------------------------------------------------------------------------
// Play a event
//-----------------------------------------------------------------------------

void CMoveHelperClient::PlaybackEventFull( int flags, int clientindex, unsigned short eventindex, float delay, Vector& origin, Vector& angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 )
{
	// TODO
	if (g_pMoveData->m_bFirstRunOfFunctions )
	{
	}
}

//-----------------------------------------------------------------------------
// Surface properties interface
//-----------------------------------------------------------------------------

IPhysicsSurfaceProps *CMoveHelperClient::GetSurfaceProps( void )
{
	extern IPhysicsSurfaceProps *physprops;
	return physprops;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bDeveloper - 
//			*pFormat - 
//			... - 
//-----------------------------------------------------------------------------
void CMoveHelperClient::Con_NPrintf( int idx, char const* pFormat, ...)
{
	va_list marker;
	char msg[8192];

	va_start(marker, pFormat);
	Q_vsnprintf(msg, sizeof( msg ), pFormat, marker);
	va_end(marker);
	
#if defined( CSTRIKE_DLL ) || defined( DOD_DLL ) // reltodo
	engine->Con_NPrintf( idx, "%s", msg );
#else
	engine->Con_NPrintf( idx, msg );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called when the player falls onto a surface fast enough to take
//			damage, according to the rules in CGameMovement::CheckFalling.
// Output : Returns true if the player survived the fall, false if they died.
//-----------------------------------------------------------------------------
bool CMoveHelperClient::PlayerFallingDamage(void)
{
	// Do nothing; falling damage is applied in MoveHelper_Server::PlayerFallingDamage.
	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Sets an animation in the player.
// Input  : eAnim - Animation to set.
//-----------------------------------------------------------------------------
void CMoveHelperClient::PlayerSetAnimation( PLAYER_ANIM eAnim )
{
	 // Do nothing on the client. Animations are set on the server.
}

bool CMoveHelperClient::IsWorldEntity( const CBaseHandle &handle )
{
	return handle == cl_entitylist->GetNetworkableHandle( 0 );
}