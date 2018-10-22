static char *frag_model_shader = "// uniform sampler2D tex;\n"
"// in vec4 colorOut;\n"
"// in vec2 texUV_out;\n"
"// in float zAt;\n"
"\n"
"out vec4 color;\n"
"void main (void) {\n"
"	// vec4 texColor = texture(tex, texUV_out);\n"
"	// float alpha = 1.0;//smoothstep(1, -0.4, zAt); \n"
"	// float a = zAt;\n"
"	// alpha *= texColor.w;\n"
"	// texColor *= alpha;\n"
"    // color = colorOut*texColor;\n"
"    color = vec4(1, 0, 0, 1);\n"
"\n"
"}";

static char *frag_shader_line_shader = "in mediump vec4 colorOut;\n"
"in mediump vec2 texUV_out;\n"
"in mediump float lengthRatio;\n"
"in mediump float percentY;\n"
"\n"
"out mediump vec4 color;\n"
"void main (void) {\n"
"    float a = lengthRatio; \n"
"    \n"
"    vec4  finalColor = colorOut;\n"
"    float alpha = 1.0f;\n"
"    \n"
"    if(texUV_out.y < percentY) {\n"
"        alpha = smoothstep(0, percentY, texUV_out.y);\n"
"        \n"
"    } else if(texUV_out.y > (1 - percentY)) {\n"
"        alpha = 1 - smoothstep((1 - percentY), 1, texUV_out.y);\n"
"    }\n"
"    \n"
"    alpha = 1;\n"
"    alpha = clamp(alpha, 0, 1);\n"
"    finalColor *= alpha;\n"
"    if(alpha <= 0) {\n"
"        discard;\n"
"    }\n"
"    \n"
"#if 0 //see depth buffer\n"
"    color = vec4(vec3(gl_FragCoord.z), 1);\n"
"#else \n"
"    color = finalColor;\n"
"#endif\n"
"}\n"
"\n"
"";

static char *frag_shader_ring_shader = "in mediump vec4 colorOut;\n"
"in mediump vec2 texUV_out;\n"
"uniform mediump float percentY;\n"
"\n"
"out mediump vec4 color;\n"
"void main (void) {\n"
"    float rel = 2*length(texUV_out - vec2(0.5f, 0.5f));\n"
"    float value = rel;\n"
"    \n"
"    //Outside a circle\n"
"    if(value > 1) discard;\n"
"    if(value < 0) discard;\n"
"    //\n"
"\n"
"    float alpha = clamp(value, 0, 1);\n"
"\n"
"    alpha = alpha;//smoothstep(1, 0, alpha); //1 - alpha; //1 - smoothstep(b, a, value); \n"
"    \n"
"    vec4 finalColor = colorOut;\n"
"    \n"
"    finalColor *= (alpha);\n"
"    \n"
"    color = finalColor;\n"
"}\n"
"\n"
"";

static char *frag_shader_shadow_shader = "uniform sampler2D tex;\n"
"in mediump vec4 colorOut;\n"
"in mediump vec2 texUV_out;\n"
"// uniform float fboWidth;\n"
"// uniform float fboHeight;\n"
"// uniform vec2 globalLightDirection;\n"
"\n"
"\n"
"out vec4 color;\n"
"void main (void) {\n"
"    vec4 texColor = texture(tex, texUV_out);\n"
"    vec4 extraColor = vec4(1, 1, 1, 1);\n"
"    if(texColor.w > 0) {\n"
"\n"
"        float fboWidth = 980;\n"
"        float fboHeight = 540;\n"
"\n"
"        // vec2 lightPos = vec2(0.5*fboWidth, 0.9*fboHeight);\n"
"        vec2 globalLightDirection = normalize(vec2(-2.0f,  -4.0f)); //get rid of this normalize \n"
"        float pixelX = texUV_out.x * fboWidth;\n"
"        float pixelY = texUV_out.y * fboHeight;\n"
"        vec2 pixelPos = vec2(pixelX, pixelY); \n"
"\n"
"        // float len = length(lightPos - pixelPos);\n"
"        float alpha = 1;\n"
"        // if(len != 0) {\n"
"            vec2 directionToLight = globalLightDirection;//vec2((lightPos.x - pixelPos.x) / len, (lightPos.y - pixelPos.y) / len);\n"
"            int count = 0;\n"
"            int maxCount = 32;\n"
"            float brightnessValue = 0;\n"
"            for(int i = 0; i < maxCount; i++) {\n"
"                pixelPos += directionToLight;       \n"
"                //vec2 pixelPosUV = vec2(pixelPos.x / fboWidth, pixelPos.y / fboHeight);\n"
"                vec4 texelColor = texelFetch(tex, ivec2(pixelPos), 0);\n"
"                // float brightness = length(texelColor);\n"
"                // brightnessValue += brightness;\n"
"                if(texelColor.w > 0.0f) {\n"
"                    count++;\n"
"                } else {\n"
"                    // break;\n"
"                }\n"
"            }\n"
"\n"
"            float maxCountF = maxCount;\n"
"            float countF = count;\n"
"            float val = countF / maxCountF;\n"
"            // float val = brightnessValue;\n"
"\n"
"            alpha = clamp(val, 0, 1);\n"
"            alpha = alpha*alpha; //expoinental \n"
"            alpha = mix(0.4, 1.0, alpha);\n"
"            // float factor = 1.3;\n"
"            // extraColor.x = mix(1, factor, alpha);\n"
"            // extraColor.y = mix(1, factor, alpha);\n"
"            //extraColor.z = mix(1, 1, alpha);\n"
"\n"
"        // }\n"
"        vec4 finalColor = vec4(colorOut.x*texColor.x, extraColor.y*colorOut.y*texColor.y, extraColor.z*colorOut.z*texColor.z, colorOut.w*texColor.w);\n"
"        float lastAlpha = finalColor.w;\n"
"        finalColor *= (alpha);\n"
"        finalColor.w = lastAlpha;\n"
"\n"
"        \n"
"        color = finalColor;//vec4(0, 0, 0, alpha);//\n"
"    } else {\n"
"        discard;\n"
"    }\n"
"}\n"
"\n"
"";

