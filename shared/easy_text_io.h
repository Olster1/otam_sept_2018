typedef enum {
    VAR_NULL,
    VAR_CHAR_STAR,
    VAR_LONG_UNSIGNED_INT,
    VAR_LONG_INT,
    VAR_INT,
    VAR_FLOAT,
    VAR_V2,
    VAR_V3,
    VAR_V4,
    VAR_BOOL,
} VarType;


void addVar_(InfiniteAlloc *mem, void *val_, int count, char *varName, VarType type) {
    char data[1028];
    sprintf(data, "\t%s: ", varName);
    addElementInifinteAllocWithCount_(mem, data, strlen(data));

    if(count > 0) {
        if(count > 1 && !(type == VAR_CHAR_STAR || type == VAR_INT || type == VAR_FLOAT)) {
            EasyAssert(!"array not handled yet");
        }
        switch(type) {
            case VAR_CHAR_STAR: {
                if(count == 1) {
                    char *val = (char *)val_;
                    sprintf(data, "\"%s\"", val);
                } else {
                    EasyAssert(count > 1);
                    printf("isArray\n");

                    char **val = (char **)val_;
                    char *bracket = "[";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    for(int i = 0; i < count; ++i) {
                        printf("%s\n", val[i]);
                        sprintf(data, "\"%s\"", val[i]);    
                        addElementInifinteAllocWithCount_(mem, data, strlen(data));
                        if(i != count - 1) {
                            char *commaString = ", ";
                            addElementInifinteAllocWithCount_(mem, commaString, 2);
                        }
                    }
                    bracket = "]";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    data[0] = 0; //clear data
                    
                }
            } break;
            case VAR_LONG_UNSIGNED_INT: {
                unsigned long *val = (unsigned long *)val_;
                sprintf(data, "%lu", val[0]);
            } break;
            case VAR_LONG_INT: {
                long *val = (long *)val_;
                sprintf(data, "%ld", val[0]);
            } break;
            case VAR_INT: {
                if(count == 1) {
                    int *val = (int *)val_;
                    sprintf(data, "%d", val[0]);
                } else {
                    EasyAssert(count > 1);

                    int *val = (int *)val_;
                    char *bracket = "[";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    for(int i = 0; i < count; ++i) {
                        sprintf(data, "%d", val[i]);    
                        addElementInifinteAllocWithCount_(mem, data, strlen(data));
                        if(i != count - 1) {
                            char *commaString = ", ";
                            addElementInifinteAllocWithCount_(mem, commaString, 2);
                        }
                    }
                    bracket = "]";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    data[0] = 0; //clear data
                }
            } break;
            case VAR_FLOAT: {
                if(count == 1) {
                    float *val = (float *)val_;
                    sprintf(data, "%f", val[0]);
                } else {
                    EasyAssert(count > 1);

                    float *val = (float *)val_;
                    char *bracket = "[";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    for(int i = 0; i < count; ++i) {
                        sprintf(data, "%f", val[i]);    
                        addElementInifinteAllocWithCount_(mem, data, strlen(data));
                        if(i != count - 1) {
                            char *commaString = ", ";
                            addElementInifinteAllocWithCount_(mem, commaString, 2);
                        }
                    }
                    bracket = "]";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    data[0] = 0; //clear data
                }
            } break;
            case VAR_V2: {
                float *val = (float *)val_;
                sprintf(data, "%f %f", val[0], val[1]);
            } break;
            case VAR_V3: {
                float *val = (float *)val_;
                sprintf(data, "%f %f %f", val[0], val[1], val[2]);
            } break;
            case VAR_V4: {
                float *val = (float *)val_;
                sprintf(data, "%f %f %f %f", val[0], val[1], val[2], val[3]);
            } break;
            case VAR_BOOL: {
                bool *val = (bool *)val_;
                const char *boolVal = val[0] ? "true" : "false";
                sprintf(data, "%s", boolVal);
            } break;
            default: {
                printf("%s\n", "Error: case not handled in saving");
            }
        }
    }
    addElementInifinteAllocWithCount_(mem, data, strlen(data));

    sprintf(data, ";\n");
    addElementInifinteAllocWithCount_(mem, data, strlen(data));
}

#define addVarArray(mem, val_, count, varName, type) addVar_(mem, val_, count, varName, type)
#define addVar(mem, val_, varName, type) addVar_(mem, val_, 1, varName, type)

typedef struct {
    VarType type;

    union {
        struct {
            float floatVal;
        };
        struct {
            char stringVal[256];
        };
        struct {
            unsigned long intVal;
        };
        struct {
            bool boolVal;
        };
    };
} DataObject;

