//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Controls the pose parameters of a model
//
//===========================================================================//


#include "cbase.h"
#include "point_posecontroller.h"


#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// SERVER CLASS
//-----------------------------------------------------------------------------

#include "baseanimating.h"
#include "props.h"


#define MAX_POSE_INTERPOLATION_TIME 10.0f
#define MAX_POSE_CYCLE_FREQUENCY 10.0f
#define MAX_POSE_FMOD_RATE 10.0f
#define MAX_POSE_FMOD_AMPLITUDE 10.0f


LINK_ENTITY_TO_CLASS( point_posecontroller, CPoseController );	


BEGIN_DATADESC( CPoseController )
	DEFINE_AUTO_ARRAY( m_hProps, FIELD_EHANDLE ),
	DEFINE_AUTO_ARRAY( m_chPoseIndex, FIELD_CHARACTER ),
	DEFINE_FIELD( m_bDisablePropLookup, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPoseValueParity, FIELD_BOOLEAN ),
	// Keys
	DEFINE_KEYFIELD( m_iszPropName, FIELD_STRING, "PropName" ),
	DEFINE_KEYFIELD( m_iszPoseParameterName, FIELD_STRING, "PoseParameterName" ),
	DEFINE_KEYFIELD( m_fPoseValue, FIELD_FLOAT, "PoseValue" ),
	DEFINE_KEYFIELD( m_fInterpolationTime, FIELD_FLOAT, "InterpolationTime" ),
	DEFINE_KEYFIELD( m_bInterpolationWrap, FIELD_BOOLEAN, "InterpolationWrap" ),
	DEFINE_KEYFIELD( m_fCycleFrequency, FIELD_FLOAT, "CycleFrequency" ),
	DEFINE_KEYFIELD( m_nFModType, FIELD_INTEGER, "FModType" ),
	DEFINE_KEYFIELD( m_fFModTimeOffset, FIELD_FLOAT, "FModTimeOffset" ),
	DEFINE_KEYFIELD( m_fFModRate, FIELD_FLOAT, "FModRate" ),
	DEFINE_KEYFIELD( m_fFModAmplitude, FIELD_FLOAT, "FModAmplitude" ),
	// Functions
	DEFINE_FUNCTION( Think ),
	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING,	"SetPoseParameterName", InputSetPoseParameterName ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetPoseValue", InputSetPoseValue ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetInterpolationTime", InputSetInterpolationTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetCycleFrequency", InputSetCycleFrequency ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetFModType", InputSetFModType ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFModTimeOffset", InputSetFModTimeOffset ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFModRate", InputSetFModRate ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFModAmplitude", InputSetFModAmplitude ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "RandomizeFMod", InputRandomizeFMod ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GetFMod", InputGetFMod ),
END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CPoseController, DT_PoseController)
	SendPropArray3( SENDINFO_ARRAY3(m_hProps), SendPropEHandle( SENDINFO_ARRAY(m_hProps) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_chPoseIndex), SendPropInt( SENDINFO_ARRAY(m_chPoseIndex), 5, SPROP_UNSIGNED ) ),	// bits sent must be enough to represent MAXSTUDIOPOSEPARAM
	SendPropBool( SENDINFO(m_bPoseValueParity) ),
	SendPropFloat( SENDINFO(m_fPoseValue), 11, 0, 0.0f, 1.0f ),
	SendPropFloat( SENDINFO(m_fInterpolationTime), 11, 0, 0.0f, MAX_POSE_INTERPOLATION_TIME ),
	SendPropBool( SENDINFO(m_bInterpolationWrap) ),
	SendPropFloat( SENDINFO(m_fCycleFrequency), 11, 0, -MAX_POSE_CYCLE_FREQUENCY, MAX_POSE_CYCLE_FREQUENCY ),
	SendPropInt( SENDINFO(m_nFModType), 3, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_fFModTimeOffset), 11, 0, -1.0f, 1.0f ),
	SendPropFloat( SENDINFO(m_fFModRate), 11, 0, -MAX_POSE_FMOD_RATE, MAX_POSE_FMOD_RATE ),
	SendPropFloat( SENDINFO(m_fFModAmplitude), 11, 0, 0.0f, MAX_POSE_FMOD_AMPLITUDE ),
