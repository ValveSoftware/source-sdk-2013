//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ragdoll_shared.h"
#include "bone_setup.h"
#include "vphysics/constraints.h"
#include "vphysics/collision_set.h"
#include "vcollide_parse.h"
#include "vphysics_interface.h"
#include "tier0/vprof.h"
#include "engine/ivdebugoverlay.h"
#include "solidsetdefaults.h"
//CLIENT
#ifdef CLIENT_DLL 
#include "c_fire_smoke.h"
#include "c_entitydissolve.h"
#include "engine/IEngineSound.h"
#endif

//SERVER
#if !defined( CLIENT_DLL )
#include "util.h"
#include "EntityFlame.h"
#include "EntityDissolve.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CRagdollLowViolenceManager g_RagdollLVManager;

void CRagdollLowViolenceManager::SetLowViolence( const char *pMapName )
{
	// set the value using the engine's low violence settings
	m_bLowViolence = UTIL_IsLowViolence();

#if !defined( CLIENT_DLL )
	// the server doesn't worry about low violence during multiplayer games
	if ( g_pGameRules && g_pGameRules->IsMultiplayer() )
	{
		m_bLowViolence = false;
	}
#endif

	// Turn the low violence ragdoll stuff off if we're in the HL2 Citadel maps because
	// the player has the super gravity gun and fading ragdolls will break things.
	if( hl2_episodic.GetBool() )
	{
		if ( Q_stricmp( pMapName, "ep1_citadel_02" ) == 0 ||
			Q_stricmp( pMapName, "ep1_citadel_02b" ) == 0 ||
			Q_stricmp( pMapName, "ep1_citadel_03" ) == 0 )
		{
			m_bLowViolence = false;
		}
	}
	else
	{
		if ( Q_stricmp( pMapName, "d3_citadel_03" ) == 0 ||
			Q_stricmp( pMapName, "d3_citadel_04" ) == 0 ||
			Q_stricmp( pMapName, "d3_citadel_05" ) == 0 ||
			Q_stricmp( pMapName, "d3_breen_01" ) == 0 )
		{
			m_bLowViolence = false;
		}
	}
}

class CRagdollCollisionRules : public IVPhysicsKeyHandler
{
public:
	CRagdollCollisionRules( IPhysicsCollisionSet *pSet )
	{
		m_pSet = pSet;
		m_bSelfCollisions = true;
	}
	virtual void ParseKeyValue( void *pData, const char *pKey, const char *pValue )
	{
		if ( !strcmpi( pKey, "selfcollisions" ) )
		{
			// keys disabled by default
			Assert( atoi(pValue) == 0 );
			m_bSelfCollisions = false;
		}
		else if ( !strcmpi( pKey, "collisionpair" ) )
		{
			if ( m_bSelfCollisions )
			{
				char szToken[256];
				const char *pStr = nexttoken(szToken, pValue, ',');
				int index0 = atoi(szToken);
				nexttoken( szToken, pStr, ',' );
				int index1 = atoi(szToken);

				m_pSet->EnableCollisions( index0, index1 );
			}
			else
			{
				Assert(0);
			}
		}
	}
	virtual void SetDefaults( void *pData ) {}

private:
	IPhysicsCollisionSet *m_pSet;
	bool				m_bSelfCollisions;
};

class CRagdollAnimatedFriction : public IVPhysicsKeyHandler
{
public:
	CRagdollAnimatedFriction( ragdoll_t *ragdoll )
	{
		m_ragdoll = ragdoll;
	}
	virtual void ParseKeyValue( void *pData, const char *pKey, const char *pValue )
	{
		if ( !strcmpi( pKey, "animfrictionmin" ) )
		{
			m_ragdoll->animfriction.iMinAnimatedFriction = atoi( pValue );
		}
		else if ( !strcmpi( pKey, "animfrictionmax" ) )
		{
			m_ragdoll->animfriction.iMaxAnimatedFriction = atoi( pValue );
		}
		else if ( !strcmpi( pKey, "animfrictiontimein" ) )
		{
			m_ragdoll->animfriction.flFrictionTimeIn = atof( pValue );
		}
		else if ( !strcmpi( pKey, "animfrictiontimeout" ) )
		{
			m_ragdoll->animfriction.flFrictionTimeOut = atof( pValue );
		}
		else if ( !strcmpi( pKey, "animfrictiontimehold" ) )
		{
			m_ragdoll->animfriction.flFrictionTimeHold = atof( pValue );
		}
	}

	virtual void SetDefaults( void *pData ) {}

private:
	ragdoll_t *m_ragdoll;
};

void RagdollSetupAnimatedFriction( IPhysicsEnvironment *pPhysEnv, ragdoll_t *ragdoll, int iModelIndex )
{
	vcollide_t* pCollide = modelinfo->GetVCollide( iModelIndex );

	if ( pCollide )
	{
		IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );

		while ( !pParse->Finished() )
		{
			const char *pBlock = pParse->GetCurrentBlockName();

			if ( !strcmpi( pBlock, "animatedfriction") ) 
			{
				CRagdollAnimatedFriction friction( ragdoll );
				pParse->ParseCustom( (void*)&friction, &friction );
			}
			else
			{
				pParse->SkipBlock();
			}
		}

		physcollision->VPhysicsKeyParserDestroy( pParse );
	}
}

