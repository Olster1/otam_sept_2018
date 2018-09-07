/*
File to enable writing with fonts easy to add to your project. Uses Sean Barret's stb_truetype file to load and write the fonts. Requires a current openGL context. 
*/

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"

static bool globalFontImmediate = false; //This is if we want to output text without going through a render buffer

typedef struct {
    int index;
    V4 color;
} CursorInfo;

typedef struct FontSheet FontSheet;
typedef struct FontSheet {
    stbtt_bakedchar *cdata;
    GLuint handle;
    int minText;
    int maxText;

    FontSheet *next;

} FontSheet;

typedef struct {
    int fontHeight;
    FontSheet *sheets;
} Font;

FontSheet *findFontSheet(Font *font, unsigned int character) {
    FontSheet *sheet = font->sheets;
    FontSheet *result = 0;
    while(sheet) {
        if(sheet->minText >= character && sheet->maxText < sheet->maxText) {
            result = sheet;
            break;
        }
        sheet = sheet->next;
    }

    return result;
}

#define FONT_SIZE 512 //I think this can be any reasonable number, it doesn't affect the size of the font. Maybe the clarity of the glyphs? 
Font initFont_(char *fileName, int firstChar, int endChar, int fontHeight)
{
    Font result = {}; 
    result.fontHeight = fontHeight;
    FontSheet *sheet = result.sheets = (FontSheet *)calloc(sizeof(FontSheet), 1);
    sheet->minText = firstChar;
    sheet->maxText = endChar;
    int bitmapW = FONT_SIZE;
    int bitmapH = FONT_SIZE;
    int numOfPixels = bitmapH*bitmapW;
    unsigned char temp_bitmap[numOfPixels]; //bakfontbitmap is one byte per pixel. 
    
    FILE *fileHandle = fopen(fileName, "rb");
    size_t fileSize = getFileSize(fileHandle);
    
    unsigned char *ttf_buffer = (unsigned char *)calloc(fileSize, 1);
    
    fread(ttf_buffer, 1, fileSize, fileHandle);
    
    int numOfChars = endChar - firstChar;
    //TODO: do we want to use an arena for this?
    sheet->cdata = (stbtt_bakedchar *)calloc(numOfChars*sizeof(stbtt_bakedchar), 1);
    //
    
    stbtt_BakeFontBitmap(ttf_buffer, 0, fontHeight, temp_bitmap, bitmapW, bitmapH, firstChar, numOfChars, sheet->cdata);
    // no guarantee this fits!
    
    free(ttf_buffer);
    // NOTE(Oliver): We expand the the data out from 8 bits per pixel to 32 bits per pixel. It doens't matter that the char data is based on the smaller size since getBakedQuad divides by the width of original, smaller bitmap. 
    
    // TODO(Oliver): can i free this once its sent to the graphics card? 
    unsigned int *bitmapTexture = (unsigned int *)calloc(sizeof(unsigned int)*numOfPixels, 1);
    unsigned char *src = temp_bitmap;
    unsigned int *dest = bitmapTexture;
    for(int y = 0; y < bitmapH; ++y) {
        for(int x = 0; x < bitmapW; ++x) {
            unsigned int alpha = *src++;
            *dest = 
                alpha << 24 |
                alpha << 16 |
                alpha << 8 |
                alpha << 0;
            dest++;
        }
    }
    #if FIXED_FUNCTION_PIPELINE
        glGenTextures(1, &result.handle);
        glBindTexture(GL_TEXTURE_2D, result.handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); 
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FONT_SIZE, FONT_SIZE, 0, GL_BGRA, GL_UNSIGNED_BYTE, bitmapTexture);
        glBindTexture(GL_TEXTURE_2D, 0);
    #else 
        sheet->handle = openGlLoadTexture(FONT_SIZE, FONT_SIZE, bitmapTexture);
    #endif
        free(bitmapTexture);
    return result;
}



Font initFont(char *fileName, int fontHeight) {
    Font result = initFont_(fileName, 32, 126, fontHeight); // ASCII 32..126 is 95 glyphs
    return result;
}

