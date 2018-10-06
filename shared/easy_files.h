/*
TODO: Merge the other file stuff into here
Have defines for platform i.e. not named sdl 
*/

typedef struct {
    bool valid;
    size_t fileSize;
    unsigned char *memory;
} FileContents;

size_t getFileSize(FILE *handle) {
    size_t result = 0;
    if(fseek(handle, 0, SEEK_END) == 0) {
        result = ftell(handle);
    }
    
    fseek(handle, 0, SEEK_SET);
    return result;
}

char *getFileExtension(char *fileName) {
    char *result = fileName;
    
    bool hasDot = false;
    while(*fileName) {
        if(*fileName == '.') { 
            result = fileName + 1;
            hasDot = true;
        }
        fileName++;
    }
    
    if(!hasDot) {
        result = 0;
    }
    
    return result;
}

char *getFileLastPortion_(char *buffer, int bufferLen, char *at) {
    char *recent = at;
    while(*at) {
        if(*at == '/' && at[1] != '\0') { 
            recent = (at + 1); //plus 1 to pass the slash
        }
        at++;
    }
    
    char *result = buffer;
    int length = (int)(at - recent) + 1; //for null termination
    if(!result) {
        result = (char *)calloc(length, 1);    
    } else {
        assert(bufferLen >= length);
        buffer[length] = '\0'; //null terminate. 
    }
    
    memcpy(result, recent, length - 1);
    
    return result;
}
#define getFileLastPortion(at) getFileLastPortion_(0, 0, at)
#define getFileLastPortionWithBuffer(buffer, bufferLen, at) getFileLastPortion_(buffer, bufferLen, at)

char *getFileLastPortionWithoutExtension(char *name) {
    char *lastPortion = getFileLastPortion(name);
    char *at = lastPortion;
    while(*at) {
        if(*at == '.') { 
            break;
        }
        at++;
    }
    
    int length = (int)(at - lastPortion) + 1; //for null termination
    char *result = (char *)calloc(length, 1);
    
    memcpy(result, lastPortion, length - 1 );

    free(lastPortion);
    return result;
}

typedef struct {
    char *names[256]; //max 32 files
    int count;
} FileNameOfType;

bool isInCharList(char *ext, char **exts, int count) {
    bool result = false;
    for(int i = 0; i < count; i++) {
        if(cmpStrNull(ext, exts[i])) {
            result = true;
            break;
        }
    }
    return result;
}

typedef struct {
    void *Data;
    bool HasErrors;
}  game_file_handle;

size_t sdl_GetFileSize(SDL_RWops *FileHandle) {
    long Result = SDL_RWseek(FileHandle, 0, RW_SEEK_END);
    if(Result < 0) {
        assert(!"Seek Error");
    }
    if(SDL_RWseek(FileHandle, 0, RW_SEEK_SET) < 0) {
        assert(!"Seek Error");
    }
    return (size_t)Result;
}

game_file_handle platformBeginFileRead(char *FileName)
{
    game_file_handle Result = {};
    
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "r+");
    
    if(FileHandle)
    {
        Result.Data = FileHandle;
    }
    else
    {
        Result.HasErrors = true;
        printf("%s\n", SDL_GetError());
    }
    
    return Result;
}

game_file_handle platformBeginFileWrite(char *FileName)
{
    game_file_handle Result = {};
    
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "w+");
    
    if(FileHandle)
    {
        Result.Data = FileHandle;
    }
    else
    {
        const char* Error = SDL_GetError();
        Result.HasErrors = true;
    }
    
    return Result;
}

void platformEndFile(game_file_handle Handle)
{
    SDL_RWops*  FileHandle = (SDL_RWops* )Handle.Data;
    if(FileHandle) {
        SDL_RWclose(FileHandle);
    }
}

void platformWriteFile(game_file_handle *Handle, void *Memory, size_t Size, int Offset)
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
                else
                {
                    assert(!"write file did not succeed");
                }
            }
        }
    }    
}

FileContents platformReadFile(game_file_handle Handle, void *Memory, size_t Size, int Offset)
{
    FileContents Result = {};
    
    SDL_RWops* FileHandle = (SDL_RWops*)Handle.Data;
    if(!Handle.HasErrors)
    {
        if(FileHandle)
        {
            if(SDL_RWseek(FileHandle, Offset, RW_SEEK_SET) >= 0)
            {
                if(SDL_RWread(FileHandle, Memory, 1, Size) == Size)
                {
                    Result.memory = (unsigned char *)Memory;
                    Result.fileSize = Size;
                    Result.valid = true;
                }
                else
                {
                    assert(!"Read file did not succeed");
                    Result.valid = false;
                }
            }
        }
    }    
    return Result;
    
}
size_t platformFileSize(char *FileName)
{
    size_t Result = 0;
    
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "rb");
    
    if(FileHandle)
    {
        Result = sdl_GetFileSize(FileHandle);
        SDL_RWclose(FileHandle);
    }
    
    return Result;
}

