//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//-----------------------------------------------------------------------------
// FILE: TRISTRIP.CPP
//
// Desc: Xbox tristripper
//
// Copyright (c) 1999-2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

// identifier was truncated to '255' characters in the debug information
#pragma warning(disable: 4786)
// conversion from 'double' to 'float'
#pragma warning(disable: 4244)
#pragma warning(disable: 4530)

#include <stdio.h>
#include <stdarg.h>
#include <algorithm>
#include <list>
#include <vector>

#include <assert.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include "mstristrip.h"

using namespace std;

//=========================================================================
// structs
//=========================================================================
typedef vector<WORD> STRIPVERTS;
typedef list<STRIPVERTS *> STRIPLIST;
typedef WORD (*TRIANGLELIST)[3];

struct TRIANGLEINFO
{
    int neighbortri[3];
    int neighboredge[3];
};

// return true if strip starts clockwise
inline bool FIsStripCW(const STRIPVERTS &stripvertices)
{
    // last index should have cw/ccw bool
    return !!stripvertices[stripvertices.size() - 1];
}

// return length of strip
inline int StripLen(const STRIPVERTS &stripvertices)
{
    return (int)stripvertices.size() - 1;
}

// free all stripverts and clear the striplist
inline void FreeStripListVerts(STRIPLIST *pstriplist)
{
    STRIPLIST::iterator istriplist = pstriplist->begin();
    while(istriplist != pstriplist->end())
    {
        STRIPVERTS *pstripverts = *istriplist;
        delete pstripverts;
        pstriplist->erase(istriplist++);
    }
}

//=========================================================================
// main stripper class
//=========================================================================
class CStripper
{
public:
    // ctors/dtors
    CStripper(int numtris, TRIANGLELIST ptriangles);
    ~CStripper();

    // initialize tri info
    void InitTriangleInfo(int tri, int vert);

    // get maximum length strip from tri/vert
    int CreateStrip(int tri, int vert, int maxlen, int *pswaps,
        bool flookahead, bool fstartcw, int *pstriptris, int *pstripverts);

    // stripify entire mesh
    void BuildStrips(STRIPLIST *pstriplist, int maxlen, bool flookahead);

    // blast strip indices to ppstripindices
    int CreateManyStrips(STRIPLIST *pstriplist, WORD **ppstripindices);
    int CreateLongStrip(STRIPLIST *pstriplist, WORD **ppstripindices);

    inline int GetNeighborCount(int tri)
    {
        int count = 0;
        for(int vert = 0; vert < 3; vert++)
        {
            int neighbortri = m_ptriinfo[tri].neighbortri[vert];
            count += (neighbortri != -1) && !m_pused[neighbortri];
        }
        return count;
    }

    // from callee
    int m_numtris;              // # tris
    TRIANGLELIST m_ptriangles;  // trilist

    TRIANGLEINFO *m_ptriinfo;   // tri edge, neighbor info
    int *m_pused;               // tri used flag
};

//=========================================================================
// vertex cache class
//=========================================================================
class CVertCache
{
public:
    CVertCache()
        { Reset(); }
    ~CVertCache()
        {};

    // reset cache
    void Reset()
    {
        m_iCachePtr = 0;
        m_cachehits = 0;
        memset(m_rgCache, 0xff, sizeof(m_rgCache));
    }

    // add vertindex to cache
    bool Add(int strip, int vertindex);

    int NumCacheHits() const
        { return m_cachehits; }

//    enum { CACHE_SIZE = 10 };
    enum { CACHE_SIZE = 18 };

private:
    int m_cachehits;                // current # of cache hits
    WORD m_rgCache[CACHE_SIZE];     // vertex cache
    int m_rgCacheStrip[CACHE_SIZE]; // strip # which added vert
    int m_iCachePtr;                // fifo ptr
};

