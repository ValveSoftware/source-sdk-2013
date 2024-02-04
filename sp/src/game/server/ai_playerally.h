//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef AI_PLAYERALLY_H
#define AI_PLAYERALLY_H

#include "utlmap.h"
#include "simtimer.h"
#include "AI_Criteria.h"
#include "ai_baseactor.h"
#include "ai_speechfilter.h"
#ifndef _WIN32
#undef min
#endif
#include "stdstring.h"
#ifndef _WIN32
#undef MINMAX_H
#include "minmax.h"
#endif

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------

#define TLK_ANSWER 			"TLK_ANSWER"
#define TLK_ANSWER_HELLO	"TLK_ANSWER_HELLO"
#define TLK_QUESTION 		"TLK_QUESTION"
#define TLK_IDLE 			"TLK_IDLE"
#define TLK_STARE 			"TLK_STARE"
#define TLK_LOOK 			"TLK_LOOK"	// player looking at player for a second
#define TLK_USE				"TLK_USE"
#define TLK_STARTFOLLOW 	"TLK_STARTFOLLOW"
#define TLK_STOPFOLLOW		"TLK_STOPFOLLOW"
#define TLK_JOINPLAYER		"TLK_JOINPLAYER"
#define TLK_STOP 			"TLK_STOP"
#define TLK_NOSHOOT			"TLK_NOSHOOT"
#define TLK_HELLO 			"TLK_HELLO"
#define TLK_PHELLO 			"TLK_PHELLO"
#define TLK_HELLO_NPC		"TLK_HELLO_NPC"
#define TLK_PIDLE 			"TLK_PIDLE"
#define TLK_PQUESTION 		"TLK_PQUESTION"
#define TLK_PLHURT1 		"TLK_PLHURT1"
#define TLK_PLHURT2 		"TLK_PLHURT2"
#define TLK_PLHURT3 		"TLK_PLHURT3"
#define TLK_PLHURT			"TLK_PLHURT"
#define TLK_PLPUSH 			"TLK_PLPUSH"
#define TLK_PLRELOAD		"TLK_PLRELOAD"
#define TLK_SMELL 			"TLK_SMELL"
#define TLK_SHOT			"TLK_SHOT"
#define TLK_WOUND 			"TLK_WOUND"
#define TLK_MORTAL 			"TLK_MORTAL"
#define TLK_DANGER			"TLK_DANGER"
#define TLK_SEE_COMBINE		"TLK_SEE_COMBINE"
#define TLK_ENEMY_DEAD		"TLK_ENEMY_DEAD"
#define TLK_ALYX_ENEMY_DEAD "TLK_ALYX_ENEMY_DEAD"
#define TLK_SELECTED		"TLK_SELECTED"	// selected by player in command mode.
#define TLK_COMMANDED		"TLK_COMMANDED" // received orders from player in command mode
#define TLK_COMMAND_FAILED	"TLK_COMMAND_FAILED" 
#define TLK_DENY_COMMAND	"TLK_DENY_COMMAND" // designer has asked this NPC to politely deny player commands to move the squad
#define TLK_BETRAYED		"TLK_BETRAYED"	// player killed an ally in front of me.
#define TLK_ALLY_KILLED		"TLK_ALLY_KILLED" // witnessed an ally die some other way.
#define TLK_ATTACKING		"TLK_ATTACKING" // about to fire my weapon at a target
#define TLK_HEAL			"TLK_HEAL" // healing someone
#define TLK_GIVEAMMO		"TLK_GIVEAMMO" // giving ammo to someone
#define TLK_DEATH			"TLK_DEATH"	// Death rattle
#define TLK_HELP_ME			"TLK_HELP_ME" // call out to the player for help
#define TLK_PLYR_PHYSATK	"TLK_PLYR_PHYSATK"	// Player's attacked me with a thrown physics object
#define TLK_NEWWEAPON		"TLK_NEWWEAPON"
#define TLK_PLDEAD			"TLK_PLDEAD"
#define TLK_HIDEANDRELOAD	"TLK_HIDEANDRELOAD"
#define TLK_STARTCOMBAT		"TLK_STARTCOMBAT"
#define TLK_WATCHOUT		"TLK_WATCHOUT"
#define TLK_MOBBED			"TLK_MOBBED"
#define TLK_MANY_ENEMIES	"TLK_MANY_ENEMIES"
#define TLK_FLASHLIGHT_ILLUM		"TLK_FLASHLIGHT_ILLUM"
#define TLK_FLASHLIGHT_ON			"TLK_FLASHLIGHT_ON"		// player turned on flashlight
#define TLK_FLASHLIGHT_OFF			"TLK_FLASHLIGHT_OFF"	// player turned off flashlight
#define TLK_DARKNESS_LOSTPLAYER		"TLK_DARKNESS_LOSTPLAYER"
#define TLK_DARKNESS_FOUNDPLAYER	"TLK_DARKNESS_FOUNDPLAYER"
#define TLK_DARKNESS_UNKNOWN_WOUND	"TLK_DARKNESS_UNKNOWN_WOUND"
#define TLK_DARKNESS_HEARDSOUND		"TLK_DARKNESS_HEARDSOUND"
#define TLK_DARKNESS_LOSTENEMY_BY_FLASHLIGHT			"TLK_DARKNESS_LOSTENEMY_BY_FLASHLIGHT"
#define TLK_DARKNESS_LOSTENEMY_BY_FLASHLIGHT_EXPIRED	"TLK_DARKNESS_LOSTENEMY_BY_FLASHLIGHT_EXPIRED"
#define TLK_DARKNESS_FOUNDENEMY_BY_FLASHLIGHT			"TLK_DARKNESS_FOUNDENEMY_BY_FLASHLIGHT"
#define TLK_DARKNESS_FLASHLIGHT_EXPIRED					"TLK_DARKNESS_FLASHLIGHT_EXPIRED"	// flashlight expired while not in combat
#define TLK_DARKNESS_ENEMY_IN_DARKNESS					"TLK_DARKNESS_ENEMY_IN_DARKNESS"	// have an enemy, but it's in the darkness
#define TLK_SPOTTED_INCOMING_HEADCRAB					"TLK_SPOTTED_INCOMING_HEADCRAB"
#define TLK_CANT_INTERACT_NOW				"TLK_CANT_INTERACT_NOW" // to busy to interact with an object the player is holding up to me
#define TLK_ALLY_IN_BARNACLE				"TLK_ALLY_IN_BARNACLE"	// Barnacle is lifting my buddy!
#define TLK_SELF_IN_BARNACLE				"TLK_SELF_IN_BARNACLE" // I was grabbed by a barnacle!
#define TLK_FOUNDPLAYER						"TLK_FOUNDPLAYER"
#define TLK_PLAYER_KILLED_NPC				"TLK_PLAYER_KILLED_NPC"
#define TLK_ENEMY_BURNING					"TLK_ENEMY_BURNING"
#define TLK_SPOTTED_ZOMBIE_WAKEUP			"TLK_SPOTTED_ZOMBIE_WAKEUP"
#define TLK_SPOTTED_HEADCRAB_LEAVING_ZOMBIE	"TLK_SPOTTED_HEADCRAB_LEAVING_ZOMBIE"
#define TLK_DANGER_ZOMBINE_GRENADE			"TLK_DANGER_ZOMBINE_GRENADE"
#define TLK_BALLSOCKETED					"TLK_BALLSOCKETED"

