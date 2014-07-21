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
#if !defined( IPREDICTION_H )
#define IPREDICTION_H
#ifdef _WIN32
#pragma once
#endif


#include "interface.h"
#include "mathlib/vector.h" // Solely to get at define for QAngle


class IMoveHelper;

//-----------------------------------------------------------------------------
// Purpose: Engine interface into client side prediction system
//-----------------------------------------------------------------------------
abstract_class IPrediction
{
public:
	virtual			~IPrediction( void ) {};

	virtual void	Init( void ) = 0;
	virtual void	Shutdown( void ) = 0;

	// Run prediction
	virtual void	Update
					( 
						int startframe,				// World update ( un-modded ) most recently received
						bool validframe,			// Is frame data valid
						int incoming_acknowledged,	// Last command acknowledged to have been run by server (un-modded)
						int outgoing_command		// Last command (most recent) sent to server (un-modded)
					) = 0;

	// We are about to get a network update from the server.  We know the update #, so we can pull any
	//  data purely predicted on the client side and transfer it to the new from data state.
	virtual void	PreEntityPacketReceived( int commands_acknowledged, int current_world_update_packet ) = 0;
	virtual void	PostEntityPacketReceived( void ) = 0;
	virtual void	PostNetworkDataReceived( int commands_acknowledged ) = 0;

	virtual void	OnReceivedUncompressedPacket( void ) = 0;

	// The engine needs to be able to access a few predicted values
	virtual void	GetViewOrigin( Vector& org ) = 0;
	virtual void	SetViewOrigin( Vector& org ) = 0;
	virtual void	GetViewAngles( QAngle& ang ) = 0;
	virtual void	SetViewAngles( QAngle& ang ) = 0;
	virtual void	GetLocalViewAngles( QAngle& ang ) = 0;
	virtual void	SetLocalViewAngles( QAngle& ang ) = 0;
};

extern IPrediction *g_pClientSidePrediction;

#define VCLIENT_PREDICTION_INTERFACE_VERSION	"VClientPrediction001"

#endif // IPREDICTION_H
