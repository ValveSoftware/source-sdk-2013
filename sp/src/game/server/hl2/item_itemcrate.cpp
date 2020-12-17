//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The various ammo types for HL2	
//
//=============================================================================//

#include "cbase.h"
#include "props.h"
#include "items.h"
#include "item_dynamic_resupply.h"
#ifdef MAPBASE
#include "point_template.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char *pszItemCrateModelName[] =
{
	"models/items/item_item_crate.mdl",
	"models/items/item_beacon_crate.mdl",
#ifdef MAPBASE
	"models/items/item_item_crate.mdl", // Custom model placeholder/fallback, this should never be selected
#endif
};

//-----------------------------------------------------------------------------
// A breakable crate that drops items
//-----------------------------------------------------------------------------
class CItem_ItemCrate : public CPhysicsProp
{
public:
	DECLARE_CLASS( CItem_ItemCrate, CPhysicsProp );
	DECLARE_DATADESC();

	void Precache( void );
	void Spawn( void );

	virtual int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_WCEDIT_POSITION; };

	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

	void InputKill( inputdata_t &data );

	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );

#ifdef MAPBASE
	void InputSetContents( inputdata_t &data );
	void InputSetItemCount( inputdata_t &data );
	void InputMergeContentsWithPlayer( inputdata_t &data );

	// Item crates always override prop data for custom models
	bool OverridePropdata( void ) { return true; }
#endif

protected:
	virtual void OnBreak( const Vector &vecVelocity, const AngularImpulse &angVel, CBaseEntity *pBreaker );

#ifdef MAPBASE
	bool ShouldRandomizeAngles( CBaseEntity *pEnt );
	#define ITEM_ITEMCRATE_TEMPLATE_TARGET m_strAlternateMaster
	CPointTemplate *FindTemplate();
#endif

private:
	// Crate types. Add more!
	enum CrateType_t
	{
		CRATE_SPECIFIC_ITEM = 0,
#ifdef MAPBASE
		CRATE_POINT_TEMPLATE,
#endif
		CRATE_TYPE_COUNT,
	};

	enum CrateAppearance_t
	{
		CRATE_APPEARANCE_DEFAULT = 0,
		CRATE_APPEARANCE_RADAR_BEACON,
#ifdef MAPBASE
		CRATE_APPEARANCE_CUSTOM,
#endif
	};

private:
	CrateType_t			m_CrateType;
	string_t			m_strItemClass;
	int					m_nItemCount;
	string_t			m_strAlternateMaster;
	CrateAppearance_t	m_CrateAppearance;

	COutputEvent m_OnCacheInteraction;
#ifdef MAPBASE
	COutputEHANDLE m_OnItem;
#endif
};


LINK_ENTITY_TO_CLASS(item_item_crate, CItem_ItemCrate);


