//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_RECIPIENTFILTER_H
#define C_RECIPIENTFILTER_H
#ifdef _WIN32
#pragma once
#endif

#include "irecipientfilter.h"
#include "utlvector.h"
#include "c_baseentity.h"
#include "soundflags.h"
#include "bitvec.h"

class C_BasePlayer;
class C_Team;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_RecipientFilter : public IRecipientFilter
{
public:
					C_RecipientFilter();
	virtual			~C_RecipientFilter();

	virtual bool	IsReliable( void ) const;

	virtual int		GetRecipientCount( void ) const;
	virtual int		GetRecipientIndex( int slot ) const;

	virtual bool	IsInitMessage( void ) const { return false; };

public:

	void			CopyFrom( const C_RecipientFilter& src );

	void			Reset( void );

	void			MakeReliable( void );
		
	void			AddAllPlayers( void );
	void			AddRecipientsByPVS( const Vector& origin );
	void			AddRecipientsByPAS( const Vector& origin );
	void			AddRecipient( C_BasePlayer *player );
	void			RemoveRecipient( C_BasePlayer *player );
	void			AddRecipientsByTeam( C_Team *team );
	void			RemoveRecipientsByTeam( C_Team *team );

	void			UsePredictionRules( void );
	bool			IsUsingPredictionRules( void ) const;

	bool			IgnorePredictionCull( void ) const;
	void			SetIgnorePredictionCull( bool ignore );

	void			AddPlayersFromBitMask( CBitVec< ABSOLUTE_PLAYER_LIMIT >& playerbits );

//private:

	bool				m_bReliable;
	bool				m_bInitMessage;
	CUtlVector< int >	m_Recipients;
	// If using prediction rules, the filter itself suppresses local player
	bool				m_bUsingPredictionRules;
	// If ignoring prediction cull, then external systems can determine
	//  whether this is a special case where culling should not occur
	bool				m_bIgnorePredictionCull;
};

//-----------------------------------------------------------------------------
// Purpose: Simple class to create a filter for a single player
//-----------------------------------------------------------------------------
class CSingleUserRecipientFilter : public C_RecipientFilter
{
public:
	CSingleUserRecipientFilter( C_BasePlayer *player )
	{
		AddRecipient( player );
	}
};

//-----------------------------------------------------------------------------
// Purpose: Simple class to create a filter for all players, unreliable
//-----------------------------------------------------------------------------
class CBroadcastRecipientFilter : public C_RecipientFilter
{
public:
	CBroadcastRecipientFilter( void )
	{
		AddAllPlayers();
	}
};

//-----------------------------------------------------------------------------
// Purpose: Simple class to create a filter for all players, reliable
//-----------------------------------------------------------------------------
class CReliableBroadcastRecipientFilter : public CBroadcastRecipientFilter
{
public:
	CReliableBroadcastRecipientFilter( void )
	{
		MakeReliable();
	}
};

//-----------------------------------------------------------------------------
// Purpose: Simple class to create a filter for a single player
//-----------------------------------------------------------------------------
class CPASFilter : public C_RecipientFilter
{
public:
	CPASFilter( const Vector& origin )
	{
		AddRecipientsByPAS( origin );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPASAttenuationFilter : public CPASFilter
{
public:
	CPASAttenuationFilter( C_BaseEntity *entity, float attenuation = ATTN_NORM ) :
		CPASFilter( entity->GetAbsOrigin() )
	{
	}

	CPASAttenuationFilter( const Vector& origin, float attenuation = ATTN_NORM ) :
		CPASFilter( origin )
	{
	}

	CPASAttenuationFilter( C_BaseEntity *entity, const char *lookupSound ) :
		CPASFilter( entity->GetAbsOrigin() )
	{
	}

	CPASAttenuationFilter( const Vector& origin, const char *lookupSound ) :
		CPASFilter( origin )
	{
	}

	CPASAttenuationFilter( C_BaseEntity *entity, const char *lookupSound, HSOUNDSCRIPTHANDLE& handle ) :
		CPASFilter( entity->GetAbsOrigin() )
	{
	}

	CPASAttenuationFilter( const Vector& origin, const char *lookupSound, HSOUNDSCRIPTHANDLE& handle ) :
		CPASFilter( origin )
	{
	}
};

//-----------------------------------------------------------------------------
// Purpose: Simple class to create a filter for a single player
//-----------------------------------------------------------------------------
class CPVSFilter : public C_RecipientFilter
{
public:
	CPVSFilter( const Vector& origin )
	{
		AddRecipientsByPVS( origin );
	}
};

class CLocalPlayerFilter : public C_RecipientFilter
{
public:
	CLocalPlayerFilter( void );
};

class CUIFilter : public C_RecipientFilter
{
public:
	CUIFilter( void )
	{
		m_Recipients.AddToTail( 1 );
//		AddRecipient( 0 );
	}
};


#endif // C_RECIPIENTFILTER_H