typedef struct {
    GLuint texHandle;
    //TODO: Can we make this file not depend on Array_Dynamic??
    InfiniteAlloc vertexData;
    InfiniteAlloc indicesData;
    V4 color;

    int quadVertexAt; //stored to keep track of quad
    int indexAt;

    float size;

    float bufferHeight;

} FontDrawState;

void pushFontGlyphQuad(FontDrawState *drawState, float u, float t, float x, float y, float z) {
    Vertex vertex = {};
    vertex.position = v3(x, y, z);
    vertex.position = transformPositionV3(vertex.position, mat4TopLeftToBottomLeft(drawState->bufferHeight));
    vertex.texUV = v2(u, t);
    vertex.color = drawState->color;
    unsigned int index = drawState->vertexData.count;
    addElementInifinteAlloc_(&drawState->vertexData, &vertex);

    addElementInifinteAlloc_(&drawState->indicesData, &index);
    assert(drawState->indicesData.count > 0);
    if(drawState->quadVertexAt == 3) {
        assert(drawState->indicesData.count >= 4);
        unsigned int *data = (unsigned int *)drawState->indicesData.memory;
        assert(drawState->indicesData.count - 4 >= 0);
        unsigned int index1 = data[drawState->indicesData.count - 4];
        unsigned int index2 = data[drawState->indicesData.count - 2];
        addElementInifinteAlloc_(&drawState->indicesData, &index1);
        addElementInifinteAlloc_(&drawState->indicesData, &index2);
        drawState->quadVertexAt = 0;
    } else {
        drawState->quadVertexAt++;
        assert(drawState->quadVertexAt <= 3);
    }
}

FontDrawState beginFontGlyphs(GLuint texHandle, V4 color, float bufferHeight, float size) {
    FontDrawState result = {};
    result.texHandle = texHandle;
    result.vertexData = initInfinteAlloc(Vertex);
    result.indicesData = initInfinteAlloc(unsigned int);
    result.bufferHeight = bufferHeight; 
    result.color = color;
    result.size = size;
    return result;
}

void endFontGlyphs(FontDrawState *drawState, int bufferWidth, int bufferHeight) {
    #if FIXED_FUNCTION_PIPELINE
    assert(!"Not implemented");
    #else 
    unsigned int *at = (unsigned int *)drawState->indicesData.memory;
    Matrix4 orthoMatrix = OrthoMatrixToScreen(bufferWidth, bufferHeight, 1);
    //orthoMatrix = Matrix4_scale(orthoMatrix, v3(drawState->size, drawState->size, 1));
    if(globalFontImmediate) {
        loadVertices(0, (Vertex *)drawState->vertexData.memory, drawState->vertexData.count, (unsigned int *)drawState->indicesData.memory, drawState->indicesData.count, &textureProgram, SHAPE_TEXTURE, drawState->texHandle, orthoMatrix, 1, drawState->color);    
    } else {
        pushRenderItem(0, &globalRenderGroup, (Vertex *)drawState->vertexData.memory, drawState->vertexData.count, (unsigned int *)drawState->indicesData.memory, drawState->indicesData.count, &textureProgram, SHAPE_TEXTURE, drawState->texHandle, orthoMatrix, 1, drawState->color, -1);    
    }
    
    releaseInfiniteAlloc(&drawState->vertexData);
    releaseInfiniteAlloc(&drawState->indicesData);
    #endif
}

typedef struct {
    stbtt_aligned_quad q;
    int index;
    V2 lastXY;
} GlyphInfo;

//This does unicode now 
Rect2f my_stbtt_print_(Font *font, float x, float y, float bufferWidth, float bufferHeight, unsigned int *text_, Rect2f margin, V4 color, float size, CursorInfo *cursorInfo, bool display) {
    Rect2f bounds = InverseInfinityRect2f();
    
    if(bounds.min.x > x) { x = bounds.min.x; }
    //if(bounds.min.y > y) { y = bounds.min.x; }
#if FIXED_FUNCTION_PIPELINE
    glMatrixMode(GL_MODELVIEW);
#endif
    float modelMatrix[] = {
        size,  0,  0,  0, 
        0, size,  0,  0, 
        0, 0,  1,  0, 
        0, 0, 0,  1
    };
#if FIXED_FUNCTION_PIPELINE
    glLoadMatrixf(modelMatrix); 
    
    glColor4f(color.x, color.y, color.z, color.w);
#endif
    float width = 0; 
    float height = 0;
    V2 pos = v2(0, 0);
    
    //For individual text boxes: DEBUG
    // Rect2f points[512] = {};
    // int pC = 0;
    
    // V2 points1[512] = {};
    // int pC1 = 0;
    
    GlyphInfo qs[256] = {};
    int quadCount = 0;
    
    unsigned int *text = text_;
#if FIXED_FUNCTION_PIPELINE
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font->handle);

    glBegin(GL_QUADS);
