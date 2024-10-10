enum DimensionEnum {
    DIMENSION_X,
    DIMENSION_Y,
    DIMENSION_Z
};

float3 convertRealWorldToBlockCoords(float3 p) {
    //NOTE: The origin is at the center of a block
    //NOTE: So 0.5 & 1.4 should both map to 1 - I think the +0.5
    //NOTE: So -0.5 & -1.4 should both map to -1 - I think the -0.5
    //NOTE: So -0.4 & 0.4 should both map to 0 - I think the -0.5
    p.x = round(p.x);
    p.y = round(p.y);
    p.z = round(p.z);

    return p;
}

uint32_t getHashForChunk(int x, int y, int z) {
    int values[3] = {x, y, z};
    uint32_t hash = get_crc32((char *)values, arrayCount(values)*sizeof(int));
    hash = hash & (CHUNK_LIST_SIZE - 1);
    assert(hash < CHUNK_LIST_SIZE);
    assert(hash >= 0);
    return hash;
}

uint32_t getHashForCloudChunk(int x, int z) {
    int values[2] = {x, z};
    uint32_t hash = get_crc32((char *)values, arrayCount(values)*sizeof(int));
    hash = hash & (CHUNK_LIST_SIZE - 1);
    assert(hash < CHUNK_LIST_SIZE);
    return hash;
}

float getBlockTime(BlockType type) {
    float result = 2.0f;

    if(type == BLOCK_TREE_WOOD) {
        result = 4.0f;
    } else if(type == BLOCK_TREE_LEAVES) {
        result = 0.1f;
    } else if(type == BLOCK_GRASS_TALL_ENTITY || type == BLOCK_GRASS_SHORT_ENTITY) {
        result = 0.05f;
    }

    return result;
}

struct AoMaskData {
    GameState *gameState;
    float3 worldP;
    BlockFlags blockFlags; 
    Block *b;
};

uint64_t getInvalidAoMaskValue() {
    return (((uint64_t)(1)) << 63);
}

Block spawnBlock(int x, int y, int z, BlockType type, BlockFlags flags) {
    //NOTE: Input positions are local to chunk
    Block b = {};

    b.x = x;
    b.y = y;
    b.z = z;

    b.flags = flags | BLOCK_EXISTS | BLOCK_FLAGS_AO | BLOCK_FLAG_STACKABLE;

    b.type = type;

    b.timeLeft = getBlockTime(type);

    b.aoMask = getInvalidAoMaskValue();

    b.exists = true;
    
    return b;
}

int getBlockIndex(int x, int y, int z) {
    int result = 0;

    result += abs(z)*CHUNK_DIM*CHUNK_DIM;
    result += abs(y)*CHUNK_DIM;
    result += abs(x);

    assert(result < CHUNK_DIM*CHUNK_DIM*CHUNK_DIM);

    return result;
}

CloudBlock initCloudBlock(int x, int z) {
    CloudBlock result = {};
    result.x = x;
    result.z = z;
    return result;
}

CloudChunk *generateCloudChunk(GameState *gameState, int x, int z, uint32_t hash) {
    CloudChunk *chunk = (CloudChunk *)malloc(sizeof(CloudChunk));
    memset(chunk, 0, sizeof(CloudChunk));

    chunk->x = x;
    chunk->z = z;

    chunk->cloudCount = 0;

    //NOTE: Generate the clouds
    float cloudThreshold = 0.4f;
    for(int localz = 0; localz < CHUNK_DIM; ++localz) {
        for(int localx = 0; localx < CHUNK_DIM; ++localx) {
            if(perlin2d(x*CHUNK_DIM + localx, z*CHUNK_DIM + localz, 0.1f, 20) < cloudThreshold) 
            {
                assert(chunk->cloudCount < arrayCount(chunk->clouds));
                if(chunk->cloudCount < arrayCount(chunk->clouds)) {
                    chunk->clouds[chunk->cloudCount++] = initCloudBlock(localx, localz);
                }
            }
        }
    }

    CloudChunk **chunkPtr = &gameState->cloudChunks[hash];

    if(*chunkPtr) {
        chunkPtr = &((*chunkPtr)->next);
    }

    *chunkPtr = chunk;

    return chunk;
}

