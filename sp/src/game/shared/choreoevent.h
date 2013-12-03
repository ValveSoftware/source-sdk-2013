//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CHOREOEVENT_H
#define CHOREOEVENT_H
#ifdef _WIN32
#pragma once
#endif

class CChoreoActor;
class CChoreoChannel;
class CChoreoEvent;
class CChoreoScene;
class IChoreoEventCallback; 
class CAudioMixer;
class CUtlBuffer;
class IChoreoStringPool;


#include "tier1/utlstring.h"
#include "tier1/utlvector.h"
#include "expressionsample.h"
#include "networkvar.h"
#include "localflexcontroller.h"

typedef CUtlString ChoreoStr_t;

//-----------------------------------------------------------------------------
// Purpose: SPEAK events can have "relative tags" that other objects can reference
//  to specify their start times off of
//-----------------------------------------------------------------------------
class CEventRelativeTag
{
public:
	DECLARE_CLASS_NOBASE( CEventRelativeTag );
	
	enum
	{
		MAX_EVENTTAG_LENGTH = 128,
	};

					CEventRelativeTag( CChoreoEvent *owner, const char *name, float percentage );
					CEventRelativeTag( const CEventRelativeTag& src );
	
	const char		*GetName( void );
	float			GetPercentage( void );
	void			SetPercentage( float percentage );

	// Returns the corrected time based on the owner's length and start time
	float			GetStartTime( void );
	CChoreoEvent	*GetOwner( void );
	void			SetOwner( CChoreoEvent *event );

protected:

	ChoreoStr_t		m_Name;
	float			m_flPercentage;
	CChoreoEvent	*m_pOwner;
};

//-----------------------------------------------------------------------------
// Purpose: GESTURE events can have "absolute tags" (where the value is not a 
//  percentage, but an actual timestamp from the start of the event)
//-----------------------------------------------------------------------------
class CEventAbsoluteTag
{
public:
	enum
	{
		MAX_EVENTTAG_LENGTH = 128,
	};

					CEventAbsoluteTag( CChoreoEvent *owner, const char *name, float percentage );
					CEventAbsoluteTag( const CEventAbsoluteTag& src );
	
	const char		*GetName( void );

	float			GetPercentage( void );
	void			SetPercentage( float percentage );
	
	float			GetEventTime( void );
	void			SetEventTime( float t );

	float			GetAbsoluteTime( void );
	void			SetAbsoluteTime( float t );

	CChoreoEvent	*GetOwner( void );
	void			SetOwner( CChoreoEvent *event );

	void			SetLocked( bool bLocked );
	bool			GetLocked( void );

	void			SetLinear( bool bLinear );
	bool			GetLinear( void );

	void			SetEntry( bool bEntry );
	bool			GetEntry( void );

	void			SetExit( bool bExit );
	bool			GetExit( void );

protected:

	ChoreoStr_t		m_Name;
	float			m_flPercentage; 
	bool			m_bLocked:1;
	bool			m_bLinear:1;
	bool			m_bEntry:1;
	bool			m_bExit:1;
	CChoreoEvent	*m_pOwner;
};

//-----------------------------------------------------------------------------
// Purpose: FLEXANIMATION events can have "timing tags" that are used to align and
//  manipulate flex animation curves
//-----------------------------------------------------------------------------
class CFlexTimingTag : public CEventRelativeTag
{
	DECLARE_CLASS( CFlexTimingTag, CEventRelativeTag );

public:
					CFlexTimingTag( CChoreoEvent *owner, const char *name, float percentage, bool locked );
					CFlexTimingTag( const CFlexTimingTag& src );
	
	bool			GetLocked( void );
	void			SetLocked( bool locked );

protected:
	bool			m_bLocked;
};

//-----------------------------------------------------------------------------
// Purpose: A flex controller position can be animated over a period of time
//-----------------------------------------------------------------------------
class CFlexAnimationTrack
{
public:
	enum
	{
		MAX_CONTROLLER_NAME = 128,
	};

