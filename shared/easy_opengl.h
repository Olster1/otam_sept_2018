/*
File for easy opengl drawing functions for game mock ups or quick drawing, using the fixed function openGl pipeline. You have to link with opengl and include opengl header file in your project. 
*/

static float FEATHER_PIXELS = 2;

#define PI32 3.14159265359
#define NEAR_CLIP_PLANE 0.1;
#define FAR_CLIP_PLANE 10000.0f

#if !defined arrayCount
#define arrayCount(arg) (sizeof(arg) / sizeof(arg[0])) 
#endif

#if !defined EASY_MATH_H
#include "easy_math.h"
#endif

#include "easy_render.h"

#define STB_IMAGE_IMPLEMENTATION 1
#include "stb_image.h"

#if !defined FIXED_FUNCTION_PIPELINE
#define FIXED_FUNCTION_PIPELINE 0
// #include <OpenGl3/gl3.h>
#else 
#include <OpenGl/gl.h>
#endif

static bool globalImmediateModeGraphics = false;

#define PROJECTION_TYPE(FUNC) \
FUNC(PERSPECTIVE_MATRIX) \
FUNC(ORTHO_MATRIX) \

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
    GLint handle;
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
} GlProgram;

GlProgram lineProgram;
GlProgram rectangleProgram;
GlProgram rectangleNoGradProgram;
GlProgram textureProgram;
GlProgram circleProgram;
GlProgram filterProgram;
GlProgram lightProgram;
GlProgram ringProgram;
GlProgram shadowProgram;
GlProgram blurProgram;

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

typedef enum {
    SHAPE_RECTANGLE,
    SHAPE_RECTANGLE_GRAD,
    SHAPE_TEXTURE,
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
    bool refresh;// this could be a flag with valid
    
} GLBufferHandles;

typedef struct {
    InfiniteAlloc triangleData;
    int triCount; 

    InfiniteAlloc indicesData; 
    int indexCount; 

    GlProgram *program; 
    ShapeType type; 

    GLuint textureId; 

    Matrix4 PVM; 
    float zAt;
    float zoom;

    int id;

    V4 color;

    int bufferId;
    bool depthTest;
    BlendFuncType blendFuncType;

    GLBufferHandles *bufferHandles;
} RenderItem;


typedef struct {
    int currentBufferId;
    bool currentDepthTest;
    GLBufferHandles *currentBufferHandles;
    BlendFuncType blendFuncType;

    int idAt; 
    InfiniteAlloc items; //type: RenderItem

} RenderGroup;

RenderGroup initRenderGroup() {
    RenderGroup result = {};
    result.items = initInfinteAlloc(RenderItem);
    result.currentDepthTest = true;
    result.blendFuncType = BLEND_FUNC_STANDARD;
    return result;  
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

static RenderGroup globalRenderGroup = {};

void pushRenderItem(GLBufferHandles *handles, RenderGroup *group, Vertex *triangleData, int triCount, unsigned int *indicesData, int indexCount, GlProgram *program, ShapeType type, GLuint textureId, Matrix4 PVM, float zoom, V4 color, float zAt) {
    if(!isInfinteAllocActive(&group->items)) {
        group->items = initInfinteAlloc(RenderItem);
    }

    RenderItem *info = (RenderItem *)addElementInifinteAlloc_(&group->items, 0);
    assert(info);
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
    info->textureId = textureId;
    info->PVM = PVM;
    info->zoom = zoom;
}

typedef struct {
    InfiniteAlloc vertexData;
    InfiniteAlloc indicesData;
} DrawStateCache;

GlProgram createProgram(char *vShaderSource, char *fShaderSource) {
    GlProgram result = {};
    
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
    
    int  vlength,    flength,    plength;
    char vlog[2048];
    char flog[2048];
    char plog[2048];
    glGetShaderInfoLog(result.glShaderV, 2048, &vlength, vlog);
    glGetShaderInfoLog(result.glShaderF, 2048, &flength, flog);
    glGetProgramInfoLog(result.glProgram, 2048, &plength, plog);
    
    if(vlength || flength || plength) {
        result.valid = false;
        printf("%s\n", vShaderSource);
        printf("%s\n", fShaderSource);
        printf("%s\n", vlog);
        printf("%s\n", flog);
        printf("%s\n", plog);
        
    }
    
    assert(result.valid);
    
    return result;
}

#define glCheckError() glCheckError_(__LINE__, (char *)__FILE__)
void glCheckError_(int lineNumber, char *fileName) {
    GLenum err = glGetError();
    if(err) {
        printf((char *)"GL error check: %x at %d in %s\n", err, lineNumber, fileName);
        assert(!err);
    }
    
}

typedef struct {
    GLint handle;
    bool valid;
} ShaderValInfo;

ShaderValInfo getAttribFromProgram(GlProgram *prog, char *name) {
    ShaderValInfo result = {};
    for(int i = 0; i < prog->attribCount; ++i) {
        ShaderVal *val = prog->attribs + i;
        if(cmpStrNull(name, val->name)) {
            result.handle = val->handle;
            result.valid = true;
            break;
        }
    }
    assert(result.valid);
    return result;
}

ShaderValInfo getUniformFromProgram(GlProgram *prog, char *name) {
    ShaderValInfo result = {};
    for(int i = 0; i < prog->uniformCount; ++i) {
        ShaderVal *val = prog->uniforms + i;
        if(cmpStrNull(name, val->name)) {
            result.handle = val->handle;
            result.valid = true;
            break;
        }
    }
    assert(result.valid);
    return result;
}

void findAttribsAndUniforms(GlProgram *prog, char *stream, bool isVertexShader) {
    EasyTokenizer tokenizer = lexBeginParsing(stream, true);
    bool parsing = true;

    while(parsing) {
        char *at = tokenizer.src;
        EasyToken token = lexGetNextToken(&tokenizer);
        assert(at != tokenizer.src);
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
                    assert(prog->uniformCount < arrayCount(prog->uniforms));
                    ShaderVal *val = prog->uniforms + prog->uniformCount++;
                    val->name = name;
                    val->handle = glGetUniformLocation(prog->glProgram, name);
                    glCheckError();
                }
                if(stringsMatchNullN("in", token.at, token.size) && isVertexShader) {
                    lexGetNextToken(&tokenizer); //this is the type
                    token = lexGetNextToken(&tokenizer);
                    char *name = nullTerminate(token.at, token.size);
                    // printf("Attrib Found: %s\n", name);
                    assert(prog->attribCount < arrayCount(prog->attribs));
                    ShaderVal *val = prog->attribs + prog->attribCount++;
                    val->name = name;
                    val->handle = glGetAttribLocation(prog->glProgram, name);
                    glCheckError();
                }
            } break;
            default: {
                //don't mind
            }
        }
    }
}


