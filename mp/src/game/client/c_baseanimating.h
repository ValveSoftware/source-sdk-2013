//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#ifndef C_BASEANIMATING_H
#define C_BASEANIMATING_H

#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
#include "studio.h"
#include "utlvector.h"
#include "ragdoll.h"
#include "mouthinfo.h"
// Shared activities
#include "ai_activity.h"
#include "animationlayer.h"
#include "sequence_Transitioner.h"
#include "bone_accessor.h"
#include "bone_merge_cache.h"
#include "ragdoll_shared.h"
#include "tier0/threadtools.h"
#include "datacache/idatacache.h"

#define LIPSYNC_POSEPARAM_NAME "mouth"
#define NUM_HITBOX_FIRES	10

/*
class C_BaseClientShader
{
	virtual void RenderMaterial( C_BaseEntity *pEntity, int count, const vec4_t *verts, const vec4_t *normals, const vec2_t *texcoords, vec4_t *lightvalues );
};
*/

class IRagdoll;
class CIKContext;
class CIKState;
class ConVar;
class C_RopeKeyframe;
class CBoneBitList;
class CBoneList;
class KeyValues;
class CJiggleBones;
class IBoneSetup;
FORWARD_DECLARE_HANDLE( memhandle_t );
typedef unsigned short MDLHandle_t;

extern ConVar vcollide_wireframe;


struct ClientModelRenderInfo_t : public ModelRenderInfo_t
{
	// Added space for lighting origin override. Just allocated space, need to set base pointer
	matrix3x4_t lightingOffset;

	// Added space for model to world matrix. Just allocated space, need to set base pointer
	matrix3x4_t modelToWorld;
};

struct RagdollInfo_t
{
	bool		m_bActive;
	float		m_flSaveTime;
	int			m_nNumBones;
	Vector		m_rgBonePos[MAXSTUDIOBONES];
	Quaternion	m_rgBoneQuaternion[MAXSTUDIOBONES];
};


class CAttachmentData
{
public:
	matrix3x4_t	m_AttachmentToWorld;
	QAngle	m_angRotation;
	Vector	m_vOriginVelocity;
	int		m_nLastFramecount : 31;
	int		m_bAnglesComputed : 1;
};


typedef unsigned int			ClientSideAnimationListHandle_t;

#define		INVALID_CLIENTSIDEANIMATION_LIST_HANDLE	(ClientSideAnimationListHandle_t)~0


class C_BaseAnimating : public C_BaseEntity, private IModelLoadCallback
{
public:
	DECLARE_CLASS( C_BaseAnimating, C_BaseEntity );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	enum
	{
		NUM_POSEPAREMETERS = 24,
		NUM_BONECTRLS = 4
	};

	C_BaseAnimating();
	~C_BaseAnimating();

	virtual C_BaseAnimating*		GetBaseAnimating() { return this; }

	bool UsesPowerOfTwoFrameBufferTexture( void );

	virtual bool	Interpolate( float currentTime );
	virtual void	Simulate();	
	virtual void	Release();	

	float	GetAnimTimeInterval( void ) const;

	virtual unsigned char	GetClientSideFade( void );

	// Get bone controller values.
	virtual void	GetBoneControllers(float controllers[MAXSTUDIOBONECTRLS]);
	virtual float	SetBoneController ( int iController, float flValue );

	LocalFlexController_t GetNumFlexControllers( void );
	const char *GetFlexDescFacs( int iFlexDesc );
	const char *GetFlexControllerName( LocalFlexController_t iFlexController );
	const char *GetFlexControllerType( LocalFlexController_t iFlexController );