						CFlexAnimationTrack( CChoreoEvent *event );
						CFlexAnimationTrack( const CFlexAnimationTrack* src );
	virtual 			~CFlexAnimationTrack( void );

	void				SetEvent( CChoreoEvent *event );
	CChoreoEvent		*GetEvent( void );

	void				SetFlexControllerName( const char *name );
	char const			*GetFlexControllerName( void );

	void				SetComboType( bool combo );
	bool				IsComboType( void );

	void				SetMin( float value );
	void				SetMax( float value );
	float				GetMin( int type = 0 );
	float				GetMax( int type = 0 );

	bool				IsInverted( void );
	void				SetInverted( bool isInverted );

	int					GetNumSamples( int type = 0 );
	CExpressionSample	*GetSample( int index, int type = 0 );

	bool				IsTrackActive( void );
	void				SetTrackActive( bool active );

	// returns scaled value for absolute time per left/right side
	float				GetIntensity( float time, int side = 0 );

	CExpressionSample	*AddSample( float time, float value, int type = 0 );
	void				RemoveSample( int index, int type = 0 );
	void				Clear( void );

	void				Resort( int type = 0 );

	// Puts in dummy start/end samples to spline to zero ( or 0.5 for
	//  left/right data) at the origins
	CExpressionSample	*GetBoundedSample( int number, bool& bClamped, int type = 0 );

	int					GetFlexControllerIndex( int side = 0 );
	LocalFlexController_t	GetRawFlexControllerIndex( int side = 0 );
	void				SetFlexControllerIndex( LocalFlexController_t raw, int index, int side = 0 );

	// returns 0..1 value for 0..1 time fraction per mag/balance
	float				GetFracIntensity( float time, int type );

	// retrieves raw intensity values (for mag vs. left/right slider setting)
	float				GetSampleIntensity( float time );
	float				GetBalanceIntensity( float time );

	void				SetEdgeInfo( bool leftEdge, int curveType, float zero );
	void				GetEdgeInfo( bool leftEdge, int& curveType, float& zero ) const;
	void				SetEdgeActive( bool leftEdge, bool state );
	bool				IsEdgeActive( bool leftEdge ) const;
	int					GetEdgeCurveType( bool leftEdge ) const;
	float				GetEdgeZeroValue( bool leftEdge ) const;

	float				GetDefaultEdgeZeroPos() const;

	void				SetServerSide( bool state );
	bool				IsServerSide() const;
private:
	// remove any samples after endtime
	void				RemoveOutOfRangeSamples( int type );

	// returns scaled value for absolute time per mag/balance
	float				GetIntensityInternal( float time, int type );

public:
	// returns the fractional (0..1) value for "zero" based on Min/Max ranges
	float				GetZeroValue( int type, bool leftSide );


private:
	char				*m_pControllerName;

	// base track has range, combo is always 0..1
	float				m_flMin;
	float				m_flMax;

	// 0 == magnitude
	// 1 == left/right
	CUtlVector< CExpressionSample > m_Samples[ 2 ];
	int								m_nFlexControllerIndex[ 2 ];
	LocalFlexController_t			m_nFlexControllerIndexRaw[ 2 ];

	// For left and right edge of type 0 flex data ( magnitude track )
	EdgeInfo_t				m_EdgeInfo[ 2 ];

	CChoreoEvent		*m_pEvent;

	// Is track active
	bool				m_bActive:1;

	// Is this a combo (magnitude + stereo) track
	bool				m_bCombo:1;
	bool				m_bServerSide:1;

	bool				m_bInverted; // track is displayed 1..0 instead of 0..1
};


