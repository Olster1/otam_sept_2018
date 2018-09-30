#define PRINT_FRAME_RATE 0

typedef struct {
	unsigned int frameBackBufferId;
	unsigned int renderBackBufferId; //used for ios 
	SDL_Window *windowHandle;
	SDL_GLContext renderContext; 
	bool valid;
} OSAppInfo;

OSAppInfo easyOS_createApp(char *windowName, V2 *screenDim) {
	OSAppInfo result = {};
	result.valid = true;
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER|SDL_INIT_GAMECONTROLLER) != 0) {
		assert(!"sdl not initialized");
		result.valid = false;
		return result;
	} 
	    
#if DESKTOP
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#else 
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    
    if(screenDim->x == 0 && screenDim->y == 0) {
	    SDL_DisplayMode DM;
		SDL_GetCurrentDisplayMode(0, &DM);
		screenDim->x = DM.w;
		screenDim->y = DM.h;
	}

    result.windowHandle = SDL_CreateWindow(
        windowName,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        screenDim->x,
        screenDim->y,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1); 

    // result.threadContext = SDL_GL_CreateContext(result.windowHandle);
    result.renderContext = SDL_GL_CreateContext(result.windowHandle);
    if(result.renderContext) {
        
        if(SDL_GL_MakeCurrent(result.windowHandle, result.renderContext) == 0) {
            if(SDL_GL_SetSwapInterval(1) == 0) {
            } else {
            	#if DESKTOP
                assert(!"Couldn't set swap interval\n");
                result.valid = false;
                #endif
            }
        } else {
            assert(!"Couldn't make context current\n");
            result.valid = false;
        }
    } else {
        assert(!"couldn't make a context");
        result.valid = false;
    }

    SDL_SysWMinfo sysInfo;
    SDL_VERSION(&sysInfo.version);
    
    SDL_bool sysResult = SDL_GetWindowWMInfo(result.windowHandle, &sysInfo);
    if(!sysResult) {
        printf("%s\n", SDL_GetError());
        assert(!"couldn't get info");
        result.valid = false;
    }

//TODO make this handle windows //
#if !DESKTOP
    result.frameBackBufferId = sysInfo.info.uikit.framebuffer;
    result.renderBackBufferId = sysInfo.info.uikit.colorbuffer;
#endif

    return result;
}

typedef struct {
	float refresh_rate;
	Matrix4 metresToPixels;
	Matrix4 pixelsToMeters;
	V2 screenRelativeSize;
	SDL_AudioSpec audioSpec;
} AppSetupInfo;

AppSetupInfo easyOS_setupApp(V2 resolution, char *resPathFolder) {
	AppSetupInfo result = {};

	V2 idealResolution = v2(1280, 720); // Not sure if this is the best place for this?? Have to see. 

    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(0, &mode);
    result.refresh_rate = mode.refresh_rate;

#if DESKTOP
    gl3wInit();
#endif
    
	// globalExeBasePath = getResPathFromExePath(SDL_GetBasePath(), resPathFolder);
    globalExeBasePath = concat(SDL_GetBasePath(), resPathFolder);

    //for stb_image so the images aren't upside down.
    stbi_set_flip_vertically_on_load(true);
    //
   
    //////////SETUP AUDIO/////
    initAudioSpec(&result.audioSpec, 44100);
    initAudio(&result.audioSpec);
    //////////

    ////SETUP OPEN GL//
    enableRenderer(resolution.x, resolution.y);
    renderCheckError();
    //////

    ////INIT RANDOM GENERATOR
    srand (time(NULL));
    //////

    /////////Scale the size factor to be keep size of app constant. 
    V2 screenRelativeSize = v2(idealResolution.x / resolution.x, idealResolution.y / resolution.y); 
    V2 ratio = v2_scale(60.0f, screenRelativeSize);
    // assert(ratio.x != 0);
    // assert(ratio.y != 0);
    result.metresToPixels = Matrix4_scale(mat4(), v3(ratio.x, ratio.y, 1));
    result.pixelsToMeters = Matrix4_scale(mat4(), v3(1.0f / ratio.x, 1.0f / ratio.y, 1));
    result.screenRelativeSize = screenRelativeSize;
    /////

    return result;
}