	virtual void	GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles );

	// Computes a box that surrounds all hitboxes
	bool ComputeHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs );
	bool ComputeEntitySpaceHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs );

	// Gets the hitbox-to-world transforms, returns false if there was a problem
	bool HitboxToWorldTransforms( matrix3x4_t *pHitboxToWorld[MAXSTUDIOBONES] );

	// base model functionality
	float		  ClampCycle( float cycle, bool isLooping );
	virtual void GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM] );
	virtual void BuildTransformations( CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed );
	virtual void ApplyBoneMatrixTransform( matrix3x4_t& transform );
 	virtual int	VPhysicsGetObjectList( IPhysicsObject **pList, int listMax );

	// model specific
	virtual bool SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime );
	virtual void UpdateIKLocks( float currentTime );
	virtual void CalculateIKLocks( float currentTime );
	virtual bool ShouldDraw();
	virtual int DrawModel( int flags );
	virtual int	InternalDrawModel( int flags );
	virtual bool OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual bool OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	void		DoInternalDrawModel( ClientModelRenderInfo_t *pInfo, DrawModelState_t *pState, matrix3x4_t *pBoneToWorldArray = NULL );

	//
	virtual CMouthInfo *GetMouth();
	virtual void	ControlMouth( CStudioHdr *pStudioHdr );
	
	// override in sub-classes
	virtual void DoAnimationEvents( CStudioHdr *pStudio );
	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual void FireObsoleteEvent( const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual const char* ModifyEventParticles( const char* token ) { return token; }

	// Parses and distributes muzzle flash events
	virtual bool DispatchMuzzleEffect( const char *options, bool isFirstPerson );

	// virtual	void AllocateMaterials( void );
	// virtual	void FreeMaterials( void );

	virtual void ValidateModelIndex( void );
	virtual CStudioHdr *OnNewModel( void );
	CStudioHdr	*GetModelPtr() const;
	void InvalidateMdlCache();
	
	virtual void SetPredictable( bool state );
	void UseClientSideAnimation();

	// C_BaseClientShader **p_ClientShaders;

	virtual	void StandardBlendingRules( CStudioHdr *pStudioHdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );
	void UnragdollBlend( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime );

	void MaintainSequenceTransitions( IBoneSetup &boneSetup, float flCycle, Vector pos[], Quaternion q[] );
	virtual void AccumulateLayers( IBoneSetup &boneSetup, Vector pos[], Quaternion q[], float currentTime );

	virtual void ChildLayerBlend( Vector pos[], Quaternion q[], float currentTime, int boneMask );

	// Attachments
	int		LookupAttachment( const char *pAttachmentName );
	int		LookupRandomAttachment( const char *pAttachmentNameSubstring );

	int		LookupPoseParameter( CStudioHdr *pStudioHdr, const char *szName );
	inline int LookupPoseParameter( const char *szName ) { return LookupPoseParameter(GetModelPtr(), szName); }

	float	SetPoseParameter( CStudioHdr *pStudioHdr, const char *szName, float flValue );
	inline float SetPoseParameter( const char *szName, float flValue ) { return SetPoseParameter( GetModelPtr(), szName, flValue ); }
	float	SetPoseParameter( CStudioHdr *pStudioHdr, int iParameter, float flValue );
	inline float SetPoseParameter( int iParameter, float flValue ) { return SetPoseParameter( GetModelPtr(), iParameter, flValue ); }

	float	GetPoseParameter( int iPoseParameter );

	bool	GetPoseParameterRange( int iPoseParameter, float &minValue, float &maxValue );

	int		LookupBone( const char *szName );
	void	GetBonePosition( int iBone, Vector &origin, QAngle &angles );
	void	GetBoneTransform( int iBone, matrix3x4_t &pBoneToWorld );

	//=============================================================================
	// HPE_BEGIN:
	// [menglish] Finds the bone associated with the given hitbox
	//=============================================================================

	int		GetHitboxBone( int hitboxIndex );

	//=============================================================================
	// HPE_END
	//=============================================================================

	// Bone attachments
	virtual void		AttachEntityToBone( C_BaseAnimating* attachTarget, int boneIndexAttached=-1, Vector bonePosition=Vector(0,0,0), QAngle boneAngles=QAngle(0,0,0) );
	void				AddBoneAttachment( C_BaseAnimating* newBoneAttachment );
	void				RemoveBoneAttachment( C_BaseAnimating* boneAttachment );
	void				RemoveBoneAttachments();
	void				DestroyBoneAttachments();
	void				MoveBoneAttachments( C_BaseAnimating* attachTarget );
	int					GetNumBoneAttachments();
	C_BaseAnimating*	GetBoneAttachment( int i );
	virtual void		NotifyBoneAttached( C_BaseAnimating* attachTarget );
	virtual void		UpdateBoneAttachments( void );

	//bool solveIK(float a, float b, const Vector &Foot, const Vector &Knee1, Vector &Knee2);
	//void DebugIK( mstudioikchain_t *pikchain );

	virtual void					PreDataUpdate( DataUpdateType_t updateType );
	virtual void					PostDataUpdate( DataUpdateType_t updateType );
	virtual int						RestoreData( const char *context, int slot, int type );

	virtual void					NotifyShouldTransmit( ShouldTransmitState_t state );
	virtual void					OnPreDataChanged( DataUpdateType_t updateType );
	virtual void					OnDataChanged( DataUpdateType_t updateType );
	virtual void					AddEntity( void );

	// This can be used to force client side animation to be on. Only use if you know what you're doing!
	// Normally, the server entity should set this.
	void							ForceClientSideAnimationOn();
	
	void							AddToClientSideAnimationList();
	void							RemoveFromClientSideAnimationList( bool bBeingDestroyed = false );

	virtual bool					IsSelfAnimating();
	virtual void					ResetLatched();

	// implements these so ragdolls can handle frustum culling & leaf visibility
	virtual void					GetRenderBounds( Vector& theMins, Vector& theMaxs );
	virtual const Vector&			GetRenderOrigin( void );
	virtual const QAngle&			GetRenderAngles( void );

	virtual bool					GetSoundSpatialization( SpatializationInfo_t& info );

	// Attachments.
	bool							GetAttachment( const char *szName, Vector &absOrigin );
	bool							GetAttachment( const char *szName, Vector &absOrigin, QAngle &absAngles );

	// Inherited from C_BaseEntity
	virtual bool					GetAttachment( int number, Vector &origin );
	virtual bool					GetAttachment( int number, Vector &origin, QAngle &angles );
	virtual bool					GetAttachment( int number, matrix3x4_t &matrix );
	virtual bool					GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel );
	
	// Returns the attachment in local space
	bool							GetAttachmentLocal( int iAttachment, matrix3x4_t &attachmentToLocal );
	bool							GetAttachmentLocal( int iAttachment, Vector &origin, QAngle &angles );
	bool                            GetAttachmentLocal( int iAttachment, Vector &origin );

	bool							GetRootBone( matrix3x4_t &rootBone );

	// Should this object cast render-to-texture shadows?
	virtual ShadowType_t			ShadowCastType();

	// Should we collide?
	virtual CollideType_t			GetCollideType( void );

	virtual bool					TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	virtual bool					TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

	// returns true if we're currently being ragdolled
	bool							IsRagdoll() const;
	bool							IsAboutToRagdoll() const;
	virtual C_BaseAnimating			*BecomeRagdollOnClient();
	C_BaseAnimating					*CreateRagdollCopy();
	bool							InitAsClientRagdoll( const matrix3x4_t *pDeltaBones0, const matrix3x4_t *pDeltaBones1, const matrix3x4_t *pCurrentBonePosition, float boneDt, bool bFixedConstraints=false );
	void							IgniteRagdoll( C_BaseAnimating *pSource );
	void							TransferDissolveFrom( C_BaseAnimating *pSource );
	virtual void					SaveRagdollInfo( int numbones, const matrix3x4_t &cameraTransform, CBoneAccessor &pBoneToWorld );
	virtual bool					RetrieveRagdollInfo( Vector *pos, Quaternion *q );
	virtual void					Clear( void );
	void							ClearRagdoll();
	void							CreateUnragdollInfo( C_BaseAnimating *pRagdoll );
	bool							ForceSetupBonesAtTime( matrix3x4_t *pBonesOut, float flTime );
	virtual bool					GetRagdollInitBoneArrays( matrix3x4_t *pDeltaBones0, matrix3x4_t *pDeltaBones1, matrix3x4_t *pCurrentBones, float boneDt );

	// For shadows rendering the correct body + sequence...
	virtual int GetBody()			{ return m_nBody; }
	virtual int GetSkin()			{ return m_nSkin; }

	bool IsOnFire() { return ( (GetFlags() & FL_ONFIRE) != 0 ); }

	inline float					GetPlaybackRate();
	inline void						SetPlaybackRate( float rate );

	void							SetModelScale( float scale, float change_duration = 0.0f  );
	float							GetModelScale() const { return m_flModelScale; }
	inline bool						IsModelScaleFractional() const;  /// very fast way to ask if the model scale is < 1.0f  (faster than if (GetModelScale() < 1.0f) )
	inline bool						IsModelScaled() const;
	void							UpdateModelScale( void );
	virtual	void					RefreshCollisionBounds( void );

	int								GetSequence();
	virtual void					SetSequence(int nSequence);
	inline void						ResetSequence(int nSequence);
	float							GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence );
	inline float					GetSequenceGroundSpeed( int iSequence ) { return GetSequenceGroundSpeed(GetModelPtr(), iSequence); }
	bool							IsSequenceLooping( CStudioHdr *pStudioHdr, int iSequence );
	inline bool						IsSequenceLooping( int iSequence ) { return IsSequenceLooping(GetModelPtr(),iSequence); }
	float							GetSequenceMoveDist( CStudioHdr *pStudioHdr, int iSequence );
	void							GetSequenceLinearMotion( int iSequence, Vector *pVec );
	void							GetBlendedLinearVelocity( Vector *pVec );
	int								LookupSequence ( const char *label );
	int								LookupActivity( const char *label );
	char const						*GetSequenceName( int iSequence ); 
	char const						*GetSequenceActivityName( int iSequence );
	Activity						GetSequenceActivity( int iSequence );
	KeyValues						*GetSequenceKeyValues( int iSequence );
	virtual void					StudioFrameAdvance(); // advance animation frame to some time in the future

	// Clientside animation
	virtual float					FrameAdvance( float flInterval = 0.0f );
	virtual float					GetSequenceCycleRate( CStudioHdr *pStudioHdr, int iSequence );
	virtual void					UpdateClientSideAnimation();
	void							ClientSideAnimationChanged();
	virtual unsigned int			ComputeClientSideAnimationFlags();

	virtual void ResetClientsideFrame( void ) { SetCycle( 0 ); }

	void SetCycle( float flCycle );
	float GetCycle() const;

	void SetBodygroup( int iGroup, int iValue );
	int GetBodygroup( int iGroup );

	const char *GetBodygroupName( int iGroup );
	int FindBodygroupByName( const char *name );
	int GetBodygroupCount( int iGroup );
	int GetNumBodyGroups( void );

	class CBoneCache				*GetBoneCache( CStudioHdr *pStudioHdr );
	void							SetHitboxSet( int setnum );
	void							SetHitboxSetByName( const char *setname );
	int								GetHitboxSet( void );
	char const						*GetHitboxSetName( void );
	int								GetHitboxSetCount( void );
	void							DrawClientHitboxes( float duration = 0.0f, bool monocolor = false );

	C_BaseAnimating*				FindFollowedEntity();

	virtual bool					IsActivityFinished( void ) { return m_bSequenceFinished; }
	inline bool						IsSequenceFinished( void );
	inline bool						SequenceLoops( void ) { return m_bSequenceLoops; }

	// All view model attachments origins are stretched so you can place entities at them and
	// they will match up with where the attachment winds up being drawn on the view model, since
	// the view models are drawn with a different FOV.
	//
	// If you're drawing something inside of a view model's DrawModel() function, then you want the
	// original attachment origin instead of the adjusted one. To get that, call this on the 
	// adjusted attachment origin.
	virtual void					UncorrectViewModelAttachment( Vector &vOrigin ) {}

	// Call this if SetupBones() has already been called this frame but you need to move the
	// entity and rerender.
	void							InvalidateBoneCache();
	bool							IsBoneCacheValid() const;	// Returns true if the bone cache is considered good for this frame.
	void							GetCachedBoneMatrix( int boneIndex, matrix3x4_t &out );

	// Wrappers for CBoneAccessor.
	const matrix3x4_t&				GetBone( int iBone ) const;
	matrix3x4_t&					GetBoneForWrite( int iBone );

	// Used for debugging. Will produce asserts if someone tries to setup bones or
	// attachments before it's allowed.
	// Use the "AutoAllowBoneAccess" class to auto push/pop bone access.
	// Use a distinct "tag" when pushing/popping - asserts when push/pop tags do not match.
	struct AutoAllowBoneAccess
	{
		AutoAllowBoneAccess( bool bAllowForNormalModels, bool bAllowForViewModels );
		~AutoAllowBoneAccess( void );
	};
	static void						PushAllowBoneAccess( bool bAllowForNormalModels, bool bAllowForViewModels, char const *tagPush );
	static void						PopBoneAccess( char const *tagPop );
	static void						ThreadedBoneSetup();
	static void						InitBoneSetupThreadPool();
	static void						ShutdownBoneSetupThreadPool();

	// Invalidate bone caches so all SetupBones() calls force bone transforms to be regenerated.
	static void						InvalidateBoneCaches();

	// Purpose: My physics object has been updated, react or extract data
	virtual void					VPhysicsUpdate( IPhysicsObject *pPhysics );

	void DisableMuzzleFlash();		// Turn off the muzzle flash (ie: signal that we handled the server's event).
	virtual void DoMuzzleFlash();	// Force a muzzle flash event. Note: this only QUEUES an event, so
									// ProcessMuzzleFlashEvent will get called later.
	bool ShouldMuzzleFlash() const;	// Is the muzzle flash event on?

	// This is called to do the actual muzzle flash effect.
	virtual void ProcessMuzzleFlashEvent();
	
	// Update client side animations
	static void UpdateClientSideAnimations();

	// Load the model's keyvalues section and create effects listed inside it
	void InitModelEffects( void );

	// Sometimes the server wants to update the client's cycle to get the two to run in sync (for proper hit detection)
	virtual void SetServerIntendedCycle( float intended ) { (void)intended; }
	virtual float GetServerIntendedCycle( void ) { return -1.0f; }

	// For prediction
	int								SelectWeightedSequence ( int activity );
	int								SelectWeightedSequenceFromModifiers( Activity activity, CUtlSymbol *pActivityModifiers, int iModifierCount );
	void							ResetSequenceInfo( void );
	float							SequenceDuration( void );
	float							SequenceDuration( CStudioHdr *pStudioHdr, int iSequence );
	inline float					SequenceDuration( int iSequence ) { return SequenceDuration(GetModelPtr(), iSequence); }
	int								FindTransitionSequence( int iCurrentSequence, int iGoalSequence, int *piDir );

	void							RagdollMoved( void );

	virtual void					GetToolRecordingState( KeyValues *msg );
	virtual void					CleanupToolRecordingState( KeyValues *msg );

	void							SetReceivedSequence( void );
	virtual bool					ShouldResetSequenceOnNewModel( void );

	virtual bool					IsViewModel() const;
	virtual void					UpdateOnRemove( void );

