
import core.stdc.config;
import std.bitmanip : bitfields;

struct CppClassSizeAttr
{
    alias size this;
    size_t size;
}
CppClassSizeAttr cppclasssize(size_t a) { return CppClassSizeAttr(a); }

struct CppSizeAttr
{
    alias size this;
    size_t size;
}
CppSizeAttr cppsize(size_t a) { return CppSizeAttr(a); }

struct CppMethodAttr{}
CppMethodAttr cppmethod() { return CppMethodAttr(); }

alias dtAllocHint = int;
enum  : dtAllocHint
{
    DT_ALLOC_PERM = 0, 
    DT_ALLOC_TEMP = 1, 
}

alias dtAllocFunc = extern(C++) void* function(ulong, dtAllocHint);

alias dtFreeFunc = extern(C++) void function(void*);

alias dtAssertFailFunc = extern(C++) void function(const(byte)*, const(byte)*, int);

extern(C++) 
float dtMathFabsf (float x);

extern(C++) 
float dtMathSqrtf (float x);

extern(C++) 
float dtMathFloorf (float x);

extern(C++) 
float dtMathCeilf (float x);

extern(C++) 
float dtMathCosf (float x);

extern(C++) 
float dtMathSinf (float x);

extern(C++) 
float dtMathAtan2f (float y, float x);

extern(C++) 
void dtIgnoreUnused (T)(ref const T );

extern(C++) 
void dtSwap (T)(ref T a, ref T b);

extern(C++) 
T dtMin (T)(T a, T b);

extern(C++) 
T dtMax (T)(T a, T b);

extern(C++) 
T dtAbs (T)(T a);

extern(C++) 
T dtSqr (T)(T a);

extern(C++) 
T dtClamp (T)(T v, T mn, T mx);

extern(C++) 
void dtVcross (float* dest, const(float)* v1, const(float)* v2);

extern(C++) 
float dtVdot (const(float)* v1, const(float)* v2);

extern(C++) 
void dtVmad (float* dest, const(float)* v1, const(float)* v2, const float s);

extern(C++) 
void dtVlerp (float* dest, const(float)* v1, const(float)* v2, const float t);

extern(C++) 
void dtVadd (float* dest, const(float)* v1, const(float)* v2);

extern(C++) 
void dtVsub (float* dest, const(float)* v1, const(float)* v2);

extern(C++) 
void dtVscale (float* dest, const(float)* v, const float t);

extern(C++) 
void dtVmin (float* mn, const(float)* v);

extern(C++) 
void dtVmax (float* mx, const(float)* v);

extern(C++) 
void dtVset (float* dest, const float x, const float y, const float z);

extern(C++) 
void dtVcopy (float* dest, const(float)* a);

extern(C++) 
float dtVlen (const(float)* v);

extern(C++) 
float dtVlenSqr (const(float)* v);

extern(C++) 
float dtVdist (const(float)* v1, const(float)* v2);

extern(C++) 
float dtVdistSqr (const(float)* v1, const(float)* v2);

extern(C++) 
float dtVdist2D (const(float)* v1, const(float)* v2);

extern(C++) 
float dtVdist2DSqr (const(float)* v1, const(float)* v2);

extern(C++) 
void dtVnormalize (float* v);

extern(C++) 
bool dtVequal (const(float)* p0, const(float)* p1);

extern(C++) 
float dtVdot2D (const(float)* u, const(float)* v);

extern(C++) 
float dtVperp2D (const(float)* u, const(float)* v);

extern(C++) 
float dtTriArea2D (const(float)* a, const(float)* b, const(float)* c);

extern(C++) 
bool dtOverlapQuantBounds (const(ushort)* amin, const(ushort)* amax, const(ushort)* bmin, const(ushort)* bmax);

extern(C++) 
bool dtOverlapBounds (const(float)* amin, const(float)* amax, const(float)* bmin, const(float)* bmax);

extern(C++) 
uint dtNextPow2 (uint v);

extern(C++) 
uint dtIlog2 (uint v);

extern(C++) 
int dtAlign4 (int x);

extern(C++) 
int dtOppositeTile (int side);

extern(C++) 
void dtSwapByte (ubyte* a, ubyte* b);

extern(C++) 
void dtSwapEndian (ushort* v);

extern(C++) 
void dtSwapEndian (short* v);

extern(C++) 
void dtSwapEndian (uint* v);

extern(C++) 
void dtSwapEndian (int* v);

extern(C++) 
void dtSwapEndian (float* v);

extern(C++) 
TypeToRetrieveAs* dtGetThenAdvanceBufferPointer (TypeToRetrieveAs)(ref const(ubyte)* buffer, ulong distanceToAdvance);

extern(C++) 
TypeToRetrieveAs* dtGetThenAdvanceBufferPointer (TypeToRetrieveAs)(ref ubyte* buffer, ulong distanceToAdvance);

alias dtStatus = uint;

__gshared static uint DT_FAILURE  = 1U << 31;
__gshared static uint DT_SUCCESS  = 1U << 30;
__gshared static uint DT_IN_PROGRESS  = 1U << 29;
__gshared static uint DT_STATUS_DETAIL_MASK  = 16777215;
__gshared static uint DT_WRONG_MAGIC  = 1 << 0;
__gshared static uint DT_WRONG_VERSION  = 1 << 1;
__gshared static uint DT_OUT_OF_MEMORY  = 1 << 2;
__gshared static uint DT_INVALID_PARAM  = 1 << 3;
__gshared static uint DT_BUFFER_TOO_SMALL  = 1 << 4;
__gshared static uint DT_OUT_OF_NODES  = 1 << 5;
__gshared static uint DT_PARTIAL_RESULT  = 1 << 6;
__gshared static uint DT_ALREADY_OCCUPIED  = 1 << 7;
extern(C++) 
bool dtStatusSucceed (uint status);

extern(C++) 
bool dtStatusFailed (uint status);

extern(C++) 
bool dtStatusInProgress (uint status);

extern(C++) 
bool dtStatusDetail (uint status, uint detail);

alias dtPolyRef = uint;

alias dtTileRef = uint;

__gshared static const int DT_VERTS_PER_POLYGON  = 6;
__gshared static const int DT_NAVMESH_MAGIC  = 'D' << 24 | 'N' << 16 | 'A' << 8 | 'V';
__gshared static const int DT_NAVMESH_VERSION  = 7;
__gshared static const int DT_NAVMESH_STATE_MAGIC  = 'D' << 24 | 'N' << 16 | 'M' << 8 | 'S';
__gshared static const int DT_NAVMESH_STATE_VERSION  = 1;
__gshared static ushort DT_EXT_LINK  = 32768;
__gshared static uint DT_NULL_LINK  = 4294967295U;
__gshared static uint DT_OFFMESH_CON_BIDIR  = 1;
__gshared static const int DT_MAX_AREAS  = 64;
alias dtTileFlags = int;
enum  : dtTileFlags
{
    DT_TILE_FREE_DATA = 1
}

