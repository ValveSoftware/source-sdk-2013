//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Condition Objects
//
//=============================================================================

#include "cbase.h"
#include "tf_condition.h"

#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "achievements_tf.h"
#endif

#ifdef CLIENT_DLL

BEGIN_PREDICTION_DATA_NO_BASE( CTFConditionList )
DEFINE_PRED_FIELD( _condition_bits, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

BEGIN_RECV_TABLE_NOBASE( CTFConditionList, DT_TFPlayerConditionListExclusive )
RecvPropInt( RECVINFO( _condition_bits ) ),
END_RECV_TABLE()

#else

BEGIN_SEND_TABLE_NOBASE( CTFConditionList, DT_TFPlayerConditionListExclusive )
SendPropInt( SENDINFO( _condition_bits ), MIN( TF_COND_LAST, 32 ), SPROP_UNSIGNED ),
END_SEND_TABLE()

#endif

//-----------------------------------------------------------------------------
// Ctor
//-----------------------------------------------------------------------------
CTFConditionList::CTFConditionList()
{
	_condition_bits = _old_condition_bits = 0;
}

//-----------------------------------------------------------------------------
// Condition factory.
//-----------------------------------------------------------------------------
bool CTFConditionList::Add( ETFCond type, float duration, CTFPlayer* outer, CBaseEntity* provider /*= NULL*/ )
{
#ifdef GAME_DLL
	return _Add( type, duration, outer, provider );
#else
	return type == TF_COND_CRITBOOSTED;
#endif
}

bool CTFConditionList::_Add( ETFCond type, float duration, CTFPlayer* outer, CBaseEntity* provider /*= NULL*/ )
{
	// If we already have a condition of this type, ask it to handle the addition of another.
	for ( int i = 0; i < _conditions.Count(); ++i )
	{
		if ( _conditions[i]->GetType() == type )
		{
			_conditions[i]->Add( duration );
			_conditions[i]->SetProvider( provider );
			return true;
		}
	}

	// Add a new condition.
	CTFCondition* newCond = NULL;
	switch ( type )
	{
		// TODO: Register new conditions anonymously instead of switching.
	case TF_COND_CRITBOOSTED:
		newCond = new CTFCondition_CritBoost( type, duration, outer, provider );
		break;
	}

	if ( newCond )
	{
		_condition_bits |= (1<<type);
		_old_condition_bits |= (1<<type);

		_conditions.AddToTail( newCond );
		newCond->OnAdded();
		newCond->SetProvider( provider );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Remove a condition from the player.
//-----------------------------------------------------------------------------
bool CTFConditionList::Remove( ETFCond type, bool ignore_duration )
{
#ifdef GAME_DLL
	bool bConditionListHandledRemoval = _Remove( type, ignore_duration );

	// The condition list only handles one type of condition. Written slightly weird
	// to avoid unused variable warnings with asserts disabled.
	if ( bConditionListHandledRemoval )
	{
		Assert( type == TF_COND_CRITBOOSTED );
	}
#endif
	return type == TF_COND_CRITBOOSTED;
}

bool CTFConditionList::_Remove( ETFCond type, bool ignore_duration )
{
	for ( int i=_conditions.Count()-1; i>=0; --i )
	{
		CTFCondition* cond = _conditions[i];
		if ( !cond || cond->GetType() != type )
			continue;

		if ( cond->UsesMinDuration() && !ignore_duration && cond->GetMinDuration() > 0 )
		{
			cond->SetMaxDuration( cond->GetMinDuration() );
			continue; // Can't remove conditions that haven't expired.
		}

		_conditions.Remove( i );

		_condition_bits &= ~(1<<type);
		_old_condition_bits &= ~(1<<type);

		cond->OnRemoved();
		delete cond;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Clear all conditions from the player.
//-----------------------------------------------------------------------------
void CTFConditionList::RemoveAll()
{
	_condition_bits = 0;
	_old_condition_bits = 0;

	for ( int i=0; i<_conditions.Count(); ++i )
	{
		_conditions[i]->OnRemoved();
	}
	_conditions.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Checks if we have at least one of a given condition applied.
//-----------------------------------------------------------------------------
bool CTFConditionList::InCond( ETFCond type ) const
{
	return ( ( _condition_bits & (1<<type) ) != 0 );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CBaseEntity *CTFConditionList::GetProvider( ETFCond type ) const
{
	CBaseEntity *pProvider = NULL;
	for ( int i = 0; i < _conditions.Count(); ++i )
	{
		if ( _conditions[i]->GetType() == type )
		{
			pProvider = _conditions[i]->GetProvider();
			break;
		}
	}

	return pProvider;
}

//-----------------------------------------------------------------------------
// Client/Server periodic condition think.
//-----------------------------------------------------------------------------
void CTFConditionList::Think()
{
	for ( int i=0; i<_conditions.Count(); ++i )
	{
		_conditions[i]->OnThink();
	}
}

//-----------------------------------------------------------------------------
// Server only per-frame think.
//-----------------------------------------------------------------------------
void CTFConditionList::ServerThink()
{
#ifdef GAME_DLL
	for ( int i=0; i<_conditions.Count(); ++i )
	{
		CTFCondition* cond = _conditions[i];
		if ( cond->GetMaxDuration() > PERMANENT_CONDITION ||
			 cond->GetMinDuration() > PERMANENT_CONDITION )
		{
			// Reduce the duration over time.
			float reduction = gpGlobals->frametime;

			// Healable conditions expire faster when we have healers.
			int numHealers = cond->GetOuter()->m_Shared.GetNumHealers();
			if ( cond->IsHealable() && numHealers > 0 )
			{
				reduction += numHealers * reduction * 4;
			}

			// Decrement min duration.
			if ( cond->GetMinDuration() > PERMANENT_CONDITION )
			{
				cond->SetMinDuration( MAX( cond->GetMinDuration() - reduction, 0 ) );
			}

			// Decrement max duration.
			if ( cond->GetMaxDuration() > PERMANENT_CONDITION )
			{
				cond->SetMaxDuration( MAX( cond->GetMaxDuration() - reduction, 0 ) );

				if ( cond->GetMaxDuration() < cond->GetMinDuration() )
				{
					cond->SetMaxDuration( cond->GetMinDuration() );
				}
			}

			if ( cond->GetMaxDuration() == 0 )
			{
				Remove( cond->GetType() );
				continue;
			}
		}

		_conditions[i]->OnServerThink();
	}
#endif
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CTFConditionList::OnPreDataChanged( void )
{
//	_old_condition_bits = _condition_bits;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CTFConditionList::OnDataChanged( CTFPlayer* outer )
{
	// Is there a way to improve this by hooking directly into network state changed?
	if ( _old_condition_bits != _condition_bits )
	{
		UpdateClientConditions( outer );

		_old_condition_bits = _condition_bits;
	}
}

//-----------------------------------------------------------------------------
// Creates or destroys conditions to make sure our state matches the server.
//-----------------------------------------------------------------------------
void CTFConditionList::UpdateClientConditions( CTFPlayer* outer )
{
	int nCondChanged = _condition_bits ^ _old_condition_bits;
	int nCondAdded = nCondChanged & _condition_bits;
	int nCondRemoved = nCondChanged & _old_condition_bits;

	int i;
	for ( i=0;i<TF_COND_LAST;i++ )
	{
		if ( nCondAdded & (1<<i) )
		{
			_Add( (ETFCond)i, PERMANENT_CONDITION, outer );
		}
		else if ( nCondRemoved & (1<<i) )
		{
			_Remove( (ETFCond)i );
		}
	}
}

#endif

//-----------------------------------------------------------------------------
// Ctor
//-----------------------------------------------------------------------------
CTFCondition::CTFCondition( ETFCond type, float duration, CTFPlayer* outer, CBaseEntity* provider /*= NULL*/ )
: _type( type ),
  _min_duration( 0 ),
  _max_duration( duration ),
  _outer( outer ),
  _provider( provider )
{
}

//-----------------------------------------------------------------------------
// Dtor
//-----------------------------------------------------------------------------
CTFCondition::~CTFCondition()
{
}

//-----------------------------------------------------------------------------
// Called if we try to add another condition of a type we already have.
//-----------------------------------------------------------------------------
void CTFCondition::Add( float duration )
{
	if ( duration != PERMANENT_CONDITION )
	{
		// If our new duration is not permanent, is shorter than
		// our current duration, and is longer than our min duration
		// make it our new min duration.
		if ( GetMaxDuration() == PERMANENT_CONDITION ||
			 duration < GetMaxDuration() )
		{
			if ( duration > GetMinDuration() )
				SetMinDuration( duration );

			return;
		}
	}
	else if ( GetMaxDuration() != PERMANENT_CONDITION )
	{
		// If our current duration is not permanent and we are adding a
		// permanent duration, make our old finite duration the new min duration.
		// This ensures we last at least that long.
		SetMinDuration( GetMaxDuration() );
	}

	SetMaxDuration( duration );
}

//=============================================================================
// Crit Boost
//=============================================================================
CTFCondition_CritBoost::CTFCondition_CritBoost( ETFCond type, float duration, CTFPlayer* outer, CBaseEntity* provider /*= NULL*/ )
: CTFCondition( type, duration, outer, provider )
{
	Assert( type == TF_COND_CRITBOOSTED );
}

void CTFCondition_CritBoost::OnAdded()
{
#ifdef CLIENT_DLL
	GetOuter()->m_Shared.UpdateCritBoostEffect();

	if ( GetOuter()->IsLocalPlayer() && GetOuter()->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
	{
		g_AchievementMgrTF.OnAchievementEvent( ACHIEVEMENT_TF_HEAVY_RECEIVE_UBER_GRIND );
	}
#endif
}

void CTFCondition_CritBoost::OnRemoved()
{
#ifdef CLIENT_DLL
	GetOuter()->m_Shared.UpdateCritBoostEffect();
#endif
}

void CTFCondition_CritBoost::OnThink()
{
#ifdef CLIENT_DLL
	if ( GetOuter()->m_pCritBoostEffect )
	{
		CBaseEntity *pWeapon = NULL;
		// Use GetRenderedWeaponModel() instead?
		if ( GetOuter()->IsLocalPlayer() )
		{
			pWeapon = GetOuter()->GetViewModel(0);
		}
		else
		{
			pWeapon = GetOuter()->GetActiveWeapon();
		}

		// Transfer the crit boosted effect if we've switched weapons
		if ( GetOuter()->m_pCritBoostEffect->GetOwner() != pWeapon )
		{
			GetOuter()->m_Shared.UpdateCritBoostEffect();
		}
	}
#endif
}

void CTFCondition_CritBoost::OnServerThink()
{
}
