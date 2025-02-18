//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IPROPS_H
#define IPROPS_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "mathlib/vector.h"
#include "utlvector.h"
#include "basehandle.h"


struct vcollide_t;
struct Ray_t;
class IClientRenderable;
class CGameTrace;
typedef CGameTrace trace_t;
class IVPhysicsKeyHandler;
class IPhysicsEnvironment;
class ICollideable;


//-----------------------------------------------------------------------------
// Interface versions for static props
//-----------------------------------------------------------------------------
#define INTERFACEVERSION_STATICPROPMGR_CLIENT		"StaticPropMgrClient004"
#define INTERFACEVERSION_STATICPROPMGR_SERVER		"StaticPropMgrServer002"


//-----------------------------------------------------------------------------
// Interface for static props
//-----------------------------------------------------------------------------
abstract_class IStaticPropMgr
{
public:
	// Create physics representations of props
	virtual void	CreateVPhysicsRepresentations( IPhysicsEnvironment *physenv, IVPhysicsKeyHandler *pDefaults, void *pGameData ) = 0;

	// Purpose: Trace a ray against the specified static Prop. Returns point of intersection in trace_t
	virtual void	TraceRayAgainstStaticProp( const Ray_t& ray, int staticPropIndex, trace_t& tr ) = 0;

	// Is a base handle a static prop?
	virtual bool	IsStaticProp( IHandleEntity *pHandleEntity ) const = 0;
	virtual bool	IsStaticProp( CBaseHandle handle ) const = 0;

	// returns a collideable interface to static props
	virtual ICollideable *GetStaticPropByIndex( int propIndex ) = 0;
};

abstract_class IStaticPropMgrClient : public IStaticPropMgr
{
public:
	// Recomputes the static prop opacity given a view origin
	virtual void	ComputePropOpacity( const Vector &viewOrigin, float factor ) = 0;

	// Adds decals to static props, returns point of decal in trace_t
	virtual void	AddDecalToStaticProp( const Vector& rayStart, const Vector& rayEnd,
		int staticPropIndex, int decalIndex, bool doTrace, trace_t& tr ) = 0;
	// Adds/removes shadows from static props
	virtual void	AddShadowToStaticProp( unsigned short shadowHandle, IClientRenderable* pRenderable ) = 0;
	virtual void	RemoveAllShadowsFromStaticProp( IClientRenderable* pRenderable ) = 0;

	// Gets the lighting + material color of a static prop
	virtual void	GetStaticPropMaterialColorAndLighting( trace_t* pTrace,
		int staticPropIndex, Vector& lighting, Vector& matColor ) = 0;

	//Changes made specifically to support the Portal mod (smack Dave Kircher if something breaks) (Added separately to both client and server to not mess with versioning)
	//===================================================================
	virtual void GetAllStaticProps( CUtlVector<ICollideable *> *pOutput ) = 0; //testing function that will eventually be removed
	virtual void GetAllStaticPropsInAABB( const Vector &vMins, const Vector &vMaxs, CUtlVector<ICollideable *> *pOutput ) = 0; //get all static props that exist wholly or partially in an AABB
	virtual void GetAllStaticPropsInOBB( const Vector &ptOrigin, const Vector &vExtent1, const Vector &vExtent2, const Vector &vExtent3, CUtlVector<ICollideable *> *pOutput ) = 0; //get all static props that exist wholly or partially in an OBB
	//===================================================================

	virtual void DrawStaticProps( IClientRenderable **pProps, int count, bool bShadowDepth, bool drawVCollideWireframe ) = 0;
	virtual void AddColorDecalToStaticProp( Vector const& rayStart, Vector const& rayEnd,
		int staticPropIndex, int decalIndex, bool doTrace, trace_t& tr, bool bUseColor, Color cColor ) = 0;
};

class IStaticPropMgrServer : public IStaticPropMgr
{
public:


	//Changes made specifically to support the Portal mod (smack Dave Kircher if something breaks) (Added separately to both client and server to not mess with versioning)
	//===================================================================
	virtual void GetAllStaticProps( CUtlVector<ICollideable *> *pOutput ) = 0; //testing function that will eventually be removed
	virtual void GetAllStaticPropsInAABB( const Vector &vMins, const Vector &vMaxs, CUtlVector<ICollideable *> *pOutput ) = 0; //get all static props that exist wholly or partially in an AABB
	virtual void GetAllStaticPropsInOBB( const Vector &ptOrigin, const Vector &vExtent1, const Vector &vExtent2, const Vector &vExtent3, CUtlVector<ICollideable *> *pOutput ) = 0; //get all static props that exist wholly or partially in an OBB
	//===================================================================
};


#endif // IPROPS_H
