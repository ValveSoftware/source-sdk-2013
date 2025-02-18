//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMECLIP_H
#define DMECLIP_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"
#include "datamodel/dmehandle.h"
#include "video/ivideoservices.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "tier1/utlmap.h"
#include "movieobjects/timeutils.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeClip;
class CDmeTimeFrame;
class CDmeBookmark;
class CDmeSound;
class CDmeChannel;
class CDmeCamera;
class CDmeLight;
class CDmeDag;
class CDmeInput;
class CDmeOperator;
class CDmeMaterial;
class CDmeTrack;
class CDmeTrackGroup;
class IMaterial;
class CDmeChannelsClip;
class CDmeAnimationSet;
class CDmeMaterialOverlayFXClip;
class DmeLog_TimeSelection_t;
struct Rect_t;


enum DmeClipSkipFlag_t
{
	DMESKIP_NONE = 0,
	DMESKIP_MUTED = 1,
	DMESKIP_INVISIBLE = 2,
};
DEFINE_ENUM_BITWISE_OPERATORS( DmeClipSkipFlag_t )


//-----------------------------------------------------------------------------
// Clip types
//-----------------------------------------------------------------------------
enum DmeClipType_t
{
	DMECLIP_UNKNOWN = -1,

	DMECLIP_FIRST = 0,

	DMECLIP_CHANNEL = 0,
	DMECLIP_SOUND,
	DMECLIP_FX,
	DMECLIP_FILM,

	DMECLIP_LAST = DMECLIP_FILM,

	DMECLIP_TYPE_COUNT
};
DEFINE_ENUM_INCREMENT_OPERATORS( DmeClipType_t )


typedef CUtlVector< CDmeHandle< CDmeClip > > DmeClipStack_t;


//-----------------------------------------------------------------------------
// Is a particular clip type non-overlapping?
//-----------------------------------------------------------------------------
inline bool IsNonoverlapping( DmeClipType_t type )
{
	return ( type == DMECLIP_FILM );
}


//-----------------------------------------------------------------------------
// String to clip type + back
//-----------------------------------------------------------------------------
DmeClipType_t ClipTypeFromString( const char *pName );
const char *ClipTypeToString( DmeClipType_t type );


//-----------------------------------------------------------------------------
// Used to move clips in non-film track groups with film clips
//-----------------------------------------------------------------------------
struct ClipAssociation_t
{
	enum AssociationType_t
	{
		HAS_CLIP = 0,
		BEFORE_START,
		AFTER_END,
		NO_MOVEMENT,
	};

	AssociationType_t m_nType;
	CDmeHandle< CDmeClip > m_hClip;
	CDmeHandle< CDmeClip > m_hAssociation;
	DmeTime_t m_offset;
};


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
class CDmeClip : public CDmElement
{
	DEFINE_ELEMENT( CDmeClip, CDmElement );

public:
	// Inherited from IDmElement
	virtual void OnAttributeArrayElementAdded( CDmAttribute *pAttribute, int nFirstElem, int nLastElem );
	virtual void OnAttributeArrayElementRemoved( CDmAttribute *pAttribute, int nFirstElem, int nLastElem );

	// Returns the time frame
	CDmeTimeFrame *GetTimeFrame() const;
	DmeTime_t ToChildMediaTime  ( DmeTime_t t, bool bClamp = true ) const;
	DmeTime_t FromChildMediaTime( DmeTime_t t, bool bClamp = true ) const;
	DmeTime_t ToChildMediaDuration  ( DmeTime_t dt ) const;
	DmeTime_t FromChildMediaDuration( DmeTime_t dt ) const;
	DmeTime_t GetStartTime() const;
	DmeTime_t GetEndTime() const;
	DmeTime_t GetDuration() const;
	DmeTime_t GetTimeOffset() const;
	DmeTime_t GetStartInChildMediaTime() const;
	DmeTime_t GetEndInChildMediaTime() const;
	float GetTimeScale() const;
	void SetStartTime ( DmeTime_t t );
	void SetDuration  ( DmeTime_t t );
	void SetTimeOffset( DmeTime_t t );
	void SetTimeScale ( float s );

	// Given a root clip and a child (or grandchild) clip, builds the stack 
	// from root on down to the destination clip. If shot is specified, then it
	// must build a clip stack that passes through the shot
	bool BuildClipStack( DmeClipStack_t* pStack, CDmeClip *pRoot, CDmeClip *pShot = NULL );

