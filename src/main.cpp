#if !DEVELOPER_MODE
#define NDEBUG
#endif

#include "gameDefines.h"

#if DEVELOPER_MODE
#define RESOURCE_PATH_EXTENSION "../res/" 
#else 
// #define NDEBUG
#define RESOURCE_PATH_EXTENSION "res/"
#endif

#if !DESKTOP
#include <OpenGLES/ES3/gl.h>
#else 
#include <GL/gl3w.h>
#endif


#if !DESKTOP
#include <sdl.h>
#include "SDL_syswm.h"
#else 
#include "../SDL2/sdl.h"
#include "../SDL2/SDL_syswm.h"
#endif

#include "easy_headers.h"

#include "easy_asset_loader.h"

#include "easy_transition.h"
#include "levels.h"
#include "menu.h"

typedef enum {
    BOARD_NULL, //0
    BOARD_STATIC, //1
    BOARD_SHAPE, //2
    BOARD_EXPLOSIVE, //3
    BOARD_INVALID, //For out of bounds  //4
} BoardState;

typedef enum {
    BOARD_VAL_NULL,
    BOARD_VAL_OLD,
    BOARD_VAL_ALWAYS,
    BOARD_VAL_SHAPE0,
    BOARD_VAL_SHAPE1,
    BOARD_VAL_SHAPE2,
    BOARD_VAL_SHAPE3,
    BOARD_VAL_SHAPE4,
    BOARD_VAL_GLOW,
    BOARD_VAL_DYNAMIC, //this is for the obstacles windmill 
    BOARD_VAL_TRANSIENT, //this isn't used for anything, just to make it so we aren't using the other ones. 
} BoardValType;

typedef struct {
    int id;
    
    bool valid;
    V2 pos;
    BoardValType type; //this is used to keep the images consistent;
} FitrisBlock;

#define MAX_SHAPE_COUNT 16
typedef struct {
    FitrisBlock blocks[MAX_SHAPE_COUNT];
    int count;
    
    Timer moveTimer;
} FitrisShape;

#define EXTRA_SHAPE_TYPE(FUNC) \
FUNC(EXTRA_SHAPE_NULL) \
FUNC(EXTRA_SHAPE_WINDMILL) \
FUNC(EXTRA_SHAPE_MAGNET) \

typedef enum {
    EXTRA_SHAPE_TYPE(ENUM)
} ExtraShapeType;

static char *ExtraShapeTypeStrings[] = { EXTRA_SHAPE_TYPE(STRING) };


typedef struct OverworldDt OverworldDt;
typedef struct OverworldDt {
    float val; 
    float x; 
    float y;
    
    OverworldDt *next;
    
} OverworldDt;



typedef struct {
    ExtraShapeType type;
    
    int id; //this is to link extra shape info with the location in the board text file 
    
    V2 pos;
    V2 growDir;
    
    bool active; //this is for the lag period. 
    
    Timer timer;
    // Timer lagTimer;
    float lagPeriod;
    float movePeriod;
    
    bool justFlipped;
    
    bool isOut; //going out or in
    
    int perpSize;
    
    bool isBomb;
    
    int count;
    
    bool timeAffected; //this is if it slows down when the player as grabbed the block 
    bool tryingToBegin;
    
    int max;
} ExtraShape;


typedef struct {
    float offsetAt;
    bool active;
    Timer fadeTimer;
    // Timer colorGlowTimer;
    bool shouldUpdate;
    bool wasHot;
} CellTracker;

typedef struct {
    bool valid;
    BoardValType type;
    BoardValType prevType;
    BoardState state;
    BoardState prevState;
    
    V4 color;

    CellTracker cellTracker;
    
    Timer fadeTimer;
} BoardValue;

typedef enum {
    GREEN_LINE,
    RED_LINE,
} GlowingLineType;

typedef struct {
    GlowingLineType type;
    int yAt;
    Timer timer; 
    bool isDead;
} GlowingLine;

#define ENTITY_TYPE(FUNC) \
FUNC(ENTITY_TYPE_NULL) \
FUNC(ENTITY_TYPE_SNOWMAN) \
FUNC(ENTITY_TYPE_IGLOO) \
FUNC(ENTITY_TYPE_MUSHROOM) \
FUNC(ENTITY_TYPE_TREE) \
FUNC(ENTITY_TYPE_TREE1) \
FUNC(ENTITY_TYPE_TREE2) \
FUNC(ENTITY_TYPE_CACTUS) \
FUNC(ENTITY_TYPE_ROCK) \
FUNC(ENTITY_TYPE_SNOW_PARTICLES) \
FUNC(ENTITY_TYPE_CLOUD) \

typedef enum {
    ENTITY_TYPE(ENUM)
} EntityType;

static char *WorldEntityTypeStrings[] = { ENTITY_TYPE(STRING) };

typedef struct {
    EntityType type;

    V2 pos;
    V2 size;

    float tAt;
    bool grows;
    bool direction;

    particle_system particleSystem;
} WorldEntity;

typedef struct {
    Arena *soundArena;
    int boardWidth;
    int boardHeight;
    BoardValue *board;
    BoardValue *copyBoard;
    
    FitrisShape currentShape;
    Texture *stoneTex;
    Texture *woodTex;
    Texture *bgTex;
    Texture *metalTex;
    Texture *explosiveTex;
    Texture *boarderTex;
    Texture *heartEmptyTex;
    Texture *heartFullTex;
    Texture *starTex;
    Texture *treeTex;
    Texture *treeTex1;
    Texture *treeTex2;
    Texture *waterTex;
    Texture *waterTexBig;
    Texture *mushroomTex;
    Texture *cactusTex;
    Texture *rockTex;
    Texture *alienTileTex;
    Texture *mapTex;
    Texture *refreshTex;
    Texture *muteTex;
    Texture *speakerTex;
    Texture *loadTex;
    Texture *errorTex;
    Texture *iglooTex;
    Texture *snowflakeTex;
    Texture *lineTemplateTex;
    Texture *snowManTex;
    
    Texture *alienTex[5];
    Texture *tilesTex[13];
    Texture *tilesTexSand[13];
    Texture *tilesTexSnow[13];
    
    WavFile *solidfyShapeSound;
    WavFile *moveSound;
    WavFile *arrangeSound;
    WavFile *backgroundSound;
    WavFile *backgroundSound2;
    WavFile *successSound;
    WavFile *explosiveSound;
    WavFile *enterLevelSound;
    WavFile *showLevelsSound;
    
    TransitionState transitionState;
    
    AppKeyStates overworldKeyStates;
    
    Timer levelNameTimer;
    
    int startOffsets[3];
    int lifePoints;
    int lifePointsMax;
    bool wasHitByExplosive; 
    int shapeSizes[3];
    int shapesCount;
    
    bool isMirrorLevel;
    
    int extraShapeCount;
    ExtraShape extraShapes[32];
    
    bool createShape;
    
    bool wasHoldingShape;
    
    LevelType currentLevelType;
    LevelData levelsData[LEVEL_COUNT];
    LevelData *levelGroups[LEVEL_COUNT]; //this is for the overworld. 
    // FileContents overworldLayout;
    
    int maxGroupId;
    LevelGroup groups[LEVEL_COUNT];

    Timer moveTimer;
    
    float moveTime;
    
    int currentHotIndex;
    
    MenuInfo menuInfo;
    
    int experiencePoints;
    int maxExperiencePoints;
    
    Array_Dynamic particleSystems;
    
    particle_system overworldParticleSys;
    particle_system cloudParticleSys;
    
    int *overworldValues;
    V2 overworldDim;
    
    float accumHoldTime;
    bool letGo;  //let the shape go
    
    GlowingLine glowingLines[16]; //these are the win lines. We annotate them with a slash in the level markup file
    int glowingLinesCount;
    
    TileLayouts tileLayout;
    
    bool isFreestyle;
    
    V2 lastMouseP;

    int entityCount;
    WorldEntity worldEntities[128];

    WorldEntity *hotEntity;

//For the 'back to origin button'
    Timer backToOriginTimer;
    V3 backToOriginStart;
    V3 overworldGroupPosAt;

    //

    int currentGroupId;

    bool confirmCloseScreen;
    
    bool backgroundSoundPlaying;
#if EDITOR_MODE
    LevelType grabbedLevel;
#endif

    Texture *blueBackgroundTex;

    LevelType lastBoundsStar;
    bool isHoveringButton;

    bool updateRows; //update the rows like tetris. Somem levels can't have this because the size of the shape is as big as the board. 

    LevelType lastLevelType;
    ////////TODO: This stuff below should be in another struct so isn't there for all projects. 
    Arena *longTermArena;
    float dt;
    SDL_Window *windowHandle;
    AppKeyStates *keyStates;
    AppKeyStates *playStateKeyStates;
    FrameBuffer mainFrameBuffer;
    Matrix4 metresToPixels;
    Matrix4 pixelsToMeters;
    
    V2 *screenDim;
    V2 *resolution; 
    
    GLuint backbufferId;
    GLuint renderbufferId;
    
    Font *font;
    Font *numberFont;
    
    V3 cameraPos;
    V3 overworldCamera;
    V3 camVel;

    float monitorRefreshRate;

    bool blackBars;
    
    unsigned int lastTime;
    ///////
    V2 resInMeters;
    
} FrameParams;

typedef enum {
    MOVE_LEFT, 
    MOVE_RIGHT,
    MOVE_DOWN
} MoveType;

void saveOverworldPositions(FrameParams *params) {
    InfiniteAlloc data = initInfinteAlloc(char);
    for(int lvlIndex = 0; lvlIndex < arrayCount(params->levelsData); lvlIndex++) {
        LevelData *levelData = params->levelsData + lvlIndex;
        if(levelData->valid) {
            assert(levelData->state != LEVEL_STATE_NULL);
            assert(levelData->name);
            
            char *lvlTypeStr = LevelTypeStrings[levelData->levelType];
            
            
            char buffer[1028] = {};
            sprintf(buffer, "{\nlevelType: \"%s\";\n", lvlTypeStr);
            addElementInifinteAllocWithCount_(&data, buffer, strlen(buffer));
            
            sprintf(buffer, "pos: %f %f;\n}\n", levelData->pos.x, levelData->pos.y);
            addElementInifinteAllocWithCount_(&data, buffer, strlen(buffer));
        }
    }
    for(int entIndex = 0; entIndex < params->entityCount; entIndex++) {
        WorldEntity *ent = params->worldEntities + entIndex;
        
        char *entityTypeStr = WorldEntityTypeStrings[ent->type];
        
        char buffer[1028] = {};
        sprintf(buffer, "{\nentityType: \"%s\";\n", entityTypeStr);
        addElementInifinteAllocWithCount_(&data, buffer, strlen(buffer));
        
        sprintf(buffer, "pos: %f %f;\n}\n", ent->pos.x, ent->pos.y);
        addElementInifinteAllocWithCount_(&data, buffer, strlen(buffer));
    }
    char *writeName = concat(globalExeBasePath, "positions.txt");
    
    game_file_handle handle = platformBeginFileWrite(writeName);
    platformWriteFile(&handle, data.memory, data.count*data.sizeOfMember, 0);
    platformEndFile(handle);
    
    releaseInfiniteAlloc(&data);
    free(writeName);

}

void updateAndRenderWorldEntity(WorldEntity *ent, FrameParams *params, float dt, V2 resolution, V3 overworldCam) {
    if(ent->type == ENTITY_TYPE_SNOW_PARTICLES || ent->type == ENTITY_TYPE_CLOUD) {
        #if EDITOR_MODE
            RenderInfo extraRenderInfo = calculateRenderInfo(v3(ent->pos.x, ent->pos.y, -1), v3(1, 1, 1), overworldCam, params->metresToPixels);

            Rect2f outputDim = rect2fCenterDimV2(extraRenderInfo.transformPos.xy, extraRenderInfo.transformDim.xy);
            if(inBounds(params->keyStates->mouseP_yUp, outputDim, BOUNDS_RECT) && wasPressed(params->keyStates->gameButtons, BUTTON_LEFT_MOUSE) && params->grabbedLevel == LEVEL_NULL) {
                params->hotEntity = ent;
            }
            if(params->hotEntity == ent) {
                V2 worldP = params->keyStates->mouseP_yUp;
                worldP = V4MultMat4(v4(worldP.x, worldP.y, 1, 1), params->pixelsToMeters).xy;
                worldP = v2_plus(worldP, params->overworldCamera.xy);
                ent->pos = worldP;
                saveOverworldPositions(params);
            }

        #endif

        drawAndUpdateParticleSystem(&ent->particleSystem, params->dt, v3(ent->pos.x, ent->pos.y, -0.3), v3(0, 0 ,0), COLOR_WHITE, overworldCam, params->metresToPixels, resolution, true);
    } else {
        Texture *tex = 0;
        ent->size = v2(1, 1);
        switch(ent->type) {
            case ENTITY_TYPE_SNOWMAN: {
                tex = params->snowManTex;
                ent->grows = true;
                ent->direction = false;
            } break;
            case ENTITY_TYPE_IGLOO: {
                tex = params->iglooTex;
                ent->grows = false;
                ent->direction = false;
            } break;
            case ENTITY_TYPE_MUSHROOM: {
                tex = params->mushroomTex;
                ent->grows = false;
                ent->direction = false;
            } break;
            case ENTITY_TYPE_TREE: {
                tex = params->treeTex;
                ent->grows = true;
                ent->direction = true;
            } break;
            case ENTITY_TYPE_TREE1: {
                tex = params->treeTex1;
                ent->grows = true;
                ent->direction = true;
            } break;
            case ENTITY_TYPE_TREE2: {
                tex = params->treeTex2;
                ent->grows = true;
                ent->direction = true;
            } break;
            case ENTITY_TYPE_CACTUS:{
                tex = params->cactusTex;
                ent->grows = true;
                ent->direction = false;
            } break;
            case ENTITY_TYPE_ROCK:  {
                tex = params->rockTex;
                ent->grows = false;
                ent->direction = false;
            } break;
            default: {
                assert(false);
            }
        }
        assert(tex);

        float width =  ent->size.x;
        float height =  ent->size.y;
        float extraVal = 0;
        V2 placement = ent->pos;
        if(ent->grows) {
            ent->tAt += dt;
            extraVal = 0.1f*sin(ent->tAt);
            if(ent->direction) {
                height += extraVal;
                placement.y += 0.5f*extraVal;
            } else {
                width += extraVal;
            }
        }

        
        
        RenderInfo extraRenderInfo = calculateRenderInfo(v3(placement.x, placement.y, -2), v3(width, height, 1), overworldCam, params->metresToPixels);

        #if EDITOR_MODE
            Rect2f outputDim = rect2fCenterDimV2(extraRenderInfo.transformPos.xy, extraRenderInfo.transformDim.xy);
            if(inBounds(params->keyStates->mouseP_yUp, outputDim, BOUNDS_RECT) && wasPressed(params->keyStates->gameButtons, BUTTON_LEFT_MOUSE) && params->grabbedLevel == LEVEL_NULL) {
                params->hotEntity = ent;
            }
            if(params->hotEntity == ent) {
                V2 worldP = params->keyStates->mouseP_yUp;
                worldP = V4MultMat4(v4(worldP.x, worldP.y, 1, 1), params->pixelsToMeters).xy;
                worldP = v2_plus(worldP, params->overworldCamera.xy);
                ent->pos = worldP;
                saveOverworldPositions(params);
            }

        #endif
        renderTextureCentreDim(tex, extraRenderInfo.pos, extraRenderInfo.dim.xy, COLOR_WHITE, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), extraRenderInfo.pvm));     
    }
}

static inline float getRandNum01_include() {
    float result = ((float)rand() / (float)RAND_MAX);
    return result;
}

static inline float getRandNum01() {
    float result = ((float)rand() / (float)(RAND_MAX - 1));
    return result;
}


BoardState getBoardState(FrameParams *params, V2 pos) {
    BoardState result = BOARD_INVALID;
    if(pos.x >= 0 && pos.x < params->boardWidth && pos.y >= 0 && pos.y < params->boardHeight) {
        BoardValue *val = &params->board[params->boardWidth*(int)pos.y + (int)pos.x];
        result = val->state;
        assert(result != BOARD_INVALID);
    }
    
    return result;
}

BoardState getBoardStateWithConection(FrameParams *params, V2 pos) {
    BoardState result = BOARD_INVALID;
    if(pos.x >= 0 && pos.x < params->boardWidth && pos.y >= 0 && pos.y < params->boardHeight) {
        BoardValue *val = &params->copyBoard[params->boardWidth*(int)pos.y + (int)pos.x];
        result = val->state;
        assert(result != BOARD_INVALID);
    }
    
    return result;
}

BoardValue *getBoardValue(FrameParams *params, V2 pos) {
    BoardValue *result = 0;
    if(pos.x >= 0 && pos.x < params->boardWidth && pos.y >= 0 && pos.y < params->boardHeight) {
        BoardValue *val = &params->board[params->boardWidth*(int)pos.y + (int)pos.x];
        result = val;
    }
    
    return result;
}

BoardValue *getCopyBoardValue(FrameParams *params, V2 pos) {
    BoardValue *result = 0;
    if(pos.x >= 0 && pos.x < params->boardWidth && pos.y >= 0 && pos.y < params->boardHeight) {
        BoardValue *val = &params->copyBoard[params->boardWidth*(int)pos.y + (int)pos.x];
        result = val;
    }
    
    return result;
}

typedef enum {
    DIRECTION_RIGHT, 
    DIRECTION_DOWN, 
    DIRECTION_LEFT, 
    DIRECTION_UP, 
} Direction;