#else 
    //THIS IS A HACK!
    FontSheet *sheet = font->sheets;//findFontSheet(Font *font, unsigned int character)
    assert(sheet);
    //////
    FontDrawState drawState = beginFontGlyphs(sheet->handle, color, bufferHeight, size);
#endif
    V2 lastXY = v2(x, y);
    V2 startP = lastXY;
#define transformToSizeX(val) ((val - startP.x)*size + startP.x)
#define transformToSizeY(val) ((val - startP.y)*size + startP.y)
    bool inWord = false;
    bool hasBeenToNewLine = false;
    unsigned int *tempAt = text;
    while (*text) {
        bool increment = true;
        bool drawText = false;
        //points1[pC1++] = v2(x, y);
        
        if (((int)(*text) >= sheet->minText && (int)(*text) < sheet->maxText) && *text != '\n') {
            stbtt_aligned_quad q  = {};

            stbtt_GetBakedQuad(sheet->cdata, FONT_SIZE, FONT_SIZE, *text - sheet->minText, &x,&y,&q, 1);
            assert(quadCount < arrayCount(qs));
            GlyphInfo *glyph = qs + quadCount++; 
            glyph->q = q;
            glyph->index = (int)(text - text_);
            glyph->lastXY = lastXY;;
        }
        
        bool overflowed = (transformToSizeX(x) > margin.maxX && inWord);
        
        if(*text == ' ' || *text == '\n') {
            inWord = false;
            hasBeenToNewLine = false;
            drawText = true;
        } else {
            inWord = true;
        }
        
        if(overflowed && !hasBeenToNewLine && inWord) {
            text = tempAt;
            increment = false;
            quadCount = 0;  //empty the quads. We lose the data because we broke the word.  
            hasBeenToNewLine = true;
        }
        
        if(overflowed || *text == '\n') {
            x = margin.minX;
            y += size*font->fontHeight; 
        }
        
        bool lastCharacter = (*(text + 1) == '\0');
        
        if(drawText || lastCharacter) {
            for(int i = 0; i < quadCount; ++i) {
                GlyphInfo *glyph = qs + i;
                stbtt_aligned_quad q = glyph->q;
                q.x0 = ((q.x0 - startP.x)*size) + startP.x;
                q.y0 = ((q.y0 - startP.y)*size) + startP.y;
                q.y1 = ((q.y1 - startP.y)*size) + startP.y;
                q.x1 = ((q.x1 - startP.x)*size) + startP.x;
                Rect2f b = rect2f(q.x0, q.y0, q.x1, q.y1);
                bounds = unionRect2f(bounds, b);
                //points[pC++] = b;
                
                if(display) {
#if FIXED_FUNCTION_PIPELINE
                    glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, -1*q.y1 + bufferHeight);
                    glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1, -1*q.y1 + bufferHeight);
                    glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1, -1*q.y0 + bufferHeight);
                    glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0, -1*q.y0 + bufferHeight);
#else 
                    pushFontGlyphQuad(&drawState, q.s0, q.t1, q.x0, q.y1, -1);
                    pushFontGlyphQuad(&drawState, q.s1, q.t1, q.x1, q.y1, -1);
                    pushFontGlyphQuad(&drawState, q.s1, q.t0, q.x1, q.y0, -1);
                    pushFontGlyphQuad(&drawState, q.s0, q.t0, q.x0, q.y0, -1);

