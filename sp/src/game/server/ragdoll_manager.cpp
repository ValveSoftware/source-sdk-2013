//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "sendproxy.h"
#include "ragdoll_shared.h"
#include "ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CRagdollManager : public CBaseEntity
{
public:
	DECLARE_CLASS( CRagdollManager, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CRagdollManager();

	virtual void	Activate();
	virtual int		UpdateTransmitState();

	void InputSetMaxRagdollCount(inputdata_t &data);
	void InputSetMaxRagdollCountDX8(inputdata_t &data);

	int DrawDebugTextOverlays(void);

public:

	void UpdateCurrentMaxRagDollCount();

	CNetworkVar( int,  m_iCurrentMaxRagdollCount );

	int m_iDXLevel;
	int m_iMaxRagdollCount;
	int m_iMaxRagdollCountDX8;

	bool m_bSaveImportant;
};


IMPLEMENT_SERVERCLASS_ST_NOBASE( CRagdollManager, DT_RagdollManager )
	SendPropInt( SENDINFO( m_iCurrentMaxRagdollCount ), 6 ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( game_ragdoll_manager, CRagdollManager );

BEGIN_DATADESC( CRagdollManager )

	//DEFINE_FIELD( m_iDXLevel, FIELD_INTEGER ),

	DEFINE_FIELD( m_iCurrentMaxRagdollCount, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_iMaxRagdollCount, FIELD_INTEGER,	"MaxRagdollCount" ),
	DEFINE_KEYFIELD( m_iMaxRagdollCountDX8, FIELD_INTEGER,	"MaxRagdollCountDX8" ),

	DEFINE_KEYFIELD( m_bSaveImportant, FIELD_BOOLEAN, "SaveImportant" ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxRagdollCount",  InputSetMaxRagdollCount ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxRagdollCountDX8",  InputSetMaxRagdollCountDX8 ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Constructor 
//-----------------------------------------------------------------------------
CRagdollManager::CRagdollManager( void )
{
	m_iMaxRagdollCount = -1;
	m_iMaxRagdollCountDX8 = -1;
	m_iCurrentMaxRagdollCount = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInfo - 
// Output : int
//-----------------------------------------------------------------------------
int CRagdollManager::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRagdollManager::Activate()
{
	BaseClass::Activate();

	// Cache off the DX level for use later.
	ConVarRef mat_dxlevel( "mat_dxlevel" );
	m_iDXLevel = mat_dxlevel.GetInt();
	
	UpdateCurrentMaxRagDollCount();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CRagdollManager::UpdateCurrentMaxRagDollCount()
{
	if ( ( m_iDXLevel < 90 ) && ( m_iMaxRagdollCountDX8 >= 0 ) )
	{
		m_iCurrentMaxRagdollCount = m_iMaxRagdollCountDX8;
	}
	else
	{
		m_iCurrentMaxRagdollCount = m_iMaxRagdollCount;
	}

	s_RagdollLRU.SetMaxRagdollCount( m_iCurrentMaxRagdollCount );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CRagdollManager::InputSetMaxRagdollCount(inputdata_t &inputdata)
{
	m_iMaxRagdollCount = inputdata.value.Int();
	UpdateCurrentMaxRagDollCount();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CRagdollManager::InputSetMaxRagdollCountDX8(inputdata_t &inputdata)
{
	m_iMaxRagdollCountDX8 = inputdata.value.Int();
	UpdateCurrentMaxRagDollCount();
}

bool RagdollManager_SaveImportant( CAI_BaseNPC *pNPC )
{
#ifdef HL2_DLL
	CRagdollManager *pEnt =	(CRagdollManager *)gEntList.FindEntityByClassname( NULL, "game_ragdoll_manager" );

	if ( pEnt == NULL )
		return false;

	if ( pEnt->m_bSaveImportant )
	{
		if ( pNPC->Classify() == CLASS_PLAYER_ALLY || pNPC->Classify() == CLASS_PLAYER_ALLY_VITAL )
		{
			return true;
		}
	}
#endif

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CRagdollManager::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		// print max ragdoll count
		Q_snprintf(tempstr,sizeof(tempstr),"max ragdoll count: %d", m_iCurrentMaxRagdollCount.Get());
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

