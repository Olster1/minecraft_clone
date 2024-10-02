float gui_drawSlider(GameState *gameState, GuiState *guiState, Renderer *renderer, char *id, float value, float yOffset = 0) {
    float width = 20;
    float centerX = 40;
    float centerY = 40 - yOffset;
    pushPlainQuadHUD(renderer, make_float3(centerX, centerY, 1), make_float2(width, 1), make_float4(1, 1, 1, 1));

    float startX = (centerX - 0.5f*width);

    uint32_t hashId = get_crc32_for_string(id);

    float2 mouse = make_float2(lerp(-50, 50, make_lerpTValue(gameState->mouseP_01.x)), lerp(50, -50, make_lerpTValue(-1*gameState->mouseP_01.y)));

    float3 handleP = make_float3(startX + (value*width), centerY, 1);
    float2 handleSize = make_float2(2, 2);
    float4 color = make_float4(1, 0, 0, 1);

    Rect2f bounds = make_rect2f_center_dim(handleP.xy, handleSize);
    if(in_rect2f_bounds(bounds, mouse)) {
        color = make_float4(1, 0, 1, 1);
        if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
            guiState->grabbedId = id;
            guiState->grabbedHash = hashId;
        }
    }

    if(guiState->grabbedId && guiState->grabbedHash == hashId) {
        if(easyString_stringsMatch_nullTerminated(guiState->grabbedId, id)) {
            //NOTE: Is already holding it
            float x = mouse.x - startX;
            x = x / width;
            x = clamp(0, 1, x);
            value = x;
            color = make_float4(1, 1, 0, 1);
        }
    }

    //NOTE: Handle
    pushFillCircle(renderer, handleP, handleSize.x, color);
   
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED) {
        guiState->grabbedId = 0;
    }
    
    return value;
}