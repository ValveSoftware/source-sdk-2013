//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "particles_simple.h"
#include "baseparticleentity.h"
#include "iefx.h"
#include "decals.h"
#include "beamdraw.h"
#include "hud.h"
#include "clienteffectprecachesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheExtinguisher )
CLIENTEFFECT_MATERIAL( "particle/particle_smokegrenade" )
CLIENTEFFECT_REGISTER_END()

class C_ExtinguisherJet : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_ExtinguisherJet, C_BaseEntity );

	C_ExtinguisherJet();
	~C_ExtinguisherJet();

	void	OnDataChanged( DataUpdateType_t updateType );
	void	Update( float fTimeDelta );
	void	Start( void );
	int		DrawModel( int flags );
	bool	ShouldDraw( void ) { return m_bEmit; }

protected:

	void		AddExtinguisherDecal( trace_t &tr );

	bool		m_bEmit;
	bool		m_bUseMuzzlePoint;
	int			m_nLength;
	int			m_nSize;

	PMaterialHandle	m_MaterialHandle;
	PMaterialHandle	m_EmberMaterialHandle;
	TimedEvent		m_ParticleSpawn;
	CSmartPtr<CSimpleEmitter> m_pEmitter;
	CSmartPtr<CEmberEffect> m_pEmberEmitter;

private:
	C_ExtinguisherJet( const C_ExtinguisherJet & );
};

//Datatable
IMPLEMENT_CLIENTCLASS_DT( C_ExtinguisherJet, DT_ExtinguisherJet, CExtinguisherJet )
	RecvPropInt(RECVINFO(m_bEmit), 0),
	RecvPropInt(RECVINFO(m_bUseMuzzlePoint), 0),
	RecvPropInt(RECVINFO(m_nLength), 0),
	RecvPropInt(RECVINFO(m_nSize), 0),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_ExtinguisherJet::C_ExtinguisherJet( void )
{
	m_bEmit			= false;

	m_pEmitter		= NULL;
	m_pEmberEmitter	= NULL;
}

