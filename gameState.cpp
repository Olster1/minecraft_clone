using namespace TextureAtlasModule;

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

    int entityToDeleteCount;
    int entitiesToDelete[MAX_ENTITY_COUNT];

    KeyStates keys;

    AOOffset aoOffsets[24];
};

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

void initGameState(GameState *gameState) {
    gameState->camera.fov = 60;
    gameState->camera.T.pos = make_float3(1000, 100, 1000);
    gameState->camera.followingPlayer = true;
    gameState->cameraOffset = CAMERA_OFFSET;
    gameState->camera.shakeTimer = -1;
    gameState->camera.runShakeTimer = -1;

    gameState->currentInventoryHotIndex = 0;

    srand(time(NULL));

    gameState->timeOfDay = 0.4f;
    
    initPlayer(&gameState->player, gameState->randomStartUpID);
    gameState->player.T.pos = gameState->camera.T.pos;

    loadWavFile(&gameState->cardFlipSound[0], "./sounds/cardFlip.wav", &gameState->audioSpec);
    loadWavFile(&gameState->cardFlipSound[1], "./sounds/cardFlip1.wav", &gameState->audioSpec);
    loadWavFile(&gameState->blockBreakSound, "./sounds/blockBreak.wav", &gameState->audioSpec);
    loadWavFile(&gameState->blockFinishSound, "./sounds/blockFinish.wav", &gameState->audioSpec);
    loadWavFile(&gameState->fallBigSound, "./sounds/fallbig.wav", &gameState->audioSpec);
    loadWavFile(&gameState->pickupSound, "./sounds/pop.wav", &gameState->audioSpec);

    // loadWavFile(&gameState->bgMusic, "./sounds/sweeden.wav", &gameState->audioSpec);

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

    createAOOffsets(gameState);

    gameState->particlerCount = 0;

    initThreadQueue(&gameState->threadsInfo);

    // loadGLTF("./models/fox/Fox.gltf");
    // loadGLTF("./models/boxAnimated/BoxAnimated.gltf");

    // createTextureAtlas(gameState->renderer, "./sprites/");
    gameState->spriteTextureAtlas = readTextureAtlas("./texture_atlas.json", "./texture_atlas.png");
    gameState->renderer->atlasTexture = gameState->spriteTextureAtlas.texture.handle;

    gameState->inited = true;

}

