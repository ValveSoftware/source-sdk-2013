//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "rtime.h"
#include "econ_holidays.h"

//-----------------------------------------------------------------------------
// Purpose: Interface that answers the simple question "on the passed-in time,
//			would this holiday be active?". Any caching of calculations is left
//			up to subclasses.
//-----------------------------------------------------------------------------
class IIsHolidayActive
{
public:
	IIsHolidayActive( const char *pszHolidayName ) : m_pszHolidayName( pszHolidayName ) { }
	virtual ~IIsHolidayActive ( ) { }
	virtual bool IsActive( const CRTime& timeCurrent ) = 0;

	const char *GetHolidayName() const { return m_pszHolidayName; }

private:
	const char *m_pszHolidayName;
};

//-----------------------------------------------------------------------------
// Purpose: Always-disabled. Dummy event needed to map to slot zero for "disabled
//			holiday".
//-----------------------------------------------------------------------------
class CNoHoliday : public IIsHolidayActive
{
public:
	CNoHoliday() : IIsHolidayActive( "none" ) { }

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		return false;
	}
};

//-----------------------------------------------------------------------------
// Purpose: A holiday that lasts exactly one and only one day.
//-----------------------------------------------------------------------------
class CSingleDayHoliday : public IIsHolidayActive
{
public:
	CSingleDayHoliday( const char *pszName, int iMonth, int iDay )
		: IIsHolidayActive( pszName )
		, m_iMonth( iMonth )
		, m_iDay( iDay )
	{
		//
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		return m_iMonth == timeCurrent.GetMonth()
			&& m_iDay == timeCurrent.GetDayOfMonth();
	}

private:
	int m_iMonth;
	int m_iDay;
};

//-----------------------------------------------------------------------------
// Purpose: We want "week long" holidays to encompass at least two weekends,
//			so that players get plenty of time interacting with the holiday
//			features.
//-----------------------------------------------------------------------------
class CWeeksBasedHoliday : public IIsHolidayActive
{
public:
	CWeeksBasedHoliday( const char *pszName, int iMonth, int iDay, int iExtraWeeks )
		: IIsHolidayActive( pszName )
		, m_iMonth( iMonth )
		, m_iDay( iDay )
		, m_iExtraWeeks( iExtraWeeks )
		, m_iCachedCalculatedYear( 0 )
	{
		// We'll calculate the interval the first time we call IsActive().
	}

	void RecalculateTimeActiveInterval( int iYear )
	{
		// Get the date of the holiday.
		tm holiday_tm = { };
		holiday_tm.tm_mday = m_iDay;
		holiday_tm.tm_mon  = m_iMonth - 1;
		holiday_tm.tm_year = iYear - 1900; // convert to years since 1900
		mktime( &holiday_tm );

		// The event starts on the first Friday at least four days prior to the holiday.
		tm start_time_tm( holiday_tm );
		start_time_tm.tm_mday -= 4;							// Move back four days.
		mktime( &start_time_tm );
		int days_offset = start_time_tm.tm_wday - kFriday;	// Find the nearest prior Friday.
		if ( days_offset < 0 )
			days_offset += 7;
		start_time_tm.tm_mday -= days_offset;
		time_t start_time = mktime( &start_time_tm );

		// The event ends on the first Monday after the holiday, maybe plus some additional fudge
		// time.
		tm end_time_tm( holiday_tm );
		days_offset = 7 - (end_time_tm.tm_wday - kMonday);
		if ( days_offset >= 7 )
			days_offset -= 7;
		end_time_tm.tm_mday += days_offset + 7 * m_iExtraWeeks;
		time_t end_time = mktime( &end_time_tm );


		m_timeStart = start_time;
		m_timeEnd = end_time;

		// We're done and our interval data is cached.
		m_iCachedCalculatedYear = iYear;
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		const int iCurrentYear = timeCurrent.GetYear();
		if ( m_iCachedCalculatedYear != iCurrentYear )
			RecalculateTimeActiveInterval( iCurrentYear );

		return timeCurrent.GetRTime32() > m_timeStart
			&& timeCurrent.GetRTime32() < m_timeEnd;
	}

private:
	static const int kMonday = 1;
	static const int kFriday = 5;