GlProgram createProgramFromFile(char *vertexShaderFilename, char *fragmentShaderFilename) {
    FileContents vertShader = loadShader(vertexShaderFilename);
    FileContents fragShader= loadShader(fragmentShaderFilename);
    
#if DESKTOP
    char *shaderVersion = "#version 150\n";
#else
    char *shaderVersion = "#version 300 es\nprecision mediump float;\n";
#endif
    char *vertStream = concat(shaderVersion, (char *)vertShader.memory);
    char *fragStream = concat(shaderVersion, (char *)fragShader.memory);

    GlProgram result = createProgram(vertStream, fragStream);

    findAttribsAndUniforms(&result, vertStream, true);
    findAttribsAndUniforms(&result, fragStream, false);
    
    free(vertStream);
    free(fragStream);
    free(vertShader.memory);
    free(fragShader.memory);
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
            0,  0,  -((farClip + nearClip)/(farClip - nearClip)),  -1, 
            0, 0,  (-2*nearClip*farClip)/(farClip - nearClip),  0
        }};
    
    return result;
}


Matrix4 OrthoMatrixToScreen(int width, int height, float zoom) {
    float a = 2.0f / (float)width; 
    float b = 2.0f / (float)height;
    
    float nearClip = NEAR_CLIP_PLANE;
    float farClip = FAR_CLIP_PLANE;
    
    Matrix4 result = {{
        a,  0,  0,  0,
        0,  b,  0,  0,
        0,  0,  (-2)/(farClip - nearClip), 0, //definitley the projection coordinate. 
        -1, -1, -((farClip + nearClip)/(farClip - nearClip)),  1
    }};
    
    // Matrix4 scaleMat = mat4();
    
    //we do this afterwards to scale from the center. 
    //scaleMat.a.x *= zoom; 
    //scaleMat.b.y *= zoom;
    //I think we have to do this to all the zises. 
    
    
    // result = Mat4Mult(scaleMat, result);
    
    return result;
}

Matrix4 OrthoMatrixToScreenViewPort(float zoom) {
    
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    Matrix4 result = OrthoMatrixToScreen(viewport[2], viewport[3], zoom);
    return result;
}

void OpenGlAdjustScreenDim(int width, int height) {
#if FIXED_FUNCTION_PIPELINE
    glMatrixMode(GL_PROJECTION); 
    
    float a = 2.0f / (float)width; 
    float b = 2.0f / (float)height;
    float ProjMat[] = {
        a,  0,  0,  0,
        0,  b,  0,  0,
        0,  0,  1,  0,
        -1, -1, 0,  1
    };
    
    glLoadMatrixf(ProjMat);
#else 
    //OrthoMatrixToScreenViewPort(1);
#endif
}

V2 transformWorldPToScreenP(V2 inputA, float zPos, V2 resolution, V2 screenDim, ProjectionType type) {
    Matrix4 projMat;
    if(type == ORTHO_MATRIX) {
        projMat = OrthoMatrixToScreen(resolution.x, resolution.y, 1);   
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

void enableOpenGl(int width, int height) {
    glViewport(0, 0, width, height);
    glCheckError();
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glCheckError();
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//Non premultiplied alpha textures! 
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glCheckError();
    //Premultiplied alpha textures! 
    glEnable(GL_DEPTH_TEST);
    //glDepthMask(GL_TRUE);  
    glCheckError();
    glDepthFunc(GL_LEQUAL);///GL_LESS);//////GL_LEQUAL);//

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    //SRGB TEXTURE???
    // glEnable(GL_SAMPLE_ALPHA_TO_ONE);
    
#if DESKTOP
    glEnable(GL_MULTISAMPLE);
#endif
    
#if FIXED_FUNCTION_PIPELINE
    glEnable(GL_TEXTURE_2D);
    
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); 
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION); 
    OpenGlAdjustScreenDim(width, height);
    
#else
    char *append = concat(globalExeBasePath, (char *)"shaders/");

    char *vertShaderLine = concat(append, (char *)"vertex_shader_line.glsl");
    //printf("%s\n", vertShaderLine);
    char *fragShaderLine = concat(append, (char *)"frag_shader_line.glsl");
    
    char *vertShaderTex = concat(append, (char *)"vertex_shader_texture.c");
    char *vertShaderRect = concat(append, (char *)"vertex_shader_rectangle.c");
    char *fragShaderRect = concat(append, (char *)"fragment_shader_rectangle.glsl");
    char *fragShaderTex = concat(append, (char *)"fragment_shader_texture.glsl");
    char *fragShaderCirle = concat(append, (char *)"fragment_shader_circle.glsl");
    char *fragShaderRectNoGrad = concat(append, (char *)"fragment_shader_rectangle_noGrad.glsl");
    char *fragShaderFilter = concat(append, (char *)"fragment_shader_texture_filter.glsl");
    char *fragShaderLight = concat(append, (char *)"fragment_shader_point_light.glsl");
    char *fragShaderRing = concat(append, (char *)"frag_shader_ring.c");
    char *fragShaderShadow = concat(append, (char *)"frag_shader_shadow.c");
    char *fragShaderBlur = concat(append, (char *)"fragment_shader_blur.c");
    
    // rectangleNoGradProgram  = createProgramFromFile(vertShaderRect, fragShaderRectNoGrad);
    // glCheckError();
    
    // lineProgram = createProgramFromFile(vertShaderLine, fragShaderLine);
    // glCheckError();
    
    rectangleProgram = createProgramFromFile(vertShaderRect, fragShaderRect);
    glCheckError();
    
    textureProgram = createProgramFromFile(vertShaderTex, fragShaderTex);
    glCheckError();

    // filterProgram = createProgramFromFile(vertShaderTex, fragShaderFilter);
    // glCheckError();
    
    // circleProgram = createProgramFromFile(vertShaderRect, fragShaderCirle);
    // glCheckError();

    // lightProgram = createProgramFromFile(vertShaderRect, fragShaderLight);
    // glCheckError();

    // ringProgram = createProgramFromFile(vertShaderRect, fragShaderRing);
    // glCheckError();

    // shadowProgram = createProgramFromFile(vertShaderRect, fragShaderShadow);
    // glCheckError();

    // blurProgram = createProgramFromFile(vertShaderRect, fragShaderBlur);
    // glCheckError();
    
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


GLuint openGlLoadTexture(int width, int height, void *imageData) {
    GLuint resultId;
    glGenTextures(1, &resultId);
    glCheckError();
    
    glBindTexture(GL_TEXTURE_2D, resultId);
    glCheckError();
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCheckError();
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    glCheckError();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glCheckError();
    return resultId;
}

typedef struct {
    GLuint bufferId;
    GLuint textureId;
    GLuint depthId;
} FrameBuffer;

void deleteFrameBuffer(FrameBuffer *frameBuffer) {
    if(frameBuffer->depthId != (GLuint)-1) {
        glDeleteTextures(1, &frameBuffer->depthId);
    }
    glDeleteTextures(1, &frameBuffer->textureId);
    glDeleteFramebuffers(1, &frameBuffer->bufferId);
    
}

typedef enum {
    FRAMEBUFFER_DEPTH = 1 << 0,
    FRAMEBUFFER_STENCIL = 1 << 1,
} FrameBufferFlag;

FrameBuffer createFrameBuffer(int width, int height, int flags) {
    GLuint mainTexture = openGlLoadTexture(width, height, 0);
    glCheckError();
    
    GLuint frameBufferHandle = 1;
    glGenFramebuffers(1, &frameBufferHandle);
    glCheckError();
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
    glCheckError();
    
    GLuint depthId = -1;

     if(flags) {
         glGenTextures(1, &depthId);
         glCheckError();
        
         glBindTexture(GL_TEXTURE_2D, depthId);
         glCheckError();
            
         if(!(flags & FRAMEBUFFER_STENCIL)) { //Just depth buffer
             glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
             glCheckError();
            
             glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthId, 0);
             glCheckError();    
         } else {
             //CREATE STENCIL BUFFER ALONG WITH DEPTH BUFFER
             glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
             glCheckError();
            
             glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthId, 0);
             glCheckError();    
         } 
     }


    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
                           mainTexture, 0);
    glCheckError();
    
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
    glCheckError();

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    
    FrameBuffer result = {};
    result.textureId = mainTexture;
    result.bufferId = frameBufferHandle;
    result.depthId = depthId; 
    
    return result;
}


