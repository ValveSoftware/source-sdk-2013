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
#if !defined( INETMSGHANDLER_H )
#define INETMSGHANDLER_H
#ifdef _WIN32
#pragma once
#endif

class	INetChannel;
typedef struct netpacket_s netpacket_t;

class INetChannelHandler
{
public:
	virtual	~INetChannelHandler( void ) {};

	virtual void ConnectionStart(INetChannel *chan) = 0;	// called first time network channel is established

	virtual void ConnectionClosing(const char *reason) = 0; // network channel is being closed by remote site

	virtual void ConnectionCrashed(const char *reason) = 0; // network error occured

	virtual void PacketStart(int incoming_sequence, int outgoing_acknowledged) = 0;	// called each time a new packet arrived

	virtual void PacketEnd( void ) = 0; // all messages has been parsed

	virtual void FileRequested(const char *fileName, unsigned int transferID ) = 0; // other side request a file for download

	virtual void FileReceived(const char *fileName, unsigned int transferID ) = 0; // we received a file
	
	virtual void FileDenied(const char *fileName, unsigned int transferID ) = 0;	// a file request was denied by other side

	virtual void FileSent(const char *fileName, unsigned int transferID ) = 0;	// we sent a file
};

#define PROCESS_NET_MESSAGE( name )	\
	virtual bool Process##name( NET_##name *msg )

#define PROCESS_SVC_MESSAGE( name )	\
	virtual bool Process##name( SVC_##name *msg )

#define PROCESS_CLC_MESSAGE( name )	\
	virtual bool Process##name( CLC_##name *msg )

#define PROCESS_MM_MESSAGE( name )	\
	virtual bool Process##name( MM_##name *msg )


