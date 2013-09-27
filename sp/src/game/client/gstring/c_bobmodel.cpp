
#include "cbase.h"
#include "c_bobmodel.h"


C_BobModel::C_BobModel()
	: m_bDirty( false )
	, m_angIdle( vec3_angle )
	, m_posIdle( vec3_origin )
	, m_bInvalid( false )
{
}

CStudioHdr *C_BobModel::OnNewModel()
{
	m_bDirty = true;

	return BaseClass::OnNewModel();
}

bool C_BobModel::IsDirty() const
{
	return m_bDirty;
}

void C_BobModel::SetDirty( bool bDirty )
{
	m_bDirty = bDirty;
}

bool C_BobModel::IsInvalid() const
{
	return m_bInvalid;
}

void C_BobModel::UpdateDefaultTransforms()
{
	int iSequence = SelectWeightedSequence( ACT_VM_IDLE );

	if ( iSequence < 0 )
	{
		m_angIdle = vec3_angle;
		m_posIdle = vec3_origin;

		m_bInvalid = true;
		return;
	}

	SetSequence( iSequence );
	SetCycle( 0.0f );

	Vector vecPos;
	QAngle ang;

	if ( GetAttachment( 1, vecPos, ang ) )
	{
		m_posIdle = vecPos;
		m_angIdle = ang;
		m_bInvalid = false;
	}
	else
	{
		m_bInvalid = true;
	}
}

void C_BobModel::GetDeltaTransforms( QAngle &angDelta )
{
	Assert( !m_bInvalid );

	Vector pos;

	GetAttachment( 1, pos, angDelta );

	for ( int i = 0; i < 3; i++ )
	{
		angDelta[ i ] = AngleDiff( angDelta[ i ], m_angIdle[ i ] );
	}
}