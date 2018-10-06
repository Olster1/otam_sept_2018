#include <stdio.h>

//TODO: change to using float arrays so we don't need the maths.h dependency!
#define isNanErrorf(value) isNanErrorf_(value, __LINE__, (char *)__FILE__)
void isNanErrorf_(float value, int line, char *fileName) {
    if(isNanf(value)) {
        printf("NAN value at line number %d in file %s\n", line, fileName);
        assert(!"is nan");
    }
}
#define isNanErrorV2(value) isNanErrorV2_(value, __LINE__, (char *)__FILE__)
void isNanErrorV2_(V2 value, int line, char *fileName) {
    if(isNanV2(value)) {
        printf("NAN value at line number %d in file %s\n", line, fileName);
        assert(!"is nan");
    }
}

#define isNanErrorV3(value) isNanErrorV3_(value, __LINE__, (char *)__FILE__)
void isNanErrorV3_(V3 value, int line, char *fileName) {
    if(isNanV3(value)) {
        printf("NAN value at line number %d in file %s\n", line, fileName);
        assert(!"is nan");
    }
}

void error_printFloat2(char *string, float *values) {
	printf("%s x:%f, y:%f\n", string, values[0], values[1]);
}

void error_printFloat3(char *string, float *values) {
	printf("%s x:%f, y:%f, z:%f\n", string, values[0], values[1], values[2]);
}

void error_printFloat4(char *string, float *values) {
    printf("%s x:%f, y:%f, z:%f, w:%f\n", string, values[0], values[1], values[2], values[3]);
}