FrameBuffer createFrameBufferMultiSample(int width, int height, int flags, int sampleCount) {
    #if FIXED_FUNCTION_PIPELINE || !DESKTOP
    FrameBuffer result = createFrameBuffer(width, height, flags);
    #else
    GLuint textureId;
    glGenTextures(1, &textureId);
    glCheckError();
    
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureId);
    glCheckError();
    
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_RGBA8, width, height, GL_FALSE);
    glCheckError();
    
    GLuint frameBufferHandle;
    glGenFramebuffers(1, &frameBufferHandle);
    glCheckError();
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
    glCheckError();
    
    if(flags) {
        GLuint resultId;
        glGenTextures(1, &resultId);
        glCheckError();
        
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, resultId);
        glCheckError();
        
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_DEPTH24_STENCIL8, width, height, GL_FALSE);
        glCheckError();
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, resultId, 0);
        glCheckError();
        
    }
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, 
                           textureId, 0);
    glCheckError();
    
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    
    FrameBuffer result = {};
    result.textureId = textureId;
    result.bufferId = frameBufferHandle;
    #endif    
    return result;
}


typedef struct {
    //We 'cook' the vertex data and keep it round. Then send down all the vertex data in one big draw call. 
    Vertex triangleData[4];
    unsigned int indicesData[6];
    bool invalid;
} CookedData;


typedef struct {
    //We 'cook' the vertex data and keep it round. Then send down all the vertex data in one big draw call. 
    InfiniteAlloc vertices;
    InfiniteAlloc indices;
} Vertices;

//use scratch arena instead of malloc
void addVertices(Arena *memoryArena, Vertices *vertices, CookedData *data) {
    expandMemoryArray(&vertices->vertices);
    
    int startTriCount = vertices->vertices.count;
    
    for(int i = 0; i < arrayCount(data->triangleData); i++) {
        ((Vertex *)(vertices->vertices.memory))[vertices->vertices.count++] = data->triangleData[i];
    }
    
    expandMemoryArray(&vertices->indices);
    
    for(int i = 0; i < arrayCount(data->indicesData); i++) {
        ((unsigned int *)(vertices->indices.memory))[vertices->indices.count++] = data->indicesData[i] + startTriCount;
    }
}

void clearBufferAndBind(GLuint bufferHandle, V4 color) {
    glBindFramebuffer(GL_FRAMEBUFFER, bufferHandle); 
    
    setFrameBufferId(&globalRenderGroup, bufferHandle);

    glClearColor(color.x, color.y, color.z, color.w);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);
    glClearDepthf(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); 
    GLenum err = glGetError();
}

#if !FIXED_FUNCTION_PIPELINE
//This call can be used for batch draw calls. It assumes the vertices are already in world space, so there is no need for a per shape rotation matrix.  


//initialization
// glGenVertexArrays
// glBindVertexArray

// glGenBuffers
// glBindBuffer
// glBufferData

// glVertexAttribPointer
// glEnableVertexAttribArray

// glBindVertexArray(0)

// glDeleteBuffers //you can already delete it after the VAO is unbound, since the
//                 //VAO still references it, keeping it alive (see comments below).

// ...

// //rendering
// glBindVertexArray
// glDrawWhatever

typedef enum {
    DRAWCALL_SINGLE,
    DRAWCALL_INSTANCED,   
} DrawCallType;

#define loadVertices(bufferHandles, triangleData, triCount, indicesData, indexCount_, program, type, textureId, PVM, zoom, color) loadVertices_(bufferHandles, triangleData, triCount, indicesData, indexCount_, program, type, textureId, 0, 0, zoom, color, DRAWCALL_SINGLE, 1)
static V2 globalBlurDir = {};
void loadVertices_(GLBufferHandles *bufferHandles, Vertex *triangleData, int triCount, unsigned int *indicesData, int indexCount_, GlProgram *program, ShapeType type, GLuint textureId, GLuint PVMId, GLuint colorId, float zoom, V4 color, DrawCallType drawCallType, int instanceCount) {
    
    glUseProgram(program->glProgram);
    glCheckError();
    
    GLuint vaoHandle;  
    GLuint vertices;
    GLuint indices;

    int indexCount = indexCount_;

    bool initialization = true;
    if(bufferHandles && bufferHandles->valid) {
        vaoHandle = bufferHandles->vaoHandle;
        indexCount = bufferHandles->indexCount;
        assert(!bufferHandles->refresh);
        glBindVertexArray(vaoHandle);
        glCheckError();
        initialization = false;
    } else {
        glGenVertexArrays(1, &vaoHandle);
        glCheckError();
        glBindVertexArray(vaoHandle);
        glCheckError();

        glGenBuffers(1, &vertices);
        glCheckError();
        // printf("INITIING %d\n", vertices);
        glBindBuffer(GL_ARRAY_BUFFER, vertices);
        glCheckError();
        
        glBufferData(GL_ARRAY_BUFFER, triCount*sizeof(Vertex), triangleData, GL_DYNAMIC_DRAW);
        glCheckError();
        
        glGenBuffers(1, &indices);
        // printf("INITIING %d\n", indices);
        glCheckError();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
        glCheckError();
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount*sizeof(unsigned int), indicesData, GL_DYNAMIC_DRAW);
        glCheckError();

        if(bufferHandles) {
            assert(!bufferHandles->valid);
            bufferHandles->vaoHandle = vaoHandle;
            bufferHandles->indexCount = indexCount;
            bufferHandles->valid = true;
            assert(!bufferHandles->refresh);
        }
    }

    GLint pvmUniform = getUniformFromProgram(program, "PVMArray").handle;
    //GLint pvmUniform = glGetUniformLocation(programId, "PVMArray");
    glCheckError();
    
    glUniform1i(pvmUniform, 0);
    glCheckError();
    glActiveTexture(GL_TEXTURE0);
    glCheckError();
    
    glBindTexture(GL_TEXTURE_BUFFER, PVMId); 
    glCheckError();

    GLint colorUniform = getUniformFromProgram(program, "ColorArray").handle;
    // GLint colorUniform = glGetUniformLocation(programId, "ColorArray");
    glCheckError();
    
    glUniform1i(colorUniform, 1);
    glCheckError();
    glActiveTexture(GL_TEXTURE1);
    glCheckError();
    
    glBindTexture(GL_TEXTURE_BUFFER, colorId); 
    glCheckError();

