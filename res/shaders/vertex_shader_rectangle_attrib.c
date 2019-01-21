in mat4 PVM;
in vec4 color;
in vec4 uvAtlas;

in vec3 vertex;
in vec2 texUV;

out vec4 colorOut; //out going
out vec2 texUV_out;

void main() {
    gl_Position = PVM * vec4(vertex, 1);
    vec4 uv = uvAtlas;
	uv.x = 0;
    colorOut = color;
    texUV_out = texUV;
}