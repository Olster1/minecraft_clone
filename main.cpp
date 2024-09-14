#include "./game_defines.h"
#include "./easy_memory.h"
#include "./memory_arena.h"
#include "./easy_string_utf8.h"
#include "./easy_string.h"
#include "./easy_files.h"
#include "./easy_lex.h"

#include "./transform.cpp"
#include "./entity.cpp"
#include "./render.cpp"
#include "./opengl.cpp"
#include "./font.cpp"
#include "./particles.cpp"

#include "./texture_atlas.cpp"
// #include "./animation.cpp"

Renderer *initRenderer(Texture grassTexture, Texture breakBlockTexture, Texture atlasTexture) {
    Renderer *renderer = (Renderer *)malloc(sizeof(Renderer));
    
    renderer->cubeCount = 0;
    renderer->atlasQuadCount = 0;
    renderer->glyphCount = 0;
    renderer->terrainTextureHandle = grassTexture.handle;
    // renderer->skyboxTextureHandle = skyboxTexture.handle;
    // renderer->circleHandle = circleTexture.handle;
    renderer->breakBlockTexture = breakBlockTexture.handle;
    // renderer->circleOutlineHandle = circleOutlineTexture.handle;
    renderer->atlasTexture = atlasTexture.handle;

    renderer->blockShader = loadShader(blockVertexShader, blockFragShader);
    renderer->quadTextureShader = loadShader(quadVertexShader, quadTextureFragShader);
    renderer->fontTextureShader = loadShader(quadVertexShader, fontTextureFragShader);
    
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
#include "./SimplexNoise.cpp"
#include "./interaction.cpp"
#include "./gameState.cpp"
#include "./chunk.cpp"
#include "./player.cpp"
#include "./camera.cpp"

TimeOfDayValues getTimeOfDayValues(GameState *gameState) {
    float4 a;
    float4 b;

    float4 dayA = make_float4(0.678, 0.847, 0.901, 1);
    float4 dayB = make_float4(0.126, 0.162, 0.529, 1);
    
    float4 afternoonA = make_float4(0.98, 0.56, 0.384, 1);
    float4 afternoonB = make_float4(0.8, 0.28, 0.576, 1);

    float4 nightTimeA = make_float4(0.18, 0.09, 0.2, 1);
    float4 nightTimeB = make_float4(0, 0, 0, 1);

    if(gameState->timeOfDay < 0.25f) {
        float t = gameState->timeOfDay / 0.25f;
        a = lerp_float4(nightTimeA, afternoonA, t);
        b = lerp_float4(nightTimeB, afternoonB, t);
    } else if(gameState->timeOfDay >= 0.25f && gameState->timeOfDay < 0.5f) {
        float t = (gameState->timeOfDay - 0.25f) / 0.25f;
        a = lerp_float4(afternoonA, dayA, t);
        b = lerp_float4(afternoonB, dayB, t);
    } else if(gameState->timeOfDay >= 0.5f && gameState->timeOfDay < 0.75f) {
        float t = (gameState->timeOfDay - 0.5f) / 0.25f;
        a = lerp_float4(dayA, afternoonA, t);
        b = lerp_float4(dayB, afternoonB, t);
    } else if(gameState->timeOfDay >= 0.75f && gameState->timeOfDay <= 1.0f) {
        float t = (gameState->timeOfDay - 0.75f) / 0.25f;
        a = lerp_float4(afternoonA, nightTimeA, t);
        b = lerp_float4(afternoonB, nightTimeB, t);
    }

    TimeOfDayValues result = {};
    result.skyColorA = a;
    result.skyColorB = b;

    return result;

}

void drawHUD(GameState *gameState) {
    for(int i = 0; i < ITEM_HOT_SPOTS; ++i) {
        float2 scale = make_float2(8, 8);
        float halfWidth = 0.5f*(ITEM_HOT_SPOTS*scale.x);
        float3 screenP = make_float3(((scale.x*i) + 0.5f*scale.x) - halfWidth, (-50 + 0.55f*scale.y), 1);

        if(i == gameState->currentInventoryHotIndex) {
            scale = scale_float2(1.2f, scale);
        }

        pushHUDOutline(gameState->renderer, screenP, scale, make_float4(1, 1, 1, 1));
        if(gameState->inventoryCount >= i && gameState->playerInventory[i].count > 0) {
            //NOTE: Draw item if in slot
            InventoryItem *item = &gameState->playerInventory[i];

            //NOTE: Draw the inventory sprite
            pushSpriteForInventoryType(gameState->renderer, screenP, scale, make_float4(1, 1, 1, 1), item->type);

            //NOTE: Draw the number of items we have
            char s[255];
            sprintf (s, "%d", item->count);
            float2 numPos = make_float2(screenP.x, screenP.y);
            
            //NOTE: Adjust the screen position from using the HUD projection T to the text projection T which uses y+ve down and starts in top corner. Both are 100 units wide. 
            numPos.x = 50 + numPos.x;
            numPos.y = -1*(numPos.y - 50);
        
            renderText(gameState->renderer, &gameState->mainFont, s, numPos, 0.2f);
            
        }
    }
    {
        //NOTE: Draw the player stamina bar
        float3 screenP = make_float3(-35, 49, 1);
        float2 scale = make_float2(30, 2.5);
        float3 innerScreenP = screenP;
        float2 innerScale = scale;
        innerScale.x *= gameState->player.stamina;

        innerScreenP.x -= 0.5f*(scale.x - innerScale.x);
        pushPlainQuadHUD(gameState->renderer, innerScreenP, innerScale, make_float4(1, 1, 1, 1));
        pushHUDOutline(gameState->renderer, screenP, scale, make_float4(1, 1, 1, 1));
    }
    
}

void updateGame(GameState *gameState) {
    if(!gameState->inited) {
        globalLongTermArena = createArena(Kilobytes(200));
        globalPerFrameArena = createArena(Kilobytes(100));
        perFrameArenaMark = takeMemoryMark(&globalPerFrameArena);

        initGameState(gameState);
    } else { 
        releaseMemoryMark(&perFrameArenaMark);
        perFrameArenaMark = takeMemoryMark(&globalPerFrameArena);
    }

    gameState->camera.targetFov = 60;

    gameState->timeOfDay += 0.001f*gameState->dt;

    if(gameState->timeOfDay > 1.0f) {
        //NOTE: Always keep in 0 - 1 range
        int a = (int)(gameState->timeOfDay*100);
        a = a % 100;
        gameState->timeOfDay = (float)(a / 100);
    }

    TimeOfDayValues timeOfDayValues = getTimeOfDayValues(gameState);

    loadEntitiesForFrameUpdate(gameState);

    updatePlayer(gameState, &gameState->player);
    updateCamera(gameState);

    //NOTE: Update the entities
    updateEntities(gameState);

    //NOTE: Update particle systems
    updateParticlers(gameState);

    {
        float3 cameraPos = plus_float3(gameState->cameraOffset, gameState->player.T.pos);
        float3 pPos = gameState->startP;
        float3 diffVec = minus_float3(cameraPos, pPos);
        float d = radiansToDegrees((atan2(diffVec.z, diffVec.x) + 0.5f*PI32));
        float3 rotation = make_float3(0, d, 0);

        //NOTE: Draw the particle
        pushAtlasQuad_(gameState->renderer, pPos, make_float3(2, 2, 2), rotation, make_float4(0, 0.25f, 0, 0.25f), make_float4(1, 1, 1, 1), false);

    }
   
    storeEntitiesAfterFrameUpdate(gameState);
    
    float16 screenGuiT = make_ortho_matrix_origin_center(100, 100*gameState->aspectRatio_y_over_x, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);
    float16 textGuiT = make_ortho_matrix_top_left_corner_y_down(100, 100*gameState->aspectRatio_y_over_x, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);

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

                if(chunk) {
                    drawChunk(gameState, chunk);
                }
            }
        }
    }

    // NOTE: Draw the clouds
    for(int z = -chunkRadiusXZ; z <= chunkRadiusXZ; ++z) {
        for(int x = -chunkRadiusXZ; x <= chunkRadiusXZ; ++x) {
            CloudChunk *chunk = getCloudChunk(gameState, chunkX + x, chunkZ + z);
            assert(chunk);
            drawCloudChunk(gameState, chunk);
        }
    }

    // pushCircleOutline(gameState->renderer, make_float3(0, 0, 1), 50, make_float4(1, 1, 1, 1));
    float16 rot = eulerAnglesToTransform(gameState->player.T.rotation.y, gameState->player.T.rotation.x, gameState->player.T.rotation.z);
    float3 lookingAxis = make_float3(rot.E_[2][0], rot.E_[2][1], rot.E_[2][2]);

    // pushTriangle(gameState->renderer, make_float3(1000, 60, 1000), make_float4(1, 0, 1, 1));

    drawHUD(gameState);
    
    rendererFinish(gameState->renderer, screenT, cameraT, screenGuiT, textGuiT, lookingAxis, cameraTWithoutTranslation, timeOfDayValues);

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