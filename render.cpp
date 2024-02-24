#define MAX_CUBES_PER_RENDER 150000000
#define MAX_CIRCLES_PER_RENDER 32
#define MAX_RENDER_ITEMS_PER_INSTANCE 32
#define MAX_WORLD_ITEMS_PER_INSTANCE 1024

struct Vertex {
    float3 pos;
    float2 texUV;
    float3 normal;
};

Vertex makeVertex(float3 pos, float2 texUV, float3 normal) {
    Vertex v = {};
    
    v.pos = pos;
    v.texUV = texUV;
    v.normal = normal;

    return v;
}

static Vertex global_quadData[] = {
    makeVertex(make_float3(0.5f, -0.5f, 0), make_float2(1, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(-0.5f, -0.5f, 0), make_float2(0, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(-0.5f,  0.5f, 0), make_float2(0, 0), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, 0.5f, 0), make_float2(1, 0), make_float3(0, 0, 1)),
};

static unsigned int global_quadIndices[] = {
    0, 1, 2, 0, 2, 3,
};

static Vertex global_cubeData[] = {
    // Top face (y = 1.0f)
    makeVertex(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0.25f,1), make_float3(0, 1, 0)),
    makeVertex(make_float3(0.5f, 0.5f, -0.5f), make_float2(0.25f, 0), make_float3(0, 1, 0)),
    makeVertex(make_float3(0.5f, 0.5f,  0.5f), make_float2(0.5f, 0), make_float3(0, 1, 0)),
    makeVertex(make_float3(-0.5f, 0.5f,  0.5f), make_float2(0.5f, 1), make_float3(0, 1, 0)),
    // Bottom face (y = -1.0f)
    makeVertex(make_float3(0.5f, -0.5f, -0.5f), make_float2(0.75f, 0), make_float3(0, -1, 0)),
    makeVertex(make_float3(-0.5f, -0.5f, -0.5f), make_float2(0.75f, 1), make_float3(0, -1, 0)),
    makeVertex(make_float3(-0.5f, -0.5f,  0.5f), make_float2(0.5f, 1), make_float3(0, -1, 0)),
    makeVertex(make_float3(0.5f, -0.5f,  0.5f), make_float2(0.5f, 0), make_float3(0, -1, 0)),
    // Front face  (z = 1.0f)
    makeVertex(make_float3(-0.5f, -0.5f, 0.5f), make_float2(0, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, -0.5f, 0.5f), make_float2(0.25f,1), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, 0.5f, 0.5f), make_float2(0.25f,0), make_float3(0, 0, 1)),
    makeVertex(make_float3(-0.5f, 0.5f, 0.5f), make_float2(0, 0), make_float3(0, 0, 1)),
    // Back face (z = -1.0f)
    makeVertex(make_float3(0.5f, -0.5f, -0.5f), make_float2(0,1), make_float3(0, 0, -1)),
    makeVertex(make_float3(-0.5f, -0.5f, -0.5f), make_float2(0.25f,1), make_float3(0, 0, -1)),
    makeVertex(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0.25f,0), make_float3(0, 0, -1)),
    makeVertex(make_float3(0.5f, 0.5f, -0.5f), make_float2(0,0), make_float3(0, 0, -1)),
    // Left face (x = -1.0f)
    makeVertex(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0, 0), make_float3(-1, 0, 0)),
    makeVertex(make_float3(-0.5f, -0.5f, -0.5f), make_float2(0, 1), make_float3(-1, 0, 0)),
    makeVertex(make_float3(-0.5f, -0.5f, 0.5f), make_float2(0.25f,1), make_float3(-1, 0, 0)),
    makeVertex(make_float3(-0.5f, 0.5f, 0.5f), make_float2(0.25f,0), make_float3(-1, 0, 0)),
    // Right face (x = 1.0f)
    makeVertex(make_float3(0.5f, -0.5f, -0.5f), make_float2(0.25f,1), make_float3(1, 0, 0)),
    makeVertex(make_float3(0.5f, 0.5f, -0.5f), make_float2(0.25f,0), make_float3(1, 0, 0)),
    makeVertex(make_float3(0.5f, 0.5f, 0.5f), make_float2(0, 0), make_float3(1, 0, 0)),
    makeVertex(make_float3(0.5f, -0.5f, 0.5f), make_float2(0, 1), make_float3(1, 0, 0)),
};