	int m_iMonth;
	int m_iDay;
	int m_iExtraWeeks;
	
	// Filled out from RecalculateTimeActiveInterval().
	int m_iCachedCalculatedYear;

	RTime32 m_timeStart;
	RTime32 m_timeEnd;
};

//-----------------------------------------------------------------------------
// Purpose: A holiday that repeats on a certain time interval, like "every N days"
//			or "once every two months" or, uh, "any time there's a full moon".
//-----------------------------------------------------------------------------
class CCyclicalHoliday : public IIsHolidayActive
{
public:
	CCyclicalHoliday( const char *pszName, int iMonth, int iDay, int iYear, float fCycleLengthInDays, float fBonusTimeInDays )
		: IIsHolidayActive( pszName )
		, m_fCycleLengthInDays( fCycleLengthInDays )
		, m_fBonusTimeInDays( fBonusTimeInDays )
	{
		// When is our initial interval?
		tm holiday_tm = { };
		holiday_tm.tm_mday = iDay;
		holiday_tm.tm_mon  = iMonth - 1;
		holiday_tm.tm_year = iYear - 1900; // convert to years since 1900
		m_timeInitial = mktime( &holiday_tm );
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		// Days-to-seconds conversion.
		const int iSecondsPerDay = 24 * 60 * 60;

		// Convert our cycle/buffer times to seconds.
		const int iCycleLengthInSeconds = (int)(m_fCycleLengthInDays * iSecondsPerDay);
		const int iBufferTimeInSeconds  = (int)(m_fBonusTimeInDays * iSecondsPerDay);

		// How long has it been since we started this cycle?
		int iSecondsIntoCycle = (timeCurrent.GetRTime32() - m_timeInitial) % iCycleLengthInSeconds;

		// If we're within the buffer period right after the start of a cycle, we're active.
		if ( iSecondsIntoCycle < iBufferTimeInSeconds )
			return true;

		// If we're within the buffer period towards the end of a cycle, we're active.
		if ( iSecondsIntoCycle > iCycleLengthInSeconds - iBufferTimeInSeconds )
			return true;

		// Alas, normal mode for us.
		return false;
	}

private:
	time_t m_timeInitial ;

	float m_fCycleLengthInDays;
	float m_fBonusTimeInDays;
};

//-----------------------------------------------------------------------------
// Purpose: A pseudo-holiday that is active when either of its child holidays
//			is active. Works through pointers but does not manage memory.
//-----------------------------------------------------------------------------
class COrHoliday : public IIsHolidayActive
{
public:
	COrHoliday( const char *pszName, IIsHolidayActive *pA, IIsHolidayActive *pB )
		: IIsHolidayActive( pszName )
		, m_pA( pA )
		, m_pB( pB )
	{
		Assert( pA );
		Assert( pB );
		Assert( pA != pB );
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		return m_pA->IsActive( timeCurrent )
			|| m_pB->IsActive( timeCurrent );
	}

private:
	IIsHolidayActive *m_pA;
	IIsHolidayActive *m_pB;
};

//-----------------------------------------------------------------------------
// Purpose: Holiday that is defined by a start and end date
//-----------------------------------------------------------------------------
class CDateBasedHoliday : public IIsHolidayActive
{
public:
	CDateBasedHoliday( const char *pszName, const char *pszStartTime, const char *pszEndTime )
		: IIsHolidayActive( pszName )
	{
		m_rtStartTime =	 CRTime::RTime32FromString( pszStartTime );
		m_rtEndTime = CRTime::RTime32FromString( pszEndTime );
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		return ( ( timeCurrent >= m_rtStartTime ) && ( timeCurrent <= m_rtEndTime ) );
	}

