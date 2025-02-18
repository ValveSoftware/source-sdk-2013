//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "viewangleanim.h"
#include "KeyValues.h"

#include "tier0/memdbgon.h"

extern ConVar cl_pitchdown;
extern ConVar cl_pitchup;


// ConCommands useful for creating view animations
CViewAngleAnimation *g_pTestAnimation = NULL;

// create a view animation object to be used for creating an animation. parameter is flags
CON_COMMAND( viewanim_create, "viewanim_create" )
{
	if ( g_pTestAnimation )
	{
		delete g_pTestAnimation;
		g_pTestAnimation = NULL;
	}

	int flags = 0;
	if ( args.ArgC() > 1 )
	{
		flags = atoi( args[1] );
	}

	g_pTestAnimation = CREATE_ENTITY( CViewAngleAnimation, "viewangleanim" );

	if ( g_pTestAnimation )
	{
		g_pTestAnimation->Spawn();
	}	
}

// run the test animation
void TestViewAnim( void )
{
	if ( g_pTestAnimation )
	{
		QAngle angles;
		engine->GetViewAngles( angles );

		g_pTestAnimation->RunAnimation( angles );
	}
	else
		Msg( "No view anim created\n" );
}
ConCommand viewanim_test( "viewanim_test", TestViewAnim, "test view animation" );

// set view angles to (0,0,0)
void ResetViewAngles( void )
{
	// create a blank anim
	QAngle angles = vec3_angle;
	engine->SetViewAngles( angles );
}
ConCommand viewanim_reset( "viewanim_reset", ResetViewAngles, "reset view angles!", FCVAR_CHEAT );

// add a key frame to the test animation. first parameter is the time taken to get to this keyframe
CON_COMMAND_F( viewanim_addkeyframe, "", FCVAR_CHEAT )
{
	if ( g_pTestAnimation )
	{
		QAngle vecTarget;
		engine->GetViewAngles( vecTarget );

		float flDelay = 0.2;
		if (args.ArgC() > 1)
		{
			flDelay = atof( args[1] );
		}

		int iFlags = 0;
		if (args.ArgC() > 1)
		{
			iFlags = atof( args[2] );
		}

		g_pTestAnimation->AddKeyFrame( new CViewAngleKeyFrame( vecTarget, flDelay, iFlags ) );
	}
	else
		Msg( "No view anim created, use viewanim_create" );
}


// save the current test anim, pass filename
CON_COMMAND( viewanim_save, "Save current animation to file" )
{
	if (args.ArgC() < 2)
		return;

	if ( g_pTestAnimation )
	{	
		char szOutput[ MAX_PATH ];
		V_FixupPathName( szOutput, sizeof(szOutput), args[1] );
		g_pTestAnimation->SaveAsAnimFile( szOutput );
	}
	else
	{
		Msg( "No view anim created\n" );
	}
}

// load a view animation file into the test anim
CON_COMMAND( viewanim_load, "load animation from file" )
{
	if (args.ArgC() < 2)
		return;

	if ( g_pTestAnimation )
	{	
		g_pTestAnimation->LoadViewAnimFile( args[1] );
	}
	else
		Msg( "No view anim created\n" );
}

LINK_ENTITY_TO_CLASS( viewangleanim, CViewAngleAnimation );

CViewAngleAnimation::CViewAngleAnimation()
{
}

CViewAngleAnimation::~CViewAngleAnimation()
{
	DeleteKeyFrames();
}

