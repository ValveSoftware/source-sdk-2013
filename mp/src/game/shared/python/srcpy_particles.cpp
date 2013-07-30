//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "srcpy_particles.h"
#include <particles/particles.h>

#ifdef CLIENT_DLL
	#include "particles_new.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#if 0
void ParticleEffectInvalid_Error()
{
	PyErr_SetString(PyExc_ValueError, "ParticleEffect invalid" );
	throw boost::python::error_already_set();
}

CNewParticleEffectHandle::CNewParticleEffectHandle( CBaseEntity *pOwner, const char *pParticleSystemName,
						 const char *pDebugName )
{
	m_sInternal = CNewParticleEffect::Create(pOwner, pParticleSystemName, pDebugName);
}

void CNewParticleEffectHandle::SetControlPointEntity( int nWhichPoint, CBaseEntity *pEntity )
{
	PARTICLE_VALID();
	m_sInternal->SetControlPointEntity(nWhichPoint, pEntity); 
}

EHANDLE CNewParticleEffectHandle::GetControlPointEntity( int nWhichPoint )
{
	PARTICLE_VALID_RV(NULL);
	return m_sInternal->GetControlPointEntity(nWhichPoint);
}
#endif // 0

// Base dynamic light python
void PyDBaseLight::CheckValid()
{
	if( m_hOwner.Get() == NULL )
	{
		PyErr_SetString(PyExc_ValueError, "DLight: no entity" );
		throw boost::python::error_already_set(); 
	}

	if( m_pDLight == NULL )
	{
		PyErr_SetString(PyExc_ValueError, "DLight: no dlight" );
		throw boost::python::error_already_set(); 
	}

	if( m_fDie != 0 && m_fDie < gpGlobals->curtime )
	{
		PyErr_SetString(PyExc_ValueError, "DLight: I'm dead" );
		throw boost::python::error_already_set(); 
	}
}

PyDLight::PyDLight( C_BaseEntity *owner ) : PyDBaseLight(owner)
{
	if( owner ) 
	{
		m_pDLight = effects->CL_AllocDlight(owner->index);
	}
}

PyELight::PyELight( C_BaseEntity *owner ) : PyDBaseLight(owner)
{
	if( owner ) 
	{
		m_pDLight = effects->CL_AllocElight(owner->index);
	}
}

#endif // CLIENT_DLL

void PyShouldLoadSheets( bool bLoadSheets )
{
	if( !g_pParticleSystemMgr )
	{
		PyErr_SetString(PyExc_ValueError, "Particle system not initialized yet!" );
		throw boost::python::error_already_set(); 
		return;
	}
	g_pParticleSystemMgr->ShouldLoadSheets( bLoadSheets );
}

bool PyReadParticleConfigFile( const char *pFileName, bool bPrecache, bool bDecommitTempMemory )
{
	if( !g_pParticleSystemMgr )
	{
		PyErr_SetString(PyExc_ValueError, "Particle system not initialized yet!" );
		throw boost::python::error_already_set(); 
		return false;
	}
	if( !pFileName ) return false;
	return g_pParticleSystemMgr->ReadParticleConfigFile( pFileName, bPrecache, bDecommitTempMemory );
}

void PyDecommitTempMemory()
{
	if( !g_pParticleSystemMgr )
	{
		PyErr_SetString(PyExc_ValueError, "Particle system not initialized yet!" );
		throw boost::python::error_already_set(); 
		return;
	}
	g_pParticleSystemMgr->DecommitTempMemory();
}