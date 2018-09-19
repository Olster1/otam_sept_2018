uniform sampler2D tex;
in mediump vec4 colorOut;
in mediump vec2 texUV_out;
// uniform float fboWidth;
// uniform float fboHeight;
// uniform vec2 globalLightDirection;


out vec4 color;
void main (void) {
    vec4 texColor = texture(tex, texUV_out);
    vec4 extraColor = vec4(1, 1, 1, 1);
    if(texColor.w > 0) {

        float fboWidth = 980;
        float fboHeight = 540;

        // vec2 lightPos = vec2(0.5*fboWidth, 0.9*fboHeight);
        vec2 globalLightDirection = normalize(vec2(-2.0f,  -4.0f)); //get rid of this normalize 
        float pixelX = texUV_out.x * fboWidth;
        float pixelY = texUV_out.y * fboHeight;
        vec2 pixelPos = vec2(pixelX, pixelY); 

        // float len = length(lightPos - pixelPos);
        float alpha = 1;
        // if(len != 0) {
            vec2 directionToLight = globalLightDirection;//vec2((lightPos.x - pixelPos.x) / len, (lightPos.y - pixelPos.y) / len);
            int count = 0;
            int maxCount = 32;
            float brightnessValue = 0;
            for(int i = 0; i < maxCount; i++) {
                pixelPos += directionToLight;       
                //vec2 pixelPosUV = vec2(pixelPos.x / fboWidth, pixelPos.y / fboHeight);
                vec4 texelColor = texelFetch(tex, ivec2(pixelPos), 0);
                // float brightness = length(texelColor);
                // brightnessValue += brightness;
                if(texelColor.w > 0.0f) {
                    count++;
                } else {
                    // break;
                }
            }

            float maxCountF = maxCount;
            float countF = count;
            float val = countF / maxCountF;
            // float val = brightnessValue;

            alpha = clamp(val, 0, 1);
            alpha = alpha*alpha; //expoinental 
            alpha = mix(0.4, 1.0, alpha);
            // float factor = 1.3;
            // extraColor.x = mix(1, factor, alpha);
            // extraColor.y = mix(1, factor, alpha);
            //extraColor.z = mix(1, 1, alpha);

        // }
        vec4 finalColor = vec4(colorOut.x*texColor.x, extraColor.y*colorOut.y*texColor.y, extraColor.z*colorOut.z*texColor.z, colorOut.w*texColor.w);
        float lastAlpha = finalColor.w;
        finalColor *= (alpha);
        finalColor.w = lastAlpha;

        
        color = finalColor;//vec4(0, 0, 0, alpha);//
    } else {
        discard;
    }
}