//=========================================================================
// Get maximum length of strip starting at tri/vert
//=========================================================================
int CStripper::CreateStrip(int tri, int vert, int maxlen, int *pswaps,
    bool flookahead, bool fstartcw, int *pstriptris, int *pstripverts)
{
    *pswaps = 0;

    // this guy has already been used?
    if(m_pused[tri])
        return 0;

    // mark tri as used
    m_pused[tri] = 1;

    int swaps = 0;

    // add first tri info
    pstriptris[0] = tri;
    pstriptris[1] = tri;
    pstriptris[2] = tri;

    if(fstartcw)
    {
        pstripverts[0] = (vert) % 3;
        pstripverts[1] = (vert + 1) % 3;
        pstripverts[2] = (vert + 2) % 3;
    }
    else
    {
        pstripverts[0] = (vert + 1) % 3;
        pstripverts[1] = (vert + 0) % 3;
        pstripverts[2] = (vert + 2) % 3;
    }
    fstartcw = !fstartcw;

    // get next tri information
    int edge = (fstartcw ? vert + 2 : vert + 1) % 3;
    int nexttri = m_ptriinfo[tri].neighbortri[edge];
    int nextvert = m_ptriinfo[tri].neighboredge[edge];

    // start building the strip until we run out of room or indices
	int stripcount;
    for( stripcount = 3; stripcount < maxlen; stripcount++)
    {
        // dead end?
        if(nexttri == -1 || m_pused[nexttri])
            break;

        // move to next tri
        tri = nexttri;
        vert = nextvert;

        // toggle orientation
        fstartcw = !fstartcw;

        // find the next natural edge
        int edge = (fstartcw ? vert + 2 : vert + 1) % 3;
        nexttri = m_ptriinfo[tri].neighbortri[edge];
        nextvert = m_ptriinfo[tri].neighboredge[edge];

        bool fswap = false;
        if(nexttri == -1 || m_pused[nexttri])
        {
            // if the next tri is a dead end - try swapping orientation
            fswap = true;
        }
        else if(flookahead)
        {
            // try a swap and see who our new neighbor would be
            int edgeswap = (fstartcw ? vert + 1 : vert + 2) % 3;
            int nexttriswap = m_ptriinfo[tri].neighbortri[edgeswap];
            int nextvertswap = m_ptriinfo[tri].neighboredge[edgeswap];

            if(nexttriswap != -1 && !m_pused[nexttriswap])
            {
                assert(nexttri != -1);

                // if the swap neighbor has a lower count, change directions
                if(GetNeighborCount(nexttriswap) < GetNeighborCount(nexttri))
                {
                    fswap = true;
                }
                else if(GetNeighborCount(nexttriswap) == GetNeighborCount(nexttri))
                {
                    // if they have the same number of neighbors - check their neighbors
                    edgeswap = (fstartcw ? nextvertswap + 2 : nextvertswap + 1) % 3;
                    nexttriswap = m_ptriinfo[nexttriswap].neighbortri[edgeswap];

                    int edge1 = (fstartcw ? nextvert + 1 : nextvert + 2) % 3;
                    int nexttri1 = m_ptriinfo[nexttri].neighbortri[edge1];

                    if(nexttri1 == -1 || m_pused[nexttri1])
                    {
                        // natural winding order leads us to a dead end so turn
                        fswap = true;
                    }
                    else if(nexttriswap != -1 && !m_pused[nexttriswap])
                    {
                        // check neighbor counts on both directions and swap if it's better
                        if(GetNeighborCount(nexttriswap) < GetNeighborCount(nexttri1))
                            fswap = true;
                    }
                }
            }
        }

        if(fswap)
        {
            // we've been told to change directions so make sure we actually can
            // and then add the swap vertex
            int edgeswap = (fstartcw ? vert + 1 : vert + 2) % 3;
            nexttri = m_ptriinfo[tri].neighbortri[edgeswap];
            nextvert = m_ptriinfo[tri].neighboredge[edgeswap];

            if(nexttri != -1 && !m_pused[nexttri])
            {
                pstriptris[stripcount] = pstriptris[stripcount - 2];
                pstripverts[stripcount] = pstripverts[stripcount - 2];
                stripcount++;
                swaps++;
                fstartcw = !fstartcw;
            }
        }

        // record index information
        pstriptris[stripcount] = tri;
        pstripverts[stripcount] = (vert + 2) % 3;

        // mark triangle as used
        m_pused[tri] = 1;
    }

    // clear the used flags
    for(int j = 2; j < stripcount; j++)
        m_pused[pstriptris[j]] = 0;

    // return swap count and striplen
    *pswaps = swaps;
    return stripcount;
}

