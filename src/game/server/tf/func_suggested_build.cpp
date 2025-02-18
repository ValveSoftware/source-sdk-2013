//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "func_suggested_build.h"

#include "tf_shareddefs.h"
#include "tf_obj.h"
#include "triggers.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
enum
{
	kObjectType_Any,
	kObjectType_Sentry,
	kObjectType_Dispenser,
	kObjectType_TeleporterEntrance,
	kObjectType_TeleporterExit,
};

enum
{
	kSpawnFlag_BuiltObjectNeverDies = 1 << 0,
};

class CFuncSuggestedBuild : public CBaseTrigger
{
	DECLARE_CLASS( CFuncSuggestedBuild, CBaseTrigger );
public:
	CFuncSuggestedBuild();

	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void Activate( void );

	// Inputs
	void	InputSetActive( inputdata_t &inputdata );
	void	InputSetInactive( inputdata_t &inputdata );
	void	InputToggleActive( inputdata_t &inputdata );

	void	SetActive( bool bActive );
	bool	GetActive() const;

	bool	MatchesObjectType( int iObjectType, int iObjectMode ) const;
	bool	IsPointInArea( const Vector &vecPoint );
	bool	IsFacingRequiredEntity( CBaseObject &baseObject ) const;

	void	OnBuildInArea( CBaseObject& baseObject );
	void	OnBuildInAreaNotFacing( CBaseObject& baseObject );
	void	OnBuildingUpgraded( CBaseObject& baseObject );

private:
	bool	m_bActive;
	int		m_iObjectType;
	string_t m_sFaceEntityName;
	float	m_flFaceEntityFOV;
	CHandle< CBaseEntity > m_hFaceEntity;
	COutputEvent m_outputBuildInsideArea;
	COutputEvent m_outputBuildNotFacing;
	COutputEvent m_outputBuildingUpgraded;
};

LINK_ENTITY_TO_CLASS( func_suggested_build, CFuncSuggestedBuild);

BEGIN_DATADESC( CFuncSuggestedBuild )
	DEFINE_KEYFIELD( m_iObjectType,		FIELD_INTEGER,	"object_type" ),
	DEFINE_KEYFIELD( m_sFaceEntityName,	FIELD_STRING,	"face_entity" ),
	DEFINE_KEYFIELD( m_flFaceEntityFOV,	FIELD_FLOAT,	"face_entity_fov" ),
	// inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "SetActive", InputSetActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetInactive", InputSetInactive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ToggleActive", InputToggleActive ),
	// outputs
	DEFINE_OUTPUT( m_outputBuildInsideArea, "OnBuildInsideArea" ),
	DEFINE_OUTPUT( m_outputBuildNotFacing, "OnBuildNotFacing" ),
	DEFINE_OUTPUT( m_outputBuildingUpgraded, "OnBuildingUpgraded" ),
END_DATADESC()


PRECACHE_REGISTER( func_suggested_build );