//-----------------------------------------------------------------------------
// Purpose: The generic scene event type
//-----------------------------------------------------------------------------
class CChoreoEvent : public ICurveDataAccessor
{
public:
	// Type of event this object represents
	typedef enum
	{
		// Don't know yet
		UNSPECIFIED = 0,

		// Section start/end
		SECTION,

		// Play an expression
		EXPRESSION,
		
		// Look at another actor
		LOOKAT,

		// Move to a location
		MOVETO,

		// Speak/visemes a wave file
		SPEAK,

		// Play a gesture
		GESTURE,

		// Play a sequence
		SEQUENCE,

		// Face another actor
		FACE,

		// Fire a trigger
		FIRETRIGGER,

		// One or more flex sliders animated over the course of the event time period
		FLEXANIMATION,

		// A contained .vcd file
		SUBSCENE,

		// Loop back to previous time (forever or up to N times)
		LOOP,

		// A time span during which the scene may be temporarily interrupted
		INTERRUPT,

		// A dummy event that is used to mark the .vcd end time
		STOPPOINT,

		// A time span during which this actor can respond to events happening in the world, etc.
		PERMIT_RESPONSES,

		// A string passed to the game code for interpretation
		GENERIC,

		// THIS MUST BE LAST!!!
		NUM_TYPES,
	} EVENTTYPE;

	enum
	{
		MAX_TAGNAME_STRING		= 128,
		MAX_CCTOKEN_STRING		= 64,
	};

	typedef enum
	{
		DEFAULT = 0,
		SIMULATION,
		DISPLAY,
	} TIMETYPE;

	typedef enum
	{
		CC_MASTER = 0,  // default, implied
		CC_SLAVE,
		CC_DISABLED,

		NUM_CC_TYPES,
	} CLOSECAPTION;

	static int	s_nGlobalID;

	// Construction
	CChoreoEvent( CChoreoScene *scene );
	CChoreoEvent( CChoreoScene *scene, EVENTTYPE type, const char *name );
	CChoreoEvent( CChoreoScene *scene, EVENTTYPE type, const char *name, const char *param );

	// Assignment
	CChoreoEvent&	operator=(const CChoreoEvent& src );

	~CChoreoEvent( void );

	// ICurveDataAccessor methods
	virtual bool	CurveHasEndTime();
	virtual int		GetDefaultCurveType();

	// Binary serialization
	void			SaveToBuffer( CUtlBuffer& buf, CChoreoScene *pScene, IChoreoStringPool *pStringPool );
	bool			RestoreFromBuffer( CUtlBuffer& buf, CChoreoScene *pScene, IChoreoStringPool *pStringPool );

	// Accessors
	EVENTTYPE		GetType( void );
	void			SetType( EVENTTYPE type );

	void			SetName( const char *name );
	const char		*GetName( void );

	void			SetParameters( const char *target );
	const char		*GetParameters( void );
	void			SetParameters2( const char *target );
	const char		*GetParameters2( void );
	void			SetParameters3( const char *target );
	const char		*GetParameters3( void );

	void			SetStartTime( float starttime );
	float			GetStartTime( void );

	void			SetEndTime( float endtime );
	float			GetEndTime( void );

	float			GetDuration( void );

	void			SetResumeCondition( bool resumecondition );
	bool			IsResumeCondition( void );

	void			SetLockBodyFacing( bool lockbodyfacing );
	bool			IsLockBodyFacing( void );

	void			SetDistanceToTarget( float distancetotarget );
	float			GetDistanceToTarget( void );

	void			SetForceShortMovement( bool bForceShortMovement );
	bool			GetForceShortMovement( void );

	void			SetSyncToFollowingGesture( bool bSyncToFollowingGesture );
	bool			GetSyncToFollowingGesture( void );

	void			SetPlayOverScript( bool bPlayOverScript );
	bool			GetPlayOverScript( void );

	int				GetRampCount( void ) { return m_Ramp.GetCount(); };
	CExpressionSample *GetRamp( int index ) { return m_Ramp.Get( index ); };
	CExpressionSample *AddRamp( float time, float value, bool selected ) { return m_Ramp.Add( time, value, selected ); };
	void			DeleteRamp( int index ) { m_Ramp.Delete( index ); };
	void			ClearRamp( void ) { m_Ramp.Clear(); };
	void			ResortRamp( void ) { m_Ramp.Resort( this ); };
	CCurveData		*GetRamp( void ) { return &m_Ramp; };