static char *fragment_shader_blur_shader = "in mediump vec4 colorOut;\n"
"in mediump vec2 texUV_out;\n"
"\n"
"uniform sampler2D tex;\n"
"// uniform float resolution;\n"
"// uniform float radius;\n"
"// uniform vec2 dir;\n"
"out mediump vec4 color;\n"
"\n"
"void main() {\n"
"\n"
"	vec4 sum = vec4(0.0f);\n"
"	vec2 uv = texUV_out;\n"
"	uv.x = 0;\n"
"\n"
"\n"
"	//discard alpha for our simple demo, multiply by vertex color and return\n"
"	color = colorOut * sum;//vec4(sum.rgb, 1.0);\n"
"}";

static char *fragment_shader_circle_shader = "in mediump vec4 colorOut;\n"
"in mediump vec2 texUV_out;\n"
"uniform mediump float percentY;\n"
"\n"
"out vec4 color;\n"
"void main (void) {\n"
"    float rel = 2*length(texUV_out - vec2(0.5f, 0.5f));\n"
"    float value = rel;//1.0f - rel;\n"
"    \n"
"    \n"
"    //MMM... i don't know \n"
"    float a = 0.5f;\n"
"    float b = a - percentY;\n"
"    \n"
"    //has to start at 0.5f \n"
"    float alpha = 1 - smoothstep(b, a, value); \n"
"    \n"
"    vec4  finalColor = colorOut;\n"
"    alpha = clamp(alpha, 0, 1);\n"
"    finalColor *= (alpha);\n"
"    color = finalColor;\n"
"}\n"
"\n"
"";

static char *fragment_shader_point_light_shader = "in mediump vec4 colorOut;\n"
"in mediump vec2 texUV_out;\n"
"uniform mediump float percentY;\n"
"\n"
"out vec4 color;\n"
"void main (void) {\n"
"    float rel = 2*length(texUV_out - vec2(0.5f, 0.5f));\n"
"    float value = rel;\n"
"    \n"
"    //Outside a circle\n"
"    if(value > 1) discard;\n"
"    if(value < 0) discard;\n"
"    //\n"
"\n"
"    float alpha = clamp(value, 0, 1);\n"
"    alpha = 1 - alpha;//smoothstep(1, 0, alpha); //1 - alpha; //1 - smoothstep(b, a, value); \n"
"    \n"
"    \n"
"    vec4 finalColor = colorOut;\n"
"    \n"
"    //finalColor *= (alpha);\n"
"    finalColor = vec4(alpha, alpha, alpha, alpha);\n"
"    \n"
"    color = finalColor;\n"
"}\n"
"\n"
"";

static char *fragment_shader_rectangle_shader = "in mediump vec4 colorOut;\n"
"in mediump vec2 texUV_out;\n"
"\n"
"uniform mediump float percentY;\n"
"\n"
"out mediump vec4 color;\n"
"void main (void) {\n"
"    \n"
"    vec4  finalColor = colorOut;\n"
"    float alpha = 1.0f;\n"
"    \n"
"    if(texUV_out.y < percentY) {\n"
"        alpha *= smoothstep(0, percentY, texUV_out.y);\n"
"        \n"
"    } else if(texUV_out.y > (1 - percentY)) {\n"
"        alpha *= 1 - smoothstep((1 - percentY), 1, texUV_out.y);\n"
"    }\n"
"    \n"
"    alpha = clamp(alpha, 0, 1);\n"
"    finalColor *= alpha;\n"
"    color = finalColor;\n"
"}";

static char *fragment_shader_rectangle_noGrad_shader = "in mediump vec4 colorOut;\n"
"in mediump vec2 texUV_out;\n"
"uniform mediump float lenRatio;\n"
"\n"
"out vec4 color;\n"
"void main (void) {\n"
"    vec2 tex = texUV_out;\n"
"    float c = lenRatio; \n"
"    color = colorOut;\n"
"}\n"
"\n"
"";

