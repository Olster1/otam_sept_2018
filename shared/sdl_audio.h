
#define SDL_AUDIO_CALLBACK(name) void name(void *udata, unsigned char *stream, int len)
typedef SDL_AUDIO_CALLBACK(sdl_audio_callback);

#define AUDIO_MONO 1
#define AUDIO_STEREO 2

typedef enum {
    AUDIO_FLAG_NULL,
    AUDIO_FLAG_MENU,
    AUDIO_FLAG_MAIN,
    AUDIO_FLAG_START_SCREEN,
    AUDIO_FLAG_COUNT,
} SoundType;

typedef struct {
    unsigned int size;
    unsigned char *data;
    char *fileName;
} WavFile;

typedef struct WavFilePtr WavFilePtr;
typedef struct WavFilePtr {
    WavFile *file;
    
    char *name;
    
    WavFilePtr *next;
} WavFilePtr;

typedef enum {
    AUDIO_BACKGROUND,
    AUDIO_FOREGROUND,
    AUDIO_CHANNEL_COUNT
} AudioChannel;

#define LOW_VOLUME 32
#define MIDDLE_VOLUME 64
#define MAX_VOLUME 64

//This is for setting the volume channels 
//NOTE: volume is between 0 - 128

typedef struct {
    float tAt; //0 to 1
    float period;
    float a;
    float b;
    bool active;
} VolumeLerp;

static int channelVolumes_[AUDIO_CHANNEL_COUNT] = {MAX_VOLUME, MAX_VOLUME};
static VolumeLerp channelVolumesLerps_[AUDIO_CHANNEL_COUNT] = {};

static bool globalChannelsState_[AUDIO_CHANNEL_COUNT] = {true, true};

static float parentChannelVolumes_[AUDIO_FLAG_COUNT] = {1, 1, 1, 1};
static VolumeLerp parentChannelVolumesLerps_[AUDIO_FLAG_COUNT] = {};

static SoundType globalSoundActiveType_ = AUDIO_FLAG_NULL;
static bool globalSoundOn = true;

bool isSoundTypeSet(SoundType type) {
    bool result = (globalSoundActiveType_ == type);
    return result;
}

void setSoundType(SoundType type) {
    globalSoundActiveType_ = type;
}

void setChannelVolume(AudioChannel channel, float targetVolume, float period) {
    VolumeLerp *lerpValue = channelVolumesLerps_ + channel;
    lerpValue->a = channelVolumes_[channel];
    lerpValue->b = clamp(0, targetVolume, 128);
    lerpValue->tAt = 0;
    lerpValue->period = period;
    lerpValue->active = true;
}

void setParentChannelVolume(SoundType channel, float targetVolume, float period) {
    VolumeLerp *lerpValue = parentChannelVolumesLerps_ + channel;
    lerpValue->a = parentChannelVolumes_[channel];
    lerpValue->b = clamp(0, targetVolume, 1);
    lerpValue->tAt = 0;
    lerpValue->period = period;
    lerpValue->active = true;
}

void updateVolumeLerp(VolumeLerp *lerpVal, void *volumeVal_, float dt, bool isInt) {
    if(lerpVal->active) {
        float a = (float)lerpVal->a;
        float b = (float)lerpVal->b;

        lerpVal->tAt += dt;
        float tValue = lerpVal->tAt / lerpVal->period;
        float volume = lerp(a, clamp(0, tValue, 1), b);
        //set channel volume
        if(isInt) {
            int *volumeVal = (int *)volumeVal_;
            *volumeVal = (int)clamp(0, volume, 128);    
        } else {
            float *volumeVal = (float *)volumeVal_;
            *volumeVal = (float)clamp(0, volume, 1);    
        }
        
        //
        if(tValue >= 1) {
            lerpVal->tAt = 0;
            lerpVal->active = false;
        }
    }
}

void updateChannelVolumes(float dt) {
    for(int channelAt = 0; channelAt < AUDIO_CHANNEL_COUNT; ++channelAt) {
        VolumeLerp *lerpVal = channelVolumesLerps_ + channelAt;
        updateVolumeLerp(lerpVal, &channelVolumes_[channelAt], dt, true);
        
    }

    for(int channelAt = 0; channelAt < AUDIO_FLAG_COUNT; ++channelAt) {
        VolumeLerp *lerpVal = parentChannelVolumesLerps_ + channelAt;
        updateVolumeLerp(lerpVal, &parentChannelVolumes_[channelAt], dt, false);
        
    }
}

typedef struct PlayingSound {
    WavFile *wavFile;
    unsigned int bytesAt;

    bool active;
    AudioChannel channel;
    SoundType soundType;
    float volume; //percent of original volume
    
    PlayingSound *nextSound;
    
    struct PlayingSound *next;
} PlayingSound;