alias dtStraightPathFlags = int;
enum  : dtStraightPathFlags
{
    DT_STRAIGHTPATH_START = 1, 
    DT_STRAIGHTPATH_END = 2, 
    DT_STRAIGHTPATH_OFFMESH_CONNECTION = 4, 
}

alias dtStraightPathOptions = int;
enum  : dtStraightPathOptions
{
    DT_STRAIGHTPATH_AREA_CROSSINGS = 1, 
    DT_STRAIGHTPATH_ALL_CROSSINGS = 2, 
}

alias dtFindPathOptions = int;
enum  : dtFindPathOptions
{
    DT_FINDPATH_ANY_ANGLE = 2
}

alias dtRaycastOptions = int;
enum  : dtRaycastOptions
{
    DT_RAYCAST_USE_COSTS = 1
}

__gshared static const float DT_RAY_CAST_LIMIT_PROPORTIONS  = 50.0f;
alias dtPolyTypes = int;
enum  : dtPolyTypes
{
    DT_POLYTYPE_GROUND = 0, 
    DT_POLYTYPE_OFFMESH_CONNECTION = 1, 
}

extern(C++)
@cppclasssize(32) align(4)
struct dtPoly
{
    @cppsize(4) public uint firstLink;
    @cppsize(12) public ushort[6] verts;
    @cppsize(12) public ushort[6] neis;
    @cppsize(2) public ushort flags;
    @cppsize(1) public ubyte vertCount;
    @cppsize(1) public ubyte areaAndtype;
    /* inline */ public void setArea(ubyte a){
        areaAndtype = (areaAndtype & 192) | (a & 63);
    }

    /* inline */ public void setType(ubyte t){
        areaAndtype = (areaAndtype & 63) | (t << 6);
    }

    /* inline */ public ubyte getArea(){
        return areaAndtype & 63;
    }

    /* inline */ public ubyte getType(){
        return areaAndtype >> 6;
    }

}

extern(C++)
@cppclasssize(12) align(4)
struct dtPolyDetail
{
    @cppsize(4) public uint vertBase;
    @cppsize(4) public uint triBase;
    @cppsize(1) public ubyte vertCount;
    @cppsize(1) public ubyte triCount;
}

extern(C++)
@cppclasssize(12) align(4)
struct dtLink
{
    @cppsize(4) public uint ref_;
    @cppsize(4) public uint next;
    @cppsize(1) public ubyte edge;
    @cppsize(1) public ubyte side;
    @cppsize(1) public ubyte bmin;
    @cppsize(1) public ubyte bmax;
}

extern(C++)
@cppclasssize(16) align(4)
struct dtBVNode
{
    @cppsize(6) public ushort[3] bmin;
    @cppsize(6) public ushort[3] bmax;
    @cppsize(4) public int i;
}

extern(C++)
@cppclasssize(36) align(4)
struct dtOffMeshConnection
{
    @cppsize(24) public float[6] pos;
    @cppsize(4) public float rad;
    @cppsize(2) public ushort poly;
    @cppsize(1) public ubyte flags;
    @cppsize(1) public ubyte side;
    @cppsize(4) public uint userId;
}

extern(C++)
@cppclasssize(100) align(4)
struct dtMeshHeader
{
    @cppsize(4) public int magic;
    @cppsize(4) public int version_;
    @cppsize(4) public int x;
    @cppsize(4) public int y;
    @cppsize(4) public int layer;
    @cppsize(4) public uint userId;
    @cppsize(4) public int polyCount;
    @cppsize(4) public int vertCount;
    @cppsize(4) public int maxLinkCount;
    @cppsize(4) public int detailMeshCount;
    @cppsize(4) public int detailVertCount;
    @cppsize(4) public int detailTriCount;
    @cppsize(4) public int bvNodeCount;
    @cppsize(4) public int offMeshConCount;
    @cppsize(4) public int offMeshBase;
    @cppsize(4) public float walkableHeight;
    @cppsize(4) public float walkableRadius;
    @cppsize(4) public float walkableClimb;
    @cppsize(12) public float[3] bmin;
    @cppsize(12) public float[3] bmax;
    @cppsize(4) public float bvQuantFactor;
}

extern(C++)
@cppclasssize(104) align(8)
struct dtMeshTile
{
    @cppsize(4) public uint salt;
    @cppsize(4) public uint linksFreeList;
    @cppsize(8) public dtMeshHeader* header;
    @cppsize(8) public dtPoly* polys;
    @cppsize(8) public float* verts;
    @cppsize(8) public dtLink* links;
    @cppsize(8) public dtPolyDetail* detailMeshes;
    @cppsize(8) public float* detailVerts;
    @cppsize(8) public ubyte* detailTris;
    @cppsize(8) public dtBVNode* bvTree;
    @cppsize(8) public dtOffMeshConnection* offMeshCons;
    @cppsize(8) public ubyte* data;
    @cppsize(4) public int dataSize;
    @cppsize(4) public int flags;
    @cppsize(8) public dtMeshTile* next;
    private this(ref dtMeshTile );

    private ref dtMeshTile opAssign(ref dtMeshTile );

}

extern(C++)
@cppclasssize(28) align(4)
struct dtNavMeshParams
{
    @cppsize(12) public float[3] orig;
    @cppsize(4) public float tileWidth;
    @cppsize(4) public float tileHeight;
    @cppsize(4) public int maxTiles;
    @cppsize(4) public int maxPolys;
}

extern(C++)
@cppclasssize(104) align(8)
class dtNavMesh
{
    @cppsize(28) private dtNavMeshParams m_params;
    @cppsize(12) private float[3] m_orig;
    @cppsize(4) private float m_tileWidth;
    @cppsize(4) private float m_tileHeight;
    @cppsize(4) private int m_maxTiles;
    @cppsize(4) private int m_tileLutSize;
    @cppsize(4) private int m_tileLutMask;
    @cppsize(8) private dtMeshTile** m_posLookup;
    @cppsize(8) private dtMeshTile* m_nextFree;
    @cppsize(8) private dtMeshTile* m_tiles;
    @cppsize(4) private uint m_saltBits;
    @cppsize(4) private uint m_tileBits;
    @cppsize(4) private uint m_polyBits;
    public final this();

    public final ~this();

    public final uint init(const(dtNavMeshParams)* params);

    public final uint init(ubyte* data, const int dataSize, const int flags);

    public final const(dtNavMeshParams)* getParams();

    public final uint addTile(ubyte* data, int dataSize, int flags, uint lastRef, uint* result);

