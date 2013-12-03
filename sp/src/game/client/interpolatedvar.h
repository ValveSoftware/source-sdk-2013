//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef INTERPOLATEDVAR_H
#define INTERPOLATEDVAR_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utllinkedlist.h"
#include "rangecheckedvar.h"
#include "lerp_functions.h"
#include "animationlayer.h"
#include "convar.h"


#include "tier0/memdbgon.h"

#define COMPARE_HISTORY(a,b) \
	( memcmp( m_VarHistory[a].GetValue(), m_VarHistory[b].GetValue(), sizeof(Type)*GetMaxCount() ) == 0 ) 			

// Define this to have it measure whether or not the interpolated entity list
// is accurate.
//#define INTERPOLATEDVAR_PARANOID_MEASUREMENT


#define LATCH_ANIMATION_VAR  (1<<0)		// use AnimTime as sample basis
#define LATCH_SIMULATION_VAR (1<<1)		// use SimulationTime as sample basis

#define EXCLUDE_AUTO_LATCH			(1<<2)
#define EXCLUDE_AUTO_INTERPOLATE	(1<<3)

#define INTERPOLATE_LINEAR_ONLY		(1<<4)	// don't do hermite interpolation
#define INTERPOLATE_OMIT_UPDATE_LAST_NETWORKED (1<<5)



#define EXTRA_INTERPOLATION_HISTORY_STORED 0.05f	// It stores this much extra interpolation history,
													// so you can always call Interpolate() this far
													// in the past from your last call and be able to 
													// get an interpolated value.

// this global keeps the last known server packet tick (to avoid calling engine->GetLastTimestamp() all the time)
extern float g_flLastPacketTimestamp;

inline void Interpolation_SetLastPacketTimeStamp( float timestamp)
{
	Assert( timestamp > 0 );
	g_flLastPacketTimestamp = timestamp;
}


// Before calling Interpolate(), you can use this use this to setup the context if 
// you want to enable extrapolation.
class CInterpolationContext
{
public:
	
	CInterpolationContext()
	{
		m_bOldAllowExtrapolation = s_bAllowExtrapolation;
		m_flOldLastTimeStamp = s_flLastTimeStamp;

		// By default, disable extrapolation unless they call EnableExtrapolation.
		s_bAllowExtrapolation = false;

		// this is the context stack
		m_pNext = s_pHead;
		s_pHead = this;
	}
	
	~CInterpolationContext()
	{
		// restore values from prev stack element
		s_bAllowExtrapolation = m_bOldAllowExtrapolation;
		s_flLastTimeStamp = m_flOldLastTimeStamp;

		Assert( s_pHead == this );
		s_pHead = m_pNext;
	}

	static void EnableExtrapolation(bool state)
	{
		s_bAllowExtrapolation = state;
	}

	static bool IsThereAContext()
	{
		return s_pHead != NULL;
	}

	static bool IsExtrapolationAllowed()
	{
		return s_bAllowExtrapolation;
	}

	static void SetLastTimeStamp(float timestamp)
	{
		s_flLastTimeStamp = timestamp;
	}
	
	static float GetLastTimeStamp()
	{
		return s_flLastTimeStamp;
	}


private:

	CInterpolationContext *m_pNext;
	bool m_bOldAllowExtrapolation;
	float m_flOldLastTimeStamp;

	static CInterpolationContext *s_pHead;
	static bool s_bAllowExtrapolation;
	static float s_flLastTimeStamp;
};


extern ConVar cl_extrapolate_amount;


template< class T >
inline T ExtrapolateInterpolatedVarType( const T &oldVal, const T &newVal, float divisor, float flExtrapolationAmount )
{
	return newVal;
}

inline Vector ExtrapolateInterpolatedVarType( const Vector &oldVal, const Vector &newVal, float divisor, float flExtrapolationAmount )
{
	return Lerp( 1.0f + flExtrapolationAmount * divisor, oldVal, newVal );
}

inline float ExtrapolateInterpolatedVarType( const float &oldVal, const float &newVal, float divisor, float flExtrapolationAmount )
{
	return Lerp( 1.0f + flExtrapolationAmount * divisor, oldVal, newVal );
}

inline QAngle ExtrapolateInterpolatedVarType( const QAngle &oldVal, const QAngle &newVal, float divisor, float flExtrapolationAmount )
{
	return Lerp<QAngle>( 1.0f + flExtrapolationAmount * divisor, oldVal, newVal );
}


// -------------------------------------------------------------------------------------------------------------- //
// IInterpolatedVar interface.
// -------------------------------------------------------------------------------------------------------------- //

abstract_class IInterpolatedVar
{
public:
	virtual		 ~IInterpolatedVar() {}

	virtual void Setup( void *pValue, int type ) = 0;
	virtual void SetInterpolationAmount( float seconds ) = 0;
	
	// Returns true if the new value is different from the prior most recent value.
	virtual void NoteLastNetworkedValue() = 0;
	virtual bool NoteChanged( float changetime, bool bUpdateLastNetworkedValue ) = 0;
	virtual void Reset() = 0;
	
	// Returns 1 if the value will always be the same if currentTime is always increasing.
	virtual int Interpolate( float currentTime ) = 0;
	
	virtual int	 GetType() const = 0;
	virtual void RestoreToLastNetworked() = 0;
	virtual void Copy( IInterpolatedVar *pSrc ) = 0;

	virtual const char *GetDebugName() = 0;
	virtual void SetDebugName( const char* pName )	= 0;

	virtual void SetDebug( bool bDebug ) = 0;
};

template< typename Type, bool IS_ARRAY >
struct CInterpolatedVarEntryBase
{
	CInterpolatedVarEntryBase()
	{
		value = NULL;
		count = 0;
		changetime = 0;
	}
	~CInterpolatedVarEntryBase()
	{
		delete[] value;
		value = NULL;
	}

	// This will transfer the data from another varentry.  This is used to avoid allocation
	// pointers can be transferred (only one varentry has a copy), but not trivially copied
	void FastTransferFrom( CInterpolatedVarEntryBase &src )
	{
		Assert(!value);
		value = src.value;
		count = src.count;
		changetime = src.changetime;
		src.value = 0;
		src.count = 0;
	}

	CInterpolatedVarEntryBase& operator=( const CInterpolatedVarEntryBase& src )
	{
		delete[] value;
		value = NULL;
		count = 0;
		if ( src.value )
		{
			count = src.count;
			value = new Type[count];
			for ( int i = 0; i < count; i++ )
			{
				value[i] = src.value[i];
			}
		}
		return *this;
	}

