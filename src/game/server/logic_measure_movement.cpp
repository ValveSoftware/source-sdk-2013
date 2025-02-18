//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This will measure the movement of a target entity and move
// another entity to match the movement of the first.
//
//=============================================================================//

#include "cbase.h"
#include "baseentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// This will measure the movement of a target entity and move
// another entity to match the movement of the first.
//-----------------------------------------------------------------------------
class CLogicMeasureMovement : public CLogicalEntity
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CLogicMeasureMovement, CLogicalEntity );

public:
	virtual void Activate();

private:
	void SetMeasureTarget( const char *pName );
	void SetMeasureReference( const char *pName );
	void SetTarget( const char *pName );
	void SetTargetReference( const char *pName );

	void InputSetMeasureTarget( inputdata_t &inputdata );
	void InputSetMeasureReference( inputdata_t &inputdata );
	void InputSetTarget( inputdata_t &inputdata );
	void InputSetTargetReference( inputdata_t &inputdata );
	void InputSetTargetScale( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	void MeasureThink();

private:
	enum
	{
		MEASURE_POSITION = 0,
		MEASURE_EYE_POSITION,
	};

	string_t m_strMeasureTarget;
	string_t m_strMeasureReference;
	string_t m_strTargetReference;

	EHANDLE m_hMeasureTarget;
	EHANDLE m_hMeasureReference;
	EHANDLE m_hTarget;
	EHANDLE m_hTargetReference;

	float m_flScale;
	int m_nMeasureType;
};


LINK_ENTITY_TO_CLASS( logic_measure_movement, CLogicMeasureMovement );


