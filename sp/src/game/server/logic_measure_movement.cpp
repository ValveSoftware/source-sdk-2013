//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This will measure the movement of a target entity and move
// another entity to match the movement of the first.
//
//=============================================================================//

#include "cbase.h"
#include "baseentity.h"
#ifdef MAPBASE
#include "filters.h"
#include "ai_basenpc.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef MAPBASE
// These spawnflags were originally only on logic_measure_direction.
#define SF_LOGIC_MEASURE_MOVEMENT_IGNORE_X ( 1 << 0 )
#define SF_LOGIC_MEASURE_MOVEMENT_IGNORE_Y ( 1 << 1 )
#define SF_LOGIC_MEASURE_MOVEMENT_IGNORE_Z ( 1 << 2 )

// Uses the "Ignore X/Y/Z" flags for the origin instead of the angles.
// logic_measure_direction uses this flag to control trace direction.
#define SF_LOGIC_MEASURE_MOVEMENT_USE_IGNORE_FLAGS_FOR_ORIGIN ( 1 << 3 )

// Uses "Teleport" instead of "SetAbsOrigin" for smoother movement
#define SF_LOGIC_MEASURE_MOVEMENT_TELEPORT ( 1 << 4 )

// Specifically refuse to set the target's angles, rather than just turning them to 0
#define SF_LOGIC_MEASURE_MOVEMENT_DONT_SET_ANGLES ( 1 << 5 )
#endif

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

#ifdef MAPBASE
public:
#else
private:
#endif
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

#ifdef MAPBASE
	// Allows for derived class trickery
	void MeasureThink(); //{ DoMeasure(); }

	// Allows for InputGetPosition(), etc.
	virtual void DoMeasure(Vector &vecOrigin, QAngle &angAngles);
	void HandleIgnoreFlags( float *vec );

	void InputSetMeasureAttachment( inputdata_t &inputdata );
	void InputSetMeasureType( inputdata_t &inputdata ) { m_nMeasureType = inputdata.value.Int(); }
	void InputGetPosition( inputdata_t &inputdata );
#else
	void MeasureThink();

private:
#endif
	enum
	{
		MEASURE_POSITION = 0,
		MEASURE_EYE_POSITION,
#ifdef MAPBASE
		MEASURE_ATTACHMENT,
		//MEASURE_BARREL_POSITION,
#endif
	};

	string_t m_strMeasureTarget;
	string_t m_strMeasureReference;
	string_t m_strTargetReference;

	EHANDLE m_hMeasureTarget;
	EHANDLE m_hMeasureReference;
	EHANDLE m_hTarget;
	EHANDLE m_hTargetReference;

#ifdef MAPBASE
	string_t m_strAttachment;
	int m_iAttachment;

	bool m_bOutputPosition;

	COutputVector m_OutPosition;
	COutputVector m_OutAngles;
