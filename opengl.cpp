#define STB_IMAGE_IMPLEMENTATION
#include "./libs/stb_image.h"
#include <OpenGL/gl3.h>
#include "./shaders/shaders_opengl.cpp"

// NOTE: Each location index in a vertex attribute index - i.e. 4 floats. that's why for matrix we skip 4 values
#define VERTEX_ATTRIB_LOCATION 0
#define NORMAL_ATTRIB_LOCATION 1
#define UV_ATTRIB_LOCATION 2
#define POS_ATTRIB_LOCATION 3
#define UVATLAS_ATTRIB_LOCATION 4
#define COLOR_ATTRIB_LOCATION 5
#define SCALE_ATTRIB_LOCATION 6
#define MODEL_TRANSFORM_ATTRIB_LOCATION 7


#define renderCheckError() renderCheckError_(__LINE__, (char *)__FILE__)
void renderCheckError_(int lineNumber, char *fileName) {
    #define RENDER_CHECK_ERRORS 1
    #if RENDER_CHECK_ERRORS
    GLenum err = glGetError();
    if(err) {
        printf((char *)"GL error check: %x at %d in %s\n", err, lineNumber, fileName);
        assert(!err);
    }
    #endif
}

Shader loadShader(char *vertexShader, char *fragShader) {
    Shader result = {};
    
    result.valid = true;
    
    GLuint vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
    renderCheckError();
    GLuint fragShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
    renderCheckError();
    
    glShaderSource(vertexShaderHandle, 1, (const GLchar **)(&vertexShader), 0);
    renderCheckError();
    glShaderSource(fragShaderHandle, 1, (const GLchar **)(&fragShader), 0);
    renderCheckError();
    
    glCompileShader(vertexShaderHandle);
    renderCheckError();
    glCompileShader(fragShaderHandle);
    renderCheckError();
    result.handle = glCreateProgram();
    renderCheckError();
    glAttachShader(result.handle, vertexShaderHandle);
    renderCheckError();
    glAttachShader(result.handle, fragShaderHandle);
    renderCheckError();

    int max_attribs;
    glGetIntegerv (GL_MAX_VERTEX_ATTRIBS, &max_attribs);

    glBindAttribLocation(result.handle, VERTEX_ATTRIB_LOCATION, "vertex");
    renderCheckError();
    glBindAttribLocation(result.handle, NORMAL_ATTRIB_LOCATION, "normal");
    renderCheckError();
    glBindAttribLocation(result.handle, UV_ATTRIB_LOCATION, "texUV");
    renderCheckError();
    glBindAttribLocation(result.handle, POS_ATTRIB_LOCATION, "pos");
    renderCheckError();
    glBindAttribLocation(result.handle, UVATLAS_ATTRIB_LOCATION, "uvAtlas");
    renderCheckError();
    glBindAttribLocation(result.handle, COLOR_ATTRIB_LOCATION, "color");
    renderCheckError();
    glBindAttribLocation(result.handle, SCALE_ATTRIB_LOCATION, "scale");
    renderCheckError();
    glBindAttribLocation(result.handle, MODEL_TRANSFORM_ATTRIB_LOCATION, "M");
    renderCheckError();

    assert(MODEL_TRANSFORM_ATTRIB_LOCATION < (max_attribs - 1));

    glLinkProgram(result.handle);
    renderCheckError();
    glUseProgram(result.handle);
    
    GLint success = 0;
    glGetShaderiv(vertexShaderHandle, GL_COMPILE_STATUS, &success);
    
    GLint success1 = 0;
    glGetShaderiv(fragShaderHandle, GL_COMPILE_STATUS, &success1); 
    
    if(success == GL_FALSE || success1 == GL_FALSE) {
        result.valid = false;
        int  vlength,    flength,    plength;
        char vlog[2048];
        char flog[2048];
        char plog[2048];
        glGetShaderInfoLog(vertexShaderHandle, 2048, &vlength, vlog);
        glGetShaderInfoLog(fragShaderHandle, 2048, &flength, flog);
        glGetProgramInfoLog(result.handle, 2048, &plength, plog);
        
        if(vlength || flength || plength) {
            printf("%s\n", vertexShader);
            printf("%s\n", fragShader);
            printf("%s\n", vlog);
            printf("%s\n", flog);
            printf("%s\n", plog);
            
        }
    }
    
    assert(result.valid);

    return result;
}

static inline void addInstanceAttribForMatrix(int index, GLuint attribLoc, int numOfFloats, size_t offsetForStruct, size_t offsetInStruct) {
    glEnableVertexAttribArray(attribLoc + index);  
    renderCheckError();
    
    glVertexAttribPointer(attribLoc + index, numOfFloats, GL_FLOAT, GL_FALSE, offsetForStruct, ((char *)0) + offsetInStruct + (4*sizeof(float)*index));
    renderCheckError();
    glVertexAttribDivisor(attribLoc + index, 1);
    renderCheckError();
}

