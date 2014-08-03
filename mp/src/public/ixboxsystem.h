//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Interface to Xbox 360 system functions. Helps deal with the async system and Live
//			functions by either providing a handle for the caller to check results or handling
//			automatic cleanup of the async data when the caller doesn't care about the results.
//
//===========================================================================//

#ifndef IXBOXSYSTEM_H
#define IXBOXSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#if !defined( _X360 )
#include "xbox/xboxstubs.h"
#endif

typedef void* AsyncHandle_t;
typedef void* XboxHandle_t;

#ifdef POSIX
#define ERROR_SUCCESS 0
#define ERROR_IO_PENDING 1
#define ERROR_IO_INCOMPLETE 2
#define ERROR_INSUFFICIENT_BUFFER 3
#endif

//-----------------------------------------------------------------------------
// Xbox system interface
//-----------------------------------------------------------------------------
abstract_class IXboxSystem
{
public:
	virtual AsyncHandle_t	CreateAsyncHandle( void ) = 0;
	virtual void			ReleaseAsyncHandle( AsyncHandle_t handle ) = 0;
	virtual int				GetOverlappedResult( AsyncHandle_t handle, uint *pResultCode, bool bWait ) = 0;
	virtual void			CancelOverlappedOperation( AsyncHandle_t handle ) = 0;

	// Save/Load
	virtual void			GetModSaveContainerNames( const char *pchModName, const wchar_t **ppchDisplayName, const char **ppchName ) = 0;
	virtual uint			GetContainerRemainingSpace( void ) = 0;
	virtual bool			DeviceCapacityAdequate( DWORD nStorageID, const char *pModName ) = 0;
	virtual DWORD			DiscoverUserData( DWORD nUserID, const char *pModName ) = 0;

	// XUI
	virtual bool			ShowDeviceSelector( bool bForce, uint *pStorageID, AsyncHandle_t *pHandle  ) = 0;
	virtual void			ShowSigninUI( uint nPanes, uint nFlags ) = 0;

	// Rich Presence and Matchmaking
	virtual int				UserSetContext( uint nUserIdx, uint nContextID, uint nContextValue, bool bAsync = true, AsyncHandle_t *pHandle = NULL ) = 0;
	virtual int				UserSetProperty( uint nUserIndex, uint nPropertyId, uint nBytes, const void *pvValue, bool bAsync = true, AsyncHandle_t *pHandle = NULL ) = 0;

	// Matchmaking
	virtual int				CreateSession( uint nFlags, uint nUserIdx, uint nMaxPublicSlots, uint nMaxPrivateSlots, uint64 *pNonce, void *pSessionInfo, XboxHandle_t *pSessionHandle, bool bAsync, AsyncHandle_t *pAsyncHandle = NULL ) = 0;
	virtual uint			DeleteSession( XboxHandle_t hSession, bool bAsync, AsyncHandle_t *pAsyncHandle = NULL ) = 0;
	virtual uint			SessionSearch( uint nProcedureIndex, uint nUserIndex, uint nNumResults, uint nNumUsers, uint nNumProperties, uint nNumContexts, XUSER_PROPERTY *pSearchProperties, XUSER_CONTEXT *pSearchContexts, uint *pcbResultsBuffer, XSESSION_SEARCHRESULT_HEADER *pSearchResults, bool bAsync, AsyncHandle_t *pAsyncHandle = NULL ) = 0;
	virtual uint			SessionStart( XboxHandle_t hSession, uint nFlags, bool bAsync, AsyncHandle_t *pAsyncHandle = NULL ) = 0;
	virtual uint			SessionEnd( XboxHandle_t hSession, bool bAsync, AsyncHandle_t *pAsyncHandle = NULL ) = 0;
	virtual int				SessionJoinLocal( XboxHandle_t hSession, uint nUserCount, const uint *pUserIndexes, const bool *pPrivateSlots, bool bAsync, AsyncHandle_t *pAsyncHandle = NULL ) = 0;
	virtual int				SessionJoinRemote( XboxHandle_t hSession, uint nUserCount, const XUID *pXuids, const bool *pPrivateSlots, bool bAsync, AsyncHandle_t *pAsyncHandle = NULL ) = 0;
	virtual int				SessionLeaveLocal( XboxHandle_t hSession, uint nUserCount, const uint *pUserIndexes, bool bAsync, AsyncHandle_t *pAsyncHandle = NULL ) = 0;
	virtual int				SessionLeaveRemote( XboxHandle_t hSession, uint nUserCount, const XUID *pXuids, bool bAsync, AsyncHandle_t *pAsyncHandle = NULL ) = 0;
	virtual int				SessionMigrate( XboxHandle_t hSession, uint nUserIndex, void *pSessionInfo, bool bAsync, AsyncHandle_t *pAsyncHandle = NULL ) = 0;
	virtual int				SessionArbitrationRegister( XboxHandle_t hSession, uint nFlags, uint64 nonce, uint *pBytes, void *pBuffer, bool bAsync, AsyncHandle_t *pAsyncHandle = NULL ) = 0;

	// Stats
	virtual int				WriteStats( XboxHandle_t hSession, XUID xuid, uint nViews, void* pViews, bool bAsync, AsyncHandle_t *pAsyncHandle = NULL ) = 0;

	// Achievements
	virtual int				EnumerateAchievements( uint nUserIdx, uint64 xuid, uint nStartingIdx, uint nCount, void *pBuffer, uint nBufferBytes, bool bAsync = true, AsyncHandle_t *pAsyncHandle = NULL ) = 0;
	virtual void			AwardAchievement( uint nUserIdx, uint nAchievementId ) = 0;

	virtual void			FinishContainerWrites( void ) = 0;
	virtual uint			GetContainerOpenResult( void ) = 0;
	virtual uint			OpenContainers( void ) = 0;
	virtual void			CloseContainers( void ) = 0;
};

#define XBOXSYSTEM_INTERFACE_VERSION	"XboxSystemInterface001"

#endif // IXBOXSYSTEM_H
