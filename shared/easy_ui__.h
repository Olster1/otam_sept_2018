#include <dirent.h>

typedef enum {
    UI_SLIDER,
    UI_TITLE,
    UI_BUTTON,
    UI_CHECKBOX,
    UI_TEXTBOX,
    UI_SCROLL_FILE_PARENT,
    UI_SCROLL_FILE,
    
} UI_Type;

typedef struct UIElement UIElement;
typedef struct UIElement{
    UI_Type type;
    
    V4 color;
    V4 hotColor;
    V4 coldColor; 
    V4 interactColor;
    Rect2f marginPercent; //in fraction of screen
    
    Array_Dynamic childElms;
    
    union {
        struct { //slider values
            V2 handleRadius;	
        };
        struct { //for text box
            InputBuffer inputBuffer;
        };
        struct { //for fileContents scroll item
            Texture texture;
            Texture **viewing;
        };
        struct { //for file scroller parent
            float xOffset;
            float xVel;
            float xAccel;
            
            float dimScale;
        };
        struct 
        {
            
        };
    };
    
    LerpV4 lerpInfo;
    
    char *title;
    void *value;	
    
} UIElement;

typedef struct {
    Array_Dynamic elements;
    UIElement *interactingWith;
    V2 grabOffset;
    
    Array_Dynamic parentElmPtrs; //current parent elms
    V2 lastMouseP;
    
    
} UIState;

UIState declareUIState() {
    UIState state = {};
    state.elements = initArray(UIElement);
    state.parentElmPtrs = initArray(UIElement *); //to store parent elements
    return state;
}


int declareUIElm_(UIState *state, UIElement *elm, V4 initialColor, V4 hotColor, V4 interactColor, Rect2f margin) {
    elm->hotColor = hotColor;
    elm->interactColor = interactColor;
    elm->coldColor = elm->color = initialColor;
    elm->lerpInfo = initLerpV4();
    elm->marginPercent = margin;
    elm->childElms = initArray(UIElement);
    
    Array_Dynamic *arrayToAddTo = &state->elements;
    if(state->parentElmPtrs.count) {
        UIElement *parentElm = *((UIElement **)(getLastElement(&state->parentElmPtrs)));
        arrayToAddTo = &parentElm->childElms;
    }
    
    int index = addElement(arrayToAddTo, *elm);
    return index;
}

int declareSlider(UIState *state, float *value, V4 initialColor, V4 hotColor, V4 interactColor, Rect2f margin, V2 handleRadius) {
    UIElement elm = {};
    elm.type = UI_SLIDER;
    elm.value = value;
    elm.handleRadius = handleRadius;
    int index = declareUIElm_(state, &elm, initialColor, hotColor, interactColor, margin);
    
    return index;
}

int declareTitle(UIState *state, char *title, V4 initialColor, V4 hotColor, V4 interactColor, Rect2f margin) {
    UIElement elm = {};
    elm.type = UI_TITLE;
    elm.title = title;
    int index = declareUIElm_(state, &elm, initialColor, hotColor, interactColor, margin);
    
    return index;
}

int declareButton(UIState *state, char *title, bool *value, V4 initialColor, V4 hotColor, V4 interactColor, Rect2f margin) {
    UIElement elm = {};
    elm.type = UI_BUTTON;
    elm.title = title;
    elm.value = value;
    int index = declareUIElm_(state, &elm, initialColor, hotColor, interactColor, margin);
    
    return index;
}

int declareScrollFile(UIState *state, char *fileName, Texture **viewing, V4 initialColor, V4 hotColor, V4 interactColor, Rect2f margin) {
    UIElement elm = {};
    elm.type = UI_SCROLL_FILE;
    elm.viewing = viewing;
    
    EasyAssert(fileName);
    
    elm.texture = loadImage(fileName);
    
    EasyAssert(state->parentElmPtrs.count > 0);
    int index = declareUIElm_(state, &elm, initialColor, hotColor, interactColor, margin);
    
    return index;
}