	Type *GetValue() { return value; }
	const Type *GetValue() const { return value; }

	void Init(int maxCount)
	{
		if ( !maxCount )
		{
			DeleteEntry();
		}
		else
		{
			// resize
			if ( maxCount != count )
			{
				DeleteEntry();
			}

			if ( !value )
			{
				count = maxCount;
				value = new Type[maxCount];
			}
		}
		Assert(count==maxCount);
	}
	Type *NewEntry( const Type *pValue, int maxCount, float time )
	{
		changetime = time;
		Init(maxCount);
		if ( value && maxCount)
		{
			memcpy( value, pValue, maxCount*sizeof(Type) );
		}
		return value;
	}

	void DeleteEntry()
	{
		delete[] value;
		value = NULL;
		count = 0;
	}

	float		changetime;
	int			count;
	Type *		value;

private:
	CInterpolatedVarEntryBase( const CInterpolatedVarEntryBase &src );
};

template<typename Type>
struct CInterpolatedVarEntryBase<Type, false>
{
	CInterpolatedVarEntryBase() {}
	~CInterpolatedVarEntryBase() {}

	const Type *GetValue() const { return &value; }
	Type *GetValue() { return &value; }

	void Init(int maxCount)
	{
		Assert(maxCount==1);
	}
	Type *NewEntry( const Type *pValue, int maxCount, float time )
	{
		Assert(maxCount==1);
		changetime = time;
		memcpy( &value, pValue, maxCount*sizeof(Type) );
		return &value;
	}
	void FastTransferFrom( CInterpolatedVarEntryBase &src )
	{
		*this = src;
	}

	void DeleteEntry() {}

	float		changetime;
	Type		value;
};

template<typename T>
class CSimpleRingBuffer
{
public:
	CSimpleRingBuffer( int startSize = 4 )
	{
		m_pElements = 0;
		m_maxElement = 0;
		m_firstElement = 0;
		m_count = 0;
		m_growSize = 16;
		EnsureCapacity(startSize);
	}
	~CSimpleRingBuffer()
	{
		delete[] m_pElements;
		m_pElements = NULL;
	}

	inline int Count() const { return m_count; }

	int Head() const { return (m_count>0) ? 0 : InvalidIndex(); }

	bool IsIdxValid( int i ) const { return (i >= 0 && i < m_count) ? true : false; }
	bool IsValidIndex(int i) const { return IsIdxValid(i); }
	static int InvalidIndex() { return -1; }

	T& operator[]( int i ) 
	{ 
		Assert( IsIdxValid(i) ); 
		i += m_firstElement;
		i = WrapRange(i);
		return m_pElements[i];
	}

	const T& operator[]( int i ) const
	{ 
		Assert( IsIdxValid(i) ); 
		i += m_firstElement;
		i = WrapRange(i);
		return m_pElements[i];
	}

	void EnsureCapacity( int capSize )
	{
		if ( capSize > m_maxElement )
		{
			int newMax = m_maxElement + ((capSize+m_growSize-1)/m_growSize) * m_growSize;
			T *pNew = new T[newMax];
			for ( int i = 0; i < m_maxElement; i++ )
			{
				// ------------
				// If you wanted to make this a more generic container you'd probably want this code
				// instead - since FastTransferFrom() is an optimization dependent on types stored
				// here defining this operation.
				//pNew[i] = m_pElements[WrapRange(i+m_firstElement)];
				pNew[i].FastTransferFrom( m_pElements[WrapRange(i+m_firstElement)] );
				// ------------
			}
			m_firstElement = 0;
			m_maxElement = newMax;
			delete[] m_pElements;
			m_pElements = pNew;
		}
	}

	int AddToHead()
	{
		EnsureCapacity( m_count + 1 );
		int i = m_firstElement + m_maxElement - 1;
		m_count++;
		i = WrapRange(i);
		m_firstElement = i;
		return 0;
	}

	int AddToHead( const T &elem )
	{
		AddToHead();
		m_pElements[m_firstElement] = elem;
		return 0;
	}

	int AddToTail()
	{
		EnsureCapacity( m_count + 1 );
		m_count++;
		return WrapRange(m_firstElement+m_count-1);
	}

	void RemoveAll()
	{
		m_count = 0;
		m_firstElement = 0;
	}

	void RemoveAtHead()
	{
		if ( m_count > 0 )
		{
			m_firstElement = WrapRange(m_firstElement+1);
			m_count--;
		}
	}

	void Truncate( int newLength )
	{
		if ( newLength < m_count )
		{
			Assert(newLength>=0);
			m_count = newLength;
		}
	}

private:
	inline int WrapRange( int i ) const
	{
		return ( i >= m_maxElement ) ? (i - m_maxElement) : i;
	}

	T *m_pElements;
	unsigned short m_maxElement;
	unsigned short m_firstElement;
	unsigned short m_count;
	unsigned short m_growSize;
};

// -------------------------------------------------------------------------------------------------------------- //
// CInterpolatedVarArrayBase - the main implementation of IInterpolatedVar.
// -------------------------------------------------------------------------------------------------------------- //

template< typename Type, bool IS_ARRAY>
class CInterpolatedVarArrayBase : public IInterpolatedVar
{
public:
	friend class CInterpolatedVarPrivate;

	CInterpolatedVarArrayBase( const char *pDebugName="no debug name" );
	virtual ~CInterpolatedVarArrayBase();

	
	// IInterpolatedVar overrides.
public:
	
	virtual void Setup( void *pValue, int type );
	virtual void SetInterpolationAmount( float seconds );
	virtual void NoteLastNetworkedValue();
	virtual bool NoteChanged( float changetime, bool bUpdateLastNetworkedValue );
	virtual void Reset();
	virtual int Interpolate( float currentTime );
	virtual int GetType() const;
	virtual void RestoreToLastNetworked();
	virtual void Copy( IInterpolatedVar *pInSrc );
	virtual const char *GetDebugName() { return m_pDebugName; }


public:

	// Just like the IInterpolatedVar functions, but you can specify an interpolation amount.
	bool NoteChanged( float changetime, float interpolation_amount, bool bUpdateLastNetworkedValue );
	int Interpolate( float currentTime, float interpolation_amount );

	void DebugInterpolate( Type *pOut, float currentTime );

	void GetDerivative( Type *pOut, float currentTime );
	void GetDerivative_SmoothVelocity( Type *pOut, float currentTime );	// See notes on ::Derivative_HermiteLinearVelocity for info.

