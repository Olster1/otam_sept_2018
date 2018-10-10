typedef struct {
	char *shortName;
	
	Texture tex;
	bool added;

} Easy_AtlasElm;

int cmpAtlasItemFunc (const void * a, const void * b) {
    Easy_AtlasElm *itemA = (Easy_AtlasElm *)a;
    Easy_AtlasElm *itemB = (Easy_AtlasElm *)b;
    bool result = true;
    int sizeA = (itemA->tex.width*itemA->tex.height);
    int sizeB = (itemB->tex.width*itemB->tex.height);
    if(sizeA == sizeB) {
        result = false;
    } else {
    	
        result = sizeA < sizeB;
    }
    
    return result;
}

static inline void easyAtlas_sortBySize(InfiniteAlloc *atlasElms) {
	bool sorted = false;
	int max = (atlasElms->count - 1);
	for(int index = 0; index < max; ++index) {
		Easy_AtlasElm *infoA = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index);
		Easy_AtlasElm *infoB = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index + 1);
		assert(infoA && infoB);
		bool swap = cmpAtlasItemFunc(infoA, infoB);
		if(swap) {
		    Easy_AtlasElm temp = *infoA;
		    *infoA = *infoB;
		    *infoB = temp;
		    sorted = true;
		}   
		if(index == (max - 1) && sorted) {
		    index = 0; 
		    sorted = false;
		}

	}
}

void easyTextureAtlas_createTextureAtlas(char *folderName, SDL_Window *windowHandle) {
	char *imgFileTypes[] = {"jpg", "jpeg", "png", "bmp"};
	FileNameOfType fileNames = getDirectoryFilesOfType(concat(globalExeBasePath, folderName), imgFileTypes, arrayCount(imgFileTypes));
	
	
	int maxSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
	int size = 4096;
	if(size > maxSize) {
		size = maxSize;
	}
	
	V2 bufferDim = v2(size, size);
	FrameBuffer atlasBuffer = createFrameBuffer(bufferDim.x, bufferDim.y, 0);
	initRenderGroup(&globalRenderGroup);
	setFrameBufferId(&globalRenderGroup, atlasBuffer.bufferId);

	InfiniteAlloc atlasElms = initInfinteAlloc(Easy_AtlasElm);


	// unsigned char *ttf_buffer = (unsigned char *)calloc(fileSize, 1);
	for(int i = 0; i < fileNames.count; ++i) {
	    char *fullName = fileNames.names[i];
	    char *shortName = getFileLastPortion(fullName);
	    if(shortName[0] != '.') { //don't load hidden file 
	        Asset *asset = findAsset(shortName);
	        assert(!asset);
	        	
        	Easy_AtlasElm elm = {};
        	elm.shortName = shortName;
        	elm.tex = loadImage(fullName);
        	free(fullName);

        	addElementInifinteAllocWithCount_(&atlasElms, &elm, 1);
	        assert(shortName);
	    }
	}

	easyAtlas_sortBySize(&atlasElms);

	float margin = 4; //4 pixel margin
	float xAt = margin;
	float yAt = margin;
	for(int index = 0; index < atlasElms.count; ++index) {
		Easy_AtlasElm *atlasElm = (Easy_AtlasElm *)getElementFromAlloc_(&atlasElms, index);
		Texture texOnStack = atlasElm->tex;
		if((xAt + texOnStack.width + margin) >= bufferDim.x) {
			xAt = margin;
			yAt += texOnStack.height + margin;
		}
		atlasElm->added = true;

		assert((xAt + texOnStack.width + margin) >= bufferDim.x);
		assert(yAt < bufferDim.y);

		Rect2f texAsRect = rect2f(xAt, yAt, xAt + texOnStack.width, xAt + texOnStack.height);
		texOnStack.uvCoords = rect2f(texAsRect.minX / bufferDim.x, texAsRect.minY / bufferDim.y, texAsRect.maxX / bufferDim.x, texAsRect.maxY / bufferDim.y);

		renderTextureCentreDim(&texOnStack, v2ToV3(getCenter(texAsRect), -1), getDim(texAsRect), COLOR_WHITE, 0, mat4(), mat4(), OrthoMatrixToScreen_BottomLeft(bufferDim.x, bufferDim.y));
	    
	    texOnStack.id = atlasBuffer.textureId;

	    Texture *tex = (Texture *)calloc(sizeof(Texture), 1);
	    memcpy(tex, &texOnStack, sizeof(Texture));
	    Asset *result = addAssetTexture(atlasElm->shortName, tex);
	    free(atlasElm->shortName);

	    xAt += texOnStack.width + margin;
	}


	renderSetViewPort(0, 0, bufferDim.x, bufferDim.y);
	drawRenderGroup(&globalRenderGroup);
	
	SDL_GL_SwapWindow(windowHandle);

	releaseInfiniteAlloc(&atlasElms);
}
