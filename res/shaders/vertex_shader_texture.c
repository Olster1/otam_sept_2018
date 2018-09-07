uniform samplerBuffer PVMArray;
uniform samplerBuffer ColorArray;
// uniform vec4 color;

in vec3 vertex;
in vec2 texUV;
//in int instanceIndex;

out vec4 colorOut; //out going
out vec2 texUV_out;
out float zAt;

void main() {
	
	int offset = 4 * int(gl_InstanceID);
	vec4 a = texelFetch(PVMArray, offset + 0);
	vec4 b = texelFetch(PVMArray, offset + 1);
	vec4 c = texelFetch(PVMArray, offset + 2);
	vec4 d = texelFetch(PVMArray, offset + 3);
	
	mat4 PVM = mat4(a, b, c, d);
    gl_Position = PVM * vec4(vertex, 1);
    colorOut = texelFetch(ColorArray, gl_InstanceID);
    texUV_out = texUV;
    zAt = gl_Position.z;
}