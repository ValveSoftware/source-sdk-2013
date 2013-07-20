//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy.h"
#include "srcpy_physics.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace bp = boost::python;

//-----------------------------------------------------------------------------
// Purpose: IPhysicsShadowController base wrapper for python.
//-----------------------------------------------------------------------------
PyPhysicsShadowController::PyPhysicsShadowController( boost::python::object refPyPhysObj )
{
	m_pPyPhysObj = NULL;
	m_pShadCont = NULL;
	if( refPyPhysObj.ptr() == Py_None )
		return;

	m_pPyPhysObj = boost::python::extract<PyPhysicsObjectBase *>(refPyPhysObj);
	m_pPyPhysObj->CheckValid();
	m_pShadCont = m_pPyPhysObj->m_pPhysObj->GetShadowController();
}

void PyPhysicsShadowController::CheckValid()
{
	if( m_pPyPhysObj == NULL )
	{
		PyErr_SetString(PyExc_ValueError, "PhysicsObject invalid" );
		throw boost::python::error_already_set();
	}
	m_pPyPhysObj->CheckValid();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
// ? Is using memcmp on a PyObject correct ?
bool PyPhysicsShadowController::Cmp( boost::python::object other )
{
	if( other.ptr() == Py_None ) {
		return m_pShadCont != NULL;
	}

	if( PyObject_IsInstance(other.ptr(), boost::python::object(_physics.attr("PyPhysicsShadowController")).ptr()) )
	{
		IPhysicsShadowController *other_ext = boost::python::extract<IPhysicsShadowController *>(other);
		return memcmp(other_ext, m_pShadCont, sizeof(int));
	}

	return memcmp(other.ptr(), m_pShadCont, sizeof(int));
}

bool PyPhysicsShadowController::NonZero()
{
	return m_pShadCont != NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsShadowController::Update( const Vector &position, const QAngle &angles, float timeOffset )
{
	CheckValid();
	m_pShadCont->Update(position, angles, timeOffset);
}

void PyPhysicsShadowController::MaxSpeed( float maxSpeed, float maxAngularSpeed )
{
	CheckValid();
	m_pShadCont->MaxSpeed(maxSpeed, maxAngularSpeed);
}

void PyPhysicsShadowController::StepUp( float height )
{
	CheckValid();
	m_pShadCont->StepUp(height);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsShadowController::SetTeleportDistance( float teleportDistance )
{
	CheckValid();
	m_pShadCont->SetTeleportDistance(teleportDistance);
}

bool PyPhysicsShadowController::AllowsTranslation()
{
	CheckValid();
	return m_pShadCont->AllowsTranslation();
}

bool PyPhysicsShadowController::AllowsRotation()
{
	CheckValid();
	return m_pShadCont->AllowsRotation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsShadowController::SetPhysicallyControlled( bool isPhysicallyControlled )
{
	CheckValid();
	m_pShadCont->SetPhysicallyControlled(isPhysicallyControlled);
}

bool PyPhysicsShadowController::IsPhysicallyControlled()
{
	CheckValid();
	return m_pShadCont->IsPhysicallyControlled();
}

void PyPhysicsShadowController::GetLastImpulse( Vector *pOut )
{
	CheckValid();
	m_pShadCont->GetLastImpulse(pOut);
}

void PyPhysicsShadowController::UseShadowMaterial( bool bUseShadowMaterial )
{
	CheckValid();
	m_pShadCont->UseShadowMaterial(bUseShadowMaterial);
}

void PyPhysicsShadowController::ObjectMaterialChanged( int materialIndex )
{
	CheckValid();
	m_pShadCont->ObjectMaterialChanged(materialIndex);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float PyPhysicsShadowController::GetTargetPosition( Vector *pPositionOut, QAngle *pAnglesOut )
{
	CheckValid();
	return m_pShadCont->GetTargetPosition(pPositionOut, pAnglesOut);
}

float PyPhysicsShadowController::GetTeleportDistance( void )
{
	CheckValid();
	return m_pShadCont->GetTeleportDistance();
}

void PyPhysicsShadowController::GetMaxSpeed( float *pMaxSpeedOut, float *pMaxAngularSpeedOut )
{
	CheckValid();
	m_pShadCont->GetMaxSpeed(pMaxSpeedOut, pMaxAngularSpeedOut);
}

//-----------------------------------------------------------------------------
// Purpose: IPhysicsObject base wrapper for python.
//-----------------------------------------------------------------------------
// ? Is using memcmp on a PyObject correct ?
bool PyPhysicsObjectBase::Cmp( boost::python::object other )
{
	if( other.ptr() == Py_None ) {
		return m_pPhysObj != NULL;
	}

	if( PyObject_IsInstance(other.ptr(), boost::python::object(_physics.attr("PyPhysicsObjectBase")).ptr()) )
	{
		IPhysicsObject *other_ext = boost::python::extract<IPhysicsObject *>(other);
		return memcmp(other_ext, m_pPhysObj, sizeof(int));
	}

	return memcmp(other.ptr(), m_pPhysObj, sizeof(int));
}

bool PyPhysicsObjectBase::NonZero()
{
	return m_pPhysObj != NULL;
}

bool PyPhysicsObjectBase::IsStatic()
{
	CheckValid();
	return m_pPhysObj->IsStatic();
}

bool PyPhysicsObjectBase::IsAsleep()
{
	CheckValid();
	return m_pPhysObj->IsAsleep();
}

bool PyPhysicsObjectBase::IsTrigger()
{
	CheckValid();
	return m_pPhysObj->IsTrigger();
}

bool PyPhysicsObjectBase::IsFluid()
{
	CheckValid();
	return m_pPhysObj->IsFluid();
}

bool PyPhysicsObjectBase::IsHinged()
{
	CheckValid();
	return m_pPhysObj->IsHinged();
}

bool PyPhysicsObjectBase::IsCollisionEnabled()
{
	CheckValid();
	return m_pPhysObj->IsCollisionEnabled();
}

bool PyPhysicsObjectBase::IsGravityEnabled()
{
	CheckValid();
	return m_pPhysObj->IsGravityEnabled();
}

bool PyPhysicsObjectBase::IsDragEnabled()
{
	CheckValid();
	return m_pPhysObj->IsDragEnabled();
}

bool PyPhysicsObjectBase::IsMotionEnabled()
{
	CheckValid();
	return m_pPhysObj->IsMotionEnabled();
}

bool PyPhysicsObjectBase::IsMoveable()
{
	CheckValid();
	return m_pPhysObj->IsMoveable();
}

bool PyPhysicsObjectBase::IsAttachedToConstraint(bool bExternalOnly)
{
	CheckValid();
	return m_pPhysObj->IsAttachedToConstraint(bExternalOnly);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsObjectBase::EnableCollisions( bool enable )
{
	CheckValid();
	m_pPhysObj->EnableCollisions(enable);
}

void PyPhysicsObjectBase::EnableGravity( bool enable )
{
	CheckValid();
	m_pPhysObj->EnableGravity(enable);
}

void PyPhysicsObjectBase::EnableDrag( bool enable )
{
	CheckValid();
	m_pPhysObj->EnableDrag(enable);
}

void PyPhysicsObjectBase::EnableMotion( bool enable )
{
	CheckValid();
	m_pPhysObj->EnableMotion(enable);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsObjectBase::SetGameFlags( unsigned short userFlags )
{
	CheckValid();
	m_pPhysObj->SetGameFlags(userFlags);
}

unsigned short PyPhysicsObjectBase::GetGameFlags( void )
{
	CheckValid();
	return m_pPhysObj->GetGameFlags();
}

void PyPhysicsObjectBase::SetGameIndex( unsigned short gameIndex )
{
	CheckValid();
	m_pPhysObj->SetGameIndex(gameIndex);
}

unsigned short PyPhysicsObjectBase::GetGameIndex( void )
{
	CheckValid();
	return m_pPhysObj->GetGameIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsObjectBase::SetCallbackFlags( unsigned short callbackflags )
{
	CheckValid();
	m_pPhysObj->SetCallbackFlags(callbackflags);
}

unsigned short PyPhysicsObjectBase::GetCallbackFlags( void )
{
	CheckValid();
	return m_pPhysObj->GetCallbackFlags();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsObjectBase::Wake( void )
{
	CheckValid();
	m_pPhysObj->Wake();
}

void PyPhysicsObjectBase::Sleep( void )
{
	CheckValid();
	m_pPhysObj->Sleep();
}

void PyPhysicsObjectBase::RecheckCollisionFilter()
{
	CheckValid();
	m_pPhysObj->RecheckCollisionFilter();
}

void PyPhysicsObjectBase::RecheckContactPoints()
{
	CheckValid();
	m_pPhysObj->RecheckContactPoints();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsObjectBase::SetMass( float mass )
{
	CheckValid();
	m_pPhysObj->SetMass(mass);
}

float PyPhysicsObjectBase::GetMass( void )
{
	CheckValid();
	return m_pPhysObj->GetMass();
}

float PyPhysicsObjectBase::GetInvMass( void )
{
	CheckValid();
	return m_pPhysObj->GetInvMass();
}

Vector PyPhysicsObjectBase::GetInertia( void )
{
	CheckValid();
	return m_pPhysObj->GetInertia();
}

Vector PyPhysicsObjectBase::GetInvInertia( void )
{
	CheckValid();
	return m_pPhysObj->GetInvInertia();
}

void PyPhysicsObjectBase::SetInertia( const Vector &inertia )
{
	CheckValid();
	m_pPhysObj->SetInertia(inertia);
}

void PyPhysicsObjectBase::SetDamping( float speed, float rot )
{
	CheckValid();
	m_pPhysObj->SetDamping(&speed, &rot);
}
bp::tuple PyPhysicsObjectBase::GetDamping()
{
	CheckValid();
    float speed;
    float rot;
	m_pPhysObj->GetDamping(&speed, &rot);
	return bp::make_tuple(speed, rot);
}

void PyPhysicsObjectBase::SetDragCoefficient( float *pDrag, float *pAngularDrag )
{
	CheckValid();
	m_pPhysObj->SetDragCoefficient(pDrag, pAngularDrag);
}
void PyPhysicsObjectBase::SetBuoyancyRatio( float ratio )
{
	CheckValid();
	m_pPhysObj->SetBuoyancyRatio(ratio);
}

int	PyPhysicsObjectBase::GetMaterialIndex()
{
	CheckValid();
	return m_pPhysObj->GetMaterialIndex();
}

void PyPhysicsObjectBase::SetMaterialIndex( int materialIndex )
{
	CheckValid();
	m_pPhysObj->SetMaterialIndex(materialIndex);
}

// contents bits
unsigned int PyPhysicsObjectBase::GetContents()
{
	CheckValid();
	return m_pPhysObj->GetContents();
}

void PyPhysicsObjectBase::SetContents( unsigned int contents )
{
	CheckValid();
	m_pPhysObj->SetContents(contents);
}

float PyPhysicsObjectBase::GetSphereRadius()
{
	CheckValid();
	return m_pPhysObj->GetSphereRadius();
}

float PyPhysicsObjectBase::GetEnergy()
{
	CheckValid();
	return m_pPhysObj->GetEnergy();
}

Vector PyPhysicsObjectBase::GetMassCenterLocalSpace()
{
	CheckValid();
	return m_pPhysObj->GetMassCenterLocalSpace();
}

void PyPhysicsObjectBase::SetPosition( const Vector &worldPosition, const QAngle &angles, bool isTeleport )
{
	CheckValid();
	m_pPhysObj->SetPosition(worldPosition, angles, isTeleport);
}

void PyPhysicsObjectBase::SetPositionMatrix( const matrix3x4_t&matrix, bool isTeleport )
{
	CheckValid();
	m_pPhysObj->SetPositionMatrix(matrix, isTeleport);
}

void PyPhysicsObjectBase::GetPosition( Vector *worldPosition, QAngle *angles )
{
	CheckValid();
	m_pPhysObj->GetPosition(worldPosition, angles);
}

void PyPhysicsObjectBase::GetPositionMatrix( matrix3x4_t *positionMatrix )
{
	CheckValid();
	m_pPhysObj->GetPositionMatrix(positionMatrix);
}

void PyPhysicsObjectBase::SetVelocity( const Vector *velocity, const AngularImpulse *angularVelocity )
{
	CheckValid();
	m_pPhysObj->SetVelocity( velocity, angularVelocity );
}

void PyPhysicsObjectBase::SetVelocityInstantaneous( const Vector *velocity, const AngularImpulse *angularVelocity )
{
	CheckValid();
	m_pPhysObj->SetVelocityInstantaneous( velocity, angularVelocity );
}

void PyPhysicsObjectBase::GetVelocity( Vector *velocity, AngularImpulse *angularVelocity )
{
	CheckValid();
	m_pPhysObj->GetVelocity( velocity, angularVelocity );
}

void PyPhysicsObjectBase::AddVelocity( const Vector *velocity, const AngularImpulse *angularVelocity )
{
	CheckValid();
	m_pPhysObj->AddVelocity( velocity, angularVelocity );
}

void PyPhysicsObjectBase::GetVelocityAtPoint( const Vector &worldPosition, Vector *pVelocity )
{
	CheckValid();
	m_pPhysObj->GetVelocityAtPoint( worldPosition, pVelocity );
}

void PyPhysicsObjectBase::GetImplicitVelocity( Vector *velocity, AngularImpulse *angularVelocity )
{
	CheckValid();
	m_pPhysObj->GetImplicitVelocity( velocity, angularVelocity );
}

void PyPhysicsObjectBase::LocalToWorld( Vector *worldPosition, const Vector &localPosition )
{
	CheckValid();
	m_pPhysObj->LocalToWorld( worldPosition, localPosition );
}

void PyPhysicsObjectBase::WorldToLocal( Vector *localPosition, const Vector &worldPosition )
{
	CheckValid();
	m_pPhysObj->WorldToLocal( localPosition, worldPosition );
}

void PyPhysicsObjectBase::LocalToWorldVector( Vector *worldVector, const Vector &localVector )
{
	CheckValid();
	m_pPhysObj->LocalToWorldVector( worldVector, localVector );
}

void PyPhysicsObjectBase::WorldToLocalVector( Vector *localVector, const Vector &worldVector )
{
	CheckValid();
	m_pPhysObj->WorldToLocalVector( localVector, worldVector );
}

void PyPhysicsObjectBase::ApplyForceCenter( const Vector &forceVector )
{
	CheckValid();
	m_pPhysObj->ApplyForceCenter( forceVector );
}

void PyPhysicsObjectBase::ApplyForceOffset( const Vector &forceVector, const Vector &worldPosition )
{
	CheckValid();
	m_pPhysObj->ApplyForceOffset( forceVector, worldPosition );
}

void PyPhysicsObjectBase::ApplyTorqueCenter( const AngularImpulse &torque )
{
	CheckValid();
	m_pPhysObj->ApplyTorqueCenter( torque );
}

void PyPhysicsObjectBase::CalculateForceOffset( const Vector &forceVector, const Vector &worldPosition, Vector *centerForce, AngularImpulse *centerTorque )
{
	CheckValid();
	m_pPhysObj->CalculateForceOffset( forceVector, worldPosition, centerForce, centerTorque );
}

void PyPhysicsObjectBase::CalculateVelocityOffset( const Vector &forceVector, const Vector &worldPosition, Vector *centerVelocity, AngularImpulse *centerAngularVelocity )
{
	CheckValid();
	m_pPhysObj->CalculateVelocityOffset( forceVector, worldPosition, centerVelocity, centerAngularVelocity );
}

float PyPhysicsObjectBase::CalculateLinearDrag( const Vector &unitDirection )
{
	CheckValid();
	return m_pPhysObj->CalculateLinearDrag( unitDirection );
}

float PyPhysicsObjectBase::CalculateAngularDrag( const Vector &objectSpaceRotationAxis )
{
	CheckValid();
	return m_pPhysObj->CalculateAngularDrag( objectSpaceRotationAxis );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsObjectBase::SetShadow( float maxSpeed, float maxAngularSpeed, bool allowPhysicsMovement, bool allowPhysicsRotation )
{
	CheckValid();
	m_pPhysObj->SetShadow( maxSpeed, maxAngularSpeed, allowPhysicsMovement, allowPhysicsRotation );
}

void PyPhysicsObjectBase::UpdateShadow( const Vector &targetPosition, const QAngle &targetAngles, bool tempDisableGravity, float timeOffset )
{
	CheckValid();
	m_pPhysObj->UpdateShadow( targetPosition, targetAngles, tempDisableGravity, timeOffset );
}

int PyPhysicsObjectBase::GetShadowPosition( Vector *position, QAngle *angles )
{
	CheckValid();
	return m_pPhysObj->GetShadowPosition( position, angles );
}

PyPhysicsShadowController PyPhysicsObjectBase::GetShadowController( void )
{
	CheckValid();

	boost::python::object ref = boost::python::object(
		boost::python::handle<>(
		boost::python::borrowed(GetPySelf())
		)
	);

	return PyPhysicsShadowController(ref);
}

void PyPhysicsObjectBase::RemoveShadowController()
{
	CheckValid();
	m_pPhysObj->RemoveShadowController();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *PyPhysicsObjectBase::GetName()
{
	CheckValid();
	return m_pPhysObj->GetName();
}

void PyPhysicsObjectBase::BecomeTrigger()
{
	CheckValid();
	m_pPhysObj->BecomeTrigger();
}

void PyPhysicsObjectBase::RemoveTrigger()
{
	CheckValid();
	m_pPhysObj->RemoveTrigger();
}

void PyPhysicsObjectBase::BecomeHinged( int localAxis )
{
	CheckValid();
	m_pPhysObj->BecomeHinged(localAxis);
}

void PyPhysicsObjectBase::RemoveHinged()
{
	CheckValid();
	m_pPhysObj->RemoveHinged();
}

void PyPhysicsObjectBase::OutputDebugInfo()
{
	CheckValid();
	m_pPhysObj->OutputDebugInfo();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
PyPhysicsObject::PyPhysicsObject() :
	 m_hEnt(NULL), m_bValid(false), m_bOwnsPhysObject(false)
{
}

#if 0
PyPhysicsObject::PyPhysicsObject( IPhysicsObject *pPhysObj ) : m_hEnt(NULL), m_bOwnsPhysObject(true)
{
	if( !pPhysObj )
	{
		m_bValid = false;
		return;
	}

	m_bValid = true;
}
#endif // 0

PyPhysicsObject::PyPhysicsObject( CBaseEntity *pEnt ) : m_hEnt(NULL), m_bOwnsPhysObject(false)
{
	if( !pEnt || !pEnt->VPhysicsGetObject() )
	{
		m_hEnt = NULL;
		m_bValid = false;
		PyErr_SetString(PyExc_ValueError, "Invalid Entity or physics object" );
		throw boost::python::error_already_set();
		return;
	}

	m_hEnt = pEnt;
	m_pPhysObj = pEnt->VPhysicsGetObject();
	m_bValid = true;
}

PyPhysicsObject::~PyPhysicsObject()
{
	Destroy();
}

void PyPhysicsObject::InitFromPhysicsObject( IPhysicsObject *pPhysObj )
{
	m_pPhysObj = pPhysObj;
	m_bValid = true;
}

void PyPhysicsObject::Destroy()
{
	if( !m_bOwnsPhysObject || !m_bValid || !m_pPhysObj )
		return;

	if( !g_EntityCollisionHash )
		return;

	PhysDestroyObject(m_pPhysObj, m_hEnt);
	m_bValid = false;
}

bp::object PyCreatePhysicsObject( IPhysicsObject *pPhysObj )
{
	bp::object pyphysref = _physics.attr("PhysicsObject")();
	PyPhysicsObject *pPyPhysObj = bp::extract<PyPhysicsObject *>(pyphysref);
	if( !pPyPhysObj )
		return bp::object();
	pPyPhysObj->InitFromPhysicsObject( pPhysObj );
	return pyphysref;
}

bp::object PyCreatePhysicsObject( CBaseEntity *pEnt )
{
	bp::object physobj = _physics.attr("PhysicsObject")( *pEnt );
	//pPyPhysObj = bp::extract<PyPhysicsObject *)(physobj);
	//if( !pPyPhysObj )
	//	return;
	return physobj;
}

void PyPhysDestroyObject( PyPhysicsObject *pPyPhysObj, CBaseEntity *pEntity )
{
	if( !pPyPhysObj || !pPyPhysObj->IsValid() )
		return;
	pPyPhysObj->Destroy();
}

//-----------------------------------------------------------------------------
// Purpose: Physic collision interface
//-----------------------------------------------------------------------------
boost::python::tuple PyPhysicsCollision::CollideGetAABB( PyPhysicsObject *pPhysObj, const Vector &collideOrigin, const QAngle &collideAngles )
{
	Vector mins, maxs;

	if( !pPhysObj ) {
		PyErr_SetString(PyExc_ValueError, "Invalid Physic Object" );
		throw boost::python::error_already_set();
	}
	pPhysObj->CheckValid();

	physcollision->CollideGetAABB(&mins, &maxs, pPhysObj->m_pPhysObj->GetCollide(), collideOrigin, collideAngles);
	return boost::python::make_tuple(mins, maxs);
}

void PyPhysicsCollision::TraceBox( PyRay_t &ray, PyPhysicsObject &physObj, const Vector &collideOrigin, const QAngle &collideAngles, trace_t &ptr )
{
	if( !physObj.IsValid() )
	{
		PyErr_SetString(PyExc_ValueError, "Invalid Physic Object" );
		throw boost::python::error_already_set();
	}
	const CPhysCollide *pCollide = physObj.GetVPhysicsObject()->GetCollide();
	physcollision->TraceBox( ray.ToRay(), pCollide, collideOrigin, collideAngles, &ptr );
}

static PyPhysicsCollision s_pyphyscollision;
PyPhysicsCollision *pyphyscollision = &s_pyphyscollision;

//-----------------------------------------------------------------------------
// Purpose: Surface Props
//-----------------------------------------------------------------------------
// parses a text file containing surface prop keys
int PyPhysicsSurfaceProps::ParseSurfaceData( const char *pFilename, const char *pTextfile )
{
	return physprops->ParseSurfaceData( pFilename, pTextfile );
}

// current number of entries in the database
int PyPhysicsSurfaceProps::SurfacePropCount( void ) const
{
	return physprops->SurfacePropCount();
}

int PyPhysicsSurfaceProps::GetSurfaceIndex( const char *pSurfacePropName ) const
{
	return physprops->GetSurfaceIndex( pSurfacePropName );
}
//void	GetPhysicsProperties( int surfaceDataIndex, float *density, float *thickness, float *friction, float *elasticity ) const;

surfacedata_t PyPhysicsSurfaceProps::GetSurfaceData( int surfaceDataIndex )
{
	surfacedata_t *pSurfData = physprops->GetSurfaceData( surfaceDataIndex );
	return *pSurfData;
}

const char *PyPhysicsSurfaceProps::GetString( unsigned short stringTableIndex ) const
{
	return physprops->GetString( stringTableIndex );
}

const char *PyPhysicsSurfaceProps::GetPropName( int surfaceDataIndex ) const
{
	return physprops->GetPropName( surfaceDataIndex );
}

void PyPhysicsSurfaceProps::GetPhysicsParameters( int surfaceDataIndex, surfacephysicsparams_t &paramsout ) const
{
	physprops->GetPhysicsParameters( surfaceDataIndex, &paramsout );
}

static PyPhysicsSurfaceProps s_pyphysprops;
PyPhysicsSurfaceProps *pyphysprops = &s_pyphysprops;

#ifndef CLIENT_DLL
// Callbacks
void PyPhysCallbackImpulse( PyPhysicsObject &pyPhysicsObject, const Vector &vecCenterForce, const AngularImpulse &vecCenterTorque )
{
	if( pyPhysicsObject.GetVPhysicsObject() )
		PhysCallbackImpulse( pyPhysicsObject.GetVPhysicsObject(), vecCenterForce, vecCenterTorque );
	else
	{
		PyErr_SetString(PyExc_ValueError, "Invalid Physics Object" );
		throw boost::python::error_already_set();
	}
}

void PyPhysCallbackSetVelocity( PyPhysicsObject &pyPhysicsObject, const Vector &vecVelocity )
{
	if( pyPhysicsObject.GetVPhysicsObject() )
		PhysCallbackSetVelocity( pyPhysicsObject.GetVPhysicsObject(), vecVelocity );
	else
	{
		PyErr_SetString(PyExc_ValueError, "Invalid Physics Object" );
		throw boost::python::error_already_set();
	}
}

void PyPhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info, gamevcollisionevent_t &event, int hurtIndex )
{
	if( !pEntity ) 
	{
		PyErr_SetString(PyExc_ValueError, "Invalid entity" );
		throw boost::python::error_already_set();
		return;
	}
	PhysCallbackDamage( pEntity, info, event, hurtIndex );
}

void PyPhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info )
{
	if( !pEntity ) 
	{
		PyErr_SetString(PyExc_ValueError, "Invalid entity" );
		throw boost::python::error_already_set();
		return;
	}
	PhysCallbackDamage( pEntity, info );
}

void PyPhysCallbackRemove(CBaseEntity *pRemove)
{
	if( !pRemove ) 
	{
		PyErr_SetString(PyExc_ValueError, "Invalid entity" );
		throw boost::python::error_already_set();
		return;
	}
	PhysCallbackRemove( pRemove->NetworkProp() );
}

// Impact damage 
float PyCalculateDefaultPhysicsDamage( int index, gamevcollisionevent_t *pEvent, float energyScale, 
									  bool allowStaticDamage, int &damageTypeOut, const char *iszDamageTableName, bool bDamageFromHeldObjects )
{
	return CalculateDefaultPhysicsDamage(index, pEvent, energyScale, allowStaticDamage, damageTypeOut, MAKE_STRING(iszDamageTableName), bDamageFromHeldObjects);
}
#endif // CLIENT_DLL

void PyForcePhysicsSimulate()
{
	IGameSystemPerFrame *pPerFrame = dynamic_cast<IGameSystemPerFrame *>( PhysicsGameSystem() );
	if( pPerFrame )
	{
#ifdef CLIENT_DLL
		PhysicsSimulate();
#else
		pPerFrame->FrameUpdatePostEntityThink();
#endif // CLIENT_DLL
	}
	else
	{
		PyErr_SetString(PyExc_ValueError, "Invalid game system" );
		throw boost::python::error_already_set();
	}
}