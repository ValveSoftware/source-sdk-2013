//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Client handler for instruction players how to play
//
//=============================================================================//

#ifndef _C_BASELESSON_H_
#define _C_BASELESSON_H_


#include "GameEventListener.h"
#include "hud_locator_target.h"


#define DECLARE_LESSON( _lessonClassName, _baseLessonClassName ) \
	typedef _baseLessonClassName BaseClass;\
	typedef _lessonClassName ThisClass;\
	_lessonClassName( const char *pchName, bool bIsDefaultHolder, bool bIsOpenOpportunity )\
		: _baseLessonClassName( pchName, bIsDefaultHolder, bIsOpenOpportunity )\
	{\
		Init();\
	}


enum LessonInstanceType
{
	LESSON_INSTANCE_MULTIPLE,
	LESSON_INSTANCE_SINGLE_OPEN,
	LESSON_INSTANCE_FIXED_REPLACE,
	LESSON_INSTANCE_SINGLE_ACTIVE,

	LESSON_INSTANCE_TYPE_TOTAL
};


// This is used to solve a problem where bots can take the place of a player, where on or the other don't have valid entities on the client at the same time
#define MAX_DELAYED_PLAYER_SWAPS 8

struct delayed_player_swap_t
{
	CHandle<C_BaseEntity> *phHandleToChange;
	int iNewUserID;

	delayed_player_swap_t( void )
	{
		phHandleToChange = NULL;
		iNewUserID = -1;
	}
};