    public final uint removeTile(uint ref_, ubyte** data, int* dataSize);

    public final void calcTileLoc(const(float)* pos, int* tx, int* ty);

    public final const(dtMeshTile)* getTileAt(const int x, const int y, const int layer);

    public final int getTilesAt(const int x, const int y, const(dtMeshTile)** tiles, const int maxTiles);

    public final uint getTileRefAt(int x, int y, int layer);

    public final uint getTileRef(const(dtMeshTile)* tile);

    public final const(dtMeshTile)* getTileByRef(uint ref_);

    public final int getMaxTiles();

    public final const(dtMeshTile)* getTile(int i);

    public final uint getTileAndPolyByRef(uint ref_, const(dtMeshTile)** tile, const(dtPoly)** poly);

    public final void getTileAndPolyByRefUnsafe(uint ref_, const(dtMeshTile)** tile, const(dtPoly)** poly);

    public final bool isValidPolyRef(uint ref_);

    public final uint getPolyRefBase(const(dtMeshTile)* tile);

    public final uint getOffMeshConnectionPolyEndPoints(uint prevRef, uint polyRef, float* startPos, float* endPos);

    public final const(dtOffMeshConnection)* getOffMeshConnectionByRef(uint ref_);

    public final uint setPolyFlags(uint ref_, ushort flags);

    public final uint getPolyFlags(uint ref_, ushort* resultFlags);

    public final uint setPolyArea(uint ref_, ubyte area);

    public final uint getPolyArea(uint ref_, ubyte* resultArea);

    public final int getTileStateSize(const(dtMeshTile)* tile);

    public final uint storeTileState(const(dtMeshTile)* tile, ubyte* data, const int maxDataSize);

    public final uint restoreTileState(dtMeshTile* tile, const(ubyte)* data, const int maxDataSize);

    /* inline */ public final uint encodePolyId(uint salt, uint it, uint ip){
        return ((dtPolyRef)salt << (m_polyBits + m_tileBits)) | ((dtPolyRef)it << m_polyBits) | (dtPolyRef)ip;
    }

    /* inline */ public final void decodePolyId(uint ref_, ref uint salt, ref uint it, ref uint ip){
        const dtPolyRef saltMask = ((dtPolyRef)1 << m_saltBits) - 1;
        const dtPolyRef tileMask = ((dtPolyRef)1 << m_tileBits) - 1;
        const dtPolyRef polyMask = ((dtPolyRef)1 << m_polyBits) - 1;
        salt = (unsigned int)((ref >> (m_polyBits + m_tileBits)) & saltMask);
        it = (unsigned int)((ref >> m_polyBits) & tileMask);
        ip = (unsigned int)(ref & polyMask);
    }

    /* inline */ public final uint decodePolyIdSalt(uint ref_){
        const dtPolyRef saltMask = ((dtPolyRef)1 << m_saltBits) - 1;
        return (unsigned int)((ref >> (m_polyBits + m_tileBits)) & saltMask);
    }

    /* inline */ public final uint decodePolyIdTile(uint ref_){
        const dtPolyRef tileMask = ((dtPolyRef)1 << m_tileBits) - 1;
        return (unsigned int)((ref >> m_polyBits) & tileMask);
    }

    /* inline */ public final uint decodePolyIdPoly(uint ref_){
        const dtPolyRef polyMask = ((dtPolyRef)1 << m_polyBits) - 1;
        return (unsigned int)(ref & polyMask);
    }

    private final this(ref dtNavMesh );

    private final ref dtNavMesh opAssign(ref dtNavMesh );

    private final dtMeshTile* getTile(int i);

    private final int getTilesAt(const int x, const int y, dtMeshTile** tiles, const int maxTiles);

    private final int getNeighbourTilesAt(const int x, const int y, const int side, dtMeshTile** tiles, const int maxTiles);

    private final int findConnectingPolys(const(float)* va, const(float)* vb, const(dtMeshTile)* tile, int side, uint* con, float* conarea, int maxcon);

    private final void connectIntLinks(dtMeshTile* tile);

    private final void baseOffMeshLinks(dtMeshTile* tile);

    private final void connectExtLinks(dtMeshTile* tile, dtMeshTile* target, int side);

    private final void connectExtOffMeshLinks(dtMeshTile* tile, dtMeshTile* target, int side);

    private final void unconnectLinks(dtMeshTile* tile, dtMeshTile* target);

    private final int queryPolygonsInTile(const(dtMeshTile)* tile, const(float)* qmin, const(float)* qmax, uint* polys, const int maxPolys);

    private final uint findNearestPolyInTile(const(dtMeshTile)* tile, const(float)* center, const(float)* halfExtents, float* nearestPt);

    private final void closestPointOnPoly(uint ref_, const(float)* pos, float* closest, bool* posOverPoly);

}

extern(C++)
@cppclasssize(208) align(8)
struct dtNavMeshCreateParams
{
    @cppsize(8) public const(ushort)* verts;
    @cppsize(4) public int vertCount;
    @cppsize(8) public const(ushort)* polys;
    @cppsize(8) public const(ushort)* polyFlags;
    @cppsize(8) public const(ubyte)* polyAreas;
    @cppsize(4) public int polyCount;
    @cppsize(4) public int nvp;
    @cppsize(8) public const(uint)* detailMeshes;
    @cppsize(8) public const(float)* detailVerts;
    @cppsize(4) public int detailVertsCount;
    @cppsize(8) public const(ubyte)* detailTris;
    @cppsize(4) public int detailTriCount;
    @cppsize(8) public const(float)* offMeshConVerts;
    @cppsize(8) public const(float)* offMeshConRad;
    @cppsize(8) public const(ushort)* offMeshConFlags;
    @cppsize(8) public const(ubyte)* offMeshConAreas;
    @cppsize(8) public const(ubyte)* offMeshConDir;
    @cppsize(8) public const(uint)* offMeshConUserID;
    @cppsize(4) public int offMeshConCount;
    @cppsize(4) public uint userId;
    @cppsize(4) public int tileX;
    @cppsize(4) public int tileY;
    @cppsize(4) public int tileLayer;
    @cppsize(12) public float[3] bmin;
    @cppsize(12) public float[3] bmax;
    @cppsize(4) public float walkableHeight;
    @cppsize(4) public float walkableRadius;
    @cppsize(4) public float walkableClimb;
    @cppsize(4) public float cs;
    @cppsize(4) public float ch;
    @cppsize(1) public bool buildBvTree;
}










extern(C++)
@cppclasssize(260) align(4)
class dtQueryFilter
{
    @cppsize(256) private float[64] m_areaCost;
    @cppsize(2) private ushort m_includeFlags;
    @cppsize(2) private ushort m_excludeFlags;
    public final this();

    public final bool passFilter(uint ref_, const(dtMeshTile)* tile, const(dtPoly)* poly);