protected:
	// View models scale their attachment positions to account for FOV. To get the unmodified
	// attachment position (like if you're rendering something else during the view model's DrawModel call),
	// use TransformViewModelAttachmentToWorld.
	virtual void					FormatViewModelAttachment( int nAttachment, matrix3x4_t &attachmentToWorld ) {}

	// View models say yes to this.
	bool							IsBoneAccessAllowed() const;
	CMouthInfo&						MouthInfo();

	// Models used in a ModelPanel say yes to this
	virtual bool					IsMenuModel() const;

	// Allow studio models to tell C_BaseEntity what their m_nBody value is
	virtual int						GetStudioBody( void ) { return m_nBody; }

	virtual bool					CalcAttachments();

private:
	// This method should return true if the bones have changed + SetupBones needs to be called
	virtual float					LastBoneChangedTime() { return FLT_MAX; }

	CBoneList*						RecordBones( CStudioHdr *hdr, matrix3x4_t *pBoneState );

	bool							PutAttachment( int number, const matrix3x4_t &attachmentToWorld );
	void							TermRopes();

	void							DelayedInitModelEffects( void );

	void							UpdateRelevantInterpolatedVars();
	void							AddBaseAnimatingInterpolatedVars();
	void							RemoveBaseAnimatingInterpolatedVars();

