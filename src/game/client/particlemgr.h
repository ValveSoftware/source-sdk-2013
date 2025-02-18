//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

//
// This module implements the particle manager for the client DLL.
// In a nutshell, to create your own effect, implement the ParticleEffect 
// interface and call CParticleMgr::AddEffect to add your effect. Then you can 
// add particles and simulate and render them.

/*

Particle manager documentation
-----------------------------------------------------------------------------

All particle effects are managed by a class called CParticleMgr. It tracks 
the list of particles, manages their materials, sorts the particles, and
has callbacks to render them.

Conceptually, CParticleMgr is NOT part of VEngine's entity system. It does
not care about entities, only particle effects. Usually, the two are implemented
together, but you should be aware the CParticleMgr talks to you through its
own interfaces and does not talk to entities. Thus, it is possible to have
particle effects that are not entities.

To make a particle effect, you need two things: 

1. An implementation of the IParticleEffect interface. This is how CParticleMgr 
   talks to you for things like rendering and updating your effect.

2. A (member) variable of type CParticleEffectBinding. This allows CParticleMgr to 
   store its internal data associated with your effect.

Once you have those two things, you call CParticleMgr::AddEffect and pass them
both in. You will then get updates through IParticleEffect::Update, and you will
be asked to render your particles with IParticleEffect::SimulateAndRender.

When you want to remove the effect, call CParticleEffectBinding::SetRemoveFlag(), which
tells CParticleMgr to remove the effect next chance it gets.

Example class:

	class CMyEffect : public IParticleEffect
	{
	public:
		// Call this to start the effect by adding it to the particle manager.
		void			Start()
		{
			ParticleMgr()->AddEffect( &m_ParticleEffect, this );
		}

		// implementation of IParticleEffect functions go here...

	public:
		CParticleEffectBinding	m_ParticleEffect;
	};



How the particle effects are integrated with the entity system
-----------------------------------------------------------------------------

There are two helper classes that you can use to create particles for your
entities. Each one is useful under different conditions.

1. CSimpleEmitter is a class that does some of the dirty work of using particles.
   If you want, you can just instantiate one of these with CSimpleEmitter::Create
   and call its AddParticle functions to add particles. When you are done and 
   want to 'free' it, call its Release function rather than deleting it, and it
   will wait until all of its particles have gone away before removing itself
   (so you don't have to write code to wait for all of the particles to go away).

   In most cases, it is the easiest and most clear to use CSimpleEmitter or
   derive a class from it, then use that class from inside an entity that wants
   to make particles.

   CSimpleEmitter and derived classes handle adding themselves to the particle
   manager, tracking how many particles in the effect are active, and 
   rendering the particles.

   CSimpleEmitter has code to simulate and render particles in a generic fashion,
   but if you derive a class from it, you can override some of its behavior
   with virtuals like UpdateAlpha, UpdateScale, UpdateColor, etc..

   Example code:
		CSimpleEmitter *pEmitter = CSimpleEmitter::Create();
		
		CEffectMaterialHandle hMaterial = pEmitter->GetCEffectMaterial( "mymaterial" );
		
		for( int i=0; i < 100; i++ )
			pEmitter->AddParticle( hMaterial, RandomVector(0,10), 4 );

		pEmitter->Release();

2. Some older effects derive from C_BaseParticleEffect and implement an entity 
   and a particle system at the same time. This gets nasty and is not encouraged anymore.

*/


#ifndef PARTICLEMGR_H
#define PARTICLEMGR_H

#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystem.h"
#include "mathlib/vector.h"
#include "mathlib/vmatrix.h"
#include "mathlib/mathlib.h"
#include "iclientrenderable.h"
#include "clientleafsystem.h"
#include "tier0/fasttimer.h"
#include "utllinkedlist.h"
#include "utldict.h"
#ifdef WIN32
#include <typeinfo.h>
#else
#include <typeinfo>
#endif
#include "tier1/utlintrusivelist.h"
#include "tier1/utlstring.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class IParticleEffect;
class IClientParticleListener;
struct Particle;
class ParticleDraw;
class CMeshBuilder;
class CUtlMemoryPool;
class CEffectMaterial;
class CParticleSimulateIterator;
class CParticleRenderIterator;
class IThreadPool;
class CParticleSystemDefinition;
class CParticleMgr;
class CNewParticleEffect;
class CParticleCollection;