// Vehicle passenger
#define	TLK_PASSENGER_WARN_COLLISION	"TLK_PASSENGER_WARN_COLLISION"	// About to collide with something
#define	TLK_PASSENGER_IMPACT			"TLK_PASSENGER_IMPACT"			// Just hit something
#define	TLK_PASSENGER_OVERTURNED		"TLK_PASSENGER_OVERTURNED"		// Vehicle has just overturned
#define	TLK_PASSENGER_REQUEST_UPRIGHT	"TLK_PASSENGER_REQUEST_UPRIGHT" // Vehicle needs to be put upright
#define TLK_PASSENGER_ERRATIC_DRIVING	"TLK_PASSENGER_ERRATIC_DRIVING"	// Vehicle is moving erratically
#define TLK_PASSENGER_VEHICLE_STARTED	"TLK_PASSENGER_VEHICLE_STARTED" // Vehicle has started moving
#define	TLK_PASSENGER_VEHICLE_STOPPED	"TLK_PASSENGER_VEHICLE_STOPPED"	// Vehicle has stopped moving
#define TLK_PASSENGER_BEGIN_ENTRANCE	"TLK_PASSENGER_BEGIN_ENTRANCE"	// Passenger started entering
#define TLK_PASSENGER_FINISH_ENTRANCE	"TLK_PASSENGER_FINISH_ENTRANCE" // Passenger finished entering (is in seat)
#define TLK_PASSENGER_BEGIN_EXIT		"TLK_PASSENGER_BEGIN_EXIT"		// Passenger started exiting
#define TLK_PASSENGER_FINISH_EXIT		"TLK_PASSENGER_FINISH_EXIT"		// Passenger finished exiting (seat is vacated)
#define TLK_PASSENGER_PLAYER_ENTERED	"TLK_PASSENGER_PLAYER_ENTERED"	// Player entered the vehicle
#define TLK_PASSENGER_PLAYER_EXITED		"TLK_PASSENGER_PLAYER_EXITED"	// Player exited the vehicle
#define TLK_PASSENGER_NEW_RADAR_CONTACT	"TLK_PASSENGER_NEW_RADAR_CONTACT"	// Noticed a brand new contact on the radar
#define TLK_PASSENGER_PUNTED			"TLK_PASSENGER_PUNTED"			// The player has punted us while we're sitting in the vehicle

