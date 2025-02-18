//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "filters.h"
#include "team_control_point.h"
#include "tf_gamerules.h"
#include "econ_wearable.h"
#include "bot/tf_bot.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// Team Fortress Team Filter
//
class CFilterTFTeam : public CBaseFilter
{
	DECLARE_CLASS( CFilterTFTeam, CBaseFilter );

public:

	void InputRoundSpawn( inputdata_t &inputdata );
	void InputRoundActivate( inputdata_t &inputdata );

	inline bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity );

private:

	string_t m_iszControlPointName;

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CFilterTFTeam )

DEFINE_KEYFIELD( m_iszControlPointName, FIELD_STRING, "controlpoint" ),

// Inputs.
DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),
DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( filter_activator_tfteam, CFilterTFTeam );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFilterTFTeam::PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
{
	// is the entity we're asking about on the winning 
	// team during the bonus time? (winners pass all filters)
	if (  TFGameRules() &&
		( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN ) && 
		( TFGameRules()->GetWinningTeam() == pEntity->GetTeamNumber() ) )
	{
		// this should open all doors for the winners
		if ( m_bNegated )
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	return ( pEntity->GetTeamNumber() == GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFilterTFTeam::InputRoundSpawn( inputdata_t &input )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFilterTFTeam::InputRoundActivate( inputdata_t &input )
{
	if ( m_iszControlPointName != NULL_STRING )
	{
		CTeamControlPoint *pControlPoint = dynamic_cast<CTeamControlPoint*>( gEntList.FindEntityByName( NULL, m_iszControlPointName ) );
		if ( pControlPoint )
		{
			ChangeTeam( pControlPoint->GetTeamNumber() );
		}
		else
		{
			Warning( "%s failed to find control point named '%s'\n", GetClassname(), STRING(m_iszControlPointName) );
		}
	}
}


/// filter the excludes people that can't cap
// has to be chained with a team filter

class CFilterTFCanCap : public CBaseFilter
{
	DECLARE_CLASS( CFilterTFCanCap, CBaseFilter );

public:

	inline bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );

		if ( !pPlayer )
			return false;

		// No disguised spies
		if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) || pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
		{
			return false;
		}

		// No stealthed spies
		if ( pPlayer->m_Shared.IsStealthed() )
		{
			return false;
		}

		// No invuln players
		if ( pPlayer->m_Shared.IsInvulnerable() || pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
		{
			return false;
		}

		return ( GetTeamNumber() == TEAM_UNASSIGNED || ( pPlayer->GetTeamNumber() == GetTeamNumber() ) );
	}

private:

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CFilterTFCanCap )
END_DATADESC()

LINK_ENTITY_TO_CLASS( filter_tf_player_can_cap, CFilterTFCanCap );


// ###################################################################
//	> FilterDamagedByWeaponInSlot
// ###################################################################
class FilterDamagedByWeaponInSlot : public CBaseFilter
{
	DECLARE_CLASS( FilterDamagedByWeaponInSlot, CBaseFilter );
	DECLARE_DATADESC();

protected:

	bool PassesFilterImpl(CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		ASSERT( false );
		return true;
	}

	bool PassesDamageFilterImpl(const CTakeDamageInfo &info)
	{
		if ( info.GetWeapon() )
		{
			CTFWeaponBase* pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
			if ( pWeapon )
			{
				return pWeapon->GetSlot() == m_iWeaponSlot;
			}
		}
		return false;
	}

	int m_iWeaponSlot;
};

LINK_ENTITY_TO_CLASS( filter_tf_damaged_by_weapon_in_slot, FilterDamagedByWeaponInSlot );

BEGIN_DATADESC( FilterDamagedByWeaponInSlot )

// Keyfields
DEFINE_KEYFIELD( m_iWeaponSlot,	FIELD_INTEGER,	"weaponSlot" ),

END_DATADESC()



// Only include bots with specific tags
class CFilterTFBotHasTag : public CBaseFilter
{
	DECLARE_CLASS( CFilterTFBotHasTag, CBaseFilter );
	DECLARE_DATADESC();
public:

	inline virtual void Spawn()
	{
		BaseClass::Spawn();

		const char* tags = STRING( m_iszTags );
		CSplitString splitTags( tags, " " );
		for ( int i=0; i<splitTags.Count(); ++i )
		{
			m_tags.CopyAndAddToTail( splitTags[i] );
		}
	}

	inline bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		CTFBot *pBot = ToTFBot( pEntity );

		if ( !pBot )
			return false;

		bool bPasses = false;
		for ( int i=0; i<m_tags.Count(); ++i )
		{
			const char* pszTag = m_tags[i];
			if ( pBot->HasTag( pszTag ) )
			{
				bPasses = true;
				if ( !m_bRequireAllTags )
				{
					break;
				}
			}
			else if ( m_bRequireAllTags )
			{
				return false;
			}
		}

		return bPasses;
	}

private:

	CUtlStringList m_tags;
	string_t m_iszTags;
	bool m_bRequireAllTags;
};

BEGIN_DATADESC( CFilterTFBotHasTag )

	DEFINE_KEYFIELD( m_iszTags, FIELD_STRING, "tags" ),
	DEFINE_KEYFIELD( m_bRequireAllTags, FIELD_BOOLEAN, "require_all_tags" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( filter_tf_bot_has_tag, CFilterTFBotHasTag );


//=============================================================================
//
// Team Fortress Condition Filter
//
class CFilterTFCondition : public CBaseFilter
{
	DECLARE_CLASS( CFilterTFCondition, CBaseFilter );

public:

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		if ( !pEntity->IsPlayer() )
			return false;

		CTFPlayer *pTFPlayer = ToTFPlayer( pEntity );
		return pTFPlayer && pTFPlayer->m_Shared.InCond( (ETFCond)m_nCondition );
	}

private:

	int m_nCondition;

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CFilterTFCondition )
	DEFINE_KEYFIELD( m_nCondition, FIELD_INTEGER, "condition" ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( filter_tf_condition, CFilterTFCondition );

//=============================================================================
//
// Team Fortress Class Filter
//
class CFilterTFClass : public CBaseFilter
{
	DECLARE_CLASS( CFilterTFClass, CBaseFilter );

public:

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		if ( !pEntity->IsPlayer() )
			return false;

		CTFPlayer *pTFPlayer = ToTFPlayer( pEntity );
		return pTFPlayer 
			&& !pTFPlayer->IsObserver()
			&& pTFPlayer->IsPlayerClass(m_nClass);
	}

private:

	int m_nClass;

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CFilterTFClass )
	DEFINE_KEYFIELD( m_nClass, FIELD_INTEGER, "tfclass" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( filter_tf_class, CFilterTFClass );
