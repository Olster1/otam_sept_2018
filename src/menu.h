#define GAME_MODE_TYPE(FUNC) \
FUNC(MENU_MODE) \
FUNC(OVERWORLD_MODE) \
FUNC(PAUSE_MODE) \
FUNC(PLAY_MODE) \
FUNC(LOAD_MODE) \
FUNC(SAVE_MODE) \
FUNC(SPLASH_SCREEN_MODE) \
FUNC(MEDIA_SETTINGS_MODE) \
FUNC(QUIT_MODE) \
FUNC(DIED_MODE) \
FUNC(CREDITS_MODE) \
FUNC(SETTINGS_MODE) \

typedef enum {
    GAME_MODE_TYPE(ENUM)
} GameMode;

#define MENU_GLOW_PERIOD 3.0f
static char *GameModeTypeStrings[] = { GAME_MODE_TYPE(STRING) };

typedef struct {
    bool notClickable;
    float size;
    V4 color;
} MenuOption;

typedef struct {
    MenuOption optionSettings[32];
    char *options[32];
    
    int count;
} MenuOptions;

typedef struct {
    int menuCursorAt;
    bool *running;
    GameMode gameMode;
    GameMode lastMode;
    GameMode stateInGame;
    
    int totalLevelCount;
    int activeSaveSlot;  //this is the active save slot

    bool deleteConfirmation;
    
    Timer colorTimer;
    Font *font;
    //TODO: Make this platform independent
    SDL_Window *windowHandle;
    
    Texture *backTex;
    
    transition_callback *callback;
    void *callBackData;
    
    TransitionState *transitionState;
    
    Timer splashScreenModeTimer;
    V2 lastMouseP;
    
    LevelCountFromFile saveStateDetails[1];

    int lastShownGroup;
    LevelData *levelDataArray;
    float resolutionDiffScale;  

    int maxGroupId;
    LevelGroup *levelGroups; 

    V3 *overworldCam;
} MenuInfo;

typedef struct {
    GameMode gameMode;
    GameMode lastMode;
    MenuInfo *info;
} MenuTransitionData;

static inline SoundType getAudioFlagForGameMode(GameMode gameMode) {
    SoundType audioFlag = AUDIO_FLAG_MENU;
    if(gameMode == PLAY_MODE) {
        audioFlag = AUDIO_FLAG_MAIN;
    } else if(gameMode == MENU_MODE) {
        audioFlag = AUDIO_FLAG_START_SCREEN;
    }
    return audioFlag;
}

void transitionCallbackForMenu(void *data_) {
    MenuTransitionData *trans = (MenuTransitionData *)data_;
    trans->info->gameMode = trans->gameMode;
    trans->info->lastMode = trans->lastMode;
    trans->info->menuCursorAt = 0;
    trans->info->callback(trans->info->callBackData);
    trans->info->colorTimer = initTimer(MENU_GLOW_PERIOD, false);
    
    SoundType newFlag = getAudioFlagForGameMode(trans->gameMode);
    setParentChannelVolume(newFlag, 1, SCENE_MUSIC_TRANSITION_TIME);
    setSoundType(newFlag);
}

void changeMenuState(MenuInfo *info, GameMode mode) {
    MenuTransitionData *data = (MenuTransitionData *)calloc(sizeof(MenuTransitionData), 1);
    data->gameMode = mode;
    data->lastMode = info->gameMode;
    data->info = info;
    setTransition_(info->transitionState, transitionCallbackForMenu, data);
    
    SoundType oldFlag = getAudioFlagForGameMode(data->lastMode);
    setParentChannelVolume(oldFlag, 0, SCENE_MUSIC_TRANSITION_TIME);
    // info->lastMouseP = v2(-1000, -1000); //make it undefined 
}

bool updateMenu(MenuOptions *menuOptions, GameButton *gameButtons, MenuInfo *info, Arena *arenaForSounds, WavFile *moveSound, float dt) {
    bool active = true;
    if(wasPressed(gameButtons, BUTTON_DOWN)) {
        active = false;
        playMenuSound(arenaForSounds, moveSound, 0, AUDIO_FOREGROUND);
        info->menuCursorAt++;
        info->colorTimer = initTimer(MENU_GLOW_PERIOD, false);
        if(info->menuCursorAt >= menuOptions->count) {
            info->menuCursorAt = 0;
        }
    } 

    TimerReturnInfo timerInfo = updateTimer(&info->colorTimer, dt);
    if(timerInfo.finished) {
        info->colorTimer = initTimer(MENU_GLOW_PERIOD, false);
    }
    
    if(wasPressed(gameButtons, BUTTON_UP)) {
        active = false;
        playMenuSound(arenaForSounds, moveSound, 0, AUDIO_FOREGROUND);
        info->menuCursorAt--;
        if(info->menuCursorAt < 0) {
            info->menuCursorAt = menuOptions->count - 1;
        }
    }
    return active;
}

