//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#ifndef VGUI_AVATARIMAGE_H
#define VGUI_AVATARIMAGE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Image.h>
#include <vgui_controls/ImagePanel.h>
#include "steam/steam_api.h"
#include "c_baseplayer.h"

// size of the friend background frame (see texture ico_friend_indicator_avatar)
#define FRIEND_ICON_SIZE_X	(55)	
#define FRIEND_ICON_SIZE_Y	(34)

// offset of avatar within the friend icon
#define FRIEND_ICON_AVATAR_INDENT_X	(22)
#define FRIEND_ICON_AVATAR_INDENT_Y	(1)

// size of the standard avatar icon (unless override by SetAvatarSize)
#define DEFAULT_AVATAR_SIZE		(32)


//=============================================================================
// HPE_CHANGE:
// [pfreese] Refactored these classes so that the CAvatarImage supports a
// default image to be used whenever the Steam avatar is not set or fails to
// be retrieved.
//=============================================================================

struct AvatarImagePair_t
{
	AvatarImagePair_t() { m_iAvatar = 0; }
	AvatarImagePair_t( CSteamID steamID, int av ) { m_SteamID = steamID; m_iAvatar = av; }
	bool operator<( const AvatarImagePair_t &rhs ) const
	{
		return m_SteamID.ConvertToUint64() < rhs.m_SteamID.ConvertToUint64() || 
		( m_SteamID.ConvertToUint64() == rhs.m_SteamID.ConvertToUint64() && m_iAvatar < rhs.m_iAvatar );
	}	
					  
	CSteamID m_SteamID;
	int m_iAvatar;
};

