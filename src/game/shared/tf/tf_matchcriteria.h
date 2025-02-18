//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef TF_MATCH_CRITERIA_H
#define TF_MATCH_CRITERIA_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_gcmessages.pb.h"
#include "tier0/dbg.h"

class CTFGCClientSystem;
class CMvMMissionSet;

//
// PerPlayerMatchCriteria
//
// Base classe
//   CTFPerPlayerMatchCriteriaProto
//    - The proto object
//
// Interfaces:
//   ITFPerPlayerMatchCriteriaReader
//    - Interface to reading match criteria
//   ITFPerPlayerMatchCriteria : public ITFPerPlayerMatchCriteriaReader
//    - Expanded interface to match criteria w/setters
//
// Impl classes:
//   CTFPerPlayerMatchCriteria
//    - Simple class that owns the proto and provides ITFPerPlayerMatchCriteria
//   RefTFPerPlayerMatchCriteria
//   ConstRefTFPerPlayerMatchCriteria ( ITFPerPlayerMatchCriteriaReader only )
//    - Wrapper classes that reference a proto object elsewhere, and can be constructed on the fly as a helper
class ITFPerPlayerMatchCriteriaReader
{
public:
	virtual ~ITFPerPlayerMatchCriteriaReader() {};

	// Check if the local player is doubling down
	bool GetSquadSurplus() const;

	// Implementor provides
	virtual const CTFPerPlayerMatchCriteriaProto &Proto() const = 0;
};

class ITFPerPlayerMatchCriteria : public ITFPerPlayerMatchCriteriaReader
{
public:
	virtual ~ITFPerPlayerMatchCriteria() {};

	// Check if the local player is doubling down
	void SetSquadSurplus( bool bSquadSurplus );

	bool MakeDelta( const ITFPerPlayerMatchCriteriaReader& msgBase, const ITFPerPlayerMatchCriteriaReader& msgFinal );
	bool ApplyDelta( const ITFPerPlayerMatchCriteriaReader& msgDelta );

	void Clear() { MutProto().Clear(); }

	ITFPerPlayerMatchCriteria &operator=( const CTFPerPlayerMatchCriteriaProto &other )
		{ MutProto() = other; return *this; }
	ITFPerPlayerMatchCriteria &operator=( const ITFPerPlayerMatchCriteriaReader &other )
		{ MutProto() = other.Proto(); return *this; }

protected:
	// Implementor provides
	virtual CTFPerPlayerMatchCriteriaProto &MutProto() = 0;
};

//
// GroupMatchCriteria
//
// Base classe
//   CTFGroupMatchCriteriaProto
//    - The proto object
//
// Interfaces:
//   ITFGroupMatchCriteriaReader
//    - Interface to reading match criteria
//   ITFGroupMatchCriteria : public ITFGroupMatchCriteriaReader
//    - Expanded interface to match criteria w/setters
//
// Impl classes:
//   CTFGroupMatchCriteria
//    - Simple class that owns the proto and provides ITFGroupMatchCriteria
//   RefTFGroupMatchCriteria
//   ConstRefTFGroupMatchCriteria ( ITFGroupMatchCriteriaReader only )
//    - Wrapper classes that reference a proto object elsewhere, and can be constructed on the fly as a helper
class ITFGroupMatchCriteriaReader
{
public:
	virtual ~ITFGroupMatchCriteriaReader() {};

	void GetMvMMissionSet( CMvMMissionSet &challenges, bool bMannup ) const;
	bool GetLateJoin() const;
	uint32_t GetCustomPingTolerance() const;

	bool IsCasualMapSelected( uint32 nMapDefIndex ) const;

	// Get or set the current criteria from a helper object
	CCasualCriteriaHelper GetCasualCriteriaHelper() const;

	// Save/load the current casual criteria to/from a file.
	void SaveCasualCriteriaToFile( const char *pszFileName ) const;

#ifdef USE_MVM_TOUR
	int GetMannUpTourIndex() const;
#endif // USE_MVM_TOUR

	// Implementor provides these so we can can wrap non-owned
	virtual const CTFGroupMatchCriteriaProto &Proto() const = 0;
};

// Superset of the reader with setter functions, implementors need to additionally provide MutProto()
class ITFGroupMatchCriteria : public ITFGroupMatchCriteriaReader
{
public:
	virtual ~ITFGroupMatchCriteria() {};

	void SetMvMMissionSet( const CMvMMissionSet &challenges, bool bMannup );
	void SetLateJoin( bool bLateJoin );
	void SetCustomPingTolerance( uint32_t unCustomPingTolerance );

	void SetCasualMapSelected( uint32 nMapDefIndex, bool bSelected );
	void SetCasualGroupSelected( EMatchmakingGroupType eGroup, bool bSelected );
	void SetCasualCategorySelected( EGameCategory eCategory, bool bSelected );
	void ClearCasualCriteria();

	// Get or set the current criteria from a helper object
	void SetCasualCriteriaFromHelper( const CCasualCriteriaHelper &helper );

