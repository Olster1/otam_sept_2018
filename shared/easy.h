#ifdef __APPLE__ 
#define MemoryBarrier()  __sync_synchronize()
#define _ReadWriteBarrier() { SDL_CompilerBarrier(); }
#include <dirent.h>
#elif _WIN32

#endif

typedef struct {
    void *Data;
    bool HasErrors;
    u64 fileOffsetAt;
}  LoggerFileHandle;

static LoggerFileHandle globalLoggerHandle;

void EasyLogInfo(LoggerFileHandle *Handle, void *Memory, size_t Size, int Offset)
{
    Handle->HasErrors = false;
    SDL_RWops *FileHandle = (SDL_RWops *)Handle->Data;
    if(!Handle->HasErrors)
    {
        Handle->HasErrors = true; 
        
        if(FileHandle)
        {
            
            if(SDL_RWseek(FileHandle, Offset, RW_SEEK_SET) >= 0)
            {
                if(SDL_RWwrite(FileHandle, Memory, 1, Size) == Size)
                {
                    Handle->HasErrors = false;
                }
            }
        }
    }    
}

static inline void EasyOpenLogger() {
    LoggerFileHandle Result = {};

    char timeStampBuffer[2018] = {};
    sprintf(timeStampBuffer, "%lu.txt", (unsigned long)time(NULL)); 
    char *logFileName = "/LogFile_";
    int newStrLen = strlen(globalExeBasePath) + strlen(logFileName) + strlen(timeStampBuffer) + 1; // +1 for null terminator
    char *fileName = (char *)calloc(newStrLen, 1); 

    sprintf(fileName, "%s%s%s", globalExeBasePath, logFileName, timeStampBuffer);
    
    SDL_RWops* FileHandle = SDL_RWFromFile(fileName, "w+");
    
    if(FileHandle)
    {
        Result.Data = FileHandle;
    }
    else
    {
        Result.HasErrors = true;
    }
    
    globalLoggerHandle = Result;
}

void EasyCloseLogger()
{
    SDL_RWops*  FileHandle = (SDL_RWops* )globalLoggerHandle.Data;
    if(FileHandle) {
        SDL_RWclose(FileHandle);
    }
}

#if DEVELOPER_MODE
#define calloc(size, item) malloc(size)
#endif

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

float max(float a, float b) {
    float result = (a < b) ? b : a;
    return result;
}

float min(float a, float b) {
    float result = (a > b) ? b : a;
    return result;
}

#ifdef _WIN32
//printf doens't print to the console annoyingly!!
#define printf(...) {char str[256]; sprintf_s(str, __VA_ARGS__); OutputDebugString(str); }
#endif

#if !defined arrayCount
#define arrayCount(array1) (sizeof(array1) / sizeof(array1[0]))
#endif 

#define invalidCodePathStr(msg) { printf(msg); exit(0); }
#if !DEVELOPER_MODE //turn off for crash EasyAssert
// #define EasyAssert(statement) if(!(statement)) { int *a = 0; a = 0;}
#define EasyAssert(statement) if(!(statement)) {printf("Something went wrong at %d in %s\n", __LINE__, __FILE__);  int *a = 0; *a = 0;}
#define EasyAssertStr(statement, str) if(!(statement)) { printf("%s\n", str); } EasyAssert(statement); 
#else
#define EasyAssert(statement) if(!(statement)) { if(!globalLoggerHandle.Data) { EasyOpenLogger(); } char charBuffer[1028]; sprintf(charBuffer, "%d %s", __LINE__, __FILE__); int sizeToWrtite = strlen(charBuffer)*sizeof(u8); EasyLogInfo(&globalLoggerHandle, charBuffer, sizeToWrtite, globalLoggerHandle.fileOffsetAt); globalLoggerHandle.fileOffsetAt += sizeToWrtite;}
#define EasyAssertStr(statement, str)
#endif

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

typedef struct MemoryPiece MemoryPiece;
typedef struct MemoryPiece {
    void *memory;
    size_t totalSize; //size of just this memory block
    // size_t totalSizeOfArena; //size of total arena to roll back with
    size_t currentSize;

    MemoryPiece *next;

} MemoryPiece; //this is for the memory to remember 

typedef struct {
    //NOTE: everything is in pieces now
    // void *memory;
    // unsigned int totalSize; //include all memory blocks
    // unsigned int totalCurrentSize;//total current size of all memory blocks
    int markCount;

    MemoryPiece *pieces; //actual details in the memory block
    MemoryPiece *piecesFreeList;

} Arena;

