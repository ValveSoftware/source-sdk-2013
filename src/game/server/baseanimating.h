//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef BASEANIMATING_H
#define BASEANIMATING_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "entityoutput.h"
#include "studio.h"
#include "datacache/idatacache.h"
#include "tier0/threadtools.h"


struct animevent_t;
struct matrix3x4_t;
class CIKContext;
class KeyValues;
FORWARD_DECLARE_HANDLE( memhandle_t );

#define	BCF_NO_ANIMATION_SKIP	( 1 << 0 )	// Do not allow PVS animation skipping (mostly for attachments being critical to an entity)
#define	BCF_IS_IN_SPAWN			( 1 << 1 )	// Is currently inside of spawn, always evaluate animations

class CBaseAnimating : public CBaseEntity
{
public:
	DECLARE_CLASS( CBaseAnimating, CBaseEntity );

	CBaseAnimating();
	~CBaseAnimating();

	DECLARE_PREDICTABLE();

	enum
	{
		NUM_POSEPAREMETERS = 24,
		NUM_BONECTRLS = 4
	};

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
	DECLARE_ENT_SCRIPTDESC();

	virtual void SetModel( const char *szModelName );
	virtual void Activate();
	virtual void Spawn();
	virtual void Precache();
	virtual void SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );

	virtual int	 Restore( IRestore &restore );
	virtual void OnRestore();

	CStudioHdr *GetModelPtr( void );
	void InvalidateMdlCache();

	virtual CStudioHdr *OnNewModel();

	virtual CBaseAnimating*	GetBaseAnimating() { return this; }

	// Cycle access
	void SetCycle( float flCycle );
	float GetCycle() const;

	float	GetAnimTimeInterval( void ) const;

	// Call this in your constructor to tell it that you will not use animtime. Then the
	// interpolation will be done correctly on the client.
	// This defaults to off.
	void	UseClientSideAnimation();

	// Tells whether or not we're using client-side animation. Used for controlling
	// the transmission of animtime.
	bool	IsUsingClientSideAnimation()	{ return m_bClientSideAnimation; }


	// Basic NPC Animation functions
	virtual float	GetIdealSpeed( ) const;
	virtual float	GetIdealAccel( ) const;
	virtual void	StudioFrameAdvance(); // advance animation frame to some time in the future
	void StudioFrameAdvanceManual( float flInterval );
	bool	IsValidSequence( int iSequence );

	inline float					GetPlaybackRate();
	inline void						SetPlaybackRate( float rate );

	inline int GetSequence() { return m_nSequence; }
	virtual void SetSequence(int nSequence);
	/* inline */ void ResetSequence(int nSequence);
	// FIXME: push transitions support down into CBaseAnimating?
	virtual bool IsActivityFinished( void ) { return m_bSequenceFinished; }
	inline bool IsSequenceFinished( void ) { return m_bSequenceFinished; }
	inline bool SequenceLoops( void ) { return m_bSequenceLoops; }
	bool		 IsSequenceLooping( CStudioHdr *pStudioHdr, int iSequence );
	inline bool	 IsSequenceLooping( int iSequence ) { return IsSequenceLooping(GetModelPtr(),iSequence); }
	inline float SequenceDuration( void ) { return SequenceDuration( m_nSequence ); }
	float	SequenceDuration( CStudioHdr *pStudioHdr, int iSequence );
	inline float SequenceDuration( int iSequence ) { return SequenceDuration(GetModelPtr(), iSequence); }
	float	ScriptGetSequenceDuration( int iSequence );
	float	GetSequenceCycleRate( CStudioHdr *pStudioHdr, int iSequence );
	inline float	GetSequenceCycleRate( int iSequence ) { return GetSequenceCycleRate(GetModelPtr(),iSequence); }
	float	GetLastVisibleCycle( CStudioHdr *pStudioHdr, int iSequence );
	virtual float	GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence );
	inline float GetSequenceGroundSpeed( int iSequence ) { return GetSequenceGroundSpeed(GetModelPtr(), iSequence); }
	void	ResetActivityIndexes ( void );
	void    ResetEventIndexes ( void );
	int		SelectWeightedSequence ( Activity activity );
	int		SelectWeightedSequence ( Activity activity, int curSequence );
	int		SelectWeightedSequenceFromModifiers( Activity activity, CUtlSymbol *pActivityModifiers, int iModifierCount );
	int		SelectHeaviestSequence ( Activity activity );
	int		LookupActivity( const char *label );
	int		LookupSequence ( const char *label );
	inline int ScriptLookupSequence( const char *label ) { return LookupSequence( label ); }
	KeyValues *GetSequenceKeyValues( int iSequence );

	float GetSequenceMoveYaw( int iSequence );
	float GetSequenceMoveDist( CStudioHdr *pStudioHdr, int iSequence );
	inline float GetSequenceMoveDist( int iSequence ) { return GetSequenceMoveDist(GetModelPtr(),iSequence);}
	void  GetSequenceLinearMotion( int iSequence, Vector *pVec );
	const char *GetSequenceName( int iSequence );
	const char *GetSequenceActivityName( int iSequence );
	Activity GetSequenceActivity( int iSequence );

	void ResetSequenceInfo ( );
	// This will stop animation until you call ResetSequenceInfo() at some point in the future
	inline void StopAnimation( void ) { m_flPlaybackRate = 0; }

	virtual void ClampRagdollForce( const Vector &vecForceIn, Vector *vecForceOut ) { *vecForceOut = vecForceIn; } // Base class does nothing.
	virtual bool BecomeRagdollOnClient( const Vector &force );
	virtual bool IsRagdoll();
	virtual bool CanBecomeRagdoll( void ); //Check if this entity will ragdoll when dead.

	virtual	void GetSkeleton( CStudioHdr *pStudioHdr, Vector pos[], Quaternion q[], int boneMask );

	virtual void GetBoneTransform( int iBone, matrix3x4_t &pBoneToWorld );
	virtual void SetupBones( matrix3x4_t *pBoneToWorld, int boneMask );
	virtual void CalculateIKLocks( float currentTime );
	virtual void Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );

	bool HasAnimEvent( int nSequence, int nEvent );
	virtual	void DispatchAnimEvents ( CBaseAnimating *eventHandler ); // Handle events that have happend since last time called up until X seconds into the future
	inline void ScriptDispatchAnimEvents( HSCRIPT hScript )
	{
		CBaseEntity *pEntity = ToEnt( hScript );
		CBaseAnimating *pAnimating = NULL;
		if ( pEntity )
			pAnimating = pEntity->GetBaseAnimating();
		return this->DispatchAnimEvents( pAnimating );
	}
	virtual void HandleAnimEvent( animevent_t *pEvent );

	int		LookupPoseParameter( CStudioHdr *pStudioHdr, const char *szName );
	inline int	LookupPoseParameter( const char *szName ) { return LookupPoseParameter(GetModelPtr(), szName); }
	int ScriptLookupPoseParameter( const char *szName ) { return LookupPoseParameter( szName ); }

	float	SetPoseParameter( CStudioHdr *pStudioHdr, const char *szName, float flValue );
	inline float SetPoseParameter( const char *szName, float flValue ) { return SetPoseParameter( GetModelPtr(), szName, flValue ); }
	float	SetPoseParameter( CStudioHdr *pStudioHdr, int iParameter, float flValue );
	inline float SetPoseParameter( int iParameter, float flValue ) { return SetPoseParameter( GetModelPtr(), iParameter, flValue ); }
	inline float ScriptSetPoseParameter( int iParameter, float flValue ) { return SetPoseParameter( iParameter, flValue ); }

	float	GetPoseParameter( const char *szName );
	float	GetPoseParameter( int iParameter );
	bool	GetPoseParameterRange( int index, float &minValue, float &maxValue );
	bool	HasPoseParameter( int iSequence, const char *szName );
	bool	HasPoseParameter( int iSequence, int iParameter );
	float	EdgeLimitPoseParameter( int iParameter, float flValue, float flBase = 0.0f );

