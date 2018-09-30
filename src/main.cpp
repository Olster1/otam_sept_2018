#include "gameDefines.h"
#if !DESKTOP
#include <OpenGLES/ES3/gl.h>
#else 
#include <GL/gl3w.h>
#endif


#if !DESKTOP
#include <sdl.h>
#include "SDL_syswm.h"
#else 
#include <SDL2/sdl.h>
#include <SDL2/SDL_syswm.h>
#endif

#include "easy_headers.h"

#include "easy_asset_loader.h"

#include "easy_transition.h"
#include "menu.h"

typedef enum {
    BOARD_NULL,
    BOARD_STATIC,
    BOARD_SHAPE,
    BOARD_EXPLOSIVE,
    BOARD_INVALID, //For out of bounds 
} BoardState;

typedef enum {
    BOARD_VAL_NULL,
    BOARD_VAL_OLD,
    BOARD_VAL_ALWAYS,
    BOARD_VAL_SHAPE0,
    BOARD_VAL_SHAPE1,
    BOARD_VAL_SHAPE2,
    BOARD_VAL_SHAPE3,
    BOARD_VAL_DYNAMIC, //this is for the obstacles windmill 
    BOARD_VAL_TRANSIENT, //this isn't used for anything, just to make it so we aren't using the other ones. 
} BoardValType;

typedef struct {
    int id;

    V2 pos;
    BoardValType type; //this is used to keep the images consistent;
} FitrisBlock;

#define MAX_SHAPE_COUNT 16
typedef struct {
    FitrisBlock blocks[MAX_SHAPE_COUNT];
    int count;
    bool valid;

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

typedef struct {
    ExtraShapeType type;

    int id; //this is to link extra shape info with the location in the board text file 

    V2 pos;
    V2 growDir;

    bool active; //this is for the lag period. 

    Timer timer;
    Timer lagTimer;
    float lagPeriod;

    bool isOut; //going out or in
    bool justFlipped;

    int perpSize;

    int count;

    bool timeAffected; //this is if it slows down when the player as grabbed the block 

    int max;
} ExtraShape;

typedef struct {
    BoardValType type;
    BoardValType prevType;
    BoardState state;
    BoardState prevState;

    V4 color;

    Timer fadeTimer;
} BoardValue;

#define LEVEL_TYPE(FUNC) \
FUNC(LEVEL_NULL) \
FUNC(LEVEL_0) \
FUNC(LEVEL_1) \
FUNC(LEVEL_2) \
FUNC(LEVEL_3) \
FUNC(LEVEL_4) \
FUNC(LEVEL_5) \
FUNC(LEVEL_6) \
FUNC(LEVEL_7) \
FUNC(LEVEL_8) \
FUNC(LEVEL_9) \
FUNC(LEVEL_10) \
FUNC(LEVEL_11) \
FUNC(LEVEL_12) \
FUNC(LEVEL_13) \
FUNC(LEVEL_14) \
FUNC(LEVEL_15) \
FUNC(LEVEL_16) \
FUNC(LEVEL_17) \
FUNC(LEVEL_18) \
FUNC(LEVEL_19) \
FUNC(LEVEL_20) \
FUNC(LEVEL_COUNT) \

typedef enum {
    LEVEL_TYPE(ENUM)
} LevelType;

static char *LevelTypeStrings[] = { LEVEL_TYPE(STRING) };

typedef struct {
    Arena *soundArena;
    int boardWidth;
    int boardHeight;
    BoardValue *board;

    FitrisShape currentShape;
    Texture *stoneTex;
    Texture *woodTex;
    Texture *bgTex;
    Texture *metalTex;
    Texture *explosiveTex;
    Texture *boarderTex;
    Texture *heartEmptyTex;
    Texture *heartFullTex;
    Texture *coinTex;

    Texture *alienTex[4];

    WavFile *solidfyShapeSound;
    WavFile *moveSound;
    WavFile *backgroundSound;
    WavFile *successSound;
    WavFile *explosiveSound;

    TransitionState transitionState;

    Timer levelNameTimer;

    int lifePoints;
    int lifePointsMax;
    bool wasHitByExplosive; 

    int extraShapeCount;
    ExtraShape extraShapes[32];

    bool createShape;

    LevelType currentLevelType;
    char *levelNames[LEVEL_COUNT];

    Timer moveTimer;

    float moveTime;

    int currentHotIndex;

    MenuInfo menuInfo;
    
    int experiencePoints;
    int maxExperiencePoints;

    particle_system particleSystem;

    float accumHoldTime;
    bool letGo;  //let the shape go

    int glowingLines[16]; //these are the win lines. We annotate them with a slash in the level markup file
    int glowingLinesCount;

    ////////TODO: This stuff below should be in another struct so isn't there for all projects. 
    Arena *longTermArena;
    float dt;
    SDL_Window *windowHandle;
    AppKeyStates *keyStates;
    FrameBuffer mainFrameBuffer;
    Matrix4 metresToPixels;
    Matrix4 pixelsToMeters;
    V2 screenRelativeSize;
    
    V2 *screenDim;
    V2 *resolution; 

    GLuint backbufferId;
    GLuint renderbufferId;

    Font *font;

    V3 cameraPos;

    unsigned int lastTime;
    ///////
    
} FrameParams;

typedef enum {
    MOVE_LEFT, 
    MOVE_RIGHT,
    MOVE_DOWN
} MoveType;



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

BoardValue *getBoardValue(FrameParams *params, V2 pos) {
    BoardValue *result = 0;
    if(pos.x >= 0 && pos.x < params->boardWidth && pos.y >= 0 && pos.y < params->boardHeight) {
        BoardValue *val = &params->board[params->boardWidth*(int)pos.y + (int)pos.x];
        result = val;
    }
    
    return result;
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
        val->fadeTimer = initTimer(FADE_TIMER_INTERVAL);
    } else {
        assert(!"invalid code path");
    }
}

