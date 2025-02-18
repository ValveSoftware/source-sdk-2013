//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Interface for the client to general GC API
//
// $NoKeywords: $
//=============================================================================

#ifndef C_TF_FREEACCOUNT_H
#define C_TF_FREEACCOUNT_H
#ifdef _WIN32
#pragma once
#endif

namespace vgui
{
	class Panel;
};
class CSelectPlayerDialog;

/**
 * @return true if the local player is using a free trial account, false otherwise
 */
bool IsFreeTrialAccount();

/**
 * @return true if the local player needs to choose their most helpful friend, false otherwse
 */
bool NeedsToChooseMostHelpfulFriend();

/**
 *   Adds an alert that the player needs to choose their most helpful friend
 */
void NotifyNeedsToChooseMostHelpfulFriend();

/**
 * Opens the dialog where the user can specify the friend that helped them the most
 * @param pParent
 * @return CSelectPlayerDialog
 */
CSelectPlayerDialog *OpenSelectMostHelpfulFriendDialog( vgui::Panel *pParent );

#endif // C_TF_FREEACCOUNT_H