CloudChunk *getCloudChunk(GameState *gameState, int x, int z) {
    uint32_t hash = getHashForCloudChunk(x, z);

    CloudChunk *chunk = gameState->cloudChunks[hash];

    bool found = false;

    while(chunk && !found) {
        if(chunk->x == x && chunk->z == z) {
            found = true;
            break;
        }
        chunk = chunk->next;
    }

    if(!chunk) {
        chunk = generateCloudChunk(gameState, x, z, hash);
    }

    return chunk;
}

#define getChunkNoGenerate(gameState, x, y, z) getChunk_(gameState, x, y, z, true, false)
#define getChunk(gameState, x, y, z) getChunk_(gameState, x, y, z, true, true)
#define getChunkReadOnly(gameState, x, y, z) getChunk_(gameState, x, y, z, false, false)

Chunk *generateChunk(GameState *gameState, int x, int y, int z, uint32_t hash);
Chunk *getChunk_(GameState *gameState, int x, int y, int z, bool shouldGenerateChunk, bool shouldGenerateFully);

#include "./generate.cpp"

Chunk *getChunk_(GameState *gameState, int x, int y, int z, bool shouldGenerateChunk, bool shouldGenerateFully) {
    uint32_t hash = getHashForChunk(x, y, z);
    
    Chunk *chunk = gameState->chunks[hash];

    bool found = false;

    while(chunk && !found) {
        if(chunk->x == x && chunk->y == y && chunk->z == z) {
            found = true;
            break;
        }
        chunk = chunk->next;
    }

    if((!chunk && shouldGenerateChunk)) {
        chunk = generateChunk(gameState, x, y, z, hash);
    }

    if(chunk && shouldGenerateFully && chunk->generateState == CHUNK_NOT_GENERATED) {
        //NOTE: Launches multi-thread work
        fillChunk(gameState, chunk);
    } 

    return chunk;
}


void getAOMaskForBlock(GameState *gameState, const float3 worldP, BlockFlags blockFlags, Block *b) {
    if(b->aoMask & (((uint64_t)(1)) << 62)) {
        //NOTE: Generating so don't start a new generation
        // assert(false);
    } else {
        b->aoMask |= (((uint64_t)(1)) << 62);
        b->aoMask |= (((uint64_t)(1)) << 63);

        MemoryBarrier();
        ReadWriteBarrier();

        //NOTE: Multi-threaded version
        AoMaskData *data = (AoMaskData *)malloc(sizeof(AoMaskData));
        
        data->gameState = gameState;
        data->worldP = worldP;
        data->blockFlags = blockFlags; 
        data->b = b;

        pushWorkOntoQueue(&gameState->threadsInfo, getAOMask_multiThreaded, data);

    }
}

void resetChunksAO(GameState *gameState, int x, int y, int z, DimensionEnum dimension, int dimensionValue) {
    Chunk *c = getChunkReadOnly(gameState, x, y, z);

    if(c) {
        for(int i = 0; i < arrayCount(c->blocks); ++i) {
            Block *b = &c->blocks[i];

            if(b->exists) {
                float3 worldP = make_float3(c->x*CHUNK_DIM + b->x, c->y*CHUNK_DIM + b->y, c->z*CHUNK_DIM + b->z);
                if(dimension == DIMENSION_X && b->x == dimensionValue) {
                    b->aoMask = getInvalidAoMaskValue();
                } else if(dimension == DIMENSION_Y && b->y == dimensionValue) {
                    b->aoMask = getInvalidAoMaskValue();
                } else if(dimension == DIMENSION_Z && b->z == dimensionValue) {
                    b->aoMask = getInvalidAoMaskValue();
                }
            }
        }
    }
}

void resetNeighbouringChunksAO(GameState *gameState, int x, int y, int z) {
    //NOTE: This function resets outer boundarie block's AO mask where they should take into account new blocks to compute their AO mask from.
    int maxOffsets[] = {CHUNK_DIM - 1, 0, CHUNK_DIM - 1, 0, CHUNK_DIM - 1, 0};
    DimensionEnum enums[] = {DIMENSION_X, DIMENSION_X, DIMENSION_Y, DIMENSION_Y, DIMENSION_Z, DIMENSION_Z};
    float3 offsets[] = {make_float3(-1, 0, 0), make_float3(1, 0, 0), make_float3(0, -1, 0), make_float3(0, 1, 0), make_float3(0, 0, -1), make_float3(0, 0, 1)};
    for(int i = 0; i < arrayCount(offsets); i++) {
        float3 o = offsets[i];
        resetChunksAO(gameState, x + o.x, y + o.y, z + o.z, enums[i], maxOffsets[i]);
    }
}


