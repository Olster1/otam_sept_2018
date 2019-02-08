typedef struct  {
    int completedCount;
    bool valid;
} LevelCountFromFile;


#define LEVEL_STATE(FUNC) \
FUNC(LEVEL_STATE_NULL) \
FUNC(LEVEL_STATE_COMPLETED) \
FUNC(LEVEL_STATE_UNLOCKED) \
FUNC(LEVEL_STATE_LOCKED) \

typedef enum {
    LEVEL_STATE(ENUM)
} LevelState;

static char *LevelStateStrings[] = { LEVEL_STATE(STRING) };

#define LEVEL_TYPE(FUNC) \
FUNC(LEVEL_NULL) \
FUNC(LEVEL_0) \
FUNC(LEVEL_1) \
FUNC(LEVEL_2) \
FUNC(LEVEL_3) \
FUNC(LEVEL_4) \
FUNC(LEVEL_5) \
FUNC(LEVEL_6) \
FUNC(LEVEL_7) \
FUNC(LEVEL_8) \
FUNC(LEVEL_9) \
FUNC(LEVEL_10) \
FUNC(LEVEL_11) \
FUNC(LEVEL_12) \
FUNC(LEVEL_13) \
FUNC(LEVEL_14) \
FUNC(LEVEL_15) \
FUNC(LEVEL_16) \
FUNC(LEVEL_17) \
FUNC(LEVEL_18) \
FUNC(LEVEL_19) \
FUNC(LEVEL_20) \
FUNC(LEVEL_21) \
FUNC(LEVEL_22) \
FUNC(LEVEL_23) \
FUNC(LEVEL_24) \
FUNC(LEVEL_25) \
FUNC(LEVEL_26) \
FUNC(LEVEL_27) \
FUNC(LEVEL_28) \
FUNC(LEVEL_29) \
FUNC(LEVEL_30) \
FUNC(LEVEL_31) \
FUNC(LEVEL_32) \
FUNC(LEVEL_33) \
FUNC(LEVEL_34) \
FUNC(LEVEL_35) \
FUNC(LEVEL_36) \
FUNC(LEVEL_37) \
FUNC(LEVEL_38) \
FUNC(LEVEL_39) \
FUNC(LEVEL_40) \
FUNC(LEVEL_41) \
FUNC(LEVEL_42) \
FUNC(LEVEL_43) \
FUNC(LEVEL_44) \
FUNC(LEVEL_45) \
FUNC(LEVEL_46) \
FUNC(LEVEL_47) \
FUNC(LEVEL_48) \
FUNC(LEVEL_49) \
FUNC(LEVEL_50) \
FUNC(LEVEL_COUNT) \

typedef enum {
    LEVEL_TYPE(ENUM)
} LevelType;

static char *LevelTypeStrings[] = { LEVEL_TYPE(STRING) };

typedef struct LevelData LevelData;
typedef struct LevelData {
    char *name;
    bool valid;
    
    LevelType levelType;
    
    FileContents contents;
    
    LevelState state;
    
    //this is for the overworld
    float angle;
    float dA;
    particle_system particleSystem;
    
    bool hasPlayedHoverSound;
    Timer showTimer;
    
    int glyphCount;
    GlyphInfo glyphs[3];
    //

    Timer displayNameTimer;
    bool justOn; //for displaying name
    
    V2 pos;
    int groupId;
    
    LevelData *next; //this is used for the overworld level groups
} LevelData;

typedef struct  {
    LevelType levels[22];
    int count;    
    bool activated;
    V2 averagePos; 
} LevelGroup;

/*
static inline V2 getPos(OverworldLevelState *state, LevelGroup *group) {
    V2 currentAverage = v2(0, 0);
    for(int i = 0; i < group->count; ++i) {
        LevelType t = group->levels[i];
        currentAverage = v2_plus(state->buttons[(int)t].pos, currentAverage);
    }
    V2 average = v2_scale(1.0f / group->count, currentAverage);
    return average;
}
*/
static inline void addLevelToGroup(LevelGroup *group, LevelType type) {
    assert(group->count < arrayCount(group->levels));
    group->levels[group->count++] = type;
    assert(group->activated == false);
}

static inline LevelCountFromFile findLevelCount(char *readName) {
    LevelCountFromFile counts = {};
    
    if(platformDoesFileExist(readName)) {
        counts.valid = true;
        
        FileContents saveFileContents = getFileContentsNullTerminate(readName);
        assert(saveFileContents.valid);
        
        EasyTokenizer tokenizer = lexBeginParsing((char *)saveFileContents.memory, true);
        bool parsing = true;
        
        
        
        while(parsing) {
            EasyToken token = lexGetNextToken(&tokenizer);
            InfiniteAlloc data = {};
            switch(token.type) {
                case TOKEN_NULL_TERMINATOR: {
                    parsing = false;
                } break;
                case TOKEN_WORD: {
                    if(stringsMatchNullN("levelState", token.at, token.size)) {
                        char *stringToCopy = getStringFromDataObjects(&data, &tokenizer);
                        LevelState stateToSet = LEVEL_STATE_NULL;
                        for(int i = 0; i < arrayCount(LevelStateStrings); ++i) {
                            if(cmpStrNull(LevelStateStrings[i], stringToCopy)) {
                                stateToSet = (LevelState)i;
                                assert(stateToSet != LEVEL_STATE_NULL);
                                break;
                            }
                        }
                        
                        if(stateToSet == LEVEL_STATE_COMPLETED) {
                            counts.completedCount++;
                        }
                    }
                } break;
                default: {
                    
                }
            }
            releaseInfiniteAlloc(&data);
        }
        free(saveFileContents.memory);
    } else {
        counts.valid = false;
    }
    
    return counts;
}

