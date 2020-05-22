/*
see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqfuncproto.h"
#include "sqclosure.h"

SQTable::SQTable(SQSharedState *ss,SQInteger nInitialSize)
{
    SQInteger pow2size=MINPOWER2;
    while(nInitialSize>pow2size)pow2size=pow2size<<1;
    AllocNodes(pow2size);
    _usednodes = 0;
    _delegate = NULL;
    INIT_CHAIN();
    ADD_TO_CHAIN(&_sharedstate->_gc_chain,this);
}

void SQTable::Remove(const SQObjectPtr &key)
{

    _HashNode *n = _Get(key, HashObj(key) & (_numofnodes - 1));
    if (n) {
        n->val.Null();
        n->key.Null();
        _usednodes--;
        Rehash(false);
    }
}

void SQTable::AllocNodes(SQInteger nSize)
{
    _HashNode *nodes=(_HashNode *)SQ_MALLOC(sizeof(_HashNode)*nSize);
    for(SQInteger i=0;i<nSize;i++){
        _HashNode &n = nodes[i];
        new (&n) _HashNode;
        n.next=NULL;
    }
    _numofnodes=nSize;
    _nodes=nodes;
    _firstfree=&_nodes[_numofnodes-1];
}

void SQTable::Rehash(bool force)
{
    SQInteger oldsize=_numofnodes;
    //prevent problems with the integer division
    if(oldsize<4)oldsize=4;
    _HashNode *nold=_nodes;
    SQInteger nelems=CountUsed();
    if (nelems >= oldsize-oldsize/4)  /* using more than 3/4? */
        AllocNodes(oldsize*2);
    else if (nelems <= oldsize/4 &&  /* less than 1/4? */
        oldsize > MINPOWER2)
        AllocNodes(oldsize/2);
    else if(force)
        AllocNodes(oldsize);
    else
        return;
    _usednodes = 0;
    for (SQInteger i=0; i<oldsize; i++) {
        _HashNode *old = nold+i;
        if (sq_type(old->key) != OT_NULL)
            NewSlot(old->key,old->val);
    }
    for(SQInteger k=0;k<oldsize;k++)
        nold[k].~_HashNode();
    SQ_FREE(nold,oldsize*sizeof(_HashNode));
}

SQTable *SQTable::Clone()
{
    SQTable *nt=Create(_opt_ss(this),_numofnodes);
#ifdef _FAST_CLONE
    _HashNode *basesrc = _nodes;
    _HashNode *basedst = nt->_nodes;
    _HashNode *src = _nodes;
    _HashNode *dst = nt->_nodes;
    SQInteger n = 0;
    for(n = 0; n < _numofnodes; n++) {
        dst->key = src->key;
        dst->val = src->val;
        if(src->next) {
            assert(src->next > basesrc);
            dst->next = basedst + (src->next - basesrc);
            assert(dst != dst->next);
        }
        dst++;
        src++;
    }
    assert(_firstfree > basesrc);
    assert(_firstfree != NULL);
    nt->_firstfree = basedst + (_firstfree - basesrc);
    nt->_usednodes = _usednodes;
#else
    SQInteger ridx=0;
    SQObjectPtr key,val;
    while((ridx=Next(true,ridx,key,val))!=-1){
        nt->NewSlot(key,val);
    }
#endif
    nt->SetDelegate(_delegate);
    return nt;
}

bool SQTable::Get(const SQObjectPtr &key,SQObjectPtr &val)
{
    if(sq_type(key) == OT_NULL)
        return false;
    _HashNode *n = _Get(key, HashObj(key) & (_numofnodes - 1));
    if (n) {
        val = _realval(n->val);
        return true;
    }
    return false;
}
bool SQTable::NewSlot(const SQObjectPtr &key,const SQObjectPtr &val)
{
    assert(sq_type(key) != OT_NULL);
    SQHash h = HashObj(key) & (_numofnodes - 1);
    _HashNode *n = _Get(key, h);
    if (n) {
        n->val = val;
        return false;
    }
    _HashNode *mp = &_nodes[h];
    n = mp;


    //key not found I'll insert it
    //main pos is not free

    if(sq_type(mp->key) != OT_NULL) {
        n = _firstfree;  /* get a free place */
        SQHash mph = HashObj(mp->key) & (_numofnodes - 1);
        _HashNode *othern;  /* main position of colliding node */

        if (mp > n && (othern = &_nodes[mph]) != mp){
            /* yes; move colliding node into free position */
            while (othern->next != mp){
                assert(othern->next != NULL);
                othern = othern->next;  /* find previous */
            }
            othern->next = n;  /* redo the chain with `n' in place of `mp' */
            n->key = mp->key;
            n->val = mp->val;/* copy colliding node into free pos. (mp->next also goes) */
            n->next = mp->next;
            mp->key.Null();
            mp->val.Null();
            mp->next = NULL;  /* now `mp' is free */
        }
        else{
            /* new node will go into free position */
            n->next = mp->next;  /* chain new position */
            mp->next = n;
            mp = n;
        }
    }
    mp->key = key;

    for (;;) {  /* correct `firstfree' */
        if (sq_type(_firstfree->key) == OT_NULL && _firstfree->next == NULL) {
            mp->val = val;
            _usednodes++;
            return true;  /* OK; table still has a free place */
        }
        else if (_firstfree == _nodes) break;  /* cannot decrement from here */
        else (_firstfree)--;
    }
    Rehash(true);
    return NewSlot(key, val);
}

SQInteger SQTable::Next(bool getweakrefs,const SQObjectPtr &refpos, SQObjectPtr &outkey, SQObjectPtr &outval)
{
    SQInteger idx = (SQInteger)TranslateIndex(refpos);
    while (idx < _numofnodes) {
        if(sq_type(_nodes[idx].key) != OT_NULL) {
            //first found
            _HashNode &n = _nodes[idx];
            outkey = n.key;
            outval = getweakrefs?(SQObject)n.val:_realval(n.val);
            //return idx for the next iteration
            return ++idx;
        }
        ++idx;
    }
    //nothing to iterate anymore
    return -1;
}


bool SQTable::Set(const SQObjectPtr &key, const SQObjectPtr &val)
{
    _HashNode *n = _Get(key, HashObj(key) & (_numofnodes - 1));
    if (n) {
        n->val = val;
        return true;
    }
    return false;
}

void SQTable::_ClearNodes()
{
    for(SQInteger i = 0;i < _numofnodes; i++) { _HashNode &n = _nodes[i]; n.key.Null(); n.val.Null(); }
}

void SQTable::Finalize()
{
    _ClearNodes();
    SetDelegate(NULL);
}

void SQTable::Clear()
{
    _ClearNodes();
    _usednodes = 0;
    Rehash(true);
}
