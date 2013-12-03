//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//===========================================================================//

#if !defined( ITEMPENTS_H )
#define ITEMPENTS_H
#ifdef _WIN32
#pragma once
#endif

#include "ipredictionsystem.h"
#include "shattersurfacetypes.h"
#include "irecipientfilter.h"

class CEffectData;
class KeyValues;


//-----------------------------------------------------------------------------
// Purpose:  Shared interface to temp entities
//-----------------------------------------------------------------------------
abstract_class ITempEntsSystem : public IPredictionSystem
{
public:
	virtual void ArmorRicochet( IRecipientFilter& filer, float delay,
		const Vector* pos, const Vector* dir ) = 0;
	virtual void BeamEntPoint( IRecipientFilter& filer, float delay,
		int	nStartEntity, const Vector *start, int nEndEntity, const Vector* end, 
		int modelindex, int haloindex, int startframe, int framerate,
		float life, float width, float endWidth, int fadeLength, float amplitude, 
		int r, int g, int b, int a, int speed ) = 0;
	virtual void BeamEnts( IRecipientFilter& filer, float delay,
		int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
		float life, float width, float endWidth, int fadeLength, float amplitude, 
		int r, int g, int b, int a, int speed ) = 0;
	virtual void BeamFollow( IRecipientFilter& filter, float delay,
		int iEntIndex, int modelIndex, int haloIndex, float life, float width, float endWidth, 
		float fadeLength, float r, float g, float b, float a ) = 0;
	virtual void BeamPoints( IRecipientFilter& filer, float delay,
		const Vector* start, const Vector* end, int modelindex, int haloindex, int startframe, int framerate,
		float life, float width, float endWidth, int fadeLength, float amplitude, 
		int r, int g, int b, int a, int speed ) = 0;
	virtual void BeamLaser( IRecipientFilter& filer, float delay,
		int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
		float life, float width, float endWidth, int fadeLength, float amplitude, int r, int g, int b, int a, int speed ) = 0;
	virtual void BeamRing( IRecipientFilter& filer, float delay,
		int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
		float life, float width, int spread, float amplitude, int r, int g, int b, int a, int speed, int flags = 0 ) = 0;
	virtual void BeamRingPoint( IRecipientFilter& filer, float delay,
		const Vector& center, float start_radius, float end_radius, int modelindex, int haloindex, int startframe, int framerate,
		float life, float width, int spread, float amplitude, int r, int g, int b, int a, int speed, int flags = 0 ) = 0;
	virtual void BeamSpline( IRecipientFilter& filer, float delay,
		int points, Vector* rgPoints ) = 0;
	virtual void BloodStream( IRecipientFilter& filer, float delay,
		const Vector* org, const Vector* dir, int r, int g, int b, int a, int amount ) = 0;
	virtual void BloodSprite( IRecipientFilter& filer, float delay,
		const Vector* org, const Vector *dir, int r, int g, int b, int a, int size ) = 0;
	virtual void BreakModel( IRecipientFilter& filer, float delay,
		const Vector& pos, const QAngle &angle, const Vector& size, const Vector& vel, 
		int modelindex, int randomization, int count, float time, int flags ) = 0;
	virtual void BSPDecal( IRecipientFilter& filer, float delay,
		const Vector* pos, int entity, int index ) = 0;
	virtual void ProjectDecal( IRecipientFilter& filter, float delay,
		const Vector* pos, const QAngle *angles, float distance, int index ) = 0;
	virtual void Bubbles( IRecipientFilter& filer, float delay,
		const Vector* mins, const Vector* maxs, float height, int modelindex, int count, float speed ) = 0;
	virtual void BubbleTrail( IRecipientFilter& filer, float delay,
		const Vector* mins, const Vector* maxs, float height, int modelindex, int count, float speed ) = 0;
	virtual void Decal( IRecipientFilter& filer, float delay,
		const Vector* pos, const Vector* start, int entity, int hitbox, int index ) = 0;
	virtual void DynamicLight( IRecipientFilter& filer, float delay,
		const Vector* org, int r, int g, int b, int exponent, float radius, float time, float decay ) = 0;
	virtual void Explosion( IRecipientFilter& filer, float delay,
		const Vector* pos, int modelindex, float scale, int framerate, int flags, int radius, int magnitude, const Vector* normal = NULL, unsigned char materialType = 'C' ) = 0;
	virtual void ShatterSurface( IRecipientFilter& filer, float delay,
		const Vector* pos, const QAngle* angle, const Vector* vForce, const Vector* vForcePos, 
		float width, float height, float shardsize, ShatterSurface_t surfacetype,
		int front_r, int front_g, int front_b, int back_r, int back_g, int back_b) = 0;
	virtual void GlowSprite( IRecipientFilter& filer, float delay,
		const Vector* pos, int modelindex, float life, float size, int brightness ) = 0;
	virtual void FootprintDecal( IRecipientFilter& filer, float delay, const Vector *origin, const Vector* right, 
		int entity, int index, unsigned char materialType ) = 0;
	virtual void Fizz( IRecipientFilter& filer, float delay,
		const CBaseEntity *ed, int modelindex, int density, int current ) = 0;
	virtual void KillPlayerAttachments( IRecipientFilter& filer, float delay,
		int player ) = 0;
	virtual void LargeFunnel( IRecipientFilter& filer, float delay,
		const Vector* pos, int modelindex, int reversed ) = 0;
	virtual void MetalSparks( IRecipientFilter& filer, float delay,
		const Vector* pos, const Vector* dir ) = 0;
	virtual void EnergySplash( IRecipientFilter& filer, float delay,
		const Vector* pos, const Vector* dir, bool bExplosive ) = 0;
	virtual void PlayerDecal( IRecipientFilter& filer, float delay,
		const Vector* pos, int player, int entity ) = 0;
	virtual void ShowLine( IRecipientFilter& filer, float delay,
		const Vector* start, const Vector* end ) = 0;
	virtual void Smoke( IRecipientFilter& filer, float delay,
		const Vector* pos, int modelindex, float scale, int framerate ) = 0;
	virtual void Sparks( IRecipientFilter& filer, float delay,
		const Vector* pos, int nMagnitude, int nTrailLength, const Vector *pDir ) = 0;
	virtual void Sprite( IRecipientFilter& filer, float delay,
		const Vector* pos, int modelindex, float size, int brightness ) = 0;
	virtual void SpriteSpray( IRecipientFilter& filer, float delay,
		const Vector* pos, const Vector* dir, int modelindex, int speed, float noise, int count ) = 0;
	virtual void WorldDecal( IRecipientFilter& filer, float delay,
		const Vector* pos, int index ) = 0;
	virtual void MuzzleFlash( IRecipientFilter& filer, float delay,
		const Vector &start, const QAngle &angles, float scale, int type ) = 0;
	virtual void Dust( IRecipientFilter& filer, float delay,
				 const Vector &pos, const Vector &dir, float size, float speed ) = 0;
	virtual void GaussExplosion( IRecipientFilter& filer, float delay,
				const Vector &pos, const Vector &dir, int type ) = 0;
	virtual void DispatchEffect( IRecipientFilter& filter, float delay,
				const Vector &pos, const char *pName, const CEffectData &data ) = 0;
	virtual void PhysicsProp( IRecipientFilter& filter, float delay, int modelindex, int skin, 
		const Vector& pos, const QAngle &angles, const Vector& vel, int flags, int effects ) = 0;

	// For playback from external tools
	virtual void TriggerTempEntity( KeyValues *pKeyValues ) = 0;

	virtual void ClientProjectile( IRecipientFilter& filter, float delay,
		const Vector* vecOrigin, const Vector* vecVelocity, int modelindex, int lifetime, CBaseEntity *pOwner ) = 0;
};

extern ITempEntsSystem *te;

#endif // ITEMPENTS_H
