//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements an explosion entity and a support spark shower entity.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "decals.h"
#include "explode.h"
#include "ai_basenpc.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "tier1/strtools.h"
#include "shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Spark shower, created by the explosion entity.
//-----------------------------------------------------------------------------
class CShower : public CPointEntity
{
public:
	DECLARE_CLASS( CShower, CPointEntity );

	void Spawn( void );
	void Think( void );
	void Touch( CBaseEntity *pOther );
	int ObjectCaps( void ) { return FCAP_DONT_SAVE; }
};

LINK_ENTITY_TO_CLASS( spark_shower, CShower );


void CShower::Spawn( void )
{
	Vector vecForward;
	AngleVectors( GetLocalAngles(), &vecForward );

	Vector vecNewVelocity;
	vecNewVelocity = random->RandomFloat( 200, 300 ) * vecForward;
	vecNewVelocity.x += random->RandomFloat(-100.f,100.f);
	vecNewVelocity.y += random->RandomFloat(-100.f,100.f);
	if ( vecNewVelocity.z >= 0 )
		vecNewVelocity.z += 200;
	else
		vecNewVelocity.z -= 200;
	SetAbsVelocity( vecNewVelocity );

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetGravity( UTIL_ScaleForGravity( 400 ) ); // fall a bit more slowly than normal
	SetNextThink( gpGlobals->curtime + 0.1f );
	SetSolid( SOLID_NONE );
	UTIL_SetSize(this, vec3_origin, vec3_origin );
	AddEffects( EF_NODRAW );
	m_flSpeed = random->RandomFloat( 0.5, 1.5 );

	SetLocalAngles( vec3_angle );
}


void CShower::Think( void )
{
	g_pEffects->Sparks( GetAbsOrigin() );

	m_flSpeed -= 0.1;
	if ( m_flSpeed > 0 )
		SetNextThink( gpGlobals->curtime + 0.1f );
	else
		UTIL_Remove( this );
	SetGroundEntity( NULL );
}


void CShower::Touch( CBaseEntity *pOther )
{
	Vector vecNewVelocity = GetAbsVelocity();

	if ( GetFlags() & FL_ONGROUND )
		vecNewVelocity *= 0.1;
	else
		vecNewVelocity *= 0.6;

	if ( (vecNewVelocity.x*vecNewVelocity.x+vecNewVelocity.y*vecNewVelocity.y) < 10.0 )
		m_flSpeed = 0;

	SetAbsVelocity( vecNewVelocity );
}


class CEnvExplosion : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvExplosion, CPointEntity );

	CEnvExplosion( void )
	{
		// Default to invalid.
		m_sFireballSprite = -1;
	};

	void Precache( void );
	void Spawn( );
	void Smoke ( void );
	void SetCustomDamageType( int iType ) { m_iCustomDamageType = iType; }
	bool KeyValue( const char *szKeyName, const char *szValue );

	int DrawDebugTextOverlays(void);

	// Input handlers
	void InputExplode( inputdata_t &inputdata );

	DECLARE_DATADESC();

	int m_iMagnitude;// how large is the fireball? how much damage?
	int m_iRadiusOverride;// For use when m_iMagnitude results in larger radius than designer desires.
	int m_spriteScale; // what's the exact fireball sprite scale? 
	float m_flDamageForce;	// How much damage force should we use?
	string_t m_iszFireballSprite;
	short m_sFireballSprite;
	EHANDLE m_hInflictor;
	int m_iCustomDamageType;

	// passed along to the RadiusDamage call
	int m_iClassIgnore;
	EHANDLE m_hEntityIgnore;

};

LINK_ENTITY_TO_CLASS( env_explosion, CEnvExplosion );

