//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <stdarg.h>
#include "gamerules.h"
#include "player.h"
#include "model_types.h"
#include "movehelper_server.h"
#include "shake.h"				// For screen fade constants
#include "engine/IEngineSound.h"

//=============================================================================
// HPE_BEGIN
// [dwenger] Necessary for stats tracking
//=============================================================================
#ifdef CSTRIKE_DLL

#include "cs_gamestats.h"
#include "cs_achievement_constants.h"

#endif
//=============================================================================
// HPE_END
//=============================================================================

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IPhysicsCollision *physcollision;


//-----------------------------------------------------------------------------
// Implementation of the movehelper on the server
//-----------------------------------------------------------------------------

class CMoveHelperServer : public IMoveHelperServer
{
public:
	CMoveHelperServer( void );
	virtual ~CMoveHelperServer();

	// Methods associated with a particular entity
	virtual	char const*		GetName( EntityHandle_t handle ) const;

	// Touch list...
	virtual void	ResetTouchList( void );
	virtual bool	AddToTouched( const trace_t &tr, const Vector& impactvelocity );
 	virtual void	ProcessImpacts( void );

	virtual bool	PlayerFallingDamage( void );
	virtual void	PlayerSetAnimation( PLAYER_ANIM eAnim );

	// Numbered line printf
	virtual void	Con_NPrintf( int idx, char const* fmt, ... );
	
	// These have separate server vs client impementations
	virtual void	StartSound( const Vector& origin, int channel, char const* sample, float volume, soundlevel_t soundlevel, int fFlags, int pitch );
	virtual void	StartSound( const Vector& origin, const char *soundname ); 

	virtual void	PlaybackEventFull( int flags, int clientindex, unsigned short eventindex, float delay, Vector& origin, Vector& angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 );
	virtual IPhysicsSurfaceProps *GetSurfaceProps( void );

	void			SetHost( CBasePlayer *host );

	virtual bool IsWorldEntity( const CBaseHandle &handle );

private:
	CBasePlayer*	m_pHostPlayer;

	// results, tallied on client and server, but only used by server to run SV_Impact.
	// we store off our velocity in the trace_t structure so that we can determine results
	// of shoving boxes etc. around.
	struct touchlist_t
	{
		Vector	deltavelocity;
		trace_t trace;
	};

	CUtlVector<touchlist_t>	m_TouchList;
};


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------

IMPLEMENT_MOVEHELPER();

IMoveHelperServer* MoveHelperServer()
{
	static CMoveHelperServer s_MoveHelperServer;
	return &s_MoveHelperServer;
}


//-----------------------------------------------------------------------------
// Converts the entity handle into a edict_t
//-----------------------------------------------------------------------------