public:
	CRagdoll						*m_pRagdoll;

	// Texture group to use
	int								m_nSkin;

	// Object bodygroup
	int								m_nBody;

	// Hitbox set to use (default 0)
	int								m_nHitboxSet;

	CSequenceTransitioner			m_SequenceTransitioner;

protected:
	CIKContext						*m_pIk;

	int								m_iEyeAttachment;

	// Animation playback framerate
	float							m_flPlaybackRate;

	// Decomposed ragdoll info
	bool							m_bStoreRagdollInfo;
	RagdollInfo_t					*m_pRagdollInfo;
	Vector							m_vecForce;
	int								m_nForceBone;

	// Is bone cache valid
	// bone transformation matrix
	unsigned long					m_iMostRecentModelBoneCounter;
	unsigned long					m_iMostRecentBoneSetupRequest;
	int								m_iPrevBoneMask;
	int								m_iAccumulatedBoneMask;

	CBoneAccessor					m_BoneAccessor;
	CThreadFastMutex				m_BoneSetupLock;

	ClientSideAnimationListHandle_t	m_ClientSideAnimationListHandle;

	// Client-side animation
	bool							m_bClientSideFrameReset;

	// Bone attachments. Used for attaching one BaseAnimating to another's bones.
	// Client side only.
	CUtlVector<CHandle<C_BaseAnimating> > m_BoneAttachments;
	int								m_boneIndexAttached;
	Vector							m_bonePosition;
	QAngle							m_boneAngles;
	CHandle<C_BaseAnimating>		m_pAttachedTo;