abstract_class CBaseLesson : public CGameEventListener
{
public:
	CBaseLesson( const char *pchName, bool bIsDefaultHolder, bool bIsOpenOpportunity );
	virtual ~CBaseLesson( void );

	void AddPrerequisite( const char *pchLessonName );

	const CGameInstructorSymbol& GetNameSymbol( void ) const { return m_stringName; }
	const char * GetName( void ) const { return m_stringName.String(); }
	int GetPriority( void ) const { return m_iPriority;	}
	const char * GetCloseReason( void ) const { return m_stringCloseReason.String(); }
	void SetCloseReason( const char *pchReason ) { m_stringCloseReason = pchReason; }

	CBaseLesson* GetRoot( void ) const { return m_pRoot; }
	void SetRoot( CBaseLesson *pRoot );
	const CUtlVector < CBaseLesson * >* GetChildren( void ) const { return &m_OpenOpportunities; }

	float GetInitTime( void ) { return m_fInitTime; }
	void SetStartTime( void ) { m_fStartTime = gpGlobals->curtime; }
	void ResetStartTime( void ) { m_fStartTime = 0.0f; m_bHasPlayedSound = false; }

	bool ShouldShowSpew( void );
	bool NoPriority( void ) const;
	bool IsDefaultHolder( void ) const { return m_bIsDefaultHolder; }
	bool IsOpenOpportunity( void ) const { return m_bIsOpenOpportunity; }
	bool IsLocked( void ) const;
	bool CanOpenWhenDead( void ) const { return m_bCanOpenWhenDead; }
	bool IsInstructing( void ) const { return ( m_fStartTime > 0.0f ); }
	bool IsLearned( void ) const;
	bool PrerequisitesHaveBeenMet( void ) const;
	bool IsTimedOut( void );

	int InstanceType( void ) const { return m_iInstanceType; }
	const CGameInstructorSymbol& GetReplaceKeySymbol( void ) const { return m_stringReplaceKey; }
	const char* GetReplaceKey( void ) const { return m_stringReplaceKey.String(); }
	int GetFixedInstancesMax( void ) const { return m_iFixedInstancesMax; }
	bool ShouldReplaceOnlyWhenStopped( void ) const { return m_bReplaceOnlyWhenStopped; }
	void SetInstanceActive( bool bInstanceActive ) { m_bInstanceActive = bInstanceActive; }
	bool IsInstanceActive( void ) const { return m_bInstanceActive; }

	void ResetDisplaysAndSuccesses( void );
	bool IncDisplayCount( void );
	bool IncSuccessCount( void );
	void SetDisplayCount( int iDisplayCount ) { m_iDisplayCount = iDisplayCount; }
	void SetSuccessCount( int iSuccessCount ) { m_iSuccessCount = iSuccessCount; }
	int GetDisplayCount( void ) const { return m_iDisplayCount; }
	int GetSuccessCount( void ) const { return m_iSuccessCount; }
	int GetDisplayLimit( void ) const { return m_iDisplayLimit; }
	int GetSuccessLimit( void ) const { return m_iSuccessLimit; }

	void Init( void );	// NOT virtual, each constructor calls their own
	virtual void InitPrerequisites( void ) {};
	virtual void Start( void ) = 0;
	virtual void Stop( void ) = 0;
	virtual void OnOpen( void ) {};
	virtual void Update( void ) {};
	virtual void UpdateInactive( void ) {};

	virtual bool ShouldDisplay( void ) const { return true; }
	virtual bool IsVisible( void ) const { return true; }
	virtual bool WasDisplayed( void ) const { return m_bWasDisplayed ? true : false; }
	virtual void SwapOutPlayers( int iOldUserID, int iNewUserID ) {}
	virtual void TakePlaceOf( CBaseLesson *pLesson );

	const char *GetGroup() { return m_szLessonGroup.String(); }
	void	SetEnabled( bool bEnabled ) { m_bDisabled = !bEnabled; }

protected:
	void MarkSucceeded( void );
	void CloseOpportunity( const char *pchReason );
	bool DoDelayedPlayerSwaps( void ) const;

private:

	CBaseLesson							*m_pRoot;
	CUtlVector < CBaseLesson * >		m_OpenOpportunities;
	CUtlVector < const CBaseLesson * >	m_Prerequisites;

	CGameInstructorSymbol		m_stringCloseReason;
	CGameInstructorSymbol		m_stringName;

	bool		m_bInstanceActive : 1;
	bool		m_bSuccessCounted : 1;
	bool		m_bIsDefaultHolder : 1;
	bool		m_bIsOpenOpportunity : 1;

protected:
	LessonInstanceType	m_iInstanceType;

	int						m_iPriority;
	CGameInstructorSymbol	m_stringReplaceKey;
	int						m_iFixedInstancesMax;
	bool					m_bReplaceOnlyWhenStopped;
	int						m_iTeam;
	bool					m_bOnlyKeyboard;
	bool					m_bOnlyGamepad;

	int		m_iDisplayLimit;
	int		m_iDisplayCount;
	bool	m_bWasDisplayed;
	int		m_iSuccessLimit;
	int		m_iSuccessCount;

	float	m_fLockDuration;
	float	m_fTimeout;
	float	m_fInitTime;
	float	m_fStartTime;
	float	m_fLockTime;
	float	m_fUpdateInterval;
	bool 	m_bHasPlayedSound;

	CGameInstructorSymbol m_szStartSound;
	CGameInstructorSymbol m_szLessonGroup;

	bool	m_bCanOpenWhenDead;
	bool	m_bBumpWithTimeoutWhenLearned;
	bool	m_bCanTimeoutWhileInactive;
	bool	m_bDisabled;

	// Right now we can only queue up 4 swaps...
	// this number can be increased if more entity handle scripted variables are added
	mutable delayed_player_swap_t	m_pDelayedPlayerSwap[ MAX_DELAYED_PLAYER_SWAPS ];
	mutable int						m_iNumDelayedPlayerSwaps;

public:

	// Colors for console spew in verbose mode
	static Color	m_rgbaVerboseHeader;
	static Color	m_rgbaVerbosePlain;
	static Color	m_rgbaVerboseName;
	static Color	m_rgbaVerboseOpen;
	static Color	m_rgbaVerboseClose;
	static Color	m_rgbaVerboseSuccess;
	static Color	m_rgbaVerboseUpdate;
};