ExtraShape *addExtraShape(FrameParams *params) {
    assert(params->extraShapeCount < arrayCount(params->extraShapes));
    ExtraShape *shape = params->extraShapes + params->extraShapeCount++;
    memset(shape, 0, sizeof(ExtraShape));
    shape->isOut = true;
    shape->justFlipped = true;
    shape->count = 0;
    shape->active = true;
    shape->perpSize = 0;

    return shape;
}

void allocateBoard(FrameParams *params, int boardWidth, int boardHeight) {
    params->boardWidth = boardWidth;
    params->boardHeight = boardHeight;

    if(params->board) { free(params->board); }
    params->board = (BoardValue *)calloc(params->boardWidth*params->boardHeight*sizeof(BoardValue), 1);

    for(int boardY = 0; boardY < params->boardHeight; ++boardY) {
        for(int boardX = 0; boardX < params->boardWidth; ++boardX) {
            BoardValue *boardVal = &params->board[boardY*params->boardWidth + boardX];

            boardVal->type = BOARD_VAL_NULL;
            boardVal->state = BOARD_NULL;
            boardVal->prevState = BOARD_NULL;

            boardVal->fadeTimer.value = -1;
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
                //Withou this ist seems to only cause a bug on the last line of the baord
                if(*at != '/') {
                    xAt++;
                }
                at++;
            }
        }
    }   
    result.y = yAt;
    return result;
}

