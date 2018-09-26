typedef struct {
    GLuint vertices;
    GLuint indexes;
    int facesCount;
    RenderProgram *prog;
    VaoHandle vaoHandle;
} Mesh;

typedef enum {
    TOKEN_NULL,
    TOKEN_NOT_HANDLED,
    FORWARD_SLASH,
    NUMBER, 
    LETTER, 
} TokenType;

typedef struct {
    char *at;
    int length;        
    TokenType type;
} Token;

bool isNumeric(char *at) {
    bool result = false;
    if((*at >= '0' && *at <= '9') || *at == '.' || *at == '-') {
        result = true;
    }
    return result;
}

bool isLetter(char *at) {
    bool result = false;
    if((*at >= 'A' && *at <= 'Z') || (*at >= 'a' && *at <= 'z')) {
        result = true;
    }
    return result;
}

typedef struct {    
    int arraySize;
    Vertex *data;
} VertexInfo;

void resizeVertexInfo(VertexInfo *info, int count) {
    if(count >= info->arraySize) {
        int oldArraySize = info->arraySize;
        info->arraySize += 4000;
        void *memory = calloc(sizeof(Vertex)*(info->arraySize), 1);
        memcpy(memory, info->data, sizeof(Vertex)*oldArraySize);
        free(info->data);
        info->data = (Vertex *)memory;
    }
}


typedef enum {
    VERTEX_NULL,
    VERTEX_POS,
    VERTEX_NORMAL,
    VERTEX_TEX_UV,
    VERTEX_FACES,
} VertexInfoMode;

typedef struct {
    unsigned int vertexPos[3];
    unsigned int vertexNormal[3];
    unsigned int vertexUV[3];
} Face;

typedef struct {
    char *at;
    Token latestToken;
    bool parsing;
    int lineCount;
} Tokenizer;

Token *getNextToken(Tokenizer *tokenizer) {
    char *at = tokenizer->at;
    bool inComment = false;
    while(*at != '\0' && (*at == ' ' || *at == '\n' || *at == '#' || inComment)) {
        if(*at == '\n') {
            inComment = false;
            tokenizer->lineCount++;
        }
        if(*at == '#') {
            inComment = true;
        }
        at++;
    }
    
    Token token = {};
    if(*at == '/') {
        token.at = at;
        token.length = 1;
        token.type = FORWARD_SLASH;
        at += 1;
    } else if(isNumeric(at)) {
        token.at = at;
        token.type = NUMBER;
        while(isNumeric(at)) {
            at++;
        }
        token.length = (int)(at - token.at);
    } else if(isLetter(at)) {
        token.at = at;
        token.type = LETTER;
        while(isLetter(at) || isNumeric(at)) {
            at++;
        }
        token.length = (int)(at - token.at);
        
    } else if(*at == '\0') {
        token.at = at;
        token.length = 1;
        token.type = TOKEN_NULL;
        tokenizer->parsing = false;
    } else {
        token.at = at;
        token.length = 1;
        token.type = TOKEN_NOT_HANDLED;
        at++;
    }
    tokenizer->at = at;
    tokenizer->latestToken = token;
    
    
    Token *result = &tokenizer->latestToken;
    return result;
}

Token seeNextToken(Tokenizer *tokenizer) {
    Tokenizer state = *tokenizer;
    getNextToken(tokenizer);
    Token result = tokenizer->latestToken;
    *tokenizer = state;
    return result;
}

static inline void addLineData(Tokenizer *tokenizer, Token *token, VertexInfo *info, int *count, int indexBegin, bool isV3) {
    resizeVertexInfo(info, *count);
    Vertex *vertex = info->data + *count;
    *count += 1;
    
    char *a = nullTerminate(token->at, token->length);
    vertex->E[indexBegin + 0] = atof(a);
    free(a);
    
    token = getNextToken(tokenizer);
    assert(token && token->type == NUMBER);
    
    a = nullTerminate(token->at, token->length);
    vertex->E[indexBegin + 1] = atof(a);
    free(a);
    
    if(isV3) {
        token = getNextToken(tokenizer);
        assert(token && token->type == NUMBER);
        
        a = nullTerminate(token->at, token->length);
        vertex->E[indexBegin + 2] = atof(a);
        free(a);
    }
}
void addFaceData(Tokenizer *tokenizer, Token *token, Face *face, int index) {
    //TODO: use arenas here instead of freeing new strings. 
    assert(token->type == NUMBER);
    char *a = nullTerminate(token->at, token->length);
    face->vertexPos[index] = (atoi(a) - 1);
    free(a);
    
    //Face data comes as the format v/tv/nv or v or v//nv or v/tv
    //TODO(olllie): this is evidence that a new line should be a concept in the tokenizer, since new lines in this format mean something. ie. end of the face data?
    int forwardSlashCount = 0; 
    Token tokenNxt = seeNextToken(tokenizer);
    while(tokenNxt.type == FORWARD_SLASH){
        getNextToken(tokenizer); //move to next token after the slash
        forwardSlashCount++;
        
        token = getNextToken(tokenizer);
        if(token->type == FORWARD_SLASH) {
            forwardSlashCount++;
            token = getNextToken(tokenizer);
        } else {
            assert(token->type == NUMBER);
        }
        assert(token->type == NUMBER);
        a = nullTerminate(token->at, token->length);
        if(forwardSlashCount == 1) {
            face->vertexUV[index] = (atoi(a) - 1);
        } else if(forwardSlashCount == 2) {
            face->vertexNormal[index] = (atoi(a) - 1);
        } else {
            assert(!"invalid code path");
        }
        free(a);
        tokenNxt = seeNextToken(tokenizer);
    }
}

