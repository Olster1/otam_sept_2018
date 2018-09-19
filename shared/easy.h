#ifdef __APPLE__ 
#define MemoryBarrier()  __sync_synchronize()
#define _ReadWriteBarrier() { SDL_CompilerBarrier(); }
#include <dirent.h>
#elif _WIN32

#endif

#if !defined arrayCount
#define arrayCount(array1) (sizeof(array1) / sizeof(array1[0]))
#endif 

#define invalidCodePathStr(msg) { printf(msg); exit(0); }

#if 1 //turn off for crash assert
#undef assert
#define assert(statement) if(!(statement)) {printf("Something went wrong at %d in %s\n", __LINE__, __FILE__);  exit(0);}
#else
#define assert(statement) if(!(statement)) { int *i_ = 0; *i_ = 0; }
#endif

#define assertStr(statement, str) if(!(statement)) { printf("%s\n", str); } assert(statement); 

#include <limits.h>

#define fori(Array) for(u32 Index = 0; Index < arrayCount(Array); ++Index)
#define fori_count(Count) for(u32 Index = 0; Index < Count; ++Index)

#define forN_(Count, Name) for(u32 Name = 0; Name < Count; Name++)
#define forN_s(Count, Name) for(s32 Name = 0; Name < Count; Name++)

#define forN(Count) forN_(Count, Count##Index)
#define forNs(Count) forN_s(Count, Count##Index)

#define PI32 3.14159265359
#define TAU32 6.283185307
#define HALF_PI32 0.5f*PI32
#define COLOR_NULL v4(0, 0, 0, 0)
#define COLOR_BLACK v4(0, 0, 0, 1)
#define COLOR_WHITE v4(1, 1, 1, 1)
#define COLOR_RED v4(1, 0, 0, 1)
#define COLOR_GREEN v4(0, 1, 0, 1)
#define COLOR_BLUE v4(0, 0, 1, 1)
#define COLOR_YELLOW v4(1, 1, 0, 1)
#define COLOR_PINK v4(1, 0, 1, 1)
#define COLOR_AQUA v4(0, 1, 1, 1)
#define COLOR_GREY v4(0.5f, 0.5f, 0.5f, 1)

#define Kilobytes(size) (size*1024)
#define Megabytes(size) (Kilobytes(size)*1024) 
#define Gigabytes(size) (Megabytes(size)*1024) 

#define zeroStruct(memory, type) zeroSize(memory, sizeof(type))
#define zeroArray(array) zeroSize(array, sizeof(array))

void zeroSize(void *memory, size_t bytes) {
    char *at = (char *)memory;
    for(int i = 0; i < bytes; i++) {
        *at = 0;
        at++;
    }
}

typedef struct {
    void *memory;
    unsigned int currentSize;
    unsigned int totalSize;
    int markCount;
} Arena;

Arena createArena(size_t size) {
    
    Arena result = {};
    result.memory = calloc(size, 1);
    result.currentSize = 0;
    result.totalSize = size;
    return result;
}

#define pushStruct(arena, type) (type *)pushSize(arena, sizeof(type))

#define pushArray(arena, size, type) (type *)pushSize(arena, sizeof(type)*size)

void *pushSize(Arena *arena, size_t size) {
    if(arena->currentSize + size > arena->totalSize){
        //TODO: handle temp memory. 
        // size_t extension = Kilobytes(1028);
        // arena->totalSize += extension;
        // arena->memory = calloc(extension, 1);
    }
    assertStr(arena->currentSize + size <= arena->totalSize, "ERROR: ran out of memory");
    
    void *result = ((char *)arena->memory) + arena->currentSize;
    arena->currentSize += size;
    
    zeroSize(result, size);
    return result;
}

typedef struct { 
    int id;
    Arena *arena;
    size_t memAt;
} MemoryArenaMark;

MemoryArenaMark takeMemoryMark(Arena *arena) {
    MemoryArenaMark result = {};
    result.arena = arena;
    result.memAt = arena->currentSize;
    result.id = arena->markCount++;
    return result;
}

void releaseMemoryMark(MemoryArenaMark *mark) {
    mark->arena->markCount--;
    assert(mark->id == mark->arena->markCount);
    assert(mark->arena->markCount >= 0);
    mark->arena->currentSize = mark->memAt;
}


bool stringsMatchN(char *a, int aLength, char *b, int bLength) {
    bool result = true;
    
    int indexCount = 0;
    while(indexCount < aLength && indexCount < bLength) {
        indexCount++;
        result &= (*a == *b);
        a++;
        b++;
    }
    result &= (indexCount == bLength && indexCount == aLength);
    
    return result;
} 


bool stringsMatchNullN(char *a, char *b, int bLen) {
    bool result = stringsMatchN(a, strlen(a), b, bLen);
    return result;
}

bool cmpStrNull(char *a, char *b) {
    bool result = stringsMatchN(a, strlen(a), b, strlen(b));
    return result;
}

