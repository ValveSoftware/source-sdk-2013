//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_weapon__stubs.h"
#include "c_basehlcombatweapon.h"
#include "fx.h"
#include "particles_localspace.h"
#include "view.h"
#include "particles_attractor.h"

class C_WeaponPhysCannon: public C_BaseHLCombatWeapon
{
	DECLARE_CLASS( C_WeaponPhysCannon, C_BaseHLCombatWeapon );
public:
	C_WeaponPhysCannon( void );

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual int DrawModel( int flags );
	virtual void ClientThink( void );

	virtual bool ShouldUseLargeViewModelVROverride() OVERRIDE { return true; }

private:

	bool	SetupEmitter( void );

	bool	m_bIsCurrentlyUpgrading;
	float	m_flTimeForceView;
	float	m_flTimeIgnoreForceView;
	bool	m_bWasUpgraded;

	CSmartPtr<CLocalSpaceEmitter>	m_pLocalEmitter;
	CSmartPtr<CSimpleEmitter>		m_pEmitter;
	CSmartPtr<CParticleAttractor>	m_pAttractor;
};

STUB_WEAPON_CLASS_IMPLEMENT( weapon_physcannon, C_WeaponPhysCannon );

IMPLEMENT_CLIENTCLASS_DT( C_WeaponPhysCannon, DT_WeaponPhysCannon, CWeaponPhysCannon )
	RecvPropBool( RECVINFO( m_bIsCurrentlyUpgrading ) ),
	RecvPropFloat( RECVINFO( m_flTimeForceView) ), 
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_WeaponPhysCannon::C_WeaponPhysCannon( void )
{
	m_bWasUpgraded = false;
	m_flTimeIgnoreForceView = -1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_WeaponPhysCannon::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_WeaponPhysCannon::SetupEmitter( void )
{
	if ( !m_pLocalEmitter.IsValid() )
	{
		m_pLocalEmitter = CLocalSpaceEmitter::Create( "physpowerup", GetRefEHandle(), LookupAttachment( "core" ) );

		if ( m_pLocalEmitter.IsValid() == false )
			return false;
	}

	if ( !m_pAttractor.IsValid() )
	{
		m_pAttractor = CParticleAttractor::Create( vec3_origin, "physpowerup_att" );

		if ( m_pAttractor.IsValid() == false )
			return false;
	}

	if ( !m_pEmitter.IsValid() )
	{
		m_pEmitter = CSimpleEmitter::Create( "physpowerup_glow" );

		if ( m_pEmitter.IsValid() == false )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Sorts the components of a vector
//-----------------------------------------------------------------------------
static inline void SortAbsVectorComponents( const Vector& src, int* pVecIdx )
{
	Vector absVec( fabs(src[0]), fabs(src[1]), fabs(src[2]) );

	int maxIdx = (absVec[0] > absVec[1]) ? 0 : 1;
	if (absVec[2] > absVec[maxIdx])
	{
		maxIdx = 2;
	}

	// always choose something right-handed....
	switch(	maxIdx )
	{
	case 0:
		pVecIdx[0] = 1;
		pVecIdx[1] = 2;
		pVecIdx[2] = 0;
		break;
	case 1:
		pVecIdx[0] = 2;
		pVecIdx[1] = 0;
		pVecIdx[2] = 1;
		break;
	case 2:
		pVecIdx[0] = 0;
		pVecIdx[1] = 1;
		pVecIdx[2] = 2;
		break;
	}
}

//-----------------------------------------------------------------------------
// Compute the bounding box's center, size, and basis
//-----------------------------------------------------------------------------
void ComputeRenderInfo( mstudiobbox_t *pHitBox, const matrix3x4_t &hitboxToWorld, 
										 Vector *pVecAbsOrigin, Vector *pXVec, Vector *pYVec )
{
	// Compute the center of the hitbox in worldspace
	Vector vecHitboxCenter;
	VectorAdd( pHitBox->bbmin, pHitBox->bbmax, vecHitboxCenter );
	vecHitboxCenter *= 0.5f;
	VectorTransform( vecHitboxCenter, hitboxToWorld, *pVecAbsOrigin );

	// Get the object's basis
	Vector vec[3];
	MatrixGetColumn( hitboxToWorld, 0, vec[0] );
	MatrixGetColumn( hitboxToWorld, 1, vec[1] );
	MatrixGetColumn( hitboxToWorld, 2, vec[2] );
//	vec[1] *= -1.0f;

	Vector vecViewDir;
	VectorSubtract( CurrentViewOrigin(), *pVecAbsOrigin, vecViewDir );
	VectorNormalize( vecViewDir );

	// Project the shadow casting direction into the space of the hitbox
	Vector localViewDir;
	localViewDir[0] = DotProduct( vec[0], vecViewDir );
	localViewDir[1] = DotProduct( vec[1], vecViewDir );
	localViewDir[2] = DotProduct( vec[2], vecViewDir );

	// Figure out which vector has the largest component perpendicular
	// to the view direction...
	// Sort by how perpendicular it is
	int vecIdx[3];
	SortAbsVectorComponents( localViewDir, vecIdx );

	// Here's our hitbox basis vectors; namely the ones that are
	// most perpendicular to the view direction
	*pXVec = vec[vecIdx[0]];
	*pYVec = vec[vecIdx[1]];

	// Project them into a plane perpendicular to the view direction
	*pXVec -= vecViewDir * DotProduct( vecViewDir, *pXVec );
	*pYVec -= vecViewDir * DotProduct( vecViewDir, *pYVec );
	VectorNormalize( *pXVec );
	VectorNormalize( *pYVec );

	// Compute the hitbox size
	Vector boxSize;
	VectorSubtract( pHitBox->bbmax, pHitBox->bbmin, boxSize );

	// We project the two longest sides into the vectors perpendicular
	// to the projection direction, then add in the projection of the perp direction
	Vector2D size( boxSize[vecIdx[0]], boxSize[vecIdx[1]] );
	size.x *= fabs( DotProduct( vec[vecIdx[0]], *pXVec ) );
	size.y *= fabs( DotProduct( vec[vecIdx[1]], *pYVec ) );

	// Add the third component into x and y
	size.x += boxSize[vecIdx[2]] * fabs( DotProduct( vec[vecIdx[2]], *pXVec ) );
	size.y += boxSize[vecIdx[2]] * fabs( DotProduct( vec[vecIdx[2]], *pYVec ) );

	// Bloat a bit, since the shadow wants to extend outside the model a bit
	size *= 2.0f;

	// Clamp the minimum size
	Vector2DMax( size, Vector2D(10.0f, 10.0f), size );

	// Factor the size into the xvec + yvec
	(*pXVec) *= size.x * 0.5f;
	(*pYVec) *= size.y * 0.5f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
// Output : int
//-----------------------------------------------------------------------------
int C_WeaponPhysCannon::DrawModel( int flags )
{
	// If we're not ugrading, don't do anything special
	if ( m_bIsCurrentlyUpgrading == false && m_bWasUpgraded == false )
		return BaseClass::DrawModel( flags );

	if ( gpGlobals->frametime == 0 )
		return BaseClass::DrawModel( flags );

	if ( !m_bReadyToDraw )
		return 0;

	m_bWasUpgraded = true;

	// Create the particle emitter if it's not already
	if ( SetupEmitter() )
	{
		// Add the power-up particles

		// See if we should draw
		if ( m_bReadyToDraw == false )
			return 0;

		C_BaseAnimating *pAnimating = GetBaseAnimating();
		if (!pAnimating)
			return 0;

		matrix3x4_t	*hitboxbones[MAXSTUDIOBONES];
		if ( !pAnimating->HitboxToWorldTransforms( hitboxbones ) )
			return 0;

		studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( pAnimating->GetModel() );
		if (!pStudioHdr)
			return false;

		mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pAnimating->GetHitboxSet() );
		if ( !set )
			return false;

		int i;

		float fadePerc = 1.0f;

		if ( m_bIsCurrentlyUpgrading )
		{
			Vector	vecSkew = vec3_origin;

			// Skew the particles in front or in back of their targets
			vecSkew = CurrentViewForward() * 4.0f;

			float spriteScale = 1.0f;
			spriteScale = clamp( spriteScale, 0.75f, 1.0f );

			SimpleParticle *sParticle;

			for ( i = 0; i < set->numhitboxes; ++i )
			{
				Vector vecAbsOrigin, xvec, yvec;
				mstudiobbox_t *pBox = set->pHitbox(i);
				ComputeRenderInfo( pBox, *hitboxbones[pBox->bone], &vecAbsOrigin, &xvec, &yvec );

				Vector offset;
				Vector	xDir, yDir;

				xDir = xvec;
				float xScale = VectorNormalize( xDir ) * 0.75f;

				yDir = yvec;
				float yScale = VectorNormalize( yDir ) * 0.75f;

				int numParticles = clamp( 4.0f * fadePerc, 1, 3 );

				for ( int j = 0; j < numParticles; j++ )
				{
					offset = xDir * Helper_RandomFloat( -xScale*0.5f, xScale*0.5f ) + yDir * Helper_RandomFloat( -yScale*0.5f, yScale*0.5f );
					offset += vecSkew;

					sParticle = (SimpleParticle *) m_pEmitter->AddParticle( sizeof(SimpleParticle), m_pEmitter->GetPMaterial( "effects/combinemuzzle1" ), vecAbsOrigin + offset );

					if ( sParticle == NULL )
						return 1;
					
					sParticle->m_vecVelocity	= vec3_origin;
					sParticle->m_uchStartSize	= 16.0f * spriteScale;
					sParticle->m_flDieTime		= 0.2f;
					sParticle->m_flLifetime		= 0.0f;

					sParticle->m_flRoll			= Helper_RandomInt( 0, 360 );
					sParticle->m_flRollDelta	= Helper_RandomFloat( -2.0f, 2.0f );

					float alpha = 40;

					sParticle->m_uchColor[0]	= alpha;
					sParticle->m_uchColor[1]	= alpha;
					sParticle->m_uchColor[2]	= alpha;
					sParticle->m_uchStartAlpha	= alpha;
					sParticle->m_uchEndAlpha	= 0;
					sParticle->m_uchEndSize		= sParticle->m_uchStartSize * 2;
				}
			}
		}
	}

	int		attachment = LookupAttachment( "core" );
	Vector	coreOrigin;
	QAngle	coreAngles;

	GetAttachment( attachment, coreOrigin, coreAngles );

	SimpleParticle *sParticle;

	// Do the core effects
	for ( int i = 0; i < 4; i++ )
	{
		sParticle = (SimpleParticle *) m_pLocalEmitter->AddParticle( sizeof(SimpleParticle), m_pLocalEmitter->GetPMaterial( "effects/strider_muzzle" ), vec3_origin );

		if ( sParticle == NULL )
			return 1;
		
		sParticle->m_vecVelocity	= vec3_origin;
		sParticle->m_flDieTime		= 0.1f;
		sParticle->m_flLifetime		= 0.0f;

		sParticle->m_flRoll			= Helper_RandomInt( 0, 360 );
		sParticle->m_flRollDelta	= 0.0f;

		float alpha = 255;

		sParticle->m_uchColor[0]	= alpha;
		sParticle->m_uchColor[1]	= alpha;
		sParticle->m_uchColor[2]	= alpha;
		sParticle->m_uchStartAlpha	= alpha;
		sParticle->m_uchEndAlpha	= 0;

		if ( i < 2 )
		{
			sParticle->m_uchStartSize	= random->RandomFloat( 1, 2 ) * (i+1);
			sParticle->m_uchEndSize		= sParticle->m_uchStartSize * 2.0f;
		}
		else
		{
			if ( random->RandomInt( 0, 20 ) == 0 )
			{
				sParticle->m_uchStartSize	= random->RandomFloat( 1, 2 ) * (i+1);
				sParticle->m_uchEndSize		= sParticle->m_uchStartSize * 4.0f;
				sParticle->m_flDieTime		= 0.25f;
			}
			else
			{
				sParticle->m_uchStartSize	= random->RandomFloat( 1, 2 ) * (i+1);
				sParticle->m_uchEndSize		= sParticle->m_uchStartSize * 2.0f;
			}
		}
	}

	if ( m_bWasUpgraded && m_bIsCurrentlyUpgrading )
	{
		// Update our attractor point
		m_pAttractor->SetAttractorOrigin( coreOrigin );

		Vector offset;

		for ( int i = 0; i < 4; i++ )
		{
			offset = coreOrigin + RandomVector( -32.0f, 32.0f );

			sParticle = (SimpleParticle *) m_pAttractor->AddParticle( sizeof(SimpleParticle), m_pAttractor->GetPMaterial( "effects/strider_muzzle" ), offset );

			if ( sParticle == NULL )
				return 1;
			
			sParticle->m_vecVelocity	= Vector(0,0,8);
			sParticle->m_flDieTime		= 0.5f;
			sParticle->m_flLifetime		= 0.0f;

			sParticle->m_flRoll			= Helper_RandomInt( 0, 360 );
			sParticle->m_flRollDelta	= 0.0f;

			float alpha = 255;

			sParticle->m_uchColor[0]	= alpha;
			sParticle->m_uchColor[1]	= alpha;
			sParticle->m_uchColor[2]	= alpha;
			sParticle->m_uchStartAlpha	= alpha;
			sParticle->m_uchEndAlpha	= 0;

			sParticle->m_uchStartSize	= random->RandomFloat( 1, 2 );
			sParticle->m_uchEndSize		= 0;
		}
	}

	return BaseClass::DrawModel( flags );
}

//---------------------------------------------------------
// On 360, raise up the player's view if the server has
// asked us to.
//---------------------------------------------------------
#define PHYSCANNON_RAISE_VIEW_GOAL	0.0f
void C_WeaponPhysCannon::ClientThink( void )
{
	if( m_flTimeIgnoreForceView > gpGlobals->curtime )
		return;

	float flTime = (m_flTimeForceView - gpGlobals->curtime);
	
	if( flTime < 0.0f )
		return;

	float flDT = 1.0f - flTime;
	if( flDT > 0.0f )
	{
		QAngle viewangles;
		engine->GetViewAngles( viewangles );

		if( viewangles.x > PHYSCANNON_RAISE_VIEW_GOAL + 1.0f )
		{
			float flDelta = PHYSCANNON_RAISE_VIEW_GOAL - viewangles.x;
			viewangles.x += (flDelta * flDT);
			engine->SetViewAngles(viewangles);
		}
		else
		{
			// We've reached our goal. Ignore the forced view angles for now.
			m_flTimeIgnoreForceView = m_flTimeForceView + 0.1f;
		}
	}

	return BaseClass::ClientThink();
}