#define INVALID_MATERIAL_HANDLE	NULL


// Various stats, disabled
// extern int			g_nParticlesDrawn;
// extern CCycleCount	g_ParticleTimer;


class CParticleSubTexture;
class CParticleSubTextureGroup;


//-----------------------------------------------------------------------------
// The basic particle description; all particles need to inherit from this.
//-----------------------------------------------------------------------------

struct Particle
{
	Particle *m_pPrev, *m_pNext;

	// Which sub texture this particle uses (so we can get at the tcoord mins and maxs).
	CParticleSubTexture *m_pSubTexture;

	// If m_Pos isn't used to store the world position, then implement IParticleEffect::GetParticlePosition()
	Vector m_Pos;			// Position of the particle in world space
};


//-----------------------------------------------------------------------------
// This is the CParticleMgr's reference to a material in the material system.
// Particles are sorted by material.
//-----------------------------------------------------------------------------

// This indexes CParticleMgr::m_SubTextures.
typedef CParticleSubTexture* PMaterialHandle;

// Each effect stores a list of particles associated with each material. The list is 
// hashed on the IMaterial pointer.
class CEffectMaterial
{
public:
	CEffectMaterial();

public:
	// This provides the material that gets bound for this material in this effect.
	// There can be multiple subtextures all within the same CEffectMaterial.
	CParticleSubTextureGroup *m_pGroup;
	
	Particle m_Particles;
	CEffectMaterial *m_pHashedNext;
};


class CParticleSubTextureGroup
{
public:
				CParticleSubTextureGroup();
				~CParticleSubTextureGroup();

	// Even though each of the subtextures has its own material, they should all basically be 
	// the same exact material and just use different texture coordinates, so this is the 
	// material of the first subtexture that is bound.
	//
	// This is gotten from GetMaterialPage().
	IMaterial	*m_pPageMaterial;
};

// Precalculated data for each material used for particles.
// This allows us to put multiple subtextures into one VTF and sort them against each other.
class CParticleSubTexture
{
public:
	CParticleSubTexture();

	float m_tCoordMins[2];	// bbox in texel space that this particle material uses.
	float m_tCoordMaxs[2];	// Specified in the SubTextureMins/SubTextureMaxs parameter in the materials.

	// Which group does this subtexture belong to?
	CParticleSubTextureGroup *m_pGroup;
	CParticleSubTextureGroup m_DefaultGroup;	// This is used as the group if a particle's material
												// isn't using a group.

#ifdef _DEBUG
	char *m_szDebugName;
#endif
	
	IMaterial *m_pMaterial;
};

// Particle simulation list, used to determine what particles to simulate and how.
struct ParticleSimListEntry_t
{
	CNewParticleEffect* m_pNewParticleEffect;
	bool m_bBoundingBoxOnly;
};


//-----------------------------------------------------------------------------
// interface IParticleEffect:
//
// This is the interface that particles effects must implement. The effect is 
// responsible for starting itself and calling CParticleMgr::AddEffect, then it 
// will get the callbacks it needs to simulate and render the particles.
//-----------------------------------------------------------------------------

abstract_class IParticleEffect
{
// Overridables.
public:
	
	virtual			~IParticleEffect() {}
	
	// Called at the beginning of a frame to precalculate data for rendering 
	// the particles. If you manage your own list of particles and want to 
	// simulate them all at once, you can do that here and just render them in 
	// the SimulateAndRender call.
	virtual void	Update( float fTimeDelta ) {}
	
	// Called once for the entire effect before the batch of SimulateAndRender() calls.
	// For particle systems using FLAGS_CAMERASPACE (the default), effectMatrix transforms the particles from
	// world space into camera space. You can change this matrix if you want your particles relative to something
	// else like an attachment's space.
	virtual void	StartRender( VMatrix &effectMatrix ) {}

	// Simulate the particles.
	virtual bool	ShouldSimulate() const = 0;
	virtual void	SetShouldSimulate( bool bSim ) = 0;
	virtual void	SimulateParticles( CParticleSimulateIterator *pIterator ) = 0;

	// Render the particles.
	virtual void	RenderParticles( CParticleRenderIterator *pIterator ) = 0;

	// Implementing this is optional. It is called when an effect is removed. It is useful if
	// you hold onto pointers to the particles you created (so when this is called, you should
	// clean up your data so you don't reference the particles again).
	// NOTE: after calling this, the particle manager won't touch the IParticleEffect
	// or its associated CParticleEffectBinding anymore.
	virtual void	NotifyRemove() {}

	// This method notifies the effect a particle is about to be deallocated.
	// Implementations should *not* actually deallocate it.
	// NOTE: The particle effect's GetNumActiveParticles is updated BEFORE this is called
	//       so if GetNumActiveParticles returns 0, then you know this is the last particle
	//       in the system being removed.
	virtual void	NotifyDestroyParticle( Particle* pParticle ) {}

	// Fill in the origin used to sort this entity.
	// This is a world space position.
	virtual const Vector &GetSortOrigin() = 0;

	// Fill in the origin used to sort this entity.
// TODO: REMOVE THIS. ALL PARTICLE SYSTEMS SHOULD EITHER SET m_Pos IN CONJUNCTION WITH THE
// PARTICLE_LOCALSPACE FLAG, OR DO SETBBOX THEMSELVES.
	virtual const Vector *GetParticlePosition( Particle *pParticle ) { return &pParticle->m_Pos; }

	virtual const char *GetEffectName() { return "???"; } 
};