Chunk *generateChunk(GameState *gameState, int x, int y, int z, uint32_t hash) {
    Chunk *chunk = (Chunk *)malloc(sizeof(Chunk));
    memset(chunk, 0, sizeof(Chunk));

    chunk->x = x;
    chunk->y = y;
    chunk->z = z;
    chunk->generateState = CHUNK_NOT_GENERATED;
    chunk->entityCount = 0;

    //NOTE: Reset all AO of neighbouring blocks
    resetNeighbouringChunksAO(gameState, x, y, z);

    chunk->next = 0;

    Chunk **chunkPtr = &gameState->chunks[hash];

    if(*chunkPtr) {
        chunkPtr = &((*chunkPtr)->next);
    }

    *chunkPtr = chunk;

    return chunk;
}



Block *blockExistsReadOnly(GameState *gameState, int worldx, int worldy, int worldz, BlockFlags flags) {
    int chunkX = worldx / CHUNK_DIM;
    int chunkY = worldy / CHUNK_DIM;
    int chunkZ = worldz / CHUNK_DIM;

    int localx = worldx - (CHUNK_DIM*chunkX); 
    int localy = worldy - (CHUNK_DIM*chunkY); 
    int localz = worldz - (CHUNK_DIM*chunkZ); 
    
    Chunk *c = getChunkReadOnly(gameState, chunkX, chunkY, chunkZ);
    Block *found = 0;
    if(c) {
        int blockIndex = getBlockIndex(localx, localy, localz);
        assert(blockIndex < arrayCount(c->blocks));
        if(blockIndex < arrayCount(c->blocks) && c->blocks[blockIndex].exists && (c->blocks[blockIndex].flags & flags)) {
            found = &c->blocks[blockIndex];
        }
    }

    return found;
}


void getAOMask_multiThreaded(void *data_) {
    AoMaskData *data = (AoMaskData *)data_;

    GameState *gameState = data->gameState;
    float3 worldP = data->worldP;
    BlockFlags blockFlags = data->blockFlags; 
    Block *b = data->b;

    // assert((b->aoMask & (((uint64_t)(1)) << 62))); //NOTE: It might get invalidated while it's on the queue, so we want to ignore this work
    // assert((b->aoMask & (((uint64_t)(1)) << 63)));

    uint64_t result = 0;

    if((blockFlags & BLOCK_FLAGS_AO)) {
        for(int i = 0; i < arrayCount(global_cubeData); ++i) {
            Vertex v = global_cubeData[i];

            bool blockValues[3] = {false, false, false};
            
            for(int j = 0; j < arrayCount(blockValues); j++) {
                float3 p = plus_float3(worldP,gameState->aoOffsets[i].offsets[j]);
                if(blockExistsReadOnly(gameState, p.x, p.y, p.z, BLOCK_FLAGS_AO)) {
                    blockValues[j] = true; 
                }
            }

            //NOTE: Get the ambient occulusion level
            uint64_t value = 0;
            //SPEED: Somehow make this not into if statments
            if(blockValues[0] && blockValues[2])  {
                value = 3;
            } else if((blockValues[0] && blockValues[1])) {
                assert(!blockValues[2]);
                value = 2;
            } else if((blockValues[1] && blockValues[2])) {
                assert(!blockValues[0]);
                value = 2;
            } else if(blockValues[0]) {
                assert(!blockValues[1]);
                assert(!blockValues[2]);
                value = 1;
            } else if(blockValues[1]) {
                assert(!blockValues[0]);
                assert(!blockValues[2]);
                value = 1;
            } else if(blockValues[2]) {
                assert(!blockValues[0]);
                assert(!blockValues[1]);
                value = 1;
            } 

            result |= (value << (uint64_t)(i*2)); //NOTE: Add the mask value
        }
    }

    MemoryBarrier();
    ReadWriteBarrier();

    b->aoMask = result;
    
    //NOTE: make sure these bits aren't set
    assert(!(b->aoMask & (((uint64_t)(1)) << 62)));
    assert(!(b->aoMask & (((uint64_t)(1)) << 63)));

    free(data_);
}

