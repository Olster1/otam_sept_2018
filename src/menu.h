typedef enum {
    MENU_MODE,
    PAUSE_MODE,
    PLAY_MODE,
    LOAD_MODE,
    SAVE_MODE,
    QUIT_MODE,
    DIED_MODE,
    SETTINGS_MODE
} GameMode;

typedef struct {
    char *options[32];
    int count;
} MenuOptions;

typedef struct {
    int menuCursorAt;
    bool *running;
    GameMode gameMode;
    GameMode lastMode;
    Font *font;
    //TODO: Make this platform independent
    SDL_Window *windowHandle;

    TransitionState *transitionState;

    V2 lastMouseP;
} MenuInfo;

typedef struct {
    GameMode gameMode;
    GameMode lastMode;
    MenuInfo *info;
} MenuTransitionData;

void transitionCallbackForMenu(void *data_) {
    MenuTransitionData *trans = (MenuTransitionData *)data_;
    trans->info->gameMode = trans->gameMode;
    trans->info->lastMode = trans->lastMode;
    trans->info->menuCursorAt = 0;
}

void changeMenuState(MenuInfo *info, GameMode mode) {
    MenuTransitionData *data = (MenuTransitionData *)calloc(sizeof(MenuTransitionData), 1);
    data->gameMode = mode;
    data->lastMode = info->gameMode;
    data->info = info;
    setTransition_(info->transitionState, transitionCallbackForMenu, data);
}

bool updateMenu(MenuOptions *menuOptions, GameButton *gameButtons, MenuInfo *info, Arena *arenaForSounds, WavFile *moveSound) {
    bool active = true;
    if(wasPressed(gameButtons, BUTTON_DOWN)) {
        active = false;
        playMenuSound(arenaForSounds, moveSound, 0, AUDIO_BACKGROUND);
        info->menuCursorAt++;
        if(info->menuCursorAt >= menuOptions->count) {
            info->menuCursorAt = 0;
        }
    } 
    
    if(wasPressed(gameButtons, BUTTON_UP)) {
        active = false;
        playMenuSound(arenaForSounds, moveSound, 0, AUDIO_BACKGROUND);
        info->menuCursorAt--;
        if(info->menuCursorAt < 0) {
            info->menuCursorAt = menuOptions->count - 1;
        }
    }
    return active;
}

void renderMenu(Texture *backgroundTex, MenuOptions *menuOptions, MenuInfo *info, Lerpf *sizeTimers, float dt, V2 screenRelativeSize, V2 mouseP, bool mouseActive) {
    
    char *titleAt = menuOptions->options[info->menuCursorAt];
    
    if(backgroundTex) {
        static GLBufferHandles backgroundMenuRenderHandles = {};
        renderDisableDepthTest(&globalRenderGroup);
        openGlTextureCentreDim(&backgroundMenuRenderHandles, backgroundTex->id, v3(0.5f*bufferWidth, 0.5f*bufferHeight, -1), v2(bufferWidth, bufferHeight), COLOR_WHITE, 0, mat4(), 1, OrthoMatrixToScreen(bufferWidth, bufferHeight, 1), mat4());
        renderEnableDepthTest(&globalRenderGroup);
    }

    float yIncrement = bufferHeight / (menuOptions->count + 1);
    Rect2f menuMargin = rect2f(0, 0, bufferWidth, bufferHeight);
    
    float xAt_ = (bufferWidth / 2);
    float yAt = yIncrement;
    static float dtValue = 0;
    dtValue += dt;

    for(int menuIndex = 0;
        menuIndex < menuOptions->count;
        ++menuIndex) {
        
        
        float fontSize = screenRelativeSize.x;//mapValue(sin(dtValue), -1, 1, 0.7f, 1.2f);
        if(sizeTimers) {
            // updateLerpf(sizeTimers[menuIndex], dt, LINEAR);
            fontSize = sizeTimers[menuIndex].value;
        }

        char *title = menuOptions->options[menuIndex];
        float xAt = xAt_ - (getBounds(easyUnicode_utf8StreamToUtf32Stream((unsigned char *)title), menuMargin, info->font, fontSize).x / 2);


        Rect2f outputDim = outputText(info->font, xAt, yAt, bufferWidth, bufferHeight, easyUnicode_utf8StreamToUtf32Stream((unsigned char *)title), menuMargin, COLOR_WHITE, fontSize, false);
        //spread across screen so the mouse hit is more easily
        outputDim.min.x = 0;
        outputDim.max.x = bufferWidth;
        //
        if(inBounds(mouseP, outputDim, BOUNDS_RECT) && mouseActive) {
            info->menuCursorAt = menuIndex;
        }

        V4 menuItemColor = COLOR_BLUE;
        
        if(menuIndex == info->menuCursorAt) {
            menuItemColor = COLOR_RED;
        }
        
        outputText(info->font, xAt, yAt, bufferWidth, bufferHeight, easyUnicode_utf8StreamToUtf32Stream((unsigned char *)title), menuMargin, menuItemColor, fontSize, true);

        yAt += yIncrement;
    }
}