void addVertexData(VertexInfo *info, Face *face, Vertex *vertex, int index) {
    vertex->E[0] = info->data[face->vertexPos[index]].E[0];
    vertex->E[1] = info->data[face->vertexPos[index]].E[1];
    vertex->E[2] = info->data[face->vertexPos[index]].E[2];
    
    vertex->E[3] = info->data[face->vertexNormal[index]].E[3];
    vertex->E[4] = info->data[face->vertexNormal[index]].E[4];
    vertex->E[5] = info->data[face->vertexNormal[index]].E[5];
    
    vertex->E[6] = info->data[face->vertexUV[index]].E[6];
    vertex->E[7] = info->data[face->vertexUV[index]].E[7];
}

Mesh loadObjFile(char *fileName) { 
    Mesh result = {};
    
    FileContents fileContents = getFileContentsNullTerminate(fileName);
    unsigned char *at = fileContents.memory;
    
    VertexInfo info = {};
    info.arraySize = 4096;
    info.data = (Vertex *)calloc(sizeof(Vertex)*info.arraySize, 1);
    
    
    VertexInfo finalInfo = {};
    finalInfo.arraySize = 4096;
    finalInfo.data = (Vertex *)calloc(sizeof(Vertex)*finalInfo.arraySize, 1);
    int vertexCount = 0;
    
    int vertexPosCount = 0;
    int vertexNormalCount = 0;
    int vertexUVCount = 0;
    
    int faceArraySize = 4096;
    Face *faceData = (Face *)calloc(sizeof(Face)*faceArraySize, 1);
    int faceCount = 0;
    
    int indexDataAt = 0;
    int indexDataArraySize = 4096;
    unsigned int *indexData = (unsigned int *)calloc(sizeof(unsigned int)*indexDataArraySize, 1);
    
    VertexInfoMode mode = VERTEX_NULL;
    Tokenizer tokenizer = {};
    tokenizer.at = (char *)at;
    tokenizer.parsing = true;
    while(tokenizer.parsing) {
        
        Token *token = getNextToken(&tokenizer);
        
        switch(token->type) {
            case NUMBER: {
                if(mode == VERTEX_POS) {
                    addLineData(&tokenizer, token, &info, &vertexPosCount, 0, true);
                } else if(mode == VERTEX_NORMAL) {
                    addLineData(&tokenizer, token, &info, &vertexNormalCount, 3, true);
                } else if(mode == VERTEX_TEX_UV) {
                    addLineData(&tokenizer, token, &info, &vertexUVCount, 6, false);
                } else if(mode == VERTEX_FACES) {
                    assert(mode == VERTEX_FACES);
                    if(faceCount >= faceArraySize) {
                        //make bigger if neccessary
                        int oldArraySize = faceArraySize;
                        faceArraySize += 4000;
                        void *memory = calloc(sizeof(Face)*(faceArraySize), 1);
                        memcpy(memory, faceData, sizeof(Face)*oldArraySize);
                        free(faceData);
                        faceData = (Face *)memory;
                    }
                    Face *face = faceData + faceCount++;
                    
                    addFaceData(&tokenizer, token, face, 0);
                    token = getNextToken(&tokenizer);
                    addFaceData(&tokenizer, token, face, 1);
                    token = getNextToken(&tokenizer);
                    addFaceData(&tokenizer, token, face, 2);
                    
                    for(int i = 0; i < 3; ++i) {
                        resizeVertexInfo(&finalInfo, (vertexCount));
                        int vertexAt = vertexCount++;
                        Vertex *vertex = finalInfo.data + vertexAt;
                        addVertexData(&info, face, vertex, i);
                        
                        if(indexDataAt >= indexDataArraySize) {
                            int oldArraySize = indexDataArraySize;
                            indexDataArraySize += 4000;
                            void *memory = calloc(sizeof(unsigned int)*(indexDataArraySize), 1);
                            memcpy(memory, indexData, sizeof(unsigned int)*oldArraySize);
                            free(indexData);
                            indexData = (unsigned int *)memory;
                        }
                        indexData[indexDataAt++] = vertexAt;
                    }
                    
                } else if(mode == VERTEX_NULL) {
                    // printf("%s\n", "NO VERTEX MODE SELECTED");
                }
            } break;
            case LETTER: {
                if(stringsMatchN(token->at, token->length, (char *)"f", 1)) {
                    mode = VERTEX_FACES;
                } else if(stringsMatchN(token->at, token->length, (char *)"v", 1)) {
                    mode = VERTEX_POS; 
                } else if(stringsMatchN(token->at, token->length, (char *)"vn", 2)) {
                    mode = VERTEX_NORMAL;
                } else if(stringsMatchN(token->at, token->length, (char *)"vt", 2)) {
                    mode = VERTEX_TEX_UV;
                } else {
                    // printf("VERTEX MODE NOT SUPPORTED: %.*s\n", token->length, token->at);
                    mode = VERTEX_NULL;
                }
            } break;
            default: {
                
            }
        }
    }
    
    glGenVertexArrays(1, &result.vaoHandle.vaoHandle);
    renderCheckError();
    glBindVertexArray(result.vaoHandle.vaoHandle);
    renderCheckError();

    assert(vertexNormalCount == vertexPosCount);
    
    GLuint vertices;
    glGenBuffers(1, &vertices);
    renderCheckError();
    glBindBuffer(GL_ARRAY_BUFFER, vertices);
    renderCheckError();
    
    glBufferData(GL_ARRAY_BUFFER, vertexCount*sizeof(Vertex), finalInfo.data, GL_STATIC_DRAW);
    renderCheckError();
        
    GLuint indexes;
    glGenBuffers(1, &indexes);
    renderCheckError();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexes);
    renderCheckError();
    
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexCount*sizeof(unsigned int), indexData, GL_STATIC_DRAW);
    renderCheckError();
    
    glBindVertexArray(0);
    renderCheckError();
    
    // glDeleteBuffers(1, &vertices);
    // glDeleteBuffers(1, &indexes);
    
    result.facesCount = faceCount;
    return result;
}

