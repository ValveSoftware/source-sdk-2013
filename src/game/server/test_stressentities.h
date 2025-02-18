//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TEST_STRESSENTITIES_H
#define TEST_STRESSENTITIES_H
#ifdef _WIN32
#pragma once
#endif


class CBaseEntity;


typedef CBaseEntity* (*StressEntityFn)();	// Function to create an entity for the stress test.


// Each game DLL can instantiate these to register types of entities it can create
// for the entity stress test.
class CStressEntityReg
{
public:

							CStressEntityReg( StressEntityFn fn )
							{
								m_pFn = fn;
								m_pNext = s_pHead;
								s_pHead = this;
							}

	static CStressEntityReg*GetListHead()	{ return s_pHead; }
	CStressEntityReg*		GetNext()		{ return m_pNext; }
	StressEntityFn			GetFn()			{ return m_pFn; }


private:
	static CStressEntityReg	*s_pHead;	// List of all CStressEntityReg's.
	CStressEntityReg		*m_pNext;
	StressEntityFn			m_pFn;
};


// Use this macro to register a function to create stresstest entities.
#define REGISTER_STRESS_ENTITY( fnName ) static CStressEntityReg s_##fnName##__( fnName );


// Helper function for the functions that create the stress entities.
// Moves the entity to a random place in the level and returns the entity.
CBaseEntity*	MoveToRandomSpot( CBaseEntity *pEnt );
Vector			GetRandomSpot();


#endif // TEST_STRESSENTITIES_H