void CViewAngleAnimation::Spawn( void )
{
	m_iFlags = 0;
	QAngle angles;
	engine->GetViewAngles( angles );	

	/*
	if ( m_iFlags & VIEWANIM_RELATIVE )
	{
		AddKeyFrame( new CViewAngleKeyFrame( vec3_angle, 0.0, 0 ) );

		// seed this so we can add keyframes and have them calc the delta properly
		m_vecBaseAngles = angles;
	}
	else
	{
		AddKeyFrame( new CViewAngleKeyFrame( angles, 0.0, 0 ) );
	}
	*/

	m_bFinished = true;	// don't run right away

	ClientEntityList().AddNonNetworkableEntity(	this );
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

void CViewAngleAnimation::DeleteKeyFrames()
{
	int i, c;

	c = m_KeyFrames.Count();
	for ( i = c - 1; i >= 0 ; --i )
	{
		delete m_KeyFrames[ i ];
	}
	m_KeyFrames.RemoveAll();
}

void CViewAngleAnimation::LoadViewAnimFile( const char *pKeyFrameFileName )
{
	DeleteKeyFrames();

	// load keyvalues from this file and stuff them in as keyframes
	KeyValues *pData = new KeyValues( pKeyFrameFileName );

	if ( false == pData->LoadFromFile( filesystem, pKeyFrameFileName, "GAME" ) )
	{
		Warning( "CViewAngleAnimation::LoadViewAnimFile failed to load script %s\n", pKeyFrameFileName );
		pData->deleteThis();
		return;
	}

	QAngle angles;
	float flTime;
	int iFlags;

	KeyValues *pKey = pData->GetFirstSubKey();

	while ( pKey )
	{
		// angles
		const char *pszAngles = pKey->GetString( "angles", "0 0 0" );
		sscanf( pszAngles, "%f %f %f", &angles[0], &angles[1], &angles[2] );

		// time
		flTime = pKey->GetFloat( "time", 0.001 );

		// flags
		iFlags = pKey->GetInt( "flags", 0 );

		AddKeyFrame( new CViewAngleKeyFrame( angles, flTime, iFlags ) );

		pKey = pKey->GetNextKey();
	}

	pData->deleteThis();
}

void CViewAngleAnimation::SaveAsAnimFile( const char *pKeyFrameFileName )
{
	// save all of our keyframes into the file
	KeyValues *pData = new KeyValues( pKeyFrameFileName );

	pData->SetInt( "flags", m_iFlags );

	KeyValues *pKey = new KeyValues( "keyframe" );
	int i;
	int c = m_KeyFrames.Count();
	char buf[64];
	for ( i=0;i<c;i++ )
	{
		pKey = pData->CreateNewKey();

		Q_snprintf( buf, sizeof(buf), "%f %f %f",
			m_KeyFrames[i]->m_vecAngles[0],
			m_KeyFrames[i]->m_vecAngles[1],
			m_KeyFrames[i]->m_vecAngles[2] );

		pKey->SetString( "angles", buf );
		pKey->SetFloat( "time", m_KeyFrames[i]->m_flTime );
		pKey->SetInt( "flags", m_KeyFrames[i]->m_iFlags );
	}

	pData->SaveToFile( filesystem, pKeyFrameFileName, NULL );
	pData->deleteThis();
}

void CViewAngleAnimation::AddKeyFrame( CViewAngleKeyFrame *pKeyFrame )
{
	pKeyFrame->m_vecAngles -= m_vecBaseAngles;
	m_KeyFrames.AddToTail( pKeyFrame );
}

bool CViewAngleAnimation::IsFinished( void )
{
	return m_bFinished;
}

void CViewAngleAnimation::RunAnimation( QAngle angles )
{
	if ( m_KeyFrames.Count() == 0 )
	{
		Warning( "CViewAngleAnimation::RunAnimation called on an empty view animation\n" );
		return;
	}

	m_flAnimStartTime = gpGlobals->curtime;
	m_bFinished = false;
	m_vecBaseAngles = angles;

	m_iFlags = m_KeyFrames[0]->m_iFlags;

	if ( !( m_iFlags & VIEWANIM_RELATIVE ) )
	{
		m_KeyFrames[0]->m_vecAngles = angles;
	}
}

void CViewAngleAnimation::ClientThink()
{
	if ( IsFinished() )
		return;

	float flCurrentTime = gpGlobals->curtime - m_flAnimStartTime;

	if ( flCurrentTime < 0 )
		flCurrentTime = 0.001;

	// find two nearest points
	int i, c;
	c = m_KeyFrames.Count();
	float flTime = 0;
	for ( i=0;i<c;i++ )
	{
		if ( flTime + m_KeyFrames[i]->m_flTime > flCurrentTime )
		{
			break;
		}

		flTime += m_KeyFrames[i]->m_flTime;
	}

	Assert( i > 0 );

	if ( i >= c )
	{
		if ( i > 0 )
		{
			// animation complete, set to end point
			SetAngles( m_KeyFrames[i-1]->m_vecAngles );
		}

		if ( m_pAnimCompleteCallback )
		{
			m_pAnimCompleteCallback();
		}

		m_bFinished = true;
		return;
	}

	if ( m_KeyFrames[i]->m_iFlags != m_iFlags )
	{
		if ( ( m_KeyFrames[i]->m_iFlags & VIEWANIM_RELATIVE ) && !( m_iFlags & VIEWANIM_RELATIVE ) )
		{
            // new relative position is current angles
			engine->GetViewAngles( m_vecBaseAngles );
		}

		// copy the rest over
		m_iFlags = m_KeyFrames[i]->m_iFlags;
	}

	// previous frame is m_KeyFrames[i-1];
	// next frame is m_KeyFrames[i];
	float flFraction = ( flCurrentTime - flTime ) / ( m_KeyFrames[i]->m_flTime );

	Vector v0, v1, v2, v3;

	if ( i-2 <= 0 )
	{
		QAngleToVector( m_KeyFrames[i-1]->m_vecAngles, v0 );
	}
	else
	{
		QAngleToVector( m_KeyFrames[i-2]->m_vecAngles, v0 );
	}

	QAngleToVector( m_KeyFrames[i-1]->m_vecAngles, v1 );
	QAngleToVector( m_KeyFrames[i]->m_vecAngles, v2 );

	if ( i+1 >= c )
	{
		QAngleToVector( m_KeyFrames[i]->m_vecAngles, v3 );
	}
	else
	{
		QAngleToVector( m_KeyFrames[i+1]->m_vecAngles, v3 );
	}

	Vector out;
	Catmull_Rom_Spline( v0,	v1, v2,	v3,	flFraction,	out );

	QAngle vecCalculatedAngles;
	QAngleToVector( out, vecCalculatedAngles );
	SetAngles( vecCalculatedAngles );
}

void CViewAngleAnimation::SetAngles( QAngle vecCalculatedAngles )
{
	if ( m_iFlags & VIEWANIM_RELATIVE )
		vecCalculatedAngles += m_vecBaseAngles;

	QAngle vecViewAngle;
	engine->GetViewAngles( vecViewAngle );

	if ( !(FBitSet( m_iFlags, VIEWANIM_IGNORE_X ) ) )
		vecViewAngle[PITCH] = vecCalculatedAngles[PITCH];

	if ( !(FBitSet( m_iFlags, VIEWANIM_IGNORE_Y ) ) )
		vecViewAngle[YAW] = vecCalculatedAngles[YAW];

	if ( !(FBitSet( m_iFlags, VIEWANIM_IGNORE_Z ) ) )
		vecViewAngle[ROLL] = vecCalculatedAngles[ROLL];

	// clamp pitch
	vecViewAngle[PITCH] = clamp( vecViewAngle[PITCH], -cl_pitchup.GetFloat(), cl_pitchdown.GetFloat() );

	engine->SetViewAngles( vecViewAngle );
}