	RTime32 GetEndRTime() const
	{
		return m_rtEndTime.GetRTime32();
	}


private:
	CRTime m_rtStartTime;
	CRTime m_rtEndTime;
};

//-----------------------------------------------------------------------------
// Purpose: Holiday that is defined by a start and end date with no year specified
//-----------------------------------------------------------------------------
class CDateBasedHolidayNoSpecificYear : public IIsHolidayActive
{
public:
	CDateBasedHolidayNoSpecificYear( const char *pszName, const char *pszStartTime, const char *pszEndTime )
		: IIsHolidayActive( pszName )
		, m_pszStartTime( pszStartTime )
		, m_pszEndTime( pszEndTime )
		, m_iCachedYear( -1 )
	{
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		const int iYear = timeCurrent.GetYear();

		if ( iYear != m_iCachedYear )
		{
			char m_szStartTime[k_RTimeRenderBufferSize];
			char m_szEndTime[k_RTimeRenderBufferSize];

			V_sprintf_safe( m_szStartTime, "%d-%s", iYear, m_pszStartTime );
			V_sprintf_safe( m_szEndTime, "%d-%s", iYear, m_pszEndTime );

			m_iCachedYear = iYear;
			m_rtCachedStartTime = CRTime::RTime32FromString( m_szStartTime );
			m_rtCachedEndTime = CRTime::RTime32FromString( m_szEndTime );
		}

		return ( ( timeCurrent >= m_rtCachedStartTime ) && ( timeCurrent <= m_rtCachedEndTime ) );
	}

	RTime32 GetEndRTime() const
	{
		return m_rtCachedEndTime.GetRTime32();
	}

private:
	const char *m_pszStartTime;
	const char *m_pszEndTime;

	int m_iCachedYear;
	CRTime m_rtCachedStartTime;
	CRTime m_rtCachedEndTime;
};

//-----------------------------------------------------------------------------
// Purpose: Actual holiday implementation objects.
//-----------------------------------------------------------------------------

static CNoHoliday			g_Holiday_NoHoliday;

static CDateBasedHolidayNoSpecificYear	g_Holiday_TF2Birthday	( "birthday",	"08-23", "08-25" );

static CDateBasedHolidayNoSpecificYear	g_Holiday_Halloween		( "halloween",	"10-01", "11-08" );

static CDateBasedHolidayNoSpecificYear	g_Holiday_ChristmasPart1( "christmas1", "12-01", "12-31 23:59:59" );
static CDateBasedHolidayNoSpecificYear	g_Holiday_ChristmasPart2( "christmas2", "01-01", "01-08" );
static COrHoliday	g_Holiday_Christmas		( "christmas", &g_Holiday_ChristmasPart1, &g_Holiday_ChristmasPart2 );

static CDateBasedHolidayNoSpecificYear	g_Holiday_ValentinesDay	( "valentines",	"02-13", "02-15" );

static CDateBasedHoliday	g_Holiday_MeetThePyro				( "meet_the_pyro",	"2012-06-26", "2012-07-05" );
														   /*					starting date		cycle length in days	bonus time in days on both sides */
static CCyclicalHoliday		g_Holiday_FullMoon					( "fullmoon",		10, 06, 2025,		29.53f,					1.0f );
																								 // note: the cycle length is 29.5 instead of 29.53 so that the time calculations always start at noon based on the way CCyclicalHoliday works
static COrHoliday			g_Holiday_HalloweenOrFullMoon		( "halloween_or_fullmoon",	&g_Holiday_Halloween,	&g_Holiday_FullMoon );

static COrHoliday			g_Holiday_HalloweenOrFullMoonOrValentines	( "halloween_or_fullmoon_or_valentines",	&g_Holiday_HalloweenOrFullMoon,	&g_Holiday_ValentinesDay );