static Vertex global_cubeData_sameTexture[] = {
    // Top face (y = 1.0f)
    makeVertex(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0,1), make_float3(0, 1, 0)),
    makeVertex(make_float3(0.5f, 0.5f, -0.5f), make_float2(0, 0), make_float3(0, 1, 0)),
    makeVertex(make_float3(0.5f, 0.5f,  0.5f), make_float2(1, 0), make_float3(0, 1, 0)),
    makeVertex(make_float3(-0.5f, 0.5f,  0.5f), make_float2(1, 1), make_float3(0, 1, 0)),
    // Bottom face (y = -1.0f)
    makeVertex(make_float3(0.5f, -0.5f, -0.5f), make_float2(1, 0), make_float3(0, -1, 0)),
    makeVertex(make_float3(-0.5f, -0.5f, -0.5f), make_float2(1, 1), make_float3(0, -1, 0)),
    makeVertex(make_float3(-0.5f, -0.5f,  0.5f), make_float2(0, 1), make_float3(0, -1, 0)),
    makeVertex(make_float3(0.5f, -0.5f,  0.5f), make_float2(0, 0), make_float3(0, -1, 0)),
    // Front face  (z = 1.0f)
    makeVertex(make_float3(-0.5f, -0.5f, 0.5f), make_float2(0, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, -0.5f, 0.5f), make_float2(1,1), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, 0.5f, 0.5f), make_float2(1,0), make_float3(0, 0, 1)),
    makeVertex(make_float3(-0.5f, 0.5f, 0.5f), make_float2(0, 0), make_float3(0, 0, 1)),
    // Back face (z = -1.0f)
    makeVertex(make_float3(0.5f, -0.5f, -0.5f), make_float2(0,1), make_float3(0, 0, -1)),
    makeVertex(make_float3(-0.5f, -0.5f, -0.5f), make_float2(1,1), make_float3(0, 0, -1)),
    makeVertex(make_float3(-0.5f, 0.5f, -0.5f), make_float2(1,0), make_float3(0, 0, -1)),
    makeVertex(make_float3(0.5f, 0.5f, -0.5f), make_float2(0,0), make_float3(0, 0, -1)),
    // Left face (x = -1.0f)
    makeVertex(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0, 0), make_float3(-1, 0, 0)),
    makeVertex(make_float3(-0.5f, -0.5f, -0.5f), make_float2(0, 1), make_float3(-1, 0, 0)),
    makeVertex(make_float3(-0.5f, -0.5f, 0.5f), make_float2(1,1), make_float3(-1, 0, 0)),
    makeVertex(make_float3(-0.5f, 0.5f, 0.5f), make_float2(1,0), make_float3(-1, 0, 0)),
    // Right face (x = 1.0f)
    makeVertex(make_float3(0.5f, -0.5f, -0.5f), make_float2(1,1), make_float3(1, 0, 0)),
    makeVertex(make_float3(0.5f, 0.5f, -0.5f), make_float2(1,0), make_float3(1, 0, 0)),
    makeVertex(make_float3(0.5f, 0.5f, 0.5f), make_float2(0, 0), make_float3(1, 0, 0)),
    makeVertex(make_float3(0.5f, -0.5f, 0.5f), make_float2(0, 1), make_float3(1, 0, 0)),
};

static unsigned int global_cubeIndices[] = {
    0, 1, 2, 0, 2, 3,
    4, 5, 6, 4, 6, 7,
    8, 9, 10, 8, 10, 11,
    12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19,
    20, 21, 22, 20, 22, 23,
};


struct Shader {
    bool valid;
    uint32_t handle;
};

struct ModelBuffer {
    uint32_t handle;
    uint32_t instanceBufferhandle;
    int indexCount;
};

struct InstanceData {
    float3 pos;
    float2 uv;
    float4 color;
    float3 scale;
    uint32_t AOMask[2];
    uint32_t samplerIndex;
};

struct InstanceDataWithRotation {
    float16 M;
    float4 color;
    float4 uv;
};

struct Renderer {
    uint32_t terrainTextureHandle;
    uint32_t circleHandle;
    uint32_t circleOutlineHandle;
    uint32_t skyboxTextureHandle;
    uint32_t breakBlockTexture;
    uint32_t atlasTexture;

    int cubeCount;
    InstanceData cubeData[MAX_CUBES_PER_RENDER];

    int atlasQuadCount;
    InstanceDataWithRotation atlasQuads[MAX_WORLD_ITEMS_PER_INSTANCE];

    int atlasQuadHUDCount;
    InstanceDataWithRotation atlasHUDQuads[MAX_CIRCLES_PER_RENDER];

    int triangleCount;
    InstanceData triangleData[MAX_RENDER_ITEMS_PER_INSTANCE];

    int blockItemsCount; //NOTE: The blocks that are rotating
    InstanceDataWithRotation blockItemsData[MAX_WORLD_ITEMS_PER_INSTANCE];

    int alphaBlockCount; //NOTE: The blocks that are rotating
    InstanceData alphaBlockData[MAX_CUBES_PER_RENDER];

    int alphaItemCount; //NOTE: The blocks that are rotating
    InstanceDataWithRotation alphaItemData[MAX_WORLD_ITEMS_PER_INSTANCE];

    Shader blockShader;
    Shader blockPickupShader;
    Shader quadShader;
    Shader quadTextureShader;
    Shader skyboxShader;
    Shader blockColorShader;
    Shader blockSameTextureShader;
    
    ModelBuffer blockModel;
    ModelBuffer blockModelWithInstancedT;
    ModelBuffer quadModel;
    ModelBuffer triangleModel;
    ModelBuffer avocadoModel;
    ModelBuffer blockModelSameTexture;
};