static void RagdollAddSolid( IPhysicsEnvironment *pPhysEnv, ragdoll_t &ragdoll, const ragdollparams_t &params, solid_t &solid )
{
	if ( solid.index >= 0 && solid.index < params.pCollide->solidCount)
	{
		Assert( ragdoll.listCount == solid.index );
		int boneIndex = Studio_BoneIndexByName( params.pStudioHdr, solid.name );
		ragdoll.boneIndex[ragdoll.listCount] = boneIndex;

		if ( boneIndex >= 0 )
		{
			if ( params.fixedConstraints )
			{
				solid.params.mass = 1000.f;
			}

			solid.params.rotInertiaLimit = 0.1;
			solid.params.pGameData = params.pGameData;
			int surfaceData = physprops->GetSurfaceIndex( solid.surfaceprop );

			if ( surfaceData < 0 )
				surfaceData = physprops->GetSurfaceIndex( "default" );

			solid.params.pName = params.pStudioHdr->pszName();
			ragdoll.list[ragdoll.listCount].pObject = pPhysEnv->CreatePolyObject( params.pCollide->solids[solid.index], surfaceData, vec3_origin, vec3_angle, &solid.params );
			ragdoll.list[ragdoll.listCount].pObject->SetPositionMatrix( params.pCurrentBones[boneIndex], true );
			ragdoll.list[ragdoll.listCount].parentIndex = -1;
			ragdoll.list[ragdoll.listCount].pObject->SetGameIndex( ragdoll.listCount );

			ragdoll.listCount++;
		}
		else
		{
			Msg( "CRagdollProp::CreateObjects:  Couldn't Lookup Bone %s\n", solid.name );
		}
	}
}


static void RagdollAddConstraint( IPhysicsEnvironment *pPhysEnv, ragdoll_t &ragdoll, const ragdollparams_t &params, constraint_ragdollparams_t &constraint )
{
	if( constraint.childIndex == constraint.parentIndex )
	{
		DevMsg( 1, "Bogus constraint on ragdoll %s\n", params.pStudioHdr->pszName() );
		constraint.childIndex = -1;
		constraint.parentIndex = -1;
	}
	if ( constraint.childIndex >= 0 && constraint.parentIndex >= 0 )
	{
		Assert(constraint.childIndex<ragdoll.listCount);


		ragdollelement_t &childElement = ragdoll.list[constraint.childIndex];
		// save parent index
		childElement.parentIndex = constraint.parentIndex;
	
		if ( params.jointFrictionScale > 0 )
		{
			for ( int k = 0; k < 3; k++ )
			{
				constraint.axes[k].torque *= params.jointFrictionScale;
			}
		}
		// this parent/child pair is not usually a parent/child pair in the skeleton.  There
		// are often bones in between that are collapsed for simulation.  So we need to compute
		// the transform.
		Studio_CalcBoneToBoneTransform( params.pStudioHdr, ragdoll.boneIndex[constraint.childIndex], ragdoll.boneIndex[constraint.parentIndex], constraint.constraintToAttached );
		MatrixGetColumn( constraint.constraintToAttached, 3, childElement.originParentSpace );
		// UNDONE: We could transform the constraint limit axes relative to the bone space
		// using this data.  Do we need that feature?
		SetIdentityMatrix( constraint.constraintToReference );
		if ( params.fixedConstraints )
		{
			// Makes the ragdoll a statue...
			constraint_fixedparams_t fixed;
			fixed.Defaults();
			fixed.InitWithCurrentObjectState( childElement.pObject, ragdoll.list[constraint.parentIndex].pObject );
			fixed.constraint.Defaults();
			childElement.pConstraint = pPhysEnv->CreateFixedConstraint( childElement.pObject, ragdoll.list[constraint.parentIndex].pObject, ragdoll.pGroup, fixed );
		}
		else
		{
			childElement.pConstraint = pPhysEnv->CreateRagdollConstraint( childElement.pObject, ragdoll.list[constraint.parentIndex].pObject, ragdoll.pGroup, constraint );
		}
	}
}


static void RagdollCreateObjects( IPhysicsEnvironment *pPhysEnv, ragdoll_t &ragdoll, const ragdollparams_t &params )
{
	ragdoll.listCount = 0;
	ragdoll.pGroup = NULL;
	ragdoll.allowStretch = params.allowStretch;
	memset( ragdoll.list, 0, sizeof(ragdoll.list) );
	memset( &ragdoll.animfriction, 0, sizeof(ragdoll.animfriction) );
	
	if ( !params.pCollide || params.pCollide->solidCount > RAGDOLL_MAX_ELEMENTS )
		return;

	constraint_groupparams_t group;
	group.Defaults();
	ragdoll.pGroup = pPhysEnv->CreateConstraintGroup( group );
 
	IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( params.pCollide->pKeyValues );
	while ( !pParse->Finished() )
	{
		const char *pBlock = pParse->GetCurrentBlockName();
		if ( !strcmpi( pBlock, "solid" ) )
		{
			solid_t solid;

			pParse->ParseSolid( &solid, &g_SolidSetup );
			RagdollAddSolid( pPhysEnv, ragdoll, params, solid );
		}
		else if ( !strcmpi( pBlock, "ragdollconstraint" ) )
		{
			constraint_ragdollparams_t constraint;
			pParse->ParseRagdollConstraint( &constraint, NULL );
			RagdollAddConstraint( pPhysEnv, ragdoll, params, constraint );
		}
		else if ( !strcmpi( pBlock, "collisionrules" ) )
		{
			IPhysicsCollisionSet *pSet = physics->FindOrCreateCollisionSet( params.modelIndex, ragdoll.listCount );
			CRagdollCollisionRules rules(pSet);
			pParse->ParseCustom( (void *)&rules, &rules );
		}
		else if ( !strcmpi( pBlock, "animatedfriction") ) 
		{
			CRagdollAnimatedFriction friction( &ragdoll );
			pParse->ParseCustom( (void*)&friction, &friction );
		}
		else
		{
			pParse->SkipBlock();
		}
	}
	physcollision->VPhysicsKeyParserDestroy( pParse );
}