//-----------------------------------------------------------------------------
// Purpose: avatar sizes, formerly used in ISteamFriends, but now only used in game code
//-----------------------------------------------------------------------------
enum EAvatarSize
{
	k_EAvatarSize32x32 = 0,
	k_EAvatarSize64x64 = 1,
	k_EAvatarSize184x184 = 2,
};


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CAvatarImage : public vgui::IImage
{
public:
	CAvatarImage( void );

	// Call this to set the steam ID associated with the avatar
	//=============================================================================
	// HPE_BEGIN:
	// [tj] Added parameter to specify size. Default is 32x32.
	//=============================================================================
	bool SetAvatarSteamID( CSteamID steamIDUser, EAvatarSize avatarSize = k_EAvatarSize32x32 );
	//=============================================================================
	// HPE_END
	//=============================================================================
	void UpdateFriendStatus( void );
	void ClearAvatarSteamID( void );

	// Call to Paint the image
	// Image will draw within the current panel context at the specified position
	virtual void Paint( void );

	// Set the position of the image
	virtual void SetPos(int x, int y)
	{
		m_nX = x;
		m_nY = y;
	}

	// Gets the size of the content
	virtual void GetContentSize(int &wide, int &tall)
	{
		wide = m_wide;
		tall = m_tall;
	}

	// Get the size the image will actually draw in (usually defaults to the content size)
	virtual void GetSize(int &wide, int &tall)
	{
		GetContentSize( wide, tall );
	}

	// Sets the size of the image
	virtual void SetSize(int wide, int tall);

	void SetAvatarSize(int wide, int tall);

	// Set the draw color
	virtual void SetColor(Color col)
	{
		m_Color = col;
	}

	bool	IsValid() { return m_bValid; }
	int		GetWide() { return m_wide; }
	int		GetTall() { return m_tall; }
	int		GetAvatarWide() { return m_avatarWide; }
	int		GetAvatarTall() { return m_avatarTall; }

	//=============================================================================
	// HPE_BEGIN:
	//=============================================================================

	// [tj] simple setter for drawing friend icon
	void	SetDrawFriend(bool drawFriend) { m_bDrawFriend = drawFriend; }

	// [pmf] specify the default (fallback) image
	void SetDefaultImage(vgui::IImage* pImage) { m_pDefaultImage = pImage; }

	//=============================================================================
	// HPE_END
	//=============================================================================

	virtual bool Evict();
	virtual int GetNumFrames();
	virtual void SetFrame( int nFrame );
	virtual vgui::HTexture GetID();
	virtual void SetRotation( int iRotation ) { return; }

protected:
	void InitFromRGBA( int iAvatar, const byte *rgba, int width, int height );

private:
	void UpdateAvatarImageSize();

	void LoadAvatarImage();

	Color m_Color;
	int m_iTextureID;
	int m_nX, m_nY;
	int m_wide, m_tall;
	int	m_avatarWide, m_avatarTall;
	bool m_bValid;
	bool m_bFriend;
	bool m_bLoadPending;
	bool m_bSetDesiredSize;
	float m_fNextLoadTime;	// used to throttle load attempts

	EAvatarSize m_AvatarSize;
	CHudTexture *m_pFriendIcon;
	CSteamID	m_SteamID;

	//=============================================================================
	// HPE_BEGIN:
	//=============================================================================

	// [tj] Whether or not we should draw the friend icon
	bool m_bDrawFriend;

	// [pmf] image to use as a fallback when get from steam fails (or not called)
	vgui::IImage* m_pDefaultImage;

	//=============================================================================
	// HPE_END
	//=============================================================================

	static CUtlMap< AvatarImagePair_t, int > s_AvatarImageCache;
	static bool m_sbInitializedAvatarCache;

	CCallback<CAvatarImage, PersonaStateChange_t, false> m_sPersonaStateChangedCallback;

	void OnPersonaStateChanged( PersonaStateChange_t *info );
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CAvatarImagePanel : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CAvatarImagePanel, vgui::Panel );

	CAvatarImagePanel( vgui::Panel *parent, const char *name );

	// Set the player that this Avatar should display for
	//=============================================================================
	// HPE_BEGIN:
	// [menglish] Added default variable of scalable to allow the avatar to be drawn at sizes other than 32, 32
	// [tj] added a parameter for drawing the friend icon. Defaulted to true to maintain backward compatibility.
	// [menglish] Added parameter to specify a default avatar
	// [menglish] Added a function to set the avatar size of the AvatarImage
	//=============================================================================

	// reset the image to its default value, clearing any info retrieved from Steam
	void ClearAvatar();

	void SetPlayer( C_BasePlayer *pPlayer, EAvatarSize avatarSize = k_EAvatarSize32x32 );

	// [tj] Overloaded function to go straight to entity index
	void SetPlayer( int entityIndex, EAvatarSize avatarSize = k_EAvatarSize32x32 );

	// [tj] lower level function that expects a steam ID instead of a player
	void SetPlayer(CSteamID steamIDForPlayer, EAvatarSize avatarSize	);

	// sets whether or not the image should scale to fit the size of the ImagePanel (defaults to false)
	void SetShouldScaleImage( bool bScaleImage );

	// sets whether to automatically draw the friend icon behind the avatar for Steam friends
	void SetShouldDrawFriendIcon( bool bDrawFriend );

	// specify the size of the avatar portion of the image (the actual image may be larger than this
	// when it incorporates the friend icon)
	void SetAvatarSize( int width, int height);

	// specify a fallback image to use
	void SetDefaultAvatar(vgui::IImage* pDefaultAvatar);

	virtual void OnSizeChanged(int newWide, int newTall);

	//=============================================================================
	// HPE_END
	//=============================================================================

	virtual void OnMousePressed(vgui::MouseCode code);

	virtual void PaintBackground();
	bool	IsValid() { return (m_pImage->IsValid()); }

	void SetClickable( bool bClickable ) { m_bClickable = bClickable; }

protected:
	CPanelAnimationVar( Color, m_clrOutline, "color_outline", "Black" );
	virtual void ApplySettings(KeyValues *inResourceData);

	void UpdateSize();

private:
	CAvatarImage *m_pImage;
	bool m_bScaleImage;
	bool m_bSizeDirty;
	bool m_bClickable;
};

#endif // VGUI_AVATARIMAGE_H