protected:
	// The modus operandi for pose parameters is that you should not use the const char * version of the functions
	// in general code -- it causes many many string comparisons, which is slower than you think. Better is to 
	// save off your pose parameters in member variables in your derivation of this function:
	virtual void	PopulatePoseParameters( void );


public:

	int  LookupBone( const char *szName );
	void GetBonePosition( const char *szName, Vector &origin, QAngle &angles );
	void GetBonePosition( int iBone, Vector &origin, QAngle &angles );
	int	GetPhysicsBone( int boneIndex );

	int GetNumBones ( void );

	int  FindTransitionSequence( int iCurrentSequence, int iGoalSequence, int *piDir );
	bool GotoSequence( int iCurrentSequence, float flCurrentCycle, float flCurrentRate,  int iGoalSequence, int &iNextSequence, float &flCycle, int &iDir );
	int  GetEntryNode( int iSequence );
	int  GetExitNode( int iSequence );
	
	void GetEyeballs( Vector &origin, QAngle &angles ); // ?? remove ??

	int LookupAttachment( const char *szName );

	// These return the attachment in world space
	bool GetAttachment( const char *szName, Vector &absOrigin, QAngle &absAngles );
	bool GetAttachment( int iAttachment, Vector &absOrigin, QAngle &absAngles );
	int GetAttachmentBone( int iAttachment );
	virtual bool GetAttachment( int iAttachment, matrix3x4_t &attachmentToWorld );

	// These return the attachment in the space of the entity
	bool GetAttachmentLocal( const char *szName, Vector &origin, QAngle &angles );
	bool GetAttachmentLocal( int iAttachment, Vector &origin, QAngle &angles );
	bool GetAttachmentLocal( int iAttachment, matrix3x4_t &attachmentToLocal );
	
	// Non-angle versions of the attachments in world space
	bool GetAttachment(  const char *szName, Vector &absOrigin, Vector *forward = NULL, Vector *right = NULL, Vector *up = NULL );
	bool GetAttachment( int iAttachment, Vector &absOrigin, Vector *forward = NULL, Vector *right = NULL, Vector *up = NULL );

	Vector ScriptGetAttachmentOrigin( int iAttachment );
	QAngle ScriptGetAttachmentAngles( int iAttachment );
	Vector ScriptGetBoneOrigin( int iBone );
	QAngle ScriptGetBoneAngles( int iBone );

	void SetBodygroup( int iGroup, int iValue );
	int GetBodygroup( int iGroup );
	int GetSkin() const { return m_nSkin; }
	void SetSkin(int nSkin) { m_nSkin = nSkin; }

	const char *GetBodygroupName( int iGroup );
	const char *GetBodygroupPartName( int iGroup, int iPart );
	int FindBodygroupByName( const char *name );
	int GetBodygroupCount( int iGroup );
	int GetNumBodyGroups( void );

	void					SetHitboxSet( int setnum );
	void					SetHitboxSetByName( const char *setname );
	int						GetHitboxSet( void );
	char const				*GetHitboxSetName( void );
	int						GetHitboxSetCount( void );
	int						GetHitboxBone( int hitboxIndex );
	bool					LookupHitbox( const char *szName, int& outSet, int& outBox );

	// Computes a box that surrounds all hitboxes
	bool ComputeHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs );
	bool ComputeEntitySpaceHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs );
	
	// Clone a CBaseAnimating from another (copies model & sequence data)
	void CopyAnimationDataFrom( CBaseAnimating *pSource );

	int ExtractBbox( int sequence, Vector& mins, Vector& maxs );
	void SetSequenceBox( void );
	int RegisterPrivateActivity( const char *pszActivityName );

	void	ResetClientsideFrame( void );