//-----------------------------------------------------------------------------
// Save/load: 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CItem_ItemCrate )

	DEFINE_KEYFIELD( m_CrateType, FIELD_INTEGER, "CrateType" ),	
	DEFINE_KEYFIELD( m_strItemClass, FIELD_STRING, "ItemClass" ),	
	DEFINE_KEYFIELD( m_nItemCount, FIELD_INTEGER, "ItemCount" ),	
	DEFINE_KEYFIELD( m_strAlternateMaster, FIELD_STRING, "SpecificResupply" ),	
	DEFINE_KEYFIELD( m_CrateAppearance, FIELD_INTEGER, "CrateAppearance" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Kill", InputKill ),
	DEFINE_OUTPUT( m_OnCacheInteraction, "OnCacheInteraction" ),
#ifdef MAPBASE
	DEFINE_OUTPUT( m_OnItem, "OnItem" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetContents", InputSetContents ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetItemCount", InputSetItemCount ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "MergeContentsWithPlayer", InputMergeContentsWithPlayer ),
#endif

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_ItemCrate::Precache( void )
{
	// Set this here to quiet base prop warnings
#ifdef MAPBASE
	// Set our model name here instead of in Spawn() so we could use custom crates.
	if (m_CrateAppearance != CRATE_APPEARANCE_CUSTOM)
		SetModelName(AllocPooledString(pszItemCrateModelName[m_CrateAppearance]));

	PrecacheModel( STRING(GetModelName()) );
	SetModel( STRING(GetModelName()) );
#else
	PrecacheModel( pszItemCrateModelName[m_CrateAppearance] );
	SetModel( pszItemCrateModelName[m_CrateAppearance] );
#endif

	BaseClass::Precache();
	if ( m_CrateType == CRATE_SPECIFIC_ITEM )
	{
		if ( NULL_STRING != m_strItemClass )
		{
			// Don't precache if this is a null string. 
			UTIL_PrecacheOther( STRING(m_strItemClass) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_ItemCrate::Spawn( void )
{ 
	if ( g_pGameRules->IsAllowedToSpawn( this ) == false )
	{
		UTIL_Remove( this );
		return;
	}

	DisableAutoFade();
#ifndef MAPBASE
	SetModelName( AllocPooledString( pszItemCrateModelName[m_CrateAppearance] ) );
#endif

	if ( NULL_STRING == m_strItemClass )
	{
		Warning( "CItem_ItemCrate(%i):  CRATE_SPECIFIC_ITEM with NULL ItemClass string (deleted)!!!\n", entindex() );
		UTIL_Remove( this );
		return;
	}

	Precache( );
#ifdef MAPBASE
	SetModel( STRING(GetModelName()) );
#else
	SetModel( pszItemCrateModelName[m_CrateAppearance] );
#endif
	AddEFlags( EFL_NO_ROTORWASH_PUSH );
	BaseClass::Spawn( );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CItem_ItemCrate::InputKill( inputdata_t &data )
{
#ifdef MAPBASE
	// Why is this its own function anyway?
	// It just overwrites the death notice stuff.
	m_OnKilled.FireOutput(data.pActivator, this);
#endif

	UTIL_Remove( this );
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CItem_ItemCrate::InputSetContents( inputdata_t &data )
{
	switch( m_CrateType )
	{
		case CRATE_POINT_TEMPLATE:
			ITEM_ITEMCRATE_TEMPLATE_TARGET = data.value.StringID();
			break;

		case CRATE_SPECIFIC_ITEM:
		default:
			m_strItemClass = data.value.StringID();
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CItem_ItemCrate::InputSetItemCount( inputdata_t &data )
{
	m_nItemCount = data.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CItem_ItemCrate::InputMergeContentsWithPlayer( inputdata_t &data )
{
	CBasePlayer *pPlayer = ToBasePlayer(data.value.Entity());
	if (!pPlayer)
		pPlayer = UTIL_GetLocalPlayer();

	if (pPlayer)
	{
		switch( m_CrateType )
		{
			case CRATE_POINT_TEMPLATE:
			{
				Warning( "%s: item_itemcrate MergeContentsWithPlayer is not supported on template crates yet!!!\n", GetDebugName() );
			} break;

			case CRATE_SPECIFIC_ITEM:
			default:
			{
				for (int i = 0; i < m_nItemCount; i++)
				{
					pPlayer->GiveNamedItem( STRING( m_strItemClass ) );
				}
			} break;
		}
	}
}
#endif


//-----------------------------------------------------------------------------
// Item crates blow up immediately
//-----------------------------------------------------------------------------
int CItem_ItemCrate::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( info.GetDamageType() & DMG_AIRBOAT )
	{
		CTakeDamageInfo dmgInfo = info;
		dmgInfo.ScaleDamage( 10.0 );
		return BaseClass::OnTakeDamage( dmgInfo );
	}

	return BaseClass::OnTakeDamage( info );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_ItemCrate::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	float flDamageScale = 1.0f;
	if ( FClassnameIs( pEvent->pEntities[!index], "prop_vehicle_airboat" ) ||
		 FClassnameIs( pEvent->pEntities[!index], "prop_vehicle_jeep" ) )
	{
		flDamageScale = 100.0f;
	}

	m_impactEnergyScale *= flDamageScale;
	BaseClass::VPhysicsCollision( index, pEvent );
	m_impactEnergyScale /= flDamageScale;
}


#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Finds the template for CRATE_POINT_TEMPLATE.
//-----------------------------------------------------------------------------
inline CPointTemplate *CItem_ItemCrate::FindTemplate()
{
	return dynamic_cast<CPointTemplate *>(gEntList.FindEntityByName( NULL, STRING(ITEM_ITEMCRATE_TEMPLATE_TARGET) ));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CItem_ItemCrate::ShouldRandomizeAngles( CBaseEntity *pEnt )
{
	// Angles probably not supposed to be randomized.
	if (m_CrateType == CRATE_POINT_TEMPLATE)
		return false;

	// If we have only one NPC, it's probably supposed to spawn correctly.
	// (if we have a bunch, it's probably a gag)
	if (m_nItemCount == 1 && pEnt->IsNPC())
		return false;

	return true;
}
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_ItemCrate::OnBreak( const Vector &vecVelocity, const AngularImpulse &angImpulse, CBaseEntity *pBreaker )
{
	// FIXME: We could simply store the name of an entity to put into the crate 
	// as a string entered in by worldcraft. Should we?	I'd do it for sure
	// if it was easy to get a dropdown with all entity types in it.

	m_OnCacheInteraction.FireOutput(pBreaker,this);

#ifdef MAPBASE
	int iCount = m_nItemCount;
	CUtlVector<CBaseEntity*> hNewEntities;
	CPointTemplate *pTemplate = FindTemplate();

	if (m_CrateType == CRATE_POINT_TEMPLATE)
	{
		if (pTemplate && pTemplate->CreateInstance(GetLocalOrigin(), GetLocalAngles(), &hNewEntities))
		{
			iCount = hNewEntities.Count() * m_nItemCount;
		}
		else
		{
			// This only runs if our template can't be found or its template instancing didn't work.
			Warning("item_item_crate %s with CRATE_POINT_TEMPLATE couldn't find point_template %s! Falling back to CRATE_SPECIFIC_ITEM...\n", GetDebugName(), STRING(ITEM_ITEMCRATE_TEMPLATE_TARGET));
			m_CrateType = CRATE_SPECIFIC_ITEM;
		}
	}
#endif

#ifdef MAPBASE
	for ( int i = 0; i < iCount; i++ )
#else
	for ( int i = 0; i < m_nItemCount; ++i )
#endif
	{
		CBaseEntity *pSpawn = NULL;
		switch( m_CrateType )
		{
		case CRATE_SPECIFIC_ITEM:
			pSpawn = CreateEntityByName( STRING(m_strItemClass) );
			break;

#ifdef MAPBASE
		case CRATE_POINT_TEMPLATE:
		{
			if (i >= hNewEntities.Count())
			{
				if (!pTemplate || !pTemplate->CreateInstance(GetLocalOrigin(), GetLocalAngles(), &hNewEntities))
				{
					pSpawn = NULL;
					i = iCount;
					break;
				}

				i = 0;
				iCount -= hNewEntities.Count();
			}
			pSpawn = hNewEntities[i];
		} break;
#endif

		default:
			break;
		}

		if ( !pSpawn )
			return;

#ifdef MAPBASE
		Vector vecOrigin;
		CollisionProp()->RandomPointInBounds(Vector(0.25, 0.25, 0.25), Vector(0.75, 0.75, 0.75), &vecOrigin);
		pSpawn->SetAbsOrigin(vecOrigin);

		if (ShouldRandomizeAngles(pSpawn))
		{
			// Give a little randomness...
			QAngle vecAngles;
			vecAngles.x = random->RandomFloat(-20.0f, 20.0f);
			vecAngles.y = random->RandomFloat(0.0f, 360.0f);
			vecAngles.z = random->RandomFloat(-20.0f, 20.0f);
			pSpawn->SetAbsAngles(vecAngles);

			Vector vecActualVelocity;
			vecActualVelocity.Random(-10.0f, 10.0f);
			//		vecActualVelocity += vecVelocity;
			pSpawn->SetAbsVelocity(vecActualVelocity);

			QAngle angVel;
			AngularImpulseToQAngle(angImpulse, angVel);
			pSpawn->SetLocalAngularVelocity(angVel);
		}
		else
		{
			// Only modify the Y value.
			QAngle vecAngles;
			vecAngles.x = 0;
			vecAngles.y = GetLocalAngles().y;
			vecAngles.z = 0;
			pSpawn->SetAbsAngles(vecAngles);
		}

		// We handle dynamic resupplies differently
		bool bDynResup = FClassnameIs( pSpawn, "item_dynamic_resupply" );
		if (!bDynResup)
			m_OnItem.Set(pSpawn, pSpawn, this);
		else if (m_OnItem.NumberOfElements() > 0)
		{
			// This is here so it could fire OnItem for each item
			CEventAction *ourlist = m_OnItem.GetActionList();
			char outputdata[256];
			for (CEventAction *ev = ourlist; ev != NULL; ev = ev->m_pNext)
			{
				Q_snprintf(outputdata, sizeof(outputdata), "%s,%s,%s,%f,%i", STRING(ev->m_iTarget), STRING(ev->m_iTargetInput), STRING(ev->m_iParameter), ev->m_flDelay, ev->m_nTimesToFire);
				pSpawn->KeyValue("OnItem", outputdata);
			}
		}
#else
		// Give a little randomness...
		Vector vecOrigin;
		CollisionProp()->RandomPointInBounds( Vector(0.25, 0.25, 0.25), Vector( 0.75, 0.75, 0.75 ), &vecOrigin );
		pSpawn->SetAbsOrigin( vecOrigin );

		QAngle vecAngles;
		vecAngles.x = random->RandomFloat( -20.0f, 20.0f );
		vecAngles.y = random->RandomFloat( 0.0f, 360.0f );
		vecAngles.z = random->RandomFloat( -20.0f, 20.0f );
		pSpawn->SetAbsAngles( vecAngles );

		Vector vecActualVelocity;
		vecActualVelocity.Random( -10.0f, 10.0f );
//		vecActualVelocity += vecVelocity;
		pSpawn->SetAbsVelocity( vecActualVelocity );

		QAngle angVel;
		AngularImpulseToQAngle( angImpulse, angVel );
		pSpawn->SetLocalAngularVelocity( angVel );
#endif

		// If we're creating an item, it can't be picked up until it comes to rest
		// But only if it wasn't broken by a vehicle
		CItem *pItem = dynamic_cast<CItem*>(pSpawn);
		if ( pItem && !pBreaker->GetServerVehicle())
		{
			pItem->ActivateWhenAtRest();
		}

		pSpawn->Spawn();

		// Avoid missing items drops by a dynamic resupply because they don't think immediately
#ifdef MAPBASE
		if (bDynResup)
#else
		if ( FClassnameIs( pSpawn, "item_dynamic_resupply" ) )
#endif
		{
			if ( m_strAlternateMaster != NULL_STRING )
			{
				DynamicResupply_InitFromAlternateMaster( pSpawn, m_strAlternateMaster );
			}
			if ( i == 0 )
			{
				pSpawn->AddSpawnFlags( SF_DYNAMICRESUPPLY_ALWAYS_SPAWN );
			}
			pSpawn->SetNextThink( gpGlobals->curtime );
		}
	}
}

void CItem_ItemCrate::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	BaseClass::OnPhysGunPickup( pPhysGunUser, reason );

	m_OnCacheInteraction.FireOutput( pPhysGunUser, this );

	if ( reason == PUNTED_BY_CANNON && m_CrateAppearance != CRATE_APPEARANCE_RADAR_BEACON )
	{
		Vector vForward;
		AngleVectors( pPhysGunUser->EyeAngles(), &vForward, NULL, NULL );
		Vector vForce = Pickup_PhysGunLaunchVelocity( this, vForward, PHYSGUN_FORCE_PUNTED );
		AngularImpulse angular = AngularImpulse( 0, 0, 0 );

		IPhysicsObject *pPhysics = VPhysicsGetObject();

		if ( pPhysics )
		{
			pPhysics->AddVelocity( &vForce, &angular );
		}

		TakeDamage( CTakeDamageInfo( pPhysGunUser, pPhysGunUser, GetHealth(), DMG_GENERIC ) );
	}
}
