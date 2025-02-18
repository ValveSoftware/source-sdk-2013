//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replaybrowserbasepanel.h"

//-----------------------------------------------------------------------------

using namespace vgui;

//-----------------------------------------------------------------------------

CReplayBasePanel::CReplayBasePanel( Panel *pParent, const char *pName )
:	BaseClass( pParent, pName )
{
}

void CReplayBasePanel::GetPosRelativeToAncestor( Panel *pAncestor, int &nXOut, int &nYOut )
{
	nXOut = nYOut = 0;

	Panel *pCurrent = this;
	while ( pCurrent && pCurrent != pAncestor )
	{
		int x,y;
		pCurrent->GetPos( x, y );
		nXOut += x;
		nYOut += y;
		pCurrent = pCurrent->GetParent();
	}

	Assert( pAncestor == pCurrent );
}

#endif