void setMenuOption(MenuOptions *menuOptions, char *title, bool clickable, float size, V4 color) {
    int indexAt = menuOptions->count++;
    MenuOption *option = menuOptions->optionSettings + indexAt;
    menuOptions->options[indexAt] = title;
    
    option->notClickable = !clickable;
    option->color = color;
    option->size = size;
}

void renderMenu(Texture *backgroundTex, MenuOptions *menuOptions, MenuInfo *info, Lerpf *sizeTimers, float dt, V2 mouseP, bool mouseActive, V2 resolution) {
    
    if(backgroundTex) {
        renderDisableDepthTest(globalRenderGroup);
        renderTextureCentreDim(backgroundTex, v3(0, 0, -2), v2(resolution.x, resolution.y), COLOR_WHITE, 0, mat4(), OrthoMatrixToScreen(resolution.x, resolution.y), mat4());
        renderEnableDepthTest(globalRenderGroup);
    }
    
    float yIncrement = resolution.y / (menuOptions->count + 1);
    Rect2f menuMargin = rect2f(0, 0, resolution.x, resolution.y);
    
    float xAt_ = (resolution.x / 2);
    float yAt = yIncrement;
    static float dtValue = 0;
    dtValue += dt;
    
    for(int menuIndex = 0;
        menuIndex < menuOptions->count;
        ++menuIndex) {
        
        MenuOption *option = menuOptions->optionSettings + menuIndex;
        
        float fontSize = option->size;//mapValue(sin(dtValue), -1, 1, 0.7f, 1.2f);
        EasyAssert(fontSize > 0.0f);
        
        char *title = menuOptions->options[menuIndex];
        float xAt = xAt_ - (getBounds(title, menuMargin, info->font, fontSize, resolution, info->resolutionDiffScale).x / 2);
        
        bool clickable = !option->notClickable;
        
        if(clickable) {
            Rect2f outputDim = outputText(info->font, xAt, yAt, -1, resolution, title, menuMargin, COLOR_WHITE, fontSize, false, info->resolutionDiffScale);
            //spread across screen so the mouse hit is more easily
            outputDim.min.x = 0;
            outputDim.max.x = resolution.x;
            //
            if(inBounds(mouseP, outputDim, BOUNDS_RECT) && mouseActive) {
                info->menuCursorAt = menuIndex;
            }
        }
        
        V4 menuItemColor = option->color;
        
        if(clickable) {
            if(menuIndex == info->menuCursorAt) {
                float canconicalVal = getTimerValue01(&info->colorTimer);
                menuItemColor = smoothStep00V4(COLOR_GREEN, canconicalVal, COLOR_YELLOW);
            }
        }
        if(clickable) {
            outputText(info->font, xAt, yAt, -1, resolution, title, menuMargin, menuItemColor, fontSize, true, info->resolutionDiffScale);
        } else {
            outputTextNoBacking(info->font, xAt, yAt, -1, resolution, title, menuMargin, menuItemColor, fontSize, true, info->resolutionDiffScale);
        }
        yAt += yIncrement;
    }
}

MenuOptions initDefaultMenuOptions() {
    MenuOptions result = {};
    
    for(int i = 0; i < arrayCount(result.options); ++i) {
        MenuOption *opt = result.optionSettings + i;
        opt->color = COLOR_BLACK;
        opt->notClickable = false;
        opt->size = 1.0f;
    }
    return result;
}

