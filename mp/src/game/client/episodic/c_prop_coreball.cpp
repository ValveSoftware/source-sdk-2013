//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"

class C_PropCoreBall : public C_BaseAnimating
{
	DECLARE_CLASS( C_PropCoreBall, C_BaseAnimating );
	DECLARE_CLIENTCLASS();
	DECLARE_DATADESC();

public:

	C_PropCoreBall();

	void ApplyBoneMatrixTransform( matrix3x4_t& transform );

	float m_flScaleX;
	float m_flScaleY;
	float m_flScaleZ;

	float m_flLerpTimeX;
	float m_flLerpTimeY;
	float m_flLerpTimeZ;

	float m_flGoalTimeX;
	float m_flGoalTimeY;
	float m_flGoalTimeZ;

	float m_flCurrentScale[3];
	bool  m_bRunningScale[3];
	float m_flTargetScale[3];
	
private:

};

void RecvProxy_ScaleX( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_PropCoreBall *pCoreData = (C_PropCoreBall *) pStruct;

	pCoreData->m_flScaleX = pData->m_Value.m_Float;
	
	if ( pCoreData->m_bRunningScale[0] == true )
	{
		pCoreData->m_flTargetScale[0] = pCoreData->m_flCurrentScale[0];
	}
}

void RecvProxy_ScaleY( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_PropCoreBall *pCoreData = (C_PropCoreBall *) pStruct;

	pCoreData->m_flScaleY = pData->m_Value.m_Float;

	if ( pCoreData->m_bRunningScale[1] == true )
	{
		pCoreData->m_flTargetScale[1] = pCoreData->m_flCurrentScale[1];
	}
}

void RecvProxy_ScaleZ( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_PropCoreBall *pCoreData = (C_PropCoreBall *) pStruct;

	pCoreData->m_flScaleZ = pData->m_Value.m_Float;

	if ( pCoreData->m_bRunningScale[2] == true )
	{
		pCoreData->m_flTargetScale[2] = pCoreData->m_flCurrentScale[2];
	}
}

IMPLEMENT_CLIENTCLASS_DT( C_PropCoreBall, DT_PropCoreBall, CPropCoreBall )
	RecvPropFloat( RECVINFO( m_flScaleX ), 0, RecvProxy_ScaleX ),
	RecvPropFloat( RECVINFO( m_flScaleY ), 0, RecvProxy_ScaleY ),
	RecvPropFloat( RECVINFO( m_flScaleZ ), 0, RecvProxy_ScaleZ ),

	RecvPropFloat( RECVINFO( m_flLerpTimeX ) ),
	RecvPropFloat( RECVINFO( m_flLerpTimeY ) ),
	RecvPropFloat( RECVINFO( m_flLerpTimeZ ) ),

	RecvPropFloat( RECVINFO( m_flGoalTimeX ) ),
	RecvPropFloat( RECVINFO( m_flGoalTimeY ) ),
	RecvPropFloat( RECVINFO( m_flGoalTimeZ ) ),
END_RECV_TABLE()


BEGIN_DATADESC( C_PropCoreBall )
	DEFINE_AUTO_ARRAY( m_flTargetScale, FIELD_FLOAT ),
	DEFINE_AUTO_ARRAY( m_bRunningScale, FIELD_BOOLEAN ),
END_DATADESC()

C_PropCoreBall::C_PropCoreBall( void )
{
	m_flTargetScale[0] = 1.0f;
	m_flTargetScale[1] = 1.0f;
	m_flTargetScale[2] = 1.0f;

	m_bRunningScale[0] = false;
	m_bRunningScale[1] = false;
	m_bRunningScale[2] = false;
}

void C_PropCoreBall::ApplyBoneMatrixTransform( matrix3x4_t& transform )
{
	BaseClass::ApplyBoneMatrixTransform( transform );

	float flVal[3] = { m_flTargetScale[0], m_flTargetScale[1], m_flTargetScale[2] };
	float *flTargetScale[3] = { &m_flTargetScale[0], &m_flTargetScale[1], &m_flTargetScale[2] };
	float flScale[3] = { m_flScaleX, m_flScaleY, m_flScaleZ };
	float flLerpTime[3] = { m_flLerpTimeX, m_flLerpTimeY, m_flLerpTimeZ };
	float flGoalTime[3] = { m_flGoalTimeX, m_flGoalTimeY, m_flGoalTimeZ };
	bool *bRunning[3] = { &m_bRunningScale[0], &m_bRunningScale[1], &m_bRunningScale[2] };
		
	for ( int i = 0; i < 3; i++ )
	{
		if ( *flTargetScale[i] != flScale[i] )
		{
			float deltaTime = (float)( gpGlobals->curtime - flGoalTime[i]) / flLerpTime[i];
			float flRemapVal = SimpleSplineRemapVal( deltaTime, 0.0f, 1.0f, *flTargetScale[i], flScale[i] );

			*bRunning[i] = true;

			if ( deltaTime >= 1.0f )
			{
				*flTargetScale[i] = flScale[i];
				*bRunning[i] = false;
			}

			flVal[i] = flRemapVal;
			m_flCurrentScale[i] = flVal[i];
		}
	}

	VectorScale( transform[0], flVal[0], transform[0] );
	VectorScale( transform[1], flVal[1], transform[1] );
	VectorScale( transform[2], flVal[2], transform[2] );
}