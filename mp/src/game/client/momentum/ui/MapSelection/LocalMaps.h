#ifndef LOCALMAPS_H
#define LOCALMAPS_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: Local maps list
//-----------------------------------------------------------------------------
class CLocalMaps : public CBaseMapsPage
{
    DECLARE_CLASS_SIMPLE(CLocalMaps, CBaseMapsPage);

public: 

    CLocalMaps(vgui::Panel *parent);
    ~CLocalMaps();

    // property page handlers
    virtual void OnPageShow();

    // IGameList handlers
    // returns true if the game list supports the specified ui elements
    virtual bool SupportsItem(InterfaceItem_e item);

    // Control which button are visible.
    void ManualShowButtons(bool bShowConnect, bool bShowRefreshAll, bool bShowFilter);

    //Filters based on the filter data
    virtual void StartRefresh();
    void GetNewMapList();//called upon loading

    virtual void OnMapStart() { BaseClass::OnMapStart(); }

    // Tell the game list what to put in there when there are no games found.
    virtual void SetEmptyListText();

    //virtual void LoadFilterSettings() {};//MOM_TODO: Make this sort by name/gametype/difficulty?

private:
    // context menu message handlers
    MESSAGE_FUNC_INT(OnOpenContextMenu, "OpenContextMenu", itemID);

    // true if we're broadcasting for servers
    bool m_bLoadedMaps;

    //Fills a mapstruct with data read from local files
    void FillMapstruct(mapstruct_t *);
};

class CTimeSortFunc
{
public:
    bool Less(KeyValues* lhs, KeyValues* rhs, void *)
    {
        return (((float) Q_atoi(lhs->GetName())) * lhs->GetFloat("rate") <
            (float) Q_atoi(rhs->GetName()) * rhs->GetFloat("rate"));
    }
};

#endif // LOCALMAPS_H