    public final float getCost(const(float)* pa, const(float)* pb, uint prevRef, const(dtMeshTile)* prevTile, const(dtPoly)* prevPoly, uint curRef, const(dtMeshTile)* curTile, const(dtPoly)* curPoly, uint nextRef, const(dtMeshTile)* nextTile, const(dtPoly)* nextPoly);

    /* inline */ public final float getAreaCost(const int i){
        return m_areaCost[i];
    }

    /* inline */ public final void setAreaCost(const int i, const float cost){
        m_areaCost[i] = cost;
    }

    /* inline */ public final ushort getIncludeFlags(){
        return m_includeFlags;
    }

    /* inline */ public final void setIncludeFlags(ushort flags){
        m_includeFlags = flags;
    }

    /* inline */ public final ushort getExcludeFlags(){
        return m_excludeFlags;
    }

    /* inline */ public final void setExcludeFlags(ushort flags){
        m_excludeFlags = flags;
    }

}

extern(C++)
@cppclasssize(48) align(8)
struct dtRaycastHit
{
    @cppsize(4) public float t;
    @cppsize(12) public float[3] hitNormal;
    @cppsize(4) public int hitEdgeIndex;
    @cppsize(8) public uint* path;
    @cppsize(4) public int pathCount;
    @cppsize(4) public int maxPath;
    @cppsize(4) public float pathCost;
}

extern(C++)
@cppclasssize(8) align(8)
class dtPolyQuery
{
    /* inline */ public ~this(){
    }

    public void process(const(dtMeshTile)* tile, dtPoly** polys, uint* refs, int count);

    public final ref dtPolyQuery opAssign(ref dtPolyQuery );

}

extern(C++)
@cppclasssize(104) align(8)
class dtNavMeshQuery
{
    extern(C++)
    @cppclasssize(72) align(8)
    struct dtQueryData
    {
        @cppsize(4) public uint status;
        @cppsize(8) public dtNode* lastBestNode;
        @cppsize(4) public float lastBestNodeCost;
        @cppsize(4) public uint startRef;
        @cppsize(4) public uint endRef;
        @cppsize(12) public float[3] startPos;
        @cppsize(12) public float[3] endPos;
        @cppsize(8) public const(dtQueryFilter)* filter;
        @cppsize(4) public uint options;
        @cppsize(4) public float raycastLimitSqr;
    }

    @cppsize(8) private const(dtNavMesh)* m_nav;
    @cppsize(72) private dtQueryData m_query;
    @cppsize(8) private dtNodePool* m_tinyNodePool;
    @cppsize(8) private dtNodePool* m_nodePool;
    @cppsize(8) private dtNodeQueue* m_openList;
    public final this();

    public final ~this();

    public final uint init(const(dtNavMesh)* nav, const int maxNodes);

    public final uint findPath(uint startRef, uint endRef, const(float)* startPos, const(float)* endPos, const(dtQueryFilter)* filter, uint* path, int* pathCount, const int maxPath);

    public final uint findStraightPath(const(float)* startPos, const(float)* endPos, const(uint)* path, const int pathSize, float* straightPath, ubyte* straightPathFlags, uint* straightPathRefs, int* straightPathCount, const int maxStraightPath, const int options);

    public final uint initSlicedFindPath(uint startRef, uint endRef, const(float)* startPos, const(float)* endPos, const(dtQueryFilter)* filter, uint options);

    public final uint updateSlicedFindPath(const int maxIter, int* doneIters);

    public final uint finalizeSlicedFindPath(uint* path, int* pathCount, const int maxPath);

    public final uint finalizeSlicedFindPathPartial(const(uint)* existing, const int existingSize, uint* path, int* pathCount, const int maxPath);

    public final uint findPolysAroundCircle(uint startRef, const(float)* centerPos, const float radius, const(dtQueryFilter)* filter, uint* resultRef, uint* resultParent, float* resultCost, int* resultCount, const int maxResult);

    public final uint findPolysAroundShape(uint startRef, const(float)* verts, const int nverts, const(dtQueryFilter)* filter, uint* resultRef, uint* resultParent, float* resultCost, int* resultCount, const int maxResult);

    public final uint getPathFromDijkstraSearch(uint endRef, uint* path, int* pathCount, int maxPath);

    public final uint findNearestPoly(const(float)* center, const(float)* halfExtents, const(dtQueryFilter)* filter, uint* nearestRef, float* nearestPt);

    public final uint queryPolygons(const(float)* center, const(float)* halfExtents, const(dtQueryFilter)* filter, uint* polys, int* polyCount, const int maxPolys);

    public final uint queryPolygons(const(float)* center, const(float)* halfExtents, const(dtQueryFilter)* filter, dtPolyQuery* query);

    public final uint findLocalNeighbourhood(uint startRef, const(float)* centerPos, const float radius, const(dtQueryFilter)* filter, uint* resultRef, uint* resultParent, int* resultCount, const int maxResult);

    public final uint moveAlongSurface(uint startRef, const(float)* startPos, const(float)* endPos, const(dtQueryFilter)* filter, float* resultPos, uint* visited, int* visitedCount, const int maxVisitedSize);

    public final uint raycast(uint startRef, const(float)* startPos, const(float)* endPos, const(dtQueryFilter)* filter, float* t, float* hitNormal, uint* path, int* pathCount, const int maxPath);

    public final uint raycast(uint startRef, const(float)* startPos, const(float)* endPos, const(dtQueryFilter)* filter, uint options, dtRaycastHit* hit, uint prevRef);

    public final uint findDistanceToWall(uint startRef, const(float)* centerPos, const float maxRadius, const(dtQueryFilter)* filter, float* hitDist, float* hitPos, float* hitNormal);

    public final uint getPolyWallSegments(uint ref_, const(dtQueryFilter)* filter, float* segmentVerts, uint* segmentRefs, int* segmentCount, const int maxSegments);

    public final uint findRandomPoint(const(dtQueryFilter)* filter, float function() frand, uint* randomRef, float* randomPt);

    public final uint findRandomPointAroundCircle(uint startRef, const(float)* centerPos, const float maxRadius, const(dtQueryFilter)* filter, float function() frand, uint* randomRef, float* randomPt);

    public final uint closestPointOnPoly(uint ref_, const(float)* pos, float* closest, bool* posOverPoly);

    public final uint closestPointOnPolyBoundary(uint ref_, const(float)* pos, float* closest);

    public final uint getPolyHeight(uint ref_, const(float)* pos, float* height);

    public final bool isValidPolyRef(uint ref_, const(dtQueryFilter)* filter);

    public final bool isInClosedList(uint ref_);

    /* inline */ public final dtNodePool* getNodePool(){
        return m_nodePool;
    }