InstanceDataWithRotation *pushAtlasQuad_(Renderer *renderer, float3 worldP, float3 scale, float4 uvs, float4 color, bool isHUD) {
    InstanceDataWithRotation *c = 0;
    if(isHUD) {
        if(renderer->atlasQuadHUDCount < arrayCount(renderer->atlasHUDQuads)) {
            c = &renderer->atlasHUDQuads[renderer->atlasQuadHUDCount++];
        }   
    } else {
        if(renderer->atlasQuadCount < arrayCount(renderer->atlasQuads)) {
            c = &renderer->atlasQuads[renderer->atlasQuadCount++];
        }
    }
    
    if(c) {
        c->M = float16_set_pos(float16_scale(float16_identity(), scale), worldP);
        c->color = color;
        c->uv = uvs;
    }

    return c;
}

void pushGrassQuad(Renderer *renderer, float3 worldP, float height, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(1, height, 1), make_float4(0, 0.25f, 0, 0.25f), color, false);
}

void pushHUDOutline(Renderer *renderer, float3 worldP, float2 scale, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(scale.x, scale.y, 1), make_float4(0.0, 0.25f, 0.25f, 0.5f), color, true);
}

void pushCircleOutline(Renderer *renderer, float3 worldP, float radius, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(radius, radius, 1), make_float4(0.5f, 1.0f, 0, 0.5f), color, true);
}

void pushFillCircle(Renderer *renderer, float3 worldP, float radius, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(radius, radius, 1), make_float4(0, 0.5f, 0.5f, 1.0f), color, true);
}

float2 getUVCoordForBlock(BlockType type) {
    float2 uv = make_float2(0, 0);
     if(type == BLOCK_GRASS) {
        uv.x = 0;
        uv.y = 0.2f;
    } else if(type == BLOCK_SOIL) {
        uv.x = 0.2f;
        uv.y = 0.4f;
    } else if(type == BLOCK_STONE) {
        uv.x = 0.4f;
        uv.y = 0.6f;
    } else if(type == BLOCK_TREE_LEAVES) {
        uv.x = 0.6f;
        uv.y = 0.8f;
    } else if(type == BLOCK_TREE_WOOD) {
        uv.x = 0.8f;
        uv.y = 1.0f;
    }
    return uv;
}

void pushCube(Renderer *renderer, float3 worldP, BlockType type, float4 color, uint64_t AOMask) {
    if(renderer->cubeCount < arrayCount(renderer->cubeData)) {
        InstanceData *cube = &renderer->cubeData[renderer->cubeCount++];

        cube->pos = worldP;
        uint32_t samplerIndex = 0;
        cube->uv = getUVCoordForBlock(type);

        cube->samplerIndex = samplerIndex;

        cube->color = color;
        
        //NOTE: Convert the AO mask into two integers since openGL doesn't support uint64 in the shader attributes
        cube->AOMask[0] = (uint32_t)(AOMask);
        cube->AOMask[1] = (uint32_t)(AOMask >> 32);
    } else {
        assert(false);
    }
}

void pushAlphaCube(Renderer *renderer, float3 worldP, BlockType type, float4 color) {
    if(renderer->alphaBlockCount < arrayCount(renderer->alphaBlockData)) {
        InstanceData *cube = &renderer->alphaBlockData[renderer->alphaBlockCount++];

        cube->pos = worldP;
        cube->color = color;
    } else {
        assert(false);
    }
}

void pushAlphaItem(Renderer *renderer, float3 worldP, float3 scale, float4 color, float breakPercent) {
    if(renderer->alphaItemCount < arrayCount(renderer->alphaItemData)) {
        InstanceDataWithRotation *cube = &renderer->alphaItemData[renderer->alphaItemCount++];

        cube->M = float16_set_pos(float16_scale(float16_identity(), scale), worldP);
        cube->color = color;

        float numOfPics = 8;
        int indexAt = (int)(breakPercent * numOfPics);

        if(indexAt >= numOfPics) {
            indexAt = numOfPics - 1;
        }

        cube->uv.x = indexAt / numOfPics;
        cube->uv.y = (indexAt + 1) / numOfPics;
    } else {
        assert(false);
    }
}


void pushBlockItem(Renderer *renderer, float16 T, BlockType type, float4 color) {
    if(renderer->blockItemsCount < arrayCount(renderer->blockItemsData)) {
        InstanceDataWithRotation *cube = &renderer->blockItemsData[renderer->blockItemsCount++];

        cube->M = T;
        float2 uvs = getUVCoordForBlock(type);
        cube->uv = make_float4(uvs.x, uvs.y, 0, 1);
        cube->color = color;

    } else {
        assert(false);
    }
}

void pushTriangle(Renderer *renderer, float3 worldP, float4 color) {
    if(renderer->triangleCount < MAX_CUBES_PER_RENDER) {
        InstanceData *t = &renderer->triangleData[renderer->triangleCount++];

        t->pos = worldP;

        t->uv.x = 0;
        t->uv.y = 1;

        t->scale = make_float3(4, 4, 4);
        
        t->color = color;
    } else {
        assert(false);
    }
}

