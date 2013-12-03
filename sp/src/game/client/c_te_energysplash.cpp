//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "IEffects.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Energy Splash TE
//-----------------------------------------------------------------------------
class C_TEEnergySplash : public C_BaseTempEntity
{
public:
	DECLARE_CLIENTCLASS();

					C_TEEnergySplash( void );
	virtual			~C_TEEnergySplash( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

	virtual void	Precache( void );

public:
	Vector			m_vecPos;
	Vector			m_vecDir;
	bool			m_bExplosive;

	const struct model_t *m_pModel;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEEnergySplash::C_TEEnergySplash( void )
{
	m_vecPos.Init();
	m_vecDir.Init();
	m_bExplosive = false;
	m_pModel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEEnergySplash::~C_TEEnergySplash( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEEnergySplash::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEEnergySplash::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEEnergySplash::PostDataUpdate" );

	g_pEffects->EnergySplash( m_vecPos, m_vecDir, m_bExplosive );
}

void TE_EnergySplash( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* dir, bool bExplosive )
{
	g_pEffects->EnergySplash( *pos, *dir, bExplosive );
}

// Expose the TE to the engine.
IMPLEMENT_CLIENTCLASS_EVENT( C_TEEnergySplash, DT_TEEnergySplash, CTEEnergySplash );

BEGIN_RECV_TABLE_NOBASE(C_TEEnergySplash, DT_TEEnergySplash)
	RecvPropVector(RECVINFO(m_vecPos)),
	RecvPropVector(RECVINFO(m_vecDir)),
	RecvPropInt(RECVINFO(m_bExplosive)),
END_RECV_TABLE()