	float			GetRampIntensity( float time ) { return m_Ramp.GetIntensity( this, time ); };

	// Calculates weighting for a given time
	float			GetIntensity( float scenetime );
	float			GetIntensityArea( float scenetime );

	// Calculates 0..1 completion for a given time
	float			GetCompletion( float time );

	// An end time of -1.0f means that the events is just triggered at the leading edge
	bool			HasEndTime( void );

	// Is the event something that can be sized ( a wave file, e.g. )
	bool			IsFixedLength( void );
	void			SetFixedLength( bool isfixedlength );

	// Move the start/end/both times by the specified dt (fixes up -1.0f endtimes)
	void			OffsetStartTime( float dt );
	void			OffsetEndTime( float dt );
	void			OffsetTime( float dt );

	// Snap to scene framerate
	void			SnapTimes( void );
	float			SnapTime( float t );

	CChoreoScene	*GetScene( void );
	void			SetScene( CChoreoScene *scene );

	// The actor the event is associated with
	void			SetActor( CChoreoActor *actor );
	CChoreoActor	*GetActor( void );

	// The channel the event is associated with
	void			SetChannel( CChoreoChannel *channel );
	CChoreoChannel	*GetChannel( void );

	// Get a more involved description of the event
	const char		*GetDescription( void );

	void			ClearAllRelativeTags( void );
	int				GetNumRelativeTags( void );
	CEventRelativeTag *GetRelativeTag( int tagnum );
	CEventRelativeTag *FindRelativeTag( const char *tagname );
	void			AddRelativeTag( const char *tagname, float percentage );
	void			RemoveRelativeTag( const char *tagname );
	
	bool			IsUsingRelativeTag( void );
	void			SetUsingRelativeTag( bool usetag, const char *tagname = 0, const char *wavname = 0);
	const char		*GetRelativeTagName( void );
	const char		*GetRelativeWavName( void );

	// Absolute tags
	typedef enum
	{
		PLAYBACK = 0,	// new timeline		- FIXME: should be stored as an absolute time
		ORIGINAL,		// original timeline - FIXME: should be stored at a fixed percentage of event
		
		NUM_ABS_TAG_TYPES,
	} AbsTagType;

	void			SetGestureSequenceDuration( float duration );
	bool			GetGestureSequenceDuration( float& duration );

	void			ClearAllAbsoluteTags( AbsTagType type );
	int				GetNumAbsoluteTags( AbsTagType type );
	CEventAbsoluteTag *GetAbsoluteTag( AbsTagType type, int tagnum );
	CEventAbsoluteTag *FindAbsoluteTag( AbsTagType type, const char *tagname );
	void			AddAbsoluteTag( AbsTagType type, const char *tagname, float t );
	void			RemoveAbsoluteTag( AbsTagType type, const char *tagname );
	bool			VerifyTagOrder( void );
	float			GetOriginalPercentageFromPlaybackPercentage( float t );
	float			GetPlaybackPercentageFromOriginalPercentage( float t );

	static const char *NameForAbsoluteTagType( AbsTagType t );
	static AbsTagType	TypeForAbsoluteTagName( const char *name );

	void			RescaleGestureTimes( float newstart, float newend, bool bMaintainAbsoluteTagPositions );
	bool			PreventTagOverlap( void );

	CEventAbsoluteTag *FindEntryTag( AbsTagType type );
	CEventAbsoluteTag *FindExitTag( AbsTagType type );

	// Flex animation type
	int				GetNumFlexAnimationTracks( void );
	CFlexAnimationTrack		*GetFlexAnimationTrack( int index );
	CFlexAnimationTrack		*AddTrack( const char *controllername );
	CFlexAnimationTrack		*FindTrack( const char *controllername );
	void			RemoveTrack( int index );
	void			RemoveAllTracks( void );
	void			OnEndTimeChanged( void );

