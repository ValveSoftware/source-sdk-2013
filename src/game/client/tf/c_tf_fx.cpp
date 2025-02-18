//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_fx_shared.h"
#include "c_basetempentity.h"
#include "tier0/vprof.h"
#include <cliententitylist.h>

class C_TEFireBullets : public C_BaseTempEntity
{
public:

	DECLARE_CLASS( C_TEFireBullets, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:

	int		m_iPlayer;
	Vector	m_vecOrigin;
	QAngle	m_vecAngles;
	int		m_iWeaponID;
	int		m_iMode;
	int		m_iSeed;
	float	m_flSpread;
	bool	m_bCritical;
};

IMPLEMENT_CLIENTCLASS_EVENT( C_TEFireBullets, DT_TEFireBullets, CTEFireBullets );

BEGIN_RECV_TABLE_NOBASE( C_TEFireBullets, DT_TEFireBullets )
RecvPropVector( RECVINFO( m_vecOrigin ) ),
RecvPropFloat( RECVINFO( m_vecAngles[0] ) ),
RecvPropFloat( RECVINFO( m_vecAngles[1] ) ),
RecvPropInt( RECVINFO( m_iWeaponID ) ),
RecvPropInt( RECVINFO( m_iMode ) ), 
RecvPropInt( RECVINFO( m_iSeed ) ),
RecvPropInt( RECVINFO( m_iPlayer ) ),
RecvPropFloat( RECVINFO( m_flSpread ) ),
RecvPropInt( RECVINFO( m_bCritical ) ),
END_RECV_TABLE()

void C_TEFireBullets::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEFireBullets::PostDataUpdate" );
	
	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Create the effect.
		m_vecAngles.z = 0;
		FX_FireBullets( NULL, m_iPlayer + 1, m_vecOrigin, m_vecAngles, m_iWeaponID, m_iMode, m_iSeed, m_flSpread, -1, m_bCritical );
	}
}