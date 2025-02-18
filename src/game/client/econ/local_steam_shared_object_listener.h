//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef LOCAL_STEAM_SHARED_OBJECT_LISTENER
#define LOCAL_STEAM_SHARED_OBJECT_LISTENER

#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/gcclient_sharedobjectcache.h"

class CLocalSteamSharedObjectListener : public GCSDK::ISharedObjectListener
{
public:
	CLocalSteamSharedObjectListener();
	virtual ~CLocalSteamSharedObjectListener();

	virtual void SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE {}
	virtual void PreSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE {}
	virtual void SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE {}
	virtual void PostSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE {}
	virtual void SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE {}
	virtual void SOCacheSubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE {}
	virtual void SOCacheUnsubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE {}
};

#endif // LOCAL_STEAM_SHARED_OBJECT_LISTENER
