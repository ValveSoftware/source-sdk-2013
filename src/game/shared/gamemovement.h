//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( GAMEMOVEMENT_H )
#define GAMEMOVEMENT_H
#ifdef _WIN32
#pragma once
#endif

#include "igamemovement.h"
#include "cmodel.h"
#include "tier0/vprof.h"

#define CTEXTURESMAX		512			// max number of textures loaded
#define CBTEXTURENAMEMAX	13			// only load first n chars of name

#define GAMEMOVEMENT_DUCK_TIME				1000.0f		// ms
#define GAMEMOVEMENT_JUMP_TIME				510.0f		// ms approx - based on the 21 unit height jump
#define GAMEMOVEMENT_JUMP_HEIGHT			21.0f		// units
#define GAMEMOVEMENT_TIME_TO_UNDUCK			( TIME_TO_UNDUCK * 1000.0f )		// ms
#define GAMEMOVEMENT_TIME_TO_UNDUCK_INV		( GAMEMOVEMENT_DUCK_TIME - GAMEMOVEMENT_TIME_TO_UNDUCK )

enum
{
	SPEED_CROPPED_RESET = 0,
	SPEED_CROPPED_DUCK = 1,
	SPEED_CROPPED_WEAPON = 2,
};

struct surfacedata_t;

class CBasePlayer;

class CGameMovement : public IGameMovement
{
public:
	DECLARE_CLASS_NOBASE( CGameMovement );
	
	CGameMovement( void );
	virtual			~CGameMovement( void );

	virtual void	ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMove );

	virtual void	StartTrackPredictionErrors( CBasePlayer *pPlayer );
	virtual void	FinishTrackPredictionErrors( CBasePlayer *pPlayer );
	virtual void	DiffPrint( PRINTF_FORMAT_STRING char const *fmt, ... );
	virtual Vector	GetPlayerMins( bool ducked ) const;
	virtual Vector	GetPlayerMaxs( bool ducked ) const;
	virtual Vector	GetPlayerViewOffset( bool ducked ) const;

// For sanity checking getting stuck on CMoveData::SetAbsOrigin
	virtual void	TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm );
	
	// allows derived classes to exclude entities from trace
	virtual void	TryTouchGround( const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int fMask, int collisionGroup, trace_t& pm );


#define BRUSH_ONLY true
	virtual unsigned int PlayerSolidMask( bool brushOnly = false );	///< returns the solid mask for the given player, so bots can have a more-restrictive set
	CBasePlayer		*player;
	CMoveData *GetMoveData() { return mv; }
