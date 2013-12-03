//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_tesla.h"
#include "fx.h"
#include <bitbuf.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_Tesla, DT_Tesla, CTesla )
	RecvPropString( RECVINFO( m_SoundName ) ),
	RecvPropString( RECVINFO( m_iszSpriteName ) )
END_RECV_TABLE()


C_Tesla::C_Tesla()
{
}

void C_Tesla::ReceiveMessage( int classID, bf_read &msg )
{
	CTeslaInfo teslaInfo;

	msg.ReadBitVec3Coord( teslaInfo.m_vPos );
	teslaInfo.m_vAngles.Init();

	teslaInfo.m_nEntIndex = msg.ReadShort();
	teslaInfo.m_flRadius = msg.ReadFloat();

	teslaInfo.m_vColor.x = ((unsigned char)msg.ReadChar()) / 255.0f;
	teslaInfo.m_vColor.y = ((unsigned char)msg.ReadChar()) / 255.0f;
	teslaInfo.m_vColor.z = ((unsigned char)msg.ReadChar()) / 255.0f;
	
	float flAlpha = 0;
	flAlpha = ((unsigned char)msg.ReadChar()) / 255.0f;

	teslaInfo.m_nBeams = msg.ReadChar();
	
	teslaInfo.m_flBeamWidth = msg.ReadFloat();
	teslaInfo.m_flTimeVisible = msg.ReadFloat();
	teslaInfo.m_pszSpriteName = m_iszSpriteName;

	EmitSound( m_SoundName );

	m_QueuedCommands.AddToTail( teslaInfo );
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


void C_Tesla::ClientThink()
{
	FOR_EACH_LL( m_QueuedCommands, i )
	{
		FX_Tesla( m_QueuedCommands[i] );
	}
	m_QueuedCommands.Purge();
	SetNextClientThink( CLIENT_THINK_NEVER );
}