void drawChunk(GameState *gameState, Chunk *c) {
    
    for(int i = 0; i < arrayCount(c->blocks); ++i) {
        Block *b = &c->blocks[i];

        if(b->exists) {
            float3 worldP = make_float3(c->x*CHUNK_DIM + b->x, c->y*CHUNK_DIM + b->y, c->z*CHUNK_DIM + b->z);
            

            float maxColor = 0.0f;

            float4 color = make_float4(maxColor, maxColor, maxColor, 1);

            BlockType t = b->type;

            if(t == BLOCK_WATER) {
                //NOTE: Only draw the water quad if there isn't any block above it - notice the +1 on the y coord
                if(!blockExistsReadOnly(gameState, worldP.x, worldP.y + 1, worldP.z, (BlockFlags)0xFFFFFFFF)) {
                    //NOTE: Draw the water
                    pushWaterQuad(gameState->renderer, worldP, make_float4(1, 1, 1, 0.6f));
                }
                
            } else if(t == BLOCK_GRASS_SHORT_ENTITY || t == BLOCK_GRASS_TALL_ENTITY) {
                float height = 1;
                if(t == BLOCK_GRASS_TALL_ENTITY) {
                    height = 2;
                }
                pushGrassQuad(gameState->renderer, worldP, height, make_float4(1, 1, 1, 1));
            } else {
                // if(b->hitBlock) {
                //     // t = BLOCK_SOIL;
                //     color = make_float4(1.0f, 1.0f, 1.0f, 1);
                // }

                //NOTE: Calculate the aoMask if haven't yet - top bit is set 
                if(b->aoMask & getInvalidAoMaskValue()) 
                { 
                    getAOMaskForBlock(gameState, worldP, (BlockFlags)b->flags, b);
                }

                uint64_t AOMask = b->aoMask;

                pushCube(gameState->renderer, worldP, t, color, AOMask);
                // pushAlphaItem(gameState->renderer, worldP, make_float3(1, 1, 1), color);
                
                // c->blocks[i].hitBlock = false;
            }
        }
    }

}

void drawCloudChunk(GameState *gameState, CloudChunk *c) {
    int cloudElevation = CLOUD_EVELVATION;
    
    for(int i = 0; i < c->cloudCount; ++i) {
        CloudBlock cloud = c->clouds[i];
        float3 worldP = make_float3(c->x*CHUNK_DIM + cloud.x, cloudElevation, c->z*CHUNK_DIM + cloud.z);
        //NOTE: Push the cloud blocks
        pushAlphaCube(gameState->renderer, worldP, BLOCK_CLOUD, make_float4(1, 1, 1, 1.0f));
    }
}

void removeEntityFromChunk(Chunk *chunk, EntityID id) {
    assert(chunk->entityCount > 0);
    bool found = false;

    for(int i = 0; i < chunk->entityCount && !found; ++i) {
        //NOTE: Check the string pointer is the same
        if(id.stringID == chunk->entities[i].id.stringID) {
            //NOTE: Double check the hash is the same for sanity check
            assert(chunk->entities[i].id.crc32Hash == id.crc32Hash);
            found = true;
            chunk->entities[i] = chunk->entities[--chunk->entityCount];
        }
    }
    assert(found);

}

