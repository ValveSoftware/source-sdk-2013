//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "clientsideeffects.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool g_FXCreationAllowed = false;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void SetFXCreationAllowed( bool state )
{
	g_FXCreationAllowed = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool FXCreationAllowed( void )
{
	return g_FXCreationAllowed;
}

// TODO:  Sort effects and their children back to front from view positon?  At least with buckets or something.

//
//-----------------------------------------------------------------------------
// Purpose: Construct and activate effect
// Input  : *name - 
//-----------------------------------------------------------------------------
CClientSideEffect::CClientSideEffect( const char *name )
{
	m_pszName = name;
	Assert( name );

	m_bActive = true;
}

//-----------------------------------------------------------------------------
// Purpose: Destroy effect
//-----------------------------------------------------------------------------
CClientSideEffect::~CClientSideEffect( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get name of effect
// Output : const char
//-----------------------------------------------------------------------------
const char *CClientSideEffect::GetName( void )
{
	return m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: Set the name of effect
// Input : const char
//-----------------------------------------------------------------------------
void CClientSideEffect::SetEffectName( const char *pszName )
{
	m_pszName = pszName;
}

//-----------------------------------------------------------------------------
// Purpose: Is effect still active?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CClientSideEffect::IsActive( void )
{
	return m_bActive;
}

//-----------------------------------------------------------------------------
// Purpose: Mark effect for destruction
//-----------------------------------------------------------------------------
void CClientSideEffect::Destroy( void )
{
	m_bActive = false;
}

#define MAX_EFFECTS 256

//-----------------------------------------------------------------------------
// Purpose: Implements effects list interface
//-----------------------------------------------------------------------------
class CEffectsList : public IEffectsList
{
public:
					CEffectsList( void );
	virtual			~CEffectsList( void );

	//	Add an effect to the effects list
	void			AddEffect( CClientSideEffect *effect );
	// Remove the specified effect
	void			RemoveEffect( CClientSideEffect *effect );
	// Draw/update all effects in the current list
	void			DrawEffects( double frametime );
	// Flush out all effects from the list
	void			Flush( void );
private:
	void			RemoveEffect( int effectIndex );
	// Current number of effects
	int				m_nEffects;
	// Pointers to current effects
	CClientSideEffect *m_rgEffects[ MAX_EFFECTS ];
};

// Implements effects list and exposes interface
static CEffectsList g_EffectsList;
// Public interface
IEffectsList *clienteffects = ( IEffectsList * )&g_EffectsList;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEffectsList::CEffectsList( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEffectsList::~CEffectsList( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Add effect to effects list
// Input  : *effect - 
//-----------------------------------------------------------------------------
void CEffectsList::AddEffect( CClientSideEffect *effect )
{
#if 0
	if ( FXCreationAllowed() == false )
	{
		//NOTENOTE: If you've hit this, you may not add a client effect where you have attempted to.
		//			Most often this means that you have added it in an entity's DrawModel function.
		//			Move this to the ClientThink function instead!

		Assert(0);
		return;
	}
#endif

	if ( effect == NULL )
		return;

	if ( m_nEffects >= MAX_EFFECTS )
	{
		DevWarning( 1, "No room for effect %s\n", effect->GetName() );
		return;
	}

	m_rgEffects[ m_nEffects++ ] = effect;
}

//-----------------------------------------------------------------------------
void CEffectsList::RemoveEffect( CClientSideEffect *effect ) 
{
	Assert( effect );
	CClientSideEffect **end = &m_rgEffects[m_nEffects];
	for( CClientSideEffect **p = &m_rgEffects[0]; p < end; ++p)
	{
		if ( *p == effect )
		{
			RemoveEffect( p - &m_rgEffects[0] ); // todo remove this crutch
			return;
		}
	}

	Assert( false ); // don't know this effect
}

//-----------------------------------------------------------------------------
// Purpose: Remove specified effect by index
// Input  : effectIndex - 
//-----------------------------------------------------------------------------
void CEffectsList::RemoveEffect( int effectIndex )
{
	if ( effectIndex >= m_nEffects || effectIndex < 0 )
		return;

	CClientSideEffect *pEffect = m_rgEffects[effectIndex];
	m_nEffects--;

	if ( m_nEffects > 0 && effectIndex != m_nEffects )
	{
		// move the last one down to fill the empty slot
		m_rgEffects[effectIndex] = m_rgEffects[m_nEffects];
	}

	pEffect->Destroy();

	delete pEffect;	//FIXME: Yes, no?
}

//-----------------------------------------------------------------------------
// Purpose: Iterate through list and simulate/draw stuff
// Input  : frametime - 
//-----------------------------------------------------------------------------
void CEffectsList::DrawEffects( double frametime )
{
	VPROF_BUDGET( "CEffectsList::DrawEffects", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	int i;
	CClientSideEffect *effect;

	// Go backwards so deleting effects doesn't screw up
	for ( i = m_nEffects - 1 ; i >= 0; i-- )
	{
		effect = m_rgEffects[ i ];
		if ( !effect )
			continue;

		// Simulate
		effect->Draw( frametime );

		// Remove it if needed
		if ( !effect->IsActive() )
		{
			RemoveEffect( i );
		}
	}
}

//==================================================
// Purpose: 
// Input: 
//==================================================

void CEffectsList::Flush( void )
{
	int i;
	CClientSideEffect *effect;

	// Go backwards so deleting effects doesn't screw up
	for ( i = m_nEffects - 1 ; i >= 0; i-- )
	{
		effect = m_rgEffects[ i ];
		
		if ( effect == NULL )
			continue;

		RemoveEffect( i );
	}
}