	// Clip stack versions of time conversion
	static DmeTime_t	ToChildMediaTime   ( const DmeClipStack_t& stack, DmeTime_t globalTime, bool bClamp = true );
	static DmeTime_t	FromChildMediaTime ( const DmeClipStack_t& stack, DmeTime_t localTime,  bool bClamp = true );
	static DmeTime_t	ToChildMediaDuration  ( const DmeClipStack_t& stack, DmeTime_t globalDuration );
	static DmeTime_t	FromChildMediaDuration( const DmeClipStack_t& stack, DmeTime_t localDuration );
	static void			ToChildMediaTime( DmeLog_TimeSelection_t &params, const DmeClipStack_t& stack );

	void SetClipColor( const Color& clr );
	Color GetClipColor() const;

	void SetClipText( const char *pText );
	const char* GetClipText() const;

	// Clip type
	virtual DmeClipType_t GetClipType() { return DMECLIP_UNKNOWN; }

	// Track group iteration methods
	int GetTrackGroupCount() const;
	CDmeTrackGroup *GetTrackGroup( int nIndex ) const;
	const CUtlVector< DmElementHandle_t > &GetTrackGroups( ) const;

	// Track group addition/removal
	void AddTrackGroup( CDmeTrackGroup *pTrackGroup );
	void AddTrackGroupBefore( CDmeTrackGroup *pTrackGroup, CDmeTrackGroup *pBefore );
	CDmeTrackGroup *AddTrackGroup( const char *pTrackGroupName );
	void RemoveTrackGroup( int nIndex );
	void RemoveTrackGroup( CDmeTrackGroup *pTrackGroup );
	void RemoveTrackGroup( const char *pTrackGroupName );

	// Track group finding
	CDmeTrackGroup *FindTrackGroup( const char *pTrackGroupName ) const;
	int GetTrackGroupIndex( CDmeTrackGroup *pTrack ) const;
	CDmeTrackGroup *FindOrAddTrackGroup( const char *pTrackGroupName );

	// Swap track groups
	void SwapOrder( CDmeTrackGroup *pTrackGroup1, CDmeTrackGroup *pTrackGroup2 );

	// Clip finding
	virtual CDmeTrack *FindTrackForClip( CDmeClip *pClip, CDmeTrackGroup **ppTrackGroup = NULL ) const;
	bool FindMultiTrackGroupForClip( CDmeClip *pClip, int *pTrackGroupIndex, int *pTrackIndex = NULL, int *pClipIndex = NULL ) const;

	// Finding clips in tracks by time
	virtual void FindClipsAtTime( DmeClipType_t clipType, DmeTime_t time, DmeClipSkipFlag_t flags, CUtlVector< CDmeClip * >& clips ) const;
	virtual void FindClipsWithinTime( DmeClipType_t clipType, DmeTime_t startTime, DmeTime_t endTime, DmeClipSkipFlag_t flags, CUtlVector< CDmeClip * >& clips ) const;

	// Is a particular clip typed able to be added?
	bool IsSubClipTypeAllowed( DmeClipType_t type ) const;

	// Returns the special film track group
	virtual CDmeTrackGroup *GetFilmTrackGroup() const { return NULL; }
	virtual CDmeTrack *GetFilmTrack() const { return NULL; }

	// Checks for muteness
	void SetMute( bool state );
	bool IsMute( ) const;

protected:
	virtual int AllowedClipTypes() const { return 1 << DMECLIP_CHANNEL; }

	// Is a track group valid to add?
	bool IsTrackGroupValid( CDmeTrackGroup *pTrackGroup );

	CDmaElementArray< CDmeTrackGroup > m_TrackGroups;

	CDmaElement< CDmeTimeFrame > m_TimeFrame;
	CDmaVar< Color > m_ClipColor;
	CDmaVar< bool > m_bMute;
	CDmaString m_ClipText;
};

inline bool CDmeClip::IsSubClipTypeAllowed( DmeClipType_t type ) const
{
	return ( AllowedClipTypes() & ( 1 << type ) ) != 0;
}

inline void CDmeClip::SetMute( bool state )
{
	m_bMute = state;
}

inline bool CDmeClip::IsMute( ) const
{
	return m_bMute;
}


