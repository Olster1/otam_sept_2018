in mediump vec4 colorOut;
in mediump vec2 texUV_out;
uniform mediump float lenRatio;

out vec4 color;
void main (void) {
    vec2 tex = texUV_out;
    float c = lenRatio; 
    color = colorOut;
}