/*
    GLint PVMUniform = glGetUniformLocation(programId, "PVM");
    glCheckError();

    glUniformMatrix4fv(PVMUniform, pvmCount, GL_FALSE, (float *)PVMs);
    glCheckError();
*/

    // GLint colorUniform = glGetUniformLocation(programId, "color");
    // glCheckError();
        
    // glUniform4f(colorUniform, color.x, color.y, color.z, color.w);
    // glCheckError();
    
    if(initialization) {
        glBindBuffer(GL_ARRAY_BUFFER, vertices);
        glCheckError();
    }
    
    if(type == SHAPE_TEXTURE || type == SHAPE_SHADOW || type == SHAPE_BLUR) {
        GLint texUniform = getUniformFromProgram(program, "tex").handle;
        //GLint texUniform = glGetUniformLocation(programId, "tex");
        glCheckError();
        
        glUniform1i(texUniform, 2);
        glCheckError();
        glActiveTexture(GL_TEXTURE2);
        glCheckError();
        
        // printf("texture id: %d\n", textureId);
        glBindTexture(GL_TEXTURE_2D, textureId); 
        glCheckError();

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
            // glCheckError();
            
            // glUniform1i(lightCountUniform, globalLightInfoCount);
            // glCheckError();

            // //lights
            // GLint lightPosUniform = glGetUniformLocation(programId, "lightsPos");
            // glCheckError();
            
            // glUniform1fv(lightPosUniform, globalLightInfoCount*3, globalLightInfos);
            // glCheckError();    
        }
            
    } else if(type == SHAPE_LINE || type == SHAPE_CIRCLE) {
        GLint percentUniform = getUniformFromProgram(program, "percentY").handle;
        //GLint percentUniform = glGetUniformLocation(programId, "percentY");
        glCheckError();
        
        if(type == SHAPE_LINE) {
            
            if(initialization) {
                GLint lenAttrib = getAttribFromProgram(program, "lengthRatioIn").handle;
                // GLint lenAttrib = glGetAttribLocation(programId, "lengthRatioIn");
                glCheckError();
                glEnableVertexAttribArray(lenAttrib);  
                glCheckError();
                unsigned int len_offset = 12;
                glVertexAttribPointer(lenAttrib, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + (len_offset*sizeof(float)));
                glCheckError();
                
                GLint percentAttrib = getAttribFromProgram(program, "percentY_").handle;
                //GLint percentAttrib = glGetAttribLocation(programId, "percentY_");
                glCheckError();
                glEnableVertexAttribArray(percentAttrib);  
                glCheckError();
                unsigned int percentY_offset = 13;
                glVertexAttribPointer(percentAttrib, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + (percentY_offset*sizeof(float)));
                glCheckError();
            }
            
        }
    }
    
    if(initialization)  {
        //these can also be retrieved before hand to speed up the process!!!
        GLint vertexAttrib = getAttribFromProgram(program, "vertex").handle;
        //GLint vertexAttrib = glGetAttribLocation(programId, "vertex");
        glCheckError();
        GLint texUVAttrib = getAttribFromProgram(program, "texUV").handle;
        //GLint texUVAttrib = glGetAttribLocation(programId, "texUV");
        glCheckError();
        // GLint instanceAttrib = getAttribFromProgram(program, "instanceIndex").handle;
        // GLint instanceAttrib = glGetAttribLocation(programId, "instanceIndex");
        // glCheckError();
        
        // GLint colorAttrib = glGetAttribLocation(programId, "color");
        // glCheckError();
        
        glEnableVertexAttribArray(texUVAttrib);  
        glCheckError();
        unsigned int texUV_offset = 6;
        glVertexAttribPointer(texUVAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + (texUV_offset*sizeof(float)));
        glCheckError();
        
        // glEnableVertexAttribArray(colorAttrib);  
        // glCheckError();
        // unsigned int color_offset = 8;
        // glVertexAttribPointer(colorAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + (color_offset*sizeof(float)));
        // glCheckError();

        // glEnableVertexAttribArray(instanceAttrib);  
        // glCheckError();
        // unsigned int byteOffset = (intptr_t)(&(((Vertex *)0)->instanceIndex));
        // glVertexAttribIPointer(instanceAttrib, 1, GL_INT, sizeof(Vertex), ((char *)0) + byteOffset);
        // glCheckError();


        glEnableVertexAttribArray(vertexAttrib);  
        glCheckError();
        glVertexAttribPointer(vertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        glCheckError();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    
    }
    
    if(drawCallType == DRAWCALL_SINGLE) {
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0); 
        glCheckError();
    } else if(drawCallType == DRAWCALL_INSTANCED) {
        glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0, instanceCount); 
        glCheckError();
    }
    
    glBindVertexArray(0);
    
    if(initialization) {
        glDeleteBuffers(1, &vertices);
        glDeleteBuffers(1, &indices);
    }

    if(!bufferHandles) {
        glDeleteBuffers(1, &vertices);
        glDeleteBuffers(1, &indices);
        glDeleteVertexArrays(1, &vaoHandle);
    }
    
    glUseProgram(0);
    
}
#endif