	void ClearHistory();
	void AddToHead( float changeTime, const Type* values, bool bFlushNewer );
	const Type&	GetPrev( int iArrayIndex=0 ) const;
	const Type&	GetCurrent( int iArrayIndex=0 ) const;
	
	// Returns the time difference betweem the most recent sample and its previous sample.
	float	GetInterval() const;
	bool	IsValidIndex( int i );
	Type	*GetHistoryValue( int index, float& changetime, int iArrayIndex=0 );
	int		GetHead() { return 0; }
	int		GetNext( int i ) 
	{ 
		int next = i + 1;
		if ( !m_VarHistory.IsValidIndex(next) )
			return m_VarHistory.InvalidIndex();
		return next;
	}

	void SetHistoryValuesForItem( int item, Type& value );
	void	SetLooping( bool looping, int iArrayIndex=0 );
	
	void SetMaxCount( int newmax );
	int GetMaxCount() const;

	// Get the time of the oldest entry.
	float GetOldestEntry();

	// set a debug name (if not provided by constructor)
	void	SetDebugName(const char *pName ) { m_pDebugName = pName; }
	virtual void SetDebug( bool bDebug ) { m_bDebug = bDebug; }
	bool GetInterpolationInfo( float currentTime, int *pNewer, int *pOlder, int *pOldest );

protected:

	typedef CInterpolatedVarEntryBase<Type, IS_ARRAY> CInterpolatedVarEntry;
	typedef CSimpleRingBuffer< CInterpolatedVarEntry > CVarHistory;
	friend class CInterpolationInfo;

	class CInterpolationInfo
	{
	public:
		bool m_bHermite;
		int oldest;	// Only set if using hermite.
		int older;
		int newer;
		float frac;
	};


protected:

	void RemoveOldEntries( float oldesttime );
	void RemoveEntriesPreviousTo( float flTime );

	bool GetInterpolationInfo( 
		CInterpolationInfo *pInfo,
		float currentTime, 
		float interpolation_amount,
		int *pNoMoreChanges );

	void TimeFixup_Hermite( 
		CInterpolatedVarEntry &fixup,
		CInterpolatedVarEntry*& prev, 
		CInterpolatedVarEntry*& start, 
		CInterpolatedVarEntry*& end	);

	// Force the time between prev and start to be dt (and extend prev out farther if necessary).
	void TimeFixup2_Hermite( 
		CInterpolatedVarEntry &fixup,
		CInterpolatedVarEntry*& prev, 
		CInterpolatedVarEntry*& start, 
		float dt
		);

	void _Extrapolate( 
		Type *pOut,
		CInterpolatedVarEntry *pOld,
		CInterpolatedVarEntry *pNew,
		float flDestinationTime,
		float flMaxExtrapolationAmount
		);

	void _Interpolate( Type *out, float frac, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end );
	void _Interpolate_Hermite( Type *out, float frac, CInterpolatedVarEntry *pOriginalPrev, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end, bool looping = false );
	
	void _Derivative_Hermite( Type *out, float frac, CInterpolatedVarEntry *pOriginalPrev, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end );
	void _Derivative_Hermite_SmoothVelocity( Type *out, float frac, CInterpolatedVarEntry *b, CInterpolatedVarEntry *c, CInterpolatedVarEntry *d );
	void _Derivative_Linear( Type *out, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end );
	
	bool ValidOrder();

protected:
	// The underlying data element
	Type								*m_pValue;
	CVarHistory							m_VarHistory;
	// Store networked values so when we latch we can detect which values were changed via networking
	Type *								m_LastNetworkedValue;
	float								m_LastNetworkedTime;
	byte								m_fType;
	byte								m_nMaxCount;
	byte *								m_bLooping;
	float								m_InterpolationAmount;
	const char *						m_pDebugName;
	bool								m_bDebug : 1;
};


template< typename Type, bool IS_ARRAY >
inline CInterpolatedVarArrayBase<Type, IS_ARRAY>::CInterpolatedVarArrayBase( const char *pDebugName )
{
	m_pDebugName = pDebugName;
	m_pValue = NULL;
	m_fType = LATCH_ANIMATION_VAR;
	m_InterpolationAmount = 0.0f;
	m_nMaxCount = 0;
	m_LastNetworkedTime = 0;
	m_LastNetworkedValue = NULL;
	m_bLooping = NULL;
	m_bDebug = false;
}

template< typename Type, bool IS_ARRAY >
inline CInterpolatedVarArrayBase<Type, IS_ARRAY>::~CInterpolatedVarArrayBase()
{
	ClearHistory();
	delete [] m_bLooping;
	delete [] m_LastNetworkedValue;
}

template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::Setup( void *pValue, int type )
{
	m_pValue = ( Type * )pValue;
	m_fType = type;
}

template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::SetInterpolationAmount( float seconds )
{
	m_InterpolationAmount = seconds;
}

template< typename Type, bool IS_ARRAY >
inline int CInterpolatedVarArrayBase<Type, IS_ARRAY>::GetType() const
{
	return m_fType;
}

template< typename Type, bool IS_ARRAY >
void CInterpolatedVarArrayBase<Type, IS_ARRAY>::NoteLastNetworkedValue()
{
	memcpy( m_LastNetworkedValue, m_pValue, m_nMaxCount * sizeof( Type ) );
	m_LastNetworkedTime = g_flLastPacketTimestamp;
}

template< typename Type, bool IS_ARRAY >
inline bool CInterpolatedVarArrayBase<Type, IS_ARRAY>::NoteChanged( float changetime, float interpolation_amount, bool bUpdateLastNetworkedValue )
{
	Assert( m_pValue );

	// This is a big optimization where it can potentially avoid expensive interpolation
	// involving this variable if it didn't get an actual new value in here.
	bool bRet = true;
	if ( m_VarHistory.Count() )
	{
		if ( memcmp( m_pValue, m_VarHistory[0].GetValue(), sizeof( Type ) * m_nMaxCount ) == 0 )
		{
			bRet = false;
		}
	}
	
	if ( m_bDebug )
	{
		char const *pDiffString = bRet ? "differs" : "identical";

		Msg( "%s LatchChanged at %f changetime %f:  %s\n", GetDebugName(), gpGlobals->curtime, changetime, pDiffString );
	}

	AddToHead( changetime, m_pValue, true );

	if ( bUpdateLastNetworkedValue )
	{
		NoteLastNetworkedValue();
	}
	
#if 0
	// Since we don't clean out the old entries until Interpolate(), make sure that there
	// aren't any super old entries hanging around.
	RemoveOldEntries( gpGlobals->curtime - interpolation_amount - 2.0f );
#else
	// JAY: It doesn't seem like the above code is correct.  This is keeping more than two seconds of history
	// for variables that aren't being interpolated for some reason.  For example, the player model isn't drawn
	// in first person, so the history is only truncated here and will accumulate ~40 entries instead of 2 or 3
	// changing over to the method in Interpolate() means that we always have a 3-sample neighborhood around
	// any data we're going to need.  Unless gpGlobals->curtime is different when samples are added vs. when
	// they are interpolated I can't see this having any ill effects.  
	RemoveEntriesPreviousTo( gpGlobals->curtime - interpolation_amount - EXTRA_INTERPOLATION_HISTORY_STORED );
#endif
	
	return bRet;
}