bool drawMenu(MenuInfo *info, Arena *longTermArena, GameButton *gameButtons, Texture *backgroundTex, WavFile *submitSound, WavFile *moveSound, float dt, V2 screenRelativeSize, V2 mouseP) {
    bool isPlayMode = false;
    MenuOptions menuOptions = {};


    if(wasPressed(gameButtons, BUTTON_ESCAPE)) {
        if(info->gameMode == PLAY_MODE) {
            changeMenuState(info, PAUSE_MODE);
        } else {
            changeMenuState(info, PLAY_MODE);
        }
    }
    bool mouseActive = false;
    bool changeMenuKey = wasPressed(gameButtons, BUTTON_ENTER) || wasPressed(gameButtons, BUTTON_LEFT_MOUSE);

    bool mouseChangedPos = !(info->lastMouseP.x == mouseP.x && info->lastMouseP.y == mouseP.y);

    //These are out of the switch scope because we have to render the menu before we release all the memory, so we do it at the end of the function
    InfiniteAlloc tempTitles = initInfinteAlloc(char *); 
    InfiniteAlloc tempStrings = initInfinteAlloc(char *);
    //
    
    Lerpf *thisSizeTimers = 0;
    switch(info->gameMode) {
        case LOAD_MODE:{

            menuOptions.options[menuOptions.count++] = "Go Back";

            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound);
            
            if(changeMenuKey) {
                // playMenuSound(longTermArena, submitSound, 0, AUDIO_BACKGROUND);
                if (info->menuCursorAt == menuOptions.count - 1) {
                    changeMenuState(info, info->lastMode);
                } 
                
                info->lastMode = LOAD_MODE;
            }

        } break;
        case QUIT_MODE:{
            menuOptions.options[menuOptions.count++] = "Really Quit?";
            menuOptions.options[menuOptions.count++] = "Go Back";
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound);
            
            if(changeMenuKey) {
                // playMenuSound(longTermArena, submitSound, 0, AUDIO_BACKGROUND);
                switch (info->menuCursorAt) {
                    case 0: {
                        *info->running = false;
                    } break;
                    case 1: {
                        changeMenuState(info, info->lastMode);
                    } break;
                }
                info->lastMode = QUIT_MODE;
            }
            
        } break;
        case DIED_MODE:{
            menuOptions.options[menuOptions.count++] = "Really Quit?";
            menuOptions.options[menuOptions.count++] = "Go Back";
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound);
            
            if(changeMenuKey) {
                // playMenuSound(longTermArena, submitSound, 0, AUDIO_BACKGROUND);
                switch (info->menuCursorAt) {
                    case 0: {
                        *info->running = false;
                    } break;
                    case 1: {
                        changeMenuState(info, info->lastMode);
                    } break;
                }
                info->lastMode = DIED_MODE;
            }
        } break;
        case SAVE_MODE:{
            menuOptions.options[menuOptions.count++] = "Save Progress";
            menuOptions.options[menuOptions.count++] = "Go Back";
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound);
            
            if(changeMenuKey) {
                // playMenuSound(longTermArena, submitSound, 0, AUDIO_BACKGROUND);
                switch (info->menuCursorAt) {
                    case 0: {
                    } break;
                    case 1: {
                        changeMenuState(info, info->lastMode);
                    } break;
                }
            }
        } break;
        case PAUSE_MODE:{
            menuOptions.options[menuOptions.count++] = "Resume";
            menuOptions.options[menuOptions.count++] = "Settings";
            menuOptions.options[menuOptions.count++] = "Quit";
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound);
            
            if(changeMenuKey) {
                // playMenuSound(longTermArena, submitSound, 0, AUDIO_BACKGROUND);
                switch (info->menuCursorAt) {
                    case 0: {
                        changeMenuState(info, PLAY_MODE);
                    } break;
                    case 1: {
                        changeMenuState(info, SETTINGS_MODE);
                    } break;
                    case 2: {
                        changeMenuState(info, QUIT_MODE);
                    } break;
                }
                info->menuCursorAt = 0;
                info->lastMode = PAUSE_MODE;
            }
        } break;
        case SETTINGS_MODE:{
            unsigned int windowFlags = SDL_GetWindowFlags(info->windowHandle);
            
            bool isFullScreen = (windowFlags & SDL_WINDOW_FULLSCREEN);
            char *fullScreenOption = isFullScreen ? (char *)"Exit Full Screen" : (char *)"Full Screen";
            char *soundOption = globalSoundOn ? (char *)"Turn Off Sound" : (char *)"Turn On Sound";
            menuOptions.options[menuOptions.count++] = fullScreenOption;
            menuOptions.options[menuOptions.count++] = soundOption;
            menuOptions.options[menuOptions.count++] = "Go Back";
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound);
            
            if(changeMenuKey) {
                // playMenuSound(longTermArena, submitSound, 0, AUDIO_BACKGROUND);
                switch (info->menuCursorAt) {
                    case 0: {
                        if(isFullScreen) {
                            // TODO(Oliver): Change this to handle resolution change. Have to query the current window size to adjust screen size. 
                            if(SDL_SetWindowFullscreen(info->windowHandle, false) < 0) {
                                printf("couldn't un-set to full screen\n");
                            }
                        } else {
                            if(SDL_SetWindowFullscreen(info->windowHandle, SDL_WINDOW_FULLSCREEN) < 0) {
                                printf("couldn't set to full screen\n");
                            }
                        }
                    } break;
                    case 1: {
                        globalSoundOn = !globalSoundOn;
                    } break;
                    case 2: {
                        changeMenuState(info, info->lastMode);
                    } break;

                }
                info->menuCursorAt = 0;
            }
        } break;
        case MENU_MODE:{
            menuOptions.options[menuOptions.count++] = "Play";
            menuOptions.options[menuOptions.count++] = "Quit";
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound);
            
            // NOTE(Oliver): Main Menu action options
            if(changeMenuKey) {
                // playMenuSound(longTermArena, submitSound, 0, AUDIO_BACKGROUND);
                switch (info->menuCursorAt) {
                    case 0: {
                        changeMenuState(info, PLAY_MODE);
                    } break;
                    case 1: {
                        changeMenuState(info, QUIT_MODE);
                    } break;
                }
            }
        } break;
        case PLAY_MODE: {
            isPlayMode = true;
            setSoundType(AUDIO_FLAG_MAIN);
        } break;
    } 

    if(!isPlayMode) {
        renderMenu(backgroundTex, &menuOptions, info, thisSizeTimers, dt, screenRelativeSize, mouseP, mouseChangedPos);
        setSoundType(AUDIO_FLAG_MENU);
    }

    for(int i = 0; i < tempTitles.count; ++i) {
        char **titlePtr = getElementFromAlloc(&tempTitles, i, char *);
        free(*titlePtr);

        char **formatStringPtr = getElementFromAlloc(&tempStrings, i, char *);
        free(*formatStringPtr);
    }
    releaseInfiniteAlloc(&tempTitles);
    releaseInfiniteAlloc(&tempStrings);


    info->lastMouseP = mouseP;

    return isPlayMode;
}
