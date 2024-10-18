struct EntityChunkInfo {
    EntityID entityID;
    Chunk *chunk;
};

struct InventoryItem {
    BlockType type;
    int count;
};

struct AOOffset {
    float3 offsets[3];
};

enum BlockFlags {
    BLOCK_FLAGS_NONE = 0,
    BLOCK_EXISTS_COLLISION = 1 << 0,
    BLOCK_EXISTS = 1 << 1, //NOTE: All blocks have this
    BLOCK_FLAG_STACKABLE = 1 << 2, //NOTE: Whether you can put a block ontop of this one
    BLOCK_NOT_PICKABLE = 1 << 3, //NOTE: Whether it destroys without dropping itself to be picked up i.e. grass just gets destroyed.
    BLOCK_FLAGS_NO_MINE_OUTLINE = 1 << 4, //NOTE: Whether it shows the mining outline
    BLOCK_FLAGS_AO = 1 << 5, //NOTE: Whether it shows the mining outline
    BLOCK_FLAGS_UNSAFE_UNDER = 1 << 6, //NOTE: Whether the block should be destroyed if underneath block destroyed
};

#define CHUNK_LIST_SIZE 4096

struct GameState {
    bool inited;
    float dt;
    float screenWidth;
    float aspectRatio_y_over_x;

    TextureAtlas spriteTextureAtlas;

    bool gravityOn;

    Interaction currentInteraction;

    float2 mouseP_screenSpace;
    float2 mouseP_01;
    float2 lastMouseP;

    int particlerCount;
    Particler particlers[512];
    
    Font mainFont;

    Texture grassTexture;
    Texture soilTexture;
    Texture circleTexture;
    Texture circleOutlineTexture;
    
    MouseKeyState mouseLeftBtn;

    Block *currentMiningBlock;
    float placeBlockTimer;
    float mineBlockTimer;
    float showCircleTimer;

    WavFile cardFlipSound[2];

    int currentInventoryHotIndex;

    //NOTE: First 8 are what player as equipped in the hotbar
    int inventoryCount;
    InventoryItem playerInventory[64];

    float3 cameraOffset;

    WavFile blockBreakSound;
    WavFile blockFinishSound;
    WavFile bgMusic;
    WavFile fallBigSound;
    WavFile pickupSound;

    float3 startP;

    ThreadsInfo threadsInfo;

    PlayingSound *miningSoundPlaying;

    //NOTE: linked hash maps
    Chunk *chunks[CHUNK_LIST_SIZE];
    CloudChunk *cloudChunks[CHUNK_LIST_SIZE];

    Renderer *renderer;

    SDL_AudioSpec audioSpec;

    Camera camera;

    float timeOfDay; //NOTE: 0 - 1 is one day

    int randomStartUpID;

    Entity player;

    #define MAX_ENTITY_COUNT 1024
    int entityCount;
    Entity *entitiesForFrame[MAX_ENTITY_COUNT]; //NOTE: Point to the entities stored on the chunk
    EntityChunkInfo entitiesForFrameChunkInfo[MAX_ENTITY_COUNT]; //NOTE: Point to the entities stored on the chunk

    int entitiesToAddCount;
    Entity entitiesToAddAfterFrame[MAX_ENTITY_COUNT]; //NOTE: Point to the entities stored on the chunk

    int entityToDeleteCount;
    int entitiesToDelete[MAX_ENTITY_COUNT];

    KeyStates keys;

    AOOffset aoOffsets[24];

    float3 searchOffsets[26];
    float3 searchOffsetsSmall[6];

    Texture perlinTestTexture;

    GuiState guiState;
    bool useCameraMovement;
    float3 perlinNoiseValue;

    uint64_t blockFlags[BLOCK_TYPE_COUNT];

    SkeletalModel foxModel;
    float3 modelLocation;
};

