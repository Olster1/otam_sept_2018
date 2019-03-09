static inline int compileFiles(char *folderName, char **fileTypes, int typeCount) {

	folderName = concat(globalExeBasePath, folderName);
	FileNameOfType fileNames = getDirectoryFilesOfType(folderName, fileTypes, typeCount);
	int result = fileNames.count;
	
	InfiniteAlloc data = initInfinteAlloc(char);
	for(int i = 0; i < fileNames.count; ++i) {
	    char *fullName = fileNames.names[i];
	    char *shortName = getFileLastPortion(fullName);
	    if(shortName[0] != '.') { //don't load hidden file 
			FileContents contents = getFileContents(fullName);        
			EasyAssert(contents.valid);
			char buffer[512] = {};
			char *name = getFileLastPortionWithoutExtension(shortName);
			sprintf(buffer, "static char *%s_shader = \"", name);
			free(name);

			addElementInifinteAllocWithCount_(&data, buffer, strlen(buffer));

			//assuming it's not utf8
			int charCount = contents.fileSize/sizeof(char);

			for(int charIndex = 0; charIndex < charCount; ++charIndex) {
				char *at = (char *)contents.memory;
				char *toAdd = at + charIndex;
				if(!(*toAdd == '\n' || *toAdd == '\r')) 
				{
					addElementInifinteAllocWithCount_(&data, toAdd, 1);		
				} else {
					toAdd = "\\n\"\n\"";
					addElementInifinteAllocWithCount_(&data, toAdd, strlen(toAdd));		
				}
				
			}

			sprintf(buffer, "\";\n\n");

			addElementInifinteAllocWithCount_(&data, buffer, strlen(buffer));

	    }
	    free(fullName);
	    free(shortName);
	}
	char writeName[512] = {};
	sprintf(writeName, "%s../../shared/easy_shaders.h", folderName);
	// printf("%s\n", writeName);
	// printf("%.*s\n", data.count, (char *)data.memory);
	game_file_handle handle = platformBeginFileWrite(writeName);
	platformWriteFile(&handle, data.memory, data.count*data.sizeOfMember, 0);
	platformEndFile(handle);
	free(folderName);
	return result;
}