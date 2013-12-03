//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Used to create a path that can be followed by NPCs and trains.
//
//=============================================================================//

#include "cbase.h"
#include "pathtrack.h"
#include "entitylist.h"
#include "ndebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CPathTrack )

	DEFINE_FIELD( m_pnext,			FIELD_CLASSPTR ),
	DEFINE_FIELD( m_pprevious,		FIELD_CLASSPTR ),
	DEFINE_FIELD( m_paltpath,		FIELD_CLASSPTR ),

	DEFINE_KEYFIELD( m_flRadius,	FIELD_FLOAT, "radius" ),
	DEFINE_FIELD( m_length,			FIELD_FLOAT ),
	DEFINE_KEYFIELD( m_altName,		FIELD_STRING, "altpath" ),
	DEFINE_KEYFIELD( m_eOrientationType, FIELD_INTEGER, "orientationtype" ),
//	DEFINE_FIELD( m_nIterVal,		FIELD_INTEGER ),
	
	DEFINE_INPUTFUNC( FIELD_VOID, "InPass", InputPass ),
	DEFINE_INPUTFUNC( FIELD_VOID, "InTeleport",  InputTeleport ),
	
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableAlternatePath", InputEnableAlternatePath ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableAlternatePath", InputDisableAlternatePath ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ToggleAlternatePath", InputToggleAlternatePath ),

	DEFINE_INPUTFUNC( FIELD_VOID, "EnablePath", InputEnablePath ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisablePath", InputDisablePath ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TogglePath", InputTogglePath ),

	// Outputs
	DEFINE_OUTPUT(m_OnPass, "OnPass"),
	DEFINE_OUTPUT(m_OnTeleport,  "OnTeleport"),

END_DATADESC()

LINK_ENTITY_TO_CLASS( path_track, CPathTrack );


//-----------------------------------------------------------------------------
// Finds circular paths
//-----------------------------------------------------------------------------
int CPathTrack::s_nCurrIterVal = 0;
bool CPathTrack::s_bIsIterating = false;


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CPathTrack::CPathTrack()
{
	m_nIterVal = -1;
	m_eOrientationType = TrackOrientation_FacePath;
}


//-----------------------------------------------------------------------------
// Spawn!
//-----------------------------------------------------------------------------
void CPathTrack::Spawn( void )
{
	SetSolid( SOLID_NONE );
	UTIL_SetSize(this, Vector(-8, -8, -8), Vector(8, 8, 8));
}


//-----------------------------------------------------------------------------
// Activate!
//-----------------------------------------------------------------------------
void CPathTrack::Activate( void )
{
	BaseClass::Activate();

	if ( GetEntityName() != NULL_STRING )		// Link to next, and back-link
	{
		Link();
	}
}


//-----------------------------------------------------------------------------
// Connects up the previous + next pointers 
//-----------------------------------------------------------------------------
void CPathTrack::Link( void  )
{
	CBaseEntity *pTarget;

	if ( m_target != NULL_STRING )
	{
		pTarget = gEntList.FindEntityByName( NULL, m_target );

		if ( pTarget == this)
		{
			Warning("ERROR: path_track (%s) refers to itself as a target!\n", GetDebugName());
			
			//FIXME: Why were we removing this?  If it was already connected to, we weren't updating the other linked
			//		 end, causing problems with walking through bogus memory links!  -- jdw

			//UTIL_Remove(this);
			//return;
		}
		else if ( pTarget )
		{
			m_pnext = dynamic_cast<CPathTrack*>( pTarget );

			if ( m_pnext )		// If no next pointer, this is the end of a path
			{
				m_pnext->SetPrevious( this );
			}
		}
		else
		{
			Warning("Dead end link: %s\n", STRING( m_target ) );
		}
	}

	// Find "alternate" path
	if ( m_altName != NULL_STRING )
	{
		pTarget = gEntList.FindEntityByName( NULL, m_altName );
		if ( pTarget )
		{
			m_paltpath = dynamic_cast<CPathTrack*>( pTarget );
			m_paltpath->SetPrevious( this );
		}
	}
}


//-----------------------------------------------------------------------------
// Circular path checking
//-----------------------------------------------------------------------------
void CPathTrack::BeginIteration()
{
	Assert( !s_bIsIterating );
	++s_nCurrIterVal;
	s_bIsIterating = true;
}

void CPathTrack::EndIteration()
{
	Assert( s_bIsIterating );
	s_bIsIterating = false;
}

void CPathTrack::Visit()
{
	m_nIterVal = s_nCurrIterVal;
}

bool CPathTrack::HasBeenVisited() const
{
	return ( m_nIterVal == s_nCurrIterVal );
}


//-----------------------------------------------------------------------------
// Do we have an alternate path?
//-----------------------------------------------------------------------------
bool CPathTrack::HasAlternathPath() const
{
	return ( m_paltpath != NULL ); 
}


