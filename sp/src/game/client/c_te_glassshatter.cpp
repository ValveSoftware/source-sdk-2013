//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "particle_simple3d.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "fx.h"
#include "tier0/vprof.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PI 3.14159265359
#define GLASS_SHARD_MIN_LIFE 2
#define GLASS_SHARD_MAX_LIFE 5
#define GLASS_SHARD_NOISE	 0.3
#define GLASS_SHARD_GRAVITY  500
#define GLASS_SHARD_DAMPING	 0.3

#include "clienteffectprecachesystem.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectGlassShatter )
CLIENTEFFECT_MATERIAL( "effects/fleck_glass1" )
CLIENTEFFECT_MATERIAL( "effects/fleck_glass2" )
CLIENTEFFECT_MATERIAL( "effects/fleck_tile1" )
CLIENTEFFECT_MATERIAL( "effects/fleck_tile2" )
CLIENTEFFECT_REGISTER_END()

//###################################################
// > C_TEShatterSurface
//###################################################
class C_TEShatterSurface : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEShatterSurface, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	C_TEShatterSurface( void );
	~C_TEShatterSurface( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

private:
	// Recording 
	void RecordShatterSurface( );

public:
	Vector					m_vecOrigin;
	QAngle					m_vecAngles;
	Vector					m_vecForce;
	Vector					m_vecForcePos;
	float					m_flWidth;
	float					m_flHeight;
	float					m_flShardSize;
	PMaterialHandle			m_pMaterialHandle;
	int						m_nSurfaceType;
	byte					m_uchFrontColor[3];
	byte					m_uchBackColor[3];
};


//------------------------------------------------------------------------------
// Networking
//------------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEShatterSurface, DT_TEShatterSurface, CTEShatterSurface)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropVector( RECVINFO(m_vecAngles)),
	RecvPropVector( RECVINFO(m_vecForce)),
	RecvPropVector( RECVINFO(m_vecForcePos)),
	RecvPropFloat( RECVINFO(m_flWidth)),
	RecvPropFloat( RECVINFO(m_flHeight)),
	RecvPropFloat( RECVINFO(m_flShardSize)),
	RecvPropInt( RECVINFO(m_nSurfaceType)),	
	RecvPropInt( RECVINFO(m_uchFrontColor[0])),
	RecvPropInt( RECVINFO(m_uchFrontColor[1])),
	RecvPropInt( RECVINFO(m_uchFrontColor[2])),
	RecvPropInt( RECVINFO(m_uchBackColor[0])),
	RecvPropInt( RECVINFO(m_uchBackColor[1])),
	RecvPropInt( RECVINFO(m_uchBackColor[2])),
END_RECV_TABLE()


//------------------------------------------------------------------------------
// Constructor, destructor
//------------------------------------------------------------------------------
C_TEShatterSurface::C_TEShatterSurface( void )
{
	m_vecOrigin.Init();
	m_vecAngles.Init();
	m_vecForce.Init();
	m_vecForcePos.Init();
	m_flWidth			= 16.0;
	m_flHeight			= 16.0;
	m_flShardSize		= 3;
	m_nSurfaceType		= SHATTERSURFACE_GLASS;
	m_uchFrontColor[0]	= 255;
	m_uchFrontColor[1]	= 255;
	m_uchFrontColor[2]	= 255;
	m_uchBackColor[0]	= 255;
	m_uchBackColor[1]	= 255;
	m_uchBackColor[2]	= 255;
}

C_TEShatterSurface::~C_TEShatterSurface()
{
}


