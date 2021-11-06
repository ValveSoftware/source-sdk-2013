//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Color correction entity.
//
 // $NoKeywords: $
//===========================================================================//
#include "cbase.h"

#include "filesystem.h"
#include "cdll_client_int.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "colorcorrectionmgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//------------------------------------------------------------------------------
// FIXME: This really should inherit from something	more lightweight
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Purpose : Shadow control entity
//------------------------------------------------------------------------------
class C_ColorCorrectionVolume : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_ColorCorrectionVolume, C_BaseEntity );

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	C_ColorCorrectionVolume();
	virtual ~C_ColorCorrectionVolume();

	void OnDataChanged(DataUpdateType_t updateType);
	bool ShouldDraw();

#ifdef MAPBASE // From Alien Swarm SDK
	void Update( C_BasePlayer *pPlayer, float ccScale );

	void StartTouch( C_BaseEntity *pOther );
	void EndTouch( C_BaseEntity *pOther );
#else
	void ClientThink();
#endif

private:
#ifdef MAPBASE // From Alien Swarm SDK
	float	m_LastEnterWeight;
	float	m_LastEnterTime;

	float	m_LastExitWeight;
	float	m_LastExitTime;
	bool	m_bEnabled;
	float	m_MaxWeight;
	float	m_FadeDuration;
#endif
	float	m_Weight;
	char	m_lookupFilename[MAX_PATH];

	ClientCCHandle_t m_CCHandle;
};

IMPLEMENT_CLIENTCLASS_DT(C_ColorCorrectionVolume, DT_ColorCorrectionVolume, CColorCorrectionVolume)
#ifdef MAPBASE // From Alien Swarm SDK
	RecvPropBool( RECVINFO( m_bEnabled ) ),
	RecvPropFloat( RECVINFO( m_MaxWeight ) ),
	RecvPropFloat( RECVINFO( m_FadeDuration ) ),
#endif
	RecvPropFloat( RECVINFO(m_Weight) ),
	RecvPropString( RECVINFO(m_lookupFilename) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_ColorCorrectionVolume )
	DEFINE_PRED_FIELD( m_Weight, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()


//------------------------------------------------------------------------------
// Constructor, destructor
//------------------------------------------------------------------------------
C_ColorCorrectionVolume::C_ColorCorrectionVolume()
{
	m_CCHandle = INVALID_CLIENT_CCHANDLE;
}

C_ColorCorrectionVolume::~C_ColorCorrectionVolume()
{
	g_pColorCorrectionMgr->RemoveColorCorrection( m_CCHandle );
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_ColorCorrectionVolume::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( m_CCHandle == INVALID_CLIENT_CCHANDLE )
		{
#ifdef MAPBASE // From Alien Swarm SDK
			// forming a unique name without extension
			char cleanName[MAX_PATH];
			V_StripExtension( m_lookupFilename, cleanName, sizeof( cleanName ) );
			char name[MAX_PATH];
			Q_snprintf( name, MAX_PATH, "%s_%d", cleanName, entindex() );

			m_CCHandle = g_pColorCorrectionMgr->AddColorCorrectionVolume( this, name, m_lookupFilename );
#else
			char filename[MAX_PATH];
			Q_strncpy( filename, m_lookupFilename, MAX_PATH );

			m_CCHandle = g_pColorCorrectionMgr->AddColorCorrection( filename );
			SetNextClientThink( ( m_CCHandle != INVALID_CLIENT_CCHANDLE ) ? CLIENT_THINK_ALWAYS : CLIENT_THINK_NEVER );
#endif
		}
	}
}

//------------------------------------------------------------------------------
// We don't draw...
//------------------------------------------------------------------------------
bool C_ColorCorrectionVolume::ShouldDraw()
{
	return false;
}

#ifdef MAPBASE // From Alien Swarm SDK
//--------------------------------------------------------------------------------------------------------
void C_ColorCorrectionVolume::StartTouch( CBaseEntity *pEntity )
{
	m_LastEnterTime = gpGlobals->curtime;
	m_LastEnterWeight = m_Weight;
}


//--------------------------------------------------------------------------------------------------------
void C_ColorCorrectionVolume::EndTouch( CBaseEntity *pEntity )
{
	m_LastExitTime = gpGlobals->curtime;
	m_LastExitWeight = m_Weight;
}


void C_ColorCorrectionVolume::Update( C_BasePlayer *pPlayer, float ccScale )
{
	if ( pPlayer )
	{
		bool isTouching = CollisionProp()->IsPointInBounds( pPlayer->EyePosition() );
		bool wasTouching = m_LastEnterTime > m_LastExitTime;

		if ( isTouching && !wasTouching )
		{
			StartTouch( pPlayer );
		}
		else if ( !isTouching && wasTouching )
		{
			EndTouch( pPlayer );
		}
	}

	if( !m_bEnabled )
	{
		m_Weight = 0.0f;
	}
	else
	{
		if( m_LastEnterTime > m_LastExitTime )
		{
			// we most recently entered the volume

			if( m_Weight < 1.0f )
			{
				float dt = gpGlobals->curtime - m_LastEnterTime;
				float weight = m_LastEnterWeight + dt / ((1.0f-m_LastEnterWeight)*m_FadeDuration);
				if( weight>1.0f )
					weight = 1.0f;

				m_Weight = weight;
			}
		}
		else
		{
			// we most recently exitted the volume

			if( m_Weight > 0.0f )
			{
				float dt = gpGlobals->curtime - m_LastExitTime;
				float weight = (1.0f-m_LastExitWeight) + dt / (m_LastExitWeight*m_FadeDuration);
				if( weight>1.0f )
					weight = 1.0f;

				m_Weight = 1.0f - weight;
			}
		}
	}

	//	Vector entityPosition = GetAbsOrigin();
	g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, m_Weight * ccScale );
}


void UpdateColorCorrectionVolumes( C_BasePlayer *pPlayer, float ccScale, C_ColorCorrectionVolume **pList, int listCount )
{
	for ( int i = 0; i < listCount; i++ )
	{
		pList[i]->Update(pPlayer, ccScale);
	}
}
#else
void C_ColorCorrectionVolume::ClientThink()
{
	Vector entityPosition = GetAbsOrigin();
	g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, m_Weight );
}
#endif













