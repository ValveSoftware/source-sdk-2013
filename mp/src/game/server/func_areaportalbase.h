//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef FUNC_AREAPORTALBASE_H
#define FUNC_AREAPORTALBASE_H
#ifdef _WIN32
#pragma once
#endif


#include "baseentity.h"
#include "utllinkedlist.h"


// Shared stuff between door portals and window portals.
class CFuncAreaPortalBase : public CBaseEntity
{
	DECLARE_CLASS( CFuncAreaPortalBase, CBaseEntity );
public:
	DECLARE_DATADESC();

					CFuncAreaPortalBase();
	virtual			~CFuncAreaPortalBase();

	// Areaportals must be placed in each map for preprocess, they can't use transitions
	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	
	// This is called each frame for each client to all portals to close 
	// when the viewer is far enough away, or on the backside.
	//
	// The default implementation closes the portal if the viewer (plus some padding)
	// is on the backside of the portal. Return false if you close the portal.
	//
	// Returns whether or not the (server part of) the engine was told to shut the 
	// portal off for purposes of flowing through portals to determine which areas to
	// send to the client.
	//
	//
	// bIsOpenOnClient is usually the same as the return value but NOT always. Here's why (explained
	// here in depth because this case was discovered in a lengthy debugging session):
	//
	// - Each CFuncAreaPortalBase represents two dareaportal_t's (matched by CFuncAreaPortalBase::m_portalNumber
	//   and dareaportal_t::m_PortalKey). Each dareaportal_t leads into one of the connected areas
	//   and the dareaportal_t's have opposite-facing planes.
	//
	// - The engine's SetAreaPortalState function takes a portal key and closes BOTH dareaportal_t's associated with it.
	//
	// - UpdateVisibility may decide a portal leading out of the _area you're sitting in_ can be closed
	//   for purposes of flowing through areas because you're on the backside of the dareaportal_t that 
	//   you would flow out of. 
	//
	// - At the same time, you might be able to look through the other dareaportal_t attached to
	//   that portal key (right back into the area you're standing in). 
	//
	// - An illustration:
	//
	//	 --------------------
	//	 |					|
	//	 |	|--------|aaaaaa|
	//	 |	|		 |bbbbbb| <---- aaaa and bbbb area both for PortalKey1
	//	 |**|   	 |     	|
	//	 |	|		 | area	|
	//	 |**|		 |	2   |
	//	 |	|		 |------|
	//	 |	|		 |		|
	//	 |	---------- area |
	//	 |				1   |
	//	 |------------------|
	//
	// "aaaa" and "bbbb" each represent a different dareaportal_t, (aaa leads into area 2 and
	// bbb leads into area 1). They both represent the same portal key though (call it PortalKey1).
	//
	// When standing in area 1 (where the "area 1" text is), the engine will check aaaa and it'll notice
	// that you're on the wrong side of aaaa to be looking through it, so it'll say that PortalKey1 is closed
	// for purposes of flowing through areas on the server (it turns out you can get to area 2 through
	// the portal right above the "area 1" text).
	//
	// If you told the client that PortalKey1 was closed, then you'd get a pop when moving from area 1
	// to area 2 because the client would think you couldn't see through PortalKey1 into area 1 UNTIL
	// the server transmitted the new updated PortalKey bits, which can be lagged. 
	//
	// That's why we have bIsOpenOnClient which doesn't include this backfacing test.
	//
	// .. and they all lived happily ever after.
	// The End
	// 
	//
	//
	// Note: when you're standing in the space between the **'s, then the server would stop transmitting
	// the contents of area 2 because there would be no portal you were on the correct side of to
	// see into area 2.
	virtual bool	UpdateVisibility( const Vector &vOrigin, float fovDistanceAdjustFactor, bool &bIsOpenOnClient );


public:

	// This matches two dareaportal_t::m_PortalKeys.	
	int				m_portalNumber;
	
	int				m_iPortalVersion;

private:
	
	unsigned short	m_AreaPortalsElement;	// link into g_AreaPortals.
};


extern CUtlLinkedList<CFuncAreaPortalBase*, unsigned short> g_AreaPortals;



#endif // FUNC_AREAPORTALBASE_H
