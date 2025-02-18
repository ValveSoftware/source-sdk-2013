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
#if !defined( IVMODEMANAGER_H )
#define IVMODEMANAGER_H
#ifdef _WIN32
#pragma once
#endif

abstract_class IVModeManager
{
public:
	virtual void	Init( void ) = 0;
	// HL2 will ignore, TF2 will change modes.
	virtual void	SwitchMode( bool commander, bool force ) = 0;
	virtual void	LevelInit( const char *newmap ) = 0;
	virtual void	LevelShutdown( void ) = 0;
};

extern IVModeManager *modemanager;

#endif // IVMODEMANAGER_H