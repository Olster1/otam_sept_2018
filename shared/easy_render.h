#define PI32 3.14159265359
#define NEAR_CLIP_PLANE -(RENDER_HANDNESS)*0.1;
#define FAR_CLIP_PLANE -(RENDER_HANDNESS)*10000.0f
#define USING_ATTRIBS_FOR_INSTANCING 1

#define PRINT_NUMBER_DRAW_CALLS 0

#if !DESKTOP
#define GL_TEXTURE_BUFFER                 0x8C2A
#define RENDER_TEXTURE_BUFFER_ENUM GL_TEXTURE_BUFFER
#else 
#define RENDER_TEXTURE_BUFFER_ENUM GL_TEXTURE_BUFFER
#endif

#if !defined arrayCount
#define arrayCount(arg) (sizeof(arg) / sizeof(arg[0])) 
#endif

#if !defined EASY_MATH_H
#include "easy_math.h"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#include "easy_shaders.h"

#define PROJECTION_TYPE(FUNC) \
FUNC(PERSPECTIVE_MATRIX) \
FUNC(ORTHO_MATRIX) \

static bool globalImmediateModeGraphics = false;

typedef enum {
    PROJECTION_TYPE(ENUM)
} ProjectionType;

static char *ProjectionTypeStrings[] = { PROJECTION_TYPE(STRING) };

typedef struct {
    V3 pos;
    float flux;
} LightInfo;

static int globalLightInfoCount;
static LightInfo globalLightInfos[64];

typedef struct {
    s32 handle;
    char *name;
} ShaderVal;

typedef struct {
    GLuint glProgram;
    GLuint glShaderV;
    GLuint glShaderF;
    
    ShaderVal uniforms[16];
    int uniformCount;
    
    ShaderVal attribs[16];
    int attribCount;
    
    bool valid;
} RenderProgram;

RenderProgram phongProgram;
RenderProgram textureProgram;

typedef struct {
    V3 pos;
    V3 dim; 
    V3 transformPos;
    V3 transformDim;
    Matrix4 pvm;
} RenderInfo;

RenderInfo calculateRenderInfo(V3 pos, V3 dim, V3 cameraPos, Matrix4 metresToPixels) {
    RenderInfo info = {};
    info.pos = pos;
    
    info.pvm = Mat4Mult(metresToPixels, Matrix4_translate(mat4(), v3_scale(-1, cameraPos)));
    
    info.transformPos = V4MultMat4(v4(pos.x, pos.y, pos.z, 1), info.pvm).xyz;
    
    info.dim = dim;
    info.transformDim = transformPositionV3(dim, metresToPixels);
    return info;
}

FileContents loadShader(char *fileName) {
    FileContents fileContents = getFileContentsNullTerminate(fileName);
    return fileContents;
}

typedef struct {
    union {
        struct {
            float E[13]; //position -> normal -> textureUVs -> colors
        };
        struct {
            V3 position;
            V3 normal;
            V2 texUV;
            V4 color;
            float lengthRatio; //just for rects. mmm, should i be sticking more things in here?????? 
            float percentY;
            u32 instanceIndex;
        };
    };
} Vertex;


#define getOffsetForVertex(attrib) (void *)(&(((Vertex *)(0))->attrib))

typedef enum {
    SHAPE_RECTANGLE,
    SHAPE_RECTANGLE_GRAD,
    SHAPE_TEXTURE,
    SHAPE_MODEL,
    SHAPE_SHADOW,
    SHAPE_CIRCLE,
    SHAPE_LINE,
    SHAPE_BLUR,
} ShapeType;

typedef enum {
    BLEND_FUNC_STANDARD,
    BLEND_FUNC_ZERO_ONE_ZERO_ONE_MINUS_ALPHA,
} BlendFuncType;

typedef struct {
    GLuint vaoHandle;
    int indexCount; // this is to keep around so opnegl knows how many triangles to draw after the initialization frame
    bool valid;
    GLuint vboHandle; //this is for instancing data
} VaoHandle;

//just has a dim of 1 by 1 and you can rotate, scale etc. by a model matrix
static V3 globalQuadPositionData[4] = {
    v3(-0.5f, -0.5f, 0),
    v3(-0.5f, 0.5f, 0),
    v3(0.5f, -0.5f, 0),
    v3(0.5f, 0.5f, 0) 
};

static unsigned int globalQuadIndicesData[6] = {0, 1, 3, 0, 2, 3};

static VaoHandle globalQuadVaoHandle = {};

typedef struct {
    GLuint id;
    int width;
    int height;
    Rect2f uvCoords;
    
} Texture;

typedef enum {
    RENDER_LIGHT_DIRECTIONAL,
    RENDER_LIGHT_POINT,
    RENDER_LIGHT_FLASHLIGHT
} RenderLightType;

typedef struct {
    RenderLightType type;
    
    V3 position; //or direction

    V3 ambient;
    V3 diffuse; 
    V3 specular; //Usually black->white mono

    //for point light and flash light -> represents the attentuation as you get further away
    float constant;
    float linear;
    float quadratic;
    ///

    //for just the flash light -> for the cone of the flash light
    float cutoff;
    float outerCutoff;
    //
} RenderLight;

typedef struct {
    Texture *diffuseMap;
    Texture *specularMap;
    float shininess;
} Material;

typedef struct {
    InfiniteAlloc triangleData;
    int triCount; 
    
    InfiniteAlloc indicesData; 
    int indexCount; 
    
    RenderProgram *program; 
    ShapeType type; 
    
    u32 textureHandle;
    Rect2f textureUVs;    
    
    Matrix4 vmMat;  //TODOL change to just VM
    Matrix4 pMat; //P of PVM

    float zAt;
    
    int id;
    
    V4 color;
    
    int bufferId;
    bool depthTest;
    BlendFuncType blendFuncType;
    
    VaoHandle *bufferHandles;
} RenderItem;

typedef struct {
    GLuint tbo; // this is attached to the buffer
    GLuint buffer;
} BufferStorage;

typedef struct {
    int currentBufferId;
    bool currentDepthTest;
    VaoHandle *currentBufferHandles;
    BlendFuncType blendFuncType;
    
    int idAt; 
    InfiniteAlloc items; //type: RenderItem
    
    int lastStorageBufferCount;
    BufferStorage lastBufferStorage[512];

    Texture *whiteTexture;

    //RenderLight lights[16];
    
} RenderGroup;

void initRenderGroup(RenderGroup *group) {
    group->items = initInfinteAlloc(RenderItem);
    group->currentDepthTest = true;
    group->blendFuncType = BLEND_FUNC_STANDARD;
    group->lastStorageBufferCount = 0;
}

void setFrameBufferId(RenderGroup *group, int bufferId) {
    group->currentBufferId = bufferId;
    
}

void renderDisableDepthTest(RenderGroup *group) {
    group->currentDepthTest = false;
}

void renderEnableDepthTest(RenderGroup *group) {
    group->currentDepthTest = true;
}

void setBlendFuncType(RenderGroup *group, BlendFuncType type) {
    group->blendFuncType = type;
}

static RenderGroup *globalRenderGroup;

void renderSetViewPort(float x0, float y0, float x1, float y1) {
    glViewport(x0, y0, x1, y1);
}