protected:

	float							m_fadeMinDist;
	float							m_fadeMaxDist;
	float							m_flFadeScale;

private:

	float							m_flGroundSpeed;	// computed linear movement rate for current sequence
	float							m_flLastEventCheck;	// cycle index of when events were last checked
	bool							m_bSequenceFinished;// flag set when StudioAdvanceFrame moves across a frame boundry
	bool							m_bSequenceLoops;	// true if the sequence loops

	// Mouth lipsync/envelope following values
	CMouthInfo						m_mouth;

	CNetworkVar( float, m_flModelScale );

	// Animation blending factors
	float							m_flPoseParameter[MAXSTUDIOPOSEPARAM];
	CInterpolatedVarArray< float, MAXSTUDIOPOSEPARAM >		m_iv_flPoseParameter;
	float							m_flOldPoseParameters[MAXSTUDIOPOSEPARAM];

	int								m_nPrevSequence;
	int								m_nRestoreSequence;

	// Ropes that got spawned when the model was created.
	CUtlLinkedList<C_RopeKeyframe*,unsigned short> m_Ropes;

	// event processing info
	float							m_flPrevEventCycle;
	int								m_nEventSequence;

	float							m_flEncodedController[MAXSTUDIOBONECTRLS];	
	CInterpolatedVarArray< float, MAXSTUDIOBONECTRLS >		m_iv_flEncodedController;
	float							m_flOldEncodedController[MAXSTUDIOBONECTRLS];

	// Clientside animation
	bool							m_bClientSideAnimation;
	bool							m_bLastClientSideFrameReset;

	int								m_nNewSequenceParity;
	int								m_nResetEventsParity;

	int								m_nPrevNewSequenceParity;
	int								m_nPrevResetEventsParity;

	bool							m_builtRagdoll;
	Vector							m_vecPreRagdollMins;
	Vector							m_vecPreRagdollMaxs;

	// Current animation sequence
	int								m_nSequence;
	bool							m_bReceivedSequence;

	// Current cycle location from server