static CDateBasedHolidayNoSpecificYear	g_Holiday_AprilFools	( "april_fools",	"03-31", "04-02" );

static CDateBasedHoliday	g_Holiday_EndOfTheLine				( "eotl_launch",	"2014-12-03", "2015-01-05" );

static CDateBasedHoliday	g_Holiday_CommunityUpdate			( "community_update", "2015-09-01", "2015-11-05" );

static CDateBasedHolidayNoSpecificYear	g_Holiday_Soldier		( "soldier", "04-12", "04-14" );

// only setup for 2025 right now...need to figure out how we want future events to run and maybe remove the year
static CDateBasedHoliday	g_Holiday_Summer( "summer", "2025-07-16", "2025-09-16" );

// ORDER NEEDS TO MATCH enum EHoliday
static IIsHolidayActive *s_HolidayChecks[] =
{
	&g_Holiday_NoHoliday,							// kHoliday_None
	&g_Holiday_TF2Birthday,							// kHoliday_TFBirthday
	&g_Holiday_Halloween,							// kHoliday_Halloween
	&g_Holiday_Christmas,							// kHoliday_Christmas
	&g_Holiday_CommunityUpdate,						// kHoliday_CommunityUpdate
	&g_Holiday_EndOfTheLine,						// kHoliday_EOTL
	&g_Holiday_ValentinesDay,						// kHoliday_Valentines
	&g_Holiday_MeetThePyro,							// kHoliday_MeetThePyro
	&g_Holiday_FullMoon,							// kHoliday_FullMoon
	&g_Holiday_HalloweenOrFullMoon,					// kHoliday_HalloweenOrFullMoon
	&g_Holiday_HalloweenOrFullMoonOrValentines,		// kHoliday_HalloweenOrFullMoonOrValentines
	&g_Holiday_AprilFools,							// kHoliday_AprilFools
	&g_Holiday_Soldier,								// kHoliday_Soldier
	&g_Holiday_Summer,								// kHoliday_Summer
};

COMPILE_TIME_ASSERT( ARRAYSIZE( s_HolidayChecks ) == kHolidayCount );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool EconHolidays_IsHolidayActive( int iHolidayIndex, const CRTime& timeCurrent )
{
	if ( iHolidayIndex < 0 || iHolidayIndex >= kHolidayCount )
		return false;

	Assert( s_HolidayChecks[iHolidayIndex] );
	if ( !s_HolidayChecks[iHolidayIndex] )
		return false;

	return s_HolidayChecks[iHolidayIndex]->IsActive( timeCurrent );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int	EconHolidays_GetHolidayForString( const char* pszHolidayName )
{
	for ( int iHoliday = 0; iHoliday < kHolidayCount; ++iHoliday )
	{
		Assert( s_HolidayChecks[iHoliday] );
		if ( s_HolidayChecks[iHoliday] &&
			 0 == Q_stricmp( pszHolidayName, s_HolidayChecks[iHoliday]->GetHolidayName() ) )
		{
			return iHoliday;
		}
	}

	return kHoliday_None;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *EconHolidays_GetActiveHolidayString()
{
	CRTime timeNow;
	timeNow.SetToCurrentTime();
	timeNow.SetToGMT( true );

	for ( int iHoliday = 0; iHoliday < kHolidayCount; iHoliday++ )
	{
		if ( EconHolidays_IsHolidayActive( iHoliday, timeNow ) )
		{
			Assert( s_HolidayChecks[iHoliday] );
			return s_HolidayChecks[iHoliday]->GetHolidayName();
		}
	}

	// No holidays currently active.
	return NULL;
}

#if defined(TF_CLIENT_DLL) || defined(TF_DLL) || defined(TF_GC_DLL)
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
RTime32 EconHolidays_TerribleHack_GetHalloweenEndData()
{
	return g_Holiday_Halloween.GetEndRTime();
}
#endif // defined(TF_CLIENT_DLL) || defined(TF_DLL) || defined(TF_GC_DLL)
