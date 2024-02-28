uint32_t getHashForChunk(int x, int y, int z) {
    int values[3] = {x, y, z};
    uint32_t hash = get_crc32((char *)values, arrayCount(values)*sizeof(int));
    hash = hash & (CHUNK_LIST_SIZE - 1);
    assert(hash < CHUNK_LIST_SIZE);
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
    }

    return result;
}

enum BlockFlags {
    BLOCK_FLAGS_NONE = 0,
    BLOCK_EXISTS_COLLISION = 1 << 0,
    BLOCK_EXISTS = 1 << 1, //NOTE: All blocks have this
};

Block spawnBlock(int x, int y, int z, BlockType type, BlockFlags flags) {
    //NOTE: Input positions are local to chunk
    Block b = {};

    b.x = x;
    b.y = y;
    b.z = z;

    b.flags = flags | BLOCK_EXISTS;

    b.type = type;

    b.maxTime = getBlockTime(type);
    b.timeLeft = b.maxTime;

    b.exists = true;
    
    return b;
}

int getBlockIndex(int x, int y, int z) {
    int result = 0;

    result += z*CHUNK_DIM*CHUNK_DIM;
    result += y*CHUNK_DIM;
    result += x;

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


void addBlock(GameState *gameState, float3 worldP, BlockType type, BlockFlags flags) {
    int chunkX = (int)worldP.x / CHUNK_DIM;
    int chunkY = (int)worldP.y / CHUNK_DIM;
    int chunkZ = (int)worldP.z / CHUNK_DIM;

    //NOTE: entity swapped chunk, so move it to the new chunk
    Chunk *c = getChunkNoGenerate(gameState, chunkX, chunkY, chunkZ);

    assert(c);
   
    if(c) {
        int localX = worldP.x - (CHUNK_DIM*chunkX); 
        int localY = worldP.y - (CHUNK_DIM*chunkY); 
        int localZ = worldP.z - (CHUNK_DIM*chunkZ); 

        int blockIndex = getBlockIndex(localX, localY, localZ);
        if(blockIndex < arrayCount(c->blocks)) {
            c->blocks[blockIndex] = spawnBlock(localX, localY, localZ, type, flags);
        } else {
            assert(false);
        }
    } 
}

void generateTree(GameState *gameState, Chunk *chunk, float3 worldP) {
    int treeHeight = (int)(3*((float)rand() / (float)RAND_MAX)) + 3;

    for(int i = 0; i < treeHeight; ++i) {
        float3 p = plus_float3(worldP, make_float3(0, i, 0));
        //NOTE: Add block
        addBlock(gameState, p, BLOCK_TREE_WOOD, BLOCK_EXISTS_COLLISION);
    }

    int z = 0;
    int x = 0;

    float3 p = plus_float3(worldP, make_float3(x, (treeHeight + 1), z));
    addBlock(gameState, p, BLOCK_TREE_LEAVES, BLOCK_EXISTS_COLLISION);

    float3 offsets[] = {make_float3(1, treeHeight, 0), make_float3(1, treeHeight, 1), 
                        make_float3(-1, treeHeight, -1), make_float3(1, treeHeight, -1), 
                        make_float3(-1, treeHeight, 1), make_float3(0, treeHeight, 1),
                        make_float3(-1, treeHeight, 0), make_float3(0, treeHeight, -1), 
                        };

    for(int j = 0; j < 2; ++j) {
        for(int i = 0; i < arrayCount(offsets); ++i) {
            float3 o = offsets[i];
            o.y -= j;
            float3 p = plus_float3(worldP, o);
            addBlock(gameState, p, BLOCK_TREE_LEAVES, BLOCK_EXISTS_COLLISION);
        }
    }
    treeHeight -=1;

    float3 offsets2[] = {make_float3(2, treeHeight, 0), make_float3(2, treeHeight, 1), make_float3(2, treeHeight, 2), 
                        make_float3(1, treeHeight, 2), make_float3(0, treeHeight, 2), 

                        make_float3(-1, treeHeight, 2), make_float3(-2, treeHeight, 2), make_float3(-2, treeHeight, 1), 
                        make_float3(-2, treeHeight, 0), make_float3(-2, treeHeight, -1), 

                        make_float3(-2, treeHeight, -2), make_float3(-1, treeHeight, -2), 
                        make_float3(0, treeHeight, -2), make_float3(1, treeHeight, -2),  make_float3(2, treeHeight, -2), make_float3(2, treeHeight, -1), 
                        };

    for(int i = 0; i < arrayCount(offsets2); ++i) {
        float3 p = plus_float3(worldP, offsets2[i]);
        addBlock(gameState, p, BLOCK_TREE_LEAVES, BLOCK_EXISTS_COLLISION);
    }

}

void fillChunk(GameState *gameState, Chunk *chunk) {
    int subSoilDepth = 5; //NOTE: 5 blocks to bedrock //TODO: could be random

    for(int z = 0; z < CHUNK_DIM; ++z) {
        for(int x = 0; x < CHUNK_DIM; ++x) {
            int worldX = x + chunk->x*CHUNK_DIM;
            int worldZ = z + chunk->z*CHUNK_DIM;

            float perlinValueLow = perlin2d(worldX, worldZ, 0.00522, 16);
            float perlinValueHigh = 0;//perlin2d(worldX, worldZ, 10, 4);
            
            float waterElevation = 40;
            float terrainAmplitude = 100;
            float terrainHeight = perlinValueLow*terrainAmplitude + perlinValueHigh; 

            for(int y = 0; y < CHUNK_DIM; ++y) {
                BlockFlags flags = BLOCK_EXISTS_COLLISION;
                int worldY = y + chunk->y*CHUNK_DIM;

                if(worldY < terrainHeight) {
                    BlockType type = BLOCK_GRASS;
                    bool isTop = false;

                    if(worldY < (terrainHeight - 1)) {
                        if(worldY >= ((terrainHeight - 1) - subSoilDepth)) {
                            type = BLOCK_SOIL;
                        } else {
                            type = BLOCK_STONE;
                        }
                    } else {
                        isTop = true;
                    }

                    int blockIndex = getBlockIndex(x, y, z);
                    if(blockIndex < arrayCount(chunk->blocks)) {
                        chunk->blocks[blockIndex] = spawnBlock(x, y, z, type, flags);
                    }

                    int treeProb = (int)(2*((float)rand() / (float)RAND_MAX));
                    float prob = (float)rand() / (float)RAND_MAX;

                    if(worldY > waterElevation && isTop && !(worldX % 4 - treeProb) && !(worldZ % 4 - treeProb) && prob > 0.5f) {
                        generateTree(gameState, chunk, make_float3(worldX, worldY + 1, worldZ));
                    } else if(worldY > waterElevation) {
                        int grassProb = (int)(2*((float)rand() / (float)RAND_MAX));
                        prob = (float)rand() / (float)RAND_MAX;
                        if(isTop && !(worldX % 2 - grassProb) && !(worldZ % 2 - grassProb) && prob > 0.5f) {
                            EntityType grassType = ENTITY_GRASS_LONG;
                            prob = (float)rand() / (float)RAND_MAX;
                            if(prob < 0.5f) {
                                grassType = ENTITY_GRASS_SHORT;
                            }

                            int chunkX = worldX / CHUNK_DIM;
                            int chunkY = (worldY + 1) / CHUNK_DIM;
                            int chunkZ = worldZ / CHUNK_DIM;

                            Chunk *c = chunk;

                            if(c->x == chunkX && c->z == chunkZ && c->y == chunkY) {

                            } else {
                                c = getChunkNoGenerate(gameState, chunkX, chunkY, chunkZ);
                            }
                            
                            initGrassEntity(c, make_float3(worldX, worldY + 1, worldZ), grassType, gameState->randomStartUpID);
                        }
                    }
                    
                } else if(worldY < waterElevation) {
                    flags = BLOCK_FLAGS_NONE;
                    //NOTE: Is water so add water
                    int blockIndex = getBlockIndex(x, y, z);
                    if(blockIndex < arrayCount(chunk->blocks)) {
                        chunk->blocks[blockIndex] = spawnBlock(x, y, z, BLOCK_WATER, flags);
                    }

                }
            }
        }
    }

    chunk->isGenerated = true;
}

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

    if(chunk && shouldGenerateFully && !chunk->isGenerated) {
        fillChunk(gameState, chunk);
    } 

    return chunk;
}