C_ExtinguisherJet::~C_ExtinguisherJet( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_ExtinguisherJet::OnDataChanged( DataUpdateType_t updateType )
{
	C_BaseEntity::OnDataChanged(updateType);

	if( updateType == DATA_UPDATE_CREATED )
	{
		Start();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ExtinguisherJet::Start( void )
{
	AddToLeafSystem( RENDER_GROUP_TRANSLUCENT_ENTITY );

	m_ParticleSpawn.Init( 100 ); //Events per second

	//Create the basic emitter
	m_pEmitter = CSimpleEmitter::Create("C_ExtinguisherJet::m_pEmitter");
	
	Assert( m_pEmitter.IsValid() );
	if ( m_pEmitter.IsValid() )
	{
		m_MaterialHandle = g_Mat_DustPuff[0];
		m_pEmitter->SetSortOrigin( GetAbsOrigin() );
	}

	//Create the "ember" emitter for the smaller flecks
	m_pEmberEmitter = CEmberEffect::Create( "C_ExtinguisherJet::m_pEmberEmitter" );

	Assert( m_pEmberEmitter.IsValid() );
	if ( m_pEmberEmitter.IsValid() )
	{
		m_EmberMaterialHandle = g_Mat_DustPuff[0];
		m_pEmberEmitter->SetSortOrigin( GetAbsOrigin() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ExtinguisherJet::AddExtinguisherDecal( trace_t &tr )
{
	C_BaseEntity *ent = cl_entitylist->GetEnt( 0 );
	
	if ( ent != NULL )
	{
		int	index = decalsystem->GetDecalIndexForName( "Extinguish" );
		if ( index >= 0 )
		{
			Vector	endpos;
			endpos.Random( -24.0f, 24.0f );
			endpos += tr.endpos;
	
			effects->DecalShoot( index, 0, ent->GetModel(), ent->GetAbsOrigin(), ent->GetAbsAngles(), endpos, 0, 0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_ExtinguisherJet::Update( float fTimeDelta )
{
	if ( m_bEmit == false )
		return;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();

	if ( m_bUseMuzzlePoint )
	{
		C_BaseViewModel *vm = player ? player->GetViewModel( 0 ) : NULL;

		if ( vm )
		{
			int iAttachment = vm->LookupAttachment( "muzzle" );
			Vector origin;
			QAngle angles;
			vm->GetAttachment( iAttachment, origin, angles );

			Assert( !GetMoveParent() );
			SetLocalOrigin( origin );
			SetLocalAngles( angles );
		}
	}

	trace_t	tr;
	Vector	shotDir, vRight, vUp;

	AngleVectors( GetAbsAngles(), &shotDir, &vRight, &vUp );
	
	//FIXME: Muzzle point is incorrect on the model!
	if ( m_bUseMuzzlePoint )
	{
		shotDir.Negate();
	}

	Vector	endPoint = GetAbsOrigin() + ( shotDir * 150.0f );
	
	UTIL_TraceLine( GetAbsOrigin(), endPoint, MASK_SHOT, NULL, COLLISION_GROUP_NONE, &tr );

	bool	hitWall = ( tr.fraction < 1.0f );

	//Add normal jet
	if ( m_pEmitter.IsValid() )
	{
		SimpleParticle	*pParticle;

		m_pEmitter->SetSortOrigin( GetAbsOrigin() );
	
		float tempDelta = fTimeDelta;
		
		//FIXME: All particles need to be within this loop
		while( m_ParticleSpawn.NextEvent( tempDelta ) )
		{
			pParticle = (SimpleParticle *) m_pEmitter->AddParticle( sizeof(SimpleParticle), m_MaterialHandle, GetAbsOrigin() );

			if ( pParticle )
			{
				pParticle->m_flDieTime	= 0.2f;
				pParticle->m_flLifetime	= 0.0f;
				
				pParticle->m_flRoll		= random->RandomInt( 0, 360 );
				pParticle->m_flRollDelta= random->RandomFloat( -4.0f, 4.0f );
				
				pParticle->m_uchStartSize	= 1;
				pParticle->m_uchEndSize		= random->RandomInt( 32, 48 );
				pParticle->m_uchStartAlpha	= random->RandomInt( 128, 164 );
				pParticle->m_uchEndAlpha	= 0;
				
				int	cScale = random->RandomInt( 192, 255 );
				pParticle->m_uchColor[0]	= cScale;
				pParticle->m_uchColor[1]	= cScale;
				pParticle->m_uchColor[2]	= cScale;

				Vector	dir;
				QAngle  ofsAngles;

				ofsAngles.Random( -8.0f, 8.0f );
				ofsAngles += GetAbsAngles();

				AngleVectors( ofsAngles, &dir );

				if ( m_bUseMuzzlePoint )
				{
					dir.Negate();
				}

				pParticle->m_vecVelocity	= dir * random->RandomInt( 400, 800 );
			}

			//Add muzzle effect
			pParticle = (SimpleParticle *) m_pEmitter->AddParticle( sizeof(SimpleParticle), m_MaterialHandle, GetAbsOrigin() );

			if ( pParticle )
			{
				pParticle->m_flDieTime	= 0.1f;
				pParticle->m_flLifetime	= 0.0f;
				
				pParticle->m_flRoll		= random->RandomInt( 0, 360 );
				pParticle->m_flRollDelta= random->RandomFloat( -4.0f, 4.0f );
				
				pParticle->m_uchStartSize	= 1;
				pParticle->m_uchEndSize		= random->RandomInt( 8, 16 );
				pParticle->m_uchStartAlpha	= random->RandomInt( 128, 255 );
				pParticle->m_uchEndAlpha	= 0;
				
				int	cScale = random->RandomInt( 192, 255 );
				pParticle->m_uchColor[0]	= cScale;
				pParticle->m_uchColor[1]	= cScale;
				pParticle->m_uchColor[2]	= cScale;

				Vector	dir;
				QAngle  ofsAngles;

				ofsAngles.Random( -64.0f, 64.0f );
				ofsAngles += GetAbsAngles();

				AngleVectors( ofsAngles, &dir );

				if ( m_bUseMuzzlePoint )
				{
					dir.Negate();
				}

				pParticle->m_vecVelocity	= dir * random->RandomInt( 32, 64 );
			}

			//Add a wall effect if needed
			if ( hitWall )
			{
				AddExtinguisherDecal( tr );

				Vector	offDir;

				offDir.Random( -16.0f, 16.0f );

				pParticle = (SimpleParticle *) m_pEmitter->AddParticle( sizeof(SimpleParticle), m_MaterialHandle, ( tr.endpos + ( tr.plane.normal * 8.0f ) ) + offDir );

				if ( pParticle )
				{
					pParticle->m_flDieTime	= 0.4f;
					pParticle->m_flLifetime	= 0.0f;
					
					pParticle->m_flRoll		= random->RandomInt( 0, 360 );
					pParticle->m_flRollDelta= random->RandomFloat( -2.0f, 2.0f );
					
					pParticle->m_uchStartSize	= random->RandomInt( 8, 16 );
					pParticle->m_uchEndSize		= random->RandomInt( 24, 32 );
					pParticle->m_uchStartAlpha	= random->RandomInt( 64, 128 );
					pParticle->m_uchEndAlpha	= 0;
					
					int	cScale = random->RandomInt( 192, 255 );
					pParticle->m_uchColor[0]	= cScale;
					pParticle->m_uchColor[1]	= cScale;
					pParticle->m_uchColor[2]	= cScale;

					Vector	rDir;

					rDir = tr.plane.normal;
					rDir[0] += random->RandomFloat( -0.9f, 0.9f );
					rDir[1] += random->RandomFloat( -0.9f, 0.9f );
					rDir[2] += random->RandomFloat( -0.9f, 0.9f );

					pParticle->m_vecVelocity = rDir * random->RandomInt( 32, 64 );
				}			
			}

			//Add small ember-like particles
			if ( random->RandomInt( 0, 1 ) == 0 )
			{
				m_pEmberEmitter->SetSortOrigin( GetAbsOrigin() );

				pParticle = (SimpleParticle *) m_pEmberEmitter->AddParticle( sizeof(SimpleParticle), g_Mat_DustPuff[0], GetAbsOrigin() );
				
				assert(pParticle);

				if ( pParticle )
				{
					pParticle->m_flLifetime		= 0.0f;
					pParticle->m_flDieTime		= 1.0f;

					pParticle->m_flRoll			= 0;
					pParticle->m_flRollDelta	= 0;

					pParticle->m_uchColor[0]	= 255;
					pParticle->m_uchColor[1]	= 255;
					pParticle->m_uchColor[2]	= 255;
					pParticle->m_uchStartAlpha	= 255;
					pParticle->m_uchEndAlpha	= 0;
					pParticle->m_uchStartSize	= 1;
					pParticle->m_uchEndSize		= 0;
					
					Vector	dir;
					QAngle  ofsAngles;

					ofsAngles.Random( -8.0f, 8.0f );
					ofsAngles += GetAbsAngles();

					AngleVectors( ofsAngles, &dir );

					if ( m_bUseMuzzlePoint )
					{
						dir.Negate();
					}

					pParticle->m_vecVelocity	= dir * random->RandomInt( 400, 800 );
				}
			}
		}
	}

	// Inner beam

	CBeamSegDraw	beamDraw;
	CBeamSeg		seg;
	const int		numPoints = 4;
	Vector			beamPoints[numPoints];

	beamPoints[0] = GetAbsOrigin();

	// Create our beam points
	int i;
	for ( i = 0; i < numPoints; i++ )
	{
		beamPoints[i] = GetAbsOrigin() + ( shotDir * (32*i*i) );

		beamPoints[i] += vRight * sin( gpGlobals->curtime * 4.0f ) * (2.0f*i);
		beamPoints[i] += vUp * sin( gpGlobals->curtime * 8.0f ) * (1.0f*i);
		beamPoints[i] += shotDir * sin( gpGlobals->curtime * (16.0f*i) ) * (1.0f*i);
	}

	IMaterial *pMat = materials->FindMaterial( "particle/particle_smokegrenade", TEXTURE_GROUP_PARTICLE );

	beamDraw.Start( numPoints, pMat );

	//Setup and draw those points	
	for( i = 0; i < numPoints; i++ )
	{
		float	t = (float) i / (numPoints - 1);
		float	color = 1.0f * (1.0f - t);

		seg.m_vColor		= Vector( color, color, color );
		seg.m_vPos			= beamPoints[i];
		seg.m_flTexCoord	= (float)i/(float)(numPoints-1) - ((gpGlobals->curtime - (int)gpGlobals->curtime) * 4.0f );
		seg.m_flWidth		= 4.0f + ( (64.0f*t) * (fabs( sin( gpGlobals->curtime * 16.0f ) )) );
		seg.m_flAlpha		= color;

		beamDraw.NextSeg( &seg );
	}
	
	beamDraw.End();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
//-----------------------------------------------------------------------------
int C_ExtinguisherJet::DrawModel( int flags )
{
	if ( m_bEmit == false )
		return 1;

	Update( Helper_GetFrameTime() );

	return 1;
}