// Controllers.
	virtual	void			InitBoneControllers ( void );
	
	// Return's the controller's angle/position in bone space.
	float					GetBoneController ( int iController );

	// Maps the angle/position value you specify into the bone's start/end and sets the specified controller to the value.
	float					SetBoneController ( int iController, float flValue );
	
	void					GetVelocity(Vector *vVelocity, AngularImpulse *vAngVelocity);

	// these two need to move somewhere else
	LocalFlexController_t GetNumFlexControllers( void );
	const char *GetFlexDescFacs( int iFlexDesc );
	const char *GetFlexControllerName( LocalFlexController_t iFlexController );
	const char *GetFlexControllerType( LocalFlexController_t iFlexController );

	virtual	Vector GetGroundSpeedVelocity( void );

	bool GetIntervalMovement( float flIntervalUsed, bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles );
	bool GetSequenceMovement( int nSequence, float fromCycle, float toCycle, Vector &deltaPosition, QAngle &deltaAngles );
	float GetInstantaneousVelocity( float flInterval = 0.0 );
	float GetEntryVelocity( int iSequence );
	float GetExitVelocity( int iSequence );
	float GetMovementFrame( float flDist );
	bool HasMovement( int iSequence );

	void ReportMissingActivity( int iActivity );
	virtual bool TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	virtual bool TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	class CBoneCache *GetBoneCache( void );
	void InvalidateBoneCache();
	void InvalidateBoneCacheIfOlderThan( float deltaTime );
	virtual int DrawDebugTextOverlays( void );
	
	// See note in code re: bandwidth usage!!!
	void				DrawServerHitboxes( float duration = 0.0f, bool monocolor = false );
	void				DrawRawSkeleton( matrix3x4_t boneToWorld[], int boneMask, bool noDepthTest = true, float duration = 0.0f, bool monocolor = false );

	void				SetModelScale( float scale, float change_duration = 0.0f );
	inline void			ScriptSetModelScale( float scale, float change_duration = 0.0f )  { SetModelScale( scale, change_duration ); }
	float				GetModelScale() const { return m_flModelScale; }

	void				UpdateModelScale();
	virtual	void		RefreshCollisionBounds( void );
	
	// also calculate IK on server? (always done on client)
	void EnableServerIK();
	void DisableServerIK();

	// for ragdoll vs. car
	int GetHitboxesFrontside( int *boxList, int boxMax, const Vector &normal, float dist );

	void	GetInputDispatchEffectPosition( const char *sInputString, Vector &pOrigin, QAngle &pAngles );

	virtual void	ModifyOrAppendCriteria( AI_CriteriaSet& set );

	// Send a muzzle flash event to the client for this entity.
	void DoMuzzleFlash();

	// Fire
	virtual void Ignite( float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );
	virtual void IgniteLifetime( float flFlameLifetime );
	virtual void IgniteNumHitboxFires( int iNumHitBoxFires );
	virtual void IgniteHitboxFireScale( float flHitboxFireScale );
	virtual void Extinguish() { RemoveFlag( FL_ONFIRE ); }
	bool IsOnFire() { return ( (GetFlags() & FL_ONFIRE) != 0 ); }
	void Scorch( int rate, int floor );
	void InputIgnite( inputdata_t &inputdata );
	void InputIgniteLifetime( inputdata_t &inputdata );
	void InputIgniteNumHitboxFires( inputdata_t &inputdata );
	void InputIgniteHitboxFireScale( inputdata_t &inputdata );
	void InputBecomeRagdoll( inputdata_t &inputdata );

	// Dissolve, returns true if the ragdoll has been created
	bool Dissolve( const char *pMaterialName, float flStartTime, bool bNPCOnly = true, int nDissolveType = 0, Vector vDissolverOrigin = vec3_origin, int iMagnitude = 0 );
	bool IsDissolving() { return ( (GetFlags() & FL_DISSOLVING) != 0 ); }
	void TransferDissolveFrom( CBaseAnimating *pAnim );

	// animation needs
	float				m_flGroundSpeed;	// computed linear movement rate for current sequence
	float				m_flLastEventCheck;	// cycle index of when events were last checked

	virtual void SetLightingOriginRelative( CBaseEntity *pLightingOriginRelative );
	void SetLightingOriginRelative( string_t strLightingOriginRelative );
	CBaseEntity *GetLightingOriginRelative();

	virtual void SetLightingOrigin( CBaseEntity *pLightingOrigin );
	void SetLightingOrigin( string_t strLightingOrigin );
	CBaseEntity *GetLightingOrigin();

	const float* GetPoseParameterArray() { return m_flPoseParameter.Base(); }
	const float* GetEncodedControllerArray() { return m_flEncodedController.Base(); }

	void BuildMatricesWithBoneMerge( const CStudioHdr *pStudioHdr, const QAngle& angles, 
		const Vector& origin, const Vector pos[MAXSTUDIOBONES],
		const Quaternion q[MAXSTUDIOBONES], matrix3x4_t bonetoworld[MAXSTUDIOBONES],
		CBaseAnimating *pParent, CBoneCache *pParentCache );

	void	SetFadeDistance( float minFadeDist, float maxFadeDist );

	int		GetBoneCacheFlags( void ) { return m_fBoneCacheFlags; }
	inline void	SetBoneCacheFlags( unsigned short fFlag ) { m_fBoneCacheFlags |= fFlag; }
	inline void	ClearBoneCacheFlags( unsigned short fFlag ) { m_fBoneCacheFlags &= ~fFlag; }

	bool PrefetchSequence( int iSequence );