//=========================================================================
// Given a striplist and current cache state, pick the best next strip
//=========================================================================
STRIPLIST::iterator FindBestCachedStrip(STRIPLIST *pstriplist,
    const CVertCache &vertcachestate)
{
    if(pstriplist->empty())
		return pstriplist->end();

    bool fFlipStrip = false;
    int maxcachehits = -1;
    STRIPLIST::iterator istriplistbest = pstriplist->begin();

    int striplen = StripLen(**istriplistbest);
    bool fstartcw = FIsStripCW(**istriplistbest);

    // go through all the other strips looking for the best caching
    for(STRIPLIST::iterator istriplist = pstriplist->begin();
        istriplist != pstriplist->end();
        ++istriplist)
    {
        bool fFlip = false;
        const STRIPVERTS &stripverts = **istriplist;
        int striplennew = StripLen(stripverts);

        // check cache if this strip is the same type as us (ie: cw/odd)
        if((FIsStripCW(stripverts) == fstartcw) &&
            ((striplen & 0x1) == (striplennew & 0x1)))
        {
            // copy current state of cache
            CVertCache vertcachenew = vertcachestate;

            // figure out what this guy would do to our cache
            for(int ivert = 0; ivert < striplennew; ivert++)
                vertcachenew.Add(2, stripverts[ivert]);

            // even length strip - see if better cache hits reversed
            if(!(striplennew & 0x1))
            {
                CVertCache vertcacheflipped = vertcachestate;

                for(int ivert = StripLen(stripverts) - 1; ivert >= 0; ivert--)
                    vertcacheflipped.Add(2, stripverts[ivert]);

                if(vertcacheflipped.NumCacheHits() > vertcachenew.NumCacheHits())
                {
                    vertcachenew = vertcacheflipped;
                    fFlip = true;
                }
            }

            // record the best number of cache hits to date
            int numcachehits = vertcachenew.NumCacheHits() - vertcachestate.NumCacheHits();
            if(numcachehits > maxcachehits)
            {
                maxcachehits = numcachehits;
                istriplistbest = istriplist;
                fFlipStrip = fFlip;
            }
        }
    }

    if(fFlipStrip)
    {
        STRIPVERTS &stripverts = **istriplistbest;
        STRIPVERTS::iterator vend = stripverts.end();
        
        reverse(stripverts.begin(), --vend);
    }

    // make sure we keep the list in order and always pull off
    // the first dude.
    if(istriplistbest != pstriplist->begin())
        swap(*istriplistbest, *pstriplist->begin());

    return pstriplist->begin();
}


