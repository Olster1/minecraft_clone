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
    makeVertex(make_float3(0.5f, -0.5f, 0.5f), make_float2(0, 0), make_float3(0, 0, 1)),
    makeVertex(make_float3(-0.5f, -0.5f, 0.5f), make_float2(0, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(-0.5f,  0.5f, 0.5f), make_float2(1, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, 0.5f, 0.5f), make_float2(1, 0), make_float3(0, 0, 1)),
};

static unsigned int global_quadIndices[] = {
    0, 1, 2, 0, 2, 3,
};

static Vertex global_cubeData[] = {
    // Top face (y = 1.0f)
    makeVertex(make_float3(0.5f, 0.5f, -0.5f), make_float2(0.25f, 0), make_float3(0, 1, 0)),
    makeVertex(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0.25f,1), make_float3(0, 1, 0)),
    makeVertex(make_float3(-0.5f, 0.5f,  0.5f), make_float2(0.5f, 1), make_float3(0, 1, 0)),
    makeVertex(make_float3(0.5f, 0.5f,  0.5f), make_float2(0.5f, 0), make_float3(0, 1, 0)),
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
};

struct InstanceDataWithRotation {
    float16 M;
    float4 color;
    float2 uv;
};

struct Renderer {
    uint32_t terrainTextureHandle;
    uint32_t circleHandle;
    uint32_t circleOutlineHandle;
    uint32_t skyboxTextureHandle;

    int cubeCount;
    InstanceData cubeData[MAX_CUBES_PER_RENDER];

    int circleCount;
    InstanceData circleData[MAX_CIRCLES_PER_RENDER];

    int filledCircleCount;
    InstanceData filledCircleData[MAX_CIRCLES_PER_RENDER];

    int triangleCount;
    InstanceData triangleData[MAX_RENDER_ITEMS_PER_INSTANCE];

    int blockItemsCount; //NOTE: The blocks that are rotating
    InstanceDataWithRotation blockItemsData[MAX_WORLD_ITEMS_PER_INSTANCE];

    int alphaBlockCount; //NOTE: The blocks that are rotating
    InstanceData alphaBlockData[MAX_WORLD_ITEMS_PER_INSTANCE];

    Shader blockShader;
    Shader blockPickupShader;
    Shader quadShader;
    Shader quadTextureShader;
    Shader skyboxShader;
    Shader blockColorShader;
    
    ModelBuffer blockModel;
    ModelBuffer blockModelWithInstancedT;
    ModelBuffer quadModel;
    ModelBuffer triangleModel;
    ModelBuffer avocadoModel;
};


void pushCircle_(Renderer *renderer, float3 worldP, bool fill, float radius, float4 color) {
    InstanceData *c = 0;
    if(fill) {
        if(renderer->filledCircleCount < arrayCount(renderer->filledCircleData)) {
            c = &renderer->filledCircleData[renderer->filledCircleCount++];
        }   
    } else {
        if(renderer->circleCount < arrayCount(renderer->circleData)) {
            c = &renderer->circleData[renderer->circleCount++];
        }
    }

    if(c) {
        c->pos = worldP;
        c->scale = make_float3(radius, radius, 1);
        c->color = color;
        // c->uv = make_float4(0, 0, 1, 1);
    }
}

void pushCircleOutline(Renderer *renderer, float3 worldP, float radius, float4 color) {
    pushCircle_(renderer, worldP, false, radius, color);
}

void pushFillCircle(Renderer *renderer, float3 worldP, float radius, float4 color) {
    pushCircle_(renderer, worldP, true, radius, color);
}

void pushCube(Renderer *renderer, float3 worldP, BlockType type, float4 color) {
    if(renderer->cubeCount < arrayCount(renderer->cubeData)) {
        InstanceData *cube = &renderer->cubeData[renderer->cubeCount++];

        cube->pos = worldP;

         if(type == BLOCK_GRASS) {
            cube->uv.x = 0;
            cube->uv.y = 0.25f;
        } else if(type == BLOCK_SOIL) {
            cube->uv.x = 0.25f;
            cube->uv.y = 0.5f;
        } else if(type == BLOCK_STONE) {
            cube->uv.x = 0.5f;
            cube->uv.y = 0.75f;
        }
        cube->color = color;
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

void pushBlockItem(Renderer *renderer, float16 T, BlockType type, float4 color) {
    if(renderer->blockItemsCount < arrayCount(renderer->blockItemsData)) {
        InstanceDataWithRotation *cube = &renderer->blockItemsData[renderer->blockItemsCount++];

        cube->M = T;

         if(type == BLOCK_GRASS) {
            cube->uv.x = 0;
            cube->uv.y = 0.25f;
        } else if(type == BLOCK_SOIL) {
            cube->uv.x = 0.25f;
            cube->uv.y = 0.5f;
        } else if(type == BLOCK_STONE) {
            cube->uv.x = 0.5f;
            cube->uv.y = 0.75f;
        }
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

