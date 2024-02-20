#define GRAVITY_POWER 28
#define JUMP_POWER 6
#define MAX_SHAKE_TIMER 0.4f
#define EPSILON_VALUE 0.2f
#define WALK_SPEED 2
#define CIRCLE_RADIUS_MAX 10
#define SHOW_CIRCLE_DELAY 2

float3 getBlockWorldPos(BlockChunkPartner b) {
    float3 p = {};

    p.x = b.block->x + (CHUNK_DIM*b.chunk->x);
    p.y = b.block->y + (CHUNK_DIM*b.chunk->y);
    p.z = b.block->z + (CHUNK_DIM*b.chunk->z);

    return p;
}

BlockChunkPartner blockExists(GameState *gameState, int worldx, int worldy, int worldz) {
    BlockChunkPartner found = {};
    found.block = 0;

    int chunkX = worldx / CHUNK_DIM;
    int chunkY = worldy / CHUNK_DIM;
    int chunkZ = worldz / CHUNK_DIM;

    int localx = worldx - (CHUNK_DIM*chunkX); 
    int localy = worldy - (CHUNK_DIM*chunkY); 
    int localz = worldz - (CHUNK_DIM*chunkZ); 
    
    Chunk *c = getChunk(gameState, chunkX, chunkY, chunkZ);
    assert(c);
    found.chunk = c;

    if(c) {
            assert(localx < CHUNK_DIM);
            assert(localy < CHUNK_DIM);
            assert(localz < CHUNK_DIM);
            int blockIndex = getBlockIndex(localx, localy, localz);
            assert(blockIndex < arrayCount(c->blocks));
            if(blockIndex < arrayCount(c->blocks) && c->blocks[blockIndex].exists) {
                // c->blocks[blockIndex].hitBlock = true;
                found.block = &c->blocks[blockIndex];
                found.blockIndex = blockIndex;
            } else {
                // found.block = 0;
            }
    }

    return found;
}

BlockChunkPartner castRayAgainstBlock(GameState *gameState, float3 dir, float length, float3 start) {
    int blockRadius = length;
    float shortestT = FLT_MAX;
    BlockChunkPartner block = {};
    block.block = 0;

    for(int z = -blockRadius; z <= blockRadius; ++z) {
        for(int x = -blockRadius; x <= blockRadius; ++x) {
            for(int y = -blockRadius; y <= blockRadius; ++y) {
                float3 pos = start;
                //NOTE: Get the block pos in world
                int worldx = (int)pos.x + x;
                int worldy = (int)pos.y + y;
                int worldz = (int)pos.z + z;

                BlockChunkPartner blockTemp = blockExists(gameState, worldx, worldy, worldz);

                if(blockTemp.block) {
                    //NOTE: Ray Cast against the block
                    Rect3f b = make_rect3f_center_dim(make_float3(worldx, worldy, worldz), make_float3(1, 1, 1));
                    float3 hitPoint;
                    float3 normalVector;
                    float tAt;
                    if(easyMath_rayVsAABB3f(pos, dir, b, &hitPoint, &tAt, &normalVector)) {
                        if(tAt <= length && tAt < shortestT) {
                            shortestT = tAt;
                            block = blockTemp;
                            block.sideNormal = normalVector;
                        }
                    }
                }
            }
        }
    }

    return block;
}

