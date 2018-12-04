in mediump vec4 colorOut;
in mediump vec2 texUV_out;

uniform mediump float percentY;

out mediump vec4 color;
void main (void) {
    float tu = texUV_out.x;
    vec4  finalColor = colorOut;
    finalColor*=finalColor.w; 
    color = finalColor;
}