void easyOS_endProgram(OSAppInfo *appInfo) {
	SDL_GL_DeleteContext(appInfo->renderContext);
    SDL_DestroyWindow(appInfo->windowHandle);
    SDL_Quit();
}

void easyOS_beginFrame(V2 resolution) {
	glViewport(0, 0, resolution.x, resolution.y);

	////Delete the storeage buffers from last frame 
	//this is the pvm data and color data we send as tables to the GPU for the shader. 
	for(int i = 0; i < lastStorageBufferCount; ++i) {
	    BufferStorage *store = lastBufferStorage + i;
	    deleteBufferStorage(store);
	}
	lastStorageBufferCount = 0;
	//
}

void easyOS_endFrame(V2 resolution, V2 screenDim, float *dt_, SDL_Window *windowHandle, unsigned int compositedFrameBufferId, unsigned int backBufferId, unsigned int renderbufferId, unsigned int *lastTime, float monitorFrameTime) {
	float dt = *dt_;

	////////Letterbox if the ratio isn't correct//
	float screenRatio =  screenDim.x / resolution.x;
	float h1 = resolution.y * screenRatio;
	if(h1 > screenDim.y) {
	    screenRatio =  screenDim.y / resolution.y;
	    float w1 = resolution.x * screenRatio;
	    // assert(w1 <= screenDim.x);
	}

	float screenX = screenRatio*resolution.x;
	float screenY = screenRatio*resolution.y;
	// assert(screenX <= screenDim.x);
	// assert(screenY <= screenDim.y);

	float wResidue = (screenDim.x - screenX) / 2.0f;
	float yResidue = (screenDim.y - screenY) / 2.0f;

	////Resolve Frame
	glViewport(0, 0, screenDim.x, screenDim.y);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, backBufferId);
	renderCheckError();
	glBindFramebuffer(GL_READ_FRAMEBUFFER, compositedFrameBufferId); 
	renderCheckError();
	glBlitFramebuffer(0, 0, resolution.x, resolution.y, wResidue, yResidue, screenDim.x - wResidue, screenDim.y - yResidue, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	renderCheckError();                    
	///////
   glViewport(0, 0, screenDim.x, screenDim.y);
   updateChannelVolumes(dt);
#if !DESKTOP
   glBindRenderbuffer(GL_RENDERBUFFER, renderbufferId);
#endif
   SDL_GL_SwapWindow(windowHandle);

   unsigned int now = SDL_GetTicks();
   float timeInFrameMilliSeconds = (now - *lastTime);
   //TODO: Do our own wait if Vsync isn't on. 
   //NOTE: This is us choosing the best frame time within the intervals of possible frame rates!!!
   dt = timeInFrameMilliSeconds / 1000.0f;
   float frameRates[] = {monitorFrameTime*1.0f, monitorFrameTime*2.0f, monitorFrameTime*3.0f, monitorFrameTime*4.0f};
   float smallestDiff = 0;
   bool set = false;
   float newRate = monitorFrameTime;
   for(int i = 0; i < arrayCount(frameRates); ++i) {
       float val = frameRates[i];
       float diff = dt - val;
       if(diff < 0.0f) {
           diff *= -1;
       }
       if(diff < smallestDiff || !set) {
           set = true;
           smallestDiff = diff;
           newRate = val;
       }
   }
   *dt_ = newRate; //set the actual dt
#if PRINT_FRAME_RATE
   printf("%f\n", 1.0f / (timeInFrameMilliSeconds / 1000.0f));
#endif
   *lastTime = now;
}

typedef struct {
	V2 mouseP;
	V2 mouseP_yUp;

	int scrollWheelY;
} AppKeyStates;

