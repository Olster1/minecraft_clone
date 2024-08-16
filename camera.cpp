void updateCamera(GameState *gameState) {
    if(!gameState->camera.followingPlayer) {
        float speed = 10;
        float rotSpeed = 13.0f;

        float2 mouseDelta = minus_float2(gameState->mouseP_screenSpace, gameState->lastMouseP) ;

        gameState->camera.T.rotation.y += gameState->dt*rotSpeed*-mouseDelta.x;
        gameState->camera.T.rotation.x += gameState->dt*rotSpeed*-mouseDelta.y;

        float16 rot = eulerAnglesToTransform(gameState->camera.T.rotation.y, gameState->camera.T.rotation.x, gameState->camera.T.rotation.z);

        float3 zAxis = make_float3(rot.E_[2][0], rot.E_[2][1], rot.E_[2][2]);
        float3 xAxis = make_float3(rot.E_[0][0], rot.E_[0][1], rot.E_[0][2]);

        if(gameState->keys.keys[KEY_LEFT]) {
            gameState->camera.T.pos = plus_float3(gameState->camera.T.pos, scale_float3(-gameState->dt*speed, xAxis));
        }
        if(gameState->keys.keys[KEY_RIGHT]) {
            gameState->camera.T.pos = plus_float3(gameState->camera.T.pos, scale_float3(gameState->dt*speed, xAxis));
        }
        if(gameState->keys.keys[KEY_DOWN]) {
            gameState->camera.T.pos = plus_float3(gameState->camera.T.pos, scale_float3(-gameState->dt*speed, zAxis));
        }
        if(gameState->keys.keys[KEY_UP]) {
            gameState->camera.T.pos = plus_float3(gameState->camera.T.pos, scale_float3(gameState->dt*speed, zAxis));
        }
    } else {
        gameState->camera.T = gameState->player.T;
        gameState->camera.T.pos = plus_float3(gameState->cameraOffset, gameState->camera.T.pos);

        float16 rot = eulerAnglesToTransform(gameState->camera.T.rotation.y, gameState->camera.T.rotation.x, gameState->camera.T.rotation.z);

        float3 yAxis = make_float3(rot.E_[1][0], rot.E_[1][1], rot.E_[1][2]);
        float3 xAxis = make_float3(rot.E_[0][0], rot.E_[0][1], rot.E_[0][2]);

        if(gameState->camera.shakeTimer >= 0) {
            gameState->camera.shakeTimer -= gameState->dt;

            if(gameState->camera.shakeTimer < 0) {
                //NOTE: Stop the shaking
                gameState->camera.shakeTimer = -1;
                gameState->cameraOffset = CAMERA_OFFSET;
            } else {
                float randomOffset = ((float)rand() / RAND_MAX) * 100; 
                float randomYOffset = ((float)rand() / RAND_MAX) * 50; 

                float tx = gameState->camera.shakeTimer*100 + randomOffset;
                float ty = gameState->camera.shakeTimer*100 + randomYOffset + randomOffset;

                float px = perlin1d(tx, 10, 4);
                float py = perlin1d(ty, 10, 4);

                float x = lerp(-1, 1, make_lerpTValue(px)) * (gameState->camera.shakeTimer);
                float y = lerp(-1, 1, make_lerpTValue(py)) * (gameState->camera.shakeTimer);

                gameState->cameraOffset = plus_float3(plus_float3(CAMERA_OFFSET, scale_float3(y, yAxis)), scale_float3(x, xAxis));
            }
        }

        if(gameState->player.running && gameState->camera.runShakeTimer >= 0) {
            //NOTE: Do the rotation around z
            gameState->camera.T.rotation.z += (float)sin(20*gameState->camera.runShakeTimer);
            gameState->camera.runShakeTimer += gameState->dt;
        } 
    }

    gameState->camera.fov = lerp(gameState->camera.fov, gameState->camera.targetFov, make_lerpTValue(0.4f));
}