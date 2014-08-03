//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PREDICTIONCOPY_H
#define PREDICTIONCOPY_H
#ifdef _WIN32
#pragma once
#endif

#include <memory.h>
#include "datamap.h"
#include "ehandle.h"
#include "tier1/utlstring.h"

#if defined( CLIENT_DLL )
class C_BaseEntity;
typedef CHandle<C_BaseEntity> EHANDLE;

#if defined( _DEBUG )
// #define COPY_CHECK_STRESSTEST
class IGameSystem;
IGameSystem* GetPredictionCopyTester( void );
#endif

#else
class CBaseEntity;
typedef CHandle<CBaseEntity> EHANDLE;
#endif

enum
{
	PC_EVERYTHING = 0,
	PC_NON_NETWORKED_ONLY,
	PC_NETWORKED_ONLY,
};

#define PC_DATA_PACKED			true
#define PC_DATA_NORMAL			false

typedef void ( *FN_FIELD_COMPARE )( const char *classname, const char *fieldname, const char *fieldtype,
	bool networked, bool noterrorchecked, bool differs, bool withintolerance, const char *value );

class CPredictionCopy
{
public:
	typedef enum
	{
		DIFFERS = 0,
		IDENTICAL,
		WITHINTOLERANCE,
	} difftype_t;

	CPredictionCopy( int type, void *dest, bool dest_packed, void const *src, bool src_packed,
		bool counterrors = false, bool reporterrors = false, bool performcopy = true, 
		bool describefields = false, FN_FIELD_COMPARE func = NULL );

	void	CopyShort( difftype_t dt, short *outvalue, const short *invalue, int count );
	void	CopyInt( difftype_t dt, int *outvalue, const int *invalue, int count );		// Copy an int
	void	CopyBool( difftype_t dt, bool *outvalue, const bool *invalue, int count );		// Copy a bool
	void	CopyFloat( difftype_t dt, float *outvalue, const float *invalue, int count );	// Copy a float
	void	CopyString( difftype_t dt, char *outstring, const char *instring );			// Copy a null-terminated string
	void	CopyVector( difftype_t dt, Vector& outValue, const Vector &inValue );				// Copy a vector
	void	CopyVector( difftype_t dt, Vector* outValue, const Vector *inValue, int count );	// Copy a vector array
	void	CopyQuaternion( difftype_t dt, Quaternion& outValue, const Quaternion &inValue );				// Copy a quaternion
	void	CopyQuaternion( difftype_t dt, Quaternion* outValue, const Quaternion *inValue, int count );				// Copy a quaternion array
	void	CopyEHandle( difftype_t dt, EHANDLE *outvalue, EHANDLE const *invalue, int count );

	void	FORCEINLINE CopyData( difftype_t dt, int size, char *outdata, const char *indata )		// Copy a binary data block
	{
		if ( !m_bPerformCopy )
			return;

		if ( dt == IDENTICAL )
			return;

		memcpy( outdata, indata, size );
	}

	int		TransferData( const char *operation, int entindex, datamap_t *dmap );

private:
	void	TransferData_R( int chaincount, datamap_t *dmap );

	void	DetermineWatchField( const char *operation, int entindex,  datamap_t *dmap );
	void	DumpWatchField( typedescription_t *field );
	void	WatchMsg( PRINTF_FORMAT_STRING const char *fmt, ... );

	difftype_t	CompareShort( short *outvalue, const short *invalue, int count );
	difftype_t	CompareInt( int *outvalue, const int *invalue, int count );		// Compare an int
	difftype_t	CompareBool( bool *outvalue, const bool *invalue, int count );		// Compare a bool
	difftype_t	CompareFloat( float *outvalue, const float *invalue, int count );	// Compare a float
	difftype_t	CompareData( int size, char *outdata, const char *indata );		// Compare a binary data block
	difftype_t	CompareString( char *outstring, const char *instring );			// Compare a null-terminated string
	difftype_t	CompareVector( Vector& outValue, const Vector &inValue );				// Compare a vector
	difftype_t	CompareVector( Vector* outValue, const Vector *inValue, int count );	// Compare a vector array
	difftype_t	CompareQuaternion( Quaternion& outValue, const Quaternion &inValue );				// Compare a Quaternion
	difftype_t	CompareQuaternion( Quaternion* outValue, const Quaternion *inValue, int count );	// Compare a Quaternion array
	difftype_t	CompareEHandle( EHANDLE *outvalue, EHANDLE const *invalue, int count );