int declareScrollFileParent(UIState *state, char *dirName, Texture **viewing, V4 initialColor, V4 hotColor, V4 interactColor, Rect2f margin) {
    UIElement elm = {};
    
    elm.type = UI_SCROLL_FILE_PARENT;
    elm.dimScale = 100;
    
    int index = declareUIElm_(state, &elm, initialColor, hotColor, interactColor, margin);
    
    UIElement *parentElm = (UIElement *)getElement(&state->elements, index);
    EasyAssert(parentElm);
    addElement(&state->parentElmPtrs, parentElm);	
    
#ifdef __APPLE__
    DIR *directory = opendir(dirName);
    if(directory) {
        
        struct dirent *dp = 0;
        
        do {
            dp = readdir(directory);
            if (dp) {
                char *fileName = concat(dirName, dp->d_name);
                char *ext = getFileExtension(fileName);
                if(strcmp(ext, "jpg") == 0 || 
                   strcmp(ext, "jpeg") == 0 || 
                   strcmp(ext, "png") == 0 || 
                   strcmp(ext, "bmp") == 0) {
                    //is an image file. 
                    globalFileCount++;
                    declareScrollFile(state, fileName, viewing, initialColor, hotColor, interactColor, margin);	
                }
            }
        } while (dp);
        closedir(directory);
    }
#else 
    EasyAssert(!"not implemented");
#endif
    
    return index;
}

Rect2f drawString(Font *font, char *string, int bufferWidth, int bufferHeight, int yAt) {
    Rect2f margin = rect2f(0, 0, bufferWidth, bufferHeight);
    V2 bounds = getBounds(string, margin, font);
    float x = (bufferWidth / 2) - (bounds.x/2);
    float y = yAt;// - (bounds.y/2);
    Rect2f result = outputText(font, x, y, bufferWidth, bufferHeight, string, margin, COLOR_BLACK, 1, true);
    
    return result;
}

typedef struct {
    Array_Dynamic *array;
    int indexAt;
} UIStackInfo;

