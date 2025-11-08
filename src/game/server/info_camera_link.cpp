//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

#include "info_camera_link.h"
#include "point_camera.h"
#include "utllinkedlist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Link between entities and cameras
//-----------------------------------------------------------------------------
class CInfoCameraLink : public CLogicalEntity
{
	DECLARE_CLASS( CInfoCameraLink, CLogicalEntity );
 	DECLARE_DATADESC();

public:
	CInfoCameraLink();
	~CInfoCameraLink();

	virtual void Activate();

private:
	void InputSetCamera(inputdata_t &inputdata);
	void InputSetTargetEntity(inputdata_t &inputdata);
	void SetCameraByName(const char *szName);

	CHandle<CPointCamera> m_hCamera;
	EHANDLE m_hTargetEntity;
	string_t m_strPointCamera;

	friend CBaseEntity *CreateInfoCameraLink( CBaseEntity *pTarget, CPointCamera *pCamera );
	friend void PointCameraSetupVisibility( CBaseEntity *pPlayer, int area, unsigned char *pvs, int pvssize );
};


//-----------------------------------------------------------------------------
// List of all info camera links
//-----------------------------------------------------------------------------
CUtlFixedLinkedList<CInfoCameraLink *> g_InfoCameraLinkList;


//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CInfoCameraLink )

	DEFINE_KEYFIELD( m_strPointCamera, FIELD_STRING, "PointCamera" ),

	DEFINE_FIELD( m_hCamera,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTargetEntity,	FIELD_EHANDLE ),

	// Outputs
	DEFINE_INPUTFUNC( FIELD_STRING, "SetCamera", InputSetCamera ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( info_camera_link, CInfoCameraLink );


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CInfoCameraLink::CInfoCameraLink()
{		
	g_InfoCameraLinkList.AddToTail( this );
}

CInfoCameraLink::~CInfoCameraLink()
{
	g_InfoCameraLinkList.FindAndRemove( this );
}


//-----------------------------------------------------------------------------
// Purpose: Called after all entities have spawned and after a load game.
//-----------------------------------------------------------------------------
void CInfoCameraLink::Activate()
{
	BaseClass::Activate();

	// Checks necessary to prevent interference with CreateInfoCameraLink
	if ( !m_hCamera )
	{
		SetCameraByName( STRING(m_strPointCamera) );
	}

	if ( !m_hTargetEntity )
	{
		m_hTargetEntity = gEntList.FindEntityByName( NULL, STRING(m_target) );
	}
}

void CInfoCameraLink::SetCameraByName(const char *szName)
{
	CBaseEntity *pBaseEnt = gEntList.FindEntityByName( NULL, szName );
	if( pBaseEnt )
	{
		m_hCamera = dynamic_cast<CPointCamera *>( pBaseEnt );
		if ( m_hCamera )
		{
			// Keep the camera name consistent for save/load
			m_strPointCamera = MAKE_STRING( szName );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInfoCameraLink::InputSetCamera(inputdata_t &inputdata)
{
	SetCameraByName( inputdata.value.String() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CreateInfoCameraLink( CBaseEntity *pTarget, CPointCamera *pCamera )
{
	CInfoCameraLink *pInfoCameraLink = (CInfoCameraLink*)CreateEntityByName( "info_camera_link" );
	if ( !pInfoCameraLink )
		return NULL;

	pInfoCameraLink->m_hCamera = pCamera;
	pInfoCameraLink->m_hTargetEntity = pTarget;
	pInfoCameraLink->Spawn();
	return pInfoCameraLink;
}


//-----------------------------------------------------------------------------
// Sets up visibility 
//-----------------------------------------------------------------------------
void PointCameraSetupVisibility( CBaseEntity *pPlayer, int area, unsigned char *pvs, int pvssize )
{
	int nPlayerIndex = pPlayer->entindex();

	for ( CPointCamera *pCameraEnt = GetPointCameraList(); pCameraEnt != NULL; pCameraEnt = pCameraEnt->m_pNext )
	{
		pCameraEnt->TransmitToPlayer( nPlayerIndex, false );
	}
	
	intp nNext;
	for ( intp i = g_InfoCameraLinkList.Head(); i != g_InfoCameraLinkList.InvalidIndex(); i = nNext )
	{
		nNext = g_InfoCameraLinkList.Next( i );

		CBaseEntity *pTargetEnt = g_InfoCameraLinkList[i]->m_hTargetEntity;
		if ( !pTargetEnt )
		{
			UTIL_Remove( g_InfoCameraLinkList[i] );
			continue;
		}

		// Don't bother if it's not visible
		if ( pTargetEnt->IsEffectActive( EF_NODRAW ) )
			continue;

		if ( !pTargetEnt->NetworkProp()->IsInPVS( pPlayer->edict(), pvs, pvssize ) )
			continue;

		if ( engine->CheckAreasConnected( area, pTargetEnt->NetworkProp()->AreaNum() ) )
		{
			CPointCamera *pCameraEnt = g_InfoCameraLinkList[i]->m_hCamera;
			if ( pCameraEnt )
			{
				engine->AddOriginToPVS( pCameraEnt->GetAbsOrigin() );
				pCameraEnt->TransmitToPlayer( nPlayerIndex, true );
			}
		}
	}
}