// Vortigaunt
#define TLK_VORTIGAUNT_DISPEL	"TLK_VORTIGAUNT_DISPEL"	// Dispel attack starting

// resume is "as I was saying..." or "anyhow..."
#define TLK_RESUME 		"TLK_RESUME"

// tourguide stuff below
#define TLK_TGSTAYPUT 	"TLK_TGSTAYPUT"
#define TLK_TGFIND 		"TLK_TGFIND"
#define TLK_TGSEEK 		"TLK_TGSEEK"
#define TLK_TGLOSTYOU 	"TLK_TGLOSTYOU"
#define TLK_TGCATCHUP 	"TLK_TGCATCHUP"
#define TLK_TGENDTOUR 	"TLK_TGENDTOUR"

#ifdef MAPBASE
// Additional concepts for companions in mods
#define TLK_TAKING_FIRE	"TLK_TAKING_FIRE"	// Someone fired at me (regardless of whether I was hit)
#define TLK_NEW_ENEMY	"TLK_NEW_ENEMY"		// A new enemy appeared while combat was already in progress
#define TLK_COMBAT_IDLE	"TLK_COMBAT_IDLE"	// Similar to TLK_ATTACKING, but specifically for when *not* currently attacking (e.g. when in cover or reloading)
#endif

//-----------------------------------------------------------------------------

#define TALKRANGE_MIN 500.0				// don't talk to anyone farther away than this

//-----------------------------------------------------------------------------

#define TALKER_STARE_DIST	128				// anyone closer than this and looking at me is probably staring at me.

#define TALKER_DEFER_IDLE_SPEAK_MIN		10
#define TALKER_DEFER_IDLE_SPEAK_MAX		20

//-----------------------------------------------------------------------------

class CAI_PlayerAlly;

//-----------------------------------------------------------------------------
//
// CLASS: CAI_AllySpeechManager
//
//-----------------------------------------------------------------------------

enum ConceptCategory_t
{
	SPEECH_IDLE,
	SPEECH_IMPORTANT,
	SPEECH_PRIORITY,

	SPEECH_NUM_CATEGORIES
};

struct ConceptCategoryInfo_t
{
	float	minGlobalDelay;
	float	maxGlobalDelay;
	float	minPersonalDelay;
	float	maxPersonalDelay;
};

enum AIConceptFlags_t
{
	AICF_DEFAULT 			= 0,
	AICF_SPEAK_ONCE			= 0x01,
	AICF_PROPAGATE_SPOKEN	= 0x02,
	AICF_TARGET_PLAYER		= 0x04,
	AICF_QUESTION			= 0x08,
	AICF_ANSWER				= 0x10,
}; 

struct ConceptInfo_t
{
	AIConcept_t			concept;
	ConceptCategory_t   category;
	float				minGlobalCategoryDelay;
	float				maxGlobalCategoryDelay;
	float				minPersonalCategoryDelay;
	float				maxPersonalCategoryDelay;
	float				minConceptDelay;
	float				maxConceptDelay;
	int 				flags;
};

//-------------------------------------

class CAI_AllySpeechManager : public CLogicalEntity
{
	DECLARE_CLASS( CAI_AllySpeechManager, CLogicalEntity );
public:
	CAI_AllySpeechManager();
	~CAI_AllySpeechManager();
	
	void Spawn();