static inline void addInstancingAttrib (GLuint attribLoc, int numOfFloats, size_t offsetForStruct, size_t offsetInStruct) {
    assert(offsetForStruct > 0);
    if(numOfFloats == 16) {
        addInstanceAttribForMatrix(0, attribLoc, 4, offsetForStruct, offsetInStruct);
        addInstanceAttribForMatrix(1, attribLoc, 4, offsetForStruct, offsetInStruct);
        addInstanceAttribForMatrix(2, attribLoc, 4, offsetForStruct, offsetInStruct);
        addInstanceAttribForMatrix(3, attribLoc, 4, offsetForStruct, offsetInStruct);
    } else {
        glEnableVertexAttribArray(attribLoc);  
        renderCheckError();
        
        assert(numOfFloats <= 4);
        glVertexAttribPointer(attribLoc, numOfFloats, GL_FLOAT, GL_FALSE, offsetForStruct, ((char *)0) + offsetInStruct);
        renderCheckError();
        
        glVertexAttribDivisor(attribLoc, 1);
        renderCheckError();
    }
}

enum AttribInstancingType {
    ATTRIB_INSTANCE_TYPE_DEFAULT,
    ATTRIB_INSTANCE_TYPE_MODEL_MATRIX,
};

void addInstancingAttribsForShader(AttribInstancingType type) {
    if(type == ATTRIB_INSTANCE_TYPE_DEFAULT) {
        size_t offsetForStruct = sizeof(InstanceData); 

        addInstancingAttrib (POS_ATTRIB_LOCATION, 3, offsetForStruct, 0);
        unsigned int uvOffset = (intptr_t)(&(((InstanceData *)0)->uv));
        addInstancingAttrib (UVATLAS_ATTRIB_LOCATION, 2, offsetForStruct, uvOffset);
        unsigned int colorOffset = (intptr_t)(&(((InstanceData *)0)->color));
        addInstancingAttrib (COLOR_ATTRIB_LOCATION, 4, offsetForStruct, colorOffset);
        renderCheckError();
        unsigned int scaleOffset = (intptr_t)(&(((InstanceData *)0)->scale));
        addInstancingAttrib (SCALE_ATTRIB_LOCATION, 3, offsetForStruct, scaleOffset);
        renderCheckError();
    } else if(type == ATTRIB_INSTANCE_TYPE_MODEL_MATRIX) {
        size_t offsetForStruct = sizeof(InstanceDataWithRotation); 

        unsigned int uvOffset = (intptr_t)(&(((InstanceDataWithRotation *)0)->uv));
        addInstancingAttrib (UVATLAS_ATTRIB_LOCATION, 2, offsetForStruct, uvOffset);
        unsigned int colorOffset = (intptr_t)(&(((InstanceDataWithRotation *)0)->color));
        addInstancingAttrib (COLOR_ATTRIB_LOCATION, 4, offsetForStruct, colorOffset);
        renderCheckError();
        unsigned int modelOffset = (intptr_t)(&(((InstanceDataWithRotation *)0)->M));
        addInstancingAttrib (MODEL_TRANSFORM_ATTRIB_LOCATION, 16, offsetForStruct, modelOffset);
        renderCheckError();
    } else {
        assert(false);
    }
    
}