static inline edict_t* GetEdict( EntityHandle_t handle )
{
	return gEntList.GetEdict( handle );
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

CMoveHelperServer::CMoveHelperServer( void ) : m_TouchList( 0, 128 )
{
	m_pHostPlayer = 0;
	SetSingleton( this );
}

CMoveHelperServer::~CMoveHelperServer( void )
{
	SetSingleton( 0 );
}

//-----------------------------------------------------------------------------
// Indicates which player we're going to move
//-----------------------------------------------------------------------------

void CMoveHelperServer::SetHost( CBasePlayer *host )
{
	m_pHostPlayer = host;

	// In case any stuff is ever left over, sigh...
	ResetTouchList();
}


//-----------------------------------------------------------------------------
// Returns the name for debugging purposes
//-----------------------------------------------------------------------------
char const* CMoveHelperServer::GetName( EntityHandle_t handle ) const
{
	// This ain't pertickulerly fast, but it's for debugging anyways
	edict_t* pEdict = GetEdict(handle);
	CBaseEntity *ent = CBaseEntity::Instance( pEdict );
	
	// Is it the world?
	if (ENTINDEX(pEdict) == 0)
		return STRING(gpGlobals->mapname);

	// Is it a model?
	if ( ent && ent->GetModelName() != NULL_STRING )
		return STRING( ent->GetModelName() );

	if ( ent->GetClassname() != NULL )
	{
		return ent->GetClassname();
	}

	return "?";
}	

//-----------------------------------------------------------------------------
// When we do a collision test, we report everything we hit.. 
//-----------------------------------------------------------------------------

void CMoveHelperServer::ResetTouchList( void )
{
	m_TouchList.RemoveAll();
}


//-----------------------------------------------------------------------------
// When a collision occurs, we add it to the touched list
//-----------------------------------------------------------------------------

bool CMoveHelperServer::AddToTouched( const trace_t &tr, const Vector& impactvelocity )
{
	Assert( m_pHostPlayer );

	// Trace missed
	if ( !tr.m_pEnt )
		return false;

	if ( tr.m_pEnt == m_pHostPlayer )
	{
		Assert( !"CMoveHelperServer::AddToTouched:  Tried to add self to touchlist!!!" );
		return false;
	}

	// Check for duplicate entities
	for ( int j = m_TouchList.Size(); --j >= 0; )
	{
		if ( m_TouchList[j].trace.m_pEnt == tr.m_pEnt )
		{
			return false;
		}
	}
	
	int i = m_TouchList.AddToTail();
	m_TouchList[i].trace = tr;
	VectorCopy( impactvelocity, m_TouchList[i].deltavelocity );

	return true;
}


//-----------------------------------------------------------------------------
// After we built the touch list, deal with all the impacts...
//-----------------------------------------------------------------------------
void CMoveHelperServer::ProcessImpacts( void )
{
	Assert( m_pHostPlayer );

	// Relink in order to build absorigin and absmin/max to reflect any changes
	//  from prediction.  Relink will early out on SOLID_NOT
	m_pHostPlayer->PhysicsTouchTriggers();

	// Don't bother if the player ain't solid
	if ( m_pHostPlayer->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
		return;

	// Save off the velocity, cause we need to temporarily reset it
	Vector vel = m_pHostPlayer->GetAbsVelocity();

	// Touch other objects that were intersected during the movement.
	for (int i = 0 ; i < m_TouchList.Size(); i++)
	{
		CBaseHandle entindex = m_TouchList[i].trace.m_pEnt->GetRefEHandle();

		// We should have culled negative indices by now
		Assert( entindex.IsValid() );

		edict_t* ent = GetEdict( entindex );
		if (!ent)
			continue;

		// Run the impact function as if we had run it during movement.
		CBaseEntity *entity = GetContainingEntity( ent );
		if ( !entity )
			continue;

		Assert( entity != m_pHostPlayer );
		// Don't ever collide with self!!!!
		if ( entity == m_pHostPlayer )
			continue;

		// Reconstruct trace results.
		m_TouchList[i].trace.m_pEnt = CBaseEntity::Instance( ent );

		// Use the velocity we had when we collided, so boxes will move, etc.
		m_pHostPlayer->SetAbsVelocity( m_TouchList[i].deltavelocity );
		
		entity->PhysicsImpact( m_pHostPlayer, m_TouchList[i].trace );
	}

	// Restore the velocity
	m_pHostPlayer->SetAbsVelocity( vel );

	// So no stuff is ever left over, sigh...
	ResetTouchList();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			*soundname - 
//-----------------------------------------------------------------------------
void CMoveHelperServer::StartSound( const Vector& origin, const char *soundname )
{
	//MDB - Changing this to send to PAS, as the overloaded function below has done.
	//Also removed the UsePredictionRules, client does not yet play the equivalent sound

	CRecipientFilter filter;
	filter.AddRecipientsByPAS( origin );

	CBaseEntity::EmitSound( filter, m_pHostPlayer->entindex(), soundname );
}

//-----------------------------------------------------------------------------
// plays a sound
//-----------------------------------------------------------------------------
void CMoveHelperServer::StartSound( const Vector& origin, int channel, char const* sample, 
						float volume, soundlevel_t soundlevel, int fFlags, int pitch )
{

	CRecipientFilter filter;
	filter.AddRecipientsByPAS( origin );
	// FIXME, these sounds should not go to the host entity ( SND_NOTHOST )
	if ( gpGlobals->maxClients == 1 )
	{
		// Always send sounds down in SP

		EmitSound_t ep;
		ep.m_nChannel = channel;
		ep.m_pSoundName = sample;
		ep.m_flVolume = volume;
		ep.m_SoundLevel = soundlevel;
		ep.m_nFlags = fFlags;
		ep.m_nPitch = pitch;
		ep.m_pOrigin = &origin;

		CBaseEntity::EmitSound( filter, m_pHostPlayer->entindex(), ep );
	}
	else
	{
		filter.UsePredictionRules();

		EmitSound_t ep;
		ep.m_nChannel = channel;
		ep.m_pSoundName = sample;
		ep.m_flVolume = volume;
		ep.m_SoundLevel = soundlevel;
		ep.m_nFlags = fFlags;
		ep.m_nPitch = pitch;
		ep.m_pOrigin = &origin;

		CBaseEntity::EmitSound( filter, m_pHostPlayer->entindex(), ep );
	}
}


//-----------------------------------------------------------------------------
// Umm...
//-----------------------------------------------------------------------------
void CMoveHelperServer::PlaybackEventFull( int flags, int clientindex, unsigned short eventindex, float delay, Vector& origin, Vector& angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 )
{
	// FIXME, Redo with new event system parameter stuff
}

IPhysicsSurfaceProps *CMoveHelperServer::GetSurfaceProps( void )
{
	extern IPhysicsSurfaceProps *physprops;
	return physprops;
}

//-----------------------------------------------------------------------------
// Purpose: Note that this only works on a listen server (since it requires graphical output)
//			*pFormat - 
//			... - 
//-----------------------------------------------------------------------------
void CMoveHelperServer::Con_NPrintf( int idx, char const* pFormat, ...)
{
	va_list marker;
	char msg[8192];

	va_start(marker, pFormat);
	Q_vsnprintf(msg, sizeof( msg ), pFormat, marker);
	va_end(marker);
	
	engine->Con_NPrintf( idx, msg );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the player falls onto a surface fast enough to take
//			damage, according to the rules in CGameMovement::CheckFalling.
// Output : Returns true if the player survived the fall, false if they died.
//-----------------------------------------------------------------------------
bool CMoveHelperServer::PlayerFallingDamage( void )
{
	float flFallDamage = g_pGameRules->FlPlayerFallDamage( m_pHostPlayer );	
	if ( flFallDamage > 0 )
	{
		m_pHostPlayer->TakeDamage( CTakeDamageInfo( GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), flFallDamage, DMG_FALL ) ); 
		StartSound( m_pHostPlayer->GetAbsOrigin(), "Player.FallDamage" );

        //=============================================================================
        // HPE_BEGIN:
        // [dwenger] Needed for fun-fact implementation
        //=============================================================================

#ifdef CSTRIKE_DLL

        // Increment the stat for fall damage
        CCSPlayer*  pPlayer = ToCSPlayer(m_pHostPlayer);

        if ( pPlayer )
        {
            CCS_GameStats.IncrementStat( pPlayer, CSSTAT_FALL_DAMAGE, (int)flFallDamage );
        }

#endif
        //=============================================================================
        // HPE_END
        //=============================================================================

    }

	if ( m_pHostPlayer->m_iHealth <= 0 )
	{
		if ( g_pGameRules->FlPlayerFallDeathDoesScreenFade( m_pHostPlayer ) )
		{
			color32 black = {0, 0, 0, 255};
			UTIL_ScreenFade( m_pHostPlayer, black, 0, 9999, FFADE_OUT | FFADE_STAYOUT );
		}
		return(false);
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Sets an animation in the player.
// Input  : eAnim - Animation to set.
//-----------------------------------------------------------------------------
void CMoveHelperServer::PlayerSetAnimation( PLAYER_ANIM eAnim )
{
	m_pHostPlayer->SetAnimation( eAnim );
}

bool CMoveHelperServer::IsWorldEntity( const CBaseHandle &handle )
{
	return handle == CBaseEntity::Instance( 0 );
}
