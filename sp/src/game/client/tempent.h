//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#if !defined( TEMPENT_H )
#define TEMPENT_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
#include "c_baseanimating.h"
#include "c_sprite.h"

// Temporary entity array
#define TENTPRIORITY_LOW	0
#define TENTPRIORITY_HIGH	1

// TEMPENTITY flags
#define	FTENT_NONE					0x00000000
#define	FTENT_SINEWAVE				0x00000001
#define	FTENT_GRAVITY				0x00000002
#define FTENT_ROTATE				0x00000004
#define	FTENT_SLOWGRAVITY			0x00000008
#define FTENT_SMOKETRAIL			0x00000010
#define FTENT_COLLIDEWORLD			0x00000020
#define FTENT_FLICKER				0x00000040
#define FTENT_FADEOUT				0x00000080
#define FTENT_SPRANIMATE			0x00000100
#define FTENT_HITSOUND				0x00000200
#define FTENT_SPIRAL				0x00000400
#define FTENT_SPRCYCLE				0x00000800
#define FTENT_COLLIDEALL			0x00001000 // will collide with world and slideboxes
#define FTENT_PERSIST				0x00002000 // tent is not removed when unable to draw 
#define FTENT_COLLIDEKILL			0x00004000 // tent is removed upon collision with anything
#define FTENT_PLYRATTACHMENT		0x00008000 // tent is attached to a player (owner)
#define FTENT_SPRANIMATELOOP		0x00010000 // animating sprite doesn't die when last frame is displayed
#define FTENT_SMOKEGROWANDFADE		0x00020000 // rapid grow and fade. Very specific for gunsmoke
#define FTENT_ATTACHTOTARGET		0x00040000 // attach to whatever we hit, and stay there until die time is up
#define FTENT_NOMODEL				0x00080000 // Doesn't have a model, never try to draw ( it just triggers other things )
#define FTENT_CLIENTCUSTOM			0x00100000 // Must specify callback.  Callback function is responsible for killing tempent and updating fields ( unless other flags specify how to do things )
#define FTENT_WINDBLOWN				0x00200000 // This is set when the temp entity is blown by the wind
#define FTENT_NEVERDIE				0x00400000 // Don't die as long as die != 0
#define FTENT_BEOCCLUDED			0x00800000 // Don't draw if my specified normal's facing away from the view
#define FTENT_CHANGERENDERONCOLLIDE	0x01000000	//when we collide with something, change our rendergroup to RENDER_GROUP_OTHER
#define FTENT_COLLISIONGROUP		0x02000000	// if set, use the C_BaseEntity::GetCollisionGroup when doing collide trace
#define FTENT_ALIGNTOMOTION			0x04000000	// Change angles to always point in the direction of motion
#define FTENT_CLIENTSIDEPARTICLES	0x08000000	// The object has a clientside particle system.
#define FTENT_USEFASTCOLLISIONS		0x10000000	// Use fast collisions (cl_fasttempentcollision).
#define FTENT_COLLIDEPROPS			0x20000000	// Collide with the world and props

class C_LocalTempEntity;

typedef int (*pfnDrawHelper)( C_LocalTempEntity *entity, int flags );

//-----------------------------------------------------------------------------
// Purpose: Should this derive from some other class
//-----------------------------------------------------------------------------
class C_LocalTempEntity : public C_BaseAnimating, public C_SpriteRenderer
{
public:
	DECLARE_CLASS( C_LocalTempEntity, C_BaseAnimating );

	C_LocalTempEntity();

	virtual void					Prepare( const model_t *pmodel, float time );

	virtual bool					IsActive( void );
	virtual bool					Frame( float frametime, int framenumber );

	// C_BaseAnimating , etc. override
	virtual int						DrawModel( int flags );

	// Sets the velocity
	void SetVelocity( const Vector &vecVelocity );
	const Vector &GetVelocity() const { return m_vecTempEntVelocity; }

	// Set the acceleration
	void SetAcceleration( const Vector &vecAccel );
	const Vector &GetAcceleration() const { return m_vecTempEntAcceleration; }

	void							SetDrawHelper( pfnDrawHelper helper ) { m_pfnDrawHelper = helper; }
	void							OnRemoveTempEntity();

	void							SetImpactEffect( const char *pszImpactEffect ) { m_pszImpactEffect = pszImpactEffect; }
	CNewParticleEffect*				AddParticleEffect( const char *pszParticleEffect );
	void							SetParticleEffect( const char *pszParticleEffect ) { m_pszParticleEffect = pszParticleEffect; }

protected:

	pfnDrawHelper					m_pfnDrawHelper;

public:
	int								flags;
	float							die;
	float							m_flFrameMax;
	float							x;
	float							y;
	float							fadeSpeed;
	float							bounceFactor;
	int								hitSound;
	int								priority;
	// if attached, this is the index of the client to stick to
	// if COLLIDEALL, this is the index of the client to ignore
	// TENTS with FTENT_PLYRATTACHMENT MUST set the clientindex! 
	short							clientIndex;	

	// if attached, client origin + tentOffset = tent origin.
	Vector							tentOffset;		

	// Used by temp entities.
	QAngle							m_vecTempEntAngVelocity;
	int								tempent_renderamt;
	Vector							m_vecNormal;

	float							m_flSpriteScale;
	int								m_nFlickerFrame;

	// 
	float							m_flFrameRate;
	float							m_flFrame;

	RenderGroup_t					m_RenderGroup;

	const char						*m_pszImpactEffect;
	const char						*m_pszParticleEffect;
	bool							m_bParticleCollision;

	int								m_iLastCollisionFrame;
	Vector							m_vLastCollisionOrigin;

private:
	C_LocalTempEntity( const C_LocalTempEntity & );

	Vector							m_vecTempEntVelocity;
	Vector							m_vecPrevLocalOrigin;

	Vector							m_vecTempEntAcceleration;

	// Draw tempent as a studio model
	int								DrawStudioModel( int flags );

};

#endif // TEMPENTITY_H