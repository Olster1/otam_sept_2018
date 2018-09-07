in mediump vec4 colorOut;
in mediump vec2 texUV_out;
in mediump float lengthRatio;
in mediump float percentY;

out mediump vec4 color;
void main (void) {
    float a = lengthRatio; 
    
    vec4  finalColor = colorOut;
    float alpha = 1.0f;
    
    if(texUV_out.y < percentY) {
        alpha = smoothstep(0, percentY, texUV_out.y);
        
    } else if(texUV_out.y > (1 - percentY)) {
        alpha = 1 - smoothstep((1 - percentY), 1, texUV_out.y);
    }
    
    alpha = 1;
    alpha = clamp(alpha, 0, 1);
    finalColor *= alpha;
    if(alpha <= 0) {
        discard;
    }
    
#if 0 //see depth buffer
    color = vec4(vec3(gl_FragCoord.z), 1);
#else 
    color = finalColor;
#endif
}

