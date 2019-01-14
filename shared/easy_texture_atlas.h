/*
Header file to build texture atlases which outputs the texture_name, uv_coords & the original width and height 
of the image. It requires stb_image_write also easy_text_io, eas_lex & easy_array header files to work for text writing/parsing 
and stretchy buffer respectively
*/

typedef struct {
	char *shortName;
	
	Texture tex;
	bool added;

} Easy_AtlasElm;

static inline void easyAtlas_sortBySize(InfiniteAlloc *atlasElms) {
	bool sorted = false;
	int max = (atlasElms->count - 1);
	for(int index = 0; index < max; ) {
		bool incrementIndex = true;
		assert(index + 1 < atlasElms->count);
		
		Easy_AtlasElm *infoA = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index);
		Easy_AtlasElm *infoB = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index + 1);
		assert(infoA && infoB);
		int sizeA = infoA->tex.width*infoA->tex.height;
    	int sizeB = infoB->tex.width*infoB->tex.height;
		bool swap = sizeA < sizeB;
		if(swap) {
		    Easy_AtlasElm temp = *infoA;
		    *infoA = *infoB;
		    *infoB = temp;
		    sorted = true;
		}   
		if(index == (max - 1) && sorted) {
		    index = 0; 
		    sorted = false;
		    incrementIndex = false;
		}

		if(incrementIndex) {
			index++;
		}
	}

	//NOTE: for debugging to check it actually sorted correctly 
	// for(int index = 0; index < max; ++index) {
	// 	Easy_AtlasElm *infoA = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index);
	// 	Easy_AtlasElm *infoB = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index + 1);
	// 	int sizeA = infoA->tex.width*infoA->tex.height;
 //    	int sizeB = infoB->tex.width*infoB->tex.height;
	// 	assert(sizeA >= sizeB);
	// }
}

typedef struct EasyAtlas_BinPartition EasyAtlas_BinPartition;
typedef struct EasyAtlas_BinPartition {
	EasyAtlas_BinPartition *next;
	EasyAtlas_BinPartition *prev;
	
	Rect2f rect;

} EasyAtlas_BinPartition;


/*
- Place new bin on the shortest side of the image 
- partition bin into two new bins 
- best fit - search list for smallest partition the image will fit in
- paritions in a doubley linked list 

*/

typedef struct {
	EasyAtlas_BinPartition sentinel;
	EasyAtlas_BinPartition *freeList;

	Arena *memoryArena;
} EasyAtlas_BinState;

static inline EasyAtlas_BinPartition *easyAtlas_findBestFitBin(EasyAtlas_BinState *state, Texture *tex) {
	EasyAtlas_BinPartition *sentinel = &state->sentinel;
	EasyAtlas_BinPartition *binAt = sentinel->next;
	EasyAtlas_BinPartition *bestFit = 0;
	V2 bestDimDiff = v2(FLT_MAX, FLT_MAX);
	while (binAt != sentinel) {
		V2 dimDiff = v2_minus(getDim(binAt->rect), v2(tex->width, tex->height));
		if(dimDiff.x >= 0 && dimDiff.x < bestDimDiff.x && dimDiff.y >= 0 && dimDiff.y < bestDimDiff.y) {	
			bestDimDiff = dimDiff;
			bestFit = binAt;
		}
		binAt = binAt->next;
	}
	return bestFit;
}

static inline void easyAtlas_addBin(Rect2f a, EasyAtlas_BinState *state) {
	V2 aDim = getDim(a);
	if(aDim.x != 0 && aDim.y != 0) {
		EasyAtlas_BinPartition *bin = 0;
		if(state->freeList) {
			bin = state->freeList;
			state->freeList = bin->next;
		} else {
			bin = pushStruct(state->memoryArena, EasyAtlas_BinPartition);	
		}
		assert(bin);
		
		//add to the doubley linked list. 
		bin->next = &state->sentinel;
		assert(state->sentinel.prev->next == &state->sentinel);
		bin->prev = state->sentinel.prev;

		state->sentinel.prev->next = bin;
		state->sentinel.prev = bin;
		
		bin->rect = a;
		
	}
}

static inline void easyAtlas_removeBin(EasyAtlas_BinPartition *bin, EasyAtlas_BinState *state) {
	assert(bin->prev->next == bin);
	bin->prev->next = bin->next;
	bin->next->prev = bin->prev;

	bin->prev = 0;
	bin->next = state->freeList;
	state->freeList = bin;
}

static inline void easyAtlas_partitionBin(EasyAtlas_BinState *state, EasyAtlas_BinPartition *bin, Texture *tex) {
	V2 texDim = v2(tex->width, tex->height);
	Rect2f br = bin->rect;
	Rect2f a = br;
	Rect2f b = br;
	
	if(texDim.x > texDim.y) {
		a.minX = br.minX + tex->width;
		a.maxY = br.minY + tex->height;

		b.minY = br.minY + tex->height;
	} else {
		a.minY = br.minY + tex->height;
		a.maxX = br.minX + tex->width;

		b.minX = br.minX + tex->width;
	}

	easyAtlas_removeBin(bin, state);	

	easyAtlas_addBin(a, state);
	easyAtlas_addBin(b, state);
	
}