	bool			GetTrackLookupSet( void );
	void			SetTrackLookupSet( bool set );

	// Flex Timing Tags (used by editor only)
	void			ClearAllTimingTags( void );
	int				GetNumTimingTags( void );
	CFlexTimingTag	*GetTimingTag( int tagnum );
	CFlexTimingTag	*FindTimingTag( const char *tagname );
	void			AddTimingTag( const char *tagname, float percentage, bool locked );
	void			RemoveTimingTag( const char *tagname );

	// Subscene ( embedded .vcd ) support
	void			SetSubScene( CChoreoScene *scene );
	CChoreoScene	*GetSubScene( void );

	bool			IsProcessing( void ) const;
	void			StartProcessing( IChoreoEventCallback *cb, CChoreoScene *scene, float t );
	void			ContinueProcessing( IChoreoEventCallback *cb, CChoreoScene *scene, float t );
	void			StopProcessing( IChoreoEventCallback *cb, CChoreoScene *scene, float t );
	bool			CheckProcessing( IChoreoEventCallback *cb, CChoreoScene *scene, float t );
	void			ResetProcessing( void );

	void			SetMixer( CAudioMixer *mixer );
	CAudioMixer		*GetMixer( void ) const;

	// Hack for LOOKAT in editor
	int				GetPitch( void ) const;
	void			SetPitch( int pitch );
	int				GetYaw( void ) const;
	void			SetYaw( int yaw );

	// For LOOP events
	void			SetLoopCount( int numloops );
	int				GetLoopCount( void );
	int				GetNumLoopsRemaining( void );
	void			SetNumLoopsRemaining( int loops );

	bool			IsMarkedForSave() const { return m_bMarkedForSave; }
	void			SetMarkedForSave( bool mark ) { m_bMarkedForSave = mark; }

	void			GetMovementStyle( char *style, int maxlen );
	void			GetDistanceStyle( char *style, int maxlen );

	int				GetGlobalID() const { return m_nGlobalID; }

	// Localization/CC support (close captioning and multiple wave file recombination)
	void			SetCloseCaptionType( CLOSECAPTION type );
	CLOSECAPTION	GetCloseCaptionType() const;
	void			SetCloseCaptionToken( char const *token );
	char const		*GetCloseCaptionToken() const;
	void			SetUsingCombinedFile( bool isusing );
	bool			IsUsingCombinedFile() const;
	void			SetRequiredCombinedChecksum( unsigned int checksum );
	unsigned int	GetRequiredCombinedChecksum();
	void			SetNumSlaves( int num );
	int				GetNumSlaves() const;
	void			SetLastSlaveEndTime( float t );
	float			GetLastSlaveEndTime() const;
	void			SetCloseCaptionTokenValid( bool valid );
	bool			GetCloseCaptionTokenValid() const;

	bool			ComputeCombinedBaseFileName( char *dest, int destlen, bool creategenderwildcard );
	bool			IsCombinedUsingGenderToken() const;
	void			SetCombinedUsingGenderToken( bool using_gender );

	bool			IsSuppressingCaptionAttenuation() const;
	void			SetSuppressingCaptionAttenuation( bool suppress );

	int				ValidateCombinedFile();

	// This returns false if the wave is CC_DISABLED or is a CC_SLAVE,
	//  otherwise it returns the actual m_szCCToken value, or if that's 
	//  blank it'll return the sounds.txt entry name (m_szParameters)
	bool			GetPlaybackCloseCaptionToken( char *dest, int destlen );

	void			ClearEventDependencies();
	void			AddEventDependency( CChoreoEvent *other );
	void			GetEventDependencies( CUtlVector< CChoreoEvent * >& list );

	void			SetActive( bool state );
	bool			GetActive() const;

	void			SetDefaultCurveType( int nCurveType );

	// Turn enum into string and vice versa
	static EVENTTYPE TypeForName( const char *name );
	static const char *NameForType( EVENTTYPE type );

