//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SMOOTH_AVERAGE_H
#define SMOOTH_AVERAGE_H
#ifdef _WIN32
#pragma once
#endif


#include "utldict.h"



// Use this macro around any value, and it'll queue up the results given to it nTimes and 
// provide a running average.
#define SMOOTH_AVERAGE( value, nCount ) CalcSmoothAverage( value, nCount, __FILE__, __LINE__ )


// Same as their counterpart functions but they return more info in a CTimingInfo structure.
#define SMOOTH_AVERAGE_STRUCT( value, nCount )				CalcSmoothAverage_Struct( value, nCount, __FILE__, __LINE__ )
#define SUM_OVER_TIME_INTERVAL_STRUCT( value, nSeconds )	SumOverTimeInterval_Struct( value, nSeconds, __FILE__, __LINE__ )


template< class T >
class CTimingInfo
{
public:
	T	m_AverageValue;	// Note: this will be the SUM of the values if using SUM_OVER_TIME_INTERVAL.
	
	// The high and low points for m_AverageValue over the time interval.
	T	m_HighAverage;	
	T	m_LowAverage;
	
	// The high and low points for the value itself over the time interval.
	T	m_HighValue;
	T	m_LowValue;
};


template< class T >
class CAveragesInfo
{
public:
	class CEntry
	{
	public:
		T	m_Average;
		T	m_Value;
	};

public:	
	CUtlVector< CEntry > m_Values;
	int m_iCurValue;
};


template< class T >
class CAveragesInfo_TimeBased
{
public:
	class CEntry
	{
	public:
		CCycleCount m_Time;	// When this sample was taken.
		T m_Value;
		T m_Average;
	};
	
	CUtlVector<CEntry> m_Values;
};


#if 0
template< class T >
inline CTimingInfo< T > CalcSmoothAverage_Struct( const T &value, int nTimes, const char *pFilename, int iLine )
{
	// Find an entry at this file and line.
	char fullStr[1024];
	Q_snprintf( fullStr, sizeof( fullStr ), "%s_%i", pFilename, iLine );
	
	int index = s_SmoothAverages.Find( fullStr );
	CAveragesInfo<T> *pInfo;
	if ( index == s_SmoothAverages.InvalidIndex() )
	{
		pInfo = new CAveragesInfo<T>;
		index = s_SmoothAverages.Insert( fullStr, pInfo );
	}
	else
	{
		pInfo = (CAveragesInfo<T>*)s_SmoothAverages[index];
	}
	
	// Add the new value.
	int newValueIndex;
	CAveragesInfo< T >::CEntry entry;
	entry.m_Value = value;
	if ( pInfo->m_Values.Count() < nTimes )
	{
		newValueIndex = pInfo->m_Values.AddToTail( entry );
		pInfo->m_iCurValue = 0;
	}
	else
	{
		newValueIndex = pInfo->m_iCurValue;
		pInfo->m_Values[pInfo->m_iCurValue] = entry;
		pInfo->m_iCurValue = (pInfo->m_iCurValue+1) % pInfo->m_Values.Count();
	}

	CTimingInfo< T > info;
	info.m_AverageValue = pInfo->m_Values[0].m_Value;
	
	info.m_HighAverage = pInfo->m_Values[0].m_Average;
	info.m_LowAverage = pInfo->m_Values[0].m_Average;
	
	info.m_HighValue = pInfo->m_Values[0].m_Value;
	info.m_LowValue = pInfo->m_Values[0].m_Value;

	for ( int i=1; i < pInfo->m_Values.Count(); i++ )
	{
		if ( i != newValueIndex )
		{
			info.m_HighAverage = max( pInfo->m_Values[i].m_Average, info.m_HighAverage );
			info.m_LowAverage = min( pInfo->m_Values[i].m_Average, info.m_LowAverage );
		}

		info.m_HighValue = max( pInfo->m_Values[i].m_Value, info.m_HighValue );
		info.m_LowValue = min( pInfo->m_Values[i].m_Value, info.m_LowValue );

		info.m_AverageValue += pInfo->m_Values[i].m_Value;
	}

	info.m_AverageValue /= pInfo->m_Values.Count();
	pInfo->m_Values[newValueIndex].m_Average = info.m_AverageValue;
	return info;
}
#endif 

template< class T >
inline T CalcSmoothAverage( const T &value, int nTimes, const char *pFilename, int iLine )
{
	CTimingInfo< T > info = CalcSmoothAverage_Struct( value, nTimes, pFilename, iLine );
	return info.m_AverageValue;
};


template< class T >
inline CTimingInfo< T > SumOverTimeInterval_Struct( const T &value, float nSeconds, const char *pFilename, int iLine )
{
	static CUtlDict< CAveragesInfo_TimeBased< T >*, int > s_SmoothAverages;

	char fullStr[1024];
	Q_snprintf( fullStr, sizeof( fullStr ), "%s_%i", pFilename, iLine );
	
	int index = s_SmoothAverages.Find( fullStr );
	CAveragesInfo_TimeBased<T> *pInfo;
	if ( index == s_SmoothAverages.InvalidIndex() )
	{
		pInfo = new CAveragesInfo_TimeBased<T>;
		index = s_SmoothAverages.Insert( fullStr, pInfo );
	}
	else
	{
		pInfo = s_SmoothAverages[index];
	}
	
	// Get the current time now.
	CCycleCount curTime;
	curTime.Sample();
	
	// Get rid of old samples.
	while ( pInfo->m_Values.Count() > 0 && (curTime.GetSeconds() - pInfo->m_Values[0].m_Time.GetSeconds()) > nSeconds )
		pInfo->m_Values.Remove( 0 );

	// Add on the new sample.
	typename CAveragesInfo_TimeBased< T >::CEntry newEntry;
	newEntry.m_Time = curTime;
	newEntry.m_Value = value;
	int newValueIndex = pInfo->m_Values.AddToTail( newEntry );

	CTimingInfo< T > info;
	info.m_AverageValue = pInfo->m_Values[0].m_Value;
	
	info.m_HighAverage = pInfo->m_Values[0].m_Average;
	info.m_LowAverage = pInfo->m_Values[0].m_Average;
	
	info.m_HighValue = pInfo->m_Values[0].m_Value;
	info.m_LowValue = pInfo->m_Values[0].m_Value;

	for ( int i=1; i < pInfo->m_Values.Count(); i++ )
	{
		if ( i != newValueIndex )
		{
			info.m_HighAverage = max( pInfo->m_Values[i].m_Average, info.m_HighAverage );
			info.m_LowAverage = min( pInfo->m_Values[i].m_Average, info.m_LowAverage );
		}

		info.m_HighValue = max( pInfo->m_Values[i].m_Value, info.m_HighValue );
		info.m_LowValue = min( pInfo->m_Values[i].m_Value, info.m_LowValue );

		info.m_AverageValue += pInfo->m_Values[i].m_Value;
	}

	info.m_AverageValue /= pInfo->m_Values.Count();
	pInfo->m_Values[newValueIndex].m_Average = info.m_AverageValue;
	return info;
}


template< class T >
inline CTimingInfo< T > SumOverTimeInterval( const T &value, float nSeconds, const char *pFilename, int iLine )
{
	CTimingInfo< T > info = SumOverTimeInterval_Struct( value, nSeconds, pFilename, iLine );
	return info.m_AverageValue;
}


#endif // SMOOTH_AVERAGE_H

