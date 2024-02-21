#define CAMERA_OFFSET make_float3(0, 0.7f, 0)

enum EntityType {
    ENTITY_PLAYER,
    ENTITY_PICKUP_ITEM
};

enum BlockType {
    BLOCK_GRASS,
    BLOCK_SOIL,
    BLOCK_STONE,
    BLOCK_CLOUD,
    BLOCK_TREE_WOOD,
    BLOCK_TREE_LEAVES,
};

struct Block {
    //NOTE: Local to the Chunk they're in
    int x;
    int y;
    int z;

    bool hitBlock;
    float timeLeft;
    float maxTime;
    bool exists;

    BlockType type;
};

struct CloudBlock {
    //NOTE: Local to the Chunk they're in
    int x;
    int z;
};

enum EntityFlags {
    SHOULD_ROTATE = 1 << 0,
    ENTITY_DELETED = 1 << 1,
};

struct EntityID {
    size_t stringSizeInBytes; //NOTE: Not include null terminator
    char *stringID; //NOTE: Allocated with malloc
    uint32_t crc32Hash;
};

static int global_entityIdCreated = 0;

EntityID makeEntityId(int randomStartUpID) {
    EntityID result = {};

    time_t timeSinceEpoch = time(0);

    #define ENTITY_ID_PRINTF_STRING "%ld-%d-%d", timeSinceEpoch, randomStartUpID, global_entityIdCreated

    //NOTE: Allocate the string
    size_t bufsz = snprintf(NULL, 0, ENTITY_ID_PRINTF_STRING);
    result.stringID = (char *)malloc(bufsz + 1);
    result.stringSizeInBytes = bufsz;
    snprintf(result.stringID, bufsz + 1, ENTITY_ID_PRINTF_STRING);

    result.crc32Hash = get_crc32(result.stringID, result.stringSizeInBytes);

    #undef ENTITY_ID_PRINTF_STRING

    //NOTE: This would have to be locked in threaded application
    global_entityIdCreated++;

    return result;
} 

struct Entity {
    EntityID id;

    float3 offset;
    float floatTime;
    
    TransformX T;
    float3 dP;
    EntityType type;

    Rect3f collisionBox;
    bool grounded;
    bool tryJump;


    uint32_t flags;
    BlockType itemType;
};

#define CHUNK_DIM 16
#define BLOCKS_PER_CHUNK CHUNK_DIM*CHUNK_DIM*CHUNK_DIM

struct CloudChunk {
    int x;
    int z;

    int cloudCount;
    CloudBlock clouds[CHUNK_DIM*CHUNK_DIM];

    CloudChunk *next;
};

struct Chunk {
    int x;
    int y;
    int z;

    //NOTE: 16 x 16 x 16
    //NOTE: Z Y X
    Block blocks[BLOCKS_PER_CHUNK];

    int entityCount;
    Entity entities[64]; //NOTE: Max entity count stored on a chunk //TODO: should be a growing array

    Chunk *next;
};

struct BlockChunkPartner {
    Block *block;
    Chunk *chunk;

    int blockIndex;

    float3 sideNormal;
};

struct Camera {
    TransformX T;
    float fov;
    float shakeTimer;
    bool followingPlayer;
};

void initBaseEntity(Entity *e, int randomStartUpID) {
    e->id = makeEntityId(randomStartUpID);
    e->T.rotation = make_float3(0, 0, 0);
    e->dP = make_float3(0, 0, 0);
}

Entity *initPlayer(Entity *e, int randomStartUpID) {
    initBaseEntity(e, randomStartUpID);
    e->T.pos = make_float3(0, 0, 0);
    float playerWidth = 0.7f;
    e->T.scale = make_float3(playerWidth, 1.7f, playerWidth);
    // e->T.scale = make_float3(1, 1, 1);
    
    e->type = ENTITY_PLAYER;
    e->offset = make_float3(0, 0, 0);
    e->grounded = false;
    e->flags = 0;
    return e;
}

void initPickupItem(Chunk *chunk, float3 pos, BlockType itemType, int randomStartUpID) {
    if(chunk->entityCount < arrayCount(chunk->entities)) {
        Entity *e = chunk->entities + chunk->entityCount++;
        initBaseEntity(e, randomStartUpID);
        e->T.pos = pos;
        float scale = 0.3f;
        e->T.scale = make_float3(scale, scale, scale);
        e->type = ENTITY_PICKUP_ITEM;
        e->offset = make_float3(0, 0, 0);
        e->grounded = false;
        e->itemType = itemType;
        e->flags = SHOULD_ROTATE;
    }
}

