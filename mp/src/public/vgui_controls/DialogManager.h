//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DIALOGMANAGER_H
#define DIALOGMANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include <utllinkedlist.h>
#include <KeyValues.h>
#include <vgui_controls/PHandle.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: utility class, maps a set of ID's to dialogs
//			used to manage sets of similar dialogs (object property dialogs, etc.)
//-----------------------------------------------------------------------------
template <class TDialog, class I = int>
class DialogManager
{
public:
	// new dialog factory function
	typedef TDialog *(*CreateNewDialogFunc_t)(I dialogID);

	// constructor
	DialogManager(CreateNewDialogFunc_t createDialogFunc);

	// finds the dialog by the specified ID
	TDialog *FindDialog(I dialogID, bool bCreate);

	// opens the dialog; creating it if specified
	TDialog *ActivateDialog(I dialogID, bool bCreate);

	// closes all the dialogs
	void CloseAll();

	// closes and deletes all the dialogs
	void CloseAndDeleteAll();

	// returns number of active dialogs
	int Count();

	// sets parent to use
	void SetParent( vgui::VPANEL parent );

private:
	// checks if an index in the dialog list is valid; if it has been deleted, removes the entry
	bool ValidateIndex(int index);

	struct DialogItem_t
	{
		I id;
		DHANDLE<TDialog> dlg;
	};

	CUtlLinkedList<DialogItem_t, int> m_Dialogs;
	CreateNewDialogFunc_t m_CreateFunc;
	vgui::VPANEL m_pVGUIParentPanel;
};


// constructor
template <class TDialog, class I>
inline DialogManager<TDialog, I>::DialogManager(CreateNewDialogFunc_t createDialogFunc)
{
	m_CreateFunc = createDialogFunc;
	m_pVGUIParentPanel = NULL;
}

// finds the dialog; creating it if necessary
template <class TDialog, class I>
inline TDialog *DialogManager<TDialog, I>::FindDialog(I dialogID, bool bCreate)
{
	for (int i = 0; i < m_Dialogs.MaxElementIndex(); i++)
	{
		if (ValidateIndex(i) && m_Dialogs[i].id == dialogID)
		{
			return m_Dialogs[i].dlg;
		}
	}

	if (bCreate)
	{
		int newIndex = m_Dialogs.AddToTail();
		if (m_CreateFunc)
		{
			m_Dialogs[newIndex].dlg = m_CreateFunc(dialogID);
		}
		else
		{
			m_Dialogs[newIndex].dlg = new TDialog(NULL, dialogID);
		}
		Assert(m_pVGUIParentPanel);
		m_Dialogs[newIndex].dlg->SetParent( m_pVGUIParentPanel );

		m_Dialogs[newIndex].id = dialogID;
		return m_Dialogs[newIndex].dlg;
	}

	// dlg not found, not created
	return NULL;
}

// opens the dialog; creating it if necessary
template <class TDialog, class I>
inline TDialog *DialogManager<TDialog, I>::ActivateDialog(I dialogID, bool bCreate)
{
	TDialog *dlg = FindDialog(dialogID, bCreate);
	if (dlg)
	{
		dlg->Activate();
	}
	return dlg;
}

// count
template <class TDialog, class I>
inline int DialogManager<TDialog, I>::Count()
{
	// validate all the indexes first
	for (int i = 0; i < m_Dialogs.MaxElementIndex(); i++)
	{
		if (ValidateIndex(i))
		{
		}
	}

	// return the (remaining) count
	return m_Dialogs.Count();
}

// closes all the dialogs
template <class TDialog, class I>
inline void DialogManager<TDialog, I>::CloseAll()
{
	for (int i = 0; i < m_Dialogs.MaxElementIndex(); i++)
	{
		if (ValidateIndex(i))
		{
			m_Dialogs[i].dlg->PostMessage(m_Dialogs[i].dlg, new KeyValues("Close"));
		}
	}
}

// closes and deletes all the dialogs
template <class TDialog, class I>
inline void DialogManager<TDialog, I>::CloseAndDeleteAll()
{
	CloseAll();
	for (int i = 0; i < m_Dialogs.MaxElementIndex(); i++)
	{
		if (ValidateIndex(i))
		{
			m_Dialogs[i].dlg->MarkForDeletion();
		}
	}
	m_Dialogs.RemoveAll();
}

// checks if a dialog is valid; if it has been deleted, removes the entry
template <class TDialog, class I>
inline bool DialogManager<TDialog, I>::ValidateIndex(int index)
{
	if (m_Dialogs.IsValidIndex(index))
	{
		if (m_Dialogs[index].dlg.Get())
		{
			return true;
		}
		else
		{
			// entry has been deleted; removed
			m_Dialogs.Remove(index);
		}
	}
	return false;
}

template <class TDialog, class I>
inline void DialogManager<TDialog, I>::SetParent( vgui::VPANEL parent )
{
	m_pVGUIParentPanel = parent;
}


} // namespace vgui

#endif // DIALOGMANAGER_H