    /* inline */ public final const(dtNavMesh)* getAttachedNavMesh(){
        return m_nav;
    }

    private final this(ref dtNavMeshQuery );

    private final ref dtNavMeshQuery opAssign(ref dtNavMeshQuery );

    private final void queryPolygonsInTile(const(dtMeshTile)* tile, const(float)* qmin, const(float)* qmax, const(dtQueryFilter)* filter, dtPolyQuery* query);

    private final uint getPortalPoints(uint from, uint to, float* left, float* right, ref ubyte fromType, ref ubyte toType);

    private final uint getPortalPoints(uint from, const(dtPoly)* fromPoly, const(dtMeshTile)* fromTile, uint to, const(dtPoly)* toPoly, const(dtMeshTile)* toTile, float* left, float* right);

    private final uint getEdgeMidPoint(uint from, uint to, float* mid);

    private final uint getEdgeMidPoint(uint from, const(dtPoly)* fromPoly, const(dtMeshTile)* fromTile, uint to, const(dtPoly)* toPoly, const(dtMeshTile)* toTile, float* mid);

    private final uint appendVertex(const(float)* pos, ubyte flags, uint ref_, float* straightPath, ubyte* straightPathFlags, uint* straightPathRefs, int* straightPathCount, const int maxStraightPath);

    private final uint appendPortals(const int startIdx, const int endIdx, const(float)* endPos, const(uint)* path, float* straightPath, ubyte* straightPathFlags, uint* straightPathRefs, int* straightPathCount, const int maxStraightPath, const int options);

    private final uint getPathToNode(dtNode* endNode, uint* path, int* pathCount, int maxPath);

}










alias dtNodeFlags = int;
enum  : dtNodeFlags
{
    DT_NODE_OPEN = 1, 
    DT_NODE_CLOSED = 2, 
    DT_NODE_PARENT_DETACHED = 4, 
}

alias dtNodeIndex = ushort;

__gshared static ushort DT_NULL_IDX  = (dtNodeIndex)~0;
__gshared static const int DT_NODE_PARENT_BITS  = 24;
__gshared static const int DT_NODE_STATE_BITS  = 2;
extern(C++)
@cppclasssize(28) align(4)
struct dtNode
{
    @cppsize(12) public float[3] pos;
    @cppsize(4) public float cost;
    @cppsize(4) public float total;
    mixin(bitfields!(
    uint, "pidx", DT_NODE_PARENT_BITS,
    uint, "state", DT_NODE_STATE_BITS,
    uint, "flags", 3,
    uint, "", 3));
    @cppsize(4) public uint id;
}

__gshared static const int DT_MAX_STATES_PER_NODE  = 1 << DT_NODE_STATE_BITS;
extern(C++)
@cppclasssize(40) align(8)
class dtNodePool
{
    @cppsize(8) private dtNode* m_nodes;
    @cppsize(8) private ushort* m_first;
    @cppsize(8) private ushort* m_next;
    @cppsize(4) private const int m_maxNodes;
    @cppsize(4) private const int m_hashSize;
    @cppsize(4) private int m_nodeCount;
    public final this(int maxNodes, int hashSize);

    public final ~this();

    public final void clear();

    public final dtNode* getNode(uint id, ubyte state);

    public final dtNode* findNode(uint id, ubyte state);

    public final uint findNodes(uint id, dtNode** nodes, const int maxNodes);

    /* inline */ public final uint getNodeIdx(const(dtNode)* node){
        if (!node)
            return 0;
        return (unsigned int)(node - m_nodes) + 1;
    }

    /* inline */ public final dtNode* getNodeAtIdx(uint idx){
        if (!idx)
            return 0;
        return &m_nodes[idx - 1];
    }

    /* inline */ public final const(dtNode)* getNodeAtIdx(uint idx){
        if (!idx)
            return 0;
        return &m_nodes[idx - 1];
    }

    /* inline */ public final int getMemUsed(){
        return sizeof (*this) + sizeof(dtNode) * m_maxNodes + sizeof(dtNodeIndex) * m_maxNodes + sizeof(dtNodeIndex) * m_hashSize;
    }

    /* inline */ public final int getMaxNodes(){
        return m_maxNodes;
    }

    /* inline */ public final int getHashSize(){
        return m_hashSize;
    }

    /* inline */ public final ushort getFirst(int bucket){
        return m_first[bucket];
    }

    /* inline */ public final ushort getNext(int i){
        return m_next[i];
    }

    /* inline */ public final int getNodeCount(){
        return m_nodeCount;
    }

    private final this(ref dtNodePool );

    private final ref dtNodePool opAssign(ref dtNodePool );

}

extern(C++)
@cppclasssize(16) align(8)
class dtNodeQueue
{
    @cppsize(8) private dtNode** m_heap;
    @cppsize(4) private const int m_capacity;
    @cppsize(4) private int m_size;
    public final this(int n);

    public final ~this();

    /* inline */ public final void clear(){
        m_size = 0;
    }

    /* inline */ public final dtNode* top(){
        return m_heap[0];
    }

    /* inline */ public final dtNode* pop(){
        dtNode *result = m_heap[0];
        m_size--;
        trickleDown(0, m_heap[m_size]);
        return result;
    }

    /* inline */ public final void push(dtNode* node){
        m_size++;
        bubbleUp(m_size - 1, node);
    }

    /* inline */ public final void modify(dtNode* node){
        for (int i = 0; i < m_size; ++i) {
            if (m_heap[i] == node) {
                bubbleUp(i, node);
                return;
            }
        }
    }

    /* inline */ public final bool empty(){
        return m_size == 0;
    }

    /* inline */ public final int getMemUsed(){
        return sizeof (*this) + sizeof(dtNode *) * (m_capacity + 1);
    }

    /* inline */ public final int getCapacity(){
        return m_capacity;
    }

    private final this(ref dtNodeQueue );

    private final ref dtNodeQueue opAssign(ref dtNodeQueue );

    private final void bubbleUp(int i, dtNode* node);

    private final void trickleDown(int i, dtNode* node);

}

__gshared static const float RC_PI  = 3.14159274f;
alias rcLogCategory = int;
enum  : rcLogCategory
{
    RC_LOG_PROGRESS = 1, 
    RC_LOG_WARNING = 2, 
    RC_LOG_ERROR = 3, 
}

