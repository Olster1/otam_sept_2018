typedef enum {
    MENU_MODE,
    OVERWORLD_MODE,
    PAUSE_MODE,
    PLAY_MODE,
    LOAD_MODE,
    SAVE_MODE,
    QUIT_MODE,
    DIED_MODE,
    CREDITS_MODE,
    SETTINGS_MODE
} GameMode;

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
    Font *font;
    //TODO: Make this platform independent
    SDL_Window *windowHandle;

    transition_callback *callback;
    void *callBackData;

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
    trans->info->callback(trans->info->callBackData);
}

void changeMenuState(MenuInfo *info, GameMode mode) {
    MenuTransitionData *data = (MenuTransitionData *)calloc(sizeof(MenuTransitionData), 1);
    data->gameMode = mode;
    data->lastMode = info->gameMode;
    data->info = info;
    setTransition_(info->transitionState, transitionCallbackForMenu, data);
    // info->lastMouseP = v2(-1000, -1000); //make it undefined 
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
        renderDisableDepthTest(&globalRenderGroup);
        renderTextureCentreDim(backgroundTex, v3(0, 0, -2), v2(resolution.x, resolution.y), COLOR_WHITE, 0, mat4(), OrthoMatrixToScreen(resolution.x, resolution.y), mat4());
        renderEnableDepthTest(&globalRenderGroup);
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
        assert(fontSize > 0.0f);

        char *title = menuOptions->options[menuIndex];
        float xAt = xAt_ - (getBounds(title, menuMargin, info->font, fontSize, resolution).x / 2);

        bool clickable = !option->notClickable;

        if(clickable) {
            Rect2f outputDim = outputText(info->font, xAt, yAt, -1, resolution, title, menuMargin, COLOR_WHITE, fontSize, false);
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
                menuItemColor = COLOR_RED;
            }
        }
        outputText(info->font, xAt, yAt, -1, resolution, title, menuMargin, menuItemColor, fontSize, true);
        yAt += yIncrement;
    }
}

MenuOptions initDefaultMenuOptions() {
    MenuOptions result = {};

    for(int i = 0; i < arrayCount(result.options); ++i) {
        MenuOption *opt = result.optionSettings + i;
        opt->color = COLOR_BLUE;
        opt->notClickable = false;
        opt->size = 1.0f;
    }
    return result;
}

GameMode drawMenu(MenuInfo *info, Arena *longTermArena, GameButton *gameButtons, Texture *backgroundTex, WavFile *submitSound, WavFile *moveSound, float dt, V2 resolution, V2 mouseP) {
    Rect2f menuMargin = rect2f(0, 0, resolution.x, resolution.y);
    bool isPlayMode = false;
    MenuOptions menuOptions = initDefaultMenuOptions();

    // if(wasPressed(gameButtons, BUTTON_ESCAPE) && info->gameMode != MENU_MODE) {
    //     if(info->gameMode == PLAY_MODE) {
    //         changeMenuState(info, PAUSE_MODE);
    //     } else {
    //         changeMenuState(info, PLAY_MODE);
    //     }
    // }
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
        case CREDITS_MODE:{

            V4 creditColor = COLOR_BLACK;
            setMenuOption(&menuOptions, "Credits", false, 1.0f, creditColor);
            setMenuOption(&menuOptions, "Programming & Game Design: Oliver Marsh", false, 0.5f, creditColor);
            setMenuOption(&menuOptions, "Music: Robert Marsh", false, 0.5f, creditColor);
            setMenuOption(&menuOptions, "Artwork: Kenny Assets", false, 0.5f, creditColor);
            setMenuOption(&menuOptions, "Sound Effects: ZapSplat", false, 0.5f, creditColor);
            setMenuOption(&menuOptions, "Go Back", true, 0.5f, COLOR_BLUE);
            
            mouseActive = updateMenu(&menuOptions, gameButtons, info, longTermArena, moveSound);
            
            if(changeMenuKey) {
                // playMenuSound(longTermArena, submitSound, 0, AUDIO_BACKGROUND);
                if (info->menuCursorAt == menuOptions.count - 1) {
                    changeMenuState(info, info->lastMode);
                } 
                info->lastMode = CREDITS_MODE;
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
            menuOptions.options[menuOptions.count++] = "Credits";
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
                        changeMenuState(info, CREDITS_MODE);
                    } break;
                    case 3: {
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
        } break;
        case MENU_MODE:{
            char *title = APP_TITLE;
            Rect2f menuMargin = rect2f(0, 0, resolution.x, resolution.y);
            float fontSize = 1;
            float xAt = (resolution.x - getBounds(title, menuMargin, info->font, fontSize, resolution).x) / 2;

            outputText(info->font, xAt, resolution.y / 2, -1, resolution, title, menuMargin, COLOR_BLACK, fontSize, true);

            char *secondTitle = "Click To Start";
            fontSize = 0.5f;
            xAt = (resolution.x - getBounds(secondTitle, menuMargin, info->font, fontSize, resolution).x) / 2;

            static float dt_val = 0;
            dt_val += dt;
            V4 color = smoothStep00V4(COLOR_WHITE, dt_val / 3.0f, COLOR_BLUE);
            if(dt_val >= 3.0f) {
                dt_val = 0;
            }
            outputText(info->font, xAt, 0.7f*resolution.y, -1, resolution, secondTitle, menuMargin, color, fontSize, true);

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