END_SEND_TABLE()


void CPoseController::Spawn( void )
{
	BaseClass::Spawn();

	// Talk to the client class when data changes
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	// Think to refresh the list of models
	SetThink( &CPoseController::Think );
	SetNextThink( gpGlobals->curtime + 1.0 );
}

void CPoseController::Think( void )
{
	if ( !m_bDisablePropLookup )
	{
		// Refresh the list of models
		BuildPropList();

		SetCurrentPose( m_fPoseValue );

		m_bDisablePropLookup = true;

		SetNextThink( gpGlobals->curtime + 1.0 );
	}
}

void CPoseController::BuildPropList( void )
{
	int iPropNum = 0;
	CBaseEntity *pEnt = gEntList.FindEntityByName( NULL, m_iszPropName );

	while ( pEnt && iPropNum < MAX_POSE_CONTROLLED_PROPS )
	{
		CBaseAnimating *pProp = dynamic_cast<CBaseAnimating*>( pEnt );
		if ( pProp )
		{
			CDynamicProp *pDynamicProp = dynamic_cast<CDynamicProp*>( pProp );
			if ( pDynamicProp )
				pDynamicProp->PropSetSequence( 0 );

			if ( m_hProps[ iPropNum ] != pProp )
			{
				// Only set new handles (to avoid network spam)
				m_hProps.Set( iPropNum, pProp );	
			}

			// Update the pose parameter index
			SetPoseIndex( iPropNum, pProp->LookupPoseParameter( m_iszPoseParameterName.ToCStr() ) );
			
			++iPropNum;
		}

		// Get the next entity with specified targetname
		pEnt = gEntList.FindEntityByName( pEnt, m_iszPropName );
	}

	// Nullify the remaining handles
	while ( iPropNum < MAX_POSE_CONTROLLED_PROPS )
	{
		if ( m_hProps[ iPropNum ] != NULL )
			m_hProps.Set( iPropNum, NULL );

		++iPropNum;
	}

	SetNextThink( gpGlobals->curtime + 1.0 );
}

void CPoseController::BuildPoseIndexList( void )
{
	for ( int iPropNum = 0; iPropNum < MAX_POSE_CONTROLLED_PROPS; ++iPropNum )
	{
		CBaseAnimating *pProp = dynamic_cast<CBaseAnimating*>( m_hProps[ iPropNum ].Get() );

		if ( pProp )
		{
			// Update the pose parameter index
			SetPoseIndex( iPropNum, pProp->LookupPoseParameter( m_iszPoseParameterName.ToCStr() ) );
		}
	}
}

void CPoseController::SetPoseIndex( int i, int iValue )
{
	if ( iValue == -1 )
	{
		// Using this as invalid lets us network less bits
		iValue = MAXSTUDIOPOSEPARAM;
	}

	if ( m_chPoseIndex[ i ] != iValue )
	{
		// Only set a new index (to avoid network spam)
		m_chPoseIndex.Set( i, iValue );
	}
}

float CPoseController::GetPoseValue( void )
{
	return m_fPoseValue;
}

void CPoseController::SetProp( CBaseAnimating *pProp )
{
	// Control a prop directly by pointer
	if ( m_hProps[ 0 ] != pProp )
	{
		// Only set new handles (to avoid network spam)
		m_hProps.Set( 0, pProp );
	}

	// Update the pose parameter index
	SetPoseIndex( 0, pProp->LookupPoseParameter( m_iszPoseParameterName.ToCStr() ) );

	// Nullify the remaining handles
	for ( int iPropNum = 1; iPropNum < MAX_POSE_CONTROLLED_PROPS; ++iPropNum )
	{
		if ( m_hProps[ iPropNum ] != NULL )
			m_hProps.Set( iPropNum, NULL );
	}

	m_bDisablePropLookup = false;
}

void CPoseController::SetPropName( const char *pName )
{
	m_iszPropName = MAKE_STRING( pName );

	BuildPropList();
}

