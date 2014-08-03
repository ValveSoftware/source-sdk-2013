//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Hooks and classes for the support of humanoid NPCs with 
//			groovy facial animation capabilities, aka, "Actors"
//
//=============================================================================//

#ifndef AI_INTEREST_TARGET_H
#define AI_INTEREST_TARGET_H

#if defined( _WIN32 )
#pragma once
#endif


//-----------------------------------------------------------------------------
// CAI_BaseActor
//
// Purpose: The base class for all facially expressive NPCS.
//
//-----------------------------------------------------------------------------

class CAI_InterestTarget_t
{
public:
	enum CAI_InterestTarget_e
	{
		LOOKAT_ENTITY = 0,
		LOOKAT_POSITION, 
		LOOKAT_BOTH
	};

public:
	bool			IsThis( CBaseEntity *pThis );
	const Vector	&GetPosition( void );
	bool			IsActive( void );
	float			Interest( void );

public:
	CAI_InterestTarget_e	m_eType; // ????

	EHANDLE		m_hTarget;
	Vector		m_vecPosition;
	float		m_flStartTime;
	float		m_flEndTime;
	float		m_flRamp;
	float		m_flInterest;

	DECLARE_SIMPLE_DATADESC();
};


class CAI_InterestTarget : public CUtlVector<CAI_InterestTarget_t>
{
public:
	void Add( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp );
	void Add( const Vector &vecPosition, float flImportance, float flDuration, float flRamp );
	void Add( CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp );
	int Find( CBaseEntity *pTarget )
	{
		int i;
		for ( i = 0; i < Count(); i++)
		{
			if (pTarget == (*this)[i].m_hTarget)
				return i;
		}
		return InvalidIndex();
	}
	
	void Cleanup( void )
	{
		int i;
		for (i = Count() - 1; i >= 0; i--)
		{
			if (!Element(i).IsActive())
			{
				Remove( i );
			}
		}
	};

private:
	void Add( CAI_InterestTarget_t::CAI_InterestTarget_e type, CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp );
};


//-----------------------------------------------------------------------------
#endif // AI_INTEREST_TARGET_H