void startGameAgain(int level) {
    assert(level >= 0 && level < 3);
    char shortName[1028] = {};
    sprintf(shortName, "saveFile%d.h", level);
    char *readName = concat(globalExeBasePath, shortName);
    if(platformDoesFileExist(readName)) {
        platformDeleteFile(readName);
    }
    free(readName);
}


//This is used when there is more than one save file & we actually have to look up the details.  
//Now we just use the data we have in frame params;
// void updateSaveStateDetailsWithFile(LevelCountFromFile *saveStateDetails, int count) {
//     for(int i = 0; i < count; i++) {
//         char readName[512] = {};
//         sprintf(readName, "%ssaveFile%d.h", globalExeBasePath, i);
//         saveStateDetails[i] = findLevelCount(readName);
//     }
// }

void loadSaveFile(LevelData *levelsData, int numberOfLevels, int saveSlot, int *lastShownGroup_) {
    
    
    for(int i = 0; i < numberOfLevels; ++i) {
        LevelData *level = levelsData + i;
        
        if(level->groupId == 0) {
            level->state = LEVEL_STATE_UNLOCKED;
        } else {
            level->state = LEVEL_STATE_LOCKED;
#if CHEAT_MODE
            //unlock everything
            level->state = LEVEL_STATE_UNLOCKED;
#endif
        }
    }
    
    char shortName[256] = {};
    sprintf(shortName, "saveFile%d.h", saveSlot);
    assert(strlen(shortName) < 255);
    char *readName = concat(globalExeBasePath, shortName);
    // printf("%s\n", readName);
    int lastShownGroup = 10000; //really big number. No groups above this. 
    if(platformDoesFileExist(readName)) {
        FileContents saveFileContents = getFileContentsNullTerminate(readName);
        assert(saveFileContents.valid);
        
        EasyTokenizer tokenizer = lexBeginParsing((char *)saveFileContents.memory, true);
        bool parsing = true;
        
        bool isLevelData = false;
        LevelType levelAt = LEVEL_NULL;
        while(parsing) {
            EasyToken token = lexGetNextToken(&tokenizer);
            InfiniteAlloc data = {};
            switch(token.type) {
                case TOKEN_NULL_TERMINATOR: {
                    parsing = false;
                } break;
                case TOKEN_WORD: {
                    if(stringsMatchNullN("levelType", token.at, token.size)) {
                        char *stringToCopy = getStringFromDataObjects(&data, &tokenizer);
                        for(int i = 0; i < arrayCount(LevelTypeStrings); ++i) {
                            if(cmpStrNull(LevelTypeStrings[i], stringToCopy)) {
                                levelAt = (LevelType)i;
                                assert(levelAt != LEVEL_NULL);
                                break;
                            }
                        }
                    }
                    if(stringsMatchNullN("levelState", token.at, token.size)) {
                        assert(levelAt != LEVEL_NULL);
                        LevelData *level = levelsData + levelAt;
                        char *stringToCopy = getStringFromDataObjects(&data, &tokenizer);
                        LevelState stateToSet = LEVEL_STATE_NULL;
                        for(int i = 0; i < arrayCount(LevelStateStrings); ++i) {
                            if(cmpStrNull(LevelStateStrings[i], stringToCopy)) {
                                stateToSet = (LevelState)i;
                                assert(stateToSet != LEVEL_STATE_NULL);
                                break;
                            }
                        }
                        
                        if(stateToSet != LEVEL_STATE_COMPLETED) {
                            assert(stateToSet == LEVEL_STATE_UNLOCKED);
                            if(level->groupId < lastShownGroup) {
                                lastShownGroup = level->groupId;
                            }
                        }
                        level->state = stateToSet;
                    }
                } break;
                case TOKEN_CLOSE_BRACKET: {
                    levelAt = LEVEL_NULL;
                    
                } break;
                default: {
                    
                }
            }
            releaseInfiniteAlloc(&data);
        }
        lastShownGroup = lastShownGroup - 1; //want to make the particle events on the latest group
    } else {
        //assumes there is a group 0?
        lastShownGroup = -1;
    }
    
    *lastShownGroup_ = lastShownGroup;

    free(readName);
}

void updateSaveStateDetails(LevelData *levelDatasArray, LevelCountFromFile *saveStateDetails, int count) {
    assert(count == 1); //this only supports _one_ save file. If you want more use the commented out function in Levels.h file, named the same name.
    int completedLevelsCount = 0;
    for(int i = 0; i < LEVEL_COUNT; ++i) {
        LevelData *data = levelDatasArray + i;
        if(data->valid) {
            assert(data->valid);
            if(data->state == LEVEL_STATE_COMPLETED) {
                completedLevelsCount++;
            }
        }
    }

    saveStateDetails[0].valid = true;
    saveStateDetails[0].completedCount = completedLevelsCount;
    
}