	void	DescribeShort( difftype_t dt, short *outvalue, const short *invalue, int count );
	void	DescribeInt( difftype_t dt, int *outvalue, const int *invalue, int count );		// Compare an int
	void	DescribeBool( difftype_t dt, bool *outvalue, const bool *invalue, int count );		// Compare a bool
	void	DescribeFloat( difftype_t dt, float *outvalue, const float *invalue, int count );	// Compare a float
	void	DescribeData( difftype_t dt, int size, char *outdata, const char *indata );		// Compare a binary data block
	void	DescribeString( difftype_t dt, char *outstring, const char *instring );			// Compare a null-terminated string
	void	DescribeVector( difftype_t dt, Vector& outValue, const Vector &inValue );				// Compare a vector
	void	DescribeVector( difftype_t dt, Vector* outValue, const Vector *inValue, int count );	// Compare a vector array
	void	DescribeQuaternion( difftype_t dt, Quaternion& outValue, const Quaternion &inValue );				// Compare a Quaternion
	void	DescribeQuaternion( difftype_t dt, Quaternion* outValue, const Quaternion *inValue, int count );	// Compare a Quaternion array
	void	DescribeEHandle( difftype_t dt, EHANDLE *outvalue, EHANDLE const *invalue, int count );

	void	WatchShort( difftype_t dt, short *outvalue, const short *invalue, int count );
	void	WatchInt( difftype_t dt, int *outvalue, const int *invalue, int count );		// Compare an int
	void	WatchBool( difftype_t dt, bool *outvalue, const bool *invalue, int count );		// Compare a bool
	void	WatchFloat( difftype_t dt, float *outvalue, const float *invalue, int count );	// Compare a float
	void	WatchData( difftype_t dt, int size, char *outdata, const char *indata );		// Compare a binary data block
	void	WatchString( difftype_t dt, char *outstring, const char *instring );			// Compare a null-terminated string
	void	WatchVector( difftype_t dt, Vector& outValue, const Vector &inValue );				// Compare a vector
	void	WatchVector( difftype_t dt, Vector* outValue, const Vector *inValue, int count );	// Compare a vector array
	void	WatchQuaternion( difftype_t dt, Quaternion& outValue, const Quaternion &inValue );				// Compare a Quaternion
	void	WatchQuaternion( difftype_t dt, Quaternion* outValue, const Quaternion *inValue, int count );	// Compare a Quaternion array
	void	WatchEHandle( difftype_t dt, EHANDLE *outvalue, EHANDLE const *invalue, int count );

	// Report function
	void	ReportFieldsDiffer( PRINTF_FORMAT_STRING const char *fmt, ... );
	void	DescribeFields( difftype_t dt, PRINTF_FORMAT_STRING const char *fmt, ... );
	
	bool	CanCheck( void );

	void	CopyFields( int chaincount, datamap_t *pMap, typedescription_t *pFields, int fieldCount );

private:

	int				m_nType;
	void			*m_pDest;
	void const		*m_pSrc;
	int				m_nDestOffsetIndex;
	int				m_nSrcOffsetIndex;


	bool			m_bErrorCheck;
	bool			m_bReportErrors;
	bool			m_bDescribeFields;
	typedescription_t *m_pCurrentField;
	char const		*m_pCurrentClassName;
	datamap_t		*m_pCurrentMap;
	bool			m_bShouldReport;
	bool			m_bShouldDescribe;
	int				m_nErrorCount;
	bool			m_bPerformCopy;

	FN_FIELD_COMPARE	m_FieldCompareFunc;

	typedescription_t	 *m_pWatchField;
	char const			*m_pOperation;
};

typedef void (*FN_FIELD_DESCRIPTION)( const char *classname, const char *fieldname, const char *fieldtype,
	bool networked, const char *value );

//-----------------------------------------------------------------------------
// Purpose: Simply dumps all data fields in object
//-----------------------------------------------------------------------------
class CPredictionDescribeData
{
public:
	CPredictionDescribeData( void const *src, bool src_packed, FN_FIELD_DESCRIPTION func = 0 );