bool platformDoesFileExist(char *FileName) {
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "r+");
        
    bool result = false;    
    if(FileHandle) { 
        SDL_RWclose(FileHandle);
        result = true; 
    }

    

    return result;
}

FileContents platformReadEntireFile(char *FileName, bool nullTerminate) {
    FileContents Result = {};
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "r+");
    
    if(FileHandle)
    {
        size_t allocSize = Result.fileSize = sdl_GetFileSize(FileHandle);

        if(nullTerminate) { allocSize += 1; }

        Result.memory = (unsigned char *)calloc(allocSize, 1);
        size_t ReturnSize = SDL_RWread(FileHandle, Result.memory, 1, Result.fileSize);
        if(ReturnSize == Result.fileSize)
        {
            if(nullTerminate) {
                Result.memory[Result.fileSize] = '\0'; // put at the end of the file
                Result.fileSize += 1;
            }
            Result.valid = true;
            //NOTE(Oliver): Successfully read
        } else {
            assert(!"Couldn't read file");
            Result.valid = false;
            free(Result.memory);
        }
        SDL_RWclose(FileHandle);
    } else {
        Result.valid = false;
        const char *Error = SDL_GetError();
        printf("%s\n", Error);
        assert(!"Couldn't open file");
    }
    return Result;
}

static inline FileContents getFileContentsNullTerminate(char *fileName) {
    FileContents result = platformReadEntireFile(fileName, true);
    return result;
}

static inline FileContents getFileContents(char *fileName) {
    FileContents result = platformReadEntireFile(fileName, false);
    return result;
}

void platformDeleteFile(char *fileName) {
#ifdef __APPLE__ 
    if(remove(fileName) != 0) {
        assert(!"couldn't delete file");
    }
#elif _WIN32
    if(DeleteFileA(fileName) == 0) {
        assert(!"couldn't delete file");
    }
#else 
    assert(!"not implemented")
#endif
}

#ifdef __APPLE__ 
#include <errno.h>
#include <sys/stat.h> //for mkdir S_IRWXU
#elif _WIN32
#include <shlwapi.h>
#endif

bool platformCreateDirectory(char *fileName) {
    bool result = false;
#ifdef __APPLE__
    DIR* dir = opendir(fileName);
    if (dir) {
        closedir(dir);
    } else if (ENOENT == errno) {
        if(mkdir(fileName, S_IRWXU) == -1) {
            assert(!"couldn't create directory");
        }
        result = true;
    } else {
        assert(!"something went wrong");
    }
#elif _WIN32
    if (CreateDirectory(fileName, NULL) == 0) {
        result = true;
    } else {
        assert(!"couldn't create directory");
    }
#endif
    return result;

}


bool platformDoesDirectoryExist(char *fileName) {
    bool result = false;
#ifdef __APPLE__
    DIR* dir = opendir(fileName);
    if(dir) {
        result = true;
        closedir(dir);
    }
#elif _WIN32
    WIN32_FIND_DATAA fileFindData;
    HANDLE dirHandle = FindFirstFileA(fileName, &fileFindData);
    if(dirHandle != INVALID_HANDLE_VALUE) { 
        result = true;
        FindClose(dirHandle);
    }
#endif
    
    return result;
}



typedef enum {
    DIR_FIND_FILE_TYPE,
    DIR_DELETE_FILE_TYPE,
    DIR_FIND_DIR_TYPE,
    DIR_COPY_FILE_TYPE,
} DirTypeOperation;

char *platformGetUniqueDirName(char *dirName) {
    char timeStampBuffer[256] = {};
    sprintf(timeStampBuffer, "%lu/", (unsigned long)time(NULL)); 
    char *newDirName = concat(dirName, timeStampBuffer);
    return newDirName;
}