	void AddCustomConcept( const ConceptInfo_t &conceptInfo );
	ConceptCategoryInfo_t *GetConceptCategoryInfo( ConceptCategory_t category );
	ConceptInfo_t *GetConceptInfo( AIConcept_t concept );
	void OnSpokeConcept( CAI_PlayerAlly *pPlayerAlly, AIConcept_t concept, AI_Response *response  );

	void SetCategoryDelay( ConceptCategory_t category, float minDelay, float maxDelay = 0.0 );
	bool CategoryDelayExpired( ConceptCategory_t category );
	bool ConceptDelayExpired( AIConcept_t concept );

private:

	CSimpleSimTimer	m_ConceptCategoryTimers[SPEECH_NUM_CATEGORIES];

	CUtlMap<string_t, CSimpleSimTimer, char> m_ConceptTimers;

	friend CAI_AllySpeechManager *GetAllySpeechManager();
	static CAI_AllySpeechManager *gm_pSpeechManager;

	DECLARE_DATADESC();
};

//-------------------------------------

CAI_AllySpeechManager *GetAllySpeechManager();

//-----------------------------------------------------------------------------
//
// CLASS: CAI_PlayerAlly
//
//-----------------------------------------------------------------------------

class CAI_AllySpeechManager;

enum AISpeechTargetSearchFlags_t
{
	AIST_PLAYERS 				= (1<<0),
	AIST_NPCS					= (1<<1),
	AIST_IGNORE_RELATIONSHIP	= (1<<2),
	AIST_ANY_QUALIFIED			= (1<<3),
	AIST_FACING_TARGET			= (1<<4),
#ifdef MAPBASE
	// I needed this for something
	AIST_NOT_GAGGED				= (1<<5),
#endif
};

struct AISpeechSelection_t
{
#ifdef NEW_RESPONSE_SYSTEM
	std::string		concept;
	AI_Response		Response;
	EHANDLE			hSpeechTarget;
#else
	AISpeechSelection_t()
	 :	pResponse(NULL)
	{
	}
	
	void Set( AIConcept_t newConcept, AI_Response *pNewResponse, CBaseEntity *pTarget = NULL )
	{
		pResponse = pNewResponse;
		concept = newConcept;
		hSpeechTarget = pTarget;
	}
	
	std::string 		concept;
	AI_Response *		pResponse;
	EHANDLE			hSpeechTarget;				
#endif
};

//-------------------------------------

class CAI_PlayerAlly : public CAI_BaseActor
{
	DECLARE_CLASS( CAI_PlayerAlly, CAI_BaseActor );

public:
	//---------------------------------

	int			ObjectCaps( void ) { return UsableNPCObjectCaps(BaseClass::ObjectCaps()); }
	void		TalkInit( void );				

	//---------------------------------
	// Behavior
	//---------------------------------
	void		GatherConditions( void );
	void		GatherEnemyConditions( CBaseEntity *pEnemy );
	void		OnStateChange( NPC_STATE OldState, NPC_STATE NewState );
	void		PrescheduleThink( void );
	int			SelectSchedule( void );
	int			SelectNonCombatSpeech( AISpeechSelection_t *pSelection );
	virtual int	SelectNonCombatSpeechSchedule();
	int			TranslateSchedule( int scheduleType );
	void		OnStartSchedule( int scheduleType );
	void		StartTask( const Task_t *pTask );
	void		RunTask( const Task_t *pTask );
	void		TaskFail( AI_TaskFailureCode_t );
	void		TaskFail( const char *pszGeneralFailText )	{ BaseClass::TaskFail( pszGeneralFailText ); }
	void		ClearTransientConditions();
	void		Touch(	CBaseEntity *pOther );

#ifdef MAPBASE
	virtual bool		CanFlinch( void );
#endif

	//---------------------------------
	// Combat
	//---------------------------------
	void		OnKilledNPC( CBaseCombatCharacter *pKilled );

#ifdef MAPBASE
	void		OnEnemyRangeAttackedMe( CBaseEntity *pEnemy, const Vector &vecDir, const Vector &vecEnd );
#endif

	//---------------------------------
	// Damage handling
	//---------------------------------
	void		TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	int			TakeHealth( float flHealth, int bitsDamageType );
	void		Event_Killed( const CTakeDamageInfo &info );
	bool		CreateVPhysics();

	//---------------------------------

	virtual void PainSound( const CTakeDamageInfo &info );

	//---------------------------------
	// Speech & Acting
	//---------------------------------
	CBaseEntity	*EyeLookTarget( void );		// Override to look at talk target
	CBaseEntity	*FindNamedEntity( const char *pszName, IEntityFindFilter *pFilter = NULL );