void storeEntitiesAfterFrameUpdate(GameState *gameState) {
    for(int i = 0; i < gameState->entityCount; ++i) {
        Entity *e = gameState->entitiesForFrame[i];

        //NOTE: Check not already deleted
        if(!(e->flags & ENTITY_DELETED)) {
            EntityChunkInfo chunkInfo = gameState->entitiesForFrameChunkInfo[i];

            float3 blockPos = convertRealWorldToBlockCoords(e->T.pos);

            int entChunkX = (int)blockPos.x / CHUNK_DIM;
            int entChunkY = (int)blockPos.y / CHUNK_DIM;
            int entChunkZ = (int)blockPos.z / CHUNK_DIM;

            Chunk *oldChunk = chunkInfo.chunk;

            int chunkX = oldChunk->x;
            int chunkY = oldChunk->y;
            int chunkZ = oldChunk->z;

            if(!(entChunkX == chunkX && entChunkY == chunkY && entChunkZ == chunkZ)) {
                //NOTE: entity swapped chunk, so move it to the new chunk
                Chunk *newChunk = getChunk(gameState, entChunkX, entChunkY, entChunkZ);
                
                if(newChunk->entityCount < arrayCount(newChunk->entities)) {
                    //NOTE: Assign the new entity to the new chunk
                    newChunk->entities[newChunk->entityCount++] = *e;

                    //NOTE: Remove entity from the old chunk
                    assert(oldChunk->entityCount > 0);
                    removeEntityFromChunk(oldChunk, chunkInfo.entityID);
                }
            }
        }
    }
}


void loadEntitiesForFrameUpdate(GameState *gameState) {
    int chunkRadiusY = 2;
    int chunkRadiusXZ = 2;
    
    //NOTE: Clear the entitiy count from last frame
    gameState->entityCount = 0;

    int chunkX = (int)gameState->player.T.pos.x / CHUNK_DIM;
    int chunkY = (int)gameState->player.T.pos.y / CHUNK_DIM;
    int chunkZ = (int)gameState->player.T.pos.z / CHUNK_DIM;
    
    for(int z = -chunkRadiusXZ; z <= chunkRadiusXZ; ++z) {
        for(int x = -chunkRadiusXZ; x <= chunkRadiusXZ; ++x) {
            for(int y = -chunkRadiusY; y <= chunkRadiusY; ++y) {
                Chunk *chunk = getChunkReadOnly(gameState, chunkX + x, chunkY + y, chunkZ + z);

                if(chunk) {
                    for(int i = 0; i < chunk->entityCount; ++i) {
                        if(gameState->entityCount < arrayCount(gameState->entitiesForFrame)) {
                            int entityIndex = gameState->entityCount++;
                            gameState->entitiesForFrame[entityIndex] = &chunk->entities[i];

                            assert(!(chunk->entities[i].flags & ENTITY_DELETED));

                            //NOTE: Assign the info to store them back afterwards if they move
                            gameState->entitiesForFrameChunkInfo[entityIndex].entityID = chunk->entities[i].id;
                            gameState->entitiesForFrameChunkInfo[entityIndex].chunk = chunk;

                        }
                    }
                }
            }
        }
    }
}

void addItemToInventory(GameState *gameState, Entity *e, int count) {
    InventoryItem *foundItem = 0;

    //NOTE: Check first if there is a slot already with the item
    for(int i = 0; i < gameState->inventoryCount && !foundItem; ++i) {
        InventoryItem *item = &gameState->playerInventory[i];
        int totalCount = item->count + count;
        if(item->type == e->itemType && totalCount <= ITEM_PER_SLOT_COUNT) {
            foundItem = item;
        }
    }
    
    if(!foundItem && gameState->inventoryCount < arrayCount(gameState->playerInventory)) {
        //NOTE: Has room in the inventory
        foundItem = &gameState->playerInventory[gameState->inventoryCount++];
    }

    if(foundItem) {
        foundItem->count += count;
        foundItem->type = e->itemType;
    }
}

void updateParticlers(GameState *gameState) {
    float3 cameraPos = plus_float3(gameState->cameraOffset, gameState->player.T.pos);
    
    for(int i = 0; i < gameState->particlerCount; ) {
        int addend = 1;
        

        Particler *p = &gameState->particlers[i];

        bool shouldRemove = updateParticler(gameState->renderer, p, cameraPos, gameState->dt);

        if(shouldRemove) {
            //NOTE: Move from the end
            gameState->particlers[i] = gameState->particlers[--gameState->particlerCount];
            addend = 0;
        } 

        i += addend;
    }
    
}