class CTextLesson : public CBaseLesson
{
public:
	DECLARE_LESSON( CTextLesson, CBaseLesson );

	void Init( void );	// NOT virtual, each constructor calls their own
	virtual void Start( void );
	virtual void Stop( void );

protected:

	CGameInstructorSymbol	m_szDisplayText;
	CGameInstructorSymbol	m_szDisplayParamText;
	CGameInstructorSymbol	m_szBinding;
	CGameInstructorSymbol	m_szGamepadBinding;
};


class CIconLesson : public CTextLesson
{
public:
	DECLARE_LESSON( CIconLesson, CTextLesson );

	void Init( void ); 	// NOT virtual, each constructor calls their own
	virtual void Start( void );
	virtual void Stop( void );
	virtual void Update( void );
	virtual void UpdateInactive( void );

	virtual bool ShouldDisplay( void ) const;
	virtual bool IsVisible( void ) const;
	virtual void SwapOutPlayers( int iOldUserID, int iNewUserID );
	virtual void TakePlaceOf( CBaseLesson *pLesson );

	void SetLocatorBinding( CLocatorTarget * pLocatorTarget );

	const char *GetCaptionColorString()	{ return m_szCaptionColor.String(); }

	bool IsPresentComplete( void );
	void PresentStart( void );
	void PresentEnd( void );

private:
	virtual void UpdateLocatorTarget( CLocatorTarget *pLocatorTarget, C_BaseEntity *pIconTarget );

#ifdef MAPBASE
	Vector GetIconTargetPosition( C_BaseEntity *pIconTarget );
#endif

protected:
	CHandle<C_BaseEntity>		m_hIconTarget;
	CGameInstructorSymbol		m_szVguiTargetName;
	CGameInstructorSymbol		m_szVguiTargetLookup;
	int		m_nVguiTargetEdge;
	float	m_flUpOffset;
	float	m_flRelativeUpOffset;
	float	m_fFixedPositionX;
	float	m_fFixedPositionY;

	int		m_hLocatorTarget;
	int		m_iFlags;

	float	m_fRange;
	float	m_fCurrentDistance;
	float	m_fOnScreenStartTime;
	float	m_fUpdateDistanceTime;

	CGameInstructorSymbol	m_szOnscreenIcon;
	CGameInstructorSymbol	m_szOffscreenIcon;
	CGameInstructorSymbol	m_szCaptionColor;

	bool	m_bFixedPosition;
	bool	m_bNoIconTarget;
	bool	m_bAllowNodrawTarget;
	bool	m_bVisible;
	bool	m_bShowWhenOccluded;
	bool	m_bNoOffscreen;
	bool	m_bForceCaption;

#ifdef MAPBASE
	int		m_iIconTargetPos;

	enum
	{
		ICON_TARGET_EYE_POSITION,
		ICON_TARGET_ORIGIN,
		ICON_TARGET_CENTER,
	};

	CGameInstructorSymbol	m_szHudHint;
#endif
};

enum LessonAction
{
	LESSON_ACTION_NONE,

	LESSON_ACTION_SCOPE_IN,
	LESSON_ACTION_SCOPE_OUT,
	LESSON_ACTION_CLOSE,
	LESSON_ACTION_SUCCESS,
	LESSON_ACTION_LOCK,
	LESSON_ACTION_PRESENT_COMPLETE,
	LESSON_ACTION_PRESENT_START,
	LESSON_ACTION_PRESENT_END,

	LESSON_ACTION_REFERENCE_OPEN,

	LESSON_ACTION_SET,
	LESSON_ACTION_ADD,
	LESSON_ACTION_SUBTRACT,
	LESSON_ACTION_MULTIPLY,
	LESSON_ACTION_IS,
	LESSON_ACTION_LESS_THAN,
	LESSON_ACTION_HAS_PREFIX,
	LESSON_ACTION_HAS_BIT,
	LESSON_ACTION_BIT_COUNT_IS,
	LESSON_ACTION_BIT_COUNT_LESS_THAN,