static inline void drawOutline(FrameParams *params, V2 drawAt, Direction direction, V4 color, float offset, V2 resolution, float zAt) {

    V2 posStart = v2(0, 0);
    V2 posStartAbs = v2(0, 0);
    V2 posEnd = v2(0, 0);
    bool init = true;
    bool end = false;
    float lineDim = 0.05f;
    float lineLength = 0.25f;
    bool blank = false;
    bool plus = true;
    while(!end) {
        V2 lineWidth = v2(0, 0);
        switch(direction) {
            case DIRECTION_RIGHT: {
                if(init) {
                    posStartAbs = posStart = v2_plus(drawAt, v2(-0.5f, 0.5f));
                    posStart.x += offset - lineLength;
                } else {
                    posStart = posEnd;
                }
                plus = true;
                posEnd = v2_plus(posStart, v2(lineLength, 0));

                if(posEnd.x - posStartAbs.x >= 1.0f) {
                    posEnd.x = posStartAbs.x + 1.0f;
                    end = true;
                }
                lineWidth.y = lineDim;

            } break;
            case DIRECTION_DOWN: {
                if(init) {
                    posStartAbs = posStart = v2_plus(drawAt, v2(0.5f, 0.5f));
                    posStart.y -= offset - lineLength;
                } else {
                    posStart = posEnd;
                }
                plus = false;
                posEnd = v2_minus(posStart, v2(0, lineLength));

                if(posStartAbs.y - posEnd.y >= 1.0f) {
                    posEnd.y = posStartAbs.y - 1.0f;
                    end = true;
                }
    
                lineWidth.x = lineDim;
            } break;
            case DIRECTION_LEFT: {
                if(init) {
                    posStartAbs = posStart = v2_plus(drawAt, v2(0.5f, -0.5f));
                    posStart.x -= offset - lineLength;
                    
                } else {
                    posStart = posEnd;
                }
                plus = false;
                posEnd = v2_minus(posStart, v2(lineLength, 0));

                if(posStartAbs.x - posEnd.x >= 1.0f) {
                    posEnd.x = posStartAbs.x - 1.0f;
                    end = true;
                }
                lineWidth.y = lineDim;
            } break;
            case DIRECTION_UP: {
                if(init) {
                    posStartAbs = posStart = v2_plus(drawAt, v2(-0.5f, -0.5f));
                    posStart.y += offset - lineLength;
                } else {
                    posStart = posEnd;
                }
                plus = true;
                posEnd = v2_plus(posStart, v2(0, lineLength));

                if(posEnd.y - posStartAbs.y >= 1.0f) {
                    posEnd.y = posStartAbs.y + 1.0f;
                    end = true;
                }
                lineWidth.x = lineDim;
                
            } break;
        }

        if(!blank) {
            V2 drawposStart = posStart;
            if(init) {
                if(plus) {
                    if(posStart.x < posStartAbs.x) {
                        drawposStart.x = posStartAbs.x;
                    }
                    if(posStart.y < posStartAbs.y) {
                        drawposStart.y = posStartAbs.y;
                    }
                } else {
                    if(posStart.x > posStartAbs.x) {
                        drawposStart.x = posStartAbs.x;
                    }
                    if(posStart.y > posStartAbs.y) {
                        drawposStart.y = posStartAbs.y;
                    }
                }

                init = false;
            }
            Rect2f rect = rect2f(drawposStart.x - lineWidth.x, drawposStart.y - lineWidth.y, posEnd.x + lineWidth.x, posEnd.y + lineWidth.y);  
            RenderInfo renderInfo = calculateRenderInfo(v2ToV3(getCenter(rect), zAt), v2ToV3(getDim(rect), 1), params->cameraPos, params->metresToPixels);
            renderDrawRectCenterDim(renderInfo.pos, renderInfo.dim.xy, color, 0, mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
            // renderTextureCentreDim(params->lineTemplateTex, renderInfo.pos, v2_scale(1, renderInfo.dim.xy), color, 0, mat4(), renderInfo.pvm, OrthoMatrixToScreen(resolution.x, resolution.y)); 
        }
        blank = !blank;
    }
}

static inline void drawCellTracker(FrameParams *params, CellTracker *tracker, V2 boardPos, V4 color, V2 resolution, float zPos) {
    // if(tracker->shouldUpdate) 
    {
        tracker->offsetAt += params->dt*0.4f;
        if(tracker->offsetAt > 0.5f) {
            tracker->offsetAt = 0;
        }
    }

    drawOutline(params, boardPos, DIRECTION_RIGHT, color, tracker->offsetAt, resolution, zPos);
    drawOutline(params, boardPos, DIRECTION_DOWN, color, tracker->offsetAt, resolution, zPos);
    drawOutline(params, boardPos, DIRECTION_LEFT, color, tracker->offsetAt, resolution, zPos);
    drawOutline(params, boardPos, DIRECTION_UP, color, tracker->offsetAt, resolution, zPos);
}

bool inBoardBounds(FrameParams *params, V2 pos) {
    bool result = false;
    if(pos.x >= 0 && pos.x < params->boardWidth && pos.y >= 0 && pos.y < params->boardHeight) {
        result = true;
    }
    return result;
}

void setBoardState(FrameParams *params, V2 pos, BoardState state, BoardValType type) {
    if(pos.x >= 0 && pos.x < params->boardWidth && pos.y >= 0 && pos.y < params->boardHeight) {
        BoardValue *val = &params->board[params->boardWidth*(int)pos.y + (int)pos.x];
        val->prevState = val->state;
        val->prevType = val->type;
        val->state = state;
        val->type = type;
        val->fadeTimer = initTimer(FADE_TIMER_INTERVAL, false);
    } else {
        assert(!"invalid code path");
    }
}

static inline ExtraShape *initExtraShape(ExtraShape *shape) {
    memset(shape, 0, sizeof(ExtraShape));
    shape->isOut = true;
    shape->count = 0;
    shape->active = true;
    shape->perpSize = 0;
    return shape;
}

ExtraShape *addExtraShape(FrameParams *params) {
    assert(params->extraShapeCount < arrayCount(params->extraShapes));
    ExtraShape *shape = params->extraShapes + params->extraShapeCount++;
    initExtraShape(shape);
    return shape;
}


void allocateBoard(FrameParams *params, int boardWidth, int boardHeight) {
    params->boardWidth = boardWidth;
    params->boardHeight = boardHeight;
    
    if(params->board) { free(params->board); }
    if(params->copyBoard) { free(params->copyBoard); }
    params->board = (BoardValue *)calloc(params->boardWidth*params->boardHeight*sizeof(BoardValue), 1);
    params->copyBoard = (BoardValue *)calloc(params->boardWidth*params->boardHeight*sizeof(BoardValue), 1);
    
    for(int boardY = 0; boardY < params->boardHeight; ++boardY) {
        for(int boardX = 0; boardX < params->boardWidth; ++boardX) {
            BoardValue *boardVal = &params->board[boardY*params->boardWidth + boardX];
            
            boardVal->type = BOARD_VAL_NULL;
            boardVal->state = BOARD_NULL;
            boardVal->prevState = BOARD_NULL;
            
            turnTimerOff(&boardVal->fadeTimer);
            turnTimerOff(&boardVal->cellTracker.fadeTimer);
            // turnTimerOff(&boardVal->cellTracker.colorGlowTimer);
            boardVal->cellTracker.shouldUpdate = false;
            boardVal->cellTracker.offsetAt = 0;
            boardVal->cellTracker.active = false;
            boardVal->cellTracker.wasHot = false;

            boardVal->color = COLOR_WHITE;
        }
    }
}

V2 parseGetBoardDim(char *at) {
    V2 result = {};
    at = lexEatWhiteSpace(at);
    bool parsing = true;
    int xAt = 0;
    int yAt = 0;
    while(parsing) {
        at = lexEatWhiteSpaceExceptNewLine(at);
        
        switch(*at) {
            case '\0': {
                parsing = false;
            } break;
            case '\r': 
            case '\n': {
                if(xAt > result.x) {
                    result.x = xAt;
                }
                xAt = 0;
                yAt++;
                at = lexEatWhiteSpace(at);
            } break;
            case '}': {
                parsing = false;
            } break;
            default: {
                //Without this ist seems to only cause a bug on the last line of the baord
                if(*at != '/' && *at != ')') {
                    xAt++;
                }
                at++;
            }
        }
    }   
    result.y = yAt;
    return result;
}

static inline void parseOverworldBoard(char *at, int *values, V2 dim) {
    at = lexEatWhiteSpace(at);
    bool parsing = true;
    int xAt = 0;
    int yAt = 0;
    
    while(parsing) {
        at = lexEatWhiteSpaceExceptNewLine(at);
        
        switch(*at) {
            case '\0': {
                parsing = false;
            } break;
            case '\r': 
            case '\n': {
                xAt = 0;
                yAt++;
                at = lexEatWhiteSpace(at);
            } break;
            default: {
                assert(xAt < dim.x);
                assert(yAt < dim.y);
                if(*at != '-') {
                    values[yAt*(int)dim.x + xAt] = 1;
                } else {
                    values[yAt*(int)dim.x + xAt] = 0;
                }
                
                xAt++;
                
                at++;
            }
        }
    }   
}

//init level
static inline int loadLevelData(FrameParams *params) {
    //INITING ALL THE LEVELS
    #if 0 //load overworld from file
    char *b_ = concat("levels/", "level_overworld.txt");
    char *c_ = concat(globalExeBasePath, b_);
    FileContents overworldContents = getFileContentsNullTerminate(c_);
    assert(overworldContents.valid);
    params->overworldLayout = overworldContents;
    free(b_);
    free(c_);

    #endif
    
    int totalLevelCount = 0;
    
    for(int i = 1; i < LEVEL_COUNT; ++i) {
        assert(i < arrayCount(LevelTypeStrings));
        char *a = concat(LevelTypeStrings[i], ".txt");
        char *b = concat("levels/", a);
        char *c = concat(globalExeBasePath, b);
        bool isFileValid = platformDoesFileExist(c);
        assert(i < arrayCount(params->levelsData));
        params->levelsData[i].valid = isFileValid;
        
        if(isFileValid) {
            
            FileContents contents = getFileContentsNullTerminate(c);
            
            totalLevelCount++;
            LevelData *levelData = params->levelsData + i;
            levelData->contents = contents;
            levelData->name = "Name Not Set!";
            
            levelData->state = LEVEL_STATE_LOCKED;
            turnTimerOff(&levelData->showTimer);
            levelData->glyphCount = 0;
            levelData->levelType = (LevelType)i;
            levelData->pos = v2(4*getRandNum01_include(), 4*getRandNum01_include()); 
            particle_system_settings particleSet = InitParticlesSettings(PARTICLE_SYS_DEFAULT);
            pushParticleBitmap(&particleSet, findTextureAsset("starGold.png"), "star");
                
            levelData->displayNameTimer = initTimer(0.4f, false);
            turnTimerOff(&levelData->displayNameTimer); //turn off
            particleSet.Loop = false;
            //particleSet.offsetP = v3(0.000000, 0.000000, 0.200000);
            particleSet.bitmapScale = 0.3f;
            particleSet.posBias = rect2f(0, 0, 0, 0);
            particleSet.VelBias = rect2f(-1, -1, 1.000000, 1);
            particleSet.angleBias = v2(0.000000, 6.280000);
            particleSet.angleForce = v2(-3.000000, 3.000000);
            particleSet.collidesWithFloor = false;
            particleSet.MaxLifeSpan = 2.0f;
            // particleSet.pressureAffected = false;
            
            InitParticleSystem(&levelData->particleSystem, &particleSet);
            // params->overworldParticleSys.MaxParticleCount = 4;
            levelData->particleSystem.viewType = ORTHO_MATRIX;
            setParticleLifeSpan(&levelData->particleSystem, 1.0f);
            // Reactivate(&levelData->particleSystem);
            // assert(levelData->particleSystem.Active);
            
            int groupId = 1; //NOTE: if no group id is set, it will default to first group. 
            
            EasyTokenizer tokenizer = lexBeginParsing((char *)contents.memory, true);
            bool parsing = true;
            
            bool isLevelData = false;
            while(parsing) {
                EasyToken token = lexGetNextToken(&tokenizer);
                InfiniteAlloc data = {};
                switch(token.type) {
                    case TOKEN_NULL_TERMINATOR: {
                        parsing = false;
                    } break;
                    case TOKEN_WORD: {
                        if(stringsMatchNullN("LevelData", token.at, token.size)) {
                            isLevelData = true;
                        }
                        if(isLevelData) {
                            if(stringsMatchNullN("name", token.at, token.size)) {
                                char *stringToCopy = getStringFromDataObjects(&data, &tokenizer);
                                // printf("%s\n", stringToCopy);
                                levelData->name = nullTerminate(stringToCopy, strlen(stringToCopy));
                                // printf("%s\n", levelData->name);
                            }
                            if(stringsMatchNullN("groupId", token.at, token.size)) {
                                groupId = getIntFromDataObjects(&data, &tokenizer);
                            }
                        }
                    } break;
                    case TOKEN_CLOSE_BRACKET: {
                        //assume we get all the information in the first block of data.
                        assert(isLevelData);
                        parsing = false;
                    } break;
                    default: {
                        
                    }
                }
                releaseInfiniteAlloc(&data);
            }
            
            if(groupId == 0) {
                levelData->state = LEVEL_STATE_UNLOCKED;
            }
            levelData->groupId = groupId;

            ///Add to group structure
            if(params->maxGroupId < levelData->groupId) {
                params->maxGroupId = levelData->groupId;
            }
            LevelGroup *group = params->groups + levelData->groupId;
            addLevelToGroup(group, levelData->levelType);

            
            ///
            
            
            
#if CHEAT_MODE
            //unlock everything
            levelData->state = LEVEL_STATE_UNLOCKED;
#endif
            //NOTE: add the group to the linked list. 
            //NOTE: We add them in order, so first on will stay at the start of the list. 
            LevelData **groupListAtPtr = params->levelGroups + groupId;
            while(*groupListAtPtr) {
                LevelData *groupListAt = *groupListAtPtr;
                
                groupListAtPtr = &groupListAt->next;
            }
            
            assert(levelData->next == 0);
            
            *groupListAtPtr = levelData;
            
            assert(contents.memory);
            assert(contents.valid);
        }
        
        free(a);
        free(b);
        free(c);
    }
    
    for(int i = 1; i < LEVEL_COUNT; ++i) {
        LevelData *levelData = params->levelsData + i;
        if(levelData->valid) {
            int levelIndexAt = i;
            int groupNum = 0;
            for(int groupIndexAt = 0; groupIndexAt <= levelData->groupId; ++groupIndexAt) {
                LevelData **groupAt = params->levelGroups + groupIndexAt;
                if(*groupAt && *groupAt != levelData) {
                    assert(*groupAt != levelData);
                    while(*groupAt && *groupAt != levelData) {
                        assert(*groupAt != levelData);
                        LevelData *groupAtDef = *groupAt;
                        groupNum++;    
                        groupAt = &groupAtDef->next;
                    }
                } else {
                    break;
                }
            }
            
            levelIndexAt = groupNum + 1;
            #if EDITOR_MODE
            // levelIndexAt = levelData->groupId;
            #endif
            if(levelIndexAt < 10) {
                assert(levelData->glyphCount < arrayCount(levelData->glyphs));
                levelData->glyphs[levelData->glyphCount++] = easyFont_getGlyph(params->numberFont, (u32)(levelIndexAt + 48));
            } else if(levelIndexAt < 100) {
                int firstUnicode = (levelIndexAt / 10) + 48;
                int secondUnicode = (levelIndexAt % 10) + 48;
                assert(levelData->glyphCount < arrayCount(levelData->glyphs));
                levelData->glyphs[levelData->glyphCount++] = easyFont_getGlyph(params->numberFont, (u32)firstUnicode);
                assert(levelData->glyphCount < arrayCount(levelData->glyphs));
                levelData->glyphs[levelData->glyphCount++] = easyFont_getGlyph(params->numberFont, (u32)secondUnicode);
            } else {
                assert(!"invalid case");
            }
        }
    }
    return totalLevelCount;
    
}

void loadLevelNames_DEPRECATED(FrameParams *params) {
    char *c = concat(globalExeBasePath, "levels/level_names.txt");
    FileContents contents = getFileContentsNullTerminate(c);
    
    assert(contents.memory);
    assert(contents.valid);
    
    free(c);
    
    char **namePtr = 0;
    EasyTokenizer tokenizer = lexBeginParsing((char *)contents.memory, true);
    bool parsing = true;
    
    for(int i = 1; i < LEVEL_COUNT; ++i) {
        assert(i < arrayCount(params->levelsData));
        params->levelsData[i].name = "Name Not Set!";
    }
    
    while(parsing) {
        EasyToken token = lexGetNextToken(&tokenizer);
        InfiniteAlloc data = {};
        switch(token.type) {
            case TOKEN_NULL_TERMINATOR: {
                parsing = false;
            } break;
            case TOKEN_INTEGER: {
                char charBuffer[256] = {};
                assert(arrayCount(charBuffer) > token.size);
                nullTerminateBuffer(charBuffer, token.at, token.size);
                int indexAt = atoi(charBuffer);
                assert(indexAt + 1 < arrayCount(params->levelsData));
                namePtr = &params->levelsData[indexAt + 1].name;
            } break;
            case TOKEN_STRING: {
                assert(namePtr);
                *namePtr = nullTerminate(token.at, token.size);
            } break;
            default: {
                
            }
        }
    }
}

static inline void setBackgroundImage(FrameParams *params, int groupId) {
    switch(groupId) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5: {
            params->bgTex = findTextureAsset("blue_shroom.png");
        } break;
        case 6:
        case 7:
        case 8:
        case 9: {
            params->bgTex = findTextureAsset("yellow_desert.png");
        } break;
        case 10: 
        case 11:
        case 12:
        case 13: 
        case 14: {
            params->bgTex = findTextureAsset("blue_land.png");
        } break;
        default: {
            assert(false);
        }
    }
}


typedef enum {
    NULL_PROPERTIES,
    EXTRA_SHAPE_PROPERTIES,
    SQUARE_PROPERTIES,
} LevelDataProperty;

//finds a shape from an id. Used for level loading 
ExtraShape *findExtraShape(ExtraShape *shapes, int count, int id) {
    ExtraShape *result = 0;
    for(int i = 0; i < count; ++i) {
        
        ExtraShape *shape = shapes + i;
        if(shape->id == id) {
            result = shape;
            break;
        }
    }
    assert(result);
    return result;
}

typedef struct {
    char id;
    int shapeId;
} SquareProperty;

static inline void addGlowingLine(FrameParams *params, int yAt, GlowingLineType type) {
    assert(params->glowingLinesCount < arrayCount(params->glowingLines));
    GlowingLine *line = params->glowingLines + params->glowingLinesCount++;
    line->yAt = yAt;
    line->type = type;
    line->isDead = false;
}

void createLevelFromFile(FrameParams *params, LevelType levelTypeIn) {
    
    assert(levelTypeIn < arrayCount(params->levelsData));
    FileContents contents = params->levelsData[levelTypeIn].contents;
    
    assert(contents.memory);
    assert(contents.valid);
    
    EasyTokenizer tokenizer = lexBeginParsing((char *)contents.memory, true);
    
    LevelDataProperty currentType = NULL_PROPERTIES;
    
    //NOTE: temp info which allows us to be able to reuse shape definitions in the levels files. 
    int extraShapeCount = 0;
    ExtraShape extraShapes[64] = {};
    ExtraShape *shape = 0;
    
    int propertyCount = 0;
    SquareProperty squareProperties[64];
    SquareProperty *currentShapeProperty = 0;
    
    bool isLevelData = false;
    
    int boardWidth = 0;
    int boardHeight = 0;
    LevelType levelType = LEVEL_NULL;
    bool hasBgImage = false;
    bool hasLifePoints = false;
    bool parsing = true;
    bool hasLevelName = true;
    params->shapeSizes[0] = 4; //default to size 4
    params->shapesCount = 1;
    params->startOffsets[0] = 0; //default to zero
    params->isMirrorLevel = false;
    params->updateRows = true;
    bool setLagPeriod = false;
    while(parsing) {
        EasyToken token = lexGetNextToken(&tokenizer);
        InfiniteAlloc data = {};
        switch(token.type) {
            case TOKEN_NULL_TERMINATOR: {
                parsing = false;
            } break;
            case TOKEN_WORD: {
                if(stringsMatchNullN("Square", token.at, token.size)) {
                    currentType = SQUARE_PROPERTIES;
                    isLevelData = false;
                    
                    assert(propertyCount < arrayCount(squareProperties));
                    currentShapeProperty = squareProperties + propertyCount++;
                    currentShapeProperty->shapeId = -1; //init to negative 1 since that means there is not shape
                }
                if(stringsMatchNullN("Windmill", token.at, token.size)) {
                    currentType = EXTRA_SHAPE_PROPERTIES;
                    assert(extraShapeCount < arrayCount(extraShapes));
                    shape = initExtraShape(&extraShapes[extraShapeCount++]);
                    isLevelData = false;
                }
                
                if(stringsMatchNullN("LevelData", token.at, token.size)) {
                    isLevelData = true;
                    currentType = NULL_PROPERTIES;
                }
                
                if(isLevelData) {
                    if(stringsMatchNullN("backgroundImage", token.at, token.size)) {
                        char *name = getStringFromDataObjects(&data, &tokenizer);
                        params->bgTex = findTextureAsset(name);
                        hasBgImage = true;
                    }
                    if(stringsMatchNullN("lifePoints", token.at, token.size)) {
                        params->lifePointsMax = getIntFromDataObjects(&data, &tokenizer);
                        hasLifePoints = true;
                    }
                    if(stringsMatchNullN("shapeSize", token.at, token.size)) {
                        params->shapeSizes[0] = getIntFromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("shapeSize2", token.at, token.size)) {
                        params->shapeSizes[1] = getIntFromDataObjects(&data, &tokenizer);
                        params->shapesCount = max(2, params->shapesCount);
                    }
                    if(stringsMatchNullN("updateFullRow", token.at, token.size)) {
                        params->updateRows = getBoolFromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("isMirror", token.at, token.size)) {
                        params->isMirrorLevel = true;
                        assert(params->shapeSizes[1]);
                    }
                    if(stringsMatchNullN("shapeSize3", token.at, token.size)) {
                        params->shapeSizes[2] = getIntFromDataObjects(&data, &tokenizer);
                        params->shapesCount = max(3, params->shapesCount);
                    }
                    if(stringsMatchNullN("startOffset", token.at, token.size)) {
                        params->startOffsets[0] = getIntFromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("startOffset2", token.at, token.size)) {
                        params->startOffsets[1] = getIntFromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("startOffset3", token.at, token.size)) {
                        params->startOffsets[2] = getIntFromDataObjects(&data, &tokenizer);
                    }
                    assert(currentType == NULL_PROPERTIES);
                    
                }
                if(stringsMatchNullN("Board", token.at, token.size)) {
                    parsing = false;
                    EasyToken nxtToken = lexGetNextToken(&tokenizer);
                    assert(nxtToken.type == TOKEN_COLON);
                    nxtToken = lexGetNextToken(&tokenizer);
                    assert(nxtToken.type == TOKEN_OPEN_BRACKET);
                    currentType = NULL_PROPERTIES;
                }
                
                if(currentType == SQUARE_PROPERTIES) {
                    assert(currentShapeProperty);
                    if(stringsMatchNullN("id", token.at, token.size)) {
                        char *idAsStr = getStringFromDataObjects(&data, &tokenizer);
                        // printf("id as string: %s\n", idAsStr);
                        assert(strlen(idAsStr) == 1);
                        currentShapeProperty->id = idAsStr[0]; //has to be length one
                        
                    }
                    if(stringsMatchNullN("shapeId", token.at, token.size)) {
                        currentShapeProperty->shapeId = getIntFromDataObjects(&data, &tokenizer);
                    }
                }
                
                if(currentType == EXTRA_SHAPE_PROPERTIES) {
                    assert(shape);
                    if(stringsMatchNullN("id", token.at, token.size)) {
                        shape->id = getIntFromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("isBomb", token.at, token.size)) {
                        shape->isBomb = getBoolFromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("growDir", token.at, token.size)) {
                        shape->growDir = buildV2FromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("growTimerPeriod", token.at, token.size)) {
                        float period = getFloatFromDataObjects(&data, &tokenizer);
                        shape->timer = initTimer(period, true);
                        shape->movePeriod = period;
                    }
                    if(stringsMatchNullN("lagTimerPeriod", token.at, token.size)) {
                        assert(false);//doens't exist anymore
                    }
                    if(stringsMatchNullN("timeAffected", token.at, token.size)) {
                        shape->timeAffected = getBoolFromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("beginTimerPeriod", token.at, token.size)) {
                        float period = getFloatFromDataObjects(&data, &tokenizer);
                        // shape->lagTimer = initTimer(period);
                        shape->lagPeriod = period;
                        if(period > 0.0f) {
                            shape->active = false;
                            setLagPeriod = true;
                        }
                    }
                    if(stringsMatchNullN("perpSize", token.at, token.size)) {
                        shape->perpSize = getIntFromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("max", token.at, token.size)) {
                        shape->max = getIntFromDataObjects(&data, &tokenizer);
                    }
                }
            } break;
            default: {
                
            }
        }
    }
    
    if(setLagPeriod) {
        shape->timer.period = shape->lagPeriod;
    }
    //default values if not specified in the level file. 
    if(!hasBgImage) {
        params->bgTex = findTextureAsset("blue_grass.png");
    }

    //Now overwride the background image so they are related to groups
    LevelData *lvlData = params->levelsData + levelTypeIn;
    
    setBackgroundImage(params, lvlData->groupId);
    params->currentGroupId = lvlData->groupId;

    if(!hasLifePoints) {
        
        params->lifePointsMax = 0;
    }
    
    char *at = tokenizer.src;
    
    V2 boardDim = parseGetBoardDim(at);
    //NOTE: we've got enough info to init the board. Note: We could implicitly find the boards dim, instead of explicitly like we're doing now! 
    allocateBoard(params, boardDim.x, boardDim.y);
    
    at = lexEatWhiteSpace(at);
    int xAt = 0;
    int yAt = boardDim.y - 1;
    //NOTE: Parse the board. Since it is individual numbers we are concerned with we can use the standard tokensizer. 
    bool justNewLine = false;
    parsing = true;
    while(parsing) {
        at = lexEatWhiteSpaceExceptNewLine(at);
        
        switch(*at) {
            case '\0': {
                parsing = false;
            } break;
            case '\r': 
            case '\n': {
                xAt = 0;
                yAt--;
                assert(!justNewLine);
                
                justNewLine = true;
                at = lexEatWhiteSpace(at);
            } break;
            case '}': {
                parsing = false;
            } break;
            case '$': {
                setBoardState(params, v2(xAt, yAt), BOARD_STATIC, BOARD_VAL_OLD);    
                at++;
                xAt++;
                justNewLine = false;
            } break;
            case '0': {
                xAt++;
                at++;
                justNewLine = false;
            } break;
            case '/': {
                addGlowingLine(params, yAt, GREEN_LINE);
                at++;
                justNewLine = false;
                params->maxExperiencePoints += XP_PER_LINE;
            } break;
            case ')': {
                addGlowingLine(params, yAt, RED_LINE);
                at++;
                justNewLine = false;
                params->maxExperiencePoints += XP_PER_LINE;
            } break;
            case 'a': {
                setBoardState(params, v2(xAt, yAt), BOARD_STATIC, BOARD_VAL_ALWAYS);    
                xAt++;
                at++;
                justNewLine = false;
            } break;
            case 'b': {
                assert(!"invalid code path");
                // setBoardState(params, v2(xAt, yAt), BOARD_STATIC, BOARD_VAL_GLOW);    
                // xAt++;
                // at++;
                // justNewLine = false;
            } break;
            case '!': {
                assert(params->lifePointsMax > 0);
                setBoardState(params, v2(xAt, yAt), BOARD_EXPLOSIVE, BOARD_VAL_ALWAYS);    
                at++;
                xAt++;
                justNewLine = false;
            } break;
            default: {
                int shapeIdToLookFor = -1;
                if(lexIsNumeric(*at)) {
                    shapeIdToLookFor = (int)((*at) - 48);
                    assert(shapeIdToLookFor >= 0);
                } else {
                    SquareProperty *prop = 0; 
                    for(int propIndex = 0; propIndex < propertyCount; ++propIndex) {
                        SquareProperty *tempProp = squareProperties + propIndex;
                        if(tempProp->id == *at) {
                            prop = tempProp;
                        }
                    }
                    if(prop) {
                        shapeIdToLookFor = prop->shapeId;
                        //add other information for different square info 
                    } else {
                        printf("the id %c didn't match a square property\n", *at);
                    }
                }
                
                if(shapeIdToLookFor >= 0) {
                    ExtraShape *shapeOnStack = findExtraShape(extraShapes, extraShapeCount, shapeIdToLookFor);
                    assert(shapeOnStack);
                    shape = addExtraShape(params);
                    *shape = *shapeOnStack;
                    shape->pos = v2(xAt, yAt); 
                }
                xAt++;
                at++;
                justNewLine = false;
            }
        }
    }
    
    if(!params->isFreestyle) {
        assert(params->maxExperiencePoints != 0);
    }
    
}

