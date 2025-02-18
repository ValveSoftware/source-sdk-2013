//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMETRACKGROUP_H
#define DMETRACKGROUP_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlflags.h"
#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"
#include "datamodel/dmehandle.h"
#include "movieobjects/timeutils.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeClip;
class CDmeFilmClip;
class CDmeTrack;
enum DmeClipType_t;
enum DmeClipSkipFlag_t;


//-----------------------------------------------------------------------------
// Default track group name
//-----------------------------------------------------------------------------
#define DMETRACKGROUP_DEFAULT_NAME "default"


//-----------------------------------------------------------------------------
// A track group
//-----------------------------------------------------------------------------
class CDmeTrackGroup : public CDmElement
{
	DEFINE_ELEMENT( CDmeTrackGroup, CDmElement );

public:
	// Max track count
	void SetMaxTrackCount( int nCount );

	// Owning clip
	CDmeClip *GetOwnerClip();
	void SetOwnerClip( CDmeClip *pClip );

	// track helper methods
	void AddTrack( CDmeTrack *track );
	CDmeTrack* AddTrack( const char *pTrackName, DmeClipType_t trackType );
	CDmeTrack* FindOrAddTrack( const char *pTrackName, DmeClipType_t trackType );
	void RemoveTrack( CDmeTrack *track );
	void RemoveTrack( const char *pTrackName );
	void RemoveTrack( int nIndex );
	const CUtlVector< DmElementHandle_t > &GetTracks( ) const;
	int GetTrackCount( ) const;
	CDmeTrack *GetTrack( int nIndex ) const;
	CDmeTrack *FindTrack( const char *pTrackName ) const;
	int GetTrackIndex( CDmeTrack *pTrack ) const;

	// Clip helper methods
	// AddClip/ChangeTrack returns non-NULL if it was successfully added
	CDmeTrack *AddClip( CDmeClip *pClip, const char *pTrackName );
	bool RemoveClip( CDmeClip *pClip );
	CDmeTrack *ChangeTrack( CDmeClip *pClip, const char *pNewTrack );
	CDmeTrack *FindTrackForClip( CDmeClip *pClip ) const;

	bool FindTrackForClip( CDmeClip *pClip, int *pTrackIndex, int *pClipIndex = NULL ) const;

	// Finding clips at a particular time
	void FindClipsAtTime( DmeClipType_t clipType, DmeTime_t time, DmeClipSkipFlag_t flags, CUtlVector< CDmeClip * >& clips ) const;
	void FindClipsIntersectingTime( DmeClipType_t clipType, DmeTime_t startTime, DmeTime_t endTime, DmeClipSkipFlag_t flags, CUtlVector< CDmeClip * >& clips ) const;
	void FindClipsWithinTime( DmeClipType_t clipType, DmeTime_t startTime, DmeTime_t endTime, DmeClipSkipFlag_t flags, CUtlVector< CDmeClip * >& clips ) const;

	// Are we a film track group?
	bool IsFilmTrackGroup();

	// Gets the film track (if this is a film track group)
	CDmeTrack *GetFilmTrack();

	// Is a particular clip typed able to be added?
	bool IsSubClipTypeAllowed( DmeClipType_t type );

	// Is this track group visible?
	void SetVisible( bool bVisible );
	bool IsVisible() const;

	// Is this track group minimized?
	void SetMinimized( bool bLocked );
	bool IsMinimized() const;

	// Track group display size
	void SetDisplaySize( int nDisplaySize );
	int GetDisplaySize() const;

	// Creates the film track group [for internal use only]
	CDmeTrack *CreateFilmTrack();

	// Sort tracks by track type, then alphabetically
	void SortTracksByType();

	// Removes empty tracks
	void RemoveEmptyTracks();

	// Muting track groups
	void SetMute( bool state );
	bool IsMute( ) const;

	// Volume for track group
	void SetVolume( float state );
	float GetVolume() const;


