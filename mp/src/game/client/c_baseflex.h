//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
// Client-side CBasePlayer

#ifndef C_STUDIOFLEX_H
#define C_STUDIOFLEX_H
#pragma once


#include "c_baseanimating.h"
#include "c_baseanimatingoverlay.h"
#include "sceneentity_shared.h"

#include "utlvector.h"

//-----------------------------------------------------------------------------
// Purpose: Item in list of loaded scene files
//-----------------------------------------------------------------------------
class CFlexSceneFile
{
public:
	enum
	{
		MAX_FLEX_FILENAME = 128,
	};

	char			filename[ MAX_FLEX_FILENAME ];
	void			*buffer;
};

// For phoneme emphasis track
struct Emphasized_Phoneme;
class CSentence;

enum
{
	PHONEME_CLASS_WEAK = 0,
	PHONEME_CLASS_NORMAL,
	PHONEME_CLASS_STRONG,

	NUM_PHONEME_CLASSES
};

// Mapping for each loaded scene file used by this actor
struct FS_LocalToGlobal_t
{
	explicit FS_LocalToGlobal_t() :
	m_Key( 0 ),
		m_nCount( 0 ),
		m_Mapping( 0 )
	{
	}

	explicit FS_LocalToGlobal_t( const flexsettinghdr_t *key ) :
	m_Key( key ),
		m_nCount( 0 ),
		m_Mapping( 0 )
	{
	}		

	void SetCount( int count )
	{
		Assert( !m_Mapping );
		Assert( count > 0 );
		m_nCount = count;
		m_Mapping = new int[ m_nCount ];
		Q_memset( m_Mapping, 0, m_nCount * sizeof( int ) );
	}

	FS_LocalToGlobal_t( const FS_LocalToGlobal_t& src )
	{
		m_Key = src.m_Key;
		delete m_Mapping;
		m_Mapping = new int[ src.m_nCount ];
		Q_memcpy( m_Mapping, src.m_Mapping, src.m_nCount * sizeof( int ) );

		m_nCount = src.m_nCount;
	}

	~FS_LocalToGlobal_t()
	{
		delete m_Mapping;
		m_nCount = 0;
		m_Mapping = 0;
	}

	const flexsettinghdr_t	*m_Key;
	int						m_nCount;
	int						*m_Mapping;	
};

bool FlexSettingLessFunc( const FS_LocalToGlobal_t& lhs, const FS_LocalToGlobal_t& rhs );

class IHasLocalToGlobalFlexSettings
{
public:
	virtual void		EnsureTranslations( const flexsettinghdr_t *pSettinghdr ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct Emphasized_Phoneme
{
	// Global fields, setup at start
	char			classname[ 64 ];
	bool			required;
	// Global fields setup first time tracks played
	bool			basechecked;
	const flexsettinghdr_t *base;
	const flexsetting_t *exp;

	// Local fields, processed for each sentence
	bool			valid;
	float			amount;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_BaseFlex : public C_BaseAnimatingOverlay, public IHasLocalToGlobalFlexSettings
{
	DECLARE_CLASS( C_BaseFlex, C_BaseAnimatingOverlay );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

					C_BaseFlex();
	virtual			~C_BaseFlex();

	virtual void Spawn();

	virtual void InitPhonemeMappings();

	void		SetupMappings( char const *pchFileRoot );

	virtual CStudioHdr *OnNewModel( void );

	virtual void	StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );

	virtual void OnThreadedDrawSetup();

	// model specific
	virtual void BuildTransformations( CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed );
	static void		LinkToGlobalFlexControllers( CStudioHdr *hdr );
	virtual	void	SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );
	virtual	bool	SetupGlobalWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );
	static void		RunFlexDelay( int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights, float &flFlexDelayTime );
	virtual	void	SetupLocalWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );
	virtual bool	UsesFlexDelayedWeights();

	static	void	RunFlexRules( CStudioHdr *pStudioHdr, float *dest );

	virtual Vector	SetViewTarget( CStudioHdr *pStudioHdr );

	virtual bool	GetSoundSpatialization( SpatializationInfo_t& info );

	virtual void	GetToolRecordingState( KeyValues *msg );

	// Called at the lowest level to actually apply a flex animation
	void				AddFlexAnimation( CSceneEventInfo *info );

	void			SetFlexWeight( LocalFlexController_t index, float value );
	float			GetFlexWeight( LocalFlexController_t index );

	// Look up flex controller index by global name
	LocalFlexController_t				FindFlexController( const char *szName );

public:
	Vector			m_viewtarget;
	CInterpolatedVar< Vector >	m_iv_viewtarget;
	// indexed by model local flexcontroller
	float			m_flexWeight[MAXSTUDIOFLEXCTRL];
	CInterpolatedVarArray< float, MAXSTUDIOFLEXCTRL >	m_iv_flexWeight;

	int				m_blinktoggle;

	static int		AddGlobalFlexController( const char *szName );
	static char const *GetGlobalFlexControllerName( int idx );

	// bah, this should be unified with all prev/current stuff.

