//NOTE: Helper funciton since the simplex lib I'm using for 3d noise maps between -1 -> 1, and we want 0 -> 1
float mapSimplexNoiseTo01(float value) {
    value += 1;
    value *= 0.5f;

    assert(value >= 0 && value <= 1);

    return value;
}

bool isTreeLocation(int worldX, int worldZ) {
    float t = perlin2d(worldX, worldZ, 0.8f, 16);
    return t < 0.25f;
}

bool isBushLocation(int worldX, int worldZ) {
    float t = perlin2d(worldX, worldZ, 0.7f, 16);
    return t < 0.4f;
}

bool isBigBush(int worldX, int worldZ) {
    float t = perlin2d(worldX, worldZ, 0.9f, 16);
    return t < 0.5f;
}

bool isIronLocation(int worldX, int worldY, int worldZ) {
    //NOTE: Bigger frequency noise overlay
    float t0 = SimplexNoise_fractal_3d(16, worldX, worldY, worldZ, 0.007143);
    t0 = mapSimplexNoiseTo01(t0);
    
    //NOTE: Smaller frequency for individual iron blocks 
    float t = SimplexNoise_fractal_3d(16, worldX, worldY, worldZ, 0.128572);
    t = mapSimplexNoiseTo01(t);

    t = t * t0;
    return t < 0.135714;
}