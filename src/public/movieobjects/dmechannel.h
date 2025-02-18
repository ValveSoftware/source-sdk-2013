//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The channel class - connects elements together, and allows for logging of data
//
//=============================================================================

#ifndef DMECHANNEL_H
#define DMECHANNEL_H
#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmeoperator.h"
#include "movieobjects/dmelog.h"
#include "movieobjects/dmeclip.h"
#include "movieobjects/proceduralpresets.h"
#include "datamodel/idatamodel.h"
#include "datamodel/dmehandle.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeClip;
class CDmeChannel;


//-----------------------------------------------------------------------------
// different channel modes of operation
//-----------------------------------------------------------------------------
enum ChannelMode_t
{
	CM_OFF,
	CM_PASS,
	CM_RECORD,
	CM_PLAY,
};

enum PlayMode_t
{
	PM_HOLD,
	PM_LOOP,
};


//-----------------------------------------------------------------------------
// A class managing channel recording
//-----------------------------------------------------------------------------
class CDmeChannelRecordingMgr
{
public:
	// constructor
	CDmeChannelRecordingMgr();

	// Activates, deactivates layer recording.
	void StartLayerRecording( const char *pUndoRedoDesc, const DmeLog_TimeSelection_t *pTimeSelection = NULL );
	void FinishLayerRecording( float flThreshold, bool bFlattenLayers = true );

	// Adds a channel to the recording layer
	void AddChannelToRecordingLayer( CDmeChannel *pChannel, CDmeClip *pRoot = NULL, CDmeClip *pShot = NULL );

	// Used to iterate over all channels currently being recorded
	// NOTE: Use CDmeChannel::AddToRecordingLayer to add a channel to the recording layer
	int GetLayerRecordingChannelCount();
	CDmeChannel* GetLayerRecordingChannel( int nIndex );

	// Computes time selection info in log time for a particular recorded channel
	// NOTE: Only valid if IsUsingTimeSelection() returns true
	void GetLocalTimeSelection( DmeLog_TimeSelection_t& selection, int nIndex );

	// Methods which control various aspects of recording
	void UpdateTimeAdvancing( bool bPaused, DmeTime_t tCurTime );
	void UpdateRecordingTimeSelectionTimes( const DmeLog_TimeSelection_t& timeSelection );
	void SetIntensityOnAllLayers( float flIntensity );
	void SetRecordingMode( RecordingMode_t mode );
	void SetPresetValue( CDmeChannel* pChannel, CDmAttribute *pPresetValue );

	void SetProceduralTarget( int nProceduralMode, const CDmAttribute *pTarget );
	void SetProceduralTarget( int nProceduralMode, const CUtlVector< KeyValues * >& list );
	int GetProceduralType() const;
	const CDmAttribute *GetProceduralTarget() const;
	const CUtlVector< KeyValues * > &GetPasteTarget() const;

	// Methods to query aspects of recording
	bool IsTimeAdvancing() const;
	bool IsUsingDetachedTimeSelection() const;
	bool IsUsingTimeSelection() const;

private:
	struct LayerChannelInfo_t
	{
		LayerChannelInfo_t() : m_pPresetValue( 0 ) {}

		CDmeHandle< CDmeChannel > m_Channel;
		DmeClipStack_t m_ClipStack;
		CDmAttribute* m_pPresetValue;
	};

	// Methods available for CDmeChannel
	bool ShouldRecordUsingTimeSelection() const;

	// Internal methods
	void FlattenLayers( float flThreshhold );
	void RemoveAllChannelsFromRecordingLayer( );

	bool									m_bActive : 1;
	bool									m_bSavedUndoState : 1;
	bool									m_bUseTimeSelection : 1;
	CUtlVector< LayerChannelInfo_t >		m_LayerChannels;
	DmeLog_TimeSelection_t					m_TimeSelection;
	int										m_nRevealType;
	const CDmAttribute						*m_pRevealTarget;
	CUtlVector< KeyValues * >				m_PasteTarget;

	friend CDmeChannel;
};

// Singleton
extern CDmeChannelRecordingMgr *g_pChannelRecordingMgr;


//-----------------------------------------------------------------------------
// A class representing a channel
//-----------------------------------------------------------------------------
class CDmeChannel : public CDmeOperator
{
	DEFINE_ELEMENT( CDmeChannel, CDmeOperator );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

	void SetInput ( CDmElement* pElement, const char* pAttribute, int index = 0 );
	void SetOutput( CDmElement* pElement, const char* pAttribute, int index = 0 );