static inline bool easyAtlas_allElmsBeenAdded(InfiniteAlloc *atlasElms) {
	int addedCount = 0;
	for(int index = 0; index < atlasElms->count; ++index) {
		Easy_AtlasElm *atlasElm = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index);
		if(atlasElm->added) {
			addedCount++;
		}
	}
	bool result = (addedCount == atlasElms->count);
	return result;
}

static inline void easyAtlas_drawAtlas(char *folderName, Arena *memoryArena, InfiniteAlloc *atlasElms, bool outputImageFile, char *name) {

	MemoryArenaMark tempMark = takeMemoryMark(memoryArena);
	
	EasyAtlas_BinState state = {};
	state.sentinel.next = state.sentinel.prev = &state.sentinel;
	state.freeList = 0;
	state.memoryArena = memoryArena;

	int addedCount = 0;
	int loopCount = 0;
	while(!easyAtlas_allElmsBeenAdded(atlasElms)) {
		loopCount++;
		//get the width of the tex atlas
		int maxSize;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
		int size = 4096;
		if(size > maxSize) {
			size = maxSize;
		}

		V2 bufferDim = v2(size, size);
		FrameBuffer atlasBuffer = createFrameBuffer(bufferDim.x, bufferDim.y, 0);
		initRenderGroup(globalRenderGroup);
		setFrameBufferId(globalRenderGroup, atlasBuffer.bufferId);
		clearBufferAndBind(atlasBuffer.bufferId, COLOR_NULL);

		//draw all the rects into the frame buffer
		easyAtlas_addBin(rect2f(0, 0, bufferDim.x, bufferDim.y), &state);

		InfiniteAlloc fileMemoryToWrite = initInfinteAlloc(char);

		bool bufferHasRoom = true;
		for(int index = 0; index < atlasElms->count; ++index) {
			Easy_AtlasElm *atlasElm = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index);
			if(!atlasElm->added) {
				Texture texOnStack = atlasElm->tex;

				if(state.sentinel.next != &state.sentinel) {
					EasyAtlas_BinPartition *bin = easyAtlas_findBestFitBin(&state, &texOnStack);
					if(bin) {
						atlasElm->added = true;

						Rect2f texAsRect = rect2f(bin->rect.minX, bin->rect.minY, bin->rect.minX + texOnStack.width, bin->rect.minY + texOnStack.height);
						renderTextureCentreDim(&texOnStack, v2ToV3(getCenter(texAsRect), -1), getDim(texAsRect), COLOR_WHITE, 0, mat4(), mat4(), OrthoMatrixToScreen_BottomLeft(bufferDim.x, bufferDim.y));
						// renderDrawRectOutlineCenterDim_(v2ToV3(getCenter(bin->rect), -0.5f), getDim(bin->rect), COLOR_RED, 0, mat4(), OrthoMatrixToScreen_BottomLeft(bufferDim.x, bufferDim.y), 4); 

						texOnStack.uvCoords = rect2f(texAsRect.minX / bufferDim.x, texAsRect.minY / bufferDim.y, texAsRect.maxX / bufferDim.x, texAsRect.maxY / bufferDim.y);
					    texOnStack.id = atlasBuffer.textureId;
					    

					    char *startBr = "{";
						addElementInifinteAllocWithCount_(&fileMemoryToWrite, startBr, 1); 

					    addVar(&fileMemoryToWrite, &texOnStack.width, "width", VAR_INT);
					    addVar(&fileMemoryToWrite, &texOnStack.height, "height", VAR_INT);
					    addVar(&fileMemoryToWrite, texOnStack.uvCoords.E, "uvCoords", VAR_V4);
					    addVar(&fileMemoryToWrite, atlasElm->shortName, "name", VAR_CHAR_STAR);

					    char *endBr = "}";
					    addElementInifinteAllocWithCount_(&fileMemoryToWrite, endBr, 1); 

					    Texture *tex = (Texture *)calloc(sizeof(Texture), 1);
					    memcpy(tex, &texOnStack, sizeof(Texture));
					    Asset *result = addAssetTexture(atlasElm->shortName, tex);

					    free(atlasElm->shortName);

					    easyAtlas_partitionBin(&state, bin, &texOnStack);
					} else {
						//make sure the texture can at least fit on a big size. 
						assert(texOnStack.width < size && texOnStack.height < size);
					}
				} else {
					printf("%s\n", "no more bins");
					//no more bins so make a new texture 
				}
			} 
		}

		renderSetViewPort(0, 0, bufferDim.x, bufferDim.y);
		drawRenderGroup(globalRenderGroup);

		glFlush();
		printf("%s\n", "successfullly rendered group");

		char buffer[512] = {};
		sprintf(buffer, "%s%s_%d.txt", folderName, name, loopCount);
		printf("%s\n", buffer);
		game_file_handle fileHandle = platformBeginFileWrite(buffer);

		platformWriteFile(&fileHandle, fileMemoryToWrite.memory, fileMemoryToWrite.sizeOfMember*fileMemoryToWrite.count, 0);

		platformEndFile(fileHandle);

		printf("%s\n", "wrote file");

		if(outputImageFile) {

			char buffer1[512] = {};
			sprintf(buffer1, "%s%s_%d.png", folderName, name, loopCount);
			
			size_t bytesPerPixel = 4;
			size_t sizeToAlloc = bufferDim.x*bufferDim.y*bytesPerPixel;
			int stride_in_bytes = bytesPerPixel*bufferDim.x;
			
			u8 *pixelBuffer = (u8 *)calloc(sizeToAlloc, 1);
			
			renderReadPixels(atlasBuffer.bufferId, 0, 0,
			             bufferDim.x,
			             bufferDim.y,
			             GL_RGBA,
			             GL_UNSIGNED_BYTE,
			             pixelBuffer);

			
			int writeResult = stbi_write_png(buffer1, bufferDim.x, bufferDim.y, 4, pixelBuffer, stride_in_bytes);

			printf("%s\n", "wrote image");
			free(pixelBuffer);
			deleteFrameBuffer(&atlasBuffer);
		} 
	}

	releaseMemoryMark(&tempMark);
}