#endif

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
#ifdef MAPBASE
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMeasureType", InputSetMeasureType ),

	DEFINE_KEYFIELD( m_strAttachment, FIELD_STRING, "MeasureAttachment" ),
	DEFINE_FIELD( m_iAttachment, FIELD_EHANDLE ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMeasureAttachment", InputSetMeasureAttachment ),

	DEFINE_INPUT( m_bOutputPosition, FIELD_BOOLEAN, "ShouldOutputPosition" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "GetPosition", InputGetPosition ),
#endif

	DEFINE_FIELD( m_hMeasureTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hMeasureReference, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTargetReference, FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetMeasureTarget", InputSetMeasureTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMeasureReference", InputSetMeasureReference ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTarget", InputSetTarget ),
#ifdef MAPBASE
	DEFINE_INPUTFUNC( FIELD_STRING, "Target", InputSetTarget ), // For legacy support...even though that name was broken before.
#endif
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTargetReference", InputSetTargetReference ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetTargetScale", InputSetTargetScale ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

#ifdef MAPBASE
	DEFINE_OUTPUT( m_OutPosition, "OutPosition" ),
	DEFINE_OUTPUT( m_OutAngles, "OutAngles" ),
#endif

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
#ifdef MAPBASE
	m_hMeasureTarget = gEntList.FindEntityByName( NULL, pName, this );
#else
	m_hMeasureTarget = gEntList.FindEntityByName( NULL, pName );
#endif
	if ( !m_hMeasureTarget )
	{
		if ( Q_strnicmp( STRING(m_strMeasureTarget), "!player", 8 ) )
		{
#ifdef MAPBASE
			Warning( "%s: Unable to find measure target entity %s\n", GetDebugName(), pName );
#else
			Warning("logic_measure_movement: Unable to find measure target entity %s\n", pName );
#endif
		}
	}
#ifdef MAPBASE
	m_iAttachment = 0;
#endif
}

void CLogicMeasureMovement::SetMeasureReference( const char *pName )
{
#ifdef MAPBASE
	m_hMeasureReference = gEntList.FindEntityByName( NULL, pName, this );
#else
	m_hMeasureReference = gEntList.FindEntityByName( NULL, pName );
#endif
	if ( !m_hMeasureReference )
	{
#ifdef MAPBASE
		Warning( "%s: Unable to find measure reference entity %s\n", GetDebugName(), pName );
#else
		Warning("logic_measure_movement: Unable to find measure reference entity %s\n", pName );
#endif
	}
}

void CLogicMeasureMovement::SetTarget( const char *pName )
{
#ifdef MAPBASE
	m_hTarget = gEntList.FindEntityByName( NULL, pName, this );
#else
	m_hTarget = gEntList.FindEntityByName( NULL, pName );
#endif
	if ( !m_hTarget )
	{
#ifdef MAPBASE
		Warning( "%s: Unable to find movement target entity %s\n", GetDebugName(), pName );
#else
		Warning("logic_measure_movement: Unable to find movement target entity %s\n", pName );
#endif
	}
}

void CLogicMeasureMovement::SetTargetReference( const char *pName )
{
#ifdef MAPBASE
	m_hTargetReference = gEntList.FindEntityByName( NULL, pName, this );
#else
	m_hTargetReference = gEntList.FindEntityByName( NULL, pName );
#endif
	if ( !m_hTargetReference )
	{
#ifdef MAPBASE
		Warning( "%s: Unable to find movement reference entity %s\n", GetDebugName(), pName );
#else
		Warning("logic_measure_movement: Unable to find movement reference entity %s\n", pName );
#endif
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
#ifdef MAPBASE
		Vector vecNewOrigin;
		QAngle vecNewAngles;
		DoMeasure(vecNewOrigin, vecNewAngles);

		if (m_bOutputPosition)
		{
			m_OutPosition.Set(vecNewOrigin, m_hTarget.Get(), this);
			m_OutAngles.Set(vecNewAngles, m_hTarget.Get(), this);
		}

		if (HasSpawnFlags( SF_LOGIC_MEASURE_MOVEMENT_TELEPORT ))
		{
			m_hTarget->Teleport( &vecNewOrigin, !HasSpawnFlags(SF_LOGIC_MEASURE_MOVEMENT_DONT_SET_ANGLES) ? &vecNewAngles : NULL, NULL );
		}
		else
		{
			m_hTarget->SetAbsOrigin( vecNewOrigin );

			if (!HasSpawnFlags(SF_LOGIC_MEASURE_MOVEMENT_DONT_SET_ANGLES))
				m_hTarget->SetAbsAngles( vecNewAngles );
		}
#else
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
#endif
	}

	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Moves logic_measure_movement's movement measurements to its own function,
//			primarily to allow for the GetPosition input without any hacks.
//			Also helps with derivative entities that would otherwise have to find a way to re-define the think function.
//			Warning: Doesn't account for whether these handles are null!
//-----------------------------------------------------------------------------
void CLogicMeasureMovement::DoMeasure( Vector &vecOrigin, QAngle &angAngles )
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

	case MEASURE_ATTACHMENT:
		if (CBaseAnimating *pAnimating = m_hMeasureTarget->GetBaseAnimating())
		{
			if (m_iAttachment <= 0)
				m_iAttachment = m_hMeasureTarget->GetBaseAnimating()->LookupAttachment(STRING(m_strAttachment));

			if (m_iAttachment == -1)
				Warning("WARNING: %s requesting invalid attachment %s on %s!\n", GetDebugName(), STRING(m_strAttachment), m_hMeasureTarget->GetDebugName());
			else
				pAnimating->GetAttachment(m_iAttachment, matWorldToMeasure);
		}
		else
		{
			Warning("WARNING: %s requesting attachment point on non-animating entity %s!\n", GetDebugName(), m_hMeasureTarget->GetDebugName());
		}
		break;
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

	// Handle origin ignorance
	if (HasSpawnFlags( SF_LOGIC_MEASURE_MOVEMENT_USE_IGNORE_FLAGS_FOR_ORIGIN ))
	{
		// Get the position from the matrix's column directly and re-assign it
		Vector vecPosition;
		MatrixGetColumn( matMeasureToRef, 3, vecPosition );

		HandleIgnoreFlags( vecPosition.Base() );

		MatrixSetColumn( vecPosition, 3, matMeasureToRef );
	}

	ConcatTransforms( m_hTargetReference->EntityToWorldTransform(), matMeasureToRef, matNewTargetToWorld );

	MatrixAngles( matNewTargetToWorld, angAngles, vecOrigin );

	// If our spawnflags are greater than 0 (and don't just contain our default "TELEPORT" flag), we might need to ignore one of our angles.
	if (GetSpawnFlags() && GetSpawnFlags() != SF_LOGIC_MEASURE_MOVEMENT_TELEPORT && !HasSpawnFlags(SF_LOGIC_MEASURE_MOVEMENT_USE_IGNORE_FLAGS_FOR_ORIGIN))
	{
		HandleIgnoreFlags( angAngles.Base() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles logic_measure_movement's ignore flags on the specified Vector/QAngle
//-----------------------------------------------------------------------------
FORCEINLINE void CLogicMeasureMovement::HandleIgnoreFlags( float *vec )
{
	if (HasSpawnFlags( SF_LOGIC_MEASURE_MOVEMENT_IGNORE_X ))
		vec[0] = 0.0f;
	if (HasSpawnFlags( SF_LOGIC_MEASURE_MOVEMENT_IGNORE_Y ))
		vec[1] = 0.0f;
	if (HasSpawnFlags( SF_LOGIC_MEASURE_MOVEMENT_IGNORE_Z ))
		vec[2] = 0.0f;
}
#endif


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
#ifdef MAPBASE

// 
// Inputs work differently now so they could take !activator, etc.
// 

void CLogicMeasureMovement::InputSetMeasureTarget( inputdata_t &inputdata )
{
	m_strMeasureTarget = inputdata.value.StringID();
	m_hMeasureTarget = gEntList.FindEntityByName( NULL, STRING(m_strMeasureTarget), this, inputdata.pActivator, inputdata.pCaller );
	if ( !m_hMeasureTarget )
	{
		if ( Q_strnicmp( STRING(m_strMeasureTarget), "!player", 8 ) )
		{
			Warning( "%s: Unable to find measure target entity %s\n", GetDebugName(), STRING(m_strMeasureTarget) );
		}
	}

	m_iAttachment = 0;

	if (!m_hTarget)
		SetTarget( STRING(m_target) );
	if (!m_hTargetReference)
		SetTargetReference( STRING(m_strTargetReference) );
}

void CLogicMeasureMovement::InputSetMeasureReference( inputdata_t &inputdata )
{
	m_strMeasureReference = inputdata.value.StringID();
	m_hMeasureReference = gEntList.FindEntityByName( NULL, STRING(m_strMeasureReference), this, inputdata.pActivator, inputdata.pCaller );
	if ( !m_hMeasureReference )
	{
		Warning( "%s: Unable to find measure reference entity %s\n", GetDebugName(), STRING(m_strMeasureReference) );
	}
}

void CLogicMeasureMovement::InputSetTarget( inputdata_t &inputdata )
{
	m_target = inputdata.value.StringID();
	m_hTarget = gEntList.FindEntityByName( NULL, STRING(m_target), this, inputdata.pActivator, inputdata.pCaller );
	if ( !m_hTarget )
	{
		Warning( "%s: Unable to find movement target entity %s\n", GetDebugName(), STRING(m_target) );
	}
}

void CLogicMeasureMovement::InputSetTargetReference( inputdata_t &inputdata )
{
	m_strTargetReference = inputdata.value.StringID();
	m_hTargetReference = gEntList.FindEntityByName( NULL, STRING(m_strTargetReference), this, inputdata.pActivator, inputdata.pCaller );
	if ( !m_hTargetReference )
	{
		Warning( "%s: Unable to find movement reference entity %s\n", GetDebugName(), STRING(m_strTargetReference) );
	}
}

void CLogicMeasureMovement::InputSetMeasureAttachment( inputdata_t &inputdata )
{
	m_strAttachment = inputdata.value.StringID();
	m_iAttachment = 0;
}

// Just gets the position once and fires outputs without moving anything.
// We don't even need a target for this.
void CLogicMeasureMovement::InputGetPosition( inputdata_t &inputdata )
{
	if ( !m_hMeasureTarget.Get() || !m_hMeasureReference.Get() || !m_hTargetReference.Get() )
		return;

	Vector vecNewOrigin;
	QAngle vecNewAngles;
	DoMeasure(vecNewOrigin, vecNewAngles);

	// m_bOutputPosition has been repurposed here to toggle between using the target or the input activator as the activator.
	m_OutPosition.Set(vecNewOrigin, m_bOutputPosition ? m_hTarget.Get() : inputdata.pActivator, this);
	m_OutAngles.Set(Vector(vecNewAngles.x, vecNewAngles.y, vecNewAngles.z), m_bOutputPosition ? m_hTarget.Get() : inputdata.pActivator, this);
}
#else
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
#endif

void CLogicMeasureMovement::InputSetTargetScale( inputdata_t &inputdata )
{
	m_flScale = inputdata.value.Float();
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// This will measure the direction of a target entity and move
// another entity to where the target entity is facing.
// 
// m_hMeasureTarget;		// Whose direction is measured
// m_hMeasureReference;		// Position where direction is measured
// m_hTarget;				// Target whose origin is applied
// m_hTargetReference;		// From where the target's origin is applied
//-----------------------------------------------------------------------------
class CLogicMeasureDirection : public CLogicMeasureMovement
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CLogicMeasureDirection, CLogicMeasureMovement );

public:

	virtual void DoMeasure(Vector &vecOrigin, QAngle &angAngles);

	CBaseFilter *GetTraceFilter();
	//void InputSetTraceFilter( inputdata_t &inputdata ) { InputSetDamageFilter(inputdata); }

private:

	float m_flTraceDistance;
	int m_iMask;
	int m_iCollisionGroup;
	bool m_bHitIfPassed;
	//string_t m_iszTraceFilter;
	//CHandle<CBaseFilter*> m_hTraceFilter;

	bool m_bTraceTargetReference;

};


LINK_ENTITY_TO_CLASS( logic_measure_direction, CLogicMeasureDirection );


BEGIN_DATADESC( CLogicMeasureDirection )

	DEFINE_KEYFIELD( m_flTraceDistance, FIELD_FLOAT, "TraceDistance" ),
	DEFINE_KEYFIELD( m_iMask, FIELD_INTEGER, "Mask" ),
	DEFINE_KEYFIELD( m_iCollisionGroup, FIELD_INTEGER, "CollisionGroup" ),
	DEFINE_KEYFIELD( m_bHitIfPassed, FIELD_BOOLEAN, "HitIfPassed" ),
	DEFINE_KEYFIELD( m_bTraceTargetReference, FIELD_BOOLEAN, "TraceTargetReference" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetTraceFilter", InputSetDamageFilter ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Gets our "trace filter".
//-----------------------------------------------------------------------------
inline CBaseFilter *CLogicMeasureDirection::GetTraceFilter()
{
	return static_cast<CBaseFilter*>(m_hDamageFilter.Get()); // pranked
}

//-----------------------------------------------------------------------------
// Purpose: Does measure.
//-----------------------------------------------------------------------------
void CLogicMeasureDirection::DoMeasure( Vector &vecOrigin, QAngle &angAngles )
{
	trace_t tr;
	Vector vecStart, vecDir;
	QAngle angStart;
	switch( m_nMeasureType )
	{
	case MEASURE_POSITION:
		vecStart = m_hMeasureReference->GetAbsOrigin();
		angStart = m_hMeasureTarget->GetAbsAngles();
		break;

	case MEASURE_EYE_POSITION:
		vecStart = m_hMeasureReference->EyePosition();
		angStart = m_hMeasureTarget->EyeAngles();
		break;

	case MEASURE_ATTACHMENT:
		CBaseAnimating *pAnimating = m_hMeasureTarget->GetBaseAnimating();
		if (pAnimating)
		{
			if (m_iAttachment <= 0)
				m_iAttachment = m_hMeasureTarget->GetBaseAnimating()->LookupAttachment(STRING(m_strAttachment));

			if (m_iAttachment == -1)
				Warning("WARNING: %s requesting invalid attachment %s on %s!\n", GetDebugName(), STRING(m_strAttachment), m_hMeasureTarget->GetDebugName());
			else
			{
				pAnimating->GetAttachment(m_iAttachment, vecStart, angStart);
			}
		}
		else
		{
			Warning("WARNING: %s requesting attachment point on non-animating entity %s!\n", GetDebugName(), m_hMeasureTarget->GetDebugName());
		}
		break;
	}

	// If we have spawn flags, we might be supposed to ignore something
	if (GetSpawnFlags() > 0)
	{
		if (!HasSpawnFlags(SF_LOGIC_MEASURE_MOVEMENT_USE_IGNORE_FLAGS_FOR_ORIGIN))
			AngleVectors(angStart, &vecDir);

		HandleIgnoreFlags( angStart.Base() );

		if (HasSpawnFlags(SF_LOGIC_MEASURE_MOVEMENT_USE_IGNORE_FLAGS_FOR_ORIGIN))
			AngleVectors(angStart, &vecDir);
	}
	else
	{
		AngleVectors(angStart, &vecDir);
	}

	CTraceFilterEntityFilter traceFilter(m_hMeasureReference, m_iCollisionGroup);
	traceFilter.m_pFilter = GetTraceFilter();
	traceFilter.m_bHitIfPassed = m_bHitIfPassed;
	UTIL_TraceLine( vecStart, vecStart + vecDir * (m_flTraceDistance != 0 ? m_flTraceDistance : MAX_TRACE_LENGTH), m_iMask, &traceFilter, &tr ); //MASK_BLOCKLOS_AND_NPCS

	Vector vecEnd = tr.endpos;

	// Apply the scale factor
	float flScale = m_flScale;
	if ( ( flScale != 0.0f ) && ( flScale != 1.0f ) )
	{
		vecEnd = (vecStart + ((vecEnd - vecStart) / flScale));
	}

	Vector refPos = m_hTargetReference->GetAbsOrigin();
	Vector vecPos = refPos + (vecEnd - vecStart);

	if (m_bTraceTargetReference)
	{
		// Make sure we can go the whole distance there
		UTIL_TraceLine( refPos, vecPos, m_iMask, &traceFilter, &tr );
		vecPos = tr.endpos;
	}

	vecOrigin = vecPos;
	angAngles = angStart;
}



//-----------------------------------------------------------------------------
// The unused, "forgotten" entity brought back to life.
// Mirrors an entity's movement across a reference.
// It derives from logic_measure_movement now so it could use its features.
// This is unfinished and I'm still figuring out how it works.
// 
// m_hMeasureTarget;		// Whose position is mirrored (m_hRemoteTarget)
// m_hMeasureReference;		// Position where position is mirrored (m_hMirrorRelative)
// m_hTarget;				// Target whose origin is mirrored (m_hMovementTarget)
// m_hTargetReference;		// From where the target's origin is mirrored (m_hMirrorTarget)
//-----------------------------------------------------------------------------
class CLogicMirrorMovement : public CLogicMeasureMovement
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CLogicMirrorMovement, CLogicMeasureMovement );

public:
	virtual void DoMeasure(Vector &vecOrigin, QAngle &angAngles);
};


LINK_ENTITY_TO_CLASS( logic_mirror_movement, CLogicMirrorMovement );

BEGIN_DATADESC( CLogicMirrorMovement )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Does measure.
//-----------------------------------------------------------------------------
void CLogicMirrorMovement::DoMeasure( Vector &vecOrigin, QAngle &angAngles )
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

	case MEASURE_ATTACHMENT:
		if (CBaseAnimating *pAnimating = m_hMeasureTarget->GetBaseAnimating())
		{
			if (m_iAttachment <= 0)
				m_iAttachment = m_hMeasureTarget->GetBaseAnimating()->LookupAttachment(STRING(m_strAttachment));

			if (m_iAttachment == -1)
				Warning("WARNING: %s requesting invalid attachment %s on %s!\n", GetDebugName(), STRING(m_strAttachment), m_hMeasureTarget->GetDebugName());
			else
				pAnimating->GetAttachment(m_iAttachment, matWorldToMeasure);
		}
		else
		{
			Warning("WARNING: %s requesting attachment point on non-animating entity %s!\n", GetDebugName(), m_hMeasureTarget->GetDebugName());
		}
		break;
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

	MatrixScaleBy( -1.0f, matRefToMeasure );

	QAngle angRot;
	Vector vecPos;
	MatrixAngles( matRefToMeasure, angRot );
	MatrixPosition( matRefToMeasure, vecPos );
	angRot.z *= -1.0f;
	vecPos.z *= -1.0f;
	AngleMatrix( angRot, vecPos, matWorldToMeasure );

	// Now apply the new matrix to the new reference point
	matrix3x4_t matMeasureToRef, matNewTargetToWorld;
	MatrixInvert( matRefToMeasure, matMeasureToRef );

	// Handle origin ignorance
	if (HasSpawnFlags( SF_LOGIC_MEASURE_MOVEMENT_USE_IGNORE_FLAGS_FOR_ORIGIN ))
	{
		// Get the position from the matrix's column directly and re-assign it
		Vector vecPosition;
		MatrixGetColumn( matRefToMeasure, 3, vecPosition );

		HandleIgnoreFlags( vecPosition.Base() );

		MatrixSetColumn( vecPosition, 3, matRefToMeasure );
	}

	ConcatTransforms( m_hTargetReference->EntityToWorldTransform(), matMeasureToRef, matNewTargetToWorld );

	MatrixAngles( matNewTargetToWorld, angAngles, vecOrigin );

	// If our spawnflags are greater than 0 (and don't just contain our default "TELEPORT" flag), we might need to ignore one of our angles.
	if (GetSpawnFlags() && GetSpawnFlags() != SF_LOGIC_MEASURE_MOVEMENT_TELEPORT && !HasSpawnFlags(SF_LOGIC_MEASURE_MOVEMENT_USE_IGNORE_FLAGS_FOR_ORIGIN))
	{
		HandleIgnoreFlags( angAngles.Base() );
	}

	/*
	VMatrix matPortal1ToWorldInv, matPortal2ToWorld;
	MatrixInverseGeneral( m_hMeasureReference->EntityToWorldTransform(), matPortal1ToWorldInv );
	switch( m_nMeasureType )
	{
	case MEASURE_POSITION:
		matPortal2ToWorld = m_hMeasureTarget->EntityToWorldTransform();
		break;

	case MEASURE_EYE_POSITION:
		matPortal2ToWorld.SetupMatrixOrgAngles( m_hMeasureTarget->EyePosition(), m_hMeasureTarget->EyeAngles() );
		break;

	case MEASURE_ATTACHMENT:
		CBaseAnimating *pAnimating = m_hMeasureTarget->GetBaseAnimating();
		if (pAnimating)
		{
			if (m_iAttachment <= 0)
				m_iAttachment = m_hMeasureTarget->GetBaseAnimating()->LookupAttachment(STRING(m_strAttachment));

			if (m_iAttachment == -1)
				Warning("WARNING: %s requesting invalid attachment %s on %s!\n", GetDebugName(), STRING(m_strAttachment), m_hMeasureTarget->GetDebugName());
			else
			{
				pAnimating->GetAttachment( m_iAttachment, matPortal2ToWorld.As3x4() );
			}
		}
		else
		{
			Warning("WARNING: %s requesting attachment point on non-animating entity %s!\n", GetDebugName(), m_hMeasureTarget->GetDebugName());
		}
		break;
	}

	// If we have spawn flags, we might be supposed to ignore something
	if (GetSpawnFlags() > 0)
	{
		if (HasSpawnFlags( SF_LOGIC_MEASURE_MOVEMENT_USE_IGNORE_FLAGS_FOR_ORIGIN ))
		{
			// Get the position from the matrix's column directly and re-assign it
			Vector vecPosition;
			MatrixGetColumn( matPortal2ToWorld, 3, &vecPosition );

			HandleIgnoreFlags( vecPosition.Base() );

			MatrixSetColumn( matPortal2ToWorld, 3, vecPosition );
		}
		else
		{
			// Get the angles from the matrix and re-assign it
			QAngle angAngles;
			MatrixToAngles( matPortal2ToWorld, angAngles );

			HandleIgnoreFlags( angAngles.Base() );

			matPortal2ToWorld.SetupMatrixAngles( angAngles );
		}
	}

	// Apply the scale factor
	if ( ( m_flScale != 0.0f ) && ( m_flScale != 1.0f ) )
	{
		Vector vecTranslation;
		MatrixGetColumn( matPortal2ToWorld.As3x4(), 3, vecTranslation );
		vecTranslation /= m_flScale;
		MatrixSetColumn( vecTranslation, 3, matPortal2ToWorld.As3x4() );
	}

	// Get our scene camera's current orientation
	Vector ptCameraPosition, vCameraLook, vCameraRight, vCameraUp;
	ptCameraPosition = m_hTargetReference->EyePosition();
	m_hTargetReference->GetVectors( &vCameraLook, &vCameraRight, &vCameraUp );

	// map this position and orientation to the remote portal, mirrored (invert the result)
	Vector ptNewPosition, vNewLook;
	ptNewPosition = matPortal1ToWorldInv * ptCameraPosition;
	ptNewPosition = matPortal2ToWorld*(Vector( -ptNewPosition.x, -ptNewPosition.y, ptNewPosition.z ));

	vNewLook = matPortal1ToWorldInv.ApplyRotation( vCameraLook );
	vNewLook = matPortal2ToWorld.ApplyRotation( Vector( -vNewLook.x, -vNewLook.y, vNewLook.z ) );

	// Set the point camera to the new location/orientation
	QAngle qNewAngles;
	VectorAngles( vNewLook, qNewAngles );

	vecOrigin = ptNewPosition;
	angAngles = qNewAngles;
	*/
}
#endif