void updatePlayerPhysics(GameState *gameState, Entity *e, float3 movementForFrame) {
    int physicsIterations = 4;
    bool didHit = false;

    //NOTE: Start the frame not grounded
    e->grounded = false;

    //NOTE: GRAVITY
    if(gameState->gravityOn) {
        e->dP.y -= GRAVITY_POWER*gameState->dt;
    }

    //NOTE: Integrate velocity
    // e->dP = plus_float3(e->dP, accelForFrame); //NOTE: Already * by dt 

    //NOTE: Apply drag
    // e->dP = scale_float3(0.95f, e->dP);

    //NOTE: Get the movement vector for this frame
    float3 dpVector = plus_float3(movementForFrame, scale_float3(gameState->dt, e->dP));
    // float lengthLeft = float3_magnitude(dpVector);

    // printf("%f %f %f\n", dpVector.x, dpVector.y, dpVector.z);

    for(int i = 0; i < physicsIterations; ++i) {
        // dpVector = normalize_float3(dpVector);
        BlockChunkPartner blockHit = {};
        blockHit.block = 0;
        
        float radius = float3_magnitude(dpVector);

        int blockRadius = maxi(ceil(radius + 1.5f), 5);

        float shortestT = FLT_MAX;
        bool hit = false;
        float3 shortestNormalVector = make_float3(0, 0, 0);

        for(int z = -blockRadius; z <= blockRadius; ++z) {
            for(int x = -blockRadius; x <= blockRadius; ++x) {
                for(int y = -blockRadius; y <= blockRadius; ++y) {
                    float3 pos = e->T.pos;
                    //NOTE: Get the block pos in world
                    int worldx = (int)pos.x + x;
                    int worldy = (int)pos.y + y;
                    int worldz = (int)pos.z + z;
                    
                    BlockChunkPartner blockPtr = blockExists(gameState, worldx, worldy, worldz);
                    if(blockPtr.block) {
                        //NOTE: Ray Cast against the block
                        Rect3f block = make_rect3f_center_dim(make_float3(worldx, worldy, worldz), make_float3(1, 1, 1));
                        Rect3f playerB = make_rect3f_center_dim(make_float3(0, 0, 0), e->T.scale);
                        Rect3f b = rect3f_minowski_plus(playerB, block, make_float3(worldx, worldy, worldz));
                        float3 hitPoint;
                        float3 normalVector;
                        float tAt;
                        if(easyMath_rayVsAABB3f(pos, dpVector, b, &hitPoint, &tAt, &normalVector)) {
                            
                            float slopeFactor = 0.3f;
                            float distToGround = 0.1f;
                            if(tAt < distToGround && float3_dot(make_float3(0, 1, 0), normalVector) > slopeFactor) {
                                e->grounded = true;
                            }

                            if(tAt <= float3_magnitude(dpVector) && tAt < shortestT) {
                                // printf("NORMAL: %f %f %f\n", normalVector.x, normalVector.y, normalVector.z);
                                // assert(normalVector.x != 0 || normalVector.y != 0 || normalVector.z != 0);
                                shortestT = tAt;
                                shortestNormalVector = normalVector;
                                hit = true;
                                blockHit = blockPtr;
                            }
                        }
                    }
                }
            }
        }

        //NOTE: Update player movement
        float3 moveVector = make_float3(0, 0, 0);
        if(hit) {
            float epsilonValue = EPSILON_VALUE;
            float moveFactor = fmax(0, shortestT - epsilonValue);
            moveVector = scale_float3(moveFactor, normalize_float3(dpVector));
        } else {
            moveVector = dpVector;
        }
        
        e->T.pos = plus_float3(e->T.pos, moveVector);

        if(hit) {
            didHit = true;

            if(e->dP.y < -20 && float3_dot(make_float3(0, 1, 0), shortestNormalVector) > 0.0f) {
                //NOTE: Big fall so shake the camera
                gameState->camera.shakeTimer = MAX_SHAKE_TIMER;
                playSound(&gameState->fallBigSound);
            }

            if(float3_dot(make_float3(1, 0, 0), shortestNormalVector) > 0.0f 
            || float3_dot(make_float3(-1, 0, 0), shortestNormalVector) > 0.0f
            || float3_dot(make_float3(0, 0, -1), shortestNormalVector) > 0.0f
            || float3_dot(make_float3(0, 0, 1), shortestNormalVector) > 0.0f) {

                if(blockHit.block && blockHit.chunk) {
                    if(!blockExistsReadOnly(gameState, blockHit.block->x + CHUNK_DIM*blockHit.chunk->x, blockHit.block->y + CHUNK_DIM*blockHit.chunk->y + 1, blockHit.block->z + CHUNK_DIM*blockHit.chunk->z)) {
                        e->tryJump = true;
                    }
                }
            }

            //NOTE: Update dpVector & total move time
            float3 d = scale_float3(float3_dot(e->dP, shortestNormalVector), shortestNormalVector);
            e->dP = minus_float3(e->dP, d);

            d = scale_float3(float3_dot(dpVector, shortestNormalVector), shortestNormalVector);
            dpVector = minus_float3(dpVector, d);

            // printf("%f %f %f\n", shortestNormalVector.x, shortestNormalVector.y, shortestNormalVector.z);
            
            
        }  
    }
}