protected:
	float							m_flCycle;
	CInterpolatedVar< float >		m_iv_flCycle;
	float							m_flOldCycle;
	bool							m_bNoModelParticles;

private:
	float							m_flOldModelScale;
	int								m_nOldSequence;
	CBoneMergeCache					*m_pBoneMergeCache;	// This caches the strcmp lookups that it has to do
														// when merg
	
	CUtlVector< matrix3x4_t >		m_CachedBoneData; // never access this directly. Use m_BoneAccessor.
	memhandle_t						m_hitboxBoneCacheHandle;
	float							m_flLastBoneSetupTime;
	CJiggleBones					*m_pJiggleBones;

	// Calculated attachment points
	CUtlVector<CAttachmentData>		m_Attachments;

	bool							SetupBones_AttachmentHelper( CStudioHdr *pStudioHdr );

	EHANDLE							m_hLightingOrigin;
	EHANDLE							m_hLightingOriginRelative;

	// These are compared against each other to determine if the entity should muzzle flash.
	CNetworkVar( unsigned char, m_nMuzzleFlashParity );
	unsigned char m_nOldMuzzleFlashParity;

	bool							m_bInitModelEffects;

	// Dynamic models
	bool							m_bDynamicModelAllowed;
	bool							m_bDynamicModelPending;
	bool							m_bResetSequenceInfoOnLoad;
	CRefCountedModelIndex			m_AutoRefModelIndex;
