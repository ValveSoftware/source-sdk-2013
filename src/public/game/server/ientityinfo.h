//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: provides an interface for plugins to query information about entities from the game dll
//
//===============================================================================================//
#ifndef IENTITYINFO_H
#define IENTITYINFO_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "pluginvariant.h"

abstract_class IEntityInfo
{
public:
	// returns true if entity is a player
	virtual const int EntityIndex() = 0;
	virtual const char *GetEntityName() = 0;
	virtual const char *GetClassname() = 0;
	virtual const char *GetModelName() = 0;
//TODO
	virtual const char *GetTargetName() = 0;
	virtual void SetModel(const char *modelName) = 0;
	virtual bool IsPlayer() = 0;
	virtual bool IsNPC() = 0;
	virtual bool IsDead() = 0;
	virtual bool IsAlive() = 0;
	virtual bool IsInWorld() = 0;
	virtual bool IsTemplate() = 0;
	virtual int GetEFlags() = 0;
	virtual void SetEFlags( int iEFlags ) = 0;
	virtual void AddEFlags( int nEFlagMask ) = 0;
	virtual bool IsEFlagSet( int EFlagMask ) = 0;

	virtual const int GetEffects( void ) = 0;
	virtual void AddEffects( int nEffects ) = 0;
	virtual void RemoveEffects( int nEffects ) = 0;
	virtual void ClearEffects( void ) = 0;
	virtual void SetEffects( int nEffects ) = 0;
	virtual bool IsEffectActive( int nEffects ) = 0;
	virtual int GetRenderMode() = 0;
	virtual void SetRenderMode( int nRenderMode ) = 0;

	virtual void SetBlocksLOS( bool bBlocksLOS ) = 0;
	virtual bool BlocksLOS( void ) = 0;

	virtual const int GetHealth() = 0;
	virtual const int GetMaxHealth() = 0;
	virtual void SetHealth( int iHealth ) = 0;
	virtual void SetMaxHealth( int iMaxHealth ) = 0;

	// returns the team the entity is on
	virtual int GetTeamIndex() = 0;
	// changes the entity to a new team (if the game dll logic allows it)
	virtual void ChangeTeam( int iTeamNum ) = 0;

	// positioning and sizes
	virtual const Vector GetAbsOrigin() = 0;
	virtual void SetAbsOrigin( Vector & vec ) = 0;
	virtual const QAngle GetAbsAngles() = 0;
	virtual void SetAbsAngles( QAngle & ang ) = 0;
	virtual const Vector GetLocalOrigin() = 0;
	virtual void SetLocalOrigin( const Vector& origin ) = 0;
	virtual const QAngle GetLocalAngles() = 0;
	virtual void SetLocalAngles( const QAngle& angles ) = 0;
	virtual const Vector GetAbsVelocity() = 0;
	virtual const Vector GetLocalVelocity() = 0;
	virtual const QAngle GetLocalAngularVelocity() = 0;
	virtual void EntityToWorldSpace( const Vector &in, Vector *pOut ) = 0;
	virtual void WorldToEntitySpace( const Vector &in, Vector *pOut ) = 0;
	virtual Vector EyePosition() = 0;
	virtual QAngle EyeAngles() = 0;
	virtual QAngle LocalEyeAngles() = 0;
	virtual Vector EarPosition() = 0;

	// returns world aligned mins/maxs of this entity
	virtual const Vector GetWorldMins() = 0;
	virtual const Vector GetWorldMaxs() = 0;
	virtual const Vector WorldSpaceCenter() = 0;

	virtual int GetWaterLevel() = 0;

	// if this entity has an owner, it returns their edict_t.
	virtual edict_t *GetOwner() = 0;
	virtual edict_t *GetParent() = 0;
	virtual edict_t *GetMoveParent() = 0;
	virtual edict_t *GetRootMoveParent() = 0;

	// if this entity is following another, returns that entities edict_t.
	virtual edict_t *GetFollowedEntity() = 0;
	virtual edict_t *GetGroundEntity() = 0; //returns the entity that this one is standing on - if set.

	// accessor to hook mod specific information about the entity.
	virtual bool		GetCustomInfo(int valueType, pluginvariant &outValue, pluginvariant options) = 0;

	// entity debugging stuff.
	virtual const char *GetDebugName() = 0;
	virtual void EntityText( int text_offset, const char *text, float flDuration, int r = 255, int g = 255, int b = 255, int a = 255 ) = 0;

	//Keyvalues
	virtual bool GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen ) = 0;


};


#define INTERFACEVERSION_ENTITYINFOMANAGER			"EntityInfoManager001"
abstract_class IEntityInfoManager
{
public:
	virtual IEntityInfo *GetEntityInfo( edict_t *pEdict ) = 0;
	virtual IEntityInfo *GetEntityInfo( int index ) = 0; //Retrieves the info 

	//Experiment..
	virtual IServerUnknown *GetServerEntity( edict_t *pEdict ) = 0;

	//-----------------------------------------------------------------------------
	// Purpose: Iterates the entities with a given classname.
	// Input  : pStartEntity - Last entity found, NULL to start a new iteration.
	//			szName - Classname to search for.
	//-----------------------------------------------------------------------------
	virtual edict_t *FindEntityByClassname( edict_t *pStartEntity, const char *szName ) = 0;

	//-----------------------------------------------------------------------------
	// Purpose: Iterates the entities with a given name.
	// Input  : pStartEntity - Last entity found, NULL to start a new iteration.
	//			szName - Name to search for.
	//-----------------------------------------------------------------------------
	virtual edict_t *FindEntityByName( edict_t *pStartEntity, const char *szName ) = 0;

	//-----------------------------------------------------------------------------
	// Purpose: Iterates the entities with a given model name.
	// Input  : pStartEntity - Last entity found, NULL to start a new iteration.
	//			szModelName - Model Name to search for.
	//-----------------------------------------------------------------------------
	virtual edict_t *FindEntityByModel( edict_t *pStartEntity, const char *szModelName ) = 0;

	//-----------------------------------------------------------------------------
	// Purpose: Used to iterate all the entities within a sphere.
	// Input  : pStartEntity - 
	//			vecCenter - 
	//			flRadius - 
	//-----------------------------------------------------------------------------
	virtual edict_t *FindEntityInSphere( edict_t *pStartEntity, const Vector &vecCenter, float flRadius ) = 0;

	virtual void GetWorldBounds( Vector &mins, Vector &maxs ) = 0;
};
#endif // IENTITYINFO_H