//=========================================================================
// Don't merge the strips - just blast em into the stripbuffer one by one
// (useful for debugging)
//=========================================================================
int CStripper::CreateManyStrips(STRIPLIST *pstriplist, WORD **ppstripindices)
{
    // allow room for each of the strips size plus the final 0
    int indexcount = (int)pstriplist->size() + 1;

    // we're storing the strips in [size1 i1 i2 i3][size2 i4 i5 i6][0] format
	STRIPLIST::iterator istriplist;
    for( istriplist = pstriplist->begin(); istriplist != pstriplist->end(); ++istriplist)
    {
        // add striplength plus potential degenerate to swap ccw --> cw
        indexcount += StripLen(**istriplist) + 1;
    }

    // alloc the space for all this stuff
    WORD *pstripindices = new WORD [indexcount];
    assert(pstripindices);

    CVertCache vertcache;
    int numstripindices = 0;

    for(istriplist = pstriplist->begin();
        !pstriplist->empty();
        istriplist = FindBestCachedStrip(pstriplist, vertcache))
    {
        const STRIPVERTS &stripverts = **istriplist;

        if(!FIsStripCW(stripverts))
        {
            // add an extra index if it's ccw
            pstripindices[numstripindices++] = StripLen(stripverts) + 1;
            pstripindices[numstripindices++] = stripverts[0];
        }
        else
        {
            // add the strip length
            pstripindices[numstripindices++] = StripLen(stripverts);
        }

        // add all the strip indices
        for(int i = 0; i < StripLen(stripverts); i++)
        {
            pstripindices[numstripindices++] = stripverts[i];
            vertcache.Add(1, stripverts[i]);
        }

        // free this guy and pop him off the list
        delete &stripverts;
        pstriplist->pop_front();
    }

    // add terminating zero
    pstripindices[numstripindices++] = 0;
    *ppstripindices = pstripindices;

    return numstripindices;
}

//=========================================================================
// Merge striplist into one big uberlist with (hopefully) optimal caching
//=========================================================================
int CStripper::CreateLongStrip(STRIPLIST *pstriplist, WORD **ppstripindices)
{
    // allow room for one strip length plus a possible 3 extra indices per
    // concatenated strip list plus the final 0
    int indexcount = ((int)pstriplist->size() * 3) + 2;

    // we're storing the strips in [size1 i1 i2 i3][size2 i4 i5 i6][0] format
	STRIPLIST::iterator istriplist;
    for( istriplist = pstriplist->begin(); istriplist != pstriplist->end(); ++istriplist)
    {
        indexcount += StripLen(**istriplist);
    }

    // alloc the space for all this stuff
    WORD *pstripindices = new WORD [indexcount];
    assert(pstripindices);

    CVertCache vertcache;
    int numstripindices = 0;

    // add first strip
    istriplist = pstriplist->begin();
    const STRIPVERTS &stripverts = **istriplist;

    // first strip should be cw
    assert(FIsStripCW(stripverts));

    for(int ivert = 0; ivert < StripLen(stripverts); ivert++)
    {
        pstripindices[numstripindices++] = stripverts[ivert];
        vertcache.Add(1, stripverts[ivert]);
    }

    // kill first dude
    delete &stripverts;
    pstriplist->erase(istriplist);

    // add all the others
    while(pstriplist->size())
    {
        istriplist = FindBestCachedStrip(pstriplist, vertcache);
        STRIPVERTS &stripverts = **istriplist;
        short lastvert = pstripindices[numstripindices - 1];
        short firstvert = stripverts[0];

        if(firstvert != lastvert)
        {
            // add degenerate from last strip
            pstripindices[numstripindices++] = lastvert;

            // add degenerate from our strip
            pstripindices[numstripindices++] = firstvert;
        }

        // if we're not orientated correctly, we need to add a degenerate
        if(FIsStripCW(stripverts) != !(numstripindices & 0x1))
        {
            // This shouldn't happen - we're currently trying very hard
            // to keep everything oriented correctly.
            assert(false);
            pstripindices[numstripindices++] = firstvert;
        }

        // add these verts
        for(int ivert = 0; ivert < StripLen(stripverts); ivert++)
        {
            pstripindices[numstripindices++] = stripverts[ivert];
            vertcache.Add(1, stripverts[ivert]);
        }

        // free these guys
        delete &stripverts;
        pstriplist->erase(istriplist);
    }

    *ppstripindices = pstripindices;
    return numstripindices;
}