	CBaseEntity *FindSpeechTarget( int flags );
	virtual bool IsValidSpeechTarget( int flags, CBaseEntity *pEntity );
	
	CBaseEntity *GetSpeechTarget()								{ return m_hTalkTarget.Get(); }
	void		SetSpeechTarget( CBaseEntity *pSpeechTarget ) 	{ m_hTalkTarget = pSpeechTarget; }

#ifdef MAPBASE
	// Needed for additional speech target responses
	CBaseEntity *GetPotentialSpeechTarget()								{ return m_hPotentialSpeechTarget.Get(); }
	void		SetPotentialSpeechTarget( CBaseEntity *pSpeechTarget ) 	{ m_hPotentialSpeechTarget = pSpeechTarget; }
#endif
	
	void		SetSpeechFilter( CAI_SpeechFilter *pFilter )	{ m_hSpeechFilter = pFilter; }
	CAI_SpeechFilter *GetSpeechFilter( void )					{ return m_hSpeechFilter; }

	//---------------------------------
	
	virtual bool SelectIdleSpeech( AISpeechSelection_t *pSelection );
	virtual bool SelectAlertSpeech( AISpeechSelection_t *pSelection );

	virtual bool SelectInterjection();
	virtual bool SelectPlayerUseSpeech();

	//---------------------------------

	virtual bool SelectQuestionAndAnswerSpeech( AISpeechSelection_t *pSelection );
	virtual void PostSpeakDispatchResponse( AIConcept_t concept, AI_Response *response );
	bool		 SelectQuestionFriend( CBaseEntity *pFriend, AISpeechSelection_t *pSelection );
	bool		 SelectAnswerFriend( CBaseEntity *pFriend, AISpeechSelection_t *pSelection, bool bRespondingToHello );

	//---------------------------------

	bool 		SelectSpeechResponse( AIConcept_t concept, const char *pszModifiers, CBaseEntity *pTarget, AISpeechSelection_t *pSelection );
	void		SetPendingSpeech( AIConcept_t concept, AI_Response *pResponse );
	void 		ClearPendingSpeech();
	bool		HasPendingSpeech()	{ return !m_PendingConcept.empty(); }

	//---------------------------------
	
	bool		CanPlaySentence( bool fDisregardState );
	int			PlayScriptedSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener );

	//---------------------------------
	
	void		DeferAllIdleSpeech( float flDelay = -1, CAI_BaseNPC *pIgnore = NULL );

	//---------------------------------
	
	bool		IsOkToSpeak( ConceptCategory_t category, bool fRespondingToPlayer = false );
	
	//---------------------------------
	
	bool		IsOkToSpeak( void );
	bool		IsOkToCombatSpeak( void );
	bool		IsOkToSpeakInResponseToPlayer( void );
	
	bool		ShouldSpeakRandom( AIConcept_t concept, int iChance );
	bool		IsAllowedToSpeak( AIConcept_t concept, bool bRespondingToPlayer = false );
#ifdef MAPBASE
	bool		IsAllowedToSpeakFollowup( AIConcept_t concept, CBaseEntity *pIssuer, bool bSpecific );
#endif
	virtual bool SpeakIfAllowed( AIConcept_t concept, const char *modifiers = NULL, bool bRespondingToPlayer = false, char *pszOutResponseChosen = NULL, size_t bufsize = 0 );
#ifdef MAPBASE
	virtual bool SpeakIfAllowed( AIConcept_t concept, AI_CriteriaSet& modifiers, bool bRespondingToPlayer = false, char *pszOutResponseChosen = NULL, size_t bufsize = 0 );
#endif
	void		ModifyOrAppendCriteria( AI_CriteriaSet& set );

	//---------------------------------
	
	float		GetTimePlayerStaring()		{ return ( m_flTimePlayerStartStare != 0 ) ? gpGlobals->curtime - m_flTimePlayerStartStare : 0; }

	//---------------------------------
	// NPC Event Response System
	virtual bool CanRespondToEvent( const char *ResponseConcept );
	virtual bool RespondedTo( const char *ResponseConcept, bool bForce, bool bCancelScene );

	//---------------------------------

	void		OnSpokeConcept( AIConcept_t concept, AI_Response *response );
	void		OnStartSpeaking();

	// Inputs
	virtual void InputIdleRespond( inputdata_t &inputdata ) {};
	void InputSpeakResponseConcept( inputdata_t &inputdata );
	virtual bool SpeakMapmakerInterruptConcept( string_t iszConcept );

	void			DisplayDeathMessage( void );
	virtual const char		*GetDeathMessageText( void ) { return "GAMEOVER_ALLY"; }
	void			InputMakeGameEndAlly( inputdata_t &inputdata );
	void			InputMakeRegularAlly( inputdata_t &inputdata );