public:

	// Keep track of what scenes are being played
	void				StartChoreoScene( CChoreoScene *scene );
	void				RemoveChoreoScene( CChoreoScene *scene );

	// Start the specifics of an scene event
	virtual bool		StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, C_BaseEntity *pTarget );

	// Manipulation of events for the object
	// Should be called by think function to process all scene events
	// The default implementation resets m_flexWeight array and calls
	//  AddSceneEvents
	virtual void		ProcessSceneEvents( bool bFlexEvents );

	// Assumes m_flexWeight array has been set up, this adds the actual currently playing
	//  expressions to the flex weights and adds other scene events as needed
	virtual	bool		ProcessSceneEvent( bool bFlexEvents, CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );

	virtual bool		ProcessSequenceSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );

	// Remove all playing events
	void				ClearSceneEvents( CChoreoScene *scene, bool canceled );

	// Stop specifics of event
	virtual	bool		ClearSceneEvent( CSceneEventInfo *info, bool fastKill, bool canceled );

	// Add the event to the queue for this actor
	void				AddSceneEvent( CChoreoScene *scene, CChoreoEvent *event, C_BaseEntity *pTarget = NULL, bool bClientSide = false );

	// Remove the event from the queue for this actor
	void				RemoveSceneEvent( CChoreoScene *scene, CChoreoEvent *event, bool fastKill );

	// Checks to see if the event should be considered "completed"
	bool				CheckSceneEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );

	// Checks to see if a event should be considered "completed"
	virtual bool		CheckSceneEventCompletion( CSceneEventInfo *info, float currenttime, CChoreoScene *scene, CChoreoEvent *event );

	int					FlexControllerLocalToGlobal( const flexsettinghdr_t *pSettinghdr, int key );

	// IHasLocalToGlobalFlexSettings
	virtual void		EnsureTranslations( const flexsettinghdr_t *pSettinghdr );

	// For handling scene files
	void				*FindSceneFile( const char *filename );

private:

	bool RequestStartSequenceSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );

	bool ProcessFlexAnimationSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	bool ProcessFlexSettingSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	void AddFlexSetting( const char *expr, float scale, 
		const flexsettinghdr_t *pSettinghdr, bool newexpression );

	// Array of active SceneEvents, in order oldest to newest
	CUtlVector < CSceneEventInfo >		m_SceneEvents;
	CUtlVector < CChoreoScene * >		m_ActiveChoreoScenes;

	bool				HasSceneEvents() const;

private:
	CUtlRBTree< FS_LocalToGlobal_t, unsigned short > m_LocalToGlobal;

	float			m_blinktime;
	int				m_prevblinktoggle;

	int				m_iBlink;
	LocalFlexController_t				m_iEyeUpdown;
	LocalFlexController_t				m_iEyeRightleft;
	bool			m_bSearchedForEyeFlexes;
	int				m_iMouthAttachment;

	float			m_flFlexDelayTime;
	float			*m_flFlexDelayedWeight;
	int				m_cFlexDelayedWeight;

	// shared flex controllers
	static int		g_numflexcontrollers;
	static char		*g_flexcontroller[MAXSTUDIOFLEXCTRL*4]; // room for global set of flexcontrollers
	static float	g_flexweight[MAXSTUDIOFLEXDESC];

protected:

	Emphasized_Phoneme m_PhonemeClasses[ NUM_PHONEME_CLASSES ];

private:

	C_BaseFlex( const C_BaseFlex & ); // not defined, not accessible

	const flexsetting_t *FindNamedSetting( const flexsettinghdr_t *pSettinghdr, const char *expr );

	void			ProcessVisemes( Emphasized_Phoneme *classes );
	void			AddVisemesForSentence( Emphasized_Phoneme *classes, float emphasis_intensity, CSentence *sentence, float t, float dt, bool juststarted );
	void			AddViseme( Emphasized_Phoneme *classes, float emphasis_intensity, int phoneme, float scale, bool newexpression );
	bool			SetupEmphasisBlend( Emphasized_Phoneme *classes, int phoneme );
	void			ComputeBlendedSetting( Emphasized_Phoneme *classes, float emphasis_intensity );

#ifdef HL2_CLIENT_DLL
public:

	Vector			m_vecLean;
	CInterpolatedVar< Vector >	m_iv_vecLean;
	Vector			m_vecShift;
	CInterpolatedVar< Vector >	m_iv_vecShift;
#endif
};


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CFlexSceneFileManager : CAutoGameSystem
{
public:

	CFlexSceneFileManager() : CAutoGameSystem( "CFlexSceneFileManager" )
	{
	}

	virtual bool Init();
	virtual void Shutdown();

	void EnsureTranslations( IHasLocalToGlobalFlexSettings *instance, const flexsettinghdr_t *pSettinghdr );
	void *FindSceneFile( IHasLocalToGlobalFlexSettings *instance, const char *filename, bool allowBlockingIO );

private:
	void DeleteSceneFiles();

	CUtlVector< CFlexSceneFile * > m_FileList;
};


//-----------------------------------------------------------------------------
// Do we have active expressions?
//-----------------------------------------------------------------------------
inline bool C_BaseFlex::HasSceneEvents() const
{
	return m_SceneEvents.Count() != 0;
}


EXTERN_RECV_TABLE(DT_BaseFlex);

float *GetVisemeWeights( int phoneme );


#endif // C_STUDIOFLEX_H




