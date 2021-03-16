//========= Copyright © 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef C_MOVIE_DISPLAY_H
#define C_MOVIE_DISPLAY_H

#include "cbase.h"

class C_MovieDisplay : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_MovieDisplay, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_MovieDisplay();
	~C_MovieDisplay();

	bool IsEnabled( void ) const { return m_bEnabled; }
	bool IsLooping( void ) const { return m_bLooping; }

	const char *GetMovieFilename( void ) const { return m_szMovieFilename; }
	const char *GetGroupName( void ) const { return m_szGroupName; }

private:
	bool	m_bEnabled;
	bool	m_bLooping;
	char	m_szMovieFilename[128];
	char	m_szGroupName[128];
};

#endif //C_MOVIE_DISPLAY_H