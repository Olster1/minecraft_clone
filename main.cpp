#include "./transform.cpp"
#include "./entity.cpp"
#include "./render.cpp"
#include "./opengl.cpp"
#include "./animation.cpp"

Renderer *initRenderer(Texture grassTexture, Texture breakBlockTexture, Texture atlasTexture) {
    Renderer *renderer = (Renderer *)malloc(sizeof(Renderer));
    
    renderer->cubeCount = 0;
    renderer->atlasQuadCount = 0;
    renderer->terrainTextureHandle = grassTexture.handle;
    // renderer->skyboxTextureHandle = skyboxTexture.handle;
    // renderer->circleHandle = circleTexture.handle;
    renderer->breakBlockTexture = breakBlockTexture.handle;
    // renderer->circleOutlineHandle = circleOutlineTexture.handle;
    renderer->atlasTexture = atlasTexture.handle;

    renderer->blockShader = loadShader(blockVertexShader, blockFragShader);
    renderer->quadTextureShader = loadShader(quadVertexShader, quadTextureFragShader);
    renderer->skyboxShader = loadShader(skyboxVertexShader, skyboxFragShader);
    renderer->quadShader = loadShader(quadVertexShader, quadFragShader);
    renderer->blockPickupShader = loadShader(blockPickupVertexShader, blockPickupFragShader);
    renderer->blockSameTextureShader = loadShader(blockSameTextureVertexShader, blockPickupFragShader);
    renderer->blockColorShader = loadShader(blockVertexShader, blockColorShader);
    
    renderer->blockModel = generateVertexBuffer(global_cubeData, 24, global_cubeIndices, 36);
    renderer->quadModel = generateVertexBuffer(global_quadData, 4, global_quadIndices, 6, ATTRIB_INSTANCE_TYPE_MODEL_MATRIX);
    renderer->blockModelWithInstancedT = generateVertexBuffer(global_cubeData, 24, global_cubeIndices, 36, ATTRIB_INSTANCE_TYPE_MODEL_MATRIX);
    renderer->blockModelSameTexture = generateVertexBuffer(global_cubeData_sameTexture, 24, global_cubeIndices, 36, ATTRIB_INSTANCE_TYPE_MODEL_MATRIX);
    
    // renderer->triangleModel = loadGLTF("./models/sparse.gltf").modelBuffer;
    // renderer->avocadoModel = loadGLTF("./models/avocado/Avocado.gltf").modelBuffer;

    return renderer;
}

#include <stdio.h>
#include "./sound.cpp"
#include "./perlin.cpp"
#include "./interaction.cpp"
#include "./gameState.cpp"
#include "./chunk.cpp"
#include "./player.cpp"
#include "./camera.cpp"

void drawHUD(GameState *gameState) {
    for(int i = 0; i < arrayCount(gameState->playerInventory); ++i) {
        float3 screenP = make_float3((10 + 10*i) - 50, (10 - 50), 1);

        float2 scale = make_float2(10, 10);

        if(i == gameState->currentInventoryHotIndex) {
            scale = scale_float2(1.2f, scale);
        }

        pushHUDOutline(gameState->renderer, screenP, scale, make_float4(1, 1, 1, 1));
    }
}

