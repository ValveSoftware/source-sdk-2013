//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "c_baseentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_PhysMagnet : public C_BaseAnimating
{
	DECLARE_CLASS( C_PhysMagnet, C_BaseAnimating );
public:
	DECLARE_CLIENTCLASS();

	C_PhysMagnet();
	virtual	~C_PhysMagnet();

	void	PostDataUpdate( DataUpdateType_t updateType );
	bool	GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const;
	
public:
	// Data received from the server
	CUtlVector< int > m_aAttachedObjectsFromServer;

	// Private list of entities on the magnet
	CUtlVector< EHANDLE > m_aAttachedObjects;
};

//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Magnet's attached object entindexes to handles
//-----------------------------------------------------------------------------
void RecvProxy_MagnetAttachedObjectList(  const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_PhysMagnet *pMagnet = (C_PhysMagnet*)pOut;
	pMagnet->m_aAttachedObjectsFromServer[pData->m_iElement] = pData->m_Value.m_Int;
}


void RecvProxyArrayLength_MagnetAttachedArray( void *pStruct, int objectID, int currentArrayLength )
{
	C_PhysMagnet *pMagnet = (C_PhysMagnet*)pStruct;
	
	if ( pMagnet->m_aAttachedObjectsFromServer.Size() != currentArrayLength )
		pMagnet->m_aAttachedObjectsFromServer.SetSize( currentArrayLength );
}

IMPLEMENT_CLIENTCLASS_DT(C_PhysMagnet, DT_PhysMagnet, CPhysMagnet)

	// ROBIN: Disabled because we don't need it anymore
	/*
	RecvPropArray2( 
		RecvProxyArrayLength_MagnetAttachedArray,
		RecvPropInt( "magnetattached_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_MagnetAttachedObjectList ), 
		128, 
		0, 
		"magnetattached_array"
		)
	*/

END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PhysMagnet::C_PhysMagnet()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PhysMagnet::~C_PhysMagnet()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PhysMagnet::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	/*
	// First, detect any entities removed from the magnet and restore their shadows
	int iCount = m_aAttachedObjects.Count();
	int iServerCount = m_aAttachedObjectsFromServer.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		int iEntIndex = m_aAttachedObjects[i]->entindex();
		for ( int j = 0; j < iServerCount; j++ )
		{
			if ( iEntIndex == m_aAttachedObjectsFromServer[j] )
				break;
		}

		if ( j == iServerCount )
		{
			// Ok, a previously attached object is no longer attached
			m_aAttachedObjects[i]->SetShadowUseOtherEntity( NULL );
			m_aAttachedObjects.Remove(i);
		}
	}

	// Make sure newly attached entities have vertical shadows too
	for ( i = 0; i < iServerCount; i++ )
	{
		C_BaseEntity *pEntity = cl_entitylist->GetEnt( m_aAttachedObjectsFromServer[i] );
		if ( m_aAttachedObjects.Find( pEntity ) == m_aAttachedObjects.InvalidIndex() )
		{
			pEntity->SetShadowUseOtherEntity( this );
			m_aAttachedObjects.AddToTail( pEntity );
		}
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Return a per-entity shadow cast direction
//-----------------------------------------------------------------------------
bool C_PhysMagnet::GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const
{
	// Magnets shadow is more vertical than others
	//Vector vecDown = g_pClientShadowMgr->GetShadowDirection() - Vector(0,0,1);
	//VectorNormalize( vecDown );
	//*pDirection = vecDown;
	*pDirection = Vector(0,0,-1);
	return true;
}