alias rcTimerLabel = int;
enum  : rcTimerLabel
{
    RC_TIMER_TOTAL = 0, 
    RC_TIMER_TEMP = 1, 
    RC_TIMER_RASTERIZE_TRIANGLES = 2, 
    RC_TIMER_BUILD_COMPACTHEIGHTFIELD = 3, 
    RC_TIMER_BUILD_CONTOURS = 4, 
    RC_TIMER_BUILD_CONTOURS_TRACE = 5, 
    RC_TIMER_BUILD_CONTOURS_SIMPLIFY = 6, 
    RC_TIMER_FILTER_BORDER = 7, 
    RC_TIMER_FILTER_WALKABLE = 8, 
    RC_TIMER_MEDIAN_AREA = 9, 
    RC_TIMER_FILTER_LOW_OBSTACLES = 10, 
    RC_TIMER_BUILD_POLYMESH = 11, 
    RC_TIMER_MERGE_POLYMESH = 12, 
    RC_TIMER_ERODE_AREA = 13, 
    RC_TIMER_MARK_BOX_AREA = 14, 
    RC_TIMER_MARK_CYLINDER_AREA = 15, 
    RC_TIMER_MARK_CONVEXPOLY_AREA = 16, 
    RC_TIMER_BUILD_DISTANCEFIELD = 17, 
    RC_TIMER_BUILD_DISTANCEFIELD_DIST = 18, 
    RC_TIMER_BUILD_DISTANCEFIELD_BLUR = 19, 
    RC_TIMER_BUILD_REGIONS = 20, 
    RC_TIMER_BUILD_REGIONS_WATERSHED = 21, 
    RC_TIMER_BUILD_REGIONS_EXPAND = 22, 
    RC_TIMER_BUILD_REGIONS_FLOOD = 23, 
    RC_TIMER_BUILD_REGIONS_FILTER = 24, 
    RC_TIMER_BUILD_LAYERS = 25, 
    RC_TIMER_BUILD_POLYMESHDETAIL = 26, 
    RC_TIMER_MERGE_POLYMESHDETAIL = 27, 
    RC_MAX_TIMERS = 28, 
}

extern(C++)
@cppclasssize(16) align(8)
class rcContext
{
    @cppsize(1) protected bool m_logEnabled;
    @cppsize(1) protected bool m_timerEnabled;
    /* inline */ public final this(bool state){
    }

    /* inline */ public ~this(){
    }

    /* inline */ public final void enableLog(bool state){
        m_logEnabled = state;
    }

    /* inline */ public final void resetLog(){
        if (m_logEnabled)
            doResetLog();
    }

    public final void log(rcLogCategory category, const(byte)* format);

    /* inline */ public final void enableTimer(bool state){
        m_timerEnabled = state;
    }

    /* inline */ public final void resetTimers(){
        if (m_timerEnabled)
            doResetTimers();
    }

    /* inline */ public final void startTimer(rcTimerLabel label){
        if (m_timerEnabled)
            doStartTimer(label);
    }

    /* inline */ public final void stopTimer(rcTimerLabel label){
        if (m_timerEnabled)
            doStopTimer(label);
    }

    /* inline */ public final int getAccumulatedTime(rcTimerLabel label){
        return m_timerEnabled ? doGetAccumulatedTime(label) : -1;
    }

    /* inline */ protected void doResetLog(){
    }

    /* inline */ protected void doLog(rcLogCategory , const(byte)* , const int ){
    }

    /* inline */ protected void doResetTimers(){
    }

    /* inline */ protected void doStartTimer(rcTimerLabel ){
    }

    /* inline */ protected void doStopTimer(rcTimerLabel ){
    }

    /* inline */ protected int doGetAccumulatedTime(rcTimerLabel ){
        return -1;
    }

    public final ref rcContext opAssign(ref rcContext );

}

extern(C++)
@cppclasssize(16) align(8)
class rcScopedTimer
{
    @cppsize(8) private const(rcContext)* m_ctx;
    @cppsize(0) private rcTimerLabel m_label;
    /* inline */ public final this(rcContext* ctx, rcTimerLabel label){
        m_ctx.startTimer(m_label);
    }

    /* inline */ public final ~this(){
        m_ctx.stopTimer(m_label);
    }

    private final this(ref rcScopedTimer );

    private final ref rcScopedTimer opAssign(ref rcScopedTimer );

}

extern(C++)
@cppclasssize(92) align(4)
struct rcConfig
{
    @cppsize(4) public int width;
    @cppsize(4) public int height;
    @cppsize(4) public int tileSize;
    @cppsize(4) public int borderSize;
    @cppsize(4) public float cs;
    @cppsize(4) public float ch;
    @cppsize(12) public float[3] bmin;
    @cppsize(12) public float[3] bmax;
    @cppsize(4) public float walkableSlopeAngle;
    @cppsize(4) public int walkableHeight;
    @cppsize(4) public int walkableClimb;
    @cppsize(4) public int walkableRadius;
    @cppsize(4) public int maxEdgeLen;
    @cppsize(4) public float maxSimplificationError;
    @cppsize(4) public int minRegionArea;
    @cppsize(4) public int mergeRegionArea;
    @cppsize(4) public int maxVertsPerPoly;
    @cppsize(4) public float detailSampleDist;
    @cppsize(4) public float detailSampleMaxError;
}

__gshared static const int RC_SPAN_HEIGHT_BITS  = 13;
__gshared static const int RC_SPAN_MAX_HEIGHT  = (1 << RC_SPAN_HEIGHT_BITS) - 1;
__gshared static const int RC_SPANS_PER_POOL  = 2048;
extern(C++)
@cppclasssize(16) align(8)
struct rcSpan
{
    mixin(bitfields!(
    uint, "smin", RC_SPAN_HEIGHT_BITS,
    uint, "smax", RC_SPAN_HEIGHT_BITS,
    uint, "area", 6,
    ));
    @cppsize(8) public rcSpan* next;
}

extern(C++)
@cppclasssize(32776) align(8)
struct rcSpanPool
{
    @cppsize(8) public rcSpanPool* next;
    @cppsize(32768) public rcSpan[2048] items;
}

extern(C++)
@cppclasssize(64) align(8)
struct rcHeightfield
{
    @cppsize(4) public int width;
    @cppsize(4) public int height;
    @cppsize(12) public float[3] bmin;
    @cppsize(12) public float[3] bmax;
    @cppsize(4) public float cs;
    @cppsize(4) public float ch;
    @cppsize(8) public rcSpan** spans;
    @cppsize(8) public rcSpanPool* pools;
    @cppsize(8) public rcSpan* freelist;
    // public this();

    public ~this();

    private this(ref rcHeightfield );

    private ref rcHeightfield opAssign(ref rcHeightfield );

}

extern(C++)
@cppclasssize(4) align(4)
struct rcCompactCell
{
    mixin(bitfields!(
    uint, "index", 24,
    uint, "count", 8));
}

extern(C++)
@cppclasssize(8) align(4)
struct rcCompactSpan
{
    @cppsize(2) public ushort y;
    @cppsize(2) public ushort reg;
    mixin(bitfields!(
    uint, "con", 24,
    uint, "h", 8));
}

