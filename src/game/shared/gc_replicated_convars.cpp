//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================

#include "cbase.h"

#include "base_gcmessages.pb.h"
#include "convar.h"
#include "gcsdk/gcclientjob.h"
#include "gcsdk/jobmgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace GCSDK;

//=============================================================================

class CGCReplicateConVars : public CGCClientJob
{
public:
	CGCReplicateConVars( CGCClient *pClient ) : CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		CProtoBufMsg< CMsgReplicateConVars > msg ( pNetPacket );
		for ( int i = 0; i < msg.Body().convars_size(); ++i )
		{
			const CMsgConVarValue &updatedConVar = msg.Body().convars( i );
			ConVar *pVar = g_pCVar->FindVar( updatedConVar.name().data() );
			if ( pVar )
			{
				pVar->SetValue( updatedConVar.value().data() );
			}
		}

		return true;
	}
};

//=============================================================================

GC_REG_JOB( CGCClient, CGCReplicateConVars, "CGCReplicateConVars", k_EMsgGCReplicateConVars, GCSDK::k_EServerTypeGCClient );

class CGCUpdateConVar : public CGCClientJob
{
public:
	CGCUpdateConVar( CGCClient *pClient ) : CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		CProtoBufMsg< CMsgConVarValue > msg ( pNetPacket );
		ConVar *pVar = g_pCVar->FindVar( msg.Body().name().data() );
		if ( pVar )
		{
			pVar->SetValue( msg.Body().value().data() );
		}
		return true;
	}
};

GC_REG_JOB( CGCClient, CGCUpdateConVar, "CGCUpdateConVar", k_EMsgGCConVarUpdated, GCSDK::k_EServerTypeGCClient );

