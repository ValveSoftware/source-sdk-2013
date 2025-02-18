//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  
//			
//=============================================================================

#ifndef TF_PROGRESSION_DESCRIPTION_H
#define TF_PROGRESSION_DESCRIPTION_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	class CBaseModelPanel;
#endif

struct LevelInfo_t
{
	uint32 m_nLevelNum;
	uint32 m_nDisplayLevel;
	uint32 m_nStartXP;	// Inclusive
	uint32 m_nEndXP;	// Non-inclusive
	CUtlString m_pszLevelTitle;
	const char* m_pszLevelUpSound;
	const char* m_pszLobbyBackgroundImage;
};

struct XPSourceDef_t
{
	const char* m_pszSoundName;
	const char* m_pszFormattingLocToken;
	const char* m_pszTypeLocToken;
	float m_flValueMultiplier;
};

extern const XPSourceDef_t g_XPSourceDefs[ CMsgTFXPSource::XPSourceType_ARRAYSIZE ];

class IProgressionDesc
{
public:
	friend class IMatchGroupDescription;

	IProgressionDesc( const char* pszBadgeName
					, const char* pszProgressionResFile 
					, const char* pszLevelToken );

#ifdef CLIENT_DLL
	virtual void GetLocalizedLevelTitle( const LevelInfo_t& level, wchar_t* wszOutString, int wszOutStringSize ) const = 0;
	virtual const char* GetRankUnitsLocToken() const = 0;
#endif // CLIENT_DLL

	virtual const LevelInfo_t& GetLevelForRating( uint32 nExperience ) const;
	const LevelInfo_t& GetLevelByNumber( uint32 nNumber ) const;
	uint32 GetNumLevels() const { return m_vecLevels.Count(); }


	const CUtlString m_strBadgeName;
	const char* m_pszLevelToken;
	const char* m_pszProgressionResFile;

protected:
#ifdef CLIENT_DLL
	virtual void SetupBadgePanel( CBaseModelPanel *pModelPanel, const LevelInfo_t& level, const CSteamID& steamID, bool bInPlacement ) const = 0;
	void EnsureBadgePanelModel( CBaseModelPanel *pModelPanel ) const;
#endif

	CUtlVector< LevelInfo_t > m_vecLevels;
};

#endif //TF_PROGRESSION_DESCRIPTION_H
