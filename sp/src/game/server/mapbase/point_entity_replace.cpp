//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Replaces a thing with another thing.
//
//=============================================================================

#include "cbase.h"
#include "TemplateEntities.h"
#include "point_template.h"
#include "saverestore_utlvector.h"
#include "datadesc_mod.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointEntityReplace : public CLogicalEntity
{
	DECLARE_CLASS( CPointEntityReplace, CLogicalEntity );
	DECLARE_DATADESC();

public:

	// The entity that will replace the target.
	string_t m_iszReplacementEntity;

	// Only used with certain replacement types
	EHANDLE m_hReplacementEntity;

	// How we should get the replacement entity.
	int m_iReplacementType;
	enum
	{
		REPLACEMENTTYPE_TARGET_TELEPORT, // Replace with target entity
		REPLACEMENTTYPE_CLASSNAME, // Replace with entity of specified classname
		REPLACEMENTTYPE_TEMPLATE, // Replace with a point_template's contents
		REPLACEMENTTYPE_TEMPLATE_RELATIVE, // Replace with a point_template's contents, maintaining relative position
		REPLACEMENTTYPE_RANDOM_TEMPLATE, // Replace with one of a point_template's templates, randomly selected
		REPLACEMENTTYPE_RANDOM_TEMPLATE_RELATIVE, // Replace with one of a point_template's templates, randomly selected, maintaining relative position
	};

	// Where the replacement entit(ies) should replace at.
	int m_iReplacementLocation;
	enum
	{
		REPLACEMENTLOC_ABSOLUTEORIGIN,
		REPLACEMENTLOC_WORLDSPACECENTER,
	};

	// Do we actually replace the target entity or just function as a glorified point_teleport?
	bool m_bRemoveOriginalEntity;

	// What stuff we should take.
	int m_iTakeStuff;
	enum
	{
		REPLACEMENTTAKE_NAME =			(1 << 0), // Takes the target's name
		REPLACEMENTTAKE_PARENT =		(1 << 1), // Takes parent and transfers children
		REPLACEMENTTAKE_OWNER =			(1 << 2), // Takes owner entity
		REPLACEMENTTAKE_MODELSTUFF =	(1 << 3), // Takes model stuff
	};

	// Used to cause it to take other fields, most of that has been moved to m_bTakeModelStuff and m_iszOtherTakes
	//bool m_bTakeStuff;

	// Other keyvalues to copy.
	CUtlVector<string_t> m_iszOtherTakes;

	// Fire OnReplace with the original entity as the caller.
	bool m_bFireOriginalEntityAsCaller;

	bool KeyValue(const char *szKeyName, const char *szValue);

	void HandleReplacement(CBaseEntity *pEntity, CBaseEntity *pReplacement);

	CBaseEntity *GetReplacementEntity(inputdata_t *inputdata);

	void ReplaceEntity(CBaseEntity *pEntity, inputdata_t &inputdata);

	// Inputs
	void InputReplace( inputdata_t &inputdata );
	void InputReplaceEntity( inputdata_t &inputdata );

	void InputSetReplacementEntity( inputdata_t &inputdata );

	COutputEHANDLE m_OnReplace; // Passes the entity we replaced the target with, fired multiple times if REPLACEMENTTYPE_TEMPLATE created multiple entities
};

LINK_ENTITY_TO_CLASS(point_entity_replace, CPointEntityReplace);