void cancelMiningInteraction(GameState *gameState) {
    gameState->currentMiningBlock->timeLeft = gameState->currentMiningBlock->maxTime;
    gameState->currentMiningBlock = 0;

    if(gameState->miningSoundPlaying) {
        gameState->miningSoundPlaying->shouldEnd = true;
        gameState->miningSoundPlaying = 0;
    }
}

void placeBlock(GameState *gameState, float3 lookingAxis, Entity *e) {
    BlockChunkPartner b = castRayAgainstBlock(gameState, lookingAxis, 4, plus_float3(gameState->cameraOffset, e->T.pos));

    if(b.block) {
        // b.block->hitBlock = true;
        int worldX = b.sideNormal.x + b.block->x + (CHUNK_DIM*b.chunk->x);
        int worldY = b.sideNormal.y + b.block->y + (CHUNK_DIM*b.chunk->y);
        int worldZ = b.sideNormal.z + b.block->z + (CHUNK_DIM*b.chunk->z);

        BlockChunkPartner nxtBlock = blockExists(gameState, worldX, worldY, worldZ);

        if(!nxtBlock.block) {
            float c = 1 + EPSILON_VALUE;
            Rect3f blockBounds = make_rect3f_center_dim(make_float3(0, 0, 0), make_float3(c, c, c));
            blockBounds = rect3f_minowski_plus(blockBounds, make_rect3f_center_dim(make_float3(0, 0, 0), e->T.scale), make_float3(worldX, worldY, worldZ));
            if(!in_rect3f_bounds(blockBounds, e->T.pos)) {
                //NOTE: place block
                if(nxtBlock.chunk) {
                    int localX = worldX - (CHUNK_DIM*nxtBlock.chunk->x); 
                    int localY = worldY - (CHUNK_DIM*nxtBlock.chunk->y); 
                    int localZ = worldZ - (CHUNK_DIM*nxtBlock.chunk->z); 

                    int blockIndex = getBlockIndex(localX, localY, localZ);
                    if(blockIndex < arrayCount(nxtBlock.chunk->blocks)) {
                        nxtBlock.chunk->blocks[blockIndex] = spawnBlock(localX, localY, localZ, BLOCK_GRASS);
                    } else {
                        assert(false);
                    }

                    playSound(&gameState->blockFinishSound);
                }
            }
        }
    }
}

void highlightBlockLookingAt(GameState *gameState, float3 lookingAxis, Entity *e) {
    BlockChunkPartner b = castRayAgainstBlock(gameState, lookingAxis, 4, plus_float3(gameState->cameraOffset, e->T.pos));

    if(b.block) {
        b.block->hitBlock = true;
    }
}


void mineBlock(GameState *gameState, float3 lookingAxis, Entity *e) {
     BlockChunkPartner b = castRayAgainstBlock(gameState, lookingAxis, 4, plus_float3(gameState->cameraOffset, e->T.pos));

        if(b.block) {
            //NOTE: Play sound
            if(!gameState->miningSoundPlaying) {
                gameState->miningSoundPlaying = playSound(&gameState->blockBreakSound);
                gameState->miningSoundPlaying->nextSound = gameState->miningSoundPlaying;
            }

            b.block->hitBlock = true;
            b.block->timeLeft -= gameState->dt;

            //NOTE: Show progress on mining the block
            float percent = 1.0f - fmax(0, b.block->timeLeft / b.block->maxTime);

            pushAlphaItem(gameState->renderer, getBlockWorldPos(b), make_float3(1.001f, 1.001f, 1.001f), make_float4(1, 1, 1, 0.7f), percent);

            gameState->showCircleTimer = 0;
            pushFillCircle(gameState->renderer, make_float3(0, 0, 0), CIRCLE_RADIUS_MAX*percent, make_float4(1, 1, 1, 1));
            
            if(b.block->timeLeft <= 0) {
                //NOTE: Add block to pickup 
                initPickupItem(b.chunk, getBlockWorldPos(b), b.block->type, gameState->randomStartUpID);

                //NOTE: Destory the block
                b.chunk->blocks[b.blockIndex].exists = false;

                playSound(&gameState->blockFinishSound);

                //NOTE: Clear the block
                b.block = 0;
            }
        }

        if(gameState->currentMiningBlock && gameState->currentMiningBlock != b.block) {
            //NOTE: Reset if we were mining another block
            cancelMiningInteraction(gameState);
        }

        gameState->currentMiningBlock = b.block;
}

