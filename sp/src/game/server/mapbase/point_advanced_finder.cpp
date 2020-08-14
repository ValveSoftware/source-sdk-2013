//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: A ballsier version of point_entity_finder.
//			Originally called logic_entityfinder because a lot of this was written 
//			before I knew about point_entity_finder in the first place.
//
//=============================================================================

#include "cbase.h"
#include "eventqueue.h"
#include "filters.h"
#include "saverestore_utlvector.h"

// Uses a CUtlVector instead of an array.
#define ENTITYFINDER_UTLVECTOR 1

// Delays outputs directly instead of relying on an input.
#define ENTITYFINDER_OUTPUT_DELAY 1

#if !ENTITYFINDER_STATIC_ARRAY
#define ENTITYFINDER_MAX_STORED_ENTITIES 64
#endif

//-----------------------------------------------------------------------------
// Purpose: Entity finder that uses filters and other criteria to search the level for a specific entity.
//-----------------------------------------------------------------------------
class CPointAdvancedFinder : public CLogicalEntity
{
	DECLARE_CLASS(CPointAdvancedFinder, CLogicalEntity);

private:
	// Inputs
	void InputSearch(inputdata_t &inputdata);
	void InputSetSearchFilter(inputdata_t &inputdata);
	void InputSetSearchPoint(inputdata_t &inputdata);
	void InputSetRadius(inputdata_t &inputdata) { m_flRadius = inputdata.value.Float(); }
	void InputSetMaxResults(inputdata_t &inputdata) { m_iNumSearches = inputdata.value.Int(); }
	void InputSetOutputDelay(inputdata_t &inputdata) { m_flOutputDelay = inputdata.value.Float(); }
	void InputSetFiringMethod(inputdata_t &inputdata) { m_iFiringMethod = inputdata.value.Int(); }
#ifndef ENTITYFINDER_OUTPUT_DELAY
	void InputFoundEntity(inputdata_t &inputdata);
#endif

	void Spawn();

	Vector GetSearchOrigin();
	bool SearchForEntities(inputdata_t &inputdata);
	void FoundEntity(CBaseEntity *pEntity, inputdata_t &inputdata);


	string_t m_iszSearchFilter;
	CHandle<CBaseFilter> m_hSearchFilter;

	string_t m_iszSearchPoint;
	EHANDLE m_hSearchPoint;

	float m_flRadius;
	int m_iNumSearches;
	int m_iFiringMethod;
	enum
	{
		FIRINGMETHOD_NONE = -1, // -1 for point_entity_finder compatibility
		FIRINGMETHOD_NEAREST,
		FIRINGMETHOD_FARTHEST,
		FIRINGMETHOD_RANDOM,
	};

	float m_flOutputDelay;
	float m_flLastOutputDelay = 0.0f;

#if ENTITYFINDER_UTLVECTOR
	CUtlVector<CBaseEntity*> m_StoredEntities;
#else
	CBaseEntity *m_StoredEntities[ENTITYFINDER_MAX_STORED_ENTITIES];
#endif

	// Outputs
	COutputEHANDLE m_OnFoundEntity;
	COutputEvent m_OnSearchFailed;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(point_advanced_finder, CPointAdvancedFinder);


BEGIN_DATADESC(CPointAdvancedFinder)

	// Keys
	DEFINE_KEYFIELD(m_iszSearchFilter, FIELD_STRING, "SearchFilter"),
	DEFINE_FIELD(m_hSearchFilter, FIELD_EHANDLE),
	DEFINE_KEYFIELD(m_iszSearchPoint, FIELD_STRING, "SearchPoint"),
	DEFINE_FIELD(m_hSearchPoint, FIELD_EHANDLE),
	DEFINE_KEYFIELD(m_flRadius, FIELD_FLOAT, "radius"),
	DEFINE_KEYFIELD(m_iNumSearches, FIELD_INTEGER, "NumberOfEntities"),
	DEFINE_KEYFIELD(m_flOutputDelay, FIELD_FLOAT, "OutputDelay"),
	DEFINE_KEYFIELD(m_iFiringMethod, FIELD_INTEGER, "Method"),
#if ENTITYFINDER_UTLVECTOR
	DEFINE_UTLVECTOR( m_StoredEntities, FIELD_CLASSPTR ),
#else
	DEFINE_ARRAY(m_StoredEntities, FIELD_CLASSPTR, ENTITYFINDER_MAX_STORED_ENTITIES),
#endif
	DEFINE_FIELD(m_flLastOutputDelay, FIELD_FLOAT),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "BeginSearch", InputSearch),
	DEFINE_INPUTFUNC(FIELD_STRING, "SetSearchFilter", InputSetSearchFilter),
	DEFINE_INPUTFUNC(FIELD_STRING, "SetSearchPoint", InputSetSearchPoint),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetRadius", InputSetRadius),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetMaxResults", InputSetMaxResults),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetOutputDelay", InputSetOutputDelay),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetFiringMethod", InputSetFiringMethod),