V2 getMoveVec(MoveType moveType) {
    V2 moveVec = v2(0, 0);
    if(moveType == MOVE_LEFT) {
        moveVec = v2(-1, 0);
    } else  if(moveType == MOVE_RIGHT) {
        moveVec = v2(1, 0);
    } else  if(moveType == MOVE_DOWN) {
        moveVec = v2(0, -1);
    } else {
        assert(!"not valid path");
    }
    return moveVec;
}

bool canShapeMove(FitrisShape *shape, FrameParams *params, MoveType moveType) {
    bool result = false;
    bool valid = true;
    int leftMostPos = params->boardWidth;
    int rightMostPos = 0;
    int bottomMostPos = params->boardHeight;;
    V2 moveVec = getMoveVec(moveType);
    for(int i = 0; i < shape->count; ++i) {
        if(shape->blocks[i].valid) {
            V2 pos = shape->blocks[i].pos;
            BoardState state = getBoardState(params, v2_plus(pos, moveVec));
            if(!(state == BOARD_NULL || state == BOARD_SHAPE || state == BOARD_EXPLOSIVE)) {
                valid = false;
                break;
            }
            if(shape->blocks[i].pos.x < leftMostPos) {
                leftMostPos = shape->blocks[i].pos.x;
            }
            if(shape->blocks[i].pos.x > rightMostPos) {
                rightMostPos = shape->blocks[i].pos.x;
            }
            if(shape->blocks[i].pos.y < bottomMostPos) {
                bottomMostPos = shape->blocks[i].pos.y;
            }
        }
    }
    if(shape->count) 
    {
        //Check shape won't move off the board//
        if(moveType == MOVE_LEFT) {
            if(leftMostPos > 0 && valid) {
                result = true;
            }
        } else if(moveType == MOVE_RIGHT) {
            assert(rightMostPos < params->boardWidth);
            if(rightMostPos < (params->boardWidth - 1) && valid) {
                result = true;
            } 
        } else if(moveType == MOVE_DOWN) { 
            if(bottomMostPos > 0 && valid) {
                assert(bottomMostPos < params->boardHeight);
                result = true;
            }
        }
    }
    return result;
}
bool isInShape(FitrisShape *shape, V2 pos) {
    bool result = false;
    for(int i = 0; i < shape->count; ++i) {
        if(shape->blocks[i].valid) {
            V2 shapePos = shape->blocks[i].pos;
            if(pos.x == shapePos.x && pos.y == shapePos.y) {
                result = true;
                break;
            }
        }
    }
    return result;
}

typedef struct {
    bool result;
    int index;
} QueryShapeInfo;

QueryShapeInfo isRepeatedInShape(FitrisShape *shape, V2 pos, int index) {
    QueryShapeInfo result = {};
    for(int i = 0; i < shape->count; ++i) {
        if(shape->blocks[i].valid) {
            V2 shapePos = shape->blocks[i].pos;
            if(i != index && pos.x == shapePos.x && pos.y == shapePos.y) {
                result.result = true;
                result.index = i;
                assert(i != index);
                break;
            }
        }
    }
    return result;
}

void resetMouseUI(FrameParams *params) {
    if(params->currentHotIndex >= 0) {
        params->letGo = true;
        V2 pos = params->currentShape.blocks[params->currentHotIndex].pos;
        BoardValue *val = getBoardValue(params, pos);
        val->color = COLOR_WHITE;
        // if(val->cellTracker.active) {
        //     val->cellTracker.fadeTimer = initTimer(0.2f, false);
        // }
        // val->cellTracker.active = false;
        // val->cellTracker.shouldUpdate = false;
    }
    
    params->currentHotIndex = -1; //reset hot ui   
}

static BoardValType BOARD_VAL_SHAPES[5] = { BOARD_VAL_SHAPE0, BOARD_VAL_SHAPE1, BOARD_VAL_SHAPE2, BOARD_VAL_SHAPE3, BOARD_VAL_SHAPE4 };

FitrisBlock *findBlockById(FitrisShape *shape, int id) {
    FitrisBlock *result = 0;
    for(int i = 0; i < shape->count; ++i) {
        if(shape->blocks[i].valid) {
            if(shape->blocks[i].id == id) {
                result = shape->blocks + i;
                break;
            }
        }
    }
    assert(result);
    return result;
}

bool moveShape(FitrisShape *shape, FrameParams *params, MoveType moveType) {
    bool result = canShapeMove(shape, params, moveType);
    if(result) {
        V2 moveVec = getMoveVec(moveType);
        
        assert(!params->wasHitByExplosive);
        // CHECK FOR EXPLOSIVES HIT
        int idsHitCount = 0;
        int idsHit[MAX_SHAPE_COUNT] = {};
        bool playedExplosiveSound = false;
        
        for(int i = 0; i < shape->count; ++i) {
            if(shape->blocks[i].valid) {
                V2 oldPos = shape->blocks[i].pos;
                V2 newPos = v2_plus(oldPos, moveVec);
                
                BoardValue *val = getBoardValue(params, oldPos);
                val->color = COLOR_WHITE;
                
                BoardValue *newVal = getBoardValue(params, newPos);
                newVal->color = COLOR_WHITE;
                
                BoardState state = getBoardState(params, newPos);
                if(state == BOARD_EXPLOSIVE) {
                    assert(params->lifePointsMax > 0);
                    params->lifePoints--;
                    params->wasHitByExplosive = true;
                    if(!playedExplosiveSound) {
                        playGameSound(params->soundArena, params->explosiveSound, 0, AUDIO_FOREGROUND);
                        playedExplosiveSound = true;
                    }
                    
                    //this is so you can't still be holding a block if there is a chance you grab it before 
                    //it explodes. 
                    if(params->currentHotIndex == i) {
                        resetMouseUI(params);
                    }
                    assert(idsHitCount < arrayCount(idsHit));
                    idsHit[idsHitCount++] = shape->blocks[i].id;
                    setBoardState(params, oldPos, BOARD_NULL, BOARD_VAL_NULL);    //this is the shape
                    setBoardState(params, newPos, BOARD_NULL, BOARD_VAL_TRANSIENT);//this is the bomb position
                } 
            }
        }
        
        //NOTE: we have this since our findBlockById wants to search the original shape with the 
        //full count so we can't change it in the loop
        // int newShapeCount = shape->count; 
        for(int hitIndex = 0; hitIndex < idsHitCount; ++hitIndex) {
            int id = idsHit[hitIndex];
            FitrisBlock *block = findBlockById(shape, id);
            block->valid = false;
            //instead of moving block we set it to not valid
            // *block = shape->blocks[--newShapeCount];
        }
        // shape->count = newShapeCount;
        
        for(int i = 0; i < shape->count; ++i) {
            if(shape->blocks[i].valid) {
                V2 oldPos = shape->blocks[i].pos;
                V2 newPos = v2_plus(oldPos, moveVec);
                
                // printf("boardState: %d, index: %d\n", getBoardState(params, oldPos), i);
                assert(getBoardState(params, oldPos) == BOARD_SHAPE);
                
                BoardState newPosState = getBoardState(params, newPos);
                assert(newPosState == BOARD_SHAPE || newPosState == BOARD_NULL);
                
                QueryShapeInfo info = isRepeatedInShape(shape, oldPos, i);
                if(!info.result) { //dind't just get set by the block in shape before. 
                    setBoardState(params, oldPos, BOARD_NULL, BOARD_VAL_NULL);    
                }
                setBoardState(params, newPos, BOARD_SHAPE, shape->blocks[i].type);    
                shape->blocks[i].pos = newPos;
            }
        }
        playGameSound(params->soundArena, params->moveSound, 0, AUDIO_FOREGROUND);
    }
    return result;
}

void solidfyShape(FitrisShape *shape, FrameParams *params) {
    for(int i = 0; i < shape->count; ++i) {
        if(shape->blocks[i].valid) {
            V2 pos = shape->blocks[i].pos;
            BoardValue *val = getBoardValue(params, pos);
            if(val->state == BOARD_SHAPE) {
                setBoardState(params, pos, BOARD_STATIC, BOARD_VAL_OLD);
            }
            val->color = COLOR_WHITE;
        }
        
    }

    playGameSound(params->soundArena, params->solidfyShapeSound, 0, AUDIO_FOREGROUND);
}

/* we were using the below code to do a flood fill to see if the shape was connected. But realised I could just see if the new 
position had any neibours of BORAD_SHAPE. 
*/
typedef struct VisitedQueue VisitedQueue;
typedef struct VisitedQueue {
    V2 pos;
    VisitedQueue *next;
    VisitedQueue *prev;
} VisitedQueue;

#define OLD_WAY_CONNECTION 0
void addToQueryList(FrameParams *params, VisitedQueue *sentinel, V2 pos, Arena *arena, bool *boardArray, int boardWidth, int boardHeight) {
    if(pos.x >= 0 && pos.x < boardWidth && pos.y >= 0 && pos.y < boardHeight) {
        bool *visitedPtr = &boardArray[(((int)pos.y)*boardWidth) + (int)pos.x];
        bool visited = *visitedPtr;
        

        #if OLD_WAY_CONNECTION
        BoardState stateToTest = getBoardState(params, pos);
        #else
        BoardState stateToTest = getBoardStateWithConection(params, pos);
        #endif

        if(!visited && stateToTest == BOARD_SHAPE) {
            *visitedPtr = true;
            VisitedQueue *queue = pushStruct(arena, VisitedQueue); 
            queue->pos = pos;
            
            //add to the search queue
            assert(sentinel->prev->next == sentinel);
            queue->prev = sentinel->prev;
            queue->next = sentinel;
            sentinel->prev->next = queue;
            sentinel->prev = queue;
        }
    }
} 
/* end the queue stuff for the flood fill.*/

static inline int getShapeCount(FitrisShape *shape) {
    int result = 0;
    for(int i = 0; i < shape->count; ++i) {
        if(shape->blocks[i].valid) {
            result++;
        }
    }
    return result;

}

typedef struct {
    int count;
    V2 poses[MAX_SHAPE_COUNT];
} IslandInfo;

IslandInfo getShapeIslandCount(FitrisShape *shape, V2 startPos, FrameParams *params) {
    IslandInfo info = {};
    //NOTE: This is since shape can be blown apart we want to still be able to move a block on their 
    //own island. So the count isn't the shapeCount but only a portion in this count. So we first search 
    //how big the island is and match against that. 
    
    MemoryArenaMark memMark = takeMemoryMark(params->longTermArena);
    bool *boardArray = pushArray(params->longTermArena, params->boardWidth*params->boardHeight, bool);
        
    VisitedQueue sentinel = {};
    sentinel.next = sentinel.prev = &sentinel;
    
#define ADD_TO_QUERY_LIST(toMoveVec, thePos) addToQueryList(params, &sentinel, v2_plus(thePos, toMoveVec), params->longTermArena, boardArray, params->boardWidth, params->boardHeight);
    ADD_TO_QUERY_LIST(v2(0, 0), startPos);
    
    VisitedQueue *queryAt = sentinel.next;
    assert(queryAt != &sentinel);
    while(queryAt != &sentinel) {
        V2 pos = queryAt->pos;
        
        assert(info.count < arrayCount(info.poses));
        info.poses[info.count++] = pos;
        
        ADD_TO_QUERY_LIST(v2(1, 0), pos);
        ADD_TO_QUERY_LIST(v2(-1, 0), pos);
        ADD_TO_QUERY_LIST(v2(0, 1), pos);
        ADD_TO_QUERY_LIST(v2(0, -1), pos);
#if CAN_ALTER_SHAPE_DIAGONAL 
        ADD_TO_QUERY_LIST(v2(1, 1), pos);
        ADD_TO_QUERY_LIST(v2(-1, 1), pos);
        ADD_TO_QUERY_LIST(v2(-1, -1), pos);
        ADD_TO_QUERY_LIST(v2(1, -1), pos);
#endif
        queryAt = queryAt->next;
    }
    releaseMemoryMark(&memMark);
    
    assert(info.count <= getShapeCount(shape));

    //   assert(params->currentHotIndex >= 0);
    // V2 holdingPos = params->currentShape.blocks[params->currentHotIndex].pos;
    // BoardValue *shapeNow = getCopyBoardValue(params, holdingPos);
    // printf("baordState: %d\n", shapeNow->state);
    // assert(shapeNow->state == BOARD_NULL);
    // BoardValue tempBlock = *shapeNow;
    // BoardValue *actualBoardVal = getBoardValue(params, holdingPos);
    // shapeNow->state = actualBoardVal->state;
    // printf("baordState:2 %d\n", shapeNow->state);
    // assert(shapeNow->state == BOARD_SHAPE);

    // *shapeNow = tempBlock;

    return info;
}

bool shapeStillConnected(FitrisShape *shape, int currentHotIndex, V2 boardPosAt, FrameParams *params) {
    bool result = true;
    
    for(int i = 0; i < shape->count; ++i) {
        if(shape->blocks[i].valid) {
            V2 pos = shape->blocks[i].pos;
            //not trying to move it onto a shape
            if(boardPosAt.x == pos.x && boardPosAt.y == pos.y) {
                result = false;
                break;
            }
            
            //has to be an empty space to move it to 
            BoardState state = getBoardState(params, boardPosAt);
            if(state != BOARD_NULL) {
                result = false;
                break;
            }
        }
    }
    if(result) {
        
        V2 oldPos = shape->blocks[currentHotIndex].pos;
        assert(shape->blocks[currentHotIndex].valid);
        
#if OLD_WAY_CONNECTION
        BoardValue *oldVal = getBoardValue(params, oldPos);
        //
        assert(oldVal->state == BOARD_SHAPE);
        //BUG!!!!
#else
        BoardValue *oldVal = getCopyBoardValue(params, oldPos);
        assert(oldVal->state == BOARD_NULL);//new
#endif
        BoardState lastState = oldVal->state;
        oldVal->state = BOARD_SHAPE; //new
        
        
        IslandInfo mainIslandInfo = getShapeIslandCount(shape, oldPos, params);
        assert(mainIslandInfo.count >= 1);

        
        if(mainIslandInfo.count <= 1) {
            //There is an isolated block
            result = false;
        } else {
            V2 idPos = mainIslandInfo.poses[1]; //won't be that starting pos since the first position will dissapear if it is correct. 
            //temporaialy set the board state to where the shape was to be null, so this can't act as a bridge in the flood fill
            oldVal->state = BOARD_NULL;
            
            //set where the board will be to a valid position
            #if OLD_WAY_CONNECTION
            BoardValue *newVal = getBoardValue(params, boardPosAt);
            #else
            BoardValue *newVal = getCopyBoardValue(params, boardPosAt);
            #endif
            assert(newVal->state == BOARD_NULL);
            newVal->state = BOARD_SHAPE;
            ////   This code isn't needed anymore. Just used for the assert below. 
            IslandInfo islandInfo = getShapeIslandCount(shape, boardPosAt, params);
            
            //See if the new pos is part of the same island
            bool found = false;
            for(int index = 0; index < islandInfo.count; ++index) {
                V2 srchPos = islandInfo.poses[index];
                if(srchPos.x == idPos.x && srchPos.y == idPos.y) {
                    found = true;
                    break;
                }
            }
            ////
            
            IslandInfo mainIslandInfo_after = getShapeIslandCount(shape, idPos, params);
            if(mainIslandInfo_after.count < mainIslandInfo.count) {
                result = false;
            } else {
                assert(found);
            }
            //set the state back to being a shape. 
            newVal->state = BOARD_NULL;
        }
        oldVal->state = lastState; //new
    }
    
    return result;
}

void changeMenuStateCallback(void *data) {
    FrameParams *params = (FrameParams *)data;
    resetMouseUI(params);
}