void RagdollSetupCollisions( ragdoll_t &ragdoll, vcollide_t *pCollide, int modelIndex )
{
	Assert(pCollide);
	if (!pCollide)
		return;

	IPhysicsCollisionSet *pSet = physics->FindCollisionSet( modelIndex );
	if ( !pSet )
	{
		pSet = physics->FindOrCreateCollisionSet( modelIndex, ragdoll.listCount );
		if ( !pSet )
			return;

		bool bFoundRules = false;

		IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );
		while ( !pParse->Finished() )
		{
			const char *pBlock = pParse->GetCurrentBlockName();
			if ( !strcmpi( pBlock, "collisionrules" ) )
			{
				IPhysicsCollisionSet *pSetRules = physics->FindOrCreateCollisionSet( modelIndex, ragdoll.listCount );
				CRagdollCollisionRules rules( pSetRules );
				pParse->ParseCustom( (void *)&rules, &rules );
				bFoundRules = true;
			}
			else
			{
				pParse->SkipBlock();
			}
		}
		physcollision->VPhysicsKeyParserDestroy( pParse );

		if ( !bFoundRules )
		{
			// these are the default rules - each piece collides with everything
			// except immediate parent/constrained object.
			int i;
			for ( i = 0; i < ragdoll.listCount; i++ )
			{
				for ( int j = i+1; j < ragdoll.listCount; j++ )
				{
					pSet->EnableCollisions( i, j );
				}
			}
			for ( i = 0; i < ragdoll.listCount; i++ )
			{
  				int parent = ragdoll.list[i].parentIndex;
				if ( parent >= 0 )
				{
  					Assert( ragdoll.list[i].pObject );
  					Assert( ragdoll.list[i].pConstraint );
					pSet->DisableCollisions( i, parent );
				}
 			}
		}
	}
}

void RagdollActivate( ragdoll_t &ragdoll, vcollide_t *pCollide, int modelIndex, bool bForceWake )
{
	RagdollSetupCollisions( ragdoll, pCollide, modelIndex );
	for ( int i = 0; i < ragdoll.listCount; i++ )
	{
		ragdoll.list[i].pObject->SetGameIndex( i );
		PhysSetGameFlags( ragdoll.list[i].pObject, FVPHYSICS_MULTIOBJECT_ENTITY );
		// now that the relationships are set, activate the collision system
		ragdoll.list[i].pObject->EnableCollisions( true );

		if ( bForceWake == true )
		{
			ragdoll.list[i].pObject->Wake();
		}
	}
	if ( ragdoll.pGroup )
	{
		// NOTE: This also wakes the objects
		ragdoll.pGroup->Activate();
		// so if we didn't want that, we'll need to put them back to sleep here
		if ( !bForceWake )
		{
			for ( int i = 0; i < ragdoll.listCount; i++ )
			{
				ragdoll.list[i].pObject->Sleep();
			}

		}
	}
}


bool RagdollCreate( ragdoll_t &ragdoll, const ragdollparams_t &params, IPhysicsEnvironment *pPhysEnv )
{
	RagdollCreateObjects( pPhysEnv, ragdoll, params );

	if ( !ragdoll.listCount )
		return false;

	int forceBone = params.forceBoneIndex;
	
	int i;
	float totalMass = 0;
	for ( i = 0; i < ragdoll.listCount; i++ )
	{
		totalMass += ragdoll.list[i].pObject->GetMass();
	}
	totalMass = MAX(totalMass,1);

	// apply force to the model
	Vector nudgeForce = params.forceVector;
	Vector forcePosition = params.forcePosition;
	// UNDONE: Test scaling the force by total mass on all bones
	
	Assert( forceBone < ragdoll.listCount );

	if ( forceBone >= 0 && forceBone < ragdoll.listCount )
	{
		ragdoll.list[forceBone].pObject->ApplyForceCenter( nudgeForce );
		//nudgeForce *= 0.5;
		ragdoll.list[forceBone].pObject->GetPosition( &forcePosition, NULL );
	}

	for ( i = 0; i < ragdoll.listCount; i++ )
	{
		PhysSetGameFlags( ragdoll.list[i].pObject, FVPHYSICS_PART_OF_RAGDOLL );
	}

	if ( forcePosition != vec3_origin )
	{
		for ( i = 0; i < ragdoll.listCount; i++ )
		{
			if ( forceBone != i )
			{
				float scale = ragdoll.list[i].pObject->GetMass() / totalMass;
				ragdoll.list[i].pObject->ApplyForceOffset( scale * nudgeForce, forcePosition );
			}
		}
	}

	return true;
}


void RagdollApplyAnimationAsVelocity( ragdoll_t &ragdoll, const matrix3x4_t *pPrevBones, const matrix3x4_t *pCurrentBones, float dt )
{
	for ( int i = 0; i < ragdoll.listCount; i++ )
	{
		Vector velocity;
		AngularImpulse angVel;
		int boneIndex = ragdoll.boneIndex[i];
		CalcBoneDerivatives( velocity, angVel, pPrevBones[boneIndex], pCurrentBones[boneIndex], dt );
		
		AngularImpulse localAngVelocity;

		// Angular velocity is always applied in local space in vphysics
		ragdoll.list[i].pObject->WorldToLocalVector( &localAngVelocity, angVel );
		ragdoll.list[i].pObject->AddVelocity( &velocity, &localAngVelocity );
	}
}