#define REGISTER_NET_MSG( name )				\
	NET_##name * p##name = new NET_##name();	\
	p##name->m_pMessageHandler = this;			\
	chan->RegisterMessage( p##name );			\

#define REGISTER_SVC_MSG( name )				\
	SVC_##name * p##name = new SVC_##name();	\
	p##name->m_pMessageHandler = this;			\
	chan->RegisterMessage( p##name );			\

#define REGISTER_CLC_MSG( name )				\
	CLC_##name * p##name = new CLC_##name();	\
	p##name->m_pMessageHandler = this;			\
	chan->RegisterMessage( p##name );			\

#define REGISTER_MM_MSG( name )					\
	MM_##name * p##name = new MM_##name();		\
	p##name->m_pMessageHandler = this;			\
	chan->RegisterMessage( p##name );			\

class NET_Tick;
class NET_StringCmd;
class NET_SetConVar;
class NET_SignonState;


class INetMessageHandler 
{
public:
	virtual ~INetMessageHandler( void ) {};

	PROCESS_NET_MESSAGE( Tick ) = 0;
	PROCESS_NET_MESSAGE( StringCmd ) = 0;
	PROCESS_NET_MESSAGE( SetConVar ) = 0;
	PROCESS_NET_MESSAGE( SignonState ) = 0;
};

class CLC_ClientInfo;
class CLC_Move;
class CLC_VoiceData;
class CLC_BaselineAck;
class CLC_ListenEvents;
class CLC_RespondCvarValue;
class CLC_FileCRCCheck;
class CLC_FileMD5Check;
class CLC_SaveReplay;
class CLC_CmdKeyValues;

class IClientMessageHandler : public INetMessageHandler
{
public:
	virtual ~IClientMessageHandler( void ) {};

	PROCESS_CLC_MESSAGE( ClientInfo ) = 0;
	PROCESS_CLC_MESSAGE( Move ) = 0;
	PROCESS_CLC_MESSAGE( VoiceData ) = 0;
	PROCESS_CLC_MESSAGE( BaselineAck ) = 0;
	PROCESS_CLC_MESSAGE( ListenEvents ) = 0;
	PROCESS_CLC_MESSAGE( RespondCvarValue ) = 0;
	PROCESS_CLC_MESSAGE( FileCRCCheck ) = 0;
	PROCESS_CLC_MESSAGE( FileMD5Check ) = 0;
#if defined( REPLAY_ENABLED )
	PROCESS_CLC_MESSAGE( SaveReplay ) = 0;
#endif
	PROCESS_CLC_MESSAGE( CmdKeyValues ) = 0;
};

class SVC_Print;
class SVC_ServerInfo;
class SVC_SendTable;
class SVC_ClassInfo;
class SVC_SetPause;
class SVC_CreateStringTable;
class SVC_UpdateStringTable;
class SVC_VoiceInit;
class SVC_VoiceData;
class SVC_Sounds;
class SVC_SetView;
class SVC_FixAngle;
class SVC_CrosshairAngle;
class SVC_BSPDecal;
class SVC_GameEvent;
class SVC_UserMessage;
class SVC_EntityMessage;
class SVC_PacketEntities;
class SVC_TempEntities;
class SVC_Prefetch;
class SVC_Menu;
class SVC_GameEventList;
class SVC_GetCvarValue;
class SVC_CmdKeyValues;
class SVC_SetPauseTimed;

class IServerMessageHandler : public INetMessageHandler
{
public:
	virtual ~IServerMessageHandler( void ) {};

	// Returns dem file protocol version, or, if not playing a demo, just returns PROTOCOL_VERSION
	virtual int GetDemoProtocolVersion() const = 0;

	PROCESS_SVC_MESSAGE( Print ) = 0;
	PROCESS_SVC_MESSAGE( ServerInfo ) = 0;
	PROCESS_SVC_MESSAGE( SendTable ) = 0;
	PROCESS_SVC_MESSAGE( ClassInfo ) = 0;
	PROCESS_SVC_MESSAGE( SetPause ) = 0;
	PROCESS_SVC_MESSAGE( CreateStringTable ) = 0;
	PROCESS_SVC_MESSAGE( UpdateStringTable ) = 0;
	PROCESS_SVC_MESSAGE( VoiceInit ) = 0;
	PROCESS_SVC_MESSAGE( VoiceData ) = 0;
	PROCESS_SVC_MESSAGE( Sounds ) = 0;
	PROCESS_SVC_MESSAGE( SetView ) = 0;
	PROCESS_SVC_MESSAGE( FixAngle ) = 0;
	PROCESS_SVC_MESSAGE( CrosshairAngle ) = 0;
	PROCESS_SVC_MESSAGE( BSPDecal ) = 0;
	PROCESS_SVC_MESSAGE( GameEvent ) = 0;
	PROCESS_SVC_MESSAGE( UserMessage ) = 0;
	PROCESS_SVC_MESSAGE( EntityMessage ) = 0;
	PROCESS_SVC_MESSAGE( PacketEntities ) = 0;
	PROCESS_SVC_MESSAGE( TempEntities ) = 0;
	PROCESS_SVC_MESSAGE( Prefetch ) = 0;
	PROCESS_SVC_MESSAGE( Menu ) = 0;
	PROCESS_SVC_MESSAGE( GameEventList ) = 0;
	PROCESS_SVC_MESSAGE( GetCvarValue ) = 0;
	PROCESS_SVC_MESSAGE( CmdKeyValues ) = 0;
	PROCESS_SVC_MESSAGE( SetPauseTimed ) = 0;
};

class MM_Heartbeat;
class MM_ClientInfo;
class MM_JoinResponse;
class MM_RegisterResponse;
class MM_Migrate;
class MM_Mutelist;
class MM_Checkpoint;

class IMatchmakingMessageHandler : public INetMessageHandler
{
public:
	virtual ~IMatchmakingMessageHandler( void ) {};

	PROCESS_MM_MESSAGE( Heartbeat ) = 0;
	PROCESS_MM_MESSAGE( ClientInfo ) = 0;
	PROCESS_MM_MESSAGE( JoinResponse ) = 0;
	PROCESS_MM_MESSAGE( RegisterResponse ) = 0;
	PROCESS_MM_MESSAGE( Migrate ) = 0;
	PROCESS_MM_MESSAGE( Mutelist ) = 0;
	PROCESS_MM_MESSAGE( Checkpoint) = 0;
};

class IConnectionlessPacketHandler
{
public:
	virtual	~IConnectionlessPacketHandler( void ) {};

	virtual bool ProcessConnectionlessPacket( netpacket_t *packet ) = 0;	// process a connectionless packet
};


#endif // INETMSGHANDLER_H