//-----------------------------------------------------------------------------
// Recording 
//-----------------------------------------------------------------------------
void C_TEShatterSurface::RecordShatterSurface( )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		Color front( m_uchFrontColor[0], m_uchFrontColor[1], m_uchFrontColor[2], 255 );
		Color back( m_uchBackColor[0], m_uchBackColor[1], m_uchBackColor[2], 255 );

		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_SHATTER_SURFACE );
 		msg->SetString( "name", "TE_ShatterSurface" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", m_vecOrigin.x );
		msg->SetFloat( "originy", m_vecOrigin.y );
		msg->SetFloat( "originz", m_vecOrigin.z );
		msg->SetFloat( "anglesx", m_vecAngles.x );
		msg->SetFloat( "anglesy", m_vecAngles.y );
		msg->SetFloat( "anglesz", m_vecAngles.z );
		msg->SetFloat( "forcex", m_vecForce.x );
		msg->SetFloat( "forcey", m_vecForce.y );
		msg->SetFloat( "forcez", m_vecForce.z );
		msg->SetFloat( "forceposx", m_vecForcePos.x );
		msg->SetFloat( "forceposy", m_vecForcePos.y );
		msg->SetFloat( "forceposz", m_vecForcePos.z );
		msg->SetColor( "frontcolor", front );
		msg->SetColor( "backcolor", back );
		msg->SetFloat( "width", m_flWidth );
		msg->SetFloat( "height", m_flHeight );
		msg->SetFloat( "size", m_flShardSize );
		msg->SetInt( "surfacetype", m_nSurfaceType );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEShatterSurface::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEShatterSurface::PostDataUpdate" );

	RecordShatterSurface();

	CSmartPtr<CSimple3DEmitter> pGlassEmitter = CSimple3DEmitter::Create( "C_TEShatterSurface 1" );
	pGlassEmitter->SetSortOrigin( m_vecOrigin );

	Vector vecColor;
	engine->ComputeLighting( m_vecOrigin, NULL, true, vecColor );

	// HACK: Blend a little toward white to match the materials...
	VectorLerp( vecColor, Vector( 1, 1, 1 ), 0.3, vecColor );

	PMaterialHandle *hMaterial;
	if (m_nSurfaceType == SHATTERSURFACE_GLASS)
	{
		hMaterial = g_Mat_Fleck_Glass;
	}
	else
	{
		hMaterial = g_Mat_Fleck_Tile;
	}

	// ---------------------------------------------------
	// Figure out number of particles required to fill space
	// ---------------------------------------------------
	int nNumWide = m_flWidth  / m_flShardSize;
	int nNumHigh = m_flHeight / m_flShardSize;

	Vector vWidthStep,vHeightStep;
	AngleVectors(m_vecAngles,NULL,&vWidthStep,&vHeightStep);
	vWidthStep	*= m_flShardSize;
	vHeightStep *= m_flShardSize;

	// ---------------------
	// Create glass shards
	// ----------------------
	Vector vCurPos = m_vecOrigin;
	vCurPos.x += 0.5*m_flShardSize;
	vCurPos.z += 0.5*m_flShardSize;

	float flMinSpeed = 9999999999.0f;
	float flMaxSpeed = 0;

	Particle3D *pParticle = NULL;

	for (int width=0;width<nNumWide;width++)
	{
		for (int height=0;height<nNumHigh;height++)
		{			
			pParticle = (Particle3D *) pGlassEmitter->AddParticle( sizeof(Particle3D), hMaterial[random->RandomInt(0,1)], vCurPos );

			Vector vForceVel = Vector(0,0,0);
			if (random->RandomInt(0, 3) != 0)
			{
				float flForceDistSqr = (vCurPos - m_vecForcePos).LengthSqr();
				vForceVel = m_vecForce;
				if (flForceDistSqr > 0 )
				{
					vForceVel *= ( 40.0f / flForceDistSqr );
				}
			}

			if (pParticle)
			{
				pParticle->m_flLifeRemaining	= random->RandomFloat(GLASS_SHARD_MIN_LIFE,GLASS_SHARD_MAX_LIFE);
				pParticle->m_vecVelocity		= vForceVel;
				pParticle->m_vecVelocity	   += RandomVector(-25,25);
				pParticle->m_uchSize			= m_flShardSize + random->RandomFloat(-0.5*m_flShardSize,0.5*m_flShardSize);
				pParticle->m_vAngles			= m_vecAngles;
				pParticle->m_flAngSpeed			= random->RandomFloat(-400,400);

				pParticle->m_uchFrontColor[0]	= (byte)(m_uchFrontColor[0] * vecColor.x );
				pParticle->m_uchFrontColor[1]	= (byte)(m_uchFrontColor[1] * vecColor.y );
				pParticle->m_uchFrontColor[2]	= (byte)(m_uchFrontColor[2] * vecColor.z );
				pParticle->m_uchBackColor[0]	= (byte)(m_uchBackColor[0] * vecColor.x );
				pParticle->m_uchBackColor[1]	= (byte)(m_uchBackColor[1] * vecColor.y );
				pParticle->m_uchBackColor[2]	= (byte)(m_uchBackColor[2] * vecColor.z );
			}

			// Keep track of min and max speed for collision detection
			float  flForceSpeed = vForceVel.Length();
			if (flForceSpeed > flMaxSpeed)
			{
				flMaxSpeed = flForceSpeed;
			}
			if (flForceSpeed < flMinSpeed)
			{
				flMinSpeed = flForceSpeed;
			}

			vCurPos += vHeightStep;
		}
		vCurPos	 -= nNumHigh*vHeightStep;
		vCurPos	 += vWidthStep;
	}

	// --------------------------------------------------
	// Set collision parameters
	// --------------------------------------------------
	Vector vMoveDir = m_vecForce;
	VectorNormalize(vMoveDir);

	pGlassEmitter->m_ParticleCollision.Setup( m_vecOrigin, &vMoveDir, GLASS_SHARD_NOISE, 
												flMinSpeed, flMaxSpeed, GLASS_SHARD_GRAVITY, GLASS_SHARD_DAMPING );
}

