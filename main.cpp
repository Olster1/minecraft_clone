#include "./game_defines.h"

uint64_t getBitWiseFlag(uint64_t bitWise) {
    return ((uint64_t)1 << bitWise);
}

static float global_fogFarDistance;
static float global_fogSeeDistance;

#include "./easy_memory.h"
#include "./resize_array.cpp"
#include "./memory_arena.h"
#include "./easy_string_utf8.h"
#include "./easy_string.h"
#include "./easy_files.h"
#include "./easy_lex.h"
#include "./imgui.h"
#include "./render.h"
#include "./transform.cpp"
#include "./animation.h"
#include "./entity.cpp"
#include "./render.cpp"
#include "./opengl.cpp"
#include "./font.cpp"
#include "./particles.cpp"
#include "./load_gltf.cpp"



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
    renderer->blockGreedyShader = loadShader(blockGreedyVertexShader, blockFragShader);
    renderer->quadTextureShader = loadShader(quadVertexShader, quadTextureFragShader);
    renderer->fontTextureShader = loadShader(quadVertexShader, fontTextureFragShader);
    
    renderer->skyboxShader = loadShader(skyboxVertexShader, skyboxFragShader);
    renderer->quadShader = loadShader(quadVertexShader, quadFragShader);
    renderer->blockPickupShader = loadShader(blockPickupVertexShader, blockPickupFragShader);
    renderer->skeletalModelShader = loadShader(skeletalVertexShader, skeletalFragShader);
    renderer->blockSameTextureShader = loadShader(blockSameTextureVertexShader, blockPickupFragShader);
    // renderer->blockColorShader = loadShader(blockVertexShader, blockColorShader);
    renderer->blockColorShader = loadShader(blockVertexShader, blockFragShader);
    
    
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
#include "./texture_atlas.cpp"
#include "./gameState.cpp"
#include "./chunk.cpp"
#include "./player.cpp"
#include "./camera.cpp"
#include "./perlin_noise_test.cpp"
#include "./imgui.cpp"
#include "./minecraft_animations.cpp"
#include "./animation.cpp"


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
    result.skyColorA = dayA;
    result.skyColorB = dayB;

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
        // pushSprite(gameState->renderer, textureAtlas_getItem(&gameState->spriteTextureAtlas, "pumpkin.png"), screenP, scale, make_float4(1, 1, 1, 1));
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