protected:
	// Input/Output for this movement
	CMoveData		*mv;
	
	int				m_nOldWaterLevel;
	float			m_flWaterEntryTime;
	int				m_nOnLadder;

	Vector			m_vecForward;
	Vector			m_vecRight;
	Vector			m_vecUp;


	// Does most of the player movement logic.
	// Returns with origin, angles, and velocity modified in place.
	// were contacted during the move.
	virtual void	PlayerMove(	void );

	// Set ground data, etc.
	void			FinishMove( void );

	virtual float	CalcRoll( const QAngle &angles, const Vector &velocity, float rollangle, float rollspeed );

	virtual	void	DecayPunchAngle( void );

	virtual void	CheckWaterJump(void );

	virtual void	WaterMove( void );

	void			WaterJump( void );

	// Handles both ground friction and water friction
	void			Friction( void );

	virtual void	AirAccelerate( Vector& wishdir, float wishspeed, float accel );

	virtual void	AirMove( void );
	virtual float	GetAirSpeedCap( void ) { return 30.f; }
	
	virtual bool	CanAccelerate();
	virtual void	Accelerate( Vector& wishdir, float wishspeed, float accel);

	// Only used by players.  Moves along the ground when player is a MOVETYPE_WALK.
	virtual void	WalkMove( void );

	// Try to keep a walking player on the ground when running down slopes etc
	void			StayOnGround( void );

	// Handle MOVETYPE_WALK.
	virtual void	FullWalkMove();

	// allow overridden versions to respond to jumping
	virtual void	OnJump( float fImpulse ) {}
	virtual void	OnLand( float fVelocity ) {}

	// Implement this if you want to know when the player collides during OnPlayerMove
	virtual void	OnTryPlayerMoveCollision( trace_t &tr ) {}

	virtual Vector	GetPlayerMins( void ) const; // uses local player
	virtual Vector	GetPlayerMaxs( void ) const; // uses local player

	typedef enum
	{
		GROUND = 0,
		STUCK,
		LADDER
	} IntervalType_t;

	virtual int		GetCheckInterval( IntervalType_t type );

	// Useful for things that happen periodically. This lets things happen on the specified interval, but
	// spaces the events onto different frames for different players so they don't all hit their spikes
	// simultaneously.
	bool			CheckInterval( IntervalType_t type );


	// Decompoosed gravity
	void			StartGravity( void );
	void			FinishGravity( void );

	// Apply normal ( undecomposed ) gravity
	void			AddGravity( void );

	// Handle movement in noclip mode.
	void			FullNoClipMove( float factor, float maxacceleration );

	// Returns true if he started a jump (ie: should he play the jump animation)?
	virtual bool	CheckJumpButton( void );	// Overridden by each game.

	// Dead player flying through air., e.g.
	virtual void    FullTossMove( void );
	
	// Player is a Observer chasing another player
	void			FullObserverMove( void );

	// Handle movement when in MOVETYPE_LADDER mode.
	virtual void	FullLadderMove();

	// The basic solid body movement clip that slides along multiple planes
	// - flSlideMultiplier controls how much of a player's velocity should be redirected along walls vs nulled out.
	//   Classic source behavior is always null-out, but some movement modes allow a redirection multiplier.
	virtual int		TryPlayerMove( Vector *pFirstDest=NULL, trace_t *pFirstTrace=NULL, float flSlideMultiplier = 0.f );

	virtual bool	LadderMove( void );
	virtual bool	OnLadder( trace_t &trace );
	virtual float	LadderDistance( void ) const { return 2.0f; }	///< Returns the distance a player can be from a ladder and still attach to it
	virtual unsigned int LadderMask( void ) const { return MASK_PLAYERSOLID; }
	virtual float	ClimbSpeed( void ) const { return MAX_CLIMB_SPEED; }
	virtual float	LadderLateralMultiplier( void ) const { return 1.0f; }

	// See if the player has a bogus velocity value.
	void			CheckVelocity( void );

	// Does not change the entities velocity at all
	void			PushEntity( Vector& push, trace_t *pTrace );

	// Slide off of the impacting object
	// returns the blocked flags:
	// 0x01 == floor
	// 0x02 == step / wall
	//
	// redirectCoeff - multiplier on the clipped velocity to apply along their new direction. 0 -> stomp into-normal
	//                 velocity, 1 -> redirect it all along new vector.
	int				ClipVelocity( Vector& in, Vector& normal, Vector& out, float overbounce, float redirectCoeff = 0.f );

	// If pmove.origin is in a solid position,
	// try nudging slightly on all axis to
	// allow for the cut precision of the net coordinates
	virtual int		CheckStuck( void );
	
	// Check if the point is in water.
	// Sets refWaterLevel and refWaterType appropriately.
	// If in water, applies current to baseVelocity, and returns true.
	virtual bool			CheckWater( void );
	
	// Determine if player is in water, on ground, etc.
	virtual void CategorizePosition( void );

	virtual void	CheckParameters( void );

	virtual	void	ReduceTimers( void );

	virtual void	CheckFalling( void );

	virtual void	PlayerRoughLandingEffects( float fvol );

	void			PlayerWaterSounds( void );

	void ResetGetPointContentsCache();
	int GetPointContentsCached( const Vector &point, int slot );

	// Ducking
	virtual void	Duck( void );
	virtual void	HandleDuckingSpeedCrop();
	virtual void	FinishUnDuck( void );
	virtual void	FinishDuck( void );
	virtual bool	CanUnduck();
	void			UpdateDuckJumpEyeOffset( void );
	bool			CanUnDuckJump( trace_t &trace );
	void			StartUnDuckJump( void );
	void			FinishUnDuckJump( trace_t &trace );
	void			SetDuckedEyeOffset( float duckFraction );
	void			FixPlayerCrouchStuck( bool moveup );
	void			ResetDuckLatched();

	float			SplineFraction( float value, float scale );

	void			CategorizeGroundSurface( trace_t &pm );

	bool			InWater( void );

	// Commander view movement
	void			IsometricMove( void );

	// Traces the player bbox as it is swept from start to end
	virtual CBaseHandle		TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm );

	// Checks to see if we should actually jump 
	void			PlaySwimSound();

	bool			IsDead( void ) const;

	// Figures out how the constraint should slow us down
	float			ComputeConstraintSpeedFactor( void );

	virtual void	SetGroundEntity( trace_t *pm );

	virtual void	StepMove( Vector &vecDestination, trace_t &trace );

	// when we step on ground that's too steep, search to see if there's any ground nearby that isn't too steep
	void			TryTouchGroundInQuadrants( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm );


protected:

	// Performs the collision resolution for fliers.
	void			PerformFlyCollisionResolution( trace_t &pm, Vector &move );

	virtual bool	GameHasLadders() const;

	enum
	{
		// eyes, waist, feet points (since they are all deterministic
		MAX_PC_CACHE_SLOTS = 3,
	};

	// Cache used to remove redundant calls to GetPointContents().
	int m_CachedGetPointContents[ MAX_PLAYERS_ARRAY_SAFE ][ MAX_PC_CACHE_SLOTS ];
	Vector m_CachedGetPointContentsPoint[ MAX_PLAYERS_ARRAY_SAFE ][ MAX_PC_CACHE_SLOTS ];	

	Vector			m_vecProximityMins;		// Used to be globals in sv_user.cpp.
	Vector			m_vecProximityMaxs;

	float			m_fFrameTime;

//private:
	int				m_iSpeedCropped;

	float			m_flStuckCheckTime[MAX_PLAYERS_ARRAY_SAFE][2]; // Last time we did a full test

	// special function for teleport-with-duck for episodic
#ifdef HL2_EPISODIC
public:
	void			ForceDuck( void );

#endif
};



#endif // GAMEMOVEMENT_H
