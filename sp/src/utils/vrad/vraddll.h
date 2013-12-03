//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VRADDLL_H
#define VRADDLL_H
#ifdef _WIN32
#pragma once
#endif


#include "ivraddll.h"
#include "ilaunchabledll.h"


class CVRadDLL : public IVRadDLL, public ILaunchableDLL
{
// IVRadDLL overrides.
public:
	virtual int			main( int argc, char **argv );
	virtual bool		Init( char const *pFilename );
	virtual void		Release();
	virtual void		GetBSPInfo( CBSPInfo *pInfo );
	virtual bool		DoIncrementalLight( char const *pVMFFile );
	virtual bool		Serialize();
	virtual float		GetPercentComplete();
	virtual void		Interrupt();
};


#endif // VRADDLL_H