void CPoseController::SetPoseParameterName( const char *pName )
{
	m_iszPoseParameterName = MAKE_STRING( pName );

	BuildPoseIndexList();
}

void CPoseController::SetPoseValue( float fValue )
{
	m_fPoseValue = clamp( fValue, 0.0f, 1.0f );

	// Force the client to set the current pose
	m_bPoseValueParity = !m_bPoseValueParity;

	SetCurrentPose( m_fPoseValue );
}

void CPoseController::SetInterpolationTime( float fValue )
{
	m_fInterpolationTime = clamp( fValue, 0.0f, MAX_POSE_INTERPOLATION_TIME );
}

void CPoseController::SetInterpolationWrap( bool bWrap )
{
	m_bInterpolationWrap = bWrap;
}

void CPoseController::SetCycleFrequency( float fValue )
{
	m_fCycleFrequency = clamp( fValue, -MAX_POSE_CYCLE_FREQUENCY, MAX_POSE_CYCLE_FREQUENCY );
}

void CPoseController::SetFModType( int nType )
{
	if ( nType < 0 || nType >= POSECONTROLLER_FMODTYPE_TOTAL )
		return;

	m_nFModType = static_cast<PoseController_FModType_t>(nType);
}

void CPoseController::SetFModTimeOffset( float fValue )
{
	m_fFModTimeOffset = clamp( fValue, -1.0f, 1.0f );
}

void CPoseController::SetFModRate( float fValue )
{
	m_fFModRate = clamp( fValue, -MAX_POSE_FMOD_RATE, MAX_POSE_FMOD_RATE );
}

void CPoseController::SetFModAmplitude( float fValue )
{
	m_fFModAmplitude = clamp( fValue, 0.0f, MAX_POSE_FMOD_AMPLITUDE );
}

void CPoseController::RandomizeFMod( float fExtremeness )
{
	fExtremeness = clamp( fExtremeness, 0.0f, 1.0f );

	SetFModType( RandomInt( 1, POSECONTROLLER_FMODTYPE_TOTAL - 1 ) );
	SetFModTimeOffset( RandomFloat( -1.0, 1.0f ) );
	SetFModRate( RandomFloat( fExtremeness * -MAX_POSE_FMOD_RATE, fExtremeness * MAX_POSE_FMOD_RATE ) );
	SetFModAmplitude( RandomFloat( 0.0f, fExtremeness * MAX_POSE_FMOD_AMPLITUDE ) );
}

void CPoseController::InputSetPoseParameterName( inputdata_t &inputdata )
{
	SetPoseParameterName( inputdata.value.String() );
}

void CPoseController::InputSetPoseValue( inputdata_t &inputdata )
{
	SetPoseValue( inputdata.value.Float() );
}

void CPoseController::InputSetInterpolationTime( inputdata_t &inputdata )
{
	SetInterpolationTime( inputdata.value.Float() );
}

void CPoseController::InputSetCycleFrequency( inputdata_t &inputdata )
{
	SetCycleFrequency( inputdata.value.Float() );
}

void CPoseController::InputSetFModType( inputdata_t &inputdata )
{
	SetFModType( inputdata.value.Int() );
}

void CPoseController::InputSetFModTimeOffset( inputdata_t &inputdata )
{
	SetFModTimeOffset( inputdata.value.Float() );
}

void CPoseController::InputSetFModRate( inputdata_t &inputdata )
{
	SetFModRate( inputdata.value.Float() );
}

void CPoseController::InputSetFModAmplitude( inputdata_t &inputdata )
{
	SetFModAmplitude( inputdata.value.Float() );
}

void CPoseController::InputRandomizeFMod( inputdata_t &inputdata )
{
	RandomizeFMod( inputdata.value.Float() );
}

void CPoseController::InputGetFMod( inputdata_t &inputdata )
{
	DevMsg( "FMod values for pose controller %s\nTYPE: %i\nTIME OFFSET: %f\nRATE: %f\nAMPLITUDE: %f\n", 
			STRING( GetEntityName() ), 
			m_nFModType.Get(), 
			m_fFModTimeOffset.Get(), 
			m_fFModRate.Get(), 
			m_fFModAmplitude.Get() );
}


