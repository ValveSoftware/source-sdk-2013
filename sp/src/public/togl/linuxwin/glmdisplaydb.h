//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef GLMDISPLAYDB_H
#define	GLMDISPLAYDB_H

#include "tier1/utlvector.h"

//===============================================================================
// modes, displays, and renderers
//===============================================================================

// GLMDisplayModeInfoFields is in glmdisplay.h

class GLMDisplayMode
{
public:
	GLMDisplayModeInfoFields	m_info;
	
	GLMDisplayMode( uint width, uint height, uint refreshHz );
	GLMDisplayMode() { };
	~GLMDisplayMode( void );

	void	Init( uint width, uint height, uint refreshHz );
	void	Dump( int which );
};

//===============================================================================

// GLMDisplayInfoFields is in glmdisplay.h

class GLMDisplayInfo
{
public:
	GLMDisplayInfoFields			m_info;
	CUtlVector< GLMDisplayMode* >	*m_modes;				// starts out NULL, set by PopulateModes

	GLMDisplayInfo( void );
	~GLMDisplayInfo( void );
	
	void	PopulateModes( void );

	void	Dump( int which );
};

//===============================================================================

// GLMRendererInfoFields is in glmdisplay.h

class GLMRendererInfo
{
public:
	GLMRendererInfoFields	m_info;
	GLMDisplayInfo			*m_display;

	GLMRendererInfo			();
	~GLMRendererInfo		( void );

	void	Init( GLMRendererInfoFields *info );
	void	PopulateDisplays();
	void	Dump( int which );
};

//===============================================================================

class GLMDisplayDB
{
public:
	GLMRendererInfo	m_renderer;

	GLMDisplayDB	( void );
	~GLMDisplayDB	( void );	

	virtual void	PopulateRenderers( void );
	virtual void	PopulateFakeAdapters( uint realRendererIndex );		// fake adapters = one real adapter times however many displays are on it
	virtual void	Populate( void );
	
	// The info-get functions return false on success.
	virtual	int		GetFakeAdapterCount( void );
	virtual	bool	GetFakeAdapterInfo( int fakeAdapterIndex, int *rendererOut, int *displayOut, GLMRendererInfoFields *rendererInfoOut, GLMDisplayInfoFields *displayInfoOut );
	
	virtual	int		GetRendererCount( void );
	virtual	bool	GetRendererInfo( int rendererIndex, GLMRendererInfoFields *infoOut );
	
	virtual	int		GetDisplayCount( int rendererIndex );
	virtual	bool	GetDisplayInfo( int rendererIndex, int displayIndex, GLMDisplayInfoFields *infoOut );

	virtual	int		GetModeCount( int rendererIndex, int displayIndex );
	virtual	bool	GetModeInfo( int rendererIndex, int displayIndex, int modeIndex, GLMDisplayModeInfoFields *infoOut );
	
	virtual	void	Dump( void );
};

#endif // GLMDISPLAYDB_H