void pushRenderItemForModel(VaoHandle *handles, RenderGroup *group, RenderProgram *program, Material *material, Matrix4 P, Matrix4 VM, V4 color) {
    if(!isInfinteAllocActive(&group->items)) {
        initRenderGroup(group);
    }
    
    RenderItem *info = (RenderItem *)addElementInifinteAlloc_(&group->items, 0);
    EasyAssert(info);
    info->bufferId = group->currentBufferId;
    info->depthTest = group->currentDepthTest;
    info->blendFuncType = group->blendFuncType;
    info->bufferHandles = handles;
    info->color = color;
    
    info->id = group->idAt++;
    
    info->program = program;
    info->type = SHAPE_MODEL;
}

void pushRenderItem(VaoHandle *handles, RenderGroup *group, Vertex *triangleData, int triCount, unsigned int *indicesData, int indexCount, RenderProgram *program, ShapeType type, Texture *texture, Matrix4 vmMat, Matrix4 pMat, V4 color, float zAt) {
    if(!isInfinteAllocActive(&group->items)) {
        initRenderGroup(group);
        // group->items = initInfinteAlloc(RenderItem);
    }
    
    RenderItem *info = (RenderItem *)addElementInifinteAlloc_(&group->items, 0);
    EasyAssert(info);
    info->bufferId = group->currentBufferId;
    info->depthTest = group->currentDepthTest;
    info->blendFuncType = group->blendFuncType;
    info->bufferHandles = handles;
    info->color = color;
    info->zAt = zAt;
    
    info->triangleData = initInfinteAlloc(Vertex);
    info->triCount = triCount; 
    
    for(int triIndex = 0; triIndex < triCount; ++triIndex) {
        addElementInifinteAlloc_(&info->triangleData, &triangleData[triIndex]);
    }
    
    info->indicesData = initInfinteAlloc(unsigned int); 
    
    for(int indicesIndex = 0; indicesIndex < indexCount; ++indicesIndex) {
        addElementInifinteAlloc_(&info->indicesData, &indicesData[indicesIndex]);
    }
    
    info->indexCount = indexCount;
    
    info->id = group->idAt++;
    
    info->program = program;
    info->type = type;
    //We are now overriding rectangles with texture calls & using a white dummy texture.
    //Could make this cleaner by original making it a SHAPE_TEXTURE. Might prevent possible bugs that 
    //might occur??!! - Oliver 22/1/19
    if(info->type == SHAPE_RECTANGLE) {
        EasyAssert(program == &textureProgram);
        info->program = &textureProgram;
        info->type = SHAPE_TEXTURE; 
        //set the texture so the render item gets assigned the uv data.
        texture = group->whiteTexture;
    } 
    
    if(texture) {
        info->textureHandle = (u32)texture->id;
        EasyAssert(info->textureHandle);
        info->textureUVs = texture->uvCoords;
    } 
    info->pMat = pMat;
    info->vmMat = vmMat;
}

RenderProgram createRenderProgram(char *vShaderSource, char *fShaderSource) {
    RenderProgram result = {};
    
    result.valid = true;
    
    result.glShaderV = glCreateShader(GL_VERTEX_SHADER);
    result.glShaderF = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource(result.glShaderV, 1, (const GLchar **)(&vShaderSource), 0);
    glShaderSource(result.glShaderF, 1, (const GLchar **)(&fShaderSource), 0);
    
    glCompileShader(result.glShaderV);
    glCompileShader(result.glShaderF);
    result.glProgram = glCreateProgram();
    glAttachShader(result.glProgram, result.glShaderV);
    glAttachShader(result.glProgram, result.glShaderF);
    glLinkProgram(result.glProgram);
    glUseProgram(result.glProgram);
    
    GLint success = 0;
    glGetShaderiv(result.glShaderV, GL_COMPILE_STATUS, &success);
    
    GLint success1 = 0;
    glGetShaderiv(result.glShaderF, GL_COMPILE_STATUS, &success1); 
    
    
    if(success == GL_FALSE || success1 == GL_FALSE) {
        result.valid = false;
        int  vlength,    flength,    plength;
        char vlog[2048];
        char flog[2048];
        char plog[2048];
        glGetShaderInfoLog(result.glShaderV, 2048, &vlength, vlog);
        glGetShaderInfoLog(result.glShaderF, 2048, &flength, flog);
        glGetProgramInfoLog(result.glProgram, 2048, &plength, plog);
        
        if(vlength || flength || plength) {
            
            printf("%s\n", vShaderSource);
            printf("%s\n", fShaderSource);
            printf("%s\n", vlog);
            printf("%s\n", flog);
            printf("%s\n", plog);
            
        }
    }
    
    EasyAssert(result.valid);
    
    return result;
}

#define renderCheckError() renderCheckError_(__LINE__, (char *)__FILE__)
void renderCheckError_(int lineNumber, char *fileName) {
    GLenum err = glGetError();
    if(err) {
        printf((char *)"GL error check: %x at %d in %s\n", err, lineNumber, fileName);
        EasyAssert(!err);
    }
    
}

typedef struct {
    s32 handle;
    bool valid;
} ShaderValInfo;

ShaderValInfo getAttribFromProgram(RenderProgram *prog, char *name) {
    ShaderValInfo result = {};
    for(int i = 0; i < prog->attribCount; ++i) {
        ShaderVal *val = prog->attribs + i;
        if(cmpStrNull(name, val->name)) {
            result.handle = val->handle;
            result.valid = true;
            break;
        }
    }
    if(!result.valid) {
        printf("%s\n", name);
    }
    EasyAssert(result.valid);
    return result;
}

ShaderValInfo getUniformFromProgram(RenderProgram *prog, char *name) {
    ShaderValInfo result = {};
    for(int i = 0; i < prog->uniformCount; ++i) {
        ShaderVal *val = prog->uniforms + i;
        if(cmpStrNull(name, val->name)) {
            result.handle = val->handle;
            //printf("%d\n", val->handle);
            //EasyAssert(result.handle > 0);
            result.valid = true;
            break;
        }
    }
    if(!result.valid) {
        printf("%s\n", name);
    }
    EasyAssert(result.valid);
    return result;
}

GLuint renderGetUniformLocation(RenderProgram *program, char *name) {
    GLuint result = glGetUniformLocation(program->glProgram, name);
    renderCheckError();
    return result;
}

GLuint renderGetAttribLocation(RenderProgram *program, char *name) {
    GLuint result = glGetAttribLocation(program->glProgram, name);
    renderCheckError();
    return result;
    
}

void findAttribsAndUniforms(RenderProgram *prog, char *stream, bool isVertexShader) {
    EasyTokenizer tokenizer = lexBeginParsing(stream, true);
    bool parsing = true;
    
    while(parsing) {
        char *at = tokenizer.src;
        EasyToken token = lexGetNextToken(&tokenizer);
        EasyAssert(at != tokenizer.src);
        switch(token.type) {
            case TOKEN_NULL_TERMINATOR: {
                parsing = false;
            } break;
            case TOKEN_WORD: {
                // lexPrintToken(&token);
                if(stringsMatchNullN("uniform", token.at, token.size)) {
                    lexGetNextToken(&tokenizer);
                    token = lexGetNextToken(&tokenizer);
                    char *name = nullTerminate(token.at, token.size);
                    //printf("Uniform Found: %s\n", name);
                    EasyAssert(prog->uniformCount < arrayCount(prog->uniforms));
                    ShaderVal *val = prog->uniforms + prog->uniformCount++;
                    val->name = name;
                    val->handle = renderGetUniformLocation(prog, name);
                    
                }
                if(stringsMatchNullN("in", token.at, token.size) && isVertexShader) {
                    lexGetNextToken(&tokenizer); //this is the type
                    token = lexGetNextToken(&tokenizer);
                    char *name = nullTerminate(token.at, token.size);
                    // printf("Attrib Found: %s\n", name);
                    EasyAssert(prog->attribCount < arrayCount(prog->attribs));
                    ShaderVal *val = prog->attribs + prog->attribCount++;
                    val->name = name;
                    val->handle = renderGetAttribLocation(prog, name);
                    
                }
            } break;
            default: {
                //don't mind
            }
        }
    }
}