char *nullTerminateBuffer(char *result, char *string, int length) {
    for(int i = 0; i < length; ++i) {
        result[i]= string[i];
    }
    result[length] = '\0';
    return result;
}

#define nullTerminate(string, length) nullTerminateBuffer((char *)malloc(length + 1), string, length)

char *concat(char *a, char *b) {
    int aLen = strlen(a);
    int bLen = strlen(b);
    
    int newStrLen = aLen + bLen + 1; // +1 for null terminator
    char *newString = (char *)calloc(newStrLen, 1); 
    newString[newStrLen - 1] = '\0';
    
    char *at = newString;
    for (int i = 0; i < aLen; ++i)
    {
        *at++ = a[i];
    }
    
    for (int i = 0; i < bLen; ++i)
    {
        *at++ = b[i];
    }
    
    return newString;
}

#define ENUM(value) value,
#define STRING(value) #value,

int findEnumValue(char *name, char **names, int nameCount) {
    int result = -1; //not found
    for(int i = 0; i < nameCount; i++) {
        if(cmpStrNull(name, names[i])) {
            result = i;
            break;
        }
    }
    return result;
}

typedef enum {
    BUTTON_NULL,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_SPACE,
    BUTTON_SHIFT,
    BUTTON_ENTER,
    BUTTON_BACKSPACE,
    BUTTON_ESCAPE,
    BUTTON_LEFT_MOUSE,
    BUTTON_RIGHT_MOUSE,
    BUTTON_1,
    BUTTON_F1,
    BUTTON_Z,
    BUTTON_COMMAND,
    BUTTON_TILDE,
    //
    BUTTON_COUNT
} ButtonType;


typedef struct {
    bool isDown;
    int transitionCount;
} GameButton;

#define INPUT_BUFFER_SIZE 1028
typedef struct {
    char chars[INPUT_BUFFER_SIZE];
    unsigned int length;
    int cursorAt;
} InputBuffer;



#define wasPressed(buttonArray, index) (buttonArray[index].isDown && buttonArray[index].transitionCount != 0)  

#define wasReleased(buttonArray, index) (!buttonArray[index].isDown && buttonArray[index].transitionCount != 0)  

#define isDown(buttonArray, index) (buttonArray[index].isDown)  

void sdlProcessGameKey(GameButton *button, bool isDown, bool repeated) {
    button->isDown = isDown;
    if(!repeated) {
        button->transitionCount++;
    }
}

void splice(InputBuffer *buffer, char *string, bool addString) { //if false will remove string
    char tempChars[INPUT_BUFFER_SIZE] = {};
    int tempCharCount = 0;
    
    char *at = string;
    
    //copy characters
    for(int i = buffer->cursorAt; i < buffer->length; ++i) {
        tempChars[tempCharCount++] = buffer->chars[i]; 
    }
    
    while(*at) {
        
        if(addString > 0) { //adding
            
            if(buffer->length < (sizeof(buffer->chars) - 1)) {
                
                buffer->chars[buffer->cursorAt++] = *at;
                buffer->length++;
            }
            
        } else {
            if(buffer->length) {
                buffer->chars[buffer->cursorAt--] = *at;
                buffer->length--;
            }
        }
        
        at++;
    }
    //replace characters
    for(int i = 0; i < tempCharCount; ++i) {
        buffer->chars[buffer->cursorAt + i] = tempChars[i]; 
    }
    assert(buffer->length < arrayCount(buffer->chars));
    buffer->chars[buffer->length] = '\0'; //null terminate buffer
}

//TODO: Make this more robust TODO: I don't think this is neccessary??
char *getResPathFromExePath(char *exePath, char *folderName) {
    assert(!"invalid path");
    // unsigned int execPathLength = strlen(exePath) + 1; //for the null terminate character
    
    // char *at = exePath;
    // char *mostRecent = at;
    // while(*at) {
    //     if(*at == '/' && at[1]) { //don't collect last slash
    //         mostRecent = at;
    //     }
    //     *dest = *at;
    //     at++;
    //     dest++;
    // }
    // int indexAt = (int)(mostRecent - exePath) + 1; //plus one to keep the slash
    // resPath[indexAt] = 'r';
    // resPath[indexAt + 1] = 'e';
    // resPath[indexAt + 2] = 's';
    // resPath[indexAt + 3] = '/';
    // resPath[indexAt + 4] = '\0';
    // assert(strlen(resPath) <= execPathLength);

    return 0;
}

char *lastFilePortion(char *at) {
    // TODO(Oliver): Make this more robust
    char *recent = at;
    while(*at) {
        if(*at == '/' && at[1] != '\0') { 
            recent = (at + 1); //plus 1 to pass the slash
        }
        at++;
    }
    
    int length = (int)(at - recent);
    char *result = (char *)calloc(length, 1);
    
    memcpy(result, recent, length);
    
    return result;
}