//-----------------------------------------------------------------------------
// Sound clip
//-----------------------------------------------------------------------------
class CDmeSoundClip : public CDmeClip
{
	DEFINE_ELEMENT( CDmeSoundClip, CDmeClip );

public:
	virtual DmeClipType_t GetClipType() { return DMECLIP_SOUND; }

	void SetShowWave( bool state );
	bool ShouldShowWave( ) const;

	CDmaElement< CDmeSound > m_Sound;
	CDmaVar< bool > m_bShowWave;

};


//-----------------------------------------------------------------------------
// Clip containing recorded data from the game
//-----------------------------------------------------------------------------
class CDmeChannelsClip : public CDmeClip
{
	DEFINE_ELEMENT( CDmeChannelsClip, CDmeClip );

public:
	virtual DmeClipType_t GetClipType() { return DMECLIP_CHANNEL; }

	CDmeChannel *CreatePassThruConnection
	( 
		char const *passThruName,
		CDmElement *pFrom, 
		char const *pFromAttribute, 
		CDmElement *pTo, 
		char const *pToAttribute,
		int index = 0 
	);

	void RemoveChannel( CDmeChannel *pChannel );

	CDmaElementArray< CDmeChannel > m_Channels;
};


//-----------------------------------------------------------------------------
// An effect clip
//-----------------------------------------------------------------------------
class CDmeFXClip : public CDmeClip
{
	DEFINE_ELEMENT( CDmeFXClip, CDmeClip );

public:
	virtual DmeClipType_t GetClipType() { return DMECLIP_FX; }

	enum
	{
		MAX_FX_INPUT_TEXTURES = 2
	};

	// All effects must be able to apply their effect
	virtual void ApplyEffect( DmeTime_t time, Rect_t &currentRect, Rect_t &totalRect, ITexture *pTextures[MAX_FX_INPUT_TEXTURES] ) {}

	// Global list of FX clip types
	static void InstallFXClipType( const char *pElementType, const char *pDescription );
	static int FXClipTypeCount();
	static const char *FXClipType( int nIndex );
	static const char *FXClipDescription( int nIndex );

private:
	enum
	{
		MAX_FXCLIP_TYPES = 16
	};
	static const char *s_pFXClipTypes[MAX_FXCLIP_TYPES];
	static const char *s_pFXClipDescriptions[MAX_FXCLIP_TYPES];
	static int s_nFXClipTypeCount;
};


//-----------------------------------------------------------------------------
// Helper Template factory for simple creation of factories
//-----------------------------------------------------------------------------
template <class T>
class CDmFXClipFactory : public CDmElementFactory<T>
{
public:
	CDmFXClipFactory( const char *pLookupName, const char *pDescription ) : CDmElementFactory<T>( pLookupName ) 
	{
		CDmeFXClip::InstallFXClipType( pLookupName, pDescription );
	}
};


//-----------------------------------------------------------------------------
// All effects must use IMPLEMENT_FX_CLIP_ELEMENT_FACTORY instead of IMPLEMENT_ELEMENT_FACTORY
//-----------------------------------------------------------------------------
#if defined( MOVIEOBJECTS_LIB ) || defined ( DATAMODEL_LIB ) || defined ( DMECONTROLS_LIB )

#define IMPLEMENT_FX_CLIP_ELEMENT_FACTORY( lookupName, className, description )	\
	IMPLEMENT_ELEMENT( className )																\
	CDmFXClipFactory< className > g_##className##_Factory( #lookupName, description );			\
	CDmElementFactoryHelper g_##className##_Helper( #lookupName, &g_##className##_Factory, true );	\
	className *g_##className##LinkerHack = NULL;

#else

#define IMPLEMENT_FX_CLIP_ELEMENT_FACTORY( lookupName, className, description )	\
	IMPLEMENT_ELEMENT( className )																\
	CDmFXClipFactory< className > g_##className##_Factory( #lookupName, description );			\
	CDmElementFactoryHelper g_##className##_Helper( #lookupName, &g_##className##_Factory, false );	\
	className *g_##className##LinkerHack = NULL;

#endif

//-----------------------------------------------------------------------------
// Film clip
//-----------------------------------------------------------------------------
class CDmeFilmClip : public CDmeClip
{
	DEFINE_ELEMENT( CDmeFilmClip, CDmeClip );

public:
	virtual DmeClipType_t GetClipType() { return DMECLIP_FILM; }