const float kDefaultFacingFOV = cos(M_PI);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncSuggestedBuild::CFuncSuggestedBuild()
	: CBaseTrigger()
	, m_bActive(false)
	, m_iObjectType(kObjectType_Any)
	, m_flFaceEntityFOV(kDefaultFacingFOV)
{
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the resource zone
//-----------------------------------------------------------------------------
void CFuncSuggestedBuild::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();

	m_bActive = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncSuggestedBuild::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncSuggestedBuild::Activate( void )
{
	BaseClass::Activate();
	SetActive( true );

	m_hFaceEntity = gEntList.FindEntityByName( NULL, m_sFaceEntityName.ToCStr() );
	m_flFaceEntityFOV = cos( DEG2RAD( m_flFaceEntityFOV ) );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncSuggestedBuild::InputSetActive( inputdata_t &inputdata )
{
	SetActive( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncSuggestedBuild::InputSetInactive( inputdata_t &inputdata )
{
	SetActive( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncSuggestedBuild::InputToggleActive( inputdata_t &inputdata )
{
	SetActive( !m_bActive );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncSuggestedBuild::SetActive( bool bActive )
{
	m_bActive = bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncSuggestedBuild::GetActive() const
{
	return m_bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncSuggestedBuild::MatchesObjectType( int iObjectType, int iObjectMode ) const
{
	bool bPassesCriteria = true;
	switch ( m_iObjectType )
	{
	case kObjectType_Any:
		bPassesCriteria = true;
		break;
	case kObjectType_Sentry:
		bPassesCriteria = iObjectType == OBJ_SENTRYGUN;
		break;
	case kObjectType_Dispenser:
		bPassesCriteria = iObjectType == OBJ_DISPENSER;
		break;
	case kObjectType_TeleporterEntrance:
		bPassesCriteria = iObjectType == OBJ_TELEPORTER && iObjectMode == MODE_TELEPORTER_ENTRANCE;
		break;
	case kObjectType_TeleporterExit:
		bPassesCriteria = iObjectType == OBJ_TELEPORTER && iObjectMode == MODE_TELEPORTER_EXIT;
		break;
	}
	return bPassesCriteria;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncSuggestedBuild::IsPointInArea( const Vector &vecPoint )
{
	Ray_t ray;
	trace_t tr;
	ICollideable *pCollide = CollisionProp();
	ray.Init( vecPoint, vecPoint );
	enginetrace->ClipRayToCollideable( ray, MASK_ALL, pCollide, &tr );
	return ( tr.startsolid );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncSuggestedBuild::IsFacingRequiredEntity( CBaseObject &baseObject ) const
{
	if ( m_hFaceEntity )
	{
		// check to see if the object is facing the required entity in 2D
		Vector facingDir;
		AngleVectors( baseObject.GetAbsAngles(), &facingDir );
		Vector toEntity = m_hFaceEntity->GetAbsOrigin() - baseObject.GetAbsOrigin();
		toEntity.z = 0;
		toEntity.NormalizeInPlace();
		if ( toEntity.IsZero() == false )
		{
			float cosAngle = DotProduct( toEntity, facingDir );
			float cosTolerance = m_flFaceEntityFOV;
			return cosAngle > cosTolerance;
		}
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncSuggestedBuild::OnBuildInArea( CBaseObject& baseObject )
{
	m_outputBuildInsideArea.FireOutput( &baseObject, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncSuggestedBuild::OnBuildInAreaNotFacing( CBaseObject& baseObject )
{
	m_outputBuildNotFacing.FireOutput( &baseObject, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncSuggestedBuild::OnBuildingUpgraded( CBaseObject& baseObject )
{
	m_outputBuildingUpgraded.FireOutput( &baseObject, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool NotifyObjectBuiltInSuggestedArea( CBaseObject &baseObject )
{
	const Vector &vecBuildOrigin = baseObject.GetAbsOrigin();
	int iObjectType = baseObject.ObjectType();
	int iObjectMode = baseObject.GetObjectMode();
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "func_suggested_build" ) ) != NULL )
	{
		CFuncSuggestedBuild *pSuggestedBuild = (CFuncSuggestedBuild *)pEntity;
		if ( pSuggestedBuild->GetActive() == false )
		{
			continue;
		}
		if ( pSuggestedBuild->MatchesObjectType( iObjectType, iObjectMode ) == false )
		{
			continue;
		}
		if ( pSuggestedBuild->IsPointInArea( vecBuildOrigin ) == false )
		{
			continue;
		}
		// check orientation last
		if ( pSuggestedBuild->IsFacingRequiredEntity( baseObject ) == false )
		{
			pSuggestedBuild->OnBuildInAreaNotFacing( baseObject );
			return true;
		}
		else
		{
			// fire off output
			pSuggestedBuild->OnBuildInArea( baseObject );
			// "transfer" flags
			if ( pSuggestedBuild->HasSpawnFlags( kSpawnFlag_BuiltObjectNeverDies ) )
			{
				baseObject.SetCannotDie( true );
			}
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool NotifyObjectUpgradedInSuggestedArea( CBaseObject &baseObject )
{
	const Vector &vecBuildOrigin = baseObject.GetAbsOrigin();
	int iObjectType = baseObject.ObjectType();
	int iObjectMode = baseObject.GetObjectMode();
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "func_suggested_build" ) ) != NULL )
	{
		CFuncSuggestedBuild *pSuggestedBuild = (CFuncSuggestedBuild *)pEntity;
		if ( pSuggestedBuild->GetActive() == false )
		{
			continue;
		}
		if ( pSuggestedBuild->MatchesObjectType( iObjectType, iObjectMode ) == false )
		{
			continue;
		}
		if ( pSuggestedBuild->IsPointInArea( vecBuildOrigin ) == false )
		{
			continue;
		}
		if ( pSuggestedBuild->IsFacingRequiredEntity( baseObject ) == false )
		{
			continue;
		}
		pSuggestedBuild->OnBuildingUpgraded( baseObject );
		return true;
	}
	return false;
}