void TE_ShatterSurface( IRecipientFilter& filter, float delay,
	const Vector* pos, const QAngle* angle, const Vector* vForce, const Vector* vForcePos, 
	float width, float height, float shardsize, ShatterSurface_t surfacetype,
	int front_r, int front_g, int front_b, int back_r, int back_g, int back_b)
{
	// Major hack to simulate receiving network message
	__g_C_TEShatterSurface.m_vecOrigin = *pos;
	__g_C_TEShatterSurface.m_vecAngles = *angle;
	__g_C_TEShatterSurface.m_vecForce = *vForce;
	__g_C_TEShatterSurface.m_vecForcePos = *vForcePos;
	__g_C_TEShatterSurface.m_flWidth = width;
	__g_C_TEShatterSurface.m_flHeight = height;
	__g_C_TEShatterSurface.m_flShardSize = shardsize;
	__g_C_TEShatterSurface.m_nSurfaceType = surfacetype;
	__g_C_TEShatterSurface.m_uchFrontColor[0] = front_r;
	__g_C_TEShatterSurface.m_uchFrontColor[1] = front_g;
	__g_C_TEShatterSurface.m_uchFrontColor[2] = front_b;
	__g_C_TEShatterSurface.m_uchBackColor[0] = back_r;
	__g_C_TEShatterSurface.m_uchBackColor[1] = back_g;
	__g_C_TEShatterSurface.m_uchBackColor[2] = back_b;

	__g_C_TEShatterSurface.PostDataUpdate( DATA_UPDATE_CREATED );
}

void TE_ShatterSurface( IRecipientFilter& filter, float delay, KeyValues *pKeyValues )
{
	Vector vecOrigin, vecForce, vecForcePos;
	QAngle angles;
	vecOrigin.x = pKeyValues->GetFloat( "originx" );
	vecOrigin.y = pKeyValues->GetFloat( "originy" );
	vecOrigin.z = pKeyValues->GetFloat( "originz" );
	angles.x = pKeyValues->GetFloat( "anglesx" );
	angles.y = pKeyValues->GetFloat( "anglesy" );
	angles.z = pKeyValues->GetFloat( "anglesz" );
	vecForce.x = pKeyValues->GetFloat( "forcex" );
	vecForce.y = pKeyValues->GetFloat( "forcey" );
	vecForce.z = pKeyValues->GetFloat( "forcez" );
	vecForcePos.x = pKeyValues->GetFloat( "forceposx" );
	vecForcePos.y = pKeyValues->GetFloat( "forceposy" );
	vecForcePos.z = pKeyValues->GetFloat( "forceposz" );
	Color front = pKeyValues->GetColor( "frontcolor" );
	Color back = pKeyValues->GetColor( "backcolor" );
	float flWidth = pKeyValues->GetFloat( "width" );
	float flHeight = pKeyValues->GetFloat( "height" );
	float flSize = pKeyValues->GetFloat( "size" );
	ShatterSurface_t nSurfaceType = (ShatterSurface_t)pKeyValues->GetInt( "surfacetype" );
	TE_ShatterSurface( filter, 0.0f, &vecOrigin, &angles, &vecForce, &vecForcePos,
		flWidth, flHeight, flSize, nSurfaceType, front.r(), front.g(), front.b(),
		back.r(), back.g(), back.b() );
}