static inline void easyAtlas_loadTextureAtlas(char *fileName) {
	char buffer0[512] = {};
	sprintf(buffer0, "%s.txt", fileName);

	char buffer1[512] = {};
	sprintf(buffer1, "%s.png", fileName);
	
	Texture atlasTex = loadImage(buffer1);

	FileContents contentsText = getFileContentsNullTerminate(buffer0);
	
	EasyTokenizer tokenizer = lexBeginParsing((char *)contentsText.memory, true);
	bool parsing = true;

	char imageName[256] = {};
	Rect2f uvCoords = rect2f(0, 0, 0, 0);
	int imgWidth = 0;
	int imgHeight = 0;
	
	while(parsing) {
	    EasyToken token = lexGetNextToken(&tokenizer);
	    InfiniteAlloc data = {};
	    switch(token.type) {
	        case TOKEN_NULL_TERMINATOR: {
	            parsing = false;
	        } break;
	        case TOKEN_CLOSE_BRACKET: {
	        	// printf("%s\n", imageName);
	        	
	        	Texture *tex = (Texture *)calloc(sizeof(Texture), 1);

	        	tex->id = atlasTex.id;
	        	tex->width = imgWidth;
	        	tex->height = imgHeight;
	        	tex->uvCoords = uvCoords;

	        	Asset *result = addAssetTexture(imageName, tex);
	        	assert(result);
	        } break;
	        case TOKEN_WORD: {
	            if(stringsMatchNullN("name", token.at, token.size)) {
	                char *string = getStringFromDataObjects(&data, &tokenizer);
                    int strSize = strlen(string); 
                    assert(strSize < arrayCount(imageName));
                    nullTerminateBuffer(imageName, string, strSize);
	            }
	            if(stringsMatchNullN("width", token.at, token.size)) {
	                imgWidth = getIntFromDataObjects(&data, &tokenizer);
	            }
	            if(stringsMatchNullN("height", token.at, token.size)) {
	                imgHeight = getIntFromDataObjects(&data, &tokenizer);
	            }
	            if(stringsMatchNullN("uvCoords", token.at, token.size)) {
	                V4 uv = buildV4FromDataObjects(&data, &tokenizer);
	                //copy over to make a rect4 instead of a V4
	                uvCoords.E[0] = uv.E[0];
	                uvCoords.E[1] = uv.E[1];
	                uvCoords.E[2] = uv.E[2];
	                uvCoords.E[3] = uv.E[3];
	            }
	        } break;
	        default: {

	        }
	    }
	}
}	

static inline void easyAtlas_createTextureAtlas(char *folderName, char *ouputFolderName, SDL_Window *windowHandle, Arena *memoryArena) {
	char *imgFileTypes[] = {"jpg", "jpeg", "png", "bmp"};
	folderName = concat(globalExeBasePath, folderName);
	ouputFolderName = concat(globalExeBasePath, ouputFolderName);
	FileNameOfType fileNames = getDirectoryFilesOfType(folderName, imgFileTypes, arrayCount(imgFileTypes));

	InfiniteAlloc atlasElms = initInfinteAlloc(Easy_AtlasElm);

	printf("File Coutn: %d\n", fileNames.count);
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

	//sorting them by size so the bigger images get first preference
	easyAtlas_sortBySize(&atlasElms);
	stbi_flip_vertically_on_write(true);//flip bytes vertically

	easyAtlas_drawAtlas(ouputFolderName, memoryArena, &atlasElms, true, "textureAtlas");

	releaseInfiniteAlloc(&atlasElms);

	free(folderName);
	free(ouputFolderName);
}