public:
	void							EnableDynamicModels() { m_bDynamicModelAllowed = true; }
	bool							IsDynamicModelLoading() const { return m_bDynamicModelPending; }
private:
	virtual void					OnModelLoadComplete( const model_t* pModel );

private:
	void							LockStudioHdr();
	void							UnlockStudioHdr();
	mutable CStudioHdr				*m_pStudioHdr;
	mutable MDLHandle_t				m_hStudioHdr;
	CThreadFastMutex				m_StudioHdrInitLock;
};

enum 
{
	RAGDOLL_FRICTION_OFF = -2,
	RAGDOLL_FRICTION_NONE,
	RAGDOLL_FRICTION_IN,
	RAGDOLL_FRICTION_HOLD,
	RAGDOLL_FRICTION_OUT,
};

class C_ClientRagdoll : public C_BaseAnimating, public IPVSNotify
{
	
public:
	C_ClientRagdoll( bool bRestoring = true );
	DECLARE_CLASS( C_ClientRagdoll, C_BaseAnimating );
	DECLARE_DATADESC();

	// inherited from IPVSNotify
	virtual void OnPVSStatusChanged( bool bInPVS );

	virtual void Release( void );
	virtual void SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );
	virtual void ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );
	void ClientThink( void );
	void ReleaseRagdoll( void ) { m_bReleaseRagdoll = true;	}
	bool ShouldSavePhysics( void ) { return true; }
	virtual void	OnSave();
	virtual void	OnRestore();
	virtual int ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_SAVE_NON_NETWORKABLE; }
	virtual IPVSNotify*				GetPVSNotifyInterface() { return this; }

	void	HandleAnimatedFriction( void );
	virtual void SUB_Remove( void );

	void	FadeOut( void );
	virtual float LastBoneChangedTime();

	bool m_bFadeOut;
	bool m_bImportant;
	float m_flEffectTime;