private:
	void LockStudioHdr();
	void UnlockStudioHdr();

	void StudioFrameAdvanceInternal( CStudioHdr *pStudioHdr, float flInterval );
	void InputSetLightingOriginRelative( inputdata_t &inputdata );
	void InputSetLightingOrigin( inputdata_t &inputdata );
	void InputSetModelScale( inputdata_t &inputdata );
	void InputSetModel( inputdata_t &inputdata );
	void InputSetCycle( inputdata_t &inputdata );
	void InputSetPlaybackRate( inputdata_t &inputdata );

	bool CanSkipAnimation( void );

public:
	void ScriptSetModel( const char *pszModel );

	CNetworkVar( int, m_nForceBone );
	CNetworkVector( m_vecForce );

	CNetworkVar( int, m_nSkin );
	CNetworkVar( int, m_nBody );
	CNetworkVar( int, m_nHitboxSet );

	// For making things thin during barnacle swallowing, e.g.
	CNetworkVar( float, m_flModelScale );

	// was pev->framerate
	CNetworkVar( float, m_flPlaybackRate );

public:
	void InitStepHeightAdjust( void );
	void SetIKGroundContactInfo( float minHeight, float maxHeight );
	void UpdateStepOrigin( void );

protected:
	float				m_flIKGroundContactTime;
	float				m_flIKGroundMinHeight;
	float				m_flIKGroundMaxHeight;

	float				m_flEstIkFloor; // debounced
	float				m_flEstIkOffset;

  	CIKContext			*m_pIk;
	int					m_iIKCounter;