InfiniteAlloc getDataObjects(EasyTokenizer *tokenizer) {
    bool parsing = true;
    //TODO: arrays
    InfiniteAlloc types = initInfinteAlloc(DataObject);
    bool isArray = false;
    while(parsing) {
        char *at = tokenizer->src;
        EasyToken token = lexGetNextToken(tokenizer);
        // lexPrintToken(&token);
        EasyAssert(at != tokenizer->src);
        switch(token.type) {
            case TOKEN_NULL_TERMINATOR: {
                parsing = false;
            } break;
            case TOKEN_WORD: {

            } break;
            case TOKEN_STRING: {
                DataObject data = {};
                data.type = VAR_CHAR_STAR;
                nullTerminateBuffer(data.stringVal, token.at, token.size);
                
                addElementInifinteAlloc_(&types, &data);
            } break;
            case TOKEN_INTEGER: {
                DataObject data = {};
                data.type = VAR_INT;
                char charBuffer[256] = {};
                char *endptr;
                // int negative = 1;
                // chat *at = token.at;
                // if(*at == '-') {  
                //     negative = -1;
                //     at++;
                // }
                unsigned long value = strtoul(nullTerminateBuffer(charBuffer, token.at, token.size), &endptr, 10);
                data.intVal = value;
                addElementInifinteAlloc_(&types, &data);
            } break;
            case TOKEN_FLOAT: {
                char charBuffer[256] = {};
                nullTerminateBuffer(charBuffer, token.at, token.size);
                float value = atof(charBuffer);
                
                DataObject data = {};
                data.type = VAR_FLOAT;
                
                data.floatVal = value;
                addElementInifinteAlloc_(&types, &data);
            } break;
            case TOKEN_BOOL: {
                DataObject data = {};
                data.type = VAR_BOOL;
                bool value = false;
                if(stringsMatchNullN("true", token.at, token.size)) {
                    value = true;
                } else if(stringsMatchNullN("false", token.at, token.size)) {
                    //
                }
                data.boolVal = value;
                addElementInifinteAlloc_(&types, &data);
            } break;
            case TOKEN_COLON: {

            } break;
            case TOKEN_OPEN_SQUARE_BRACKET: {
                isArray = true;
                //TODO: Do we want to check that this is before any other data??
            } break;
            case TOKEN_SEMI_COLON: {
                parsing = false;
            } break;
            default : {
                
            }
        }
    }

    return types;
}

static inline float easyText_getIntOrFloat(DataObject obj) {
    float a = 0;
    if(obj.type == VAR_FLOAT) {
        a = obj.floatVal;
    } else {
        EasyAssert(obj.type == VAR_INT);
        a = (int)obj.intVal;
    }
    return a;
}

V2 buildV2FromDataObjects(InfiniteAlloc *data, EasyTokenizer *tokenizer) {
    *data = getDataObjects(tokenizer);
    DataObject *objs = (DataObject *)data->memory;
    EasyAssert(objs[0].type == VAR_FLOAT || objs[0].type == VAR_INT);
    EasyAssert(objs[1].type == VAR_FLOAT || objs[0].type == VAR_INT);

    float a = easyText_getIntOrFloat(objs[0]);
    float b = easyText_getIntOrFloat(objs[1]);

    V2 result = v2(a, b);
    return result;
}

V3 buildV3FromDataObjects(InfiniteAlloc *data, EasyTokenizer *tokenizer) {
    *data = getDataObjects(tokenizer);
    DataObject *objs = (DataObject *)data->memory;
    EasyAssert(objs[0].type == VAR_FLOAT);
    EasyAssert(objs[1].type == VAR_FLOAT);
    EasyAssert(objs[2].type == VAR_FLOAT);

    V3 result = v3(objs[0].floatVal, objs[1].floatVal, objs[2].floatVal);
    return result;
}

V4 buildV4FromDataObjects(InfiniteAlloc *data, EasyTokenizer *tokenizer) {
    *data = getDataObjects(tokenizer);
    DataObject *objs = (DataObject *)data->memory;
    EasyAssert(objs[0].type == VAR_FLOAT);
    EasyAssert(objs[1].type == VAR_FLOAT);
    EasyAssert(objs[2].type == VAR_FLOAT);
    EasyAssert(objs[3].type == VAR_FLOAT);

    V4 result = v4(objs[0].floatVal, objs[1].floatVal, objs[2].floatVal, objs[3].floatVal);
    return result;
}

char *getStringFromDataObjects(InfiniteAlloc *data, EasyTokenizer *tokenizer) {
    *data = getDataObjects(tokenizer);
    DataObject *objs = (DataObject *)data->memory;
    EasyAssert(objs[0].type == VAR_CHAR_STAR);

    char *result = objs[0].stringVal;

    return result;
}

#define getIntFromDataObjects(data, tokenizer) (int)getIntFromDataObjects_(data, tokenizer)
#define getULongFromDataObjects(data, tokenizer) getIntFromDataObjects_(data, tokenizer)
#define getLongFromDataObjects(data, tokenizer) (long)getIntFromDataObjects_(data, tokenizer)

unsigned long getIntFromDataObjects_(InfiniteAlloc *data, EasyTokenizer *tokenizer) {
    *data = getDataObjects(tokenizer);
    DataObject *objs = (DataObject *)data->memory;
    EasyAssert(objs[0].type == VAR_INT);
    
    unsigned long result = objs[0].intVal;

    return result;
}

bool getBoolFromDataObjects(InfiniteAlloc *data, EasyTokenizer *tokenizer) {
    *data = getDataObjects(tokenizer);
    DataObject *objs = (DataObject *)data->memory;
    EasyAssert(objs[0].type == VAR_BOOL);
    
    bool result = objs[0].boolVal;

    return result;
}


float getFloatFromDataObjects(InfiniteAlloc *data, EasyTokenizer *tokenizer) {
    *data = getDataObjects(tokenizer);
    DataObject *objs = (DataObject *)data->memory;
    EasyAssert(objs[0].type == VAR_FLOAT);
    
    float result = objs[0].floatVal;
    return result;
}