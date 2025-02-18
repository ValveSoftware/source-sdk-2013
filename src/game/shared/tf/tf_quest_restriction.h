//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_QUEST_RESTRICTION_H
#define TF_QUEST_RESTRICTION_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_proto_def_messages.h"

#ifdef CLIENT_DLL
#define CTFPlayer C_TFPlayer
#endif // CLIENT_DLL

class CTFQuestEvaluator;
class CTFPlayer;

enum EInvalidReasons_t
{
	INVALID_QUEST_REASON_WRONG_MAP = 0,
	INVALID_QUEST_REASON_WRONG_CLASS,
	INVALID_QUEST_REASON_WRONG_GAME_MODE,
	INVALID_QUEST_REASON_NOT_ENOUGH_PLAYERS,
	INVALID_QUEST_REASON_VALVE_SERVERS_ONLY,
	INVALID_QUEST_REASON_MATCH_TYPE,

	NUM_INVALID_REASONS,
};

struct InvalidReason
{
	InvalidReason() {}

	bool IsValid() const 
	{ 
		return m_bits.IsAllClear();	
	}

	CBitVec< NUM_INVALID_REASONS > m_bits;
};

typedef InvalidReason InvalidReasonsContainer_t;

void GetInvalidReasonsNames( const InvalidReasonsContainer_t&, CUtlVector< CUtlString >& vecStrings );

//-----------------------------------------------------------------------------
// Purpose: base quest condition
//-----------------------------------------------------------------------------
class CTFQuestCondition
{
public:
	DECLARE_CLASS_NOBASE( CTFQuestCondition )

	CTFQuestCondition();
	virtual ~CTFQuestCondition();

	virtual const char *GetBaseName() const = 0;
	virtual const char *GetConditionName() const { return m_pszTypeName; }
	virtual const char *GetValueString() const { return ""; }
	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ );

	virtual void PrintDebugText() const;

	virtual const CTFPlayer *GetQuestOwner() const;
	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const;

	virtual bool IsOperator() const { return false; }
	virtual bool IsEvaluator() const { return false; }

	void SetParent( CTFQuestCondition *pParent ) { m_pParent = pParent; }
	CTFQuestCondition *GetParent() const { return m_pParent; }

	virtual CTFQuestCondition* AddChildByName( const char *pszChildName ) { Assert( 0 );  return NULL; }

	virtual int GetChildren( CUtlVector< CTFQuestCondition* >& vecChildren ) { return 0; }
	virtual bool RemoveAndDeleteChild( CTFQuestCondition *pChild ) { return false; }

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) {}
	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) {}

	virtual void GetValidTypes( CUtlVector< const char* >& vecOutValidChildren ) const = 0;
	virtual void GetValidChildren( CUtlVector< const char* >& vecOutValidChildren ) const = 0;

	virtual int GetMaxInputCount() const { return 0; }

	virtual const char* GetEventName() const { Assert( 0 ); return NULL; }
	virtual void SetEventName( const char *pszEventName ) { Assert( 0 ); }

	void SetFieldName( const char* pszFieldName ) { m_pszFieldName = pszFieldName; }
	void SetTypeName( const char* pszTypeName ) { m_pszTypeName = pszTypeName; }

protected:

	void GetValidRestrictions( CUtlVector< const char* >& vecOutValidChildren ) const;
	void GetValidEvaluators( CUtlVector< const char* >& vecOutValidChildren ) const;

	const char *m_pszFieldName;
	const char *m_pszTypeName;

private:

	CTFQuestCondition *m_pParent;
};


//-----------------------------------------------------------------------------
// Purpose: base quest restriction
//-----------------------------------------------------------------------------
class CTFQuestRestriction : public CTFQuestCondition
{
public:
	DECLARE_CLASS( CTFQuestRestriction, CTFQuestCondition )

	virtual const char *GetBaseName() const OVERRIDE { return "restriction"; }
	virtual bool PassesRestrictions( IGameEvent *pEvent ) const = 0;
	void SetEventName( const char *pszEventName ) OVERRIDE { m_pszEventName = pszEventName; }
	virtual const char* GetEventName() const OVERRIDE { return m_pszEventName; }

	virtual void GetValidTypes( CUtlVector< const char* >& vecOutValidChildren ) const OVERRIDE;
	virtual void GetValidChildren( CUtlVector< const char* >& vecOutValidChildren ) const OVERRIDE;

protected:

	const char *m_pszEventName;
};