extern(C++)
@cppclasssize(96) align(8)
struct rcCompactHeightfield
{
    @cppsize(4) public int width;
    @cppsize(4) public int height;
    @cppsize(4) public int spanCount;
    @cppsize(4) public int walkableHeight;
    @cppsize(4) public int walkableClimb;
    @cppsize(4) public int borderSize;
    @cppsize(2) public ushort maxDistance;
    @cppsize(2) public ushort maxRegions;
    @cppsize(12) public float[3] bmin;
    @cppsize(12) public float[3] bmax;
    @cppsize(4) public float cs;
    @cppsize(4) public float ch;
    @cppsize(8) public rcCompactCell* cells;
    @cppsize(8) public rcCompactSpan* spans;
    @cppsize(8) public ushort* dist;
    @cppsize(8) public ubyte* areas;
    // public this();

    public ~this();

}

extern(C++)
@cppclasssize(88) align(8)
struct rcHeightfieldLayer
{
    @cppsize(12) public float[3] bmin;
    @cppsize(12) public float[3] bmax;
    @cppsize(4) public float cs;
    @cppsize(4) public float ch;
    @cppsize(4) public int width;
    @cppsize(4) public int height;
    @cppsize(4) public int minx;
    @cppsize(4) public int maxx;
    @cppsize(4) public int miny;
    @cppsize(4) public int maxy;
    @cppsize(4) public int hmin;
    @cppsize(4) public int hmax;
    @cppsize(8) public ubyte* heights;
    @cppsize(8) public ubyte* areas;
    @cppsize(8) public ubyte* cons;
}

extern(C++)
@cppclasssize(16) align(8)
struct rcHeightfieldLayerSet
{
    @cppsize(8) public rcHeightfieldLayer* layers;
    @cppsize(4) public int nlayers;
    // public this();

    public ~this();

}

extern(C++)
@cppclasssize(32) align(8)
struct rcContour
{
    @cppsize(8) public int* verts;
    @cppsize(4) public int nverts;
    @cppsize(8) public int* rverts;
    @cppsize(4) public int nrverts;
    @cppsize(2) public ushort reg;
    @cppsize(1) public ubyte area;
}

extern(C++)
@cppclasssize(64) align(8)
struct rcContourSet
{
    @cppsize(8) public rcContour* conts;
    @cppsize(4) public int nconts;
    @cppsize(12) public float[3] bmin;
    @cppsize(12) public float[3] bmax;
    @cppsize(4) public float cs;
    @cppsize(4) public float ch;
    @cppsize(4) public int width;
    @cppsize(4) public int height;
    @cppsize(4) public int borderSize;
    @cppsize(4) public float maxError;
    // public this();

    public ~this();

}

extern(C++)
@cppclasssize(96) align(8)
struct rcPolyMesh
{
    @cppsize(8) public ushort* verts;
    @cppsize(8) public ushort* polys;
    @cppsize(8) public ushort* regs;
    @cppsize(8) public ushort* flags;
    @cppsize(8) public ubyte* areas;
    @cppsize(4) public int nverts;
    @cppsize(4) public int npolys;
    @cppsize(4) public int maxpolys;
    @cppsize(4) public int nvp;
    @cppsize(12) public float[3] bmin;
    @cppsize(12) public float[3] bmax;
    @cppsize(4) public float cs;
    @cppsize(4) public float ch;
    @cppsize(4) public int borderSize;
    @cppsize(4) public float maxEdgeError;
    // public this();

    public ~this();

}

extern(C++)
@cppclasssize(40) align(8)
struct rcPolyMeshDetail
{
    @cppsize(8) public uint* meshes;
    @cppsize(8) public float* verts;
    @cppsize(8) public ubyte* tris;
    @cppsize(4) public int nmeshes;
    @cppsize(4) public int nverts;
    @cppsize(4) public int ntris;
}

__gshared static ushort RC_BORDER_REG  = 32768;
__gshared static ushort RC_MULTIPLE_REGS  = 0;
__gshared static const int RC_BORDER_VERTEX  = 65536;
__gshared static const int RC_AREA_BORDER  = 131072;
alias rcBuildContoursFlags = int;
enum  : rcBuildContoursFlags
{
    RC_CONTOUR_TESS_WALL_EDGES = 1, 
    RC_CONTOUR_TESS_AREA_EDGES = 2, 
}

__gshared static const int RC_CONTOUR_REG_MASK  = 65535;
__gshared static ushort RC_MESH_NULL_IDX  = 65535;
__gshared static ubyte RC_NULL_AREA  = 0;
__gshared static ubyte RC_WALKABLE_AREA  = 63;
__gshared static const int RC_NOT_CONNECTED  = 63;
extern(C++) 
void rcIgnoreUnused (T)(ref const T );

extern(C++) 
void rcSwap (T)(ref T a, ref T b);

extern(C++) 
T rcMin (T)(T a, T b);

extern(C++) 
T rcMax (T)(T a, T b);

extern(C++) 
T rcAbs (T)(T a);

extern(C++) 
T rcSqr (T)(T a);

extern(C++) 
T rcClamp (T)(T v, T mn, T mx);

extern(C++) 
void rcVcross (float* dest, const(float)* v1, const(float)* v2);

extern(C++) 
float rcVdot (const(float)* v1, const(float)* v2);

extern(C++) 
void rcVmad (float* dest, const(float)* v1, const(float)* v2, const float s);

extern(C++) 
void rcVadd (float* dest, const(float)* v1, const(float)* v2);

extern(C++) 
void rcVsub (float* dest, const(float)* v1, const(float)* v2);

extern(C++) 
void rcVmin (float* mn, const(float)* v);

extern(C++) 
void rcVmax (float* mx, const(float)* v);

extern(C++) 
void rcVcopy (float* dest, const(float)* v);

extern(C++) 
float rcVdist (const(float)* v1, const(float)* v2);

extern(C++) 
float rcVdistSqr (const(float)* v1, const(float)* v2);

extern(C++) 
void rcVnormalize (float* v);

extern(C++) 
void rcSetCon (ref rcCompactSpan s, int dir, int i);

extern(C++) 
int rcGetCon (ref rcCompactSpan s, int dir);

extern(C++) 
int rcGetDirOffsetX (int dir);

extern(C++) 
int rcGetDirOffsetY (int dir);

extern(C++) 
int rcGetDirForOffset (int x, int y);

alias rcAssertFailFunc = extern(C++) void function(const(byte)*, const(byte)*, int);

alias rcAllocHint = int;
enum  : rcAllocHint
{
    RC_ALLOC_PERM = 0, 
    RC_ALLOC_TEMP = 1, 
}

alias rcAllocFunc = extern(C++) void* function(ulong, rcAllocHint);

alias rcFreeFunc = extern(C++) void function(void*);

