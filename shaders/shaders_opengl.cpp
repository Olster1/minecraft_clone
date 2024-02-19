
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

"in uvec2 AOMask;"

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
"out vec3 normalInModelSpace;"
"out float distanceFromEye;"
"out float AOValue;"
"float aoFactors[4] = float[4](1, 0.9, 0.8, 0.7);"

"void main() {"
    "mat4 MV = V;"
    "vec4 cameraSpace = MV * vec4((vertex + pos), 1);"
    "distanceFromEye = cameraSpace.z;"
    "gl_Position = projection * cameraSpace;"
    "normal_frag_view_space = mat3(transpose(inverse(MV))) * normal;"
    "sunAngle = mat3(transpose(inverse(MV))) * vec3(0.7071, 0, 0.7071);"
    "fragPosInViewSpace = vec3(MV * vec4(vertex, 1));"
    "normalInModelSpace = normal;"
    "uint aoIndex = uint(3);"
    "uint aoMask = AOMask.x >> uint(gl_VertexID*2);"
    "if(gl_VertexID >= 16) {"
        "aoMask = AOMask.y >> (uint(2*(gl_VertexID - 16)));"
    "}"
    "AOValue = aoFactors[aoMask & aoIndex];"
    // "color_frag = vec4(AOValue, 0, 0, 1);"//color;"
    "color_frag = color;"

   " uv_frag = vec2(texUV.x, mix(uvAtlas.x, uvAtlas.y, texUV.y));"
"}";

static char *blockFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec3 normal_frag_view_space;"//viewspace
"in vec2 uv_frag; "
"in vec3 sunAngle;"
"in vec3 fragPosInViewSpace;" //view space
"in float AOValue;"
"in float distanceFromEye;"
"uniform sampler2D diffuse;"
"uniform vec3 lookingAxis;"

"out vec4 color;"
"void main() {"
    "vec4 diffSample = texture(diffuse, uv_frag);"
    "float mixValue = max(dot(normal_frag_view_space, sunAngle), 0);"
    "vec4 c = color_frag;"
    "float darkness = 0.9;"
    "if(color_frag.x == 1) {"
        "c = color_frag*1.5;"
    "} else {"
        "c = mix(vec4(darkness, darkness, darkness, 1), 1.5*color_frag, mixValue);"
    "}"
    "c = vec4(AOValue*c.xyz, c.w);"
    "vec4 fogFactor = mix(diffSample*c, vec4(0.7, 0.7, 0.7, 1), (distanceFromEye - 10) / 50);"
    
    "color = vec4((AOValue*fogFactor).xyz, 1);"//fogFactor
"}";

static char *blockColorShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec3 normalInModelSpace;"
"out vec4 color;"
"float factors[2] = float[](1, 0.7f);"
"void main() {"
    "float d = dot(vec3(0, -1, 0), normalInModelSpace);"
    "int index = int(clamp(d, 0, 1));"
    "float factor = factors[index];"
    "color = vec4(color_frag.x*factor, color_frag.y*factor, color_frag.z*factor, color_frag.w);"
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
"float value = dot(normalize(uv_frag), vec3(0, 1, 0));"
"color = mix(vec4(0.678, 0.847, 0.901, 1), vec4(0.126, 0.162, 0.529, 1), value);"
"}";