void updateAndDrawDebugCode(GameState *gameState) {
    // perlinNoiseDrawTests(gameState, gameState->camera.T.pos.x, gameState->camera.T.pos.y, gameState->camera.T.pos.z);
    // global_fogSeeDistance = gui_drawSlider(gameState, &gameState->guiState, gameState->renderer, "Perlin Noise Slider x", global_fogSeeDistance);
    // global_fogFarDistance = gui_drawSlider(gameState, &gameState->guiState, gameState->renderer, "Perlin Noise Slider y", global_fogFarDistance, 3);
    // gameState->perlinNoiseValue.z = gui_drawSlider(gameState, &gameState->guiState, gameState->renderer, "Perlin Noise Slider z", gameState->perlinNoiseValue.z, 6);

    // printf("%f %f\n", global_fogSeeDistance, global_fogFarDistance);
    {
            char s[255];
            int charsRendered = sprintf (s, "Player Pos: %f %f %f", gameState->player.T.pos.x, gameState->player.T.pos.y, gameState->player.T.pos.z);
            assert(charsRendered < arrayCount(s));
            renderText(gameState->renderer, &gameState->mainFont, s, make_float2(10, 5), 0.1f);
        }
    {
        char s[255];
        int charsRendered = sprintf (s, "Blocks Drawn: %d", gameState->DEBUG_BlocksDrawnForFrame);
        assert(charsRendered < arrayCount(s));
        renderText(gameState->renderer, &gameState->mainFont, s, make_float2(10, 10), 0.1f);
    }
    {
        char s[255];
        int charsRendered = sprintf (s, "Frame Rate: %dFPS", (int)(round((1.0f / gameState->dt))));
        assert(charsRendered < arrayCount(s));
        renderText(gameState->renderer, &gameState->mainFont, s, make_float2(10, 10 + 5), 0.1f);
    }
     

    {
        float occupiedBuckets = 0;
        float avgChainLength = 0;
        int maxChainLength = 0;
        for(int i = 0; i < arrayCount(gameState->chunks); ++i) {
            Chunk *chunk = gameState->chunks[i];
            if(chunk) {
                occupiedBuckets++;
                
            }

            int chainLength = 0;
            while(chunk) {
                avgChainLength++;
                chainLength++;
                chunk = chunk->next;
            }

            if(chainLength > maxChainLength) {
                maxChainLength = chainLength;
            }
        }
        avgChainLength = avgChainLength / occupiedBuckets;

        {
            char s[255];
            int charsRendered = sprintf (s, "Load Factor: %f", occupiedBuckets / (float)arrayCount(gameState->chunks));
            assert(charsRendered < arrayCount(s));
            renderText(gameState->renderer, &gameState->mainFont, s, make_float2(10, 10 + 10), 0.1f);
        }
        {
            char s[255];
            int charsRendered = sprintf (s, "Avg Chain Length: %f", avgChainLength);
            assert(charsRendered < arrayCount(s));
            renderText(gameState->renderer, &gameState->mainFont, s, make_float2(10, 10 + 15), 0.1f);
        }
        {
            char s[255];
            int charsRendered = sprintf (s, "Max Chain Length: %d", maxChainLength);
            assert(charsRendered < arrayCount(s));
            renderText(gameState->renderer, &gameState->mainFont, s, make_float2(10, 10 + 20), 0.1f);
        }
    }

    



    
}


void DEBUG_chunkTests(GameState *gameState) {
    Chunk *c = getChunk_(gameState, 3, 4, 5, true, false);
    Chunk *d = getChunk_(gameState, 8, 33, 5, true, false);
    Chunk *b = getChunk_(gameState, 8, 66, 5, true, false);

    assert(c);
    assert(b);
    assert(d);

    Chunk *e = getChunk_(gameState, 8, 33, 5, false, false);
    assert(e);
    Chunk *f = getChunk_(gameState, 8, 66, 5, false, false);
    assert(f);
    Chunk *g = getChunk_(gameState, 3, 4, 5, false, false);
    assert(g);

}

static SkeletalModel triangleHandle;
static Texture modelTexture;

