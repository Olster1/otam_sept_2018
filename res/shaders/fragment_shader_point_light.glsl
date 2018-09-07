in mediump vec4 colorOut;
in mediump vec2 texUV_out;
uniform mediump float percentY;

out vec4 color;
void main (void) {
    float rel = 2*length(texUV_out - vec2(0.5f, 0.5f));
    float value = rel;
    
    //Outside a circle
    if(value > 1) discard;
    if(value < 0) discard;
    //

    float alpha = clamp(value, 0, 1);
    alpha = 1 - alpha;//smoothstep(1, 0, alpha); //1 - alpha; //1 - smoothstep(b, a, value); 
    
    
    vec4 finalColor = colorOut;
    
    //finalColor *= (alpha);
    finalColor = vec4(alpha, alpha, alpha, alpha);
    
    color = finalColor;
}

