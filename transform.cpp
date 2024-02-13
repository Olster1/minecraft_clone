struct TransformX {
    float3 pos; 
    float3 scale;
    float3 rotation;
};

float16 getModelToViewSpace(TransformX T) {
    float16 i = float16_identity();

    i = float16_scale(i, T.scale);
    i = float16_multiply(eulerAnglesToTransform(T.rotation.y, T.rotation.x, T.rotation.z), i);
    i = float16_set_pos(i, T.pos);

    return i;
}

float16 getCameraX(TransformX T) {
    float16 cameraT = float16_set_pos(float16_identity(), float3_negate(T.pos));
    float16 rotT = eulerAnglesToTransform(T.rotation.y, T.rotation.x, T.rotation.z);
    rotT = float16_transpose(rotT);
    cameraT = float16_multiply(rotT, cameraT);
    return cameraT;
}

float16 getCameraX_withoutTranslation(TransformX T) {
    float16 rotT = eulerAnglesToTransform(T.rotation.y, T.rotation.x, T.rotation.z);
    rotT = float16_transpose(rotT);
    return rotT;
}