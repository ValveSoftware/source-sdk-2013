//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ITHREADEDTCPSOCKET_H
#define ITHREADEDTCPSOCKET_H
#ifdef _WIN32
#pragma once
#endif


#include "iphelpers.h"


class IThreadedTCPSocket;


class CTCPPacket
{
public:
	// Access the contents of the packet.
	const char* GetData() const;
	int GetLen() const;

	// You can attach some user data to the packet.
	int GetUserData() const;
	void SetUserData( int userData );

	// Free resources associated with the packet.
	void Release();

public:
	friend class CThreadedTCPSocket;
	~CTCPPacket(); // Use Release(), not delete.

	int m_UserData;
	int m_Len;
	char m_Data[1];
};


inline const char* CTCPPacket::GetData() const
{
	return m_Data;
}


inline int CTCPPacket::GetLen() const
{
	return m_Len;
}


// The application implements this to handle packets that are received.
// Note that the implementation must be thread-safe because these functions can be called
// from various threads.
class ITCPSocketHandler
{
public:

	enum
	{
		SocketError=0,
		ConnectionTimedOut
	};

	// This is called right when the socket becomes ready to have data sent through it and
	// before OnPacketReceive is ever called.
	virtual void Init( IThreadedTCPSocket *pSocket ) = 0;

	// This is called when a packet arrives. NOTE: you are responsible for freeing the packet 
	// by calling CTCPPacket::Release() on it.
	virtual void OnPacketReceived( CTCPPacket *pPacket ) = 0;

	// Handle errors inside the socket. After this is called, the socket is no longer alive.
	// Note: this might be called from ANY thread (the main thread, the send thread, or the receive thread).
	//
	// errorCode is one of the enums above (SocketError, ConnectionTimedOut, etc).
	virtual void OnError( int errorCode, const char *pErrorString ) = 0;

	virtual void Release( bool bForce = false ) = 0;

	virtual int GetConnectionID( ) = 0;
};


//
// This is the main threaded TCP socket class.
// The way these work is that they have a thread for sending and a thread for receiving data.
//
// The send thread is continually pushing your data out the door.
//
// The receive thread is continually receiving data. When it receives data, it calls your HandlePacketFn
// to allow the user to handle it. Be very careful in your HandlePacketFn, since it is in another thread.
// Anything it accesses should be protected by mutexes and the like.
//
class IThreadedTCPSocket
{
public:
	// Cleanup everything and exit.
	// Note: if the receive thread is inside your HandlePacketFn returns, this function blocks until that function returns.
	virtual void Release() = 0;

	// Returns the address of whoever you are connected to.
	virtual CIPAddr GetRemoteAddr() const = 0;	

	// Returns true if the socket is connected and ready to go. If this returns false, then the socket won't
	// send or receive data any more. It also means that your ITCPSocketHandler's OnError function has been called.
	virtual bool IsValid() = 0;

	// Send data. Any thread can call these functions, and they don't block. They make a copy of the data, then
	// enqueue it for sending.
	virtual bool Send( const void *pData, int len ) = 0;
	virtual bool SendChunks( void const * const *pChunks, const int *pChunkLengths, int nChunks ) = 0;

	virtual ITCPSocketHandler *GetHandler( ) = 0;
};



// Use these to get incoming connections.
class ITCPConnectSocket
{
public:
	// Call this to stop listening for connections and delete the object.
	virtual void Release() = 0;
	
	// Keep calling this as long as you want to wait for connections.
	//
	// If it returns true and pSocket is NULL, it means it hasn't connected yet.
	// If it returns true and pSocket is non-NULL, then it has connected.
	// If it returns false, then the connection attempt failed and all further Update() calls will return false.
	virtual bool Update( IThreadedTCPSocket **pSocket, unsigned long milliseconds=0 ) = 0;
};	


// This class is implemented by the app and passed into CreateListener. When the listener makes
// a new connection, it calls CreateNewHandler() to have the app create something that will handle
// the received packets and errors for the new socket.
class IHandlerCreator
{
public:
	// This function must return a valid value.
	virtual ITCPSocketHandler* CreateNewHandler() = 0;
	virtual bool DeleteHandlersOnTerm() { return true; }
};


// Use this to listen for TCP connections. The ITCPConnectSocket will keep returning connections 
// until you call Release().
ITCPConnectSocket* ThreadedTCP_CreateListener( 
	IHandlerCreator *pHandlerCreator,	// This handles messages from the socket.
	const unsigned short port,			// Listen on this port.
	int nQueueLength = 5				// How many connections 
	);


// Use this to connect to a remote process. After Update() returns a non-NULL value, you should
// call Release() on the ITCPConnectSocket because it won't ever return another connection.
ITCPConnectSocket* ThreadedTCP_CreateConnector( 
	const CIPAddr &addr,			// Who to connect to.
	const CIPAddr &localAddr,		// Local address to bind to. Leave uninitialized (pass in CIPAddr()) and it will 
									// an interface and a port for you. You can also just fill in the port, and it will
									// use that port and choose an interface for you.
	IHandlerCreator *pHandlerCreator// If it connects, it asks this thing to make a handler for the connection.
	);


// Enable or disable timeouts.
void ThreadedTCP_EnableTimeouts( bool bEnable );

// This should be called at init time. If set to true, it'll set the send and recv threads to low priority.
// (Default is true).
void ThreadedTCP_SetTCPSocketThreadPriorities( bool bSetTCPSocketThreadPriorities );


#endif // ITHREADEDTCPSOCKET_H