RenderProgram createProgramFromFile(char *vertexShaderFilename, char *fragmentShaderFilename, bool isFileName) {
    char *vertMemory = vertexShaderFilename;
    char *fragMemory = fragmentShaderFilename;
    if(isFileName) {
        vertMemory = (char *)loadShader(vertexShaderFilename).memory;
        fragMemory= (char *)loadShader(fragmentShaderFilename).memory;
    } 
    
#if DESKTOP
    char *shaderVersion = "#version 150\n";
#else
    char *shaderVersion = "#version 300 es\nprecision mediump float;\n";
#endif
    char *vertStream = concat(shaderVersion, vertMemory);
    char *fragStream = concat(shaderVersion, fragMemory);
    
    RenderProgram result = createRenderProgram(vertStream, fragStream);
    
    findAttribsAndUniforms(&result, vertStream, true);
    findAttribsAndUniforms(&result, fragStream, false);
    
    free(vertStream);
    free(fragStream);
    if(isFileName) {
        free(vertMemory);
        free(fragMemory);
    }
    
    return result;
}

Matrix4 projectionMatrixFOV(float FOV_degrees, float aspectRatio) { //where aspect ratio = width/height of frame buffer resolution
    float nearClip = NEAR_CLIP_PLANE;
    float farClip = FAR_CLIP_PLANE;
    
    float FOV_radians = (FOV_degrees*PI32) / 180.0f;
    float t = tan(FOV_radians/2)*nearClip;
    float b = -t;
    float r = t*aspectRatio;
    float l = -r;
    
    float a1 = (2*nearClip) / (r - l); 
    float b1 = (2*nearClip) / (t - b);
    
    float c1 = (r + l) / (r - l);
    float d1 = (t + b) / (t - b);
    
    Matrix4 result = {{
            a1,  0,  0,  0,
            0,  b1,  0,  0,
            c1,  d1,  -((farClip + nearClip)/(farClip - nearClip)),  RENDER_HANDNESS, 
            0, 0,  (-2*nearClip*farClip)/(farClip - nearClip),  0
        }};
    
    return result;
}


Matrix4 projectionMatrixToScreen(int width, int height) {
    float a = 2 / (float)width; 
    float b = 2 / (float)height;
    
    float nearClip = NEAR_CLIP_PLANE;
    float farClip = FAR_CLIP_PLANE;
    
    Matrix4 result = {{
            a,  0,  0,  0,
            0,  b,  0,  0,
            0,  0,  -((farClip + nearClip)/(farClip - nearClip)),  RENDER_HANDNESS, 
            0, 0,  (-2*nearClip*farClip)/(farClip - nearClip),  0
        }};
    
    return result;
}


Matrix4 OrthoMatrixToScreen_(int width, int height, float offsetX, float offsetY) {
    float a = 2.0f / (float)width; 
    float b = 2.0f / (float)height;
    
    float nearClip = NEAR_CLIP_PLANE;
    float farClip = FAR_CLIP_PLANE;
    
    Matrix4 result = {{
            a,  0,  0,  0,
            0,  b,  0,  0,
            0,  0,  (-2)/(farClip - nearClip), 0, //definitley the projection coordinate. 
            offsetX, offsetY, -((farClip + nearClip)/(farClip - nearClip)),  1
        }};
    
    return result;
}

Matrix4 OrthoMatrixToScreen(int width, int height) {
    return OrthoMatrixToScreen_(width, height, 0, 0);
}

Matrix4 OrthoMatrixToScreen_BottomLeft(int width, int height) {
    return OrthoMatrixToScreen_(width, height, -1, -1);
}

static inline V3 screenSpaceToWorldSpace(Matrix4 perspectiveMat, V2 screenP, V2 resolution, float zAt, Matrix4 cameraToWorld) {
    
    V2 screenP_01 = v2(screenP.x / resolution.x, screenP.y / resolution.y);
    V2 ndcSpace = v2(inverse_lerp(-1, screenP_01.x, 1), inverse_lerp(-1, screenP_01.y, 1));

    V4 probePos = v4(0, 0, zAt, 1); //transform a position in the world to see what the clip space z would be
    
    V4 clipSpaceP_ = V4MultMat4(probePos, perspectiveMat);

    V4 clipSpaceP = v4(ndcSpace.x*clipSpaceP_.w, ndcSpace.y*clipSpaceP_.w, clipSpaceP_.z, clipSpaceP_.w);

    Matrix4 inverseMat = mat4();
    bool valid = mat4_inverse(perspectiveMat.E_, inverseMat.E_);
    EasyAssert(valid);
        
    V3 cameraSpaceP = V4MultMat4(clipSpaceP, inverseMat).xyz;

    V3 worldP = V4MultMat4(v3ToV4Homogenous(cameraSpaceP), cameraToWorld).xyz;

    return worldP;

}

V2 transformWorldPToScreenP(V2 inputA, float zPos, V2 resolution, V2 screenDim, ProjectionType type) {
    Matrix4 projMat;
    if(type == ORTHO_MATRIX) {
        projMat = OrthoMatrixToScreen(resolution.x, resolution.y);   
    } else if(type == PERSPECTIVE_MATRIX) {
        projMat = projectionMatrixToScreen(resolution.x, resolution.y);   
    }
    V4 screenSpace = transformPositionV3ToV4(v2ToV3(inputA, zPos), projMat);
    //Homogeneous divide -> does the perspective divide. 
    V3 screenSpaceV3 = v3(inputA.x / screenSpace.w, inputA.y / screenSpace.w, screenSpace.z / screenSpace.w);
    
    //Map back onto the screen. 
    V2 result = v2_plus(screenSpaceV3.xy, v2_scale(0.5f, resolution));
    result.x /= resolution.x;
    result.x *= screenDim.x;
    result.y /= resolution.y;
    result.y *= screenDim.y;
    return result;
}

V3 transformScreenPToWorldP(V2 inputA, float zPos, V2 resolution, V2 screenDim, Matrix4 metresToPixels, V3 cameraPos) {
    inputA.x /= screenDim.x;
    inputA.y /= screenDim.y;
    inputA.x *= resolution.x;
    inputA.y *= resolution.y;
    
    inputA = v2_minus(inputA, v2_scale(0.5f, resolution));
    
    V4 trans = transformPositionV3ToV4(v2ToV3(inputA, zPos), mat4_transpose(projectionMatrixToScreen(resolution.x, resolution.y)));
    
    inputA.x *= trans.w;
    inputA.y *= trans.w;
    
    V3 result = v2ToV3(inputA, zPos);
    result = V4MultMat4(v4(result.x, result.y, result.z, 1), mat4_transpose(metresToPixels)).xyz;
    result = v3_plus(result, cameraPos);
    
    return result;
}