#endif
                }
                if(cursorInfo && (glyph->index == cursorInfo->index)) {
                    width = q.x1 - q.x0;
                    height = 32*size;//q.y1 - q.y0;
                    pos = v2(transformToSizeX(glyph->lastXY.x) + 0.5f*width, transformToSizeY(glyph->lastXY.y) - 0.5f*height);
                }
                
            }
            
            quadCount = 0; //empty the quads now that we have finished drawing them
        }
        
        lastXY.x = x;
        lastXY.y = y;
        if(increment) {
            ++text;
        }
        if(!inWord) {
            tempAt = text;
        }
    }
#if FIXED_FUNCTION_PIPELINE
    glEnd();
    glLoadIdentity();
#else 
    endFontGlyphs(&drawState, bufferWidth, bufferHeight);
#endif
    
    if(cursorInfo && (int)(text - text_) <= cursorInfo->index) {
        width = size*16;
        height = size*32;
        
        pos = v2(transformToSizeX(x) + 0.5f*width, transformToSizeY(y) - 0.5f*height);
    }
    
    if(cursorInfo) {
        openGlDrawRectCenterDim(0, v2ToV3(pos, -1), v2(width, height), cursorInfo->color, 0, mat4TopLeftToBottomLeft(bufferHeight), 1, OrthoMatrixToScreen(bufferWidth, bufferHeight, 1));
    }
#if 0 //draw idvidual text boxes
    for(int i = 0; i < pC; ++i) {
        Rect2f b = points[i];
        V2 d = points1[i];
        
        float w = b.maxX - b.minX;
        float h = b.maxY - b.minY;
        V2 c = v2(b.minX + 0.5f*w, b.minY + 0.5f*h);
        openGlDrawRectOutlineCenterDim(c, v2(w, h), v4(1, 0, 0, 1), 0, v2(1, -1), v2(0, bufferHeight));
        
    }
#endif
#if 0 //draw text bounds
    openGlDrawRectOutlineCenterDim(getCenter(bounds), getDim(bounds), v4(1, 0, 0, 1), 0, v2(1, -1), v2(0, bufferHeight));
#endif
    
    if(text == text_) { //there wasn't any text
        bounds = rect2f(0, 0, 0, 0);
    }
    
    return bounds;
    
}

Rect2f outputText_with_cursor(Font *font, float x, float y, float bufferWidth, float bufferHeight, unsigned int *text, Rect2f margin, V4 color, float size, int cursorIndex, V4 cursorColor, bool display) {
    CursorInfo cursorInfo = {};
    cursorInfo.index = cursorIndex;
    cursorInfo.color = cursorColor;
    
    Rect2f result = my_stbtt_print_(font, x, y, bufferWidth, bufferHeight, text, margin, color, size, &cursorInfo, display);
    return result;
}

Rect2f outputText(Font *font, float x, float y, float bufferWidth, float bufferHeight, unsigned int *text, Rect2f margin, V4 color, float size, bool display) {
    Rect2f result = my_stbtt_print_(font, x, y, bufferWidth, bufferHeight, text, margin, color, size, 0, display);
    return result;
}

Rect2f outputTextWithLength(Font *font, float x, float y, float bufferWidth, float bufferHeight, unsigned int *allText, int textLength, Rect2f margin, V4 color, float size, bool display) {
    unsigned int *text = (unsigned int *)malloc(sizeof(unsigned int)*(textLength + 1));
    for(int i = 0; i < textLength; ++i) {
        text[i] = allText[i];
    }

    text[textLength] = '\0'; //null terminate. 
    Rect2f result = my_stbtt_print_(font, x, y, bufferWidth, bufferHeight, text, margin, color, size, 0, display);
    free(text);
    return result;
}

V2 getBounds(unsigned int *string, Rect2f margin, Font *font, float size) {
    Rect2f bounds  = outputText(font, margin.minX, margin.minY, bufferWidth, bufferHeight, string, margin, v4(0, 0, 0, 1), size, false);
    
    V2 result = getDim(bounds);
    return result;
}


Rect2f getBoundsRectf(unsigned int *string, float xAt, float yAt, Rect2f margin, Font *font, float size) {
    Rect2f bounds  = outputText(font, xAt, yAt, bufferWidth, bufferHeight, string, margin, v4(0, 0, 0, 1), size, false);
    
    return bounds;
}

