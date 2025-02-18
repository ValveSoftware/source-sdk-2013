//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Utility code.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "te.h"
#include "shake.h"
#include "decals.h"
#include "IEffects.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern short		g_sModelIndexSmoke;			// (in combatweapon.cpp) holds the index for the smoke cloud
extern short		g_sModelIndexBloodDrop;		// (in combatweapon.cpp) holds the sprite index for the initial blood
extern short		g_sModelIndexBloodSpray;	// (in combatweapon.cpp) holds the sprite index for splattered blood


//-----------------------------------------------------------------------------
// Client-server neutral effects interface
//-----------------------------------------------------------------------------
class CEffectsServer : public IEffects
{
public:
	CEffectsServer();
	virtual ~CEffectsServer();

	// Members of the IEffect interface
	virtual void Beam( const Vector &Start, const Vector &End, int nModelIndex, 
		int nHaloIndex, unsigned char frameStart, unsigned char frameRate,
		float flLife, unsigned char width, unsigned char endWidth, unsigned char fadeLength, 
		unsigned char noise, unsigned char red, unsigned char green,
		unsigned char blue, unsigned char brightness, unsigned char speed);
	virtual void Smoke( const Vector &origin, int mModel, float flScale, float flFramerate );
	virtual void Sparks( const Vector &position, int nMagnitude = 1, int nTrailLength = 1, const Vector *pvecDir = NULL );
	virtual void Dust( const Vector &pos, const Vector &dir, float size, float speed );
	virtual void MuzzleFlash( const Vector &origin, const QAngle &angles, float scale, int type );
	virtual void MetalSparks( const Vector &position, const Vector &direction ); 
	virtual void EnergySplash( const Vector &position, const Vector &direction, bool bExplosive = false );
	virtual void Ricochet( const Vector &position, const Vector &direction );

	// FIXME: Should these methods remain in this interface? Or go in some 
	// other client-server neutral interface?
	virtual float Time();
	virtual bool IsServer();
	virtual void SuppressEffectsSounds( bool bSuppress ) { Assert(0); }

private:
	//-----------------------------------------------------------------------------
	// Purpose: Returning true means don't even call TE func
	// Input  : filter - 
	//			*suppress_host - 
	// Output : static bool
	//-----------------------------------------------------------------------------
	bool SuppressTE( CRecipientFilter& filter )
	{
		if ( GetSuppressHost() )
		{
			if ( !filter.IgnorePredictionCull() )
			{
				filter.RemoveRecipient( (CBasePlayer *)GetSuppressHost()  );
			}

			if ( !filter.GetRecipientCount() )
			{
				// Suppress it
				return true;
			}
		}

		// There's at least one recipient
		return false;
	}
};


//-----------------------------------------------------------------------------
// Client-server neutral effects interface accessor
//-----------------------------------------------------------------------------
static CEffectsServer s_EffectServer;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CEffectsServer, IEffects, IEFFECTS_INTERFACE_VERSION, s_EffectServer);
IEffects *g_pEffects = &s_EffectServer;


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CEffectsServer::CEffectsServer()
{
}

CEffectsServer::~CEffectsServer()
{
}


//-----------------------------------------------------------------------------
// Generates a beam
//-----------------------------------------------------------------------------
void CEffectsServer::Beam( const Vector &vecStart, const Vector &vecEnd, int nModelIndex, 
	int nHaloIndex, unsigned char frameStart, unsigned char frameRate,
	float flLife, unsigned char width, unsigned char endWidth, unsigned char fadeLength, 
	unsigned char noise, unsigned char red, unsigned char green,
	unsigned char blue, unsigned char brightness, unsigned char speed)
{
	CBroadcastRecipientFilter filter;
	if ( !SuppressTE( filter ) )
	{
		te->BeamPoints( filter, 0.0,
			&vecStart, &vecEnd, nModelIndex, nHaloIndex, frameStart, frameRate, flLife,  
			width, endWidth, fadeLength, noise, red, green, blue, brightness, speed );
	}
}


//-----------------------------------------------------------------------------
// Generates various tempent effects
//-----------------------------------------------------------------------------
void CEffectsServer::Smoke( const Vector &origin, int mModel, float flScale, float flFramerate )
{
	CPVSFilter filter( origin );
	if ( !SuppressTE( filter ) )
	{
		te->Smoke( filter, 0.0, &origin, mModel, flScale * 0.1f, flFramerate );
	}
}

void CEffectsServer::Sparks( const Vector &position, int nMagnitude, int nTrailLength, const Vector *pvecDir )
{
	CPVSFilter filter( position );
	if ( !SuppressTE( filter ) )
	{
		te->Sparks( filter, 0.0, &position, nMagnitude, nTrailLength, pvecDir );
	}
}

void CEffectsServer::Dust( const Vector &pos, const Vector &dir, float size, float speed )
{
	CPVSFilter filter( pos );
	if ( !SuppressTE( filter ) )
	{
		te->Dust( filter, 0.0, pos, dir, size, speed );
	}
}

void CEffectsServer::MuzzleFlash( const Vector &origin, const QAngle &angles, float scale, int type )
{
	CPVSFilter filter( origin );
	if ( !SuppressTE( filter ) )
	{
		te->MuzzleFlash( filter, 0.0f, origin, angles, scale, type );
	}
}

void CEffectsServer::MetalSparks( const Vector &position, const Vector &direction )
{
	CPVSFilter filter( position );
	if ( !SuppressTE( filter ) )
	{
		te->MetalSparks( filter, 0.0, &position, &direction );
	}
}

void CEffectsServer::EnergySplash( const Vector &position, const Vector &direction, bool bExplosive )
{
	CPVSFilter filter( position );
	if ( !SuppressTE( filter ) )
	{
		te->EnergySplash( filter, 0.0, &position, &direction, bExplosive );
	}
}

void CEffectsServer::Ricochet( const Vector &position, const Vector &direction )
{
	CPVSFilter filter( position );
	if ( !SuppressTE( filter ) )
	{
		te->ArmorRicochet( filter, 0.0, &position, &direction );
	}
}


//-----------------------------------------------------------------------------
// FIXME: Should these methods remain in this interface? Or go in some 
// other client-server neutral interface?
//-----------------------------------------------------------------------------
float CEffectsServer::Time()
{
	return gpGlobals->curtime;
}

bool CEffectsServer::IsServer()
{
	return true;
}
