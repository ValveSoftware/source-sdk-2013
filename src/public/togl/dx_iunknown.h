#ifndef DX_IUNKNOWN_H
#define DX_IUNKNOWN_H

#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "tier1/utlmap.h"

struct CUnknown
{
	int	m_refcount[2];
		
	CUnknown()
	{
		m_refcount[0] = 1;
		m_refcount[1] = 0;
	};
				
	virtual	~CUnknown()
	{
	};
		
	void	AddRef( int which=0, char *comment = NULL )
	{
		m_refcount[which]++;
	};
		
	ULONG __stdcall	Release( int which=0, char *comment = NULL )
	{
		bool deleting = false;
			
		m_refcount[which]--;
		if ( (!m_refcount[0]) && (!m_refcount[1]) )
		{
			deleting = true;
		}

		if (deleting)
		{
			delete this;
			return 0;
		}
		else
		{
			return m_refcount[0];
		}
	};
		
	void	SetMark( bool markValue, char *comment=NULL )
	{
	}
};

// Note(Josh): This is gross! - Josh
#if defined(DX_TO_GL_ABSTRACTION) && !defined(USE_DXVK)
struct IUnknown
{
	int	m_refcount[2];
		
	IUnknown()
	{
		m_refcount[0] = 1;
		m_refcount[1] = 0;
	};
				
	virtual	~IUnknown()
	{
	};
		
	void	AddRef( int which=0, char *comment = NULL )
	{
		m_refcount[which]++;
	};
		
	ULONG __stdcall	Release( int which=0, char *comment = NULL )
	{
		bool deleting = false;
			
		m_refcount[which]--;
		if ( (!m_refcount[0]) && (!m_refcount[1]) )
		{
			deleting = true;
		}

		if (deleting)
		{
			delete this;
			return 0;
		}
		else
		{
			return m_refcount[0];
		}
	};
		
	void	SetMark( bool markValue, char *comment=NULL )
	{
	}
};
#endif

#endif