#ifndef ENTITYFINDER_OUTPUT_DELAY
	DEFINE_INPUTFUNC(FIELD_EHANDLE, "FoundEntity", InputFoundEntity),
#endif

	// Outputs
	DEFINE_OUTPUT(m_OnFoundEntity, "OnFoundEntity"),
	DEFINE_OUTPUT(m_OnSearchFailed, "OnSearchFailed"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointAdvancedFinder::Spawn()
{
	if (m_iszSearchFilter == NULL_STRING)
	{
		Warning("%s (%s) has no search filter!\n", GetClassname(), GetDebugName());
		UTIL_Remove(this);
		return;
	}

	m_hSearchFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iszSearchFilter, this ));

	m_hSearchPoint = gEntList.FindEntityByName( NULL, m_iszSearchPoint, this );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointAdvancedFinder::InputSearch(inputdata_t &inputdata)
{
	SearchForEntities(inputdata);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointAdvancedFinder::InputSetSearchFilter(inputdata_t &inputdata)
{
	m_iszSearchFilter = inputdata.value.StringID();
	m_hSearchFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iszSearchFilter, this, inputdata.pActivator, inputdata.pCaller ));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointAdvancedFinder::InputSetSearchPoint(inputdata_t &inputdata)
{
	m_iszSearchPoint = inputdata.value.StringID();
	m_hSearchPoint = gEntList.FindEntityByName( NULL, m_iszSearchPoint, this, inputdata.pActivator, inputdata.pCaller );
}

