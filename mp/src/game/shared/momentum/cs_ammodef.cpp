//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "cs_ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
void CCSAmmoDef::AddAmmoCost( char const* name, int cost, int buySize )
{
	int index = Index( name );
	if ( index < 1 || index >= m_nAmmoIndex )
		return;

	m_csAmmo[index].buySize = buySize;
	m_csAmmo[index].cost = cost;
}


//-----------------------------------------------------------------------------
int CCSAmmoDef::GetBuySize( int index ) const
{
	if ( index < 1 || index >= m_nAmmoIndex )
		return 0;

	return m_csAmmo[index].buySize;
}


//-----------------------------------------------------------------------------
int CCSAmmoDef::GetCost( int index ) const
{
	if ( index < 1 || index >= m_nAmmoIndex )
		return 0;

	return m_csAmmo[index].cost;
}


//-----------------------------------------------------------------------------
CCSAmmoDef::CCSAmmoDef(void)
{
	memset( m_csAmmo, 0, sizeof( m_csAmmo ) );
}


//-----------------------------------------------------------------------------
CCSAmmoDef::~CCSAmmoDef( void )
{
}

//-----------------------------------------------------------------------------
