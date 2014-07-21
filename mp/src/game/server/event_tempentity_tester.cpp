//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "basetempentity.h"
#include "event_tempentity_tester.h"
#include "tier1/strtools.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define TEMPENT_TEST_GAP		1.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecOrigin - 
//			&vecAngles - 
//			*single_te - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CTempEntTester::Create( const Vector &vecOrigin, const QAngle &vecAngles, const char *lifetime, const char *single_te )
{
	float life;
	char classname[ 128 ];
	if ( lifetime && lifetime[0] )
	{
		life = atoi( lifetime );
		life = MAX( 1.0, life );
		life = MIN( 1000.0, life );

		life += gpGlobals->curtime;
	}
	else
	{
		Msg( "Usage:  te <lifetime> <entname>\n" );
		return NULL;
	}

	if ( single_te && single_te[0] )
	{
		Q_strncpy( classname, single_te ,sizeof(classname));
		strlwr( classname );
	}
	else
	{
		Msg( "Usage:  te <lifetime> <entname>\n" );
		return NULL;
	}

	CTempEntTester *p = ( CTempEntTester * )CBaseEntity::CreateNoSpawn( "te_tester", vecOrigin, vecAngles );
	if ( !p )
	{
		return NULL;
	}

	Q_strncpy( p->m_szClass, classname ,sizeof(p->m_szClass));
	p->m_fLifeTime = life;

	p->Spawn();

	return p;
}

LINK_ENTITY_TO_CLASS( te_tester, CTempEntTester );

//-----------------------------------------------------------------------------
// Purpose: Called when object is being created
//-----------------------------------------------------------------------------
void CTempEntTester::Spawn( void )
{
	// Not a physical thing...
	AddEffects( EF_NODRAW );

	m_pCurrent = CBaseTempEntity::GetList();
	while ( m_pCurrent )
	{
		char name[ 128 ];
		Q_strncpy( name, m_pCurrent->GetName() ,sizeof(name));
		strlwr( name );
		if ( strstr( name, m_szClass ) )
		{
			break;
		}

		m_pCurrent = m_pCurrent->GetNext();
	}

	if ( !m_pCurrent )
	{
		DevMsg("Couldn't find temp entity '%s'\n", m_szClass );
		UTIL_Remove( this );
		return;
	}

	// Think right away
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Called when object should fire itself and move on
//-----------------------------------------------------------------------------
void CTempEntTester::Think( void )
{
	// Should never happen
	if ( !m_pCurrent )
	{
		UTIL_Remove( this );
		return;
	}

	m_pCurrent->Test( GetLocalOrigin(), GetLocalAngles() );
	SetNextThink( gpGlobals->curtime + TEMPENT_TEST_GAP );

	// Time to destroy?
	if ( gpGlobals->curtime >= m_fLifeTime )
	{
		UTIL_Remove( this );
		return;
	}
}