BlockChunkPartner blockExists(GameState *gameState, int worldx, int worldy, int worldz, BlockFlags flags) {
    BlockChunkPartner found = {};
    found.block = 0;

    int chunkX = worldx / CHUNK_DIM;
    int chunkY = worldy / CHUNK_DIM;
    int chunkZ = worldz / CHUNK_DIM;

    int localx = worldx - (CHUNK_DIM*chunkX); 
    int localy = worldy - (CHUNK_DIM*chunkY); 
    int localz = worldz - (CHUNK_DIM*chunkZ); 
    
    Chunk *c = getChunk(gameState, chunkX, chunkY, chunkZ);

    if(c) {
            found.chunk = c;
            assert(localx < CHUNK_DIM);
            assert(localy < CHUNK_DIM);
            assert(localz < CHUNK_DIM);
            int blockIndex = getBlockIndex(localx, localy, localz);
            assert(blockIndex < arrayCount(c->blocks));
            if(blockIndex < arrayCount(c->blocks) && c->blocks[blockIndex].exists && c->blocks[blockIndex].flags & flags) {
                // c->blocks[blockIndex].hitBlock = true;
                found.block = &c->blocks[blockIndex];
                found.blockIndex = blockIndex;
            } else {
                // found.block = 0;
            }
    }

    return found;
}

float3 findClosestFreePosition(GameState *gameState, float3 startP, float3 defaultDir, float3 *searchOffsets, int searchOffsetCount, int priorityIndex) {
    float3 result = defaultDir;
    float closestDist = FLT_MAX;
    float3 startP_block = convertRealWorldToBlockCoords(startP);
    bool found = false;

    for(int i = 0; i < searchOffsetCount; ++i) {
        if(found && i >= priorityIndex) {
            //NOTE: Prioritise the first 4 positions
            break;
        }
        float3 offset = searchOffsets[i];

        float3 blockP = plus_float3(offset, startP_block);

        BlockChunkPartner blockPtr = blockExists(gameState, blockP.x, blockP.y, blockP.z, BLOCK_EXISTS_COLLISION);
        if(!blockPtr.block) {
            float3 dirVector = minus_float3(blockP, startP);
            float d = float3_magnitude_sqr(dirVector);

            if(d < closestDist) {
                result = dirVector;
                closestDist = d;
                found = true;
            }
        }
    }
    
    return result;
}


void updateRecoverMovement(GameState *gameState, Entity *e) {
    //NOTE: Apply drag
    e->recoverDP = scale_float3(0.95f, e->recoverDP);

    //NOTE:Integrate velocity
    e->T.pos = plus_float3(e->T.pos, scale_float3(gameState->dt, e->recoverDP));
}