ModelBuffer generateVertexBuffer(Vertex *triangleData, int vertexCount, unsigned int *indicesData, int indexCount, AttribInstancingType attribInstancingType = ATTRIB_INSTANCE_TYPE_DEFAULT) {
    ModelBuffer result = {};
    glGenVertexArrays(1, &result.handle);
    renderCheckError();
    glBindVertexArray(result.handle);
    renderCheckError();
    
    GLuint vertices;
    GLuint indices;
    
    glGenBuffers(1, &vertices);
    renderCheckError();
    
    glBindBuffer(GL_ARRAY_BUFFER, vertices);
    renderCheckError();
    
    glBufferData(GL_ARRAY_BUFFER, vertexCount*sizeof(Vertex), triangleData, GL_STATIC_DRAW);
    renderCheckError();
    
    glGenBuffers(1, &indices);
    renderCheckError();
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    renderCheckError();
    
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount*sizeof(unsigned int), indicesData, GL_STATIC_DRAW);
    renderCheckError();
    
    result.indexCount = indexCount;
    
    //NOTE: Assign the attribute locations with the data offsets & types
    GLint vertexAttrib = VERTEX_ATTRIB_LOCATION;
    renderCheckError();
    glEnableVertexAttribArray(vertexAttrib);  
    renderCheckError();
    glVertexAttribPointer(vertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    renderCheckError();
    
    GLint texUVAttrib = UV_ATTRIB_LOCATION;
    glEnableVertexAttribArray(texUVAttrib);  
    renderCheckError();
    unsigned int uvByteOffset = (intptr_t)(&(((Vertex *)0)->texUV));
    glVertexAttribPointer(texUVAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + uvByteOffset);
    renderCheckError();

    GLint normalsAttrib = NORMAL_ATTRIB_LOCATION;
    glEnableVertexAttribArray(normalsAttrib);  
    renderCheckError();
    unsigned int normalOffset = (intptr_t)(&(((Vertex *)0)->normal));
    glVertexAttribPointer(normalsAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + normalOffset);
    renderCheckError();

    // vbo instance buffer
    {
        glGenBuffers(1, &result.instanceBufferhandle);
        renderCheckError();

        glBindBuffer(GL_ARRAY_BUFFER, result.instanceBufferhandle);
        renderCheckError();

        glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_DYNAMIC_DRAW);
        
        addInstancingAttribsForShader(attribInstancingType);
    }
    
    glBindVertexArray(0);
        
    //we can delete these buffers since they are still referenced by the VAO 
    glDeleteBuffers(1, &vertices);
    glDeleteBuffers(1, &indices);

    return result;
}

void initBackendRenderer() {
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    // glFrontFace(GL_CW);  

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL); 

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
}

struct Texture {
    int w;
    int h;

    uint32_t handle;
};

Texture loadCubeMapTextureToGPU(char *folderName) {
    Texture result = {};

    glGenTextures(1, &result.handle);
    glBindTexture(GL_TEXTURE_CUBE_MAP, result.handle);

    char *faces[] = {
    "right.jpg",
    "left.jpg",
    "top.jpg",
    "bottom.jpg",
    "front.jpg",
    "back.jpg"};
    
    int width, height, nrChannels;
    for (unsigned int i = 0; i < arrayCount(faces); i++)
    {
        size_t bytesToAlloc = snprintf(NULL, 0, "%s%s", folderName, faces[i]);
        char *concatFileName = (char *)malloc(bytesToAlloc + 1);
        snprintf(concatFileName, bytesToAlloc + 1, "%s%s", folderName, faces[i]);
        unsigned char *data = stbi_load(concatFileName, &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        } else {
            printf("Cubemap tex failed to load\n");
            stbi_image_free(data);
        }
        free(concatFileName);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    printf("%d\n", result.handle);

    return result;
}

Texture loadTextureToGPU(char *fileName) {
    Texture t = {};

    unsigned char *imageData = (unsigned char *)stbi_load(fileName, &t.w, &t.h, 0, STBI_rgb_alpha);

    if(imageData) {
        // assert(result.comp == 4);
    } else {
        printf("%s\n", fileName);
        assert(!"no image found");
    }

    GLuint resultId;
    glGenTextures(1, &resultId);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D, resultId);
    renderCheckError();
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    renderCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    renderCheckError();
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, t.w, t.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    renderCheckError();

    glGenerateMipmap(GL_TEXTURE_2D);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    renderCheckError();

    t.handle = resultId;

    stbi_image_free(imageData);

    return t;
    
}

void updateInstanceData(uint32_t bufferHandle, void *data, size_t sizeInBytes) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferHandle);
    renderCheckError();
    
    //send the data to GPU. glBufferData deletes the old one
    //NOTE(ollie): We were using glBufferData which deletes the old buffer and resends the create a new buffer, but 
    //NOTE(ollie): I saw on Dungeoneer code using glsubbufferdata is faster because it doesn't have to delete it.
    // glBufferSubData(GL_ARRAY_BUFFER, 0, sizeInBytes, data);
    glBufferData(GL_ARRAY_BUFFER, sizeInBytes, data, GL_DYNAMIC_DRAW); 
    renderCheckError();

    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    renderCheckError();
}

enum ShaderFlags {
    SHADER_CUBE_MAP = 1 << 0,
};

void bindTexture(char *uniformName, int slotId, GLint textureId, Shader *shader, uint32_t flags) {
    GLint texUniform = glGetUniformLocation(shader->handle, uniformName); 
    renderCheckError();
    
    glUniform1i(texUniform, slotId);
    renderCheckError();
    
    glActiveTexture(GL_TEXTURE0 + slotId);
    renderCheckError();
    
    if(flags & SHADER_CUBE_MAP) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureId); 
        renderCheckError();
    } else {
        glBindTexture(GL_TEXTURE_2D, textureId); 
        renderCheckError();
    }
}