//=========================================================================
// Build a (hopefully) optimal set of strips from a trilist
//=========================================================================
void CStripper::BuildStrips(STRIPLIST *pstriplist, int maxlen, bool flookahead)
{
    // temp indices storage
    const int ctmpverts = 1024;
    int pstripverts[ctmpverts + 1];
    int pstriptris[ctmpverts + 1];

    assert(maxlen <= ctmpverts);

    // clear all the used flags for the tris
    memset(m_pused, 0, sizeof(m_pused[0]) * m_numtris);

    bool fstartcw = true;
    for(;;)
    {
        int besttri = 0;
        int bestvert = 0;
        float bestratio = 2.0f;
        int bestneighborcount = INT_MAX;

		int tri;
        for( tri = 0; tri < m_numtris; tri++)
        {
            // if used the continue
            if(m_pused[tri])
                continue;

            // get the neighbor count
            int curneightborcount = GetNeighborCount(tri);
            assert(curneightborcount >= 0 && curneightborcount <= 3);

            // push all the singletons to the very end
            if(!curneightborcount)
                curneightborcount = 4;

            // if this guy has more neighbors than the current best - bail
            if(curneightborcount > bestneighborcount)
                continue;

            // try starting the strip with each of this tris verts
            for(int vert = 0; vert < 3; vert++)
            {
                int swaps;
                int len = CreateStrip(tri, vert, maxlen, &swaps, flookahead,
                    fstartcw, pstriptris, pstripverts);
                assert(len);

                float ratio = (len == 3) ? 1.0f : (float)swaps / len;

                // check if this ratio is better than what we've already got for
                // this neighborcount
                if((curneightborcount < bestneighborcount) ||
                    ((curneightborcount == bestneighborcount) && (ratio < bestratio)))
                {
                    bestneighborcount = curneightborcount;

                    besttri = tri;
                    bestvert = vert;
                    bestratio = ratio;
                }

            }
        }

        // no strips found?
        if(bestneighborcount == INT_MAX)
            break;

        // recreate this strip
        int swaps;
        int len = CreateStrip(besttri, bestvert, maxlen,
            &swaps, flookahead, fstartcw, pstriptris, pstripverts);
        assert(len);

        // mark the tris on the best strip as used
        for(tri = 0; tri < len; tri++)
            m_pused[pstriptris[tri]] = 1;

        // create a new STRIPVERTS and stuff in the indices
        STRIPVERTS *pstripvertices = new STRIPVERTS(len + 1);
        assert(pstripvertices);

        // store orientation in first entry
        for(tri = 0; tri < len; tri++)
            (*pstripvertices)[tri] = m_ptriangles[pstriptris[tri]][pstripverts[tri]];
        (*pstripvertices)[len] = fstartcw;

        // store the STRIPVERTS
        pstriplist->push_back(pstripvertices);

        // if strip was odd - swap orientation
        if((len & 0x1))
            fstartcw = !fstartcw;
    }

#ifdef _DEBUG
    // make sure all tris are used
    for(int t = 0; t < m_numtris; t++)
        assert(m_pused[t]);
#endif
}

//=========================================================================
// Guesstimate on the total index count for this list of strips
//=========================================================================
int EstimateStripCost(STRIPLIST *pstriplist)
{
    int count = 0;

    for(STRIPLIST::iterator istriplist = pstriplist->begin();
        istriplist != pstriplist->end();
        ++istriplist)
    {
        // add count of indices
        count += StripLen(**istriplist);
    }

    // assume 2 indices per strip to tack all these guys together
    return count + ((int)pstriplist->size() - 1) * 2;
}