void openGlDrawRectOutlineCenterDim(GLBufferHandles *handles, V3 center, V2 dim, V4 color, float rot, Matrix4 offsetTransform, float zoom, Matrix4 projectionMatrix) {
#if FIXED_FUNCTION_PIPELINE
    glDisable(GL_TEXTURE_2D);
    glMatrixMode(GL_MODELVIEW);
    glColor4f(color.x, color.y, color.z, color.w);
    
#endif
    V3 deltaP = V4MultMat4(v3ToV4Homogenous(center), offsetTransform).xyz; 
    
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
        PI32 + HALF_PI32,
        PI32,
        HALF_PI32
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
    
    float thickness = 0.1;
    
    for(int i = 0; i < 4; ++i) {
        float rotat = rotations[i];
        float halfLen = 0.5f*lengths[i];
        V2 offset = offsets[i];
        
        Matrix4 rotationMat1 = {{
                cos(rotat),  sin(rotat),  0,  0,
                -sin(rotat),  cos(rotat),  0,  0,
                0,  0,  1,  0,
                offset.x, offset.y, 0,  1
            }};
#if FIXED_FUNCTION_PIPELINE
        glLoadMatrixf(rotationMat.val);
        glMultMatrixf(rotationMat1.val);
        
        glBegin(GL_TRIANGLES);
        glTexCoord2f(0, 0);
        glVertex2f(0, -halfLen);
        glTexCoord2f(0, 1);
        glVertex2f(0, halfLen);
        glTexCoord2f(1, 1);
        glVertex2f(thickness, halfLen);
        
        glTexCoord2f(0, 0);
        glVertex2f(0, -halfLen);
        glTexCoord2f(1, 0);
        glVertex2f(thickness, -halfLen);
        glTexCoord2f(1, 1);
        glVertex2f(thickness, halfLen);
        glEnd();
#else 
        GLenum err;
        
        Vertex triangleData[4] = {};
        triangleData[0].color = color;
        triangleData[0].texUV = v2(0, 0);
        triangleData[0].position = v3(0, -halfLen, 0);
        
        triangleData[1].color = color;
        triangleData[1].texUV = v2(0, 1);
        triangleData[1].position = v3(0, halfLen, 0);
        
        triangleData[2].color = color;
        triangleData[2].texUV = v2(1, 0);
        triangleData[2].position = v3(thickness, -halfLen, 0);
        
        triangleData[3].color = color;
        triangleData[3].texUV = v2(1, 1);
        triangleData[3].position = v3(thickness, halfLen, 0);
        
        unsigned int indicesData[6] = {0, 1, 3, 0, 2, 3};
        
        for(int j = 0; j < 4; j++) {
            // Matrix4 rot = Mat4Mult(rotationMat1, rotationMat);
            // triangleData[j].position = transformPositionV3(triangleData[j].position, rot);
        }
        
        //the vertices 
        if(globalImmediateModeGraphics) {
            loadVertices(handles, triangleData, arrayCount(triangleData), indicesData, arrayCount(indicesData), &rectangleProgram, SHAPE_RECTANGLE, 0, projectionMatrix, zoom, color);
        } else {
            pushRenderItem(handles, &globalRenderGroup, triangleData, arrayCount(triangleData), indicesData, arrayCount(indicesData), &rectangleProgram, SHAPE_RECTANGLE, 0, Mat4Mult(projectionMatrix, Mat4Mult(rotationMat, rotationMat1)), zoom, color, center.z);
        }
        
#endif
    }
#if FIXED_FUNCTION_PIPELINE
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
#endif
    
}

void openGlDrawRectOutlineRect2f(GLBufferHandles *handles, Rect2f rect, V4 color, float rot, Matrix4 offsetTransform, float zoom, Matrix4 projectionMatrix) {
    openGlDrawRectOutlineCenterDim(handles, v2ToV3(getCenter(rect), -1), getDim(rect), color, rot, offsetTransform, zoom, projectionMatrix);
}

static inline void gl_SetColor(V4 color) {
#if FIXED_FUNCTION_PIPELINE
    glColor4f(color.x, color.y, color.z, color.w);
#endif
}

//
void openGlDrawRectCenterDim_(GLBufferHandles *handles, V3 center, V2 dim, V4 *colors, float rot, Matrix4 offsetTransform, GLint textureId, ShapeType type, GlProgram *program, float zoom, Matrix4 viewMatrix, Matrix4 projectionMatrix) {
    
#if FIXED_FUNCTION_PIPELINE
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#endif
    Vertex triangleData[4] = {};
    unsigned int indicesData[6] = {0, 1, 3, 0, 2, 3};

    float a1 = cos(rot);
    float a2 = sin(rot);
    float b1 = cos(rot + HALF_PI32);
    float b2 = sin(rot + HALF_PI32);
    
    V4 centerV4 = v3ToV4Homogenous(center);
    
    V3 deltaP = V4MultMat4(centerV4, offsetTransform).xyz; 
    Matrix4 rotationMat = {{
            dim.x*a1,  dim.x*a2,  0,  0,
            dim.y*b1,  dim.y*b2,  0,  0,
            0,  0,  1,  0,
            deltaP.x, deltaP.y, deltaP.z,  1
        }};

    if(!handles || !handles->valid || handles->refresh) {   
        V2 halfDim = v2(0.5f, 0.5f); //Dim is 1 and we pass the actual dim in the PVM matrix
    #if FIXED_FUNCTION_PIPELINE
        glMultMatrixf(rotationMat.val);
        
        //colors specified clockwise. 
        glBegin(GL_TRIANGLES);
        gl_SetColor(colors[0]);
        glTexCoord2f(0, 0);
        glVertex2f(-halfDim.x, -halfDim.y);
        
        gl_SetColor(colors[1]);
        glTexCoord2f(0, 1);
        glVertex2f(-halfDim.x, halfDim.y);
        
        gl_SetColor(colors[2]);
        glTexCoord2f(1, 1);
        glVertex2f(halfDim.x, halfDim.y);
        
        gl_SetColor(colors[0]);
        glTexCoord2f(0, 0);
        glVertex2f(-halfDim.x, -halfDim.y);
        
        gl_SetColor(colors[3]);
        glTexCoord2f(1, 0);
        glVertex2f(halfDim.x, -halfDim.y);
        
        gl_SetColor(colors[2]);
        glTexCoord2f(1, 1);
        glVertex2f(halfDim.x, halfDim.y);
        glEnd();
    #else 
        GLenum err;
        
        triangleData[0].color = colors[0];
        triangleData[0].texUV = v2(0, 0);
        triangleData[0].position = v3(-halfDim.x, -halfDim.y, 0);
        
        triangleData[1].color = colors[1];
        triangleData[1].texUV = v2(0, 1);
        triangleData[1].position = v3(-halfDim.x, halfDim.y, 0);
        
        triangleData[2].color = colors[2];
        triangleData[2].texUV = v2(1, 0);
        triangleData[2].position = v3(halfDim.x, -halfDim.y, 0);
        
        triangleData[3].color = colors[3];
        triangleData[3].texUV = v2(1, 1);
        triangleData[3].position = v3(halfDim.x, halfDim.y, 0);
        


        // for(int i = 0; i < 4; i++) {
        //     triangleData[i].position = transformPositionV3(triangleData[i].position, rotationMat);
        // }
    }
    if(globalImmediateModeGraphics) {
        //loadVertices(0, triangleData, arrayCount(triangleData), indicesData, arrayCount(indicesData), programId, type, textureId, Mat4Mult(projectionMatrix, Mat4Mult(viewMatrix, rotationMat)), zoom);
    } else {
        int triCount = arrayCount(triangleData);
        int indicesCount = arrayCount(indicesData);
        if(handles && !(!handles->valid || handles->refresh)) {
            triCount = 0;
            indicesCount = 0;

        } 
        pushRenderItem(handles, &globalRenderGroup, triangleData, triCount, indicesData, indicesCount, program, type, textureId, Mat4Mult(projectionMatrix, Mat4Mult(viewMatrix, rotationMat)), zoom, colors[0], center.z);
    }    
    
    
#endif
}

