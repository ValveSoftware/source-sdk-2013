//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "damagemodifier.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDamageModifier::CDamageModifier()
{
	m_flModifier = 1;
	m_bDoneToMe = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDamageModifier::AddModifierToEntity( CBaseEntity *pEntity )
{
	RemoveModifier();

	pEntity->m_DamageModifiers.AddToTail( this );
	m_hEnt = pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDamageModifier::RemoveModifier()
{
	if ( m_hEnt.Get() )
	{
		m_hEnt->m_DamageModifiers.FindAndRemove( this );
		m_hEnt = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDamageModifier::SetModifier( float flScale )
{
	m_flModifier = flScale;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CDamageModifier::GetModifier() const
{
	return m_flModifier;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity* CDamageModifier::GetCharacter() const
{
	return m_hEnt.Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDamageModifier::SetDoneToMe( bool bDoneToMe )
{
	m_bDoneToMe = bDoneToMe;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDamageModifier::IsDamageDoneToMe() const
{
	return m_bDoneToMe;
}