void updatePlayer(GameState *gameState, Entity *e) {
    float rotSpeed = 13.0f;

    float2 mouseDelta = minus_float2(gameState->mouseP_screenSpace, gameState->lastMouseP);

    e->T.rotation.y += gameState->dt*rotSpeed*-mouseDelta.x;
    e->T.rotation.x += gameState->dt*rotSpeed*-mouseDelta.y;

    if(e->T.rotation.x > 89) {
        e->T.rotation.x = 89;
    }
    if(e->T.rotation.x < -89) {
        e->T.rotation.x = -89;
    }

    float16 rot = eulerAnglesToTransform(e->T.rotation.y, e->T.rotation.x, e->T.rotation.z);

    float3 zAxis = make_float3(rot.E_[2][0], 0, rot.E_[2][2]); //NOTE: rot.E_[2][1] in y if we want to fly
    zAxis = normalize_float3(zAxis);
    float3 xAxis = make_float3(rot.E_[0][0], rot.E_[0][1], rot.E_[0][2]);

    float3 movement = make_float3(0, 0, 0);

    float magnitude = gameState->dt*WALK_SPEED;

    if(gameState->keys.keys[KEY_LEFT]) {
        movement = plus_float3(movement, float3_negate(xAxis));
    }
    if(gameState->keys.keys[KEY_RIGHT]) {
        movement = plus_float3(movement, xAxis);
    }
    if(gameState->keys.keys[KEY_DOWN]) {
        movement = plus_float3(movement, float3_negate(zAxis));
    }
    if(gameState->keys.keys[KEY_UP]) {
        movement = plus_float3(movement, zAxis);
    }

    movement = normalize_float3(movement);
    movement = scale_float3(magnitude, movement);
    
    if((gameState->keys.keys[KEY_SPACE] == MOUSE_BUTTON_PRESSED || e->tryJump) && (e->grounded || !gameState->gravityOn)) {
        //NOTE: JUMP
        e->dP.y += JUMP_POWER;
    }
    e->tryJump = false;
    
    updatePlayerPhysics(gameState, e, movement);

    float3 lookingAxis = make_float3(rot.E_[2][0], rot.E_[2][1], rot.E_[2][2]);

    if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
        gameState->placeBlockTimer = 0;
        gameState->mineBlockTimer = 0;
    }
    
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN) {
        gameState->showCircleTimer = 0;
        
       if(gameState->placeBlockTimer >= 0) {
            gameState->placeBlockTimer += gameState->dt;
        }

        if(gameState->mineBlockTimer >= 0) {
            gameState->mineBlockTimer += gameState->dt;
        }

        if(gameState->mineBlockTimer > 0.45f) {
            //NOTE: Placing a block down
            mineBlock(gameState, lookingAxis, e);
        }
    }

    if(gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED) {
        if(gameState->placeBlockTimer < 0.2f) {
            //NOTE: Placing a block down
            placeBlock(gameState, lookingAxis, e);
        }

        if(gameState->currentMiningBlock) {
            cancelMiningInteraction(gameState);
        }

        gameState->placeBlockTimer = -1;
        gameState->mineBlockTimer = -1;
    }

    if(gameState->showCircleTimer >= 0) {
        highlightBlockLookingAt(gameState, lookingAxis, e);
        pushCircleOutline(gameState->renderer, make_float3(0, 0, 0), CIRCLE_RADIUS_MAX, make_float4(1, 1, 1, 1));
        
        if(gameState->mouseLeftBtn == MOUSE_BUTTON_NONE) {
            gameState->showCircleTimer += gameState->dt;
            if(gameState->showCircleTimer >= SHOW_CIRCLE_DELAY) {
                //NOTE: Finish the timer
                gameState->showCircleTimer = -1;
            }
        }
    }
    

}