	// Returns the flattened clip count
	int GetSubClipCount() const;
	void GetSubClips( CDmeClip **ppClips );

private:
	CDmaElementArray< CDmeTrack > m_Tracks;
	CDmaVar<bool> m_bIsVisible;
	CDmaVar<bool> m_bMinimized;
	CDmaVar< bool >	m_bMute;
	CDmaVar< float > m_Volume;
	CDmaVar<int> m_nDisplaySize;

	DmElementHandle_t m_hOwner;
	int m_nMaxTrackCount;
};


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline int CDmeTrackGroup::GetTrackCount( ) const
{
	return m_Tracks.Count();
}

inline CDmeTrack *CDmeTrackGroup::GetTrack( int nIndex ) const
{
	return m_Tracks[nIndex];
}

inline const CUtlVector< DmElementHandle_t > &CDmeTrackGroup::GetTracks( ) const
{
	return m_Tracks.Get();
}

	
//-----------------------------------------------------------------------------
// Is this track group visible?
//-----------------------------------------------------------------------------
inline void CDmeTrackGroup::SetVisible( bool bVisible )
{
	m_bIsVisible = bVisible;
}

inline bool CDmeTrackGroup::IsVisible() const
{
	return m_bIsVisible;
}


//-----------------------------------------------------------------------------
// Is this track group minimized?
//-----------------------------------------------------------------------------
inline void CDmeTrackGroup::SetMinimized( bool bMinimized )
{
	m_bMinimized = bMinimized;
}

inline bool CDmeTrackGroup::IsMinimized() const
{
	return m_bMinimized;
}


//-----------------------------------------------------------------------------
// Track group display size
//-----------------------------------------------------------------------------
inline void CDmeTrackGroup::SetDisplaySize( int nDisplaySize )
{
	m_nDisplaySize = nDisplaySize;
}

inline int CDmeTrackGroup::GetDisplaySize() const
{
	return m_nDisplaySize;
}
	

//-----------------------------------------------------------------------------
// Iterator macro
//-----------------------------------------------------------------------------
#define DMETRACKGROUP_FOREACH_CLIP_START( _dmeTrackGroup, _dmeTrack, _dmeClip )	\
	{																	\
		int _tc = (_dmeTrackGroup)->GetTrackCount();					\
		for ( int _i = 0; _i < _tc; ++_i )								\
		{																\
			CDmeTrack *_dmeTrack = (_dmeTrackGroup)->GetTrack( _i );	\
			if ( !_dmeTrack )											\
				continue;												\
																		\
			int _cc = _dmeTrack->GetClipCount();						\
			for ( int _j = 0; _j < _cc; ++_j )							\
			{															\
				CDmeClip *_dmeClip = _dmeTrack->GetClip( _j );			\
				if ( !_dmeClip )										\
					continue;

#define DMETRACKGROUP_FOREACH_CLIP_END( )		\
			}									\
		}										\
	}

#define DMETRACKGROUP_FOREACH_CLIP_TYPE_START( _clipType, _dmeTrackGroup, _dmeTrack, _dmeClip )	\
	{																	\
		int _tc = (_dmeTrackGroup)->GetTrackCount();					\
		for ( int _i = 0; _i < _tc; ++_i )								\
		{																\
			CDmeTrack *_dmeTrack = (_dmeTrackGroup)->GetTrack( _i );	\
			if ( !_dmeTrack )											\
				continue;												\
																		\
			if ( _dmeTrack->GetClipType() != CDmeClipInfo< _clipType >::ClipType() )	\
				continue;												\
																		\
			int _cc = _dmeTrack->GetClipCount();						\
			for ( int _j = 0; _j < _cc; ++_j )							\
			{															\
				_clipType *_dmeClip = static_cast< _clipType* >( _dmeTrack->GetClip( _j ) );	\
				if ( !_dmeClip )										\
					continue;

#define DMETRACKGROUP_FOREACH_CLIP_TYPE_END( )	\
			}									\
		}										\
	}

//-----------------------------------------------------------------------------
// helper methods
//-----------------------------------------------------------------------------
CDmeFilmClip *GetParentClip( CDmeTrackGroup *pTrackGroup );

#endif // DMETRACKGROUP_H