#else //#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// CLIENT CLASS
//-----------------------------------------------------------------------------


IMPLEMENT_CLIENTCLASS_DT( C_PoseController, DT_PoseController, CPoseController )
	RecvPropArray3( RECVINFO_ARRAY(m_hProps), RecvPropEHandle( RECVINFO(m_hProps[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_chPoseIndex), RecvPropInt( RECVINFO(m_chPoseIndex[0]) ) ),
	RecvPropBool( RECVINFO(m_bPoseValueParity) ),
	RecvPropFloat( RECVINFO(m_fPoseValue) ),
	RecvPropFloat( RECVINFO(m_fInterpolationTime) ),
	RecvPropBool( RECVINFO(m_bInterpolationWrap) ),
	RecvPropFloat( RECVINFO(m_fCycleFrequency) ),
	RecvPropInt( RECVINFO(m_nFModType) ),
	RecvPropFloat( RECVINFO(m_fFModTimeOffset) ),
	RecvPropFloat( RECVINFO(m_fFModRate) ),
	RecvPropFloat( RECVINFO(m_fFModAmplitude) ),
END_RECV_TABLE()


void C_PoseController::Spawn( void )
{
	SetThink( &C_PoseController::ClientThink );
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	m_fCurrentFMod = 0.0f;
	m_PoseTransitionValue.Init( 0.0f, 0.0f, 0.0f );

	BaseClass::Spawn();
}

void C_PoseController::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Start thinking (Baseclass stops it)
		SetNextClientThink( CLIENT_THINK_ALWAYS );

		m_bOldPoseValueParity = m_bPoseValueParity;
		m_fCurrentPoseValue = m_fPoseValue;

		SetCurrentPose( m_fCurrentPoseValue );
	}

	if ( m_bOldPoseValueParity != m_bPoseValueParity )
	{
		// If the pose value was set directly set the actual pose value
		float fClientPoseValue = m_fCurrentPoseValue + m_PoseTransitionValue.Interp( gpGlobals->curtime );

		if ( fClientPoseValue < 0.0f )
			fClientPoseValue += 1.0f;
		else if ( fClientPoseValue > 1.0f )
			fClientPoseValue -= 1.0f;

		float fInterpForward = fClientPoseValue - m_fPoseValue;

		if ( m_bInterpolationWrap )
		{
			float fInterpBackward = ( fClientPoseValue + ( ( fClientPoseValue < 0.5f ) ? ( 1.0f ) : ( -1.0f ) ) ) - m_fPoseValue;

			m_PoseTransitionValue.Init( ( ( fabsf( fInterpForward ) < fabsf( fInterpBackward ) ) ? ( fInterpForward ) : ( fInterpBackward ) ), 0.0f, m_fInterpolationTime );
		}
		else
		{
			m_PoseTransitionValue.Init( fInterpForward, 0.0f, m_fInterpolationTime );
		}

		m_bOldPoseValueParity = m_bPoseValueParity;
		m_fCurrentPoseValue = m_fPoseValue;
	}
}

void C_PoseController::ClientThink( void )
{
	UpdateModulation();

	UpdatePoseCycle( m_fCycleFrequency + m_fCurrentFMod );
}

