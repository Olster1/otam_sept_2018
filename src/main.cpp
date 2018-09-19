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
//NOTE : a global variable names index seems to be a defined function by sdl.h Newer Gcc picks this up as an error if wehere using it. 
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

#define MAX_SHAPE_COUNT 16
typedef struct {
    V2 coords[MAX_SHAPE_COUNT];
    int count;
    bool valid;

    Timer moveTimer;
} FitrisShape;

typedef enum {
    SHAPE_WINDMILL,
} ExtraShapeType;

typedef struct {
    ExtraShapeType type;
    V2 pos;

    Timer timer;

    bool onX; //on x or on y
    bool isOut; //going out or in

    int count;

    int xMax;
    int yMax;
    
} ExtraShape;

typedef enum {
    BOARD_VAL_NULL,
    BOARD_VAL_OLD,
    BOARD_VAL_ALWAYS,
    BOARD_VAL_TRANSIENT, //this isn't used for anything, just to make it so we aren't using the other ones. 
} BoardValType;

typedef struct {
    BoardValType type;
    BoardState state;
    BoardState prevState;

    V4 color;

    Timer fadeTimer;
} BoardValue;

typedef enum {
    LEVEL_0,
    LEVEL_1, 
    LEVEL_2, 
    LEVEL_3, 
    LEVEL_4, 
} LevelType;

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

    WavFile *solidfyShapeSound;
    WavFile *moveSound;
    WavFile *backgroundSound;
    WavFile *successSound;
    WavFile *explosiveSound;

    TransitionState transitionState;

    int lifePoints;
    int lifePointsMax;
    bool wasHitByExplosive; 

    int extraShapeCount;
    ExtraShape extraShapes[32];

    bool createShape;

    int currentBlockCount;
    LevelType currentLevelType;

    Timer moveTimer;

    int currentHotIndex;

    MenuInfo menuInfo;
    
    int experiencePoints;

    particle_system particleSystem;

    float slowTimeFactor;

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
        val->state = state;
        val->type = type;
        val->fadeTimer = initTimer(FADE_TIMER_INTERVAL);
    } else {
        assert(!"invalid code path");
    }
}

void createLevel(FrameParams *params, int blockCount, LevelType levelType) {
    if(levelType == LEVEL_4) {
        assert(params->extraShapeCount < arrayCount(params->extraShapes));
        ExtraShape *shape = params->extraShapes + params->extraShapeCount++;
        shape->type = SHAPE_WINDMILL;

        shape->pos = v2(2, 2);

        setBoardState(params, shape->pos, BOARD_STATIC, BOARD_VAL_ALWAYS);    
        shape->timer = initTimer(0.5f);
        shape->isOut = true;
        shape->xMax = 3;
        shape->yMax = 3;

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
        V2 pos = shape->coords[i];
        BoardState state = getBoardState(params, v2_plus(pos, moveVec));
        if(!(state == BOARD_NULL || state == BOARD_SHAPE || state == BOARD_EXPLOSIVE)) {
            valid = false;
            break;
        }
        if(shape->coords[i].x < leftMostPos) {
            leftMostPos = shape->coords[i].x;
        }
        if(shape->coords[i].x > rightMostPos) {
            rightMostPos = shape->coords[i].x;
        }
        if(shape->coords[i].y < bottomMostPos) {
            bottomMostPos = shape->coords[i].y;
        }
    }

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
    return result;
}
bool isInShape(FitrisShape *shape, V2 pos) {
    bool result = false;
    for(int i = 0; i < shape->count; ++i) {
      V2 shapePos = shape->coords[i];
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
      V2 shapePos = shape->coords[i];
      if(i != index && pos.x == shapePos.x && pos.y == shapePos.y) {
        result.result = true;
        result.index = i;
        assert(i != index);
        break;
      }
   }
   return result;
}