void enableRenderer(int width, int height, Arena *arena) {
    
    globalRenderGroup = pushStruct(arena, RenderGroup);
#if RENDER_BACKEND == OPENGL_BACKEND
    glViewport(0, 0, width, height);
    renderCheckError();
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    renderCheckError();
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//Non premultiplied alpha textures! 
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    renderCheckError();
    //Premultiplied alpha textures! 
    glEnable(GL_DEPTH_TEST);
    //glDepthMask(GL_TRUE);  
    renderCheckError();
    glDepthFunc(GL_LEQUAL);///GL_LESS);//////GL_LEQUAL);//
    
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    //SRGB TEXTURE???
    // glEnable(GL_SAMPLE_ALPHA_TO_ONE);
    // glEnable(GL_DEBUG_OUTPUT);
    // renderCheckError();
    
#if DESKTOP
    glEnable(GL_MULTISAMPLE);
#endif
    
    char *append = concat(globalExeBasePath, (char *)"shaders/");
    
    // char *vertShaderLine = concat(append, (char *)"vertex_shader_line.glsl");
    // //printf("%s\n", vertShaderLine);
    // char *fragShaderLine = concat(append, (char *)"frag_shader_line.glsl");
    
    // char *vertShaderTex = concat(append, (char *)"vertex_shader_texture.c");
    // char *vertShaderRect = concat(append, (char *)"vertex_shader_rectangle.c");
    // char *fragShaderRect = concat(append, (char *)"fragment_shader_rectangle.glsl");
    // char *fragShaderTex = concat(append, (char *)"fragment_shader_texture.c");
    // char *fragShaderCirle = concat(append, (char *)"fragment_shader_circle.glsl");
    // char *fragShaderRectNoGrad = concat(append, (char *)"fragment_shader_rectangle_noGrad.glsl");
    // char *fragShaderFilter = concat(append, (char *)"fragment_shader_texture_filter.glsl");
    // char *fragShaderLight = concat(append, (char *)"fragment_shader_point_light.glsl");
    // char *fragShaderRing = concat(append, (char *)"frag_shader_ring.c");
    // char *fragShaderShadow = concat(append, (char *)"frag_shader_shadow.c");
    // char *fragShaderBlur = concat(append, (char *)"fragment_shader_blur.c");
    
    // char *vertPhong = concat(append, (char *)"vertex_model.c");
    // char *fragPhong = concat(append, (char *)"frag_model.c");
    
    // rectangleNoGradProgram  = createProgramFromFile(vertex_shader_rectangle_shader, fragShaderRectNoGrad);
    // renderCheckError();
    
    // lineProgram = createProgramFromFile(vertShaderLine, fragShaderLine);
    // renderCheckError();
    
    // rectangleProgram = createProgramFromFile(vertex_shader_rectangle_attrib_shader, fragment_shader_rectangle_shader, false);
    // renderCheckError();

    // phongProgram = createProgramFromFile(vertex_model_shader, frag_model_shader, false);
    // renderCheckError();
    
    textureProgram = createProgramFromFile(vertex_shader_tex_attrib_shader, fragment_shader_texture_shader, false);
    renderCheckError();
    
    // filterProgram = createProgramFromFile(vertShaderTex, fragShaderFilter);
    // renderCheckError();
    
    // circleProgram = createProgramFromFile(vertex_shader_rectangle_shader, fragShaderCirle);
    // renderCheckError();
    
    // lightProgram = createProgramFromFile(vertex_shader_rectangle_shader, fragShaderLight);
    // renderCheckError();
    
    // ringProgram = createProgramFromFile(vertex_shader_rectangle_shader, fragShaderRing);
    // renderCheckError();
    
    // shadowProgram = createProgramFromFile(vertex_shader_rectangle_shader, fragShaderShadow);
    // renderCheckError();
    
    // blurProgram = createProgramFromFile(vertex_shader_rectangle_shader, fragShaderBlur);
    // renderCheckError();
    // free(append);
#endif
}


static inline V4 hexARGBTo01Color(unsigned int color) {
    V4 result = {};
    
    result.x = (float)((color >> 16) & 0xFF) / 255.0f; //red
    result.z = (float)((color >> 0) & 0xFF) / 255.0f;
    result.y = (float)((color >> 8) & 0xFF) / 255.0f;
    result.w = (float)((color >> 24) & 0xFF) / 255.0f;
    return result;
}


GLuint renderLoadTexture(int width, int height, void *imageData) {
#if RENDER_BACKEND == OPENGL_BACKEND
    GLuint resultId;
    glGenTextures(1, &resultId);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D, resultId);
    renderCheckError();
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    renderCheckError();
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    renderCheckError();
#endif
    return resultId;
}

typedef struct {
    GLuint bufferId;
    GLuint textureId;
    GLuint depthId;
} FrameBuffer;


void renderReadPixels(u32 bufferId, int x0, int y0,
                      int x1,
                      int y1,
                      u32 layout,
                      u32 format,
                      u8 *stream) {
    
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)bufferId);
    
    glReadPixels(x0, y0,
                 x1, y1,
                 layout,
                 format,
                 stream);
}

void renderDeleteTextures(int count, GLuint *handle) {
	glDeleteTextures(1, handle);
}

void renderDeleteFramebuffers(int count, GLuint *handle) {
	glDeleteFramebuffers(1, handle);
}

void deleteFrameBuffer(FrameBuffer *frameBuffer) {
    if(frameBuffer->depthId != -1) {
        renderDeleteTextures(1, &frameBuffer->depthId);
    }
    renderDeleteTextures(1, &frameBuffer->textureId);
    renderDeleteFramebuffers(1, &frameBuffer->bufferId);
    
}

typedef enum {
    FRAMEBUFFER_DEPTH = 1 << 0,
    FRAMEBUFFER_STENCIL = 1 << 1,
} FrameBufferFlag;

FrameBuffer createFrameBuffer(int width, int height, int flags) {
    GLuint mainTexture = renderLoadTexture(width, height, 0);
    
    GLuint frameBufferHandle = 1;
    glGenFramebuffers(1, &frameBufferHandle);
    renderCheckError();
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
    renderCheckError();
    
    GLuint depthId = -1;
    
    if(flags) {
        glGenTextures(1, &depthId);
        renderCheckError();
        
        glBindTexture(GL_TEXTURE_2D, depthId);
        renderCheckError();
        
        if(!(flags & FRAMEBUFFER_STENCIL)) { //Just depth buffer
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
            renderCheckError();
            
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthId, 0);
            renderCheckError();    
        } else {
            //CREATE STENCIL BUFFER ALONG WITH DEPTH BUFFER
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
            renderCheckError();
            
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthId, 0);
            renderCheckError();    
        } 
    }
    
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
                           mainTexture, 0);
    renderCheckError();
    
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
    renderCheckError();
    
    EasyAssert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    
    FrameBuffer result = {};
    result.textureId = mainTexture;
    result.bufferId = frameBufferHandle;
    result.depthId = depthId; 
    
    return result;
}


