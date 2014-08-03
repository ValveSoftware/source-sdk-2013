//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HUDTEXTUREHANDLE_H
#define HUDTEXTUREHANDLE_H
#ifdef _WIN32
#pragma once
#endif

class CHudTexture;

class CHudTextureHandle
{
public:
	CHudTextureHandle()
	{
		m_pValue = NULL;
	}

	// Assign a value to the handle.
	const CHudTextureHandle& operator=( const CHudTexture *t )
	{
		m_pValue = (CHudTexture *)t;
		return *this;
	}

	void Set( CHudTexture *t )
	{
		m_pValue = t;
	}

	CHudTexture *Get()
	{
		return m_pValue;
	}

	operator CHudTexture*()
	{
		return m_pValue;
	}

	operator CHudTexture*() const
	{
		return m_pValue;
	}

	CHudTexture	*operator->() const
	{
		return m_pValue;
	}

private:
	CHudTexture *m_pValue;
};

#endif // HUDTEXTUREHANDLE_H