void RagdollApplyAnimationAsVelocity( ragdoll_t &ragdoll, const matrix3x4_t *pBoneToWorld )
{
	for ( int i = 0; i < ragdoll.listCount; i++ )
	{
		matrix3x4_t inverse;
		MatrixInvert( pBoneToWorld[i], inverse );
		Quaternion q;
		Vector pos;
		MatrixAngles( inverse, q, pos );

		Vector velocity;
		AngularImpulse angVel;
		float flSpin;

		Vector localVelocity;
		AngularImpulse localAngVelocity;

		QuaternionAxisAngle( q, localAngVelocity, flSpin );
		localAngVelocity *= flSpin;
		localVelocity = pos;

		// move those bone-local coords back to world space using the ragdoll transform
		ragdoll.list[i].pObject->LocalToWorldVector( &velocity, localVelocity );

		ragdoll.list[i].pObject->AddVelocity( &velocity, &localAngVelocity );
	}
}


void RagdollDestroy( ragdoll_t &ragdoll )
{
	if ( !ragdoll.listCount )
		return;

	int i;
	for ( i = 0; i < ragdoll.listCount; i++ )
	{
		physenv->DestroyConstraint( ragdoll.list[i].pConstraint );
		ragdoll.list[i].pConstraint = NULL;
	}
	for ( i = 0; i < ragdoll.listCount; i++ )
	{
		// during level transitions these can get temporarily loaded without physics objects
		// purely for the purpose of testing for PVS of transition.  If they fail they get
		// deleted before the physics objects are loaded.  The list count will be nonzero
		// since that is saved separately.
		if ( ragdoll.list[i].pObject )
		{
			physenv->DestroyObject( ragdoll.list[i].pObject );
		}
		ragdoll.list[i].pObject = NULL;
	}
	physenv->DestroyConstraintGroup( ragdoll.pGroup );
	ragdoll.pGroup = NULL;
	ragdoll.listCount = 0;
}

// Parse the ragdoll and obtain the mapping from each physics element index to a bone index
// returns num phys elements
int RagdollExtractBoneIndices( int *boneIndexOut, CStudioHdr *pStudioHdr, vcollide_t *pCollide )
{
	int elementCount = 0;

	IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );
	while ( !pParse->Finished() )
	{
		const char *pBlock = pParse->GetCurrentBlockName();
		if ( !strcmpi( pBlock, "solid" ) )
		{
			solid_t solid;
			pParse->ParseSolid( &solid, NULL );
			if ( elementCount < RAGDOLL_MAX_ELEMENTS )
			{
				boneIndexOut[elementCount] = Studio_BoneIndexByName( pStudioHdr, solid.name );
				elementCount++;
			}
		}
		else
		{
			pParse->SkipBlock();
		}
	}
	physcollision->VPhysicsKeyParserDestroy( pParse );

	return elementCount;
}

bool RagdollGetBoneMatrix( const ragdoll_t &ragdoll, CBoneAccessor &pBoneToWorld, int objectIndex )
{
	int boneIndex = ragdoll.boneIndex[objectIndex];
	if ( boneIndex < 0 )
		return false;

	const ragdollelement_t &element = ragdoll.list[objectIndex];

	// during restore if a model has changed since the file was saved, this could be NULL
	if ( !element.pObject )
		return false;
	element.pObject->GetPositionMatrix( &pBoneToWorld.GetBoneForWrite( boneIndex ) );
	if ( element.parentIndex >= 0 && !ragdoll.allowStretch )
	{
		// overwrite the position from physics to force rigid attachment
		// UNDONE: If we support other types of constraints (or multiple constraints per object)
		// make sure these don't fight !
		int parentBoneIndex = ragdoll.boneIndex[element.parentIndex];
		Vector out;
		VectorTransform( element.originParentSpace, pBoneToWorld.GetBone( parentBoneIndex ), out );
		MatrixSetColumn( out, 3, pBoneToWorld.GetBoneForWrite( boneIndex ) );
	}
	return true;
}

void RagdollComputeExactBbox( const ragdoll_t &ragdoll, const Vector &origin, Vector &outMins, Vector &outMaxs )
{
	outMins = origin;
	outMaxs = origin;

	for ( int i = 0; i < ragdoll.listCount; i++ )
	{
		Vector mins, maxs;
		Vector objectOrg;
		QAngle objectAng;
		IPhysicsObject *pObject = ragdoll.list[i].pObject;
		pObject->GetPosition( &objectOrg, &objectAng );
		physcollision->CollideGetAABB( &mins, &maxs, pObject->GetCollide(), objectOrg, objectAng );
		for ( int j = 0; j < 3; j++ )
		{
			if ( mins[j] < outMins[j] )
			{
				outMins[j] = mins[j];
			}
			if ( maxs[j] > outMaxs[j] )
			{
				outMaxs[j] = maxs[j];
			}
		}
	}
}

bool RagdollIsAsleep( const ragdoll_t &ragdoll )
{
	for ( int i = 0; i < ragdoll.listCount; i++ )
	{
		if ( ragdoll.list[i].pObject && !ragdoll.list[i].pObject->IsAsleep() )
			return false;
	}

	return true;
}

