int loadAndAddImagesToAssets(char *folderName) {
	char *imgFileTypes[] = {"jpg", "jpeg", "png", "bmp"};
	FileNameOfType fileNames = getDirectoryFilesOfType(concat(globalExeBasePath, folderName), imgFileTypes, arrayCount(imgFileTypes));
	int result = fileNames.count;
	
	for(int i = 0; i < fileNames.count; ++i) {
	    char *fullName = fileNames.names[i];
	    char *shortName = getFileLastPortion(fullName);
	    if(shortName[0] != '.') { //don't load hidden file 
	        Asset *asset = findAsset(shortName);
	        assert(!asset);
	        if(!asset) {
	            asset = loadImageAsset(fullName);
	        }
	        asset = findAsset(shortName);
	        assert(shortName);
	    }
	    free(fullName);
	    free(shortName);
	}
	return result;
}

int loadAndAddSoundsToAssets(char *folderName, SDL_AudioSpec *audioSpec) {
	char *soundFileTypes[] = {"wav"};
	FileNameOfType soundFileNames = getDirectoryFilesOfType(concat(globalExeBasePath, folderName), soundFileTypes, arrayCount(soundFileTypes));
	int result = soundFileNames.count;
	for(int i = 0; i < soundFileNames.count; ++i) {
	    char *fullName = soundFileNames.names[i];
	    char *shortName = getFileLastPortion(fullName);
	    if(shortName[0] != '.') { //don't load hidden file 
	        Asset *asset = findAsset(shortName);
	        assert(!asset);
	        if(!asset) {
	            asset = loadSoundAsset(fullName, audioSpec);
	        }
	        asset = findAsset(shortName);
	        assert(shortName);
	    }
	    free(fullName);
	    free(shortName);
	}
	return result;
}