	LESSON_ACTION_GET_DISTANCE,
	LESSON_ACTION_GET_ANGULAR_DISTANCE,
	LESSON_ACTION_GET_PLAYER_DISPLAY_NAME,
	LESSON_ACTION_CLASSNAME_IS,
	LESSON_ACTION_MODELNAME_IS,
	LESSON_ACTION_TEAM_IS,
	LESSON_ACTION_HEALTH_LESS_THAN,
	LESSON_ACTION_HEALTH_PERCENTAGE_LESS_THAN,
	LESSON_ACTION_GET_ACTIVE_WEAPON,
	LESSON_ACTION_WEAPON_IS,
	LESSON_ACTION_WEAPON_HAS,
	LESSON_ACTION_GET_ACTIVE_WEAPON_SLOT,
	LESSON_ACTION_GET_WEAPON_SLOT,
	LESSON_ACTION_GET_WEAPON_IN_SLOT,
	LESSON_ACTION_CLIP_PERCENTAGE_LESS_THAN,
	LESSON_ACTION_WEAPON_AMMO_LOW,
	LESSON_ACTION_WEAPON_AMMO_FULL,
	LESSON_ACTION_WEAPON_AMMO_EMPTY,
	LESSON_ACTION_WEAPON_CAN_USE,
	LESSON_ACTION_USE_TARGET_IS,
	LESSON_ACTION_GET_USE_TARGET,
	LESSON_ACTION_GET_POTENTIAL_USE_TARGET,

	// Enum continued in Mod_LessonAction
	LESSON_ACTION_MOD_START,
};

struct LessonElement_t
{
	int						iVariable;
	int						iParamVarIndex;
	int						iAction;
	_fieldtypes				paramType;
	CGameInstructorSymbol	szParam;
	bool					bNot : 1;
	bool					bOptionalParam : 1;

	LessonElement_t( int p_iVariable, int p_iAction, bool p_bNot, bool p_bOptionalParam, const char *pchParam, int p_iParamVarIndex, _fieldtypes p_paramType )
	{
		iVariable = p_iVariable;
		iAction = p_iAction;
		bNot = p_bNot;
		bOptionalParam = p_bOptionalParam;
		szParam = pchParam;
		iParamVarIndex = p_iParamVarIndex;
		paramType = p_paramType;
	}

	LessonElement_t( const LessonElement_t &p_LessonElement )
	{
		iVariable = p_LessonElement.iVariable;
		iAction = p_LessonElement.iAction;
		bNot = p_LessonElement.bNot;
		bOptionalParam = p_LessonElement.bOptionalParam;
		szParam = p_LessonElement.szParam;
		iParamVarIndex = p_LessonElement.iParamVarIndex;
		paramType = p_LessonElement.paramType;
	}
};

struct LessonEvent_t
{
	CUtlVector< LessonElement_t >	elements;
	CGameInstructorSymbol			szEventName;
};

class CScriptedIconLesson : public CIconLesson
{
public:
	DECLARE_LESSON( CScriptedIconLesson, CIconLesson )

	virtual ~CScriptedIconLesson( void );

	static void PreReadLessonsFromFile( void );
	static void Mod_PreReadLessonsFromFile( void );

	void Init( void );	// NOT virtual, each constructor calls their own
	virtual void InitPrerequisites( void );
	virtual void OnOpen( void );
	virtual void Update( void );

	virtual void SwapOutPlayers( int iOldUserID, int iNewUserID );

	virtual void FireGameEvent( IGameEvent *event );
	virtual void ProcessOpenGameEvents( const CScriptedIconLesson *pRootLesson, const char *name, IGameEvent *event );
	virtual void ProcessCloseGameEvents( const CScriptedIconLesson *pRootLesson, const char *name, IGameEvent *event );
	virtual void ProcessSuccessGameEvents( const CScriptedIconLesson *pRootLesson, const char *name, IGameEvent *event );