#ifdef MAPBASE
	bool			AskQuestionNow( CBaseEntity *pSpeechTarget = NULL, int iQARandomNumber = -1, const char *concept = TLK_QUESTION );
	void			InputAskQuestion( inputdata_t &inputdata );
#endif
	void			InputAnswerQuestion( inputdata_t &inputdata );
	void			InputAnswerQuestionHello( inputdata_t &inputdata );
	void			InputEnableSpeakWhileScripting( inputdata_t &inputdata );
	void			InputDisableSpeakWhileScripting( inputdata_t &inputdata );
	
	void			AnswerQuestion( CAI_PlayerAlly *pQuestioner, int iQARandomNum, bool bAnsweringHello );

protected:
	
#ifdef HL2_DLL
	// Health regeneration for friendly allies
	virtual bool ShouldRegenerateHealth( void ) { return ( Classify() == CLASS_PLAYER_ALLY_VITAL ); }
#endif

	inline bool CanSpeakWhileScripting();

	// Whether we are a vital ally (useful for wrting Classify() for classes that are only sometimes vital, 
	// such as the Lone Vort in Ep2.) The usual means by which any other function should determine if a character
	// is vital is to determine Classify() == CLASS_PLAYER_ALLY_VITAL. Do not use this function outside that
	// context. 
	inline bool IsGameEndAlly( void ) { return m_bGameEndAlly; }

	//-----------------------------------------------------
	// Conditions, Schedules, Tasks
	//-----------------------------------------------------
	enum
	{
		SCHED_TALKER_SPEAK_PENDING_IDLE = BaseClass::NEXT_SCHEDULE,
		SCHED_TALKER_SPEAK_PENDING_ALERT,
		SCHED_TALKER_SPEAK_PENDING_COMBAT,
		NEXT_SCHEDULE,
		
		TASK_TALKER_SPEAK_PENDING = BaseClass::NEXT_TASK,
		NEXT_TASK,
		
		COND_TALKER_CLIENTUNSEEN = BaseClass::NEXT_CONDITION,
		COND_TALKER_PLAYER_DEAD,
		COND_TALKER_PLAYER_STARING,
		NEXT_CONDITION
	};

private:
	void SetCategoryDelay( ConceptCategory_t category, float minDelay, float maxDelay = 0.0 )	{ m_ConceptCategoryTimers[category].Set( minDelay, maxDelay ); }
	bool CategoryDelayExpired( ConceptCategory_t category )										{ return m_ConceptCategoryTimers[category].Expired(); }

	friend class CAI_AllySpeechManager;

	//---------------------------------
	
	AI_Response		m_PendingResponse;
	std::string		m_PendingConcept;
	float			m_TimePendingSet;

	//---------------------------------
	
	EHANDLE			m_hTalkTarget;	// who to look at while talking
	float			m_flNextRegenTime;
	float			m_flTimePlayerStartStare;
	EHANDLE			m_hPotentialSpeechTarget;	// NPC to tell the response rules about when trying to find a response to talk to them with
	float			m_flNextIdleSpeechTime;
	int				m_iQARandomNumber;

	//---------------------------------

	CSimpleSimTimer	m_ConceptCategoryTimers[3];
	
	//---------------------------------
	
	CHandle<CAI_SpeechFilter>	m_hSpeechFilter;

	bool m_bGameEndAlly;
	bool m_bCanSpeakWhileScripting;	// Allows mapmakers to override NPC_STATE_SCRIPT or IsScripting() for responses.

	float	m_flTimeLastRegen;		// Last time I regenerated a bit of health.
	float	m_flHealthAccumulator;	// Counterpart to the damage accumulator in CBaseCombatCharacter. So ally health regeneration is accurate over time.

#ifdef _XBOX
protected:
#endif
	DECLARE_DATADESC();
protected:
	DEFINE_CUSTOM_AI;
};


bool CAI_PlayerAlly::CanSpeakWhileScripting()
{
	return m_bCanSpeakWhileScripting;
}

//-----------------------------------------------------------------------------

#endif // AI_PLAYERALLY_H