extern(C++)
@cppclasssize(1) align(1)
struct rcNewTag
{
    // public this();

    public this(ref rcNewTag );

    public this(ref rcNewTag );

    public ~this();

}

alias rcSizeType = long;

extern(C++)
class rcVectorBase(T, H)
{
    @cppsize(8) private long m_size;
    @cppsize(8) private long m_cap;
    @cppsize(8) private T* m_data;
    /* inline */ private static final void construct(T* p, ref const T v){
        .new (rcNewTag(), (void *)p) T((v));
    }

    /* inline */ private static final void construct(T* p){
        .new (rcNewTag(), (void *)p) T;
    }

    private static final void construct_range(T* begin, T* end);

    private static final void construct_range(T* begin, T* end, ref const T value);

    private static final void copy_range(T* dst, const(T)* begin, const(T)* end);

    private final void destroy_range(long begin, long end);

    private final T* allocate_and_copy(long size);

    private final void resize_impl(long size, const(T)* value);

    /* inline */ public final this(){
    }

    /* inline */ public final this(ref rcVectorBase!(T, H) other);
    /* inline */ public final this(long count);
    /* inline */ public final this(long count, ref const T value){
        resize(count, value);
    }

    /* inline */ public final this(const(T)* begin, const(T)* end);
    /* inline */ public final ~this(){
        destroy_range(0, m_size);
        rcFree(m_data);
    }

    public final bool reserve(long size);

    /* inline */ public final void assign(long count, ref const T value);
    public final void assign(const(T)* begin, const(T)* end);

    /* inline */ public final void resize(long size){
        resize_impl(size, 0);
    }

    /* inline */ public final void resize(long size, ref const T value){
        resize_impl(size, &value);
    }

    /* inline */ public final void clear();
    public final void push_back(ref const T value);

    /* inline */ public final void pop_back(){
        {
            rcAssertFailFunc *failFunc = rcAssertFailGetCustom();
            if (failFunc == 0) {
                (void)((!!(m_size > 0)) || (_wassert(L"m_size > 0", L"C:\\recastnav\\RecastAlloc.h", (unsigned int)(131)) , 0));
            } else if (!(m_size > 0)) {
                (*failFunc)("m_size > 0", "C:\\recastnav\\RecastAlloc.h", 131);
            }
        }
        ;
        back().~T();
        m_size--;
    }

    /* inline */ public final long size(){
        return m_size;
    }

    /* inline */ public final long capacity();
    /* inline */ public final bool empty();
    /* inline */ public final ref const T opIndex(long i){
        {
            rcAssertFailFunc *failFunc = rcAssertFailGetCustom();
            if (failFunc == 0) {
                (void)((!!(i >= 0 && i < m_size)) || (_wassert(L"i >= 0 && i < m_size", L"C:\\recastnav\\RecastAlloc.h", (unsigned int)(137)) , 0));
            } else if (!(i >= 0 && i < m_size)) {
                (*failFunc)("i >= 0 && i < m_size", "C:\\recastnav\\RecastAlloc.h", 137);
            }
        }
        ;
        return m_data[i];
    }

    /* inline */ public final ref T opIndex(long i){
        {
            rcAssertFailFunc *failFunc = rcAssertFailGetCustom();
            if (failFunc == 0) {
                (void)((!!(i >= 0 && i < m_size)) || (_wassert(L"i >= 0 && i < m_size", L"C:\\recastnav\\RecastAlloc.h", (unsigned int)(138)) , 0));
            } else if (!(i >= 0 && i < m_size)) {
                (*failFunc)("i >= 0 && i < m_size", "C:\\recastnav\\RecastAlloc.h", 138);
            }
        }
        ;
        return m_data[i];
    }

    /* inline */ public final ref const T front();
    /* inline */ public final ref T front();
    /* inline */ public final ref const T back();
    /* inline */ public final ref T back(){
        {
            rcAssertFailFunc *failFunc = rcAssertFailGetCustom();
            if (failFunc == 0) {
                (void)((!!(m_size)) || (_wassert(L"m_size", L"C:\\recastnav\\RecastAlloc.h", (unsigned int)(143)) , 0));
            } else if (!(m_size)) {
                (*failFunc)("m_size", "C:\\recastnav\\RecastAlloc.h", 143);
            }
        }
        ;
        return m_data[m_size - 1];
    }

    /* inline */ public final const(T)* data();
    /* inline */ public final T* data();
    /* inline */ public final T* begin();
    /* inline */ public final T* end();
    /* inline */ public final const(T)* begin();
    /* inline */ public final const(T)* end();
    public final void swap(ref rcVectorBase!(T, H) other);

    public final ref rcVectorBase!(T, H) opAssign(ref rcVectorBase!(T, H) other);

}


extern(C++)
class rcTempVector(T) : rcVectorBase!(T, RC_ALLOC_TEMP)
{
    /* inline */ public final this(){
    }

    /* inline */ public final this(long size);
    /* inline */ public final this(long size, ref const T value){
    }

    /* inline */ public final this(ref rcTempVector!(T) other);
    /* inline */ public final this(const(T)* begin, const(T)* end);
}


extern(C++)
class rcPermVector(T) : rcVectorBase!(T, RC_ALLOC_PERM)
{
    /* inline */ public final this();
    /* inline */ public final this(long size);
    /* inline */ public final this(long size, ref const T value);
    /* inline */ public final this(ref rcPermVector!(T) other);
    /* inline */ public final this(const(T)* begin, const(T)* end);
}

extern(C++)
@cppclasssize(24) align(8)
class rcIntArray
{
    @cppsize(24) private rcTempVector!(int) m_impl;
    /* inline */ public final this(){
    }

    /* inline */ public final this(int n){
    }

    /* inline */ public final void push(int item){
        m_impl.push_back(item);
    }

    /* inline */ public final void resize(int size){
        m_impl.resize(size);
    }

    /* inline */ public final int pop(){
        int v = m_impl.back();
        m_impl.pop_back();
        return v;
    }

    /* inline */ public final int size(){
        return m_impl.size();
    }

    /* inline */ public final ref int opIndex(int index){
        return m_impl[index];
    }

    /* inline */ public final int opIndex(int index){
        return m_impl[index];
    }

    public final this(ref rcIntArray );

    public final this(ref rcIntArray );

    public final ref rcIntArray opAssign(ref rcIntArray );

}

extern(C++)
class rcScopedDelete(T)
{
    @cppsize(8) private T* ptr;
    /* inline */ public final this();
    /* inline */ public final this(T* p);
    /* inline */ public final ~this();
    /* inline */ public final T* opCast!(T*)();
    private final this(ref rcScopedDelete!(T) );

    private final ref rcScopedDelete!(T) opAssign(ref rcScopedDelete!(T) );

}