	// Attribute changed
	virtual void OnElementUnserialized( );
	virtual void PreAttributeChanged( CDmAttribute *pAttribute );
	virtual void OnAttributeChanged( CDmAttribute *pAttribute );

	// Resolve
	virtual void Resolve();

	// Returns the special film track group
	virtual CDmeTrackGroup *GetFilmTrackGroup() const;
	virtual CDmeTrack *GetFilmTrack() const;

	CDmeTrackGroup *FindOrCreateFilmTrackGroup();
	CDmeTrack *FindOrCreateFilmTrack();

	// Clip finding
	virtual CDmeTrack *FindTrackForClip( CDmeClip *pClip, CDmeTrackGroup **ppTrackGroup = NULL ) const;

	// Finding clips in tracks by time
	virtual void FindClipsAtTime( DmeClipType_t clipType, DmeTime_t time, DmeClipSkipFlag_t flags, CUtlVector< CDmeClip * >& clips ) const;
	virtual void FindClipsWithinTime( DmeClipType_t clipType, DmeTime_t startTime, DmeTime_t endTime, DmeClipSkipFlag_t flags, CUtlVector< CDmeClip * >& clips ) const;

	// mapname helper methods
	const char *GetMapName();
	void SetMapName( const char *pMapName );

	// Returns the camera associated with the clip
	CDmeCamera *GetCamera();
	void SetCamera( CDmeCamera *pCamera );

	// Audio volume
	void			SetVolume( float state );
	float			GetVolume() const;

	// Returns the monitor camera associated with the clip (for now, only 1 supported)
	CDmeCamera *GetMonitorCamera();
	void AddMonitorCamera( CDmeCamera *pCamera );
	void RemoveMonitorCamera( CDmeCamera *pCamera );
	void SelectMonitorCamera( CDmeCamera *pCamera );
	int FindMonitorCamera( CDmeCamera *pCamera );

	// Light helper methods
	int GetLightCount();
	CDmeLight *GetLight( int nIndex );
	void AddLight( CDmeLight *pLight );

	// Scene / Dag helper methods
	void SetScene( CDmeDag *pDag );
	CDmeDag *GetScene();

	// helper for inputs and operators
	int GetInputCount();
	CDmeInput *GetInput( int nIndex );
	void AddInput( CDmeInput *pInput );
	void RemoveAllInputs();
	void AddOperator( CDmeOperator *pOperator );
	void CollectOperators( CUtlVector< DmElementHandle_t > &operators );

	// Helper for overlays
	// FIXME: Change this to use CDmeMaterials
	IMaterial *GetOverlayMaterial();
	void SetOverlay( const char *pMaterialName );
	float GetOverlayAlpha();
	void SetOverlayAlpha( float alpha );
	void DrawOverlay( DmeTime_t time, Rect_t &currentRect, Rect_t &totalRect );
	bool HasOpaqueOverlay();

	// AVI tape out
	void UseCachedVersion( bool bUseCachedVersion );
	bool IsUsingCachedVersion() const;
	IVideoMaterial *GetCachedVideoMaterial();
	void SetCachedAVI( const char *pAVIFile );

	int	GetAnimationSetCount();
	CDmeAnimationSet *GetAnimationSet( int idx );
	void	AddAnimationSet( CDmeAnimationSet *element );
	void	RemoveAllAnimationSets();
	CDmaElementArray< CDmElement > &GetAnimationSets(); // raw access to the array
	const CDmaElementArray< CDmElement > &GetAnimationSets() const;

	const CDmaElementArray< CDmeBookmark > &GetBookmarks() const;
	CDmaElementArray< CDmeBookmark > &GetBookmarks();

	void SetFadeTimes( DmeTime_t fadeIn, DmeTime_t fadeOut ) { m_fadeInDuration = fadeIn.GetTenthsOfMS(); m_fadeOutDuration = fadeOut.GetTenthsOfMS(); }
	void SetFadeInTime( DmeTime_t t ) { m_fadeInDuration = t.GetTenthsOfMS(); }
	void SetFadeOutTime( DmeTime_t t ) { m_fadeOutDuration = t.GetTenthsOfMS(); }
	DmeTime_t GetFadeInTime() const { return DmeTime_t( m_fadeInDuration.Get() ); }
	DmeTime_t GetFadeOutTime() const { return DmeTime_t( m_fadeOutDuration.Get() ); }

