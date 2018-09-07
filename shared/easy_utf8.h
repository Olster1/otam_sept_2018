// The leading bytes and the continuation bytes do not share values 
// (continuation bytes start with 10 while single bytes start with 0 and longer lead bytes start with 11)

bool easyUnicode_isContinuationByte(unsigned char byte) { //10
	unsigned char continuationMarker = (1 << 1);
	bool result = (byte >> 6) == continuationMarker;

	return result;
}

bool easyUnicode_isSingleByte(unsigned char byte) { //top bit 0
	unsigned char marker = 0;
	bool result = (byte >> 7) == marker;

	return result;	
}

bool easyUnicode_isLeadingByte(unsigned char byte) { //top bits 11
	unsigned char marker = (1 << 1 | 1 << 0);
	bool result = (byte >> 6) == marker;

	return result;	
}


int easyUnicode_unicodeLength(unsigned char byte) {
	unsigned char bytes2 = (1 << 3 | 1 << 2);
	unsigned char bytes3 = (1 << 3 | 1 << 2 | 1 << 1);
	unsigned char bytes4 = (1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);

	int result = 1;
	unsigned char shiftedByte = byte >> 4;
	if(!easyUnicode_isContinuationByte(byte) && !easyUnicode_isSingleByte(byte)) {
		assert(easyUnicode_isLeadingByte(byte));
		if(shiftedByte == bytes2) { result = 2; }
		if(shiftedByte == bytes3) { result = 3; }
		if(shiftedByte == bytes4) { result = 4; }
		if(result == 1) assert(!"invalid path");
	} 

	return result;

}

//NOTE: this advances your pointer
unsigned int easyUnicode_utf8ToUtf32(unsigned char **streamPtr) {
	unsigned char *stream = *streamPtr;
	unsigned int result = 0;
	unsigned int sixBitsFull = (1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	unsigned int fiveBitsFull = (1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	unsigned int fourBitsFull = (1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);

	if(easyUnicode_isContinuationByte(stream[0])) { assert(!"shouldn't be a continuation byte. Have you advanced pointer correctly?"); }
	int unicodeLen = easyUnicode_unicodeLength(stream[0]);
	if(unicodeLen > 1) {
		assert(easyUnicode_isLeadingByte(stream[0]));
		//needs to be decoded
		switch(unicodeLen) {
			case 2: {
				// printf("%s\n", "two byte unicode");
				unsigned int firstByte = stream[0];
				unsigned int secondByte = stream[1];
				assert(easyUnicode_isContinuationByte(secondByte));
				result |= (secondByte & sixBitsFull);
				result |= ((firstByte & sixBitsFull) << 6);

				(*streamPtr) += 2;
			} break;
			case 3: {
				// printf("%s\n", "three byte unicode");
				unsigned int firstByte = stream[0];
				unsigned int secondByte = stream[1];
				unsigned int thirdByte = stream[2];
				assert(easyUnicode_isContinuationByte(secondByte));
				assert(easyUnicode_isContinuationByte(thirdByte));
				result |= (thirdByte & sixBitsFull);
				result |= ((secondByte & sixBitsFull) << 6);
				result |= ((firstByte & fiveBitsFull) << 12);

				(*streamPtr) += 3;
			} break;
			case 4: {
				// printf("%s\n", "four byte unicode");
				unsigned int firstByte = stream[0];
				unsigned int secondByte = stream[1];
				unsigned int thirdByte = stream[2];
				unsigned int fourthByte = stream[3];
				assert(easyUnicode_isContinuationByte(secondByte));
				assert(easyUnicode_isContinuationByte(thirdByte));
				assert(easyUnicode_isContinuationByte(fourthByte));
				result |= (thirdByte & sixBitsFull);
				result |= ((secondByte & sixBitsFull) << 6);
				result |= ((firstByte & sixBitsFull) << 12);
				result |= ((firstByte & fourBitsFull) << 18);


				(*streamPtr) += 4;
			} break;
			default: {
				assert(!"invalid path");
			}
		}
	} else {
		// printf("%s\n", "single byte unicode");
		result = stream[0];
		(*streamPtr) += 1;
	}

	return result;
}

//string must be null terminated. 
//TODO: this would be a good place for simd. 
unsigned int *easyUnicode_utf8StreamToUtf32Stream(unsigned char *stream) {
	int size = strlen((char *)stream) + 1; //for null terminator
	// printf("%d\n", size);
	unsigned int *result = (unsigned int *)calloc(size*sizeof(unsigned int), 1);
	unsigned int *at = result;
	while(*stream) {
		unsigned char *a = stream;
		*at = easyUnicode_utf8ToUtf32(&stream);
		assert(stream != a);
		at++;
	}
	// printf("the end: %d\n", size);
	result[size - 1] = '\0';
	return result;
}