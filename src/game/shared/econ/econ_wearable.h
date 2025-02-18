//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ECON_WEARABLE_H
#define ECON_WEARABLE_H
#ifdef _WIN32
#pragma once
#endif

#include "econ_entity.h"

enum
{
	MAX_WEARABLES_SENT_FROM_SERVER =
#ifdef LOADOUT_MAX_WEARABLES_COUNT // we actually do want to just check for macro definition here -- undefined means "fall back to whatever default"
									 LOADOUT_MAX_WEARABLES_COUNT
#else
									 8 // hard-coded constant to match old behavior
#endif
};

#if defined( CLIENT_DLL )
#define CEconWearable	C_EconWearable
#define CTFWearableItem	C_TFWearableItem
#endif

enum
{
	ITEM_DROP_TYPE_NULL,
	ITEM_DROP_TYPE_NONE,
	ITEM_DROP_TYPE_DROP,
	ITEM_DROP_TYPE_BREAK,
};

class CEconWearable : public CEconEntity
{
	DECLARE_CLASS( CEconWearable, CEconEntity );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CEconWearable();

	virtual bool IsWearable( void ) const					{ return true; }

	// Shared
	virtual void			Spawn( void );
	virtual void			GiveTo( CBaseEntity *pOther );
	virtual void			RemoveFrom( CBaseEntity *pOther );
	virtual bool			CanEquip( CBaseEntity *pOther ) { return true; }
	virtual void			Equip( CBasePlayer *pOwner );
	virtual void			UnEquip( CBasePlayer* pOwner );
	virtual void			OnWearerDeath( void );
	virtual int				GetDropType( void );
//	virtual bool			UpdateBodygroups( CBasePlayer* pOwner, int iState );

	void					SetAlwaysAllow( bool bVal ) { m_bAlwaysAllow = bVal; }
	bool					AlwaysAllow( void ) { return m_bAlwaysAllow; }

	virtual bool			IsViewModelWearable( void ) { return false; }

	// Server
#if defined( GAME_DLL )
#endif

	// Client
#if defined( CLIENT_DLL )
	virtual ShadowType_t	ShadowCastType() OVERRIDE;
	virtual bool			ShouldDraw();
	virtual bool			ShouldDrawWhenPlayerIsDead() { return true; }
	virtual void			OnDataChanged( DataUpdateType_t updateType );
	virtual void			ClientThink( void );
	virtual bool			ShouldDrawParticleSystems( void );
	virtual RenderGroup_t	GetRenderGroup();
#endif

	virtual int				GetSkin( void );

	// Static
	static void				UpdateWearableBodyGroups( CBasePlayer *pPlayer );

protected:
	virtual void			InternalSetPlayerDisplayModel( void );

private:
	bool					m_bAlwaysAllow;		// Wearable will not be removed by ManageRegularWeapons. Only use this for wearables managed by other items!
};

//-----------------------------------------------------------------------------
// Purpose: For backwards compatibility with older demos
//-----------------------------------------------------------------------------
class CTFWearableItem : public CEconWearable
{
	DECLARE_CLASS( CTFWearableItem, CEconWearable );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFWearableItem();
};

#ifdef CLIENT_DLL
// Clientside wearable physics props. Used to have wearables fall off dying players.
class C_EconWearableGib	: public CEconEntity
{
	DECLARE_CLASS( C_EconWearableGib, CEconEntity );
public:
	C_EconWearableGib();
	~C_EconWearableGib();

	bool			Initialize( bool bWillBeParented );
	bool			FinishModelInitialization( void );

	virtual CStudioHdr *OnNewModel( void );

	virtual bool	ValidateEntityAttachedToPlayer( bool &bShouldRetry );

	virtual void	SpawnClientEntity();
	virtual void	Spawn();
	virtual void	ClientThink( void );
	void			StartFadeOut( float fDelay );
	virtual void	ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );
	virtual CollideType_t	GetCollideType( void ) { return ENTITY_SHOULD_RESPOND; }

	bool			UpdateThinkState( void );

private:
	bool	m_bParented;
	bool	m_bDelayedInit;
	float 	m_fDeathTime;		// Point at which this object self destructs.  
								// The default of -1 indicates the object shouldn't destruct.
};
#endif

#endif // ECON_WEARABLE_H