FrameBuffer createFrameBufferMultiSample(int width, int height, int flags, int sampleCount) {
#if !DESKTOP
    FrameBuffer result = createFrameBuffer(width, height, flags);
#else
    GLuint textureId;
    glGenTextures(1, &textureId);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureId);
    renderCheckError();
    
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_RGBA8, width, height, GL_FALSE);
    renderCheckError();
    
    GLuint frameBufferHandle;
    glGenFramebuffers(1, &frameBufferHandle);
    renderCheckError();
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
    renderCheckError();
    
    if(flags) {
        GLuint resultId;
        glGenTextures(1, &resultId);
        renderCheckError();
        
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, resultId);
        renderCheckError();
        
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_DEPTH24_STENCIL8, width, height, GL_FALSE);
        renderCheckError();
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, resultId, 0);
        renderCheckError();
        
    }
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, 
                           textureId, 0);
    renderCheckError();
    
    EasyAssert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    
    FrameBuffer result = {};
    result.textureId = textureId;
    result.bufferId = frameBufferHandle;
#endif    
    return result;
}

void clearBufferAndBind(u32 bufferHandle, V4 color) {
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)bufferHandle); 
    
    setFrameBufferId(globalRenderGroup, bufferHandle);
    
    glClearColor(color.x, color.y, color.z, color.w);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);
    glClearDepthf(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); 
    GLenum err = glGetError();
}

typedef enum {
    DRAWCALL_SINGLE,
    DRAWCALL_INSTANCED,   
} DrawCallType;

static inline void addInstanceAttribForMatrix(int index, GLuint attribLoc, int numOfFloats, size_t offsetForStruct, size_t offsetInStruct, int divisor) {
    
    glEnableVertexAttribArray(attribLoc + index);  
    renderCheckError();
    
    glVertexAttribPointer(attribLoc + index, numOfFloats, GL_FLOAT, GL_FALSE, offsetForStruct, ((char *)0) + offsetInStruct + (4*sizeof(float)*index));
    renderCheckError();
    glVertexAttribDivisor(attribLoc + index, divisor);
    renderCheckError();
}

static inline void addInstancingAttrib (GLuint attribLoc, int numOfFloats, size_t offsetForStruct, size_t offsetInStruct, int divisor) {
    
    EasyAssert(attribLoc < GL_MAX_VERTEX_ATTRIBS);
    EasyAssert(offsetForStruct > 0);
    if(numOfFloats == 16) {
        addInstanceAttribForMatrix(0, attribLoc, 4, offsetForStruct, offsetInStruct, divisor);
        addInstanceAttribForMatrix(1, attribLoc, 4, offsetForStruct, offsetInStruct, divisor);
        addInstanceAttribForMatrix(2, attribLoc, 4, offsetForStruct, offsetInStruct, divisor);
        addInstanceAttribForMatrix(3, attribLoc, 4, offsetForStruct, offsetInStruct, divisor);
    } else {
        glEnableVertexAttribArray(attribLoc);  
        renderCheckError();
        
        EasyAssert(numOfFloats <= 4);
        glVertexAttribPointer(attribLoc, numOfFloats, GL_FLOAT, GL_FALSE, offsetForStruct, ((char *)0) + offsetInStruct);
        renderCheckError();
        
        glVertexAttribDivisor(attribLoc, divisor);
        renderCheckError();
    }
}

static inline void renderModel(VaoHandle *bufferHandles, 
    Matrix4 modelViewMatrix, 
    Matrix4 perspectiveMatrix, 
    RenderProgram *program) {

}

static inline void initVao(VaoHandle *bufferHandles, Vertex *triangleData, int triCount, unsigned int *indicesData, int indexCount, RenderProgram *program) {
    if(!bufferHandles->valid) {
        glGenVertexArrays(1, &bufferHandles->vaoHandle);
        renderCheckError();
        glBindVertexArray(bufferHandles->vaoHandle);
        renderCheckError();
        
        GLuint vertices;
        GLuint indices;
        
        glGenBuffers(1, &vertices);
        renderCheckError();
        
        glBindBuffer(GL_ARRAY_BUFFER, vertices);
        renderCheckError();
        
        glBufferData(GL_ARRAY_BUFFER, triCount*sizeof(Vertex), triangleData, GL_STATIC_DRAW);
        renderCheckError();
        
        glGenBuffers(1, &indices);
        renderCheckError();
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
        renderCheckError();
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount*sizeof(unsigned int), indicesData, GL_STATIC_DRAW);
        renderCheckError();
        
        bufferHandles->indexCount = indexCount;
        
        EasyAssert(!bufferHandles->valid);
        bufferHandles->valid = true;
        
        //these can also be retrieved before hand to speed up the process!!!
        GLint vertexAttrib = getAttribFromProgram(program, "vertex").handle;
        renderCheckError();
        ShaderValInfo texUV_val= getAttribFromProgram(program, "texUV");
        renderCheckError();
        
        if(texUV_val.valid) {
            
            GLint texUVAttrib = texUV_val.handle;
            glEnableVertexAttribArray(texUVAttrib);  
            renderCheckError();
            unsigned int uvByteOffset = (intptr_t)(&(((Vertex *)0)->texUV));
            glVertexAttribPointer(texUVAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + uvByteOffset);
            renderCheckError();
        }
        
        glEnableVertexAttribArray(vertexAttrib);  
        renderCheckError();
        glVertexAttribPointer(vertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        renderCheckError();

        glGenBuffers(1, &bufferHandles->vboHandle);
        renderCheckError();

        glBindBuffer(GL_ARRAY_BUFFER, bufferHandles->vboHandle);
        renderCheckError();

        GLint vmAttrib = getAttribFromProgram(program, "VM").handle;
        renderCheckError();
        GLint pAttrib = getAttribFromProgram(program, "P").handle;
        renderCheckError();
        GLint colorAttrib = getAttribFromProgram(program, "color").handle;
        renderCheckError();
        
        size_t offsetForStruct = sizeof(float)*(16+16+4+4); 
        
        //matrix plus vector4 plus vector4
        addInstancingAttrib (vmAttrib, 16, offsetForStruct, 0, 1);
        addInstancingAttrib (pAttrib, 16, offsetForStruct, sizeof(float)*16, 1);
        addInstancingAttrib (colorAttrib, 4, offsetForStruct, sizeof(float)*32, 1);
        // if(hasUvs) 
        {
            GLint UVattrib = getAttribFromProgram(program, "uvAtlas").handle;
            // printf("uv attrib: %d\n", UVattrib);
            renderCheckError();
            addInstancingAttrib (UVattrib, 4, offsetForStruct, sizeof(float)*36, 1);
        }
        
        EasyAssert(offsetForStruct == sizeof(float)*40);
        
        glBindVertexArray(0);
        
        glDeleteBuffers(1, &vertices);
        glDeleteBuffers(1, &indices);
    }
}