void updateGame(GameState *gameState) {
    if(!gameState->inited) {
        initGameState(gameState);
    }

    loadEntitiesForFrameUpdate(gameState);

    updatePlayer(gameState, &gameState->player);
    updateCamera(gameState);

    //NOTE: Update the entities
    updateEntities(gameState);

    storeEntitiesAfterFrameUpdate(gameState);
    
    float16 screenGuiT = make_ortho_matrix_origin_center(100, 100*gameState->aspectRatio_y_over_x, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);
    float16 screenT = make_perspective_matrix_origin_center(gameState->camera.fov, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE, 1.0f / gameState->aspectRatio_y_over_x);
    float16 cameraT = getCameraX(gameState->camera.T);
    float16 cameraTWithoutTranslation = getCameraX_withoutTranslation(gameState->camera.T);
    
    int chunkX = (int)gameState->camera.T.pos.x / CHUNK_DIM;
    int chunkY = (int)gameState->camera.T.pos.y / CHUNK_DIM;
    int chunkZ = (int)gameState->camera.T.pos.z / CHUNK_DIM;
    
    // Chunk *chunk = getChunk(gameState, chunkX, chunkY, chunkZ);
    // drawChunk(gameState, chunk);
    // printf("%d %d %d\n", chunkX, chunkY, chunkZ);

    int chunkRadiusY = 1;
    int chunkRadiusXZ = 3;
    
    for(int z = -chunkRadiusXZ; z <= chunkRadiusXZ; ++z) {
        for(int x = -chunkRadiusXZ; x <= chunkRadiusXZ; ++x) {
            for(int y = -chunkRadiusY; y <= chunkRadiusY; ++y) {
                Chunk *chunk = getChunk(gameState, chunkX + x, chunkY + y, chunkZ + z);
                
                drawChunk(gameState, chunk);
            }
        }
    }

    //NOTE: Draw the clouds
    for(int z = -chunkRadiusXZ; z <= chunkRadiusXZ; ++z) {
        for(int x = -chunkRadiusXZ; x <= chunkRadiusXZ; ++x) {
            CloudChunk *chunk = getCloudChunk(gameState, chunkX + x, chunkZ + z);
            assert(chunk);
            drawCloudChunk(gameState, chunk);
        }
    }

    // printf("Player Y: %f\n", gameState->player.T.pos.y);

    // pushCircleOutline(gameState->renderer, make_float3(0, 0, 1), 50, make_float4(1, 1, 1, 1));
    float16 rot = eulerAnglesToTransform(gameState->player.T.rotation.y, gameState->player.T.rotation.x, gameState->player.T.rotation.z);
    float3 lookingAxis = make_float3(rot.E_[2][0], rot.E_[2][1], rot.E_[2][2]);

    // pushTriangle(gameState->renderer, make_float3(1000, 60, 1000), make_float4(1, 0, 1, 1));

    drawHUD(gameState);
    
    rendererFinish(gameState->renderer, screenT, cameraT, screenGuiT, lookingAxis, cameraTWithoutTranslation);

    //NOTE: End Mouse interaction if release
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED && gameState->currentInteraction.isValid) {
        gameState->currentInteraction.isValid = false;
    }

    if(gameState->keys.keys[KEY_1] == MOUSE_BUTTON_PRESSED) {
        gameState->camera.followingPlayer = !gameState->camera.followingPlayer;
        gameState->currentInventoryHotIndex = 0;
    }
    if(gameState->keys.keys[KEY_2] == MOUSE_BUTTON_PRESSED) {
        gameState->currentInventoryHotIndex = 1;
    }
    if(gameState->keys.keys[KEY_3] == MOUSE_BUTTON_PRESSED) {
        gameState->currentInventoryHotIndex = 2;
    }
    if(gameState->keys.keys[KEY_4] == MOUSE_BUTTON_PRESSED) {
        gameState->currentInventoryHotIndex = 3;
    }
    if(gameState->keys.keys[KEY_5] == MOUSE_BUTTON_PRESSED) {
        gameState->currentInventoryHotIndex = 4;
    }
    if(gameState->keys.keys[KEY_6] == MOUSE_BUTTON_PRESSED) {
        gameState->currentInventoryHotIndex = 5;
    }
    if(gameState->keys.keys[KEY_7] == MOUSE_BUTTON_PRESSED) {
        gameState->currentInventoryHotIndex = 6;
    }
    if(gameState->keys.keys[KEY_8] == MOUSE_BUTTON_PRESSED) {
        gameState->currentInventoryHotIndex = 7;
    }
}