//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_bot_hint_entity.h"
#include "tf_obj.h"
#include "tf_player.h"


BEGIN_DATADESC( CBaseTFBotHintEntity )
	DEFINE_KEYFIELD( m_isDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC()

IMPLEMENT_AUTO_LIST( ITFBotHintEntityAutoList );

//------------------------------------------------------------------------------
CBaseTFBotHintEntity::CBaseTFBotHintEntity( void )
	: m_isDisabled( false ),
	m_hintType( HINT_INVALID )
{
}


bool CBaseTFBotHintEntity::OwnerObjectHasNoOwner() const
{
	CBaseEntity* pOwner = GetOwnerEntity();
	if ( pOwner && pOwner->IsBaseObject() )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( pOwner );
		if ( pObj->GetBuilder() == NULL )
		{
			return true;
		}
		else
		{
			if ( !pObj->GetBuilder()->IsPlayerClass( TF_CLASS_ENGINEER ) )
			{
				AssertMsg( 0, "Object has an owner that's not engineer." );
				Warning( "Object has an owner that's not engineer." );
			}
		}
	}
	return false;
}


bool CBaseTFBotHintEntity::OwnerObjectFinishBuilding() const
{
	CBaseEntity* pOwner = GetOwnerEntity();
	if ( pOwner && pOwner->IsBaseObject() )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( pOwner );
		return !pObj->IsBuilding();
	}
	return false;
}