static V2 globalBlurDir = {};
void drawVao(VaoHandle *bufferHandles, RenderProgram *program, ShapeType type, u32 textureId, u32 PVMId, u32 colorId, u32 uvsId, V4 color, DrawCallType drawCallType, int instanceCount) {
    
    EasyAssert(bufferHandles);
    EasyAssert(bufferHandles->valid);
    
    glUseProgram(program->glProgram);
    renderCheckError();
    
    glBindVertexArray(bufferHandles->vaoHandle);
    renderCheckError();
    
#if 0
    glBindBuffer(GL_ARRAY_BUFFER, bufferHandles->vboHandle);
    renderCheckError();
#endif
    
#if !USING_ATTRIBS_FOR_INSTANCING
    GLint pvmUniform = getUniformFromProgram(program, "PVMArray").handle;
    //GLint pvmUniform = glGetUniformLocation(programId, "PVMArray");
    renderCheckError();
    
    glUniform1i(pvmUniform, 0);
    renderCheckError();
    glActiveTexture(GL_TEXTURE0);
    renderCheckError();
    
    glBindTexture(RENDER_TEXTURE_BUFFER_ENUM, PVMId); 
    renderCheckError();
    
    GLint colorUniform = getUniformFromProgram(program, "ColorArray").handle;
    // GLint colorUniform = glGetUniformLocation(programId, "ColorArray");
    renderCheckError();
    
    glUniform1i(colorUniform, 1);
    renderCheckError();
    glActiveTexture(GL_TEXTURE1);
    renderCheckError();
    
    glBindTexture(RENDER_TEXTURE_BUFFER_ENUM, colorId); 
    renderCheckError();
    
    if(uvsId) {
        GLint uvUniform = getUniformFromProgram(program, "UVArray").handle;
        renderCheckError();
        
        glUniform1i(uvUniform, 2);
        renderCheckError();
        glActiveTexture(GL_TEXTURE2);
        renderCheckError();
        
        glBindTexture(RENDER_TEXTURE_BUFFER_ENUM, uvsId); 
        renderCheckError();
    }
#endif
    
    if(type == SHAPE_TEXTURE || type == SHAPE_SHADOW || type == SHAPE_BLUR) {
        GLint texUniform = getUniformFromProgram(program, "tex").handle;
        //GLint texUniform = glGetUniformLocation(programId, "tex");
        renderCheckError();
        
        glUniform1i(texUniform, 3);
        renderCheckError();
        glActiveTexture(GL_TEXTURE3);
        renderCheckError();
        
        // printf("texture id: %d\n", textureId);
        glBindTexture(GL_TEXTURE_2D, textureId); 
        renderCheckError();
        
        if(type == SHAPE_BLUR) {
            //GLint directionUniform = glGetUniformLocation(programId, "dir");
            GLint directionUniform = getUniformFromProgram(program, "dir").handle;
            glUniform2f(directionUniform, globalBlurDir.x, globalBlurDir.y);
        }
        
        if(type == SHAPE_SHADOW) {
            // GLint fboWidthUniform = glGetUniformLocation(programId, "fboWidth");
            // glUniform1f(fboWidthUniform, );
            //Lighting info stuff
            // //light count 
            // GLint lightCountUniform = glGetUniformLocation(programId, "lightCount");
            // renderCheckError();
            
            // glUniform1i(lightCountUniform, globalLightInfoCount);
            // renderCheckError();
            
            // //lights
            // GLint lightPosUniform = glGetUniformLocation(programId, "lightsPos");
            // renderCheckError();
            
            // glUniform1fv(lightPosUniform, globalLightInfoCount*3, globalLightInfos);
            // renderCheckError();    
        }
        
    } else if(type == SHAPE_LINE || type == SHAPE_CIRCLE) {
        GLint percentUniform = getUniformFromProgram(program, "percentY").handle;
        //GLint percentUniform = glGetUniformLocation(programId, "percentY");
        renderCheckError();
    }
    
    
    if(drawCallType == DRAWCALL_SINGLE) {
        glDrawElements(GL_TRIANGLES, bufferHandles->indexCount, GL_UNSIGNED_INT, 0); 
        renderCheckError();
    } else if(drawCallType == DRAWCALL_INSTANCED) {
        // if(program == &rectangleProgram) {
            // printf("%s\n", "isQuad");
        // }
        glDrawElementsInstanced(GL_TRIANGLES, bufferHandles->indexCount, GL_UNSIGNED_INT, 0, instanceCount); 
        renderCheckError();
    }
    
    glBindVertexArray(0);

    renderCheckError();    
    glUseProgram(0);
    renderCheckError();
    
}

void getQuadVertexes(Vertex *triangleData) { //has to be length of four
    triangleData[0].position = globalQuadPositionData[0];
    triangleData[1].position = globalQuadPositionData[1];
    triangleData[2].position = globalQuadPositionData[2];
    triangleData[3].position = globalQuadPositionData[3];
    
    triangleData[0].texUV = v2(0, 0);
    triangleData[1].texUV = v2(0, 1);
    triangleData[2].texUV = v2(1, 0);
    triangleData[3].texUV = v2(1, 1);
}

#define renderDrawRectOutlineCenterDim(center, dim, color, rot, offsetTransform, projectionMatrix) renderDrawRectOutlineCenterDim_(center, dim, color, rot, offsetTransform, projectionMatrix, 0.1f)
void renderDrawRectOutlineCenterDim_(V3 center, V2 dim, V4 color, float rot, Matrix4 offsetTransform, Matrix4 projectionMatrix, float thickness) {
    V3 deltaP = transformPositionV3(center, offsetTransform);
    
    float a1 = cos(rot);
    float a2 = sin(rot);
    float b1 = cos(rot + HALF_PI32);
    float b2 = sin(rot + HALF_PI32);
    
    Matrix4 rotationMat = {{
            a1,  a2,  0,  0,
            b1,  b2,  0,  0,
            0,  0,  1,  0,
            deltaP.x, deltaP.y, deltaP.z,  1
        }};
    
    
    V2 halfDim = v2(0.5f*dim.x, 0.5f*dim.y);
    
    float rotations[4] = {
        0,
        (float)(PI32 + HALF_PI32),
        (float)(PI32),
        (float)(HALF_PI32)
    };
    
    float lengths[4] = {
        dim.y,
        dim.x, 
        dim.y,
        dim.x,
    };
    
    V2 offsets[4] = {
        v2(-halfDim.x, 0),
        v2(0, halfDim.y),
        v2(halfDim.x, 0),
        v2(0, -halfDim.y),
    };
    
    Vertex triangleData[4] = {};
    if(!globalQuadVaoHandle.valid) {
        getQuadVertexes(triangleData);
    }
    
    for(int i = 0; i < 4; ++i) {
        float rotat = rotations[i];
        float halfLen = 0.5f*lengths[i];
        V2 offset = offsets[i];
        
        Matrix4 rotationMat1 = {{
                thickness*(float)cos(rotat),  thickness*(float)sin(rotat),  0,  0,
                lengths[i]*-(float)sin(rotat),  lengths[i]*(float)cos(rotat),  0,  0,
                0,  0,  1,  0,
                offset.x, offset.y, 0,  1
            }};
        pushRenderItem(&globalQuadVaoHandle, globalRenderGroup, triangleData, arrayCount(triangleData), globalQuadIndicesData, arrayCount(globalQuadIndicesData), &textureProgram, SHAPE_RECTANGLE, 0, Mat4Mult(rotationMat, rotationMat1), projectionMatrix,  color, center.z);
    }
}

void renderDrawRectOutlineRect2f(Rect2f rect, V4 color, float rot, Matrix4 offsetTransform, Matrix4 projectionMatrix) {
    renderDrawRectOutlineCenterDim(v2ToV3(getCenter(rect), -1), getDim(rect), color, rot, offsetTransform, projectionMatrix);
}