void updateEntities(GameState *gameState) {
    Entity *player = &gameState->player;

    if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN) {
        Entity *shortestEntity = 0;
        float shortestT = FLT_MAX;
        int entityIndex = -1;

        float16 rot = eulerAnglesToTransform(player->T.rotation.y, player->T.rotation.x, player->T.rotation.z);

        float3 lookingAxis = make_float3(rot.E_[2][0], rot.E_[2][1], rot.E_[2][2]);

        for(int i = 0; i < gameState->entityCount; ++i) {
            Entity *e = gameState->entitiesForFrame[i];
            if(e->flags & ENTITY_DESTRUCTIBLE) {
                Rect3f b = make_rect3f_center_dim(e->T.pos, e->T.scale);
                float3 hitPoint;
                float3 normalVector;
                float tAt;
                if(easyMath_rayVsAABB3f(plus_float3(gameState->cameraOffset, player->T.pos), lookingAxis, b, &hitPoint, &tAt, &normalVector)) {
                    if(tAt <= DISTANCE_CAN_PLACE_BLOCK && tAt < shortestT) {
                        shortestT = tAt;
                        shortestEntity = e;
                        entityIndex = i;
                    }
                }
            }
        }

        if(shortestEntity) {
            assert(gameState->entityToDeleteCount < arrayCount(gameState->entitiesToDelete));
            gameState->entitiesToDelete[gameState->entityToDeleteCount++] = entityIndex;
            shortestEntity->flags |= ENTITY_DELETED;
        }
    }

    for(int i = 0; i < gameState->entityCount; ++i) {
        Entity *e = gameState->entitiesForFrame[i];
        

        if(e->flags & SHOULD_ROTATE) {
            //NOTE: Update the rotation
            e->T.rotation.y += 100*gameState->dt;
            e->floatTime += gameState->dt;
            // e->offset.y = 0.01f*sin(2*e->floatTime);
        }

        float3 accelForFrame = make_float3(0, 0, 0);
        if(e->type == ENTITY_PICKUP_ITEM) {

            //NOTE: Pick up the block
            Rect3f bounds = rect3f_minowski_plus(player->T.scale, e->T.scale, e->T.pos);
            if(in_rect3f_bounds(bounds, player->T.pos)) {
                if(gameState->inventoryCount < arrayCount(gameState->playerInventory)) {
                    //NOTE: Pickup the item
                    addItemToInventory(gameState, e, 1);

                    assert(gameState->entityToDeleteCount < arrayCount(gameState->entitiesToDelete));
                    gameState->entitiesToDelete[gameState->entityToDeleteCount++] = i;
                    e->flags |= ENTITY_DELETED;

                    playSound(&gameState->pickupSound);
                    
                }
            } else {
                //NOTE: Check if inside a block 
                float3 worldP = convertRealWorldToBlockCoords(e->T.pos);
                
                if(blockExistsReadOnly(gameState, worldP.x, worldP.y, worldP.z, BLOCK_EXISTS_COLLISION)) {
                    //NOTE: Pickup block is inside another block so moveout of the way
                    float3 moveDir = findClosestFreePosition(gameState, e->T.pos, make_float3(0, 1, 0), gameState->searchOffsets, arrayCount(gameState->searchOffsets), arrayCount(gameState->searchOffsets));
                    moveDir = normalize_float3(moveDir);
                    accelForFrame = scale_float3(gameState->dt*500, moveDir);

                    if(accelForFrame.y < 0) {
                        accelForFrame.y = 0;
                    }
                } else if(!blockExistsReadOnly(gameState, worldP.x, worldP.y -1 , worldP.z, BLOCK_EXISTS_COLLISION)) {
                    //NOTE: check if should apply gravity - that there isn't a block underneath it
                    accelForFrame.y = -10;
                }

                //NOTE: Apply magnetic force when near player
                float3 dir = minus_float3(gameState->player.T.pos, e->T.pos);
                float radiusSqr = 3*3;
                float dirSqr = float3_magnitude_sqr(dir);
                
                if(dirSqr < radiusSqr) {
                    float f = (dirSqr / radiusSqr);
                    float scaleFactor = 30*(1.0f - clamp(0, 1, f*f));
                    //NOTE: In radius so apply force
                    dir = normalize_float3(dir);
                    accelForFrame = plus_float3(accelForFrame, scale_float3(scaleFactor, dir));
                }
            }
        }

        updateRecoverMovement(gameState, e);

        //NOTE: Integrate velocity
        e->dP = plus_float3(e->dP, scale_float3(gameState->dt, accelForFrame)); //NOTE: Already * by dt 
        //NOTE: Apply drag
        e->dP = scale_float3(0.95f, e->dP);
        //NOTE: Get the movement vector for this frame
        e->T.pos = plus_float3(e->T.pos, scale_float3(gameState->dt, e->dP));

        if(e->type == ENTITY_GRASS_SHORT || e->type == ENTITY_GRASS_LONG) {
            float height = 1;
            if(e->type == ENTITY_GRASS_LONG) {
                height = 2;
            }
            pushGrassQuad(gameState->renderer, plus_float3(e->offset, e->T.pos), height, make_float4(1, 1, 1, 1));
        } else {
            //NOTE: Draw the entity now
            float16 T = eulerAnglesToTransform(e->T.rotation.y, e->T.rotation.x, e->T.rotation.z);
            T = float16_scale(T, e->T.scale);
            T = float16_set_pos(T, plus_float3(e->offset, e->T.pos));

            // pushAlphaCube(gameState->renderer, e->T.pos, BLOCK_CLOUD, make_float4(0, 0, 1, 1.0f));
            pushBlockItem(gameState->renderer, T, e->itemType, make_float4(1, 1, 1, 1));   
        }
    }

    //NOTE: Delete entities that should be deleted
    for(int i = 0; i < gameState->entityToDeleteCount; ++i) {
        int index = gameState->entitiesToDelete[i];
        
        EntityChunkInfo chunkInfo = gameState->entitiesForFrameChunkInfo[index];

        assert(chunkInfo.chunk->entityCount > 0);
        removeEntityFromChunk(chunkInfo.chunk, chunkInfo.entityID);
    }

    gameState->entityToDeleteCount = 0;
}