template< typename Type, bool IS_ARRAY >
inline bool CInterpolatedVarArrayBase<Type, IS_ARRAY>::NoteChanged( float changetime, bool bUpdateLastNetworkedValue )
{
	return NoteChanged( changetime, m_InterpolationAmount, bUpdateLastNetworkedValue );
}


template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::RestoreToLastNetworked()
{
	Assert( m_pValue );
	memcpy( m_pValue, m_LastNetworkedValue, m_nMaxCount * sizeof( Type ) );
}

template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::ClearHistory()
{
	for ( int i = 0; i < m_VarHistory.Count(); i++ )
	{
		m_VarHistory[i].DeleteEntry();
	}
	m_VarHistory.RemoveAll();
}

template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::AddToHead( float changeTime, const Type* values, bool bFlushNewer )
{
	MEM_ALLOC_CREDIT_CLASS();
	int newslot;
	
	if ( bFlushNewer )
	{
		// Get rid of anything that has a timestamp after this sample. The server might have
		// corrected our clock and moved us back, so our current changeTime is less than a 
		// changeTime we added samples during previously.
		while ( m_VarHistory.Count() )
		{
			if ( (m_VarHistory[0].changetime+0.0001f) > changeTime )
			{
				m_VarHistory.RemoveAtHead();
			}
			else
			{
				break;
			}
		}

		newslot = m_VarHistory.AddToHead();
	}
	else
	{
		newslot = m_VarHistory.AddToHead();
		for ( int i = 1; i < m_VarHistory.Count(); i++ )
		{
			if ( m_VarHistory[i].changetime <= changeTime )
				break;
			m_VarHistory[newslot].FastTransferFrom( m_VarHistory[i] );
			newslot = i;
		}
		}

	CInterpolatedVarEntry *e = &m_VarHistory[ newslot ];
	e->NewEntry( values, m_nMaxCount, changeTime );
}

template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::Reset()
{
	ClearHistory();

	if ( m_pValue )
	{
		AddToHead( gpGlobals->curtime, m_pValue, false );
		AddToHead( gpGlobals->curtime, m_pValue, false );
		AddToHead( gpGlobals->curtime, m_pValue, false );

		memcpy( m_LastNetworkedValue, m_pValue, m_nMaxCount * sizeof( Type ) );
	}
}


template< typename Type, bool IS_ARRAY >
inline float CInterpolatedVarArrayBase<Type, IS_ARRAY>::GetOldestEntry()
{
	float lastVal = 0;
	if ( m_VarHistory.Count() )
	{
		lastVal = m_VarHistory[m_VarHistory.Count()-1].changetime;
	}
	return lastVal;
}


template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::RemoveOldEntries( float oldesttime )
{
	int newCount = m_VarHistory.Count();
	for ( int i = m_VarHistory.Count(); --i > 2; )
	{
		if ( m_VarHistory[i].changetime > oldesttime )
			break;
		newCount = i;
	}
	m_VarHistory.Truncate(newCount);
}


template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::RemoveEntriesPreviousTo( float flTime )
{
	for ( int i = 0; i < m_VarHistory.Count(); i++ )
	{
		if ( m_VarHistory[i].changetime < flTime )
		{
			// We need to preserve this sample (ie: the one right before this timestamp)
			// and the sample right before it (for hermite blending), and we can get rid
			// of everything else.
			m_VarHistory.Truncate( i + 3 );
			break;
		}
	}
}


template< typename Type, bool IS_ARRAY >
inline bool CInterpolatedVarArrayBase<Type, IS_ARRAY>::GetInterpolationInfo( 
	typename CInterpolatedVarArrayBase<Type, IS_ARRAY>::CInterpolationInfo *pInfo,
	float currentTime, 
	float interpolation_amount,
	int *pNoMoreChanges
	)
{
	Assert( m_pValue );

	CVarHistory &varHistory = m_VarHistory;

	float targettime = currentTime - interpolation_amount;

	pInfo->m_bHermite = false;
	pInfo->frac = 0;
	pInfo->oldest = pInfo->older = pInfo->newer = varHistory.InvalidIndex();
	
	for ( int i = 0; i < varHistory.Count(); i++ )
	{
		pInfo->older = i;
		
		float older_change_time = m_VarHistory[ i ].changetime;
		if ( older_change_time == 0.0f )
			break;

		if ( targettime < older_change_time )
		{
			pInfo->newer = pInfo->older;
			continue;
		}

		if ( pInfo->newer == varHistory.InvalidIndex() )
		{
			// Have it linear interpolate between the newest 2 entries.
			pInfo->newer = pInfo->older; 

			// Since the time given is PAST all of our entries, then as long
			// as time continues to increase, we'll be returning the same value.
			if ( pNoMoreChanges )
				*pNoMoreChanges = 1;
			return true;
		}

		float newer_change_time = varHistory[ pInfo->newer ].changetime;
		float dt = newer_change_time - older_change_time;
		if ( dt > 0.0001f )
		{
			pInfo->frac = ( targettime - older_change_time ) / ( newer_change_time - older_change_time );
			pInfo->frac = MIN( pInfo->frac, 2.0f );

			int oldestindex = i+1;
														    
			if ( !(m_fType & INTERPOLATE_LINEAR_ONLY) && varHistory.IsIdxValid(oldestindex) )
			{
				pInfo->oldest = oldestindex;
				float oldest_change_time = varHistory[ oldestindex ].changetime;
				float dt2 = older_change_time - oldest_change_time;
				if ( dt2 > 0.0001f )
				{
					pInfo->m_bHermite = true;
				}
			}

			// If pInfo->newer is the most recent entry we have, and all 2 or 3 other
			// entries are identical, then we're always going to return the same value
			// if currentTime increases.
			if ( pNoMoreChanges && pInfo->newer == m_VarHistory.Head() )
			{
				 if ( COMPARE_HISTORY( pInfo->newer, pInfo->older ) )
				 {
					if ( !pInfo->m_bHermite || COMPARE_HISTORY( pInfo->newer, pInfo->oldest ) )
						*pNoMoreChanges = 1;
				 }
			}
		}
		return true;
	}

	// Didn't find any, return last entry???
	if ( pInfo->newer != varHistory.InvalidIndex() )
	{
		pInfo->older = pInfo->newer;
		return true;
	}


	// This is the single-element case
	pInfo->newer = pInfo->older;
	return (pInfo->older != varHistory.InvalidIndex());
}


