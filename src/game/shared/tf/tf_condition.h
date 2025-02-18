//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Condition Objects
//
//=============================================================================
#ifndef TF_CONDITION_H
#define TF_CONDITION_H

#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "utlstack.h"
#include "tf_shareddefs.h"

#ifdef CLIENT_DLL
	// Avoid redef warnings
	#undef CTFPlayer
	#define CTFPlayer C_TFPlayer
	class C_TFPlayer;
#endif

class CTFPlayer;
class CTFCondition;

class CTFConditionList
{
public:
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( CTFConditionList );
	DECLARE_PREDICTABLE();

	CTFConditionList();

	bool Add( ETFCond type, float duration, CTFPlayer* outer, CBaseEntity* provider = NULL );
	bool _Add( ETFCond type, float duration, CTFPlayer* outer, CBaseEntity* provider = NULL );
	bool Remove( ETFCond type, bool ignore_duration=false );
	bool _Remove( ETFCond type, bool ignore_duration=false );
	void RemoveAll();

	bool InCond( ETFCond type ) const;
	CBaseEntity *GetProvider( ETFCond type ) const;

	void Think();
	void ServerThink();

#ifdef CLIENT_DLL
	// Forwarded from player shared.
	virtual void OnPreDataChanged( void );
	virtual void OnDataChanged( CTFPlayer* outer );
	void UpdateClientConditions( CTFPlayer* outer );
#endif

private:
	CUtlVector< CTFCondition* > _conditions;

	CNetworkVar( int, _condition_bits ); // Bitfield of set conditions for fast checking.
	int _old_condition_bits;
};

class CTFCondition
{
public:
	CTFCondition( ETFCond type, float duration, CTFPlayer* outer, CBaseEntity* provider = NULL );
	virtual ~CTFCondition();

	virtual void Add( float duration );

	virtual void OnAdded() = 0;
	virtual void OnRemoved() = 0;
	virtual void OnThink() = 0;
	virtual void OnServerThink() = 0;

	// Condition Traits
	virtual bool IsHealable() { return false; }
	virtual bool UsesMinDuration() { return false; }

	ETFCond	GetType() { return _type; }
	float	GetMaxDuration() { return _max_duration; }
	void	SetMaxDuration( float val ) { _max_duration = val; }
	float	GetMinDuration() { return _min_duration; }
	void	SetMinDuration( float val ) { if ( UsesMinDuration() ) { _min_duration = val; } }
	CTFPlayer* GetOuter() { return _outer; }
	void	SetProvider( CBaseEntity *provider ) { _provider = provider; }
	CBaseEntity* GetProvider() { return _provider; }

private:
	float			_min_duration;
	float			_max_duration;
	const ETFCond	_type;
	CTFPlayer*		_outer;
	CHandle< CBaseEntity >	_provider;
};

class CTFCondition_CritBoost : public CTFCondition
{
public:
	CTFCondition_CritBoost( ETFCond type, float duration, CTFPlayer* outer, CBaseEntity* provider = NULL );

	virtual void OnAdded();
	virtual void OnRemoved();
	virtual void OnThink();
	virtual void OnServerThink();

	// Condition Traits
	virtual bool IsHealable() { return false; }
	virtual bool UsesMinDuration() { return true; }
};

#endif
