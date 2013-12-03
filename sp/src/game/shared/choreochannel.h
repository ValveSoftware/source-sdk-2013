//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CHOREOCHANNEL_H
#define CHOREOCHANNEL_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlvector.h"
#include "tier1/utlrbtree.h"

class CChoreoEvent;
class CChoreoActor;
class CChoreoScene;
class CUtlBuffer;
class IChoreoStringPool;

//-----------------------------------------------------------------------------
// Purpose: A channel is owned by an actor and contains zero or more events
//-----------------------------------------------------------------------------
class CChoreoChannel
{
public:
	// Construction
					CChoreoChannel( void );
					CChoreoChannel( const char *name );

	// Assignment
	CChoreoChannel&	operator=(const CChoreoChannel& src );

	// Serialization
	void			SaveToBuffer( CUtlBuffer& buf, CChoreoScene *pScene, IChoreoStringPool *pStringPool );
	bool			RestoreFromBuffer( CUtlBuffer& buf, CChoreoScene *pScene, CChoreoActor *pActor, IChoreoStringPool *pStringPool );

	// Accessors
	void			SetName( const char *name );
	const char		*GetName( void );

	// Iterate children
	int				GetNumEvents( void );
	CChoreoEvent	*GetEvent( int event );

	// Manipulate children
	void			AddEvent( CChoreoEvent *event );
	void			RemoveEvent( CChoreoEvent *event );
	int				FindEventIndex( CChoreoEvent *event );
	void			RemoveAllEvents();

	CChoreoActor	*GetActor( void );
	void			SetActor( CChoreoActor *actor );

	void						SetActive( bool active );
	bool						GetActive( void ) const;

	// Compute true start/end times for gesture events in this channel, factoring in "null" gestures as needed
	void			ReconcileGestureTimes();
	// Compute master/slave, count, endtime info for close captioning data
	void			ReconcileCloseCaption();

	bool			IsMarkedForSave() const { return m_bMarkedForSave; }
	void			SetMarkedForSave( bool mark ) { m_bMarkedForSave = mark; }

	void			MarkForSaveAll( bool mark );

	bool			GetSortedCombinedEventList( char const *cctoken, CUtlRBTree< CChoreoEvent * >& sorted );

private:
	// Initialize fields
	void			Init( void );

	enum
	{
		MAX_CHANNEL_NAME = 128,
	};

	CChoreoActor	*m_pActor;

	// Channels are just named
	char			m_szName[ MAX_CHANNEL_NAME ];

	// All of the events for this channel
	CUtlVector < CChoreoEvent * > m_Events;

	bool			m_bActive;

	// Purely for save/load
	bool			m_bMarkedForSave;
};

#endif // CHOREOCHANNEL_H