static PlayingSound *playingSoundsFreeList;
static PlayingSound *playingSounds;

WavFilePtr *sounds[4096];

int getSoundHashKey_(char *at, int maxSize) {
    int hashKey = 0;
    while(*at) {
        //Make the hash look up different prime numbers. 
        hashKey += (*at)*19;
        at++;
    }
    hashKey %= maxSize;
    return hashKey;
}

WavFile *easyAudio_findSound(char *fileName) {
    int hashKey = getSoundHashKey_(fileName, arrayCount(sounds));
    
    WavFilePtr *file = sounds[hashKey];
    WavFile *result = 0;
    
    bool found = false;
    
    while(!found) {
        if(!file) {
            found = true; 
        } else {
            assert(file->file);
            assert(file->name);
            if(cmpStrNull(fileName, file->name)) {
                result = file->file;
                found = true;
            } else {
                file = file->next;
            }
        }
    }
    return result;
}

char *sdlAudiolastFilePortion_(char *at) {
    // TODO(Oliver): Make this more robust
    char *recent = at;
    while(*at) {
        if(*at == '/' && at[1] != '\0') { 
            recent = (at + 1); //plus 1 to pass the slash
        }
        at++;
    }
    
    int length = (int)(at - recent) + 1;
    char *result = (char *)calloc(length, 1);
    
    memcpy(result, recent, length);
    result[length - 1] = '\0';
    // printf("%s\n", result);
    
    return result;
}

void addSound_(WavFile *sound) {
    char *truncName = sdlAudiolastFilePortion_(sound->fileName);
    int hashKey = getSoundHashKey_(truncName, arrayCount(sounds));
    
    WavFilePtr **filePtr = sounds + hashKey;
    
    bool found = false; 
    while(!found) {
        WavFilePtr *file = *filePtr;
        if(!file) {
            file = (WavFilePtr *)calloc(sizeof(WavFilePtr), 1);
            file->file = sound;
            file->name = truncName;
            *filePtr = file;
            found = true;
        } else {
            filePtr = &file->next;
        }
    }
    assert(found);
    
}

PlayingSound *playSound(Arena *arena, WavFile *wavFile, PlayingSound *nextSoundToPlay, AudioChannel channel) {
    PlayingSound *result = playingSoundsFreeList;
    if(result) {
        playingSoundsFreeList = result->next;
    } else {
        result = pushStruct(arena, PlayingSound);
    }
    assert(result);
    
    //add to playing sounds. 
    result->next = playingSounds;
    playingSounds = result;
    
    result->active = true;
    result->volume = 1.0f;
    result->channel = channel;
    result->nextSound = nextSoundToPlay;
    result->bytesAt = 0;
    result->wavFile = wavFile;
    result->soundType = AUDIO_FLAG_NULL;

    return result;
}

//This call is for setting a sound up but not playing it. 
PlayingSound *pushSound(Arena *arena, WavFile *wavFile, PlayingSound *nextSoundToPlay, AudioChannel channel) {
    PlayingSound *result = playSound(arena, wavFile, nextSoundToPlay, channel);
    result->active = false;
    return result;
}

PlayingSound *playMenuSound(Arena *arena, WavFile *wavFile, PlayingSound *nextSoundToPlay, AudioChannel channel) {
    PlayingSound *result = playSound(arena, wavFile, nextSoundToPlay, channel);
    result->soundType = AUDIO_FLAG_MENU;
    return result;
}

//This call is for setting a sound up but not playing it. 
PlayingSound *pushMenuSound(Arena *arena, WavFile *wavFile, PlayingSound *nextSoundToPlay, AudioChannel channel) {
    PlayingSound *result = pushSound(arena, wavFile, nextSoundToPlay, channel);
    result->soundType = AUDIO_FLAG_MENU;
    return result;
}

//TODO: Change this to define staments
PlayingSound *playGameSound(Arena *arena, WavFile *wavFile, PlayingSound *nextSoundToPlay, AudioChannel channel) {
    PlayingSound *result = playSound(arena, wavFile, nextSoundToPlay, channel);
    result->soundType = AUDIO_FLAG_MAIN;
    return result;
}

PlayingSound *pushGameSound(Arena *arena, WavFile *wavFile, PlayingSound *nextSoundToPlay, AudioChannel channel) {
    PlayingSound *result = pushSound(arena, wavFile, nextSoundToPlay, channel);
    result->soundType = AUDIO_FLAG_MAIN;
    return result;
}