void RagdollSolveSeparation( ragdoll_t &ragdoll, CBaseEntity *pEntity )
{
	byte needsFix[256];
	int fixCount = 0;
	Assert(ragdoll.listCount<=ARRAYSIZE(needsFix));
	for ( int i = 0; i < ragdoll.listCount; i++ )
	{
		needsFix[i] = 0;
		const ragdollelement_t &element = ragdoll.list[i];
		if ( element.pConstraint && element.parentIndex >= 0 )
		{
			Vector start, target;
			element.pObject->GetPosition( &start, NULL );
			ragdoll.list[element.parentIndex].pObject->LocalToWorld( &target, element.originParentSpace );
			if ( needsFix[element.parentIndex] )
			{
				needsFix[i] = 1;
				++fixCount;
				continue;
			}
			Vector dir = target-start;
			if ( dir.LengthSqr() > 1.0f )
			{
				// this fixes a bug in ep2 with antlion grubs, but causes problems in TF2 - revisit, but disable for TF now
#if !defined(TF_CLIENT_DLL)
				// heuristic: guess that anything separated and small mass ratio is in some state that's 
				// keeping the solver from fixing it
				float mass = element.pObject->GetMass();
				float massParent = ragdoll.list[element.parentIndex].pObject->GetMass();

				if ( mass*2.0f < massParent )
				{
					// if this is <0.5 mass of parent and still separated it's attached to something heavy or 
					// in a bad state
					needsFix[i] = 1;
					++fixCount;
					continue;
				}
#endif

				if ( PhysHasContactWithOtherInDirection(element.pObject, dir) )
				{
					Ray_t ray;
					trace_t tr;
					ray.Init( target, start );
					UTIL_TraceRay( ray, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tr );
					if ( tr.DidHit() )
					{
						needsFix[i] = 1;
						++fixCount;
					}
				}
			}
		}
	}

	if ( fixCount )
	{
		for ( int i = 0; i < ragdoll.listCount; i++ )
		{
			if ( !needsFix[i] )
				continue;

			const ragdollelement_t &element = ragdoll.list[i];
			Vector target, velocity;
			ragdoll.list[element.parentIndex].pObject->LocalToWorld( &target, element.originParentSpace );
			ragdoll.list[element.parentIndex].pObject->GetVelocityAtPoint( target, &velocity );
			matrix3x4_t xform;
			element.pObject->GetPositionMatrix( &xform );
			MatrixSetColumn( target, 3, xform );
			element.pObject->SetPositionMatrix( xform, true );
			element.pObject->SetVelocity( &velocity, &vec3_origin );
		}
		DevMsg(2, "TICK:%5d:Ragdoll separation count: %d\n", gpGlobals->tickcount, fixCount );
	}
	else
	{
		ragdoll.pGroup->ClearErrorState();
	}
}

//-----------------------------------------------------------------------------
// LRU
//-----------------------------------------------------------------------------
#ifdef _XBOX
// xbox defaults to 4 ragdolls max
ConVar g_ragdoll_maxcount("g_ragdoll_maxcount", "4", FCVAR_REPLICATED );
#else
ConVar g_ragdoll_maxcount("g_ragdoll_maxcount", "8", FCVAR_REPLICATED );
#endif
ConVar g_debug_ragdoll_removal("g_debug_ragdoll_removal", "0", FCVAR_REPLICATED |FCVAR_CHEAT );

CRagdollLRURetirement s_RagdollLRU( "CRagdollLRURetirement" );

void CRagdollLRURetirement::LevelInitPreEntity( void )
{
	m_iMaxRagdolls = -1;
	m_LRUImportantRagdolls.RemoveAll();
	m_LRU.RemoveAll();
}

bool ShouldRemoveThisRagdoll( CBaseAnimating *pRagdoll )
{
	if ( g_RagdollLVManager.IsLowViolence() )
	{
		return true;
	}

#ifdef CLIENT_DLL

	/* we no longer ignore enemies just because they are on fire -- a ragdoll in front of me
	   is always a higher priority for retention than a flaming zombie behind me. At the 
	   time I put this in, the ragdolls do clean up their own effects if culled via SUB_Remove().
	   If you're encountering trouble with ragdolls leaving effects behind, try renabling the code below.
    /////////////////////
	//Just ignore it until we're done burning/dissolving.
	if ( pRagdoll->GetEffectEntity() )
		return false;
	*/

	// Bail if we have a null ragdoll pointer.
	if ( !pRagdoll->m_pRagdoll )
		return true;

	Vector vMins, vMaxs;

	Vector origin = pRagdoll->m_pRagdoll->GetRagdollOrigin();
	pRagdoll->m_pRagdoll->GetRagdollBounds( vMins, vMaxs );

	if( engine->IsBoxInViewCluster( vMins + origin, vMaxs + origin) == false )
	{
		if ( g_debug_ragdoll_removal.GetBool() )
		{
			if ( debugoverlay )
			{
				debugoverlay->AddBoxOverlay( origin, vMins, vMaxs, QAngle( 0, 0, 0 ), 0, 255, 0, 16, 5 );
				debugoverlay->AddLineOverlay( origin, origin + Vector( 0, 0, 64 ), 0, 255, 0, true, 5 );
			}
		}

		return true;
	}
	else if( engine->CullBox( vMins + origin, vMaxs + origin ) == true )
	{
		if ( g_debug_ragdoll_removal.GetBool() )
		{
			if ( debugoverlay )
			{
				debugoverlay->AddBoxOverlay( origin, vMins, vMaxs, QAngle( 0, 0, 0 ), 0, 0, 255, 16, 5 );
				debugoverlay->AddLineOverlay( origin, origin + Vector( 0, 0, 64 ), 0, 0, 255, true, 5 );
			}
		}

		return true;
	}

#else
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	if( !UTIL_FindClientInPVS( pRagdoll->edict() ) )
	{
		if ( g_debug_ragdoll_removal.GetBool() )
			 NDebugOverlay::Line( pRagdoll->GetAbsOrigin(), pRagdoll->GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 255, 0, true, 5 );

		return true;
	}
	else if( !pPlayer->FInViewCone( pRagdoll ) )
	{
		if ( g_debug_ragdoll_removal.GetBool() )
			 NDebugOverlay::Line( pRagdoll->GetAbsOrigin(), pRagdoll->GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 0, 255, true, 5 );
		
		return true;
	}

#endif

	return false;
}




