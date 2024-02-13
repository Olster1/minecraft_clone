struct EntityChunkInfo {
    EntityID entityID;
    Chunk *chunk;
};

struct InventoryItem {
    BlockType type;
    int count;
};

struct GameState {
    bool inited;
    float dt;
    float screenWidth;
    float aspectRatio_y_over_x;

    bool gravityOn;

    Interaction currentInteraction;

    float2 mouseP_screenSpace;
    float2 mouseP_01;
    float2 lastMouseP;

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

    int inventoryCount;
    InventoryItem playerInventory[64];

    float3 cameraOffset;

    WavFile blockBreakSound;
    WavFile blockFinishSound;
    WavFile bgMusic;
    WavFile fallBigSound;
    WavFile pickupSound;

    PlayingSound *miningSoundPlaying;

    //NOTE: linked hash map
    Chunk *chunks[4096];

    Renderer *renderer;

    SDL_AudioSpec audioSpec;

    Camera camera;

    int randomStartUpID;

    Entity player;

    #define MAX_ENTITY_COUNT 1024
    int entityCount;
    Entity *entitiesForFrame[MAX_ENTITY_COUNT]; //NOTE: Point to the entities stored on the chunk
    EntityChunkInfo entitiesForFrameChunkInfo[MAX_ENTITY_COUNT]; //NOTE: Point to the entities stored on the chunk

    int entityToDeleteCount;
    int entitiesToDelete[MAX_ENTITY_COUNT];

    KeyStates keys;
};

void initGameState(GameState *gameState) {
    gameState->camera.fov = 60;
    gameState->camera.T.pos = make_float3(1000, 100, 1000);
    gameState->camera.followingPlayer = true;
    gameState->cameraOffset = CAMERA_OFFSET;
    gameState->camera.shakeTimer = -1;

    srand(time(NULL));

    
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
    gameState->soilTexture = loadTextureToGPU("./images/soil_block.png");
    Texture skyboxTexture = loadCubeMapTextureToGPU("./images/skybox/");

    gameState->circleOutlineTexture = loadTextureToGPU("./images/circleOutline.png");
    // gameState->circleOutlineTexture = loadTextureToGPU("./models/avocado/Avocado_baseColor.png");
    gameState->circleTexture = loadTextureToGPU("./images/fillCircle.png");

    gameState->currentMiningBlock = 0;

    gameState->renderer = initRenderer(gameState->grassTexture, gameState->circleTexture, gameState->circleOutlineTexture, skyboxTexture);

    gameState->placeBlockTimer = -1;
    gameState->mineBlockTimer = -1;
    gameState->showCircleTimer = -1;

    gameState->inventoryCount = 0;
    gameState->entityToDeleteCount = 0;

    playSound(&gameState->bgMusic);

    gameState->randomStartUpID = rand();

    // loadGLTF("./models/cesiumMan/CesiumMan.gltf");
    // loadGLTF("./models/boxAnimated/BoxAnimated.gltf");

    gameState->inited = true;

}