void updateUI(UIState *state, int bufferWidth, int bufferHeight, float yAt, float dt, V2 mouseP_yUp, Font *titleFont) {
    UIElement *hotElm = 0;
    V2 hotElmP = {};
    
    Array_Dynamic indexAts = initArray(UIStackInfo);
    
    Array_Dynamic *currentArray = &state->elements;
    Rect2f interactingbounds = {};
    Matrix4 topDownMat4 = mat4TopLeftToBottomLeft(bufferHeight);
    
    float xScrollAt = 0;
    int currentIndexAt = 0;
    bool processing = true;
    
    UIElement *parentElm = 0;
    Rect2f parentBounds = {};
    for(; processing && currentIndexAt < currentArray->count; ) {
        bool incrementIndex = true;
        UIElement *elm = (UIElement *)getElement(currentArray, currentIndexAt);
        if(elm) {
            BoundsType boundsType = BOUNDS_RECT;
            Rect2f uiBounds = {};
            float minX = elm->marginPercent.minX*bufferWidth;
            float maxX = elm->marginPercent.maxX*bufferWidth;
            switch(elm->type) {
                case UI_SLIDER: {
                    float value01 = *((float *)elm->value);
                    
                    float valueAt = lerp(minX, value01, maxX);
                    
                    V2 circleDim = elm->handleRadius;
                    Matrix4 topDownMat4 = mat4TopLeftToBottomLeft(bufferHeight);
                    openGlDrawLine(v2(minX, yAt), v2(maxX, yAt), COLOR_BLACK, 3, topDownMat4, 0, 0, 1);
                    
                    V2 circleP = v2(valueAt, yAt);
                    updateLerpV4(&elm->lerpInfo, dt, SMOOTH_STEP_01);
                    
                    openGlDrawCircle(circleP, circleDim, elm->color, topDownMat4, 1);
                    boundsType = BOUNDS_CIRCLE;
                    uiBounds = rect2fCenterDimV2(circleP, circleDim);
                    float extraSpacing = 20;
                    yAt += (3*circleDim.y) + extraSpacing;
                } break;
                case UI_TITLE: {
                    Rect2f bounds = drawString(titleFont, elm->title, bufferWidth, bufferHeight, yAt);
                    float height = getDim(bounds).y;
                    if(height) {
                        yAt += height + 0.1f*titleFont->fontHeight;
                    }
                } break;
                case UI_CHECKBOX: {
                    boundsType = BOUNDS_RECT;
                    
                    updateLerpV4(&elm->lerpInfo, dt, SMOOTH_STEP_01);
                    
                    float extraSpacing = 20;
                    yAt += getDim(uiBounds).y + extraSpacing;
                } break;
                case UI_BUTTON: {
                    boundsType = BOUNDS_RECT;
                    
                    bool value01 = *((bool *)elm->value);
                    
                    float xAt = lerp(minX, 0.5f, maxX);
                    
                    Rect2f absMargin = rect2f(minX, 0, maxX, bufferHeight);
                    
                    uiBounds = getBoundsRectf(elm->title, xAt, yAt, absMargin, titleFont);
                    
                    uiBounds = expandRectf(uiBounds, v2(10, 10));
                    float moveFactorX = getDim(uiBounds).x / 2;
                    xAt -= moveFactorX;
                    
                    uiBounds.minX -= moveFactorX;
                    uiBounds.maxX -= moveFactorX;
                    
                    
                    openGlDrawRect(uiBounds, elm->color, 0, topDownMat4, 1);
                    openGlDrawRectOutlineRect2f(uiBounds, COLOR_BLACK, 0, topDownMat4, 1);
                    
                    outputText(titleFont, xAt, yAt, bufferWidth, bufferHeight, elm->title, absMargin, COLOR_BLACK, 1, true);
                    
                    updateLerpV4(&elm->lerpInfo, dt, SMOOTH_STEP_01);
                    
                    float extraSpacing = 0;
                    yAt += getDim(uiBounds).y + extraSpacing;
                } break;
                case UI_TEXTBOX: {
                    EasyAssert(!"not implemented");
                } break;
                case UI_SCROLL_FILE_PARENT: {
                    elm->xOffset += elm->xAccel*sqr(dt) + elm->xVel*dt;
                    elm->xVel += dt*elm->xAccel;
                    elm->xVel -= 0.4f*elm->xVel*dt;
                    
                    
                    xScrollAt = (minX) - elm->xOffset;
                    
                    
                    boundsType = BOUNDS_RECT;
                    V2 uiDim = v2(maxX - minX, elm->dimScale);
                    yAt += uiDim.y/2;
                    uiBounds = rect2fCenterDim(lerp(minX, 0.5, maxX), yAt, uiDim.x, uiDim.y);
                    
                    //openGlDrawRect(uiBounds, elm->color, 0, topDownMat4);
                    
                    updateLerpV4(&elm->lerpInfo, dt, SMOOTH_STEP_01);
                } break;
                case UI_SCROLL_FILE: {
                    boundsType = BOUNDS_RECT;
                    
                    Texture *texture = &elm->texture;
                    GLuint texId = texture->id;
                    
                    V2 dim = v2(texture->width, texture->height);
                    dim = normalizeV2(dim);
                    EasyAssert(parentElm);
                    EasyAssert(parentElm->type == UI_SCROLL_FILE_PARENT);
                    dim = v2_scale(parentElm->dimScale, dim);
                    
                    xScrollAt += dim.x/2;
                    V2 center = v2(xScrollAt, yAt);
                    
                    glEnable(GL_SCISSOR_TEST);
                    V2 parentDim = getDim(parentBounds);
                    Rect2f bottomUpRect = transformRect2f(parentBounds, topDownMat4);
                    glScissor(bottomUpRect.min.x, bottomUpRect.min.y, parentDim.x, parentDim.y);
                    openGlTextureCentreDim(texId, center, dim, elm->color, 0, topDownMat4, 1);
                    glDisable(GL_SCISSOR_TEST);
                    
                    uiBounds = rect2fCenterDimV2(center, dim);
                    
                    updateLerpV4(&elm->lerpInfo, dt, SMOOTH_STEP_01);
                    xScrollAt += (dim.x/2) + 70;
                } break;
                
            }
            V4 color = elm->coldColor;
            float period = 0.3f;
            if(!state->interactingWith) {
                if(inBounds(mouseP_yUp, uiBounds, boundsType)) {
                    hotElm = elm;
                    hotElmP = getCenter(uiBounds);
                    color = elm->hotColor;
                    period = 1;  
                    
                    setLerpInfoV4_s(&elm->lerpInfo, color, 
                                    period, &elm->color);
                } else {
                    setLerpInfoV4_s(&elm->lerpInfo, color, 
                                    period, &elm->color);
                } 
                
                
            } else if(state->interactingWith == elm) {
                interactingbounds = uiBounds;
            }
            
            if(elm->childElms.count) { 
                //has children
                UIStackInfo tempInfo = {};
                tempInfo.array = currentArray;
                tempInfo.indexAt = currentIndexAt;
                parentElm = elm;
                parentBounds = uiBounds;
                
                addElement(&indexAts, tempInfo);
                
                currentArray = &elm->childElms;
                currentIndexAt = 0;
                incrementIndex = false;
            } else if(currentIndexAt == currentArray->count - 1) {
                //finished array, move back to parent. 
                
                UIStackInfo *stackInfo = (UIStackInfo *)getElement(&indexAts, indexAts.count - 1);
                if(stackInfo) {
                    currentArray = stackInfo->array;
                    currentIndexAt = stackInfo->indexAt;
                    removeElement_unordered(&indexAts, indexAts.count - 1);
                } else {
                    //we're finished
                    processing = false;
                }
                
            }
            if(incrementIndex) {
                currentIndexAt++;
            }
        }
    }
    
    if(state->interactingWith) {
        UIElement *intElm = state->interactingWith;
        switch(intElm->type) {
            case UI_SLIDER: {
                float minX = intElm->marginPercent.minX*bufferWidth;
                float maxX = intElm->marginPercent.maxX*bufferWidth;
                
                *((float *)intElm->value) = (v2_minus(mouseP_yUp, state->grabOffset).x - minX) / (maxX - minX);
                *((float *)intElm->value) = clamp(0, *((float *)intElm->value), 1);
            } break;
            case UI_SCROLL_FILE_PARENT: {
                V2 deltaMouseP = normalizeV2(v2_minus(state->lastMouseP, mouseP_yUp));
                intElm->xAccel = 100*deltaMouseP.x;
            } break;
            default: {
                
            }
        }
        if(wasReleased(gameButtons, BUTTON_LEFT_MOUSE)) {
            switch(intElm->type) {
                case UI_BUTTON: {
                    if(inBounds(mouseP_yUp, interactingbounds, BOUNDS_RECT)) {
                        *((bool *)intElm->value) = !(*((bool *)intElm->value));
                    }
                } break;
                case UI_SCROLL_FILE_PARENT: {
                    intElm->xVel = 0;
                } break;
                case UI_SCROLL_FILE: {
                    *intElm->viewing = &intElm->texture;
                } break;
                default: {
                    
                }
            }
            
            state->interactingWith = 0;
        }
    } else {
        if(wasPressed(gameButtons, BUTTON_LEFT_MOUSE)) {
            if(hotElm) {
                state->interactingWith = hotElm;
                setLerpInfoV4_s(&hotElm->lerpInfo, hotElm->interactColor, 
                                0.3f, &hotElm->color);
                state->grabOffset = v2_minus(mouseP_yUp, hotElmP);
            }
        }
    }
    
    state->lastMouseP = mouseP_yUp;
}