GameMode drawMenu(MenuInfo *info, Arena *longTermArena, GameButton *gameButtons, Texture *backgroundTex, WavFile *submitSound, WavFile *moveSound, float dt, V2 resolution, V2 mouseP, LevelData *levelsData, int levelDataCount, int *lastShowGroup) {
    Rect2f menuMargin = rect2f(0, 0, resolution.x, resolution.y);
    bool isPlayMode = false;
    MenuOptions menuOptions = initDefaultMenuOptions();
    bool renderBackHelp = true;

    if(!isOn(&info->colorTimer)) {
        info->colorTimer = initTimer(MENU_GLOW_PERIOD, false);
    }
    
    if(wasPressed(gameButtons, BUTTON_ESCAPE) && info->gameMode != MENU_MODE) {
        if(info->gameMode == PLAY_MODE || info->gameMode == OVERWORLD_MODE) {
            info->stateInGame = info->gameMode;
            changeMenuState(info, PAUSE_MODE);
        } else if(info->gameMode == PAUSE_MODE) {
            changeMenuState(info, info->stateInGame);
        } else {
            changeMenuState(info, info->lastMode);
            info->deleteConfirmation = false;
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
            
            menuOptions.options[menuOptions.count++] = "Back";
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound, dt);
            
            if(changeMenuKey) {
                // playMenuSound(longTermArena, submitSound, 0, AUDIO_BACKGROUND);
                if (info->menuCursorAt == menuOptions.count - 1) {
                    changeMenuState(info, info->lastMode);
                } 
                
                
            }
            
        } break;
        case SPLASH_SCREEN_MODE: {
            float alpha = 0;
            if(isOn(&info->splashScreenModeTimer)) {
                TimerReturnInfo timeInfo = updateTimer(&info->splashScreenModeTimer, dt);
                alpha = smoothStep00(0, timeInfo.canonicalVal, 1);
                if(timeInfo.finished) {
                    changeMenuState(info, MENU_MODE);
                    turnTimerOff(&info->splashScreenModeTimer);
                    
                }
            } 
            RenderInfo renderInfo = calculateRenderInfo(v3(0, 0, -1), v3(0.7f*resolution.x, 0.7f*resolution.y, 1), v3(0, 0, 0), mat4());
            renderTextureCentreDim(findTextureAsset("logo.png"), renderInfo.pos, renderInfo.dim.xy, v4(1, 1, 1, alpha), 0, mat4(), OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm); 

        } break;
        case CREDITS_MODE:{
            
            V4 creditColor = COLOR_BLACK;
            setMenuOption(&menuOptions, "Credits", false, 1.0f, creditColor);
            setMenuOption(&menuOptions, "Programming & Game Design: Oliver Marsh", false, 0.5f, creditColor);
            setMenuOption(&menuOptions, "Music: Robert Marsh", false, 0.5f, creditColor);
            setMenuOption(&menuOptions, "Artwork: Kenny Assets", false, 0.5f, creditColor);
            setMenuOption(&menuOptions, "Sound Effects: ZapSplat", false, 0.5f, creditColor);
            setMenuOption(&menuOptions, "Thankyou to Casey Muratori's Handmade Hero", false, 0.5f, creditColor);
            setMenuOption(&menuOptions, "Thankyou to everyone at Handmade Network", false, 0.5f, creditColor);
            setMenuOption(&menuOptions, "Thankyou for playing!", false, 0.5f, creditColor);
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound, dt);
            
        } break;
        case QUIT_MODE:{
            menuOptions.options[menuOptions.count++] = "Really Quit?";
            menuOptions.options[menuOptions.count++] = "Go Back To Menu";
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound, dt);
            
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
                
            }
            
        } break;
        case DIED_MODE:{
            menuOptions.options[menuOptions.count++] = "Really Quit?";
            menuOptions.options[menuOptions.count++] = "Go Back";
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound, dt);
            
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
                
            }
        } break;
        case SAVE_MODE:{
            menuOptions.options[menuOptions.count++] = "Save Progress";
            menuOptions.options[menuOptions.count++] = "Go Back";
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound, dt);
            
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
            menuOptions.options[menuOptions.count++] = "Save File";
            menuOptions.options[menuOptions.count++] = "Credits";
            menuOptions.options[menuOptions.count++] = "Quit";
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound, dt);
            
            if(changeMenuKey) {
                // playMenuSound(longTermArena, submitSound, 0, AUDIO_BACKGROUND);
                switch (info->menuCursorAt) {
                    case 0: {
                        changeMenuState(info, info->stateInGame);
                    } break;
                    case 1: {
                        changeMenuState(info, MEDIA_SETTINGS_MODE);
                    } break;
                    case 2: {
                        updateSaveStateDetails(info->levelDataArray, info->saveStateDetails, arrayCount(info->saveStateDetails));
                        changeMenuState(info, SETTINGS_MODE);
                    } break;
                    case 3: {
                        changeMenuState(info, CREDITS_MODE);
                    } break;
                    case 4: {
                        changeMenuState(info, QUIT_MODE);
                    } break;
                }
            }
        } break;
        case MEDIA_SETTINGS_MODE:{
            unsigned int windowFlags = SDL_GetWindowFlags(info->windowHandle);
            
            bool isFullScreen = (windowFlags & SDL_WINDOW_FULLSCREEN);

            const char *fullscreenString = isFullScreen ? "Fullscreen On" : "Fullscreen Off";
            menuOptions.options[menuOptions.count++] = (char *)fullscreenString;
            const char * soundOption = (globalChannelsState_[AUDIO_BACKGROUND]) ? "Music On" : "Music Off";
            menuOptions.options[menuOptions.count++] = (char *)soundOption;
            const char * soundEffectOption = (globalChannelsState_[AUDIO_FOREGROUND]) ? "Sound Effects On" : "Sound Effects Off";
            menuOptions.options[menuOptions.count++] = (char *)soundEffectOption;
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound, dt);
            
            if(changeMenuKey) {
                // playMenuSound(longTermArena, submitSound, 0, AUDIO_BACKGROUND);
                switch (info->menuCursorAt) {
                    case 0: {
                         if(isFullScreen) {
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
                        globalChannelsState_[AUDIO_BACKGROUND] = !globalChannelsState_[AUDIO_BACKGROUND];
                    } break;
                    case 2: {
                        globalChannelsState_[AUDIO_FOREGROUND] = !globalChannelsState_[AUDIO_FOREGROUND];
                    }
                }
            }
        } break;
        case SETTINGS_MODE:{
            {
                TimerReturnInfo timerInfo = updateTimer(&info->colorTimer, dt);
                if(timerInfo.finished) {
                    info->colorTimer = initTimer(MENU_GLOW_PERIOD, false);
                }

                float settingsFontSize = 0.7f;
                
                V2 mouseP_settings = mouseP;//v2_minus(mouseP, v2_scale(0.5f, resolution));
                // mouseP_settings.y = resolution.y - mouseP_settings.y; //flip it so it is y up
                // printf("%f\n", mouseP_settings.y);
                float yAt = 0.38f*resolution.y;
                float width = resolution.x / 3.0f;
                float xAt = resolution.x / (2.0f*(float)arrayCount(info->saveStateDetails));
                
                
                static LerpV4 cLerps[arrayCount(info->saveStateDetails)];
                static bool initied = false;
                if(!initied) {
                    for(int lerpIndex = 0; lerpIndex < arrayCount(cLerps); ++lerpIndex)  {
                        V4 color = COLOR_WHITE;
                        if(lerpIndex == info->activeSaveSlot && arrayCount(info->saveStateDetails) > 1) {
                            color = COLOR_GREEN;
                        }
                        cLerps[lerpIndex] = initLerpV4(color);
                        
                    }   
                    initied = true;
                }
                
                static LerpV4 dLerps[arrayCount(info->saveStateDetails)];
                static bool initied2 = false;
                if(!initied2) {
                    for(int lerpIndex = 0; lerpIndex < arrayCount(dLerps); ++lerpIndex)  {
                        V4 color = COLOR_BLACK;
                        dLerps[lerpIndex] = initLerpV4(color);
                    }   
                    initied2 = true;
                }
                
                for(int at = 0; at < arrayCount(info->saveStateDetails); ++at) {
                    char saveDataName[512] = {};
                    int levelsCompleted = info->saveStateDetails[at].completedCount;
                    int totalLevelCount = info->totalLevelCount;
                    
                    sprintf(saveDataName, "Levels Completed: %d/%d", levelsCompleted, totalLevelCount);
                    // if(info->saveStateDetails[at].valid) {
                    
                    
                    // } else {
                    //     sprintf(saveDataName, "Save Slot %d", (at + 1));
                    // }
                    
                    float picDim = 0.6f*width;
                    RenderInfo renderInfo = calculateRenderInfo(v3(xAt, yAt, -1), v3(picDim, picDim, 1), v3(0, 0, 0), mat4());

                    // float canconicalVal = getTimerValue01(&info->colorTimer);
                    V4 uiColor = COLOR_GREEN;

                    Rect2f outputDim = rect2fCenterDimV2(renderInfo.transformPos.xy, renderInfo.transformDim.xy);
                    if(arrayCount(info->saveStateDetails) > 1) {
                        
                        float lerpPeriod = 0.3f;
                        if(!updateLerpV4(&cLerps[at], dt, LINEAR)) {
                            if(!easyLerp_isAtDefault(&cLerps[at])) {
                                setLerpInfoV4_s(&cLerps[at], cLerps[at].defaultVal, 0.1, &cLerps[at].value);
                            }
                            
                        }
                        
                        
                        if(inBounds(mouseP_settings, outputDim, BOUNDS_RECT)) {
                            setLerpInfoV4_s(&cLerps[at], UI_BUTTON_COLOR, 0.2f, &cLerps[at].value);
                            if(wasPressed(gameButtons, BUTTON_LEFT_MOUSE)) {
                                if(info->activeSaveSlot != at) {
                                    info->activeSaveSlot = at;
                                    loadSaveFile(levelsData, levelDataCount, info->activeSaveSlot, lastShowGroup);
                                    playMenuSound(longTermArena, submitSound, 0, AUDIO_FOREGROUND);
                                    setLerpInfoV4_s(&cLerps[at], COLOR_GREEN, 0.2f, &cLerps[at].value);
                                    for(int lerpIndex = 0; lerpIndex < arrayCount(cLerps); lerpIndex++) {
                                        if(at != lerpIndex) {
                                            cLerps[lerpIndex].defaultVal = COLOR_WHITE;    
                                        } else {
                                            cLerps[lerpIndex].defaultVal = COLOR_GREEN;        
                                        }
                                        
                                    }
                                }
                            }
                        }
                    }
                    
                    
                    // renderTextureCentreDim(findTextureAsset("save.png"), renderInfo.pos, renderInfo.dim.xy, uiColor, 0, mat4TopLeftToBottomLeft(resolution.y), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y), renderInfo.pvm); 
                    
                    float fontY = 0.5f*resolution.y;//outputDim.maxY + settingsFontSize*info->font->fontHeight;
                    Rect2f saveMargin = rect2f(0, 0, resolution.x, resolution.y);
                    Rect2f textOutputDim = outputText(info->font, saveMargin.minX, saveMargin.minY, -1, resolution, saveDataName, saveMargin, COLOR_BLACK, settingsFontSize, false, info->resolutionDiffScale);                    
                    textOutputDim = outputTextNoBacking(info->font, xAt - (getDim(textOutputDim).x/2), fontY, -1, resolution, saveDataName, saveMargin, COLOR_BLACK, settingsFontSize, true, info->resolutionDiffScale);                    
                    
                    if(info->saveStateDetails[at].valid) {
                        
                        char *deleteFileTitle = "Delete Save File?";
                        Rect2f deleteTextOutputDim = outputText(info->font, 0 , 0, -1, resolution, deleteFileTitle, menuMargin, COLOR_BLACK, settingsFontSize, false, info->resolutionDiffScale);                    
                        
                        float xFor = xAt - (getDim(deleteTextOutputDim).x/2);
                        float yFor = textOutputDim.maxY + settingsFontSize*info->font->fontHeight;
                        
                        Rect2f deleteBounds = outputText(info->font, xFor, yFor, -1, resolution, deleteFileTitle, menuMargin, COLOR_WHITE, settingsFontSize, false, info->resolutionDiffScale);                        
                        V4 deleteButtonColor = dLerps[at].value;
                        // error_printFloat4("dim", deleteBounds.E);
                        // error_printFloat2("mouseP", mouseP_settings.E);
                        float lerpPeriod = 0.3f;
                        if(!updateLerpV4(&dLerps[at], dt, LINEAR)) {
                            if(!easyLerp_isAtDefault(&dLerps[at]) && !info->deleteConfirmation) {
                                setLerpInfoV4_s(&dLerps[at], dLerps[at].defaultVal, 0.01, &dLerps[at].value);
                            }
                            
                        }
                        if(!info->deleteConfirmation) {
                            if(inBounds(mouseP_settings, deleteBounds, BOUNDS_RECT)) {
                                setLerpInfoV4_s(&dLerps[at], COLOR_YELLOW, 0.2f, &dLerps[at].value);
                                if(wasPressed(gameButtons, BUTTON_LEFT_MOUSE)) {
                                    info->deleteConfirmation = true;
                                }
                            }
                        } else {
                            char *confirmText = "Confirm";
                            Rect2f confrimTextOutputDim = outputText(info->font, 0 , 0, -1, resolution, confirmText, menuMargin, COLOR_BLACK, settingsFontSize, false, info->resolutionDiffScale);                    
                            
                            float yAtForConfirm = 0.2f*resolution.y;
                            float thrid = resolution.x / 3;
                            float xForCY = (2*thrid) - (getDim(confrimTextOutputDim).x/2);
                            
                            char *cancelText = "Cancel";
                            Rect2f cancelTextOutputDim = outputText(info->font, 0 , 0, -1, resolution, cancelText, menuMargin, COLOR_BLACK, settingsFontSize, false, info->resolutionDiffScale);                    
                            
                            float xForCN = (1*thrid) - (getDim(cancelTextOutputDim).x/2);
                            
                            
                            Rect2f confirmBounds = outputText(info->font, xForCY, yAtForConfirm, -1, resolution, confirmText, menuMargin, COLOR_WHITE, settingsFontSize, false, info->resolutionDiffScale);                        
                            Rect2f cancelBounds = outputText(info->font, xForCN, yAtForConfirm, -1, resolution, cancelText, menuMargin, COLOR_WHITE, settingsFontSize, false, info->resolutionDiffScale);                        
                            
                            static LerpV4 confirmLerp = initLerpV4(COLOR_BLACK);
                            static LerpV4 cancelLerp = initLerpV4(COLOR_BLACK);
                            
                            if(!updateLerpV4(&confirmLerp, dt, LINEAR)) {
                                if(!easyLerp_isAtDefault(&confirmLerp)) {
                                    setLerpInfoV4_s(&confirmLerp, confirmLerp.defaultVal, 0.01, &confirmLerp.value);
                                }
                            }
                            
                            V4 confirmColor = confirmLerp.value;
                            if(inBounds(mouseP_settings, confirmBounds, BOUNDS_RECT)) {
                                setLerpInfoV4_s(&confirmLerp, COLOR_YELLOW, 0.2f, &confirmLerp.value);
                                if(wasPressed(gameButtons, BUTTON_LEFT_MOUSE)) {
                                    info->overworldCam->xy = info->levelDataArray[LEVEL_0].pos;
                                    startGameAgain(at);
                                    info->saveStateDetails[at].completedCount = 0;
                                    for(int lvlIndex = 0; lvlIndex < LEVEL_COUNT; ++lvlIndex) {
                                        LevelData *lvlData = info->levelDataArray + lvlIndex;
                                        if(lvlData->valid) {
                                            if(lvlData->groupId == 0) {
                                                lvlData->state = LEVEL_STATE_UNLOCKED;
                                            } else {
                                                lvlData->state = LEVEL_STATE_LOCKED;
                                            }
                                        }
                                    }
                                    for(int groupIndex = 0; groupIndex < info->maxGroupId; ++groupIndex) {
                                        info->levelGroups[groupIndex].activated = false;
                                    }

                                    info->lastShownGroup = -1;
                                    // updateSaveStateDetailsWithFile(info->saveStateDetails, arrayCount(info->saveStateDetails));
                                    info->deleteConfirmation = false;
                                }
                            }
                            
                            
                            if(!updateLerpV4(&cancelLerp, dt, LINEAR)) {
                                if(!easyLerp_isAtDefault(&cancelLerp)) {
                                    setLerpInfoV4_s(&cancelLerp, cancelLerp.defaultVal, 0.01, &cancelLerp.value);
                                }
                            }
                            
                            V4 cancelColor = cancelLerp.value;
                            if(inBounds(mouseP_settings, cancelBounds, BOUNDS_RECT)) {
                                setLerpInfoV4_s(&cancelLerp, COLOR_YELLOW, 0.2f, &cancelLerp.value);
                                if(wasPressed(gameButtons, BUTTON_LEFT_MOUSE)) {
                                    info->deleteConfirmation = false;
                                }
                            }
                            
                            outputText(info->font, xForCY, yAtForConfirm, -1, resolution, confirmText, menuMargin, confirmColor, settingsFontSize, true, info->resolutionDiffScale);                        
                            outputText(info->font, xForCN, yAtForConfirm, -1, resolution, cancelText, menuMargin, cancelColor, settingsFontSize, true, info->resolutionDiffScale);                        
                            
                        }
                        
                        
                        outputTextNoBacking(info->font, xFor, yFor, -1, resolution, deleteFileTitle, menuMargin, deleteButtonColor, settingsFontSize, true, info->resolutionDiffScale);                        
                    }
                    
                    xAt += width;
                }
                
                // { //back button
                //     RenderInfo renderInfo = calculateRenderInfo(v2ToV3(v2(0.1f*resolution.x, 0.1f*resolution.y), -1), v3(100, 100, 1), v3(0, 0, 0), mat4());
                    
                //     Rect2f outputDim2 = rect2fCenterDimV2(renderInfo.transformPos.xy, renderInfo.transformDim.xy);
                //     static LerpV4 eLerp = initLerpV4(COLOR_WHITE);
                //     // error_printFloat4("bounds", outputDim2.E);
                //     float lerpPeriod = 0.3f;
                //     if(!updateLerpV4(&eLerp, dt, LINEAR)) {
                //         if(!easyLerp_isAtDefault(&eLerp)) {
                //             setLerpInfoV4_s(&eLerp, COLOR_WHITE, 0.1, &eLerp.value);
                //         }
                        
                //     }
                    
                //     V4 backUIColor = eLerp.value;
                    
                //     if(!deleteConfirmation) {
                        
                //         if(inBounds(mouseP_settings, outputDim2, BOUNDS_RECT)) {
                //             setLerpInfoV4_s(&eLerp, UI_BUTTON_COLOR, 0.2f, &eLerp.value);
                //             if(wasPressed(gameButtons, BUTTON_LEFT_MOUSE)) {

                //                 changeMenuState(info, OVERWORLD_MODE);
                //                 deleteConfirmation = false;
                //             }
                //         }
                //     }
                    
                //     // renderDrawRectOutlineCenterDim(renderInfo.pos, renderInfo.dim.xy, COLOR_BLACK, 0, mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm));            
                //     renderTextureCentreDim(info->backTex, renderInfo.pos, renderInfo.dim.xy, backUIColor, 0, mat4TopLeftToBottomLeft(resolution.y), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y), renderInfo.pvm); 
                // }   
            }
            
            /*
                        unsigned int windowFlags = SDL_GetWindowFlags(info->windowHandle);
                        
                        bool isFullScreen = (windowFlags & SDL_WINDOW_FULLSCREEN);
                        
                        
                        float zMenuAt = -1;
                        float zMenuAtBack = zMenuAt - 0.2f;
                        RenderInfo renderInfo;
                        float yAt = 0.4f*resolution.y;
                        float xAt = 0.5f*resolution.x;
                        
                        float backWidth = 0.4f*resolution.y;
                        float backHeight = info->font->fontHeight;
                        float yoffset = info->font->fontHeight/8;
                        
                        
                        float boxSize = 30;
                        float menuFontSize = 0.5f;
                        
                        renderInfo = calculateRenderInfo(v3(xAt, yAt - yoffset, zMenuAt), v3(boxSize, boxSize, 1), v3(0, 0, 0), mat4());
                        Texture *checkBoxTex = (isFullScreen) ? findTextureAsset("UIBox.png") : findTextureAsset("UIBox_empty.png");
                        Rect2f outputDim = rect2fCenterDimV2(renderInfo.transformPos.xy, renderInfo.transformDim.xy);
                        
                        V4 color = COLOR_WHITE;
                        if(inBounds(mouseP, outputDim, BOUNDS_RECT)) {
                            color = COLOR_GREEN;
                            if(wasPressed(gameButtons, BUTTON_LEFT_MOUSE)) {
                                if(isFullScreen) {
                                   if(SDL_SetWindowFullscreen(info->windowHandle, false) < 0) {
                                       printf("couldn't un-set to full screen\n");
                                   }
                               } else {
                                   if(SDL_SetWindowFullscreen(info->windowHandle, SDL_WINDOW_FULLSCREEN) < 0) {
                                       printf("couldn't set to full screen\n");
                                   }
                               }
                            }
                        }
                        
                        
                        renderTextureCentreDim(checkBoxTex, renderInfo.pos, renderInfo.dim.xy, color, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(), Mat4Mult(OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y), renderInfo.pvm)); 
                        xAt += boxSize;
                        
                        outputText(info->font, xAt, yAt, zMenuAt, resolution, "Fullscreen", menuMargin, COLOR_BLACK, menuFontSize, true);
                        yAt += 100;
                        xAt = 0.5f*resolution.x;
                        
                        renderInfo = calculateRenderInfo(v3(xAt, yAt - yoffset, zMenuAt), v3(boxSize, boxSize, 1), v3(0, 0, 0), mat4());
                        checkBoxTex = (globalSoundOn) ? findTextureAsset("UIBox.png") : findTextureAsset("UIBox_empty.png");
                        outputDim = rect2fCenterDimV2(renderInfo.transformPos.xy, renderInfo.transformDim.xy);
                        
                        color = COLOR_WHITE;
                        if(inBounds(mouseP, outputDim, BOUNDS_RECT)) {
                            color = COLOR_GREEN;
                            if(wasPressed(gameButtons, BUTTON_LEFT_MOUSE)) {
                               globalSoundOn = !globalSoundOn;
                            }
                        }
                        
                        
                        renderTextureCentreDim(checkBoxTex, renderInfo.pos, renderInfo.dim.xy, color, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(), Mat4Mult(OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y), renderInfo.pvm)); 
                        xAt += boxSize;
                        
                        outputText(info->font, xAt, yAt, zMenuAt, resolution, "Sound", menuMargin, COLOR_BLACK, menuFontSize, true);
                        float backDim = 80;
                        
                        outputDim = rect2fCenterDimV2(v2(backDim, backDim), v2(backDim, backDim));
                        
                        if(inBounds(mouseP, outputDim, BOUNDS_RECT)) {
                            color = COLOR_GREEN;
                            if(wasPressed(gameButtons, BUTTON_LEFT_MOUSE)) {
                               changeMenuState(info, PLAY_MODE);
                            }
                        }
                        
                        renderTextureCentreDim(findTextureAsset("back.png"), v3(backDim, backDim, zMenuAt), v2(backDim, backDim), COLOR_WHITE, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y)); 
                        */
        } break;
        case MENU_MODE:{
            renderBackHelp = false;
            char *title = APP_TITLE;
            Rect2f menuMargin = rect2f(0, 0, resolution.x, resolution.y);
            float fontSize = 1;
            float xAt = (resolution.x - getBounds(title, menuMargin, info->font, fontSize, resolution, info->resolutionDiffScale).x) / 2;
            
            outputTextNoBacking(info->font, xAt, resolution.y / 2, -1, resolution, title, menuMargin, COLOR_BLACK, fontSize, true, info->resolutionDiffScale);
            
            char *secondTitle = "Click To Start";
            fontSize = 0.5f;
            xAt = (resolution.x - getBounds(secondTitle, menuMargin, info->font, fontSize, resolution, info->resolutionDiffScale).x) / 2;
            
            static float dt_val = 0;
            dt_val += dt;
            V4 color = smoothStep00V4(COLOR_WHITE, dt_val / 3.0f, COLOR_BLUE);
            if(dt_val >= 3.0f) {
                dt_val = 0;
            }
            outputTextNoBacking(info->font, xAt, 0.7f*resolution.y, -1, resolution, secondTitle, menuMargin, color, fontSize, true, info->resolutionDiffScale);
            
            if(wasPressed(gameButtons, BUTTON_LEFT_MOUSE)) {
                changeMenuState(info, OVERWORLD_MODE);
            }
            // menuOptions.options[menuOptions.count++] = "Play";
            // menuOptions.options[menuOptions.count++] = "Quit";
            
            // mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound);
            
            // // NOTE(Oliver): Main Menu action options
            // if(changeMenuKey) {
            //     // playMenuSound(longTermArena, submitSound, 0, AUDIO_BACKGROUND);
            //     switch (info->menuCursorAt) {
            //         case 0: {
            //             changeMenuState(info, PLAY_MODE);
            //         } break;
            //         case 1: {
            //             changeMenuState(info, QUIT_MODE);
            //         } break;
            //     }
            // }
        } break;
        case PLAY_MODE: {
            isPlayMode = true;
            setSoundType(AUDIO_FLAG_MAIN);
        } break;
        case OVERWORLD_MODE:{
            isPlayMode = true;
        } break;
    } 
    
    if(!isPlayMode) {
        renderMenu(backgroundTex, &menuOptions, info, thisSizeTimers, dt, mouseP, mouseChangedPos, resolution);
        setSoundType(AUDIO_FLAG_MENU);
        if(renderBackHelp) {
            char *goBackString = "ESC to go Back";
            V2 strBounds = getBounds(goBackString, menuMargin, info->font, 0.5f, resolution, info->resolutionDiffScale);
            outputText(info->font, resolution.x - strBounds.x - 15*info->resolutionDiffScale , resolution.y - strBounds.y, -1, resolution, goBackString, menuMargin, COLOR_WHITE, 0.5f, true, info->resolutionDiffScale);
        }
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
    
    return info->gameMode;
}