static char *fragment_shader_texture_shader = "uniform sampler2D tex;\n"
"in vec4 colorOut;\n"
"in vec2 texUV_out;\n"
"in float zAt;\n"
"\n"
"out vec4 color;\n"
"void main (void) {\n"
"	vec4 texColor = texture(tex, texUV_out);\n"
"	float alpha = texColor.w;\n"
"	float a = zAt;\n"
"	vec4 b = colorOut*colorOut.w;\n"
"	vec4 c = b*texColor;\n"
"	c *= alpha;\n"
"    color = c;\n"
"}";

static char *fragment_shader_texture_filter_shader = "uniform sampler2D tex;\n"
"in mediump vec4 colorOut;\n"
"in mediump vec2 texUV_out;\n"
"in mediump float zAt;\n"
"\n"
"out vec4 color;\n"
"void main (void) {\n"
"	vec4 texColor = texture(tex, texUV_out);\n"
"	float alpha = 1;//smoothstep(1, -0.4, zAt); \n"
"	float a = zAt;\n"
"	alpha *= texColor.w;\n"
"	texColor *= alpha;\n"
"    color = colorOut*texColor;\n"
"}";

static char *vertex_model_shader = "uniform mat4 perspective;\n"
"uniform mat4 view;\n"
"uniform mat4 model;\n"
"\n"
"// uniform vec4 color;\n"
"\n"
"in vec3 vertex;\n"
"// in vec2 texUV;\n"
"// in vec3 normal;\n"
"//in int instanceIndex;\n"
"\n"
"// out vec4 colorOut; //out going\n"
"// out vec2 texUV_out;\n"
"\n"
"void main() {\n"
"    gl_Position = perspective * view * model * vec4(vertex, 1);\n"
"    // colorOut = texelFetch(ColorArray, gl_InstanceID);\n"
"    // texUV_out = texUV;\n"
"}";

static char *vertex_shader_line_shader = "uniform mat4 PVM;\n"
"\n"
"in mediump vec3 vertex;\n"
"in mediump vec2 texUV;\n"
"in mediump vec4 color;\n"
"in mediump float lengthRatioIn;\n"
"in mediump float percentY_;\n"
"\n"
"out mediump vec4 colorOut; //out going\n"
"out mediump vec2 texUV_out;\n"
"out mediump float lengthRatio;\n"
"out mediump float percentY;\n"
"\n"
"void main() {\n"
"    gl_Position = PVM * vec4(vertex, 1);\n"
"    colorOut = color;\n"
"    texUV_out = texUV;\n"
"    lengthRatio = lengthRatioIn;\n"
"    percentY = percentY_;\n"
"}\n"
"\n"
"";

static char *vertex_shader_rectangle_shader = "uniform samplerBuffer PVMArray;\n"
"uniform samplerBuffer ColorArray;\n"
"\n"
"uniform vec4 color;\n"
"\n"
"in vec3 vertex;\n"
"in vec2 texUV;\n"
"//in int instanceIndex;\n"
"\n"
"out vec4 colorOut; //out going\n"
"out vec2 texUV_out;\n"
"\n"
"void main() {\n"
"	mat4 PVM = mat4(1.0f);\n"
"	int offset = 4 * int(gl_InstanceID);\n"
"	for(int i = 0; i < 4; ++i) {\n"
"		PVM[i] = texelFetch(PVMArray, offset + i);\n"
"	}\n"
"    gl_Position = PVM * vec4(vertex, 1);\n"
"    colorOut = texelFetch(ColorArray, gl_InstanceID);\n"
"    texUV_out = texUV;\n"
"}";

static char *vertex_shader_texture_shader = "uniform samplerBuffer PVMArray;\n"
"uniform samplerBuffer ColorArray;\n"
"uniform samplerBuffer UVArray;\n"
"// uniform vec4 color;\n"
"\n"
"in vec3 vertex;\n"
"in vec2 texUV;\n"
"//in int instanceIndex;\n"
"\n"
"out vec4 colorOut; //out going\n"
"out vec2 texUV_out;\n"
"out float zAt;\n"
"\n"
"void main() {\n"
"	\n"
"	int offset = 4 * int(gl_InstanceID);\n"
"	vec4 a = texelFetch(PVMArray, offset + 0);\n"
"	vec4 b = texelFetch(PVMArray, offset + 1);\n"
"	vec4 c = texelFetch(PVMArray, offset + 2);\n"
"	vec4 d = texelFetch(PVMArray, offset + 3);\n"
"	\n"
"	mat4 PVM = mat4(a, b, c, d);\n"
"    gl_Position = PVM * vec4(vertex, 1);\n"
"    colorOut = texelFetch(ColorArray, gl_InstanceID);\n"
"\n"
"    vec4 uvQuad = texelFetch(UVArray, gl_InstanceID);\n"
"\n"
"    int xAt = int(texUV.x*2);\n"
"    int yAt = int(texUV.y*2) + 1;\n"
"    texUV_out = vec2(uvQuad[xAt], uvQuad[yAt]);\n"
"\n"
"    zAt = gl_Position.z;\n"
"}";