//Creates last folder if doesn't exist, but note recursive
void platformCopyFile(char *fileName, char *copyDir) {
    FileContents contents = platformReadEntireFile(fileName, false);
    assert(contents.valid);
    
    platformCreateDirectory(copyDir);
    {
        char *lastPortion = getFileLastPortionWithoutExtension(fileName);
        char *extension = getFileExtension(fileName);
        char *copyName = concat(copyDir, lastPortion);
        char *copyName1 = concat(copyName, "_copy.");
        char *copyName2 = concat(copyName1, extension);

        game_file_handle handle = platformBeginFileWrite(copyName2);
        platformWriteFile(&handle, contents.memory, contents.fileSize, 0);
        platformEndFile(handle);

        assert(!handle.HasErrors);

        free(copyName);
        free(copyName1);
        free(copyName2);
        free(lastPortion);
        
    } 

}

#ifdef _WIN32
static HANDLE FindFirstFileInDir(char *dirName, WIN32_FIND_DATAA *fileFindData) {
    char fileName[MAX_PATH] = "";
    /* remove '..' sections in path name */
    if (PathCanonicalizeA(fileName, dirName) == FALSE)
        return INVALID_HANDLE_VALUE;
    /* add directory separator + * to get files in the directory */
    if (strlen(fileName) + 2 > MAX_PATH)
        return INVALID_HANDLE_VALUE;
    if (fileName[strlen(fileName)-1] != '/')
        strcat(fileName, "/");
    strcat(fileName, "*");
    return FindFirstFileA(fileName, fileFindData);
}
#endif

FileNameOfType getDirectoryFilesOfType_(char *dirName, char *copyDir, char **exts, int count, DirTypeOperation opType) { 
    FileNameOfType fileNames = {};
    #ifdef __APPLE__
        DIR *directory = opendir(dirName);
        if(directory) {
            struct dirent *dp = 0;
    #elif _WIN32
        WIN32_FIND_DATAA fileFindData;
        HANDLE dirHandle = FindFirstFileInDir(dirName, &fileFindData);
        if(dirHandle != INVALID_HANDLE_VALUE) {
            BOOL findResult = true;
            bool firstTurn = true;
    #else 
        assert(!"not implemented");
    #endif
               do {
                
#if __APPLE__
                dp = readdir(directory);
                if (dp) {
                    char *name = dp->d_name;
#elif _WIN32
                if(!firstTurn) {
                    findResult = FindNextFileA(dirHandle, &fileFindData);
                } else {
                    firstTurn = false;
                }
                if(findResult != 0) {
                    char *name = fileFindData.cFileName;
#else 
assert(!"not implemented");
#endif
                        char *fileName = concat(dirName, name);
                        char *ext = getFileExtension(fileName);
                        switch(opType) {
                            case DIR_FIND_FILE_TYPE: {
                                if(isInCharList(ext, exts, count)) {
                                    assert(fileNames.count < arrayCount(fileNames.names));
                                    fileNames.names[fileNames.count++] = fileName;
                                }
                            } break;
                            case DIR_DELETE_FILE_TYPE: {
                                if(isInCharList(ext, exts, count)) {
                                    platformDeleteFile(fileName);
                                    free(fileName);
                                }
                            } break;
                            case DIR_FIND_DIR_TYPE: {
                                if(!ext) { //is folder
                                    assert(fileNames.count < arrayCount(fileNames.names));
                                    fileNames.names[fileNames.count++] = fileName;
                                }
                            } break;
                            case DIR_COPY_FILE_TYPE: {
                                if(isInCharList(ext, exts, count)) {
                                    platformCopyFile(fileName, copyDir);
                                    free(fileName);
                                }
                            } break;
                        }
                   }
#if __APPLE__
               } while (dp);
#elif _WIN32
               } while (findResult);
#else
               assert(!"not implemented");
#endif
#if __APPLE__
            closedir(directory);
#elif _WIN32
            FindClose(dirHandle);
#else 
            assert(!"not implemented");
#endif
        }

    return fileNames;
}

#define getDirectoryFilesOfType(dirName, exts, count) getDirectoryFilesOfType_(dirName, 0, exts, count, DIR_FIND_FILE_TYPE)
#define deleteAllFilesOfType(dirName, exts, count) getDirectoryFilesOfType_(dirName, 0, exts, count, DIR_DELETE_FILE_TYPE)
#define copyAllFilesOfType(dirName, copyDir, exts, count) getDirectoryFilesOfType_(dirName, copyDir, exts, count, DIR_COPY_FILE_TYPE)
#define getDirectoryFolders(dirName) getDirectoryFilesOfType_(dirName, 0, 0, 0, DIR_FIND_DIR_TYPE)