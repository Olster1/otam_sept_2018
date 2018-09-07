uniform mat4 PVM;

in mediump vec3 vertex;
in mediump vec2 texUV;
in mediump vec4 color;
in mediump float lengthRatioIn;
in mediump float percentY_;

out mediump vec4 colorOut; //out going
out mediump vec2 texUV_out;
out mediump float lengthRatio;
out mediump float percentY;

void main() {
    gl_Position = PVM * vec4(vertex, 1);
    colorOut = color;
    texUV_out = texUV;
    lengthRatio = lengthRatioIn;
    percentY = percentY_;
}