bool moveShape(FitrisShape *shape, FrameParams *params, MoveType moveType) {
    bool result = canShapeMove(shape, params, moveType);
    if(result) {
        V2 moveVec = getMoveVec(moveType);

        assert(!params->wasHitByExplosive);
       // CHECK FOR EXPLOSIVES HIT
        int indexesHitCount = 0;
        int indexesHit[MAX_SHAPE_COUNT] = {};
        for(int i = 0; i < shape->count; ++i) {
          V2 oldPos = shape->coords[i];
          V2 newPos = v2_plus(oldPos, moveVec);
          BoardState state = getBoardState(params, newPos);
          if(state == BOARD_EXPLOSIVE) {
            params->lifePoints--;
            params->wasHitByExplosive = true;
            playSound(params->soundArena, params->explosiveSound, 0, AUDIO_FOREGROUND);
            //remove from shapea
            assert(indexesHitCount < arrayCount(indexesHit));
            indexesHit[indexesHitCount++] = i;
            setBoardState(params, oldPos, BOARD_NULL, BOARD_VAL_TRANSIENT);    
            setBoardState(params, newPos, BOARD_NULL, BOARD_VAL_TRANSIENT);    
          }
        }

        for(int hitIndex = 0; hitIndex < indexesHitCount; ++hitIndex) {
            int indexAt = indexesHit[hitIndex];
            shape->coords[indexAt] = shape->coords[--shape->count];
        }

       for(int i = 0; i < shape->count; ++i) {
            V2 oldPos = shape->coords[i];
            V2 newPos = v2_plus(oldPos, moveVec);
           
            assert(getBoardState(params, oldPos) == BOARD_SHAPE);
            BoardState newPosState = getBoardState(params, newPos);
            assert(newPosState == BOARD_SHAPE || newPosState == BOARD_NULL);

            QueryShapeInfo info = isRepeatedInShape(shape, oldPos, i);
            if(!info.result) { //dind't just get set by the block in shape before. 
                setBoardState(params, oldPos, BOARD_NULL, BOARD_VAL_TRANSIENT);    
            }
            setBoardState(params, newPos, BOARD_SHAPE, BOARD_VAL_TRANSIENT);    
            shape->coords[i] = newPos;
        }
        playSound(params->soundArena, params->moveSound, 0, AUDIO_FOREGROUND);
    }
    return result;
}

void solidfyShape(FitrisShape *shape, FrameParams *params) {
    for(int i = 0; i < shape->count; ++i) {
        V2 pos = shape->coords[i];
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
        V2 pos = shape->coords[i];
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
        V2 oldPos = shape->coords[currentHotIndex];

        BoardValue *oldVal = getBoardValue(params, oldPos);
        assert(oldVal->state == BOARD_SHAPE);

        IslandInfo mainIslandInfo = getShapeIslandCount(shape, oldPos, params);
        assert(mainIslandInfo.count >= 1);
        if(mainIslandInfo.count <= 1) {
            result = false;
        } else {
            V2 idPos = mainIslandInfo.poses[1]; //won't be that starting pos. 
            //temporaialy set the board state to where the shape was to be null, so this can act as a bridge in the flood fill
            oldVal->state = BOARD_NULL;

            BoardValue *newVal = getBoardValue(params, boardPosAt);
            assert(newVal->state == BOARD_NULL);
            newVal->state = BOARD_SHAPE;
            ////   

            IslandInfo islandInfo = getShapeIslandCount(shape, boardPosAt, params);

            bool found = false;
            for(int index = 0; index < islandInfo.count; ++index) {
                V2 srchPos = islandInfo.poses[index];
                if(srchPos.x == idPos.x && srchPos.y == idPos.y) {
                    found = true;
                    break;
                }
            }

            if(islandInfo.count != mainIslandInfo.count || !found) {
                result = false;
            }
            //set the state back to being a shape. 
            oldVal->state = BOARD_SHAPE;
            newVal->state = BOARD_NULL;
        }
    }
    
    return result;
}

