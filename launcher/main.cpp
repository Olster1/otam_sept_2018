#if DEVELOPER_MODE
#define RESOURCE_PATH_EXTENSION "../res/" 
#else 
#define RESOURCE_PATH_EXTENSION "res/"
#endif

#include <GL/gl3w.h>


#include "../SDL2/sdl.h"
#include "../SDL2/SDL_syswm.h"

#include "defines.h"
#include "easy_headers.h"
#include "easy_asset_loader.h"

#if __APPLE__
#include <unistd.h>
#endif

/*
    Have white texture known to the renderer 
    Have arenas setup automatically 
    have all the other stuff like monifotr refresh rate setup automatically 
    have it so you don't have to have an extra buffer
    Make button a more reusable concept
    Use matrixes instead of meters per pixels

*/

int main(int argc, char *args[]) {
    V2 screenDim = v2(0, 0); //init in create app function
    V2 resolution = v2(400, 400);
    screenDim = resolution;
    OSAppInfo appInfo = easyOS_createApp("Feoh the Fitter Launcher", &screenDim, false);
    assert(appInfo.valid);
    
    if(appInfo.valid) {
        Arena longTermArena = createArena(Kilobytes(200));
        assets = (Asset **)pushSize(&longTermArena, 4096*sizeof(Asset *));
        AppSetupInfo setupInfo = easyOS_setupApp(resolution, RESOURCE_PATH_EXTENSION, &longTermArena);
        
        float monitorRefreshRate = (float)setupInfo.refresh_rate;
        float dt = 1.0f / min(monitorRefreshRate, 60.0f); //use monitor refresh rate 
        float idealFrameTime = 1.0f / 60.0f;
        
        ////INIT FONTS
        char *fontName = concat(globalExeBasePath, "/fonts/Khand-Regular.ttf");//Roboto-Regular.ttf");/);
        Font mainFont = initFont(fontName, 128);
        ///
        FrameBuffer mainFrameBuffer = createFrameBuffer(resolution.x, resolution.y, 0);

        unsigned int lastTime = SDL_GetTicks();
        bool scrollBarActive = false;

        loadAndAddImagesToAssets("img/");

        Texture *fitrisTex = findTextureAsset("Fitris.png");
        Texture *tickTex = findTextureAsset("UIBox.png");
        Texture *emptyTex = findTextureAsset("UIBox_empty.png");

        Texture *upTex = findTextureAsset("upButton.png");
        Texture *downTex = findTextureAsset("downButton.png");

        bool fullscreen = true;
        bool blackBars = true;

        bool holdingButton = false;

        int gameResolutionIndex = 0;

        //v2(750, 1334) iphone resolution -> need work to modify the the screen so you can see everything. 
        V2 resolutions[] = {v2(0, 0), v2(1280, 720), v2(1980, 1080), v2(640, 480), v2(800, 500), v2(1024, 640), v2(1024, 768), v2(1152, 720), v2(1280, 800)};

        SDL_DisplayMode DM;
        SDL_GetCurrentDisplayMode(0, &DM);
        resolutions[0].x = DM.w;
        resolutions[0].y = DM.h;
        if(resolutions[0].x == 0 || resolutions[0].y == 0) {
            resolutions[0].x = 400;
            resolutions[0].y = 400;
        }

        int startingIndex = 0; //this is so we don't repeat a resolution
        for(int i = 1; i < arrayCount(resolutions); ++i) {
            if(resolutions[i].x == resolutions[0].x && 
                resolutions[i].y == resolutions[0].y) {
                startingIndex = 1;
                gameResolutionIndex = i;
                break;        
            }
        }
        

        bool barOpen = false;
        LerpV4 fullLerp = initLerpV4(COLOR_WHITE);
        LerpV4 barsLerp = initLerpV4(COLOR_WHITE);     
        LerpV4 launchLerp = initLerpV4(COLOR_WHITE);
        bool running = true;
        AppKeyStates keyStates = {};
        char *titleApp = "Feoh the Fitter";
        Rect2f margin = rect2f(0, 0, resolution.x, resolution.y);
        while(running) {
            easyOS_processKeyStates(&keyStates, resolution, &screenDim, &running);
            easyOS_beginFrame(resolution);
            
            beginRenderGroupForFrame(globalRenderGroup);
            globalRenderGroup->whiteTexture = findTextureAsset("white.png");

            clearBufferAndBind(appInfo.frameBackBufferId, COLOR_WHITE);
            clearBufferAndBind(mainFrameBuffer.bufferId, hexARGBTo01Color(0xFFCCFDFF));
            
            renderEnableDepthTest(globalRenderGroup);
            setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD);

            V2 posAt = v2(resolution.x/2.0f, resolution.y*0.4f);
            outputText(&mainFont, posAt.x - 120, posAt.y, -1, resolution, "Resolution:", margin, COLOR_BLACK, 1, true, setupInfo.screenRelativeSize);
            char string[1028]; 
            if(barOpen) {
                V2 startP = posAt;
                float maxWidth = 0;
                float offset = 5;  
                /////////////////
                V4 buttonColor = COLOR_BLACK;
                sprintf(string, "%d x %d", (int)resolutions[gameResolutionIndex].x, (int)resolutions[gameResolutionIndex].y);
                Rect2f bounds = outputText(&mainFont, posAt.x, posAt.y, -1, resolution, string, margin, COLOR_BLACK, 1.0f, false, setupInfo.screenRelativeSize);
                if(getDim(bounds).x > maxWidth)  maxWidth = getDim(bounds).x;
                startP.y -= getDim(bounds).y;

                if(inBounds(keyStates.mouseP, bounds, BOUNDS_RECT)) {
                    buttonColor = COLOR_YELLOW;
                    if(wasPressed(keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                        barOpen = false;
                    }
                }
                outputText(&mainFont, posAt.x, posAt.y, -0.8f, resolution, string, margin, buttonColor, 1.0f, true, setupInfo.screenRelativeSize);

                posAt.y += getDim(bounds).y + offset;
                ////////
                for(int i = startingIndex; i < arrayCount(resolutions); i++) {
                    if(i != gameResolutionIndex) {
                        V4 buttonColor = COLOR_BLACK;
                        sprintf(string, "%d x %d", (int)resolutions[i].x, (int)resolutions[i].y);
                        Rect2f bounds = outputText(&mainFont, posAt.x, posAt.y, -1, resolution, string, margin, COLOR_BLACK, 1.0f, false, setupInfo.screenRelativeSize);
                        if(getDim(bounds).x > maxWidth)  maxWidth = getDim(bounds).x;

                        if(inBounds(keyStates.mouseP, bounds, BOUNDS_RECT)) {
                            buttonColor = COLOR_YELLOW;
                            if(wasPressed(keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                                barOpen = false;
                                gameResolutionIndex = i;
                            }
                        }
                        outputText(&mainFont, posAt.x, posAt.y, -0.8f, resolution, string, margin, buttonColor, 1.0f, true, setupInfo.screenRelativeSize);

                        if(i != arrayCount(resolutions) - 1) {
                            posAt.y += getDim(bounds).y + offset;
                        }
                    }
                }

                V2 backDim = v2(maxWidth, posAt.y - startP.y);
                V2 backDimScaled = v2_hadamard(v2(1.2f, 1.1f), v2(maxWidth, posAt.y - startP.y));
                renderDrawRectCenterDim(v2ToV3(v2_plus(startP, v2(0.5f*backDim.x, 0.5f*backDim.y)), -0.9f), backDimScaled, COLOR_WHITE, 0, mat4TopLeftToBottomLeft(resolution.y), Mat4Mult(OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y), mat4())); 
            } else {

                sprintf(string, "%d x %d", (int)resolutions[gameResolutionIndex].x, (int)resolutions[gameResolutionIndex].y);
                V4 buttonColor = COLOR_BLACK;
                Rect2f bounds = outputText(&mainFont, posAt.x, posAt.y, -1, resolution, string, margin, buttonColor, 1.0f, false, setupInfo.screenRelativeSize);
                if(inBounds(keyStates.mouseP, bounds, BOUNDS_RECT)) {
                    buttonColor = COLOR_YELLOW;
                    if(wasPressed(keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                        barOpen = true;
                    }
                }
                V2 backDim = getDim(bounds);
                V2 scaledBackDim = v2_scale(1.3f, backDim);
                renderDrawRectCenterDim(v2ToV3(v2_plus(posAt, v2(0.5f*backDim.x, -0.5f*backDim.y)), -0.9f), scaledBackDim, COLOR_WHITE, 0, mat4TopLeftToBottomLeft(resolution.y), Mat4Mult(OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y), mat4())); 
                outputText(&mainFont, posAt.x, posAt.y, -0.8f, resolution, string, margin, buttonColor, 1.0f, true, setupInfo.screenRelativeSize);
            }

            RenderInfo renderInfo = calculateRenderInfo(v3(0, 3, -1), v3_scale(1, v3(4, 4, 1)), v3(0, 0, 0), setupInfo.metresToPixels);
            // renderTextureCentreDim(fitrisTex, renderInfo.pos, renderInfo.dim.xy, COLOR_WHITE, 0, mat4(), OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm); 

            V2 bounds = getBounds(titleApp, margin, &mainFont, 1.5f, resolution, setupInfo.screenRelativeSize);
            outputText(&mainFont, resolution.x/2.0f - bounds.x/2.0f, resolution.y*0.2f, -1, resolution, titleApp, margin, COLOR_BLACK, 1.5f, true, setupInfo.screenRelativeSize);
            /////The actual buttons
            
            Texture *fullscreenTex = fullscreen ? tickTex : emptyTex;
            if(EasyUI_UpdateCheckBox(!barOpen, fullscreenTex, &keyStates, v3(-1.0f, 0, -1), v3(1, 1, 1), setupInfo.metresToPixels, resolution, dt, &fullLerp)) { 
                    fullscreen = !fullscreen;
            }   
            outputText(&mainFont, resolution.x/2.0f, 210, -1, resolution, "Fullscreen?", margin, COLOR_BLACK, 1, true, setupInfo.screenRelativeSize);

            
            Texture *blackBarsTex = blackBars ? tickTex : emptyTex;
            if(EasyUI_UpdateCheckBox(!barOpen, blackBarsTex, &keyStates, v3(-1.0f, -2, -1), v3(1, 1, 1), setupInfo.metresToPixels, resolution, dt, &barsLerp)) { 
                    blackBars = !blackBars;
            }   
            outputText(&mainFont, resolution.x/2.0f, 250, -1, resolution, "Pixel Perfect?", margin, COLOR_BLACK, 1, true, setupInfo.screenRelativeSize);


            Texture *buttonTex = holdingButton ? downTex : upTex;
            if(EasyUI_UpdateButton(!barOpen, &holdingButton, buttonTex, &mainFont, "Play!", &keyStates, 1, v3(9, 15, -1), setupInfo.metresToPixels, setupInfo.pixelsToMeters, resolution, dt, &launchLerp, setupInfo.screenRelativeSize)) {
                char arguments[1028] = {};
                sprintf(arguments, "%d %d %d %d", (int)resolutions[gameResolutionIndex].x, (int)resolutions[gameResolutionIndex].y, fullscreen, blackBars);
                
                #ifdef __APPLE__ 

                    const char *argsForApp = (const char *)arguments;
                    char *execLocation = concat(SDL_GetBasePath(), "game"); 
                    char *arg1 = execLocation;
                    char arg2[512];
                    char arg3[512];
                    char arg4[512];
                    char arg5[512];

                    sprintf(arg2, "%d", (int)resolutions[gameResolutionIndex].x);
                    sprintf(arg3, "%d", (int)resolutions[gameResolutionIndex].y);
                    sprintf(arg4, "%d", (int)fullscreen);
                    sprintf(arg5, "%d", (int)blackBars);


                    if (execl((const char *)execLocation, arg1, arg2, arg3, arg4, arg5, NULL) < 0) {
                        printf("%s\n", strerror(errno));
                        assert(false);
                    }
                    // running = false;
                #elif _WIN32
                if(WinExec(concat("./game ", arguments), SW_SHOWDEFAULT) > 31) {
                    running = false;
                } else {
                    assert(false);
                }
                #endif
            }



            drawRenderGroup(globalRenderGroup);
            
            easyOS_endFrame(resolution, screenDim, &dt, appInfo.windowHandle, mainFrameBuffer.bufferId, appInfo.frameBackBufferId, appInfo.renderBackBufferId, &lastTime, monitorRefreshRate, false);
        }
        easyOS_endProgram(&appInfo);
    }
    return(0);
}
    