	// Turn enum into string and vice versa
	static CLOSECAPTION CCTypeForName( const char *name );
	static const char *NameForCCType( CLOSECAPTION type );

private:

	// Declare copy constructor private to prevent accidental usage...
					CChoreoEvent(const CChoreoEvent& src );

	void SaveFlexAnimationsToBuffer( CUtlBuffer& buf, IChoreoStringPool *pStringPool );
	bool RestoreFlexAnimationsFromBuffer( CUtlBuffer& buf, IChoreoStringPool *pStringPool );

	float			GetBoundedAbsoluteTagPercentage( AbsTagType type, int tagnum );

	float			_GetIntensity( float time );

	// String bounds
	enum
	{
		MAX_CHOREOEVENT_NAME	= 128,
		MAX_PARAMETERS_STRING	= 128,
	};

	// Base initialization
	void			Init( CChoreoScene *scene );

	// Type of event
	byte			m_fType;

	// Close caption type
	byte			m_ccType;

	// Name of event
	ChoreoStr_t		m_Name;

	// Event parameters
	ChoreoStr_t		m_Parameters;
	ChoreoStr_t		m_Parameters2;
	ChoreoStr_t		m_Parameters3;

	// Event start time
	float			m_flStartTime;

	// Event end time ( -1.0f means no ending, just leading edge triggered )
	float			m_flEndTime;

	// Duration of underlying gesture sequence
	float			m_flGestureSequenceDuration;

	// For CChoreoEvent::LOOP
	int				m_nNumLoops; // -1 == no limit
	int				m_nLoopsRemaining;

	// Overall intensity curve
	CCurveData		m_Ramp;

	// Start time is computed based on length of item referenced by tagged name
	ChoreoStr_t		m_TagName;
	ChoreoStr_t		m_TagWavName;

	// Associated actor
	CChoreoActor	*m_pActor;
	// Associated channel
	CChoreoChannel	*m_pChannel;

	CUtlVector < CEventRelativeTag > m_RelativeTags;
	CUtlVector < CFlexTimingTag > m_TimingTags;
	CUtlVector < CEventAbsoluteTag > m_AbsoluteTags[ NUM_ABS_TAG_TYPES ];

	CUtlVector < CFlexAnimationTrack * > m_FlexAnimationTracks;

	CChoreoScene	*m_pSubScene;
	CAudioMixer		*m_pMixer;

	// Scene which owns this event
	CChoreoScene	*m_pScene;

	int				m_nPitch;
	int				m_nYaw;

	float			m_flDistanceToTarget;

	int				m_nGlobalID;

	ChoreoStr_t		m_CCToken;
	unsigned int	m_uRequiredCombinedChecksum; 
	// on master only, the combined file must have the same checksum to be useable
	int				m_nNumSlaves;
	// Only set on master, helps UI draw underbar
	float			m_flLastSlaveEndTime;	
	// true if the cc token was found in the cc manager's database

	CUtlVector< CChoreoEvent * >	m_Dependencies;

	int				m_nDefaultCurveType;

public:
	// used only during scrubbing of looping sequences
	float			m_flPrevCycle;
	float			m_flPrevTime;

	// Flags

	bool			m_bFixedLength:1;
	// True if this event must be "finished" before the next section can be started
	//  after playback is paused from a globalevent
	bool			m_bResumeCondition:1;
	bool			m_bUsesTag:1;
	bool			m_bTrackLookupSet:1;
	bool			m_bProcessing:1;
	bool			m_bLockBodyFacing:1;
	// Purely for save/load
	bool			m_bMarkedForSave:1;
	bool			m_bUsingCombinedSoundFile:1;
	bool			m_bCCTokenValid:1;   
	bool			m_bCombinedUsingGenderToken:1;

	bool			m_bSuppressCaptionAttenuation:1;

	bool			m_bForceShortMovement:1;
	bool			m_bSyncToFollowingGesture:1;
	bool			m_bActive:1;
	bool			m_bPlayOverScript:1;
};

#endif // CHOREOEVENT_H
