//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#if !defined( HUDELEMENT_H )
#define HUDELEMENT_H
#ifdef _WIN32
#pragma once
#endif

#include "hud.h"
#include "hud_element_helper.h"
#include "networkvar.h"
#include "GameEventListener.h"
#include "tier0/memdbgon.h"
#undef new

//-----------------------------------------------------------------------------
// Purpose: Base class for all hud elements
//-----------------------------------------------------------------------------
class CHudElement : public CGameEventListener
{
public:
	DECLARE_CLASS_NOBASE( CHudElement );
	
	// constructor - registers object in global list
								CHudElement( const char *pElementName );
	// destructor - removes object from the global list
	virtual						~CHudElement();

	// called when the Hud is initialised (whenever the DLL is loaded)
	virtual void				Init( void ) { return; }

	// called whenever the video mode changes, and whenever Init() would be called, so the hud can vid init itself
	virtual void				VidInit( void ) { return; }

	// LevelInit's called whenever a new level's starting
	virtual void				LevelInit( void ) { return; };
	// LevelShutdown's called whenever a level's finishing
	virtual void				LevelShutdown( void ) { return; };

	// called whenever the hud receives "reset" message, which is (usually) every time the client respawns after getting killed
	virtual void				Reset( void ) { return; }

	// Called once per frame for visible elements before general key processing
	virtual void				ProcessInput( void ) { return; }
	// 
	virtual const char			*GetName( void ) const { return m_pElementName; };

	// Return true if this hud element should be visible in the current hud state
	virtual bool				ShouldDraw( void );

	virtual bool				IsActive( void ) { return m_bActive; };
	virtual void				SetActive( bool bActive );

	// Hidden bits. 
	// HIDEHUD_ flags that note when this element should be hidden in the HUD
	virtual void				SetHiddenBits( int iBits );

	bool						IsParentedToClientDLLRootPanel() const;
	void						SetParentedToClientDLLRootPanel( bool parented );

	// memory handling, uses calloc so members are zero'd out on instantiation
    void *operator new( size_t stAllocateBlock )	
	{												
		Assert( stAllocateBlock != 0 );				
		void *pMem = malloc( stAllocateBlock );
		memset( pMem, 0, stAllocateBlock );
		return pMem;												
	}
	
	void* operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine )  
	{ 
		Assert( stAllocateBlock != 0 );
		void *pMem = MemAlloc_Alloc( stAllocateBlock, pFileName, nLine );
		memset( pMem, 0, stAllocateBlock );
		return pMem;												
	}

	void operator delete( void *pMem )				
	{												
#if defined( _DEBUG )
		int size = _msize( pMem );					
		memset( pMem, 0xcd, size );					
#endif
		free( pMem );								
	}

	void operator delete( void *pMem, int nBlockUse, const char *pFileName, int nLine )				
	{
		operator delete( pMem );
	}

	void SetNeedsRemove( bool needsremove );

	void RegisterForRenderGroup( const char *pszName );
	void UnregisterForRenderGroup( const char *pszGroupName );
	void HideLowerPriorityHudElementsInGroup( const char *pszGroupName );
	void UnhideLowerPriorityHudElementsInGroup( const char *pszGroupName );

	// For now, CHUdElements declare a single priority value. They will only be hidden 
	// by panels with a lower priority and will only lock out panels with a lower priority
	virtual int	GetRenderGroupPriority();

public: // IGameEventListener Interface
	
	virtual void FireGameEvent( IGameEvent * event ) {}

public:

	// True if this element is visible, and should think
	bool						m_bActive;

protected:
	int							m_iHiddenBits;

private:
	const char					*m_pElementName;
	bool						m_bNeedsRemove;
	bool						m_bIsParentedToClientDLLRootPanel;

	CUtlVector< int >			m_HudRenderGroups;
};

#include "utlpriorityqueue.h"

inline bool RenderGroupLessFunc( CHudElement * const &lhs, CHudElement * const &rhs )
{
	return ( lhs->GetRenderGroupPriority() < rhs->GetRenderGroupPriority() );
}

// hud elements declare themselves to be part of a hud render group, by name
// we register with each hudelement a list of indeces of groups they are in
// then they can query by index the state of their render group
class CHudRenderGroup
{
public:
	CHudRenderGroup()
	{
		m_pLockingElements.SetLessFunc( RenderGroupLessFunc );
		bHidden = false;
	}

	bool bHidden;
	CUtlPriorityQueue< CHudElement * >	m_pLockingElements;
};

#include "tier0/memdbgoff.h"

#endif // HUDELEMENT_H