Chunk *generateChunk(GameState *gameState, int x, int y, int z, uint32_t hash) {
    Chunk *chunk = (Chunk *)malloc(sizeof(Chunk));
    memset(chunk, 0, sizeof(Chunk));

    chunk->x = x;
    chunk->y = y;
    chunk->z = z;
    chunk->isGenerated = false;
    chunk->entityCount = 0;

    chunk->next = 0;

    Chunk **chunkPtr = &gameState->chunks[hash];

    if(*chunkPtr) {
        chunkPtr = &((*chunkPtr)->next);
    }

    *chunkPtr = chunk;

    return chunk;
}



bool blockExistsReadOnly(GameState *gameState, int worldx, int worldy, int worldz, BlockFlags flags) {
    int chunkX = worldx / CHUNK_DIM;
    int chunkY = worldy / CHUNK_DIM;
    int chunkZ = worldz / CHUNK_DIM;

    int localx = worldx - (CHUNK_DIM*chunkX); 
    int localy = worldy - (CHUNK_DIM*chunkY); 
    int localz = worldz - (CHUNK_DIM*chunkZ); 
    
    Chunk *c = getChunkReadOnly(gameState, chunkX, chunkY, chunkZ);
    bool found = false;
    if(c) {
        int blockIndex = getBlockIndex(localx, localy, localz);
        assert(blockIndex < arrayCount(c->blocks));
        if(blockIndex < arrayCount(c->blocks) && c->blocks[blockIndex].exists && c->blocks[blockIndex].flags & flags) {
            found = true;
        }
    }

    return found;
}