AppKeyStates easyOS_processKeyStates(V2 resolution, V2 *screenDim, bool *running) {
	AppKeyStates state = {};
	//Save state of last frame game buttons 
	bool mouseWasDown = isDown(gameButtons, BUTTON_LEFT_MOUSE);
	bool mouseWasDownRight = isDown(gameButtons, BUTTON_RIGHT_MOUSE);
	bool leftArrowWasDown = isDown(gameButtons, BUTTON_LEFT);
	bool rightArrowWasDown = isDown(gameButtons, BUTTON_RIGHT);
	bool upArrowWasDown = isDown(gameButtons, BUTTON_UP);
	bool downArrowWasDown = isDown(gameButtons, BUTTON_DOWN);
	bool shiftWasDown = isDown(gameButtons, BUTTON_SHIFT);
	bool commandWasDown = isDown(gameButtons, BUTTON_COMMAND);
	bool spaceWasDown = isDown(gameButtons, BUTTON_SPACE);
	/////
	zeroArray(gameButtons);
	
	assert(gameButtons[BUTTON_LEFT_MOUSE].transitionCount == 0);
	//ask player for new input
	SDL_Event event;
	state.scrollWheelY = 0;
	
	while( SDL_PollEvent( &event ) != 0 ) {
        switch(event.type) {
	        case SDL_APP_DIDENTERFOREGROUND: {
	        	SDL_Log("SDL_APP_DIDENTERFOREGROUND");
	        } break;
		    case SDL_APP_DIDENTERBACKGROUND: {
		        SDL_Log("SDL_APP_DIDENTERBACKGROUND");
		    } break;
		    case SDL_APP_LOWMEMORY: {
		        SDL_Log("SDL_APP_LOWMEMORY");
		    } break;
		    case SDL_APP_TERMINATING: {
		        SDL_Log("SDL_APP_TERMINATING");
		    } break;
		    case SDL_APP_WILLENTERBACKGROUND: {
		        SDL_Log("SDL_APP_WILLENTERBACKGROUND");
		    } break;
		    case SDL_APP_WILLENTERFOREGROUND: {
		        SDL_Log("SDL_APP_WILLENTERFOREGROUND");
		    } break;
		    case SDL_FINGERMOTION: {
		        // SDL_Log("Finger Motion");
		    } break;
		    case SDL_FINGERDOWN: {
		        // SDL_Log("Finger Down");
			} break;		        
		    case SDL_FINGERUP: {
		        // SDL_Log("Finger Up");
		    } break;
		}
		
	    if (event.type == SDL_WINDOWEVENT) {
	        switch(event.window.event) {
	            case SDL_WINDOWEVENT_RESIZED: {
	                screenDim->x = event.window.data1;
	                screenDim->y = event.window.data2;
	            } break;
	            default: {
	            }
	        }
	    }  else if( event.type == SDL_QUIT ) {
	        *running = false;
	    } else if(event.type == SDL_MOUSEWHEEL) {
	        state.scrollWheelY = event.wheel.y;
	    } else if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
	        
	        SDL_Keycode keyCode = event.key.keysym.sym;
	        
	        bool altKeyWasDown = (event.key.keysym.mod & KMOD_ALT);
	        bool shiftKeyWasDown = (event.key.keysym.mod & KMOD_SHIFT);
	        bool isDown = (event.key.state == SDL_PRESSED);
	        bool repeated = event.key.repeat;
	        ButtonType buttonType = BUTTON_NULL;
	        switch(keyCode) {
	            case SDLK_RETURN: {
	                buttonType = BUTTON_ENTER;
	            } break;
	            case SDLK_SPACE: {
	                buttonType = BUTTON_SPACE;
	            } break;
	            case SDLK_UP: {
	                // buttonType = BUTTON_UP;
	            } break;
	            case SDLK_ESCAPE: {
	                buttonType = BUTTON_ESCAPE;
	            } break;
	            case SDLK_DOWN: {
	                // buttonType = BUTTON_DOWN;
	            } break;
	            case SDLK_1: {
	                buttonType = BUTTON_1;
	            } break;
	            case SDLK_BACKQUOTE: {
	                buttonType = BUTTON_TILDE;
	            } break;
	            case SDLK_F1: {
	                buttonType = BUTTON_F1;
	            } break;
	            case SDLK_LGUI: {
	                // buttonType = BUTTON_COMMAND;
	            } break;
	            case SDLK_z: {
	                buttonType = BUTTON_Z;
	            } break;
	            case SDLK_r: {
	                buttonType = BUTTON_R;
	            } break;
	            case SDLK_LSHIFT: {
	                //buttonType = BUTTON_SHIFT;
	            } break;
	            case SDLK_LEFT: {
	                if(isDown) {
	                }
	                //buttonType = BUTTON_LEFT; //NOTE: use key states instead
	            } break;
	            case SDLK_RIGHT: {
	                if(isDown) {
	                }
	                //buttonType = BUTTON_RIGHT; //NOTE: use key states instead
	            } break;
	            case SDLK_BACKSPACE: {
	                if(isDown) {
	                }
	                
	            } break;
	        }
	        if(buttonType) {
	            sdlProcessGameKey(&gameButtons[buttonType], isDown, repeated);
	        }
	        
	    } else if( event.type == SDL_TEXTINPUT ) {
	        char *inputString = event.text.text;
	        //splice(&inputBuffer, inputString, 1);
	        //cursorTimer.value = 0;
	    }
	}
	
	const Uint8* keystates = SDL_GetKeyboardState(NULL);

	bool leftArrowIsDown = keystates[SDL_SCANCODE_LEFT];
	bool rightArrowIsDown = keystates[SDL_SCANCODE_RIGHT];
	bool upArrowIsDown = keystates[SDL_SCANCODE_UP];
	bool downArrowIsDown = keystates[SDL_SCANCODE_DOWN];

	bool shiftIsDown = keystates[SDL_SCANCODE_LSHIFT];
	bool commandIsDown = keystates[SDL_SCANCODE_LGUI];
	
	sdlProcessGameKey(&gameButtons[BUTTON_LEFT], leftArrowIsDown, leftArrowWasDown == leftArrowIsDown);
	sdlProcessGameKey(&gameButtons[BUTTON_RIGHT], rightArrowIsDown, rightArrowWasDown == rightArrowIsDown);
	sdlProcessGameKey(&gameButtons[BUTTON_DOWN], downArrowIsDown, downArrowWasDown == downArrowIsDown);
	sdlProcessGameKey(&gameButtons[BUTTON_UP], upArrowIsDown, upArrowWasDown == upArrowIsDown);
	sdlProcessGameKey(&gameButtons[BUTTON_SHIFT], shiftIsDown, shiftWasDown == shiftIsDown);
	sdlProcessGameKey(&gameButtons[BUTTON_COMMAND], commandIsDown, commandWasDown == commandIsDown);

	int mouseX, mouseY;

	unsigned int mouseState = SDL_GetMouseState(&mouseX, &mouseY);
	
	float screenDimX = screenDim->x;
	float screenDimY = screenDim->y;
	state.mouseP = v2(mouseX/screenDimX*resolution.x, mouseY/screenDimY*resolution.y);
	
	state.mouseP_yUp = v2_minus(v2(state.mouseP.x, -1*state.mouseP.y + resolution.y), v2_scale(0.5f, resolution));

	bool leftMouseDown = mouseState & SDL_BUTTON_LMASK;
	sdlProcessGameKey(&gameButtons[BUTTON_LEFT_MOUSE], leftMouseDown, leftMouseDown == mouseWasDown);
	
	bool rightMouseDown = mouseState & SDL_BUTTON_RMASK;
	sdlProcessGameKey(&gameButtons[BUTTON_RIGHT_MOUSE], rightMouseDown, rightMouseDown == mouseWasDownRight);

	return state;
}