private:
	int m_iCurrentFriction;
	int m_iMinFriction;
	int m_iMaxFriction;
	float m_flFrictionModTime;
	float m_flFrictionTime;

	int  m_iFrictionAnimState;
	bool m_bReleaseRagdoll;

	bool m_bFadingOut;

	float m_flScaleEnd[NUM_HITBOX_FIRES];
	float m_flScaleTimeStart[NUM_HITBOX_FIRES];
	float m_flScaleTimeEnd[NUM_HITBOX_FIRES];
};

//-----------------------------------------------------------------------------
// Purpose: Serves the 90% case of calling SetSequence / ResetSequenceInfo.
//-----------------------------------------------------------------------------
inline void C_BaseAnimating::ResetSequence(int nSequence)
{
	SetSequence( nSequence );
	ResetSequenceInfo();
}

inline float C_BaseAnimating::GetPlaybackRate()
{
	return m_flPlaybackRate;
}

inline void C_BaseAnimating::SetPlaybackRate( float rate )
{
	m_flPlaybackRate = rate;
}

inline const matrix3x4_t& C_BaseAnimating::GetBone( int iBone ) const
{
	return m_BoneAccessor.GetBone( iBone );
}

inline matrix3x4_t& C_BaseAnimating::GetBoneForWrite( int iBone )
{
	return m_BoneAccessor.GetBoneForWrite( iBone );
}


inline bool C_BaseAnimating::ShouldMuzzleFlash() const
{
	return m_nOldMuzzleFlashParity != m_nMuzzleFlashParity;
}

inline float C_BaseAnimating::GetCycle() const
{
	return m_flCycle;
}

//-----------------------------------------------------------------------------
// Purpose: return a pointer to an updated studiomdl cache cache
//-----------------------------------------------------------------------------

inline CStudioHdr *C_BaseAnimating::GetModelPtr() const
{ 
	if ( IsDynamicModelLoading() )
		return NULL;

#ifdef _DEBUG
	// GetModelPtr() is often called before OnNewModel() so go ahead and set it up first chance.
//	static IDataCacheSection *pModelCache = datacache->FindSection( "ModelData" );
//	AssertOnce( pModelCache->IsFrameLocking() );
#endif
	if ( !m_pStudioHdr )
	{
		const_cast<C_BaseAnimating *>(this)->LockStudioHdr();
	}
	Assert( m_pStudioHdr ? m_pStudioHdr->GetRenderHdr() == mdlcache->GetStudioHdr(m_hStudioHdr) : m_hStudioHdr == MDLHANDLE_INVALID );
	return m_pStudioHdr;
}


inline void C_BaseAnimating::InvalidateMdlCache()
{
	UnlockStudioHdr();
}

inline bool C_BaseAnimating::IsModelScaleFractional() const
{
	return ( m_flModelScale < 1.0f );
}

inline bool C_BaseAnimating::IsModelScaled() const
{
	return ( m_flModelScale > 1.0f+FLT_EPSILON || m_flModelScale < 1.0f-FLT_EPSILON );
}

//-----------------------------------------------------------------------------
// Sequence access
//-----------------------------------------------------------------------------
inline int C_BaseAnimating::GetSequence() 
{ 
	return m_nSequence; 
}

inline bool C_BaseAnimating::IsSequenceFinished( void ) 
{ 
	return m_bSequenceFinished; 
}

inline float C_BaseAnimating::SequenceDuration( void )
{ 
	return SequenceDuration( GetSequence() ); 
}


//-----------------------------------------------------------------------------
// Mouth
//-----------------------------------------------------------------------------
inline CMouthInfo& C_BaseAnimating::MouthInfo()			
{ 
	return m_mouth; 
}


// FIXME: move these to somewhere that makes sense
void GetColumn( matrix3x4_t& src, int column, Vector &dest );
void SetColumn( Vector &src, int column, matrix3x4_t& dest );

EXTERN_RECV_TABLE(DT_BaseAnimating);


extern void DevMsgRT( PRINTF_FORMAT_STRING char const* pMsg, ... );

#endif // C_BASEANIMATING_H
