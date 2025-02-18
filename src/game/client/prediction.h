//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( PREDICTION_H )
#define PREDICTION_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "iprediction.h"
#include "c_baseplayer.h"
#include "cdll_bounded_cvars.h"

class CMoveData;
class CUserCmd;

//-----------------------------------------------------------------------------
// Purpose: Implements prediction in the client .dll
//-----------------------------------------------------------------------------
class CPrediction : public IPrediction
{
// Construction
public:
	DECLARE_CLASS_GAMEROOT( CPrediction, IPrediction );

					CPrediction( void );
	virtual			~CPrediction( void );

	virtual void	Init( void );
	virtual void	Shutdown( void );

// Implement IPrediction
public:

	virtual void	Update
					( 
						int startframe,		// World update ( un-modded ) most recently received
						bool validframe,		// Is frame data valid
						int incoming_acknowledged, // Last command acknowledged to have been run by server (un-modded)
						int outgoing_command	// Last command (most recent) sent to server (un-modded)
					);

	virtual void	OnReceivedUncompressedPacket( void );

	virtual void	PreEntityPacketReceived( int commands_acknowledged, int current_world_update_packet );
	virtual void	PostEntityPacketReceived( void );
	virtual void	PostNetworkDataReceived( int commands_acknowledged );

	virtual bool	InPrediction( void ) const;
	virtual bool	IsFirstTimePredicted( void ) const;

#if !defined( NO_ENTITY_PREDICTION )
	virtual int		GetIncomingPacketNumber( void ) const;
#endif

	float			GetIdealPitch( void ) const 
	{
		return m_flIdealPitch;
	}

	// The engine needs to be able to access a few predicted values
	virtual void	GetViewOrigin( Vector& org );
	virtual void	SetViewOrigin( Vector& org );
	virtual void	GetViewAngles( QAngle& ang );
	virtual void	SetViewAngles( QAngle& ang );

	virtual void	GetLocalViewAngles( QAngle& ang );
	virtual void	SetLocalViewAngles( QAngle& ang );

	virtual void	RunCommand( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *moveHelper );

// Internal
protected:
	virtual void	SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void	FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move );
	virtual void	SetIdealPitch ( C_BasePlayer *player, const Vector& origin, const QAngle& angles, const Vector& viewheight );

	void			CheckError( int commands_acknowledged );

	// Called before and after any movement processing
	void			StartCommand( C_BasePlayer *player, CUserCmd *cmd );
	void			FinishCommand( C_BasePlayer *player );

	// Helpers to call pre and post think for player, and to call think if a think function is set
	void			RunPreThink( C_BasePlayer *player );
	void			RunThink (C_BasePlayer *ent, double frametime );
	void			CheckMovingGround( C_BasePlayer* player, double frametime );
	void			RunPostThink( C_BasePlayer *player );

private:
	virtual void	_Update
					( 
						bool received_new_world_update,
						bool validframe,		// Is frame data valid
						int incoming_acknowledged, // Last command acknowledged to have been run by server (un-modded)
						int outgoing_command	// Last command (most recent) sent to server (un-modded)
					);

	// Actually does the prediction work, returns false if an error occurred
	bool			PerformPrediction( bool received_new_world_update, C_BasePlayer *localPlayer, int incoming_acknowledged, int outgoing_command );

	void			ShiftIntermediateDataForward( int slots_to_remove, int previous_last_slot );
	void			RestoreEntityToPredictedFrame( int predicted_frame );
	int				ComputeFirstCommandToExecute( bool received_new_world_update, int incoming_acknowledged, int outgoing_command );

	void			DumpEntity( C_BaseEntity *ent, int commands_acknowledged );

	void			ShutdownPredictables( void );
	void			ReinitPredictables( void );

	void			RemoveStalePredictedEntities( int last_command_packet );
	void			RestoreOriginalEntityState( void );
	void			RunSimulation( int current_command, float curtime, CUserCmd *cmd, C_BasePlayer *localPlayer );
	void			Untouch( void );
	void			StorePredictionResults( int predicted_frame );
	bool			ShouldDumpEntity( C_BaseEntity *ent );

	void			SmoothViewOnMovingPlatform( C_BasePlayer *pPlayer, Vector& offset );

#if !defined( NO_ENTITY_PREDICTION )
// Data
protected:
	// Last object the player was standing on
	CHandle< C_BaseEntity > m_hLastGround;
private:
	bool			m_bInPrediction;
	bool			m_bFirstTimePredicted;
	bool			m_bOldCLPredictValue;
	bool			m_bEnginePaused;

	// Last network origin for local player
	int				m_nPreviousStartFrame;

	int				m_nCommandsPredicted;
	int				m_nServerCommandsAcknowledged;
	int				m_bPreviousAckHadErrors;
	int				m_bPreviousAckErrorTriggersFullLatchReset;
	CUtlVector< CHandle< CBaseEntity > > m_EntsWithPredictionErrorsInLastAck;
	int				m_nIncomingPacketNumber;

#endif
	float			m_flIdealPitch;

};
 
extern CPrediction *prediction;

#endif // PREDICTION_H