	// Save/load the current casual criteria to/from a file.
	void LoadCasualCriteriaFromFile( const char *pszFileName );

#ifdef USE_MVM_TOUR
	void SetMannUpTourIndex( int idxTour );
#endif // USE_MVM_TOUR

	// These return true if any fields were applied to/from the delta
	bool MakeDelta( const ITFGroupMatchCriteriaReader& msgBase, const ITFGroupMatchCriteriaReader& msgFinal );
	bool ApplyDelta( const ITFGroupMatchCriteriaReader& msgDelta );

	void Clear() { MutProto().Clear(); }

	ITFGroupMatchCriteria &operator=( const CTFGroupMatchCriteriaProto &other )
		{ MutProto() = other; return *this; }
	ITFGroupMatchCriteria &operator=( const ITFGroupMatchCriteriaReader &other )
		{ MutProto() = other.Proto(); return *this; }

	// Implementor provides these so we can can wrap non-owned
	virtual const CTFGroupMatchCriteriaProto &Proto() const = 0;

protected:
	virtual CTFGroupMatchCriteriaProto &MutProto() = 0;
};

//
// Generic impls
//


// Deduplicate providing Proto/MutProto/Assignment for implementors that have an m_proto ref or object
#define _MATCHCRITERIA_IMPL( classname, criterianame ) \
		virtual const CTF##criterianame##MatchCriteriaProto &Proto() const final override { return m_proto; }

#define _MATCHCRITERIA_IMPL_MUTABLE( classname, criterianame )                  \
	_MATCHCRITERIA_IMPL( classname, criterianame )                              \
	classname &operator=( const CTF##criterianame##MatchCriteriaProto &other )  \
		{ MutProto() = other; return *this; }                                   \
	classname &operator=( const ITF##criterianame##MatchCriteriaReader &other ) \
		{ MutProto() = other.Proto(); return *this; }                           \
	protected:                                                                  \
		virtual CTF##criterianame##MatchCriteriaProto &MutProto() final override { return m_proto; }

/// PerPlayerMatchCriteria

//-----------------------------------------------------------------------------
// Basic implementor that just owns the proto
class CTFPerPlayerMatchCriteria : public ITFPerPlayerMatchCriteria
{
public:
	_MATCHCRITERIA_IMPL_MUTABLE( CTFPerPlayerMatchCriteria, PerPlayer )
private:
	CTFPerPlayerMatchCriteriaProto m_proto;
};

//-----------------------------------------------------------------------------
// Wrapper implementor that holds a reference
class RefTFPerPlayerMatchCriteria : public ITFPerPlayerMatchCriteria
{
public:
	RefTFPerPlayerMatchCriteria( CTFPerPlayerMatchCriteriaProto &proto ) : m_proto( proto ) {};
	_MATCHCRITERIA_IMPL_MUTABLE( RefTFPerPlayerMatchCriteria, PerPlayer )
private:
	CTFPerPlayerMatchCriteriaProto &m_proto;
};

//-----------------------------------------------------------------------------
// Const version of ref implementor, only exposes reader interfaces
class ConstRefTFPerPlayerMatchCriteria : public ITFPerPlayerMatchCriteriaReader
{
public:
	ConstRefTFPerPlayerMatchCriteria( const CTFPerPlayerMatchCriteriaProto &proto ) : m_proto( proto ) {};
	_MATCHCRITERIA_IMPL( ConstRefTFPerPlayerMatchCriteria, PerPlayer )
private:
	const CTFPerPlayerMatchCriteriaProto &m_proto;
};

/// GroupMatchCriteria

//-----------------------------------------------------------------------------
// Basic implementor that just owns the proto
class CTFGroupMatchCriteria : public ITFGroupMatchCriteria
{
public:
	_MATCHCRITERIA_IMPL_MUTABLE( CTFGroupMatchCriteria, Group )
private:
	CTFGroupMatchCriteriaProto m_proto;
};

//-----------------------------------------------------------------------------
// Wrapper implementor that holds a reference
class RefTFGroupMatchCriteria : public ITFGroupMatchCriteria
{
public:
	RefTFGroupMatchCriteria( CTFGroupMatchCriteriaProto &proto ) : m_proto( proto ) {};
	_MATCHCRITERIA_IMPL_MUTABLE( RefTFGroupMatchCriteria, Group )
private:
	CTFGroupMatchCriteriaProto &m_proto;
};

//-----------------------------------------------------------------------------
// Const version of ref implementor, only exposes reader interfaces
class ConstRefTFGroupMatchCriteria : public ITFGroupMatchCriteriaReader
{
public:
	ConstRefTFGroupMatchCriteria( const CTFGroupMatchCriteriaProto &proto ) : m_proto( proto ) {};
	ConstRefTFGroupMatchCriteria( const CTFGroupMatchCriteria &other ) : m_proto( other.Proto() ) {};
	_MATCHCRITERIA_IMPL( ConstRefTFGroupMatchCriteria, Group )
private:
	const CTFGroupMatchCriteriaProto &m_proto;
};

#undef _MATCHCRITERIA_IMPL
#undef _MATCHCRITERIA_IMPL_MUTABLE

#endif // TF_MATCH_CRITERIA_H
