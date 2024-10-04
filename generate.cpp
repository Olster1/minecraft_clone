//NOTE: These functions are used Multi threaded
struct FillChunkData {
    GameState *gameState;
    Chunk *chunk;
};

float getTerrainHeight(int worldX, int worldZ, float freq = 0.00522, float octaves = 16) {
    float perlinValueLow = perlin2d(worldX, worldZ, freq, octaves); //SimplexNoise_fractal_2d(16, worldX, worldZ, 0.00522);
    float terrainAmplitude = 100;
    float terrainHeight = perlinValueLow*terrainAmplitude; 

    return terrainHeight;
}

void getAOMask_multiThreaded(void *data_);

void addBlock(GameState *gameState, float3 worldP, BlockType type, BlockFlags flags) {
    int chunkX = (int)worldP.x / CHUNK_DIM;
    int chunkY = (int)worldP.y / CHUNK_DIM;
    int chunkZ = (int)worldP.z / CHUNK_DIM;

    //NOTE: entity swapped chunk, so move it to the new chunk
    Chunk *c = getChunkNoGenerate(gameState, chunkX, chunkY, chunkZ);
    // assert(c);
   
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

bool isTreeLocation(int worldX, int worldZ) {
    float t = perlin2d(worldX, worldZ, 0.8f, 16);
    return t < 0.25f;
}

bool isBushLocation(int worldX, int worldZ) {
    float t = perlin2d(worldX, worldZ, 0.7f, 16);
    return t < 0.4f;
}

bool isBigBush(int worldX, int worldZ) {
    float t = perlin2d(worldX, worldZ, 0.9f, 16);
    return t < 0.5f;
}

void generateTree_multiThread(GameState *gameState, Chunk *chunk, float3 worldP) {
    int treeHeight = (int)(3*((float)rand() / (float)RAND_MAX)) + 3;

    for(int i = 0; i < treeHeight; ++i) {
        float3 p = plus_float3(worldP, make_float3(0, i, 0));
        //NOTE: Add block
        addBlock(gameState, p, BLOCK_TREE_WOOD, BLOCK_EXISTS_COLLISION);
    }

    int z = 0;
    int x = 0;

    BlockFlags leaveFlags = (BlockFlags)(BLOCK_EXISTS_COLLISION | BLOCK_FLAGS_NO_MINE_OUTLINE);

    float3 p = plus_float3(worldP, make_float3(x, (treeHeight + 1), z));
    addBlock(gameState, p, BLOCK_TREE_LEAVES, leaveFlags);

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
            addBlock(gameState, p, BLOCK_TREE_LEAVES, leaveFlags);
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

void fillChunk_multiThread(void *data_) {
    FillChunkData *data = (FillChunkData *)data_;

    GameState *gameState = data->gameState;
    Chunk *chunk = data->chunk;

    int subSoilDepth = 5; //NOTE: 5 blocks to bedrock //TODO: could be random

    for(int z = 0; z < CHUNK_DIM; ++z) {
        for(int x = 0; x < CHUNK_DIM; ++x) {
            int worldX = x + chunk->x*CHUNK_DIM;
            int worldZ = z + chunk->z*CHUNK_DIM;

            float waterElevation = WATER_ELEVATION;

            float terrainHeight = getTerrainHeight(worldX, worldZ);

            for(int y = 0; y < CHUNK_DIM; ++y) {
                BlockFlags flags = BLOCK_EXISTS_COLLISION;
                int worldY = y + chunk->y*CHUNK_DIM;

                bool underWater = worldY < waterElevation;

                if(worldY < terrainHeight) {
                    BlockType type = BLOCK_GRASS;
                    bool isTop = false;

                    if(underWater) {
                        float value = SimplexNoise_fractal_3d(8, worldX, worldY, worldZ, 0.1f);
                        type = BLOCK_SOIL;
                        if(value < 0) {
                            type = BLOCK_STONE;
                        }
                    } else if(worldY < (terrainHeight - 1)) {
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
                        assert(chunk->blocks[blockIndex].flags != 0);
                    }

                    if(worldY > waterElevation && isTop && isTreeLocation(worldX, worldZ)) {
                        generateTree_multiThread(gameState, chunk, make_float3(worldX, worldY + 1, worldZ));
                    } else if(worldY > waterElevation) {
                        if(isTop && isBushLocation(worldX, worldZ)) {
                            EntityType grassType = ENTITY_GRASS_LONG;
                            float height = 2;
                            bool bigBush = isBigBush(worldX, worldZ);
                            if(!bigBush) {
                                grassType = ENTITY_GRASS_SHORT;
                                height = 1;
                            }

                            int chunkX = worldX / CHUNK_DIM;
                            int chunkY = (worldY + 1) / CHUNK_DIM;
                            int chunkZ = worldZ / CHUNK_DIM;


                            int localX = x;
                            int localY = (worldY + 1) - (CHUNK_DIM*chunkY); 
                            int localZ = z;

                            //NOTE: New way by making a block entity
                            int blockIndex = getBlockIndex(localX, localY, localZ);
                            if(blockIndex < arrayCount(chunk->blocks)) {
                                chunk->blocks[blockIndex] = spawnBlock(localX, localY, localZ, BLOCK_GRASS_ENTITY, (BlockFlags)(flags | BLOCK_NOT_PICKABLE | BLOCK_FLAGS_NO_MINE_OUTLINE | BLOCK_FLAGS_UNSAFE_UNDER));
                                chunk->blocks[blockIndex].grassHeight = height;
                                chunk->blocks[blockIndex].flags &= ~(BLOCK_FLAGS_AO | BLOCK_FLAG_STACKABLE | BLOCK_EXISTS_COLLISION);
                                assert(chunk->blocks[blockIndex].flags & BLOCK_EXISTS);
                            }
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

    MemoryBarrier();
    ReadWriteBarrier();

    chunk->generateState = CHUNK_GENERATED;

    free(data_);
}

void fillChunk(GameState *gameState, Chunk *chunk) {
    assert(chunk->generateState == CHUNK_NOT_GENERATED);
    chunk->generateState = CHUNK_GENERATING;

    MemoryBarrier();
    ReadWriteBarrier();

    FillChunkData *data = (FillChunkData *)malloc(sizeof(FillChunkData));

    data->gameState = gameState;
    data->chunk = chunk;

    //NOTE: Multi-threaded version
    pushWorkOntoQueue(&gameState->threadsInfo, fillChunk_multiThread, data);
}