BEGIN_DATADESC( CPointEntityReplace )

	// Keys
	DEFINE_KEYFIELD( m_iszReplacementEntity, FIELD_STRING, "ReplacementEntity" ),
	DEFINE_FIELD( m_hReplacementEntity, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_iReplacementType, FIELD_INTEGER, "ReplacementType" ),
	DEFINE_KEYFIELD( m_iReplacementLocation, FIELD_INTEGER, "ReplacementLocation" ),
	DEFINE_KEYFIELD( m_bRemoveOriginalEntity, FIELD_BOOLEAN, "RemoveOriginalEntity" ),
	DEFINE_FIELD( m_iTakeStuff, FIELD_INTEGER ),
	DEFINE_UTLVECTOR( m_iszOtherTakes, FIELD_STRING ),
	DEFINE_KEYFIELD( m_bFireOriginalEntityAsCaller, FIELD_BOOLEAN, "TargetIsCaller" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Replace", InputReplace ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "ReplaceEntity", InputReplaceEntity ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetReplacementEntity", InputSetReplacementEntity ),

	// Outputs
	DEFINE_OUTPUT(m_OnReplace, "OnReplace"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Handles key values from the BSP before spawn is called.
//-----------------------------------------------------------------------------
bool CPointEntityReplace::KeyValue( const char *szKeyName, const char *szValue )
{
	if (!stricmp(szKeyName, "OtherStuff"))
	{
		if (szValue && szValue[0] != '\0')
		{
			CUtlStringList vecKeyList;
			Q_SplitString(szValue, ",", vecKeyList);
			FOR_EACH_VEC(vecKeyList, i)
			{
				m_iszOtherTakes.AddToTail(AllocPooledString(vecKeyList[i]));
			}
		}
		return true;
	}
	else if (strnicmp(szKeyName, "Take", 4) == 0)
	{
		const char *subject = (szKeyName + 4);
		int flag = 0;
		if (FStrEq(subject, "Targetname"))
			flag |= REPLACEMENTTAKE_NAME;
		else if (FStrEq(subject, "Parent"))
			flag |= REPLACEMENTTAKE_PARENT;
		else if (FStrEq(subject, "Owner"))
			flag |= REPLACEMENTTAKE_OWNER;
		else if (FStrEq(subject, "ModelStuff"))
			flag |= REPLACEMENTTAKE_MODELSTUFF;
		else
			return BaseClass::KeyValue(szKeyName, szValue);

		// Remove the flag if 0, add the flag if not 0
		!FStrEq(szValue, "0") ? (m_iTakeStuff |= flag) : (m_iTakeStuff &= ~flag);

		return true;
	}

	return BaseClass::KeyValue(szKeyName, szValue);
}

//-----------------------------------------------------------------------------
// Purpose: Takes targetname, fields, etc. Keep in mind neither are checked for null!
//-----------------------------------------------------------------------------
void CPointEntityReplace::HandleReplacement(CBaseEntity *pEntity, CBaseEntity *pReplacement)
{
	if (m_iTakeStuff & REPLACEMENTTAKE_NAME)
		pReplacement->SetName(pEntity->GetEntityName());

	if (m_iTakeStuff & REPLACEMENTTAKE_PARENT)
	{
		if (pEntity->GetParent())
			pReplacement->SetParent(pEntity->GetParent(), pEntity->GetParentAttachment());

		TransferChildren(pEntity, pReplacement);
	}

	if (m_iTakeStuff & REPLACEMENTTAKE_OWNER)
	{
		if (pEntity->GetOwnerEntity())
			pReplacement->SetOwnerEntity(pEntity->GetOwnerEntity());
	}

	if (m_iTakeStuff & REPLACEMENTTAKE_MODELSTUFF)
	{
		pReplacement->m_nRenderMode = pEntity->m_nRenderMode;
		pReplacement->m_nRenderFX = pEntity->m_nRenderFX;
		pReplacement->m_clrRender = pEntity->m_clrRender;
		if (pEntity->GetBaseAnimating() && pReplacement->GetBaseAnimating())
		{
			CBaseAnimating *pEntityAnimating = pEntity->GetBaseAnimating();
			CBaseAnimating *pReplacementAnimating = pReplacement->GetBaseAnimating();

			pReplacementAnimating->CopyAnimationDataFrom(pEntityAnimating);

			for ( int iPose = 0; iPose < MAXSTUDIOPOSEPARAM; ++iPose )
			{
				pReplacementAnimating->SetPoseParameter( iPose, pEntityAnimating->GetPoseParameter( iPose ) );
			}
		}
	}

	if (m_iszOtherTakes.Count() > 0)
	{
		CUtlVector<variant_t> szValues;
		szValues.AddMultipleToTail( m_iszOtherTakes.Count() );

		for ( datamap_t *dmap = pEntity->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
		{
			// search through all the readable fields in the data description, looking for a match
			for ( int i = 0; i < dmap->dataNumFields; i++ )
			{
				if ( dmap->dataDesc[i].flags & (FTYPEDESC_SAVE | FTYPEDESC_KEY) && dmap->dataDesc[i].fieldName )
				{
					DevMsg("Target Field Name: %s,\n", dmap->dataDesc[i].fieldName);
					for (int i2 = 0; i2 < m_iszOtherTakes.Count(); i2++)
					{
						if ( FStrEq(dmap->dataDesc[i].fieldName, STRING(m_iszOtherTakes[i2])) )
						{
							//szValues[i2] = (((char *)pEntity) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ]);
							szValues[i2].Set( dmap->dataDesc[i].fieldType, ((char*)pEntity) + dmap->dataDesc[i].fieldOffset[TD_OFFSET_NORMAL] );
							break;
						}
					}
				}
			}
		}

		for ( datamap_t *dmap = pReplacement->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
		{
			// search through all the readable fields in the data description, looking for a match
			for ( int i = 0; i < dmap->dataNumFields; i++ )
			{
				if ( dmap->dataDesc[i].flags & (FTYPEDESC_SAVE | FTYPEDESC_KEY) && dmap->dataDesc[i].fieldName )
				{
					DevMsg("Replacement Field Name: %s,\n", dmap->dataDesc[i].fieldName);
					for (int i2 = 0; i2 < m_iszOtherTakes.Count(); i2++)
					{
						if ( FStrEq(dmap->dataDesc[i].fieldName, STRING(m_iszOtherTakes[i2])) )
						{
							//(void*)(((char *)pReplacement) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ]) = szValues[i2];

							//char *data = (((char *)pReplacement) + dmap->dataDesc[i].fieldOffset[TD_OFFSET_NORMAL]);
							//memcpy(data, szValues[i2], sizeof(*data));

							//Datadesc_SetFieldString( szValues[i2], pReplacement, &dmap->dataDesc[i] );

							szValues[i2].SetOther( (((char *)pReplacement) + dmap->dataDesc[i].fieldOffset[TD_OFFSET_NORMAL]) );
							break;
						}
					}
				}
			}
		}
	}

	/*
	// This is largely duplicated from phys_convert
	if (m_bTakeStuff)
	{
		pReplacement->m_nRenderMode = pEntity->m_nRenderMode;
		pReplacement->m_nRenderFX = pEntity->m_nRenderFX;
		const color32 rclr = pEntity->GetRenderColor();
		pReplacement->SetRenderColor(rclr.r, rclr.g, rclr.b, rclr.a);
		if (pEntity->GetBaseAnimating() && pReplacement->GetBaseAnimating())
		{
			CBaseAnimating *pEntityAnimating = pEntity->GetBaseAnimating();
			CBaseAnimating *pReplacementAnimating = pReplacement->GetBaseAnimating();

			pReplacementAnimating->CopyAnimationDataFrom(pEntityAnimating);

			//pReplacementAnimating->SetCycle(pEntityAnimating->GetCycle());
			//pReplacementAnimating->IncrementInterpolationFrame();
			//pReplacementAnimating->SetSequence(pEntityAnimating->GetSequence());
			//pReplacementAnimating->m_flAnimTime = pEntityAnimating->m_flAnimTime;
			//pReplacementAnimating->m_nBody = pEntityAnimating->m_nBody;
			//pReplacementAnimating->m_nSkin = pEntityAnimating->m_nSkin;
			//pReplacementAnimating->SetModelScale(pEntityAnimating->GetModelScale());
		}

		UTIL_TransferPoseParameters(pEntity, pReplacement);
		TransferChildren(pEntity, pReplacement);
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CBaseEntity *CPointEntityReplace::GetReplacementEntity(inputdata_t *inputdata)
{
	if (!m_hReplacementEntity)
		m_hReplacementEntity.Set(gEntList.FindEntityByName(NULL, STRING(m_iszReplacementEntity), this, inputdata ? inputdata->pActivator : NULL, inputdata ? inputdata->pCaller : NULL));
	return m_hReplacementEntity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointEntityReplace::ReplaceEntity(CBaseEntity *pEntity, inputdata_t &inputdata)
{
	Vector vecOrigin;
	if (m_iReplacementLocation == REPLACEMENTLOC_WORLDSPACECENTER)
		vecOrigin = pEntity->WorldSpaceCenter();
	else
		vecOrigin = pEntity->GetAbsOrigin();

	QAngle angAngles = pEntity->GetAbsAngles();
	Vector vecVelocity;
	QAngle angVelocity;
	if (pEntity->VPhysicsGetObject())
	{
		AngularImpulse angImpulse;
		pEntity->VPhysicsGetObject()->GetVelocity(&vecVelocity, &angImpulse);
		AngularImpulseToQAngle(angImpulse, angVelocity);
	}
	else
	{
		vecVelocity = pEntity->GetAbsVelocity();
		angVelocity = pEntity->GetLocalAngularVelocity();
	}

	// No conflicts with the replacement entity until we're finished
	if (m_bRemoveOriginalEntity && !(m_iTakeStuff & REPLACEMENTTAKE_MODELSTUFF))
	{
		pEntity->AddSolidFlags( FSOLID_NOT_SOLID );
		pEntity->AddEffects( EF_NODRAW );
	}

	CBaseEntity *pCaller = m_bFireOriginalEntityAsCaller ? pEntity : this;

	switch (m_iReplacementType)
	{
		case REPLACEMENTTYPE_TARGET_TELEPORT:
		{
			CBaseEntity *pReplacementEntity = GetReplacementEntity(&inputdata);
			if (pReplacementEntity)
			{
				HandleReplacement(pEntity, pReplacementEntity);
				pReplacementEntity->Teleport(&vecOrigin, &angAngles, &vecVelocity);

				if (pReplacementEntity->VPhysicsGetObject())
				{
					AngularImpulse angImpulse;
					QAngleToAngularImpulse(angAngles, angImpulse);
					pReplacementEntity->VPhysicsGetObject()->SetVelocityInstantaneous(&vecVelocity, &angImpulse);
				}
				else
				{
					pReplacementEntity->SetAbsVelocity(vecVelocity);
					pReplacementEntity->SetBaseVelocity( vec3_origin );
					pReplacementEntity->SetLocalAngularVelocity(angVelocity);
				}

				m_OnReplace.Set(pReplacementEntity, pReplacementEntity, pCaller);
			}
		} break;
		case REPLACEMENTTYPE_CLASSNAME:
		{
			CBaseEntity *pReplacementEntity = CreateNoSpawn(STRING(m_iszReplacementEntity), vecOrigin, angAngles);
			if (pReplacementEntity)
			{
				HandleReplacement(pEntity, pReplacementEntity);

				DispatchSpawn(pReplacementEntity);

				if (pReplacementEntity->VPhysicsGetObject())
				{
					IPhysicsObject *pPhys = pReplacementEntity->VPhysicsGetObject();
					AngularImpulse angImpulse;
					QAngleToAngularImpulse(angAngles, angImpulse);
					pPhys->SetVelocityInstantaneous(&vecVelocity, &angImpulse);
				}
				else
				{
					pReplacementEntity->SetAbsVelocity(vecVelocity);
					pReplacementEntity->SetBaseVelocity( vec3_origin );
					pReplacementEntity->SetLocalAngularVelocity(angVelocity);
				}

				m_OnReplace.Set(pReplacementEntity, pReplacementEntity, pCaller);
			}
		} break;
		case REPLACEMENTTYPE_TEMPLATE:
		case REPLACEMENTTYPE_TEMPLATE_RELATIVE:
		{
			CPointTemplate *pTemplate = dynamic_cast<CPointTemplate*>(GetReplacementEntity(&inputdata));
			if (!pTemplate)
			{
				Warning("%s unable to retrieve entity %s. It either does not exist or is not a point_template.\n", GetDebugName(), STRING(m_iszReplacementEntity));
				return;
			}

			CUtlVector<CBaseEntity*> hNewEntities;
			if ( !pTemplate->CreateInstance( vecOrigin, angAngles, &hNewEntities ) || hNewEntities.Count() == 0 )
				return;

			CBaseEntity *pTemplateEntity = NULL;
			for (int i = 0; i < hNewEntities.Count(); i++)
			{
				pTemplateEntity = hNewEntities[i];
				if (pTemplateEntity)
				{
					HandleReplacement(pEntity, pTemplateEntity);

					if (m_iReplacementType != REPLACEMENTTYPE_TEMPLATE_RELATIVE)
					{
						// We have to do this again.
						pTemplateEntity->Teleport( &vecOrigin, &angAngles, &vecVelocity );
					}

					if (pTemplateEntity->VPhysicsGetObject())
					{
						AngularImpulse angImpulse;
						QAngleToAngularImpulse(angAngles, angImpulse);
						pTemplateEntity->VPhysicsGetObject()->SetVelocityInstantaneous(&vecVelocity, &angImpulse);
					}
					else
					{
						pTemplateEntity->SetAbsVelocity(vecVelocity);
						pTemplateEntity->SetBaseVelocity( vec3_origin );
						pTemplateEntity->SetLocalAngularVelocity(angVelocity);
					}

					m_OnReplace.Set(pTemplateEntity, pTemplateEntity, pCaller);
				}
			}
		} break;
		case REPLACEMENTTYPE_RANDOM_TEMPLATE:
		case REPLACEMENTTYPE_RANDOM_TEMPLATE_RELATIVE:
		{
			CPointTemplate *pTemplate = dynamic_cast<CPointTemplate*>(GetReplacementEntity(&inputdata));
			if (!pTemplate)
			{
				Warning("%s unable to retrieve entity %s. It either does not exist or is not a point_template.\n", GetDebugName(), STRING(m_iszReplacementEntity));
				return;
			}

			CBaseEntity *pTemplateEntity = NULL;
			if ( !pTemplate->CreateSpecificInstance( RandomInt(0, pTemplate->GetNumTemplates() - 1), vecOrigin, angAngles, &pTemplateEntity ) )
				return;

			if (pTemplateEntity)
			{
				HandleReplacement(pEntity, pTemplateEntity);

				if (m_iReplacementType != REPLACEMENTTYPE_RANDOM_TEMPLATE_RELATIVE)
				{
					// We have to do this again.
					pTemplateEntity->Teleport( &vecOrigin, &angAngles, &vecVelocity );
				}

				if (pTemplateEntity->VPhysicsGetObject())
				{
					AngularImpulse angImpulse;
					QAngleToAngularImpulse(angAngles, angImpulse);
					pTemplateEntity->VPhysicsGetObject()->SetVelocityInstantaneous(&vecVelocity, &angImpulse);
				}
				else
				{
					pTemplateEntity->SetAbsVelocity(vecVelocity);
					pTemplateEntity->SetBaseVelocity( vec3_origin );
					pTemplateEntity->SetLocalAngularVelocity(angVelocity);
				}

				m_OnReplace.Set(pTemplateEntity, pTemplateEntity, pCaller);
			}
		} break;
	}

	if (m_bRemoveOriginalEntity)
		UTIL_Remove(pEntity);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointEntityReplace::InputReplace( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = gEntList.FindEntityByName(NULL, STRING(m_target), this, inputdata.pActivator, inputdata.pCaller);
	if (pEntity)
		ReplaceEntity(pEntity, inputdata);
	else
		Warning("%s unable to find replacement target %s.\n", GetDebugName(), STRING(m_target));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointEntityReplace::InputReplaceEntity( inputdata_t &inputdata )
{
	if (inputdata.value.Entity())
		ReplaceEntity(inputdata.value.Entity(), inputdata);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointEntityReplace::InputSetReplacementEntity( inputdata_t &inputdata )
{
	m_iszReplacementEntity = inputdata.value.StringID();
	m_hReplacementEntity = NULL;
}
