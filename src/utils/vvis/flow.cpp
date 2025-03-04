#include "vis.h"
#include "vmpi.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

int g_TraceClusterStart = -1;
int g_TraceClusterStop = -1;

__global__ void CountBitsKernel(byte* bits, int numbits, int* result) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < numbits) {
        atomicAdd(result, (bits[i / 8] & (1 << (i % 8))) ? 1 : 0);
    }
}

int CountBits(byte* bits, int numbits) {
    int* d_result;
    cudaMalloc(&d_result, sizeof(int));
    cudaMemset(d_result, 0, sizeof(int));

    int threadsPerBlock = 256;
    int blocksPerGrid = (numbits + threadsPerBlock - 1) / threadsPerBlock;
    CountBitsKernel<<<blocksPerGrid, threadsPerBlock>>>(bits, numbits, d_result);

    int result;
    cudaMemcpy(&result, d_result, sizeof(int), cudaMemcpyDeviceToHost);
    cudaFree(d_result);

    return result;
}

int c_fullskip;
int c_portalskip, c_leafskip;
int c_vistest, c_mighttest;

int c_chop, c_nochop;

int active;

#ifdef MPI
extern bool g_bVMPIEarlyExit;
#endif

void CheckStack(leaf_t* leaf, threaddata_t* thread) {
    pstack_t* p, * p2;

    for (p = thread->pstack_head.next; p; p = p->next) {
        if (p->leaf == leaf)
            Error("CheckStack: leaf recursion");
        for (p2 = thread->pstack_head.next; p2 != p; p2 = p2->next)
            if (p2->leaf == p->leaf)
                Error("CheckStack: late leaf recursion");
    }
}

winding_t* AllocStackWinding(pstack_t* stack) {
    int i;

    for (i = 0; i < 3; i++) {
        if (stack->freewindings[i]) {
            stack->freewindings[i] = 0;
            return &stack->windings[i];
        }
    }

    Error("Out of memory. AllocStackWinding: failed");

    return NULL;
}

void FreeStackWinding(winding_t* w, pstack_t* stack) {
    int i;

    i = w - stack->windings;

    if (i < 0 || i > 2)
        return;  // not from local

    if (stack->freewindings[i])
        Error("FreeStackWinding: already free");
    stack->freewindings[i] = 1;
}

