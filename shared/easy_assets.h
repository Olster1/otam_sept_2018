typedef enum {
    ASSET_TEXTURE,
    ASSET_SOUND, 
    ASSET_ANIMATION,
    ASSET_EVENT        
} AssetType;

typedef struct
{
    
} Event;

typedef struct Asset Asset;
typedef struct Asset {
    char *name;

	void *file;

	Asset *next;
} Asset;

static Asset **assets = 0;

int getAssetHash(char *at, int maxSize) {
	int hashKey = 0;
    while(*at) {
        //Make the hash look up different prime numbers. 
        hashKey += (*at)*19;
        at++;
    }
    hashKey %= maxSize;
    return hashKey;
}

Asset *findAsset(char *fileName) {
    int hashKey = getAssetHash(fileName, arrayCount(assets));
    
    Asset *file = assets[hashKey];
    Asset *result = 0;
    
    bool found = false;
    
    while(!found) {
        if(!file) {
            found = true; 
        } else {
            EasyAssert(file->file);
            EasyAssert(file->name);
            if(cmpStrNull(fileName, file->name)) {
                result = file;
                found = true;
            } else {
                file = file->next;
            }
        }
    }
    return result;
}

#define findTextureAsset(fileName) (Texture *)findAsset(fileName)->file
#define findSoundAsset(fileName) (WavFile *)findAsset(fileName)->file
#define findAnimationAsset(fileName) (AnimationParent *)findAsset(fileName)->file
#define findEventAsset(fileName) (Event *)findAsset(fileName)->file

Texture *getTextureAsset(Asset *assetPtr) {
    Texture *result = (Texture *)(assetPtr->file);
    EasyAssert(result);
    return result;
}

WavFile *getSoundAsset(Asset *assetPtr) {
    WavFile *result = (WavFile *)(assetPtr->file);
    EasyAssert(result);
    return result;
}

AnimationParent *getAnimationAsset(Asset *assetPtr) {
    AnimationParent *result = (AnimationParent *)(assetPtr->file);
    EasyAssert(result);
    return result;
}

Event *getEventAsset(Asset *assetPtr) {
    Event *result = (Event *)(assetPtr->file);
    EasyAssert(result);
    return result;
}

Asset *addAsset_(char *fileName, void *asset) { 
    char *truncName = getFileLastPortion(fileName);
    int hashKey = getAssetHash(truncName, arrayCount(assets));
    EasyAssert(fileName != truncName);
    Asset **filePtr = assets + hashKey;
    
    bool found = false; 
    Asset *result = 0;
    while(!found) {
        Asset *file = *filePtr;
        if(!file) {
            file = (Asset *)calloc(sizeof(Asset), 1);
            file->file = asset;
            file->name = truncName;
            file->next = 0;
            *filePtr = file;
            result = file;
            found = true;
        } else {
            filePtr = &file->next;
        }
    }
    EasyAssert(found);
    return result;
}

Asset *addAssetTexture(char *fileName, Texture *asset) { // we have these for type checking
    Asset *result = addAsset_(fileName, asset);
    return result;
}

Asset *addAssetSound(char *fileName, WavFile *asset) { // we have these for type checking
    Asset *result = addAsset_(fileName, asset);
    return result;
}

Asset *addAssetEvent(char *fileName, Event *asset) { // we have these for type checking
    Asset *result = addAsset_(fileName, asset);
    return result;
}

Asset *loadImageAsset(char *fileName) {
    Texture texOnStack = loadImage(fileName, TEXTURE_FILTER_LINEAR);
    Texture *tex = (Texture *)calloc(sizeof(Texture), 1);
    memcpy(tex, &texOnStack, sizeof(Texture));
    Asset *result = addAssetTexture(fileName, tex);
    EasyAssert(result);
    return result;
}

Asset *loadSoundAsset(char *fileName, SDL_AudioSpec *audioSpec) {
    WavFile *sound = (WavFile *)calloc(sizeof(WavFile), 1);
    loadWavFile(sound, fileName, audioSpec);
    Asset *result = addAssetSound(fileName, sound);
    EasyAssert(result);
    //free(fileName);
    return result;
}