void renderModel(Mesh *mesh,
                 Matrix4 modelMatrix, 
                 Matrix4 viewMatrix,
                 Matrix4 perspectiveMatrix, 
                 RenderProgram *program) {
    
    glUseProgram(program->glProgram);
    renderCheckError();

    assert(mesh->vaoHandle.vaoHandle);
    glBindVertexArray(mesh->vaoHandle.vaoHandle);
    renderCheckError();

    GLuint modelUniform = getUniformFromProgram(program, "model").handle; 
    assert(modelUniform);
    GLuint viewUniform = getUniformFromProgram(program, "view").handle; 
    assert(viewUniform);
    GLuint perspectiveUniform = getUniformFromProgram(program, "perspective").handle; 
    // assert(perspectiveUniform);

    glUniformMatrix4fv(modelUniform, 1, GL_FALSE, modelMatrix.val);
    renderCheckError();
    glUniformMatrix4fv(viewUniform, 1, GL_FALSE, viewMatrix.val);
    renderCheckError();
    glUniformMatrix4fv(perspectiveUniform, 1, GL_FALSE, perspectiveMatrix.val);
    renderCheckError();
    
    GLuint vertexAttrib = getAttribFromProgram(program, "vertex").handle; 
    // assert(vertexAttrib);
    // GLuint normalAttrib = getAttribFromProgram(program, "normal").handle; 
    // assert(normalAttrib);

    glEnableVertexAttribArray(vertexAttrib);  
    renderCheckError();
    glVertexAttribPointer(vertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), getOffsetForVertex(position)); 
    renderCheckError();
    
    // glEnableVertexAttribArray(normalAttrib);
    // renderCheckError();
    // glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), getOffsetForVertex(normal));
    // renderCheckError();
    
    glDrawElements(GL_TRIANGLES, mesh->facesCount*3, GL_UNSIGNED_INT, 0); //this is the number or verticies for the count. 
    renderCheckError();
        
    glBindVertexArray(0);
    renderCheckError();

    glUseProgram(0);
    renderCheckError();
}