template< typename Type, bool IS_ARRAY >
inline bool CInterpolatedVarArrayBase<Type, IS_ARRAY>::GetInterpolationInfo( float currentTime, int *pNewer, int *pOlder, int *pOldest )
{
	CInterpolationInfo info;
	bool result = GetInterpolationInfo( &info, currentTime, m_InterpolationAmount, NULL );

	if (pNewer)
		*pNewer = (int)info.newer;

	if (pOlder)
		*pOlder = (int)info.older;

	if (pOldest)
		*pOldest = (int)info.oldest;

	return result;
}


template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::DebugInterpolate( Type *pOut, float currentTime )
{
	float interpolation_amount = m_InterpolationAmount;

	int noMoreChanges = 0;

	CInterpolationInfo info;
	GetInterpolationInfo( &info, currentTime, interpolation_amount, &noMoreChanges );

	CVarHistory &history = m_VarHistory;

	if ( info.m_bHermite )
	{
		// base cast, we have 3 valid sample point
		_Interpolate_Hermite( pOut, info.frac, &history[info.oldest], &history[info.older], &history[info.newer] );
	}
	else if ( info.newer == info.older  )
	{
		// This means the server clock got way behind the client clock. Extrapolate the value here based on its
		// previous velocity (out to a certain amount).
		int realOlder = info.newer+1;
		if ( CInterpolationContext::IsExtrapolationAllowed() &&
			IsValidIndex( realOlder ) &&
			history[realOlder].changetime != 0.0 &&
			interpolation_amount > 0.000001f &&
			CInterpolationContext::GetLastTimeStamp() <= m_LastNetworkedTime )
		{
			// At this point, we know we're out of data and we have the ability to get a velocity to extrapolate with.
			//
			// However, we only want to extraploate if the server is choking. We don't want to extrapolate if 
			// the object legimately stopped moving and the server stopped sending updates for it.
			//
			// The way we know that the server is choking is if we haven't heard ANYTHING from it for a while.
			// The server's update interval should be at least as often as our interpolation amount (otherwise,
			// we wouldn't have the ability to interpolate).
			//
			// So right here, if we see that we haven't gotten any server updates since the last interpolation
			// history update to this entity (and since we're in here, we know that we're out of interpolation data),
			// then we can assume that the server is choking and decide to extrapolate.
			//
			// The End

			// Use the velocity here (extrapolate up to 1/4 of a second).
			_Extrapolate( pOut, &history[realOlder], &history[info.newer], currentTime - interpolation_amount, cl_extrapolate_amount.GetFloat() );
		}
		else
		{
			_Interpolate( pOut, info.frac, &history[info.older], &history[info.newer] );
		}
	}
	else
	{
		_Interpolate( pOut, info.frac, &history[info.older], &history[info.newer] );
	}
}

template< typename Type, bool IS_ARRAY >
inline int CInterpolatedVarArrayBase<Type, IS_ARRAY>::Interpolate( float currentTime, float interpolation_amount )
{
	int noMoreChanges = 0;
	
	CInterpolationInfo info;
	if (!GetInterpolationInfo( &info, currentTime, interpolation_amount, &noMoreChanges ))
		return noMoreChanges;

	
	CVarHistory &history = m_VarHistory;

	if ( m_bDebug )
	{
		// "value will hold" means we are either extrapolating, or the samples in GetInterpolationInfo are all the same... In either case there are no more "changes" until we latch a new
		//  value and we can remove this var from the interpolated var list (bit perf optimization)
		Msg( "%s Interpolate at %f%s\n", GetDebugName(), currentTime, noMoreChanges ? " [value will hold]" : "" );
	}


#ifdef INTERPOLATEDVAR_PARANOID_MEASUREMENT
	Type *backupValues = (Type*)_alloca( m_nMaxCount * sizeof(Type) );
	memcpy( backupValues, m_pValue, sizeof( Type ) * m_nMaxCount );
#endif

	if ( info.m_bHermite )
	{
		// base cast, we have 3 valid sample point
		_Interpolate_Hermite( m_pValue, info.frac, &history[info.oldest], &history[info.older], &history[info.newer] );
	}
	else if ( info.newer == info.older  )
	{
		// This means the server clock got way behind the client clock. Extrapolate the value here based on its
		// previous velocity (out to a certain amount).
		int realOlder = info.newer+1;
		if ( CInterpolationContext::IsExtrapolationAllowed() &&
			IsValidIndex( realOlder ) &&
			history[realOlder].changetime != 0.0 &&
			interpolation_amount > 0.000001f &&
			CInterpolationContext::GetLastTimeStamp() <= m_LastNetworkedTime )
		{
			// At this point, we know we're out of data and we have the ability to get a velocity to extrapolate with.
			//
			// However, we only want to extraploate if the server is choking. We don't want to extrapolate if 
			// the object legimately stopped moving and the server stopped sending updates for it.
			//
			// The way we know that the server is choking is if we haven't heard ANYTHING from it for a while.
			// The server's update interval should be at least as often as our interpolation amount (otherwise,
			// we wouldn't have the ability to interpolate).
			//
			// So right here, if we see that we haven't gotten any server updates since the last interpolation
			// history update to this entity (and since we're in here, we know that we're out of interpolation data),
			// then we can assume that the server is choking and decide to extrapolate.
			//
			// The End

			// Use the velocity here (extrapolate up to 1/4 of a second).
			_Extrapolate( m_pValue, &history[realOlder], &history[info.newer], currentTime - interpolation_amount, cl_extrapolate_amount.GetFloat() );
		}
		else
		{
			_Interpolate( m_pValue, info.frac, &history[info.older], &history[info.newer] );
		}
	}
	else
	{
		_Interpolate( m_pValue, info.frac, &history[info.older], &history[info.newer] );
	}

#ifdef INTERPOLATEDVAR_PARANOID_MEASUREMENT
	if ( memcmp( backupValues, m_pValue, sizeof( Type ) * m_nMaxCount ) != 0 )
	{
		extern int g_nInterpolatedVarsChanged;
		extern bool g_bRestoreInterpolatedVarValues;
		
		++g_nInterpolatedVarsChanged;

		// This undoes the work that we do in here so if someone is in the debugger, they
		// can find out which variable changed.
		if ( g_bRestoreInterpolatedVarValues )
		{
			memcpy( m_pValue, backupValues, sizeof( Type ) * m_nMaxCount );
			return noMoreChanges;
		}
	}
#endif

	// Clear out all entries before the oldest since we should never access them again.
	// Usually, Interpolate() calls never go backwards in time, but C_BaseAnimating::BecomeRagdollOnClient for one
	// goes slightly back in time
	RemoveEntriesPreviousTo( currentTime - interpolation_amount - EXTRA_INTERPOLATION_HISTORY_STORED );
	return noMoreChanges;
}