//-----------------------------------------------------------------------------
// Cull stale ragdolls. There is an ifdef here: one version for episodic, 
// one for everything else.
//-----------------------------------------------------------------------------
#if HL2_EPISODIC

void CRagdollLRURetirement::Update( float frametime ) // EPISODIC VERSION
{
	VPROF( "CRagdollLRURetirement::Update" );
	// Compress out dead items
	int i, next;

	int iMaxRagdollCount = m_iMaxRagdolls;

	if ( iMaxRagdollCount == -1 )
	{
		iMaxRagdollCount = g_ragdoll_maxcount.GetInt();
	}

	// fade them all for the low violence version
	if ( g_RagdollLVManager.IsLowViolence() )
	{
		iMaxRagdollCount = 0;
	}
	m_iRagdollCount = 0;
	m_iSimulatedRagdollCount = 0;

	// First, find ragdolls that are good candidates for deletion because they are not
	// visible at all, or are in a culled visibility box
	for ( i = m_LRU.Head(); i < m_LRU.InvalidIndex(); i = next )
	{
		next = m_LRU.Next(i);
		CBaseAnimating *pRagdoll = m_LRU[i].Get();
		if ( pRagdoll )
		{
			m_iRagdollCount++;
			IPhysicsObject *pObject = pRagdoll->VPhysicsGetObject();
			if (pObject && !pObject->IsAsleep())
			{
				m_iSimulatedRagdollCount++;
			}
			if ( m_LRU.Count() > iMaxRagdollCount )
			{
				//Found one, we're done.
				if ( ShouldRemoveThisRagdoll( m_LRU[i] ) == true )
				{
#ifdef CLIENT_DLL
					m_LRU[ i ]->SUB_Remove();
#else
					m_LRU[ i ]->SUB_StartFadeOut( 0 );
#endif

					m_LRU.Remove(i);
					return;
				}
			}
		}
		else 
		{
			m_LRU.Remove(i);
		}
	}

	//////////////////////////////
	///   EPISODIC ALGORITHM   ///
	//////////////////////////////
	// If we get here, it means we couldn't find a suitable ragdoll to remove,
	// so just remove the furthest one.
	int furthestOne = m_LRU.Head();
	float furthestDistSq = 0;
#ifdef CLIENT_DLL
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
#else
	CBasePlayer  *pPlayer = UTIL_GetLocalPlayer();
#endif

	if (pPlayer && m_LRU.Count() > iMaxRagdollCount) // find the furthest one algorithm
	{
		Vector PlayerOrigin = pPlayer->GetAbsOrigin();
		// const CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	
		for ( i = m_LRU.Head(); i < m_LRU.InvalidIndex(); i = next )
		{
			CBaseAnimating *pRagdoll = m_LRU[i].Get();

			next = m_LRU.Next(i);
			IPhysicsObject *pObject = pRagdoll->VPhysicsGetObject();
			if ( pRagdoll && (pRagdoll->GetEffectEntity() || ( pObject && !pObject->IsAsleep()) ) )
				continue;

			if ( pRagdoll )
			{
				// float distToPlayer = (pPlayer->GetAbsOrigin() - pRagdoll->GetAbsOrigin()).LengthSqr();
				float distToPlayer = (PlayerOrigin - pRagdoll->GetAbsOrigin()).LengthSqr();

				if (distToPlayer > furthestDistSq)
				{
					furthestOne = i;
					furthestDistSq = distToPlayer;
				}
			}
			else // delete bad rags first.
			{
				furthestOne = i;
				break;
			}
		}

#ifdef CLIENT_DLL
		m_LRU[ furthestOne ]->SUB_Remove();
#else
		m_LRU[ furthestOne ]->SUB_StartFadeOut( 0 );
#endif

	}
	else // fall back on old-style pick the oldest one algorithm
	{
		for ( i = m_LRU.Head(); i < m_LRU.InvalidIndex(); i = next )
		{
			if ( m_LRU.Count() <=  iMaxRagdollCount )
				break;

			next = m_LRU.Next(i);

			CBaseAnimating *pRagdoll = m_LRU[i].Get();

			//Just ignore it until we're done burning/dissolving.
			IPhysicsObject *pObject = pRagdoll->VPhysicsGetObject();
			if ( pRagdoll && (pRagdoll->GetEffectEntity() || ( pObject && !pObject->IsAsleep()) ) )
				continue;

	#ifdef CLIENT_DLL
			m_LRU[ i ]->SUB_Remove();
	#else
			m_LRU[ i ]->SUB_StartFadeOut( 0 );
	#endif
			m_LRU.Remove(i);
		}
	}
}

#else

