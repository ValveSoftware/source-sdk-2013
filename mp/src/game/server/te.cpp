//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "te.h"
#include "effect_dispatch_data.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// External definitions
void TE_ArmorRicochet( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* dir );
void TE_BeamEntPoint( IRecipientFilter& filter, float delay,
	int	nStartEntity, const Vector *start, int nEndEntity, const Vector* end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, float endWidth, int fadeLength, float amplitude, 
	int r, int g, int b, int a, int speed );
void TE_BeamEnts( IRecipientFilter& filter, float delay,
	int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, float endWidth, int fadeLength, float amplitude, 
	int r, int g, int b, int a, int speed );
void TE_BeamFollow( IRecipientFilter& filter, float delay,
	int iEntIndex, int modelIndex, int haloIndex, float life, float width, float endWidth, 
	float fadeLength,float r, float g, float b, float a );
void TE_BeamPoints( IRecipientFilter& filter, float delay,
	const Vector* start, const Vector* end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, float endWidth, int fadeLength, float amplitude, 
	int r, int g, int b, int a, int speed );
void TE_BeamLaser( IRecipientFilter& filter, float delay,
	int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, float endWidth, int fadeLength, float amplitude, int r, int g, int b, int a, int speed );
void TE_BeamRing( IRecipientFilter& filter, float delay,
	int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, int spread, float amplitude, int r, int g, int b, int a, int speed, int flags = 0 );
void TE_BeamRingPoint( IRecipientFilter& filter, float delay,
	const Vector& center, float start_radius, float end_radius, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, int spread, float amplitude, int r, int g, int b, int a, int speed, int flags = 0 );
void TE_BeamSpline( IRecipientFilter& filter, float delay,
	int points, Vector* rgPoints );
void TE_BloodStream( IRecipientFilter& filter, float delay,
	const Vector* org, const Vector* dir, int r, int g, int b, int a, int amount );
void TE_BloodSprite( IRecipientFilter& filter, float delay,
	const Vector* org, const Vector *dir, int r, int g, int b, int a, int size );
void TE_BreakModel( IRecipientFilter& filter, float delay,
	const Vector& pos, const QAngle &angles, const Vector& size, const Vector& vel, 
	int modelindex, int randomization, int count, float time, int flags );
void TE_BSPDecal( IRecipientFilter& filter, float delay,
	const Vector* pos, int entity, int index );
void TE_ProjectDecal( IRecipientFilter& filer, float delay,
	const Vector* pos, const QAngle *angles, float distance, int index );
void TE_Bubbles( IRecipientFilter& filter, float delay,
	const Vector* mins, const Vector* maxs, float height, int modelindex, int count, float speed );
void TE_BubbleTrail( IRecipientFilter& filter, float delay,
	const Vector* mins, const Vector* maxs, float height, int modelindex, int count, float speed );
void TE_Decal( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* start, int entity, int hitbox, int index );
void TE_DynamicLight( IRecipientFilter& filter, float delay,
	const Vector* org, int r, int g, int b, int exponent, float radius, float time, float decay );
void TE_Explosion( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, float scale, int framerate, int flags, int radius, int magnitude, const Vector* normal = NULL, unsigned char materialType = 'C' );
void TE_ShatterSurface( IRecipientFilter& filter, float delay,
	const Vector* pos, const QAngle* angle, const Vector* vForce, const Vector* vForcePos, 
	float width, float height, float shardsize, ShatterSurface_t surfacetype,
	int front_r, int front_g, int front_b, int back_r, int back_g, int back_b);
void TE_GlowSprite( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, float life, float size, int brightness );
void TE_FootprintDecal( IRecipientFilter& filter, float delay, const Vector *origin, const Vector* right, 
	int entity, int index, unsigned char materialType );
void TE_Fizz( IRecipientFilter& filter, float delay,
	const CBaseEntity *ed, int modelindex, int density, int current );
void TE_KillPlayerAttachments( IRecipientFilter& filter, float delay,
	int player );
void TE_LargeFunnel( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, int reversed );
void TE_MetalSparks( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* dir );
void TE_EnergySplash( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* dir, bool bExplosive );
void TE_PlayerDecal( IRecipientFilter& filter, float delay,
	const Vector* pos, int player, int entity );
void TE_ShowLine( IRecipientFilter& filter, float delay,
	const Vector* start, const Vector* end );
void TE_Smoke( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, float scale, int framerate );
void TE_Sparks( IRecipientFilter& filter, float delay,
	const Vector* pos, int nMagnitude, int nTrailLength, const Vector *pDir );