template< typename Type, bool IS_ARRAY >
void CInterpolatedVarArrayBase<Type, IS_ARRAY>::GetDerivative( Type *pOut, float currentTime )
{
	CInterpolationInfo info;
	if (!GetInterpolationInfo( &info, currentTime, m_InterpolationAmount, NULL ))
		return;

	if ( info.m_bHermite )
	{
		_Derivative_Hermite( pOut, info.frac, &m_VarHistory[info.oldest], &m_VarHistory[info.older], &m_VarHistory[info.newer] );
	}
	else
	{
		_Derivative_Linear( pOut, &m_VarHistory[info.older], &m_VarHistory[info.newer] );
	}
}


template< typename Type, bool IS_ARRAY >
void CInterpolatedVarArrayBase<Type, IS_ARRAY>::GetDerivative_SmoothVelocity( Type *pOut, float currentTime )
{
	CInterpolationInfo info;
	if (!GetInterpolationInfo( &info, currentTime, m_InterpolationAmount, NULL ))
		return;

	CVarHistory &history = m_VarHistory;
	bool bExtrapolate = false;
	int realOlder = 0;
	
	if ( info.m_bHermite )
	{
		_Derivative_Hermite_SmoothVelocity( pOut, info.frac, &history[info.oldest], &history[info.older], &history[info.newer] );
		return;
	}
	else if ( info.newer == info.older && CInterpolationContext::IsExtrapolationAllowed() )
	{
		// This means the server clock got way behind the client clock. Extrapolate the value here based on its
		// previous velocity (out to a certain amount).
		realOlder = info.newer+1;
		if ( IsValidIndex( realOlder ) && history[realOlder].changetime != 0.0 )
		{
			// At this point, we know we're out of data and we have the ability to get a velocity to extrapolate with.
			//
			// However, we only want to extraploate if the server is choking. We don't want to extrapolate if 
			// the object legimately stopped moving and the server stopped sending updates for it.
			//
			// The way we know that the server is choking is if we haven't heard ANYTHING from it for a while.
			// The server's update interval should be at least as often as our interpolation amount (otherwise,
			// we wouldn't have the ability to interpolate).
			//
			// So right here, if we see that we haven't gotten any server updates for a whole interpolation 
			// interval, then we know the server is choking.
			//
			// The End
			if ( m_InterpolationAmount > 0.000001f &&
				 CInterpolationContext::GetLastTimeStamp() <= (currentTime - m_InterpolationAmount) )
			{
				bExtrapolate = true;
			}
		}
	}

	if ( bExtrapolate )
	{
		// Get the velocity from the last segment.
		_Derivative_Linear( pOut, &history[realOlder], &history[info.newer] );

		// Now ramp it to zero after cl_extrapolate_amount..
		float flDestTime = currentTime - m_InterpolationAmount;
		float diff = flDestTime - history[info.newer].changetime;
		diff = clamp( diff, 0.f, cl_extrapolate_amount.GetFloat() * 2 );
		if ( diff > cl_extrapolate_amount.GetFloat() )
		{
			float scale = 1 - (diff - cl_extrapolate_amount.GetFloat()) / cl_extrapolate_amount.GetFloat();
			for ( int i=0; i < m_nMaxCount; i++ )
			{
				pOut[i] *= scale;
			}
		}
	}
	else
	{
		_Derivative_Linear( pOut, &history[info.older], &history[info.newer] );
	}

}


template< typename Type, bool IS_ARRAY >
inline int CInterpolatedVarArrayBase<Type, IS_ARRAY>::Interpolate( float currentTime )
{
	return Interpolate( currentTime, m_InterpolationAmount );
}

template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::Copy( IInterpolatedVar *pInSrc )
{
	CInterpolatedVarArrayBase<Type, IS_ARRAY> *pSrc = dynamic_cast< CInterpolatedVarArrayBase<Type, IS_ARRAY>* >( pInSrc );

	if ( !pSrc || pSrc->m_nMaxCount != m_nMaxCount )
	{
		if ( pSrc )
		{
			AssertMsg3( false, "pSrc->m_nMaxCount (%i) != m_nMaxCount (%i) for %s.", pSrc->m_nMaxCount, m_nMaxCount, m_pDebugName);
		}
		else
		{
			AssertMsg( false, "pSrc was null in CInterpolatedVarArrayBase<Type, IS_ARRAY>::Copy.");
		}

		return;
	}

	Assert( (m_fType & ~EXCLUDE_AUTO_INTERPOLATE) == (pSrc->m_fType & ~EXCLUDE_AUTO_INTERPOLATE) );
	Assert( m_pDebugName == pSrc->GetDebugName() );

	for ( int i=0; i < m_nMaxCount; i++ )
	{
		m_LastNetworkedValue[i] = pSrc->m_LastNetworkedValue[i];
		m_bLooping[i] = pSrc->m_bLooping[i];
	}

	m_LastNetworkedTime = pSrc->m_LastNetworkedTime;

	// Copy the entries.
	m_VarHistory.RemoveAll();

	for ( int i = 0; i < pSrc->m_VarHistory.Count(); i++ )
	{
		int newslot = m_VarHistory.AddToTail();

		CInterpolatedVarEntry *dest = &m_VarHistory[newslot];
		CInterpolatedVarEntry *src	= &pSrc->m_VarHistory[i];
		dest->NewEntry( src->GetValue(), m_nMaxCount, src->changetime );
	}
}