void CRagdollLRURetirement::Update( float frametime ) // Non-episodic version
{
	VPROF( "CRagdollLRURetirement::Update" );
	// Compress out dead items
	int i, next;

	int iMaxRagdollCount = m_iMaxRagdolls;

	if ( iMaxRagdollCount == -1 )
	{
		iMaxRagdollCount = g_ragdoll_maxcount.GetInt();
	}

	// fade them all for the low violence version
	if ( g_RagdollLVManager.IsLowViolence() )
	{
		iMaxRagdollCount = 0;
	}
	m_iRagdollCount = 0;
	m_iSimulatedRagdollCount = 0;

	for ( i = m_LRU.Head(); i < m_LRU.InvalidIndex(); i = next )
	{
		next = m_LRU.Next(i);
		CBaseAnimating *pRagdoll = m_LRU[i].Get();
		if ( pRagdoll )
		{
			m_iRagdollCount++;
			IPhysicsObject *pObject = pRagdoll->VPhysicsGetObject();
			if (pObject && !pObject->IsAsleep())
			{
				m_iSimulatedRagdollCount++;
			}
			if ( m_LRU.Count() > iMaxRagdollCount )
			{
				//Found one, we're done.
				if ( ShouldRemoveThisRagdoll( m_LRU[i] ) == true )
				{
#ifdef CLIENT_DLL
					m_LRU[ i ]->SUB_Remove();
#else
					m_LRU[ i ]->SUB_StartFadeOut( 0 );
#endif

					m_LRU.Remove(i);
					return;
				}
			}
		}
		else 
		{
			m_LRU.Remove(i);
		}
	}


	//////////////////////////////
	///   ORIGINAL ALGORITHM   ///
	//////////////////////////////
	// not episodic -- this is the original mechanism

	for ( i = m_LRU.Head(); i < m_LRU.InvalidIndex(); i = next )
	{
		if ( m_LRU.Count() <=  iMaxRagdollCount )
			break;

		next = m_LRU.Next(i);

		CBaseAnimating *pRagdoll = m_LRU[i].Get();

		//Just ignore it until we're done burning/dissolving.
		if ( pRagdoll && pRagdoll->GetEffectEntity() )
			continue;

#ifdef CLIENT_DLL
		m_LRU[ i ]->SUB_Remove();
#else
		m_LRU[ i ]->SUB_StartFadeOut( 0 );
#endif
		m_LRU.Remove(i);
	}
}

#endif // HL2_EPISODIC

//This is pretty hacky, it's only called on the server so it just calls the update method.
void CRagdollLRURetirement::FrameUpdatePostEntityThink( void )
{
	Update( 0 );
}

ConVar g_ragdoll_important_maxcount( "g_ragdoll_important_maxcount", "2", FCVAR_REPLICATED );

//-----------------------------------------------------------------------------
// Move it to the top of the LRU
//-----------------------------------------------------------------------------
void CRagdollLRURetirement::MoveToTopOfLRU( CBaseAnimating *pRagdoll, bool bImportant )
{
	if ( bImportant )
	{
		m_LRUImportantRagdolls.AddToTail( pRagdoll );

		if ( m_LRUImportantRagdolls.Count() > g_ragdoll_important_maxcount.GetInt() )
		{
			int iIndex = m_LRUImportantRagdolls.Head();

			CBaseAnimating *pRagdollLRU = m_LRUImportantRagdolls[iIndex].Get();

			if ( pRagdollLRU )
			{
#ifdef CLIENT_DLL
				pRagdollLRU->SUB_Remove();
#else
				pRagdollLRU->SUB_StartFadeOut( 0 );
#endif
				m_LRUImportantRagdolls.Remove(iIndex);
			}

		}
		return;
	}
	for ( int i = m_LRU.Head(); i < m_LRU.InvalidIndex(); i = m_LRU.Next(i) )
	{
		if ( m_LRU[i].Get() == pRagdoll )
		{
			m_LRU.Remove(i);
			break;
		}
	}

	m_LRU.AddToTail( pRagdoll );
}


//EFFECT/ENTITY TRANSFERS

//CLIENT
#ifdef CLIENT_DLL

#define DEFAULT_FADE_START 2.0f
#define DEFAULT_MODEL_FADE_START 1.9f
#define DEFAULT_MODEL_FADE_LENGTH 0.1f
#define DEFAULT_FADEIN_LENGTH 1.0f



C_EntityDissolve *DissolveEffect( C_BaseEntity *pTarget, float flTime )
{
	C_EntityDissolve *pDissolve = new C_EntityDissolve;

	if ( pDissolve->InitializeAsClientEntity( "sprites/blueglow1.vmt", RENDER_GROUP_TRANSLUCENT_ENTITY ) == false )
	{
		pDissolve->Release();
		return NULL;
	}

	if ( pDissolve != NULL )
	{
		pTarget->AddFlag( FL_DISSOLVING );
		pDissolve->SetParent( pTarget );
		pDissolve->OnDataChanged( DATA_UPDATE_CREATED );
		pDissolve->SetAbsOrigin( pTarget->GetAbsOrigin() );

		pDissolve->m_flStartTime = flTime;
		pDissolve->m_flFadeOutStart = DEFAULT_FADE_START;
		pDissolve->m_flFadeOutModelStart = DEFAULT_MODEL_FADE_START;
		pDissolve->m_flFadeOutModelLength = DEFAULT_MODEL_FADE_LENGTH;
		pDissolve->m_flFadeInLength = DEFAULT_FADEIN_LENGTH;
		
		pDissolve->m_nDissolveType = 0;
		pDissolve->m_flNextSparkTime = 0.0f;
		pDissolve->m_flFadeOutLength = 0.0f;
		pDissolve->m_flFadeInStart = 0.0f;

		// Let this entity know it needs to delete itself when it's done
		pDissolve->SetServerLinkState( false );
		pTarget->SetEffectEntity( pDissolve );
	}

	return pDissolve;

}