void openGlDrawRectCenterDim(GLBufferHandles *handles, V3 center, V2 dim, V4 color, float rot, Matrix4 offsetTransform, float zoom, Matrix4 projectionMatrix) {
#if FIXED_FUNCTION_PIPELINE
    glDisable(GL_TEXTURE_2D);
#endif
    V4 colors[4] = {color, color, color, color}; 
    openGlDrawRectCenterDim_(handles, center, dim, colors, rot, offsetTransform, 0, SHAPE_RECTANGLE, &rectangleProgram, zoom, mat4(), projectionMatrix);
#if FIXED_FUNCTION_PIPELINE
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
#endif
    
}

void openGlDrawRectCenterDim_NoGrad(GLBufferHandles *handles, V3 center, V2 dim, V4 color, float rot, Matrix4 offsetTransform, float zoom, Matrix4 projectionMatrix) {
#if FIXED_FUNCTION_PIPELINE
    glDisable(GL_TEXTURE_2D);
#endif
    V4 colors[4] = {color, color, color, color}; 
    openGlDrawRectCenterDim_(handles, center, dim, colors, rot, offsetTransform, 0, SHAPE_RECTANGLE, &rectangleNoGradProgram, zoom, mat4(), projectionMatrix);
#if FIXED_FUNCTION_PIPELINE
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
#endif
    
}

void openGlDrawRect(GLBufferHandles *handles, Rect2f rect, float zValue, V4 color, float rot, Matrix4 offsetTransform, float zoom, Matrix4 projectionMatrix) {
#if FIXED_FUNCTION_PIPELINE
    glDisable(GL_TEXTURE_2D);
#endif
    V4 colors[4] = {color, color, color, color}; 
    openGlDrawRectCenterDim_(handles, v2ToV3(getCenter(rect), zValue), getDim(rect), colors, rot, 
                             offsetTransform, 0, SHAPE_RECTANGLE, &rectangleProgram, zoom, mat4(), projectionMatrix);
#if FIXED_FUNCTION_PIPELINE
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
#endif
    
}

//must be an array of four. Specified clock wise.
void openGlDrawRectCenterDim_gradient(GLBufferHandles *handles, V3 center, V2 dim, V4 *colors, float rot, Matrix4 offsetTransform, float zoom, Matrix4 projectionMatrix) {
#if FIXED_FUNCTION_PIPELINE
    glDisable(GL_TEXTURE_2D);
#endif
    openGlDrawRectCenterDim_(handles, center, dim, colors, rot, offsetTransform, 0, SHAPE_RECTANGLE_GRAD, &rectangleProgram, zoom, mat4(), projectionMatrix);
#if FIXED_FUNCTION_PIPELINE
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
#endif
    
}


void openGlTextureCentreDim(GLBufferHandles *handles, GLuint textureId, V3 center, V2 dim, V4 color, float rot, Matrix4 offsetTransform, float zoom, Matrix4 viewMatrix, Matrix4 projectionMatrix) {
#if FIXED_FUNCTION_PIPELINE
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId); 
#endif
    
    V4 colors[4] = {color, color, color, color}; 
    openGlDrawRectCenterDim_(handles, center, dim, colors, rot, offsetTransform, textureId, SHAPE_TEXTURE, &textureProgram, zoom, viewMatrix, projectionMatrix);
    glBindTexture(GL_TEXTURE_2D, 0); 
#if FIXED_FUNCTION_PIPELINE
    glLoadIdentity();
    glDisable(GL_TEXTURE_2D);
#endif
}


#define openGlDrawCircle(handles, center, dim, color, offsetTransform, zoom, viewMatrix, projectionMatrix) openGlDrawCircle_(handles, center, dim, color, offsetTransform, zoom, viewMatrix, projectionMatrix, circleProgram)
#define openGlDrawLight(handles, center, dim, color, offsetTransform, zoom, viewMatrix, projectionMatrix) openGlDrawCircle_(handles, center, dim, color, offsetTransform, zoom, viewMatrix, projectionMatrix, lightProgram)
#define openGlDrawRing(handles, center, dim, color, offsetTransform, zoom, viewMatrix, projectionMatrix) openGlDrawCircle_(handles, center, dim, color, offsetTransform, zoom, viewMatrix, projectionMatrix, ringProgram)

void openGlDrawCircle_(GLBufferHandles *handles, V3 center, V2 dim, V4 color, Matrix4 offsetTransform, float zoom, Matrix4 viewMatrix, Matrix4 projectionMatrix, GlProgram program) {
    
#if FIXED_FUNCTION_PIPELINE
    glMatrixMode(GL_MODELVIEW);
    
    glDisable(GL_TEXTURE_2D);
    
    V4 centerV4 = v3ToV4Homogenous(center);
    
    V3 deltaP = V4MultMat4(centerV4, offsetTransform).xyz; 
    
    float rotationMat[] = {
        1,  0,  0,  0,
        0,  1,  0,  0,
        0,  0,  1,  0,
        deltaP.x, deltaP.y, deltaP.z,  1
    };
    
    glLoadMatrixf(rotationMat);
    
    V4 colors[8] = {
        v4(1, 0, 0, 1), 
        v4(1, 1, 0, 1), 
        v4(1, 1, 1, 1), 
        v4(1, 0, 1, 1), 
        v4(0, 0, 1, 1), 
        v4(0, 1, 1, 1), 
        v4(0, 1, 0, 1), 
        v4(0, 0, 0, 1), 
    };
    
    int colorAt = 0;
    
    glColor4f(color.x, color.y, color.z, color.w);
    
    glBegin(GL_TRIANGLES);
    
    float radAt = 0;
    int numOfTriangles = 20;
    float rad_dt = 2*PI32 / (float)numOfTriangles;
    
    V2 p1 = v2(dim.x*cos(radAt), dim.y*sin(radAt)); 
    V2 firstP = p1;
    for(int i = 0 ; i < numOfTriangles; ++i) {
        
        radAt += rad_dt;
        V2 p2 = v2(dim.x*cos(radAt), dim.y*sin(radAt));
        
        glTexCoord2f(0, 0);
        glVertex2f(0, 0);
        glTexCoord2f(0, 1);
        glVertex2f(p1.x, p1.y);
        glTexCoord2f(1, 1);
        glVertex2f(p2.x, p2.y);
        
        p1 = p2;
    }
    
    glEnd();
    
    glLoadIdentity();
#else
    V4 colors[4] = {color, color, color, color};
    openGlDrawRectCenterDim_(handles, center, dim, colors, 0, offsetTransform, 0, SHAPE_CIRCLE, &program, zoom, viewMatrix, projectionMatrix);
#endif
#if FIXED_FUNCTION_PIPELINE
    glEnable(GL_TEXTURE_2D);
#endif
}