winding_t* ChopWinding(winding_t* in, pstack_t* stack, plane_t* split) {
    vec_t dists[128];
    int sides[128];
    int counts[3];
    vec_t dot;
    int i, j;
    Vector mid;
    winding_t* neww;

    counts[0] = counts[1] = counts[2] = 0;

    for (i = 0; i < in->numpoints; i++) {
        dot = DotProduct(in->points[i], split->normal);
        dot -= split->dist;
        dists[i] = dot;
        if (dot > ON_VIS_EPSILON)
            sides[i] = SIDE_FRONT;
        else if (dot < -ON_VIS_EPSILON)
            sides[i] = SIDE_BACK;
        else {
            sides[i] = SIDE_ON;
        }
        counts[sides[i]]++;
    }

    if (!counts[1])
        return in;  // completely on front side

    if (!counts[0]) {
        FreeStackWinding(in, stack);
        return NULL;
    }

    sides[i] = sides[0];
    dists[i] = dists[0];

    neww = AllocStackWinding(stack);

    neww->numpoints = 0;

    for (i = 0; i < in->numpoints; i++) {
        Vector& p1 = in->points[i];

        if (neww->numpoints == MAX_POINTS_ON_FIXED_WINDING) {
            FreeStackWinding(neww, stack);
            return in;  // can't chop -- fall back to original
        }

        if (sides[i] == SIDE_ON) {
            VectorCopy(p1, neww->points[neww->numpoints]);
            neww->numpoints++;
            continue;
        }

        if (sides[i] == SIDE_FRONT) {
            VectorCopy(p1, neww->points[neww->numpoints]);
            neww->numpoints++;
        }

        if (sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
            continue;

        if (neww->numpoints == MAX_POINTS_ON_FIXED_WINDING) {
            FreeStackWinding(neww, stack);
            return in;  // can't chop -- fall back to original
        }

        Vector& p2 = in->points[(i + 1) % in->numpoints];

        dot = dists[i] / (dists[i] - dists[i + 1]);
        for (j = 0; j < 3; j++) {
            if (split->normal[j] == 1)
                mid[j] = split->dist;
            else if (split->normal[j] == -1)
                mid[j] = -split->dist;
            else
                mid[j] = p1[j] + dot * (p2[j] - p1[j]);
        }

        VectorCopy(mid, neww->points[neww->numpoints]);
        neww->numpoints++;
    }

    FreeStackWinding(in, stack);

    return neww;
}

winding_t* ClipToSeperators(winding_t* source, winding_t* pass, winding_t* target, bool flipclip, pstack_t* stack) {
    int i, j, k, l;
    plane_t plane;
    Vector v1, v2;
    float d;
    vec_t length;
    int counts[3];
    bool fliptest;

    for (i = 0; i < source->numpoints; i++) {
        l = (i + 1) % source->numpoints;
        VectorSubtract(source->points[l], source->points[i], v1);

        for (j = 0; j < pass->numpoints; j++) {
            VectorSubtract(pass->points[j], source->points[i], v2);

            plane.normal[0] = v1[1] * v2[2] - v1[2] * v2[1];
            plane.normal[1] = v1[2] * v2[0] - v1[0] * v2[2];
            plane.normal[2] = v1[0] * v2[1] - v1[1] * v2[0];

            length = plane.normal[0] * plane.normal[0] + plane.normal[1] * plane.normal[1] + plane.normal[2] * plane.normal[2];

            if (length < ON_VIS_EPSILON)
                continue;

            length = 1 / sqrt(length);

            plane.normal[0] *= length;
            plane.normal[1] *= length;
            plane.normal[2] *= length;

            plane.dist = DotProduct(pass->points[j], plane.normal);

            fliptest = false;
            for (k = 0; k < source->numpoints; k++) {
                if (k == i || k == l)
                    continue;
                d = DotProduct(source->points[k], plane.normal) - plane.dist;
                if (d < -ON_VIS_EPSILON) {
                    fliptest = false;
                    break;
                }
                else if (d > ON_VIS_EPSILON) {
                    fliptest = true;
                    break;
                }
            }
            if (k == source->numpoints)
                continue;

            if (fliptest) {
                VectorSubtract(vec3_origin, plane.normal, plane.normal);
                plane.dist = -plane.dist;
            }

            counts[0] = counts[1] = counts[2] = 0;
            for (k = 0; k < pass->numpoints; k++) {
                if (k == j)
                    continue;
                d = DotProduct(pass->points[k], plane.normal) - plane.dist;
                if (d < -ON_VIS_EPSILON)
                    break;
                else if (d > ON_VIS_EPSILON)
                    counts[0]++;
                else
                    counts[2]++;
            }
            if (k != pass->numpoints)
                continue;

            if (!counts[0])
                continue;

            if (flipclip) {
                VectorSubtract(vec3_origin, plane.normal, plane.normal);
                plane.dist = -plane.dist;
            }

            target = ChopWinding(target, stack, &plane);
            if (!target)
                return NULL;
        }
    }

    return target;
}

class CPortalTrace {
public:
    CUtlVector<Vector> m_list;
    CThreadFastMutex m_mutex;
} g_PortalTrace;

void WindingCenter(winding_t* w, Vector& center) {
    int i;
    float scale;

    VectorCopy(vec3_origin, center);
    for (i = 0; i < w->numpoints; i++)
        VectorAdd(w->points[i], center, center);

    scale = 1.0 / w->numpoints;
    VectorScale(center, scale, center);
}

Vector ClusterCenter(int cluster) {
    Vector mins, maxs;
    ClearBounds(mins, maxs);
    int count = leafs[cluster].portals.Count();
    for (int i = 0; i < count; i++) {
        winding_t* w = leafs[cluster].portals[i]->winding;
        for (int j = 0; j < w->numpoints; j++) {
            AddPointToBounds(w->points[j], mins, maxs);
        }
    }
    return (mins + maxs) * 0.5f;
}

void DumpPortalTrace(pstack_t* pStack) {
    AUTO_LOCK(g_PortalTrace.m_mutex);
    if (g_PortalTrace.m_list.Count())
        return;

    Warning("Dumped cluster trace!!!\n");
    Vector mid;
    mid = ClusterCenter(g_TraceClusterStart);
    g_PortalTrace.m_list.AddToTail(mid);
    for (; pStack != NULL; pStack = pStack->next) {
        winding_t* w = pStack->pass ? pStack->pass : pStack->portal->winding;
        WindingCenter(w, mid);
        g_PortalTrace.m_list.AddToTail(mid);
        for (int i = 0; i < w->numpoints; i++) {
            g_PortalTrace.m_list.AddToTail(w->points[i]);
            g_PortalTrace.m_list.AddToTail(mid);
        }
        for (int i = 0; i < w->numpoints; i++) {
            g_PortalTrace.m_list.AddToTail(w->points[i]);
        }
        g_PortalTrace.m_list.AddToTail(w->points[0]);
        g_PortalTrace.m_list.AddToTail(mid);
    }
    mid = ClusterCenter(g_TraceClusterStop);
    g_PortalTrace.m_list.AddToTail(mid);
}

void WritePortalTrace(const char* source) {
    Vector mid;
    FILE* linefile;
    char filename[1024];

    if (!g_PortalTrace.m_list.Count()) {
        Warning("No trace generated from %d to %d\n", g_TraceClusterStart, g_TraceClusterStop);
        return;
    }

    sprintf(filename, "%s.lin", source);
    linefile = fopen(filename, "w");
    if (!linefile)
        Error("Couldn't open %s\n", filename);

    for (int i = 0; i < g_PortalTrace.m_list.Count(); i++) {
        Vector p = g_PortalTrace.m_list[i];
        fprintf(linefile, "%f %f %f\n", p[0], p[1], p[2]);
    }
    fclose(linefile);
    Warning("Wrote %s!!!\n", filename);
}

void RecursiveLeafFlow(int leafnum, threaddata_t* thread, pstack_t* prevstack) {
    pstack_t stack;
    portal_t* p;
    plane_t backplane;
    leaf_t* leaf;
    int i, j;
    long* test, * might, * vis, more;
    int pnum;

#ifdef MPI
    if (g_bVMPIEarlyExit)
        return;
#endif

    if (leafnum == g_TraceClusterStop) {
        DumpPortalTrace(&thread->pstack_head);
        return;
    }
    thread->c_chains++;

    leaf = &leafs[leafnum];

    prevstack->next = &stack;

    stack.next = NULL;
    stack.leaf = leaf;
    stack.portal = NULL;

    might = (long*)stack.mightsee;
    vis = (long*)thread->base->portalvis;

    for (i = 0; i < leaf->portals.Count(); i++) {
        p = leaf->portals[i];
        pnum = p - portals;

        if (!(prevstack->mightsee[pnum >> 3] & (1 << (pnum & 7)))) {
            continue;
        }

        if (p->status == stat_done) {
            test = (long*)p->portalvis;
        }
        else {
            test = (long*)p->portalflood;
        }

        more = 0;
        for (j = 0; j < portallongs; j++) {
            might[j] = ((long*)prevstack->mightsee)[j] & test[j];
            more |= (might[j] & ~vis[j]);
        }

        if (!more && CheckBit(thread->base->portalvis, pnum)) {
            continue;
        }

        stack.portalplane = p->plane;
        VectorSubtract(vec3_origin, p->plane.normal, backplane.normal);
        backplane.dist = -p->plane.dist;

        stack.portal = p;
        stack.next = NULL;
        stack.freewindings[0] = 1;
        stack.freewindings[1] = 1;
        stack.freewindings[2] = 1;

        float d = DotProduct(p->origin, thread->pstack_head.portalplane.normal);
        d -= thread->pstack_head.portalplane.dist;
        if (d < -p->radius) {
            continue;
        }
        else if (d > p->radius) {
            stack.pass = p->winding;
        }
        else {
            stack.pass = ChopWinding(p->winding, &stack, &thread->pstack_head.portalplane);
            if (!stack.pass)
                continue;
        }

        d = DotProduct(thread->base->origin, p->plane.normal);
        d -= p->plane.dist;
        if (d > thread->base->radius) {
            continue;
        }
        else if (d < -thread->base->radius) {
            stack.source = prevstack->source;
        }
        else {
            stack.source = ChopWinding(prevstack->source, &stack, &backplane);
            if (!stack.source)
                continue;
        }

        if (!prevstack->pass) {
            SetBit(thread->base->portalvis, pnum);

            RecursiveLeafFlow(p->leaf, thread, &stack);
            continue;
        }

        stack.pass = ClipToSeperators(stack.source, prevstack->pass, stack.pass, false, &stack);
        if (!stack.pass)
            continue;

        stack.pass = ClipToSeperators(prevstack->pass, stack.source, stack.pass, true, &stack);
        if (!stack.pass)
            continue;

        SetBit(thread->base->portalvis, pnum);

        RecursiveLeafFlow(p->leaf, thread, &stack);
    }
}

void PortalFlow(int iThread, int portalnum) {
    threaddata_t data;
    int i;
    portal_t* p;
    int c_might, c_can;

    p = sorted_portals[portalnum];
    p->status = stat_working;

    c_might = CountBits(p->portalflood, g_numportals * 2);

    memset(&data, 0, sizeof(data));
    data.base = p;

    data.pstack_head.portal = p;
    data.pstack_head.source = p->winding;
    data.pstack_head.portalplane = p->plane;
    for (i = 0; i < portallongs; i++)
        ((long*)data.pstack_head.mightsee)[i] = ((long*)p->portalflood)[i];

    RecursiveLeafFlow(p->leaf, &data, &data.pstack_head);

    p->status = stat_done;

    c_can = CountBits(p->portalvis, g_numportals * 2);

    qprintf("portal:%4i  mightsee:%4i  cansee:%4i (%i chains)\n",
        (int)(p - portals), c_might, c_can, data.c_chains);
}

void SimpleFlood(portal_t* srcportal, int leafnum) {
    int i;
    leaf_t* leaf;
    portal_t* p;
    int pnum;

    leaf = &leafs[leafnum];

    for (i = 0; i < leaf->portals.Count(); i++) {
        p = leaf->portals[i];
        pnum = p - portals;
        if (!CheckBit(srcportal->portalfront, pnum))
            continue;

        if (CheckBit(srcportal->portalflood, pnum))
            continue;

        SetBit(srcportal->portalflood, pnum);

        SimpleFlood(srcportal, p->leaf);
    }
}

void BasePortalVis(int iThread, int portalnum) {
    int j, k;
    portal_t* tp, * p;
    float d;
    winding_t* w;
    Vector segment;
    double dist2, minDist2;

    p = portals + portalnum;

    p->portalfront = (byte*)malloc(portalbytes);
    memset(p->portalfront, 0, portalbytes);

    p->portalflood = (byte*)malloc(portalbytes);
    memset(p->portalflood, 0, portalbytes);

    p->portalvis = (byte*)malloc(portalbytes);
    memset(p->portalvis, 0, portalbytes);

    for (j = 0, tp = portals; j < g_numportals * 2; j++, tp++) {
        if (j == portalnum)
            continue;

        w = tp->winding;
        for (k = 0; k < w->numpoints; k++) {
            d = DotProduct(w->points[k], p->plane.normal) - p->plane.dist;
            if (d > ON_VIS_EPSILON)
                break;
        }
        if (k == w->numpoints)
            continue;

        w = p->winding;
        for (k = 0; k < w->numpoints; k++) {
            d = DotProduct(w->points[k], tp->plane.normal) - tp->plane.dist;
            if (d < -ON_VIS_EPSILON)
                break;
        }
        if (k == w->numpoints)
            continue;

        if (g_bUseRadius) {
            w = tp->winding;
            minDist2 = 1024000000.0;
            for (k = 0; k < w->numpoints; k++) {
                VectorSubtract(w->points[k], p->origin, segment);
                dist2 = (segment[0] * segment[0]) + (segment[1] * segment[1]) + (segment[2] * segment[2]);
                if (dist2 < minDist2) {
                    minDist2 = dist2;
                }
            }

            if (minDist2 > g_VisRadius)
                continue;
        }

        SetBit(p->portalfront, j);
    }

    SimpleFlood(p, p->leaf);

    p->nummightsee = CountBits(p->portalflood, g_numportals * 2);
    c_flood += p->nummightsee;
}

void RecursiveLeafBitFlow(int leafnum, byte* mightsee, byte* cansee) {
    portal_t* p;
    leaf_t* leaf;
    int i, j;
    long more;
    int pnum;
    byte newmight[MAX_PORTALS / 8];

    leaf = &leafs[leafnum];

    for (i = 0; i < leaf->portals.Count(); i++) {
        p = leaf->portals[i];
        pnum = p - portals;

        if (!CheckBit(mightsee, pnum))
            continue;

        more = 0;
        for (j = 0; j < portallongs; j++) {
            ((long*)newmight)[j] = ((long*)mightsee)[j] & ((long*)p->portalflood)[j];
            more |= ((long*)newmight)[j] & ~((long*)cansee)[j];
        }

        if (!more)
            continue;

        SetBit(cansee, pnum);

        RecursiveLeafBitFlow(p->leaf, newmight, cansee);
    }
}

void BetterPortalVis(int portalnum) {
    portal_t* p;

    p = portals + portalnum;

    RecursiveLeafBitFlow(p->leaf, p->portalflood, p->portalvis);

    p->nummightsee = CountBits(p->portalvis, g_numportals * 2);
    c_vis += p->nummightsee;
}
