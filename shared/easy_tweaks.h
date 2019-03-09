#include <sys/stat.h>

typedef struct {
	char *name;
	InfiniteAlloc data;
	VarType type;
} TweakVar;

typedef struct {
	TweakVar vars[32];
	int varCount;
	long modTime;
} Tweaker;

int getHashForString(char *name, int modValue) {
	int hashValue = 0;
	for(int i = 0; i < strlen(name); ++i) {
		hashValue += name[i]*17;
	}

	hashValue %= modValue;
	return hashValue;
}

TweakVar *addTweakVar(Tweaker *tweaker, char *name, InfiniteAlloc *data) {
	//int hashValue = getHashForString(name, arrayCount(tweaker->vars));
	EasyAssert(tweaker->varCount < arrayCount(tweaker->vars));
	TweakVar *var = tweaker->vars + tweaker->varCount++;

	var->name = name;
	var->data = *data;
	EasyAssert(var->data.memory);

	return var;
}

TweakVar *findTweakVar(Tweaker *tweaker, char *name) {
	TweakVar *result = 0;
	for(int i = 0; i < tweaker->varCount && !result; ++i) {
		TweakVar *var = tweaker->vars + i;
		if(cmpStrNull(var->name, name)) {
			result = var;
			break;
		}
	}
	return result;
}

V3 getV3FromTweakData(Tweaker *tweaker, char *name) {
	TweakVar *var = findTweakVar(tweaker, name);
	EasyAssert(var);
	DataObject *objs = (DataObject *)var->data.memory;
	EasyAssert(objs[0].type == VAR_FLOAT);
	EasyAssert(objs[1].type == VAR_FLOAT);
	EasyAssert(objs[2].type == VAR_FLOAT);
	EasyAssert(var->data.count == 3);

	V3 result = v3(objs[0].floatVal, objs[1].floatVal, objs[2].floatVal);
	return result;
}

V2 getV2FromTweakData(Tweaker *tweaker, char *name) {
	TweakVar *var = findTweakVar(tweaker, name);
	EasyAssert(var);
	DataObject *objs = (DataObject *)var->data.memory;
	EasyAssert(objs[0].type == VAR_FLOAT);
	EasyAssert(objs[1].type == VAR_FLOAT);
	EasyAssert(var->data.count == 2);

	V2 result = v2(objs[0].floatVal, objs[1].floatVal);
	return result;
}

float getFloatFromTweakData(Tweaker *tweaker, char *name) {
	TweakVar *var = findTweakVar(tweaker, name);
	EasyAssert(var);
	DataObject *objs = (DataObject *)var->data.memory;
	EasyAssert(objs[0].type == VAR_FLOAT);
	EasyAssert(var->data.count == 1);

	float result = objs[0].floatVal;
	return result;
}

char *getStringFromTweakData(Tweaker *tweaker, char *name) {
    TweakVar *var = findTweakVar(tweaker, name);
    EasyAssert(var);
	DataObject *objs = (DataObject *)var->data.memory;
    EasyAssert(objs[0].type == VAR_CHAR_STAR);

    char *result = objs[0].stringVal;

    return result;
}
#define getIntFromTweakData(tweaker, name) (int)getIntFromTweakData_(tweaker, name)
#define getULongFromTweakData(tweaker, name) getIntFromTweakData_(tweaker, name)
#define getLongFromTweakData(tweaker, name) (long)getIntFromTweakData_(tweaker, name)

unsigned long getIntFromTweakData_(Tweaker *tweaker, char *name) {
    TweakVar *var = findTweakVar(tweaker, name);
    EasyAssert(var);
	DataObject *objs = (DataObject *)var->data.memory;
    EasyAssert(objs[0].type == VAR_INT);
    
    unsigned long result = objs[0].intVal;

    return result;
}

bool getBoolFromTweakData(Tweaker *tweaker, char *name) {
    TweakVar *var = findTweakVar(tweaker, name);
    EasyAssert(var);
	DataObject *objs = (DataObject *)var->data.memory;
    EasyAssert(objs[0].type == VAR_BOOL);
    
    bool result = objs[0].boolVal;

    return result;
}

bool refreshTweakFile(char *fileName, Tweaker *tweaker) {
	bool refreshed = false;
	struct stat result;
	long mod_time = 0;
	if(stat(fileName, &result) == 0) {
	    mod_time = result.st_mtime;
	}
	if(mod_time != tweaker->modTime) {
		refreshed = true;
		if(tweaker->varCount) {
			for(int i = 0; i < tweaker->varCount; ++i) {
				TweakVar *var = tweaker->vars + i;
				free(var->name);
				releaseInfiniteAlloc(&var->data);
			}
			tweaker->varCount = 0;
		}

		FileContents contents = getFileContentsNullTerminate(fileName);

	   tweaker->modTime = mod_time;

		EasyAssert(contents.memory);
		EasyAssert(contents.valid);

		EasyTokenizer tokenizer = lexBeginParsing((char *)contents.memory, true);

		bool parsing = true;
		while(parsing) {
		    EasyToken token = lexGetNextToken(&tokenizer);
		    InfiniteAlloc data = {};
		    switch(token.type) {
		        case TOKEN_NULL_TERMINATOR: {
		            parsing = false;
		        } break;
		        case TOKEN_WORD: {
		        	data = getDataObjects(&tokenizer);

		        	char *name = nullTerminate(token.at, token.size);
		        	addTweakVar(tweaker, name, &data);
		        } break;
		        default: {

		        }
		    }
		}

		free(contents.memory);
	}
	return refreshed;
}