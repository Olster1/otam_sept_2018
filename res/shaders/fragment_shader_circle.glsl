in mediump vec4 colorOut;
in mediump vec2 texUV_out;
uniform mediump float percentY;

out vec4 color;
void main (void) {
    float rel = 2*length(texUV_out - vec2(0.5f, 0.5f));
    float value = rel;//1.0f - rel;
    
    
    //MMM... i don't know 
    float a = 0.5f;
    float b = a - percentY;
    
    //has to start at 0.5f 
    float alpha = 1 - smoothstep(b, a, value); 
    
    vec4  finalColor = colorOut;
    alpha = clamp(alpha, 0, 1);
    finalColor *= (alpha);
    color = finalColor;
}