//
void renderDrawRectCenterDim_(V3 center, V2 dim, V4 *colors, float rot, Matrix4 offsetTransform, Texture *texture, ShapeType type, RenderProgram *program, Matrix4 viewMatrix, Matrix4 projectionMatrix) {
    float a1 = cos(rot);
    float a2 = sin(rot);
    float b1 = cos(rot + HALF_PI32);
    float b2 = sin(rot + HALF_PI32);
    
    V3 deltaP = transformPositionV3(center, offsetTransform);
    Matrix4 rotationMat = {{
            dim.x*a1,  dim.x*a2,  0,  0,
            dim.y*b1,  dim.y*b2,  0,  0,
            0,  0,  1,  0,
            deltaP.x, deltaP.y, deltaP.z,  1
        }};
    
    Vertex triangleData[4] = {};
    if(!globalQuadVaoHandle.valid) {
        getQuadVertexes(triangleData);
        
    }
    if(globalImmediateModeGraphics) {
    } else {
        int triCount = arrayCount(triangleData);
        int indicesCount = arrayCount(globalQuadIndicesData);
        pushRenderItem(&globalQuadVaoHandle, globalRenderGroup, triangleData, triCount, 
                       globalQuadIndicesData, indicesCount, program, type, texture, 
                       Mat4Mult(viewMatrix, rotationMat), projectionMatrix, colors[0], center.z);
    }    
}

void renderDrawRectCenterDim(V3 center, V2 dim, V4 color, float rot, Matrix4 offsetTransform, Matrix4 projectionMatrix) {
    V4 colors[4] = {color, color, color, color}; 
    renderDrawRectCenterDim_(center, dim, colors, rot, offsetTransform, 0, SHAPE_RECTANGLE, &textureProgram, mat4(), projectionMatrix);
}

void renderDrawRect(Rect2f rect, float zValue, V4 color, float rot, Matrix4 offsetTransform, Matrix4 projectionMatrix) {
    V4 colors[4] = {color, color, color, color}; 
    renderDrawRectCenterDim_(v2ToV3(getCenter(rect), zValue), getDim(rect), colors, rot, 
                             offsetTransform, 0, SHAPE_RECTANGLE, &textureProgram, mat4(), projectionMatrix);
}

void renderTextureCentreDim(Texture *texture, V3 center, V2 dim, V4 color, float rot, Matrix4 offsetTransform, Matrix4 viewMatrix, Matrix4 projectionMatrix) {
    V4 colors[4] = {color, color, color, color}; 
    renderDrawRectCenterDim_(center, dim, colors, rot, offsetTransform, texture, SHAPE_TEXTURE, &textureProgram, viewMatrix, projectionMatrix);
}

void renderDeleteVaoHandle(VaoHandle *handles) {
    glDeleteVertexArrays(1, &handles->vaoHandle);
    renderCheckError();
    handles->vaoHandle = 0;
    handles->indexCount = 0;
    handles->valid = false;
}

RenderItem *getRenderItem(RenderGroup *group, int index) {
    RenderItem *info = 0;
    if(index < group->items.count) {
        info = (RenderItem *)getElementFromAlloc_(&group->items, index);
    }
    return info;
}

BufferStorage createBufferStorage(InfiniteAlloc *array) {
    BufferStorage result = {};
    glGenBuffers(1, &result.tbo);
    renderCheckError();
    // printf("TBO: %d\n", result.tbo);
    glBindBuffer(RENDER_TEXTURE_BUFFER_ENUM, result.tbo);
    renderCheckError();
    glBufferData(RENDER_TEXTURE_BUFFER_ENUM, array->sizeOfMember*array->count, array->memory, GL_DYNAMIC_DRAW);
    renderCheckError();
    
    glGenTextures(1, &result.buffer);
    renderCheckError();
    glBindTexture(RENDER_TEXTURE_BUFFER_ENUM, result.buffer);
    renderCheckError();
    
    glTexBuffer(RENDER_TEXTURE_BUFFER_ENUM, GL_RGBA32F, result.tbo);
    renderCheckError();
    
    return result;
}


//Instancing data
// typedef struct {
//     float pvm[16];
//     float color[4];
//     float uvs[4];
// }



//This is using vertex attribs
void createBufferStorage2(VaoHandle *vao, InfiniteAlloc *array, RenderProgram *program, bool hasUvs) {
    glBindVertexArray(vao->vaoHandle);
    renderCheckError();
    glBindBuffer(GL_ARRAY_BUFFER, vao->vboHandle);
    renderCheckError();
    
    //send the data to GPU. glBufferData deletes the 
    glBufferData(GL_ARRAY_BUFFER, array->sizeOfMember*array->count, array->memory, GL_DYNAMIC_DRAW);
    renderCheckError();
    
    glBindVertexArray(0);
    renderCheckError();
}

void deleteBufferStorage(BufferStorage *store) {
    // printf("buffer id: %d\n", store->buffer);
    glDeleteTextures(1, &store->buffer);
    renderCheckError();
    
    // printf("tbo id: %d\n", store->tbo);
    glDeleteBuffers(1, &store->tbo);
    renderCheckError();
}

int cmpRenderItemFunc (const void * a, const void * b) {
    RenderItem *itemA = (RenderItem *)a;
    RenderItem *itemB = (RenderItem *)b;
    bool result = true;
    
    if(itemA->zAt == itemB->zAt) {
        if(itemA->textureHandle == itemB->textureHandle) {
            if(itemA->bufferHandles == itemB->bufferHandles) {
                if(itemA->program == itemB->program) {
                    result = false;
                } else {
                    result = (intptr_t)itemA->program > (intptr_t)itemB->program;
                }
            } else {
                result = (intptr_t)itemA->bufferHandles > (intptr_t)itemB->bufferHandles;                
            }
        } else {
            result = (intptr_t)itemA->textureHandle > (intptr_t)itemB->textureHandle;    
        }
    } else {
        result = itemA->zAt > itemB->zAt;
    }
    
    return result;
}

void sortItems(RenderGroup *group) {
    //this is a bubble sort. I think this is typically a bit slow. 
    bool sorted = false;
    int max = (group->items.count - 1);
    for (int index = 0; index < max;) {
        bool incrementIndex = true;
        RenderItem *infoA = (RenderItem *)getElementFromAlloc_(&group->items, index);
        RenderItem *infoB = (RenderItem *)getElementFromAlloc_(&group->items, index + 1);
        EasyAssert(infoA && infoB);
        bool swap = cmpRenderItemFunc(infoA, infoB);
        if(swap) {
            RenderItem temp = *infoA;
            *infoA = *infoB;
            *infoB = temp;
            sorted = true;
        }   
        if(index == (max - 1) && sorted) {
            index = 0; 
            sorted = false;
            incrementIndex = false;
        }
        
        if(incrementIndex) {
            index++;
        }
    }
}

void beginRenderGroupForFrame(RenderGroup *group) {
    ////Delete the storeage buffers from last frame 
    //this is the pvm data and color data we send as tables to the GPU for the shader. 
    for(int i = 0; i < group->lastStorageBufferCount; ++i) {
        BufferStorage *store = group->lastBufferStorage + i;
        deleteBufferStorage(store);
    }
    group->lastStorageBufferCount = 0;
    //


}