void updateGame(GameState *gameState) {
    if(!gameState->inited) {
        globalLongTermArena = createArena(Kilobytes(200));
        globalPerFrameArena = createArena(Kilobytes(100));
        perFrameArenaMark = takeMemoryMark(&globalPerFrameArena);
        // DEBUG_chunkTests(gameState);

        // triangleHandle = loadGLTF("./models/fox/Fox.gltf");
        // modelTexture = loadTextureToGPU("./models/fox/Texture.png");

        triangleHandle = loadGLTF("./models/fox.gltf");
        modelTexture = loadTextureToGPU("./models/fox.png");

        // triangleHandle = loadGLTF("./models/horse.gltf");
        // modelTexture = loadTextureToGPU("./models/horse.png");
        
        // triangleHandle = loadGLTF("./models/avocado/Avocado.gltf");
        // modelTexture = loadTextureToGPU("./models/avocado/Avocado_baseColor.png");

        initGameState(gameState);
    } else { 
        releaseMemoryMark(&perFrameArenaMark);
        perFrameArenaMark = takeMemoryMark(&globalPerFrameArena);
    }
    gameState->DEBUG_BlocksDrawnForFrame = 0;
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

    float3 worldP = convertRealWorldToBlockCoords(gameState->camera.T.pos);

    float16 rot = eulerAnglesToTransform(gameState->player.T.rotation.y, gameState->player.T.rotation.x, gameState->player.T.rotation.z);
    float3 lookingAxis = make_float3(rot.E_[2][0], rot.E_[2][1], rot.E_[2][2]);

    gameState->modelLocation = gameState->camera.T.pos;
    gameState->cameraRotation = rot;

    if(!gameState->camera.followingPlayer) {
        worldP = convertRealWorldToBlockCoords(gameState->modelLocation);
    }
    
    int chunkX = roundChunkCoord(worldP.x / (float)CHUNK_DIM);
    int chunkY = roundChunkCoord(worldP.y / (float)CHUNK_DIM);
    int chunkZ = roundChunkCoord(worldP.z / (float)CHUNK_DIM);
    
    int chunkRadiusY = 3;
    int chunkRadiusXZ = 10; //TODO: This should be able to get to 64 at 60FPS

    ChunkListItem *chunkList = 0;

    for(int z = -chunkRadiusXZ; z <= chunkRadiusXZ; ++z) {
        for(int x = -chunkRadiusXZ; x <= chunkRadiusXZ; ++x) {
            for(int y = -chunkRadiusY; y <= chunkRadiusY; ++y) {
                Chunk *chunk = getChunk(gameState, chunkX + x, chunkY + y, chunkZ + z);
                if(chunk) {
                    Rect3f rect = make_rect3f_min_dim((chunkX + x)*CHUNK_DIM, (chunkY + y)*CHUNK_DIM, (chunkZ + z)*CHUNK_DIM, CHUNK_DIM, CHUNK_DIM, CHUNK_DIM);
                    if(rect3fInsideViewFrustrum(rect, gameState->modelLocation, gameState->cameraRotation, gameState->camera.fov, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE, gameState->aspectRatio_y_over_x)) 
                    {
                        if(chunk->modelBuffer.indexCount > 0) {
                            prepareChunkRender(&chunk->modelBuffer, &gameState->renderer->blockGreedyShader, gameState->renderer->terrainTextureHandle, screenT, cameraT, lookingAxis, gameState->renderer->underWater);
                        }

                        bool drewMesh = drawChunk(gameState, gameState->renderer, chunk);

                        if(chunk->alphaModelBuffer.indexCount > 0 && drewMesh) {
                            ChunkListItem *c = pushStruct(&globalPerFrameArena, ChunkListItem);
                            c->next = chunkList;
                            c->chunk = chunk;
                            chunkList = c;
                        }

                        if(chunk->modelBuffer.indexCount > 0) {
                            endChunkRender();
                        }
                    } 
                    
                } else {
                    int h = 0;
                }
            }
        }
    }

    //NOTE: Alpha blocks
    while(chunkList) {
        Chunk *chunk = chunkList->chunk;
        prepareChunkRender(&chunk->alphaModelBuffer, &gameState->renderer->blockGreedyShader, gameState->renderer->terrainTextureHandle, screenT, cameraT, lookingAxis, gameState->renderer->underWater);

        glDrawElements(GL_TRIANGLES, chunk->alphaModelBuffer.indexCount, GL_UNSIGNED_INT, 0); 
        renderCheckError();

        endChunkRender();

        chunkList = chunkList->next;
    }

    // NOTE: Draw the clouds
    // for(int z = -chunkRadiusXZ; z <= chunkRadiusXZ; ++z) {
    //     for(int x = -chunkRadiusXZ; x <= chunkRadiusXZ; ++x) {
    //         CloudChunk *chunk = getCloudChunk(gameState, chunkX + x, chunkZ + z);
    //         assert(chunk);
    //         drawCloudChunk(gameState, chunk);
    //     }
    // }

    drawHUD(gameState);
    updateAndDrawDebugCode(gameState);
    rendererFinish(gameState->renderer, screenT, cameraT, screenGuiT, textGuiT, lookingAxis, cameraTWithoutTranslation, timeOfDayValues, gameState->perlinTestTexture.handle);

    {
        int count = 0;
        ChunkVertexToCreate **infoPtr = &gameState->meshesToCreate;
        int maxLoopCount = 10;
        
        while(*infoPtr && count < maxLoopCount) {
            ChunkVertexToCreate *info = *infoPtr;
            if(info->ready) {
                processMeshData(info);
                //NOTE: Take off list
                *infoPtr = info->next;

                //NOTE: Add to free list
                info->next = gameState->meshesToCreateFreeList;
                gameState->meshesToCreateFreeList = info;
                
                count++;
            } else {
                infoPtr = &info->next;
            }
        }

    //     int totalCount = 0;
    //     ChunkVertexToCreate *info = gameState->meshesToCreate;
    //     while(info) {
    //         totalCount++;
    //         info = info->next;
    //     }

    //     if(totalCount == 0) {
    //         chunkRadiusY++;
    //         chunkRadiusXZ++;

    //         if(chunkRadiusXZ > 40) {
    //             chunkRadiusXZ = 40;
    //         }
    //         if(chunkRadiusY > 5) {
    //             chunkRadiusY = 5;
    //         }
    //     }
    }

    // printf("xz: %d y: %d\n", chunkRadiusXZ, chunkRadiusY);

    // {
    //     //NOTE: Draw all the examples on the gltf website
    //     gameState->player.animationState.animation.animation = &triangleHandle.animations[1];
    //     // printf("%s\n",triangleHandle.animations[1].name);
    //     buildSkinningMatrix(gameState, &triangleHandle, &gameState->player.animationState, ENTITY_FOX);
    //     // float scale = 0.01f;
    //     float scale = 1;
    //     float16 T = float16_scale(float16_identity(), make_float3(scale, scale, scale));
    //     T = float16_set_pos(T, gameState->modelLocation);
    //     pushModel(gameState->renderer, T, make_float4(1, 1, 1, 1));
    //     updateInstanceData(triangleHandle.modelBuffer.instanceBufferhandle, gameState->renderer->modelData, gameState->renderer->modelItemCount*sizeof(InstanceDataWithRotation));
    //     drawModels(&triangleHandle.modelBuffer, &gameState->renderer->skeletalModelShader, modelTexture.handle, gameState->renderer->modelItemCount, screenT, cameraT, lookingAxis, false, timeOfDayValues, 0, triangleHandle.modelBuffer.textureHandle);
    //     gameState->renderer->modelItemCount = 0;
    // }


    //NOTE: End Mouse interaction if release
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED && gameState->currentInteraction.isValid) {
        gameState->currentInteraction.isValid = false;
    }

    if(gameState->camera.followingPlayer) {
            gameState->modelLocation = make_float3(gameState->camera.T.pos.x, gameState->camera.T.pos.y, gameState->camera.T.pos.z);
            gameState->cameraRotation = rot;
    } 

    if(gameState->keys.keys[KEY_3] == MOUSE_BUTTON_PRESSED) {
        gameState->drawBlocks = !gameState->drawBlocks;
    }

    if(gameState->keys.keys[KEY_1] == MOUSE_BUTTON_PRESSED) {
        gameState->camera.followingPlayer = !gameState->camera.followingPlayer;
        if(gameState->camera.followingPlayer) {
            //NOTE: Reset the player to the camera position
            gameState->player.T.pos = minus_float3(gameState->camera.T.pos, gameState->cameraOffset);
        } 

        if(!gameState->camera.followingPlayer) {
            gameState->cameraRotation = rot;
        }   
        gameState->currentInventoryHotIndex = 0;
    }
    if(gameState->keys.keys[KEY_2] == MOUSE_BUTTON_PRESSED) {
        gameState->currentInventoryHotIndex = 1;
        gameState->useCameraMovement = !gameState->useCameraMovement;
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