void TE_Sprite( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, float size, int brightness );
void TE_SpriteSpray( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* dir, int modelindex, int speed, float noise, int count );
void TE_WorldDecal( IRecipientFilter& filter, float delay,
	const Vector* pos, int index );
void TE_MuzzleFlash( IRecipientFilter& filter, float delayt,
	const Vector &start, const QAngle &angles, float scale, int type );
void TE_Dust( IRecipientFilter& filter, float delayt,
			 const Vector &pos, const Vector &dir, float size, float speed );
void TE_DispatchEffect( IRecipientFilter& filter, float delay,
				const Vector &pos, const char *pName, const CEffectData &data );
void TE_PhysicsProp( IRecipientFilter& filter, float delay,
	int modelindex, int skin, const Vector& pos, const QAngle &angles, const Vector& vel, int flags, int effects );
void TE_ClientProjectile( IRecipientFilter& filter, float delay,
	 const Vector* vecOrigin, const Vector* vecVelocity, int modelindex, int lifetime, CBaseEntity *pOwner );

#ifdef HL2_DLL
void TE_GaussExplosion( IRecipientFilter& filter, float delayt,
			 const Vector &pos, const Vector &dir, int type );
#endif

class CTempEntsSystem : public ITempEntsSystem
{
private:
	//-----------------------------------------------------------------------------
	// Purpose: Returning true means don't even call TE func
	// Input  : filter - 
	//			*suppress_host - 
	// Output : static bool
	//-----------------------------------------------------------------------------
	bool SuppressTE( IRecipientFilter& filter )
	{
		if ( GetSuppressHost() )
		{
			CRecipientFilter& _filter = (( CRecipientFilter & )filter);

			if ( !_filter.IgnorePredictionCull() )
			{
				_filter.RemoveRecipient( (CBasePlayer *)GetSuppressHost()  );
			}

			if ( !_filter.GetRecipientCount() )
			{
				// Suppress it
				return true;
			}
		}

		// There's at least one recipient
		return false;
	}
public:

