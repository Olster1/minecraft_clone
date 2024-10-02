
void perlinNoiseDrawTests(GameState *gameState, int originWorldX, int originWorldZ) {
    deleteTexture(&gameState->perlinTestTexture);

    uint32_t perlinBuffer[PERLIN_SIZE*PERLIN_SIZE];
    for(int i = 0; i < PERLIN_SIZE; ++i) {
        for(int j = 0; j < PERLIN_SIZE; ++j) {
            int worldX = originWorldX + j;
            int worldZ = originWorldZ + i;

            float t = getTerrainHeight(worldX, worldZ);
            uint8_t color = (255 * (t / 100));
            uint32_t v = (0xFF << 24) | (color << 16) | (color << 8) | (color << 0);
            perlinBuffer[(i*PERLIN_SIZE) + j] = v;
        }
    }

    gameState->perlinTestTexture = createGPUTexture(PERLIN_SIZE, PERLIN_SIZE, perlinBuffer);
}