void drawModels(ModelBuffer *model, Shader *shader, uint32_t textureId, int instanceCount, float16 projectionTransform, float16 modelViewTransform, float3 lookingAxis, uint32_t flags = 0) {
    // printf("%d\n", instanceCount);
    glUseProgram(shader->handle);
    renderCheckError();
    
    glBindVertexArray(model->handle);
    renderCheckError();

    glUniformMatrix4fv(glGetUniformLocation(shader->handle, "V"), 1, GL_FALSE, modelViewTransform.E);
    renderCheckError();

    glUniformMatrix4fv(glGetUniformLocation(shader->handle, "projection"), 1, GL_FALSE, projectionTransform.E);
    renderCheckError();

    glUniform3f(glGetUniformLocation(shader->handle, "lookingAxis"), lookingAxis.x, lookingAxis.y, lookingAxis.z);
    renderCheckError();

    bindTexture("diffuse", 1, textureId, shader, flags);
    renderCheckError();

    glDrawElementsInstanced(GL_TRIANGLES, model->indexCount, GL_UNSIGNED_INT, 0, instanceCount); 
    renderCheckError();
    
    glBindVertexArray(0);
    renderCheckError();    

    glUseProgram(0);
    renderCheckError();
    
}

void rendererFinish(Renderer *renderer, float16 projectionTransform, float16 modelViewTransform, float16 projectionScreenTransform, float3 lookingAxis, float16 cameraTransformWithoutTranslation) {

    if(renderer->cubeCount > 0) {
        //NOTE: Draw Cubes
        updateInstanceData(renderer->blockModel.instanceBufferhandle, renderer->cubeData, renderer->cubeCount*sizeof(InstanceData));
        drawModels(&renderer->blockModel, &renderer->blockShader, renderer->terrainTextureHandle, renderer->cubeCount, projectionTransform, modelViewTransform, lookingAxis);

        renderer->cubeCount = 0;
    }

    if(renderer->blockItemsCount > 0) {
        updateInstanceData(renderer->blockModelWithInstancedT.instanceBufferhandle, renderer->blockItemsData, renderer->blockItemsCount*sizeof(InstanceDataWithRotation));
        drawModels(&renderer->blockModelWithInstancedT, &renderer->blockPickupShader, renderer->terrainTextureHandle, renderer->blockItemsCount, projectionTransform, modelViewTransform, lookingAxis);

        renderer->blockItemsCount = 0;
    }

    //NOTE: Draw the skybox here
    glDepthMask(GL_FALSE); //NOTE: Disable WRITING to the depth buffer
    drawModels(&renderer->blockModel, &renderer->skyboxShader, renderer->skyboxTextureHandle, 1, projectionTransform, cameraTransformWithoutTranslation, lookingAxis, SHADER_CUBE_MAP);
    glDepthMask(GL_TRUE);

    if(renderer->filledCircleCount > 0) {
        //NOTE: Draw filled circles
        updateInstanceData(renderer->quadModel.instanceBufferhandle, renderer->filledCircleData, renderer->filledCircleCount*sizeof(InstanceData));
        drawModels(&renderer->quadModel, &renderer->quadTextureShader, renderer->circleHandle, renderer->filledCircleCount, projectionScreenTransform, float16_identity(), lookingAxis);

        renderer->filledCircleCount = 0;
    }
    
    if(renderer->circleCount > 0) {
        //NOTE: Draw circle oultines
        updateInstanceData(renderer->quadModel.instanceBufferhandle, renderer->circleData, renderer->circleCount*sizeof(InstanceData));
        drawModels(&renderer->quadModel, &renderer->quadTextureShader, renderer->circleOutlineHandle, renderer->circleCount, projectionScreenTransform, float16_identity(), lookingAxis);

        renderer->circleCount = 0;
    }

    if(renderer->triangleCount > 0) {
        //NOTE: Draw circle oultines
        // updateInstanceData(renderer->triangleModel.instanceBufferhandle, renderer->triangleData, renderer->triangleCount*sizeof(InstanceData));
        // drawModels(&renderer->triangleModel, &renderer->quadShader, renderer->circleOutlineHandle, renderer->triangleCount, projectionScreenTransform, float16_identity(), lookingAxis);

        updateInstanceData(renderer->avocadoModel.instanceBufferhandle, renderer->triangleData, renderer->triangleCount*sizeof(InstanceData));
        drawModels(&renderer->avocadoModel, &renderer->quadShader, renderer->circleOutlineHandle, renderer->triangleCount, projectionTransform, modelViewTransform, lookingAxis);

        renderer->triangleCount = 0;
    }

 
}