BEGIN_DATADESC( CLogicMeasureMovement )

	DEFINE_KEYFIELD( m_strMeasureTarget, FIELD_STRING, "MeasureTarget" ),
	DEFINE_KEYFIELD( m_strMeasureReference, FIELD_STRING, "MeasureReference" ),
	DEFINE_KEYFIELD( m_strTargetReference, FIELD_STRING, "TargetReference" ),
	DEFINE_KEYFIELD( m_flScale, FIELD_FLOAT, "TargetScale" ),
	DEFINE_KEYFIELD( m_nMeasureType, FIELD_INTEGER, "MeasureType" ),

	DEFINE_FIELD( m_hMeasureTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hMeasureReference, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTargetReference, FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetMeasureTarget", InputSetMeasureTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMeasureReference", InputSetMeasureReference ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTarget", InputSetTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTargetReference", InputSetTargetReference ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetTargetScale", InputSetTargetScale ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_THINKFUNC( MeasureThink ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Methods to change various targets
//-----------------------------------------------------------------------------
void CLogicMeasureMovement::Activate()
{
	BaseClass::Activate();

	SetMeasureTarget( STRING(m_strMeasureTarget) );
	SetMeasureReference( STRING(m_strMeasureReference) );
	SetTarget( STRING(m_target) );
	SetTargetReference( STRING(m_strTargetReference) );
	
	SetThink( &CLogicMeasureMovement::MeasureThink );
	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
}


//-----------------------------------------------------------------------------
// Sets the name
//-----------------------------------------------------------------------------
void CLogicMeasureMovement::SetMeasureTarget( const char *pName )
{
	m_hMeasureTarget = gEntList.FindEntityByName( NULL, pName );
	if ( !m_hMeasureTarget )
	{
		if ( Q_strnicmp( STRING(m_strMeasureTarget), "!player", 8 ) )
		{
			Warning("logic_measure_movement: Unable to find measure target entity %s\n", pName );
		}
	}
}

void CLogicMeasureMovement::SetMeasureReference( const char *pName )
{
	m_hMeasureReference = gEntList.FindEntityByName( NULL, pName );
	if ( !m_hMeasureReference )
	{
		Warning("logic_measure_movement: Unable to find measure reference entity %s\n", pName );
	}
}

void CLogicMeasureMovement::SetTarget( const char *pName )
{
	m_hTarget = gEntList.FindEntityByName( NULL, pName );
	if ( !m_hTarget )
	{
		Warning("logic_measure_movement: Unable to find movement target entity %s\n", pName );
	}
}

void CLogicMeasureMovement::SetTargetReference( const char *pName )
{
	m_hTargetReference = gEntList.FindEntityByName( NULL, pName );
	if ( !m_hTargetReference )
	{
		Warning("logic_measure_movement: Unable to find movement reference entity %s\n", pName );
	}
}


//-----------------------------------------------------------------------------
// Apply movement
//-----------------------------------------------------------------------------
void CLogicMeasureMovement::MeasureThink( )
{
	// FIXME: This is a hack to make measuring !player simpler. The player isn't
	// created at Activate time, so m_hMeasureTarget may be NULL because of that.
	if ( !m_hMeasureTarget.Get() && !Q_strnicmp( STRING(m_strMeasureTarget), "!player", 8 ) )
	{
		SetMeasureTarget( STRING(m_strMeasureTarget) );
	}

	// Make sure all entities are valid
	if ( m_hMeasureTarget.Get() && m_hMeasureReference.Get() && m_hTarget.Get() && m_hTargetReference.Get() )
	{
		matrix3x4_t matRefToMeasure, matWorldToMeasure;
		switch( m_nMeasureType )
		{
		case MEASURE_POSITION:
			MatrixInvert( m_hMeasureTarget->EntityToWorldTransform(), matWorldToMeasure );
			break;

		case MEASURE_EYE_POSITION:
			AngleIMatrix( m_hMeasureTarget->EyeAngles(), m_hMeasureTarget->EyePosition(), matWorldToMeasure );
			break;

		// FIXME: Could add attachment point measurement here easily
		}

		ConcatTransforms( matWorldToMeasure, m_hMeasureReference->EntityToWorldTransform(), matRefToMeasure );
		
		// Apply the scale factor
		if ( ( m_flScale != 0.0f ) && ( m_flScale != 1.0f ) )
		{
			Vector vecTranslation;
			MatrixGetColumn( matRefToMeasure, 3, vecTranslation );
			vecTranslation /= m_flScale;
			MatrixSetColumn( vecTranslation, 3, matRefToMeasure );
		}

		// Now apply the new matrix to the new reference point
		matrix3x4_t matMeasureToRef, matNewTargetToWorld;
		MatrixInvert( matRefToMeasure, matMeasureToRef );

		ConcatTransforms( m_hTargetReference->EntityToWorldTransform(), matMeasureToRef, matNewTargetToWorld );

		Vector vecNewOrigin;
		QAngle vecNewAngles;
		MatrixAngles( matNewTargetToWorld, vecNewAngles, vecNewOrigin );
		m_hTarget->SetAbsOrigin( vecNewOrigin );
		m_hTarget->SetAbsAngles( vecNewAngles );
	}

	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
}


//-----------------------------------------------------------------------------
// Enable, disable
//-----------------------------------------------------------------------------
void CLogicMeasureMovement::InputEnable( inputdata_t &inputdata )
{
	SetThink( &CLogicMeasureMovement::MeasureThink );
	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
}

void CLogicMeasureMovement::InputDisable( inputdata_t &inputdata )
{
	SetThink( NULL );
}


//-----------------------------------------------------------------------------
// Methods to change various targets
//-----------------------------------------------------------------------------
void CLogicMeasureMovement::InputSetMeasureTarget( inputdata_t &inputdata )
{
	m_strMeasureTarget = MAKE_STRING( inputdata.value.String() );
	SetMeasureTarget( inputdata.value.String() );
	SetTarget( STRING(m_target) );
	SetTargetReference( STRING(m_strTargetReference) );
}

void CLogicMeasureMovement::InputSetMeasureReference( inputdata_t &inputdata )
{
	m_strMeasureReference = MAKE_STRING( inputdata.value.String() );
	SetMeasureReference( inputdata.value.String() );
}

void CLogicMeasureMovement::InputSetTarget( inputdata_t &inputdata )
{
	m_target = MAKE_STRING( inputdata.value.String() );
	SetTarget( inputdata.value.String() );
}

void CLogicMeasureMovement::InputSetTargetReference( inputdata_t &inputdata )
{
	m_strTargetReference = MAKE_STRING( inputdata.value.String() );
	SetTargetReference( inputdata.value.String() );
}

void CLogicMeasureMovement::InputSetTargetScale( inputdata_t &inputdata )
{
	m_flScale = inputdata.value.Float();
}
