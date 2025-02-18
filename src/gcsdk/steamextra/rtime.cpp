//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Encapsulates real world (wall clock) time
//
//=============================================================================

#include "stdafx.h"
#ifdef POSIX
#include <sys/time.h>
#else
#include "winlite.h"
#endif
#include "rtime.h"
#include <time.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( WIN32 ) || defined( _PS3 )
// This strptime implementation is taken from the Goolge Site Map Generator project:

// Copyright 2009 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Implement strptime under windows
static const char* kWeekFull[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday",
	"Thursday", "Friday", "Saturday"
};

static const char* kWeekAbbr[] = {
	"Sun", "Mon", "Tue", "Wed",
	"Thu", "Fri", "Sat"
};

static const char* kMonthFull[] = {
	"January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December"
};

static const char* kMonthAbbr[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char* _parse_num(const char* s, int low, int high, int* value) {
	const char* p = s;
	for (*value = 0; *p != NULL && V_isdigit(*p); ++p) {
		*value = (*value) * 10 + static_cast<int>(*p) - static_cast<int>('0');
	}

	if (p == s || *value < low || *value > high) return NULL;
	return p;
}

static char* _strptime(const char *s, const char *format, struct tm *tm) {
	while (*format != NULL && *s != NULL) {
		if (*format != '%') {
			if (*s != *format) return NULL;

			++format;
			++s;
			continue;
		}

		++format;
		int len = 0;
		switch (*format) {
			// weekday name.
	  case 'a':
	  case 'A':
		  tm->tm_wday = -1;
		  for (int i = 0; i < 7; ++i) {
			  len = static_cast<int>(strlen(kWeekAbbr[i]));
			  if (V_strnicmp(kWeekAbbr[i], s, len) == 0) {
				  tm->tm_wday = i;
				  break;
			  }

			  len = static_cast<int>(strlen(kWeekFull[i]));
			  if ( V_strnicmp(kWeekFull[i], s, len) == 0) {
				  tm->tm_wday = i;
				  break;
			  }
		  }
		  if (tm->tm_wday == -1) return NULL;
		  s += len;
		  break;

		  // month name.
	  case 'b':
	  case 'B':
	  case 'h':
		  tm->tm_mon = -1;
		  for (int i = 0; i < 12; ++i) {
			  len = static_cast<int>(strlen(kMonthAbbr[i]));
			  if ( V_strnicmp(kMonthAbbr[i], s, len) == 0) {
				  tm->tm_mon = i;
				  break;
			  }

			  len = static_cast<int>(strlen(kMonthFull[i]));
			  if ( V_strnicmp(kMonthFull[i], s, len) == 0) {
				  tm->tm_mon = i;
				  break;
			  }
		  }
		  if (tm->tm_mon == -1) return NULL;
		  s += len;
		  break;

		  // month [1, 12].
	  case 'm':
		  s = _parse_num(s, 1, 12, &tm->tm_mon);
		  if (s == NULL) return NULL;
		  --tm->tm_mon;
		  break;

		  // day [1, 31].
	  case 'd':
	  case 'e':
		  s = _parse_num(s, 1, 31, &tm->tm_mday);
		  if (s == NULL) return NULL;
		  break;

		  // hour [0, 23].
	  case 'H':
		  s = _parse_num(s, 0, 23, &tm->tm_hour);
		  if (s == NULL) return NULL;
		  break;

		  // minute [0, 59]
	  case 'M':
		  s = _parse_num(s, 0, 59, &tm->tm_min);
		  if (s == NULL) return NULL;
		  break;

		  // seconds [0, 60]. 60 is for leap year.
	  case 'S':
		  s = _parse_num(s, 0, 60, &tm->tm_sec);
		  if (s == NULL) return NULL;
		  break;

		  // year [1900, 9999].
	  case 'Y':
		  s = _parse_num(s, 1900, 9999, &tm->tm_year);
		  if (s == NULL) return NULL;
		  tm->tm_year -= 1900;
		  break;

		  // year [0, 99].
	  case 'y':
		  s = _parse_num(s, 0, 99, &tm->tm_year);
		  if (s == NULL) return NULL;
		  if (tm->tm_year <= 68) {
			  tm->tm_year += 100;
		  }
		  break;
		  // arbitray whitespace.
	  case 't':
	  case 'n':
		  while (V_isspace(*s)) ++s;
		  break;

		  // '%'.
	  case '%':
		  if (*s != '%') return NULL;
		  ++s;
		  break;

		  // All the other format are not supported.
	  default:
		  AssertMsg( false, "Invalid format string to strptime!" );
		  return NULL;
		}
		++format;
	}

	if (*format != NULL) {
		return NULL;
	} else {
		return const_cast<char*>(s);
	}
}

char* strptime(const char *buf, const char *fmt, struct tm *tm) {
	return _strptime(buf, fmt, tm);
}
#endif  // WIN32


// Our cached copy of the current time
RTime32  CRTime::sm_nTimeLastSystemTimeUpdate = 0;	// initialize to large negative value to trigger immediate FileTimeCur update
char CRTime::sm_rgchLocalTimeCur[16]="";
char CRTime::sm_rgchLocalDateCur[16]="";
RTime32 CRTime::sm_nTimeCur = 0;




//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CRTime::CRTime()
{
	if ( sm_nTimeCur == 0 )
	{
		sm_nTimeCur = time(NULL);
	}

	m_nStartTime = sm_nTimeCur;
	m_bGMT = false;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the amount of time that's passed between our time and the
//			current time.
// Output:	Time that's passed between our time and the current time
//-----------------------------------------------------------------------------
int CRTime::CSecsPassed() const
{
	return( sm_nTimeCur - m_nStartTime );
}


//-----------------------------------------------------------------------------
// Purpose: Updates our current time value.  We only
//			update the time once per frame-- the rest of the time, we just
//			access a cached copy of the time.  
//			NOTE: This should only be called once per frame.
//-----------------------------------------------------------------------------
void CRTime::UpdateRealTime()
{
	// BUGBUG Alfred: update this less often than once per frame?
	RTime32 nTimePrev = sm_nTimeCur;
	sm_nTimeCur = time(NULL);

	if ( sm_nTimeCur < nTimePrev )
	{
		// time can go backwards sometimes if clock sync adjusts system time; warn when this happens
		EmitInfo( SPEW_SYSTEM_MISC, SPEW_ALWAYS, LOG_ALWAYS, "Warning: system time went backward by %d seconds\n", ( nTimePrev - sm_nTimeCur ) );
	}

	// update our time from file time once per second
	if ( sm_nTimeCur - sm_nTimeLastSystemTimeUpdate >= 1 )
	{
#ifdef _WIN32
		// get the local time, generate time & date strings and cache the strings, as we will need these
		// frequently for logs.
		SYSTEMTIME systemTimeLocal;
		GetLocalTime( &systemTimeLocal );
		GetTimeFormat( LOCALE_USER_DEFAULT, 0, &systemTimeLocal, "HH:mm:ss", sm_rgchLocalTimeCur, Q_ARRAYSIZE( sm_rgchLocalTimeCur ) );
		GetDateFormat( LOCALE_USER_DEFAULT, 0, &systemTimeLocal, "MM/dd/yy", sm_rgchLocalDateCur, Q_ARRAYSIZE( sm_rgchLocalDateCur ) );
#elif defined(POSIX)
		time_t now; 
		time( &now );
		struct tm tmStruct;
		struct tm *localTime = Plat_gmtime( &now, &tmStruct );
		strftime( sm_rgchLocalTimeCur, Q_ARRAYSIZE( sm_rgchLocalTimeCur ), "%H:%M:%S", localTime );
		strftime( sm_rgchLocalDateCur, Q_ARRAYSIZE( sm_rgchLocalDateCur ), "%m/%d/%y", localTime );
#else
#error "Implement me"
#endif
		sm_nTimeLastSystemTimeUpdate = sm_nTimeCur;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the system clock on this box to specified world time
// Input:	rTime32Current - world time to set
//-----------------------------------------------------------------------------
void CRTime::SetSystemClock( RTime32 rTime32Current )
{
#ifdef _WIN32
	FILETIME fileTime;
	SYSTEMTIME systemTime = {0};
	// convert from seconds since 1/1/1970 to filetime (100 nanoseconds since 1/1/1601) with this magic formula courtesy of MSDN
    uint64 ulTmp = ( ( (uint64) rTime32Current ) * 10 * k_nMillion ) + 116444736000000000;
	fileTime.dwLowDateTime = (DWORD) ulTmp;
    fileTime.dwHighDateTime = ulTmp >> 32;

	// convert from filetime to system time (note this also does time zone conversion to UTC)
	BOOL bRet = FileTimeToSystemTime( &fileTime, &systemTime );
	Assert( bRet ); // should never fail
	if ( !bRet )	// but if it does, don't set system clock to garbage
		return;

	// set system time in UTC
	bRet = SetSystemTime( &systemTime );
	Assert( bRet );

	// update our cached time
	sm_nTimeCur = rTime32Current;
#else
	Assert( !"Not implemented" );
#endif // _WIN32
}


//-----------------------------------------------------------------------------
// Purpose: Renders the time
// Output :	ptr to time string
//-----------------------------------------------------------------------------
const char* CRTime::Render( char (&buf)[k_RTimeRenderBufferSize] ) const
{
	return Render( m_nStartTime, buf );
}

//-----------------------------------------------------------------------------
// Purpose: Renders the time - static function
// Input  : rTime32 - time to render
// Output :	ptr to time string
//-----------------------------------------------------------------------------
const char* CRTime::Render( const RTime32 rTime32, char (&buf)[k_RTimeRenderBufferSize] )
{
	// The return value string contains exactly 26 characters and has the form:	Wed Jan 02 02:03:55 1980\n\0
	time_t tTime = rTime32;
	char pchTime[32];
	if ( !Plat_ctime( &tTime, pchTime, Q_ARRAYSIZE( pchTime ) ) )
		return 0;

	// Remove '\n'
	Assert( Q_strlen( pchTime ) == 25 );
	pchTime[ 24 ] = '\0';

	if ( rTime32 == k_RTime32Infinite )
		Q_strncpy( buf, "Infinite time value", k_RTimeRenderBufferSize );
	else if ( rTime32 == k_RTime32Nil )
		Q_strncpy( buf, "Nil time value", k_RTimeRenderBufferSize );
	else if ( rTime32 < k_RTime32MinValid )
		Q_strncpy( buf, "Invalid time value", k_RTimeRenderBufferSize );
	else
		Q_strncpy( buf, pchTime, k_RTimeRenderBufferSize );

	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: Get the calendar year (absolute) for the current time
//-----------------------------------------------------------------------------
int CRTime::GetYear() const
{
	time_t timeCur = m_nStartTime;
	struct tm tmStruct;
	struct tm *ptmCur = m_bGMT ? Plat_gmtime( &timeCur, &tmStruct ) : Plat_localtime( &timeCur, &tmStruct );
	return ptmCur->tm_year + 1900;
}


//-----------------------------------------------------------------------------
// Purpose: Get the calendar month (0-11) for the current time
//-----------------------------------------------------------------------------
int CRTime::GetMonth() const
{
	time_t timeCur = m_nStartTime;
	struct tm tmStruct;
	struct tm *ptmCur = m_bGMT ? Plat_gmtime( &timeCur, &tmStruct ) : Plat_localtime( &timeCur, &tmStruct );
	return ptmCur->tm_mon;
}


//-----------------------------------------------------------------------------
// Purpose: Get the day of the calendar year (0-365) for the current time
//-----------------------------------------------------------------------------
int CRTime::GetDayOfYear() const
{
	time_t timeCur = m_nStartTime;
	struct tm tmStruct;
	struct tm *ptmCur = m_bGMT ? Plat_gmtime( &timeCur, &tmStruct ) : Plat_localtime( &timeCur, &tmStruct );
	return ptmCur->tm_yday;
}


//-----------------------------------------------------------------------------
// Purpose: Get the day of the month (1-31) for the current time
//-----------------------------------------------------------------------------
int CRTime::GetDayOfMonth() const
{
	time_t timeCur = m_nStartTime;
	struct tm tmStruct;
	struct tm *ptmCur = m_bGMT ? Plat_gmtime( &timeCur, &tmStruct ) : Plat_localtime( &timeCur, &tmStruct );
	return ptmCur->tm_mday;
}


//-----------------------------------------------------------------------------
// Purpose: Get the day of the week (0-6, 0=Sunday) for the current time
//-----------------------------------------------------------------------------
int CRTime::GetDayOfWeek() const
{
	time_t timeCur = m_nStartTime;
	struct tm tmStruct;
	struct tm *ptmCur = m_bGMT ? Plat_gmtime( &timeCur, &tmStruct ) : Plat_localtime( &timeCur, &tmStruct );
	return ptmCur->tm_wday;
}


//-----------------------------------------------------------------------------
// Purpose: Get the current hour (0-23)
//-----------------------------------------------------------------------------
int CRTime::GetHour( ) const
{
	time_t timeCur = m_nStartTime;
	struct tm tmStruct;
	struct tm *ptmCur = m_bGMT ? Plat_gmtime( &timeCur, &tmStruct ) : Plat_localtime( &timeCur, &tmStruct );
	return ptmCur->tm_hour;
}


//-----------------------------------------------------------------------------
// Purpose: Get the current minute value (0-59)
//-----------------------------------------------------------------------------
int CRTime::GetMinute( ) const
{
	time_t timeCur = m_nStartTime;
	struct tm tmStruct;
	struct tm *ptmCur = m_bGMT ? Plat_gmtime( &timeCur, &tmStruct ) : Plat_localtime( &timeCur, &tmStruct );
	return ptmCur->tm_min;
}


//-----------------------------------------------------------------------------
// Purpose: Get the current second value (0-59)
//-----------------------------------------------------------------------------
int CRTime::GetSecond() const
{
	time_t timeCur = m_nStartTime;
	struct tm tmStruct;
	struct tm *ptmCur = m_bGMT ? Plat_gmtime( &timeCur, &tmStruct ) : Plat_localtime( &timeCur, &tmStruct );
	return ptmCur->tm_sec;
}


//-----------------------------------------------------------------------------
// Purpose: Get the ISO week number
//-----------------------------------------------------------------------------
int CRTime::GetISOWeekOfYear() const
{
	int nDay = GetDayOfYear() - ( 1 + GetDayOfWeek() );
	int nISOWeek = nDay / 7;
	return nISOWeek;
}


//-----------------------------------------------------------------------------
// Purpose: let me know if this is a leap year or not
//-----------------------------------------------------------------------------
/* static */ bool CRTime::BIsLeapYear( int nYear )
{
	// every for years, unless it is a century; or if it is every 4th century
	if ( ( nYear % 4 == 0 && nYear % 100 != 0) || nYear % 400 == 0)
		return true; /* leap */
	else
		return false; /* no leap */
}


//-----------------------------------------------------------------------------
// Purpose: Calculate and return a time value corresponding to given sting
//			Using a format string to convert
// Input:	pchFmt -	Format string that describes how to parse the value
//						YY or YYYY is year, MM month, DD day of the month,
//						hh mm ss is hour minute second.
//						Z0000 is a time-zone offset, eg -0700.
//						Everything except YY is optional (will be considered 0 if not given)
//			pchValue -	String containing the value to covert
// Output:	RTime32
//-----------------------------------------------------------------------------
// STATIC
RTime32 CRTime::RTime32FromFmtString( const char *pchFmt, const char* pchValue )
{
	struct tm tm;

	char rgchNum[8];
	char rgchValue[64];

	Q_memset( &tm, 0x0, sizeof( tm ) );
	Q_strncpy( rgchValue, pchValue, sizeof( rgchValue) );

	int cchFmt = Q_strlen( pchFmt );		
	int cchValue = Q_strlen( rgchValue );
	if ( cchFmt != cchValue || cchFmt < 4 )
	{
		Assert( false );
		return k_RTime32Nil;
	}

	const char *pchYYYY = Q_strstr( pchFmt, "YYYY" );
	const char *pchYY = Q_strstr( pchFmt, "YY" );
	const char *pchMM = Q_strstr( pchFmt, "MM" );
	const char *pchMnt = Q_strstr( pchFmt, "Mnt" );
	const char *pchDD = Q_strstr( pchFmt, "DD" );
	const char *pchThh = Q_strstr( pchFmt, "hh" );
	const char *pchTmm = Q_strstr( pchFmt, "mm" );
	const char *pchTss = Q_strstr( pchFmt, "ss" );
	const char *pchTzone = Q_strstr( pchFmt, "Z0000" );

	if ( pchYYYY )
	{
		pchYYYY = rgchValue + ( pchYYYY - pchFmt );
		Q_strncpy( rgchNum, pchYYYY, 5 );
		tm.tm_year = strtol( rgchNum, 0, 10 ) - 1900;
	}
	else if ( pchYY )
	{
		pchYY = rgchValue + ( pchYY - pchFmt );
		Q_strncpy( rgchNum, pchYY, 3 );
		tm.tm_year = strtol( rgchNum, 0, 10 ) + 100;
	}
	else
		return k_RTime32Nil;	// must have a year

	if ( pchMM )
	{
		pchMM = rgchValue + ( pchMM - pchFmt );
		Q_strncpy( rgchNum, pchMM, 3 );
		tm.tm_mon = strtol( rgchNum, 0, 10 ) - 1;
	}
	if ( pchMnt )
	{
		static const char *rgszMonthNames[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
		pchMnt = rgchValue + ( pchMnt - pchFmt );
		int i;
		for ( i = 0; i < 12; i++ )
		{
			if ( !V_strnicmp( rgszMonthNames[i], pchMnt, 3 ) )
				break;
		}
		if ( i < 12 )
			tm.tm_mon = i;
	}
	if ( pchDD )
	{
		pchDD = rgchValue + (pchDD - pchFmt );
		Q_strncpy( rgchNum, pchDD, 3 );
		tm.tm_mday = strtol( rgchNum, 0, 10 );
	}
	if ( pchThh )
	{
		pchThh = rgchValue + ( pchThh - pchFmt );
		Q_strncpy( rgchNum, pchThh, 3 );
		tm.tm_hour = strtol( rgchNum, 0, 10 );
	}
	if ( pchTmm )
	{
		pchTmm = rgchValue + ( pchTmm - pchFmt );
		Q_strncpy( rgchNum, pchTmm, 3 );
		tm.tm_min = strtol( rgchNum, 0, 10 );
	}
	if ( pchTss )
	{
		pchTss = rgchValue + (pchTss - pchFmt );
		Q_strncpy( rgchNum, pchTss, 3 );
		tm.tm_sec = strtol( rgchNum, 0, 10 );
	}
	if ( pchTzone )
	{
		long nOffset = 0;
		pchTzone = rgchValue + (pchTzone - pchFmt);
		Q_strncpy( rgchNum, pchTzone, 6 );
		nOffset = strtol( rgchNum, 0, 10 );
		tm.tm_hour -= nOffset / 100; // to go from -0700 to UTC, need to ADD seven

		// is this a sub-hour timezone? eg +0545 Kathmandu
		int nMinutesOffset = nOffset % 100;
		if ( nMinutesOffset )
			tm.tm_min -= nMinutesOffset;

		// OK, so this is somewhat lame: mktime assumes our tm units are in LOCAL time.
		// However, we have just created a UTC time by using the supplied timezone offset.
		// The rational thing to do here would be to call mkgmtime() instead of mktime(),
		// but that function isn't available in unix-land.
		// SO, instead we will MANUALLY convert this tm back to local time

#if ( defined( _MSC_VER ) && _MSC_VER >= 1900 )
		#define timezone _timezone
		#define daylight _daylight
#endif

		// subtract timezone, which is in SECONDS. timezone is (UTC - local), so local = UTC - timezone
		tm.tm_sec -= timezone;
		// timezone does NOT account for DST, so if we are in DST, we need to ADD an hour. 
		// This is because the value of timezone we subtracted was one hour TOO LARGE
		tm.tm_hour += daylight ? 1 : 0;

	}

	// We don't know if DST is in effect, let the CRT 
	// figure it out
	tm.tm_isdst = -1;

	return mktime( &tm );
}


//-----------------------------------------------------------------------------
// Purpose: Calculate and return a time value corresponding to given sting which is
// expected to be in one of the common HTTP date formats.
//-----------------------------------------------------------------------------
// STATIC
RTime32 CRTime::RTime32FromHTTPDateString( const char* pchValue )
{
	// First format here is RFC 822/1123 format
	struct tm tm;
	if ( strptime( pchValue, "%a, %e %b %Y %H:%M:%S", &tm ) )
	{
		return Plat_timegm( &tm );
	}

	// If that failed, try RFC 850/1036 format
	if ( strptime( pchValue, "%a, %e-%b-%y %H:%M:%S", &tm ) )
	{
		return Plat_timegm( &tm );
	}

	// If that also failed, give up
	return k_RTime32Nil;
}


//-----------------------------------------------------------------------------
// Purpose: Parse time from string RFC3339 format (assumes UTC)
//-----------------------------------------------------------------------------
// STATIC
RTime32 CRTime::RTime32FromRFC3339UTCString( const char* pchValue )
{

	// UTC only from RFC 3339. Should be 2005-05-15T17:11:51Z
	struct tm tm;
	if ( strptime( pchValue, "%Y-%m-%dT%H:%M:%SZ", &tm ) )
	{
		return Plat_timegm( &tm );
	}	

	// If that also failed, give up
	return k_RTime32Nil;

}


//-----------------------------------------------------------------------------
// Purpose: Output time in RFC3339 format (assumes UTC)
//-----------------------------------------------------------------------------
// STATIC
const char* CRTime::RTime32ToRFC3339UTCString( const RTime32 rTime32, char (&buf)[k_RTimeRenderBufferSize] )
{
	// Store the result in a temporary buffer, so that you can use several in a single printf.
	time_t tTime = rTime32; 
	struct tm tmStruct;
	struct tm *ptm = Plat_gmtime( &tTime, &tmStruct );

	if ( rTime32 == k_RTime32Nil || !ptm )
		return "NIL";

	if ( rTime32 == k_RTime32Infinite )
		return "Infinite time value";

	if ( rTime32 < k_RTime32MinValid || !ptm )
		return "Invalid time value";

	Q_snprintf( buf, k_RTimeRenderBufferSize, "%04u-%02u-%02uT%02u:%02u:%02uZ", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec );	
	return buf;
}


//-----------------------------------------------------------------------------
// Purpose: Calculate and return a time value corresponding to given sting
// "YYYY-MM-DD hh:mm:ss" (copied from sqlhelpers.cpp)
//-----------------------------------------------------------------------------
// STATIC
RTime32 CRTime::RTime32FromString( const char* pszValue )
{
	struct tm tm;
	
	char num[5];
	char szValue[64];

	Q_memset( &tm, 0x0, sizeof( tm ) );
	Q_strncpy( szValue, pszValue, sizeof( szValue) );
		
	const char *str= szValue;

	num[0] =*str++; num[1] =*str++; num[2] =*str++; num[3] =*str++; num[4] = 0;
	tm.tm_year = strtol( num, 0, 10 ) - 1900;
	if (*str == '-') str++;
	num[0] = *str++; num[1] = *str++; num[2] = 0;
	tm.tm_mon = strtol( num, 0, 10 ) - 1;
	if (*str == '-') str++;
	num[0] = *str++; num[1] = *str++; num[2] = 0;
	tm.tm_mday = strtol( num, 0, 10 );

	if ( *str != 0 )
	{
		// skip an optional space or T between date and time
		if ( *str == ' ' || *str == 'T' )
			str++;

		// time is given too
		num[0] = *str++; num[1] = *str++; num[2] = 0;
		tm.tm_hour = strtol( num, 0, 10 );
		if (*str == ':') str++;
		num[0] = *str++; num[1] = *str++; num[2] = 0;
		tm.tm_min = strtol( num, 0, 10 );
		if (*str == ':') str++;
		num[0] = *str++; num[1] = *str++; num[2] = 0;
		tm.tm_sec = strtol( num, 0, 10 );
	}
	tm.tm_isdst = -1;

	return mktime( &tm );
}


//-----------------------------------------------------------------------------
// Purpose: Returns a static string "YYYY-MM-DD hh:mm:ss" for given RTime32
// Input:	rTime32 -
//			bNoPunct -	No dashes, colons or spaces will be in the output string
//			bOnlyDate -	Only output the date
// Output:	const char * -- only usable till the next yield
//-----------------------------------------------------------------------------
// STATIC
const char* CRTime::RTime32ToString( const RTime32 rTime32, char (&buf)[k_RTimeRenderBufferSize], bool bNoPunct /*=false*/, bool bOnlyDate /*= false*/ )
{
	// Store the result in a temporary buffer, so that you can use several in a single printf.
	time_t tTime = rTime32; 
	struct tm tmStruct;
	struct tm *ptm = Plat_localtime( &tTime, &tmStruct );

	const char *pchOnlyDateFmt = bNoPunct ? "%04u%02u%02u" : "%04u-%02u-%02u";
	const char *pchDateTimeFmt = bNoPunct ? "%04u%02u%02u%02u%02u%02u" : "%04u-%02u-%02u %02u:%02u:%02u";
	if ( rTime32 == k_RTime32Nil || !ptm )
		return "NIL";

	if ( rTime32 == k_RTime32Infinite )
		return "Infinite time value";

	if ( rTime32 < k_RTime32MinValid || !ptm )
		return "Invalid time value";

	if ( bOnlyDate )
	{
		Q_snprintf( buf, k_RTimeRenderBufferSize, pchOnlyDateFmt, 
			ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday );
	}
	else
	{
		Q_snprintf( buf, k_RTimeRenderBufferSize, pchDateTimeFmt, 
			ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
			ptm->tm_hour, ptm->tm_min, ptm->tm_sec );
	}

	return buf;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a static string like "Aug 21" for given RTime32
// Input:	rTime32 -
//		
// Output:	const char * -- only usable till the next yield
//-----------------------------------------------------------------------------
// STATIC
const char* CRTime::RTime32ToDayString( const RTime32 rTime32, char (&buf)[k_RTimeRenderBufferSize], bool bGMT )
{
	// Store the result in a temporary buffer, so that you can use several in a single printf.
	time_t tTime = rTime32; 
	struct tm tmStruct;
	struct tm *ptm = bGMT ? Plat_gmtime( &tTime, &tmStruct ) : Plat_localtime( &tTime, &tmStruct );

	DbgVerify( strftime( buf, k_RTimeRenderBufferSize, "%b %d", ptm ) );
	return buf;
}



//-----------------------------------------------------------------------------
// Purpose: Calculate and return a time value corresponding to the beginning of
//			the day represented by rtime32
//-----------------------------------------------------------------------------
// STATIC
RTime32 CRTime::RTime32BeginningOfDay( const RTime32 rtime32 )
{
	time_t timeCur = rtime32;
	struct tm tmStruct;
	struct tm *ptmCur = Plat_localtime( &timeCur, &tmStruct );
	if ( !ptmCur )
		return k_RTime32Nil;

	// midnight
	ptmCur->tm_hour = 0;
	ptmCur->tm_min = 0;
	ptmCur->tm_sec = 0;

	// Let it compute DST
	ptmCur->tm_isdst = -1;

	return mktime( ptmCur );
}


//-----------------------------------------------------------------------------
// Purpose: Calculate and return a time value corresponding to the beginning of 
//			the next day after rtime32
//-----------------------------------------------------------------------------
// STATIC
RTime32 CRTime::RTime32BeginningOfNextDay( const RTime32 rtime32 )
{
	time_t timeCur = rtime32;
	struct tm tmStruct;
	struct tm *ptmCur = Plat_localtime( &timeCur, &tmStruct );
	if ( !ptmCur )
		return k_RTime32Nil;

	// It will move to the next month etc if need be
	ptmCur->tm_mday++;

	// midnight
	ptmCur->tm_hour = 0;
	ptmCur->tm_min = 0;
	ptmCur->tm_sec = 0;

	// Let it compute DST
	ptmCur->tm_isdst = -1;

	return mktime( ptmCur );
}


//-----------------------------------------------------------------------------
// Purpose: Calculate and return a time value corresponding to the first day of
//			the month indicated by rtime32
//-----------------------------------------------------------------------------
// STATIC
RTime32 CRTime::RTime32FirstDayOfMonth( const RTime32 rtime32 )
{
	time_t timeCur = rtime32;
	struct tm tmStruct;
	struct tm *ptmCur = Plat_localtime( &timeCur, &tmStruct );
	if ( !ptmCur )
		return k_RTime32Nil;

	// first day of month
	ptmCur->tm_mday = 1;

	// midnight
	ptmCur->tm_hour = 0;
	ptmCur->tm_min = 0;
	ptmCur->tm_sec = 0;

	// Let it compute DST
	ptmCur->tm_isdst = -1;

	return mktime( ptmCur );
}


//-----------------------------------------------------------------------------
// Purpose: Calculate and return a time value corresponding to the last day of
//			the month indicated by rtime32
//-----------------------------------------------------------------------------
// STATIC
RTime32 CRTime::RTime32LastDayOfMonth( const RTime32 rtime32 )
{
	time_t timeCur = rtime32;
	struct tm tmStruct;
	struct tm *ptmCur = Plat_localtime( &timeCur, &tmStruct );
	if ( !ptmCur )
		return k_RTime32Nil;

	// Zeroth day of month N becomes last day of month (N-1)
	ptmCur->tm_mon++;
	ptmCur->tm_mday = 0;

	// midnight
	ptmCur->tm_hour = 0;
	ptmCur->tm_min = 0;
	ptmCur->tm_sec = 0;

	// Let it compute DST
	ptmCur->tm_isdst = -1;

	return mktime( ptmCur );
}


//-----------------------------------------------------------------------------
// Purpose: Calculate and return a time value corresponding to the first day of
//			the month after the one indicated by rtime32
//-----------------------------------------------------------------------------
// STATIC
RTime32 CRTime::RTime32FirstDayOfNextMonth( const RTime32 rtime32 )
{
	time_t timeCur = rtime32;
	struct tm tmStruct;
	struct tm *ptmCur = Plat_localtime( &timeCur, &tmStruct );
	if ( !ptmCur )
		return k_RTime32Nil;

	ptmCur->tm_mon++;
	ptmCur->tm_mday = 1;

	// midnight
	ptmCur->tm_hour = 0;
	ptmCur->tm_min = 0;
	ptmCur->tm_sec = 0;

	// Let it compute DST
	ptmCur->tm_isdst = -1;

	return mktime( ptmCur );
}


//-----------------------------------------------------------------------------
// Purpose: Calculate and return a time value corresponding to the last day of
//			the month after the one indicated by rtime32
//-----------------------------------------------------------------------------
// STATIC
RTime32 CRTime::RTime32LastDayOfNextMonth( const RTime32 rtime32 )
{
	time_t timeCur = rtime32;
	struct tm tmStruct;
	struct tm *ptmCur = Plat_localtime( &timeCur, &tmStruct );
	if ( !ptmCur )
		return k_RTime32Nil;

	// use zeroth-day trick - skip 2 months then back a day
	ptmCur->tm_mon += 2;
	ptmCur->tm_mday = 0;

	// midnight
	ptmCur->tm_hour = 0;
	ptmCur->tm_min = 0;
	ptmCur->tm_sec = 0;

	// Let it compute DST
	ptmCur->tm_isdst = -1;

	return mktime( ptmCur );
}


//-----------------------------------------------------------------------------
// Purpose: Calculate and return a time value corresponding to the Nth day of
//			the month. If that month only has K days, K < N, it will return
//			the Kth day. The input should be reasonable (don't ask for the -5th
//			day of the month).
//
// Input:	rtime32 -		Time representing some time in the month interested in
//			nDay -			The day of that month you want the return to be set to
//
// Return:	Time value equal to midnight on that day.
//-----------------------------------------------------------------------------
// STATIC
RTime32 CRTime::RTime32NthDayOfMonth( const RTime32 rtime32, int nDay )
{
	Assert( nDay > 0 );
	Assert( nDay < 32 );

	time_t timeCur = rtime32;
	struct tm tmStruct;
	struct tm *ptmCur = Plat_localtime( &timeCur, &tmStruct );
	if ( !ptmCur )
		return k_RTime32Nil;

	int nCurMonth = ptmCur->tm_mon;

	ptmCur->tm_mday = nDay;

	// midnight
	ptmCur->tm_hour = 0;
	ptmCur->tm_min = 0;
	ptmCur->tm_sec = 0;

	// Let it compute DST
	ptmCur->tm_isdst = -1;

	// This call will modify ptmCur in-place
	time_t timeThen = mktime( ptmCur );

	// See if the month changed
	if ( ptmCur->tm_mon != nCurMonth )
	{
		// use zeroth-day trick to just get the last day of this month
		ptmCur->tm_mday = 0;
		// Let it compute DST
		ptmCur->tm_isdst = -1;
		timeThen = mktime( ptmCur );
	}

	return timeThen;
}


//-----------------------------------------------------------------------------
// Purpose: Add X months to the current date, and return the Nth day of that
//			month.
//
// Input:	nNthDayOfMonth -		Day of the target month to return a date for
//			rtime32StartDate -		Time value to add X months to
//			nMonthsToAdd -			X
//
// Return:	Time value equal to midnight on that day.
//-----------------------------------------------------------------------------
// STATIC
RTime32 CRTime::RTime32MonthAddChooseDay( int nNthDayOfMonth, RTime32 rtime32StartDate, int nMonthsToAdd )
{
	// Get the first day of start month
	RTime32 rtime32FirstDayOfStartMonth = CRTime( rtime32StartDate ).GetFirstDayOfMonth();

	// Add X months to that - guaranteed to be correct month
	RTime32 rtime32FirstDayOfTargetMonth = CRTime::RTime32DateAdd( rtime32FirstDayOfStartMonth, nMonthsToAdd, k_ETimeUnitMonth );

	// Then get the Nth day of that month
	RTime32 rtime32Target = CRTime::RTime32NthDayOfMonth( rtime32FirstDayOfTargetMonth, nNthDayOfMonth );

	return rtime32Target;
}


//-----------------------------------------------------------------------------
// Purpose: Add or subtract N units of time from the current value.
//			Units may be days, weeks, seconds, etc
// Input:	rtime32 -				Reference time
//			nAmount -				Number of units to add (neg for subtract)
//			eTimeFlagAmountType -	Indicates what units are on nAmount
//
// Return:	The newly calculated offset time (the input is unmodified)
//-----------------------------------------------------------------------------
// STATIC
RTime32 CRTime::RTime32DateAdd(  const RTime32 rtime32, int nAmount, ETimeUnit eTimeAmountType )
{
	time_t timeCur = rtime32;
	struct tm tmStruct;
	struct tm *ptmCur = Plat_localtime( &timeCur, &tmStruct );
	if ( !ptmCur )
		return k_RTime32Nil;

	// mktime() is smart enough to take day-of-month values that are out of range and adjust
	// everything to make sense. So you can go back 3 weeks by just subtracting 21 from tm_mday.
	switch ( eTimeAmountType )
	{
	default:
		AssertMsg( false, "Bad flag in RTime32DateAdd" );
		break;
	case k_ETimeUnitForever:
		return k_RTime32Infinite;
	case k_ETimeUnitYear:
		ptmCur->tm_year += nAmount;
		break;
	case k_ETimeUnitMonth:
		ptmCur->tm_mon += nAmount;
		break;
	case k_ETimeUnitWeek:
		ptmCur->tm_mday += 7 * nAmount;
		break;
	case k_ETimeUnitDay:
		ptmCur->tm_mday += nAmount;
		break;
	case k_ETimeUnitHour:
		ptmCur->tm_hour += nAmount;
		break;
	case k_ETimeUnitMinute:
		ptmCur->tm_min += nAmount;
		break;
	case k_ETimeUnitSecond:
		ptmCur->tm_sec += nAmount;
		break;
	}

	// Let it compute DST
	ptmCur->tm_isdst = -1;

	return mktime( ptmCur );
}

//-----------------------------------------------------------------------------
// Purpose: Compare two times and evaluate what calendar boundaries have
//			been crossed (eg day, month, hour) between the two times.
//
//			Note: in general, the crossing of a large boundary will be accompanied
//			by the crossing of all smaller boundaries. The exception is Week:
//			the Week boundary is from Saturday to Sunday, and it is possible to
//			go over a Month or Year boundary without beginning a new week.
//
//			So, the return value is the largest time boundary that was crossed. 
//			However, the pbWeekChanged value will be set to indicate if the week
//			changed in cases where the return value is Month or Year.
//
// Input:	unTime1 -		First time value
//			unTime2 -		Second time value
//			pbWeekChanged -	Indicates if the Week changed
//
// Return:	Largest time boundary crossed
//-----------------------------------------------------------------------------
// STATIC
ETimeUnit CRTime::FindTimeBoundaryCrossings( RTime32 unTime1, RTime32 unTime2, bool *pbWeekChanged )
{
	time_t time1 = unTime1;
	time_t time2 = unTime2;

	// have to cache the first one locally, because it's a global object
	struct tm tmStruct1;
	struct tm *ptmTime1 = Plat_localtime( &time1, &tmStruct1 );
	if ( !ptmTime1 )
		return k_ETimeUnitForever;
	struct tm _tmTime1 = *ptmTime1;
	ptmTime1 = &_tmTime1;
	struct tm tmStruct2;
	struct tm *ptmTime2 = Plat_localtime( &time2, &tmStruct2 );
	if ( !ptmTime2 )
		return k_ETimeUnitForever;

	// Need a little extra logic to find week boundaries
	// Find this out first, because it may or may not be true even if a 
	// month / year boundary was crossed.
	*pbWeekChanged = false;

	// If the difference is more than 6 days, we crossed a week boundary
	if ( ( ( unTime1 > unTime2 ) && ( ( unTime1 - unTime2 ) > k_cSecondsPerWeek ) )
		 || ( ( unTime2 > unTime1 ) && ( ( unTime2 - unTime1 ) > k_cSecondsPerWeek ) ) )
	{
		*pbWeekChanged = true;
	}
	else if ( ptmTime1->tm_yday != ptmTime2->tm_yday )
	{
		// Otherwise, we have to look at wday - if the later time
		// has a lower or equal wday value, then we crossed a week boundary
		if ( unTime2 > unTime1 )
		{
			if ( ptmTime2->tm_wday <= ptmTime1->tm_wday )
				*pbWeekChanged = true;
		}
		else
		{
			if ( ptmTime1->tm_wday <= ptmTime2->tm_wday )
				*pbWeekChanged = true;
		}
	}

	// Evaluate larger boundaries first. As soon as we detect
	// that we've crossed a boundary, we consider all smaller boundaries
	// crossed too.

	// Year
	if ( ptmTime1->tm_year != ptmTime2->tm_year )
		return k_ETimeUnitYear;

	// Month
	if ( ptmTime1->tm_mon != ptmTime2->tm_mon )
		return k_ETimeUnitMonth;

	// If the week changed, return that now
	if ( *pbWeekChanged )
		return k_ETimeUnitWeek;

	// Day
	if ( ptmTime1->tm_yday != ptmTime2->tm_yday )
		return  k_ETimeUnitDay;

	// Hour
	if ( ptmTime1->tm_hour != ptmTime2->tm_hour )
		return  k_ETimeUnitHour;

	// If DST changed, make sure that we know an hour boundary was crossed
	// (overlap from the "fall back" case may otherwise trick us)
	if ( ptmTime1->tm_isdst != ptmTime2->tm_isdst )
		return  k_ETimeUnitHour;

	// Minute
	if ( ptmTime1->tm_min != ptmTime2->tm_min )
		return  k_ETimeUnitMinute;

	// Second
	if ( ptmTime1->tm_sec != ptmTime2->tm_sec )
		return  k_ETimeUnitSecond;

	return k_ETimeUnitNone;

}