#define pushStruct(arena, type) (type *)pushSize(arena, sizeof(type))

#define pushArray(arena, size, type) (type *)pushSize(arena, sizeof(type)*size)

void *pushSize(Arena *arena, size_t size) {
    if(!arena->pieces || ((arena->pieces->currentSize + size) > arena->pieces->totalSize)){ //doesn't fit in arena
        MemoryPiece *piece = arena->piecesFreeList; //get one of the free list

        size_t extension = max(Kilobytes(1028), size);
        if(piece)  {
            MemoryPiece **piecePtr = &arena->piecesFreeList;
            EasyAssert(piece->totalSize > 0);
            while(piece && piece->totalSize < extension) {//find the right size piece. 
                piecePtr = &piece->next; 
                piece = piece->next;
            }
            if(piece) {
                //take off list
                *piecePtr = piece->next;             
                piece->currentSize = 0;
            }
            
        } 

        if(!piece) {//need to allocate a new piece
            piece = (MemoryPiece *)calloc(sizeof(MemoryPiece), 1);
            piece->memory = calloc(extension, 1);
            piece->totalSize = extension;
            piece->currentSize = 0;
        }
        EasyAssert(piece);
        EasyAssert(piece->memory);
        EasyAssert(piece->totalSize > 0);
        EasyAssert(piece->currentSize == 0);

        //stick on list
        piece->next = arena->pieces;
        arena->pieces = piece;

        // piece->totalSizeOfArena = arena->totalSize;
        // EasyAssert((arena->currentSize_ + size) <= arena->totalSize); 
    }

    MemoryPiece *piece = arena->pieces;

    EasyAssert(piece);
    EasyAssert((piece->currentSize + size) <= piece->totalSize); 
    
    void *result = ((u8 *)piece->memory) + piece->currentSize;
    piece->currentSize += size;
    
    zeroSize(result, size);
    return result;
}

Arena createArena(size_t size) {
    Arena result = {};
    pushSize(&result, size);
    EasyAssert(result.pieces);
    EasyAssert(result.pieces->memory);
    return result;
}

typedef struct { 
    int id;
    Arena *arena;
    size_t memAt; //the actuall value we roll back, don't need to do anything else
    MemoryPiece *piece;
} MemoryArenaMark;

MemoryArenaMark takeMemoryMark(Arena *arena) {
    MemoryArenaMark result = {};
    result.arena = arena;
    result.memAt = arena->pieces->currentSize;
    result.id = arena->markCount++;
    result.piece = arena->pieces;
    return result;
}

void releaseMemoryMark(MemoryArenaMark *mark) {
    mark->arena->markCount--;
    Arena *arena = mark->arena;
    EasyAssert(mark->id == arena->markCount);
    EasyAssert(arena->markCount >= 0);
    EasyAssert(arena->pieces);
    //all ways the top piece is the current memory block for the arena. 
    MemoryPiece *piece = arena->pieces;
    if(mark->piece != piece) {
        //not on the same memory block
        bool found = false;
        while(!found) {
            piece = arena->pieces;
            if(piece == mark->piece) {
                //found the right one
                found = true;
                break;
            } else {
                arena->pieces = piece->next;
                EasyAssert(arena->pieces);
                //put on free list
                piece->next = arena->piecesFreeList;
                arena->piecesFreeList = piece;
            }
        }
        EasyAssert(found);
    } 
    EasyAssert(arena->pieces == mark->piece);
    //roll back size
    piece->currentSize = mark->memAt;
    EasyAssert(piece->currentSize <= piece->totalSize);
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
    EasyAssert(at == &newString[newStrLen - 1])
    EasyAssert(newString[newStrLen - 1 ] == '\0');
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
    BUTTON_2,
    BUTTON_3,
    BUTTON_4,
    BUTTON_5,
    BUTTON_6,
    BUTTON_7,
    BUTTON_8,
    BUTTON_9,
    BUTTON_F1,
    BUTTON_Z,
    BUTTON_R,
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
        
        if(addString) { //adding
            
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
    EasyAssert(buffer->length < arrayCount(buffer->chars));
    buffer->chars[buffer->length] = '\0'; //null terminate buffer
}

//TODO: Make this more robust TODO: I don't think this is neccessary??
char *getResPathFromExePath(char *exePath, char *folderName) {
    EasyAssert(!"invalid path");
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
    // EasyAssert(strlen(resPath) <= execPathLength);
    
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
    EasyAssert(result[length - 1] == '\0');
    
    return result;
}