	virtual void ArmorRicochet( IRecipientFilter& filter, float delay,
		const Vector* pos, const Vector* dir )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_ArmorRicochet( filter, delay, pos, dir );
		}
	}

	virtual void BeamEntPoint( IRecipientFilter& filter, float delay,
		int	nStartEntity, const Vector *pStart, int nEndEntity, const Vector* pEnd, 
		int modelindex, int haloindex, int startframe, int framerate,
		float life, float width, float endWidth, int fadeLength, float amplitude, 
		int r, int g, int b, int a, int speed )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_BeamEntPoint( filter, delay, nStartEntity, pStart, nEndEntity, pEnd, modelindex, haloindex, startframe, framerate,
				life, width, endWidth, fadeLength, amplitude, r, g, b, a, speed );
		}
	}

	virtual void BeamEnts( IRecipientFilter& filter, float delay,
		int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
		float life, float width, float endWidth, int fadeLength, float amplitude, 
		int r, int g, int b, int a, int speed )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_BeamEnts( filter, delay,
				start, end, modelindex, haloindex, startframe, framerate,
				life, width, endWidth, fadeLength, amplitude, 
				r, g, b, a, speed );
		}
	}
	virtual void BeamFollow( IRecipientFilter& filter, float delay,
		int iEntIndex, int modelIndex, int haloIndex, float life, float width, float endWidth, 
		float fadeLength, float r, float g, float b, float a )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_BeamFollow( filter, delay,
				iEntIndex, modelIndex, haloIndex, life, width, endWidth, fadeLength, 
				r, g, b, a );
		}
	}
	virtual void BeamPoints( IRecipientFilter& filter, float delay,
		const Vector* start, const Vector* end, int modelindex, int haloindex, int startframe, int framerate,
		float life, float width, float endWidth, int fadeLength, float amplitude, 
		int r, int g, int b, int a, int speed )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_BeamPoints( filter, delay,
				start, end, modelindex, haloindex, startframe, framerate,
				life, width, endWidth, fadeLength, amplitude, 
				r, g, b, a, speed );
		}
	}
	virtual void BeamLaser( IRecipientFilter& filter, float delay,
		int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
		float life, float width, float endWidth, int fadeLength, float amplitude, int r, int g, int b, int a, int speed )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_BeamLaser( filter, delay,
				start, end, modelindex, haloindex, startframe, framerate,
				life, width, endWidth, fadeLength, amplitude, r, g, b, a, speed );
		}
	}
	virtual void BeamRing( IRecipientFilter& filter, float delay,
		int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
		float life, float width, int spread, float amplitude, int r, int g, int b, int a, int speed, int flags )
	{
		if ( !SuppressTE( filter ) )
		{	
			TE_BeamRing( filter, delay,
				start, end, modelindex, haloindex, startframe, framerate,
				life, width, spread, amplitude, r, g, b, a, speed, flags );
		}
	}
	virtual void BeamRingPoint( IRecipientFilter& filter, float delay,
		const Vector& center, float start_radius, float end_radius, int modelindex, int haloindex, int startframe, int framerate,
		float life, float width, int spread, float amplitude, int r, int g, int b, int a, int speed, int flags )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_BeamRingPoint( filter, delay,
				center, start_radius, end_radius, modelindex, haloindex, startframe, framerate,
				life, width, spread, amplitude, r, g, b, a, speed, flags );
		}
	}
	virtual void BeamSpline( IRecipientFilter& filter, float delay,
		int points, Vector* rgPoints )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_BeamSpline( filter, delay, points, rgPoints );
		}
	}
	virtual void BloodStream( IRecipientFilter& filter, float delay,
		const Vector* org, const Vector* dir, int r, int g, int b, int a, int amount )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_BloodStream( filter, delay, org, dir, r, g, b, a, amount );
		}
	}
	virtual void BloodSprite( IRecipientFilter& filter, float delay,
		const Vector* org, const Vector *dir, int r, int g, int b, int a, int size )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_BloodSprite( filter, delay, org, dir, r, g, b, a, size );
		}
	}
	virtual void BreakModel( IRecipientFilter& filter, float delay,
		const Vector& pos, const QAngle &angle, const Vector& size, const Vector& vel, 
		int modelindex, int randomization, int count, float time, int flags )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_BreakModel( filter, delay, pos, angle, size, vel, modelindex, randomization, count, time, flags );
		}
	}
	virtual void BSPDecal( IRecipientFilter& filter, float delay,
		const Vector* pos, int entity, int index )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_BSPDecal( filter, delay, pos, entity, index );
		}
	}

	virtual void ProjectDecal( IRecipientFilter& filter, float delay,
		const Vector* pos, const QAngle *angles, float distance, int index )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_ProjectDecal( filter, delay, pos, angles, distance, index );
		}
	}

	virtual void Bubbles( IRecipientFilter& filter, float delay,
		const Vector* mins, const Vector* maxs, float height, int modelindex, int count, float speed )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_Bubbles( filter, delay, mins, maxs, height, modelindex, count, speed );
		}
	}
	virtual void BubbleTrail( IRecipientFilter& filter, float delay,
		const Vector* mins, const Vector* maxs, float flWaterZ, int modelindex, int count, float speed )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_BubbleTrail( filter, delay, mins, maxs, flWaterZ, modelindex, count, speed );
		}
	}
	virtual void Decal( IRecipientFilter& filter, float delay,
		const Vector* pos, const Vector* start, int entity, int hitbox, int index )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_Decal( filter, delay, pos, start, entity, hitbox, index );
		}
	}
	virtual void DynamicLight( IRecipientFilter& filter, float delay,
		const Vector* org, int r, int g, int b, int exponent, float radius, float time, float decay )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_DynamicLight( filter, delay, org, r, g, b, exponent, radius, time, decay );
		}
	}
	virtual void Explosion( IRecipientFilter& filter, float delay,
		const Vector* pos, int modelindex, float scale, int framerate, int flags, int radius, int magnitude, const Vector* normal = NULL, unsigned char materialType = 'C' )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_Explosion( filter, delay, pos, modelindex, scale, framerate, flags, radius, magnitude, normal, materialType );
		}
	}
	virtual void ShatterSurface( IRecipientFilter& filter, float delay,
		const Vector* pos, const QAngle* angle, const Vector* vForce, const Vector* vForcePos, 
		float width, float height, float shardsize, ShatterSurface_t surfacetype,
		int front_r, int front_g, int front_b, int back_r, int back_g, int back_b)
	{
		if ( !SuppressTE( filter ) )
		{
			TE_ShatterSurface( filter, delay, pos, angle, vForce, vForcePos, width, height, shardsize, surfacetype,
				 front_r, front_g, front_b, back_r, back_g, back_b );
		}
	}
	virtual void GlowSprite( IRecipientFilter& filter, float delay,
		const Vector* pos, int modelindex, float life, float size, int brightness )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_GlowSprite( filter, delay, pos, modelindex, life, size, brightness );
		}
	}
	virtual void FootprintDecal( IRecipientFilter& filter, float delay, const Vector *origin, const Vector* right, 
		int entity, int index, unsigned char materialType )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_FootprintDecal( filter, delay, origin, right, 
				entity, index, materialType );
		}
	}
	virtual void Fizz( IRecipientFilter& filter, float delay,
		const CBaseEntity *ed, int modelindex, int density, int current )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_Fizz( filter, delay,
				ed, modelindex, density, current );
		}
	}
	virtual void KillPlayerAttachments( IRecipientFilter& filter, float delay,
		int player )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_KillPlayerAttachments( filter, delay, player );
		}
	}
	virtual void LargeFunnel( IRecipientFilter& filter, float delay,
		const Vector* pos, int modelindex, int reversed )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_LargeFunnel( filter, delay, pos, modelindex, reversed );
		}
	}
	virtual void MetalSparks( IRecipientFilter& filter, float delay,
		const Vector* pos, const Vector* dir )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_MetalSparks( filter, delay, pos, dir );
		}
	}
	virtual void EnergySplash( IRecipientFilter& filter, float delay,
		const Vector* pos, const Vector* dir, bool bExplosive )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_EnergySplash( filter, delay,
				pos, dir, bExplosive );
		}
	}
	virtual void PlayerDecal( IRecipientFilter& filter, float delay,
		const Vector* pos, int player, int entity )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_PlayerDecal( filter, delay,
				pos, player, entity );
		}
	}
	virtual void ShowLine( IRecipientFilter& filter, float delay,
		const Vector* start, const Vector* end )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_ShowLine( filter, delay,
				start, end );
		}
	}
	virtual void Smoke( IRecipientFilter& filter, float delay,
		const Vector* pos, int modelindex, float scale, int framerate )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_Smoke( filter, delay,
				pos, modelindex, scale, framerate );
		}
	}
	virtual void Sparks( IRecipientFilter& filter, float delay,
		const Vector* pos, int nMagnitude, int nTrailLength, const Vector *pDir )
	{
		if ( !SuppressTE( filter ) )
		{	
			TE_Sparks( filter, delay,
				pos, nMagnitude, nTrailLength, pDir );
		}
	}
	virtual void Sprite( IRecipientFilter& filter, float delay,
		const Vector* pos, int modelindex, float size, int brightness )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_Sprite( filter, delay,
				pos, modelindex, size, brightness );
		}
	}
	virtual void SpriteSpray( IRecipientFilter& filter, float delay,
		const Vector* pos, const Vector* dir, int modelindex, int speed, float noise, int count )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_SpriteSpray( filter, delay,
				pos, dir, modelindex, speed, noise, count );
		}
	}
	virtual void WorldDecal( IRecipientFilter& filter, float delay,
		const Vector* pos, int index )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_WorldDecal( filter, delay,
				pos, index );
		}
	}
	virtual void MuzzleFlash( IRecipientFilter& filter, float delay,
		const Vector &start, const QAngle &angles, float scale, int type )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_MuzzleFlash( filter, delay,
				start, angles, scale, type );
		}
	}
	virtual void Dust( IRecipientFilter& filter, float delay,
				 const Vector &pos, const Vector &dir, float size, float speed )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_Dust( filter, delay,
				pos, dir, size, speed );
		}
	}
	virtual void GaussExplosion( IRecipientFilter& filter, float delay,
				const Vector &pos, const Vector &dir, int type )
	{
#ifdef HL2_DLL
		if ( !SuppressTE( filter ) )
		{
			TE_GaussExplosion( filter, delay, pos, dir, type );
		}
#endif
	}

	virtual void DispatchEffect( IRecipientFilter& filter, float delay,
				const Vector &pos, const char *pName, const CEffectData &data )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_DispatchEffect( filter, delay, pos, pName, data );
		}
	}

	virtual void PhysicsProp( IRecipientFilter& filter, float delay, int modelindex, int skin,
		const Vector& pos, const QAngle &angles, const Vector& vel, int flags, int effects )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_PhysicsProp( filter, delay, modelindex, skin, pos, angles, vel, flags, effects );
		}
	}

	// For playback from external tools
	virtual void TriggerTempEntity( KeyValues *pKeyValues )
	{
		Assert(0);
	}

	virtual void ClientProjectile( IRecipientFilter& filter, float delay,
		const Vector* vecOrigin, const Vector* vecVelocity, int modelindex, int lifetime, CBaseEntity *pOwner )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_ClientProjectile( filter, delay, vecOrigin, vecVelocity, modelindex, lifetime, pOwner );
		}
	}
};

static CTempEntsSystem g_TESystem;
// Expose to rest of engine
ITempEntsSystem *te = &g_TESystem;