template< typename Type, bool IS_ARRAY >
inline const Type& CInterpolatedVarArrayBase<Type, IS_ARRAY>::GetPrev( int iArrayIndex ) const
{
	Assert( m_pValue );
	Assert( iArrayIndex >= 0 && iArrayIndex < m_nMaxCount );

	if ( m_VarHistory.Count() > 1 )
	{
		return m_VarHistory[1].GetValue()[iArrayIndex];
	}
	return m_pValue[ iArrayIndex ];
}

template< typename Type, bool IS_ARRAY >
inline const Type& CInterpolatedVarArrayBase<Type, IS_ARRAY>::GetCurrent( int iArrayIndex ) const
{
	Assert( m_pValue );
	Assert( iArrayIndex >= 0 && iArrayIndex < m_nMaxCount );

	if ( m_VarHistory.Count() > 0 )
	{
		return m_VarHistory[0].GetValue()[iArrayIndex];
	}
	return m_pValue[ iArrayIndex ];
}

template< typename Type, bool IS_ARRAY >
inline float CInterpolatedVarArrayBase<Type, IS_ARRAY>::GetInterval() const
{	
	if ( m_VarHistory.Count() > 1 )
	{
		return m_VarHistory[0].changetime - m_VarHistory[1].changetime;
		}

	return 0.0f;
}

template< typename Type, bool IS_ARRAY >
inline bool	CInterpolatedVarArrayBase<Type, IS_ARRAY>::IsValidIndex( int i )
{
	return m_VarHistory.IsValidIndex( i );
}

template< typename Type, bool IS_ARRAY >
inline Type	*CInterpolatedVarArrayBase<Type, IS_ARRAY>::GetHistoryValue( int index, float& changetime, int iArrayIndex )
{
	Assert( iArrayIndex >= 0 && iArrayIndex < m_nMaxCount );
	if ( m_VarHistory.IsIdxValid(index) )
	{
		CInterpolatedVarEntry *entry = &m_VarHistory[ index ];
		changetime = entry->changetime;
		return &entry->GetValue()[ iArrayIndex ];
	}
	else
	{
		changetime = 0.0f;
		return NULL;
	}
}

template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::SetHistoryValuesForItem( int item, Type& value )
{
	Assert( item >= 0 && item < m_nMaxCount );

	for ( int i = 0; i < m_VarHistory.Count(); i++ )
	{
		CInterpolatedVarEntry *entry = &m_VarHistory[ i ];
		entry->GetValue()[ item ] = value;
	}
}

template< typename Type, bool IS_ARRAY >
inline void	CInterpolatedVarArrayBase<Type, IS_ARRAY>::SetLooping( bool looping, int iArrayIndex )
{
	Assert( iArrayIndex >= 0 && iArrayIndex < m_nMaxCount );
	m_bLooping[ iArrayIndex ] = looping;
}

template< typename Type, bool IS_ARRAY >
inline void	CInterpolatedVarArrayBase<Type, IS_ARRAY>::SetMaxCount( int newmax )
{
	bool changed = ( newmax != m_nMaxCount ) ? true : false;

	// BUGBUG: Support 0 length properly?
	newmax = MAX(1,newmax);

	m_nMaxCount = newmax;
	// Wipe everything any time this changes!!!
	if ( changed )
	{
		delete [] m_bLooping;
		delete [] m_LastNetworkedValue;
		m_bLooping = new byte[m_nMaxCount];
		m_LastNetworkedValue = new Type[m_nMaxCount];
		memset( m_bLooping, 0, sizeof(byte) * m_nMaxCount);
		memset( m_LastNetworkedValue, 0, sizeof(Type) * m_nMaxCount);

		Reset();
	}
}


template< typename Type, bool IS_ARRAY >
inline int CInterpolatedVarArrayBase<Type, IS_ARRAY>::GetMaxCount() const
{
	return m_nMaxCount;
}


template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::_Interpolate( Type *out, float frac, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end )
{
	Assert( start );
	Assert( end );
	
	if ( start == end )
	{
		// quick exit
		for ( int i = 0; i < m_nMaxCount; i++ )
		{
			out[i] = end->GetValue()[i];
			Lerp_Clamp( out[i] );
		}
		return;
	}

	Assert( frac >= 0.0f && frac <= 1.0f );

	// Note that QAngle has a specialization that will do quaternion interpolation here...
	for ( int i = 0; i < m_nMaxCount; i++ )
	{
		if ( m_bLooping[ i ] )
		{
			out[i] = LoopingLerp( frac, start->GetValue()[i], end->GetValue()[i] );
		}
		else
		{
			out[i] = Lerp( frac, start->GetValue()[i], end->GetValue()[i] );
		}
		Lerp_Clamp( out[i] );
	}
}


template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::_Extrapolate( 
	Type *pOut,
	CInterpolatedVarEntry *pOld,
	CInterpolatedVarEntry *pNew,
	float flDestinationTime,
	float flMaxExtrapolationAmount
	)
{
	if ( fabs( pOld->changetime - pNew->changetime ) < 0.001f || flDestinationTime <= pNew->changetime )
	{
		for ( int i=0; i < m_nMaxCount; i++ )
			pOut[i] = pNew->GetValue()[i];
	}
	else
	{
		float flExtrapolationAmount = MIN( flDestinationTime - pNew->changetime, flMaxExtrapolationAmount );

		float divisor = 1.0f / (pNew->changetime - pOld->changetime);
		for ( int i=0; i < m_nMaxCount; i++ )
		{
			pOut[i] = ExtrapolateInterpolatedVarType( pOld->GetValue()[i], pNew->GetValue()[i], divisor, flExtrapolationAmount );
		}
	}
}


template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::TimeFixup2_Hermite( 
	typename CInterpolatedVarArrayBase<Type, IS_ARRAY>::CInterpolatedVarEntry &fixup,
	typename CInterpolatedVarArrayBase<Type, IS_ARRAY>::CInterpolatedVarEntry*& prev, 
	typename CInterpolatedVarArrayBase<Type, IS_ARRAY>::CInterpolatedVarEntry*& start, 
	float dt1
	)
{
	float dt2 = start->changetime - prev->changetime;

	// If times are not of the same interval renormalize the earlier sample to allow for uniform hermite spline interpolation
	if ( fabs( dt1 - dt2 ) > 0.0001f &&
		dt2 > 0.0001f )
	{
		// Renormalize
		float frac = dt1 / dt2;

		// Fixed interval into past
		fixup.changetime = start->changetime - dt1;

		for ( int i = 0; i < m_nMaxCount; i++ )
		{
			if ( m_bLooping[i] )
			{
				fixup.GetValue()[i] = LoopingLerp( 1-frac, prev->GetValue()[i], start->GetValue()[i] );
			}
			else
			{
				fixup.GetValue()[i] = Lerp( 1-frac, prev->GetValue()[i], start->GetValue()[i] );
			}
		}

		// Point previous sample at fixed version
		prev = &fixup;
	}
}


