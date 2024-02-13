
static char *blockPickupVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"

//per instanced variables
"in mat4 M;"
"in vec2 uvAtlas;"
"in vec4 color;"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"
"out vec3 normal_frag_view_space;"
"out vec2 uv_frag;"
"out vec3 fragPosInViewSpace;"
"out vec3 sunAngle;"

"void main() {"
    "mat4 MV = V * M;"
    "gl_Position = projection * MV * vec4((vertex), 1);"
    "color_frag = color;"
    "normal_frag_view_space = mat3(transpose(inverse(MV))) * normal;"
    "sunAngle = mat3(transpose(inverse(MV))) * vec3(0.7071, 0, 0.7071);"
    "fragPosInViewSpace = vec3(MV * vec4(vertex, 1));"

   " uv_frag = vec2(texUV.x, mix(uvAtlas.x, uvAtlas.y, texUV.y));"
"}";

static char *blockPickupFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec3 normal_frag_view_space;"//viewspace
"in vec2 uv_frag; "
"in vec3 sunAngle;"
"in vec3 fragPosInViewSpace;" //view space
"uniform sampler2D diffuse;"
"uniform vec3 lookingAxis;"

"out vec4 color;"
"void main() {"
    "vec4 diffSample = texture(diffuse, uv_frag);"
    "vec4 c = color_frag;"
    "color = diffSample*c;"
"}";

static char *blockVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"

//per instanced variables
"in vec3 pos;"
"in vec2 uvAtlas;"
"in vec4 color;"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"
"out vec3 normal_frag_view_space;"
"out vec2 uv_frag;"
"out vec3 fragPosInViewSpace;"
"out vec3 sunAngle;"

"void main() {"
    "mat4 MV = V;"
    "gl_Position = projection * MV * vec4((vertex + pos), 1);"
    "color_frag = color;"
    "normal_frag_view_space = mat3(transpose(inverse(MV))) * normal;"
    "sunAngle = mat3(transpose(inverse(MV))) * vec3(0.7071, 0, 0.7071);"
    "fragPosInViewSpace = vec3(MV * vec4(vertex, 1));"

   " uv_frag = vec2(texUV.x, mix(uvAtlas.x, uvAtlas.y, texUV.y));"
"}";

static char *blockFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec3 normal_frag_view_space;"//viewspace
"in vec2 uv_frag; "
"in vec3 sunAngle;"
"in vec3 fragPosInViewSpace;" //view space
"uniform sampler2D diffuse;"
"uniform vec3 lookingAxis;"

"out vec4 color;"
"void main() {"
    "vec4 diffSample = texture(diffuse, uv_frag);"
    "float mixValue = max(dot(normal_frag_view_space, sunAngle), 0);"
    "vec4 c = color_frag;"
    "float darkness = 0.5;"
    "if(color_frag.x == 1) {"
        "c = color_frag;"
    "} else {"
        "c = mix(vec4(darkness, darkness, darkness, 1), color_frag, mixValue);"
    "}"
    

    "color = diffSample*c;"
"}";


static char *quadVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"

//per instanced variables
"in vec3 pos;"
"in vec2 uvAtlas;"
"in vec4 color;"
"in vec3 scale;"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"
"out vec2 uv_frag;"

"void main() {"
    "gl_Position = projection * V * vec4((vertex*scale + pos), 1);"
    "color_frag = color;"

   " uv_frag = texUV;"
"}";

static char *quadTextureFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec2 uv_frag; "
"uniform sampler2D diffuse;"
"out vec4 color;"
"void main() {"
    "vec4 diffSample = texture(diffuse, uv_frag);"
    "color = diffSample*color_frag;"
"}";

static char *quadFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec2 uv_frag; "
"out vec4 color;"
"void main() {"
    "color = color_frag;"
"}";

static char *skyboxVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"

//outgoing variables
"out vec3 uv_frag;"

"void main() {"
    "uv_frag = vertex;"
    "vec4 pos = projection * V * vec4(vertex, 1);"
    "gl_Position = pos.xyww;"
"}";

static char *skyboxFragShader = 
"#version 330\n"
"in vec3 uv_frag; "
"uniform samplerCube diffuse;"
"out vec4 color;"
"void main() {"
"color = texture(diffuse, uv_frag);"
"}";