//=========================================================================
// Initialize triangle information (edges, #neighbors, etc.)
//=========================================================================
void CStripper::InitTriangleInfo(int tri, int vert)
{
    WORD *ptriverts = &m_ptriangles[tri + 1][0];
    int vert1 = m_ptriangles[tri][(vert + 1) % 3];
    int vert2 = m_ptriangles[tri][vert];

    for(int itri = tri + 1; itri < m_numtris; itri++, ptriverts += 3)
    {
        if(m_pused[itri] != 0x7)
        {
            for(int ivert = 0; ivert < 3; ivert++)
            {
                if((ptriverts[ivert] == vert1) &&
                    (ptriverts[(ivert + 1) % 3] == vert2))
                {
                    // add the triangle info
                    m_ptriinfo[tri].neighbortri[vert] = itri;
                    m_ptriinfo[tri].neighboredge[vert] = ivert;
                    m_pused[tri] |= (1 << vert);

                    m_ptriinfo[itri].neighbortri[ivert] = tri;
                    m_ptriinfo[itri].neighboredge[ivert] = vert;
                    m_pused[itri] |= (1 << ivert);
                    return;
                }
            }
        }
    }
}

//=========================================================================
// CStripper ctor
//=========================================================================
CStripper::CStripper(int numtris, TRIANGLELIST ptriangles)
{
    // store trilist info
    m_numtris = numtris;
    m_ptriangles = ptriangles;

    m_pused = new int[numtris];
    assert(m_pused);
    m_ptriinfo = new TRIANGLEINFO[numtris];
    assert(m_ptriinfo);

    // init triinfo
	int itri;
    for( itri = 0; itri < numtris; itri++)
    {
        m_ptriinfo[itri].neighbortri[0] = -1;
        m_ptriinfo[itri].neighbortri[1] = -1;
        m_ptriinfo[itri].neighbortri[2] = -1;
    }

    // clear the used flag
    memset(m_pused, 0, sizeof(m_pused[0]) * m_numtris);

    // go through all the triangles and find edges, neighbor counts
    for(itri = 0; itri < numtris; itri++)
    {
        for(int ivert = 0; ivert < 3; ivert++)
        {
            if(!(m_pused[itri] & (1 << ivert)))
                InitTriangleInfo(itri, ivert);
        }
    }

    // clear the used flags from InitTriangleInfo
    memset(m_pused, 0, sizeof(m_pused[0]) * m_numtris);
}

//=========================================================================
// CStripper dtor
//=========================================================================
CStripper::~CStripper()
{
    // free stuff
    delete [] m_pused;
    m_pused = NULL;

    delete [] m_ptriinfo;
    m_ptriinfo = NULL;
}

//=========================================================================
// Add an index to the cache - returns true if it was added, false otherwise
//=========================================================================
bool CVertCache::Add(int strip, int vertindex)
{
    // find index in cache
    for(int iCache = 0; iCache < CACHE_SIZE; iCache++)
    {
        if(vertindex == m_rgCache[iCache])
        {
            // if it's in the cache and it's from a different strip
            // change the strip to the new one and count the cache hit
            if(strip != m_rgCacheStrip[iCache])
            {
                m_cachehits++;
                m_rgCacheStrip[iCache] = strip;
                return true;
            }

            // we added this item to the cache earlier - carry on
            return false;
        }
    }

    // not in cache, add vert and strip
    m_rgCache[m_iCachePtr] = vertindex;
    m_rgCacheStrip[m_iCachePtr] = strip;
    m_iCachePtr = (m_iCachePtr + 1) % CACHE_SIZE;
    return true;
}

#ifdef _DEBUG
//=========================================================================
// Turn on c runtime leak checking, etc.
//=========================================================================
void EnableLeakChecking()
{
    int flCrtDbgFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

    flCrtDbgFlags &=
        ~(_CRTDBG_LEAK_CHECK_DF |
        _CRTDBG_CHECK_ALWAYS_DF |
        _CRTDBG_DELAY_FREE_MEM_DF);

    // always check for memory leaks
    flCrtDbgFlags |= _CRTDBG_LEAK_CHECK_DF;

    // others you may / may not want to set
    flCrtDbgFlags |= _CRTDBG_CHECK_ALWAYS_DF;
    flCrtDbgFlags |= _CRTDBG_DELAY_FREE_MEM_DF;

    _CrtSetDbgFlag(flCrtDbgFlags);

    // all types of reports go via OutputDebugString
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

    // big errors and asserts get their own assert window
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW);

    // _CrtSetBreakAlloc(0);
}
#endif

