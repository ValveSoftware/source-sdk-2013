//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Namespace for functions having to do with WC Edit mode
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef WCEDIT_H
#define WCEDIT_H
#pragma once

class CBaseEntity;

//=============================================================================
//	>> NWCEdit
//=============================================================================
namespace NWCEdit
{
	Vector	AirNodePlacementPosition( void );
	bool	IsWCVersionValid(void);
	void	CreateAINode(   CBasePlayer *pPlayer );
	void	DestroyAINode(  CBasePlayer *pPlayer );
	void	CreateAILink(	CBasePlayer *pPlayer );
	void	DestroyAILink(  CBasePlayer *pPlayer );
	void	UndoDestroyAINode(void);
	void	RememberEntityPosition( CBaseEntity *pEntity );
	void	UpdateEntityPosition( CBaseEntity *pEntity );
};

#endif // WCEDIT_H