PlayingSound *playStartMenuSound(Arena *arena, WavFile *wavFile, PlayingSound *nextSoundToPlay, AudioChannel channel) {
    PlayingSound *result = playSound(arena, wavFile, nextSoundToPlay, channel);
    result->soundType = AUDIO_FLAG_START_SCREEN;
    return result;
}

///

void loadWavFile(WavFile *result, char *fileName, SDL_AudioSpec *audioSpec) {
    int desiredChannels = audioSpec->channels;

    SDL_AudioSpec* outputSpec = SDL_LoadWAV(fileName, audioSpec, &result->data, &result->size);
    result->fileName = fileName;

    addSound_(result);

    ///NOTE: Upsample to Stereo if mono sound
    if(outputSpec->channels != desiredChannels) {
        assert(outputSpec->channels == AUDIO_MONO);
        assert(audioSpec->channels != desiredChannels);
        audioSpec->channels = desiredChannels;

        
        unsigned int newSize = 2 * result->size;
        //assign double the data 
        unsigned char *newData = (unsigned char *)calloc(sizeof(unsigned char)*newSize, 1); 
        //TODO :SIMD this 
        s16 *samples = (s16 *)result->data;
        s16 *newSamples = (s16 *)newData;
        int sampleCount = result->size/sizeof(s16);
        for(int i = 0; i < sampleCount; ++i) {
            s16 value = samples[i];
            int sampleAt = 2*i;
            newSamples[sampleAt] = value;
            newSamples[sampleAt + 1] = value;
        }
        result->size = newSize;
        SDL_FreeWAV(result->data);
        result->data = newData;
    }
    /////////////
    
    if(outputSpec) {
        assert(audioSpec->freq == outputSpec->freq);
        assert(audioSpec->format = outputSpec->format);
        assert(audioSpec->channels == outputSpec->channels);   
        assert(audioSpec->samples == outputSpec->samples);
    } else {
        fprintf(stderr, "Couldn't open wav file: %s\n", SDL_GetError());
        assert(!"couldn't open file");
    }
}

#define initAudioSpec(audioSpec, frequency) initAudioSpec_(audioSpec, frequency, audioCallback)

void initAudioSpec_(SDL_AudioSpec *audioSpec, int frequency, sdl_audio_callback *callback) {
    /* Set the audio format */
    audioSpec->freq = frequency;
    audioSpec->format = AUDIO_S16;
    audioSpec->channels = AUDIO_STEREO;
    audioSpec->samples = 4096; 
    audioSpec->callback = callback;
    audioSpec->userdata = NULL;
}

bool initAudio(SDL_AudioSpec *audioSpec) {
    bool successful = true;
    /* Open the audio device, forcing the desired format */
    if ( SDL_OpenAudio(audioSpec, NULL) < 0 ) {
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        successful = false;
    }

    SDL_PauseAudio(0); //play audio
    return successful;
}

static inline bool channelIsOn(AudioChannel channel) {
    return globalChannelsState_[channel];
}

//TODO: Pull mixing function out into it's own function???
SDL_AUDIO_CALLBACK(audioCallback) {
    SDL_memset(stream, 0, len);

    for(PlayingSound **soundPrt = &playingSounds;
        *soundPrt; 
        ) {
        bool advancePtr = true;
        PlayingSound *sound = *soundPrt;
        bool isSoundType = true;//isSoundTypeSet(sound->soundType) || sound->soundType == AUDIO_FLAG_NULL;
        if(sound->active && isSoundType) {
            unsigned char *samples = sound->wavFile->data + sound->bytesAt;
            int remainingBytes = sound->wavFile->size - sound->bytesAt;
            
            assert(remainingBytes >= 0);
            
            unsigned int bytesToWrite = (remainingBytes < len) ? remainingBytes: len;
            
            int volume = 0;
            if(globalSoundOn && channelIsOn(sound->channel)) {
                volume = lerp(0, parentChannelVolumes_[sound->soundType], lerp(0, sound->volume, channelVolumes_[sound->channel]));
                
            }
            SDL_MixAudio(stream, samples, bytesToWrite, volume);
            
            sound->bytesAt += bytesToWrite;
            
            if(sound->bytesAt >= sound->wavFile->size) {
                assert(sound->bytesAt == sound->wavFile->size);
                if(sound->nextSound) {
                    //TODO: Allow the remaining bytes to loop back round and finish the full duration 
                    sound->active = false;
                    sound->bytesAt = 0;
                    sound->nextSound->active = true;
                } else {
                    //remove from linked list
                    advancePtr = false;
                    *soundPrt = sound->next;
                    sound->next = playingSoundsFreeList;
                    playingSoundsFreeList = sound;
                }
            }
        }
        
        if(advancePtr) {
            soundPrt = &((*soundPrt)->next);
        }
    }
}