//=========================================================================
// Main Stripify routine
//=========================================================================
int Stripify(int numtris, WORD *ptriangles, int *pnumindices, WORD **ppstripindices)
{
    if(!numtris || !ptriangles)
        return 0;

#ifdef _DEBUG
//    EnableLeakChecking();
#endif

    CStripper stripper(numtris, (TRIANGLELIST)ptriangles);

    // map of various args to try stripifying mesh with
    struct ARGMAP
    {
        int     maxlen;         // maximum length of strips
        bool    flookahead;     // use sgi greedy lookahead (or not)
    } rgargmap[] =
    {
        { 1024,  true  },
        { 1024,  false },
    };
    static const int cargmaps = sizeof(rgargmap) / sizeof(rgargmap[0]);
    STRIPLIST   striplistbest;
    int         bestlistcost = 0;

    for(int imap = 0; imap < cargmaps; imap++)
    {
        STRIPLIST striplist;

        // build the strip with the various args
        stripper.BuildStrips(&striplist, rgargmap[imap].maxlen,
            rgargmap[imap].flookahead);

        // guesstimate the list cost and store it if it's good
        int listcost = EstimateStripCost(&striplist);
        if(!bestlistcost || (listcost < bestlistcost))
        {
            // free the old best list
            FreeStripListVerts(&striplistbest);

            // store the new best list
            striplistbest = striplist;
            bestlistcost = listcost;
            assert(bestlistcost > 0);
        }
        else
        {
            FreeStripListVerts(&striplist);
        }
    }

#ifdef NEVER
    // Return the strips in [size1 i1 i2 i3][size2 i4 i5 i6]...[0] format
    // Very useful for debugging...
    return stripper.CreateManyStrips(&striplistbest, ppstripindices);
#endif // NEVER

    // return one big long strip
    int numindices = stripper.CreateLongStrip(&striplistbest, ppstripindices);

    if(pnumindices)
        *pnumindices = numindices;
    return numindices;
}

//=========================================================================
// Class used to vertices for locality of access.
//=========================================================================
struct SortEntry
{
public:
    int iFirstUsed;
    int iOrigIndex;

    bool operator<(const SortEntry& rhs) const
    {
        return iFirstUsed < rhs.iFirstUsed;
    }
};

//=========================================================================
// Reorder the vertices
//=========================================================================
void ComputeVertexPermutation(int numstripindices, WORD* pstripindices,
                              int* pnumverts, WORD** ppvertexpermutation)
{
    // Sort verts to maximize locality.
    SortEntry* pSortTable = new SortEntry[*pnumverts];

    // Fill in original index.
	int i;
    for( i = 0; i < *pnumverts; i++)
    {
        pSortTable[i].iOrigIndex = i;
        pSortTable[i].iFirstUsed = -1;
    }

    // Fill in first used flag.
    for(i = 0; i < numstripindices; i++)
    {
        int index = pstripindices[i];

        if(pSortTable[index].iFirstUsed == -1)
            pSortTable[index].iFirstUsed = i;
    }

    // Sort the table.
    sort(pSortTable, pSortTable + *pnumverts);

    // Copy re-mapped to orignal vertex permutaion into output array.
    *ppvertexpermutation = new WORD[*pnumverts];

    for(i = 0; i < *pnumverts; i++)
    {
        (*ppvertexpermutation)[i] = pSortTable[i].iOrigIndex;
    }

    // Build original to re-mapped permutation.
    WORD* pInversePermutation = new WORD[numstripindices];

    for(i = 0; i < *pnumverts; i++)
    {
        pInversePermutation[pSortTable[i].iOrigIndex] = i;
    }

    // We need to remap indices as well.
    for(i = 0; i < numstripindices; i++)
    {
        pstripindices[i] = pInversePermutation[pstripindices[i]];
    }

    delete[] pSortTable;
    delete[] pInversePermutation;
}