	void SetInput( CDmAttribute *pAttribute, int index = 0 );
	void SetOutput( CDmAttribute *pAttribute, int index = 0 );

	CDmElement *GetFromElement() const;
	CDmElement *GetToElement() const;

	CDmAttribute *GetFromAttribute();
	CDmAttribute *GetToAttribute();

	int GetFromArrayIndex() const;
	int GetToArrayIndex() const;

	ChannelMode_t GetMode();
	void SetMode( ChannelMode_t mode );

	void ClearLog();
	CDmeLog *GetLog();
	void SetLog( CDmeLog *pLog );
	CDmeLog *CreateLog( DmAttributeType_t type );
	template < class T > CDmeTypedLog<T> *CreateLog();

	void ClearTimeMetric();
	void SetCurrentTime( DmeTime_t time, DmeTime_t start, DmeTime_t end );
	void SetCurrentTime( DmeTime_t time ); // Simple version. Only works if multiple active channels clips do not reference the same channels
	DmeTime_t GetCurrentTime() const;

	void SetChannelToPlayToSelf( const char *outputAttributeName, float defaultValue, bool force = false );

	// need this until we have the EditApply message queue
	void OnAttributeChanged( CDmAttribute *pAttribute );

	template< class T >
	bool	GetCurrentPlaybackValue( T& value );
	template< class T >
	bool	GetPlaybackValueAtTime( DmeTime_t time, T& value );

	void Play();

	void SetNextKeyCurveType( int nCurveType );

	// Builds a clip stack for the channel
	CDmeClip* FindOwnerClipForChannel( CDmeClip *pRoot );
	bool BuildClipStack( DmeClipStack_t *pClipStack, CDmeClip *pRoot, CDmeClip *pShot );

protected:
	// Used to cache off handles to attributes
	CDmAttribute* SetupFromAttribute();
	CDmAttribute* SetupToAttribute();

	void Record();
	void Pass();

	CDmaElement< CDmElement > m_fromElement;
	CDmaString m_fromAttribute;
	CDmaVar< int > m_fromIndex;
	CDmaElement< CDmElement > m_toElement;
	CDmaString m_toAttribute;
	CDmaVar< int > m_toIndex;
	CDmaVar< int > m_mode;
	CDmaElement< CDmeLog > m_log;

	DmAttributeHandle_t m_FromAttributeHandle;
	DmAttributeHandle_t m_ToAttributeHandle;

	DmeTime_t m_timeOutsideTimeframe;
	DmeTime_t m_tCurrentTime;
	DmeTime_t m_tPreviousTime;

	int m_nRecordLayerIndex;
	int m_nNextCurveType;

	friend class CDmeChannelRecordingMgr;
};


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
template < class T > 
inline CDmeTypedLog<T> *CDmeChannel::CreateLog()
{
	return CastElement< CDmeTypedLog<T> >( CreateLog( CDmAttributeInfo<T>::AttributeType() ) );
}

inline CDmAttribute *CDmeChannel::GetFromAttribute()
{
	CDmAttribute *pAttribute = g_pDataModel->GetAttribute( m_FromAttributeHandle );
	if ( !pAttribute )
	{
		pAttribute = SetupFromAttribute();
	}
	return pAttribute;
}

inline CDmAttribute *CDmeChannel::GetToAttribute()
{
	CDmAttribute *pAttribute = g_pDataModel->GetAttribute( m_ToAttributeHandle );
	if ( !pAttribute )
	{
		pAttribute = SetupToAttribute();
	}
	return pAttribute;
}

template< class T >
inline bool CDmeChannel::GetPlaybackValueAtTime( DmeTime_t time, T& value )
{
	CDmeTypedLog< T > *pLog = CastElement< CDmeTypedLog< T > >( GetLog() );
	if ( !pLog || pLog->IsEmpty() )
		return false;

	DmeTime_t t0 = pLog->GetBeginTime();
	DmeTime_t tn = pLog->GetEndTime();

	PlayMode_t pmode = PM_HOLD;
	switch ( pmode )
	{
	case PM_HOLD:
		time = clamp( time, t0, tn );
		break;

	case PM_LOOP:
		if ( tn == t0 )
		{
			time = t0;
		}
		else
		{
			time -= t0;
			time = time % ( tn - t0 );
			time += t0;
		}
		break;
	}

	value = pLog->GetValue( time );
	return true;
}

template< class T >
inline bool CDmeChannel::GetCurrentPlaybackValue( T& value )
{
	return GetPlaybackValueAtTime( GetCurrentTime(), value );
}

#endif // DMECHANNEL_H