static inline void updateShapeMoveTime(FitrisShape *shape, FrameParams *params) {
    if(wasReleased(params->playStateKeyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
        resetMouseUI(params);
    }
    bool isHoldingShape = params->currentHotIndex >= 0;
    params->moveTime = params->dt;
    
    if(isHoldingShape) { 
        if(!params->wasHoldingShape) {
            params->accumHoldTime = params->moveTimer.period - params->moveTimer.value_;
            assert(params->accumHoldTime >= 0);
        }
        params->moveTime = 0.0f;
        assert(!params->letGo);
    } 
    params->wasHoldingShape = isHoldingShape;
    
    if(params->letGo) {
        assert(!isHoldingShape);
        params->moveTime = params->accumHoldTime;
        params->accumHoldTime = 0;
        params->letGo = false;
        
    } 
}

static inline int getTotalNumberOfShapeBlocks(FrameParams *params) {
    int result = 0;
    for(int i = 0; i < params->shapesCount; ++i) {
        result += params->shapeSizes[i];
    }
    
    return result;
}

bool hasMirrorPartner(FrameParams *params, int hotBlockIndex, int mirrorIndexAt) {
    bool result = false;
    assert(params->isMirrorLevel);
    assert(params->currentShape.blocks[hotBlockIndex].valid);
    assert(mirrorIndexAt >= 0);
    if(mirrorIndexAt < arrayCount(params->currentShape.blocks) && params->currentShape.blocks[mirrorIndexAt].valid) {
        if(hotBlockIndex < mirrorIndexAt) {
            result = mirrorIndexAt < getTotalNumberOfShapeBlocks(params);
        } else {
            assert(hotBlockIndex != mirrorIndexAt);
            result = mirrorIndexAt < params->shapeSizes[0];
        }
    } else {
        // printf("%s: %d\n", "not valid", mirrorIndexAt);
    }
    return result;
}

static inline bool isMirrorPartnerIndex(FrameParams *params, int currentHotIndex, int i, bool isCurrentHotIndex/*for the assert*/) {
    bool result = false;
    
    int mirrorOffsetCount = params->shapeSizes[0];
    int mirrorIndexAt = currentHotIndex < mirrorOffsetCount ? (currentHotIndex + mirrorOffsetCount) : (currentHotIndex - mirrorOffsetCount);
    
    if(params->isMirrorLevel && currentHotIndex >= 0 && hasMirrorPartner(params, currentHotIndex, mirrorIndexAt)) {
        if(currentHotIndex < mirrorOffsetCount) {
            assert(currentHotIndex + mirrorOffsetCount < getTotalNumberOfShapeBlocks(params));
            if((currentHotIndex + mirrorOffsetCount) == i) {
                if(isCurrentHotIndex) {
                    assert(isDown(params->playStateKeyStates->gameButtons, BUTTON_LEFT_MOUSE));
                }
                //this would be if you grabbed the 'lower shape' and you want to hightlight the one above
                result = true;          
            } 
        } else {
            assert(currentHotIndex - mirrorOffsetCount >= 0);
            if((currentHotIndex - mirrorOffsetCount) == i) {
                if(isCurrentHotIndex) {
                    assert(isDown(params->playStateKeyStates->gameButtons, BUTTON_LEFT_MOUSE));
                }
                //this would be if you grabbed the 'higher shape' 
                result = true;
            }
        }
    }
    return result;
}

typedef struct {
    V4 color1;
    V4 color2;
} TwoColors;

typedef struct {
    bool *boardArray;
    int width;
    int height;
} BoardBoolInfo;


bool copyBoardValToCopyBoard(FrameParams *params, V2 pos_, V2 addend, BoardBoolInfo *boardBoolInfo, bool addValue) {
    V2 newPos = v2(pos_.x + addend.x, pos_.y + addend.y);
    bool result = false;
    if(newPos.x >= 0 && newPos.x < boardBoolInfo->width && newPos.y >= 0 && newPos.y < boardBoolInfo->height) {
        bool *alreadyVisited = &boardBoolInfo->boardArray[(((int)newPos.y)*boardBoolInfo->width) + (int)newPos.x];
        if(!(*alreadyVisited)) {
            BoardValue *val1 = getBoardValue(params, newPos);        
            *alreadyVisited = true;
            if(val1->state == BOARD_SHAPE && addValue) {
                BoardValue *copyval = getCopyBoardValue(params, newPos);        
                *copyval = *val1;
                result = true;
            }
            
        }
    }
    return result;
}

void addToQueueForCopyBoard(MemoryArenaMark *memMark, V2 newPos, VisitedQueue *sentinel) {
    VisitedQueue *que = pushStruct(memMark->arena, VisitedQueue);
    que->pos = newPos;
    que->next = sentinel;
    que->prev = sentinel->prev;
    sentinel->prev->next = que;
    sentinel->prev = que;
}

//Note this doesn't put the shape block on the board that is the hot index since this moves during the move. 
void floodFillShape(FrameParams *params, MemoryArenaMark *memMark, int hotIndex) {
    if(params->currentShape.blocks[hotIndex].valid) {
        assert(hotIndex >= 0);
        V2 pos_ = params->currentShape.blocks[hotIndex].pos;
        
        VisitedQueue sentinel = {};
        sentinel.prev = sentinel.next = &sentinel;
        
        bool *boardArray = pushArray(memMark->arena, params->boardWidth*params->boardHeight, bool);
        BoardBoolInfo boardBoolInfo = {};
        boardBoolInfo.width = params->boardWidth;
        boardBoolInfo.height = params->boardHeight;
        boardBoolInfo.boardArray = boardArray;
            
        sentinel.prev = sentinel.next = pushStruct(memMark->arena, VisitedQueue);
        sentinel.next->prev = sentinel.next->next = &sentinel;
        assert(sentinel.next != &sentinel);
        sentinel.next->pos = pos_;

        bool didCopy = copyBoardValToCopyBoard(params, sentinel.next->pos, v2(0, 0), &boardBoolInfo, false);
        assert(!didCopy);

        VisitedQueue *queryAt = sentinel.next;
        assert(queryAt != &sentinel);

        while(queryAt != &sentinel) {
            V2 pos = queryAt->pos;
            if(copyBoardValToCopyBoard(params, pos, v2(1, 0), &boardBoolInfo, true)) {
                V2 newPos = v2_plus(pos, v2(1, 0));
                addToQueueForCopyBoard(memMark, newPos, &sentinel);
            }
            if(copyBoardValToCopyBoard(params, pos, v2(-1, 0), &boardBoolInfo, true)) {
                V2 newPos = v2_plus(pos, v2(-1, 0));
                addToQueueForCopyBoard(memMark, newPos, &sentinel); 
            }
            if(copyBoardValToCopyBoard(params, pos, v2(0, 1), &boardBoolInfo, true)) {
                V2 newPos = v2_plus(pos, v2(0, 1));
                addToQueueForCopyBoard(memMark, newPos, &sentinel);
            }
            if(copyBoardValToCopyBoard(params, pos, v2(0, -1), &boardBoolInfo, true)) {
                V2 newPos = v2_plus(pos, v2(0, -1));
                addToQueueForCopyBoard(memMark, newPos, &sentinel);
            }
            queryAt = queryAt->next;
        }
    }
}

void takeBoardCopy(FrameParams *params) {
    MemoryArenaMark memMark = takeMemoryMark(params->longTermArena);

    for(int y = 0; y < params->boardHeight; y++) {
        for(int x = 0; x < params->boardWidth; x++) {
            // BoardValue *val = getBoardValue(params, v2(x, y));        
            // assert(val);
            BoardValue *copyval = getCopyBoardValue(params, v2(x, y));        
            zeroStruct(copyval, BoardValue);
            // if(val->state != BOARD_SHAPE) {
            //     *copyval = *val;
            // } 
        }
    }

    floodFillShape(params, &memMark, params->currentHotIndex);
    if(params->isMirrorLevel) {
        int mirrorOffsetCount = params->shapeSizes[0];
        int mirrorIndex = params->currentHotIndex < mirrorOffsetCount ? (params->currentHotIndex + mirrorOffsetCount) : (params->currentHotIndex - mirrorOffsetCount);
        if(hasMirrorPartner(params, params->currentHotIndex, mirrorIndex)) { //
            floodFillShape(params, &memMark, mirrorIndex);
        }
    }

    
    releaseMemoryMark(&memMark);
}

/*
NOTE: This is since alien blocks are different colors and so when the hover & 
grab color are the same they don't show up on some. For example yellow hover on a yellow block.
*/
static inline TwoColors getAlienHoverColor(FitrisShape *shape, int indexAt) {
    TwoColors result = {};
    
    BoardValType type = shape->blocks[indexAt].type;
    switch(type) {
        case BOARD_VAL_SHAPE0: { //green
            result.color1 = COLOR_YELLOW;
            result.color2 = COLOR_GREEN;
        } break;
        case BOARD_VAL_SHAPE1: { //yellow
            result.color1 = COLOR_GREEN;
            result.color2 = COLOR_GREEN;
        } break;
        case BOARD_VAL_SHAPE2: { //blue
            result.color1 = COLOR_YELLOW;
            result.color2 = COLOR_GREEN;
        } break;
        case BOARD_VAL_SHAPE3: { //pink
            result.color1 = COLOR_YELLOW;
            result.color2 = COLOR_GREEN;
        } break;
        case BOARD_VAL_SHAPE4: { //beige
            result.color1 = COLOR_YELLOW;
            result.color2 = COLOR_GREEN;
        } break;
        default: {
            assert(!"invalid code path");
        }
    }
    return result;
}

void updateAndRenderShape(FitrisShape *shape, V3 cameraPos, V2 resolution, V2 screenDim, Matrix4 metresToPixels, FrameParams *params, float moveTime) {
    assert(!params->wasHitByExplosive);
    assert(!params->createShape);
    
    
    bool isHoldingShape = params->currentHotIndex >= 0;
    
    bool turnSolid = false;
    
    TimerReturnInfo timerInfo = updateTimer(&params->moveTimer, moveTime);
    if(timerInfo.finished) {
        // printf("%s\n", "moved");
        turnTimerOn(&params->moveTimer);
        timerSetResidue(&params->moveTimer, timerInfo.residue);
        if(!moveShape(shape, params, MOVE_DOWN)) {
            turnSolid = true;
        }
    }
    params->wasHitByExplosive = false;
    if(turnSolid) {
        solidfyShape(shape, params);
        params->createShape = true; 
        params->wasHitByExplosive = false;   
        resetMouseUI(params);
    } else {
        
        int hotBlockIndex = -1;
        for(int i = 0; i < shape->count; ++i) {
            if(shape->blocks[i].valid) {
                //NOTE: Render the hover indications & check for hot player block 
                V2 pos = shape->blocks[i].pos;
                TwoColors alienColors = getAlienHoverColor(shape, i);
                
                RenderInfo renderInfo = calculateRenderInfo(v3(pos.x, pos.y, -1), v3(1, 1, 1), cameraPos, metresToPixels);
                
                Rect2f blockBounds = rect2fCenterDimV2(renderInfo.transformPos.xy, renderInfo.transformDim.xy);
                
                V4 color = COLOR_WHITE;
                
                if(inBounds(params->keyStates->mouseP_yUp, blockBounds, BOUNDS_RECT)) {
                    assert(shape->blocks[i].valid);
                    hotBlockIndex = i;
                    if(params->currentHotIndex < 0) {
                        color = alienColors.color1;
                    }
                    BoardValue *val = getBoardValue(params, pos);
                    if(params->currentHotIndex < 0) { //not already holding a shape
                        val->cellTracker.wasHot = true;
                        val->cellTracker.shouldUpdate = true;
                    }
                    
                }
                
                if(hotBlockIndex >= 0 && params->currentHotIndex < 0) { //something was hot but not holding anything
                    if(isMirrorPartnerIndex(params, hotBlockIndex, i, false)) {
                        //this is the hot partner
                        BoardValue *val = getBoardValue(params, pos);
                        val->cellTracker.wasHot = true;
                        val->cellTracker.shouldUpdate = true;
                        // color = alienColors.color1; //
                    }
                }
                
                if(params->currentHotIndex == i) { //we are holding this shape
                    assert(isDown(params->playStateKeyStates->gameButtons, BUTTON_LEFT_MOUSE));
                    // color = alienColors.color2;
                }
                
                if(isMirrorPartnerIndex(params, params->currentHotIndex, i, true)) {
                    //this is the current hot index
                    // color = alienColors.color2;
                    BoardValue *val = getBoardValue(params, pos);
                    val->cellTracker.wasHot = true;
                    val->cellTracker.shouldUpdate = false;
                }
                
                BoardValue *val = getBoardValue(params, pos);
                assert(val);
                // val->color = color;
            }
        }
        
        //NOTE: Have to do this afterwards since we the block can be before for the mirrorHotIndex
        if(params->currentHotIndex < 0 && hotBlockIndex >= 0 && params->isMirrorLevel) {
            int mirrorOffsetCount = params->shapeSizes[0];
            int mirrorIndexAt = hotBlockIndex < mirrorOffsetCount ? (hotBlockIndex + mirrorOffsetCount) : (hotBlockIndex - mirrorOffsetCount);
            
            if(hasMirrorPartner(params, hotBlockIndex, mirrorIndexAt)) { //same size
                assert(mirrorIndexAt >= 0 && mirrorIndexAt < getTotalNumberOfShapeBlocks(params));
                
                V2 pos = shape->blocks[mirrorIndexAt].pos;
                BoardValue *val = getBoardValue(params, pos);
                assert(val);
                val->cellTracker.wasHot = true;
                val->cellTracker.shouldUpdate = true;
                // val->color = getAlienHoverColor(shape, hotBlockIndex).color1;
            } 
        }
        
        if(wasPressed(params->playStateKeyStates->gameButtons, BUTTON_LEFT_MOUSE) && hotBlockIndex >= 0) {
            params->currentHotIndex = hotBlockIndex;
            takeBoardCopy(params);
        }
        
        if(params->currentHotIndex >= 0) {
            //We are holding onto a block
            V2 pos = params->keyStates->mouseP_yUp;

            //update cell tracker
            BoardValue *val = getBoardValue(params, shape->blocks[params->currentHotIndex].pos);
            val->cellTracker.wasHot = true;
            val->cellTracker.shouldUpdate = false;
            ///
            
            V2 boardPosAt = V4MultMat4(v4(pos.x, pos.y, 1, 1), params->pixelsToMeters).xy;
            boardPosAt.x += cameraPos.x;
            boardPosAt.y += cameraPos.y;
            boardPosAt.x = (int)(clamp(0, boardPosAt.x, params->boardWidth - 1) + 0.5f);
            boardPosAt.y = (int)(clamp(0, boardPosAt.y, params->boardHeight -1) + 0.5f);
            
            int hotIndex = params->currentHotIndex;
            bool okToMove = shapeStillConnected(shape, hotIndex, boardPosAt, params);
            
            V2 boardPosAtMirror = v2(0, 0); //not used unless is a mirror level
            int mirrorIndex = -1;
            bool isEvenSize = true;
            if(params->isMirrorLevel) {
                int mirrorOffsetCount = params->shapeSizes[0];
                mirrorIndex = hotIndex < mirrorOffsetCount ? (hotIndex + mirrorOffsetCount) : (hotIndex - mirrorOffsetCount);
                if(hasMirrorPartner(params, params->currentHotIndex, mirrorIndex)) { //
                    V2 mirrorOffset = v2_minus(boardPosAt, shape->blocks[hotIndex].pos);
                    boardPosAtMirror = v2_plus(shape->blocks[mirrorIndex].pos, mirrorOffset);
                    
                    okToMove &= shapeStillConnected(shape, mirrorIndex, boardPosAtMirror, params);
                } else {
                    // assert(params->shapeSizes[0] != params->shapeSizes[1]);
                    //just move one block
                    isEvenSize = false;
                }
            }
            
            int mirCount = (params->isMirrorLevel && isEvenSize) ? 2 : 1;
            int hotIndexes[2] = {hotIndex, mirrorIndex};
            V2 newPoses[2] = {boardPosAt, boardPosAtMirror};
            
            if(okToMove) {
                //play the sound once
                playGameSound(params->soundArena, params->arrangeSound, 0, AUDIO_FOREGROUND);
                for(int m = 0; m < mirCount; ++m) {
                    int thisHotIndex = hotIndexes[m];
                    
                    V2 oldPos = shape->blocks[thisHotIndex].pos;
                    V2 newPos = newPoses[m];
                    
                    BoardValue *oldVal = getBoardValue(params, oldPos);
                    oldVal->color = COLOR_WHITE;
                    
                    assert(getBoardState(params, oldPos) == BOARD_SHAPE);
                    assert(getBoardState(params, newPos) == BOARD_NULL);
                    setBoardState(params, oldPos, BOARD_NULL, BOARD_VAL_NULL);    
                    setBoardState(params, newPos, BOARD_SHAPE, shape->blocks[thisHotIndex].type);    
                    shape->blocks[thisHotIndex].pos = newPos;
                    assert(getBoardState(params, newPos) == BOARD_SHAPE);
                }
            }
        }
        
    }
}

Texture *getBoardTex(BoardValue *boardVal, BoardState boardState, BoardValType type, FrameParams *params) {
    Texture *tex = 0;
    if(boardState != BOARD_NULL) {
        switch(boardState) {
            case BOARD_STATIC: {
                if(type == BOARD_VAL_OLD) {
                    tex = params->metalTex;
                    assert(tex);
                } else if(type == BOARD_VAL_DYNAMIC) {
                    tex = params->woodTex;
                    assert(tex);
                } else if (type == BOARD_VAL_ALWAYS){
                    tex = params->stoneTex;
                } else {
                    assert(!"invalid path");
                }
                assert(tex);
            } break;
            case BOARD_EXPLOSIVE: {
                tex = params->explosiveTex;
            } break;
            case BOARD_SHAPE: {
                switch(type) {
                    case BOARD_VAL_SHAPE0: {
                        tex = params->alienTex[0];
                    } break;
                    case BOARD_VAL_SHAPE1: {
                        tex = params->alienTex[1];
                    } break;
                    case BOARD_VAL_SHAPE2: {
                        tex = params->alienTex[2];
                    } break;
                    case BOARD_VAL_SHAPE3: {
                        tex = params->alienTex[3];
                    } break;
                    case BOARD_VAL_SHAPE4: {
                        tex = params->alienTex[4];
                    } break;
                    default: {
                        assert(!"invalid path");
                    }
                }
            } break;
            default: {
                assert(!"not handled");
            }
        }
        assert(tex);
    }
    return tex;
}

void loadOverworldPositions(FrameParams *params) {
    char *loadName = concat(globalExeBasePath, "positions.txt");

    LevelType levelType = LEVEL_NULL;
    EntityType entityType = ENTITY_TYPE_NULL;
    V2 pos = v2(0, 0);
    bool parsing = true;
    FileContents contents = getFileContentsNullTerminate(loadName);
    EasyTokenizer tokenizer = lexBeginParsing((char *)contents.memory, true);
    bool wasStar = false;
    while(parsing) {
        EasyToken token = lexGetNextToken(&tokenizer);
        InfiniteAlloc data = {};
        switch(token.type) {
            case TOKEN_NULL_TERMINATOR: {
                parsing = false;
            } break;
            case TOKEN_WORD: {
                if(stringsMatchNullN("pos", token.at, token.size)) {
                    pos = buildV2FromDataObjects(&data, &tokenizer);
                }
                if(stringsMatchNullN("levelType", token.at, token.size)) {
                    wasStar = true;
                    char *stringToCopy = getStringFromDataObjects(&data, &tokenizer);
                    for(int i = 0; i < arrayCount(LevelTypeStrings); ++i) {
                        if(cmpStrNull(LevelTypeStrings[i], stringToCopy)) {
                            levelType = (LevelType)i;
                        }
                    }
                }
                if(stringsMatchNullN("entityType", token.at, token.size)) {
                    wasStar = false;
                    char *stringToCopy = getStringFromDataObjects(&data, &tokenizer);
                    for(int i = 0; i < arrayCount(WorldEntityTypeStrings); ++i) {
                        if(cmpStrNull(WorldEntityTypeStrings[i], stringToCopy)) {
                            entityType = (EntityType)i;
                        }
                    }
                }
            } break;
            case TOKEN_CLOSE_BRACKET: {
                if(wasStar) {
                    params->levelsData[levelType].pos = pos;
                } else {
                    assert(params->entityCount < arrayCount(params->worldEntities));
                    WorldEntity *ent = params->worldEntities + params->entityCount++;
                    ent->type = entityType;
                    ent->pos = pos;
                    if(ent->type == ENTITY_TYPE_SNOW_PARTICLES) {
                        ent->particleSystem = params->overworldParticleSys;
                        prewarmParticleSystem(&ent->particleSystem, v3(0, 0, 0));
                    } else if(ent->type == ENTITY_TYPE_CLOUD) {
                        ent->particleSystem = params->cloudParticleSys;
                        prewarmParticleSystem(&ent->particleSystem, v3(0, 0, 0));
                    }
                }
            } break;
            default: {

            }
        }
    }
    free(loadName);
    free(contents.memory);

}

void initBoard(FrameParams *params, LevelType levelType) {
    params->lastLevelType = levelType; //used to see the position in findaveragepos when we are out of groups to show!!
    params->extraShapeCount = 0;
    params->experiencePoints = 0;
    params->currentHotIndex = -1;
    params->maxExperiencePoints = 0;
    params->glowingLinesCount = 0;
    params->accumHoldTime = 0;
    params->letGo = false;  
    
    params->createShape = true;   
    float tempPeriod = params->moveTimer.period;
    params->moveTimer = initTimer(tempPeriod, true);
    if(params->currentLevelType != levelType) { //not retrying the level
        params->levelNameTimer = initTimer(1.0f, false);
    }
    params->currentLevelType = levelType;
    createLevelFromFile(params, levelType);
    params->lifePoints = params->lifePointsMax;
    
}

static inline V2 findAveragePos(FrameParams *params, LevelType lastLevel) {
    bool foundHighestGroup = false;
    int highestGroupId = 0;
    V2 result = v2(0, 0);
    LevelGroup *highestGroup = 0;
    for(int i = 0; i <= params->maxGroupId && !foundHighestGroup; ++i) {
        LevelGroup *group = params->groups + i;
        group->averagePos = v2(0, 0);
        if(group->count > 0) {
            for(int j = 0; j < group->count; ++j) {
                LevelType type = group->levels[j];
                LevelData *data = &params->levelsData[(int)type];
                group->averagePos = v2_plus(group->averagePos, data->pos);
                if(data->state == LEVEL_STATE_COMPLETED || data->state == LEVEL_STATE_UNLOCKED) {
                    highestGroup = group;
                    if(highestGroupId < i) {
                        highestGroupId = i;
                    }
                    
                } else {
                    foundHighestGroup = true;
                }
            }
        }
    }
    bool groupsToShow = highestGroupId > params->menuInfo.lastShownGroup;
    if(!groupsToShow && lastLevel != LEVEL_NULL) { //when we aren't unlocking anymore groups

        result = params->levelsData[(int)lastLevel].pos;
    } else {
        assert(highestGroup);
        assert(highestGroup->count > 0);
        result = v2_scale(1.0f / (float)highestGroup->count, highestGroup->averagePos);
    }
    assert(!(result.x == 0 && result.y == 0));
    return result;
}

typedef struct {
    LevelType levelType;
    FrameParams *params;
} TransitionDataLevel;

typedef struct {
    TransitionDataLevel levelData;
    FrameParams *params;
    
    MenuInfo *info;
    GameMode lastMode;
    GameMode newMode;
} TransitionDataStartOrEndGame;


void transitionCallbackForLevel(void *data_) {
    TransitionDataLevel *trans = (TransitionDataLevel *)data_;
    FrameParams *params = trans->params;
    
    initBoard(params, trans->levelType);
    resetMouseUI(params);
} 

void transitionCallbackForStartOrEndGame(void *data_) {
    TransitionDataStartOrEndGame *trans = (TransitionDataStartOrEndGame *)data_;
    
    transitionCallbackForLevel(&trans->levelData);
    
    trans->info->gameMode = trans->newMode;
    trans->info->lastMode = trans->lastMode;
    trans->info->menuCursorAt = 0;
    setParentChannelVolume(AUDIO_FLAG_MAIN, 1, SCENE_MUSIC_TRANSITION_TIME);
    setSoundType(AUDIO_FLAG_MAIN);
} 

void transitionCallbackForBackToOverworld(void *data_) {
    TransitionDataStartOrEndGame *trans = (TransitionDataStartOrEndGame *)data_;
    FrameParams *params = trans->params;
    for(int i = 0; i < LEVEL_COUNT; ++i) {
        params->levelsData[i].angle = 0;
        params->levelsData[i].dA = 0;
        params->levelsData[i].particleSystem.Active = false;
        turnTimerOff(&params->levelsData[i].showTimer);
    }

    V3 newPos = v2ToV3(findAveragePos(params, params->lastLevelType), params->overworldCamera.z);
    params->overworldGroupPosAt = newPos;
    params->overworldCamera = newPos;
    params->bgTex = findTextureAsset("blue_grass.png");
    trans->info->gameMode = trans->newMode;
    trans->info->lastMode = trans->lastMode;
    trans->info->menuCursorAt = 0;
    assert(parentChannelVolumes_[AUDIO_FLAG_MENU] == 0);
    setParentChannelVolume(AUDIO_FLAG_MENU, 1, SCENE_MUSIC_TRANSITION_TIME);
    setSoundType(AUDIO_FLAG_MENU);
} 

void transitionCallbackForSettingsScreen(void *data_) {
    TransitionDataStartOrEndGame *trans = (TransitionDataStartOrEndGame *)data_;
    FrameParams *params = trans->params;
    trans->info->gameMode = trans->newMode;
    trans->info->lastMode = trans->lastMode;
    trans->info->menuCursorAt = 0;
    setParentChannelVolume(AUDIO_FLAG_MENU, 1, SCENE_MUSIC_TRANSITION_TIME);
    setSoundType(AUDIO_FLAG_MENU);
} 

void setLevelTransition(FrameParams *params, LevelType levelType) {
    TransitionDataLevel *data = (TransitionDataLevel *)calloc(sizeof(TransitionDataLevel), 1);
    data->levelType = levelType;
    data->params = params;
    setTransition_(&params->transitionState, transitionCallbackForLevel, data);
}

void setBackToOverworldTransition(FrameParams *params) {
    TransitionDataStartOrEndGame *data = (TransitionDataStartOrEndGame *)calloc(sizeof(TransitionDataStartOrEndGame), 1);
    data->info = &params->menuInfo;
    data->lastMode = params->menuInfo.gameMode;
    data->newMode = OVERWORLD_MODE;
    data->params = params;
    setParentChannelVolume(AUDIO_FLAG_MAIN, 0, SCENE_MUSIC_TRANSITION_TIME);
    setTransition_(&params->transitionState, transitionCallbackForBackToOverworld, data);
    
}
void setToLoadScreenTransition(FrameParams *params) {
    TransitionDataStartOrEndGame *data = (TransitionDataStartOrEndGame *)calloc(sizeof(TransitionDataStartOrEndGame), 1);
    data->info = &params->menuInfo;
    data->lastMode = params->menuInfo.gameMode;
    data->newMode = SETTINGS_MODE;
    updateSaveStateDetails(params->levelsData, params->menuInfo.saveStateDetails, arrayCount(params->menuInfo.saveStateDetails));
    data->params = params;
    setParentChannelVolume(AUDIO_FLAG_MAIN, 0, SCENE_MUSIC_TRANSITION_TIME);
    setTransition_(&params->transitionState, transitionCallbackForSettingsScreen, data);
    
}

void setStartOrEndGameTransition(FrameParams *params, LevelType levelType, GameMode newMode) {
    TransitionDataStartOrEndGame *data = (TransitionDataStartOrEndGame *)calloc(sizeof(TransitionDataStartOrEndGame), 1);
    data->levelData.params = params;
    data->levelData.levelType = levelType;
    
    data->info = &params->menuInfo;
    data->lastMode = params->menuInfo.gameMode;
    data->newMode = newMode;
    
    setParentChannelVolume(AUDIO_FLAG_MENU, 0, SCENE_MUSIC_TRANSITION_TIME);
    //this one is different to just setLevelTransition since it changes game mode as well. 
    setTransition_(&params->transitionState, transitionCallbackForStartOrEndGame, data);
}

void removeWinLine(FrameParams *params, int lineToRemove) {
    bool found = false;
    for(int winLineIndex = 0; winLineIndex < params->glowingLinesCount; winLineIndex++) {
        GlowingLine *line = params->glowingLines + winLineIndex;
        if(line->yAt == lineToRemove) {
            //NOTE: Writing the last one in the array over the one we are removing. 
            params->glowingLines[winLineIndex] = params->glowingLines[--params->glowingLinesCount];
            found = true;
            break;
        }
    }
    assert(found);
}

void saveFileData(FrameParams *params) {
    InfiniteAlloc data = initInfinteAlloc(char);
    for(int lvlIndex = 0; lvlIndex < arrayCount(params->levelsData); lvlIndex++) {
        LevelData *levelData = params->levelsData + lvlIndex;
        if(levelData->valid && levelData->state != LEVEL_STATE_LOCKED) {
            assert(levelData->state != LEVEL_STATE_NULL);
            assert(levelData->name);
            
            char *lvlTypeStr = LevelTypeStrings[levelData->levelType];
            char *lvlStateStr = LevelStateStrings[levelData->state]; 
            
            char buffer[256] = {};
            sprintf(buffer, "{\nlevelType: \"%s\";\n", lvlTypeStr);
            addElementInifinteAllocWithCount_(&data, buffer, strlen(buffer));
            
            sprintf(buffer, "levelState: \"%s\";\n}\n", lvlStateStr);
            addElementInifinteAllocWithCount_(&data, buffer, strlen(buffer));
        }
    }
    char shortName[512] = {};
    sprintf(shortName, "saveFile%d.h", params->menuInfo.activeSaveSlot);
    char *writeName = concat(globalExeBasePath, shortName);
    // printf("SAVE FILE: %s\n", writeName);
    
    game_file_handle handle = platformBeginFileWrite(writeName);
    platformWriteFile(&handle, data.memory, data.count*data.sizeOfMember, 0);
    platformEndFile(handle);
    
    free(writeName);
    releaseInfiniteAlloc(&data);
}

static void updateBoardRows(FrameParams *params) {
    if(params->updateRows) {
        for(int boardY = 0; boardY < params->boardHeight; ++boardY) {
            bool isFull = true;
            for(int boardX = 0; boardX < params->boardWidth && isFull; ++boardX) {
                BoardValue *boardVal = &params->board[boardY*params->boardWidth + boardX];
                
                if(!(boardVal->state == BOARD_STATIC && (boardVal->type == BOARD_VAL_OLD || boardVal->type == BOARD_VAL_ALWAYS))) {
                    isFull = false;
                }
            }
            if(isFull) {
                for(int boardX = 0; boardX < params->boardWidth; ++boardX) {
                    BoardValue *boardVal = &params->board[boardY*params->boardWidth + boardX];
                    if(boardVal->state == BOARD_STATIC && boardVal->type == BOARD_VAL_OLD) {
                        setBoardState(params, v2(boardX, boardY), BOARD_NULL, BOARD_VAL_OLD);
                    }
                }
            }
        }
    }
}

static void updateBoardWinState(FrameParams *params) {
    for(int winLineIndex = 0; winLineIndex < params->glowingLinesCount; winLineIndex++) {
        bool increment = true;
        GlowingLine *line = params->glowingLines + winLineIndex;
        if(!line->isDead) {
            int boardY = line->yAt;
            bool win = true;
            for(int boardX = 0; boardX < params->boardWidth; ++boardX) {
                BoardValue *boardVal = &params->board[boardY*params->boardWidth + boardX];
                
                bool answer = true;
                switch(line->type) {
                    case GREEN_LINE: {
                        if(!(boardVal->state == BOARD_STATIC && (boardVal->type == BOARD_VAL_OLD || boardVal->type == BOARD_VAL_ALWAYS))) {
                            answer = false;
                        }  
                    } break;
                    case RED_LINE: {
                        if(!(boardVal->state == BOARD_STATIC && boardVal->type == BOARD_VAL_DYNAMIC)) {
                            answer = false;
                        } 
                    } break;
                    default: {
                        assert(!"invalid code path");
                    }
                }
                if(!answer) {
                    win = false;
                    break;
                }
                
                assert(boardVal->state != BOARD_INVALID);
            }
            
            if(win) {
                params->experiencePoints += XP_PER_LINE;
                for(int boardX = 0; boardX < params->boardWidth; ++boardX) {
                    BoardValue *boardVal = &params->board[boardY*params->boardWidth + boardX];
                    if(boardVal->state == BOARD_STATIC && boardVal->type == BOARD_VAL_OLD) {
                        //only get rid of the squares have been the player's shape
                        setBoardState(params, v2(boardX, boardY), BOARD_NULL, BOARD_VAL_OLD);
                    } else {
                        // printf("state is: %d\n", boardVal->state);
                        // printf("type is: %d\n", boardVal->type);
                        // assert(!"type not recognised");
                    }
                }
                // playGameSound(params->soundArena, params->successSound, 0, AUDIO_FOREGROUND);
                line->isDead = true;
                line->timer = initTimer(0.5f, false);
            }
            
        }
    }
    
    bool allLinesAreDead = true; 
    
    for(int glowLineIndex = 0; glowLineIndex < params->glowingLinesCount && allLinesAreDead; ++glowLineIndex) {
        GlowingLine *glLine = params->glowingLines + glowLineIndex;
        allLinesAreDead &= glLine->isDead; //try break the match 
    }
    
    if(allLinesAreDead && !params->isFreestyle && !params->transitionState.currentTransition) {
        int levelAsInt = (int)params->currentLevelType;
        
        LevelData *currentLevel = params->levelsData + levelAsInt;

        assert(currentLevel->valid);
        int originalGroup = currentLevel->groupId;
        currentLevel->state = LEVEL_STATE_COMPLETED;

        //Now we check to see if we can unlock levels in the next group or if we have finished the game
        
        LevelGroup *lvlGroup = params->groups + originalGroup;
        int lvlsFinishedInGroup = 0;
        LevelType nextLevel = LEVEL_NULL; 
        for(int lvlIndex = 0; lvlIndex < lvlGroup->count; ++lvlIndex) {
            LevelType lvlType = lvlGroup->levels[lvlIndex];
            LevelData *lvlData = params->levelsData + (int)lvlType;
            if(lvlData->state == LEVEL_STATE_COMPLETED) {
                lvlsFinishedInGroup++;
            } else {
                assert(lvlData->state == LEVEL_STATE_UNLOCKED);
                if(nextLevel == LEVEL_NULL) {
                    nextLevel = lvlType;
                } 
            }
        }
        bool goBackToOverworld = false;
        bool finishedGame = false;
        bool unlockNextGroup = false;
        
        if(lvlsFinishedInGroup == lvlGroup->count) {
            unlockNextGroup = true; 
            assert(nextLevel == LEVEL_NULL);
            //finished the group
            goBackToOverworld = true;
            //finished all the levels;
            int completedLevelsCount = 0;
            for(int i = 0; i < LEVEL_COUNT; ++i) {
                LevelData *data = params->levelsData + i;
                if(data->valid) {
                    assert(data->valid);
                    if(data->state == LEVEL_STATE_COMPLETED) {
                        completedLevelsCount++;
                    }
                }
            }
            if(completedLevelsCount == params->menuInfo.totalLevelCount) {
                finishedGame = true;
                goBackToOverworld = false;
                unlockNextGroup = false;
            }
            
        } else {
            float percentage = ((float)lvlsFinishedInGroup / (float)lvlGroup->count);
            // printf("levelsFinsihedInGroup: %d\n", lvlsFinishedInGroup);
            // printf("LevelCount: %d\n", lvlGroup->count);
            // printf("percent:%f\n", percentage);
            
            unlockNextGroup = false;
            if(percentage >= 0.5f) {
                //open next group
                int nextGroupId = originalGroup + 1;
                LevelGroup *nextGroup = params->groups + nextGroupId;
                if(nextGroup->count > 0 && !nextGroup->activated) {
                    unlockNextGroup = true;
                }
            }
        }

        if(unlockNextGroup) {
            int nextGroupId = originalGroup + 1;
            LevelGroup *nextGroup = params->groups + nextGroupId;
            if(!nextGroup->activated) {
                assert(nextGroup->activated == false);
                nextGroup->activated = true;
                for(int groupId = 0; groupId < nextGroup->count; ++groupId) {
                    int lvlId = nextGroup->levels[groupId];
                    assert(params->levelsData[lvlId].valid);
                    assert(params->levelsData[lvlId].state == LEVEL_STATE_LOCKED);
                    params->levelsData[lvlId].state = LEVEL_STATE_UNLOCKED;
                }
            }
        }
        
        if(!finishedGame) {
            if(goBackToOverworld) {
                setBackToOverworldTransition(params);
            } else {
                assert(nextLevel != LEVEL_NULL);
                setLevelTransition(params, nextLevel); 
            }
        } else {
            setStartOrEndGameTransition(params, LEVEL_0, MENU_MODE);
        }
        saveFileData(params);
    }
}

void updateWindmillSide(FrameParams *params, ExtraShape *shape) {
    bool repeat = true;
    bool repeatedYet = false;
    while(repeat) { //this is for when we get blocked 
        repeat = false;
        assert(shape->count <= shape->max);
        BoardState stateToSet;
        BoardState staticState = (shape->isBomb) ? BOARD_EXPLOSIVE : BOARD_STATIC;
        
        int addend = 0;
        if(shape->isOut) {
            addend = 1;
            stateToSet = staticState;
        } else {
            stateToSet = BOARD_NULL;
            addend = -1;
        }
        
        bool wasJustFlipped = shape->justFlipped;
        if(!shape->justFlipped) {
            shape->count += addend;        
        } else {
            shape->justFlipped = false;
        }
        // printf("%d\n", shape->count);
        if(shape->count != 0) {
            BoardState toBoardState = BOARD_INVALID;
            assert(shape->count >= 0 && shape->count <= shape->max);
            V2 shift = v2(shape->growDir.x*(shape->count - 1), shape->growDir.y*(shape->count - 1)); 
            V2 newPos = v2_plus(shape->pos, shift);
            
            toBoardState = getBoardState(params, newPos);
            
            bool settingBlock = (toBoardState == BOARD_NULL && stateToSet == staticState);
            bool isInBounds = inBoardBounds(params, newPos);
            bool blocked = (toBoardState != BOARD_NULL && stateToSet == staticState);
            if((blocked || !isInBounds)) {
                assert(shape->isOut);
                if(shape->count == 1) {
                    assert(stateToSet != BOARD_NULL);
                    shape->isOut = true;
                    if(shape->tryingToBegin) { //on the second attempt after the shape has moved
                        for(int i = 0; i < shape->perpSize; ++i) {
                            shape->growDir = perp(shape->growDir);
                        }
                    }
                    shape->count = 0;
                    shape->tryingToBegin = true;
                } else {
                    shape->isOut = false;
                    repeat = true;
                    shape->justFlipped = true;
                    shape->count--;
                }
            } else {
                assert(shape->count > 0);
                assert(stateToSet != BOARD_INVALID);
                
                assert(isInBounds);
                if(settingBlock || stateToSet == BOARD_NULL) {
                    bool goThrough = true;
                    if(stateToSet == BOARD_NULL) { 
                        if(toBoardState != staticState) { goThrough = false; }
                    }
                    if(goThrough) {
                        setBoardState(params, newPos, stateToSet, BOARD_VAL_DYNAMIC);
                    }
                    
                }
            }
            
            assert(shape->max > 0);
            
            if(shape->count == shape->max) {
                if(!wasJustFlipped) {
                    shape->isOut = false;
                    shape->justFlipped = true;
                } else {
                    assert(!shape->isOut);
                }
            }
        } else {
            assert(shape->count == 0);
            assert(!shape->isOut);
            shape->isOut = true;
            for(int i = 0; i < shape->perpSize; ++i) {
                shape->growDir = perp(shape->growDir);
            }
            // if(shape->lagPeriod != 0.0f) {
            //     repeatedYet = true; //don't repeat for lagging shapes
            //     shape->timer.period = shape->lagPeriod; //we change from the begin period to the lag period
            //     shape->active = false; //we lag for a bit. 
            // }
        }
        
        if(shape->count == 0 && !repeatedYet) {
            repeat = true;
            repeatedYet = true;
        }
    }
}

void renderXPBarAndHearts(FrameParams *params, V2 resolution) {
    if(params->lifePointsMax > 0) {
        float heartDim = 0.6f;
        float across = params->lifePointsMax*heartDim / 2;
        float heartY = params->boardHeight;
        float xAt = 0.5f*params->boardWidth - across;
        for(int heartIndex = 0; heartIndex < params->lifePointsMax; ++heartIndex) {
            Texture *heartTex = 0;
            if(params->lifePoints <= heartIndex) { 
                heartTex = params->heartEmptyTex;
            } else {
                heartTex = params->heartFullTex;
            }
            RenderInfo renderInfo = calculateRenderInfo(v3(xAt, heartY, -2), v3_scale(heartDim, v3(1, 1, 1)), params->cameraPos, params->metresToPixels);
            renderTextureCentreDim(heartTex, renderInfo.pos, renderInfo.dim.xy, COLOR_WHITE, 0, mat4(), renderInfo.pvm, OrthoMatrixToScreen(resolution.x, resolution.y));                    
            xAt += heartDim;
        }
    }
    
    if(params->isFreestyle) {
        
    } else {
        // float barHeight = 0.4f;
        // float ratioXp = clamp01((float)params->experiencePoints / (float)params->maxExperiencePoints);
        // float startXp = -0.5f; //move back half a square
        // float halfXp = 0.5f*params->boardWidth;
        // float xpWidth = ratioXp*params->boardWidth;
        // RenderInfo renderInfo = calculateRenderInfo(v3(startXp + 0.5f*xpWidth, -2*barHeight, -2), v3_scale(1, v3(xpWidth, barHeight, 1)), params->cameraPos, params->metresToPixels);
        // renderDrawRectCenterDim(renderInfo.pos, renderInfo.dim.xy, COLOR_GREEN, 0, mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
        
        // renderInfo = calculateRenderInfo(v3(startXp + halfXp, -2*barHeight, -2), v3_scale(1, v3(params->boardWidth, barHeight, 1)), params->cameraPos, params->metresToPixels);
        // renderDrawRectOutlineCenterDim(renderInfo.pos, renderInfo.dim.xy, COLOR_BLACK, 0, mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
    }
}

//NOTE: this is for indicating which lines will win the level. 
GlowingLine *checkIsWinLine(FrameParams *params, int lineCheckAt) {
    GlowingLine *result = 0;
    for(int winLineIndex = 0; winLineIndex < params->glowingLinesCount; winLineIndex++) {
        GlowingLine *line = params->glowingLines + winLineIndex;
        int line_yAt = line->yAt;
        if(line_yAt == lineCheckAt) {
            result  = line;
            break;
        }
    }
    return result;
}

static inline Texture *getTileTex(FrameParams *params, int xAt, int yAt, Texture **tilesTex) {
    int spots[3][3] = {};
    int xMin = xAt - 1;
    int yMin = yAt - 1;
    for(int yIndex = 0; yIndex < 3; ++yIndex) {
        for(int xIndex = 0; xIndex < 3; ++xIndex) {
            int value = 0;
            int x = xMin + xIndex;
            int y = yMin + yIndex;
            if(x >= 0 && y >= 0 && x < params->overworldDim.x && y < params->overworldDim.y) {
                value = params->overworldValues[y*(int)params->overworldDim.x + x];
            } else {
                assert(!(yIndex == 1 && xIndex == 1));
            }
            //flip them round since we display the board top to bottom
            spots[2 - yIndex][xIndex] = value;
        }
    }
    assert(spots[1][1] == 1);
    
    // for(int i = 0; i < 9; ++i) {
    //     if(i == 3 || i == 6) {
    //         printf("\n");
    //     }
    //     printf("%d", spots[i]);
    
    // }
    // printf("\n");
    // printf("\n");
    tile_pos_type tiletType = easyTile_getTileType(&params->tileLayout, (int *)spots);
    int tileIndex = (int)tiletType;
    Texture *tileTex = tilesTex[tileIndex];
    return tileTex;
}

// static inline float getAndUpdateOverworldDtValue(FrameParams *params, float xAt, float yAt, float dtIn) {   
//     float dtVal = 0;
//     int indexIn = xAt*19 + yAt*19;
//     indexIn %= arrayCount(params->overworldDts);
    
//     OverworldDt *ow_dt = params->overworldDts[indexIn];
    
//     while(ow_dt) {
//         if(ow_dt->x == xAt && ow_dt->y == yAt) {
//             dtVal = ow_dt->val;
//             break;
//         }
//         ow_dt = ow_dt->next; 
//     }
    
//     if(!ow_dt) {
//         OverworldDt *newOne = (OverworldDt *)pushStruct(params->longTermArena, OverworldDt);
//         ow_dt = newOne;
//         newOne->x = xAt;
//         newOne->y = yAt;
//         newOne->val = 0;
        
//         newOne->next = params->overworldDts[indexIn];
//         params->overworldDts[indexIn] = newOne;
//     }
    
//     assert(ow_dt);
//     float returnVal = ow_dt->val;
//     ow_dt->val += dtIn;
//     return returnVal;
// }

// void drawMapSquare(FrameParams *params, float xAt, float yAt, float xSpace, float ySpace, char *at, V2 resolution, V3 overworldCam, char lastEnvir) {
//     V2 offset = params->overworldValuesOffset[(int)(yAt*params->overworldDim.x) + (int)xAt];
//     float xVal = xSpace*xAt + xSpace*offset.x;
//     float yVal = ySpace*yAt + ySpace*offset.y;
//     if(*at == '#') {
//         float heightOfTree = 1.5f*ySpace;
//         float widthOfTree = xSpace;
//         float extraVal = 0.3f*sin(getAndUpdateOverworldDtValue(params, xAt, yAt, params->dt));
//         heightOfTree += extraVal;
//         // widthOfTree += extraVal;
        
//         float yPosOfTree = yVal + 0.5f*extraVal;
//         float xPosOfTree = xVal;
//         RenderInfo extraRenderInfo = calculateRenderInfo(v3(xPosOfTree, yPosOfTree, -1 - (yAt / 100)), v3(widthOfTree, heightOfTree, 1), overworldCam, params->metresToPixels);
//         renderTextureCentreDim(params->treeTex, extraRenderInfo.pos, extraRenderInfo.dim.xy, COLOR_WHITE, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), extraRenderInfo.pvm));     
//     }
//     if(*at == '$') {
//         RenderInfo extraRenderInfo = calculateRenderInfo(v3(xVal, yVal, -1 - (yAt / 100)), v3(xSpace, ySpace, 1), overworldCam, params->metresToPixels);
//         renderTextureCentreDim(params->alienTileTex, extraRenderInfo.pos, extraRenderInfo.dim.xy, COLOR_WHITE, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), extraRenderInfo.pvm));     
//     }
//     if(*at == '*') {
//         float heightOfTree = ySpace;
//         float widthOfTree = 1.4f*xSpace;
//         float extraVal = 0.3f*cos(getAndUpdateOverworldDtValue(params, xAt, yAt, params->dt));
//         widthOfTree += extraVal;
//         // widthOfTree += extraVal;
        
//         float yPosOfTree = yVal;
//         float xPosOfTree = xVal;// + 0.3f*extraVal;
//         RenderInfo extraRenderInfo = calculateRenderInfo(v3(xPosOfTree, yPosOfTree, -1 - (yAt / 100)), v3(widthOfTree, heightOfTree, 1), overworldCam, params->metresToPixels);
//         renderTextureCentreDim(params->cactusTex, extraRenderInfo.pos, extraRenderInfo.dim.xy, COLOR_WHITE, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), extraRenderInfo.pvm));     
//     }
//     if(*at == '^') {
//         RenderInfo extraRenderInfo = calculateRenderInfo(v3(xVal, yVal, -1 - (yAt / 100)), v3(xSpace, ySpace, 1), overworldCam, params->metresToPixels);
//         renderTextureCentreDim(params->rockTex, extraRenderInfo.pos, extraRenderInfo.dim.xy, COLOR_WHITE, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), extraRenderInfo.pvm));     
//     }
//     if(*at == '%') {
//         float heightOfMushroom = ySpace;
//         float widthOfMushroom = xSpace;
//         float extraVal = 0;//0.3f*sin(getAndUpdateOverworldDtValue(params, xAt, yAt, params->dt));
//         heightOfMushroom += extraVal;
//         // widthOfTree += extraVal;
        
//         float yPosOfMushroom = yVal + 0.5f*extraVal;
//         float xPosOfMushroom = xVal;
        
//         RenderInfo extraRenderInfo = calculateRenderInfo(v3(xPosOfMushroom, yPosOfMushroom, -1 - (yAt / 100)), v3(widthOfMushroom, heightOfMushroom, 1), overworldCam, params->metresToPixels);
//         renderTextureCentreDim(params->mushroomTex, extraRenderInfo.pos, extraRenderInfo.dim.xy, COLOR_WHITE, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), extraRenderInfo.pvm));     
//     }
//     // assert(*at == '!');
//     RenderInfo renderInfo = calculateRenderInfo(v3(xSpace*xAt, ySpace*yAt, -2), v3(xSpace, ySpace, 1), overworldCam, params->metresToPixels);
    
//     Texture **tilesTexArray = params->tilesTex;
//     if(*at == '|' || (lastEnvir == '|' && *at != '!')) {
//         tilesTexArray = params->tilesTexSand;
//     }
//     Texture *enviroTileTex = getTileTex(params, xSpace*xAt, ySpace*yAt, tilesTexArray);
//     renderTextureCentreDim(enviroTileTex, renderInfo.pos, renderInfo.dim.xy, COLOR_WHITE, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
// }

void gameUpdateAndRender(void *params_) {
    
    FrameParams *params = (FrameParams *)params_;
    V2 screenDim = *params->screenDim;
    V2 resolution = *params->resolution;
    V2 middleP = v2_scale(0.5f, resolution);
    //ceneter the camera
    params->cameraPos.xy = v2_scale(0.5f, v2((float)params->boardWidth - 1, (float)params->boardHeight - 1));

    easyOS_processKeyStates(params->keyStates, resolution, params->screenDim, params->menuInfo.running);
    //make this platform independent
    easyOS_beginFrame(resolution);
    
    beginRenderGroupForFrame(globalRenderGroup);
    globalRenderGroup->whiteTexture = findTextureAsset("white.png");
    //////CLEAR BUFFERS
    // 
    clearBufferAndBind(params->backbufferId, COLOR_BLACK);
    clearBufferAndBind(params->mainFrameBuffer.bufferId, COLOR_PINK);
    // initRenderGroup(&globalRenderGroup);
    renderEnableDepthTest(globalRenderGroup);
    setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD);
    
    if(params->menuInfo.gameMode != OVERWORLD_MODE && params->menuInfo.gameMode != SPLASH_SCREEN_MODE) {
        if(params->menuInfo.gameMode == PLAY_MODE) {
            setBackgroundImage(params, params->currentGroupId);
        } else {
            params->bgTex = params->blueBackgroundTex;
        }
        
        V2 bgSize = {};
        if(resolution.x > resolution.y) {
            float ratio = params->bgTex->height / params->bgTex->width;
            bgSize.x = resolution.x;
            bgSize.y = resolution.x*ratio;
        } else {
            float ratio = params->bgTex->width / params->bgTex->height;
            bgSize.x = resolution.y*ratio;
            bgSize.y = resolution.y;
        }
        renderTextureCentreDim(params->bgTex, v2ToV3(v2(0, 0), -5), bgSize, COLOR_WHITE, 0, mat4(), mat4(), OrthoMatrixToScreen(resolution.x, resolution.y));                    
    } else if(params->menuInfo.gameMode == SPLASH_SCREEN_MODE) {
        clearBufferAndBind(params->mainFrameBuffer.bufferId, COLOR_WHITE);
    }
    
    for(int partIndex = 0; partIndex < params->particleSystems.count; ++partIndex) {
        // drawAndUpdateParticleSystem(&params->particleSystem, params->dt, v3(0, 0, -4), v3(0, 0 ,0), params->cameraPos, params->metresToPixels, resolution);
    }

    
    
    V2 mouseP = params->keyStates->mouseP;
    V2 halfResInMeters = v2_scale(0.5f, params->resInMeters);
    
    GameMode currentGameMode = drawMenu(&params->menuInfo, params->longTermArena, params->keyStates->gameButtons, 0, params->solidfyShapeSound, params->moveSound, params->dt, resolution, mouseP, params->levelsData, arrayCount(params->levelsData), &params->menuInfo.lastShownGroup);
    
    bool transitioning = updateTransitions(&params->transitionState, resolution, params->dt);
    Rect2f menuMargin = rect2f(0, 0, resolution.x, resolution.y);
    
    bool isPlayState = (currentGameMode == PLAY_MODE);
    
    float helpTextY = 40;
    float uiZAt = -0.3f;
    float uiXPosOffset = halfResInMeters.x - 1;

    params->isHoveringButton = false; //this is so we can't go to a level when we hovering over a button
    
    bool retryButtonPressed = false;    
    if(isPlayState) {
        
        if(!params->backgroundSoundPlaying) {
            //Play and repeat background sound
            PlayingSound *sound = playGameSound(params->soundArena, params->backgroundSound, 0, AUDIO_BACKGROUND);
            // PlayingSound *sound2 = pushGameSound(params->soundArena, params->backgroundSound2, 0, AUDIO_BACKGROUND);
            sound->nextSound = sound;
            // sound2->nextSound = sound;
            params->backgroundSoundPlaying = true;
        }
        
        { //refresh level button
            RenderInfo renderInfo = calculateRenderInfo(v3(-halfResInMeters.x + 1.5f, halfResInMeters.y - 1, -1), v3(1, 1, 1), v3(0, 0, 0), params->metresToPixels);
            
            Rect2f outputDim = rect2fCenterDimV2(renderInfo.transformPos.xy, renderInfo.transformDim.xy);
            static LerpV4 cLerp = initLerpV4(COLOR_WHITE);
            
            float lerpPeriod = 0.3f;
            if(!updateLerpV4(&cLerp, params->dt, LINEAR)) {
                if(!easyLerp_isAtDefault(&cLerp)) {
                    setLerpInfoV4_s(&cLerp, COLOR_WHITE, 0.01, &cLerp.value);
                }
                
            }
            
            V4 uiColor = cLerp.value;
            if(inBounds(params->keyStates->mouseP_yUp, outputDim, BOUNDS_RECT)) {
                setLerpInfoV4_s(&cLerp, UI_BUTTON_COLOR, 0.2f, &cLerp.value);
                if(wasPressed(params->keyStates->gameButtons, BUTTON_LEFT_MOUSE) && !transitioning) {
                    retryButtonPressed = true;
                }
            }
            
            // renderDrawRectOutlineCenterDim(renderInfo.pos, renderInfo.dim.xy, COLOR_BLACK, 0, mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm));            
            renderTextureCentreDim(params->refreshTex, renderInfo.pos, renderInfo.dim.xy, uiColor, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
        }   
    }
    

    if(!transitioning && isPlayState && !params->confirmCloseScreen) {   
        float dtLeft = params->dt;
        float oldDt = params->dt;
        float increment = 1.0f / 480.0f;
        bool rAWasPressed = wasPressed(params->keyStates->gameButtons, BUTTON_R);
        // if(rAWasPressed) { printf("%s\n", "r Was pressed"); }
        while(dtLeft > 0.0f) {
            easyOS_processKeyStates(params->playStateKeyStates, resolution, params->screenDim, params->menuInfo.running);
            
            params->dt = min(increment, dtLeft);
            assert(params->dt > 0.0f);
            //if updating a transition don't update the game logic, just render the game board. 
            bool canDie = params->lifePointsMax > 0;
            
            bool retryLevel = ((params->lifePoints <= 0) && canDie) || rAWasPressed || retryButtonPressed; 

            int testCount = 0;
            for(int shpIndex = 0; shpIndex < params->currentShape.count; shpIndex++) {
                if(!params->currentShape.blocks[shpIndex].valid) {
                    testCount++;
                }
            }
            if(testCount == params->currentShape.count) {
                params->createShape = true;
            }
            if(params->createShape || retryLevel) {
                if(!retryLevel) {
                    params->currentShape.count = 0;
                }
                int totalShapeCountSoFar = 0;
                
                for(int shpIndex = 0; shpIndex < params->shapesCount && !retryLevel; shpIndex++) {
                    int shpSizeAt = params->shapeSizes[shpIndex];
                    int shpOffset = params->startOffsets[shpIndex];
                    for (int i = 0; i < shpSizeAt && !retryLevel; ++i) {
                        int xAt = i + shpOffset;
                        
                        int rowOffset = 0;
                        if(xAt >= params->boardWidth) {
                            rowOffset = xAt / params->boardWidth;
                            xAt %= params->boardWidth;
                        }
                        int yAt = (params->boardHeight - 1) - rowOffset;
                        // assert(i == params->currentShape.count);
                        FitrisBlock *block = &params->currentShape.blocks[params->currentShape.count++];
                        block->pos = v2(xAt, yAt);
                        block->type = BOARD_VAL_SHAPES[i];
                        block->id = totalShapeCountSoFar;
                        block->valid = true;
                        
                        V2 pos = v2(xAt, yAt);
                        if(getBoardState(params, pos) != BOARD_NULL) {
                            //at the top of the board
                            retryLevel = true;
                            break;
                            //
                        } else {
                            setBoardState(params, pos, BOARD_SHAPE, block->type);    
                        }
                        totalShapeCountSoFar++;
                    }
                }
                if(retryLevel) {
                    if(!params->transitionState.currentTransition) 
                    {
                        setLevelTransition(params, params->currentLevelType);
                    } else {
                        
                    }
                }
                
                float tempPeriod = params->moveTimer.period;
                params->moveTimer = initTimer(tempPeriod, true);
                params->createShape = false;
                // assert(params->currentShape.count > 0);
                
            }
            
            updateShapeMoveTime(&params->currentShape, params);
            
            for(int extraIndex = 0; extraIndex < params->extraShapeCount; ++extraIndex) {
                ExtraShape *extraShape = params->extraShapes + extraIndex;
                
                float tUpdate = (extraShape->timeAffected) ? params->moveTime : params->dt;
                // printf("%f\n", tUpdate);
                while(tUpdate > 0.0f) {
                    assert(params->dt >= 0);
                    
                    float dt;
                    if(extraShape->timeAffected) {
                        dt = min(increment, tUpdate);
                    } else {
                        dt = min(params->dt, tUpdate);
                    }

                    float shapeDt = dt;
                    if(!extraShape->active) { 
                        assert(extraShape->lagPeriod > 0.0f);
                        if(extraShape->lagPeriod > 0.0f) {
                            TimerReturnInfo lagInfo = updateTimer(&extraShape->timer, dt);
                            // printf("lag extraShape: %f\n", extraShape->timer.value);
                            if(lagInfo.finished) {
                                turnTimerOn(&extraShape->timer);
                                // printf("time Is: %f\n", extraShape->timer.value);
                                shapeDt = lagInfo.residue;
                                extraShape->active = true; 
                                extraShape->timer.period = extraShape->movePeriod;
                            }
                        }
                    } 
                    
                    if(extraShape->active) {
                        TimerReturnInfo info = updateTimer(&extraShape->timer, shapeDt);
                        // printf("extraShape: %f\n", extraShape->timer.value);
                        if(info.finished) {
                            turnTimerOn(&extraShape->timer);
                            updateWindmillSide(params, extraShape);
                            timerSetResidue(&extraShape->timer, info.residue);
                            
                        }
                    }
                    tUpdate -= dt;
                    assert(tUpdate >= 0.0f);
                }
            }
            updateAndRenderShape(&params->currentShape, params->cameraPos, resolution, screenDim, params->metresToPixels, params, params->moveTime);
            for(int extraIndex = 0; extraIndex < params->extraShapeCount; ++extraIndex) {
                ExtraShape *extraShape = params->extraShapes + extraIndex;
                if(extraShape->tryingToBegin) {
                    updateWindmillSide(params, extraShape);
                }
                extraShape->tryingToBegin = false;
            }
            if(!params->isFreestyle) {
                updateBoardWinState(params);
                updateBoardRows(params);
            } else {
                updateBoardRows(params);
            }
            dtLeft -= params->dt;
            assert(dtLeft >= 0.0f);
            // printf("%s\n", "//////////");
        }
        params->dt = oldDt;
    }
    
    if(isPlayState || currentGameMode == OVERWORLD_MODE) {
        if(currentGameMode == OVERWORLD_MODE) { //Load Button
            // {
            
                // RenderInfo renderInfo = calculateRenderInfo(v3(uiXPosOffset - 3, halfResInMeters.y - 1, uiZAt), v3(1, 1, 1), v3(0, 0, 0), params->metresToPixels);
                
                // Rect2f outputDim = rect2fCenterDimV2(renderInfo.transformPos.xy, renderInfo.transformDim.xy);
                // static LerpV4 cLerp = initLerpV4(COLOR_WHITE);
                
                // if(!updateLerpV4(&cLerp, params->dt, LINEAR)) {
                //     if(!easyLerp_isAtDefault(&cLerp)) {
                //         setLerpInfoV4_s(&cLerp, COLOR_WHITE, 0.01, &cLerp.value);
                //     }
                    
                // }
                
                // V4 uiColor = cLerp.value;
                // if(inBounds(params->keyStates->mouseP_yUp, outputDim, BOUNDS_RECT)) {
                //     params->isHoveringButton = true;
                //     setLerpInfoV4_s(&cLerp, UI_BUTTON_COLOR, 0.2f, &cLerp.value);
                //     if(wasPressed(params->keyStates->gameButtons, BUTTON_LEFT_MOUSE) && !transitioning) {
                        
                //         setToLoadScreenTransition(params);
                //     }
                // }
                
                // renderTextureCentreDim(params->loadTex, renderInfo.pos, renderInfo.dim.xy, uiColor, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
            // }   

            { //Back to overworld button
                RenderInfo renderInfo = calculateRenderInfo(v3(uiXPosOffset - 1.0f, halfResInMeters.y - 1, uiZAt), v3(1, 1, 1), v3(0, 0, 0), params->metresToPixels);
                
                Rect2f outputDim = rect2fCenterDimV2(renderInfo.transformPos.xy, renderInfo.transformDim.xy);
                static LerpV4 cLerp = initLerpV4(COLOR_WHITE);
                
                if(!updateLerpV4(&cLerp, params->dt, LINEAR)) {
                    if(!easyLerp_isAtDefault(&cLerp)) {
                        setLerpInfoV4_s(&cLerp, COLOR_WHITE, 0.01, &cLerp.value);
                    }
                }
                
                V4 uiColor = cLerp.value;
                if(inBounds(params->keyStates->mouseP_yUp, outputDim, BOUNDS_RECT)) {
                    params->isHoveringButton = true;
                    setLerpInfoV4_s(&cLerp, UI_BUTTON_COLOR, 0.2f, &cLerp.value);
                    if(wasPressed(params->keyStates->gameButtons, BUTTON_LEFT_MOUSE) && !transitioning && !isOn(&params->backToOriginTimer)) {
                        params->backToOriginTimer = initTimer(1, false);
                        params->backToOriginStart = params->overworldCamera;
                    }
                }
                
                renderTextureCentreDim(params->mapTex, renderInfo.pos, renderInfo.dim.xy, uiColor, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
            }
        }
        
        // { //Sound Button
        //     RenderInfo renderInfo = calculateRenderInfo(v3(halfResInMeters.y - 1 - 1.5f, halfResInMeters.y - 1, uiZAt), v3(1, 1, 1), v3(0, 0, 0), params->metresToPixels);
            
        //     Rect2f outputDim = rect2fCenterDimV2(renderInfo.transformPos.xy, renderInfo.transformDim.xy);
        //     static LerpV4 cLerp = initLerpV4(COLOR_WHITE);
            
        //     if(!updateLerpV4(&cLerp, params->dt, LINEAR)) {
        //         if(!easyLerp_isAtDefault(&cLerp)) {
        //             setLerpInfoV4_s(&cLerp, COLOR_WHITE, 0.01, &cLerp.value);
        //         }
                
        //     }
            
        //     V4 uiColor = cLerp.value;
        //     if(inBounds(params->keyStates->mouseP_yUp, outputDim, BOUNDS_RECT)) {
        //         params->isHoveringButton = true;
        //         setLerpInfoV4_s(&cLerp, UI_BUTTON_COLOR, 0.2f, &cLerp.value);
        //         if(wasPressed(params->keyStates->gameButtons, BUTTON_LEFT_MOUSE) && !transitioning) {
        //             globalSoundOn = !globalSoundOn;
        //             // changeMenuState(&params->menuInfo, SETTINGS_MODE);
        //         }
        //     }
            
        //     Texture *currentSoundTex = (globalSoundOn) ? params->speakerTex : params->muteTex;
            
        //     renderTextureCentreDim(currentSoundTex, renderInfo.pos, renderInfo.dim.xy, uiColor, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
        // }
        
        // { //Exit Button
        //     RenderInfo renderInfo = calculateRenderInfo(v3(uiXPosOffset, halfResInMeters.y - 1, uiZAt), v3(1, 1, 1), v3(0, 0, 0), params->metresToPixels);
            
        //     Rect2f outputDim = rect2fCenterDimV2(renderInfo.transformPos.xy, renderInfo.transformDim.xy);
        //     static LerpV4 cLerp = initLerpV4(COLOR_WHITE);
            
        //     if(!updateLerpV4(&cLerp, params->dt, LINEAR)) {
        //         if(!easyLerp_isAtDefault(&cLerp)) {
        //             setLerpInfoV4_s(&cLerp, COLOR_WHITE, 0.01, &cLerp.value);
        //         }
                
        //     }
            
        //     V4 uiColor = cLerp.value;
        //     if(inBounds(params->keyStates->mouseP_yUp, outputDim, BOUNDS_RECT)) {
        //         params->isHoveringButton = true;
        //         setLerpInfoV4_s(&cLerp, UI_BUTTON_COLOR, 0.2f, &cLerp.value);
        //         if(wasPressed(params->keyStates->gameButtons, BUTTON_LEFT_MOUSE) && !transitioning) {
        //             changeMenuState(&params->menuInfo, QUIT_MODE);
                    
        //         }
        //     }
            
        //     renderTextureCentreDim(params->errorTex, renderInfo.pos, renderInfo.dim.xy, uiColor, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
        // }
    }

    //Stil render when we are in a transition
    if(isPlayState) {
        
        { //Back to overworld button
            RenderInfo renderInfo = calculateRenderInfo(v3(uiXPosOffset - 1.0f, halfResInMeters.y - 1, uiZAt), v3(1, 1, 1), v3(0, 0, 0), params->metresToPixels);
            
            Rect2f outputDim = rect2fCenterDimV2(renderInfo.transformPos.xy, renderInfo.transformDim.xy);
            static LerpV4 cLerp = initLerpV4(COLOR_WHITE);
            
            if(!updateLerpV4(&cLerp, params->dt, LINEAR)) {
                if(!easyLerp_isAtDefault(&cLerp)) {
                    setLerpInfoV4_s(&cLerp, COLOR_WHITE, 0.01, &cLerp.value);
                }
            }
            
            V4 uiColor = cLerp.value;
            if(inBounds(params->keyStates->mouseP_yUp, outputDim, BOUNDS_RECT)) {
                setLerpInfoV4_s(&cLerp, UI_BUTTON_COLOR, 0.2f, &cLerp.value);
                if(wasPressed(params->keyStates->gameButtons, BUTTON_LEFT_MOUSE) && !transitioning) {
                    setBackToOverworldTransition(params);
                    
                }
            }
            
            renderTextureCentreDim(params->mapTex, renderInfo.pos, renderInfo.dim.xy, uiColor, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
        }
        
        
        
        if(isOn(&params->levelNameTimer)) {
            TimerReturnInfo nameTimeInfo = updateTimer(&params->levelNameTimer, params->dt);
            V4 levelNameFontColor = smoothStep00V4(COLOR_NULL, nameTimeInfo.canonicalVal, COLOR_BLACK);
            float levelNameFontSize = 1.0f;
            char *title = params->levelsData[params->currentLevelType].name;
            float xFontAt = (resolution.x/2) - (getBounds(title, menuMargin, params->font, levelNameFontSize, resolution, params->menuInfo.resolutionDiffScale).x / 2);
            outputTextNoBacking(params->font, xFontAt, 0.5f*resolution.y, -1, resolution, title, menuMargin, levelNameFontColor, levelNameFontSize, true, params->menuInfo.resolutionDiffScale);
        }
        
        //outputText(params->font, 10, helpTextY, -1, resolution, "Press R to reset", menuMargin, COLOR_BLACK, 0.5f, true);
        
        renderXPBarAndHearts(params, resolution);
        for(int boardY = 0; boardY < params->boardHeight; ++boardY) {
            GlowingLine *isWinLine = checkIsWinLine(params, boardY);
            bool renderWinLine = isWinLine;
            TimerReturnInfo winLineInfo = {};
            float alpha = 0.4f;
            V4 winColor = v4(0, 1, 0, alpha);
            
            if(isWinLine) {
                if(isWinLine->type == RED_LINE) {
                    winColor = v4(1, 0, 0, alpha);
                }
                if(isWinLine->isDead) {
                    winLineInfo = updateTimer(&isWinLine->timer, params->dt);
                    alpha = smoothStep01(alpha, winLineInfo.canonicalVal, 0);
                    
                    if(winLineInfo.finished) {
                        if(isWinLine->isDead) {
                            removeWinLine(params, isWinLine->yAt);
                        }
                    }
                } 
            }
            
            for(int boardX = 0; boardX < params->boardWidth; ++boardX) {
                RenderInfo bgRenderInfo = calculateRenderInfo(v3(boardX, boardY, -3), v3(1, 1, 1), params->cameraPos, params->metresToPixels);
                BoardValue *boardVal = &params->board[boardY*params->boardWidth + boardX];
                renderTextureCentreDim(params->boarderTex, bgRenderInfo.pos, bgRenderInfo.dim.xy, COLOR_WHITE, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), bgRenderInfo.pvm));            

                //cell tracker
                {
                    CellTracker *tracker = &boardVal->cellTracker;
                    bool shouldDraw = true;

                    if(tracker->wasHot) {
                        if(!tracker->active) {
                            tracker->active = true;
                            // tracker->shouldUpdate = true;
                            tracker->fadeTimer = initTimer(0.11f, false);    
                        }
                    } else {
                        if(tracker->active) {
                          tracker->fadeTimer = initTimer(0.2f, false);       
                          tracker->active = false;
                          tracker->shouldUpdate = false;
                        }
                    }

                    V4 outlinerColor = tracker->shouldUpdate ? hexARGBTo01Color(0xFFB2D2F4) : hexARGBTo01Color(0xFFCDFFC9);

                    // if(isOn(&tracker->colorGlowTimer)) {
                    //     TimerReturnInfo colorTimerInfo = updateTimer(&tracker->colorGlowTimer, params->dt);
                    //     if(colorTimerInfo.finished) {
                    //         turnTimerOn(&tracker->colorGlowTimer);
                    //     }
                    //     outlinerColor = smoothStep00V4(outlinerColor, colorTimerInfo.canonicalVal, COLOR_YELLOW);
                    // }
                    
                    if(isOn(&tracker->fadeTimer)) {
                        TimerReturnInfo timerInfo = updateTimer(&tracker->fadeTimer, params->dt);
                        if(timerInfo.finished) {
                            turnTimerOff(&tracker->fadeTimer);
                        }
                        if(tracker->active) {
                            outlinerColor = lerpV4(COLOR_NULL, timerInfo.canonicalVal, outlinerColor);    
                        } else {
                            outlinerColor = lerpV4(outlinerColor, timerInfo.canonicalVal, COLOR_NULL);    
                        }
                        
                    } else {
                        if(tracker->active) {
                            
                        } else {
                            shouldDraw = false;

                        }
                    }
                    
                    if(shouldDraw) {
                        drawCellTracker(params, tracker, v2(boardX, boardY), outlinerColor, resolution, -0.2f);
                    }
                    //make all blocks not hot
                    tracker->wasHot = false;
                }
                
                if(!(boardVal->prevState == BOARD_NULL && boardVal->state == BOARD_NULL)) {

                    V4 currentColor = boardVal->color;
                    if(isOn(&boardVal->fadeTimer)) {
                        TimerReturnInfo timeInfo = updateTimer(&boardVal->fadeTimer, params->dt);
                        
                        float lerpT = timeInfo.canonicalVal;
                        V4 prevColor = lerpV4(boardVal->color, clamp01(lerpT), COLOR_NULL);
                        currentColor = lerpV4(COLOR_NULL, lerpT, boardVal->color);
                        
                        RenderInfo prevRenderInfo = calculateRenderInfo(v3(boardX, boardY, -1), v3(1, 1, 1), params->cameraPos, params->metresToPixels);
                        
                        Texture *tex = getBoardTex(boardVal, boardVal->prevState, boardVal->prevType, params);
                        if(tex) {
                            renderTextureCentreDim(tex, prevRenderInfo.pos, prevRenderInfo.dim.xy, prevColor, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), prevRenderInfo.pvm));            
                        }    
                        
                        if(timeInfo.finished) {
                            boardVal->prevState = boardVal->state;
                        }
                    }
                    
                    Texture *tex = getBoardTex(boardVal, boardVal->state, boardVal->type, params);
                    if(tex) {
                        RenderInfo currentStateRenderInfo = calculateRenderInfo(v3(boardX, boardY, -2), v3(1, 1, 1), params->cameraPos, params->metresToPixels);
                        renderTextureCentreDim(tex, currentStateRenderInfo.pos, currentStateRenderInfo.dim.xy, currentColor, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), currentStateRenderInfo.pvm));            
                    }
                } else {
                    assert(!isOn(&boardVal->fadeTimer));
                }
                if(renderWinLine) {
                    winColor.w = alpha;
                    RenderInfo winBlockRenderInfo = calculateRenderInfo(v3(boardX, boardY, -2.5f), v3(1, 1, 1), params->cameraPos, params->metresToPixels);                        
                    renderDrawRectCenterDim(winBlockRenderInfo.pos, winBlockRenderInfo.dim.xy, winColor, 0, mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), winBlockRenderInfo.pvm));            
                    
                    
                }
            }   
        }
    } else if(currentGameMode == OVERWORLD_MODE) {
        float xAt = 0;
        float yAt = 0;
        float coint_xAt = 0;
        float coint_yAt = 0;
        
        float fontSize = 0.7f;
        float lowerY = 0.8f*resolution.y;
        float nameY = 0.9f*resolution.y;
        float ySpace = 0.7f;
        float xSpace = ySpace;
        
        //this is a fixed update loop since we are using a static drag coefficient
        float updateTForMouse = params->dt;
        float updatePerLoop = 1 / 480.0f;
        
        if(isOn(&params->backToOriginTimer)) {
            TimerReturnInfo timerInfo = updateTimer(&params->backToOriginTimer, params->dt);
            params->overworldCamera = smoothStep01V3(params->backToOriginStart, timerInfo.canonicalVal, params->overworldGroupPosAt);
            if(timerInfo.finished) {
                turnTimerOff(&params->backToOriginTimer);
            }

        } else {
            while(updateTForMouse > 0.0f) {
                easyOS_processKeyStates(&params->overworldKeyStates, resolution, params->screenDim, params->menuInfo.running);
                V2 overworldMouseP = params->overworldKeyStates.mouseP;
                if(wasPressed(params->overworldKeyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                    params->lastMouseP = overworldMouseP;
                }
                
                V3 accel = v3(0, 0, 0);
                
                if(isDown(params->overworldKeyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                    V2 diffVec = v2_minus(overworldMouseP, params->lastMouseP);
                    accel.xy = normalizeV2(diffVec);
                    accel.x *= -1; //inverse the pull direction. Since y is down for mouseP, y is already flipped 
                }
                
                if(isDown(params->overworldKeyStates.gameButtons, BUTTON_LEFT)) {
                    accel.x = -1;
                } 
                if(isDown(params->overworldKeyStates.gameButtons, BUTTON_RIGHT)) {
                    accel.x = 1;
                }
                if(isDown(params->overworldKeyStates.gameButtons, BUTTON_UP)) {
                    accel.y = 1;
                }
                if(isDown(params->overworldKeyStates.gameButtons, BUTTON_DOWN)) {
                   accel.y = -1;
                }
                accel.xy = v2_scale(3600.0f, accel.xy);
                
                #if EDITOR_MODE
                if(isDown(params->overworldKeyStates.gameButtons, BUTTON_SHIFT)) {
                #endif

                    params->camVel = v3_plus(v3_scale(updatePerLoop, accel), params->camVel);
                    params->camVel = v3_minus(params->camVel, v3_scale(0.3f, params->camVel));
                    params->overworldCamera = v3_plus(v3_scale(updatePerLoop, params->camVel), params->overworldCamera);
                #if EDITOR_MODE
                }
                #endif
                
                //params->lastMouseP = overworldMouseP;
                updateTForMouse -= updatePerLoop;
                if(updateTForMouse < updatePerLoop) {
                    updatePerLoop = updateTForMouse;
                }
            }
        }

        
        
        float factor = 10.0f;
        V3 overworldCam = params->overworldCamera;
        // overworldCam.x = ((int)((params->overworldCamera.x + 0.5f)*factor)) / factor;
        // overworldCam.y = ((int)((params->overworldCamera.y + 0.5f)*factor)) / factor;
        
        char *at = (char *)global_level_overworld;//params->overworldLayout.memory;
        
        LevelData *levelsAtStore[LEVEL_COUNT] = {};
        bool finished[LEVEL_COUNT] = {};
        
        float waterDim = 2.0f;
        
        int yAcross = (int)(((float)params->resInMeters.y / (float)waterDim)) + 1;
        int xAcross = (int)(((float)params->resInMeters.x / (float)waterDim)) + 1;
        float halfWaterDim = 0.5f*waterDim;
        for(int yVal = 0; yVal < yAcross; yVal++) {
            for(int xVal = 0; xVal < xAcross; xVal++) {
                
                RenderInfo extraRenderInfo = calculateRenderInfo(v3(xVal*waterDim + halfWaterDim, yVal*waterDim + halfWaterDim, -3), v3(waterDim, waterDim, 1), v3(0, 0, 0), params->metresToPixels);
                renderTextureCentreDim(params->waterTex, extraRenderInfo.pos, extraRenderInfo.dim.xy, COLOR_WHITE, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y), extraRenderInfo.pvm));     
            }
            
        }

        // renderTextureCentreDim(params->waterTexBig, v2ToV3(v2(0, 0), -5), resolution, COLOR_WHITE, 0, mat4(), mat4(), OrthoMatrixToScreen(resolution.x, resolution.y));                    
        char lastEnvirChar = '!';
        
        int highestGroupId = params->menuInfo.lastShownGroup;
        at = lexEatWhiteSpace(at);
        bool parsing = true;
        while(parsing) {
            at = lexEatWhiteSpaceExceptNewLine(at);
            char tempChar = *at;
            switch(*at) {
                case '\0': {
                    parsing = false;
                } break;
                case '\r': 
                case '\n': {
                    xAt = 0;
                    yAt++;
                    at = lexEatWhiteSpace(at);
                } break;
                default: {
                    if(*at != '-') {
                        Texture **tilesTexArray = params->tilesTex;
                        if(tempChar == '|') {
                            tilesTexArray = params->tilesTexSand;
                        } else if (tempChar == '=') {
                            tilesTexArray = params->tilesTexSnow;
                        }
                        RenderInfo tileRenderInfo = calculateRenderInfo(v3(xSpace*xAt, ySpace*yAt, -2.5f), v3(xSpace, ySpace, 1), overworldCam, params->metresToPixels);
                        renderTextureCentreDim(getTileTex(params, xAt, yAt, tilesTexArray), tileRenderInfo.pos, tileRenderInfo.dim.xy, COLOR_WHITE, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), tileRenderInfo.pvm)); 
                        
                    }
                    
                    xAt++;
                    at++;
                }
            }
        }

        #if EDITOR_MODE
        
        if(wasPressed(params->keyStates->gameButtons, BUTTON_1)) {
            assert(params->entityCount < arrayCount(params->worldEntities));
            WorldEntity *ent = params->worldEntities + params->entityCount++;
            ent->type = ENTITY_TYPE_ROCK;
        }
        if(wasPressed(params->keyStates->gameButtons, BUTTON_2)) {
            assert(params->entityCount < arrayCount(params->worldEntities));
            WorldEntity *ent = params->worldEntities + params->entityCount++;
            ent->type = ENTITY_TYPE_TREE;
        }
        if(wasPressed(params->keyStates->gameButtons, BUTTON_3)) {
            assert(params->entityCount < arrayCount(params->worldEntities));
            WorldEntity *ent = params->worldEntities + params->entityCount++;
            ent->type = ENTITY_TYPE_MUSHROOM;
        }
        if(wasPressed(params->keyStates->gameButtons, BUTTON_4)) {
            assert(params->entityCount < arrayCount(params->worldEntities));
            WorldEntity *ent = params->worldEntities + params->entityCount++;
            ent->type = ENTITY_TYPE_CACTUS;
        }
        if(wasPressed(params->keyStates->gameButtons, BUTTON_5)) {
            assert(params->entityCount < arrayCount(params->worldEntities));
            WorldEntity *ent = params->worldEntities + params->entityCount++;
            ent->type = ENTITY_TYPE_IGLOO;
        }
        if(wasPressed(params->keyStates->gameButtons, BUTTON_6)) {
            assert(params->entityCount < arrayCount(params->worldEntities));
            WorldEntity *ent = params->worldEntities + params->entityCount++;
            ent->type = ENTITY_TYPE_SNOWMAN;
        }
        if(wasPressed(params->keyStates->gameButtons, BUTTON_7)) {
            assert(params->entityCount < arrayCount(params->worldEntities));
            WorldEntity *ent = params->worldEntities + params->entityCount++;
            ent->type = ENTITY_TYPE_SNOW_PARTICLES;
            ent->particleSystem = params->overworldParticleSys;
            prewarmParticleSystem(&ent->particleSystem, v3(0, 0, 0));
        }
        if(wasPressed(params->keyStates->gameButtons, BUTTON_8)) {
            assert(params->entityCount < arrayCount(params->worldEntities));
            WorldEntity *ent = params->worldEntities + params->entityCount++;
            ent->type = ENTITY_TYPE_CLOUD;
            ent->particleSystem = params->cloudParticleSys;
            prewarmParticleSystem(&ent->particleSystem, v3(0, 0, 0));
        }
        if(wasPressed(params->keyStates->gameButtons, BUTTON_9)) {
            assert(params->entityCount < arrayCount(params->worldEntities));
            WorldEntity *ent = params->worldEntities + params->entityCount++;
            ent->type = ENTITY_TYPE_TREE1;
        }
        if(wasPressed(params->keyStates->gameButtons, BUTTON_TILDE)) {
            assert(params->entityCount < arrayCount(params->worldEntities));
            WorldEntity *ent = params->worldEntities + params->entityCount++;
            ent->type = ENTITY_TYPE_TREE2;
        }
        #endif
        for(int entIndex = 0; entIndex < params->entityCount; ++entIndex) {
            WorldEntity *ent = params->worldEntities + entIndex;

            updateAndRenderWorldEntity(ent, params, params->dt, resolution, overworldCam);

        }
        #if EDITOR_MODE 
        if(wasReleased(params->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
            params->hotEntity = 0;    
        }
        
        #endif
        
        bool playedSound = false;
        bool hoveringOtherStar = false;
        LevelData *nextName = 0;
        for(int levelIndex = 0; levelIndex < LEVEL_COUNT; ++levelIndex) {
            LevelData *levelData = params->levelsData + levelIndex;
            if(levelData->valid) {
                int groupId = levelData->groupId;
                if(!finished[groupId]) {
                    LevelData *levelAt = levelsAtStore[groupId];
                    if(levelAt) {
                        //move to the next one
                        levelAt = levelAt->next;
                        if(!levelAt) {
                            finished[groupId] = true;
                        }
                        
                    } else {
                        //NOTE: set up the level group store 
                        levelAt = params->levelGroups[groupId];
                    }
                    levelsAtStore[groupId] = levelAt;
                    
                    {
                        
                        if(levelAt->state != LEVEL_STATE_LOCKED) {   
                            if(groupId > params->menuInfo.lastShownGroup) {
                                if(highestGroupId < groupId) {
                                    highestGroupId = groupId;
                                }
                                if(levelAt->state != LEVEL_STATE_COMPLETED) {
                                    assert(levelAt->state == LEVEL_STATE_UNLOCKED);
                                    if(!playedSound) {
                                        playMenuSound(params->soundArena, params->showLevelsSound, 0, AUDIO_FOREGROUND);
                                        playedSound = true;
                                    }
                                    levelAt->showTimer = initTimer(2.0f, false);
                                    Reactivate(&levelAt->particleSystem);
                                    levelAt->dA = 10;
                                }
                            }
                        }
                        
                        TimerReturnInfo timeInfo = updateTimer(&levelAt->showTimer, params->dt);
                        float scale = smoothStep00(1, timeInfo.canonicalVal, 2.0f);
                        
                        if(timeInfo.finished) {
                            // levelAt->dA = 0;   
                        }
                        
                        //Update Level icon
                        
                        // levelAt->angle += params->dt*levelAt->dA;
                        levelAt->angle = lerp(0, timeInfo.canonicalVal, 4*PI32);
                        
                        V3 starLocation = v3(levelAt->pos.x, levelAt->pos.y, -1);
                        drawAndUpdateParticleSystem(&levelAt->particleSystem, params->dt, v3(starLocation.x, starLocation.y, starLocation.z + 0.1f), v3(0, 0 ,0), COLOR_WHITE, overworldCam, params->metresToPixels, resolution, true);
                        
                        
                        RenderInfo renderInfo = calculateRenderInfo(starLocation, v3(scale*1, scale*1, 1), overworldCam, params->metresToPixels);
                        
                        V4 color = COLOR_PINK;
                        switch(levelAt->state) {
                            case LEVEL_STATE_COMPLETED: {
                                color = COLOR_GREEN;
                            } break;
                            case LEVEL_STATE_UNLOCKED: {
                                color = COLOR_YELLOW;
                            } break;
                            case LEVEL_STATE_LOCKED: {
                                color = COLOR_GREY;
                            } break;
                            default: {
                                assert(!"invalid code path");
                            }
                        }
                        
                        V2 dim = renderInfo.transformDim.xy;
                        dim.x /= scale;
                        dim.y /= scale;
                        Rect2f outputDim = rect2fCenterDimV2(renderInfo.transformPos.xy, dim);
                        
                        V4 nameColor = COLOR_BLACK;
                        if(isOn(&levelAt->displayNameTimer)) {
                            TimerReturnInfo nameTimerInfo = updateTimer(&levelAt->displayNameTimer, params->dt);
                            nameColor.w = smoothStep01(0, nameTimerInfo.canonicalVal, 1);
                            if(nameTimerInfo.finished) {
                                turnTimerOff(&levelAt->displayNameTimer);
                            }
                        } 

                        if(inBounds(params->keyStates->mouseP_yUp, outputDim, BOUNDS_RECT) && !params->isHoveringButton && !hoveringOtherStar) {
                            hoveringOtherStar = true;
                            //Output the levels name
                            char *levelName = levelAt->name;
                            if(!levelAt->hasPlayedHoverSound) {
                                playMenuSound(params->soundArena, params->arrangeSound, 0, AUDIO_FOREGROUND);    
                                levelAt->hasPlayedHoverSound = true;
                            }
                            //printf("%s\n", levelName);
                            Rect2f outputNameDim = outputText(params->font, 0, 0, -1, resolution, levelName, menuMargin, COLOR_WHITE, fontSize, false, params->menuInfo.resolutionDiffScale);
                            V2 nameDim = getDim(outputNameDim);

                            
                            nextName = levelData;

                            if(!isOn(&levelAt->displayNameTimer) && !levelAt->justOn) {
                                nameColor = COLOR_NULL;
                                turnTimerOn(&nextName->displayNameTimer);
                            }
                            outputText(params->font, 0.5f*(resolution.x - nameDim.x), nameY, -0.1f, resolution, levelName, menuMargin, nameColor, fontSize, true, params->menuInfo.resolutionDiffScale);
                            
                            levelAt->justOn = true;

                            if(wasPressed(params->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
                                // float torqueForce = 1000.0f;
                                // levelAt->dA += params->dt*torqueForce;

                                #if EDITOR_MODE
                                if(!params->hotEntity) {
                                    params->grabbedLevel = levelAt->levelType;
                                }
                                #else
                                    if(levelAt->state != LEVEL_STATE_LOCKED) {
                                        playGameSound(params->soundArena, params->enterLevelSound, 0, AUDIO_FOREGROUND);
                                        
                                        setStartOrEndGameTransition(params, levelAt->levelType, PLAY_MODE);
                                }
                                #endif
                            }
                        } else {
                            levelAt->hasPlayedHoverSound = false;
                            levelAt->justOn = false;
                            turnTimerOff(&levelAt->displayNameTimer);
                        }
                        #if EDITOR_MODE
                        if(wasReleased(params->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
                            params->grabbedLevel = LEVEL_NULL;
                        }   
                        if(params->grabbedLevel != LEVEL_NULL && params->grabbedLevel == levelAt->levelType) {
                            V2 worldP = params->keyStates->mouseP_yUp;
                            worldP = V4MultMat4(v4(worldP.x, worldP.y, 1, 1), params->pixelsToMeters).xy;
                            worldP = v2_plus(worldP, params->overworldCamera.xy);
                            levelAt->pos = worldP;
                            saveOverworldPositions(params);
                        }
                        #endif
                        
                        renderTextureCentreDim(params->starTex, renderInfo.pos, renderInfo.dim.xy, color, levelAt->angle, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
                        
                        float glyphSize = scale*0.2f;
                        V3 numLocation = v3(starLocation.x, starLocation.y, -0.5f);//make sure its in front of star
                        if(levelAt->glyphCount > 1) {
                            float gOffset = levelAt->glyphCount*glyphSize;
                            numLocation.x = numLocation.x - gOffset/2 + (glyphSize/2); //to get half way
                        }
                        for(int gIndex = 0; gIndex < levelAt->glyphCount; ++gIndex) {
                            Texture tempTex = {};
                            GlyphInfo glyph = levelAt->glyphs[gIndex];
                            tempTex.id = glyph.textureHandle;
                            tempTex.uvCoords = glyph.uvCoords;
                            V3 thisNumLocation = numLocation;
                            thisNumLocation.x += gIndex*glyphSize;
                            
                            RenderInfo renderInfo = calculateRenderInfo(thisNumLocation, v3(glyphSize, glyphSize, 1), overworldCam, params->metresToPixels);
                            renderTextureCentreDim(&tempTex, renderInfo.pos, renderInfo.dim.xy, COLOR_BLACK, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
                        }
                    }
                }
            }
        }

        // if(nextName) {
        //     if(params->lastBoundsStar != nextName->levelType) {
        //         turnTimerOn(&nextName->displayNameTimer);
        //         params->lastBoundsStar = nextName->levelType;
        //     }
        // } else {
        //     params->lastBoundsStar = LEVEL_NULL;
        // }
        
        params->menuInfo.lastShownGroup = highestGroupId;
    }
    
    // outputText(params->font, 30, 400, -1, resolution, "hey˙हिन्दीdf©˙\n∆˚", rect2f(0, 0, resolution.x, resolution.y), COLOR_BLACK, 1, true, 1);
    drawRenderGroup(globalRenderGroup);
    
    easyOS_endFrame(resolution, screenDim, &params->dt, params->windowHandle, params->mainFrameBuffer.bufferId, params->backbufferId, params->renderbufferId, &params->lastTime, params->monitorRefreshRate, params->blackBars);
}

int main(int argc, char *args[]) {
#if 0// 3d stuff
    V2 res = v2(1980, 1080);
    Matrix4 perspectiveMat = projectionMatrixFOV(60.0f, res.x/res.y);

    EasyCamera camera;
    easy3d_initCamera(&camera, v3(0, 0, 0));
    V3 pos = screenSpaceToWorldSpace(perspectiveMat, v2(100, 100), res, 10, easy3d_getViewToWorld(&camera));
    error_printFloat3("world Pos: ", pos.E);
    exit(0);
#endif  

    V2 screenDim = {}; //init in create app function
    V2 resolution = v2(0, 0);
    bool blackBars = true;
    bool fullscreen = false;
    if(argc > 1) {
        assert(argc == 5);
        char *resolutionX = args[1];
        char *resolutionY = args[2];
        char *fullscreenStr = args[3];
        char *blackBarsStr = args[4];
        resolution.x = atoi(resolutionX);
        resolution.y = atoi(resolutionY);
        blackBars = atoi(blackBarsStr);
        fullscreen = atoi(fullscreenStr);
    } else {
        resolution = v2(1280, 720);
        // resolution = v2(1980, 1080);
    }

    // V2 resolution = v2(1280, 720);
    // V2 resolution = v2(1980, 1080);
    // V2 resolution = v2(750, 1334);//iphone
    // V2 resolution = v2(640, 480);
    // V2 resolution = v2(800, 500);
    // V2 resolution = v2(1024, 640);
    // V2 resolution = v2(1024, 768);
    // V2 resolution = v2(1152, 720);
    // V2 resolution = v2(1280, 800);
    
    OSAppInfo appInfo = easyOS_createApp(APP_TITLE, &screenDim, fullscreen);
    assert(appInfo.valid);
    
    if(appInfo.valid) {
        Arena soundArena = createArena(Kilobytes(200));
        Arena longTermArena = createArena(Kilobytes(200));
        
        AppSetupInfo setupInfo = easyOS_setupApp(resolution, RESOURCE_PATH_EXTENSION, &longTermArena);
        
        assets = (Asset **)pushSize(&longTermArena, 4096*sizeof(Asset *));
        

        float dt = 1.0f / min((float)setupInfo.refresh_rate, 60.0f); //use monitor refresh rate 
        float idealFrameTime = 1.0f / 60.0f;
        
        ////INIT FONTS
        char *fontName = concat(globalExeBasePath, "/fonts/Khand-Regular.ttf");//Roboto-Regular.ttf");/);
        Font mainFont = initFont(fontName, 128);
        Font numberFont = initFont(concat(globalExeBasePath, "/fonts/UbuntuMono-Regular.ttf"), 42);
        ///
        
#define CREATE_FONT_ATLAS 0
#if CREATE_FONT_ATLAS
        easyAtlas_createTextureAtlas("textureAtlas", "img/", "atlas/", appInfo.windowHandle, &longTermArena, TEXTURE_FILTER_LINEAR, 10);
        easyAtlas_createTextureAtlas("tileAtlas", "tiles/", "atlas/", appInfo.windowHandle, &longTermArena, TEXTURE_FILTER_NEAREST, 0);
        exit(0);
#endif
        // loadAndAddImagesToAssets("img/");
        // loadAndAddImagesToAssets("tiles/");
        easyAtlas_loadTextureAtlas(concat(globalExeBasePath, "atlas/textureAtlas_1"), TEXTURE_FILTER_LINEAR);
        easyAtlas_loadTextureAtlas(concat(globalExeBasePath, "atlas/tileAtlas_1"), TEXTURE_FILTER_NEAREST);
        loadAndAddSoundsToAssets("sounds/", &setupInfo.audioSpec);
        
        bool running = true;
        
        LevelType startLevel = LEVEL_0;
        GameMode startGameMode = MENU_MODE;

        char shortName[1028] = {};
        sprintf(shortName, "saveFile0.h");
        char *saveLevelName = concat(globalExeBasePath, shortName);
        if(platformDoesFileExist(saveLevelName)) {
            startGameMode = OVERWORLD_MODE;
        }
        
        free(saveLevelName);
#if DEVELOPER_MODE
        Tweaker tweaker = {};
        if(refreshTweakFile(concat(globalExeBasePath, "../src/tweakFile.txt"), &tweaker)) {
            char *startLevelStr = getStringFromTweakData(&tweaker, "startingLevel");
            assert(startLevelStr);
            for(int i = 0; i < arrayCount(LevelTypeStrings); ++i) {
                if(cmpStrNull(LevelTypeStrings[i], startLevelStr)) {
                    startLevel = (LevelType)i;
                }
            }
            
            char *startModeStr = getStringFromTweakData(&tweaker, "startMode");
            assert(startModeStr);
            for(int i = 0; i < arrayCount(GameModeTypeStrings); ++i) {
                if(cmpStrNull(GameModeTypeStrings[i], startModeStr)) {
                    startGameMode = (GameMode)i;
                }
            }
            
            globalSoundOn = getBoolFromTweakData(&tweaker, "globalSoundOn");
        }
#endif
        
        Texture *stoneTex = findTextureAsset("elementStone023.png");
        Texture *woodTex = findTextureAsset("elementWood022.png");
        Texture *bgTex = findTextureAsset("blue_grass.png");
        Texture *metalTex = findTextureAsset("elementMetal023.png");
        Texture *explosiveTex = findTextureAsset("elementExplosive049.png");
        Texture *boarderTex = findTextureAsset("elementMetal030.png");
        Texture *heartFullTex = findTextureAsset("hud_heartFull.png");
        Texture *heartEmptyTex = findTextureAsset("hud_heartEmpty.png");
        Texture *starTex = findTextureAsset("starGold.png");
        Texture *treeTex = findTextureAsset("tree3.png");
        Texture *treeTex1 = findTextureAsset("tree1.png");
        Texture *treeTex2 = findTextureAsset("tree2.png");
        Texture *mushroomTex = findTextureAsset("mushrooomTile.png");
        Texture *rockTex = findTextureAsset("brownRock.png");
        Texture *cactusTex = findTextureAsset("cactus.png");

        Texture *waterTex = findTextureAsset("waterTile.png");
        Texture *alienTileTex = findTextureAsset("alienTile.png");
        Texture *mapTex = findTextureAsset("placeholder.png");
        Texture *refreshTex = findTextureAsset("reload.png");
        
        FrameParams *params = (FrameParams *)calloc(sizeof(FrameParams), 1);
        memset(params, 0, sizeof(FrameParams));
        assert(params->levelGroups[0] == 0);
        assert(params);
        
        params->muteTex = findTextureAsset("mute.png");
        params->speakerTex = findTextureAsset("speaker.png");
        params->loadTex = findTextureAsset("save.png");
        params->errorTex = findTextureAsset("error.png");
        params->snowManTex = findTextureAsset("snowman.png");
        params->iglooTex = findTextureAsset("iceHouse.png");
        params->snowflakeTex = findTextureAsset("snowflake.png");
        params->lineTemplateTex = findTextureAsset("lineTemplate.png");

        params->blueBackgroundTex = findTextureAsset("blue_grass.png");
        
        params->solidfyShapeSound = findSoundAsset("thud.wav");
        params->successSound = findSoundAsset("Success2.wav");
        params->explosiveSound = findSoundAsset("explosion.wav");
        params->showLevelsSound = findSoundAsset("showLevels.wav");
        initArray(&params->particleSystems, particle_system);
        
        
        params->alienTex[0] = findTextureAsset("alienGreen.png");
        params->alienTex[1] = findTextureAsset("alienYellow.png");
        params->alienTex[2] = findTextureAsset("alienBlue.png");
        params->alienTex[3] = findTextureAsset("alienPink.png");
        params->alienTex[4] = findTextureAsset("alienBeige.png");
        
        params->tilesTex[0] = findTextureAsset("tileTopLeft.png");
        params->tilesTex[1] = findTextureAsset("tileTopMiddle.png");
        params->tilesTex[2] = findTextureAsset("tileTopRight.png");
        
        params->tilesTex[3] = findTextureAsset("tileMiddleLeft.png");
        params->tilesTex[4] = findTextureAsset("tileMiddleMiddle.png");
        params->tilesTex[5] = findTextureAsset("tileMiddleRight.png");
        
        params->tilesTex[6] = findTextureAsset("tileBottomLeft.png");
        params->tilesTex[7] = findTextureAsset("tileBottomMiddle.png");
        params->tilesTex[8] = findTextureAsset("tileBottomRight.png");
        
        params->tilesTex[9] = findTextureAsset("tileCenterTopLeft.png");
        params->tilesTex[10] = findTextureAsset("tileCenterTopRight.png");
        params->tilesTex[11] = findTextureAsset("tileCenterLeftBottom.png");
        params->tilesTex[12] = findTextureAsset("tileCenterRightBottom.png");
        
        params->tilesTexSand[0] = findTextureAsset("tileTopLeftSand.png");
        params->tilesTexSand[1] = findTextureAsset("tileTopMiddleSand.png");
        params->tilesTexSand[2] = findTextureAsset("tileTopRightSand.png");
        
        params->tilesTexSand[3] = findTextureAsset("tileMiddleLeftSand.png");
        params->tilesTexSand[4] = findTextureAsset("tileMiddleMiddleSand.png");
        params->tilesTexSand[5] = findTextureAsset("tileMiddleRightSand.png");
        
        params->tilesTexSand[6] = findTextureAsset("tileBottomLeftSand.png");
        params->tilesTexSand[7] = findTextureAsset("tileBottomMiddleSand.png");
        params->tilesTexSand[8] = findTextureAsset("tileBottomRightSand.png");
        
        params->tilesTexSand[9] = findTextureAsset("tileCenterTopLeftSand.png");
        params->tilesTexSand[10] = findTextureAsset("tileCenterTopRightSand.png");
        params->tilesTexSand[11] = findTextureAsset("tileCenterLeftBottomSand.png");
        params->tilesTexSand[12] = findTextureAsset("tileCenterRightBottomSand.png");

        params->tilesTexSnow[0] = findTextureAsset("tileTopLeftSnow.png");
        params->tilesTexSnow[1] = findTextureAsset("tileTopMiddleSnow.png");
        params->tilesTexSnow[2] = findTextureAsset("tileTopRightSnow.png");
        
        params->tilesTexSnow[3] = findTextureAsset("tileMiddleLeftSnow.png");
        params->tilesTexSnow[4] = findTextureAsset("tileMiddleMiddleSnow.png");
        params->tilesTexSnow[5] = findTextureAsset("tileMiddleRightSnow.png");
        
        params->tilesTexSnow[6] = findTextureAsset("tileBottomLeftSnow.png");
        params->tilesTexSnow[7] = findTextureAsset("tileBottomMiddleSnow.png");
        params->tilesTexSnow[8] = findTextureAsset("tileBottomRightSnow.png");
        
        params->tilesTexSnow[9] = findTextureAsset("tileCenterTopLeftSnow.png");
        params->tilesTexSnow[10] = findTextureAsset("tileCenterTopRightSnow.png");
        params->tilesTexSnow[11] = findTextureAsset("tileCenterLeftBottomSnow.png");
        params->tilesTexSnow[12] = findTextureAsset("tileCenterRightBottomSnow.png");
        

        params->monitorRefreshRate = 1.0f / (float)setupInfo.refresh_rate;
        params->blackBars = blackBars;

        params->tileLayout = easyTile_initLayouts();
        
#if 1 //particle system in background. Was to distracting. 
        particle_system_settings particleSet = InitParticlesSettings(PARTICLE_SYS_DEFAULT);
        // pushParticleBitmap(&particleSet, findTextureAsset("cloud1.png"), "cloud1");
        // pushParticleBitmap(&particleSet, findTextureAsset("cloud2.png"), "cloud1");
        // pushParticleBitmap(&particleSet, findTextureAsset("cloud3.png"), "cloud1");
        // pushParticleBitmap(&particleSet, findTextureAsset("cloud4.png"), "cloud1");
        pushParticleBitmap(&particleSet, findTextureAsset("snowflake.png"), "snowflake");
        
        particleSet.Loop = true;
        //particleSet.offsetP = v3(0.000000, 0.000000, 0.200000);
        particleSet.bitmapScale = 0.3f;
        particleSet.posBias = rect2f(-1, 0, 1, 0);
        particleSet.VelBias = rect2f(0, -0.5f, 0, -1.0f);
        particleSet.angleBias = v2(0.000000, 6.280000);
        // particleSet.angleForce = v2(-3.000000, 3.000000);
        particleSet.collidesWithFloor = false;
        particleSet.pressureAffected = false;
        
        
        InitParticleSystem(&params->overworldParticleSys, &particleSet);
        params->overworldParticleSys.MaxParticleCount = 16;
        params->overworldParticleSys.viewType = ORTHO_MATRIX;
        setParticleLifeSpan(&params->overworldParticleSys, 5.0f);
        Reactivate(&params->overworldParticleSys);
        assert(params->overworldParticleSys.Active);


        ////////////  Set up the cloud particle system //////////////
        particleSet = InitParticlesSettings(PARTICLE_SYS_DEFAULT);
        // pushParticleBitmap(&particleSet, findTextureAsset("cloud1.png"), "cloud1");
        // pushParticleBitmap(&particleSet, findTextureAsset("cloud2.png"), "cloud1");
        // pushParticleBitmap(&particleSet, findTextureAsset("cloud3.png"), "cloud1");
        // // pushParticleBitmap(&particleSet, findTextureAsset("clouds4.png"), "cloud1");
        pushParticleBitmap(&particleSet, findTextureAsset("snowflake.png"), "snowflake");
        
        particleSet.Loop = true;
        particleSet.bitmapScale = 4.0f;
        particleSet.posBias = rect2f(0, -10, 0, 10);
        particleSet.VelBias = rect2f(0, 1, 0, 2);
        particleSet.collidesWithFloor = false;
        particleSet.pressureAffected = false;
        
        
        InitParticleSystem(&params->cloudParticleSys, &particleSet);
        params->cloudParticleSys.MaxParticleCount = 6;
        params->cloudParticleSys.viewType = ORTHO_MATRIX;
        setParticleLifeSpan(&params->cloudParticleSys, 0.1f);
        Reactivate(&params->cloudParticleSys);
        assert(params->cloudParticleSys.Active);
#endif
        
        params->moveSound = findSoundAsset("menuSound.wav");
        params->arrangeSound = findSoundAsset("click2.wav");
        
        params->backgroundSound = findSoundAsset("Fitris_Soundtrack.wav");//findSoundAsset("Fitris_Soundtrack.wav");
        //params->backgroundSound2 = findSoundAsset("runaway.wav");//findSoundAsset("Fitris_Soundtrack.wav");
        params->enterLevelSound = findSoundAsset("click2.wav");
        
        params->backgroundSoundPlaying = false;
        
        PlayingSound *menuSound = playMenuSound(&soundArena, findSoundAsset("wind.wav"), 0, AUDIO_BACKGROUND);
        menuSound->volume = 0.6f;
        menuSound->nextSound = menuSound;
        
        PlayingSound *startMenuSound = playStartMenuSound(&soundArena, findSoundAsset("Tetris.wav"), 0, AUDIO_BACKGROUND);
        startMenuSound->nextSound = startMenuSound;
        
        //
        
        params->soundArena = &soundArena;
        params->longTermArena = &longTermArena;
        params->dt = dt;
        params->windowHandle = appInfo.windowHandle;
        params->backbufferId = appInfo.frameBackBufferId;
        params->renderbufferId = appInfo.renderBackBufferId;
        params->mainFrameBuffer = createFrameBuffer(resolution.x, resolution.y, FRAMEBUFFER_DEPTH | FRAMEBUFFER_STENCIL);
        params->resolution = &resolution;
        params->screenDim = &screenDim;
        params->metresToPixels = setupInfo.metresToPixels;
        params->pixelsToMeters = setupInfo.pixelsToMeters;
        
        params->keyStates = pushStruct(&longTermArena, AppKeyStates);
        params->playStateKeyStates = pushStruct(&longTermArena, AppKeyStates);
        
        params->font = &mainFont;
        params->numberFont = &numberFont;
        
        params->bgTex = bgTex;
        
        if(startGameMode == PLAY_MODE) {
            parentChannelVolumes_[AUDIO_FLAG_MENU] = 0;
            parentChannelVolumes_[AUDIO_FLAG_START_SCREEN] = 0;
            parentChannelVolumes_[AUDIO_FLAG_MAIN] = 1;
            setSoundType(AUDIO_FLAG_MAIN);
        } else if(startGameMode == MENU_MODE) {
            parentChannelVolumes_[AUDIO_FLAG_MAIN] = 0;
            parentChannelVolumes_[AUDIO_FLAG_MENU] = 0;
            parentChannelVolumes_[AUDIO_FLAG_START_SCREEN] = 1;
            setSoundType(AUDIO_FLAG_START_SCREEN);
        } else {
            parentChannelVolumes_[AUDIO_FLAG_MAIN] = 0;
            parentChannelVolumes_[AUDIO_FLAG_START_SCREEN] = 0;
            parentChannelVolumes_[AUDIO_FLAG_MENU] = 1;
            setSoundType(AUDIO_FLAG_MENU);
        }
        
        int totalLevelCount = loadLevelData(params);
        initBoard(params, startLevel);

        
        char *at = (char *)global_level_overworld;//params->overworldLayout.memory;
        assert(at);
        params->overworldDim = parseGetBoardDim(at);
        int boardCellSize = params->overworldDim.x*params->overworldDim.y;
        params->overworldValues = (int *)calloc(boardCellSize*sizeof(int), 1);
        
        parseOverworldBoard(at, params->overworldValues, params->overworldDim);
        
        params->moveTimer = initTimer(MOVE_INTERVAL, true);
        params->woodTex = woodTex;
        params->stoneTex = stoneTex;
        params->metalTex = metalTex;
        params->explosiveTex = explosiveTex;
        params->boarderTex = boarderTex;
        params->heartFullTex = heartFullTex;
        params->heartEmptyTex = heartEmptyTex;
        params->starTex = starTex;
        params->treeTex = treeTex;
        params->treeTex2 = treeTex1;
        params->treeTex1 = treeTex2;
        params->waterTex = waterTex;
        params->waterTexBig = findTextureAsset("watertilebig.png");
        params->mushroomTex = mushroomTex;
        params->rockTex = rockTex;
        params->cactusTex = cactusTex;
        params->alienTileTex = alienTileTex;
        params->mapTex = mapTex;
        params->refreshTex = refreshTex;
        
        params->lastTime = SDL_GetTicks();
        
        params->cameraPos = v3(0, 0, 0);

        
        TransitionState transState = {};
        transState.transitionSound = findSoundAsset("click.wav");
        transState.soundArena = &soundArena;
        transState.longTermArena = &longTermArena;
        
        params->transitionState = transState;
        
        MenuInfo menuInfo = {};
        menuInfo.font = &mainFont;
        menuInfo.windowHandle = appInfo.windowHandle; 
        menuInfo.running = &running;
        menuInfo.lastMode = menuInfo.gameMode = startGameMode; //from the defines file
        menuInfo.transitionState = &params->transitionState;
        menuInfo.callback = changeMenuStateCallback;
        menuInfo.callBackData = params;
        menuInfo.totalLevelCount = totalLevelCount;
        menuInfo.backTex = findTextureAsset("back.png");
        menuInfo.splashScreenModeTimer = initTimer(5, false);
        menuInfo.levelDataArray = params->levelsData;
        menuInfo.resolutionDiffScale = setupInfo.screenRelativeSize;
        menuInfo.lastShownGroup = -1;
        menuInfo.overworldCam = &params->overworldCamera;
        
        menuInfo.levelGroups = params->groups;
        menuInfo.maxGroupId = params->maxGroupId;
        params->menuInfo = menuInfo;
        
        int lastActiveSaveSlot = 0; //get this from a file that is saved out to disk
        params->menuInfo.activeSaveSlot = lastActiveSaveSlot;

         params->resInMeters = V4MultMat4(v4(resolution.x, resolution.y, 1, 1), params->pixelsToMeters).xy;
        
        loadSaveFile(params->levelsData, arrayCount(params->levelsData), params->menuInfo.activeSaveSlot, &params->menuInfo.lastShownGroup);
        
        for(int groupIndex = 0; groupIndex < params->maxGroupId; ++groupIndex) {
            LevelGroup *group = params->groups + groupIndex;
            for(int lvlIndex = 0; lvlIndex < group->count; ++lvlIndex) {
                LevelData *levelData = &params->levelsData[(int)group->levels[lvlIndex]];
                if(levelData->state == LEVEL_STATE_UNLOCKED || levelData->state == LEVEL_STATE_COMPLETED) {
                    group->activated = true;    
                }
            }
        }

        loadOverworldPositions(params);

        params->overworldCamera = v2ToV3(findAveragePos(params, LEVEL_0), 0);
        params->overworldGroupPosAt = params->overworldCamera;
        turnTimerOff(&params->backToOriginTimer);
        //
        
#if !DESKTOP    
        if(SDL_iPhoneSetAnimationCallback(appInfo.windowHandle, 1, gameUpdateAndRender, params) < 0) {
            assert(!"falid to set");
        }
#endif
        // if(SDL_AddEventWatch(EventFilter, NULL) < 0) {
        // 	assert(!"falid to set");
        // }
        
        while(running) {
            
#if DESKTOP
            gameUpdateAndRender(params);
#endif
        }
        
        for(int levelIndex = 0; levelIndex < LEVEL_COUNT; ++levelIndex) {
            LevelData data = params->levelsData[levelIndex];
            if(data.valid) {
                if(data.name) {
                    //NOTE: should be able to do this once we have a name for every level. But not really neccessary!
                    // free(data.name);
                }
                if(data.contents.memory) {
                    free(data.contents.memory);
                }
            }
        }
        
        easyOS_endProgram(&appInfo);
    }
    return 0;
}