void getLineVertices(CookedData *lastData, V2 begin_, V2 end_, V4 color, float thickness, Matrix4 offsetTransform, CookedData *data, float zAt) {
    
#if FIXED_FUNCTION_PIPELINE
    glDisable(GL_TEXTURE_2D);
    glMatrixMode(GL_MODELVIEW);
#endif
    //thickness += FEATHER_PIXELS; //feather pixles is for just one side
    
    //V2 circleDim = v2_scale(2, v2(thickness, thickness));
    //openGlDrawCircle(begin_, circleDim, color, offsetTransform);
    //openGlDrawCircle(end_, circleDim, color, offsetTransform);
    
    V2 begin = V4MultMat4(v2ToV4Homogenous(begin_), offsetTransform).xy; 
    
    V2 end = V4MultMat4(v2ToV4Homogenous(end_), offsetTransform).xy; 
    
    V2 rel = v2_minus(end, begin);
    
    float len = getLength(rel);
    rel = normalize_(rel, len);
    
    float halfThickness = 0.5f*thickness;
    len += thickness;
    
    float topY = halfThickness;
    float bottomY = -halfThickness;
    if(v2Equal_withError(rel, v2(0, 0), 0.0001)) {
        rel = v2(1, 0);
    }
    
    float a1 = rel.x;
    float a2 = rel.y;
    float b1 = -rel.y;
    float b2 = rel.x;
    Matrix4 rotationMat = {{
            a1,  a2,  0,  0,
            b1,  b2,  0,  0,
            0,  0,  1,  0,
            begin.x - (halfThickness*rel.x), begin.y - (halfThickness*rel.y), 0,  1
        }};
    
#if FIXED_FUNCTION_PIPELINE
    glLoadMatrixf(rotationMat.val);
    
    glColor4f(color.x, color.y, color.z, color.w);
    
    glBegin(GL_TRIANGLES);
    glTexCoord2f(0, 0);
    glVertex2f(0, -halfThickness);
    glTexCoord2f(0, 1);
    glVertex2f(0, halfThickness);
    glTexCoord2f(1, 1);
    glVertex2f(len, halfThickness);
    
    glTexCoord2f(0, 0);
    glVertex2f(0, -halfThickness);
    glTexCoord2f(1, 0);
    glVertex2f(len, -halfThickness);
    glTexCoord2f(1, 1);
    glVertex2f(len, halfThickness);
    glEnd();
    
    glLoadIdentity();
    
#else
    float lenRatio = len / thickness;
    assert(lenRatio >= 1.0f);
    
    float shadeValue = FEATHER_PIXELS / thickness;
    shadeValue = clamp(0, shadeValue, 0.35f); //don't go up to 0.5
    
    data->triangleData[0].percentY = shadeValue;
    data->triangleData[0].lengthRatio = lenRatio;
    data->triangleData[0].color = color;
    data->triangleData[0].texUV = v2(0, 0);
    data->triangleData[0].position = v3(0, bottomY, zAt);
    
    data->triangleData[1].percentY = shadeValue;
    data->triangleData[1].lengthRatio = lenRatio;
    data->triangleData[1].color = color;
    data->triangleData[1].texUV = v2(0, 1);
    data->triangleData[1].position = v3(0, topY, zAt);
    
    data->triangleData[2].percentY = shadeValue;
    data->triangleData[2].lengthRatio = lenRatio;
    data->triangleData[2].color = color;
    data->triangleData[2].texUV = v2(1, 0);
    data->triangleData[2].position = v3(len, bottomY, zAt);
    
    data->triangleData[3].percentY = shadeValue;
    data->triangleData[3].lengthRatio = lenRatio;
    data->triangleData[3].color = color;
    data->triangleData[3].texUV = v2(1, 1);
    data->triangleData[3].position = v3(len, topY, zAt);
    
    for(int i = 0; i < 4; i++) {
        data->triangleData[i].position = transformPositionV3(data->triangleData[i].position, rotationMat);
    }
    
    unsigned int indicesData[6] = {0, 1, 3, 0, 2, 3};
    
    memcpy(data->indicesData, indicesData, sizeof(unsigned int)*arrayCount(indicesData));
    
#endif
#if FIXED_FUNCTION_PIPELINE
    glEnable(GL_TEXTURE_2D);
#endif
    
}

void openGlDrawLine(GLBufferHandles *handles, V2 begin_, V2 end_, V4 color, float thickness, Matrix4 offsetTransform, CookedData *data, float zoom) {
#if FIXED_FUNCTION_PIPELINE
    assert(!"Not implemented");
#else
    CookedData tempData = {}; //this is for people who don't want to keep the vertices!  
    if(!data) {
        data = &tempData; 
    }
    getLineVertices(0, begin_, end_, color, thickness, offsetTransform, data, 0);
    
    if(globalImmediateModeGraphics) {
        Matrix4 pvm = mat4();
        loadVertices(handles, data->triangleData, arrayCount(data->triangleData), data->indicesData, arrayCount(data->indicesData), &rectangleProgram, SHAPE_LINE, 0, pvm, zoom, color); //TODO: HACK: this thickness thickness is not right 
    } else {
        pushRenderItem(handles, &globalRenderGroup, data->triangleData, arrayCount(data->triangleData), data->indicesData, arrayCount(data->indicesData), &rectangleProgram, SHAPE_LINE, 0, mat4(), zoom, color, -1);
    }        
    
#endif
};

void OpenGLdeleteBufferHandles(GLBufferHandles *handles) {
    glDeleteVertexArrays(1, &handles->vaoHandle);
    glCheckError();
    handles->vaoHandle = 0;
    handles->indexCount = 0;
    handles->valid = false;
    handles->refresh = false;
}

RenderItem *getRenderItem(RenderGroup *group, int index) {
    RenderItem *info = 0;
    if(index < group->items.count) {
        info = (RenderItem *)getElementFromAlloc_(&group->items, index);
    }
    return info;
}

typedef struct {
    GLuint tbo; // this is attached to the buffer
    GLuint buffer;
} BufferStorage;