public:
	Vector	GetStepOrigin( void ) const;
	QAngle	GetStepAngles( void ) const;

private:
	bool				m_bSequenceFinished;// flag set when StudioAdvanceFrame moves across a frame boundry
	bool				m_bSequenceLoops;	// true if the sequence loops
	bool				m_bResetSequenceInfoOnLoad; // true if a ResetSequenceInfo was queued up during dynamic load
	float				m_flDissolveStartTime;

	// was pev->frame
	CNetworkVar( float, m_flCycle );
	CNetworkVar( int, m_nSequence );	
	CNetworkArray( float, m_flPoseParameter, NUM_POSEPAREMETERS );	// must be private so manual mode works!
	CNetworkArray( float, m_flEncodedController, NUM_BONECTRLS );		// bone controller setting (0..1)

	// Client-side animation (useful for looping animation objects)
	CNetworkVar( bool, m_bClientSideAnimation );
	CNetworkVar( bool, m_bClientSideFrameReset );

	CNetworkVar( int, m_nNewSequenceParity );
	CNetworkVar( int, m_nResetEventsParity );

	// Incremented each time the entity is told to do a muzzle flash.
	// The client picks up the change and draws the flash.
	CNetworkVar( unsigned char, m_nMuzzleFlashParity );

	CNetworkHandle( CBaseEntity, m_hLightingOrigin );
	CNetworkHandle( CBaseEntity, m_hLightingOriginRelative );

	string_t m_iszLightingOriginRelative;	// for reading from the file only
	string_t m_iszLightingOrigin;			// for reading from the file only

	memhandle_t		m_boneCacheHandle;
	unsigned short	m_fBoneCacheFlags;		// Used for bone cache state on model