template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::TimeFixup_Hermite( 
	typename CInterpolatedVarArrayBase<Type, IS_ARRAY>::CInterpolatedVarEntry &fixup,
	typename CInterpolatedVarArrayBase<Type, IS_ARRAY>::CInterpolatedVarEntry*& prev, 
	typename CInterpolatedVarArrayBase<Type, IS_ARRAY>::CInterpolatedVarEntry*& start, 
	typename CInterpolatedVarArrayBase<Type, IS_ARRAY>::CInterpolatedVarEntry*& end	)
{
	TimeFixup2_Hermite( fixup, prev, start, end->changetime - start->changetime );
}


template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::_Interpolate_Hermite( 
	Type *out, 
	float frac, 
	CInterpolatedVarEntry *prev, 
	CInterpolatedVarEntry *start, 
	CInterpolatedVarEntry *end, 
	bool looping )
{
	Assert( start );
	Assert( end );

	// Disable range checks because we can produce weird values here and it's not an error.
	// After interpolation, we will clamp the values.
	CDisableRangeChecks disableRangeChecks; 

	CInterpolatedVarEntry fixup;
	fixup.Init(m_nMaxCount);
	TimeFixup_Hermite( fixup, prev, start, end );

	for( int i = 0; i < m_nMaxCount; i++ )
	{
		// Note that QAngle has a specialization that will do quaternion interpolation here...
		if ( m_bLooping[ i ] )
		{
			out[ i ] = LoopingLerp_Hermite( frac, prev->GetValue()[i], start->GetValue()[i], end->GetValue()[i] );
		}
		else
		{
			out[ i ] = Lerp_Hermite( frac, prev->GetValue()[i], start->GetValue()[i], end->GetValue()[i] );
		}

		// Clamp the output from interpolation. There are edge cases where something like m_flCycle
		// can get set to a really high or low value when we set it to zero after a really small
		// time interval (the hermite blender will think it's got a really high velocity and
		// skyrocket it off into la-la land).
		Lerp_Clamp( out[i] );
	}
}

template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::_Derivative_Hermite( 
	Type *out, 
	float frac, 
	CInterpolatedVarEntry *prev, 
	CInterpolatedVarEntry *start, 
	CInterpolatedVarEntry *end )
{
	Assert( start );
	Assert( end );

	// Disable range checks because we can produce weird values here and it's not an error.
	// After interpolation, we will clamp the values.
	CDisableRangeChecks disableRangeChecks; 

	CInterpolatedVarEntry fixup;
	fixup.value = (Type*)_alloca( sizeof(Type) * m_nMaxCount );
	TimeFixup_Hermite( fixup, prev, start, end );

	float divisor = 1.0f / (end->changetime - start->changetime);

	for( int i = 0; i < m_nMaxCount; i++ )
	{
		Assert( !m_bLooping[ i ] );
		out[i] = Derivative_Hermite( frac, prev->GetValue()[i], start->GetValue()[i], end->GetValue()[i] );
		out[i] *= divisor;
	}
}


template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::_Derivative_Hermite_SmoothVelocity( 
	Type *out, 
	float frac, 
	CInterpolatedVarEntry *b, 
	CInterpolatedVarEntry *c, 
	CInterpolatedVarEntry *d )
{
	CInterpolatedVarEntry fixup;
	fixup.Init(m_nMaxCount);
	TimeFixup_Hermite( fixup, b, c, d );
	for ( int i=0; i < m_nMaxCount; i++ )
	{
		Type prevVel = (c->GetValue()[i] - b->GetValue()[i]) / (c->changetime - b->changetime);
		Type curVel  = (d->GetValue()[i] - c->GetValue()[i]) / (d->changetime - c->changetime);
		out[i] = Lerp( frac, prevVel, curVel );
	}
}


template< typename Type, bool IS_ARRAY >
inline void CInterpolatedVarArrayBase<Type, IS_ARRAY>::_Derivative_Linear( 
	Type *out, 
	CInterpolatedVarEntry *start, 
	CInterpolatedVarEntry *end )
{
	if ( start == end || fabs( start->changetime - end->changetime ) < 0.0001f )
	{
		for( int i = 0; i < m_nMaxCount; i++ )
		{
			out[ i ] = start->GetValue()[i] * 0;
		}
	}
	else 
	{
		float divisor = 1.0f / (end->changetime - start->changetime);
		for( int i = 0; i < m_nMaxCount; i++ )
		{
			out[ i ] = (end->GetValue()[i] - start->GetValue()[i]) * divisor;
		}
	}
}


template< typename Type, bool IS_ARRAY >
inline bool CInterpolatedVarArrayBase<Type, IS_ARRAY>::ValidOrder()
{
	float newestchangetime = 0.0f;
	bool first = true;
	for ( int i = 0; i < m_VarHistory.Count(); i++ )
	{
		CInterpolatedVarEntry *entry = &m_VarHistory[ i ];
		if ( first )
		{
			first = false;
			newestchangetime = entry->changetime;
			continue;
		}

		// They should get older as wel walk backwards
		if ( entry->changetime > newestchangetime )
		{
			Assert( 0 );
			return false;
		}

		newestchangetime = entry->changetime;
	}

	return true;
}

template< typename Type, int COUNT >
class CInterpolatedVarArray : public CInterpolatedVarArrayBase<Type, true >
{
public:
	CInterpolatedVarArray( const char *pDebugName = "no debug name" )
		: CInterpolatedVarArrayBase<Type, true>( pDebugName )
	{
		this->SetMaxCount( COUNT );
	}
};


// -------------------------------------------------------------------------------------------------------------- //
// CInterpolatedVar.
// -------------------------------------------------------------------------------------------------------------- //

template< typename Type >
class CInterpolatedVar : public CInterpolatedVarArrayBase< Type, false >
{
public:
	CInterpolatedVar( const char *pDebugName = NULL )
		: CInterpolatedVarArrayBase< Type, false >(pDebugName) 
	{
		this->SetMaxCount( 1 );
	}
};

#include "tier0/memdbgoff.h"

#endif // INTERPOLATEDVAR_H