#ifndef ENTITYFINDER_OUTPUT_DELAY
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointAdvancedFinder::InputFoundEntity(inputdata_t &inputdata)
{
	CBaseEntity *pEntity = inputdata.value.Entity();
	if (!pEntity)
		return;

	m_OnFoundEntity.Set(pEntity, pEntity, inputdata.pCaller);
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline Vector CPointAdvancedFinder::GetSearchOrigin()
{
	if (m_hSearchPoint == NULL)
		m_hSearchPoint = this;

	return m_hSearchPoint->GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPointAdvancedFinder::SearchForEntities(inputdata_t &inputdata)
{
	if ( !m_hSearchFilter )
		return false;

	m_flLastOutputDelay = 0.0f;

	int iNumResults = 0;

	float flRadius = m_flRadius * m_flRadius;

	bool bShouldStoreEntities = (m_iFiringMethod > FIRINGMETHOD_NONE || m_flOutputDelay > 0);
#if !ENTITYFINDER_UTLVECTOR
	if (bShouldStoreEntities)
	{
		if (m_iNumSearches > ENTITYFINDER_MAX_STORED_ENTITIES)
		{
			Warning("%s (%s) needs to store entities, but we're asked to look for more than the maximum, %i, with %i! Reducing to max...\n", GetClassname(), GetDebugName(), ENTITYFINDER_MAX_STORED_ENTITIES, m_iNumSearches);
			m_iNumSearches = ENTITYFINDER_MAX_STORED_ENTITIES;
		}
		else if (m_iNumSearches == 0)
		{
			m_iNumSearches = ENTITYFINDER_MAX_STORED_ENTITIES;
		}
	}
	else
#endif
	if (m_iNumSearches == 0)
	{
		m_iNumSearches = MAX_EDICTS;
	}

	const CEntInfo *pInfo = gEntList.FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		if (iNumResults >= m_iNumSearches)
			break;

		if ( m_hSearchFilter && !m_hSearchFilter->PassesFilter( this, ent ) )
			continue;

		if (flRadius > 0 && (ent->GetAbsOrigin() - GetSearchOrigin()).LengthSqr() > flRadius)
			continue;

		if ( bShouldStoreEntities )
		{
			// Fire it later
#if ENTITYFINDER_UTLVECTOR
			m_StoredEntities.AddToTail(ent);
#else
			m_StoredEntities[ iNumResults ] = ent;
#endif
		}
		else
		{
			// Fire it now
			FoundEntity(ent, inputdata);
		}

		iNumResults++;
	}

	if (iNumResults > 0)
	{
		if (bShouldStoreEntities)
		{
			if (m_iFiringMethod == FIRINGMETHOD_NEAREST || m_iFiringMethod == FIRINGMETHOD_FARTHEST)
			{
				bool bNotFarthest = m_iFiringMethod != FIRINGMETHOD_FARTHEST;
				float flMaxDist = m_flRadius;
				float flMinDist = 0;

				if (flMaxDist == 0)
					flMaxDist = MAX_TRACE_LENGTH;

				for (int iCur = 0; iCur < iNumResults; iCur++)
				{
					float flClosest = bNotFarthest ? flMaxDist : 0;
					float flDistance = 0;
					CBaseEntity *pClosest = NULL;
					for (int i = 0; i < iNumResults; i++)
					{
						if (!m_StoredEntities[i])
							continue;

						flDistance = (m_StoredEntities[i]->GetAbsOrigin() - GetSearchOrigin()).Length();
						if (flDistance < flMaxDist && flDistance > flMinDist)
						{
							if (bNotFarthest ? (flDistance < flClosest) : (flDistance > flClosest))
							{
								pClosest = m_StoredEntities[i];
								flClosest = flDistance;
							}
						}
					}

					if (pClosest)
					{
						bNotFarthest ? flMinDist = flClosest : flMaxDist = flClosest;
						FoundEntity(pClosest, inputdata);
					}
					else
					{
						DevWarning("%s (%s): NO CLOSEST!!!\n", GetClassname(), GetDebugName());
					}
				}
			}
			else if (m_iFiringMethod == FIRINGMETHOD_RANDOM)
			{
				// This could probaly be better...
				CUtlVector<int> iResultIndices;
				for (int i = 0; i < iNumResults; i++)
					iResultIndices.AddToTail(i);

				while (iResultIndices.Count() > 0)
				{
					int index = iResultIndices[RandomInt(0, iResultIndices.Count() - 1)];

					if (m_StoredEntities[index])
					{
						FoundEntity(m_StoredEntities[index], inputdata);
					}
					else
					{
						DevWarning("%s (%s): Found entity is null: %i\n", GetClassname(), GetDebugName(), index);
					}

					iResultIndices.FindAndRemove(index);
				}
			}
			else if (m_iFiringMethod == FIRINGMETHOD_NONE)
			{
				for (int i = 0; i < iNumResults; i++)
				{
					if (m_StoredEntities[i])
					{
						FoundEntity(m_StoredEntities[i], inputdata);
					}
					else
					{
						DevWarning("%s (%s): Found entity is null: %i\n", GetClassname(), GetDebugName(), i);
					}
				}
			}

			m_StoredEntities.RemoveAll();
		}
		return true;
	}
	else
	{
		m_OnSearchFailed.FireOutput(inputdata.pActivator, inputdata.pCaller);
		return false;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointAdvancedFinder::FoundEntity(CBaseEntity *pEntity, inputdata_t &inputdata)
{
#ifdef ENTITYFINDER_OUTPUT_DELAY
	variant_t variant;
	variant.SetEntity(pEntity);

	DevMsg("%s (%s): Firing entity output in %f, added from %f\n", GetClassname(), GetDebugName(), m_flLastOutputDelay, m_flOutputDelay);
	m_OnFoundEntity.FireOutput(variant, pEntity, this, m_flLastOutputDelay);
#else
	if (m_flOutputDelay == 0)
	{
		// Just fire it now
		m_OnFoundEntity.Set(pEntity, pEntity, inputdata.pCaller);
		DevMsg("%s (%s): Delay 0, firing now\n", GetClassname(), GetDebugName());
		return;
	}

	//if (m_flLastOutputDelay == 0)
		//m_flLastOutputDelay = gpGlobals->curtime;

	variant_t variant;
	variant.SetEntity(pEntity);

	DevMsg("%s (%s): Firing entity output in %f, added from %f\n", GetClassname(), GetDebugName(), m_flLastOutputDelay, m_flOutputDelay);
	g_EventQueue.AddEvent(this, "FoundEntity", variant, m_flLastOutputDelay, inputdata.pActivator, inputdata.pCaller);
#endif

	m_flLastOutputDelay += m_flOutputDelay;
}