uint64_t getAOMask(GameState *gameState, const float3 worldP) {
    uint64_t result = 0;

    for(int i = 0; i < arrayCount(global_cubeData); ++i) {
        Vertex v = global_cubeData[i];

        bool blockValues[3] = {false, false, false};
        
        for(int j = 0; j < arrayCount(blockValues); j++) {
            float3 p = plus_float3(worldP,gameState->aoOffsets[i].offsets[j]);
            if(blockExistsReadOnly(gameState, p.x, p.y, p.z, (BlockFlags)0xFFFFFFFF)) {
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

    return result;

}


void drawChunk(GameState *gameState, Chunk *c) {
    
    for(int i = 0; i < arrayCount(c->blocks); ++i) {
        Block b = c->blocks[i];

        if(b.exists) {
            float3 worldP = make_float3(c->x*CHUNK_DIM + b.x, c->y*CHUNK_DIM + b.y, c->z*CHUNK_DIM + b.z);

            float maxColor = 0.9f;

            float4 color = make_float4(maxColor, maxColor, maxColor, 1);

            BlockType t = b.type;

            if(t == BLOCK_WATER) {
                if(!blockExistsReadOnly(gameState, worldP.x, worldP.y + 1, worldP.z, (BlockFlags)0xFFFFFFFF)) {
                    //NOTE: Draw the water
                    pushWaterQuad(gameState->renderer, worldP, make_float4(1, 1, 1, 0.6f));
                }
                
            } else {
                if(b.hitBlock) {
                    // t = BLOCK_SOIL;
                    color = make_float4(1, 1, 1, 1);
                }

                uint64_t AOMask = 0;//getAOMask(gameState, worldP);
                
                pushCube(gameState->renderer, worldP, t, color, AOMask);
                // pushAlphaItem(gameState->renderer, worldP, make_float3(1, 1, 1), color);
                
                c->blocks[i].hitBlock = false;
            }
        }
    }

}

void drawCloudChunk(GameState *gameState, CloudChunk *c) {
    int cloudElevation = 80;
    
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

            int entChunkX = (int)e->T.pos.x / CHUNK_DIM;
            int entChunkY = (int)e->T.pos.y / CHUNK_DIM;
            int entChunkZ = (int)e->T.pos.z / CHUNK_DIM;

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

void updateEntities(GameState *gameState) {
    Entity *player = &gameState->player;
    for(int i = 0; i < gameState->entityCount; ++i) {
        Entity *e = gameState->entitiesForFrame[i];

        if(e->flags & SHOULD_ROTATE) {
            //NOTE: Update the rotation
            e->T.rotation.y += 100*gameState->dt;
            e->floatTime += gameState->dt;
            e->offset.y = 0.3f*sin(2*e->floatTime);
        }

        if(e->type == ENTITY_PICKUP_ITEM) {

            //NOTE: Update Fall
            

            Rect3f bounds = rect3f_minowski_plus(player->T.scale, e->T.scale, e->T.pos);
            if(in_rect3f_bounds(bounds, player->T.pos)) {
                if(gameState->inventoryCount < arrayCount(gameState->playerInventory)) {
                    //NOTE: Pickup the item
                    InventoryItem *item = &gameState->playerInventory[gameState->inventoryCount++];
                    item->count++;
                    item->type = e->itemType;

                    assert(gameState->entityToDeleteCount < arrayCount(gameState->entitiesToDelete));

                    gameState->entitiesToDelete[gameState->entityToDeleteCount++] = i;
                    e->flags |= ENTITY_DELETED;

                    playSound(&gameState->pickupSound);
                    
                }
            }
        }

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