	// Used to move clips in non-film track groups with film clips
	// Call BuildClipAssociations before modifying the film track,
	// then UpdateAssociatedClips after modifying it.
	void BuildClipAssociations( CUtlVector< ClipAssociation_t > &association, bool bHandleGaps = true );
	void UpdateAssociatedClips( CUtlVector< ClipAssociation_t > &association );

	// Rolls associated clips so they remain in the same relative time
	void RollAssociatedClips( CDmeClip *pClip, CUtlVector< ClipAssociation_t > &association, DmeTime_t dt );

	// Shifts associated clips so they remain in the same relative time when pClip is scaled
	void ScaleAssociatedClips( CDmeClip *pClip, CUtlVector< ClipAssociation_t > &association, float ratio, DmeTime_t oldOffset );

private:
	virtual int AllowedClipTypes() const { return (1 << DMECLIP_CHANNEL) | (1 << DMECLIP_SOUND) | (1 << DMECLIP_FX) | (1 << DMECLIP_FILM); }

	CDmaElement< CDmeTrackGroup >			m_FilmTrackGroup;

	CDmaString								m_MapName;
	CDmaElement     < CDmeCamera >			m_Camera;
	CDmaElementArray< CDmeCamera >			m_MonitorCameras;
	CDmaVar< int >							m_nActiveMonitor;
	CDmaElement     < CDmeDag >				m_Scene;
	CDmaElementArray< CDmeLight >			m_Lights;

	CDmaElementArray< CDmeInput >			m_Inputs;
	CDmaElementArray< CDmeOperator >		m_Operators;

	CDmaString								m_AVIFile;

	CDmaVar< int >							m_fadeInDuration;
	CDmaVar< int >							m_fadeOutDuration;

	CDmaElement< CDmeMaterialOverlayFXClip >m_MaterialOverlayEffect;
	CDmaVar< bool >							m_bIsUsingCachedVersion;

	CDmaElementArray< CDmElement >			m_AnimationSets; // "animationSets"
	CDmaElementArray< CDmeBookmark >		m_Bookmarks;

	CDmaVar< float >						m_Volume;

	IVideoMaterial						   *m_pCachedVersion;	
	bool m_bReloadCachedVersion;
	CMaterialReference m_FadeMaterial;
};


//-----------------------------------------------------------------------------
// Fast type conversions
//-----------------------------------------------------------------------------
inline bool IsFilmClip( CDmeClip *pClip )
{
	return pClip && pClip->IsA( CDmeFilmClip::GetStaticTypeSymbol() );
}


//-----------------------------------------------------------------------------
// Creates a slug clip
//-----------------------------------------------------------------------------
CDmeFilmClip *CreateSlugClip( const char *pClipName, DmeTime_t startTime, DmeTime_t endTime, DmFileId_t fileid );


//-----------------------------------------------------------------------------
// For use in template functions
//-----------------------------------------------------------------------------
template <class T>
class CDmeClipInfo
{
public:
	static DmeClipType_t ClipType( ) { return DMECLIP_UNKNOWN; }
};

#define DECLARE_DMECLIP_TYPE( _className, _dmeClipType )			\
	template< > class CDmeClipInfo< _className >					\
	{																\
	public:															\
		static DmeClipType_t ClipType() { return _dmeClipType; }	\
	};

DECLARE_DMECLIP_TYPE( CDmeSoundClip,	DMECLIP_SOUND )
DECLARE_DMECLIP_TYPE( CDmeChannelsClip,	DMECLIP_CHANNEL )
DECLARE_DMECLIP_TYPE( CDmeFXClip,		DMECLIP_FX )
DECLARE_DMECLIP_TYPE( CDmeFilmClip,		DMECLIP_FILM )

#define DMECLIP_TYPE( _className )	CDmeClipInfo<T>::ClipType()


//-----------------------------------------------------------------------------
// helper methods
//-----------------------------------------------------------------------------
CDmeTrack *GetParentTrack( CDmeClip *pClip );
CDmeChannel *FindChannelTargetingElement( CDmeChannelsClip *pChannelsClip, CDmElement *pElement, const char *pAttributeName = NULL );
CDmeChannel *FindChannelTargetingElement( CDmeFilmClip *pClip, CDmElement *pElement, const char *pAttributeName, CDmeChannelsClip **ppChannelsClip, CDmeTrack **ppTrack = NULL, CDmeTrackGroup **ppTrackGroup = NULL );

#endif // DMECLIP_H