protected:
	CNetworkVar( float, m_fadeMinDist );	// Point at which fading is absolute
	CNetworkVar( float, m_fadeMaxDist );	// Point at which fading is inactive
	CNetworkVar( float, m_flFadeScale );	// Scale applied to min / max

public:
	COutputEvent m_OnIgnite;

private:
	CStudioHdr			*m_pStudioHdr;
	CThreadFastMutex	m_StudioHdrInitLock;
	CThreadFastMutex	m_BoneSetupMutex;

// FIXME: necessary so that cyclers can hack m_bSequenceFinished
friend class CFlexCycler;
friend class CCycler;
friend class CBlendingCycler;
};

//-----------------------------------------------------------------------------
// Purpose: return a pointer to an updated studiomdl cache cache
//-----------------------------------------------------------------------------
inline CStudioHdr *CBaseAnimating::GetModelPtr( void ) 
{ 
	if ( IsDynamicModelLoading() )
		return NULL;

#ifdef _DEBUG
	if ( !HushAsserts() )
	{
		// GetModelPtr() is often called before OnNewModel() so go ahead and set it up first chance.
		static IDataCacheSection *pModelCache = datacache->FindSection( "ModelData" );
		AssertOnce( pModelCache->IsFrameLocking() );
	}
#endif

	if ( !m_pStudioHdr && GetModel() )
	{
		LockStudioHdr();
	}
	return ( m_pStudioHdr && m_pStudioHdr->IsValid() ) ? m_pStudioHdr : NULL;
}

inline void CBaseAnimating::InvalidateMdlCache()
{
	UnlockStudioHdr();
	if ( m_pStudioHdr != NULL )
	{
		delete m_pStudioHdr;
		m_pStudioHdr = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Serves the 90% case of calling SetSequence / ResetSequenceInfo.
//-----------------------------------------------------------------------------

/*
inline void CBaseAnimating::ResetSequence(int nSequence)
{
	m_nSequence = nSequence;
	ResetSequenceInfo();
}
*/

inline float CBaseAnimating::GetPlaybackRate()
{
	return m_flPlaybackRate;
}

inline void CBaseAnimating::SetPlaybackRate( float rate )
{
	m_flPlaybackRate = rate;
}

inline void CBaseAnimating::SetLightingOrigin( CBaseEntity *pLightingOrigin )
{
	m_hLightingOrigin = pLightingOrigin;
}

inline CBaseEntity *CBaseAnimating::GetLightingOrigin()
{
	return m_hLightingOrigin;
}

inline void CBaseAnimating::SetLightingOriginRelative( CBaseEntity *pLightingOriginRelative )
{
	m_hLightingOriginRelative = pLightingOriginRelative;
}

inline CBaseEntity *CBaseAnimating::GetLightingOriginRelative()
{
	return m_hLightingOriginRelative;
}

//-----------------------------------------------------------------------------
// Cycle access
//-----------------------------------------------------------------------------
inline float CBaseAnimating::GetCycle() const
{
	return m_flCycle;
}

inline void CBaseAnimating::SetCycle( float flCycle )
{
	m_flCycle = flCycle;
}


EXTERN_SEND_TABLE(DT_BaseAnimating);



#define ANIMATION_SEQUENCE_BITS			12	// 4096 sequences
#define ANIMATION_SKIN_BITS				10	// 1024 body skin selections FIXME: this seems way high
#define ANIMATION_BODY_BITS				32	// body combinations
#define ANIMATION_HITBOXSET_BITS		2	// hit box sets 
#if defined( TF_DLL )
#define ANIMATION_POSEPARAMETER_BITS	8	// pose parameter resolution
#else
#define ANIMATION_POSEPARAMETER_BITS	11	// pose parameter resolution
#endif
#define ANIMATION_PLAYBACKRATE_BITS		8	// default playback rate, only used on leading edge detect sequence changes

#endif // BASEANIMATING_H
