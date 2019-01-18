in vec3 vertex;
in vec2 texUV;

in mat4 PVM;
in vec4 color;

out vec4 colorOut; //out going
out vec2 texUV_out;

void main() {
    gl_Position = PVM * vec4(vertex, 1);
    colorOut = color;
    texUV_out = texUV;
}