C_EntityFlame *FireEffect( C_BaseAnimating *pTarget, C_BaseEntity *pServerFire, float *flScaleEnd, float *flTimeStart, float *flTimeEnd )
{
	C_EntityFlame *pFire = new C_EntityFlame;

	if ( pFire->InitializeAsClientEntity( NULL, RENDER_GROUP_TRANSLUCENT_ENTITY ) == false )
	{
		pFire->Release();
		return NULL;
	}

	if ( pFire != NULL )
	{
		pFire->RemoveFromLeafSystem();
		
		pTarget->AddFlag( FL_ONFIRE );
		pFire->SetParent( pTarget );
		pFire->m_hEntAttached = (C_BaseEntity *) pTarget;

		pFire->OnDataChanged( DATA_UPDATE_CREATED );
		pFire->SetAbsOrigin( pTarget->GetAbsOrigin() );

#ifdef HL2_EPISODIC
		if ( pServerFire )
		{
			if ( pServerFire->IsEffectActive(EF_DIMLIGHT) )
			{
				pFire->AddEffects( EF_DIMLIGHT );
			}
			if ( pServerFire->IsEffectActive(EF_BRIGHTLIGHT) )
			{
				pFire->AddEffects( EF_BRIGHTLIGHT );
			}
		}
#endif

		//Play a sound
		CPASAttenuationFilter filter( pTarget );
		pTarget->EmitSound( filter, pTarget->GetSoundSourceIndex(), "General.BurningFlesh" );

		pFire->SetNextClientThink( gpGlobals->curtime + 7.0f );
	}

	return pFire;
}

void C_BaseAnimating::IgniteRagdoll( C_BaseAnimating *pSource )
{
	C_BaseEntity *pChild = pSource->GetEffectEntity();
	
	if ( pChild )
	{
		C_EntityFlame *pFireChild = dynamic_cast<C_EntityFlame *>( pChild );
		C_ClientRagdoll *pRagdoll = dynamic_cast< C_ClientRagdoll * > ( this );

		if ( pFireChild )
		{
			pRagdoll->SetEffectEntity ( FireEffect( pRagdoll, pFireChild, NULL, NULL, NULL ) );
		}
	}
}



void C_BaseAnimating::TransferDissolveFrom( C_BaseAnimating *pSource )
{
	C_BaseEntity *pChild = pSource->GetEffectEntity();
	
	if ( pChild )
	{
		C_EntityDissolve *pDissolveChild = dynamic_cast<C_EntityDissolve *>( pChild );

		if ( pDissolveChild )
		{
			C_ClientRagdoll *pRagdoll = dynamic_cast< C_ClientRagdoll * > ( this );

			if ( pRagdoll )
			{
				pRagdoll->m_flEffectTime = pDissolveChild->m_flStartTime;

				C_EntityDissolve *pDissolve = DissolveEffect( pRagdoll, pRagdoll->m_flEffectTime );

				if ( pDissolve )
				{
					pDissolve->SetRenderMode( pDissolveChild->GetRenderMode() );
					pDissolve->m_nRenderFX = pDissolveChild->m_nRenderFX;
					pDissolve->SetRenderColor( 255, 255, 255, 255 );
					pDissolveChild->SetRenderColorA( 0 );

					pDissolve->m_vDissolverOrigin = pDissolveChild->m_vDissolverOrigin;
					pDissolve->m_nDissolveType = pDissolveChild->m_nDissolveType;

					if ( pDissolve->m_nDissolveType == ENTITY_DISSOLVE_CORE )
					{
						pDissolve->m_nMagnitude = pDissolveChild->m_nMagnitude;
						pDissolve->m_flFadeOutStart = CORE_DISSOLVE_FADE_START;
						pDissolve->m_flFadeOutModelStart = CORE_DISSOLVE_MODEL_FADE_START;
						pDissolve->m_flFadeOutModelLength = CORE_DISSOLVE_MODEL_FADE_LENGTH;
						pDissolve->m_flFadeInLength = CORE_DISSOLVE_FADEIN_LENGTH;
					}
				}
			}
		}
	}
}

#endif

//SERVER
#if !defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Transfer dissolve
//-----------------------------------------------------------------------------
void CBaseAnimating::TransferDissolveFrom( CBaseAnimating *pAnim )
{
	if ( !pAnim || !pAnim->IsDissolving() )
		return;

	CEntityDissolve *pDissolve = CEntityDissolve::Create( this, pAnim );
	if (pDissolve)
	{
		AddFlag( FL_DISSOLVING );
		m_flDissolveStartTime = pAnim->m_flDissolveStartTime;

		CEntityDissolve *pDissolveFrom = dynamic_cast < CEntityDissolve * > (pAnim->GetEffectEntity());

		if ( pDissolveFrom )
		{
			pDissolve->SetDissolverOrigin( pDissolveFrom->GetDissolverOrigin() );
			pDissolve->SetDissolveType( pDissolveFrom->GetDissolveType() );

			if ( pDissolveFrom->GetDissolveType() == ENTITY_DISSOLVE_CORE )
			{
				pDissolve->SetMagnitude( pDissolveFrom->GetMagnitude() );
				pDissolve->m_flFadeOutStart = CORE_DISSOLVE_FADE_START;
				pDissolve->m_flFadeOutModelStart = CORE_DISSOLVE_MODEL_FADE_START;
				pDissolve->m_flFadeOutModelLength = CORE_DISSOLVE_MODEL_FADE_LENGTH;
				pDissolve->m_flFadeInLength = CORE_DISSOLVE_FADEIN_LENGTH;
			}
		}
	}
}

#endif
