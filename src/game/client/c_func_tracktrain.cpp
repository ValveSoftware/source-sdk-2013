//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_baseentity.h"
#include "soundinfo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// An entity which emits other entities at points 
//-----------------------------------------------------------------------------
class C_FuncTrackTrain : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_FuncTrackTrain, C_BaseEntity );
	DECLARE_CLIENTCLASS();

public:
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual bool GetSoundSpatialization( SpatializationInfo_t& info );

	virtual bool IsBaseTrain( void ) const { return true; }


private:
	int m_nLongAxis;
	float m_flRadius;
	float m_flLineLength;
};


//-----------------------------------------------------------------------------
// Datatable
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_FuncTrackTrain, DT_FuncTrackTrain, CFuncTrackTrain )
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Sound spatialization
//-----------------------------------------------------------------------------
void C_FuncTrackTrain::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if (updateType == DATA_UPDATE_CREATED)
	{
		// Compute the cross-sectional area and dimension and length of the line segment
		int nIndex1, nIndex2;
		const Vector &vecOBBSize = CollisionProp()->OBBSize();
		if ( ( vecOBBSize.x > vecOBBSize.y ) && ( vecOBBSize.x > vecOBBSize.z ) )
		{
			m_nLongAxis = 0;
			nIndex1 = 1; nIndex2 = 2;
		}
		else if ( vecOBBSize.y > vecOBBSize.z )
		{
			m_nLongAxis = 1;
			nIndex1 = 0; nIndex2 = 2;
		}
		else
		{
			m_nLongAxis = 2;
			nIndex1 = 0; nIndex2 = 1;
		}

		m_flRadius = sqrt( vecOBBSize[nIndex1] * vecOBBSize[nIndex1] + vecOBBSize[nIndex2] * vecOBBSize[nIndex2] ) * 0.5f;
		m_flLineLength = vecOBBSize[m_nLongAxis];
	}
}


//-----------------------------------------------------------------------------
// Sound spatialization
//-----------------------------------------------------------------------------
bool C_FuncTrackTrain::GetSoundSpatialization( SpatializationInfo_t& info )
{
	// Out of PVS
	if ( IsDormant() )
		return false;
	
	if ( info.pflRadius )
	{
		*info.pflRadius = m_flRadius;
	}
	
	if ( info.pOrigin )
	{
		Vector vecStart, vecEnd, vecWorldDir;
		Vector vecDir = vec3_origin;
		vecDir[m_nLongAxis] = 1.0f;
		VectorRotate( vecDir, EntityToWorldTransform(), vecWorldDir );
		VectorMA( WorldSpaceCenter(), -0.5f * m_flLineLength, vecWorldDir, vecStart );
		VectorMA( vecStart, m_flLineLength, vecWorldDir, vecEnd );

		float t;
		CalcClosestPointOnLine( info.info.vListenerOrigin, vecStart, vecEnd, *info.pOrigin, &t ); 
		if ( t < 0.0f )
		{
			*info.pOrigin = vecStart;
		}
		else if ( t > 1.0f )
		{
			*info.pOrigin = vecEnd;
		}
	}

	if ( info.pAngles )
	{
		VectorCopy( CollisionProp()->GetCollisionAngles(), *info.pAngles );
	}

	return true;
}