BEGIN_DATADESC( CEnvExplosion )

	DEFINE_KEYFIELD( m_iMagnitude, FIELD_INTEGER, "iMagnitude" ),
	DEFINE_KEYFIELD( m_iRadiusOverride, FIELD_INTEGER, "iRadiusOverride" ),
	DEFINE_FIELD( m_spriteScale, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_flDamageForce, FIELD_FLOAT, "DamageForce" ),
	DEFINE_FIELD( m_iszFireballSprite, FIELD_STRING ),
	DEFINE_FIELD( m_sFireballSprite, FIELD_SHORT ),
	DEFINE_FIELD( m_hInflictor, FIELD_EHANDLE ),
	DEFINE_FIELD( m_iCustomDamageType, FIELD_INTEGER ),

	DEFINE_FIELD( m_iClassIgnore, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_hEntityIgnore, FIELD_EHANDLE, "ignoredEntity" ),

	// Function Pointers
	DEFINE_THINKFUNC( Smoke ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Explode", InputExplode),

END_DATADESC()


bool CEnvExplosion::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "fireballsprite"))
	{
		m_iszFireballSprite = AllocPooledString( szValue );
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

void CEnvExplosion::Precache( void )
{
	if ( m_iszFireballSprite != NULL_STRING )
	{
		m_sFireballSprite = PrecacheModel( STRING( m_iszFireballSprite ) );
	}
}

void CEnvExplosion::Spawn( void )
{ 
	Precache();

	SetSolid( SOLID_NONE );
	AddEffects( EF_NODRAW );

	SetMoveType( MOVETYPE_NONE );
	/*
	if ( m_iMagnitude > 250 )
	{
		m_iMagnitude = 250;
	}
	*/

	float flSpriteScale;
	flSpriteScale = ( m_iMagnitude - 50) * 0.6;

	// Control the clamping of the fireball sprite
	if( m_spawnflags & SF_ENVEXPLOSION_NOCLAMPMIN )
	{
		// Don't inhibit clamping altogether. Just relax it a bit.
		if ( flSpriteScale < 1 )
		{
			flSpriteScale = 1;
		}
	}
	else
	{
		if ( flSpriteScale < 10 )
		{
			flSpriteScale = 10;
		}
	}

	if( m_spawnflags & SF_ENVEXPLOSION_NOCLAMPMAX )
	{
		// We may need to adjust this to suit designers' needs.
		if ( flSpriteScale > 200 )
		{
			flSpriteScale = 200;
		}
	}
	else
	{
		if ( flSpriteScale > 50 )
		{
			flSpriteScale = 50;
		}
	}

	m_spriteScale = (int)flSpriteScale;
	m_iCustomDamageType = -1;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for making the explosion explode.
//-----------------------------------------------------------------------------
void CEnvExplosion::InputExplode( inputdata_t &inputdata )
{ 
	trace_t tr;

	SetModelName( NULL_STRING );//invisible
	SetSolid( SOLID_NONE );// intangible

	Vector vecSpot = GetAbsOrigin() + Vector( 0 , 0 , 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), (MASK_SOLID_BRUSHONLY | MASK_WATER), this, COLLISION_GROUP_NONE, &tr );
	
	// Pull out of the wall a bit. We used to move the explosion origin itself, but that seems unnecessary, not to mention a
	// little weird when you consider that it might be in hierarchy. Instead we just calculate a new virtual position at
	// which to place the explosion. We don't use that new position to calculate radius damage because according to Steve's
	// comment, that adversely affects the force vector imparted on explosion victims when they ragdoll.
	Vector vecExplodeOrigin = GetAbsOrigin();
	if ( tr.fraction != 1.0 )
	{
		vecExplodeOrigin = tr.endpos + (tr.plane.normal * 24 );
	}

	// draw decal
	if (! ( m_spawnflags & SF_ENVEXPLOSION_NODECAL))
	{
		UTIL_DecalTrace( &tr, "Scorch" );
	}

	// It's stupid that this entity's spawnflags and the flags for the
	// explosion temp ent don't match up. But because they don't, we
	// have to reinterpret some of the spawnflags to determine which
	// flags to pass to the temp ent.
	int nFlags = TE_EXPLFLAG_NONE;

	if( m_spawnflags & SF_ENVEXPLOSION_NOFIREBALL )
	{
		nFlags |= TE_EXPLFLAG_NOFIREBALL;
	}
	
	if( m_spawnflags & SF_ENVEXPLOSION_NOSOUND )
	{
		nFlags |= TE_EXPLFLAG_NOSOUND;
	}
	
	if ( m_spawnflags & SF_ENVEXPLOSION_RND_ORIENT )
	{
		nFlags |= TE_EXPLFLAG_ROTATE;
	}

	if ( m_nRenderMode == kRenderTransAlpha )
	{
		nFlags |= TE_EXPLFLAG_DRAWALPHA;
	}
	else if ( m_nRenderMode != kRenderTransAdd )
	{
		nFlags |= TE_EXPLFLAG_NOADDITIVE;
	}

	if( m_spawnflags & SF_ENVEXPLOSION_NOPARTICLES )
	{
		nFlags |= TE_EXPLFLAG_NOPARTICLES;
	}

	if( m_spawnflags & SF_ENVEXPLOSION_NODLIGHTS )
	{
		nFlags |= TE_EXPLFLAG_NODLIGHTS;
	}

	if ( m_spawnflags & SF_ENVEXPLOSION_NOFIREBALLSMOKE )
	{
		nFlags |= TE_EXPLFLAG_NOFIREBALLSMOKE;
	}

	//Get the damage override if specified
	int	iRadius = ( m_iRadiusOverride > 0 ) ? m_iRadiusOverride : ( m_iMagnitude * 2.5f );

	CPASFilter filter( vecExplodeOrigin );
	te->Explosion( filter, 0.0,
		&vecExplodeOrigin, 
		( m_sFireballSprite < 1 ) ? g_sModelIndexFireball : m_sFireballSprite,
		!( m_spawnflags & SF_ENVEXPLOSION_NOFIREBALL ) ? ( m_spriteScale / 10.0 ) : 0.0,
		15,
		nFlags,
		iRadius,
		m_iMagnitude );

	// do damage
	if ( !( m_spawnflags & SF_ENVEXPLOSION_NODAMAGE ) )
	{
		CBaseEntity *pAttacker = GetOwnerEntity() ? GetOwnerEntity() : this;

		// Only calculate damage type if we didn't get a custom one passed in
		int iDamageType = m_iCustomDamageType;
		if ( iDamageType == -1 )
		{
			iDamageType = HasSpawnFlags( SF_ENVEXPLOSION_GENERIC_DAMAGE ) ? DMG_GENERIC : DMG_BLAST;
		}

		CTakeDamageInfo info( m_hInflictor ? m_hInflictor : this, pAttacker, m_iMagnitude, iDamageType );

		if( HasSpawnFlags( SF_ENVEXPLOSION_SURFACEONLY ) )
		{
			info.AddDamageType( DMG_BLAST_SURFACE );
		}

		if ( m_flDamageForce )
		{
			// Not the right direction, but it'll be fixed up by RadiusDamage.
			info.SetDamagePosition( GetAbsOrigin() );
			info.SetDamageForce( Vector( m_flDamageForce, 0, 0 ) );
		}

		RadiusDamage( info, GetAbsOrigin(), iRadius, m_iClassIgnore, m_hEntityIgnore.Get() );
	}

	SetThink( &CEnvExplosion::Smoke );
	SetNextThink( gpGlobals->curtime + 0.3 );

	// Only do these effects if we're not submerged
	if ( UTIL_PointContents( GetAbsOrigin() ) & CONTENTS_WATER )
	{
		// draw sparks
		if ( !( m_spawnflags & SF_ENVEXPLOSION_NOSPARKS ) )
		{
			int sparkCount = random->RandomInt(0,3);

			for ( int i = 0; i < sparkCount; i++ )
			{
				QAngle angles;
				VectorAngles( tr.plane.normal, angles );
				Create( "spark_shower", vecExplodeOrigin, angles, NULL );
			}
		}
	}
}


void CEnvExplosion::Smoke( void )
{
	if ( !(m_spawnflags & SF_ENVEXPLOSION_REPEATABLE) )
	{
		UTIL_Remove( this );
	}
}


// HACKHACK -- create one of these and fake a keyvalue to get the right explosion setup
void ExplosionCreate( const Vector &center, const QAngle &angles, 
	CBaseEntity *pOwner, int magnitude, int radius, int nSpawnFlags, float flExplosionForce, CBaseEntity *pInflictor, int iCustomDamageType,
	const EHANDLE *ignoredEntity , Class_T ignoredClass )
{
	char			buf[128];

	CEnvExplosion *pExplosion = (CEnvExplosion*)CBaseEntity::Create( "env_explosion", center, angles, pOwner );
	Q_snprintf( buf,sizeof(buf), "%3d", magnitude );
	char *szKeyName = "iMagnitude";
	char *szValue = buf;
	pExplosion->KeyValue( szKeyName, szValue );

	pExplosion->AddSpawnFlags( nSpawnFlags );

	if ( radius )
	{
		Q_snprintf( buf,sizeof(buf), "%d", radius );
		pExplosion->KeyValue( "iRadiusOverride", buf );
	}

	if ( flExplosionForce != 0.0f )
	{
		Q_snprintf( buf,sizeof(buf), "%.3f", flExplosionForce );
		pExplosion->KeyValue( "DamageForce", buf );
	}

	variant_t emptyVariant;
	pExplosion->m_nRenderMode = kRenderTransAdd;
	pExplosion->SetOwnerEntity( pOwner );
	pExplosion->Spawn();
	pExplosion->m_hInflictor = pInflictor;
	pExplosion->SetCustomDamageType( iCustomDamageType );
	if (ignoredEntity)
	{
		pExplosion->m_hEntityIgnore = *ignoredEntity;
	}
	pExplosion->m_iClassIgnore = ignoredClass;

	pExplosion->AcceptInput( "Explode", NULL, NULL, emptyVariant, 0 );
}


void ExplosionCreate( const Vector &center, const QAngle &angles, 
	CBaseEntity *pOwner, int magnitude, int radius, bool doDamage, float flExplosionForce, bool bSurfaceOnly, bool bSilent, int iCustomDamageType )
{
	// For E3, no sparks
	int nFlags = SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE;
	if ( !doDamage )
	{
		nFlags |= SF_ENVEXPLOSION_NODAMAGE;
	}

	if( bSurfaceOnly )
	{
		nFlags |= SF_ENVEXPLOSION_SURFACEONLY;
	}

	if( bSilent )
	{
		nFlags |= SF_ENVEXPLOSION_NOSOUND;
	}

	ExplosionCreate( center, angles, pOwner, magnitude, radius, nFlags, flExplosionForce, NULL, iCustomDamageType );
}

// this version lets you specify classes or entities to be ignored
void ExplosionCreate( const Vector &center, const QAngle &angles, 
					 CBaseEntity *pOwner, int magnitude, int radius, bool doDamage, 
					 const EHANDLE *ignoredEntity, Class_T ignoredClass,
					 float flExplosionForce , bool bSurfaceOnly , bool bSilent , int iCustomDamageType )
{
	// For E3, no sparks
	int nFlags = SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE;
	if ( !doDamage )
	{
		nFlags |= SF_ENVEXPLOSION_NODAMAGE;
	}

	if( bSurfaceOnly )
	{
		nFlags |= SF_ENVEXPLOSION_SURFACEONLY;
	}

	if( bSilent )
	{
		nFlags |= SF_ENVEXPLOSION_NOSOUND;
	}

	ExplosionCreate( center, angles, pOwner, magnitude, radius, nFlags, flExplosionForce, NULL, iCustomDamageType, ignoredEntity, ignoredClass );
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CEnvExplosion::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf(tempstr,sizeof(tempstr),"    magnitude: %i", m_iMagnitude);
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}