void createBlockFlags(GameState *gameState) {
    for(int i = 0; i < arrayCount(gameState->blockFlags); ++i) {
        uint64_t flags = BLOCK_EXISTS | BLOCK_FLAGS_AO | BLOCK_FLAG_STACKABLE | BLOCK_EXISTS_COLLISION;
        BlockType t = (BlockType)i;
        switch(t) {
            case BLOCK_WATER: {
                flags = flags | BLOCK_FLAGS_NONE | BLOCK_FLAGS_NO_MINE_OUTLINE;
                flags &= ~BLOCK_EXISTS_COLLISION;
            } break;
            case BLOCK_GRASS_SHORT_ENTITY:
            case BLOCK_GRASS_TALL_ENTITY: {
                flags = (BlockFlags)(flags | BLOCK_NOT_PICKABLE | BLOCK_FLAGS_NO_MINE_OUTLINE | BLOCK_FLAGS_UNSAFE_UNDER);
                flags &= ~(BLOCK_FLAGS_AO | BLOCK_FLAG_STACKABLE | BLOCK_EXISTS_COLLISION);
            } break;
            case BLOCK_TREE_LEAVES: {
                flags = (BlockFlags)(flags | BLOCK_EXISTS_COLLISION | BLOCK_FLAGS_NO_MINE_OUTLINE);
            } break;
            default: {
                
            };
        }
        gameState->blockFlags[i] = flags;
    }
}

void createAOOffsets(GameState *gameState) {
    for(int i = 0; i < arrayCount(global_cubeData); ++i) {
        assert(i < arrayCount(gameState->aoOffsets));

        Vertex v = global_cubeData[i];
        float3 normal = v.normal;
        float3 sizedOffset = scale_float3(2, v.pos);

        float3 masks[2] = {};
        int maskCount = 0;

        for(int k = 0; k < 3; k++) {
            if(normal.E[k] == 0) {
                assert(maskCount < arrayCount(masks));
                float3 m = make_float3(0, 0, 0);
                m.E[k] = -sizedOffset.E[k];

                masks[maskCount++] = m;
            }
        }

        gameState->aoOffsets[i].offsets[0] = plus_float3(sizedOffset, masks[0]);
        gameState->aoOffsets[i].offsets[1] = sizedOffset; 
        gameState->aoOffsets[i].offsets[2] = plus_float3(sizedOffset, masks[1]);
    }
}

void createSearchOffsets(GameState *gameState) {
    int index = 0;
    for(int z = -1; z <= 1; z++) {
        for(int y = -1; y <= 1; y++) {
            for(int x = -1; x <= 1; x++) {
                if(x == 0 && y == 0 && z == 0) {
                    continue;
                } else {
                    gameState->searchOffsets[index++] = make_float3(x, y, z);
                }

            }
        }
    }
    assert(index == 26);

    gameState->searchOffsetsSmall[0] = make_float3(1, 0, 0); 
    gameState->searchOffsetsSmall[0] = make_float3(0, 0, 1); 
    gameState->searchOffsetsSmall[0] = make_float3(-1, 0, 0); 
    gameState->searchOffsetsSmall[0] = make_float3(0, 0, -1); 

    gameState->searchOffsetsSmall[0] = make_float3(0, 1, 0); 
    gameState->searchOffsetsSmall[0] = make_float3(0, -1, 0); 
}