BufferStorage createBufferStorage(InfiniteAlloc *array) {
    BufferStorage result = {};
    glGenBuffers(1, &result.tbo);
    glCheckError();
    // printf("TBO: %d\n", result.tbo);
    glBindBuffer(GL_TEXTURE_BUFFER, result.tbo);
    glCheckError();
    glBufferData(GL_TEXTURE_BUFFER, array->sizeOfMember*array->count, array->memory, GL_DYNAMIC_DRAW);
    glCheckError();

    glGenTextures(1, &result.buffer);
    glCheckError();
    glBindTexture(GL_TEXTURE_BUFFER, result.buffer);
    glCheckError();
    
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, result.tbo);
    glCheckError();

    return result;
}

void deleteBufferStorage(BufferStorage *store) {
    // printf("buffer id: %d\n", store->buffer);
    glDeleteTextures(1, &store->buffer);
    glCheckError();

    // printf("tbo id: %d\n", store->tbo);
    glDeleteBuffers(1, &store->tbo);
    glCheckError();
}

static int lastStorageBufferCount = 0;
static BufferStorage lastBufferStorage[256] = {};

int cmpRenderItemFunc (const void * a, const void * b) {
    RenderItem *itemA = (RenderItem *)a;
    RenderItem *itemB = (RenderItem *)b;
    int result = (int)(100*(itemA->zAt - itemB->zAt));
    if(result == 0) {
        result = (intptr_t)itemA->bufferHandles - (intptr_t)itemB->bufferHandles;    
    }
    return result;
}

void drawRenderGroup(RenderGroup *group) {

    qsort(group->items.memory, group->items.count, group->items.sizeOfMember, cmpRenderItemFunc);

    for(int i = 0; i < group->items.count; ++i) {
        RenderItem *info = (RenderItem *)getElementFromAlloc_(&group->items, i);
        GLBufferHandles *handle = info->bufferHandles;
        if(handle) {
            if(handle->refresh) {
                OpenGLdeleteBufferHandles(handle);
                assert(!handle->refresh);
                assert(!handle->valid);
            }
        }
    }

    int drawCallCount = 0;

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
                assert(!"case not handled");
            }
        }
        // for(int nextIndex = 0; nextIndex < info->triangleData.count; ++nextIndex) {
        //     Vertex *vert = (Vertex *)getElementFromAlloc_(&info->triangleData, nextIndex);
        //     vert->instanceIndex = instanceIndexAt;
        // }
        // instanceIndexAt++;

        InfiniteAlloc pvms = initInfinteAlloc(float);
        InfiniteAlloc colors = initInfinteAlloc(float);

        addElementInifinteAllocWithCount_(&pvms, info->PVM.val, 16);
        addElementInifinteAllocWithCount_(&colors, info->color.E, 4);

        int instanceCount = 1;
        bool collecting = true;
        while(collecting) {
            RenderItem *nextItem = getRenderItem(group, i + 1);
            if(nextItem && info->bufferHandles == nextItem->bufferHandles && info->textureId == nextItem->textureId && info->program == nextItem->program) {
                
                assert(info->blendFuncType == nextItem->blendFuncType);
                assert(info->depthTest == nextItem->depthTest);
                //collect data
                addElementInifinteAllocWithCount_(&pvms, nextItem->PVM.val, 16);
                addElementInifinteAllocWithCount_(&colors, nextItem->color.E, 4);

                //NOTE: this is if we are trying to send data all ourselved, but we are going to use glDrawInstances
                // //Set the instance index
                // for(int nextIndex = 0; nextIndex < nextItem->triangleData.count; ++nextIndex) {
                //     Vertex *vert = (Vertex *)getElementFromAlloc_(&nextItem->triangleData, nextIndex);
                //     vert->instanceIndex = instanceIndexAt;
                // }
                // instanceIndexAt++;

                // addElementInifinteAllocWithCount_(&info->triangleData, nextItem->triangleData.memory, nextItem->triangleData.count);
                // //Move indexes down to account for the offset
                // for(int nextIndex = 0; nextIndex < nextItem->indicesData.count; ++nextIndex) {
                //     unsigned int *indexVal = (unsigned int *)getElementFromAlloc_(&nextItem->indicesData, nextIndex);
                //     *indexVal = (*indexVal) + info->triCount;
                // }
                // //
                // addElementInifinteAllocWithCount_(&info->indicesData, nextItem->indicesData.memory, nextItem->indicesData.count);

                // info->indexCount += nextItem->indexCount;
                // info->triCount += nextItem->triCount;
                instanceCount++;
                releaseInfiniteAlloc(&nextItem->triangleData);
                releaseInfiniteAlloc(&nextItem->indicesData);
                //
                i++;
            } else {
                collecting = false;
            }
        }

        // if(info->textureId != 3) 
        {
            BufferStorage pvmStore = createBufferStorage(&pvms);
            BufferStorage colorStore = createBufferStorage(&colors);
        
            loadVertices_(info->bufferHandles, (Vertex *)info->triangleData.memory, info->triCount, (unsigned int *)info->indicesData.memory, info->indexCount, info->program, info->type, info->textureId, pvmStore.buffer, colorStore.buffer, info->zoom, info->color, DRAWCALL_INSTANCED, instanceCount);
            drawCallCount++;
            
            assert(lastStorageBufferCount < arrayCount(lastBufferStorage));
            lastBufferStorage[lastStorageBufferCount++] = pvmStore;
            assert(lastStorageBufferCount < arrayCount(lastBufferStorage));
            lastBufferStorage[lastStorageBufferCount++] = colorStore;
        }

        releaseInfiniteAlloc(&info->triangleData);
        releaseInfiniteAlloc(&info->indicesData);
        releaseInfiniteAlloc(&pvms);
        releaseInfiniteAlloc(&colors);

        

    }
     releaseInfiniteAlloc(&group->items);
    //printf("NUMBER OF DRAW CALLS: %d\n", drawCallCount);
    group->idAt = 0;
}

typedef struct {
    GLuint id;
    int width;
    int height;

    GLBufferHandles drawHandles;

} Texture;

Texture createTextureOnGPU(unsigned char *image, int w, int h, int comp) {
    Texture result = {};
    if(image) {

        result.width = w;
        result.height = h;
        
        glGenTextures(1, &result.id);
        
        glBindTexture(GL_TEXTURE_2D, result.id);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        if(comp == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        } else if(comp == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        } else {
            assert(!"Channel number not handled!");
        }
        
        glBindTexture(GL_TEXTURE_2D, 0);
    } 

    return result;
}

Texture loadImage(char *fileName) {
    int w;
    int h;
    int comp = 4;
    unsigned char* image = stbi_load(fileName, &w, &h, &comp, STBI_rgb_alpha);

    if(image) {
        if(comp == 3) {
            stbi_image_free(image);
            image = stbi_load(fileName, &w, &h, &comp, STBI_rgb);
            assert(image);
            assert(comp == 3);
        }
    } else {
        printf("%s\n", fileName);
        assert(!"no image found");
    }
    
    Texture result = createTextureOnGPU(image, w, h, comp);

    if(image) {
        stbi_image_free(image);
    }
    return result;
}