void drawRenderGroup(RenderGroup *group) {
    sortItems(group);
    
    // for(int i = 0; i < group->items.count; ++i) {
    //     RenderItem *info = (RenderItem *)getElementFromAlloc_(&group->items, i);
    //     VaoHandle *handle = info->bufferHandles;
    //     if(handle) {
    //         if(handle->refresh) {
    //             renderDeleteVaoHandle(handle);
    //             EasyAssert(!handle->refresh);
    //             EasyAssert(!handle->valid);
    //         }
    //     }
    // }
    
    int drawCallCount = 0;
    
    // printf("Render Items count: %d\n", group->items.count);
    // int instanceIndexAt = 0;
    for(int i = 0; i < group->items.count; ++i) {
        RenderItem *info = (RenderItem *)getElementFromAlloc_(&group->items, i);
        glBindFramebuffer(GL_FRAMEBUFFER, info->bufferId);
        
        if(info->depthTest) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        
        switch(info->blendFuncType) {
            case BLEND_FUNC_ZERO_ONE_ZERO_ONE_MINUS_ALPHA: {
                glBlendFuncSeparate(GL_ZERO, GL_ONE, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
            } break;
            case BLEND_FUNC_STANDARD: {
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            } break;
            default: {
                EasyAssert(!"case not handled");
            }
        }
        InfiniteAlloc allInstanceData = initInfinteAlloc(float);        
        
        addElementInifinteAllocWithCount_(&allInstanceData, info->vmMat.val, 16);

        addElementInifinteAllocWithCount_(&allInstanceData, info->pMat.val, 16);
        
        addElementInifinteAllocWithCount_(&allInstanceData, info->color.E, 4);
        if(info->textureHandle != 0) {
            addElementInifinteAllocWithCount_(&allInstanceData, info->textureUVs.E, 4);
        } else {
            //We do this to keep the spacing correct for the struct.
            float nullData[4] = {};
            addElementInifinteAllocWithCount_(&allInstanceData, nullData, 4);
        }
        
        
        
        int instanceCount = 1;
        bool collecting = true;
        while(collecting) {
            RenderItem *nextItem = getRenderItem(group, i + 1);
            if(nextItem) {
                
                if(info->bufferHandles == nextItem->bufferHandles && info->textureHandle == nextItem->textureHandle && info->program == nextItem->program) {
                    
                    EasyAssert(info->blendFuncType == nextItem->blendFuncType);
                    EasyAssert(info->depthTest == nextItem->depthTest);
                    //collect data
                    addElementInifinteAllocWithCount_(&allInstanceData, nextItem->vmMat.val, 16);

                    addElementInifinteAllocWithCount_(&allInstanceData, nextItem->pMat.val, 16);
                    
                    addElementInifinteAllocWithCount_(&allInstanceData, nextItem->color.E, 4);
                    
                    
                    if(nextItem->textureHandle) {
                        addElementInifinteAllocWithCount_(&allInstanceData, nextItem->textureUVs.E, 4);
                    } else {
                        //We do this to keep the spacing correct for the struct.
                        float nullData[4] = {};
                        addElementInifinteAllocWithCount_(&allInstanceData, nullData, 4);
                    }
                    
                    instanceCount++;
                    releaseInfiniteAlloc(&nextItem->triangleData);
                    releaseInfiniteAlloc(&nextItem->indicesData);
                    //
                    i++;
                } else {
                    collecting = false;
                }
            } else {
                collecting = false;
            }
        }
        
        
        initVao(info->bufferHandles, (Vertex *)info->triangleData.memory, info->triCount, (unsigned int *)info->indicesData.memory, info->indexCount, info->program);
        
#if USING_ATTRIBS_FOR_INSTANCING
    
        createBufferStorage2(info->bufferHandles, &allInstanceData, info->program, info->textureHandle);
        BufferStorage pvmStore = {};
        BufferStorage colorStore = {};
        u32 uvId = 0;
#else
        BufferStorage pvmStore = createBufferStorage(&pvms);
        BufferStorage colorStore = createBufferStorage(&colors);
        BufferStorage uvStore = {};
        u32 uvId = 0;
        if(uvs.count > 0) {             
            uvStore = createBufferStorage(&uvs);
            uvId = uvStore.buffer;
        }
#endif
        //if(info->program == &rectangleProgram) //Debug: just draw rectangles
        {
            drawVao(info->bufferHandles, info->program, info->type, info->textureHandle, pvmStore.buffer, colorStore.buffer, uvId, info->color, DRAWCALL_INSTANCED, instanceCount);
            drawCallCount++;
        }
        
#if !USING_ATTRIBS_FOR_INSTANCING
        EasyAssert(group->lastStorageBufferCount < arrayCount(group->lastBufferStorage));
        group->lastBufferStorage[group->lastStorageBufferCount++] = pvmStore;
        EasyAssert(group->lastStorageBufferCount < arrayCount(group->lastBufferStorage));
        group->lastBufferStorage[group->lastStorageBufferCount++] = colorStore;
        if(uvs.count > 0) {
            EasyAssert(group->lastStorageBufferCount < arrayCount(group->lastBufferStorage));
            group->lastBufferStorage[group->lastStorageBufferCount++] = uvStore;
        }
#endif
        releaseInfiniteAlloc(&info->triangleData);
        releaseInfiniteAlloc(&info->indicesData);
        
        releaseInfiniteAlloc(&allInstanceData);
    }
    releaseInfiniteAlloc(&group->items);
#if PRINT_NUMBER_DRAW_CALLS
    printf("NUMBER OF DRAW CALLS: %d\n", drawCallCount);
#endif
    group->idAt = 0;
}

typedef enum {
    TEXTURE_FILTER_LINEAR, 
    TEXTURE_FILTER_NEAREST, 
} RenderTextureFilter;


Texture createTextureOnGPU(unsigned char *image, int w, int h, int comp, RenderTextureFilter filter) {
    Texture result = {};
    if(image) {
        
        result.width = w;
        result.height = h;
        result.uvCoords = rect2f(0, 0, 1, 1);
        
        glGenTextures(1, &result.id);
        
        glBindTexture(GL_TEXTURE_2D, result.id);
        
        GLuint filterVal = GL_LINEAR;
        if(filter == TEXTURE_FILTER_NEAREST) {
            filterVal = GL_NEAREST;
        } 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterVal);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        
        if(comp == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        } else if(comp == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        } else {
            EasyAssert(!"Channel number not handled!");
        }
        
        glBindTexture(GL_TEXTURE_2D, 0);
    } 
    
    return result;
}

Texture loadImage(char *fileName, RenderTextureFilter filter) {
    int w;
    int h;
    int comp = 4;
    unsigned char* image = stbi_load(fileName, &w, &h, &comp, STBI_rgb_alpha);
    
    if(image) {
        if(comp == 3) {
            stbi_image_free(image);
            image = stbi_load(fileName, &w, &h, &comp, STBI_rgb);
            EasyAssert(image);
            EasyAssert(comp == 3);
        }
    } else {
        printf("%s\n", fileName);
        EasyAssert(!"no image found");
    }
    
    Texture result = createTextureOnGPU(image, w, h, comp, filter);
    
    if(image) {
        stbi_image_free(image);
    }
    return result;
}