	CUtlVector< LessonEvent_t >& GetOpenEvents( void ) { return m_OpenEvents; }
	CUtlVector< LessonEvent_t >& GetCloseEvents( void ) { return m_CloseEvents; }
	CUtlVector< LessonEvent_t >& GetSuccessEvents( void ) { return m_SuccessEvents; }
	CUtlVector< LessonEvent_t >& GetOnOpenEvents( void ) { return m_OnOpenEvents; }
	CUtlVector< LessonEvent_t >& GetUpdateEvents( void ) { return m_UpdateEvents; }

	bool ProcessElements( IGameEvent *event, const CUtlVector< LessonElement_t > *pElements );

private:
	void InitElementsFromKeys( CUtlVector< LessonElement_t > *pLessonElements, KeyValues *pKey );
	void InitElementsFromElements( CUtlVector< LessonElement_t > *pLessonElements, const CUtlVector< LessonElement_t > *pLessonElements2 );

	void InitFromKeys( KeyValues *pKey );

	bool ProcessElement( IGameEvent *event, const LessonElement_t *pLessonElement, bool bInFailedScope );

	bool ProcessElementAction( int iAction, bool bNot, const char *pchVarName, float &bVar, const CGameInstructorSymbol *pchParamName, float fParam );
	bool ProcessElementAction( int iAction, bool bNot, const char *pchVarName, int &bVar, const CGameInstructorSymbol *pchParamName, float fParam );
	bool ProcessElementAction( int iAction, bool bNot, const char *pchVarName, bool &bVar, const CGameInstructorSymbol *pchParamName, float fParam );
	bool ProcessElementAction( int iAction, bool bNot, const char *pchVarName, EHANDLE &hVar, const CGameInstructorSymbol *pchParamName, float fParam, C_BaseEntity *pParam, const char *pchParam );
	bool ProcessElementAction( int iAction, bool bNot, const char *pchVarName, CGameInstructorSymbol *pchVar, const CGameInstructorSymbol *pchParamName, const char *pchParam );

	// Implemented per mod so they can have custom actions
	bool Mod_ProcessElementAction( int iAction, bool bNot, const char *pchVarName, EHANDLE &hVar, const CGameInstructorSymbol *pchParamName, float fParam, C_BaseEntity *pParam, const char *pchParam, bool &bModHandled );

	LessonEvent_t * AddOpenEvent( void );
	LessonEvent_t * AddCloseEvent( void );
	LessonEvent_t * AddSuccessEvent( void );
	LessonEvent_t * AddOnOpenEvent( void );
	LessonEvent_t * AddUpdateEvent( void );

private:
	static CUtlDict< int, int > CScriptedIconLesson::LessonActionMap;

	EHANDLE					m_hLocalPlayer;
	float					m_fOutput;
	CHandle<C_BaseEntity>	m_hEntity1;
	CHandle<C_BaseEntity>	m_hEntity2;
	CGameInstructorSymbol	m_szString1;
	CGameInstructorSymbol	m_szString2;
	int						m_iInteger1;
	int						m_iInteger2;
	float					m_fFloat1;
	float					m_fFloat2;

	CUtlVector< CGameInstructorSymbol >	m_PrerequisiteNames;
	CUtlVector< LessonEvent_t >	m_OpenEvents;
	CUtlVector< LessonEvent_t >	m_CloseEvents;
	CUtlVector< LessonEvent_t >	m_SuccessEvents;
	CUtlVector< LessonEvent_t >	m_OnOpenEvents;
	CUtlVector< LessonEvent_t >	m_UpdateEvents;

	float					m_fUpdateEventTime;
	CScriptedIconLesson		*m_pDefaultHolder;

	int		m_iScopeDepth;

	// Need this to get offsets to scripted variables
	friend class LessonVariableInfo;
	friend int LessonActionFromString( const char *pchName );
};


#endif // _C_BASELESSON_H_