void resetMouseUI(FrameParams *params) {
    params->currentHotIndex = -1; //reset hot ui   
    params->slowTimeFactor = 1; 
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
    TimerReturnInfo timerInfo = updateTimer(&params->moveTimer, params->slowTimeFactor*params->dt);
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
            V2 *pos = shape->coords +i;
            
            RenderInfo renderInfo = calculateRenderInfo(v3(pos->x, pos->y, -1), v3(1, 1, 1), cameraPos, metresToPixels);

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
            BoardValue *val = getBoardValue(params, *pos);
            assert(val);
            val->color = color;
        }

        if(wasPressed(gameButtons, BUTTON_LEFT_MOUSE) && hotBlockIndex >= 0) {
            params->currentHotIndex = hotBlockIndex;
            params->slowTimeFactor = 0.0f;  //don't move block if we are rearranging
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
                V2 oldPos = shape->coords[params->currentHotIndex];
                V2 newPos = boardPosAt;
                assert(getBoardState(params, oldPos) == BOARD_SHAPE);
                setBoardState(params, oldPos, BOARD_NULL, BOARD_VAL_TRANSIENT);    
                setBoardState(params, newPos, BOARD_SHAPE, BOARD_VAL_TRANSIENT);    
                shape->coords[params->currentHotIndex] = newPos;
            }
        }

    }
}