#define REGISTER_EFFECT( effect )														\
	IParticleEffect* effect##_Factory()													\
	{																					\
		return new effect;																\
	}																					\
	struct effect##_RegistrationHelper													\
	{																					\
		effect##_RegistrationHelper()													\
		{																				\
			ParticleMgr()->RegisterEffect( typeid( effect ).name(), effect##_Factory );	\
		}																				\
	};																					\
	static effect##_RegistrationHelper g_##effect##_RegistrationHelper

#define REGISTER_EFFECT_USING_CREATE( effect )											\
	IParticleEffect* effect##_Factory()													\
	{																					\
		return effect::Create( #effect ).GetObject();									\
	}																					\
	struct effect##_RegistrationHelper													\
	{																					\
		effect##_RegistrationHelper()													\
		{																				\
			ParticleMgr()->RegisterEffect( typeid( effect ).name(), effect##_Factory );	\
		}																				\
	};																					\
	static effect##_RegistrationHelper g_##effect##_RegistrationHelper


// In order to create a particle effect, you must have one of these around and
// implement IParticleEffect. Pass them both into CParticleMgr::AddEffect and you
// are good to go.
class CParticleEffectBinding : public CDefaultClientRenderable
{
	friend class CParticleMgr;
	friend class CParticleSimulateIterator;
	friend class CNewParticleEffect;

public:
	CParticleEffectBinding();
	~CParticleEffectBinding();
	

// Helper functions to setup, add particles, etc..
public:

	// Simulate all the particles.
	void			SimulateParticles( float flTimeDelta );

	// Use this to specify materials when adding particles. 
	// Returns the index of the material it found or added.
	// Returns INVALID_MATERIAL_HANDLE if it couldn't find or add a material.
	PMaterialHandle	FindOrAddMaterial( const char *pMaterialName );

	// Allocate particles. The Particle manager will automagically
	// deallocate them when the IParticleEffect SimulateAndRender() method 
	// returns false. The first argument is the size of the particle
	// structure in bytes
	Particle*		AddParticle( int sizeInBytes, PMaterialHandle pMaterial );

	// This is an optional call you can make if you want to manually manage the effect's
	// bounding box. Normally, the bounding box is managed automatically, but in certain
	// cases it is more efficient to set it manually.
	//
	// Note: this is a WORLD SPACE bounding box, even if you've used SetLocalSpaceTransform.
	//
	// After you make this call, the particle manager will no longer update the bounding
	// box automatically if bDisableAutoUpdate is true.
	void			SetBBox( const Vector &bbMin, const Vector &bbMax, bool bDisableAutoUpdate = true );
	// gets a copy of the current bbox mins/maxs in worldspace
	void			GetWorldspaceBounds( Vector *pMins, Vector *pMaxs );

	// This tells the particle manager that your particles are transformed by the specified matrix.
	// That way, it can transform the bbox defined by Particle::m_Pos into world space correctly.
	//
	// It also sets up the matrix returned by CParticleMgr::GetModelView() to include this matrix, so you
	// can do TransformParticle with it like any other particle system.
	const matrix3x4_t&	GetLocalSpaceTransform() const;
	void			SetLocalSpaceTransform( const matrix3x4_t &transform );

	// This expands the bbox to contain the specified point. Returns true if bbox changed
	bool			EnlargeBBoxToContain( const Vector &pt );

	// The EZ particle singletons use this - they don't want to be added to all the leaves and drawn through the
	// leaf system - they are specifically told to draw each frame at a certain point.
	void			SetDrawThruLeafSystem( int bDraw );

	// Some view model particle effects want to be drawn right before the view model (after everything else is 
	// drawn).
	void			SetDrawBeforeViewModel( int bDraw );

	// Call this to have the effect removed whenever it safe to do so.
	// This is a lot safer than calling CParticleMgr::RemoveEffect.
	int				GetRemoveFlag()									{ return GetFlag( FLAGS_REMOVE ); }
	void			SetRemoveFlag()									{ SetFlag( FLAGS_REMOVE, 1 ); }

	// Set this flag to tell the particle manager to simulate your particles even
	// if the particle system isn't visible. Tempents and fast effects can always use
	// this if they want since they want to simulate their particles until they go away.
	// This flag is ON by default.
	int				GetAlwaysSimulate()								{ return GetFlag( FLAGS_ALWAYSSIMULATE ); }
	void			SetAlwaysSimulate( int bAlwaysSimulate )		{ SetFlag( FLAGS_ALWAYSSIMULATE, bAlwaysSimulate ); }

	void			SetIsNewParticleSystem( void )		{ SetFlag( FLAGS_NEW_PARTICLE_SYSTEM, 1 ); }
	// Set if the effect was drawn the previous frame.
	// This can be used by particle effect classes
	// to decide whether or not they want to spawn
	// new particles - if they weren't drawn, then
	// they can 'freeze' the particle system to avoid
	// overhead.
	int				WasDrawnPrevFrame()								{ return GetFlag( FLAGS_DRAWN_PREVFRAME ); }
	void			SetWasDrawnPrevFrame( int bWasDrawnPrevFrame )	{ SetFlag( FLAGS_DRAWN_PREVFRAME, bWasDrawnPrevFrame ); }

	// When the effect is in camera space mode, then the transforms are setup such that
	// the particle vertices are specified in camera space (in CParticleDraw) rather than world space. 
	//
	// This makes it faster to specify the particles - you only have to transform the center 
	// by CParticleMgr::GetModelView then add to X and Y to build the quad.
	//
	// Effects that want to specify verts (in CParticleDraw) in world space should set this to false and
	// ignore CParticleMgr::GetModelView.
	//
	// Camera space mode is ON by default.
	int				IsEffectCameraSpace()							{ return GetFlag( FLAGS_CAMERASPACE ); }
	void			SetEffectCameraSpace( int bCameraSpace )		{ SetFlag( FLAGS_CAMERASPACE, bCameraSpace ); }

	// This tells it whether or not to apply the local transform to the matrix returned by CParticleMgr::GetModelView().
	// Usually, you'll want this, so you can just say TransformParticle( pMgr->GetModelView(), vPos ), but you may want
	// to manually apply your local transform before saying TransformParticle.
	//
	// This is ON by default.
	int				GetAutoApplyLocalTransform() const				{ return GetFlag( FLAGS_AUTOAPPLYLOCALTRANSFORM ); }
	void			SetAutoApplyLocalTransform( int b )				{ SetFlag( FLAGS_AUTOAPPLYLOCALTRANSFORM, b ); }

	// If this is true, then the bbox is calculated from particle positions. This works
	// fine if you always simulate (SetAlwaysSimulateFlag) so the system can become visible
	// if it moves into the PVS. If you don't use this, then you should call SetBBox at 
	// least once to tell the particle manager where your entity is.
	int				GetAutoUpdateBBox()							{ return GetFlag( FLAGS_AUTOUPDATEBBOX ); }
	void			SetAutoUpdateBBox( int bAutoUpdate )		{ SetFlag( FLAGS_AUTOUPDATEBBOX, bAutoUpdate ); }

	// Get the current number of particles in the effect.
	int				GetNumActiveParticles();

	// The is the max size of the particles for use in bounding	computation
	void			SetParticleCullRadius( float flMaxParticleRadius );

	// Build a list of all active particles, returns actual count filled in
	int				GetActiveParticleList( int nCount, Particle **ppParticleList );

	// detect origin/bbox changes and update leaf system if necessary
	void			DetectChanges();

private:
	// Change flags..
	void			SetFlag( int flag, int bOn )	{ if( bOn ) m_Flags |= flag; else m_Flags &= ~flag; }
	int				GetFlag( int flag ) const		{ return m_Flags & flag; }

	void			Init( CParticleMgr *pMgr, IParticleEffect *pSim );
	void			Term();

	// Get rid of the specified particle.
	void			RemoveParticle( Particle *pParticle );

	void			StartDrawMaterialParticles(
						CEffectMaterial *pMaterial,
						float flTimeDelta,
						IMesh* &pMesh,
						CMeshBuilder &builder,
						ParticleDraw &particleDraw,
						bool bWireframe );

	int				DrawMaterialParticles( 
						bool bBucketSort,
						CEffectMaterial *pMaterial, 
						float flTimeDelta,
						bool bWireframe
						 );

	void			GrowBBoxFromParticlePositions( CEffectMaterial *pMaterial, bool &bboxSet, Vector &bbMin, Vector &bbMax );

	void			RenderStart( VMatrix &mTempModel, VMatrix &mTempView );
	void			RenderEnd( VMatrix &mModel, VMatrix &mView );

	void			BBoxCalcStart( Vector &bbMin, Vector &bbMax );
	void			BBoxCalcEnd( bool bboxSet, Vector &bbMin, Vector &bbMax );
	
	void			DoBucketSort( 
						CEffectMaterial *pMaterial, 
						float *zCoords, 
						int nZCoords,
						float minZ,
						float maxZ );

	int				GetRemovalInProgressFlag()					{ return GetFlag( FLAGS_REMOVALINPROGRESS ); }
	void			SetRemovalInProgressFlag()					{ SetFlag( FLAGS_REMOVALINPROGRESS, 1 ); }

	// BBox is recalculated before it's put into the tree for the first time.
	int				GetNeedsBBoxUpdate()						{ return GetFlag( FLAGS_NEEDS_BBOX_UPDATE ); }
	void			SetNeedsBBoxUpdate( int bFirstUpdate )		{ SetFlag( FLAGS_NEEDS_BBOX_UPDATE, bFirstUpdate ); }

	// Set on creation and cleared after the first PostRender (whether or not the system was rendered).
	int				GetFirstFrameFlag()							{ return GetFlag( FLAGS_FIRST_FRAME ); }
	void			SetFirstFrameFlag( int bFirstUpdate )		{ SetFlag( FLAGS_FIRST_FRAME, bFirstUpdate ); }

	int				WasDrawn()									{ return GetFlag( FLAGS_DRAWN ); }
	void			SetDrawn( int bDrawn )						{ SetFlag( FLAGS_DRAWN, bDrawn ); }

	// Update m_Min/m_Max. Returns false and sets the bbox to the sort origin if there are no particles.
	bool			RecalculateBoundingBox();

	CEffectMaterial* GetEffectMaterial( CParticleSubTexture *pSubTexture );

// IClientRenderable overrides.
public:		

	virtual const Vector&			GetRenderOrigin( void );
	virtual const QAngle&			GetRenderAngles( void );
	virtual const matrix3x4_t &		RenderableToWorldTransform();
	virtual void					GetRenderBounds( Vector& mins, Vector& maxs );
	virtual bool					ShouldDraw( void );
	virtual bool					IsTransparent( void );
	virtual int						DrawModel( int flags );


private:

	enum
	{
		FLAGS_REMOVE =				(1<<0),	// Set in SetRemoveFlag
		FLAGS_REMOVALINPROGRESS =	(1<<1), // Set while the effect is being removed to prevent
											// infinite recursion.
		FLAGS_NEEDS_BBOX_UPDATE =	(1<<2),	// This is set until the effect's bbox has been updated once.
		FLAGS_AUTOUPDATEBBOX =		(1<<3),	// Update bbox automatically? Cleared in SetBBox.
		FLAGS_ALWAYSSIMULATE =		(1<<4), // See SetAlwaysSimulate.
		FLAGS_DRAWN =				(1<<5),	// Set if the effect is drawn through the leaf system.
		FLAGS_DRAWN_PREVFRAME =		(1<<6),	// Set if the effect was drawn the previous frame.
											// This can be used by particle effect classes
											// to decide whether or not they want to spawn
											// new particles - if they weren't drawn, then
											// they can 'freeze' the particle system to avoid
											// overhead.
		FLAGS_CAMERASPACE =			(1<<7),	// See SetEffectCameraSpace.
		FLAGS_DRAW_THRU_LEAF_SYSTEM=(1<<8),	// This is the default - do the effect's visibility through the leaf system.
		FLAGS_DRAW_BEFORE_VIEW_MODEL=(1<<9),// Draw before the view model? If this is set, it assumes FLAGS_DRAW_THRU_LEAF_SYSTEM goes off.
		FLAGS_AUTOAPPLYLOCALTRANSFORM=(1<<10), // Automatically apply the local transform to CParticleMgr::GetModelView()'s matrix.
		FLAGS_FIRST_FRAME =         (1<<11),	// Cleared after the first frame that this system exists (so it can simulate after rendering once).
		FLAGS_NEW_PARTICLE_SYSTEM=  (1<<12) // uses new particle system
	};


	VMatrix m_LocalSpaceTransform;
	bool m_bLocalSpaceTransformIdentity;	// If this is true, then m_LocalSpaceTransform is assumed to be identity.
	
	// Bounding box. Stored in WORLD space.
	Vector							m_Min;
	Vector							m_Max;

	// paramter copies to detect changes
	Vector							m_LastMin;
	Vector							m_LastMax;
	
	// The particle cull size
	float							m_flParticleCullRadius;

	// Number of active particles.
	unsigned short					m_nActiveParticles;

	// See CParticleMgr::m_FrameCode.
	unsigned short					m_FrameCode;

	// For CParticleMgr's list index.
	unsigned short					m_ListIndex;

	IParticleEffect					*m_pSim;
	CParticleMgr					*m_pParticleMgr;
	
	// Combination of the CParticleEffectBinding::FLAGS_ flags.
	int								m_Flags;

	// Materials this effect is using.
	enum { EFFECT_MATERIAL_HASH_SIZE = 8 };
	CEffectMaterial *m_EffectMaterialHash[EFFECT_MATERIAL_HASH_SIZE];
	
	// For faster iteration.
	CUtlLinkedList<CEffectMaterial*, unsigned short> m_Materials;

	// auto updates the bbox after N frames
	unsigned short					m_UpdateBBoxCounter;
};


class CParticleLightInfo
{
public:
	Vector	m_vPos;
	Vector	m_vColor;	// 0-1
	float	m_flIntensity;
};

typedef IParticleEffect* (*CreateParticleEffectFN)();

enum
{
	TOOLPARTICLESYSTEMID_INVALID = -1,
};


class CParticleMgr
{
	friend class CParticleEffectBinding;
	friend class CParticleCollection;

public:

	CParticleMgr();
	virtual			~CParticleMgr();

	// Call at init time to preallocate the bucket of particles.
	bool			Init(unsigned long nPreallocatedParticles, IMaterialSystem *pMaterial);

	// Shutdown - free everything.
	void			Term();

	void			LevelInit();

	void			RegisterEffect( const char *pEffectType, CreateParticleEffectFN func );
	IParticleEffect	*CreateEffect( const char *pEffectType );

	// Add and remove effects from the active list.
	// Note: once you call AddEffect, CParticleEffectBinding will automatically call
	//       RemoveEffect in its destructor.
	// Note: it's much safer to call CParticleEffectBinding::SetRemoveFlag instead of
	//       CParticleMgr::RemoveEffect.
	bool			AddEffect( CParticleEffectBinding *pEffect, IParticleEffect *pSim );
	void			RemoveEffect( CParticleEffectBinding *pEffect );

	void			AddEffect( CNewParticleEffect *pEffect );
	void			RemoveEffect( CNewParticleEffect *pEffect );

	// Called at level shutdown to free all the lingering particle effects (usually
	// CParticleEffect-derived effects that can linger with noone holding onto them).
	void			RemoveAllEffects();

	// This should be called at the start of the frame.
	void			IncrementFrameCode();
	
	// This updates all the particle effects and inserts them into the leaves.
	void			Simulate( float fTimeDelta );

	// This just marks effects that were drawn so during their next simulation they can know
	// if they were drawn in the previous frame.
	void			PostRender();

	// Draw the effects marked with SetDrawBeforeViewModel.
	void			DrawBeforeViewModelEffects();

	// Returns the modelview matrix
	VMatrix&		GetModelView();

	Particle		*AllocParticle( int size );
	void			FreeParticle( Particle * );

	PMaterialHandle	GetPMaterial( const char *pMaterialName );
	IMaterial*		PMaterialToIMaterial( PMaterialHandle hMaterial );

	//HACKHACK: quick fix that compensates for the fact that this system was designed to never release materials EVER.
	void RepairPMaterial( PMaterialHandle hMaterial );

	// Particles drawn with the ParticleSphere material will use this info.
	// This should be set in IParticleEffect.
	void GetDirectionalLightInfo( CParticleLightInfo &info ) const;
	void SetDirectionalLightInfo( const CParticleLightInfo &info );

	// add a class that gets notified of entity events
	void AddEffectListener( IClientParticleListener *pListener );
	void RemoveEffectListener( IClientParticleListener *pListener );

	// Tool effect ids
	int AllocateToolParticleEffectId();

	// Remove all new effects
	void RemoveAllNewEffects();

	// Should particle effects be rendered?
	void RenderParticleSystems( bool bEnable );
	bool ShouldRenderParticleSystems() const;

	// Quick profiling (counts only, not clock cycles).
	bool		m_bStatsRunning;
	int			m_nStatsFramesSinceLastAlert;

	void StatsAccumulateActiveParticleSystems();
	void StatsReset();
	void StatsSpewResults();
	void StatsNewParticleEffectDrawn ( CNewParticleEffect *pParticles );
	void StatsOldParticleEffectDrawn ( CParticleEffectBinding *pParticles );

private:
	struct RetireInfo_t
	{
		CParticleCollection *m_pCollection;
		float m_flScreenArea;
		bool m_bFirstFrame;
	};

	// Call Update() on all the effects.
	void UpdateAllEffects( float flTimeDelta );

	void UpdateNewEffects( float flTimeDelta );				// update new particle effects

	CParticleSubTextureGroup* FindOrAddSubTextureGroup( IMaterial *pPageMaterial );

	int ComputeParticleDefScreenArea( int nInfoCount, RetireInfo_t *pInfo, float *pTotalArea, CParticleSystemDefinition* pDef, 
		const CViewSetup& view, const VMatrix &worldToPixels, float flFocalDist );

	bool RetireParticleCollections( CParticleSystemDefinition* pDef, int nCount, RetireInfo_t *pInfo, float flScreenArea, float flMaxTotalArea );

	void BuildParticleSimList( CUtlVector< ParticleSimListEntry_t > &list );
	bool EarlyRetireParticleSystems( int nCount, ParticleSimListEntry_t *ppEffects );
	static int RetireSort( const void *p1, const void *p2 ); 

private:

	int m_nCurrentParticlesAllocated;

	// Directional lighting info.
	CParticleLightInfo m_DirectionalLight;

	// Frame code, used to prevent CParticleEffects from simulating multiple times per frame.
	// Their DrawModel can be called multiple times per frame because of water reflections,
	// but we only want to simulate the particles once.
	unsigned short					m_FrameCode;

	bool							m_bUpdatingEffects;
	bool							m_bRenderParticleEffects;

	// All the active effects.
	CUtlLinkedList<CParticleEffectBinding*, unsigned short>		m_Effects;

	// all the active effects using the new particle interface
	CUtlIntrusiveDList< CNewParticleEffect > m_NewEffects;

	
	CUtlVector< IClientParticleListener *> m_effectListeners;

	IMaterialSystem					*m_pMaterialSystem;

	// Store the concatenated modelview matrix
	VMatrix							m_mModelView;
	
	CUtlVector<CParticleSubTextureGroup*>				m_SubTextureGroups;	// lookup by group name
	CUtlDict<CParticleSubTexture*,unsigned short>		m_SubTextures;		// lookup by material name
	CParticleSubTexture m_DefaultInvalidSubTexture; // Used when they specify an invalid material name.

	CUtlMap< const char*, CreateParticleEffectFN > m_effectFactories;

	int m_nToolParticleEffectId;

	IThreadPool *m_pThreadPool[2];
};

inline int CParticleMgr::AllocateToolParticleEffectId()
{
	return m_nToolParticleEffectId++;
}

// Implement this class and register with CParticleMgr to receive particle effect add/remove notification
class IClientParticleListener
{
public:
	virtual void OnParticleEffectAdded( IParticleEffect *pEffect ) = 0;
	virtual void OnParticleEffectRemoved( IParticleEffect *pEffect ) = 0;
};



// Helper functions to abstract out the particle testbed app.
float	Helper_GetTime();
float	Helper_GetFrameTime();
float	Helper_RandomFloat( float minVal, float maxVal );
int		Helper_RandomInt( int minVal, int maxVal );



// ------------------------------------------------------------------------ //
// CParticleMgr inlines
// ------------------------------------------------------------------------ //

inline VMatrix& CParticleMgr::GetModelView()
{
	return m_mModelView;
}



// ------------------------------------------------------------------------ //
// CParticleEffectBinding inlines.
// ------------------------------------------------------------------------ //

inline const matrix3x4_t& CParticleEffectBinding::GetLocalSpaceTransform() const
{
	return m_LocalSpaceTransform.As3x4();
}



// ------------------------------------------------------------------------ //
// GLOBALS
// ------------------------------------------------------------------------ //

CParticleMgr *ParticleMgr();




//-----------------------------------------------------------------------------
// StandardParticle_t; this is just one type of particle
// effects may implement their own particle data structures
//-----------------------------------------------------------------------------

struct StandardParticle_t : public Particle
{
	// Color and alpha values are 0 - 1
	void			SetColor(float r, float g, float b);
	void			SetAlpha(float a);

	Vector			m_Velocity;
	
	// How this is used is up to the effect's discretion. Some use it for how long it has been alive
	// and others use it to count down until the particle disappears.
	float			m_Lifetime;

	unsigned char	m_EffectData;	// Data specific to the IParticleEffect. This can be used to distinguish between
									// different types of particles the effect is simulating.
	unsigned short	m_EffectDataWord;

	unsigned char	m_Color[4];		// RGBA - not all effects need to use this.
};


// ------------------------------------------------------------------------ //
// Transform a particle.
// ------------------------------------------------------------------------ //

inline void TransformParticle(const VMatrix &vMat, const Vector &vIn, Vector &vOut)
{
	//vOut = vMat.VMul4x3(vIn);
	vOut.x = vMat.m[0][0]*vIn.x + vMat.m[0][1]*vIn.y + vMat.m[0][2]*vIn.z + vMat.m[0][3];
	vOut.y = vMat.m[1][0]*vIn.x + vMat.m[1][1]*vIn.y + vMat.m[1][2]*vIn.z + vMat.m[1][3];
	vOut.z = vMat.m[2][0]*vIn.x + vMat.m[2][1]*vIn.y + vMat.m[2][2]*vIn.z + vMat.m[2][3];
}


// ------------------------------------------------------------------------ //
// CEffectMaterial inlines
// ------------------------------------------------------------------------ //

inline void StandardParticle_t::SetColor(float r, float g, float b)
{
	m_Color[0] = (unsigned char)(r * 255.9f);
	m_Color[1] = (unsigned char)(g * 255.9f);
	m_Color[2] = (unsigned char)(b * 255.9f);
}

inline void StandardParticle_t::SetAlpha(float a)
{
	m_Color[3] = (unsigned char)(a * 255.9f);
}



//-----------------------------------------------------------------------------
// List functions.
//-----------------------------------------------------------------------------

inline void UnlinkParticle( Particle *pParticle )
{
	pParticle->m_pPrev->m_pNext = pParticle->m_pNext;
	pParticle->m_pNext->m_pPrev = pParticle->m_pPrev;
}

inline void InsertParticleBefore( Particle *pInsert, Particle *pNext )
{
	// link pCur before pPrev
	pInsert->m_pNext = pNext;
	pInsert->m_pPrev = pNext->m_pPrev;
	pInsert->m_pNext->m_pPrev = pInsert->m_pPrev->m_pNext = pInsert;
}

inline void InsertParticleAfter( Particle *pInsert, Particle *pPrev )
{
	pInsert->m_pPrev = pPrev;
	pInsert->m_pNext = pPrev->m_pNext;

	pInsert->m_pNext->m_pPrev = pInsert->m_pPrev->m_pNext = pInsert;
}

inline void SwapParticles( Particle *pPrev, Particle *pCur )
{
	// unlink pCur
	UnlinkParticle( pCur );
	InsertParticleBefore( pCur, pPrev );
}


#include "particle_iterators.h"


#endif