void loadLevelNames(FrameParams *params) {
    char *c = concat(globalExeBasePath, "levels/level_names.txt");
    FileContents contents = getFileContentsNullTerminate(c);
    
    assert(contents.memory);
    assert(contents.valid);

    free(c);

    char **namePtr = 0;
    EasyTokenizer tokenizer = lexBeginParsing((char *)contents.memory, true);
    bool parsing = true;

    for(int i = 1; i < LEVEL_COUNT; ++i) {
        params->levelNames[i] = "Name Not Set!";
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
                nullTerminateBuffer(charBuffer, token.at, token.size);
                int indexAt = atoi(charBuffer);
                namePtr = params->levelNames + indexAt;
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

//finds a shape from an id. Used for level loading 
ExtraShape *findExtraShape(FrameParams *params, int id) {
    ExtraShape *result = 0;
    for(int i = 0; i < params->extraShapeCount; ++i) {

        ExtraShape *shape = params->extraShapes + i;
        if(shape->id == id) {
            result = shape;
            break;
        }
    }
    assert(result);
    return result;
}

void createLevelFromFile(FrameParams *params, LevelType levelTypeIn) {

    char *a = concat(LevelTypeStrings[levelTypeIn], ".txt");
    char *b = concat("levels/", a);
    char *c = concat(globalExeBasePath, b);
    FileContents contents = getFileContentsNullTerminate(c);
    
    assert(contents.memory);
    assert(contents.valid);

    free(a);
    free(b);
    free(c);

    EasyTokenizer tokenizer = lexBeginParsing((char *)contents.memory, true);

    ExtraShapeType currentType = EXTRA_SHAPE_NULL;
    ExtraShape *shape = 0;
    bool isLevelData = false;

    int boardWidth = 0;
    int boardHeight = 0;
    LevelType levelType = LEVEL_NULL;
    bool hasBgImage = false;
    bool hasLifePoints = false;
    bool parsing = true;
    bool hasLevelName = true;
    while(parsing) {
        EasyToken token = lexGetNextToken(&tokenizer);
        InfiniteAlloc data = {};
        switch(token.type) {
            case TOKEN_NULL_TERMINATOR: {
                parsing = false;
            } break;
            case TOKEN_WORD: {
                if(stringsMatchNullN("Windmill", token.at, token.size)) {
                    currentType = EXTRA_SHAPE_WINDMILL;
                    shape = addExtraShape(params);
                    isLevelData = false;
                }

                if(stringsMatchNullN("LevelData", token.at, token.size)) {
                    isLevelData = true;
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
                }
                if(stringsMatchNullN("Board", token.at, token.size)) {
                    parsing = false;
                    EasyToken nxtToken = lexGetNextToken(&tokenizer);
                    assert(nxtToken.type == TOKEN_COLON);
                    nxtToken = lexGetNextToken(&tokenizer);
                    assert(nxtToken.type == TOKEN_OPEN_BRACKET);
                }

                if(currentType == EXTRA_SHAPE_WINDMILL) {
                    assert(shape);
                    if(stringsMatchNullN("id", token.at, token.size)) {
                        shape->id = getIntFromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("growDir", token.at, token.size)) {
                        shape->growDir = buildV2FromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("growTimerPeriod", token.at, token.size)) {
                        float period = getFloatFromDataObjects(&data, &tokenizer);
                        shape->timer = initTimer(period);
                    }
                    if(stringsMatchNullN("lagTimerPeriod", token.at, token.size)) {
                        float period = getFloatFromDataObjects(&data, &tokenizer);
                        shape->lagPeriod = period;
                    }
                    if(stringsMatchNullN("timeAffected", token.at, token.size)) {
                        shape->timeAffected = getBoolFromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("beginTimerPeriod", token.at, token.size)) {
                        float period = getFloatFromDataObjects(&data, &tokenizer);
                        shape->lagTimer = initTimer(period);
                        if(period != 0.0f) {
                            shape->active = false;
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

    //default values if not specified in the level file. 
    if(!hasBgImage) {
        params->bgTex = findTextureAsset("blue_grass.png");
    }
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
            case '0': {
                xAt++;
                at++;
                justNewLine = false;
            } break;
            case '/': {
                assert(params->glowingLinesCount < arrayCount(params->glowingLines));
                params->glowingLines[params->glowingLinesCount++] = yAt;
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
            case '!': {
                setBoardState(params, v2(xAt, yAt), BOARD_EXPLOSIVE, BOARD_VAL_ALWAYS);    
                at++;
                xAt++;
                justNewLine = false;
            } break;
            default: {
                if(lexIsNumeric(*at)) {
                    int id = (int)((*at) - 48);
                    ExtraShape *shape = findExtraShape(params, id);
                    shape->pos = v2(xAt, yAt);
                }
                xAt++;
                at++;
                justNewLine = false;
            }
        }
    }

    assert(params->maxExperiencePoints != 0);
    
}

void createLevel_DEPRECTATED(FrameParams *params, int blockCount, LevelType levelType) {
    if(levelType == LEVEL_4) {
        ExtraShape *shape = addExtraShape(params);

        shape->pos = v2(2, 2);
        shape->growDir = v2(1, 0);

        // setBoardState(params, shape->pos, BOARD_STATIC, BOARD_VAL_ALWAYS);    
        shape->timer = initTimer(0.5f);
        shape->lagTimer = initTimer(0.5f);
        shape->max = 3;
    }

    for(int i = 0; i < blockCount && levelType != LEVEL_0 && levelType != LEVEL_4; ++i) {
        V2 pos = {};
        float rand1 = getRandNum01_include();
        float rand2 = getRandNum01_include();
        pos.x = lerp(0, rand1, (float)(params->boardWidth - 1));
        pos.y = lerp(0, rand2, (float)(params->boardHeight - 5)); // so we don't block the shape creation

        BoardState state = BOARD_NULL;
        switch(levelType) {
            case LEVEL_1: {
                state = BOARD_STATIC;
            } break;
            case LEVEL_2: {
                int type = (int)lerp(0, getRandNum01(), 2);
                if(type == 0) { state = BOARD_STATIC; }
                if(type == 1) { state = BOARD_EXPLOSIVE; }
            } break;
            case LEVEL_3: {
                state = BOARD_EXPLOSIVE;
            } break;
            default: {
                assert(!"case not handled");
            }
        }
        
        setBoardState(params, v2((int)pos.x, (int)pos.y), state, BOARD_VAL_ALWAYS);    
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
      V2 shapePos = shape->blocks[i].pos;
      if(pos.x == shapePos.x && pos.y == shapePos.y) {
        result = true;
        break;
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
      V2 shapePos = shape->blocks[i].pos;
      if(i != index && pos.x == shapePos.x && pos.y == shapePos.y) {
        result.result = true;
        result.index = i;
        assert(i != index);
        break;
      }
   }
   return result;
}

static BoardValType BOARD_VAL_SHAPES[4] = { BOARD_VAL_SHAPE0, BOARD_VAL_SHAPE1, BOARD_VAL_SHAPE2, BOARD_VAL_SHAPE3 };

FitrisBlock *findBlockById(FitrisShape *shape, int id) {
    FitrisBlock *result = 0;
    for(int i = 0; i < shape->count; ++i) {
        if(shape->blocks[i].id == id) {
            result = shape->blocks + i;
            break;
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
        
        for(int i = 0; i < shape->count; ++i) {
          V2 oldPos = shape->blocks[i].pos;
          V2 newPos = v2_plus(oldPos, moveVec);
          BoardState state = getBoardState(params, newPos);
          if(state == BOARD_EXPLOSIVE) {
            assert(params->lifePointsMax > 0);
            params->lifePoints--;
            params->wasHitByExplosive = true;
            playSound(params->soundArena, params->explosiveSound, 0, AUDIO_FOREGROUND);
            //remove from shapea
            assert(idsHitCount < arrayCount(idsHit));
            idsHit[idsHitCount++] = shape->blocks[i].id;
            setBoardState(params, oldPos, BOARD_NULL, BOARD_VAL_NULL);    //this is the shape
            setBoardState(params, newPos, BOARD_NULL, BOARD_VAL_TRANSIENT);//this is the bomb position
          } 
        }

        //NOTE: we have this since our findBlockById wants to search the original shape with the 
        //full count so we can't change it in the loop
        int newShapeCount = shape->count; 
        for(int hitIndex = 0; hitIndex < idsHitCount; ++hitIndex) {
            int id = idsHit[hitIndex];
            FitrisBlock *block = findBlockById(shape, id);
            *block = shape->blocks[--newShapeCount];
        }
        shape->count = newShapeCount;

       for(int i = 0; i < shape->count; ++i) {
            V2 oldPos = shape->blocks[i].pos;
            V2 newPos = v2_plus(oldPos, moveVec);
    
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
        playSound(params->soundArena, params->moveSound, 0, AUDIO_FOREGROUND);
    }
    return result;
}

void solidfyShape(FitrisShape *shape, FrameParams *params) {
    for(int i = 0; i < shape->count; ++i) {
        V2 pos = shape->blocks[i].pos;
        BoardValue *val = getBoardValue(params, pos);
        if(val->state == BOARD_SHAPE) {
            setBoardState(params, pos, BOARD_STATIC, BOARD_VAL_OLD);
        }
        val->color = COLOR_WHITE;

    }
    playSound(params->soundArena, params->solidfyShapeSound, 0, AUDIO_FOREGROUND);
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

void addToQueryList(FrameParams *params, VisitedQueue *sentinel, V2 pos, Arena *arena, bool *boardArray, int boardWidth) {
    bool *visitedPtr = &boardArray[(((int)pos.y)*boardWidth) + (int)pos.x];
    bool visited = *visitedPtr;
    
    if(!visited && getBoardState(params, pos) == BOARD_SHAPE) {
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
/* end the queue stuff for the flood fill.*/

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

#define ADD_TO_QUERY_LIST(toMoveVec, thePos) addToQueryList(params, &sentinel, v2_plus(thePos, toMoveVec), params->longTermArena, boardArray, params->boardWidth);
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
    assert(info.count <= shape->count);

    return info;
}

bool shapeStillConnected(FitrisShape *shape, int currentHotIndex, V2 boardPosAt, FrameParams *params) {
    bool result = true;

    for(int i = 0; i < shape->count; ++i) {
        V2 pos = shape->blocks[i].pos;
        if(boardPosAt.x == pos.x && boardPosAt.y == pos.y) {
            result = false;
            break;
        }

        BoardState state = getBoardState(params, boardPosAt);
        if(state != BOARD_NULL) {
            result = false;
            break;
        }
    }
    if(result) {
        V2 oldPos = shape->blocks[currentHotIndex].pos;

        BoardValue *oldVal = getBoardValue(params, oldPos);
        assert(oldVal->state == BOARD_SHAPE);

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
            BoardValue *newVal = getBoardValue(params, boardPosAt);
            assert(newVal->state == BOARD_NULL);
            newVal->state = BOARD_SHAPE;
            ////   

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
            //islandInfo.count != mainIslandInfo.count
            if(!found) {
                result = false;
            }
            //set the state back to being a shape. 
            newVal->state = BOARD_NULL;
            oldVal->state = BOARD_SHAPE;
        }
    }
    
    return result;
}

void resetMouseUI(FrameParams *params) {
    if(params->currentHotIndex >= 0) {
        params->letGo = true;
    }

    params->currentHotIndex = -1; //reset hot ui   

}

void updateAndRenderShape(FitrisShape *shape, V3 cameraPos, V2 resolution, V2 screenDim, Matrix4 metresToPixels, FrameParams *params) {
    assert(!params->wasHitByExplosive);
    assert(!params->createShape);
    bool areRearranging = (params->currentHotIndex >= 0);
#if CAN_MOVE_WITH_ARROW_KEYS
    if(wasPressed(gameButtons, BUTTON_LEFT) && !areRearranging) {
        moveShape(shape, params, MOVE_LEFT);
    }
    if(wasPressed(gameButtons, BUTTON_RIGHT) && !areRearranging) {
        moveShape(shape, params, MOVE_RIGHT);
    }
    if(wasPressed(gameButtons, BUTTON_DOWN) && !areRearranging) {
        if(moveShape(shape, params, MOVE_DOWN)) {
            params->moveTimer.value = 0;
        }
    }
#endif

    if(wasReleased(gameButtons, BUTTON_LEFT_MOUSE)) {
        resetMouseUI(params);
    }

    bool turnSolid = false;
    bool isHoldingShape = params->currentHotIndex >= 0;
    params->moveTime = params->dt;
    if(isHoldingShape) { 
        params->accumHoldTime += params->dt;
        params->moveTime = 0.0f;
        assert(!params->letGo);
    } 

    if(params->letGo) {
        assert(!isHoldingShape);
        params->moveTime = params->accumHoldTime;
        params->accumHoldTime = 0;
        params->letGo = false;
    }

    TimerReturnInfo timerInfo = updateTimer(&params->moveTimer, params->moveTime);
    if(timerInfo.finished) {
        turnTimerOn(&params->moveTimer);
        if(!moveShape(shape, params, MOVE_DOWN)) {
            turnSolid = true;
        }
    }
    params->wasHitByExplosive = false;
    if(turnSolid) {// || params->wasHitByExplosive
        solidfyShape(shape, params);
        params->createShape = true; 
        params->wasHitByExplosive = false;   
        resetMouseUI(params);
    } else {
        
        int hotBlockIndex = -1;
        for(int i = 0; i < shape->count; ++i) {
            V2 pos = shape->blocks[i].pos;
            
            RenderInfo renderInfo = calculateRenderInfo(v3(pos.x, pos.y, -1), v3(1, 1, 1), cameraPos, metresToPixels);

            Rect2f blockBounds = rect2fCenterDimV2(renderInfo.transformPos.xy, renderInfo.transformDim.xy);
            
            V4 color = COLOR_WHITE;
            
            if(inBounds(params->keyStates->mouseP_yUp, blockBounds, BOUNDS_RECT)) {
                hotBlockIndex = i;
                if(params->currentHotIndex < 0) {
                    color = COLOR_YELLOW;
                }
            }
            if(params->currentHotIndex == i) {
                assert(isDown(gameButtons, BUTTON_LEFT_MOUSE));
                color = COLOR_GREEN;
            }
            BoardValue *val = getBoardValue(params, pos);
            assert(val);
            val->color = color;
        }

        if(wasPressed(gameButtons, BUTTON_LEFT_MOUSE) && hotBlockIndex >= 0) {
            params->currentHotIndex = hotBlockIndex;

        }
        
        if(params->currentHotIndex >= 0) {
            //We are holding onto a block
            V2 pos = params->keyStates->mouseP_yUp;
            
            V2 boardPosAt = V4MultMat4(v4(pos.x, pos.y, 1, 1), params->pixelsToMeters).xy;
            boardPosAt.x += cameraPos.x;
            boardPosAt.y += cameraPos.y;
            boardPosAt.x = (int)(clamp(0, boardPosAt.x, params->boardWidth - 1) + 0.5f);
            boardPosAt.y = (int)(clamp(0, boardPosAt.y, params->boardHeight -1) + 0.5f);

            if(shapeStillConnected(shape, params->currentHotIndex, boardPosAt, params)) {
                V2 oldPos = shape->blocks[params->currentHotIndex].pos;
                V2 newPos = boardPosAt;
                assert(getBoardState(params, oldPos) == BOARD_SHAPE);
                assert(getBoardState(params, newPos) == BOARD_NULL);
                setBoardState(params, oldPos, BOARD_NULL, BOARD_VAL_NULL);    
                setBoardState(params, newPos, BOARD_SHAPE, shape->blocks[params->currentHotIndex].type);    
                shape->blocks[params->currentHotIndex].pos = newPos;
                assert(getBoardState(params, newPos) == BOARD_SHAPE);
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
                } else if(type == BOARD_VAL_ALWAYS || type == BOARD_VAL_DYNAMIC) {
                    tex = params->woodTex;
                    assert(tex);
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

void initBoard(FrameParams *params, LevelType levelType) {
    params->extraShapeCount = 0;
    params->experiencePoints = 0;
    params->currentHotIndex = -1;
    params->maxExperiencePoints = 0;
    params->glowingLinesCount = 0;

    params->createShape = true;   
    params->moveTimer.value = 0;
    if(params->currentLevelType != levelType) { //not retrying the level
        params->levelNameTimer = initTimer(1.0f);
    }
    params->currentLevelType = levelType;
    createLevelFromFile(params, levelType);
    params->lifePoints = params->lifePointsMax;
    
}

typedef struct {
    LevelType levelType;
    FrameParams *params;
} TransitionDataLevel;

typedef struct {
    LevelType levelType;
    FrameParams *params;
    MenuInfo *info;
    GameMode lastMode;
    GameMode newMode;
} TransitionDataStartOrEndGame;



void transitionCallbackForLevel(void *data_) {
    TransitionDataLevel *trans = (TransitionDataLevel *)data_;
    FrameParams *params = trans->params;

    initBoard(params, trans->levelType);
} 

void transitionCallbackForStartOrEndGame(void *data_) {
    TransitionDataStartOrEndGame *trans = (TransitionDataStartOrEndGame *)data_;
    FrameParams *params = trans->params;

    initBoard(params, trans->levelType);

    trans->info->gameMode = trans->newMode;
    trans->info->lastMode = trans->lastMode;
    trans->info->menuCursorAt = 0;
} 

void setLevelTransition(FrameParams *params, LevelType levelType) {
    TransitionDataLevel *data = (TransitionDataLevel *)calloc(sizeof(TransitionDataLevel), 1);
    data->levelType = levelType;
    data->params = params;
    setTransition_(&params->transitionState, transitionCallbackForLevel, data);
}

void setStartOrEndGameTransition(FrameParams *params, LevelType levelType, GameMode newMode) {
    TransitionDataStartOrEndGame *data = (TransitionDataStartOrEndGame *)calloc(sizeof(TransitionDataStartOrEndGame), 1);
    data->params = params;
    data->info = &params->menuInfo;
    data->levelType = levelType;
    data->lastMode = params->menuInfo.gameMode;
    data->newMode = newMode;
    setTransition_(&params->transitionState, transitionCallbackForStartOrEndGame, data);
}

void removeWinLine(FrameParams *params, int lineToRemove) {
    bool found = false;
    for(int winLineIndex = 0; winLineIndex < params->glowingLinesCount; winLineIndex++) {
        int line_yAt = params->glowingLines[winLineIndex];
        if(line_yAt == lineToRemove) {
            //NOTE: Writing the last one in the array over the one we are removing. 
            params->glowingLines[winLineIndex] = params->glowingLines[--params->glowingLinesCount];
            found = true;
            break;
        }
    }
    assert(found);
}

void updateBoardWinState(FrameParams *params) {
    for(int winLineIndex = 0; winLineIndex < params->glowingLinesCount; ) {
        bool increment = true;
        int boardY = params->glowingLines[winLineIndex];
        bool win = true;
        for(int boardX = 0; boardX < params->boardWidth; ++boardX) {
            BoardValue *boardVal = &params->board[boardY*params->boardWidth + boardX];
            if(boardVal->state == BOARD_NULL || boardVal->state == BOARD_SHAPE || boardVal->type != BOARD_VAL_OLD) {
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
                    setBoardState(params, v2(boardX, boardY), BOARD_NULL, BOARD_VAL_OLD);
                }
            }
            // playSound(params->soundArena, params->successSound, 0, AUDIO_FOREGROUND);
            removeWinLine(params, boardY);
            increment = false;
        }

        if(increment) {
            winLineIndex++;
        }
    }

    if(params->glowingLinesCount == 0) {
        int levelAsInt = (int)params->currentLevelType;
        levelAsInt++;
        LevelType nextLevel = (LevelType)levelAsInt;
        if(nextLevel < LEVEL_COUNT) {
            setLevelTransition(params, nextLevel);
        } else {
            //finished the game -> go back to main menu
            setStartOrEndGameTransition(params, LEVEL_0, MENU_MODE);
        }
        
    }
}

void updateWindmillSide(FrameParams *params, ExtraShape *shape) {
    bool repeat = true;
    while(repeat) { //this is for when we get blocked 
        repeat = false;
        int beginCount = shape->count;
        bool beginOut = shape->isOut;
        BoardState stateToSet;
        
        int addend = 0;
        if(shape->isOut) {
            addend = 1;
            stateToSet = BOARD_STATIC;
        } else {
            stateToSet = BOARD_NULL;
            addend = -1;
        }
        bool justFlipped = shape->justFlipped;
        if(!shape->justFlipped) {
            shape->count += addend;
        } else {
            shape->justFlipped = false;
        }

        V2 shift = v2(shape->growDir.x*shape->count, shape->growDir.y*shape->count); 
        V2 newPos = v2_plus(shape->pos, shift);

        BoardState toBoardState = getBoardState(params, newPos);
        bool settingBlock = (toBoardState == BOARD_NULL && stateToSet == BOARD_STATIC);
        bool isInBounds = inBoardBounds(params, newPos);
        bool blocked = (toBoardState != BOARD_NULL && stateToSet == BOARD_STATIC);
        if(blocked || !isInBounds) {
            assert(shape->isOut);
            shape->isOut = false;
            shape->justFlipped = true;

            if(shape->count == 0) {
                assert(stateToSet != BOARD_NULL);
                shape->isOut = true;
            } else {
                repeat = true;
                shape->count--;
            }
        } else {
            assert(isInBounds);
            if(settingBlock || stateToSet == BOARD_NULL) {
                if(stateToSet == BOARD_NULL) { assert(toBoardState == BOARD_STATIC); }
                setBoardState(params, newPos, stateToSet, BOARD_VAL_DYNAMIC);
            }
        }

        if(shape->count == (shape->max) && !justFlipped) {
            shape->isOut = false;
            shape->justFlipped = true;
        }
        if(shape->count == 0 && !justFlipped) {
            assert(!shape->isOut);
            shape->isOut = true;
            shape->justFlipped = true;
            for(int i = 0; i < shape->perpSize; ++i) {
                shape->growDir = perp(shape->growDir);
            }
            shape->lagTimer.period = shape->lagPeriod; //we change from the begin period to the lag period
            if(shape->lagTimer.period != 0.0f) {
                shape->active = false; //we lag for a bit. 
            }
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

    float barHeight = 0.4f;
    float ratioXp = clamp01((float)params->experiencePoints / (float)params->maxExperiencePoints);
    float startXp = -0.5f; //move back half a square
    float halfXp = 0.5f*params->boardWidth;
    float xpWidth = ratioXp*params->boardWidth;
    RenderInfo renderInfo = calculateRenderInfo(v3(startXp + 0.5f*xpWidth, -2*barHeight, -2), v3_scale(1, v3(xpWidth, barHeight, 1)), params->cameraPos, params->metresToPixels);
    renderDrawRectCenterDim(renderInfo.pos, renderInfo.dim.xy, COLOR_GREEN, 0, mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 

    renderInfo = calculateRenderInfo(v3(startXp + halfXp, -2*barHeight, -2), v3_scale(1, v3(params->boardWidth, barHeight, 1)), params->cameraPos, params->metresToPixels);
    renderDrawRectOutlineCenterDim(renderInfo.pos, renderInfo.dim.xy, COLOR_BLACK, 0, mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
}

//NOTE: this is for indicating which lines will win the level. 
bool checkIsWinLine(FrameParams *params, int lineCheckAt) {
    bool result = false;
    for(int winLineIndex = 0; winLineIndex < params->glowingLinesCount; winLineIndex++) {
        int line_yAt = params->glowingLines[winLineIndex];
        if(line_yAt == lineCheckAt) {
            result  = true;
            break;
        }
    }
    return result;
}

void gameUpdateAndRender(void *params_) {
    FrameParams *params = (FrameParams *)params_;
    V2 screenDim = *params->screenDim;
    V2 resolution = *params->resolution;
    V2 middleP = v2_scale(0.5f, resolution);
    //ceneter the camera
    params->cameraPos.xy = v2_scale(0.5f, v2((float)params->boardWidth - 1, (float)params->boardHeight - 1));

    //make this platform independent
    easyOS_beginFrame(resolution);
    //////CLEAR BUFFERS
    // 
    clearBufferAndBind(params->backbufferId, COLOR_BLACK);
    clearBufferAndBind(params->mainFrameBuffer.bufferId, COLOR_PINK);
    renderEnableDepthTest(&globalRenderGroup);
    renderTextureCentreDim(params->bgTex, v2ToV3(v2(0, 0), -5), resolution, COLOR_WHITE, 0, mat4(), mat4(), OrthoMatrixToScreen(resolution.x, resolution.y));                    

    // drawAndUpdateParticleSystem(&params->particleSystem, params->dt, v3(0, 0, -4), v3(0, 0 ,0), params->cameraPos, params->metresToPixels, resolution);
    
    V2 mouseP = params->keyStates->mouseP;
    GameMode currentGameMode = drawMenu(&params->menuInfo, params->longTermArena, gameButtons, 0, params->successSound, params->moveSound, params->dt, resolution, mouseP);

    bool transitioning = updateTransitions(&params->transitionState, resolution, params->dt);
    Rect2f menuMargin = rect2f(0, 0, resolution.x, resolution.y);

    bool isPlayState = (currentGameMode == PLAY_MODE);

    if(!transitioning && isPlayState) {
        
        //if updating a transition don't update the game logic, just render the game board. 
        bool canDie = params->lifePointsMax > 0;
        bool retryLevel = ((params->lifePoints <= 0) && canDie) || wasPressed(gameButtons, BUTTON_R);
        if(params->createShape || retryLevel) {
            params->currentShape.count = 0;
            for (int i = 0; i < 4 && !retryLevel; ++i) {
                int xAt = i % params->boardWidth;
                int yAt = (params->boardHeight - 1) - (i / params->boardWidth);
                assert(i == params->currentShape.count);
                FitrisBlock *block = &params->currentShape.blocks[params->currentShape.count++];
                block->pos = v2(xAt, yAt);
                block->type = BOARD_VAL_SHAPES[i];
                block->id = i;

                V2 pos = v2(xAt, yAt);
                if(getBoardState(params, pos) != BOARD_NULL) {
                    //at the top of the board
                    retryLevel = true;
                    break;
                    //
                } else {
                    setBoardState(params, pos, BOARD_SHAPE, params->currentShape.blocks[i].type);    
                }
            }
            if(retryLevel) {
                setLevelTransition(params, params->currentLevelType);
            }

            params->moveTimer.value = 0;
            params->createShape = false;
        }


        updateAndRenderShape(&params->currentShape, params->cameraPos, resolution, screenDim, params->metresToPixels, params);

        bool isHoldingShape = (params->currentHotIndex >= 0);
        for(int extraIndex = 0; extraIndex < params->extraShapeCount; ++extraIndex) {
            ExtraShape *extraShape = params->extraShapes + extraIndex;

            float tUpdate = (extraShape->timeAffected) ? params->moveTime : params->dt;
            float extraTime = params->dt;
            if(!extraShape->active) { 
                TimerReturnInfo lagInfo = updateTimer(&extraShape->lagTimer, tUpdate);
                if(lagInfo.finished) {
                    turnTimerOn(&extraShape->lagTimer);
                    extraShape->active = true; 
                    extraTime = lagInfo.residue;
                }
            } 

            if(extraShape->active) {
                TimerReturnInfo info = updateTimer(&extraShape->timer, tUpdate);
                if(info.finished) {
                    turnTimerOn(&extraShape->timer);
                    updateWindmillSide(params, extraShape);
                }
            }
        }
        
        updateBoardWinState(params);
    }

    //Stil render when we are in a transition
    if(isPlayState) {

        if(isOn(&params->levelNameTimer)) {
            TimerReturnInfo nameTimeInfo = updateTimer(&params->levelNameTimer, params->dt);
            V4 levelNameFontColor = smoothStep00V4(COLOR_NULL, nameTimeInfo.canonicalVal, COLOR_BLACK);
            float levelNameFontSize = 1.0f;
            char *title = params->levelNames[params->currentLevelType];
            float xFontAt = (resolution.x/2) - (getBounds(title, menuMargin, params->font, levelNameFontSize, resolution).x / 2);
            outputText(params->font, xFontAt, 0.5f*resolution.y, -1, resolution, title, menuMargin, levelNameFontColor, levelNameFontSize, true);
        }
        
        outputText(params->font, 10, 40, -1, resolution, "Press R to reset", menuMargin, COLOR_BLACK, 0.5f, true);

        renderXPBarAndHearts(params, resolution);
        for(int boardY = 0; boardY < params->boardHeight; ++boardY) {
            bool isWinLine = checkIsWinLine(params, boardY);
            for(int boardX = 0; boardX < params->boardWidth; ++boardX) {
                RenderInfo bgRenderInfo = calculateRenderInfo(v3(boardX, boardY, -3), v3(1, 1, 1), params->cameraPos, params->metresToPixels);
                BoardValue *boardVal = &params->board[boardY*params->boardWidth + boardX];
                renderTextureCentreDim(params->boarderTex, bgRenderInfo.pos, bgRenderInfo.dim.xy, COLOR_WHITE, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), bgRenderInfo.pvm));            
                
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
                if(isWinLine) {
                    //TODO: make the glowing effect better
                    RenderInfo winBlockRenderInfo = calculateRenderInfo(v3(boardX, boardY, -2.5f), v3(1, 1, 1), params->cameraPos, params->metresToPixels);
                    // V4 winColor = smoothStep01V4(v4(0, 1, 0, 0), tAt/5.0f, v4(0, 1, 0, 0.2f));
                    V4 winColor = v4(0, 1, 0, 0.2f);
                    renderDrawRectCenterDim(winBlockRenderInfo.pos, winBlockRenderInfo.dim.xy, winColor, 0, mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), winBlockRenderInfo.pvm));            
                }
            }   
        }
    } else if(currentGameMode == OVERWORLD_MODE) {
        float xAt = 0;
        float yAt = 50;
        float coint_xAt = 0;
        float coint_yAt = 0;
        
        float fontSize = 0.7f;
        float lowerY = 0.8f*resolution.y;
        float nameY = 0.9f*resolution.y;
        for(int levelIndex = 1; levelIndex < LEVEL_COUNT; ++levelIndex) {
            char *title = LevelTypeStrings[levelIndex];
            // RenderInfo levelBlockRenderInfo = calculateRenderInfo(v3(coint_xAt, coint_yAt, -1), v3(1, 1, 1), params->cameraPos, params->metresToPixels);
            // renderTextureCentreDim(params->coinTex, levelBlockRenderInfo.pos, levelBlockRenderInfo.dim.xy, COLOR_WHITE, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), levelBlockRenderInfo.pvm));            
            Rect2f outputDim = outputText(params->font, xAt, yAt, -1, resolution, title, menuMargin, COLOR_WHITE, fontSize, false);
            
            V4 color = COLOR_BLACK;
            if(inBounds(mouseP, outputDim, BOUNDS_RECT)) {
                // info->menuCursorAt = menuIndex;
                color = COLOR_GREEN;
                char *levelName = params->levelNames[levelIndex];
                outputText(params->font, 0, nameY, -1, resolution, levelName, menuMargin, COLOR_BLUE, fontSize, true);

                if(wasPressed(gameButtons, BUTTON_LEFT_MOUSE)) {
                    setStartOrEndGameTransition(params, (LevelType)levelIndex, PLAY_MODE);
                }
            }

            V2 wordDim = getDim(outputDim);
            outputText(params->font, xAt, yAt, -1, resolution, title, menuMargin, color, fontSize, true);
            if(wordDim.y + yAt >= lowerY) {
                yAt = 0;
                xAt += wordDim.x + 50;
            }

            yAt += 50;//fontSize*params->font->fontHeight;

        }
    }
    

    // outputText(params->font, 0, 400, -1, resolution, "hey˙  हिन् दी df ©˙ \n∆˚ ", rect2f(0, 0, resolution.x, resolution.y), COLOR_BLACK, 1, true);
   drawRenderGroup(&globalRenderGroup);
   
   easyOS_endFrame(resolution, screenDim, &params->dt, params->windowHandle, params->mainFrameBuffer.bufferId, params->backbufferId, params->renderbufferId, &params->lastTime, 1.0f / 60.0f);
}

int main(int argc, char *args[]) {
	
	V2 screenDim = {}; //init in create app function
	V2 resolution = v2(1280, 720); //this could be variable -> passed in by the app etc. like unity window 

  OSAppInfo appInfo = easyOS_createApp("Fitris", &screenDim);
  assert(appInfo.valid);
    
  if(appInfo.valid) {
    AppSetupInfo setupInfo = easyOS_setupApp(resolution, RESOURCE_PATH_EXTENSION);

    float dt = 1.0f / min((float)setupInfo.refresh_rate, 60.0f); //use monitor refresh rate 
    float idealFrameTime = 1.0f / 60.0f;

    ////INIT FONTS
    char *fontName = concat(globalExeBasePath, "/fonts/Khand-Regular.ttf");//Roboto-Regular.ttf");/);
    Font mainFont = initFont(fontName, 128);
    ///

    Arena soundArena = createArena(Megabytes(200));
    Arena longTermArena = createArena(Megabytes(200));

    loadAndAddImagesToAssets("img/");
    loadAndAddSoundsToAssets("sounds/", &setupInfo.audioSpec);

    Texture *stoneTex = findTextureAsset("elementStone023.png");
    Texture *woodTex = findTextureAsset("elementWood022.png");
    Texture *bgTex = findTextureAsset("blue_grass.png");
    Texture *metalTex = findTextureAsset("elementMetal023.png");
    Texture *explosiveTex = findTextureAsset("elementExplosive049.png");
    Texture *boarderTex = findTextureAsset("elementMetal030.png");
    Texture *heartFullTex = findTextureAsset("hud_heartFull.png");
    Texture *heartEmptyTex = findTextureAsset("hud_heartEmpty.png");
    Texture *coinTex = findTextureAsset("coinGold.png");
    

    assert(metalTex);
    bool running = true;
      
    FrameParams params = {};
    params.solidfyShapeSound = findSoundAsset("slate_sound.wav");
    params.successSound = findSoundAsset("Success2.wav");
    params.explosiveSound = findSoundAsset("explosion.wav");
    

    params.alienTex[0] = findTextureAsset("alienGreen.png");
    params.alienTex[1] = findTextureAsset("alienYellow.png");
    params.alienTex[2] = findTextureAsset("alienBlue.png");
    params.alienTex[3] = findTextureAsset("alienPink.png");

#if 0 //particle system in background. Was to distracting. 
    particle_system_settings particleSet = InitParticlesSettings(PARTICLE_SYS_DEFAULT);
    // pushParticleBitmap(&particleSet, findTextureAsset("leaf1.png"), "stone Tex");

    particleSet.Loop = true;
    //particleSet.offsetP = v3(0.000000, 0.000000, 0.200000);
    particleSet.bitmapScale = 1.0f;
    particleSet.posBias = rect2f(-10.000000, 10, 10.000000, 10);
    particleSet.VelBias = rect2f(0.000000, -2.000000, 0.000000, -4.000000);
    particleSet.angleBias = v2(0.000000, 6.280000);
    particleSet.angleForce = v2(-3.000000, 3.000000);
    particleSet.collidesWithFloor = false;
    particleSet.pressureAffected = false;


    InitParticleSystem(&params.particleSystem, &particleSet);
    params.particleSystem.viewType = ORTHO_MATRIX;
    setParticleLifeSpan(&params.particleSystem, 10.0f);
    Reactivate(&params.particleSystem);
    assert(params.particleSystem.Active);
#endif

    params.moveSound = findSoundAsset("menuSound.wav");
#if 1 //background sound
    params.backgroundSound = findSoundAsset("Fitris_Soundtrack.wav");//

    //Play and repeat background sound
    PlayingSound *sound = playSound(&soundArena, params.backgroundSound, 0, AUDIO_FOREGROUND);
    sound->nextSound = sound;
    //
#endif

    params.soundArena = &soundArena;
    params.longTermArena = &longTermArena;
    params.dt = dt;
    params.windowHandle = appInfo.windowHandle;
    params.backbufferId = appInfo.frameBackBufferId;
    params.renderbufferId = appInfo.renderBackBufferId;
    params.mainFrameBuffer = createFrameBuffer(resolution.x, resolution.y, FRAMEBUFFER_DEPTH | FRAMEBUFFER_STENCIL);
    params.resolution = &resolution;
    params.screenDim = &screenDim;
    params.metresToPixels = setupInfo.metresToPixels;
    params.pixelsToMeters = setupInfo.pixelsToMeters;
    AppKeyStates keyStates = {};
    params.keyStates = &keyStates;
    params.font = &mainFont;
    params.screenRelativeSize = setupInfo.screenRelativeSize;

    params.bgTex = bgTex;
    initBoard(&params, START_LEVEL);
    loadLevelNames(&params);
    
    params.moveTimer = initTimer(MOVE_INTERVAL);
    params.woodTex = woodTex;
    params.stoneTex = stoneTex;
    params.metalTex = metalTex;
    params.explosiveTex = explosiveTex;
    params.boarderTex = boarderTex;
    params.heartFullTex = heartFullTex;
    params.heartEmptyTex = heartEmptyTex;
    params.coinTex = coinTex;
    
    params.lastTime = SDL_GetTicks();
    
    params.cameraPos = v3(0, 0, 0);

    TransitionState transState = {};
    transState.transitionSound = findSoundAsset("click.wav");
    transState.soundArena = &soundArena;
    transState.longTermArena = &longTermArena;

    params.transitionState = transState;

    MenuInfo menuInfo = {};
    menuInfo.font = &mainFont;
    menuInfo.windowHandle = appInfo.windowHandle; 
    menuInfo.running = &running;
    menuInfo.lastMode = menuInfo.gameMode = START_MENU_MODE; //from the defines file
    menuInfo.transitionState = &params.transitionState;
    params.menuInfo = menuInfo;
        
    //

#if !DESKTOP    
    if(SDL_iPhoneSetAnimationCallback(appInfo.windowHandle, 1, gameUpdateAndRender, &params) < 0) {
    	assert(!"falid to set");
    }
#endif
    // if(SDL_AddEventWatch(EventFilter, NULL) < 0) {
    // 	assert(!"falid to set");
    // }
    
    while(running) {
        
    	keyStates = easyOS_processKeyStates(resolution, &screenDim, &running);
#if DESKTOP
      gameUpdateAndRender(&params);
#endif
    }
    easyOS_endProgram(&appInfo);
	}
    return 0;
}
