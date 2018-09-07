uniform samplerBuffer PVMArray;
uniform samplerBuffer ColorArray;

uniform vec4 color;

in vec3 vertex;
in vec2 texUV;
//in int instanceIndex;

out vec4 colorOut; //out going
out vec2 texUV_out;

void main() {
	mat4 PVM = mat4(1.0f);
	int offset = 4 * int(gl_InstanceID);
	for(int i = 0; i < 4; ++i) {
		PVM[i] = texelFetch(PVMArray, offset + i);
	}
    gl_Position = PVM * vec4(vertex, 1);
    colorOut = texelFetch(ColorArray, gl_InstanceID);
    texUV_out = texUV;
}