	void	DescribeShort( const short *invalue, int count );
	void	DescribeInt( const int *invalue, int count );		
	void	DescribeBool( const bool *invalue, int count );	
	void	DescribeFloat( const float *invalue, int count );	
	void	DescribeData( int size, const char *indata );		
	void	DescribeString( const char *instring );			
	void	DescribeVector( const Vector &inValue );
	void	DescribeVector( const Vector *inValue, int count );
	void	DescribeQuaternion( const Quaternion &inValue );
	void	DescribeQuaternion( const Quaternion *inValue, int count );
	void	DescribeEHandle( EHANDLE const *invalue, int count );

	void	DumpDescription( datamap_t *pMap );

private:
	void	DescribeFields_R( int chain_count, datamap_t *pMap, typedescription_t *pFields, int fieldCount );

	void const		*m_pSrc;
	int				m_nSrcOffsetIndex;

	void			Describe( PRINTF_FORMAT_STRING const char *fmt, ... );

	typedescription_t *m_pCurrentField;
	char const		*m_pCurrentClassName;
	datamap_t		*m_pCurrentMap;

	bool			m_bShouldReport;

	FN_FIELD_DESCRIPTION	m_FieldDescFunc;
};

#if defined( CLIENT_DLL )
class CValueChangeTracker
{
public:
	CValueChangeTracker();

	void Reset();

	void StartTrack( char const *pchContext );
	void EndTrack();

	bool IsActive() const;

	void SetupTracking( C_BaseEntity *ent, char const *pchFieldName );
	void ClearTracking();

	void Spew();

	C_BaseEntity *GetEntity();

private:

	enum
	{
		eChangeTrackerBufSize = 128,
	};

	// Returns field size
	void				GetValue( char *buf, size_t bufsize );

	bool				m_bActive : 1;
	bool				m_bTracking : 1;
	EHANDLE				m_hEntityToTrack;
	CUtlVector< typedescription_t * > m_FieldStack;
	CUtlString			m_strFieldName;
	CUtlString			m_strContext;
	// First 128 bytes of data is all we will consider
	char				m_OrigValueBuf[ eChangeTrackerBufSize ];
	CUtlVector< CUtlString >	 m_History;
};

extern CValueChangeTracker *g_pChangeTracker;

class CValueChangeTrackerScope
{
public:
	CValueChangeTrackerScope( char const *pchContext )
	{
		m_bCallEndTrack = true;
		g_pChangeTracker->StartTrack( pchContext );
	}

	// Only calls Start/End if passed in entity matches entity to track
	CValueChangeTrackerScope( C_BaseEntity *pEntity, char const *pchContext )
	{
		m_bCallEndTrack = g_pChangeTracker->GetEntity() == pEntity;
		if ( m_bCallEndTrack )
		{
			g_pChangeTracker->StartTrack( pchContext );
		}
	}

	~CValueChangeTrackerScope()
	{
		if ( m_bCallEndTrack )
		{
			g_pChangeTracker->EndTrack();
		}
	}
private:

	bool		m_bCallEndTrack;
};

#if defined( _DEBUG )
#define PREDICTION_TRACKVALUECHANGESCOPE( context )		CValueChangeTrackerScope scope( context );
#define PREDICTION_TRACKVALUECHANGESCOPE_ENTITY( entity, context ) CValueChangeTrackerScope scope( entity, context );
#define PREDICTION_STARTTRACKVALUE( context )			g_pChangeTracker->StartTrack( context );
#define PREDICTION_ENDTRACKVALUE()						g_pChangeTracker->EndTrack();
#define PREDICTION_SPEWVALUECHANGES()					g_pChangeTracker->Spew();
#else
#define PREDICTION_TRACKVALUECHANGESCOPE( context )
#define PREDICTION_TRACKVALUECHANGESCOPE_ENTITY( entity, context )
#define PREDICTION_STARTTRACKVALUE( context )
#define PREDICTION_ENDTRACKVALUE()	
#define PREDICTION_SPEWVALUECHANGES() 
#endif

#endif // !CLIENT_DLL
#endif // PREDICTIONCOPY_H