Texture *getBoardTex(BoardValue *boardVal, BoardState boardState, FrameParams *params) {
    Texture *tex = 0;
    if(boardState != BOARD_NULL) {
        switch(boardState) {
            case BOARD_STATIC: {
                if(boardVal->type == BOARD_VAL_OLD) {
                    tex = params->metalTex;
                    assert(tex);
                } else if(boardVal->type == BOARD_VAL_ALWAYS) {
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
                tex = params->stoneTex;
            } break;
            default: {
                assert(!"not handled");
            }
        }
        assert(tex);
    }
    return tex;
}

void updateBoardWinState(FrameParams *params) {
    int winCount = 0;
    for(int boardY = 0; boardY < params->boardHeight; ++boardY) {
        bool win = true;
        for(int boardX = 0; boardX < params->boardWidth; ++boardX) {
            BoardValue *boardVal = &params->board[boardY*params->boardWidth + boardX];
            if(boardVal->state == BOARD_NULL || boardVal->state == BOARD_SHAPE) {
                win = false;
                break;
            } 
            assert(boardVal->state != BOARD_INVALID);
        }
        if(win) {
            winCount++;
            for(int boardX = 0; boardX < params->boardWidth; ++boardX) {
                BoardValue *boardVal = &params->board[boardY*params->boardWidth + boardX];
                if(boardVal->state == BOARD_STATIC && boardVal->type == BOARD_VAL_OLD) {
                    setBoardState(params, v2(boardX, boardY), BOARD_NULL, BOARD_VAL_OLD);
                }
            }
            playSound(params->soundArena, params->successSound, 0, AUDIO_FOREGROUND);
        }
    }
    params->experiencePoints += sqr(winCount)*100;
}

void initBoard(Arena *longTermArena, FrameParams *params, int boardWidth, int boardHeight, LevelType levelType, int blockCount, bool createArray) {
    params->boardWidth = boardWidth;
    params->boardHeight = boardHeight;

    params->lifePoints = params->lifePointsMax;
    if(createArray) {
        params->board = pushArray(longTermArena, params->boardWidth*params->boardHeight, BoardValue);
    } 

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

    createLevel(params, blockCount, levelType);
}

typedef struct {
    int blockCount;
    LevelType levelType;
    FrameParams *params;
} TransitionDataLevel;

void transitionCallbackForLevel(void *data_) {
    TransitionDataLevel *trans = (TransitionDataLevel *)data_;
    FrameParams *params = trans->params;

    initBoard(params->longTermArena, params, params->boardWidth, params->boardHeight, trans->levelType, trans->blockCount, false);
    params->createShape = true;   
    params->lifePoints = params->lifePointsMax;
} 

void setLevelTransition(FrameParams *params,  int blockCount, LevelType levelType) {
    TransitionDataLevel *data = (TransitionDataLevel *)calloc(sizeof(TransitionDataLevel), 1);
    data->blockCount = blockCount;
    data->levelType = levelType;
    data->params = params;
    setTransition_(&params->transitionState, transitionCallbackForLevel, data);
}

void updateWindmillSide(FrameParams *params, V2 pos, int max, int *count_, bool *isOut_, bool *axis_) {
    int count = *count_;
    bool isOut = *isOut_;
    bool axis = *axis_;

    V2 shift = v2(0, 0);
    BoardState stateToSet = BOARD_NULL;
    if(isOut) {
        count++;
        if(axis) { shift = v2(count, 0); } //is xAxis
        if(!axis) { shift = v2(0, count); } //is xyAxis
        stateToSet = BOARD_STATIC;
    } else {
        if(axis) { shift = v2(count, 0); } //is xAxis
        if(!axis) { shift = v2(0, count); } //is xAxis
        count--;
    }

    V2 newPos = v2_plus(pos, shift);

    if(getBoardState(params, newPos) != BOARD_NULL && stateToSet == BOARD_STATIC) {
        isOut = false;
    }
    if(inBoardBounds(params, newPos)) {
        if((getBoardState(params, newPos) == BOARD_NULL && stateToSet == BOARD_STATIC) || stateToSet == BOARD_NULL) {
            setBoardState(params, newPos, stateToSet, BOARD_VAL_ALWAYS);
        }
    } else {
        isOut = false;
        count--;
    }

    if(count == max) {
        isOut = false;
    }
    if(count == 0) {
        axis = !axis;
        isOut = true;
        count = 0;
    }
    *count_ = count;
    *axis_ = axis;
    *isOut_ = isOut;
}

void renderXPBarAndHearts(FrameParams *params, V2 resolution) {
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

    float barHeight = 0.4f;
    float maxExperiencePoints = 500;
    float ratioXp = clamp01(params->experiencePoints / maxExperiencePoints);
    float startXp = -0.5f; //move back half a square
    float halfXp = 0.5f*params->boardWidth;
    float xpWidth = ratioXp*params->boardWidth;
    RenderInfo renderInfo = calculateRenderInfo(v3(startXp + 0.5f*xpWidth, -2*barHeight, -2), v3_scale(1, v3(xpWidth, barHeight, 1)), params->cameraPos, params->metresToPixels);
    renderDrawRectCenterDim(renderInfo.pos, renderInfo.dim.xy, COLOR_GREEN, 0, mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 

    renderInfo = calculateRenderInfo(v3(startXp + halfXp, -2*barHeight, -2), v3_scale(1, v3(params->boardWidth, barHeight, 1)), params->cameraPos, params->metresToPixels);
    renderDrawRectOutlineCenterDim(renderInfo.pos, renderInfo.dim.xy, COLOR_BLACK, 0, mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 
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
    

    bool isPlayState = drawMenu(&params->menuInfo, params->longTermArena, gameButtons, 0, params->successSound, params->moveSound, params->dt, resolution, params->keyStates->mouseP);
    bool transitioning = updateTransitions(&params->transitionState, resolution, params->dt);
    if(!transitioning && isPlayState) {
        //if updating a transition don't update the game logic, just render the game board. 
        if(params->createShape || !params->lifePoints) {
            params->currentShape.count = 0;
            bool retryLevel = !params->lifePoints;
            for (int i = 0; i < 4 && !retryLevel; ++i) {
                int xAt = i % params->boardWidth;
                int yAt = (params->boardHeight - 1) - (i / params->boardWidth);
                params->currentShape.coords[params->currentShape.count++] = v2(xAt, yAt);
                V2 pos = v2(xAt, yAt);
                if(getBoardState(params, pos) != BOARD_NULL) {
                    //at the top of the board
                    retryLevel = true;
                    break;
                    //
                } else {
                    setBoardState(params, pos, BOARD_SHAPE, BOARD_VAL_TRANSIENT);    
                }
            }
            if(retryLevel) {
                setLevelTransition(params, params->currentBlockCount, params->currentLevelType);
            }

            params->moveTimer.value = 0;
            params->createShape = false;
        }

        for(int extraIndex = 0; extraIndex < params->extraShapeCount; ++extraIndex) {
            ExtraShape *extraShape = params->extraShapes + extraIndex;
            switch(extraShape->type) {
                case SHAPE_WINDMILL: {
                    TimerReturnInfo info = updateTimer(&extraShape->timer, params->dt);
                    if(info.finished) {
                        turnTimerOn(&extraShape->timer);
                        if(extraShape->onX) {
                            updateWindmillSide(params, extraShape->pos, extraShape->xMax, &extraShape->count, &extraShape->isOut, &extraShape->onX);
                        } else {
                            updateWindmillSide(params, extraShape->pos, extraShape->yMax, &extraShape->count, &extraShape->isOut, &extraShape->onX);
                        }
                        
                    }
                } break;
            }
        }
        updateAndRenderShape(&params->currentShape, params->cameraPos, resolution, screenDim, params->metresToPixels, params);
        updateBoardWinState(params);
    }

    //Stil render when we are in a transition
    if(isPlayState) {
        renderXPBarAndHearts(params, resolution);
        for(int boardY = 0; boardY < params->boardHeight; ++boardY) {
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

                        Texture *tex = getBoardTex(boardVal, boardVal->prevState, params);
                        if(tex) {
                            renderTextureCentreDim(tex, prevRenderInfo.pos, prevRenderInfo.dim.xy, prevColor, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), prevRenderInfo.pvm));            
                        }    

                        if(timeInfo.finished) {
                            boardVal->prevState = boardVal->state;
                        }
                    }
                        
                    Texture *tex = getBoardTex(boardVal, boardVal->state, params);
                    if(tex) {
                        RenderInfo currentStateRenderInfo = calculateRenderInfo(v3(boardX, boardY, -2), v3(1, 1, 1), params->cameraPos, params->metresToPixels);
                        renderTextureCentreDim(tex, currentStateRenderInfo.pos, currentStateRenderInfo.dim.xy, currentColor, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), currentStateRenderInfo.pvm));            
                    }
                } else {
                    assert(!isOn(&boardVal->fadeTimer));
                }
            }
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
    AppSetupInfo setupInfo = easyOS_setupApp(resolution, "../res/");

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

    assert(metalTex);
    bool running = true;
      
    FrameParams params = {};
    params.solidfyShapeSound = findSoundAsset("slate_sound.wav");
    params.successSound = findSoundAsset("Success2.wav");
    params.explosiveSound = findSoundAsset("explosion.wav");
    params.experiencePoints = 100;

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
#if 0 //background sound
    params.backgroundSound = findSoundAsset("Illusionist Finale.wav");

    //Play and repeat background sound
    PlayingSound *sound = playSound(&soundArena, params.backgroundSound, 0, AUDIO_FOREGROUND);
    sound->nextSound = sound;
    //
#endif

    params.soundArena = &soundArena;
    params.longTermArena = &longTermArena;
    params.dt = dt;
    params.slowTimeFactor = 1.0f;
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
        
    int blockCount = 7;
    initBoard(&longTermArena, &params, BOARD_WIDTH, BOARD_HEIGHT, START_LEVEL, blockCount, true);
    params.currentBlockCount = blockCount;
    params.currentLevelType = START_LEVEL; //this is from the defines file
    params.lifePointsMax = 3;
    params.lifePoints = params.lifePointsMax;
    
    params.createShape = true;
    params.moveTimer = initTimer(1.0f);
    params.woodTex = woodTex;
    params.stoneTex = stoneTex;
    params.metalTex = metalTex;
    params.explosiveTex = explosiveTex;
    params.boarderTex = boarderTex;
    params.heartFullTex = heartFullTex;
    params.heartEmptyTex = heartEmptyTex;
    params.bgTex = bgTex;
    params.lastTime = SDL_GetTicks();
    params.currentHotIndex = -1;

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
}