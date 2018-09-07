typedef void transition_callback(void *data);

typedef struct SceneTransition SceneTransition;
typedef struct SceneTransition {
    transition_callback *callback;
    void *data;

    Timer timer;
    bool direction; //true for in //false for out


    SceneTransition *next;
} SceneTransition;

typedef struct {
    SceneTransition *freeListTransitions;
    SceneTransition *currentTransition;

    WavFile *transitionSound;
    Arena *soundArena;
    Arena *longTermArena;
} TransitionState;

SceneTransition *setTransition_(TransitionState *state, transition_callback *callback, void *data) {
    SceneTransition *trans = state->freeListTransitions;
    if(trans) {
        //pull off free list. 
        state->freeListTransitions = trans->next;
    } else {
        trans = pushStruct(state->longTermArena, SceneTransition);
    }
    playSound(state->soundArena, state->transitionSound, 0, AUDIO_FOREGROUND);

    trans->timer = initTimer(SCENE_TRANSITION_TIME);
    trans->data = data;
    trans->callback = callback;
    trans->direction = true;

    state->currentTransition = trans;

    return trans;
}

bool updateTransitions(TransitionState *transState, V2 resolution, float dt) {
    SceneTransition *trans = transState->currentTransition;
    bool result = false;
    if(trans) {
        result = true;
        TimerReturnInfo timeInfo = updateTimer(&trans->timer, dt);
        float halfScreen = 0.5f*resolution.x;

        //this should really be a smoothstep00 and somehow find out when we reach halfway to do the close interval. This could be another timer. 
        float tValue = timeInfo.canonicalVal;
        if(!trans->direction) {
            tValue = 1.0f - timeInfo.canonicalVal;
        }
        float transWidth = smoothStep01(0, tValue, halfScreen);   
        //These are the black rects to make it look like a shutter opening and closing. 
        Rect2f rect1 = rect2f(0, 0, transWidth, resolution.y);
        Rect2f rect2 = rect2f(resolution.x - transWidth, 0, resolution.x, resolution.y);
        static GLBufferHandles handles = {};
        openGlDrawRect(&handles, rect1, -0.5, COLOR_BLACK, 0, mat4(), 1, OrthoMatrixToScreen(resolution.x, resolution.y, 1));                    
        openGlDrawRect(&handles, rect2, -0.5, COLOR_BLACK, 0, mat4(), 1, OrthoMatrixToScreen(resolution.x, resolution.y, 1));                    

        if(timeInfo.finished) {
            if(trans->direction) {
                assert(trans->data);
                trans->callback(trans->data);
                free(trans->data);

                trans->direction = false;
                turnTimerOn(&trans->timer);
            } else {
                //finished the transition
                trans->next = transState->freeListTransitions;
                transState->freeListTransitions = trans;

                transState->currentTransition = 0;
            }
        }
    
    }
    return result;
}