//-----------------------------------------------------------------------------
// Purpose: Toggles the track to or from its alternate path
//-----------------------------------------------------------------------------
void CPathTrack::ToggleAlternatePath( void )
{
	// Use toggles between two paths
	if ( m_paltpath != NULL )
	{
		if ( FBitSet( m_spawnflags, SF_PATH_ALTERNATE ) == false )
		{
			EnableAlternatePath();
		}
		else
		{
			DisableAlternatePath();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathTrack::EnableAlternatePath( void )
{
	if ( m_paltpath != NULL )
	{
		SETBITS( m_spawnflags, SF_PATH_ALTERNATE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathTrack::DisableAlternatePath( void )
{
	if ( m_paltpath != NULL )
	{
		CLEARBITS( m_spawnflags, SF_PATH_ALTERNATE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPathTrack::InputEnableAlternatePath( inputdata_t &inputdata )
{
	EnableAlternatePath();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPathTrack::InputDisableAlternatePath( inputdata_t &inputdata )
{
	DisableAlternatePath();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPathTrack::InputToggleAlternatePath( inputdata_t &inputdata )
{
	ToggleAlternatePath();
}

//-----------------------------------------------------------------------------
// Purpose: Toggles the track to or from its alternate path
//-----------------------------------------------------------------------------
void CPathTrack::TogglePath( void )
{
	// Use toggles between two paths
	if ( FBitSet( m_spawnflags, SF_PATH_DISABLED ) )
	{
		EnablePath();
	}
	else
	{
		DisablePath();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathTrack::EnablePath( void )
{
	CLEARBITS( m_spawnflags, SF_PATH_DISABLED );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathTrack::DisablePath( void )
{
	SETBITS( m_spawnflags, SF_PATH_DISABLED );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPathTrack::InputEnablePath( inputdata_t &inputdata )
{
	EnablePath();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPathTrack::InputDisablePath( inputdata_t &inputdata )
{
	DisablePath();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPathTrack::InputTogglePath( inputdata_t &inputdata )
{
	TogglePath();
}


void CPathTrack::DrawDebugGeometryOverlays() 
{
	// ----------------------------------------------
	// Draw line to next target is bbox is selected
	// ----------------------------------------------
	if (m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_ABSBOX_BIT))
	{
		if (m_pnext)
		{
			NDebugOverlay::Line(GetAbsOrigin(),m_pnext->GetAbsOrigin(),255,100,100,true,0.0);
		}
	}
	BaseClass::DrawDebugGeometryOverlays();
}

CPathTrack	*CPathTrack::ValidPath( CPathTrack	*ppath, int testFlag )
{
	if ( !ppath )
		return NULL;

	if ( testFlag && FBitSet( ppath->m_spawnflags, SF_PATH_DISABLED ) )
		return NULL;

	return ppath;
}


void CPathTrack::Project( CPathTrack *pstart, CPathTrack *pend, Vector &origin, float dist )
{
	if ( pstart && pend )
	{
		Vector dir = (pend->GetLocalOrigin() - pstart->GetLocalOrigin());
		VectorNormalize( dir );
		origin = pend->GetLocalOrigin() + dir * dist;
	}
}

CPathTrack *CPathTrack::GetNext( void )
{
	if ( m_paltpath && FBitSet( m_spawnflags, SF_PATH_ALTERNATE ) && !FBitSet( m_spawnflags, SF_PATH_ALTREVERSE ) )
	{
		Assert( !m_paltpath.IsValid() || m_paltpath.Get() != NULL );
		return m_paltpath;
	}
	
	// The paths shouldn't normally be getting deleted so assert that if it was set, it's valid.
	Assert( !m_pnext.IsValid() || m_pnext.Get() != NULL );
	return m_pnext;
}



CPathTrack *CPathTrack::GetPrevious( void )
{
	if ( m_paltpath && FBitSet( m_spawnflags, SF_PATH_ALTERNATE ) && FBitSet( m_spawnflags, SF_PATH_ALTREVERSE ) )
	{
		Assert( !m_paltpath.IsValid() || m_paltpath.Get() != NULL );
		return m_paltpath;
	}
	
	Assert( !m_pprevious.IsValid() || m_pprevious.Get() != NULL );
	return m_pprevious;
}



void CPathTrack::SetPrevious( CPathTrack *pprev )
{
	// Only set previous if this isn't my alternate path
	if ( pprev && !FStrEq( STRING(pprev->GetEntityName()), STRING(m_altName) ) )
		m_pprevious = pprev;
}


CPathTrack *CPathTrack::GetNextInDir( bool bForward )
{
	if ( bForward )
		return GetNext();
	
	return GetPrevious();
}


//-----------------------------------------------------------------------------
// Purpose: Assumes this is ALWAYS enabled
// Input  : origin - position along path to look ahead from
//			dist - distance to look ahead, negative values look backward
//			move - 
// Output : Returns the track that we will be PAST in 'dist' units.
//-----------------------------------------------------------------------------
CPathTrack *CPathTrack::LookAhead( Vector &origin, float dist, int move, CPathTrack **pNextNext )
{
	CPathTrack *pcurrent = this;
	float originalDist = dist;
	Vector currentPos = origin;

	bool bForward = true;
	if ( dist < 0 )
	{
		// Travelling backwards along the path.
		dist = -dist;
		bForward = false;
	}

	// Move along the path, until we've gone 'dist' units or run out of path.
	while ( dist > 0 )
	{
		// If there is no next path track, or it's disabled, we're done.
		if ( !ValidPath( pcurrent->GetNextInDir( bForward ), move ) )
		{
			if ( !move )
			{
				Project( pcurrent->GetNextInDir( !bForward ), pcurrent, origin, dist );
			}

			return NULL;
		}

		// The next path track is valid. How far are we from it?
		Vector dir = pcurrent->GetNextInDir( bForward )->GetLocalOrigin() - currentPos;
		float length = dir.Length();

		// If we are at the next node and there isn't one beyond it, return the next node.
		if ( !length  && !ValidPath( pcurrent->GetNextInDir( bForward )->GetNextInDir( bForward ), move ) )
		{
			if ( pNextNext )
			{
				*pNextNext = NULL;
			}

			if ( dist == originalDist )
			{
				// Didn't move at all, must be in a dead end.
				return NULL;
			}

			return pcurrent->GetNextInDir( bForward );
		}

		// If we don't hit the next path track within the distance remaining, we're done.
		if ( length > dist )
		{
			origin = currentPos + ( dir * ( dist / length ) );
			if ( pNextNext )
			{
				*pNextNext = pcurrent->GetNextInDir( bForward );
			}

			return pcurrent;
		}

		// We hit the next path track, advance to it.
		dist -= length;
		currentPos = pcurrent->GetNextInDir( bForward )->GetLocalOrigin();
		pcurrent = pcurrent->GetNextInDir( bForward );
		origin = currentPos;
	}

	// We consumed all of the distance, and exactly landed on a path track.
	if ( pNextNext )
	{
		*pNextNext = pcurrent->GetNextInDir( bForward );
	}

	return pcurrent;
}


// Assumes this is ALWAYS enabled
CPathTrack *CPathTrack::Nearest( const Vector &origin )
{
	int			deadCount;
	float		minDist, dist;
	Vector		delta;
	CPathTrack	*ppath, *pnearest;


	delta = origin - GetLocalOrigin();
	delta.z = 0;
	minDist = delta.Length();
	pnearest = this;
	ppath = GetNext();

	// Hey, I could use the old 2 racing pointers solution to this, but I'm lazy :)
	deadCount = 0;
	while ( ppath && ppath != this )
	{
		deadCount++;
		if ( deadCount > 9999 )
		{
			Warning( "Bad sequence of path_tracks from %s\n", GetDebugName() );
			Assert(0);
			return NULL;
		}
		delta = origin - ppath->GetLocalOrigin();
		delta.z = 0;
		dist = delta.Length();
		if ( dist < minDist )
		{
			minDist = dist;
			pnearest = ppath;
		}
		ppath = ppath->GetNext();
	}
	return pnearest;
}


//-----------------------------------------------------------------------------
// Purpose: Returns how the path follower should orient itself when moving
//			through this path track.
//-----------------------------------------------------------------------------
TrackOrientationType_t CPathTrack::GetOrientationType()
{
	return m_eOrientationType;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
QAngle CPathTrack::GetOrientation( bool bForwardDir )
{
	TrackOrientationType_t eOrient = GetOrientationType();
	if ( eOrient == TrackOrientation_FacePathAngles )
	{
		return GetLocalAngles();
	}

	CPathTrack *pPrev = this;
	CPathTrack *pNext = GetNextInDir( bForwardDir );

	if ( !pNext )
	{	pPrev = GetNextInDir( !bForwardDir );
		pNext = this;
	}

	Vector vecDir = pNext->GetLocalOrigin() - pPrev->GetLocalOrigin();

	QAngle angDir;
	VectorAngles( vecDir, angDir );
	return angDir;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pent - 
// Output : CPathTrack
//-----------------------------------------------------------------------------
CPathTrack *CPathTrack::Instance( edict_t *pent )
{
	CBaseEntity *pEntity = CBaseEntity::Instance( pent );
	if ( FClassnameIs( pEntity, "path_track" ) )
		return (CPathTrack *)pEntity;
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathTrack::InputPass( inputdata_t &inputdata )
{
	m_OnPass.FireOutput( inputdata.pActivator, this );

#ifdef TF_DLL
	IGameEvent * event = gameeventmanager->CreateEvent( "path_track_passed" );
	if ( event )
	{
		event->SetInt( "index", entindex() );
		gameeventmanager->FireEvent( event, true );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathTrack::InputTeleport( inputdata_t &inputdata )
{
	m_OnTeleport.FireOutput( inputdata.pActivator, this );
}