// 
// Let's stop defining every objective for every class/map/mode/team combination.
// Evaluators are for detecting a specific event happening.  These modifiers
// are for determining the OWNING player's state.
//
class ITFQuestModifier
{
public:
	virtual ~ITFQuestModifier() {}
	virtual bool BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const = 0;
};

// Have to be this class
class CTFClassQuestModifier : public ITFQuestModifier
{
public:
	CTFClassQuestModifier( uint32 nValidClassesMask ) 
		: m_nValidClassesMask( nValidClassesMask )
	{}

	virtual bool BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const OVERRIDE;
private:

	uint32 m_nValidClassesMask;
};

// Have to be on this map
class CTFMapQuestModifier : public ITFQuestModifier
{
public:
	CTFMapQuestModifier()
	{}

	void AddMapName( const char* pszMapName );
	virtual bool BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const OVERRIDE;
private:

	CUtlVector< CUtlString > m_vecStrMapNames;
};

// Have to be on this game mode
class CTFGameModeQuestModifier : public ITFQuestModifier
{
public:
	CTFGameModeQuestModifier( uint32 nValidGameModeMask )
		: m_nValidGameModesMask( nValidGameModeMask )
	{}

	virtual bool BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const OVERRIDE;

private:

	uint32 m_nValidGameModesMask;
};

// Have to be on this team
class CTFTeamQuestModifier : public ITFQuestModifier
{
public:
	CTFTeamQuestModifier( uint32 nTeamNumber )
		: m_nTeamNum( nTeamNumber )
	{}

	bool BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const OVERRIDE;

private:
	int m_nTeamNum;
};

// Have to have this condition at the time
class CTFConditionQuestModifier : public ITFQuestModifier
{
public:
	CTFConditionQuestModifier( const LogicalOperation& operation )
		: m_Operation( operation )
	{}

	bool BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const OVERRIDE;
	void AddCondition( ETFCond eCond ) { m_vecRequiredConditions.AddToTail( eCond ); }

private:
	
	CUtlVector< ETFCond > m_vecRequiredConditions;
	LogicalOperation m_Operation;
};

// Have to have these items equipped at the time
class CTFEquippedItemsQuestModifier : public ITFQuestModifier
{
public:
	CTFEquippedItemsQuestModifier( const LogicalOperation& operation )
		: m_Operation( operation )
	{}

	bool BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const OVERRIDE;
	void AddItem( const char* pszName ) { m_vecRequiredItemDefs.AddToTail( CSchemaItemDefHandle( pszName ) ); }

private:

	CUtlVector< CSchemaItemDefHandle > m_vecRequiredItemDefs;
	LogicalOperation m_Operation;
};

// Have to be on this team
class CTFJumpStateQuestModifier : public ITFQuestModifier
{
public:
	CTFJumpStateQuestModifier( uint32 nJumpCount )
		: m_nJumpCount( nJumpCount )
	{}

	bool BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const OVERRIDE;

private:
	int m_nJumpCount; // 0: On the ground, 1: Jumped, 2: Double-Jumped, 3: Double Jumped
};


//-----------------------------------------------------------------------------
// Purpose: base quest evaluator
//-----------------------------------------------------------------------------
class CTFQuestEvaluator : public CTFQuestCondition
{
public:
	CTFQuestEvaluator();
	~CTFQuestEvaluator();

	virtual const char *GetBaseName() const OVERRIDE { return "evaluator"; }

	virtual bool IsEvaluator() const OVERRIDE { return true; }

	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const OVERRIDE;

	virtual void EvaluateCondition( CTFQuestEvaluator *pSender, int nScore ) = 0;
	virtual void ResetCondition() = 0;

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE;
	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE;

	virtual void GetValidTypes( CUtlVector< const char* >& vecOutValidChildren ) const OVERRIDE;
	virtual void GetValidChildren( CUtlVector< const char* >& vecOutValidChildren ) const OVERRIDE;

	void SetAction( const char *pszAction ) { m_pszAction = pszAction; }
	const char *GetAction() const { return m_pszAction; }

	void AddModifiers( ITFQuestModifier* pModifier ); // This takes ownership

private:
	const char *m_pszAction;
	CUtlVector< ITFQuestModifier* > m_vecModifiers;
};

CTFQuestRestriction *CreateRestrictionByName( const char *pszName, CTFQuestCondition* pParent );
CTFQuestEvaluator *CreateEvaluatorByName( const char *pszName, CTFQuestCondition*);

#endif // TF_QUEST_RESTRICTION_H
