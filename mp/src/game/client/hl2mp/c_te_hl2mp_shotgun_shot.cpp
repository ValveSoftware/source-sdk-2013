//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_basetempentity.h"
#include <cliententitylist.h>
#include "ammodef.h"
#include "c_te_effect_dispatch.h"
#include "shot_manipulator.h"

class C_TEHL2MPFireBullets : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEHL2MPFireBullets, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

	void CreateEffects( void );

public:
	int		m_iPlayer;
	Vector	m_vecOrigin;
	Vector  m_vecDir;
	int		m_iAmmoID;
	int		m_iWeaponIndex;
	int		m_iSeed;
	float	m_flSpread;
	int		m_iShots;
	bool	m_bDoImpacts;
	bool	m_bDoTracers;
};

class CTraceFilterSkipPlayerAndViewModelOnly : public CTraceFilter
{
public:
	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		C_BaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		if( pEntity &&
			( ( dynamic_cast<C_BaseViewModel *>( pEntity ) != NULL ) ||
			( dynamic_cast<C_BasePlayer *>( pEntity ) != NULL ) ) )
		{
			return false;
		}
		else
		{
			return true;
		}
	}
};

void C_TEHL2MPFireBullets::CreateEffects( void )
{
	CAmmoDef*	pAmmoDef	= GetAmmoDef();

	if ( pAmmoDef == NULL )
		 return;

	C_BaseEntity *pEnt = ClientEntityList().GetEnt( m_iPlayer );

	if ( pEnt )
	{
		C_BasePlayer *pPlayer = dynamic_cast<C_BasePlayer *>(pEnt);

		if ( pPlayer && pPlayer->GetActiveWeapon() )
		{
			C_BaseCombatWeapon *pWpn = dynamic_cast<C_BaseCombatWeapon *>( pPlayer->GetActiveWeapon() );

			if ( pWpn )
			{
				int iSeed = m_iSeed;
					
				CShotManipulator Manipulator( m_vecDir );

				for (int iShot = 0; iShot < m_iShots; iShot++)
				{
					RandomSeed( iSeed );	// init random system with this seed

					// Don't run the biasing code for the player at the moment.
					Vector vecDir = Manipulator.ApplySpread( Vector( m_flSpread, m_flSpread, m_flSpread ) );
					Vector vecEnd = m_vecOrigin + vecDir * MAX_TRACE_LENGTH;
					trace_t tr;
					CTraceFilterSkipPlayerAndViewModelOnly traceFilter;

					if( m_iShots > 1 && iShot % 2 )
					{
						// Half of the shotgun pellets are hulls that make it easier to hit targets with the shotgun.
						UTIL_TraceHull( m_vecOrigin, vecEnd, Vector( -3, -3, -3 ), Vector( 3, 3, 3 ), MASK_SHOT, &traceFilter, &tr );
					}
					else
					{
						UTIL_TraceLine( m_vecOrigin, vecEnd, MASK_SHOT, &traceFilter, &tr);
					}

					if ( m_bDoTracers )
					{
						const char *pTracerName = pWpn->GetTracerType();

						CEffectData data;
						data.m_vStart = tr.startpos;
						data.m_vOrigin = tr.endpos;
						data.m_hEntity = pWpn->GetRefEHandle();
						data.m_flScale = 0.0f;
						data.m_fFlags |= TRACER_FLAG_USEATTACHMENT;
						// Stomp the start, since it's not going to be used anyway
						data.m_nAttachmentIndex = 1;

						if ( pTracerName )
						{
							DispatchEffect( pTracerName, data );
						}
						else
						{
							DispatchEffect( "Tracer", data );
						}
					}
					
					if ( m_bDoImpacts )
					{
						pWpn->DoImpactEffect( tr, pAmmoDef->DamageType( m_iAmmoID ) );
					}

					iSeed++;
				}
			}
		}
	}

}

void C_TEHL2MPFireBullets::PostDataUpdate( DataUpdateType_t updateType )
{
	if ( m_bDoTracers || m_bDoImpacts )
	{
		CreateEffects();
	}
}


IMPLEMENT_CLIENTCLASS_EVENT( C_TEHL2MPFireBullets, DT_TEHL2MPFireBullets, CTEHL2MPFireBullets );


BEGIN_RECV_TABLE_NOBASE(C_TEHL2MPFireBullets, DT_TEHL2MPFireBullets )
	RecvPropVector( RECVINFO( m_vecOrigin ) ),
	RecvPropVector( RECVINFO( m_vecDir ) ),
	RecvPropInt( RECVINFO( m_iAmmoID ) ),
	RecvPropInt( RECVINFO( m_iSeed ) ),
	RecvPropInt( RECVINFO( m_iShots ) ),
	RecvPropInt( RECVINFO( m_iPlayer ) ),
	RecvPropInt( RECVINFO( m_iWeaponIndex ) ),
	RecvPropFloat( RECVINFO( m_flSpread ) ),
	RecvPropBool( RECVINFO( m_bDoImpacts ) ),
	RecvPropBool( RECVINFO( m_bDoTracers ) ),
END_RECV_TABLE()