void C_PoseController::UpdateModulation( void )
{
	switch ( m_nFModType )
	{
	case POSECONTROLLER_FMODTYPE_NONE:
		{
			// No modulation
			m_fCurrentFMod = 0.0f;
			break;
		}

	case POSECONTROLLER_FMODTYPE_SINE:
		{
			float fCycleTime = m_fFModRate * ( gpGlobals->curtime + m_fFModTimeOffset );

			m_fCurrentFMod = m_fFModAmplitude * sinf( fCycleTime * ( 2.0f * M_PI ) );
			break;
		}

	case POSECONTROLLER_FMODTYPE_SQUARE:
		{
			float fCycleTime = fabsf( m_fFModRate * 2.0f * ( gpGlobals->curtime + m_fFModTimeOffset ) );

			// Separate the current time into integer and decimal
			int iIntegerPortion = static_cast<int>( fCycleTime );

			// Find if it's going up or down
			if ( ( iIntegerPortion % 2 ) == 0 )
				m_fCurrentFMod = m_fFModAmplitude;
			else
				m_fCurrentFMod = -m_fFModAmplitude;

			break;
		}

	case POSECONTROLLER_FMODTYPE_TRIANGLE:
		{
			float fCycleTime = fabsf( m_fFModRate * 4.0f * ( gpGlobals->curtime + m_fFModTimeOffset ) );

			// Separate the current time into integer and decimal
			int iIntegerPortion = static_cast<int>( fCycleTime );
			float fDecimalPortion = fCycleTime - static_cast<float>( iIntegerPortion );

			// Find if it's going up from 0, down from 1, down from 0, or up from -1
			switch ( iIntegerPortion % 4 )
			{
			case 0:
				m_fCurrentFMod = fDecimalPortion * m_fFModAmplitude;
				break;
			case 1:
				m_fCurrentFMod = ( 1.0f - fDecimalPortion ) * m_fFModAmplitude;
				break;
			case 2:
				m_fCurrentFMod = -fDecimalPortion * m_fFModAmplitude;
				break;
			case 3:
				m_fCurrentFMod = ( -1.0f + fDecimalPortion ) * m_fFModAmplitude;
				break;
			}

			break;
		}

	case POSECONTROLLER_FMODTYPE_SAWTOOTH:
		{
			float fCycleTime = fabsf( m_fFModRate * 2.0f * ( gpGlobals->curtime + m_fFModTimeOffset ) );

			// Separate the current time into integer and decimal
			int iIntegerPortion = static_cast<int>( fCycleTime );
			float fDecimalPortion = fCycleTime - static_cast<float>( iIntegerPortion );

			// Find if it's going up from 0 or up from -1
			if ( ( iIntegerPortion % 2 ) == 0 )
				m_fCurrentFMod = fDecimalPortion * m_fFModAmplitude;
			else
				m_fCurrentFMod = ( -1.0f + fDecimalPortion ) * m_fFModAmplitude;

			break;
		}

	case POSECONTROLLER_FMODTYPE_NOISE:
		{
			// Randomly increase or decrease by the rate
			if ( RandomInt( 0, 1 ) == 0 )
				m_fCurrentFMod += m_fFModRate * gpGlobals->frametime;
			else
				m_fCurrentFMod -= m_fFModRate * gpGlobals->frametime;

			m_fCurrentFMod = clamp( m_fCurrentFMod, -m_fFModAmplitude, m_fFModAmplitude );

			break;
		}
	}
}

void C_PoseController::UpdatePoseCycle( float fCycleAmount )
{
	m_fCurrentPoseValue += fCycleAmount * gpGlobals->frametime;

	float fNewPoseValue = m_fCurrentPoseValue + m_PoseTransitionValue.Interp( gpGlobals->curtime );

	if ( fNewPoseValue < 0.0f )
		fNewPoseValue += 1.0f;
	else if ( fNewPoseValue > 1.0f )
		fNewPoseValue -= 1.0f;

	SetCurrentPose( fNewPoseValue );
}

#define CPoseController C_PoseController
#define CBaseAnimating C_BaseAnimating


#endif //#ifndef CLIENT_DLL


void CPoseController::SetCurrentPose( float fCurrentPoseValue )
{
	for ( int iPropNum = 0; iPropNum < MAX_POSE_CONTROLLED_PROPS; ++iPropNum )
	{
		// Control each model's pose parameter
		CBaseAnimating *pProp = dynamic_cast<CBaseAnimating*>( m_hProps[ iPropNum ].Get() );

		if ( pProp )
		{
			float fPoseValueMin;
			float fPoseValueMax;

			// Map to the pose parameter's range
			pProp->GetPoseParameterRange( m_chPoseIndex[ iPropNum ], fPoseValueMin, fPoseValueMax );
			pProp->SetPoseParameter( m_chPoseIndex[ iPropNum ], fPoseValueMin + fCurrentPoseValue * ( fPoseValueMax - fPoseValueMin ) );
		}
	}
}