void initGameState(GameState *gameState) {
    
    assert(BLOCK_TYPE_COUNT < 255);
    gameState->camera.fov = 60;
    gameState->camera.T.pos = make_float3(1000, 100, 1000);
    gameState->camera.followingPlayer = true;
    gameState->cameraOffset = CAMERA_OFFSET;
    gameState->camera.shakeTimer = -1;
    gameState->camera.runShakeTimer = -1;

    gameState->currentInventoryHotIndex = 0;

    createBlockFlags(gameState);
    memset(gameState->chunks, 0, arrayCount(gameState->chunks)*sizeof(Chunk *));

    srand(time(NULL));
    gameState->entitiesToAddCount = 0;

    gameState->timeOfDay = 0.4f;
    
    initPlayer(&gameState->player, gameState->randomStartUpID);
    gameState->player.T.pos = gameState->camera.T.pos;

    loadWavFile(&gameState->cardFlipSound[0], "./sounds/cardFlip.wav", &gameState->audioSpec);
    loadWavFile(&gameState->cardFlipSound[1], "./sounds/cardFlip1.wav", &gameState->audioSpec);
    loadWavFile(&gameState->blockBreakSound, "./sounds/blockBreak.wav", &gameState->audioSpec);
    loadWavFile(&gameState->blockFinishSound, "./sounds/blockFinish.wav", &gameState->audioSpec);
    loadWavFile(&gameState->fallBigSound, "./sounds/fallbig.wav", &gameState->audioSpec);
    loadWavFile(&gameState->pickupSound, "./sounds/pop.wav", &gameState->audioSpec);
    loadWavFile(&gameState->bgMusic, "./sounds/sweeden.wav", &gameState->audioSpec);

    gameState->gravityOn = true;

    gameState->lastMouseP = gameState->mouseP_screenSpace;

    gameState->grassTexture = loadTextureToGPU("./images/grass_block.png");
    // gameState->soilTexture = loadTextureToGPU("./images/soil_block.png");
    // Texture skyboxTexture = loadCubeMapTextureToGPU("./images/skybox/");
    Texture breakBlockTexture = loadTextureToGPU("./images/break_block.png");
    // Texture woodBlockTexture = loadTextureToGPU("./images/woodBlock.png");
    // Texture hotBarTexture = loadTextureToGPU("./images/hotBar.png");
    // Texture leavesTexture = loadTextureToGPU("./images/leaves.png");
    Texture atlasTexture = loadTextureToGPU("./images/atlas.png");

    // gameState->circleOutlineTexture = loadTextureToGPU("./images/circleOutline.png");
    // gameState->circleOutlineTexture = loadTextureToGPU("./models/avocado/Avocado_baseColor.png");
    // gameState->circleTexture = loadTextureToGPU("./images/fillCircle.png");

    gameState->currentMiningBlock = 0;

    // DEBUG_ArrayTests();

    gameState->renderer = initRenderer(gameState->grassTexture, breakBlockTexture, atlasTexture);

    gameState->mainFont = initFontAtlas("./fonts/Minecraft.ttf");
    gameState->renderer->fontAtlasTexture = gameState->mainFont.textureHandle;

    gameState->placeBlockTimer = -1;
    gameState->mineBlockTimer = -1;
    gameState->showCircleTimer = -1;

    gameState->inventoryCount = 0;
    gameState->entityToDeleteCount = 0;

    playSound(&gameState->bgMusic);

    gameState->randomStartUpID = rand();

    // printf("chunk size: %ld\n", sizeof(Chunk));
    // printf("block size: %ld\n", sizeof(Block));
    // printf("Entity size: %ld\n", sizeof(Entity));

    createAOOffsets(gameState);

    gameState->particlerCount = 0;

    initThreadQueue(&gameState->threadsInfo);

    // gameState->foxModel = loadGLTF("./models/fox/Fox.gltf");
    // loadGLTF("./models/boxAnimated/BoxAnimated.gltf");

    // createTextureAtlas(gameState->renderer, "./sprites/");
    gameState->spriteTextureAtlas = readTextureAtlas("./texture_atlas.json", "./texture_atlas.png");
    // gameState->renderer->atlasTexture = gameState->spriteTextureAtlas.texture.handle;
    
    GLint maxUniformBlockSize;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    assert((maxUniformBlockSize / sizeof(float16)) > MAX_BONES_PER_MODEL);
    

    createSearchOffsets(gameState);

    gameState->perlinTestTexture = createGPUTexture(PERLIN_SIZE, PERLIN_SIZE, 0);

    gameState->guiState.grabbedId = 0;

    gameState->useCameraMovement = true;
    gameState->perlinNoiseValue.x = 0.5f;
    gameState->perlinNoiseValue.y = 0.5f;
    gameState->perlinNoiseValue.z = 0.5f;

    gameState->inited = true;

}

